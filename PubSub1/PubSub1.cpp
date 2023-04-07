#include "../Common/PubSub1.h"
#include <conio.h>


int main(void)
{
    SOCKET listenSocketPublisher = INVALID_SOCKET;

    int iResult;

    //char recvbuf[DEFAULT_BUFLEN];

    if (InitializeWindowsSockets() == -1)
    {
        return 1;
    }

    listenSocketPublisher = InitializeListenSocket(PUBSUB_PORT1,1);
    if (listenSocketPublisher == SOCKET_ERROR || listenSocketPublisher == INVALID_SOCKET)
    {
        return 1;
    }

    iResult = listen(listenSocketPublisher, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocketPublisher);
        WSACleanup();
        return 1;
    }

    if (Connect())
    {
        return 1;
    }

    printf("Server initialized, waiting for clients.\n");

    InitAllCriticalSections();
    CreateSemaphores();
    CreateThreadsEngine1(&listenSocketPublisher);

    if (!t1 || !t2 || !t3) {
        ReleaseSemaphore(FinishSignal, 3, NULL);
    }

    while (1) {

        if (_kbhit()) {
            char c = _getch();
            if (c == 'q') {
                ReleaseSemaphore(FinishSignal, 3, NULL);
                break;
            }
        }
    }

    if (t1) {
        WaitForSingleObject(t1, INFINITE);
    }
    if (t2) {
        WaitForSingleObject(t2, INFINITE);
    }
    if (t3) {
        WaitForSingleObject(t3, INFINITE);
    }

    DeleteAllCriticalSections();
    DeleteAllThreadsAndSemaphores();



    ZatvoriSveSocketeZaListu(publisherSockets);

    deleteList(&publisherSockets);

    closesocket(listenSocketPublisher);
    listenSocketPublisher = INVALID_SOCKET;
    WSACleanup();

    return 0;

}
