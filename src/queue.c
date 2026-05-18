/* OWNER AREA: B - Core C Backend */

#include "../include/hospital.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void initPriorityQueue(PriorityQueue *queue) {
    queue->front = NULL;
}

int isHigherPriority(const Patient *a, const Patient *b) {
    int timeCompare;

    if (a->finalPriority < b->finalPriority) {
        return 1;
    }

    if (a->finalPriority > b->finalPriority) {
        return 0;
    }

    timeCompare = strcmp(a->receptionTime, b->receptionTime);
    if (timeCompare < 0) {
        return 1;
    }

    if (timeCompare > 0) {
        return 0;
    }

    return a->patientNo < b->patientNo;
}

void enqueueByPriority(PriorityQueue *queue, const Patient *patient) {
    Patient *newNode = copyPatient(patient);
    Patient *current;

    if (queue->front == NULL || isHigherPriority(newNode, queue->front)) {
        newNode->next = queue->front;
        queue->front = newNode;
        return;
    }

    current = queue->front;
    while (current->next != NULL && !isHigherPriority(newNode, current->next)) {
        current = current->next;
    }

    newNode->next = current->next;
    current->next = newNode;
}

Patient *dequeuePatient(PriorityQueue *queue) {
    Patient *removed;

    if (queue->front == NULL) {
        return NULL;
    }

    removed = queue->front;
    queue->front = queue->front->next;
    removed->next = NULL;

    return removed;
}

Patient *peekNextPatient(PriorityQueue *queue) {
    return queue->front;
}

void printQueue(PriorityQueue *queue) {
    Patient *current = queue->front;
    int count = 1;

    if (current == NULL) {
        printf("\n현재 진료 대기 환자가 없습니다.\n");
        return;
    }

    printf("\n[진료 대기열]\n");
    printLine('-', 94);
    printf("%-4s %-8s %-12s %-18s %-12s %-20s\n",
           "순번", "중요도", "이름", "증상", "구분", "접수 시간");
    printLine('-', 94);

    while (current != NULL) {
        printf("%-4d %-8d %-12s %-18s %-12s %-20s\n",
               count, current->finalPriority, current->name, current->symptom,
               current->patientType, current->receptionTime);
        current = current->next;
        count++;
    }
}

void searchPatient(PriorityQueue *queue) {
    char name[NAME_SIZE];
    Patient *current;
    int found = 0;

    readLine("\n검색할 환자 이름: ", name, sizeof(name));
    current = queue->front;

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            if (!found) {
                printf("\n[현재 대기 중인 환자 검색 결과]\n");
            }
            printPatientDetail(current);
            found = 1;
        }
        current = current->next;
    }

    if (!found) {
        printf("\n현재 대기열에서 %s 환자를 찾을 수 없습니다.\n", name);
    }
}

void freeQueue(PriorityQueue *queue) {
    freePatientList(queue->front);
    queue->front = NULL;
}
