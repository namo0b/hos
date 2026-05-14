#include "../include/hospital.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#error This simple web server is configured for Windows Winsock.
#endif

#define REQUEST_SIZE 16384
#define RESPONSE_SIZE 131072

static PriorityQueue waitingQueue;
static int nextWebPatientNo = 1;

static void appendText(char *buffer, size_t size, const char *text) {
    strncat(buffer, text, size - strlen(buffer) - 1);
}

static void appendFormat(char *buffer, size_t size, const char *format, ...) {
    char temp[2048];
    va_list args;

    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);

    appendText(buffer, size, temp);
}

static void jsonEscape(const char *src, char *dest, int size) {
    int index = 0;

    while (*src != '\0' && index < size - 1) {
        if (*src == '"' || *src == '\\') {
            if (index < size - 2) {
                dest[index++] = '\\';
                dest[index++] = *src;
            }
        } else if (*src == '\n' || *src == '\r') {
            if (index < size - 3) {
                dest[index++] = '\\';
                dest[index++] = 'n';
            }
        } else {
            dest[index++] = *src;
        }
        src++;
    }

    dest[index] = '\0';
}

static int hexValue(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return 0;
}

static void urlDecode(const char *src, char *dest, int size) {
    int index = 0;

    while (*src != '\0' && index < size - 1) {
        if (*src == '%' && src[1] != '\0' && src[2] != '\0') {
            dest[index++] = (char)(hexValue(src[1]) * 16 + hexValue(src[2]));
            src += 3;
        } else if (*src == '+') {
            dest[index++] = ' ';
            src++;
        } else {
            dest[index++] = *src;
            src++;
        }
    }

    dest[index] = '\0';
}

static int getFormValue(const char *body, const char *key, char *value, int size) {
    char searchKey[64];
    const char *start;
    const char *end;
    char encoded[512];
    int length;

    snprintf(searchKey, sizeof(searchKey), "%s=", key);
    start = strstr(body, searchKey);
    if (start == NULL) {
        value[0] = '\0';
        return 0;
    }

    start += strlen(searchKey);
    end = strchr(start, '&');
    if (end == NULL) {
        end = start + strlen(start);
    }

    length = (int)(end - start);
    if (length >= (int)sizeof(encoded)) {
        length = (int)sizeof(encoded) - 1;
    }

    strncpy(encoded, start, length);
    encoded[length] = '\0';
    urlDecode(encoded, value, size);
    return 1;
}

static const char *mimeType(const char *path) {
    const char *ext = strrchr(path, '.');

    if (ext == NULL) {
        return "text/plain; charset=utf-8";
    }
    if (strcmp(ext, ".html") == 0) {
        return "text/html; charset=utf-8";
    }
    if (strcmp(ext, ".css") == 0) {
        return "text/css; charset=utf-8";
    }
    if (strcmp(ext, ".js") == 0) {
        return "application/javascript; charset=utf-8";
    }
    if (strcmp(ext, ".svg") == 0) {
        return "image/svg+xml";
    }
    return "text/plain; charset=utf-8";
}

static void sendHttp(SOCKET client, int status, const char *statusText,
                     const char *contentType, const char *body) {
    char header[512];
    int bodyLength = (int)strlen(body);

    snprintf(header, sizeof(header),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "Cache-Control: no-store\r\n"
             "\r\n",
             status, statusText, contentType, bodyLength);

    send(client, header, (int)strlen(header), 0);
    send(client, body, bodyLength, 0);
}

static void sendFile(SOCKET client, const char *urlPath) {
    char filePath[260];
    FILE *file;
    char *body;
    long length;
    const char *localPath = urlPath;

    if (strcmp(urlPath, "/") == 0) {
        localPath = "/index.html";
    }

    if (strstr(localPath, "..") != NULL) {
        sendHttp(client, 403, "Forbidden", "text/plain; charset=utf-8", "Forbidden");
        return;
    }

    snprintf(filePath, sizeof(filePath), "public%s", localPath);
    file = fopen(filePath, "rb");
    if (file == NULL) {
        sendHttp(client, 404, "Not Found", "text/plain; charset=utf-8", "Not found");
        return;
    }

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    body = (char *)malloc((size_t)length + 1);
    if (body == NULL) {
        fclose(file);
        sendHttp(client, 500, "Internal Server Error", "text/plain; charset=utf-8", "Memory error");
        return;
    }

    fread(body, 1, (size_t)length, file);
    body[length] = '\0';
    fclose(file);

    sendHttp(client, 200, "OK", mimeType(filePath), body);
    free(body);
}

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

