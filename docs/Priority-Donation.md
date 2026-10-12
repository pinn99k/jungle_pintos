# Priority Scheduling + Donation 팀 브리핑

## 1. 동기화가 왜 필요한가

PintOS는 멀티스레드 OS다. CPU는 하나인데 스레드는 여러 개. timer interrupt가 주기적으로 스레드를 교체한다.

문제는 **공유 자원**이다. 전역 변수 `count = 0`을 스레드 A, B가 둘 다 `count++` 하면:

```
count++는 CPU 입장에서 3단계:
① count를 레지스터로 읽기 (read: 0)
② 레지스터에서 +1        (compute: 1)
③ 레지스터 값을 count에 쓰기 (write: count = 1)

A가 ①번 했는데 timer interrupt → B로 교체 → B가 ①②③ 전부 실행 → count = 1
→ 다시 A → A 레지스터엔 아직 0 → +1 → count = 1

두 스레드가 각각 +1 했는데 결과가 1. 2가 되어야 하는데.
```

이게 **race condition**이다. A가 count를 건드리는 동안 B가 못 건드리게 막아야 한다. 그게 lock이고, lock의 기반이 semaphore다.

---

## 2. 동기화 도구의 구조

synch.c는 3층짜리 건물이다. 아래층이 위층의 재료가 된다.

```
3층  Condition Variable  — "조건이 맞을 때만 깨워줘"
2층  Lock                — "한 명만 들어와" + 소유자 기록
1층  Semaphore           — "허가증 N개, 없으면 대기"
```

---

## 3. 세마포어 — 핵심 메커니즘

### 세마포어란

```c
struct semaphore {
    unsigned value;         // 허가증 개수
    struct list waiters;    // 이 세마포어를 기다리는 스레드 대기줄
};
```

세마포어는 **메모리에 고정된 객체**다. 아무도 "들고 다니지" 않는다. 벽에 박혀 있는 신호등과 같다. 스레드가 찾아가서 value를 확인하고, 바꾸는 것이다.

### 세마포어는 어디서 만들어지나

자동으로 생기지 않는다. 코드에서 직접 만든다:

```c
struct lock file_lock;           // 전역 변수. 프로그램 시작~끝까지 같은 메모리 주소.
lock_init(&file_lock);           // 내부에서 sema_init(&lock->semaphore, 1) 호출. 딱 한 번.
```

이후 모든 스레드가 **같은 `&file_lock` 주소**로 찾아가서 acquire/release 한다. lock이 이동하는 게 아니라, 주소를 알려주는 것이다.

### sema_down (P 연산) — "허가증 가져감, 없으면 잠듦"

```c
while (sema->value == 0) {
    list_push_back(&sema->waiters, &thread_current()->elem);
    thread_block();      // ← 여기서 코드 실행 정지. CPU 내놓음.
}
sema->value--;           // 허가증 하나 가져감
```

- value > 0이면: while 안 들어감, value-- 하고 계속 실행
- value == 0이면: waiters에 자기를 넣고 `thread_block()`으로 멈춤
- 깨어나면 while 조건 재확인부터 재개 (바로 value--로 가는 게 아님)
- while인 이유: 깨어났는데 다른 스레드가 먼저 value를 가져갔을 수 있으므로

### sema_up (V 연산) — "허가증 반납, 대기자 있으면 깨움"

```c
if (!list_empty(&sema->waiters))
    thread_unblock(list_entry(list_pop_front(&sema->waiters),
                              struct thread, elem));
sema->value++;
```

waiters에서 맨 앞을 꺼내 깨움. 깨어난 스레드는 sema_down의 while 재확인에서 재개.

---

## 4. Lock — 세마포어 + 소유자

```c
struct lock {
    struct thread *holder;      // 이 lock의 현재 소유자
    struct semaphore semaphore; // value=1짜리 세마포어
};
```

lock = value가 1인 세마포어 + holder. holder는 "이 lock의 주인이 누구인지" 기록하는 것이다. 스레드가 자기를 잡는 게 아니라, lock이라는 물건에 "주인 스티커"를 붙이는 것이다.

### 동작 추적

```
[초기화] file_lock: holder=NULL, value=1, waiters=[]

[A가 lock_acquire]
→ sema_down: value=1, 0 아님 → value-- → value=0
→ holder = A
결과: holder=A, value=0, waiters=[]

[B가 lock_acquire]
→ sema_down: value=0 → while 진입 → B를 waiters에 넣고 → thread_block()
→ B 멈춤
결과: holder=A, value=0, waiters=[B]

[A가 lock_release]
→ holder = NULL
→ sema_up: waiters에서 B 꺼냄 → thread_unblock(B) → value++
결과: holder=NULL, value=1, waiters=[], B는 ready_list로

[B가 CPU 받고 재개]
→ while 재확인: value=1, 0 아님 → 탈출 → value-- → value=0
→ holder = B
결과: holder=B, value=0, waiters=[]
```

