/* OWNER AREA: B - Core C Backend */

#include "../include/hospital.h"

#include <stdio.h>

void printLine(char ch, int count) {
    int i;

    for (i = 0; i < count; i++) {
        putchar(ch);
    }
    putchar('\n');
}

void printMainMenu(void) {
    printf("\n");
    printLine('=', 54);
    printf("          응급실 환자 접수 및 진료 관리 시스템\n");
    printLine('=', 54);
    printf("  1. 환자 접수\n");
    printf("  2. 환자 분류 트리 출력\n");
    printf("  3. 진료 대기열 출력\n");
    printf("  4. 다음 진료 환자 확인\n");
    printf("  5. 진료 완료 처리\n");
    printf("  6. 대기 환자 검색\n");
    printf("  7. 환자별 진료 기록 조회\n");
    printf("  8. 증상별 우선순위 안내\n");
    printf("  0. 프로그램 종료\n");
    printLine('-', 54);
}

void printSymptomGuide(void) {
    printf("\n[증상별 우선순위 안내]\n");
    printLine('-', 54);
    printf("1단계: 심정지, 의식 없음, 호흡곤란, 심한 흉통\n");
    printf("2단계: 고열, 심한 복통, 출혈, 골절 의심\n");
    printf("3단계: 어지러움, 두통, 구토, 중등도 통증\n");
    printf("4단계: 감기, 기침, 가벼운 복통, 기타 증상\n");
    printf("5단계: 단순 상담, 약 처방 요청, 경미한 증상\n");
}

void printRecordLoadMessage(int count) {
    if (count > 0) {
        printf("\n기존 진료 기록 %d건을 %s에서 불러왔습니다.\n", count, RECORD_FILE);
    } else {
        printf("\n불러올 기존 진료 기록이 없습니다. 새 기록은 %s에 저장됩니다.\n", RECORD_FILE);
    }
}
