#include "../include/hospital.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PatientRecordStack *findPatientRecordStack(PatientRecordStack *head, const char *name) {
    PatientRecordStack *current = head;

    while (current != NULL) {
        if (strcmp(current->patientName, name) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

PatientRecordStack *createPatientRecordStack(const char *name) {
    PatientRecordStack *stack = (PatientRecordStack *)malloc(sizeof(PatientRecordStack));

    if (stack == NULL) {
        printf("메모리 할당에 실패했습니다.\n");
        exit(1);
    }

    strcpy(stack->patientName, name);
    stack->top = NULL;
    stack->next = NULL;

    return stack;
}

PatientRecordStack *getOrCreatePatientRecordStack(PatientRecordStack **head, const char *name) {
    PatientRecordStack *stack = findPatientRecordStack(*head, name);

    if (stack != NULL) {
        return stack;
    }

    stack = createPatientRecordStack(name);
    stack->next = *head;
    *head = stack;

    return stack;
}

void pushMedicalRecord(PatientRecordStack *stack, const char *name,
                       const char *symptom, const char *timeText, int priority) {
    MedicalRecord *record = (MedicalRecord *)malloc(sizeof(MedicalRecord));

    if (record == NULL) {
        printf("메모리 할당에 실패했습니다.\n");
        exit(1);
    }

    strcpy(record->patientName, name);
    strcpy(record->symptom, symptom);
    strcpy(record->treatmentTime, timeText);
    record->priority = priority;
    record->next = stack->top;
    stack->top = record;
}

MedicalRecord *peekLatestMedicalRecord(PatientRecordStack *stack) {
    if (stack == NULL) {
        return NULL;
    }

    return stack->top;
}

void printMedicalRecordStack(PatientRecordStack *head) {
    char name[NAME_SIZE];
    PatientRecordStack *stack;
    MedicalRecord *current;
    int count = 1;

    readLine("\n조회할 환자 이름: ", name, sizeof(name));
    stack = findPatientRecordStack(head, name);

    if (stack == NULL || stack->top == NULL) {
        printf("\n%s 환자의 진료 기록이 없습니다.\n", name);
        return;
    }

    printf("\n[%s 환자 진료 기록]\n", name);
    printLine('-', 74);
    printf("%-4s %-20s %-30s %-8s\n", "번호", "진료 시간", "증상", "중요도");
    printLine('-', 74);

    current = stack->top;
    while (current != NULL) {
        printf("%-4d %-20s %-30s %d단계\n",
               count, current->treatmentTime, current->symptom, current->priority);
        current = current->next;
        count++;
    }
}

int loadMedicalRecordsFromCsv(PatientRecordStack **recordHead) {
    FILE *file = fopen(RECORD_FILE, "r");
    char line[512];
    int count = 0;

    if (file == NULL) {
        return 0;
    }

    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return 0;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char *cursor = line;
        char name[NAME_SIZE];
        char symptom[SYMPTOM_SIZE];
        char timeText[TIME_SIZE];
        char priorityText[20];
        int priority;
        PatientRecordStack *stack;

        removeNewline(line);

        if (!readCsvField(&cursor, name, sizeof(name)) ||
            !readCsvField(&cursor, symptom, sizeof(symptom)) ||
            !readCsvField(&cursor, timeText, sizeof(timeText)) ||
            !readCsvField(&cursor, priorityText, sizeof(priorityText))) {
            continue;
        }

        priority = atoi(priorityText);
        if (priority <= 0) {
            continue;
        }

        stack = getOrCreatePatientRecordStack(recordHead, name);
        pushMedicalRecord(stack, name, symptom, timeText, priority);
        count++;
    }

    fclose(file);
    return count;
}

void appendMedicalRecordToCsv(const char *name, const char *symptom,
                              const char *timeText, int priority) {
    FILE *checkFile = fopen(RECORD_FILE, "r");
    FILE *file;
    int needHeader = (checkFile == NULL);

    if (checkFile != NULL) {
        fclose(checkFile);
    }

    file = fopen(RECORD_FILE, "a");
    if (file == NULL) {
        printf("\n경고: %s 파일을 열 수 없어 진료 기록을 저장하지 못했습니다.\n", RECORD_FILE);
        return;
    }

    if (needHeader) {
        fprintf(file, "name,symptom,treatment_time,priority\n");
    }

    writeCsvField(file, name);
    fprintf(file, ",");
    writeCsvField(file, symptom);
    fprintf(file, ",");
    writeCsvField(file, timeText);
    fprintf(file, ",%d\n", priority);

    fclose(file);
}

void writeCsvField(FILE *file, const char *text) {
    const char *current;
    int needsQuote = 0;

    for (current = text; *current != '\0'; current++) {
        if (*current == ',' || *current == '"' || *current == '\n' || *current == '\r') {
            needsQuote = 1;
            break;
        }
    }

    if (!needsQuote) {
        fprintf(file, "%s", text);
        return;
    }

    fputc('"', file);
    for (current = text; *current != '\0'; current++) {
        if (*current == '"') {
            fputc('"', file);
        }
        fputc(*current, file);
    }
    fputc('"', file);
}

int readCsvField(char **cursor, char *buffer, int size) {
    int index = 0;
    char *current = *cursor;

    if (current == NULL || *current == '\0') {
        buffer[0] = '\0';
        return 0;
    }

    if (*current == '"') {
        current++;
        while (*current != '\0') {
            if (*current == '"' && *(current + 1) == '"') {
                if (index < size - 1) {
                    buffer[index++] = '"';
                }
                current += 2;
            } else if (*current == '"') {
                current++;
                break;
            } else {
                if (index < size - 1) {
                    buffer[index++] = *current;
                }
                current++;
            }
        }
    } else {
        while (*current != '\0' && *current != ',') {
            if (index < size - 1) {
                buffer[index++] = *current;
            }
            current++;
        }
    }

    buffer[index] = '\0';

    if (*current == ',') {
        current++;
    }

    *cursor = current;
    return 1;
}

void freeMedicalRecordStack(PatientRecordStack *head) {
    PatientRecordStack *currentStack = head;
    PatientRecordStack *nextStack;
    MedicalRecord *currentRecord;
    MedicalRecord *nextRecord;

    while (currentStack != NULL) {
        currentRecord = currentStack->top;

        while (currentRecord != NULL) {
            nextRecord = currentRecord->next;
            free(currentRecord);
            currentRecord = nextRecord;
        }

        nextStack = currentStack->next;
        free(currentStack);
        currentStack = nextStack;
    }
}
