/* OWNER AREA: B - Core C Backend */

#include "../include/web.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void sendFile(SOCKET client, const char *urlPath) {
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
