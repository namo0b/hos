/* OWNER AREA: B - Core C Backend */

#include "../include/web.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void appendText(char *buffer, size_t size, const char *text) {
    strncat(buffer, text, size - strlen(buffer) - 1);
}

void appendFormat(char *buffer, size_t size, const char *format, ...) {
    char temp[2048];
    va_list args;

    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);

    appendText(buffer, size, temp);
}

void jsonEscape(const char *src, char *dest, int size) {
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

int getFormValue(const char *body, const char *key, char *value, int size) {
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
