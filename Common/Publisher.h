#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "Structs.h"

#include "Connection.h"
#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016

SOCKET connectSocket = INVALID_SOCKET;

int Connect() {

    connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(DEFAULT_PORT);
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    return 0;
}

int Publish(void* topic, void* messageToSend) {
    PORUKA poruka;
    strcpy_s(poruka.topic, (char*)topic);
    strcpy_s(poruka.text, (char*)messageToSend);

    int iResult = send(connectSocket, (char*)&poruka, (int)sizeof(PORUKA), 0);
    if (iResult == SOCKET_ERROR)
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return -1;
    }
    return iResult;
}
