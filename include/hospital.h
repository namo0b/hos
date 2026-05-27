#ifndef HOSPITAL_H
#define HOSPITAL_H

/* OWNER AREA: B - Core C Backend */

#include <stdio.h>

#define NAME_SIZE 50
#define SYMPTOM_SIZE 256
#define TIME_SIZE 30
#define TYPE_SIZE 20
#define CATEGORY_SIZE 30
#define EMPTY_TEXT "없음"
#define DATA_DIR "data"
#define RECORD_FILE "data/patient_records.csv"

typedef struct Patient {
    int patientNo;
    char name[NAME_SIZE];
    char symptom[SYMPTOM_SIZE];
    char receptionTime[TIME_SIZE];
    char patientType[TYPE_SIZE];
    char recentRecord[SYMPTOM_SIZE];
    int recentRecordPriority;
    int currentSymptomPriority;
    int finalPriority;
    struct Patient *next;
} Patient;

typedef struct MedicalRecord {
    char patientName[NAME_SIZE];
    char symptom[SYMPTOM_SIZE];
    char treatmentTime[TIME_SIZE];
    int priority;
    struct MedicalRecord *next;
} MedicalRecord;

typedef struct PatientRecordStack {
    char patientName[NAME_SIZE];
    MedicalRecord *top;
    struct PatientRecordStack *next;
} PatientRecordStack;

typedef struct TreeNode {
    char categoryName[CATEGORY_SIZE];
    Patient *patientList;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

typedef struct PriorityQueue {
    Patient *front;
} PriorityQueue;

void removeNewline(char *str);
void readLine(const char *message, char *buffer, int size);
int readMenuChoice(void);
void getCurrentTime(char *buffer, int size);
int calculateSymptomPriority(const char *symptom);
int calculateSymptomsPriority(const char *symptoms);
int calculateFinalPriority(int isRevisit, int recentPriority, int currentPriority);
void ensureDataDirectory(void);

void printLine(char ch, int count);
void printMainMenu(void);
void printSymptomGuide(void);
void printRecordLoadMessage(int count);

Patient *createPatient(int patientNo, const char *name, const char *symptom,
                       const char *receptionTime, int isRevisit,
                       const char *recentRecord, int recentPriority,
                       int currentPriority, int finalPriority);
Patient *copyPatient(const Patient *src);
void printPatientDetail(const Patient *patient);
void receivePatient(TreeNode *root, PriorityQueue *queue,
                    PatientRecordStack **recordHead, int *nextPatientNo);
void freePatientList(Patient *head);

PatientRecordStack *findPatientRecordStack(PatientRecordStack *head, const char *name);
PatientRecordStack *createPatientRecordStack(const char *name);
PatientRecordStack *getOrCreatePatientRecordStack(PatientRecordStack **head, const char *name);
void pushMedicalRecord(PatientRecordStack *stack, const char *name,
                       const char *symptom, const char *timeText, int priority);
MedicalRecord *peekLatestMedicalRecord(PatientRecordStack *stack);
void printMedicalRecordStack(PatientRecordStack *head);
int loadMedicalRecordsFromCsv(PatientRecordStack **recordHead);
void appendMedicalRecordToCsv(const char *name, const char *symptom,
                              const char *timeText, int priority);
void writeCsvField(FILE *file, const char *text);
int readCsvField(char **cursor, char *buffer, int size);
void freeMedicalRecordStack(PatientRecordStack *head);

TreeNode *createTreeNode(const char *categoryName);
TreeNode *createClassificationTree(void);
void classifyPatientByTree(TreeNode *root, const Patient *patient, int isRevisit);
void appendPatient(Patient **head, const Patient *patient);
void printTree(TreeNode *root);
void printTreePatientList(Patient *head, int isRevisit);
void freeTree(TreeNode *root);

void initPriorityQueue(PriorityQueue *queue);
int isHigherPriority(const Patient *a, const Patient *b);
void enqueueByPriority(PriorityQueue *queue, const Patient *patient);
Patient *dequeuePatient(PriorityQueue *queue);
Patient *peekNextPatient(PriorityQueue *queue);
void printQueue(PriorityQueue *queue);
void searchPatient(PriorityQueue *queue);
void freeQueue(PriorityQueue *queue);

int runWebServer(int port);

#endif
