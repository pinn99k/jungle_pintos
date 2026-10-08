# 팀원용 협업 가이드

> 이 문서만 따라 하면 됨. 모르면 바로 팀장한테 물어보기.

## 0. 브랜치 구조

```
main      ← 프로젝트(주차) 완료본만. 직접 push 금지
 └─ dev   ← 통합 브랜치. PR로만 들어감
     ├─ feat/alarm-clock
     ├─ feat/priority-scheduling
     └─ fix/...
```

- **main, dev 에 직접 push 절대 금지** → 항상 내 브랜치 → PR → dev
- 작업 단위 = 기능 하나 (예: alarm clock, priority donation)

## 1. 최초 1회 세팅

```bash
git clone https://github.com/pinn99k/jungle_pintos.git
cd jungle_pintos
git config core.filemode false   # Windows: 권한(755↔644) 변경이 수정으로 잡히는 것 방지
git config core.autocrlf input   # Windows: CRLF 유입 방지 (Docker 안 셸스크립트 깨짐)
git switch dev
```

## 2. 매일 작업 흐름

```bash
# ① 최신 dev 받기
git switch dev
git pull origin dev

# ② 내 브랜치 만들기 (dev에서 출발)
git switch -c feat/alarm-clock

# ③ 작업 + 커밋 (자주, 작게)
git add pintos/threads/thread.c pintos/devices/timer.c
git commit -m "feat(threads): timer_sleep busy-wait 제거"

# ④ push
git push -u origin feat/alarm-clock

# ⑤ GitHub에서 PR 생성: feat/alarm-clock → dev   (base 가 main 아닌지 꼭 확인!)
# ⑥ 팀원 1명 Approve → 초록 [Merge pull request] 버튼 클릭   ← 이때 dev 에 들어감
# ⑦ 브랜치 삭제 (7번 참고) → 다음 작업은 다시 ①부터
```

> dev 에 직접 `git push origin dev` 는 **막혀 있음**. 항상 내 브랜치를 push 하고 PR 로 합친다.

PR 올리기 전 체크:
- [ ] Docker 컨테이너에서 빌드 성공 (`make` in `pintos/threads`)
- [ ] 관련 테스트 통과 (`make check` 또는 개별 테스트)
- [ ] `git status` 에 내 작업 아닌 파일 안 섞였는지

## 3. 브랜치 이름

| 접두어 | 용도 | 예시 |
|---|---|---|
| `feat/` | 기능 구현 | `feat/priority-donation` |
| `fix/` | 버그 수정 | `fix/sema-up-yield` |
| `merge/` | 충돌 해결 전용 | `merge/donation-into-dev` |
| `docs/` | 문서/주석 | `docs/design-note` |

소문자 + 하이픈. 이름만 보고 뭐 하는 브랜치인지 알 수 있게.

## 4. 커밋 메시지

```
<type>(<영역>): <무엇을 했는지 한 줄>

feat(threads): ready_list 우선순위 정렬 삽입
fix(synch): lock_release 시 donation 복구 누락
test(threads): priority-donate-nest 통과
refactor(timer): sleep_list 처리 함수 분리
```

## 5. 충돌(conflict) 났을 때

PR 화면에 "This branch has conflicts" 가 뜨면:

### 기본: 내 브랜치에서 해결 (대부분 이걸로 충분)

```bash
git switch feat/alarm-clock
git pull origin dev              # dev 변경을 내 브랜치로 merge → 충돌 발생
# 충돌 파일 열어서 <<<<<<< ======= >>>>>>> 정리
git add <해결한 파일>
git commit                        # merge 커밋 완료
# 빌드 + 테스트 다시 돌리기!
git push                          # PR 자동 갱신
```

### 충돌이 크거나 여러 명 코드가 엉킬 때: 해결 전용 브랜치

```bash
git switch feat/priority-donation
git switch -c merge/donation-into-dev
git pull origin dev
# 관련 팀원이랑 같이 보면서 해결 → 빌드 + 테스트
git push -u origin merge/donation-into-dev
# PR: merge/donation-into-dev → dev   (원래 PR은 닫기)
```

충돌 해결 규칙:
- **남의 코드를 지우는 쪽으로 해결할 땐 반드시 그 사람한테 먼저 물어보기**
- 해결 후 테스트 안 돌리고 push 금지
- 모르겠으면 `git merge --abort` 로 되돌리고 팀장 호출

## 6. 하지 말 것

- `git push --force` (내 브랜치에서도 PR 올린 뒤엔 금지)
- main / dev 에서 직접 커밋
- `git add .` 로 빌드 산출물(`build/`) 같이 올리기
- 오래된 브랜치 붙잡고 있기 → 하루 1번은 `git pull origin dev`

## 7. PR 리뷰 · 머지 · 브랜치 삭제

- PR엔 최소 1명 승인 받아야 머지 버튼 활성화 (내 PR은 내가 승인 못 함)
- 리뷰할 때: "왜 이렇게 했어?" 질문 환영 — 정글은 설명할 수 있어야 내 것
- 머지 버튼은 **Create a merge commit** 하나뿐 (팀 설정)

머지 후 브랜치 삭제 (자동 삭제 꺼둠 → 직접 연습):

```bash
git switch dev
git pull                                   # 머지된 최신 dev 받기
git branch -d feat/alarm-clock             # 내 PC 브랜치 삭제 (-d: 머지 안 됐으면 거부 → 안전)
git push origin --delete feat/alarm-clock  # GitHub 브랜치 삭제 (PR 화면 Delete branch 버튼과 같음)
git fetch --prune                          # 이미 지워진 원격 브랜치 목록 정리
```
