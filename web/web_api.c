/* OWNER AREA: A - Frontend + Web Integration */
// 웹 화면에서 호출하는 API 요청을 처리하고, 진료 기록/대기열/환자 접수 결과를 JSON으로 응답하는 파일

#include "../include/web.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// CSV에 저장된 전체 진료 기록을 /api/records 응답 JSON으로 만드는 함수
static void buildRecordsJson(char *json, size_t size) {
    FILE *file = fopen(RECORD_FILE, "r");
    char line[512];
    int first = 1;

    strcpy(json, "{\"records\":[");

    if (file == NULL) { // 기록 파일이 없으면 빈 records 배열 반환
        appendText(json, size, "]}");
        return;
    }

    fgets(line, sizeof(line), file); // CSV 헤더 줄 건너뛰기

    while (fgets(line, sizeof(line), file) != NULL) {
        char *cursor = line;
        char name[NAME_SIZE];
        char symptom[SYMPTOM_SIZE];
        char timeText[TIME_SIZE];
        char priorityText[20];
        char safeName[NAME_SIZE * 2];
        char safeSymptom[SYMPTOM_SIZE * 2];
        char safeTime[TIME_SIZE * 2];

        removeNewline(line);

        // CSV 한 줄에서 이름, 증상, 시간, 중요도 순서로 필드 읽기
        if (!readCsvField(&cursor, name, sizeof(name)) ||
            !readCsvField(&cursor, symptom, sizeof(symptom)) ||
            !readCsvField(&cursor, timeText, sizeof(timeText)) ||
            !readCsvField(&cursor, priorityText, sizeof(priorityText))) {
            continue;
        }

        // JSON 문자열에 들어갈 수 있도록 특수 문자 처리
        jsonEscape(name, safeName, sizeof(safeName));
        jsonEscape(symptom, safeSymptom, sizeof(safeSymptom));
        jsonEscape(timeText, safeTime, sizeof(safeTime));

        if (!first) {
            appendText(json, size, ",");
        }
        appendFormat(json, size,
                     "{\"name\":\"%s\",\"symptom\":\"%s\",\"time\":\"%s\",\"priority\":%d}",
                     safeName, safeSymptom, safeTime, atoi(priorityText));
        first = 0;
    }

    fclose(file);
    appendText(json, size, "]}");
}

// 같은 이름의 환자 기록 중 마지막으로 읽힌 기록을 최근 기록으로 저장
static void findLatestRecord(const char *targetName, char *recentSymptom,
                             int symptomSize, int *recentPriority) {
    FILE *file = fopen(RECORD_FILE, "r");
    char line[512];

    strcpy(recentSymptom, EMPTY_TEXT);
    *recentPriority = 0;

    if (file == NULL) {
        return;
    }

    fgets(line, sizeof(line), file); // CSV 헤더 줄 건너뛰기

    while (fgets(line, sizeof(line), file) != NULL) {
        char *cursor = line;
        char name[NAME_SIZE];
        char symptom[SYMPTOM_SIZE];
        char timeText[TIME_SIZE];
        char priorityText[20];

        removeNewline(line);

        if (!readCsvField(&cursor, name, sizeof(name)) ||
            !readCsvField(&cursor, symptom, sizeof(symptom)) ||
            !readCsvField(&cursor, timeText, sizeof(timeText)) ||
            !readCsvField(&cursor, priorityText, sizeof(priorityText))) {
            continue;
        }

        if (strcmp(name, targetName) == 0) { // 같은 이름이 여러 번 나오면 가장 마지막 기록으로 갱신
            strncpy(recentSymptom, symptom, symptomSize - 1);
            recentSymptom[symptomSize - 1] = '\0';
            *recentPriority = atoi(priorityText);
        }
    }

    fclose(file);
}

// 프론트 전송용 구분자(|)를 사람이 읽기 쉬운 표시용 문자열로 변경
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

