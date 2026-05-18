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
        printf("Winsock 珥덇린?붿뿉 ?ㅽ뙣?덉뒿?덈떎.\n");
        return 1;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("?쒕쾭 ?뚯폆??留뚮뱾 ???놁뒿?덈떎.\n");
        WSACleanup();
        return 1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons((unsigned short)port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        printf("?ы듃 %d瑜??ъ슜?????놁뒿?덈떎. ?대? ?ㅽ뻾 以묒씤吏 ?뺤씤??二쇱꽭??\n", port);
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("?붿껌 ?湲??곹깭濡??꾪솚?섏? 紐삵뻽?듬땲??\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    printf("\n???쒕쾭媛 ?ㅽ뻾?섏뿀?듬땲??\n");
    printf("釉뚮씪?곗??먯꽌 http://localhost:%d 瑜??댁뼱二쇱꽭??\n", port);
    printf("醫낅즺?섎젮硫???李쎌뿉??Ctrl+C瑜??꾨Ⅴ?몄슂.\n\n");

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
