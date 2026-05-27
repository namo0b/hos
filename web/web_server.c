/* OWNER AREA: B - Core C Backend */

#include "../include/web.h"

#include <stdio.h>

PriorityQueue waitingQueue;
int nextWebPatientNo = 1;

int runWebServer(int port) {
    WSADATA wsaData;
    SOCKET serverSocket;
    struct sockaddr_in serverAddress;

    initPriorityQueue(&waitingQueue);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Winsock 초기화에 실패했습니다.\n");
        return 1;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("서버 소켓을 만들 수 없습니다.\n");
        WSACleanup();
        return 1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons((unsigned short)port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        printf("포트 %d를 사용할 수 없습니다. 이미 실행 중인지 확인해 주세요.\n", port);
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("요청 대기 상태로 전환하지 못했습니다.\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    printf("\n웹 서버가 실행되었습니다.\n");
    printf("브라우저에서 http://localhost:%d 를 열어 주세요.\n", port);
    printf("종료하려면 이 창에서 Ctrl+C를 누르세요.\n\n");

    while (1) {
        SOCKET client = accept(serverSocket, NULL, NULL);
        if (client == INVALID_SOCKET) {
            continue;
        }
        handleClient(client);
        closesocket(client);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