**lock은 공유 자원을 쓸 때만 잡는다.** 공유 자원을 안 건드리는 스레드는 lock_acquire를 호출하지 않는다. ready_list에서 CPU 순서만 기다릴 뿐이다.

---

## 5. Condition Variable — 선택적 깨우기

```c
struct condition {
    struct list waiters;    // semaphore_elem들의 리스트
};

struct semaphore_elem {
    struct list_elem elem;
    struct semaphore semaphore;   // 대기자 전용 세마포어 (value=0)
};
```

대기자마다 전용 세마포어를 하나씩 만든다. cond_signal이 특정 대기자만 골라 깨울 수 있게.

- `cond_wait`: 전용 세마포어(value=0) 생성 → cond->waiters에 등록 → lock 해제 → sema_down(바로 잠듦) → 깨어나면 lock 재획득
- `cond_signal`: cond->waiters에서 하나 꺼내서 그 전용 세마포어에 sema_up → 해당 스레드 깨어남

---

## 6. 현재 base code의 문제: FIFO

지금 base code는 모든 대기줄이 **먼저 온 순서(FIFO)**다:

```c
// sema_down — 대기줄에 넣을 때
list_push_back(&sema->waiters, &thread_current()->elem);  // 맨 뒤에

// sema_up — 대기줄에서 꺼낼 때
list_pop_front(&sema->waiters);  // 맨 앞에서
```

priority 60이 먼저 줄 서고, priority 90이 나중에 줄 서면 → 60을 먼저 깨운다. 90이 더 급한데.

ready_list도 마찬가지. `thread_unblock`, `thread_yield`에서 `list_push_back`으로 넣고 `list_pop_front`로 꺼냄. FIFO.

---

## 7. Priority Inversion 문제

```
스레드 3개: H(60), M(30), L(10)
L이 lock을 잡고 작업 중.

[시간 1] H 등장 → lock_acquire → L이 갖고 있음 → H 잠듦 (waiters)
[시간 2] M 등장 → lock 안 씀 → 그냥 CPU 경쟁

ready_list: M(30), L(10). H는 waiters에서 잠자는 중.

누가 CPU 받음? → M(30).
L(10)은 M보다 낮아서 계속 밀림.
L이 안 끝나면 lock_release를 못 함.
lock_release가 안 되면 H는 영원히 못 깨어남.

결과: priority 30인 M이 priority 60인 H를 막고 있다.
```

이게 **Priority Inversion**이다. 해결법이 **Priority Donation**이다.

H가 lock_acquire 호출 → lock->holder(L) 확인 → L의 priority를 60으로 올림 → L이 M(30)보다 높아짐 → L 실행 → lock_release → H 깨어남. 정상.

---

## 8. 구현 — 사전 합의 (3명 함께, Phase 0)

코드 작성 전에 반드시 3명이 같이 확정해야 할 것들이다. 여기서 struct가 안 맞으면 머지할 때 전부 다시 짜야 한다.

### thread 구조체에 추가할 필드 (thread.h)

```c
int base_priority;              // donation 전 원래 priority
                                // → lock_release 때 원래 priority로 돌아가야 하니까

struct list donations;          // 나에게 donation해준 스레드들의 리스트
                                // → lock을 여러 개 잡고 있을 때, 하나를 release하면
                                //   남은 donation 중 max를 찾아야 하니까

struct list_elem donation_elem; // 상대방의 donations 리스트에 매달리는 고리
                                // → elem은 ready_list/waiters에서 이미 쓰고 있어서
                                //   donations용 고리가 별도로 필요

struct lock *wait_on_lock;      // 내가 기다리고 있는 lock (없으면 NULL)
                                // → nested donation에서 chain을 따라가려면
                                //   "이 스레드가 어떤 lock을 기다리는지" 알아야 하니까
```

### thread_init()에서 초기화

```c
t->base_priority = priority;
list_init(&t->donations);
t->wait_on_lock = NULL;
```

### 비교 함수 시그니처

```c
bool cmp_priority(const struct list_elem *a, const struct list_elem *b, void *aux);
bool cmp_sema_priority(const struct list_elem *a, const struct list_elem *b, void *aux);
```

### 합의 체크리스트

- [x] 필드 이름, 타입 확정
- [x] 초기화 값 확정
- [x] 비교 함수 시그니처 확정
- [x] donation_elem이 들어가는 리스트 확인 (holder의 donations 리스트)

