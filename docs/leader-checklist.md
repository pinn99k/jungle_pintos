# 팀장용 체크리스트

> 팀원 공지 전에 레포 상태를 먼저 깨끗하게 만드는 게 핵심.

## ✅ 0. (완료) 먼저 해결: 지금 630개 파일이 "수정됨"으로 뜨는 문제

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

## ✅ 1. (완료) dev 브랜치 생성

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
| General → Default branch | `dev` 로 변경 → clone 시 dev에서 시작, PR 기본 대상도 dev |
| General → Pull Requests | **Allow merge commits 만 체크** (squash·rebase 해제) / "Automatically delete head branches" **해제** (브랜치 삭제는 직접 연습) |
| Rules → Rulesets (main, dev) | Restrict deletions ✅ / Require a pull request (approvals: 1) ✅ / Block force pushes ✅ / Bypass list 비움 |

## 3. 머지 정책

- **모든 PR은 `Create a merge commit` 하나로 통일** (Settings에서 이 버튼만 남겨둠)
- 머지 = GitHub PR 화면의 초록 버튼. dev 에 직접 push 는 룰 때문에 막혀 있음
- 흐름: 내 브랜치 push → PR → 팀원 1명 Approve → Merge 버튼 → 브랜치 삭제

## 4. 작업 분배 — 🔜 TODO (코드 파악 후 결정)

- [ ] Pintos 코드 읽고 기능별로 어떤 파일/함수를 건드리는지 파악
- [ ] 분업 방식 결정 (기능별 분업 + 리뷰 / 각자 전부 구현 후 베스트 머지)
- [ ] 결정되면 공지 템플릿 5번 채우기

참고 메모:
- Pintos는 `thread.c`, `synch.c` 에 작업이 몰려서 충돌이 잦음
- `struct thread` 필드 추가는 한 명이 먼저 PR → 머지 후 다들 pull 하고 시작하면 충돌 절반 감소
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
# GitHub에서 PR: dev → main → Merge
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
5. PR은 팀원 1명 Approve 후 Merge, 머지 후 브랜치는 직접 삭제
6. 이번 주 담당: (TODO)
```
