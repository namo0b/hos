/* OWNER AREA: A - Frontend + Web Integration */

#include "../include/web.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void buildRecordsJson(char *json, size_t size) {
    FILE *file = fopen(RECORD_FILE, "r");
    char line[512];
    int first = 1;

    strcpy(json, "{\"records\":[");

    if (file == NULL) {
        appendText(json, size, "]}");
        return;
    }

    fgets(line, sizeof(line), file);

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

        if (!readCsvField(&cursor, name, sizeof(name)) ||
            !readCsvField(&cursor, symptom, sizeof(symptom)) ||
            !readCsvField(&cursor, timeText, sizeof(timeText)) ||
            !readCsvField(&cursor, priorityText, sizeof(priorityText))) {
            continue;
        }

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

static void findLatestRecord(const char *targetName, char *recentSymptom,
                             int symptomSize, int *recentPriority) {
    FILE *file = fopen(RECORD_FILE, "r");
    char line[512];

    strcpy(recentSymptom, EMPTY_TEXT);
    *recentPriority = 0;

    if (file == NULL) {
        return;
    }

    fgets(line, sizeof(line), file);

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

        if (strcmp(name, targetName) == 0) {
            strncpy(recentSymptom, symptom, symptomSize - 1);
            recentSymptom[symptomSize - 1] = '\0';
            *recentPriority = atoi(priorityText);
        }
    }

    fclose(file);
}

void handleRecordsApi(SOCKET client) {
    char *json = (char *)malloc(RESPONSE_SIZE);

    if (json == NULL) {
        sendHttp(client, 500, "Internal Server Error", "text/plain; charset=utf-8", "Memory error");
        return;
    }

    buildRecordsJson(json, RESPONSE_SIZE);
    sendHttp(client, 200, "OK", "application/json; charset=utf-8", json);
    free(json);
}

static void buildQueueJson(char *json, size_t size) {
    Patient *current = waitingQueue.front;
    int first = 1;

    strcpy(json, "{\"queue\":[");

    while (current != NULL) {
        char safeName[NAME_SIZE * 2];
        char safeSymptom[SYMPTOM_SIZE * 2];
        char safeTime[TIME_SIZE * 2];
        char safeType[TYPE_SIZE * 2];
        char safeRecent[SYMPTOM_SIZE * 2];

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

void handleQueueApi(SOCKET client) {
    char *json = (char *)malloc(RESPONSE_SIZE);

    if (json == NULL) {
        sendHttp(client, 500, "Internal Server Error", "text/plain; charset=utf-8", "Memory error");
        return;
    }

    buildQueueJson(json, RESPONSE_SIZE);
    sendHttp(client, 200, "OK", "application/json; charset=utf-8", json);
    free(json);
}

void handleCompleteApi(SOCKET client) {
    Patient *done = dequeuePatient(&waitingQueue);
    char json[2048];
    char safeName[NAME_SIZE * 2];
    char safeSymptom[SYMPTOM_SIZE * 2];
    char safeTime[TIME_SIZE * 2];

    if (done == NULL) {
        sendHttp(client, 200, "OK", "application/json; charset=utf-8",
                 "{\"completed\":false,\"message\":\"waiting queue is empty\"}");
        return;
    }

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

void handleRegisterApi(SOCKET client, const char *body) {
    char name[NAME_SIZE];
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

    if (!getFormValue(body, "name", name, sizeof(name)) ||
        !getFormValue(body, "symptom", symptom, sizeof(symptom)) ||
        strlen(name) == 0 || strlen(symptom) == 0) {
        sendHttp(client, 400, "Bad Request", "application/json; charset=utf-8",
                 "{\"error\":\"name and symptom are required\"}");
        return;
    }

    findLatestRecord(name, recentSymptom, sizeof(recentSymptom), &recentPriority);
    isRevisit = (recentPriority > 0);
    currentPriority = calculateSymptomPriority(symptom);
    finalPriority = calculateFinalPriority(isRevisit, recentPriority, currentPriority);
    getCurrentTime(receptionTime, sizeof(receptionTime));
    appendMedicalRecordToCsv(name, symptom, receptionTime, finalPriority);

    patient = createPatient(nextWebPatientNo, name, symptom, receptionTime,
                            isRevisit, recentSymptom, recentPriority,
                            currentPriority, finalPriority);
    nextWebPatientNo++;
    enqueueByPriority(&waitingQueue, patient);
    free(patient);

    jsonEscape(name, safeName, sizeof(safeName));
    jsonEscape(symptom, safeSymptom, sizeof(safeSymptom));
    jsonEscape(recentSymptom, safeRecent, sizeof(safeRecent));
    jsonEscape(receptionTime, safeTime, sizeof(safeTime));

    snprintf(json, sizeof(json),
             "{\"name\":\"%s\",\"symptom\":\"%s\",\"time\":\"%s\","
             "\"patientNo\":%d,\"patientType\":\"%s\",\"recentRecord\":\"%s\","
             "\"recentPriority\":%d,\"currentPriority\":%d,\"finalPriority\":%d}",
             safeName, safeSymptom, safeTime, nextWebPatientNo - 1,
             isRevisit ? "?ъ쭊 ?섏옄" : "?좉퇋 ?섏옄",
             safeRecent, recentPriority, currentPriority, finalPriority);

    sendHttp(client, 200, "OK", "application/json; charset=utf-8", json);
}
