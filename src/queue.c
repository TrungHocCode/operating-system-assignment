#include <stdio.h>
#include <stdlib.h>
#include "../include/queue.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
        int size=q->size;
        q->proc[size]=proc;
        q->size++; 
}


struct pcb_t * dequeue(struct queue_t * q) {
        if (q->size == 0) {
            return NULL;
        }
        int idx = 0; 
        for (int i = 1; i < q->size; i++) {
            if (q->proc[i]->priority > q->proc[idx]->priority) {
                idx = i;
            }
        }
        struct pcb_t * proc = q->proc[idx];
        for (int i = idx; i < q->size - 1; i++) {
            q->proc[i] = q->proc[i + 1];
        }
        q->size--;
        return proc;
    }