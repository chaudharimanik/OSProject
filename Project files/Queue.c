
/*
Auther	: Manik Chaudhari

This file contains all queue related functions.
Declaration of functions are in process.h
 */

#include			"process.h"

int ReadyQCounter = 0;
int WaitingQCounter = 0;


/********************************************************************************************************

	AddInReadyQueueByPriority() : 

	This routine saves PCB by its priority. The PCB with highest priority is head of ready queue.


 ********************************************************************************************************/
void AddInReadyQueueByPriority(PCB *current_PCB)
{

	ReadyQueue *new = NULL;
	ReadyQueue *ptr1;
	new = (ReadyQueue*)malloc(sizeof(ReadyQueue));
	new -> current_PCB = current_PCB;
	new -> next = NULL;

	if (ReadyQhead == NULL)
	{
		ReadyQtail = ReadyQhead = new;
		return;
	}
	else if(new -> current_PCB -> priority < ReadyQhead -> current_PCB-> priority)
	{
		new -> next = ReadyQhead;
		ReadyQhead = new;
		return;
	}
	else
	{
		/*serach a position in a list to insert a new element*/
		ptr1=ReadyQhead;
		while(ptr1!=NULL)
		{
			if ((ptr1->current_PCB ->priority <= new-> current_PCB->priority) &&
					(ptr1->next == NULL || (ptr1->next->current_PCB->priority > new->current_PCB->priority)))
			{
				new->next=ptr1->next;
				ptr1->next=new;
				if (ptr1 == ReadyQtail)
				{
					ReadyQtail = new;
				}
				return;
			}
			ptr1= ptr1->next;
		}
	}

}
/********************************************************************************************************

	AddInWaitingQueue() : 

	This routine saves PCB by its timedelay. The PCB with lowest timedelay is head of 
	Waiting queue.


 ********************************************************************************************************/
void AddInWaitingQueue(PCB *current_PCB)
{
	INT32			Time, RelativeTime, TimeDelay,Status;


	WaitingQueue *new = NULL;
	WaitingQueue *ptr1;
	new = (WaitingQueue*)malloc(sizeof(WaitingQueue));
	new -> current_PCB = current_PCB;
	new -> next = NULL;
	//printf("id : %d\n",current_PCB -> process_id);
	if (WaitingQhead == NULL)
	{
		//MEM_READ( Z502ClockStatus, &Time );
		WaitingQtail = WaitingQhead = new;
		//printf("adding in waiting queue\n");
		return;
	}
	else if(new -> current_PCB -> timedelay < WaitingQhead -> current_PCB-> timedelay)
	{
		new -> next = WaitingQhead;
		WaitingQhead = new;

		MEM_READ( Z502ClockStatus, &Time );

		MEM_READ(Z502TimerStatus, &Status);
		TimeDelay = WaitingQhead -> current_PCB-> timedelay;
		RelativeTime = TimeDelay - Time;
		if(RelativeTime > 0)
			MEM_WRITE(Z502TimerStart, &RelativeTime);
		return;
	}
	else
	{
		/*serach a position in a list to insert a new element*/
		ptr1=WaitingQhead;
		while(ptr1!=NULL)
		{
			if ((ptr1->current_PCB ->timedelay <= new-> current_PCB->timedelay) &&
					(ptr1->next == NULL || (ptr1->next->current_PCB->timedelay > new->current_PCB->timedelay)))
			{
				new->next=ptr1->next;
				ptr1->next=new;
				if (ptr1 == WaitingQtail)
					WaitingQtail = new;
				return;
			}
			ptr1= ptr1->next;
		}
	}

}
/********************************************************************************************************

	DequeueHeadFromWaitingQueue() : 

	This routine removes head of the waiting queue and returns the PCB of head element.
	Returns NULL when queue is empty


 ********************************************************************************************************/
