#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <WinSock2.h>
#include "List.h"
#define _CRT_SECURE_NO_WARNINGS

#define table_size 10

unsigned int HashFunction(const char* topic) {

	int length = strlen(topic);
	unsigned int hash_value = 0;
	for (int i = 0; i < length; i++) {

		hash_value += topic[i];
		hash_value = (hash_value * topic[i]) % table_size;
	}

	return hash_value;

}

bool AddToTable(subscribers** tabela, subscribers* s) {
	if (s == NULL) return false;
	int index = HashFunction(s->topic);
	//ovde negde malloc odraditi

	s->next = tabela[index];
	tabela[index] = s;

	return true;
}

void initTable(subscribers** tabela) {
	for (int i = 0; i < table_size; i++) {
		tabela[i] = NULL;

	}
}

bool DeleteFromTable(subscribers** tabela, char* topic) {
	int index = HashFunction(topic);
	subscribers* temp = tabela[index];
	subscribers* prev = NULL;
	while (temp != NULL && strcmp(temp->topic, topic) != 0) {
		prev = temp;
		temp = temp->next;
	}
	if (temp == NULL) {
		return false;
	}
	if (prev == NULL) {
		tabela[index] = temp->next;
	}
	else {
		prev->next = temp->next;
	}

	deleteList(&temp->acceptedSocketsForTopic);
	free(temp);
	temp = NULL;
	return true;
}

void DeleteAllTable(subscribers** tabela) {
	for (int i = 0; i < table_size; i++) {
		if (tabela[i] == NULL) continue;
		else {
			subscribers* temp = tabela[i];
			while (tabela[i] != NULL) {
				DeleteFromTable(tabela, tabela[i]->topic);
			}
		}
	}
}


subscribers* FindSubscriberInTable(subscribers** tabela, const char* topic) {
	int index = HashFunction(topic);
	subscribers* temp = tabela[index];
	while (temp != NULL && strcmp(temp->topic, topic) != 0) {
		temp = temp->next;
	}
	return temp;
}

subscribers* CreateSubscriber(const char* topic) {

	subscribers* novi = (subscribers*)malloc(sizeof(subscribers));
	if (novi == NULL) return NULL;

	strcpy_s(novi->topic, topic);
	novi->acceptedSocketsForTopic = NULL;
	return novi;
}


void DeleteSubscriberFromTable(subscribers** tabela, SOCKET socket) {
	for (int i = 0; i < table_size; i++) {
		if (tabela[i] == NULL) continue;
		else {
			subscribers* temp = tabela[i];
			while (temp != NULL)
			{
				if (RemoveSocketFromList(&temp->acceptedSocketsForTopic, socket)) {
					printf("Nasao ga i brisem iz liste...\n");
				}
				temp = temp->next;
			}
		}
	}


}

void printTable(subscribers** tabela) {
	printf("**** POCETAK ISPISA TABELE *****\n");
	for (int i = 0; i < table_size; i++) {
		if (tabela[i] == NULL) {
			printf("%i \t NULL\n", i);
		}
		else {
			printf("%i\t", i);
			subscribers* temp = tabela[i];
			while (temp != NULL)
			{
				printf("%s -> [ ", temp->topic);
				uticnica* aca = temp->acceptedSocketsForTopic;
				while (aca != NULL) {
					printf(" %d - ", aca->acceptedSocket);
					aca = aca->next;
				}
				printf(" ] --");
				temp = temp->next;
			}
			printf("\n");
		}
	}
	printf("**** KRAJ ISPISA TABELE *****\n");
}