---

## 9. 구현 — 파트 A: Priority Scheduling (thread.c)

**담당**: 1명 **난이도**: ★★☆ **한 줄 요약**: ready_list를 FIFO에서 priority 순서로 바꾼다 **파일**: thread.c (단독)

### 수정 대상

|함수|현재|변경|
|---|---|---|
|`thread_unblock()`|`list_push_back`|`list_insert_ordered(cmp_priority)`|
|`thread_yield()`|`list_push_back`|`list_insert_ordered(cmp_priority)`|
|`thread_create()`|생성만|새 스레드 priority > 현재 → `thread_yield()`|
|`thread_set_priority()`|priority만 변경|변경 후 나보다 높은 게 ready_list에 있으면 `thread_yield()`|
|`next_thread_to_run()`|`list_pop_front`|수정 불필요 (정렬되어 있으므로 앞에서 꺼내면 됨)|

### 테스트 (혼자 바로 가능)

```bash
pintos -- run priority-change
pintos -- run priority-preempt
pintos -- run priority-fifo
```

### 주의

- `thread_set_priority()`는 파트 C에서도 수정한다. 파트 A에서는 yield 판단만 넣고, 파트 C가 머지할 때 donation 계산을 위에 추가한다.
- ready_list는 대기 중 donation으로 바뀌지 않으므로 **넣을 때 정렬** OK.

---

## 10. 구현 — 파트 B: Semaphore/Condition Priority (synch.c 스케줄링)

**담당**: 1명 **난이도**: ★★☆ **한 줄 요약**: waiters에서 꺼낼 때 가장 높은 priority를 꺼낸다 **파일**: synch.c (sema_up, cond_signal 영역만)

### 수정 대상

|함수|현재|변경|
|---|---|---|
|`sema_up()`|`list_pop_front`|`list_sort` 후 `list_pop_front` + 깨운 스레드 priority > 현재면 `thread_yield()`|
|`cond_signal()`|`list_pop_front`|최고 priority의 `semaphore_elem`을 꺼냄|

### 새로 구현할 함수

```c
// priority 비교 (내림차순 — 높은 priority가 리스트 앞에)
bool cmp_priority(const struct list_elem *a, const struct list_elem *b, void *aux) {
    struct thread *ta = list_entry(a, struct thread, elem);
    struct thread *tb = list_entry(b, struct thread, elem);
    return ta->priority > tb->priority;
}

// semaphore_elem 비교 (cond_signal에서 사용)
// 각 semaphore_elem의 waiters에서 가장 높은 priority를 비교
bool cmp_sema_priority(const struct list_elem *a, const struct list_elem *b, void *aux) {
    struct semaphore_elem *sa = list_entry(a, struct semaphore_elem, elem);
    struct semaphore_elem *sb = list_entry(b, struct semaphore_elem, elem);
    struct list_elem *la = list_begin(&sa->semaphore.waiters);
    struct list_elem *lb = list_begin(&sb->semaphore.waiters);
    struct thread *ta = list_entry(la, struct thread, elem);
    struct thread *tb = list_entry(lb, struct thread, elem);
    return ta->priority > tb->priority;
}
```

### 왜 꺼낼 때 정렬인가

대기 중에 donation으로 priority가 바뀔 수 있다. 넣을 때 정렬하면 바뀐 priority가 반영 안 됨. 꺼낼 때 `list_sort` 하면 항상 최신 priority 기준.

### 테스트 (파트 A 머지 후)

```bash
pintos -- run priority-sema
pintos -- run priority-condvar
```

파트 A가 머지되어야 테스트 가능. sema_up에서 최고 priority를 깨워도 ready_list가 FIFO면 순서가 밀린다. A 머지 전까지는 cmp 함수 구현 + 컴파일 확인만.

---

## 11. 구현 — 파트 C: Priority Donation (synch.c donation + thread.c 일부)

**담당**: 1명 (가장 실력 있는 사람) **난이도**: ★★★★ **한 줄 요약**: lock 기다릴 때 holder에게 priority를 빌려주고, lock 풀 때 회수한다 **파일**: synch.c (lock_acquire, lock_release 영역) + thread.c (thread_set_priority)

### lock_acquire — sema_down 전에 추가

```
1. 내가 이 lock을 기다린다고 기록:
   thread_current()->wait_on_lock = lock

2. lock->holder가 있고, 내 priority > holder의 priority면:
   a. holder의 priority = 내 priority (donation)
   b. 나를 holder의 donations 리스트에 추가
   c. nested donation:
      holder->wait_on_lock이 NULL이 아니면
      → 그 lock의 holder에게도 donation
      → chain 따라가며 반복 (최대 8단계)
```

