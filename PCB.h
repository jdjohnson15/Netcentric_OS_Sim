//PCB.h
//3/4/2014
//Jesse Johnson
//CS 3074
//

//This is the structure for Proccess control boards
// new! updated to contain a segement table


#ifndef PCB_H
#define PCB_H
#include "memManager.h"

#define MAX_PCB 24
#define MAX_NAME_LENGTH 60


// PCB strcture
typedef struct PCB
{
	int tm_save;
	int total_time;
	char name[MAX_NAME_LENGTH];
	REGTYPE rx;
	REGTYPE sx;
	REGTYPE tx;
	REGTYPE ux;
	REGTYPE vx;
	void* PC_save;
	int state; //0 for unused/terminated, 1 for ready, 2 for running, 3 for blocked

	//new!
	Seg_data segment_table[MAX_SEGMENTS]; 

	//new!
	int pgFault_flag;
	int quick_save_tm;
	void* quick_save_PC;

	
}PCB;


PCB process_table[MAX_PCB];

int proc_index = 0;

// sets all PCBs in process table to "unused" status
void init_processTable()
{
	int i;
	for (i = 0; i < MAX_PCB; ++i)
	{
		process_table[i].state = 0;
		process_table[i].pgFault_flag = 0;
	}
}

#endif
