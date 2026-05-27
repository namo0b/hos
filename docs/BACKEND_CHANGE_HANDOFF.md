# 백엔드 변경 및 팀원 수정 안내

작성 기준: 현재 작업 트리와 git 기준 버전의 차이(`git diff`)를 기준으로 정리했습니다.

추가 확인: GitHub `main`도 확인했습니다.

- 확인한 GitHub 링크: https://github.com/namo0b/hos/tree/main/.vscode
- 원격 원본 파일 예시: https://raw.githubusercontent.com/namo0b/hos/main/web/web_server.c
- 원격 `main` 기준으로는 웹 백엔드가 `web/web_server.c` 한 파일에 대부분 들어있는 구조입니다.
- 현재 로컬 기준으로는 웹 백엔드가 `web/web_server.c`, `web/router.c`, `web/web_api.c`, `web/web_utils.c`, `web/http_response.c`, `web/static_files.c`, `include/web.h`로 분리되어 있습니다.
- 따라서 아래 라인 번호는 모두 현재 로컬 파일 기준입니다.

GitHub `main`과 비교했을 때 추가로 팀원에게 말해야 할 핵심:

- 단순히 다중 증상 로직만 바뀐 것이 아니라, 웹 서버 코드가 기능별 파일로 분리된 상태입니다.
- 원격 `main`에는 `include/web.h`, `web/web_api.c`, `web/router.c`, `web/web_utils.c`, `web/http_response.c`, `web/static_files.c`가 없거나 아직 반영되지 않은 구조로 보입니다.
- 원격 `main`의 `web/web_server.c`에는 `handleRegisterApi`, `handleQueueApi`, `sendHttp`, `sendFile`, `getFormValue` 등이 한 파일 안에 있습니다.
- 현재 로컬에서는 그중 API 처리 로직은 `web/web_api.c`, 라우팅은 `web/router.c`, HTTP 응답은 `web/http_response.c`, 정적 파일 응답은 `web/static_files.c`, 공통 유틸은 `web/web_utils.c`로 이동되어 있습니다.

## 확인 결과

- C 콘솔 빌드: `.\build.ps1` 성공
- C 웹 서버 빌드: `.\build-web.ps1` 성공
- 경고 빌드: `gcc -Wall -Wextra` 기준 미사용 변수/함수 경고 없음
- JS 문법 검사: 기본 `node`는 `Access is denied`로 막혔지만, Codex 번들 Node 경로로 검사 성공

JS 문법 검사에 사용한 명령:

```powershell
& "C:\Users\전세림\.cache\codex-runtimes\codex-primary-runtime\dependencies\node\bin\node.exe" --check public\app.js
```

## 팀원이 확인하거나 수정할 부분

### 1. API 문서 또는 README의 접수 파라미터 설명 수정 필요

기존 설명이 `POST /api/register`에서 `symptom` 단일 값을 받는다고 되어 있다면, 이제는 프론트에서 `symptoms`에 여러 증상을 `|`로 묶어 보냅니다.

수정해야 할 설명 예시:

```text
POST /api/register
- name: 환자 이름
- symptoms: 선택한 여러 증상. 예: 심정지|구토|고열
- symptom: 기존 단일 증상 입력 호환용 fallback
```

관련 코드 위치:

- 파일: `web/web_api.c`
- 라인: 225-228

```c
if (!getFormValue(body, "name", name, sizeof(name)) ||
    (!getFormValue(body, "symptoms", symptomsRaw, sizeof(symptomsRaw)) &&
     !getFormValue(body, "symptom", symptomsRaw, sizeof(symptomsRaw))) ||
    strlen(name) == 0 || strlen(symptomsRaw) == 0) {
```

### 2. 다중 증상 저장 형식 공유 필요

프론트는 여러 증상을 `심정지|구토` 형태로 전송하지만, CSV와 화면 표시에는 `심정지, 구토` 형태로 저장합니다.

관련 코드 위치:

- 파일: `web/web_api.c`
- 라인: 99-116

```c
static void formatSymptomsForDisplay(const char *rawSymptoms, char *display, int size) {
    int sourceIndex;
    int destIndex = 0;

    /* CHANGED: 프론트에서 보낸 심정지|구토 형식을 저장/표시용 심정지, 구토 형식으로 변환 */
    for (sourceIndex = 0; rawSymptoms[sourceIndex] != '\0' && destIndex < size - 1; sourceIndex++) {
        if (rawSymptoms[sourceIndex] == '|') {
            if (destIndex < size - 2) {
                display[destIndex++] = ',';
                display[destIndex++] = ' ';
            }
        } else {
            display[destIndex++] = rawSymptoms[sourceIndex];
        }
    }

    display[destIndex] = '\0';
}
```

