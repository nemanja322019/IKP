#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "List.h"
#include "Connection.h"


#include "RingBuffer.h"

#pragma comment(lib, "ws2_32.lib")

#define SAFE_DELETE_HANDLE(a) if(a){CloseHandle(a);}
#define DEFAULT_BUFLEN 512

#define PUBSUB_PORT1 "27016"
#define PUBSUB_PORT2 27017

char recvbuf[DEFAULT_BUFLEN];

CRITICAL_SECTION criticalSectionForPublisher, criticalSectionForRing;
HANDLE FinishSignal, publisherSemafor, publisherFinishSemafor, Full, Empty;

HANDLE t1,t2,t3;
DWORD thread1ID, thread2ID, thread3ID;

uticnica* publisherSockets = NULL;
RingBuffer ring;

SOCKET connectSocketPubSub = INVALID_SOCKET;



void InitAllCriticalSections() {
    InitializeCriticalSection(&criticalSectionForPublisher);
    InitializeCriticalSection(&criticalSectionForRing);
}

void DeleteAllCriticalSections() {
    DeleteCriticalSection(&criticalSectionForPublisher);
    DeleteCriticalSection(&criticalSectionForRing);
}


void CreateSemaphores() {
    FinishSignal = CreateSemaphore(0, 0, 3, NULL);
    publisherSemafor = CreateSemaphore(0, 0, 1, NULL);
    publisherFinishSemafor = CreateSemaphore(0, 0, 1, NULL);
    Full = CreateSemaphore(0, 0, RING_SIZE, NULL);
    Empty = CreateSemaphore(0, RING_SIZE, RING_SIZE, NULL);
}

void DeleteAllThreadsAndSemaphores() {
    SAFE_DELETE_HANDLE(t1);
    SAFE_DELETE_HANDLE(FinishSignal);
    SAFE_DELETE_HANDLE(publisherSemafor);
    SAFE_DELETE_HANDLE(publisherFinishSemafor);
    SAFE_DELETE_HANDLE(t2);
    SAFE_DELETE_HANDLE(t3);
    SAFE_DELETE_HANDLE(Full);
    SAFE_DELETE_HANDLE(Empty);
}

int Connect() {

    connectSocketPubSub = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectSocketPubSub == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(PUBSUB_PORT2);
    if (connect(connectSocketPubSub, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocketPubSub);
        WSACleanup();
        return 1;
    }

    return 0;
}



DWORD WINAPI Thread1(LPVOID param) {
    SOCKET listenSocketPublisher = *(SOCKET*)param;
    SOCKET acceptedSocketPublisher = INVALID_SOCKET;
    unsigned long mode = 1;

    if (InitializeWindowsSockets() == -1)
    {
        return 1;
    }

    fd_set readfds;
    struct timeval timeVal;
    timeVal.tv_sec = 1;
    timeVal.tv_usec = 0;

    while (WaitForSingleObject(FinishSignal, 500) == WAIT_TIMEOUT) {

        FD_ZERO(&readfds);
        FD_SET(listenSocketPublisher, &readfds);

        int selectResult = select(0, &readfds, NULL, NULL, &timeVal);
        if (selectResult == SOCKET_ERROR) {

            printf("LISTEN SOCKET FAILED WITH ERROR: %d\n", WSAGetLastError());
            closesocket(listenSocketPublisher);
            WSACleanup();
            return 1;

        }
        else if (selectResult == 0) {
            continue;
        }
        else {
            acceptedSocketPublisher = accept(listenSocketPublisher, NULL, NULL);
            if (acceptedSocketPublisher == INVALID_SOCKET) {

                if (WSAGetLastError() == WSAECONNRESET) {
                    printf("accept failed, beacuse timeout for client request has expired.\n");
                }

            }
            else {
                if (ioctlsocket(acceptedSocketPublisher, FIONBIO, &mode) != 0) {
                    printf("ioctlsocket failed with error.");
                    continue;
                }
                else
                {
                    printf("New publisher accepted.\n");
                
                    EnterCriticalSection(&criticalSectionForPublisher);
                    AddSocketToList(&publisherSockets, acceptedSocketPublisher);
                    printf("New publisher socket added.\n");
                    LeaveCriticalSection(&criticalSectionForPublisher);
                    ReleaseSemaphore(publisherSemafor, 1, NULL);
                }
                
            }

        }

    }

    ReleaseSemaphore(publisherFinishSemafor, 1, NULL);
    
    closesocket(listenSocketPublisher);
    WSACleanup();

    return 0;
}