PCB *DequeueHeadFromWaitingQueue( )
{
	PCB * cPCB= NULL;

	if(WaitingQtail == WaitingQhead)
	{
		if (WaitingQhead == NULL)
		{
			//printf("TIMER Queue is empty ..\n");
			return cPCB;
		}

		cPCB = WaitingQhead -> current_PCB;
		WaitingQtail = WaitingQhead = NULL;
	}
	else
	{
		cPCB = WaitingQhead -> current_PCB;
		//printf("Process state : %s\n\n\n",cPCB -> state);
		WaitingQhead = WaitingQhead->next;
	}
	//printf("removing head of waiting queue %ld\n",cPCB->process_id);
	return cPCB;
}

/********************************************************************************************************

	DequeueHeadFromReadyQueue() : 

	This routine removes head of the ready queue and returns the PCB of head element.
	Returns NULL when queue is empty


 ********************************************************************************************************/
PCB *DequeueHeadFromReadyQueue( )
{


	PCB *cPCB= NULL;


	if (IsEmptyQueue(ReadyQhead))
	{
		return cPCB;
	}
	if(ReadyQtail == ReadyQhead)
	{
		if (ReadyQhead == NULL)
		{
			return cPCB;
		}
		cPCB = ReadyQhead -> current_PCB;
		ReadyQtail = ReadyQhead = NULL;
		//printf("1st Process PCB is removed from READY queue\n");
	}
	else
	{
		cPCB = ReadyQhead -> current_PCB;
		ReadyQhead = ReadyQhead->next;
	}
	return cPCB;
}

/********************************************************************************************************

	DequeueFromReadyQueue() : 

	This routine finds process id in the queue and removes element from queue and return the PCB. If
	process id is not found on queue then it returns NULL.


 ********************************************************************************************************/

PCB* DequeueFromReadyQueue(long pid)
{
	QueuePtr *next1 = NULL;
	QueuePtr *prev = NULL;
	QueuePtr *ptr = ReadyQhead;
	PCB *PCB_to_return = NULL;


	if(!IsEmptyQueue(ReadyQhead))
	{
		if(ReadyQhead != NULL && ReadyQhead -> current_PCB -> process_id == pid)
		{
			PCB_to_return = ReadyQhead -> current_PCB;
			if (ReadyQhead == ReadyQtail)
				ReadyQhead = ReadyQtail = NULL;
			else
				ReadyQhead = ReadyQhead -> next;
		}
		else
		{
			while(ptr!=NULL )
			{
				prev = ptr;
				next1 = ptr -> next;
				if (next1 != NULL  && next1 -> current_PCB -> process_id == pid)
				{
					PCB_to_return = next1 -> current_PCB;
					prev -> next = next1 -> next;
					if(next1 == ReadyQtail)
						ReadyQtail = prev;
					break;
				}
				ptr = ptr -> next;
			}
		}
		ReadyQCounter --;
	}
	return PCB_to_return;
}

/********************************************************************************************************

	DequeueFromWaitingQueue() : 

	This routine finds process id in waiting queue and removes element from queue and return the PCB. If
	process id is not found on queue then it returns NULL.


 ********************************************************************************************************/
PCB* DequeueFromWaitingQueue(long pid)
{
	QueuePtr *next1 = NULL;
	QueuePtr *prev = NULL;
	QueuePtr *ptr = WaitingQhead;
	PCB *PCB_to_return = NULL;

	//printf("in dequeue function pid: %ld\n",pid);
	//printQueue(head);
	if(!IsEmptyQueue(WaitingQhead))
	{
		if(WaitingQhead != NULL && WaitingQhead -> current_PCB -> process_id == pid)
		{
			PCB_to_return = WaitingQhead -> current_PCB;
			if (WaitingQhead == WaitingQtail)
				WaitingQhead = WaitingQtail = NULL;
			else
				WaitingQhead = WaitingQhead -> next;
		}
		else
		{
			while(ptr!=NULL )
			{
				prev = ptr;
				next1 = ptr -> next;
				if (next1 != NULL  && next1 -> current_PCB -> process_id == pid)
				{
					PCB_to_return = next1 -> current_PCB;
					prev -> next = next1 -> next;
					if(next1 == WaitingQtail)
						WaitingQtail = prev;
					break;
				}
				ptr = ptr -> next;
			}
		}
		WaitingQCounter --;
	}
	return PCB_to_return;
}

