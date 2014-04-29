//Project2.c
//4/3/2014
//Jesse Johnson
//CS 3074
//

/*
	This is a kernal program that handles process scheduling, and detects and handles interrupts and syscalls. 
	It has been upgraded to include a paged segementation memory manager.
*/  


#define MAX_FRAMES 8
#define MAX_SEGMENTS 8
#define TIME_SLICE 64
#define TOTAL_TIME 2048
#define UNUSED  0
#define READY   1
#define RUNNING 2
#define BLOCKED 3
#define READ	0
#define WRITE 	1
#define SHARED  2
#define TERMINATE_FLAG  5
#define EXCEPTION_FLAG  6
#define TIMER_FLAG 	8
#define SYSCALL_FLAG	9

//potential memory access flags
#define PASS		    0 
#define MULTIPAGE	    1
#define NONEXISTENT_SEGMENT 2
#define OFF_END_OF_SEGMENT  3

#include	<stdio.h>
#include	"moses_m.h"


#include	"scheduler.h" //custom linked list using Node pointer structures, with essential functions
#include	"semaphore.h" //semaphore structure and functions
#include 	"memManager.h"//all structres and functions necessary for a paging segment memory management scheme 
#include 	"PCB.h"



  

//---------------------------function prototypes---------------------------
	

int read1Bit(unsigned index); 	// reads the bit at "index" in the PSW. 
				// PSW must be present. 

void terminateProcess();	// removes current process from queue and resets 
				// its PCB for future use

void interruptHandler();	// intercepts interrupt flags and call functions 
				// to deal with the interrupts. 
				// If no interrupts occured but a syscall was signaled, 
				// the syscallhandler will be called.

void syscallHandler();		// intercepts syscall codes and calls functions 
				// to deal with them

/*updated*/
void loadPCB();			// loads the data in the current process's PCB into the PSW
				// loads the previous PC value after a page fault occured on the 
				// process's previous cycle. 

void storePCB();		// loads the date from the PSW into the current process's PCB

void nextProcess();		// cycles the scheduling queue and reads in the next process
				// index from the queue for the process table

void timerInterrupt();		// deals with the timer interrupt 
				// (resets timer, decrements total time, and cycles queue)

void newProcess();		// loads the first unused PCB in the process table with 
				// the information from the shared 
				// hardware and creates a node in the queue
void outputRequest();		// calls wait function on a semaphore to check if another process			
				// is using the shared buffer

void output();			// let's process write to the shared buffer

void outputComplete();		// calls signal function on a semaphore to tell next process
				// it can use the shared buffer

void enableInterrupts();	// sets the PSW to allow interrupts
void disableInterrupts();
void enableUserMode();		// sets the PSW to user mode
void enableSupervisorMode();

//UPDATED Memory management functions. Now they handle multipage reads and writes

void memoryActivityHandler();   // when a syscall 3 is intercepted, this function will 
				// will read bits 14 and 15 to determine which memory activity
				// it needs to perform. Activities 0 and 3 (accessing shared segments) 
				// is disabled at the moment. act. 1 will fetch data from memory,
				// act. 2 will write to memory

//NEW! check for segmentation faults 
int checkSegment(int size, unsigned int length, LOGICADDR log_address);

//---------------------------------------------------------------------------------
//---------------------------main program------------------------------------------
//---------------------------------------------------------------------------------



int main ()
{
	//call init_moses() and set up the first process, the process table, and the scheduling queue
	init_moses();
	init_scheduler();
	init_processTable();
	
	init_semaphore (mutex);
	init_frames();
	newProcess();
	vmemDump();
	segDump(0);

	//while there are processes in the queue, loop the following:
	while(listPopulated())
	{
			
		//printDEBUG();		
		enableInterrupts();		
		enableUserMode();
		
		loadPCB();
		
		//this allows the process to start where it left off after a page fault occured.
		process_table[proc_index].quick_save_PC = PSW[1];
		process_table[proc_index].quick_save_tm = *((short*) &PSW[0] + 1);
			
		(*(((FNPTR*)&PSW)+1))();	//call function at the address found in the PC
		storePCB();
		
		interruptHandler();		//deal with interrupts (if no interrupts but there is a 
						// syscall, it will call the syscallhandler)

	} 
	
	// show queue is empty and the loop is done
	printf("\n\n\n\n\n[ ALL PROCESSES TERMINATED ]\n\n");

	enableSupervisorMode();
	disableInterrupts();
	iolog();
	enableInterrupts();
	enableUserMode();
	saveAllToDisk(); 
	vmemDump();
	segDump(0);	

	return 0;

}





