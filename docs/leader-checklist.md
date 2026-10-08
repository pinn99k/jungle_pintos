# 팀장용 체크리스트

> 팀원 공지 전에 레포 상태를 먼저 깨끗하게 만드는 게 핵심.

## ⚠️ 0. 먼저 해결: 지금 630개 파일이 "수정됨"으로 뜨는 문제

원인: 내용 변경 0줄, **파일 권한만 바뀜** (`100755 → 100644`). Windows 체크아웃에서 생기는 현상.

```bash
git config core.filemode false
git status          # 깨끗해졌는지 확인
```

이대로 `git add .` 하면 권한 변경 630개가 커밋되고 팀원 전부 충돌남 → 반드시 먼저 처리.

추가로 `core.autocrlf=true` 상태 → 셸 스크립트(`pintos/activate`, `utils/*`)가 CRLF가 되면 Docker(Linux)에서 깨짐. 레포에 `.gitattributes` 넣기:

```gitattributes
* text=auto eol=lf
*.png binary
*.dsk binary
```

```bash
git add .gitattributes
git commit -m "chore: LF 줄바꿈 고정"
git push origin main
```

## 1. dev 브랜치 생성

```bash
git switch main
git pull origin main
git switch -c dev
git push -u origin dev
```

## 2. GitHub 설정 (Settings)

| 위치 | 설정 |
|---|---|
| Collaborators | 팀원 초대 (Write 권한) |
| Branches → Default branch | `dev` 로 변경 → PR 기본 base 가 dev 가 됨 (main 실수 방지) |
| Rules → Rulesets (main, dev) | Require a pull request before merging ✅ / Required approvals: 1 ✅ / Block force pushes ✅ |
| General → Pull Requests | "Automatically delete head branches" ✅ |

## 3. 머지 정책 결정 (팀원 공지 전에 정해두기)

| 방향 | 방식 | 이유 |
|---|---|---|
| feat → dev | **Squash and merge** | dev 히스토리 = 기능 1개당 커밋 1개 → 깔끔 |
| dev → main | **Create a merge commit** | 주차 마일스톤 기록용 (예: "Project 1 완료") |

## 4. 작업 분배 팁 (충돌 줄이기)

Pintos는 `thread.c`, `synch.c` 에 다 몰려서 충돌이 잦음.

- 기능별로 **건드리는 함수를 미리 나눠서** 공지 (예: A=timer_sleep/wakeup, B=ready_list 정렬, C=donation)
- `struct thread` 필드 추가는 **한 명이 먼저 PR** → 머지 후 다들 pull 하고 시작
- 의존성 있는 기능(priority → donation)은 순서대로 머지

## 5. 충돌 해결 브랜치 운영 (구상한 방식 정리)

```
dev ──●────────●──────────●─── 
       \        \        /
        feat/A ──●──(dev pull)── merge/A-into-dev
```

- 소규모 충돌: 작성자가 자기 feat 브랜치에서 `git pull origin dev` 로 해결 (팀원 가이드 5번)
- 대규모/여러 명 엉킴: `merge/<기능>-into-dev` 브랜치 → 관련자 같이 해결 → PR
- 해결 PR 리뷰 포인트: **지워진 코드가 누구 것인지**, 테스트 통과 여부

## 6. 마일스톤 (주차 마감)

```bash
# dev 에서 전체 make check 통과 확인 후
# GitHub에서 PR: dev → main  (merge commit)
git switch main && git pull
git tag project1-threads
git push origin project1-threads
```

## 7. 팀원 공지 템플릿

```
[협업 규칙 공지]
1. 레포: https://github.com/pinn99k/jungle_pintos  (초대 수락해주세요)
2. 가이드: docs/team-guide.md 꼭 읽기
3. Windows면 clone 후 아래 2줄 필수
   git config core.filemode false
   git config core.autocrlf input
4. main/dev 직접 push 금지 → 브랜치 파서 dev로 PR
5. 이번 주 담당: A=___ / B=___ / C=___
```
