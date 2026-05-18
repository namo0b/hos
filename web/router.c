/* OWNER AREA: B - Core C Backend */

#include "../include/web.h"

#include <stdio.h>
#include <string.h>

void handleClient(SOCKET client) {
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