//******************************** basic functions ********************************


int read1Bit(unsigned index)
{
	char *charp;
	charp = (char*) &PSW[0];
	int rightShift = 7 - (index%8) ;
	char miniBuffer;

	miniBuffer = *(charp + (index/8)) >> (rightShift) << (7); 
	if (miniBuffer<0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


//------------------------------------------------------------------------------------
void terminateProcess()
{
	removeNode();
	
	if (listPopulated())
	{
		proc_index = getProcID();
		while (process_table[getProcID()].state == BLOCKED)
		{
			cycleQueue();
			proc_index = getProcID();
		}
	}else{
		printf("[ **NO PROCESSES IN QUEUE** ]\n");
	}
	
}

//------------------------------------------------------------------------------------
void loadPCB()
{
	
	short *shortp;
	shortp = (short*) &PSW[0];
	
	
	Rx = process_table[proc_index].rx; 
	Sx = process_table[proc_index].sx;
	Tx = process_table[proc_index].tx;
	Ux = process_table[proc_index].ux;
	Vx = process_table[proc_index].vx;
	
	//this allows the process to start where it left off after a page fault occured.
	if (process_table[proc_index].pgFault_flag == 1)
	{
		PSW[1] = process_table[proc_index].quick_save_PC;
		process_table[proc_index].pgFault_flag = 0; 
		*(shortp + 1) = process_table[proc_index].quick_save_tm;	
	}else
	{
		*(shortp + 1) = process_table[proc_index].tm_save;
		PSW[1] = process_table[proc_index].PC_save;
	}	
}

//------------------------------------------------------------------------------------
void storePCB()
{
	short *shortp;
	shortp = (short*) &PSW[0];
	process_table[proc_index].tm_save = *(shortp + 1);
	
	process_table[proc_index].rx = Rx; 
	process_table[proc_index].sx = Sx;
	process_table[proc_index].tx = Tx;
	process_table[proc_index].ux = Ux;
	process_table[proc_index].vx = Vx;
	
	process_table[proc_index].PC_save = PSW[1];	
}

//------------------------------------------------------------------------------------

void nextProcess()
{
	if (process_table[proc_index].state == RUNNING)
	{
		process_table[proc_index].state = READY;
	}
	

	cycleQueue();
	
	proc_index = getProcID(); 
	while (process_table[proc_index].state == BLOCKED)
	{
		printf("%s is blocked\n",process_table[proc_index].name); 
		cycleQueue();
		proc_index = getProcID(); 
		
	}
	
	if (listPopulated())
	{
		proc_index = getProcID(); 
		process_table[proc_index].state = RUNNING;
	}
		
}



//******************************** interupts ******************************** 

void interruptHandler()
{
	if (read1Bit(TERMINATE_FLAG)){
		printf("[ %s HAS BEEN TERMINATED ]\n", process_table[proc_index].name);
		terminateProcess(); 
	}else{

		if (read1Bit(EXCEPTION_FLAG)){
			printf("[ %s HAS CRASHED ]\n", process_table[proc_index].name);
			terminateProcess();
		}else{
			if (read1Bit(TIMER_FLAG)){
				printf("[ TIMER INTERRUPT: %s has run out of time ]\n", process_table[proc_index].name);
				timerInterrupt();
			}else{
				if (read1Bit(SYSCALL_FLAG))
				{
					syscallHandler();
				}
			}
		}
	}

}

//------------------------------------------------------------------------------------

void timerInterrupt()
{
	process_table[proc_index].tm_save = TIME_SLICE;
	process_table[proc_index].total_time = process_table[proc_index].total_time - TIME_SLICE;
	if (process_table[proc_index].state != BLOCKED)
	{
		process_table[proc_index].state = READY;
	}
	printf("[Total time for process %s: %d ]\n\n\n", process_table[proc_index].name, process_table[proc_index].total_time);
	if (process_table[proc_index].total_time < 0)
	{
		printf("[ process %s timed out and has been terminated ]\n", process_table[proc_index].name);
		terminateProcess();
		
	}else
	{
		nextProcess(); 
	}
}


//******************************** Syscalls ******************************** 

void syscallHandler()
{
	//bits 10-13 contain the 4-bit system call type code

	printf("%s performed syscall: %d%d%d%d\n", process_table[proc_index].name, read1Bit(10), read1Bit(11), read1Bit(12), read1Bit(13));

	//0011 (3) (general) virtual memory access
	if (!read1Bit(10) && !read1Bit(11) && read1Bit(12) && read1Bit(13))
	{
		printf("\n[ Syscall: %s is accessing virtual memory ]\n", process_table[proc_index].name);
		memoryActivityHandler();
	}
	//0101 (5) new process
	else if (!read1Bit(10) && read1Bit(11) && !read1Bit(12) && read1Bit(13))
	{
		printf("\n[ Syscall: %s initiated new process ]\n", process_table[proc_index].name);
		newProcess();
	} 
	//1000 (8) request output to shared buffer
	else if (read1Bit(10) && !read1Bit(11) && !read1Bit(12) && !read1Bit(13))
	{
		printf("\n[ Syscall: %s requested access to output buffer ]\n", process_table[proc_index].name);
		outputRequest();
	} 
	//1001 (9) output to shared buffer
	else if (read1Bit(10) && !read1Bit(11) && !read1Bit(12) && read1Bit(13))
	{
		printf("\n[ Syscall: %s wrote to output buffer ]\n", process_table[proc_index].name);
		output();
	} 
	//1010 (10) complete output to shared buffer
	else if (read1Bit(10) && !read1Bit(11) && read1Bit(12) && !read1Bit(13))
	{
		printf("\n[ Syscall: %s completed access to output buffer ]\n", process_table[proc_index].name);
		outputComplete();
	}
//if none of the above syscalls triggered, then the syscall is not recognized
	else
	{
		printf("\n[ %s attempted a systemcall of unknown type. Process terminated ]\n", process_table[proc_index].name);
		terminateProcess();
	} 
}

//------------------------------------------------------------------------------------

void newProcess()
{
	
	int index;
	int empty = 0;
	int i;
	for (i = 0; empty == 0; ++i)
	{
		if (process_table[i].state == 0)
		{
			empty = 1;
			index = i;
	
		}
	}
	printf("\tsaved new process %s in PCB %d\n\n", Ux, index);
	
	process_table[index].tm_save = TIME_SLICE;
	process_table[index].total_time = TOTAL_TIME;
	strcpy(process_table[index].name, Ux);
	process_table[index].rx = Rx; 
	process_table[index].sx = Sx;
	process_table[index].tx = Tx;
	process_table[index].ux = Ux;
	process_table[index].vx = Vx;
	process_table[index].PC_save = Vx;
	process_table[index].state = 1;
	
	i = 0;

	//load segment data into segment table (note: protection keys disabled)
	unsigned long temp;
	int protkey;
	VDISKADDR diskAddr;
	unsigned long size;

	while ((unsigned long)(((REGTYPE *) Sx)[2*i]) != 0)
	{
		
		temp = (unsigned long)(((REGTYPE *) Sx)[2*i+1]);
		protkey = ( temp & PROT_KEY_MASK ) >> (sizeof(REGTYPE)*BIT_BYTE/2);
		diskAddr = (VDISKADDR)( temp & DISK_ADDRESS_MASK );
		size = (unsigned long)(((REGTYPE *) Sx)[2*i]);
		
		if ( protkey == SHARED)
		{
			//This is a shared segement if the protection key is 2. 
			//Therefore, the segment must be visible to multiple processes
			
			int j = 0;
			while (shared_segment_table[j].diskAddress != diskAddr && j < num_shared_segments){
				++j;
			}
			// make sure the segment isn't already considered shared (^) 
			// then save it to the array of shared segments (v)
			if (j == num_shared_segments)
			{
				shared_segment_table[j].protection_key = SHARED;
				shared_segment_table[j].size = size; 
				shared_segment_table[j].diskAddress = diskAddr;
				process_table[index].segment_table[i].shared_index = j; //indicate which shared segment it is! 
				process_table[index].segment_table[i].size = size; //indicate this segment exists to the process
				init_pages(shared_segment_table[j]);
				init_semaphore(&shared_segment_table[j].semaphore);
				
				printf("disk address %x is a shared address with index %d\n",diskAddr, j); 
				++num_shared_segments;
			}else
			{
				process_table[index].segment_table[i].shared_index = j;
				process_table[index].segment_table[i].size = size; //indicate this segment exists to the process
			}

		}else{
			//if it is not a shared segment, save it to the PCB as normal
			process_table[index].segment_table[i].size = size; 
			process_table[index].segment_table[i].protection_key = protkey;
			process_table[index].segment_table[i].diskAddress = diskAddr;
			init_pages(process_table[index].segment_table[i]);	
		}

		//indicate whether the segment is shared, read only, or neither
		process_table[index].segment_table[i].protection_key = protkey;
		++i;
	}


	addNode(index);


	
}

//------------------------------------------------------------------------------------

void outputRequest()
{
	
	int block = 0;
	wait(mutex, proc_index, &block);
	
	if (block == 1)
	{
		process_table[proc_index].state = BLOCKED;
		nextProcess();
	}
}

//------------------------------------------------------------------------------------

void output()
{
	if (process_table[proc_index].state != BLOCKED)
	{
		enableSupervisorMode();
		io(Ux);
		enableUserMode();
	}
}

//------------------------------------------------------------------------------------

void outputComplete()
{
	
	int index;
	int unblock = 0;
	if (process_table[proc_index].state != BLOCKED)
	{
		enableSupervisorMode();
		disableInterrupts();
		iofl();
		enableInterrupts();
		enableUserMode();
		signal(mutex, &index, &unblock);
		if (unblock == 1)
		{
			process_table[index].state = READY;
		}
	}
}


//UPDATED to handle multipage reads and writes.
//_________________________________________________________________________________________________________
void memoryActivityHandler()
{
	if (!read1Bit(14) && !read1Bit(15)) //activity 0 (00)
	{
		printf("\n[ %s requests permission for shared Vmem f/w ]\n", process_table[proc_index].name);


		int i = 0;
		
		while (shared_segment_table[i].diskAddress != Rx && i < num_shared_segments)
		{
			++i;
		}
		if (i == num_shared_segments)
		{
			printf("[error: given segment is not shared.]\n");		
		}

		int block = 0;
		sem = &shared_segment_table[i].semaphore;
		wait(sem, proc_index, &block);
		
		
		if (block == 1)
		{
			process_table[proc_index].state = BLOCKED;
			printf("\n[ %s blocked by shared segment access violation ]\n", process_table[proc_index].name);
			nextProcess();
		}
	
	}

	//fetch--------------------------------------------

	if (!read1Bit(14) && read1Bit(15)) //activity 1 (01)
	{
		printf("\n[ %s is fetching ]\n", process_table[proc_index].name);
		
		unsigned int seg_num, page_num;
		int pageFault = 0;
		int* present_bit;
		int* present_bit2;
		int* frame_number;
		int* frame_number2;
		VDISKADDR seg_address;

		int segCheck;

		
		//get segment number and page number from log address in Rx
		seg_num = ((unsigned int)Rx & SEG_MASK )>> PAGE_BITS >> DISP_BITS;
		page_num = ((unsigned int)Rx & PAGE_MASK) >> DISP_BITS;

		int shared_index = process_table[proc_index].segment_table[seg_num].shared_index;
		printf("shared index = %d\n", process_table[proc_index].segment_table[seg_num].shared_index);

		segCheck = checkSegment(process_table[proc_index].segment_table[seg_num].size, (unsigned int)Sx, Rx);

	 
		
		if (segCheck == PASS || segCheck == MULTIPAGE)
		{
			// if the page is not in a frame, a page fault occurs and page must be swapped into frame.
			if (process_table[proc_index].segment_table[seg_num].protection_key == 2)
			{
				
				seg_address = shared_segment_table[shared_index].diskAddress;
				present_bit = &shared_segment_table[shared_index].page_table[page_num].present;
				frame_number = &shared_segment_table[shared_index].page_table[page_num].frameNumber;

				if (shared_segment_table[shared_index].page_table[page_num].present == 0)
				{
					pageFault = 1;		
				}
				if (segCheck == MULTIPAGE)
				{
					frame_number2 = &shared_segment_table[shared_index].page_table[page_num+1].frameNumber;
					present_bit2 = &shared_segment_table[shared_index].
							page_table[page_num+1].present;					
					if (shared_segment_table[shared_index].page_table[page_num+1].present == 0)
					{	
						pageFault = 2;
					}	
				}
					
			}else{
			
				seg_address = process_table[proc_index].segment_table[seg_num].diskAddress;
				present_bit = &process_table[proc_index].segment_table[seg_num].page_table[page_num].present;	
				frame_number = &process_table[proc_index].segment_table[seg_num].page_table[page_num].frameNumber;

				if (process_table[proc_index].segment_table[seg_num].page_table[page_num].present == 0)
				{
					pageFault = 1;
				}
				if (segCheck == MULTIPAGE)
				{
					frame_number2 = &process_table[proc_index].segment_table[seg_num].
						page_table[page_num+1].frameNumber;
					present_bit2 = &process_table[proc_index].segment_table[seg_num]. 							page_table[page_num+1].present;	
					if (process_table[proc_index].segment_table[seg_num].page_table[page_num+1].present == 0)
					{	
						pageFault = 2;
						
					}	
				}
			}
			printf("reading from address %x\n", seg_address);
			
			if (pageFault > 0)
			{
				
		
				swap (seg_address, page_num, present_bit, frame_number);
				if (pageFault == 2)
				{
					swap (seg_address, page_num + 1, present_bit2, frame_number2);
				}

				
				// log page fault in paging.txt
				report_page_fault(process_table[proc_index].name, 
						seg_address, 
						seg_num, 
						page_num, 
						*frame_number, 
						READ);

				//immediately cycle process and save the PC so the process can try again on its next turn
				process_table[proc_index].pgFault_flag = 1;
				nextProcess();	
			}else{
				//create physical address for read
				unsigned int disp = (unsigned int)Rx & DISP_MASK;
				unsigned int location = (unsigned int) *frame_number << DISP_BITS;

				if (segCheck == MULTIPAGE)
				{ 
					unsigned int excess_length = disp + (unsigned int)Sx - MAX_PAGE_SIZE;
					location &= FRAME_MASK;
					location |= disp;
					vmemR ((char*)Tx, location, (unsigned int)Sx - excess_length);
						
					disp = 0;
					location = (unsigned int) *frame_number2 << DISP_BITS;
					location &= FRAME_MASK;
					location |= disp;
					vmemR ((char*)(Tx + (unsigned int)Sx - excess_length), location, excess_length);
					
					//set lock and reference bits to 0 and 1, respectively. Set manipulated bit to 1
					frame[*frame_number2].L = 0;
					frame[*frame_number2].R = 1;
					frame[*frame_number2].M = 1;

			

				}else{

					location &= FRAME_MASK;
					location |= disp;
					vmemR ((char*)Tx, location, (unsigned int)Sx);
		
				}
	
				//set lock and reference bits to 0 and 1, respectively. Set manipulated bit to 1
				frame[*frame_number].L = 0;
				frame[*frame_number].R = 1;
				frame[*frame_number].M = 1;
			
	
			} 
		}
		
	}


	//write---------------------------------------------

	if (read1Bit(14) && !read1Bit(15)) //activity 2 (10)
	{
		
		printf("\n[ %s is writing ]\n", process_table[proc_index].name);

		unsigned int seg_num, page_num;
		int pageFault = 0;
		int* present_bit;
		int* present_bit2;
		int* frame_number;
		int* frame_number2;
		VDISKADDR seg_address;

		int segCheck;
		
		//get segment number and page number from log address in Rx
		seg_num = ((unsigned int)Rx & SEG_MASK )>> PAGE_BITS >> DISP_BITS;
		page_num = ((unsigned int)Rx & PAGE_MASK) >> DISP_BITS;
		
		int shared_index = process_table[proc_index].segment_table[seg_num].shared_index;

		segCheck = checkSegment(process_table[proc_index].segment_table[seg_num].size, (unsigned int)Sx, Rx);
		
		if (segCheck == PASS || segCheck == MULTIPAGE)
		{

			//if the segment is READ ONLY, then process should crash
			if (process_table[proc_index].segment_table[seg_num].protection_key == 0)
			{
				printf("[ %s ATTEMPTED WRITE TO READ ONLY SEGMENT. PROCESS TERMINATED ]\n", process_table[proc_index].name);
				terminateProcess();
			}else{
				if (process_table[proc_index].segment_table[seg_num].protection_key == 2)
				{
					seg_address = shared_segment_table[shared_index].diskAddress;
					present_bit = &shared_segment_table[shared_index].page_table[page_num].present;
					frame_number = &shared_segment_table[shared_index].page_table[page_num].frameNumber;

					if (shared_segment_table[shared_index].page_table[page_num].present == 0)
					{
						pageFault = 1;	
					}
					if (segCheck == MULTIPAGE)
					{
						frame_number2 = &shared_segment_table[shared_index].
								page_table[page_num+1].frameNumber;
						present_bit2 = &shared_segment_table[shared_index].
								page_table[page_num+1].present;
						if (shared_segment_table[shared_index].page_table[page_num+1].present == 0)
						{	
							pageFault = 2;
						}	
					}
				}else{
			
					seg_address = process_table[proc_index].segment_table[seg_num].diskAddress;
					present_bit = &process_table[proc_index].segment_table[seg_num].page_table[page_num].present;	
					frame_number = &process_table[proc_index].segment_table[seg_num].page_table[page_num].frameNumber;

					if (process_table[proc_index].segment_table[seg_num].page_table[page_num].present == 0)
					{
						pageFault = 1;
					}
					if (segCheck == MULTIPAGE)
					{
							frame_number2 = &process_table[proc_index].segment_table[seg_num].
									page_table[page_num+1].frameNumber;
							present_bit2 = &process_table[proc_index].segment_table[seg_num].
									page_table[page_num+1].present;

						if (process_table[proc_index].segment_table[seg_num].page_table[page_num+1].present == 0)
						{	
							pageFault = 2;
				
						}	
					}
				}
				// if the page is not in a frame, a page fault occurs and page must be swapped into frame.
				if (pageFault > 0)
				{
					
			
						swap (seg_address, page_num, present_bit, frame_number);
						if (pageFault == 2)
						{
							swap (seg_address, page_num + 1, present_bit2, frame_number2);
						}

						// log page fault in paging.txt
						report_page_fault(process_table[proc_index].name, 
								seg_address, 
								seg_num, 
								page_num, 
								*frame_number, 
								WRITE);

						//immediately cycle process and save the PC so the process can try again on its next turn
						process_table[proc_index].pgFault_flag = 1;
						
						nextProcess();
						
				}else{
					
					//create physical address for write
					unsigned int disp = (unsigned int)Rx & DISP_MASK;
					unsigned int location = (unsigned int) *frame_number << DISP_BITS;
					location &= FRAME_MASK;
					location |= disp;

					if (segCheck == MULTIPAGE)
					{ 
						
						unsigned int excess_length = disp + (unsigned int)Sx - MAX_PAGE_SIZE;
						
						vmemW (location, (char*)Tx, (unsigned int)Sx - excess_length);
						
						disp = 0;
						
						location = (unsigned int) *frame_number2 << DISP_BITS;
						
						location &= FRAME_MASK;
						location |= disp;
						
						vmemW (location, (char*)(Tx + (unsigned int)Sx - excess_length), excess_length);
	
						//set lock and reference bits to 0 and 1, respectively. Set manipulated bit to 1
						frame[*frame_number2].L = 0;
						frame[*frame_number2].R = 1;
						frame[*frame_number2].M = 1;

							

					}else{
						vmemW (location, (char*)Tx, (unsigned int)Sx);
					}
		
					//set lock and reference bits to 0 and 1, respectively. Set manipulated bit to 1
					frame[*frame_number].L = 0;
					frame[*frame_number].R = 1;
					frame[*frame_number].M = 1;
					
	

				}
			}
		}
		
				
	}

	if (read1Bit(14) && read1Bit(15)) //activity 3 (11)
	{
		printf("\n[ %s has ended its Vmem f/w ]\n", process_table[proc_index].name);


		int i = 0;
		while (shared_segment_table[i].diskAddress != Rx)
		{
			++i;
		}

		int unblock = 0;
		int index;
		
		sem = &shared_segment_table[i].semaphore;
		
		signal(sem, &index, &unblock);

		i = 0;
		for (i = 0; i < MAX_FRAMES; ++i)
		{
			if (frame[i].diskAddress == Rx)
			{
				VMEMADDR location = (VMEMADDR) (( i << DISP_BITS) + 0);
				pageW (location, Rx, frame[i].logicalPageNum);
			}
		}
		
		if (unblock == 1)
		{
			process_table[index].state = READY;
			printf("\n[ %s unblocked by mem manager ]\n", process_table[index].name);
		}
		
		
		
		
		
		//segPrint(Rx);	
		
	}
}


//
//NEW!---------------------------------------------------------------------
//
int checkSegment(int size, unsigned int length, LOGICADDR log_address)
{
	if (size == 0)
	{
		printf("[ %s SEGMENTATION FAULT (tried to access nonexistent segment)]\n", process_table[proc_index].name);
		terminateProcess(); 
		getchar();
		return NONEXISTENT_SEGMENT;
	}
	unsigned int requested_address = (unsigned int)((unsigned int)log_address & (0x03ff)) + length;
	printf("size: %d, requested address: %d\n", size, requested_address);
	if (requested_address > size)
	{
		printf("[ %s SEGMENTATION FAULT (tried to access off end if segment)]\n", process_table[proc_index].name);
		terminateProcess(); 
		getchar();
		return OFF_END_OF_SEGMENT;
	} 

	if (((unsigned int)log_address & DISP_MASK) + length > MAX_PAGE_SIZE)
	{
		printf("[ Multipage found ]\n");		
		return MULTIPAGE;
	} 
	
	return PASS;	
	

}

//******************************** system tools/extra functions ******************************** 

void enableInterrupts()
{
	unsigned char mask;
	mask = (unsigned char) (1 << 5); 
	char *charp;
	charp = (char*) &PSW[0];
	
	*(charp) |= mask;
	//printf("[ INTERRUPTS ENABLED ]\n");
}
void disableInterrupts()
{
	unsigned char mask;
	mask = (unsigned char) (1 << 5); 
	char *charp;
	charp = (char*) &PSW[0];
	
	*(charp) &= ~mask;
	//printf("[ INTERRUPTS DISABLED ]\n");
}
//------------------------------------------------------------------------------------

void enableUserMode()
{
	unsigned char mask;
	mask = (unsigned char) (3 << 6); 
	char *charp;
	charp = (char*) &PSW[0];
	
	*(charp) |= mask;

	//printf("[ USER MODE ENABLED ]\n");
}
void enableSupervisorMode()
{
	unsigned char mask;
	mask = (unsigned char) (3 << 6); 
	char *charp;
	charp = (char*) &PSW[0];
	
	*(charp) &= ~mask;
	//printf("[ SUPERVISOR MODE ENABLED ]\n");
}
//------------------------------------------------------------------------------------


void printDEBUG()
{
	printf("PROCESSES IN TABLE:\n");
	cursor = head->next;
	while (cursor != 0)
	{
		printf("%d: %s; State: ", cursor->processID, process_table[cursor->processID].name);
		if (process_table[cursor->processID].state == BLOCKED)
		{
			printf("BLOCKED\n");
		}
		if (process_table[cursor->processID].state == READY)
		{
			printf("READY\n");
		}
		if (process_table[cursor->processID].state == RUNNING)
		{
			printf("RUNNING\n");
		}
		cursor = cursor -> next;
	}
	
	printf("\n");
	cursor = head->next;
}
