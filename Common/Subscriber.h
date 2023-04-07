#pragma once

#include "Connection.h"
#include "Structs.h"
#include <conio.h>

#define DEFAULT_BUFLEN 524
#define DEFAULT_PORT 27018

#define SAFE_DELETE_HANDLE(a) if(a){CloseHandle(a);} 

SOCKET connectSocket = INVALID_SOCKET;

CRITICAL_SECTION criticalSectionForInput;

HANDLE FinishSignal, InputSignal;

HANDLE t1;
DWORD thread1ID;

PORUKA* poruka;
char recvbuf[DEFAULT_BUFLEN];

void InitAllCriticalSections() {
    InitializeCriticalSection(&criticalSectionForInput);
}

void DeleteAllCriticalSections() {
    DeleteCriticalSection(&criticalSectionForInput);
}

void CreateSemaphores() {
    FinishSignal = CreateSemaphore(0, 0, 1, NULL);
    InputSignal = CreateSemaphore(0, 0, 1, NULL);
}

void DeleteAllThreadsAndSemaphores() {
    SAFE_DELETE_HANDLE(t1);
    SAFE_DELETE_HANDLE(FinishSignal);
    SAFE_DELETE_HANDLE(InputSignal);
}

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

void Subscribe(void* topic) {

    char* poruka = (char*)topic;

    int iResult = send(connectSocket, poruka, strlen(poruka), 0);

    if (iResult == SOCKET_ERROR)
    {
        printf("send failed with error: %d\n", WSAGetLastError());
    }

    printf("Bytes Sent: %ld\n", iResult);

}


DWORD WINAPI Thread1(LPVOID param) {

    while (WaitForSingleObject(FinishSignal, 500) == WAIT_TIMEOUT) {

        if (_kbhit()) {
            EnterCriticalSection(&criticalSectionForInput);
            char ch = _getch();
            LeaveCriticalSection(&criticalSectionForInput);
            if (ch == 'x') {

                EnterCriticalSection(&criticalSectionForInput);
                char topic[16];
                do {
                    printf("Unesi topic za pretplatu: ");
                    gets_s(topic, sizeof(topic));

                } while (strlen(topic) == 0);
                Subscribe((void*)topic);
                LeaveCriticalSection(&criticalSectionForInput);
            }
            else if (ch == 'q') {
                ReleaseSemaphore(InputSignal, 1, NULL);
                break;
            }

        }
    }
    closesocket(connectSocket);
    WSACleanup();
    return 0;
}



void ReceiveMessages(){
    int iResult;

    do {

        iResult = recv(connectSocket, recvbuf, (int)(sizeof(PORUKA)), 0);
        if (iResult > 0)
        {
            poruka = (PORUKA*)recvbuf;

            EnterCriticalSection(&criticalSectionForInput);
            printf("Naslov: %s\n", poruka->topic);
            printf("Tekst: %s \n", poruka->text);
            LeaveCriticalSection(&criticalSectionForInput);
        }
        else if (iResult == 0)
        {
            EnterCriticalSection(&criticalSectionForInput);
            printf("Connection with PubSubEngine closed.\n");
            LeaveCriticalSection(&criticalSectionForInput);
            closesocket(connectSocket);
            ReleaseSemaphore(FinishSignal, 1, NULL);
            break;
        }
        else
        {
            EnterCriticalSection(&criticalSectionForInput);
            printf("PubSunEngine finished. Closing socket....");
            LeaveCriticalSection(&criticalSectionForInput);
            closesocket(connectSocket);
            ReleaseSemaphore(FinishSignal, 1, NULL);
            break;
        }
    } while (WaitForSingleObject(InputSignal, 200) == WAIT_TIMEOUT);
    return;
}



void CreateThreadsSubscriber() {
    t1 = CreateThread(NULL, 0, &Thread1, 0, 0, &thread1ID);
}