/********************************************************************************************************

	AddInSuspenedQueue() : 

	This routine saves suspended process PCB on queue at tail of queue.

 ********************************************************************************************************/

void AddInSuspenedQueue(PCB *current_PCB)
{
	ReadyQueue *ptr = NULL;

	ptr = (SuspendedQueue*)malloc(sizeof(SuspendedQueue));
	ptr -> current_PCB = current_PCB;
	ptr -> next = NULL;

	//printf("ReadyQueue conuter : %d\n",ReadyQCounter);
	if (SuspendedQhead == NULL)
	{
		SuspenedQtail = SuspendedQhead = ptr;
		//ReadyQCounter++;
		return;
		//printf("Process PCB is added into READY Queue at the end\n");
	}
	else
	{
		SuspenedQtail -> next = ptr;
		SuspenedQtail = ptr;
		//ReadyQCounter++;
		return;
	}
}

/********************************************************************************************************

	DequeueFromSuspenedQueue() : 

	This routine finds process id on suspended queue then removes element from queue and return the PCB. 
	If process id is not found on queue then it returns NULL.


 ********************************************************************************************************/

PCB* DequeueFromSuspenedQueue(long pid)
{
	QueuePtr *next1 = NULL;
	QueuePtr *prev = NULL;
	QueuePtr *ptr = SuspendedQhead;
	PCB *PCB_to_return = NULL;



	//printQueue(head);
	if(!IsEmptyQueue(SuspendedQhead))
	{
		if(SuspendedQhead != NULL && SuspendedQhead -> current_PCB -> process_id == pid)
		{
			PCB_to_return = SuspendedQhead -> current_PCB;
			if (SuspendedQhead == SuspenedQtail)
				SuspendedQhead = SuspenedQtail = NULL;
			else
				SuspendedQhead = SuspendedQhead -> next;
		}
		else
		{
			while(ptr!=NULL )
			{
				prev = ptr;
				next1 = ptr -> next;
				if (next1 != NULL  && next1 -> current_PCB -> process_id == pid)
				{
					PCB_to_return = next1 -> current_PCB;
					prev -> next = next1 -> next;
					if(next1 == SuspenedQtail)
						SuspenedQtail = prev;
					break;
				}
				ptr = ptr -> next;
			}
		}
		//WaitingQCounter --;
	}
	return PCB_to_return;
}
/********************************************************************************************************

	AddInWaitingReceiveQueue() : 

	This routine saves PCB of a process which is waiting to receive message at tail of queue.


 ********************************************************************************************************/
void AddInWaitingReceiveQueue(PCB *current_PCB)
{
	WaitingToReceiveQueue *ptr = NULL;

	ptr = (SuspendedQueue*)malloc(sizeof(SuspendedQueue));
	ptr -> current_PCB = current_PCB;
	ptr -> next = NULL;

	//printf("ReadyQueue conuter : %d\n",ReadyQCounter);
	if (WaitingToReceiveQhead == NULL)
	{
		WaitingToReceiveQtail = WaitingToReceiveQhead = ptr;
		//ReadyQCounter++;
		return;
		//printf("Process PCB is added into READY Queue at the end\n");
	}
	else
	{
		WaitingToReceiveQtail -> next = ptr;
		WaitingToReceiveQtail = ptr;
		//ReadyQCounter++;
		return;
	}
}

/********************************************************************************************************

	DequeueFromWatingReceiveQueue() : 

	This routine finds process id on WaitingToReceive queue then removes element from queue and return
	the PCB. If process id is not found on queue then it returns NULL.


 ********************************************************************************************************/
