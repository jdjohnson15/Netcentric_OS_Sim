//semaphore.h
//3/4/2014
//Jesse Johnson
//CS 3074
//

/*
	This is a set of functions for the semaphore structure

*/  

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#ifndef READY
#define READY 1
#endif

#ifndef BLOCKED
#define BLOCKED 3
#endif

#include	<stdio.h>
#include 	<stdlib.h>





//nodes for the semaphore's waiting list

typedef struct SemNode
{
	int ID;
	struct SemNode *next; 
}SemNode;

typedef struct Semaphore
{
	int counter;
	struct SemNode *head, *cursor, *tail;  
}Semaphore;

Semaphore mut; 

Semaphore* mutex = &mut; 



//initialize semaphore

void init_semaphore (Semaphore* sem)
{
	sem->counter = 1;
	sem->head = (struct SemNode *) malloc(sizeof(struct SemNode)); 
							
	sem->head->next = (struct SemNode *) malloc(sizeof(struct SemNode));

	sem->cursor = sem->head;
	sem->tail   = sem->head;
}

void printBlocked(Semaphore* sem)
{
	printf("Blocked processes:\n");
	sem->cursor = sem->head->next;
	while (sem->cursor != 0)
	{
		printf("--%d", sem->cursor->ID);
		sem->cursor = sem->cursor -> next;
	}
	printf("\n");
	sem->cursor = sem->head->next;
}



//semaphore wait operation
//	decrements semaphore int variable
//      test: if int var is negative, block calling process and put its ID
//		in the end of the semaphore's waiting list


void wait(Semaphore* sem, int index, int* block)
{
	
	--sem->counter;
	printf("counter = %d\n", sem->counter);
	if ( sem->counter < 0 )
	{

		*block = 1;
	
		sem->tail -> next = (struct SemNode *) malloc(sizeof(struct SemNode));
	
		sem->tail = sem->tail -> next;
		
		sem->tail -> ID = index;
		sem->tail -> next = 0;
		
	}
	
}

//semaphore signal operation
//	increments semaphore int variable
//	test: if variable is nonpositive, 
//		remove next process from front of waiting list
//		and change the process's state from blocked to ready

void signal(Semaphore* sem, int* index, int* unblock)
{
	++sem->counter;
	if ( sem->counter <= 0 )
	{
		if (sem->head->next != 0)
		{
		
			*index = sem->head -> next -> ID;
			struct SemNode *trash = (struct SemNode *) malloc(sizeof(struct SemNode));
			trash = sem->head -> next; 
			sem->head -> next = sem->head -> next -> next;
		
			free(trash);
			if (sem->head->next == 0)
			{
				sem->tail = sem->head;
				sem->cursor = sem->head;
			}
			*unblock = 1;
					  
		}
	}
	
}




#endif
