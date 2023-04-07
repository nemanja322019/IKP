
#include "../Common/Subscriber.h"


int __cdecl main(int argc, char** argv)
{
    char topic[16];

    if (InitializeWindowsSockets() == -1)
    {
        return 1;
    }

    if (Connect())
    {
        return 1;
    }

    do {
        printf("Unesi topic za pretplatu: ");
        gets_s(topic, sizeof(topic));

    } while (strlen(topic) == 0);

    Subscribe((void*)topic);


    InitAllCriticalSections();
    CreateSemaphores();
    CreateThreadsSubscriber();

    ReceiveMessages();

    if (t1) {
        WaitForSingleObject(t1, INFINITE);
    }
    DeleteAllCriticalSections();
    DeleteAllThreadsAndSemaphores();

    closesocket(connectSocket);
    WSACleanup();

    return 0;
}