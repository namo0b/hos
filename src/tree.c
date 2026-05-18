/* OWNER AREA: B - Core C Backend */

#include "../include/hospital.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TreeNode *createTreeNode(const char *categoryName) {
    TreeNode *node = (TreeNode *)malloc(sizeof(TreeNode));

    if (node == NULL) {
        printf("메모리 할당에 실패했습니다.\n");
        exit(1);
    }

    strcpy(node->categoryName, categoryName);
    node->patientList = NULL;
    node->left = NULL;
    node->right = NULL;

    return node;
}

TreeNode *createClassificationTree(void) {
    TreeNode *root = createTreeNode("전체 환자");

    root->left = createTreeNode("신규 환자");
    root->right = createTreeNode("재진 환자");

    return root;
}

void classifyPatientByTree(TreeNode *root, const Patient *patient, int isRevisit) {
    if (root == NULL) {
        return;
    }

    if (isRevisit) {
        appendPatient(&(root->right->patientList), patient);
    } else {
        appendPatient(&(root->left->patientList), patient);
    }
}

void appendPatient(Patient **head, const Patient *patient) {
    Patient *newNode = copyPatient(patient);
    Patient *current;

    if (*head == NULL) {
        *head = newNode;
        return;
    }

    current = *head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = newNode;
}

void printTree(TreeNode *root) {
    if (root == NULL) {
        return;
    }

    printf("\n[%s 분류]\n", root->categoryName);
    printLine('-', 54);
    printf("신규 환자\n");
    printTreePatientList(root->left->patientList, 0);
    printf("\n재진 환자\n");
    printTreePatientList(root->right->patientList, 1);
}

void printTreePatientList(Patient *head, int isRevisit) {
    Patient *current = head;
    int count = 1;

    if (current == NULL) {
        printf("  등록된 환자가 없습니다.\n");
        return;
    }

    while (current != NULL) {
        if (isRevisit) {
            printf("  %d. %s / %s / 최근 기록: %s / 중요도 %d단계\n",
                   count, current->name, current->symptom,
                   current->recentRecord, current->finalPriority);
        } else {
            printf("  %d. %s / %s / 중요도 %d단계\n",
                   count, current->name, current->symptom, current->finalPriority);
        }
        current = current->next;
        count++;
    }
}

void freeTree(TreeNode *root) {
    if (root == NULL) {
        return;
    }

    freeTree(root->left);
    freeTree(root->right);
    freePatientList(root->patientList);
    free(root);
}
