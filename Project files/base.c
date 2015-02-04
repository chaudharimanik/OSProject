/************************************************************************

			This code forms the base of the operating system you will
			build.  It has only the barest rudiments of what you will
			eventually construct; yet it contains the interfaces that
			allow test.c and z502.c to be successfully built together.

			Revision History:
			1.0 August 1990
			1.1 December 1990: Portability attempted.
			1.3 July     1992: More Portability enhancements.
							   Add call to sample_code.
			1.4 December 1992: Limit (temporarily) printout in
							   interrupt handler.  More portability.
			2.0 January  2000: A number of small changes.
			2.1 May      2001: Bug fixes and clear STAT_VECTOR
			2.2 July     2002: Make code appropriate for undergrads.
							   Default program start is in test0.
			3.0 August   2004: Modified to support memory mapped IO
			3.1 August   2004: hardware interrupt runs on separate thread
			3.11 August  2004: Support for OS level locking
		4.0  July    2013: Major portions rewritten to support multiple threads
 ************************************************************************/
/*
Auther	: Manik Chaudhari
 */
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             "process.h"
#include			 "page.h"
#include			 "z502.h"


// These loacations are global and define information about the page table
extern UINT16        *Z502_PAGE_TBL_ADDR;
extern INT16         Z502_PAGE_TBL_LENGTH;

extern void          *TO_VECTOR [];

char                 *call_names[] = { "mem_read ", "mem_write",
		"read_mod ", "get_time ", "sleep    ",
		"get_pid  ", "create   ", "term_proc",
		"suspend  ", "resume   ", "ch_prior ",
		"send     ", "receive  ", "disk_read",
		"disk_wrt ", "def_sh_ar" };
char				 *output_text = NULL;
//extern char MEMORY[PHYS_MEM_PGS * PGSIZE ];

//INT32				SENDER_ID;
int					counter=0;
int					statecounter=0;
int					filecounter = 0;
int 				openFileCounter=0;
BOOL				isDisk;

BOOL				is_Memory_printer_mode = FALSE;
/********************************************************************************************************

	state_printer() : 

        This routine sets all variables which are needed for SP_print_line() in state_printer.
		This function only calls when state_printer is enabled for user test program.

		If id is -1, we don't want to print state for particular mode.
		While printing states for action SEND or RECEIVE , target_id or source_id is -1. This means
		message is broadcast to all. Ther is no specific target. So we don't print target that time.

 ********************************************************************************************************/

