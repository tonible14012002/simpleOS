#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */
		if (q->size == MAX_QUEUE_SIZE) return;
		// find position
		int pos;
		for (pos=0; pos < q->size; pos++){
			if (proc->prio <= q->proc[pos]->prio) break;
		} 

		for (int i = q->size; i > pos; i--) {
			q->proc[i] = q->proc[i-1];
		}

		q->proc[pos] = proc;
		q->size++;

	}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	if (! q->size) return NULL;
	q->size --;
	return q->proc[q->size];
}