PCB* DequeueFromWatingReceiveQueue(long pid)
{
	QueuePtr *next1 = NULL;
	QueuePtr *prev = NULL;
	QueuePtr *ptr = WaitingToReceiveQhead;
	PCB *PCB_to_return = NULL;


	//printQueue(head);
	if(!IsEmptyQueue(WaitingToReceiveQhead))
	{
		if(WaitingToReceiveQhead != NULL && WaitingToReceiveQhead -> current_PCB -> process_id == pid)
		{
			PCB_to_return = WaitingToReceiveQhead -> current_PCB;
			if (WaitingToReceiveQhead == WaitingToReceiveQtail)
				WaitingToReceiveQhead = WaitingToReceiveQtail = NULL;
			else
				WaitingToReceiveQhead = WaitingToReceiveQhead -> next;
		}
		else
		{
			while(ptr!=NULL )
			{
				prev = ptr;
				next1 = ptr -> next;
				if (next1 != NULL  && next1 -> current_PCB -> process_id == pid)
				{
					PCB_to_return = next1 -> current_PCB;
					prev -> next = next1 -> next;
					if(next1 == WaitingToReceiveQtail)
						WaitingToReceiveQtail = prev;
					break;
				}
				ptr = ptr -> next;
			}
		}
		//WaitingQCounter --;
	}
	return PCB_to_return;
}
/********************************************************************************************************

	AddInDiskQueue() :

	This routine saves PCB of a process which is waiting Disk write or read.


 ********************************************************************************************************/
void AddInDiskQueue(PCB *current_PCB,long disk_id)
{
	DiskQueue *ptr = NULL;

	//printf("adding in disk queue %ld\n",disk_id);
	ptr = (DiskQueue*)malloc(sizeof(DiskQueue));
	ptr -> current_PCB = current_PCB;
	ptr -> next = NULL;

	//printf("current_pcb : %ld disk_id %ld\n",ptr -> current_PCB -> process_id,disk_id);
	if (DiskQHead[disk_id] == NULL)
	{
		DiskQTail[disk_id] = DiskQHead[disk_id] = ptr;
		//printf("Process PCB is added into disk Queue as a first element\n");
		return;
	}
	else
	{
		DiskQTail[disk_id] -> next = ptr;
		DiskQTail[disk_id] = ptr;
		//ReadyQCounter++;
		return;
	}

}
/********************************************************************************************************

	DequeueFromDiskQueue() :

	This routine removes head from Disk Queue which is pointed by head and returns the PCB of head element.
	Returns NULL when queue is empty


 ********************************************************************************************************/
PCB *DequeueFromDiskQueue(long disk_id)
{


	PCB *cPCB= NULL;

	//printf("in dequeue function %ld\n",disk_id);
	if (IsEmptyQueue(DiskQHead[disk_id]))
	{
		return cPCB;
	}
	if(DiskQTail[disk_id] == DiskQHead[disk_id])
	{
		if ( DiskQHead[disk_id] == NULL)
		{
			//printf("returning null %ld--->\n",disk_id);
			return cPCB;
		}
		cPCB = DiskQHead[disk_id] -> current_PCB;
		DiskQTail[disk_id] = DiskQHead[disk_id] = NULL;
	}
	else
	{
		cPCB = DiskQHead[disk_id] -> current_PCB;
		DiskQHead[disk_id] = DiskQHead[disk_id]->next;

	}
	//printf("returning  %ld ---> %ld\n",cPCB->process_id);
	return cPCB;
}

/********************************************************************************************************

	IsSuspended() : 

	This routine checks whether process is already on suspended queue or not.

	Return values :

	1 : if process is already suspened.
	0 : process is not suspended


 ********************************************************************************************************/
int IsSuspended(long pid)
{
	QueuePtr *ptr = SuspendedQhead;
	while(ptr!=NULL)
	{
		if (ptr -> current_PCB -> process_id == pid)
		{
			return 1;
		}
		ptr = ptr ->next;
	}

	return 0;
}
/********************************************************************************************************

	IsEmptyQueue() : 

	This routine checks whether queue is empty or not.

	Return values :

	1 : if queue is empty.
	0 : queue is not empty.


 ********************************************************************************************************/
int IsEmptyQueue(struct Queue *head)
{
	if (head == NULL)
		return 1;
	return 0;
}
/********************************************************************************************************

	PrintQueue() : 

	This routine prints element of queue.


 ********************************************************************************************************/
