#pragma once
#include<stdio.h>
#include <stdlib.h>
#include "Structs.h"

#define RING_SIZE 100

// Kruzni bafer - FIFO
struct RingBuffer {
	unsigned int tail;
	unsigned int head;
	PORUKA data[RING_SIZE];
};
// Operacije za rad sa kruznim baferom
PORUKA ringBufGetMessage(RingBuffer* apBuffer) {
	int index;
	index = apBuffer->head;
	apBuffer->head = (apBuffer->head + 1) % RING_SIZE;
	return apBuffer->data[index];
}
void ringBufPutMessage(RingBuffer* apBuffer, PORUKA c) {
	apBuffer->data[apBuffer->tail] = c;
	apBuffer->tail = (apBuffer->tail + 1) % RING_SIZE;
}