void state_printer(char *action, INT32 target, INT32 running_proc_id, INT32 done, INT32 new)
{
	INT32			Time;
	INT32			i;
	INT32			disk_id =-1;
	MEM_READ( Z502ClockStatus, &Time );								//getting time
	//get elements of ready queue
	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	GetReadyQElements();
	for ( i=0;ReadyQElement[i]!=-1 ; i++)
	{
		SP_setup(SP_READY_MODE,ReadyQElement[i]);				//set all elemets of ready queue
	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

	//get Waiting queues elements

	READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	GetWaitingQElements();
	for ( i=0;WaitingQElement[i]!=-1 ; i++)
	{
		SP_setup(SP_WAITING_MODE,WaitingQElement[i]);			//set all elemets of Waiting queue
	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	//get element of suspended queue
	GetSuspenedQElements();
	for ( i=0;SuspendedQElement[i]!=-1 ; i++)
	{
		SP_setup(SP_SUSPENDED_MODE,SuspendedQElement[i]);		//set all elemets of suspened queue
	}
	if(isDisk)
	{
		if(target == 0 || target == 1)
			disk_id = 1;
		else if(target == 2 || target == 3)
			disk_id = 2;
		else if(target == 4 || target == 5)
			disk_id = 3;
		GetDiskQElements(disk_id);
		for ( i=0;DiskQElement[i]!=-1 ; i++)
		{
			SP_setup(SP_SUSPENDED_MODE,DiskQElement[i]);		//set all elemets of suspened queue
		}
	}
	if (target != -1)
		SP_setup(SP_TARGET_MODE,target);							//target of program
	SP_setup(SP_TIME_MODE,Time);									//assigning current time
	SP_setup_action(SP_ACTION_MODE, action);						//set current action which has been taken
	if (running_proc_id != -1)
		SP_setup(SP_RUNNING_MODE,running_proc_id);					//set running process

	if (done != -1)
		SP_setup(SP_TERMINATED_MODE,done);							//set terminated process

	if (new != -1)
		SP_setup(SP_NEW_MODE,new);									//set newly created process
	SP_print_header();
	SP_print_line();
	printf("\n");
	SetQElementsToDefault();


}
/********************************************************************************************************

		SetMemoryPrinter_mode() :

		This routine sets Memory_printer mode for required user test program.

 ********************************************************************************************************/
int SetMemoryPrinter_mode(char *process)
{
	if(strcmp(process,"test2a")==0 || strcmp(process,"test2b")==0 || strcmp(process,"test2e")==0 || strcmp(process,"test2f")==0)
	{
		is_Memory_printer_mode = TRUE;
		return 1;
	}
	is_Memory_printer_mode = FALSE;
	return 0;
}
/********************************************************************************************************

		memory_printer() :

		This routine sets entries in frame table to MP_setup if any frame in memory is
		assigned to frame.

		The state of logical page is calculated according to valid, referenced and modified bit.

		The state is addition of following bit values defined in state_printer.c
		4- valid
		1- referenced
		2- modified

		Ideally, if frame is in memory, state should be atleast 4, 5 ,6 ,7

		This routine prints all current state of logical page.

 ********************************************************************************************************/
void memory_printer()
{
	int 			i;
	int 			state = 0 ;
	UINT16 			*pagePointer;


	for(i=PHYS_MEM_PGS-1;i>=0;i--)
	{
		if(frame_table[i]->logical_page_num !=-1)
		{
			pagePointer = frame_table[i]->pagePointer;
			if((*pagePointer & PTBL_VALID_BIT) == PTBL_VALID_BIT)
			{
				state = 4;
			}
			if((*pagePointer & PTBL_REFERENCED_BIT) == PTBL_REFERENCED_BIT)
			{
				state += 1;
			}
			if((*pagePointer & PTBL_MODIFIED_BIT) == PTBL_MODIFIED_BIT)
			{
				state += 2;
			}
			MP_setup(i,frame_table[i]->process_id,frame_table[i]->logical_page_num ,state);
		}
	}
	MP_print_line();
}
/********************************************************************************************************

		MemoryPrinter() :

		This routine try to fulfil output requirements for memory printer in this project.
		-For test2a and test2b, it prints memory states when fault is occured.
		-Fore test2e, it prints  memory states after each 8 page faults.
		-Fore test2f, it prints  memory states after each 50 page faults.

 ********************************************************************************************************/
void MemoryPrinter()
{
	if (strcmp(Current_PCB->process_name,"test2a")==0 || strcmp(Current_PCB->process_name,"test2b")==0)
	{
		memory_printer();
	}
	if(strcmp(Current_PCB->process_name,"test2e")==0 )
	{
		//printf("conter %d\n",counter);
		if((counter % 8) == 0)
		{
			memory_printer();
		}
		counter++;
	}
	if(strcmp(Current_PCB->process_name,"test2f")==0 )
	{
		//printf("conter %d\n",counter);
		if((counter % 40) == 0)
		{
			memory_printer();
		}
		counter++;
	}
}

/********************************************************************************************************

	GiveUpCPU() : 

        This routine give up CPU when process call SLEEP system call or when process calls suspend
		on itself. 
		It checks whether ready queue is empty or not. If it found ready queue empty 
		then it calls IDLE() otherwise just returns to allow next process on ready queue to run.
		This functionality increases CPU utilization.


 ********************************************************************************************************/

void GiveUpCPU()
{
	INT32				 IsEmpty;

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	IsEmpty = IsEmptyQueue(ReadyQhead);
	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	if ( IsEmpty )//&& Current_PCB -> process_id == 0)
	{
		//printf("calling idle in giveup\n");
		Z502Idle();
	}
}

/********************************************************************************************************

	Dispatcher() : 

        This routine takes off a first process from ready queue and make it running.

		If ready queue is empty it increaments clock and waits until it gets any process to schedule.
		As soon as process is available on ready queue, it Dequeues head form ready queue and
		does switch context on just dequeued PCB context which causes a thread to run for dequeued PCB.
		This function also prints states if user test program require to print states.

		This routine also prints all current states of program for DISPATCH action.

 ********************************************************************************************************/

void Dispatcher()
{
	INT32			IsEmpty;
	long			id;
	PCB				*prevContent;

	IsEmpty = IsEmptyQueue(ReadyQhead);
	while (IsEmpty )
	{
		CALL();
		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		IsEmpty = IsEmptyQueue(ReadyQhead);
		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	}
	id = Current_PCB -> process_id;

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

	Current_PCB = DequeueHeadFromReadyQueue();
	Z502_REG2 = Current_PCB -> process_id;

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	
	if(strcmp(Current_PCB->process_name,"test2d")==0)
	{
			if((counter % 10)== 0)
			{
				if( isStatePrinter == TRUE)
					state_printer("DISPATCH",Current_PCB -> process_id, id, -1,-1);
			}
			counter++;
	}
	else if( isStatePrinter == TRUE)
	{
		state_printer("DISPATCH",Current_PCB -> process_id, id, -1,-1);
	}

	CALL(Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &Current_PCB -> context ));
	//printf("leaving dispatcher\n\n");
}

/********************************************************************************************************

	MakeReady_To_Run() : 

        This routine takes out PCB from waiting queue whose delay time is already passed and 
		make to ready run.

		Function checks emptiness of waiting queue. If there is nothing on waiting queue then it just 
		returns to caller. Otherwise, it loops till it gets negative timedelay. It dequeue PCB from
		waiting queue and put it on ready queue.

 ********************************************************************************************************/

void MakeReady_To_Run()
{
	INT32				Time,RelativeTime;
	PCB				   *PCBptr = NULL;
	INT32				 IsEmpty;

	READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

	IsEmpty = IsEmptyQueue(WaitingQhead);
	READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);


	if(!IsEmpty)
	{
		do
		{
			MEM_READ( Z502ClockStatus, &Time );
			//printf("time %d\n",Time);

			RelativeTime = WaitingQhead -> current_PCB -> timedelay - Time;

			if (RelativeTime < 0)
			{
				READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
				PCBptr = DequeueHeadFromWaitingQueue();		//timer queue
				READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

				if (PCBptr != NULL)
				{
					READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
					AddInReadyQueueByPriority(PCBptr);		//ready queue
					READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
				}
				else
					break;
			}

			READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
			IsEmpty = IsEmptyQueue(WaitingQhead);
			READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		}
		while (RelativeTime < 0 && !IsEmpty);
		if(RelativeTime > 0)
		{
			MEM_WRITE(Z502TimerStart, &RelativeTime);
		}
	}
}


/********************************************************************************************************

    StartTimer() : 

        This routine starts the timer with delay time and calculate delay of process and add in PCB 
		which calls SLEEP. 
		It adds the current process PCB on waiting queue which just called SLEEP and call GiveUpCPU()
		and Dispatcher for next task.

		This routine also prints all current states of program for SLEEP action.

 ********************************************************************************************************/

void StartTimer(long delay)
{
	INT32				Time;
	INT32				Status;


	MEM_READ( Z502ClockStatus, &Time );
	//printf("time %d\n",Time);

	MEM_WRITE(Z502TimerStart, &delay);
	//printf("timer start for %d\n",delay);
	MEM_READ(Z502TimerStatus, &Status);

	Current_PCB -> timedelay = Time + delay;
	if(delay < 10)
	{	//save on ready queue because delay time is too less
		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

		MEM_READ( Z502ClockStatus, &Time );

		AddInReadyQueueByPriority(Current_PCB);

		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	}
	else
	{
		READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

		AddInWaitingQueue(Current_PCB);

		READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	}
	
	if (isStatePrinter == TRUE)
	{
		state_printer("SLEEP",Current_PCB -> process_id, Current_PCB -> process_id, -1,-1);
	}


	GiveUpCPU();
	Dispatcher();
}
/********************************************************************************************************

    StartDisk() :

        This routine sets Z502 Disk parameters to start depending upon
        readwrite bit.

        The Disk starts only if its available to read/write.


 ********************************************************************************************************/

