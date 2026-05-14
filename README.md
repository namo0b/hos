# 응급실 환자 접수 및 진료 관리 시스템

C 언어로 작성한 콘솔 기반 환자 접수 프로그램입니다. 환자의 증상에 따라 우선순위를 계산하고, 환자별 진료 기록을 CSV 파일로 저장해 프로그램을 다시 실행해도 기록을 불러올 수 있습니다.

## 폴더 구성

```text
um/
├─ test.c                  # main 함수
├─ web_main.c              # 웹 서버 main 함수
├─ include/
│  └─ hospital.h           # 공통 구조체, 상수, 함수 선언
├─ src/
│  ├─ patient.c            # 환자 생성, 접수, 상세 출력
│  ├─ records.c            # 환자별 기록 스택, CSV 저장/불러오기
│  ├─ queue.c              # 진료 우선순위 큐
│  ├─ tree.c               # 신규/재진 분류 트리
│  ├─ ui.c                 # 메뉴와 안내 화면
│  └─ utils.c              # 입력, 시간, 우선순위 계산, data 폴더 생성
├─ web/
│  └─ web_server.c         # C 기반 HTTP 서버와 API
├─ public/
│  ├─ index.html           # HTML 프론트엔드
│  ├─ style.css            # 화면 스타일
│  └─ app.js               # API 연동 스크립트
├─ build.ps1               # Windows PowerShell 빌드 스크립트
├─ run.ps1                 # 빌드 후 실행 스크립트
├─ build-web.ps1           # 웹 서버 빌드 스크립트
├─ run-web.ps1             # 웹 서버 빌드 후 실행 스크립트
├─ README.md               # 프로젝트 설명
├─ data/
│  └─ patient_records.csv  # 환자 진료 기록 CSV
└─ bin/
   ├─ test.exe             # 콘솔 실행 파일
   └─ hospital_web.exe     # 웹 서버 실행 파일
```

## 빌드

PowerShell에서 다음 명령을 실행합니다.

```powershell
.\build.ps1
```

직접 컴파일하려면 다음 명령을 사용해도 됩니다.

```powershell
gcc .\test.c -o .\bin\test.exe
```

모듈화 이후 직접 컴파일하려면 `src` 폴더의 `.c` 파일도 함께 컴파일해야 합니다.

```powershell
gcc .\test.c .\src\*.c -o .\bin\test.exe
```

## 실행

```powershell
.\run.ps1
```

또는 빌드 후 직접 실행합니다.

```powershell
.\bin\test.exe
```

## 웹 화면 실행

C 백엔드와 HTML 프론트엔드를 함께 실행하려면 다음 명령을 사용합니다.

```powershell
.\run-web.ps1
```

서버가 실행되면 브라우저에서 아래 주소를 엽니다.

```text
http://localhost:8080
```

웹 백엔드는 다음 API를 제공합니다.

```text
GET  /api/records
GET  /api/queue
POST /api/register
POST /api/complete
```

`POST /api/register`는 `application/x-www-form-urlencoded` 형식으로 `name`, `symptom` 값을 받습니다.
`GET /api/queue`는 중요도와 접수 시간을 기준으로 정렬된 현재 진료 대기열을 반환합니다.
`POST /api/complete`는 현재 대기열의 첫 번째 환자를 진료 완료 처리하고 대기열에서 제거합니다.

## 기록 저장

환자 접수 기록은 `data/patient_records.csv`에 저장됩니다. 이 파일을 삭제하면 기존 환자 기록도 함께 사라집니다.