void PrintQueue (QueuePtr *head)
{
	ReadyQueue *ptr;
	ptr = head;

	while (ptr != NULL)
	{
		printf("%ld\t", ptr -> current_PCB -> process_id);
		ptr = ptr -> next;
	}
	printf("\n");

}
/********************************************************************************************************

	GetReadyQElements() : 

	This routine stores all process id in ReadyQElement[] which are on ready queue.
	This ReadyQElement[] is used by state printer to print the elements of ready queue


 ********************************************************************************************************/
void GetReadyQElements()
{
	ReadyQueue		*ptr;
	INT32			i = 0;
	ptr = ReadyQhead;

	if (IsEmptyQueue(ptr))
		return;
	else
	{
		while (ptr != NULL)
		{
			ReadyQElement[i] = ptr -> current_PCB -> process_id;
			ptr = ptr -> next;
			i++;
		}
	}
}

/********************************************************************************************************

	GetSuspenedQElements() : 

	This routine stores all process id in SuspendedQElement[] which are on suspended queue.
	This SuspendedQElement[] is used by state printer to print the elements of suspended queue


 ********************************************************************************************************/
void GetSuspenedQElements()
{
	ReadyQueue		*ptr;
	INT32			i = 0;
	ptr = SuspendedQhead;

	if (IsEmptyQueue(ptr))
		return;
	else
	{
		while (ptr != NULL)
		{
			SuspendedQElement[i] = ptr -> current_PCB -> process_id;
			ptr = ptr -> next;
			i++;
		}
	}
}
/********************************************************************************************************

	GetWaitingQElements() : 

	This routine stores all process id in WaitingQElement[] which are on timer queue.
	This WaitingQElement[] is used by state printer to print the elements of timer dqueue


 ********************************************************************************************************/
void GetWaitingQElements()
{
	ReadyQueue		*ptr;
	INT32			i = 0;
	ptr = WaitingQhead;

	if (IsEmptyQueue(ptr))
		return;
	else
	{
		while (ptr != NULL)
		{
			WaitingQElement[i] = ptr -> current_PCB -> process_id;
			ptr = ptr -> next;
			i++;
		}
	}
}
/********************************************************************************************************

	GetWaitingQElements() :

	This routine stores all process id in WaitingQElement[] which are on timer queue.
	This WaitingQElement[] is used by state printer to print the elements of timer dqueue


 ********************************************************************************************************/
void GetDiskQElements(int disk_id)
{
	ReadyQueue		*ptr;
	INT32			i = 0;
	ptr = DiskQHead[disk_id];

	if (IsEmptyQueue(ptr))
		return;
	else
	{
		while (ptr != NULL)
		{
			DiskQElement[i] = ptr -> current_PCB -> process_id;
			ptr = ptr -> next;
			i++;
		}
	}
}
/********************************************************************************************************

	GetWaitingToReceiveQElements() : 

	This routine stores all process id in WaitingToReceiveQElement[] which are on timer queue.
	This WaitingToReceiveQElement[] is used by state printer to print the elements of timer dqueue


 ********************************************************************************************************/
void GetWaitingToReceiveQElements()
{
	ReadyQueue		*ptr;
	INT32			i = 0;
	ptr = WaitingToReceiveQhead;

	if (IsEmptyQueue(ptr))
		return;
	else
	{
		while (ptr != NULL)
		{
			Waiting_To_Recieve[i] = ptr -> current_PCB -> process_id;
			ptr = ptr -> next;
			i++;
		}
	}
}
/********************************************************************************************************

	SetQElementsToDefault() : 

	This routine assigns the ReadyQElement[], WaitingQElement[], SuspendedQElement[] to -1 
	after state printer done with printing. -1 is set because process id can not be negative.


 ********************************************************************************************************/
void SetQElementsToDefault()
{
	INT32			i;
	for(i =0 ; i < MAX_NUMBER_OF_PROCESSES ; i++)
	{
		ReadyQElement[i] = WaitingQElement[i] = SuspendedQElement[i]= Waiting_To_Recieve[i] =DiskQElement[i]= -1;
	}
}



