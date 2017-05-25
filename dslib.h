#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#ifndef DSLIB_H
#define DSLIB_H

#define NUM_LEN 64

typedef struct data
{
	char p[NUM_LEN];
	short unit;
} Data;

typedef struct stk
{
	Data d;
	struct stk *link;
} stack;

typedef struct queue
{
	Data d;
	struct queue *link;
} queue;

void push(stack **top, Data d)
{
	stack *new = (stack*)malloc(sizeof(stack));
	new->d = d;
	new->link = NULL;

	if (*top == NULL)
		*top = new;
	else {
		new->link = *top;
		*top = new;
	}
}

Data pop(stack **top)
{
	Data d = {"\0", 0};
	if (*top == NULL)
		return d;
	else {
		stack *pp = *top;
		*top = (*top)->link;
		d = pp->d;
		free(pp);
		return d;
	}
}

void enqueue(queue **front, queue **rear, Data d)
{
	queue *new = (queue*)malloc(sizeof(queue));
	new->d = d;
	new->link = NULL;

	if (*front == NULL && *rear == NULL)
		*front = *rear = new;
	else {
		(*rear)->link = new;
		*rear = new;
	}
}

Data dequeue(queue **front,queue **rear)
{
	Data d = {"\0", 0};

	if (*front == NULL && *rear == NULL)
		return d;
	else if (*front == *rear){
		queue *del = *front;
		d = del->d;
		free(del);
		*front = *rear = NULL;
		return d;
	} else {
		queue *del = *front;
		*front = (*front)->link;
		d = del->d;
		free(del);
		return d;
	}
}

int isempty(stack *top)
{
	if(top == NULL)
		return 1;
	else
		return 0;
}

char* top(stack *top)
{
	if (top == NULL)
		return NULL;
	else
		return top->d.p;
}

void printstack(stack *top)
{
	stack *i;
	printf("\nStack: ");
	if (top==NULL)puts("Empty");
	for (i = top; i != NULL; i = i->link)
		printf(" %s ", i->d.p);
}

void printqueue(queue *front)
{
	queue *i;
	printf("Queue: ");
	for (i = front; i != NULL; i = i->link)
		printf(" %s ", i->d.p);
	puts("\n");
}

#endif
