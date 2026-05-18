/* OWNER AREA: B - Core C Backend */

#include "../include/hospital.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void removeNewline(char *str) {
    int len = (int)strlen(str);

    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

void readLine(const char *message, char *buffer, int size) {
    while (1) {
        printf("%s", message);
        fflush(stdout);

        if (fgets(buffer, size, stdin) == NULL) {
            buffer[0] = '\0';
            return;
        }

        removeNewline(buffer);

        if (strlen(buffer) > 0) {
            return;
        }

        printf("입력값이 비어 있습니다. 다시 입력해 주세요.\n");
    }
}

int readMenuChoice(void) {
    char buffer[20];

    printf("메뉴 선택: ");
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return -1;
    }

    return atoi(buffer);
}

void getCurrentTime(char *buffer, int size) {
    time_t now = time(NULL);
    struct tm timeInfo;
    struct tm *timePtr;

    timePtr = localtime(&now);
    if (timePtr == NULL) {
        strcpy(buffer, "시간 오류");
        return;
    }

    timeInfo = *timePtr;
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &timeInfo);
}

int calculateSymptomPriority(const char *symptom) {
    if (strcmp(symptom, "심정지") == 0 ||
        strcmp(symptom, "의식 없음") == 0 ||
        strcmp(symptom, "호흡곤란") == 0 ||
        strcmp(symptom, "심한 흉통") == 0) {
        return 1;
    }

    if (strcmp(symptom, "고열") == 0 ||
        strcmp(symptom, "심한 복통") == 0 ||
        strcmp(symptom, "출혈") == 0 ||
        strcmp(symptom, "골절 의심") == 0) {
        return 2;
    }

    if (strcmp(symptom, "어지러움") == 0 ||
        strcmp(symptom, "두통") == 0 ||
        strcmp(symptom, "구토") == 0 ||
        strcmp(symptom, "중등도 통증") == 0) {
        return 3;
    }

    if (strcmp(symptom, "감기") == 0 ||
        strcmp(symptom, "기침") == 0 ||
        strcmp(symptom, "가벼운 복통") == 0) {
        return 4;
    }

    if (strcmp(symptom, "단순 상담") == 0 ||
        strcmp(symptom, "약 처방 요청") == 0 ||
        strcmp(symptom, "경미한 증상") == 0) {
        return 5;
    }

    return 4;
}

int calculateFinalPriority(int isRevisit, int recentPriority, int currentPriority) {
    if (isRevisit && recentPriority < currentPriority) {
        return recentPriority;
    }

    return currentPriority;
}

void ensureDataDirectory(void) {
#ifdef _WIN32
    _mkdir(DATA_DIR);
#else
    mkdir(DATA_DIR, 0755);
#endif
}
