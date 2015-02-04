/*
 * page.c
 *
 *  Created on: Nov 7, 2013
 *      Author: mkchaudhari
 */
#include 		"page.h"
#include 		"process.h"
#include		"global.h"
#include		"string.h"

//int used_frame  = PHYS_MEM_PGS -1;
long disk_num = 1;
BOOL isFull = FALSE;
INT16		framePointer = PHYS_MEM_PGS -1;
extern UINT16 *Z502_PAGE_TBL_ADDR;

/********************************************************************************************************

	getFIFOFrame() :

	This routine returns free frame in descending order.

	This function is like FIFO algorithm.

	When memory is full it just again loop through all memory frames in descending order and returns
	current frame pointed by framePointer.


 ********************************************************************************************************/
int GetFIFOFrame()
{
	int num = 0;

	num = framePointer --;
	if(framePointer < 0)
	{
		isFull = TRUE;
		framePointer = PHYS_MEM_PGS -1;
	}
	return num;
}
/********************************************************************************************************

	getFrame() :

	This routine returns free frame if available by checking each frame entry.
	If frame is not assigned to any logical page, routine returns that free frame.

	Otherwise, get least recently frame and return it.


 ********************************************************************************************************/

int GetFrame()
{
	int num = 0,i;
	if(!IsPhyMemoryFull())
	{
		//printf("memory is not full\n");
		//num =  frame_num --;
		for(i = PHYS_MEM_PGS -1 ;i >=0 ;i--)
			{
				if(frame_num[i] != -1)
				{
					num = i;
					frame_num[i] = -1;
					return num;
				}
			}
		if(i < 0)
		{
			isFull = TRUE;
			//framePointer = 63;
		}
	}
	else
		num = GetLRUFrame();
	return num;
}
/********************************************************************************************************

	isPhyMemoryFull() :

	This routine return isFull value

	TRUE			if memory is full
	FALSE			memory not full

 ********************************************************************************************************/
BOOL IsPhyMemoryFull()
{
	return isFull;
}

/********************************************************************************************************

	makeFrameTableEntry() :

	This routine makes entry in frame table to map logical page to physical page.
	Also store process id to determine which process owns frame and pagePointer which is helpful in
	multiprocessing enviornment.

 ********************************************************************************************************/
void MakeFrameTableEntry(int frame_num,int logical_pg_num,long pid)
{
	if(frame_num < 0)
	{
		printf("ERROR : Invalid frame number\n");
		Z502Halt();
	}
	frame_table[frame_num]->logical_page_num = logical_pg_num;
	frame_table[frame_num]->process_id = pid;
	frame_table[frame_num]->pagePointer = &Z502_PAGE_TBL_ADDR[logical_pg_num];
	//printf("in frame tabel %d is assigned to %d of process pid %ld\n",frame_num,frame_table[frame_num]->logical_page_num,frame_table[frame_num]->process_id);

}
/********************************************************************************************************

	getDiskLocation() :

	This routine returns disk_loc and disk id to store previous contents of physical memory
	while page replacement.
 ********************************************************************************************************/
void GetDiskLocation(long *disk_id,long *disk_loc)
{
	if (sector >= NUM_LOGICAL_SECTORS) {
		sector = 0;
		disk_num++;
	}
	*disk_id = disk_num;
	*disk_loc = sector;
	sector++;
}

/********************************************************************************************************

	getLRUFrame() :

	This routine returns least recently used frame.
	Frame table is searched by circular way by framePointer to search least recently used frame.
	Initially goes through all entry and make it unreferenced and returns first frame.

 	If page is valid and accessed recently so this algorithm unreferenced it and returns next
 	unreferenced frame.

 ********************************************************************************************************/
int GetLRUFrame()
{
	INT32		frame = PHYS_MEM_PGS - 1;
	UINT16 		*pagePointer;

	//TODO:check pagepointer if process is terminated and still there is entry for it in process table
	if(framePointer < 0)
		framePointer = PHYS_MEM_PGS -1;
	while(TRUE)
	{
		if(frame_table[framePointer]->process_id == -1)
		{
			printf("ERROR in getLRUFrame function\nCurrently Physical memory is full \nbut entry of frame number - %d is wrong\n\n",framePointer);
			Z502Halt();
		}
		pagePointer = frame_table[framePointer]->pagePointer;

		if((*pagePointer & PTBL_VALID_BIT) == PTBL_VALID_BIT)
		{
			if((*pagePointer & PTBL_REFERENCED_BIT) == PTBL_REFERENCED_BIT)
			{
				*pagePointer = (*pagePointer ^ PTBL_REFERENCED_BIT);
			}
			else
			{
				//getting frame from memory resident page pointer
				frame = *pagePointer & PTBL_PHYS_PG_NO;
				framePointer --;
				break;
			}
		}
		framePointer --;
		if(framePointer < 0)
			framePointer = PHYS_MEM_PGS -1;
	}

	return frame;
}

/********************************************************************************************************

	freeFramesOfProcess() :

	This routine sets frame_table entries of terminated process to default to indicate frame is now free
	to use.
 ********************************************************************************************************/
void FreeFramesOfProcess(long id)
{
	int 			i,j=0;

	for(i = 0;i < PHYS_MEM_PGS ;i++)
	{
		if(frame_table[i]->process_id == id)
		{
			//printf("freeing %d frame for process \n",i);
			frame_table[i] -> logical_page_num = -1;
			frame_table[i] ->process_id = -1;
			frame_table[i] -> pagePointer = NULL;
			frame_num[i] = i;
			j++;
			//printf("%d is free now\n",i);
		}
	}
	if(j > 0)
		isFull = FALSE;
}