### 3. 우선순위 계산 방식 변경 공유 필요

여러 증상이 들어오면 각 증상의 우선순위를 계산한 뒤, 가장 긴급한 단계의 값을 사용합니다. 숫자가 작을수록 더 긴급합니다.

관련 코드 위치:

- 파일: `src/utils.c`
- 라인: 110-136

```c
int calculateSymptomsPriority(const char *symptoms) {
    char buffer[SYMPTOM_SIZE];
    char *token;
    int bestPriority = 5;

    /* CHANGED: 여러 증상이 | 또는 , 로 들어오면 가장 긴급한 단계로 계산 */
    strncpy(buffer, symptoms, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    token = strtok(buffer, "|,");
    while (token != NULL) {
        while (*token == ' ') {
            token++;
        }

        {
            int priority = calculateSymptomPriority(token);
            if (priority < bestPriority) {
                bestPriority = priority;
            }
        }

        token = strtok(NULL, "|,");
    }

    return bestPriority;
}
```

선언 추가 위치:

- 파일: `include/hospital.h`
- 라인: 60

```c
int calculateSymptomsPriority(const char *symptoms);
```

### 4. 콘솔 접수도 다중 증상 입력으로 변경됨

웹뿐 아니라 콘솔 접수에서도 쉼표로 여러 증상을 입력하면 가장 긴급한 단계로 계산합니다.

관련 코드 위치:

- 파일: `src/patient.c`
- 라인: 78, 95-96

```c
readLine("증상 입력(여러 개는 쉼표로 구분): ", symptom, sizeof(symptom));

/* CHANGED: 콘솔 입력도 여러 증상을 쉼표로 넣으면 가장 긴급한 단계로 계산 */
currentPriority = calculateSymptomsPriority(symptom);
```

### 5. 증상 버퍼 크기 증가

여러 증상을 한 문자열로 담기 위해 증상 문자열 크기를 `100`에서 `256`으로 늘렸습니다.

관련 코드 위치:

- 파일: `include/hospital.h`
- 라인: 9

```c
#define SYMPTOM_SIZE 256
```

### 6. 웹 접수 처리 흐름 변경

웹 접수 시 raw 증상 문자열과 표시용 증상 문자열을 분리해서 사용합니다.

- `symptomsRaw`: 프론트에서 받은 원본. 예: `심정지|구토`
- `symptom`: 저장/표시용 변환 결과. 예: `심정지, 구토`

관련 코드 위치:

- 파일: `web/web_api.c`
- 라인: 210-211, 234-241

```c
char symptomsRaw[SYMPTOM_SIZE];
char symptom[SYMPTOM_SIZE];
```

```c
/* CHANGED: 다중 증상은 raw 문자열로 우선순위를 계산하고, 표시용 문자열은 별도로 저장 */
formatSymptomsForDisplay(symptomsRaw, symptom, sizeof(symptom));
findLatestRecord(name, recentSymptom, sizeof(recentSymptom), &recentPriority);
isRevisit = (recentPriority > 0);
currentPriority = calculateSymptomsPriority(symptomsRaw);
finalPriority = calculateFinalPriority(isRevisit, recentPriority, currentPriority);
getCurrentTime(receptionTime, sizeof(receptionTime));
appendMedicalRecordToCsv(name, symptom, receptionTime, finalPriority);
```

### 7. 환자 구분 문자열 깨짐 수정

웹 접수 응답 JSON에서 깨져 있던 환자 구분 문자열을 정상 한글로 수정했습니다.

관련 코드 위치:

- 파일: `web/web_api.c`
- 라인: 255-261

```c
snprintf(json, sizeof(json),
         "{\"name\":\"%s\",\"symptom\":\"%s\",\"time\":\"%s\","
         "\"patientNo\":%d,\"patientType\":\"%s\",\"recentRecord\":\"%s\","
         "\"recentPriority\":%d,\"currentPriority\":%d,\"finalPriority\":%d}",
         safeName, safeSymptom, safeTime, nextWebPatientNo - 1,
         isRevisit ? "재진 환자" : "신규 환자",
         safeRecent, recentPriority, currentPriority, finalPriority);
```

