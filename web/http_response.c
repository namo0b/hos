/* OWNER AREA: B - Core C Backend */

#include "../include/web.h"

#include <stdio.h>
#include <string.h>

void sendHttp(SOCKET client, int status, const char *statusText,
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
