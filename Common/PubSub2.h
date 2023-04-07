#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Connection.h"
#include "List.h"
#include "RingBuffer.h"
#include "HashTable.h"

#pragma comment(lib, "ws2_32.lib")

#define SAFE_DELETE_HANDLE(a) if(a){CloseHandle(a);}
#define DEFAULT_BUFLEN 512

#define PUBSUB_PORT2 "27017"
#define PUBSUB_PORT_SUBSCRIBER "27018"

char recvbuf[DEFAULT_BUFLEN];
char recvbufSub[DEFAULT_BUFLEN];

uticnica* subscriberSockets = NULL;

subscribers* tabela[table_size];

CRITICAL_SECTION criticalSectionForRing, criticalSectionForSubscribers;

HANDLE FinishSignal, Full, Empty;


HANDLE t1, t2, t3, t4, t5;
DWORD thread1ID, thread2ID, thread3ID, thread4ID, thread5ID;

RingBuffer ring;

void InitAllCriticalSections() {
    InitializeCriticalSection(&criticalSectionForRing);
    InitializeCriticalSection(&criticalSectionForSubscribers);
}

void DeleteAllCriticalSections() {
    DeleteCriticalSection(&criticalSectionForRing);
    DeleteCriticalSection(&criticalSectionForSubscribers);
}


void CreateSemaphores() {
    FinishSignal = CreateSemaphore(0, 0, 5, NULL);
    Full = CreateSemaphore(0, 0, RING_SIZE, NULL);
    Empty = CreateSemaphore(0, RING_SIZE, RING_SIZE, NULL);
}

void DeleteAllThreadsAndSemaphores() {
    SAFE_DELETE_HANDLE(t1);
    SAFE_DELETE_HANDLE(FinishSignal);
    SAFE_DELETE_HANDLE(Full);
    SAFE_DELETE_HANDLE(Empty);
    SAFE_DELETE_HANDLE(t2);
    SAFE_DELETE_HANDLE(t3);
    SAFE_DELETE_HANDLE(t4);
    SAFE_DELETE_HANDLE(t5);

}

DWORD WINAPI Thread1(LPVOID param) {

    SOCKET acceptedSocketEngine = *(SOCKET*)param;
    int iResult;

    PORUKA * poruka;

    while (WaitForSingleObject(FinishSignal, 500) == WAIT_TIMEOUT) {
        iResult = recv(acceptedSocketEngine, recvbuf, (int)(sizeof(PORUKA)), 0);
        if (iResult > 0)
        {
            poruka = (PORUKA*)recvbuf;
            printf(" Message received from client: %s.\n", poruka->text);

            WaitForSingleObject(Empty, INFINITE);

            EnterCriticalSection(&criticalSectionForRing);
            ringBufPutMessage(&ring, *poruka);
            LeaveCriticalSection(&criticalSectionForRing);

            ReleaseSemaphore(Full, 1, NULL);
        }
        else if (iResult == 0)
        {
            // connection was closed gracefully
            printf("Connection with client closed.\n");
            closesocket(acceptedSocketEngine);
        }
        else
        {
            // there was an error during recv
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(acceptedSocketEngine);
        }
    }

    closesocket(acceptedSocketEngine);
    WSACleanup();
    return 0;
}

