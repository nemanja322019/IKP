#include "../Common/Publisher.h"


int __cdecl main(void)
{
    int iResult;
    char topic[16];
    char messageToSend[256];

    if (InitializeWindowsSockets() == -1)
    {
        return 1;
    }

    if (Connect())
    {
        return 1;
    }

    do {

        printf("Unesi topic za slanje: ");
        gets_s(topic, sizeof(topic));
        printf("Unesi poruku za slanje: ");
        gets_s(messageToSend, sizeof(messageToSend));

        if (strcmp("exit", messageToSend) == 0 || strcmp("exit", topic) == 0)
        {
            break;
        }

        iResult = Publish((void*)topic, (void*)messageToSend);
        if (iResult == -1)
            break;

        printf("Bytes Sent: %ld\n", iResult);

    } while (1);

    /*char topic2[16];
    char messageToSend2[256];
    int i = 0;
    strcpy_s(topic, "qwe");
    strcpy_s(messageToSend, "qqqqqqqq");
    strcpy_s(topic2, "eee");
    strcpy_s(messageToSend2, "eeeeeeeee");
    while (i < 1000000000)
    {
        i++;
        iResult = Publish((void*)topic, (void*)messageToSend);
        if (iResult == -1)
            break;

        printf("Bytes Sent: %ld\n", iResult);
        iResult = Publish((void*)topic2, (void*)messageToSend2);
        if (iResult == -1)
            break;

        printf("Bytes Sent: %ld\n", iResult);
        printf("Broj poruka: %d\n", i*2);
    }*/


    closesocket(connectSocket);
    WSACleanup();

    return 0;
}