### 8. 웹 서버 출력 메시지 확인

로컬 diff 기준으로는 웹 서버 실행/오류 메시지의 깨진 한글을 정상 문장으로 수정했습니다.

다만 GitHub `main`의 `web/web_server.c`를 직접 확인하면 해당 메시지는 이미 정상 한글로 보입니다. 따라서 팀원이 GitHub `main`을 기준으로 본다면 이 항목은 큰 기능 변경이 아니라 출력 문구/띄어쓰기 확인 정도로 보면 됩니다.

관련 코드 위치:

- 파일: `web/web_server.c`
- 라인: 18, 24, 34, 40, 47-49

확인할 문장:

```c
printf("Winsock 초기화에 실패했습니다.\n");
printf("서버 소켓을 만들 수 없습니다.\n");
printf("포트 %d를 사용할 수 없습니다. 이미 실행 중인지 확인해 주세요.\n", port);
printf("요청 대기 상태로 전환하지 못했습니다.\n");
printf("\n웹 서버가 실행되었습니다.\n");
printf("브라우저에서 http://localhost:%d 를 열어 주세요.\n", port);
printf("종료하려면 이 창에서 Ctrl+C를 누르세요.\n\n");
```

### 9. GitHub main 기준 웹 백엔드 파일 분리 확인 필요

GitHub `main`에서는 웹 서버 관련 함수들이 대부분 `web/web_server.c`에 모여 있습니다. 현재 로컬에서는 아래처럼 파일이 나뉘어 있습니다.

팀원이 원격 `main`에 병합하거나 비교할 때는 이 구조 변경을 먼저 확인해야 합니다.

관련 코드 위치:

- 파일: `include/web.h`
- 라인: 23-39

```c
extern PriorityQueue waitingQueue;
extern int nextWebPatientNo;

void appendText(char *buffer, size_t size, const char *text);
void appendFormat(char *buffer, size_t size, const char *format, ...);
void jsonEscape(const char *src, char *dest, int size);
int getFormValue(const char *body, const char *key, char *value, int size);

void sendHttp(SOCKET client, int status, const char *statusText,
              const char *contentType, const char *body);
void sendFile(SOCKET client, const char *urlPath);

void handleRecordsApi(SOCKET client);
void handleQueueApi(SOCKET client);
void handleCompleteApi(SOCKET client);
void handleRegisterApi(SOCKET client, const char *body);
void handleClient(SOCKET client);
```

현재 분리된 주요 파일:

- `web/web_server.c`: Winsock 초기화, 서버 소켓 생성, accept 루프
- `web/router.c`: HTTP 요청 파싱과 URL별 핸들러 연결
- `web/web_api.c`: `/api/records`, `/api/queue`, `/api/register`, `/api/complete`
- `web/web_utils.c`: 문자열 append, JSON escape, URL decode, form 값 추출
- `web/http_response.c`: HTTP 응답 헤더/본문 전송
- `web/static_files.c`: HTML/CSS/JS 정적 파일 응답

## 프론트와 백엔드 연결 규칙

프론트에서 보내는 코드:

- 파일: `public/app.js`
- 라인: 261-273

```js
const selectedSymptoms = [...form.querySelectorAll('input[name="symptoms"]:checked')]
    .map((input) => input.value);

const formData = new URLSearchParams();
formData.set("name", document.querySelector("#nameInput").value.trim());
formData.set("symptoms", selectedSymptoms.join("|"));
```

백엔드에서 받는 코드:

- 파일: `web/web_api.c`
- 라인: 225-228

```c
if (!getFormValue(body, "name", name, sizeof(name)) ||
    (!getFormValue(body, "symptoms", symptomsRaw, sizeof(symptomsRaw)) &&
     !getFormValue(body, "symptom", symptomsRaw, sizeof(symptomsRaw))) ||
    strlen(name) == 0 || strlen(symptomsRaw) == 0) {
```

## 남은 주의사항

- `data/patient_records.csv`도 수정 상태입니다. 테스트 데이터라면 커밋 전 포함 여부를 팀에서 결정해야 합니다.
- `.vs/` 폴더가 untracked 상태입니다. Visual Studio 임시 폴더라면 커밋하지 않는 것이 좋습니다.
- README가 깨진 인코딩처럼 보이는 부분이 있습니다. 팀 제출용 문서라면 UTF-8로 다시 정리하는 것이 좋습니다.