// GET /api/records: 저장된 진료 기록 목록을 JSON으로 반환
void handleRecordsApi(SOCKET client) {
    char *json = (char *)malloc(RESPONSE_SIZE);

    if (json == NULL) { // 큰 JSON 응답 버퍼 할당 실패 처리
        sendHttp(client, 500, "Internal Server Error", "text/plain; charset=utf-8", "Memory error");
        return;
    }

    buildRecordsJson(json, RESPONSE_SIZE);
    sendHttp(client, 200, "OK", "application/json; charset=utf-8", json);
    free(json);
}

// 현재 대기열을 /api/queue 응답 JSON으로 만드는 함수
static void buildQueueJson(char *json, size_t size) {
    Patient *current = waitingQueue.front;
    int first = 1;

    strcpy(json, "{\"queue\":[");

    while (current != NULL) { // 우선순위 큐의 앞에서부터 환자 정보를 JSON 배열로 추가
        char safeName[NAME_SIZE * 2];
        char safeSymptom[SYMPTOM_SIZE * 2];
        char safeTime[TIME_SIZE * 2];
        char safeType[TYPE_SIZE * 2];
        char safeRecent[SYMPTOM_SIZE * 2];

        // 환자 정보에 따옴표 등이 들어와도 JSON이 깨지지 않도록 처리
        jsonEscape(current->name, safeName, sizeof(safeName));
        jsonEscape(current->symptom, safeSymptom, sizeof(safeSymptom));
        jsonEscape(current->receptionTime, safeTime, sizeof(safeTime));
        jsonEscape(current->patientType, safeType, sizeof(safeType));
        jsonEscape(current->recentRecord, safeRecent, sizeof(safeRecent));

        if (!first) {
            appendText(json, size, ",");
        }

        appendFormat(json, size,
                     "{\"patientNo\":%d,\"name\":\"%s\",\"symptom\":\"%s\","
                     "\"time\":\"%s\",\"patientType\":\"%s\",\"recentRecord\":\"%s\","
                     "\"recentPriority\":%d,\"currentPriority\":%d,\"finalPriority\":%d}",
                     current->patientNo, safeName, safeSymptom, safeTime, safeType,
                     safeRecent, current->recentRecordPriority,
                     current->currentSymptomPriority, current->finalPriority);

        first = 0;
        current = current->next;
    }

    appendText(json, size, "]}");
}

// GET /api/queue: 현재 진료 대기열을 JSON으로 반환
void handleQueueApi(SOCKET client) {
    char *json = (char *)malloc(RESPONSE_SIZE);

    if (json == NULL) { // 큰 JSON 응답 버퍼 할당 실패 처리
        sendHttp(client, 500, "Internal Server Error", "text/plain; charset=utf-8", "Memory error");
        return;
    }

    buildQueueJson(json, RESPONSE_SIZE);
    sendHttp(client, 200, "OK", "application/json; charset=utf-8", json);
    free(json);
}

// POST /api/complete: 대기열 첫 번째 환자를 진료 완료 처리
void handleCompleteApi(SOCKET client) {
    Patient *done = dequeuePatient(&waitingQueue);
    char json[2048];
    char safeName[NAME_SIZE * 2];
    char safeSymptom[SYMPTOM_SIZE * 2];
    char safeTime[TIME_SIZE * 2];

    if (done == NULL) { // 대기 환자가 없으면 completed false 응답
        sendHttp(client, 200, "OK", "application/json; charset=utf-8",
                 "{\"completed\":false,\"message\":\"waiting queue is empty\"}");
        return;
    }

    // 완료된 환자 정보를 응답으로 보내기 전에 JSON 문자열 안전 처리
    jsonEscape(done->name, safeName, sizeof(safeName));
    jsonEscape(done->symptom, safeSymptom, sizeof(safeSymptom));
    jsonEscape(done->receptionTime, safeTime, sizeof(safeTime));

    snprintf(json, sizeof(json),
             "{\"completed\":true,\"patientNo\":%d,\"name\":\"%s\","
             "\"symptom\":\"%s\",\"time\":\"%s\",\"finalPriority\":%d}",
             done->patientNo, safeName, safeSymptom, safeTime, done->finalPriority);

    free(done);
    sendHttp(client, 200, "OK", "application/json; charset=utf-8", json);
}