DWORD WINAPI Thread2(LPVOID param) {

    SOCKET listenSocket = *(SOCKET*)param;
    SOCKET acceptedSocketSubscriber = INVALID_SOCKET;

    int iResult;

    fd_set readfds;
    timeval timeVal;
    timeVal.tv_sec = 1;
    timeVal.tv_usec = 0;
    unsigned long mode = 1;

    uticnica* trenutni = NULL;

    if (InitializeWindowsSockets() == -1)
    {
        return 1;
    }

    while (WaitForSingleObject(FinishSignal, 1000) == WAIT_TIMEOUT) {
        trenutni = subscriberSockets;

        FD_ZERO(&readfds);

        FD_SET(listenSocket, &readfds);

        while (trenutni != NULL)
        {
            FD_SET(trenutni->acceptedSocket, &readfds);
            trenutni = trenutni->next;
        }

        int selectResult = select(0, &readfds, NULL, NULL, &timeVal);

        if (selectResult == SOCKET_ERROR) {
            printf("SELECT FOR SUBS FAILED WITH ERROR: %d\n", WSAGetLastError());
        }
        else if (selectResult == 0) {
            continue;
        }
        else {
            if (FD_ISSET(listenSocket, &readfds)) {
                sockaddr_in clientAddr;

                acceptedSocketSubscriber = accept(listenSocket, NULL, NULL);

                if (acceptedSocketSubscriber == INVALID_SOCKET) {

                    if (WSAGetLastError() == WSAECONNRESET) {
                        printf("accept failed, beacuse timeout for client request has expired.\n");
                    }

                }
                else {
                    if (ioctlsocket(acceptedSocketSubscriber, FIONBIO, &mode) != 0) {
                        printf("ioctlsocket failed with error.");
                        continue;
                    }
                    else {
                        AddSocketToList(&subscriberSockets, acceptedSocketSubscriber);
                        printf("New Subscriber request accepted\n");
                    }
                }
            }

            else {
                trenutni = subscriberSockets;
                while (trenutni != NULL)
                {
                    if (FD_ISSET(trenutni->acceptedSocket, &readfds)) {
                        
                        iResult = recv(trenutni->acceptedSocket, recvbufSub, DEFAULT_BUFLEN, 0);

                        if (iResult > 0)
                        {
                            recvbufSub[iResult] = '\0';
                            printf("Message received from subscriber: %s.\n", recvbufSub);

                            subscribers* temp = FindSubscriberInTable(tabela, recvbufSub);

                            if (temp == NULL) {
                                // topic ne postoji
                                subscribers* novi = CreateSubscriber(recvbufSub);
                                if (novi == NULL)
                                {
                                    printf("Nije uspelo kreiranje novog topica i subscribera.\n");
                                }
                                else {
                                    EnterCriticalSection(&criticalSectionForSubscribers);
                                    if (AddSocketToList(&novi->acceptedSocketsForTopic, trenutni->acceptedSocket))
                                    {
                                        printf("Subscriber dodat u listu za odredjeni topic\n");
                                        if (AddToTable(tabela, novi))
                                        {
                                            printf("Uspelo dodavanje topica i subscribera u tabelu\n");
                                        }
                                        else
                                        {
                                            printf("subscriber nije dodat u listu za odredjeni topic");
                                        }
                                    }
                                    else
                                    {
                                        printf("subscriber nije dodat u listu za odredjeni topic");
                                    }
                                    LeaveCriticalSection(&criticalSectionForSubscribers);
                                }
                            }
                            else
                            {
                                // topic vec postoji
                                EnterCriticalSection(&criticalSectionForSubscribers);
                                if (!FindInList(&temp->acceptedSocketsForTopic, trenutni->acceptedSocket)) {
                                    if (AddSocketToList(&temp->acceptedSocketsForTopic, trenutni->acceptedSocket))
                                    {
                                        printf("Subscriber dodat u listu za odredjeni topic.");
                                    }
                                    else
                                    {
                                        printf("subscriber nije dodat u listu za odredjeni topic. Topic vec postoji\n");
                                    }
                                }
                                else
                                {
                                    printf("Vec je pretplacen na tu temu!\n");
                                }
                                LeaveCriticalSection(&criticalSectionForSubscribers);
                            }
                            trenutni = trenutni->next;
                        }
                        else if (iResult == 0)
                        {
                            printf("Connection with subscriber closed.\n");
                            closesocket(trenutni->acceptedSocket);

                            EnterCriticalSection(&criticalSectionForSubscribers);
                            DeleteSubscriberFromTable(tabela, trenutni->acceptedSocket);
                            LeaveCriticalSection(&criticalSectionForSubscribers);

                            uticnica* zaBrisanje = trenutni;
                            trenutni = trenutni->next;

                            RemoveSocketFromList(&subscriberSockets, zaBrisanje->acceptedSocket);
                        }
                        else
                        {
                            printf("subscriber recv failed with error: %d\n", WSAGetLastError());
                            closesocket(trenutni->acceptedSocket);

                            EnterCriticalSection(&criticalSectionForSubscribers);
                            DeleteSubscriberFromTable(tabela, trenutni->acceptedSocket);
                            LeaveCriticalSection(&criticalSectionForSubscribers);

                            uticnica* zaBrisanje = trenutni;
                            trenutni = trenutni->next;

                            RemoveSocketFromList(&subscriberSockets, zaBrisanje->acceptedSocket);
                        }
                    }
                    else
                    {
                        trenutni = trenutni->next;
                    }
                }
            }
        }
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;
}

DWORD WINAPI ThreadPool(LPVOID param) {
    int identifikator = (int)param;

    int iResult;
    PORUKA poruka;

    const int broj_semafora = 2;
    HANDLE semafori[broj_semafora] = { FinishSignal,Full };

    while (WaitForMultipleObjects(broj_semafora, semafori, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {

        EnterCriticalSection(&criticalSectionForRing);
        poruka = ringBufGetMessage(&ring);
        LeaveCriticalSection(&criticalSectionForRing);

        printf("\nSkinuto sa ring buffera: %s ", poruka.topic);
        printf("Thread: %d ", identifikator);

        ReleaseSemaphore(Empty, 1, NULL);


        EnterCriticalSection(&criticalSectionForSubscribers);
        subscribers* temp = FindSubscriberInTable(tabela, poruka.topic);
        LeaveCriticalSection(&criticalSectionForSubscribers);

        if (temp != NULL) {
            uticnica* listaPretplacenih = NULL;

            EnterCriticalSection(&criticalSectionForSubscribers);
            
            listaPretplacenih = temp->acceptedSocketsForTopic;

            while (listaPretplacenih != NULL) {

                iResult = send(listaPretplacenih->acceptedSocket, (char*)&poruka, (int)(sizeof(PORUKA)), 0);

                listaPretplacenih = listaPretplacenih->next;
            }

            LeaveCriticalSection(&criticalSectionForSubscribers);

        }

    }

    return 0;
}


void CreateThreadsEngine2(SOCKET* acceptedSocketEngine, SOCKET* listenSocket) {
    t1 = CreateThread(NULL, 0, &Thread1, acceptedSocketEngine, 0, &thread1ID);
    t2 = CreateThread(NULL, 0, &Thread2, listenSocket, 0, &thread2ID);

    t3 = CreateThread(NULL, 0, &ThreadPool, (LPVOID)0, 0, &thread3ID);
    t4 = CreateThread(NULL, 0, &ThreadPool, (LPVOID)1, 0, &thread4ID);
    t5 = CreateThread(NULL, 0, &ThreadPool, (LPVOID)2, 0, &thread5ID);
}