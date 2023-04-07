#pragma once
#include<stdio.h>
#include<stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include "../Common/Structs.h"


bool AddSocketToList(uticnica** head, SOCKET data) {
	uticnica* new_node;
	new_node = (uticnica*)malloc(sizeof(uticnica));
	if (new_node == NULL) {
		return false;
	}

	new_node->acceptedSocket = data;
	new_node->next = *head;
	*head = new_node;
	return true;
}

bool RemoveSocketFromList(uticnica** head, SOCKET sock) {
	uticnica* current = *head;
	uticnica* previous = NULL;

	if (current == NULL) {
		return false;
	}

	while (current->acceptedSocket != sock) {

		//if it is last node
		if (current->next == NULL) {
			return false;
		}
		else {
			//store reference to current link
			previous = current;
			//move to next link
			current = current->next;
		}
	}

	if (current == *head) {
		//change first to point to next link
		*head = (*head)->next;
	}
	else {
		//bypass the current link
		previous->next = current->next;
	}

	free(current);
	current = NULL;
	return true;
}

bool FindInList(uticnica** head, SOCKET s) {

	uticnica* temp = NULL;
	uticnica* current = *head;
	while (current != NULL) {
		if (current->acceptedSocket == s)
			return true;
		current = current->next;
	}

	return false;
}


void deleteList(uticnica** head) {
	uticnica* temp = NULL;
	uticnica* current = *head;

	while (current != NULL) {
		temp = current;
		current = current->next;
		free(temp);
		temp = NULL;
	}
	*head = current;
}

void ZatvoriSveSocketeZaListu(uticnica* lista) {
	int iResult;
	while (lista != NULL) {
		iResult = shutdown(lista->acceptedSocket, SD_SEND);
		if (iResult == SOCKET_ERROR)
		{
			printf("shutdown ZATVORI SVE SOCKETE FUNKCIJA  failed with error: %d\n", WSAGetLastError());
			closesocket(lista->acceptedSocket);
			WSACleanup();
		}
		closesocket(lista->acceptedSocket);
		lista = lista->next;
	}
}