// POST /api/register: 환자 접수, 기록 저장, 대기열 추가를 한 번에 처리
void handleRegisterApi(SOCKET client, const char *body) {
    char name[NAME_SIZE];
    char symptomsRaw[SYMPTOM_SIZE];
    char symptom[SYMPTOM_SIZE];
    char receptionTime[TIME_SIZE];
    char recentSymptom[SYMPTOM_SIZE];
    char safeName[NAME_SIZE * 2];
    char safeSymptom[SYMPTOM_SIZE * 2];
    char safeRecent[SYMPTOM_SIZE * 2];
    char safeTime[TIME_SIZE * 2];
    int recentPriority;
    int currentPriority;
    int finalPriority;
    int isRevisit;
    Patient *patient;
    char json[2048];

    // 새 프론트는 symptoms를 보내고, 기존 단일 symptom 요청도 호환되도록 유지
    if (!getFormValue(body, "name", name, sizeof(name)) ||
        (!getFormValue(body, "symptoms", symptomsRaw, sizeof(symptomsRaw)) &&
         !getFormValue(body, "symptom", symptomsRaw, sizeof(symptomsRaw))) ||
        strlen(name) == 0 || strlen(symptomsRaw) == 0) {
        sendHttp(client, 400, "Bad Request", "application/json; charset=utf-8",
                 "{\"error\":\"name and symptom are required\"}");
        return;
    }

    /* CHANGED: 다중 증상은 raw 문자열로 우선순위를 계산하고, 표시용 문자열은 별도로 저장 */
    formatSymptomsForDisplay(symptomsRaw, symptom, sizeof(symptom));
    findLatestRecord(name, recentSymptom, sizeof(recentSymptom), &recentPriority);
    isRevisit = (recentPriority > 0);
    currentPriority = calculateSymptomsPriority(symptomsRaw);
    finalPriority = calculateFinalPriority(isRevisit, recentPriority, currentPriority);
    getCurrentTime(receptionTime, sizeof(receptionTime));
    appendMedicalRecordToCsv(name, symptom, receptionTime, finalPriority);

    // 접수 환자를 생성한 뒤 우선순위 큐에 복사해서 넣고 원본 포인터는 해제
    patient = createPatient(nextWebPatientNo, name, symptom, receptionTime,
                            isRevisit, recentSymptom, recentPriority,
                            currentPriority, finalPriority);
    nextWebPatientNo++;
    enqueueByPriority(&waitingQueue, patient);
    free(patient);

    // 접수 결과 JSON 응답 생성 전 문자열 안전 처리
    jsonEscape(name, safeName, sizeof(safeName));
    jsonEscape(symptom, safeSymptom, sizeof(safeSymptom));
    jsonEscape(recentSymptom, safeRecent, sizeof(safeRecent));
    jsonEscape(receptionTime, safeTime, sizeof(safeTime));

    snprintf(json, sizeof(json),
             "{\"name\":\"%s\",\"symptom\":\"%s\",\"time\":\"%s\","
             "\"patientNo\":%d,\"patientType\":\"%s\",\"recentRecord\":\"%s\","
             "\"recentPriority\":%d,\"currentPriority\":%d,\"finalPriority\":%d}",
             safeName, safeSymptom, safeTime, nextWebPatientNo - 1,
             isRevisit ? "재진 환자" : "신규 환자",
             safeRecent, recentPriority, currentPriority, finalPriority);

    sendHttp(client, 200, "OK", "application/json; charset=utf-8", json);
}
