#include "../Common/PubSub2.h"
#include <conio.h>


int main(void)
{
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET acceptedSocket = INVALID_SOCKET;

	int iResult;

	if (InitializeWindowsSockets() == -1)
	{
		return 1;
	}

	listenSocket = InitializeListenSocket(PUBSUB_PORT2,0);
	if (listenSocket == SOCKET_ERROR || listenSocket == INVALID_SOCKET)
	{
		return 1;
	}

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}


	acceptedSocket = accept(listenSocket, NULL, NULL);
	if (acceptedSocket == INVALID_SOCKET)
	{
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	listenSocket = INVALID_SOCKET;
	listenSocket = InitializeListenSocket(PUBSUB_PORT_SUBSCRIBER, 1);
	if (listenSocket == SOCKET_ERROR || listenSocket == INVALID_SOCKET)
	{
		return 1;
	}

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}


	printf("Server initialized, waiting for clients.\n");

	initTable(tabela);
	InitAllCriticalSections();
	CreateSemaphores();
	CreateThreadsEngine2(&acceptedSocket, &listenSocket);

	if (!t1 || !t2 || !t3 || !t4 || !t5) {
		ReleaseSemaphore(FinishSignal, 5, NULL);
	}

	while (1) {

		if (_kbhit()) {
			char c = _getch();
			if (c == 'q') {
				ReleaseSemaphore(FinishSignal, 5, NULL);
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
	if (t4) {
		WaitForSingleObject(t4, INFINITE);
	}
	if (t5) {
		WaitForSingleObject(t5, INFINITE);
	}

	DeleteAllCriticalSections();
	DeleteAllThreadsAndSemaphores();
	DeleteAllTable(tabela);

	ZatvoriSveSocketeZaListu(subscriberSockets);

	deleteList(&subscriberSockets);

	closesocket(listenSocket);
	listenSocket = INVALID_SOCKET;
	WSACleanup();

	return 0;
}