#define _CRT_SECURE_NO_WARNINGS

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include <stdio.h>
#include <stdlib.h>

#include "include/hospital.h"

int main(void) {
    TreeNode *root;
    PriorityQueue queue;
    PatientRecordStack *recordHead = NULL;
    int nextPatientNo = 1;
    int choice;
    int loadedCount;

#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    ensureDataDirectory();
    root = createClassificationTree();
    initPriorityQueue(&queue);
    loadedCount = loadMedicalRecordsFromCsv(&recordHead);
    printRecordLoadMessage(loadedCount);

    while (1) {
        printMainMenu();
        choice = readMenuChoice();

        switch (choice) {
        case 1:
            receivePatient(root, &queue, &recordHead, &nextPatientNo);
            break;
        case 2:
            printTree(root);
            break;
        case 3:
            printQueue(&queue);
            break;
        case 4: {
            Patient *next = peekNextPatient(&queue);
            if (next == NULL) {
                printf("\n현재 진료 대기 환자가 없습니다.\n");
            } else {
                printf("\n[다음 진료 환자]\n");
                printPatientDetail(next);
            }
            break;
        }
        case 5: {
            Patient *done = dequeuePatient(&queue);
            if (done == NULL) {
                printf("\n진료 완료 처리할 환자가 없습니다.\n");
            } else {
                printf("\n[진료 완료 처리]\n");
                printPatientDetail(done);
                free(done);
            }
            break;
        }
        case 6:
            searchPatient(&queue);
            break;
        case 7:
            printMedicalRecordStack(recordHead);
            break;
        case 8:
            printSymptomGuide();
            break;
        case 0:
            freeQueue(&queue);
            freeMedicalRecordStack(recordHead);
            freeTree(root);
            printf("\n프로그램을 종료합니다.\n");
            return 0;
        default:
            printf("\n잘못된 메뉴입니다. 다시 선택해 주세요.\n");
        }
    }
}
