# Work Division

이 문서는 두 명이 프로젝트를 나눠 맡을 때 충돌을 줄이기 위한 담당 영역표입니다.

## 추천 분담

### A. Frontend + Web Integration

웹 화면과 브라우저-C 서버 연결을 담당합니다.

주요 파일:

```text
public/index.html
public/style.css
public/app.js
include/web.h
web/web_api.c
```

주요 역할:

- HTML 화면 구조 수정
- CSS 화면 디자인 수정
- JavaScript 이벤트, fetch API 호출 수정
- `/api/records`, `/api/queue`, `/api/register`, `/api/complete` 응답 형식 확인
- 프론트 화면에서 필요한 데이터가 부족하면 `web/web_api.c`에서 JSON 응답 보강

대략적인 코드량:

```text
public/      약 816 lines
web_api.c    약 239 lines
web.h         약 36 lines
합계          약 1091 lines
```

### B. Core C Backend

환자 접수 로직, 우선순위 큐, 기록 저장, 콘솔 실행 흐름을 담당합니다.

주요 파일:

```text
include/hospital.h
src/patient.c
src/queue.c
src/records.c
src/tree.c
src/ui.c
src/utils.c
test.c
web_main.c
build.ps1
build-web.ps1
run.ps1
run-web.ps1
web/web_server.c
web/router.c
web/static_files.c
web/http_response.c
web/web_utils.c
```

주요 역할:

- `Patient`, `PriorityQueue`, `MedicalRecord` 구조체와 함수 관리
- 증상별 우선순위 계산
- 접수/진료 완료/검색/기록 저장 로직 수정
- CSV 저장과 불러오기
- 콘솔 버전 실행 흐름 관리
- 웹 서버 시작, 요청 라우팅, 정적 파일 전송, HTTP 응답 관리
- 빌드/실행 스크립트 관리

대략적인 코드량:

```text
src/ + hospital.h       약 900 lines
web server utilities    약 325 lines
entry/scripts           약 170 lines
합계                    약 1395 lines
```

## 왜 이렇게 나눴는가

`web/web_api.c`는 C 파일이지만 프론트엔드와 가장 자주 맞닿는 파일입니다. 화면에서 필요한 데이터 형식이 바뀌면 이 파일을 같이 봐야 하므로 A 담당에 넣었습니다.

반대로 `web/router.c`, `web/static_files.c`, `web_server.c`는 화면 기능보다 서버 동작 자체에 가깝기 때문에 B 담당에 넣었습니다.

## 협업 규칙

- A가 `public/`을 수정할 때는 B가 같은 시간에 `public/`을 수정하지 않습니다.
- B가 `src/`와 `include/hospital.h`를 수정할 때는 A가 같은 파일을 수정하지 않습니다.
- `web/web_api.c`는 A 담당이지만, 내부 C 로직이 바뀌면 B와 함께 확인합니다.
- `include/hospital.h` 구조체나 함수 선언을 바꾸면 반드시 두 사람이 같이 확인합니다.
- 작업 전에는 `git pull`, 작업 후에는 branch에서 commit 후 PR을 만듭니다.

## 파일 상단 표시

각 주요 파일 상단에는 아래 형식의 주석이 있습니다.

```text
OWNER AREA: A - Frontend + Web Integration
OWNER AREA: B - Core C Backend
```

이 표시는 담당자를 고정하는 규칙이라기보다, “먼저 확인해야 하는 사람”을 나타내는 표식입니다.