static void handleRecordsApi(SOCKET client) {
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

static void handleQueueApi(SOCKET client) {
    char *json = (char *)malloc(RESPONSE_SIZE);

    if (json == NULL) {
        sendHttp(client, 500, "Internal Server Error", "text/plain; charset=utf-8", "Memory error");
        return;
    }

    buildQueueJson(json, RESPONSE_SIZE);
    sendHttp(client, 200, "OK", "application/json; charset=utf-8", json);
    free(json);
}

static void handleCompleteApi(SOCKET client) {
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

static void handleRegisterApi(SOCKET client, const char *body) {
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
             isRevisit ? "재진 환자" : "신규 환자",
             safeRecent, recentPriority, currentPriority, finalPriority);

    sendHttp(client, 200, "OK", "application/json; charset=utf-8", json);
}

static void handleClient(SOCKET client) {
    char request[REQUEST_SIZE];
    char method[12];
    char path[256];
    char *body;
    char *headerEnd;
    char *contentLengthHeader;
    int contentLength = 0;
    int headerLength;
    int bodyLength;
    int received = recv(client, request, sizeof(request) - 1, 0);

    if (received <= 0) {
        return;
    }

    request[received] = '\0';

    headerEnd = strstr(request, "\r\n\r\n");
    contentLengthHeader = strstr(request, "Content-Length:");
    if (contentLengthHeader != NULL) {
        sscanf(contentLengthHeader, "Content-Length: %d", &contentLength);
    }

    if (headerEnd != NULL && contentLength > 0) {
        headerLength = (int)((headerEnd + 4) - request);
        bodyLength = received - headerLength;

        while (bodyLength < contentLength && received < (int)sizeof(request) - 1) {
            int more = recv(client, request + received,
                            (int)sizeof(request) - received - 1, 0);
            if (more <= 0) {
                break;
            }
            received += more;
            request[received] = '\0';
            bodyLength = received - headerLength;
        }
    }

    sscanf(request, "%11s %255s", method, path);

    body = strstr(request, "\r\n\r\n");
    if (body != NULL) {
        body += 4;
    } else {
        body = "";
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/records") == 0) {
        handleRecordsApi(client);
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/queue") == 0) {
        handleQueueApi(client);
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/register") == 0) {
        handleRegisterApi(client, body);
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/complete") == 0) {
        handleCompleteApi(client);
    } else if (strcmp(method, "GET") == 0) {
        sendFile(client, path);
    } else {
        sendHttp(client, 405, "Method Not Allowed", "text/plain; charset=utf-8", "Method not allowed");
    }
}

int runWebServer(int port) {
    WSADATA wsaData;
    SOCKET serverSocket;
    struct sockaddr_in serverAddress;

    initPriorityQueue(&waitingQueue);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Winsock 초기화에 실패했습니다.\n");
        return 1;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("서버 소켓을 만들 수 없습니다.\n");
        WSACleanup();
        return 1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons((unsigned short)port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        printf("포트 %d를 사용할 수 없습니다. 이미 실행 중인지 확인해 주세요.\n", port);
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("요청 대기 상태로 전환하지 못했습니다.\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    printf("\n웹 서버가 실행되었습니다.\n");
    printf("브라우저에서 http://localhost:%d 를 열어주세요.\n", port);
    printf("종료하려면 이 창에서 Ctrl+C를 누르세요.\n\n");

    while (1) {
        SOCKET client = accept(serverSocket, NULL, NULL);
        if (client == INVALID_SOCKET) {
            continue;
        }
        handleClient(client);
        closesocket(client);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
