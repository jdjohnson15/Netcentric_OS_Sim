//scheduler.h
//2/18/2013
//Jesse Johnson
//CS 3074
//

/*
	This is a linked list designed to be used in the simuluated OS kernal for Project1

*/  

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include	<stdio.h>
#include 	<stdlib.h>

struct Node
{
	int processID;
	struct Node *next; 
};

struct Node *head, *cursor, *tail;

//initialize scheduling queue.
void init_scheduler()
{

	head = (struct Node *) malloc(sizeof(struct Node)); 	//reference used to learn this c syntax: 
								//http://www.cprogramming.com/tutorial/c/lesson15.html
	head->next = 0;

	cursor = head;
	tail   = head;


}

//add a new node to the end of the list, which will loop to the start
void addNode(int index)
{
	tail -> next = (struct Node *) malloc(sizeof(struct Node));
	tail = tail -> next;
	tail -> processID = index;
	tail -> next = 0;
}

//remove the node at the current cursor location
void removeNode()
{
	if (head->next != 0)
	{
		
		struct Node *trash = (struct Node *) malloc(sizeof(struct Node));
		trash = head -> next;
		
		head -> next = head -> next -> next;
		free(trash);	//reference used to learn this c syntax: 
			  	//http://www.yolinux.com/TUTORIALS/Cpp-DynamicMemory.html
	}
} 

// make second item in list first, make first item last. 
void cycleQueue()
{
	
	if (head->next->next == 0)
	{
	
	}else{
		if (head->next != 0)
		{
			
			tail -> next = head -> next;	
			
			head -> next = head -> next -> next;
			tail = tail -> next;
			tail -> next = 0; 
			

		}else{
		
		}
	}
}

//print debug for the list

void printQueue()
{
	printf("SCHEDULING QUEUE CONTENTS:\n");
	cursor = head->next;
	while (cursor != 0)
	{
		printf("--%d", cursor->processID);
		cursor = cursor -> next;
	}
	printf("\n");
	cursor = head->next;
}

//tells if the list has items in it
int listPopulated()
{
	if (head -> next == 0)
	{
		return 0;
	}
		return 1;
}

//returns the process ID stored in the 1st node
int getProcID()
{
	
	return head->next->processID;
}


#endif