void StartDisk(long disk_id,long sector, char *data, int readwrite)
{
	INT32			Temp;
	//	printf("starting disk %ld for %ld...\n",disk_id,Current_PCB->process_id);
	if(disk_id < 1 || disk_id > MAX_NUMBER_OF_DISKS)
	{
		printf("StartDisk ERROR : Invalid disk id (%ld) found\n\n",disk_id);
		return;
	}
	if(sector < 0 || disk_id > 1600)
	{
		printf("StartDisk ERROR : Invalid sector (%ld) found\n\n",sector);
		return;
	}
	/*if(readwrite !=0 || readwrite != 1)
	{
		printf("StartDisk ERROR : Invalid readwrite (%ld) bit found\n\n",readwrite);
		return;
	}*/
	MEM_WRITE(Z502DiskSetID, &disk_id);
	MEM_READ(Z502DiskStatus, &Temp);
	if (Temp == DEVICE_FREE)        // Disk hasn't been used - should be free
	{
		MEM_WRITE(Z502DiskSetSector, &sector);
		MEM_WRITE(Z502DiskSetBuffer, (INT32 * )data);
		//Temp = 1;
		MEM_WRITE(Z502DiskSetAction, &readwrite);		// Specify to read or write
		Temp = 0;                        // Must be set to 0
		MEM_WRITE(Z502DiskStart, &Temp);
		//		printf("Disk has been started\n");
	}
}
/********************************************************************************************************

    DiskRead() :

        This routine is called by SVC to handle DISK_READ system call.

        Routine saves disk information in Current process's PCB to start in future when disk
        will be available. The current process is added in disk queue to wait to complete its
        read operation.

        If disk if free, it starts disk and give up CPU and call Dispatcher to schedule another process
        to run.
        If disk is not free, it simply gives up CPU and call Dispatcher to wait for disk to free.

        Routine returns without doing any operation for following
        	-disk_id < 0 or disk_id > MAX_NUMBER_OF_DISKS
        	-sector < 0 or sector >= 1600

 ********************************************************************************************************/

