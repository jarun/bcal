#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#ifndef DSLIB_H
#define DSLIB_H

#define NUM_LEN 64

typedef struct data {
	char p[NUM_LEN];
	short unit;
} Data;

typedef struct stack {
	Data d;
	struct stack *link;
} stack;

typedef struct queue {
	Data d;
	struct queue *link;
} queue;

void push(stack **top, Data d)
{
	stack *new = (stack *)malloc(sizeof(stack));

	new->d = d;
	new->link = NULL;

	if (*top == NULL)
		*top = new;
	else {
		new->link = *top;
		*top = new;
	}
}

void pop(stack **top, Data *d)
{
	d->p[0] = '\0';
	d->unit = 0;

	if (*top != NULL) {
		stack *tmp = *top;
		*top = (*top)->link;
		*d = tmp->d;
		free(tmp);
	}
}

void enqueue(queue **front, queue **rear, Data d)
{
	queue *new = (queue *)malloc(sizeof(queue));

	new->d = d;
	new->link = NULL;

	if (*front == NULL)
		*front = *rear = new;
	else {
		(*rear)->link = new;
		*rear = new;
	}
}

void dequeue(queue **front, queue **rear, Data *d)
{
	d->p[0] = '\0';
	d->unit = 0;

	if (*front != NULL) {
		queue *tmp;

		if (*front == *rear) {
			tmp = *front;
			*d = tmp->d;
			free(tmp);
			*front = *rear = NULL;
		} else {
			tmp = *front;
			*front = (*front)->link;
			*d = tmp->d;
			free(tmp);
		}
	}
}

int isempty(stack *top)
{
	if (top == NULL)
		return 1;

	return 0;
}

char *top(stack *top)
{
	if (top == NULL)
		return NULL;

	return top->d.p;
}

void emptystack(stack **top)
{
	stack *tmp;

	while (*top != NULL) {
		tmp = *top;
		*top = (*top)->link;
		free(tmp);
	}
}

void cleanqueue(queue **front)
{
	queue *tmp;

	while (*front != NULL) {
		tmp = *front;
		*front = (*front)->link;
		free(tmp);
	}
}

void printstack(stack *top)
{
	stack *i;

	printf("\nStack: ");

	if (top == NULL)
		printf("Empty");

	for (i = top; i != NULL; i = i->link)
		printf(" %s ", i->d.p);

	printf("\n");
}

void printqueue(queue *front)
{
	queue *i;

	printf("\nQueue: ");

	if (front == NULL)
		printf("Empty");

	for (i = front; i != NULL; i = i->link)
		printf(" %s ", i->d.p);

	printf("\n");
}

#endif