DWORD WINAPI Thread2(LPVOID param) {

    int iResult;
    char recvbuf[DEFAULT_BUFLEN];
    
    fd_set readfds;
    timeval timeVal;
    timeVal.tv_sec = 1;
    timeVal.tv_usec = 0;

    PORUKA* poruka;
    UTICNICA* trenutni = NULL;
    bool finish = false;

    if (InitializeWindowsSockets() == -1)
    {
        return 1;
    }

    const int broj_semafora = 2;
    HANDLE semafori[broj_semafora] = { publisherSemafor, publisherFinishSemafor };

    while (WaitForSingleObject(FinishSignal, 500) == WAIT_TIMEOUT) {
        trenutni = publisherSockets;
        while (trenutni == NULL) {
            if (WaitForMultipleObjects(broj_semafora, semafori, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {
                finish = true;
                break;
            }
            trenutni = publisherSockets;
        }

        if (finish) break;

        FD_ZERO(&readfds);
        
        EnterCriticalSection(&criticalSectionForPublisher);
        while (trenutni != NULL)
        {
            FD_SET(trenutni->acceptedSocket, &readfds);
            trenutni = trenutni->next;
        }
        LeaveCriticalSection(&criticalSectionForPublisher);

        int selectResult = select(0, &readfds, NULL, NULL, &timeVal);

        if (selectResult == SOCKET_ERROR) {

            printf("SELECT LISTE ACCEPTED SOCKETA PUBLISHERA FAILED WITH ERROR: %d\n", WSAGetLastError());

        }
        else if (selectResult == 0) {
            // vreme zadato u timeVal isteklo a pri tome se nije desio
            //ni jedan dogadjaj ni na jednom socketu -> nastavi dalje da slusas
            continue;
        }
        else {
            EnterCriticalSection(&criticalSectionForPublisher);
            trenutni = publisherSockets;
            while (trenutni != NULL)
            {
                if (FD_ISSET(trenutni->acceptedSocket, &readfds)) {
                    iResult = recv(trenutni->acceptedSocket, recvbuf, (int)(sizeof(PORUKA)), 0);
                    if (iResult > 0)
                    {
                        poruka = (PORUKA*)recvbuf;
                        /*printf("\n%s", poruka->topic);
                        printf("\n%s", poruka->text);*/

                        WaitForSingleObject(Empty, INFINITE);

                        EnterCriticalSection(&criticalSectionForRing);
                        ringBufPutMessage(&ring, *poruka);
                        LeaveCriticalSection(&criticalSectionForRing);

                        ReleaseSemaphore(Full, 1, NULL);

                        trenutni = trenutni->next;
                    }
                    else if (iResult == 0)
                    {
                        printf("Connection with publisher closed.\n");
                        closesocket(trenutni->acceptedSocket);

                        uticnica* zaBrisanje = trenutni;
                        trenutni = trenutni->next;

                        RemoveSocketFromList(&publisherSockets, zaBrisanje->acceptedSocket);
                    }
                    else {
                        printf("\npublisher recv failed with error: %d\n", WSAGetLastError());
                        closesocket(trenutni->acceptedSocket);

                        uticnica* zaBrisanje = trenutni;
                        trenutni = trenutni->next;

                        RemoveSocketFromList(&publisherSockets, zaBrisanje->acceptedSocket);
                    }
                }
                else
                {
                    trenutni = trenutni->next;
                }
            }
            LeaveCriticalSection(&criticalSectionForPublisher);
        }

    }
    WSACleanup();
    return 0;
}

DWORD WINAPI Thread3(LPVOID param) {
    int iResult;

    PORUKA poruka;

    


    const int broj_semafora = 2;
    HANDLE semafori[broj_semafora] = { FinishSignal, Full };

    while (WaitForMultipleObjects(broj_semafora, semafori, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {

        EnterCriticalSection(&criticalSectionForRing);
        poruka = ringBufGetMessage(&ring);
        LeaveCriticalSection(&criticalSectionForRing);

        printf("\nSkinuto sa ring buffera: %s", poruka.topic);

        iResult = send(connectSocketPubSub, (char*)&poruka, (int)(sizeof(PORUKA)), 0);

        ReleaseSemaphore(Empty, 1, NULL);

    }


    return 0;
}

void CreateThreadsEngine1(SOCKET* listenSocketPublisher) {
    t1 = CreateThread(NULL, 0, &Thread1, listenSocketPublisher, 0, &thread1ID);
    t2 = CreateThread(NULL, 0, &Thread2, NULL, 0, &thread2ID);
    t3 = CreateThread(NULL, 0, &Thread3, NULL, 0, &thread3ID);
}