void DiskRead(long disk_id,long sector, char *data)
{
	//printf("----------------------------------------------------->%ld process disk %ld reading at %ld\n",Current_PCB->process_id,disk_id,sector);
	INT32			Temp;
	if(disk_id < 1 || disk_id > MAX_NUMBER_OF_DISKS)
	{
		printf("DISK_READ ERROR : Invalid disk id (%ld) found\n\n",disk_id);
		return;
	}
	if(sector < 0 || disk_id >= 1600)
	{
		printf("DISK_READ ERROR : Invalid sector (%ld) found\n\n",sector);
		return;
	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE+3, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

	Current_PCB->diskInfo->data = data;
	Current_PCB->diskInfo ->disk_id = disk_id;
	Current_PCB ->diskInfo ->sector= sector;
	Current_PCB ->diskInfo -> readWrite = 0;

	AddInDiskQueue(Current_PCB,disk_id);
	
	READ_MODIFY(MEMORY_INTERLOCK_BASE+3, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

	MEM_WRITE(Z502DiskSetID, &disk_id);
	MEM_READ(Z502DiskStatus, &Temp);
	if (Temp == DEVICE_FREE)        // Disk hasn't been used - should be free
	{
		READ_MODIFY(MEMORY_INTERLOCK_BASE+4, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		StartDisk(disk_id,sector,data,0);
		READ_MODIFY(MEMORY_INTERLOCK_BASE+4, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		GiveUpCPU();
		Dispatcher();
	}
	else
	{
		//printf(" Device not free.\n");
		GiveUpCPU();
		Dispatcher();
	}
}
/********************************************************************************************************

    DiskWrite() :

        This routine is called by SVC to handle DISK_WriteD system call.

        Routine saves disk information in Current process's PCB to start in future when disk
        will be available. The current process is added in disk queue to wait to complete its
        write operation.

        If disk if free to write, it starts disk immediately, then gives up CPU and call Dispatcher to
        schedule another process to run.
        If disk is not free, it simply gives up CPU and call Dispatcher to wait for disk to free.

        Routine returns without doing any operation for following
        	-disk_id < 0 or disk_id > MAX_NUMBER_OF_DISKS
        	-sector < 0 or sector >= 1600


 ********************************************************************************************************/
void DiskWrite(long disk_id,long sector, char *data)
{

	//printf("---------------------------------------------------> %ld process disk %ld writing at %ld\n",Current_PCB->process_id,disk_id,sector);
	INT32			Temp;
	if(disk_id < 1 || disk_id > MAX_NUMBER_OF_DISKS)
	{
		printf("DISK_WRITE ERROR : Invalid disk id (%ld) found\n\n",disk_id);
		return;
	}
	if(sector < 0 || disk_id >= 1600)
	{
		printf("DISK_WRITE ERROR : Invalid sector (%ld) found\n\n",sector);
		return;
	}

	READ_MODIFY(MEMORY_INTERLOCK_BASE+3, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

	Current_PCB->diskInfo->data = data;
	Current_PCB->diskInfo ->disk_id = disk_id;
	Current_PCB ->diskInfo ->sector= sector;
	Current_PCB ->diskInfo -> readWrite = 1;

	AddInDiskQueue(Current_PCB,disk_id);
	///printf("Disk(%ld)   ",disk_id);
	//PrintQueue(DiskQHead[disk_id]);

	READ_MODIFY(MEMORY_INTERLOCK_BASE+3, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

	MEM_WRITE(Z502DiskSetID, &disk_id);
	MEM_READ(Z502DiskStatus, &Temp);
	if (Temp == DEVICE_FREE)        // Disk hasn't been used - should be free
	{
		READ_MODIFY(MEMORY_INTERLOCK_BASE+4, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

		StartDisk(disk_id,sector,data,1);

		READ_MODIFY(MEMORY_INTERLOCK_BASE+4, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

		GiveUpCPU();
		Dispatcher();
	}
	else
	{
		//give up cpu and call dispatcher because disk is busy with other operation
		GiveUpCPU();
		Dispatcher();
	}
}
/********************************************************************************************************

	HandleDiskInterrupt() :

		This routine first determines disk id depending upon device id which we get when interrupt_
		handler is called.

		Dequeue head from disk queue and start disk for next waiting process to complete its task
		if any request is pending on queue.

		Add dequeued process on Ready queue according to priority to run.

 ********************************************************************************************************/
/*void HandleDiskInterrupt(INT32 device_id)
{
	long			   	disk_id;
	PCB 				*dequeued_pcb,*ptr;
	INT32				i;

	for(i=1; i < 6; i++)
	{
		READ_MODIFY(MEMORY_INTERLOCK_BASE+3, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		dequeued_pcb = DequeueFromDiskQueue(i);		//dequeue head from queue coz disk operation is finished for it.

		READ_MODIFY(MEMORY_INTERLOCK_BASE+3, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		//printf("unlocked\n");

		if(DiskQHead[i]!= NULL)
		{
			ptr = DiskQHead[i] ->current_PCB;
			READ_MODIFY(MEMORY_INTERLOCK_BASE+4, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

			StartDisk(ptr->diskInfo->disk_id,ptr->diskInfo->sector,ptr->diskInfo->data,ptr->diskInfo->readWrite);

			READ_MODIFY(MEMORY_INTERLOCK_BASE+4, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		}
		if(dequeued_pcb != NULL)
		{
			READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
			//printf("ready queue beforein iterrupt\n");
			//PrintQueue(ReadyQhead);
			AddInReadyQueueByPriority(dequeued_pcb);		//ready queue
			//		printf("ready queue after\n");
			//		PrintQueue(ReadyQhead);
			READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		}
	}
	
}*/
void HandleDiskInterrupt(INT32 device_id)
{
	INT32			   	disk_id,Temp;
	PCB 				*dequeued_pcb,*ptr;
	int					i;

	//determining disk id depending upon device_id
	if(device_id == 5)
		disk_id =1;
	else if(device_id == 6)
		disk_id =2;
	else if(device_id == 7)
		disk_id =3;
	else if(device_id == 8)
		disk_id =4;
	else if(device_id == 9)
		disk_id = 5;
	READ_MODIFY(MEMORY_INTERLOCK_BASE+3, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	dequeued_pcb = DequeueFromDiskQueue(disk_id);		//dequeue head from queue coz disk operation is finished for it.

	READ_MODIFY(MEMORY_INTERLOCK_BASE+3, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	//printf("unlocked\n");
	//checks next pending request. If finds, then start disk for that request.
	if(DiskQHead[disk_id]!= NULL)
	{
		ptr = DiskQHead[disk_id] ->current_PCB;
		READ_MODIFY(MEMORY_INTERLOCK_BASE+4, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

		StartDisk(ptr->diskInfo->disk_id,ptr->diskInfo->sector,ptr->diskInfo->data,ptr->diskInfo->readWrite);

		READ_MODIFY(MEMORY_INTERLOCK_BASE+4, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	}
	if(dequeued_pcb != NULL)
	{
		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		//adding finished disk read/write process PCB on ready queue
		AddInReadyQueueByPriority(dequeued_pcb);
		
		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	}

}
/********************************************************************************************************

	interrupt_handler() : 

		When the Z502 gets a hardware interrupt, it transfers control to
		this routine in the OS.

		For TIMER_INTERRUPT, call MakeReady_To_Run() function.
		For DISK_INTERRUPT, call HandleDiskInterrupt().

 ********************************************************************************************************/

void    interrupt_handler( void ) {
	INT32              	device_id;
	INT32              	status;
	INT32              	Index = 0;
	//printf("interrupt handler\n");
	// Get cause of interrupt
	CALL(MEM_READ(Z502InterruptDevice, &device_id ));
	// Set this device as target of our query
	CALL(MEM_WRITE(Z502InterruptDevice, &device_id ));
	// Now read the status of this device
	CALL(MEM_READ(Z502InterruptStatus, &status ));
	//printf( "interrupt_handler: Found vector type %d with value %d\n",device_id, status );

	switch(device_id)
	{
	//case DISK_INTERRUPT to 9 are for disks
	case DISK_INTERRUPT:
		//printf("entering in disk_interrupt\n");
		HandleDiskInterrupt( device_id);
		break;
	case 6:
		HandleDiskInterrupt( device_id);
		break;
	case 7:
		HandleDiskInterrupt( device_id);
		break;
	case 8:
		HandleDiskInterrupt( device_id);
		break;
	case 9:
		HandleDiskInterrupt( device_id);
		break;
	case TIMER_INTERRUPT:
		//printf("entering in timer_interrupt\n");
		CALL(MakeReady_To_Run());
		break;
	}
	// Clear out this device - we're done with it
	MEM_WRITE(Z502InterruptClear, &Index );
	//printf("leaving interrupt handler\n");
}                                       /* End of interrupt_handler */

/********************************************************************************************************

	MemoryManagement() :

		This routine is generally called when page fault is occurred to load logical page into memory.

		- Allocate page table if not present for process and make each entry to 0.

		-If physical memory is free, then get a free frame and assign it to logical page in page table
		and make fault_page Valid to read or modify.

		- If physical memory is not free, then get least recently used frame.
		-Check if that page is present on disk by checking into shadow table of process.
		-Read content from disk if present.
		-Write previous frame number content of its previous logical page into disk.
		-Make previous logical page invalid.
		-Write fault_page contents on its assigned physical memory at frame_numer location if present
		on disk.

		-Make new entry in frame table for fault_page to map fault_page to frame with process id.
 ********************************************************************************************************/
void MemoryManagement(INT32	logical_page)
{
	INT32 		page_num;
	INT32		frame_number=-1,i;

	INT32		prev_logical_page;
	long		disk_loc = 0;
	long		disk_id = 1;
	//INT32		frame;
	char		prevMemdata[PGSIZE]  = {0};
	char		readdata[PGSIZE] = {0};
	//int			*state;
	BOOL		isRead = FALSE;
	UINT16 		*pagePointer;
	PCB			*prev_page_PCB;
	long		prev_process_id;

	page_num = logical_page;
	isRead = FALSE;
	if( page_num >= VIRTUAL_MEM_PGS)
	{
		printf("Fault ERROR : Invalid page number(%d) found\n\n",page_num);
		Z502Halt();
	}
	//allocate page table at 1st page fault and assign each entry to 0
	if(Z502_PAGE_TBL_ADDR == NULL)
	{
		pgTable = (UINT16 *) calloc(PAGE_TABLE_SIZE, sizeof(UINT16));
		for(i = 0;i< PAGE_TABLE_SIZE;i++)
			pgTable[i] = 0x0000;
		Z502_PAGE_TBL_ADDR = pgTable;
		Z502_PAGE_TBL_LENGTH = PAGE_TABLE_SIZE;
	}
	else
		pgTable = Z502_PAGE_TBL_ADDR;
	
	 
	//get physical frame to load fault_page
	frame_number = GetFrame();
	//frame_number = GetFIFOFrame();
	prev_logical_page = frame_table[frame_number]->logical_page_num;
	if(prev_logical_page != -1)
	{
		prev_page_PCB = getPCB(frame_table[frame_number]->process_id) ;
		prev_process_id = frame_table[frame_number]->process_id;
	}
	//printf("frame : %d page %d\n",frame_number,page_num);
	//check whether physical memory is full or not.
	if(prev_logical_page != -1 || Current_PCB->shadow_table[page_num]->disk_loc != -1)//&& isPhyMemoryFull()  )
	{
		//if process's shadow table has entry that page is on disk
		//then read page contents from disk to load in memory
		if(Current_PCB->shadow_table[page_num]->disk_loc != -1)
		{		
			disk_loc = Current_PCB->shadow_table[page_num]->disk_loc;
			disk_id = Current_PCB->shadow_table[page_num]->disk_id;
			DiskRead(disk_id,disk_loc,(char*)readdata);
			isRead = TRUE;
		}

		//return previous frame content
		Z502ReadPhysicalMemory(frame_number,(char *)prevMemdata);

		//prev_logical_page = frame_table[frame_number]->logical_page_num;
		if(prev_logical_page !=-1)
		{
			
			if(prev_page_PCB->shadow_table[prev_logical_page]->disk_id  == -1
					|| prev_page_PCB->shadow_table[prev_logical_page]->disk_loc == -1)
			{
				GetDiskLocation(&disk_id,&disk_loc);
				//making shadow table entry to remember data of logical page on disk
				prev_page_PCB->shadow_table[prev_logical_page]->disk_loc = disk_loc;
				prev_page_PCB->shadow_table[prev_logical_page]->disk_id = disk_id;	 
			}
			else
			{
				//page is alreay on disk so getting its location form shadow table
				disk_loc = prev_page_PCB->shadow_table[prev_logical_page]->disk_loc;
				disk_id = prev_page_PCB->shadow_table[prev_logical_page]->disk_id;
			}

			//making previous logical page of frame invalid
			pagePointer = frame_table[frame_number]->pagePointer ;
			*pagePointer = *pagePointer ^ PTBL_VALID_BIT;

			//writing prev content to disk at disk_loc
			DiskWrite(disk_id,disk_loc,(char*)prevMemdata);
			//MemoryPrinter();
		}
	}
	//assigning the frame number to virtual fault_page in page table for current process
	Z502_PAGE_TBL_ADDR[page_num] = frame_number;
	//making fault page valid to read or write
	Z502_PAGE_TBL_ADDR[page_num] = (Z502_PAGE_TBL_ADDR[page_num] | PTBL_VALID_BIT);
	//making entry in frame_table to map page to frame currectly.
	MakeFrameTableEntry(frame_number,page_num,Current_PCB->process_id);
	if(isRead)
	{
		Z502WritePhysicalMemory(frame_number,(char*)readdata); //write fault_page data on frame if present on disk
	}
	//printing each frame entry with its status
	MemoryPrinter();
}
/********************************************************************************************************
	fault_handler() :

		The beginning of the OS502.  Used to receive hardware faults.
		This function realize what causes fault and gives user message and call Z502Halt() to exit for
		following faults
			-CPU_ERROR
			-INVALID_PHYSICAL_MEMORY
			-PRIVILEGED_INSTRUCTION.

		On page fault, hardware calls fault handler with device id - 2 INVALID_MEMORY to load current
		logical page in memory. This routine calls MemoryManagement() function to handle this fault.


 ********************************************************************************************************/
void    fault_handler( void )
{
	INT32       device_id;
	INT32       status;
	INT32       Index = 0;

	// Get cause of interrupt
	CALL(MEM_READ(Z502InterruptDevice, &device_id ));

	// Set this device as target of our query
	CALL(MEM_WRITE(Z502InterruptDevice, &device_id ));
	// Now read the status of this device
	CALL(MEM_READ(Z502InterruptStatus, &status ));

	//printf( "Fault_handle r: Found vector type %d with value %d\n",device_id, status );
	switch(device_id)
	{
	case CPU_ERROR:
		printf("Fault handler ERROR : CPU error found!!!!\n\n");
		Z502Halt();
		break;

	case INVALID_MEMORY:
		MemoryManagement(status);
		break;

	case INVALID_PHYSICAL_MEMORY :
		printf("Fault handler ERROR : Invalid physical address found!!!!\n");
		Z502Halt();
		break;

	case PRIVILEGED_INSTRUCTION:
		printf("Fault handler ERROR : Try to access PRIVILEGED instruction in USER MODE!!!!\n\n");
		Z502Halt();
		break;

	default:
		break;

	}
	// Clear out this device - we're done with it
	CALL(MEM_WRITE(Z502InterruptClear, &Index ));
	//printf("ending fault\n");
}                                       /* End of fault_handler */

/********************************************************************************************************
	IsFileExists() :

		This routine checks for file existence.Checks file name in file table.

		Return values
		file pointer			if file exists
		NULL					if file does not exist

 ********************************************************************************************************/
FILE_STRUCT *IsFileExists(char *name)
{
	int i=0;
	
	while(i < filecounter)
	{
		if(strcmp(file_table[i]->file_name, name)==0)
		{
			return file_table[i];
		}
		i++;
	}
	return NULL;
}

/********************************************************************************************************
	OpenFile() :

		This function checks for file existence. If file exists then add 
		FCB in open files table.

		Return Values
		ERROR			already open file, non existence file
		SUCCESS			file is exists and not already opened

 ********************************************************************************************************/
long OpenFile(char *name)
{
	INT32			Time,i;

	FILE_STRUCT *file = IsFileExists(name);
	if (file == NULL)
	{
		printf("OPEN_FILE ERROR: File is not present\n\n");
		return ERROR;
	}
	i=0;
	while(i < openFileCounter)
	{
		if(open_files[i] == file)
		{
			printf("OPEN_FILE ERROR: File - %s is already opened\n\n",open_files[i]);
			return ERROR;
		}
		i++;
	}
	//adding FCB on open file table
		open_files[openFileCounter]= file;
		openFileCounter++;
	return SUCCESS;
}
/********************************************************************************************************
	CreateFile() :

		- This function allocate space for new file FCB. 
		- Default directory is HOME.
		- Use disk 1 to write its data. While creating file disk_location are set to -1.
		- The file has fixed size of 10 disk blocks.
		- Add entry in file table and increaments counter to 1.

		Return Values:
		ERROR			duplicate file name, try to create more than max number of files.
		SUCCESS			Successful file creation

 ********************************************************************************************************/
long CreateFile(char *name)
{
	INT32			Time,i=0;
	
	FILE_STRUCT *file = IsFileExists(name);
		if (file != NULL)
		{
				return ERROR;
		}
		if(filecounter > MAX_NUM_OF_FILES)
		{
			printf("CREATE_FILE ERROR: Only %d files can be created\n\n",MAX_NUM_OF_FILES);
			return ERROR;
		}
		else
		{
			file = (FILE_STRUCT*)calloc(1,sizeof(FILE_STRUCT));
			file -> Owner = Current_PCB->process_id;
			file ->directory = "HOME";
			strcpy(file ->file_name,name);
			//file ->file_name = filename;
			file ->disk = 1;
			MEM_READ( Z502ClockStatus, &Time );
			file -> time = Time;
			file -> type = 1;    	//specifies type is file
			for(i = 0;i<10;i++)
			{
				file->disk_loc[1] = -1;
			}
			//adding newly created PCB on file table
			file_table[filecounter] = file;
			filecounter++;
		}
		
		return SUCCESS;
		
}

/********************************************************************************************************
	CloseFile() :

		This function closes opened file. This function checks whether file is open 
		or not. For open file, deletes entry from open file table. Otherwise
		return error.

		Return Values:
		ERROR			non-existent process, non opened file.
		SUCCESS			Successful file close operation

 ********************************************************************************************************/

long CloseFile(char *name)
{
	INT32	i=0;
	FILE_STRUCT *file = IsFileExists(name);
	if (file == NULL)
	{
		printf("CLOSE_FILE ERROR: File is not present\n\n");
		return ERROR;
	}
	while(i < openFileCounter)
	{
		if(open_files[i] == file)
		{		
			break;
		}
		i++;
	}
	if(i < openFileCounter)
	{
		while(i < openFileCounter && i < 10)
		{
			//deleting open file entry from table
			open_files[i] = open_files[i+1];
			i++;
		}
		openFileCounter--;
		return SUCCESS;
	}
	else
	{
		printf("CLOSE_FILE ERROR: File is not currently opened\n\n");
		return ERROR;
	}
}

/********************************************************************************************************
		SVC() :

			The beginning of the OS502.  Used to receive software interrupts.
			All system calls come to this point in the code and are to be
			handled by the student written code here.
			The variable do_print is designed to print out the data for the
			incoming calls, but does so only for the first ten calls.  This
			allows the user to see what's happening, but doesn't overwhelm
			with the amount of data.
 ********************************************************************************************************/

void    svc( SYSTEM_CALL_DATA *SystemCallData )
{
	short               call_type;
	//static short        do_print = 10;
	INT32				Time;
	int					pid = -1;
	long				p =0,pid1;
	//printf("TEST0 process_id : %d\n",current_PCB -> process_id);
	call_type = (short)SystemCallData->SystemCallNumber;


	switch (call_type)
	{
	//CREATE_PROCESS system call
	case SYSNUM_CREATE_PROCESS:

		p = OsCreateProcess((char *)SystemCallData->Argument[0], (void *)SystemCallData->Argument[1],&pid,
				(unsigned long)SystemCallData->Argument[2]);
		//returns error and process pid
		*SystemCallData->Argument[3] = (long)pid;
		*SystemCallData->Argument[4] = p;
		//This causes to print current states of program while CREATE_PROCESS system call if state_printer is true
		if(*SystemCallData->Argument[4] != ERROR && isStatePrinter == TRUE)
			state_printer("CREATE",pid, -1,-1,pid);

		break;

		// GET_TIME_OF_DAY service call
	case SYSNUM_GET_TIME_OF_DAY:   // This value is found in syscalls.h
		//returns current time
		CALL(MEM_READ( Z502ClockStatus, &Time ));
		*(INT32 *)SystemCallData->Argument[0] = Time;

		break;
		//SLEEP system call
	case SYSNUM_SLEEP :

		CALL(StartTimer((long)SystemCallData->Argument[0]));
		break;

		//GET_PROCESS_ID process id
	case SYSNUM_GET_PROCESS_ID :
		//returns ERROR or SUCCESS to user
		*SystemCallData->Argument[2] = GetProcessID((char *)SystemCallData->Argument[0],SystemCallData->Argument[1]);

		break;

		//SUSPEND_PROCESS system call
	case SYSNUM_SUSPEND_PROCESS :
		//returns ERROR or SUCCESS to user
		pid1 = (long)SystemCallData->Argument[0];
		*SystemCallData->Argument[1] = SuspendProcess(pid1);
		//This causes to print current states of program while SUSPEND_PROCESS system call if state_printer is true
		if(*SystemCallData->Argument[1] != ERROR && isStatePrinter == TRUE)
			state_printer("SUSPEND",pid1, Current_PCB -> process_id,-1,-1);

		break;

		//RESUME_PROCESS system call
	case SYSNUM_RESUME_PROCESS :
		//returns ERROR or SUCCESS to user
		pid1 = (long)SystemCallData->Argument[0];
		*SystemCallData->Argument[1] = ResumeProcess(pid1);

		//This causes to print current states of program while RESUME_PROCESS system call if state_printer is true
		if(*SystemCallData->Argument[1] != ERROR &&isStatePrinter == TRUE )
			state_printer("RESUME",pid1, Current_PCB -> process_id,-1,-1);

		break;

		//CHANGE_PRIORITY system call
	case SYSNUM_CHANGE_PRIORITY:

		*SystemCallData->Argument[2] = ChangePriority((long)SystemCallData->Argument[0],(long)SystemCallData->Argument[1]);

		break;

		//SEND_MESSAGE system call
	case SYSNUM_SEND_MESSAGE :
		pid1 = (long)SystemCallData->Argument[0];

		//print current states of program while SEND_MESSAGE system call if state_printer is true
		if(*SystemCallData->Argument[3] != ERROR &&isStatePrinter == TRUE )
			state_printer("SEND",pid1, Current_PCB -> process_id,-1,-1);
		if(!IsEmptyQueue(WaitingToReceiveQhead))
		{
			printf("\nWaiting To Receive \n");
			PrintQueue(WaitingToReceiveQhead);
		}
		//returns ERROR or SUCCESS to user
		*SystemCallData->Argument[3] = SendMessage((long)SystemCallData->Argument[0],
				(long)SystemCallData->Argument[2],(char *)SystemCallData->Argument[1]);


		break;

		//RECEIVE_MESSAGE system call
	case SYSNUM_RECEIVE_MESSAGE :

		pid1 = (long)SystemCallData->Argument[0];
		//print current states of program while RECEIVE_MESSAGE system call if state_printer is true
		if(*SystemCallData->Argument[5] != ERROR &&isStatePrinter == TRUE )
			state_printer("RECEIVE",pid1, Current_PCB -> process_id,-1,-1);
		if(!IsEmptyQueue(WaitingToReceiveQhead))
		{
			printf("\nWaiting To Receive \n");
			PrintQueue(WaitingToReceiveQhead);
		}
		//returns ERROR or SUCCESS to user
		*SystemCallData->Argument[5] = RecieveMessage((long)SystemCallData->Argument[0],(char *)SystemCallData->Argument[1],
				(long)SystemCallData->Argument[2],SystemCallData->Argument[3],SystemCallData->Argument[4]);

		break;

		// TERMINATE_PROCESS system call
	case SYSNUM_TERMINATE_PROCESS:
		//printf("in terminate\n");
		pid1 = (long)SystemCallData->Argument[0];
		if (Current_PCB== NULL)
		{
			printf("Program is terminating abnormally\n");
			CALL(Z502Halt());
		}
		//print current states of program while TERMINATE_PROCESS system call if state_printer is true
		if (isStatePrinter == TRUE)
		{
			state_printer("KILL",Current_PCB -> process_id, -1,Current_PCB -> process_id,-1);
		}
		//free physical frames for process which is going to be terminated
		FreeFramesOfProcess(Current_PCB->process_id);

		/*if pid = -1, this means process do not have child process or child process itself pass 1st parameter -1 to terminate.
				Child process should remove manually from process table to continue with other process.
				If pid = -2 then process calls terminate system call to exit forcefully even
				though there are other process which haven't finish its works
		 */
		if(pid1 == -1 && Current_PCB -> process_id == 0)
		{
			//printf("terminating here\n");
			CALL(Z502Halt());
		}
		if (pid1 == -1 || Current_PCB -> process_id != 0)
			pid1 = Current_PCB -> process_id;

		if (pid1 < 0 )
			CALL(Z502Halt());
		//manually terminate process if 1st parameter od system call is -1
		*SystemCallData->Argument[1] = TerminateProc(pid1);
		//printf("Current_PCB -> isDead - %d %d\n",Current_PCB -> isDead );
		/*if (IsEmptyQueue(ReadyQhead) && IsEmptyQueue(WaitingQhead) && Current_PCB -> process_id !=0)
		{
			Z502Halt();
		}*/
		if (CheckLegalID(Current_PCB ->process_id)== ERROR)
			Current_PCB -> process_id = -1;
		//To schedule next process from ready queue if any child process is terminated manually
		if(Current_PCB -> isDead == 1 && Current_PCB -> process_id !=0)
			Dispatcher();
		if (Current_PCB -> isDead == 1 && Current_PCB -> process_id ==0)
			Z502Halt();

		break;
	case SYSNUM_DISK_READ :
		
		if(strcmp(Current_PCB->process_name,"test2d")==0)
		{
			//prints states after each 10 requests
			if((counter % 10) ==0)
			{
				if( isStatePrinter == TRUE)
			state_printer("DISKREAD",(long)SystemCallData->Argument[0], Current_PCB->process_id,-1,-1);
			}
			counter++;
		}
		else
		{
			if( isStatePrinter == TRUE)
			state_printer("DISKREAD",(long)SystemCallData->Argument[0], Current_PCB->process_id,-1,-1);
		}
		DiskRead((long)SystemCallData->Argument[0],(long)SystemCallData->Argument[1],(char*)SystemCallData->Argument[2]);
		break;
	case SYSNUM_DISK_WRITE :
		if(strcmp(Current_PCB->process_name,"test2d")==0)
		{
			//prints states after each 10 requests
			if((statecounter % 10) == 0)
			{
				if( isStatePrinter == TRUE)
					state_printer("DSKWRITE",(long)SystemCallData->Argument[0], Current_PCB->process_id,-1,-1);
			}
			statecounter++;
		}
		else
		{
			if( isStatePrinter == TRUE)
				state_printer("DSKWRITE",(long)SystemCallData->Argument[0], Current_PCB->process_id,-1,-1);
		}
		DiskWrite((long)SystemCallData->Argument[0],(long)SystemCallData->Argument[1],(char*)SystemCallData->Argument[2]);

		break;
	case SYSNUM_DEFINE_SHARED_AREA:
		
		break;
	case SYSNUM_OPEN_FILE:
			*SystemCallData->Argument[1] = OpenFile((char*)SystemCallData->Argument[0]);
		break;
	case SYSNUM_CREATE_FILE:
		*SystemCallData->Argument[1] = CreateFile((char*)SystemCallData->Argument[0]);
		break;
	case SYSNUM_CLOSE_FILE:
		*SystemCallData->Argument[1] = CloseFile((char*)SystemCallData->Argument[0]);
		break;

	default:
		printf( "ERROR!  call_type not recognized!\n" );
		printf( "Call_type is - %i\n", call_type);
		break;
	}											// End of switch

}                                               // End of svc

/********************************************************************************************************
	intializeToDefault() :

		This routine sets some global variables to default values which operating system will
		use for its operation.
		Also set memory_printer_mode to print current frame and logical page states.

 ********************************************************************************************************/
void intializeToDefault(char *process)
{

	int					i;

	proc_counter = SuspendedProcCounter = proc_id = 0;
	Current_PCB  = NULL;
	//creating processTable by allocating memory.
	ProcessTable = (TABLE *)calloc(1, sizeof(TABLE));

	for (i=0;i<MAX_NUMBER_OF_PROCESSES ;i++ )
		ProcessTable -> PCB[i] = NULL;

	WaitingQhead  = WaitingToReceiveQhead = NULL;
	WaitingQtail  = WaitingToReceiveQtail = NULL;

	ReadyQhead = SuspendedQhead = diskhead = disktail = NULL;
	ReadyQtail = SuspenedQtail =  NULL;


	for (i=0;i<MAX_NUMBER_OF_PROCESSES ;i++ )
	{
		MessageQueue[i] = (MESSAGE_BLOCK *)calloc(1, sizeof (MESSAGE_BLOCK));
		strcpy(MessageQueue[i] -> msg,"");
		MessageQueue[i] -> length = 0;
		MessageQueue[i] -> source_pid = -2;
		MessageQueue[i] -> target_pid = -2;
	}
	for(i =0 ; i < MAX_NUMBER_OF_PROCESSES ; i++)
		ReadyQElement[i] = WaitingQElement[i] = SuspendedQElement[i] = Waiting_To_Recieve [i] = DiskQElement[i] = -1;

	message_counter = waitingMsgCounter = 0;
	isStatePrinter = FALSE;
	for(i = 0;i< 64;i++)
	{
		frame_table[i] = (FRAMETABLE *)calloc(1, sizeof(TABLE));
		frame_table[i]->logical_page_num = -1;
		frame_table[i]->process_id = -1;
		frame_num[i] = i;
	}
	for(i = 0;i< 12;i++)
	{
		DiskQHead[i] = DiskQTail[i] = NULL;
	}
	SetMemoryPrinter_mode(process);
	sector=0;
	if((strcmp(process,"test2c"))==0 || (strcmp(process,"test2d"))==0)
	{
		isDisk = TRUE;
	}
}

/************************************************************************
		osInit() :

			This is the first routine called after the simulation begins.  This
			is equivalent to boot code.  All the initial OS components can be
			defined and initialized here.
 ************************************************************************/

void    osInit( int argc, char *argv[]  ) {

	INT32               i;
	int pid;


	/* Demonstrates how calling arguments are passed thru to here       */

	printf( "Program called with %d arguments:", argc );
	for ( i = 0; i < argc; i++ )
		printf( " %s", argv[i] );
	printf("\n");
	//printf( "Calling with argument 'sample' executes the sample program.\n" );

	/*          Setup so handlers will come to code in base.c           */

	TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR]   = (void *)interrupt_handler;
	TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR] = (void *)fault_handler;
	TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR]  = (void *)svc;
	intializeToDefault(argv[1]);
	if (argc > 1 )
	{
		if (strcmp( argv[1], "test0" ) == 0 )
			OsCreateProcess("test0",(void *)test0,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "sample" ) == 0 )
			OsCreateProcess("test0",(void *)sample_code,&pid,DEFAULT_PRIORITY);
		else if ((strcmp( argv[1], "test1a" ) == 0 ))
			OsCreateProcess("test1a",(void *)test1a,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test1b" ) == 0 )
			OsCreateProcess("test1b",(void *)test1b,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test1c" ) == 0 )
			OsCreateProcess("test1c",(void *)test1c,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test1d" ) == 0 )
			OsCreateProcess("test1d",(void *)test1d,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test1e" ) == 0 )
			OsCreateProcess("test1e",(void *)test1e,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test1f" ) == 0 )
			OsCreateProcess("test1f",(void *)test1f,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test1g" ) == 0 )
			OsCreateProcess("test1g",(void *)test1g,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test1h" ) == 0 )
			OsCreateProcess("test1h",(void *)test1h,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test1i" ) == 0 )
			OsCreateProcess("test1i",(void *)test1i,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test1j" ) == 0 )
			OsCreateProcess("test1j",(void *)test1j,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test1k" ) == 0 )
			OsCreateProcess("test1k",(void *)test1k,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test1l" ) == 0 )
			OsCreateProcess("test1l",(void *)test1l,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test2a" ) == 0 )
			OsCreateProcess("test2a",(void *)test2a,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test2b" ) == 0 )
			OsCreateProcess("test2b",(void *)test2b,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test2c" ) == 0 )
			OsCreateProcess("test2c",(void *)test2c,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test2d" ) == 0 )
			OsCreateProcess("test2d",(void *)test2d,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test2e" ) == 0 )
			OsCreateProcess("test2e",(void *)test2e,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test2f" ) == 0 )
			OsCreateProcess("test2f",(void *)test2f,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test2g" ) == 0 )
			OsCreateProcess("test2g",(void *)test2g,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test2h" ) == 0 )
			OsCreateProcess("test2h",(void *)test2h,&pid,DEFAULT_PRIORITY);
		else if (strcmp( argv[1], "test2i" ) == 0 )
					OsCreateProcess("test2i",(void *)test2i,&pid,DEFAULT_PRIORITY);
	}
	else
		printf("Nothing to run \n\n");

}                                               // End of osInitif (strcmp( argv[1], "test0" ) == 0 )

