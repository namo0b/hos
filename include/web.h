#ifndef HOSPITAL_WEB_H
#define HOSPITAL_WEB_H

/* OWNER AREA: A - Frontend + Web Integration */

// 웹 서버 쪽 C 파일들이 공통으로 참고하는 함수 선언 헤더 파일

#include "hospital.h"

#include <stddef.h>

#ifdef _WIN32 // Windows 소켓 API 사용
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#error This simple web server is configured for Windows Winsock.
#endif

#define REQUEST_SIZE 16384
#define RESPONSE_SIZE 131072
// HTTP 요청과 JSON 응답을 담기 위한 최대 버퍼 크기

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

#endif