### lock_acquire — sema_down 후에 추가

```
1. thread_current()->wait_on_lock = NULL
2. lock->holder = thread_current()
```

### lock_release — sema_up 전에 추가

```
1. donations 리스트에서 이 lock을 기다리던 스레드들 제거
   (wait_on_lock == 이 lock인 elem들)

2. priority 재계산:
   - donations 비었으면 → priority = base_priority
   - 남아있으면 → priority = max(base_priority, donations 중 최고 priority)

3. lock->holder = NULL
```

### thread_set_priority 수정

```
1. base_priority = new_priority
2. priority = max(base_priority, donations 중 최고 priority)
3. 나보다 높은 게 ready_list에 있으면 yield
```

### Multiple Donation — donations 리스트가 필요한 이유

```
L이 lock_A, lock_B 두 개를 잡고 있음.
H1(60)이 lock_A 기다림 → L에게 60 donation
H2(50)이 lock_B 기다림 → L에게 50 donation
L의 priority = 60

L이 lock_A를 release → H1의 donation(60) 제거
남은 donations: H2(50) → priority = max(base_priority, 50) = 50

donations 리스트가 없으면 base_priority(10)로 돌아가버림. H2가 밀림.
```

### Nested Donation — chain 전파

```
H(60) → lock_A 기다림 → holder는 M(30)
M(30) → lock_B 기다림 → holder는 L(10)

H가 M에게 60 donation → M = 60
근데 M도 잠자고 있음. L이 lock_B를 풀어야 M이 깨어남.
L은 아직 10. M보다 낮아서 밀림.

→ M의 wait_on_lock(lock_B) 확인 → lock_B->holder(L)
→ L에게도 60 donation
→ L이 실행 → lock_B release → M 깨어남 → lock_A release → H 깨어남
```

wait_on_lock이 있어야 chain을 따라갈 수 있다. 최대 8단계.

### 테스트 (파트 A+B 머지 후)

```bash
pintos -- run priority-donate-one
pintos -- run priority-donate-multiple
pintos -- run priority-donate-multiple2
pintos -- run priority-donate-nest
pintos -- run priority-donate-chain
pintos -- run priority-donate-sema
pintos -- run priority-donate-lower
```

파트 A+B가 머지되어야 테스트 가능. donation이 priority를 바꿔도 ready_list 정렬(A)과 waiters 정렬(B)이 안 되어 있으면 의미 없다. A+B 머지 전까지는 로직만 짜두고 컴파일 확인.

---

## 12. 머지 순서와 일정

```
Phase 0: 구조체 합의 (3명 함께, 1시간)
  └─ thread.h 수정 + thread_init() 초기화 + 비교 함수 시그니처 확정

Phase 1: 각자 구현 (병렬)
  ├─ A: thread.c 수정 → 혼자 테스트 가능
  ├─ B: synch.c 스케줄링 수정 + cmp 함수 → 컴파일 확인만
  └─ C: donation 로직 구현 → 컴파일 확인만

Phase 2: 머지 + 테스트 (3명 함께)
  1. A 머지 → priority-change, priority-preempt, priority-fifo 통과 확인
  2. B 머지 → priority-sema, priority-condvar 통과 확인
  3. C 머지 → priority-donate-* 전체 통과 확인

Phase 3: 전체 회귀 테스트
  └─ alarm-multiple 포함 전체 테스트
```

### 파일 충돌 지도

||thread.c|synch.c|thread.h|
|---|---|---|---|
|A|`thread_unblock`, `thread_yield`, `thread_create`, `thread_set_priority`|—|—|
|B|—|`sema_up`, `cond_signal` + cmp 함수|—|
|C|`thread_set_priority`|`lock_acquire`, `lock_release`|—|
|공통|—|—|struct 추가 (Phase 0)|

충돌 1곳: `thread_set_priority()` — A가 yield 판단을 넣고, C가 머지할 때 donation 계산을 그 위에 추가.

---

## 13. 전체 테스트 명령어

```bash
cd src/threads
make
cd build

# 기본 priority scheduling (파트 A)
pintos -- run priority-change
pintos -- run priority-preempt
pintos -- run priority-fifo

# semaphore/condition priority (파트 B)
pintos -- run priority-sema
pintos -- run priority-condvar

# donation (파트 C)
pintos -- run priority-donate-one
pintos -- run priority-donate-multiple
pintos -- run priority-donate-multiple2
pintos -- run priority-donate-nest
pintos -- run priority-donate-chain
pintos -- run priority-donate-sema
pintos -- run priority-donate-lower

# 기존 과제 안 깨졌는지
pintos -- run alarm-multiple
```