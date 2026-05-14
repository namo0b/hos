#include "../include/hospital.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Patient *createPatient(int patientNo, const char *name, const char *symptom,
                       const char *receptionTime, int isRevisit,
                       const char *recentRecord, int recentPriority,
                       int currentPriority, int finalPriority) {
    Patient *patient = (Patient *)malloc(sizeof(Patient));

    if (patient == NULL) {
        printf("메모리 할당에 실패했습니다.\n");
        exit(1);
    }

    patient->patientNo = patientNo;
    strcpy(patient->name, name);
    strcpy(patient->symptom, symptom);
    strcpy(patient->receptionTime, receptionTime);
    strcpy(patient->patientType, isRevisit ? "재진 환자" : "신규 환자");
    strcpy(patient->recentRecord, recentRecord);
    patient->recentRecordPriority = recentPriority;
    patient->currentSymptomPriority = currentPriority;
    patient->finalPriority = finalPriority;
    patient->next = NULL;

    return patient;
}

Patient *copyPatient(const Patient *src) {
    return createPatient(src->patientNo, src->name, src->symptom,
                         src->receptionTime,
                         strcmp(src->patientType, "재진 환자") == 0,
                         src->recentRecord, src->recentRecordPriority,
                         src->currentSymptomPriority, src->finalPriority);
}

void printPatientDetail(const Patient *patient) {
    printLine('-', 54);
    printf("환자 번호        : %d\n", patient->patientNo);
    printf("이름             : %s\n", patient->name);
    printf("증상             : %s\n", patient->symptom);
    printf("환자 구분        : %s\n", patient->patientType);
    printf("접수 시간        : %s\n", patient->receptionTime);
    printf("최근 진료 기록   : %s\n", patient->recentRecord);

    if (patient->recentRecordPriority == 0) {
        printf("최근 기록 중요도 : 없음\n");
    } else {
        printf("최근 기록 중요도 : %d단계\n", patient->recentRecordPriority);
    }

    printf("현재 증상 중요도 : %d단계\n", patient->currentSymptomPriority);
    printf("최종 중요도      : %d단계\n", patient->finalPriority);
    printLine('-', 54);
}

void receivePatient(TreeNode *root, PriorityQueue *queue,
                    PatientRecordStack **recordHead, int *nextPatientNo) {
    char name[NAME_SIZE];
    char symptom[SYMPTOM_SIZE];
    char receptionTime[TIME_SIZE];
    char recentRecord[SYMPTOM_SIZE] = EMPTY_TEXT;
    int recentPriority = 0;
    int currentPriority;
    int finalPriority;
    int isRevisit;
    PatientRecordStack *stack;
    MedicalRecord *latestRecord;
    Patient *patient;

    printf("\n[환자 접수]\n");
    readLine("환자 이름: ", name, sizeof(name));
    readLine("증상 입력: ", symptom, sizeof(symptom));

    printf("\n기존 진료 기록을 검색합니다.\n");

    stack = findPatientRecordStack(*recordHead, name);
    latestRecord = peekLatestMedicalRecord(stack);
    isRevisit = (latestRecord != NULL);

    if (isRevisit) {
        strcpy(recentRecord, latestRecord->symptom);
        recentPriority = latestRecord->priority;
        printf("%s 환자의 기존 진료 기록을 찾았습니다.\n", name);
        printf("최근 기록: %s / 중요도 %d단계\n", recentRecord, recentPriority);
    } else {
        printf("%s 환자의 기존 진료 기록이 없습니다. 신규 환자로 분류합니다.\n", name);
    }

    currentPriority = calculateSymptomPriority(symptom);
    finalPriority = calculateFinalPriority(isRevisit, recentPriority, currentPriority);
    getCurrentTime(receptionTime, sizeof(receptionTime));

    patient = createPatient(*nextPatientNo, name, symptom, receptionTime,
                            isRevisit, recentRecord, recentPriority,
                            currentPriority, finalPriority);
    (*nextPatientNo)++;

    classifyPatientByTree(root, patient, isRevisit);
    enqueueByPriority(queue, patient);

    stack = getOrCreatePatientRecordStack(recordHead, name);
    pushMedicalRecord(stack, name, symptom, receptionTime, finalPriority);
    appendMedicalRecordToCsv(name, symptom, receptionTime, finalPriority);

    printf("\n접수가 완료되었습니다.\n");
    printPatientDetail(patient);
    printf("진료 대기열과 %s에 기록을 저장했습니다.\n", RECORD_FILE);

    free(patient);
}

void freePatientList(Patient *head) {
    Patient *current = head;
    Patient *next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}
