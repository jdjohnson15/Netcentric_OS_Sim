//memManager.h
//4/1/2014
//Jesse Johnson
//CS 3074
//

//These are the various structures used for a segmentation memory management scheme

#define BIT_BYTE    8
#define DISK_ADDRESS_MASK ((unsigned long)(-1)>>(sizeof(REGTYPE)*BIT_BYTE/2))
#define PROT_KEY_MASK ((unsigned long)(-1)<<(sizeof(REGTYPE)*BIT_BYTE/2))

#define	SEG_MASK    (0x01C00)
#define	SEG_BITS    3   

#define	NUM_PAGES  16

#define	PAGE_MASK   (0x003C0)
#define	PAGE_BITS   4  
#define MAX_PAGE_SIZE 64 

#define	DISP_MASK   (0x0003F)
#define	DISP_BITS   6  

#define	FRAME_MASK  (0x001C0)
#define	FRAME_BITS  3    

#ifndef MEMMAN_H
#define MEMMAN_H

#include        <string.h>
#include        "semaphore.h"

FILE *outfile;

typedef struct Page
{
	int present;
	int frameNumber;
}Page;

typedef struct Seg_data
{
	int protection_key;
	Semaphore semaphore;
	int size;
	VDISKADDR diskAddress;
	int shared_index; //ONLY USED FOR SHARED SEGMENTS
	Page page_table[NUM_PAGES];
	
	
}Seg_data;

typedef struct Frame
{
	int logicalPageNum;
	VDISKADDR diskAddress; 
	int used;
	int* present;
	int M;
	int L;
	int R; 
}Frame;

Seg_data shared_segment_table[MAX_SEGMENTS];
int num_shared_segments = 0;
Frame frame[MAX_FRAMES];

Semaphore* sem;

void init_frames()
{
	
	outfile = fopen( "paging.txt", "w" );
	char rowLine[120];
	strcpy (rowLine, "-----------------------------------------------------------------------------------------------");    
	fprintf(outfile, "%s\n| NAME  |  SEGMENT DISK @ |  SEGMENT NO.  | LOGICAL PAGE NO. | FRAME NO. | R/W\t\t|\n%s\n", rowLine, rowLine);
	
	int i;
	for (i = 0; i < MAX_FRAMES; ++i)
	{
		frame[i].L = 0;
		frame[i].R = 0;
		frame[i].M = 0;
		frame[i].used = 0;
	}
}

void init_pages(Seg_data seg)
{
	int i;
	for (i = 0; i < NUM_PAGES; ++i)
	{
		seg.page_table[i].present = 0;
	}
}

void swap (VDISKADDR seg_address, unsigned long page, int *pres_addr, int *frame_number)
{
	unsigned int i = 0;
	while ( ( frame[i].L == 1 || frame[i].R == 1 ) && i < MAX_FRAMES )
	{

		if (frame[i].R == 1)
		{

			frame[i].R = 0;
		}	
		++i;
	}
	if (i >= MAX_FRAMES)
	{

		i = 0;
		while ( frame[i].L == 1 )
		{
			++i;
		}	
	}

	if (frame[i].used == 1)
	{
		*(frame[i].present) = 0;
	}

	VMEMADDR location = (VMEMADDR) ((i << DISP_BITS) + 0);

	if (frame[i].M == 1)
	{
		pageW (location, frame[i].diskAddress, frame[i].logicalPageNum);
		
	}

	pageR (location, seg_address, page);
	
	frame[i].L = 1; 
	frame[i].R = 0;
	frame[i].M = 0; 
	frame[i].used = 1;
	frame[i].logicalPageNum = page; 
	frame[i].diskAddress = seg_address;
	frame[i].present = pres_addr;
	
	*(frame[i].present) = 1;
	
	
	printf("frame load successful\n");

	*frame_number = i; 

}

void saveAllToDisk()
{
	int i = 0;
	while (i < MAX_FRAMES)
	{
		if (frame[i].M == 1)
		{
			VMEMADDR location = (VMEMADDR) ((i << DISP_BITS) + 0);
			pageW (location, frame[i].diskAddress, frame[i].logicalPageNum);	
		}
		++i;
	}
	printf("[ ALL MEMORY SAVED TO DISK ]\n");
	//fclose(outfile);
}

void report_page_fault (char name[], VDISKADDR segAddress, unsigned int segNum, unsigned int pageNum, unsigned int frameNum, int R_W) 
{
	
	char rowLine[120];
	char RW[6];
	strcpy (rowLine, "-----------------------------------------------------------------------------------------------");    
	if (R_W == 0)
	{
		strcpy (RW, "READ");
	}else
	{
		strcpy (RW, "WRITE");
	}
	fprintf(outfile, "|%s\t| %X\t\t  | %d\t\t  | %d\t\t     | %d\t |\t%s\t|\n%s\n",
		 name, segAddress, segNum, pageNum, frameNum, RW, rowLine);
	


}


#endif

