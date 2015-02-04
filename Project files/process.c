/*
Auther	: Manik Chaudhari


This file contains all process related function which operating system use.
Declaration of functions are in process.h

 */

#include	"process.h"
#include	"ctype.h"
#include	"string.h"


/********************************************************************************************************

        printProcessTable() :

        This routine prints all processes in the process table

 ********************************************************************************************************/
void PrintProcessTable()
{
	int j = 0;
	printf("*******************************************************************\n");
	printf("Number of processes : %d\n",proc_counter);
	printf("processes in table: \n");
	while(j < proc_counter)
	{

		//Now = ProcessTable -> PCB[i];
		printf("--->Process name : %s with priority %ld\n",ProcessTable -> PCB[j]-> process_name,
				ProcessTable -> PCB[j]-> priority);
		j++;
	}
	printf("*******************************************************************\n");
}

/********************************************************************************************************

        getPCB() :

        This routine gives pcb for process

 ********************************************************************************************************/
PCB* getPCB(long pid)
{
	int j = 0;
	PCB		*rPCB = NULL;

	while(j < proc_counter)
	{
		if(ProcessTable -> PCB[j]->process_id == pid)
		{
			rPCB = ProcessTable -> PCB[j];
		}
		j++;
	}
	return rPCB;
}

/********************************************************************************************************

        isDuplicateProcess() :

        This routine checks for duplicate process name in the process table.

		This function mainly used by OsCreateProcess() to check whether process with name is already 
		exists or  not

		Return values : 

		0 -> duplicate process name founds 
		1 -> no duplicate name 

 ********************************************************************************************************/

int IsDuplicateProcess(char * process_name)
{
	int i = 1;
	PCB *Now;

	//printProcessTable();
	while(i < proc_counter)
	{
		Now = ProcessTable -> PCB[i];
		if (strcmp(Now -> process_name,process_name) == 0)
		{
			return 0;
		}
		i++;
	}
	return 1;
}
/********************************************************************************************************

		CheckParentProcess ()
		 This routine checks name with array element to indentify to do switch context for user test
		 programs.
		 This function is used by OsCreateProcess().

		Return values : 

		1 -> name founds 
		0 -> no name  found


 ********************************************************************************************************/
int CheckParentProcess(char *name)
{
	int i;
	char *array [22] = {"test0","test1a","test1b","test1c","test1d","test1e","test1f",
			"test1g","test1h","test1i","test1j","test1k","test1l","test2a","test2b",
		"test2c","test2d","test2e","test2f","test2g","test2h","test2i"};
	for(i=0;i<22;i++)
	{
		if (strcmp(array[i],name)==0 )
		{
			return 1;
		}
	}
	return 0;
}

/********************************************************************************************************

	OsCreateProcess() : 

	This function creates a new process. 
	Function creates process if all parameter is correct and process is not already exist.

	Function returns ERROR for following contions:
		1. Illegal priority.
		2. Number of processes exceeds defined maximum number of process.
		3. Duplicate process name found.
	otherwise return SUCCESS code.

 ********************************************************************************************************/

long OsCreateProcess(char *process, void * test, INT32* p_id, long priority)
{
	void	*next_context;
	int returnCode  = 1;
	PCB *PCBptr;



	printf("--------------------------------------------------------------------------\n");

	if(priority < 0)
	{
		//printf("process counter %d\n",proc_counter);
		printf("\n(CREATE_PROCESS) ERROR : Illegal priority for process %s!!!\n",process);
		*p_id = -1;
		return ERROR;
	}

	if(proc_counter == MAX_NUMBER_OF_PROCESSES)
	{
		printf("\n(CREATE_PROCESS) ERROR: Can not create more than %d processes!!!\n",proc_counter);
		*p_id = -1;
		return ERROR;
	}
	CALL(returnCode = IsDuplicateProcess(process));
	if (returnCode == 0)
	{
		printf("\n(CREATE_PROCESS) ERROR : Process %s is already present!!!\n",process);
		*p_id = -1;
		return ERROR;
	}

	PCBptr = ( PCB *)malloc(sizeof ( PCB));
	ProcessTable -> PCB[proc_counter] = PCBptr;

	proc_counter ++;

	CALL(SetPCBContents(process, p_id, PCBptr, priority));

	CALL(Z502MakeContext( &next_context,test, USER_MODE ));

	PCBptr -> context = (void *)next_context;	//assigning the context pointer to process PCB

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

	CALL(AddInReadyQueueByPriority(PCBptr));

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

	/*If osInit routinue requests to create process for User test program then immidiately
		do the Swtich Context to execute process as a parent process.*/
	if (CheckParentProcess(process))
	{
		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		Current_PCB = DequeueHeadFromReadyQueue();
		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		SetStatePrinter_Mode(process);
		SetMemoryPrinter_mode(process);
		CALL(Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &Current_PCB -> context ));
	}

	return SUCCESS;
}

/********************************************************************************************************

        SetPCBContents() : 

        This routine sets parameters of process control block structure and 
		returns the process id of newly created process.

		This function ia mainly used by osCreateProcess().

 ********************************************************************************************************/
void SetPCBContents(char *process,INT32 *p_id, PCB *currentPCB,long priority)
{
	int		i;
	currentPCB -> process_id = proc_id;
	*p_id = proc_id;
	proc_id++;
	strcpy(currentPCB -> process_name,process);
	currentPCB -> timedelay = 0;
	currentPCB -> isDead = 0;
	currentPCB -> priority = priority;
	DISKINFO *diskinfo = ( DISKINFO *)calloc(1,sizeof ( DISKINFO));
	currentPCB->diskInfo = diskinfo;
	//allocate space for shadow table.
	for(i = 0;i< VIRTUAL_MEM_PGS;i++)
	{
		currentPCB->shadow_table[i] = ( SHADOW_TABLE *)calloc(1,sizeof ( SHADOW_TABLE));
		currentPCB->shadow_table[i] ->disk_id = -1;
		currentPCB->shadow_table[i] ->disk_loc = -1;
	}
}

/********************************************************************************************************

        TerminateProc() : 

        This routine terminates the child process by deleting entry in process- 
		table 

		Return values: 

		SUCCESS	:	if process is deleted.
		ERROR	:	try to terminate for non existent process.

 ********************************************************************************************************/

long TerminateProc(long pid)
{

	INT32		i;

	printf("*****************Enter in terminate manual process %ld\n",pid);
	//printf("process counter : %d\n",proc_counter);

	if(pid ==0 )
	{
		Z502Halt();
	}
	for (i=0;i<proc_counter ; i++)
	{
		if (ProcessTable -> PCB[i] -> process_id == pid)
			break;
	}
	if (i == proc_counter)
	{
		return ERROR;
	}
	if(i == proc_counter-1)
		ProcessTable -> PCB[proc_counter-1] = NULL;
	else
	{
		while(i < proc_counter-1)
		{
			ProcessTable -> PCB[i] = ProcessTable -> PCB[i+1];
			i++;
		}
		ProcessTable -> PCB[i] = NULL;
	}
	proc_counter--;
	DequeueFromReadyQueue(pid);
	DequeueFromWaitingQueue(pid);
	//	if(Current_PCB -> process_id != 0)
	Current_PCB -> isDead = 1;

	return SUCCESS;

}


/********************************************************************************************************

        GetProcessID() : 

        This routine gets id for process from process table. 

		SUCCESS	:	if process found
		ERROR	:	if process not found
		id      :   for process name if Process exist otherise return -1

 ********************************************************************************************************/
long GetProcessID(char *pname, long* id)
{
	//printf("in getid : %s\n",pname);
	int j = 0;
	if (strlen(pname) == 0)
	{
		*id = (long)Current_PCB -> process_id;
		return SUCCESS;
	}
	while(j < proc_counter)
	{
		if (strcmp(ProcessTable -> PCB[j]-> process_name,pname)==0)
		{
			*id = (long)ProcessTable -> PCB[j]-> process_id;
			
			return SUCCESS;
		}
		//printf("--->Process name : %s \n",ProcessTable -> PCB[j]-> process_name);
		j++;
	}
	if(j > proc_counter)
	{
		printf("------------------------->>>failed %s\n",pname);
	}

	printf("\n(GET_PROCESS_ID) ERROR : Process does not exist!!!!!!\n");
	*id = (long)-1;
	return ERROR;
}

/********************************************************************************************************

        ChecklegalID() : 

		This rotinue checks id in process table.

		Return values
		ERROR :			if ID is illegal.
		SUCCESS:		if ID is legal or found in process table.

 ********************************************************************************************************/

long CheckLegalID(long id)
{
	INT32		 j = 0;
	if (id > MAX_NUMBER_OF_PROCESSES)
		return ERROR;
	while(j < proc_counter)
	{
		if (ProcessTable -> PCB[j]-> process_id == id)
			return SUCCESS;
		j++;
	}
	return ERROR;
}

/********************************************************************************************************
        SuspendProcess() : 

		This routine suspends a process when SUSPEND_PROCESS system is called.

		This routine does error checking on parameters of suspend process. 
		If everything is ok then removes PCB from ready queue and puts on Suspened queue.


		Special case:

		It process calls SUSPEND_PROCESS on itself, adds PCB of current running process onto 
		suspended queue. 
		Calls GiveUpCPU(), Dispatcher() to schedule another process from ready queue.
		This function does not return back to this function until process is resumed by any routinue.

		Return Values:
		ERROR :			Illegal id, try to suspend itself and abnormal situation
		SUCCESS :		Upon successfull suspension


 ********************************************************************************************************/

long SuspendProcess(long id)
{
	PCB *rPCB = NULL;

	//printf("in suspend id : %ld\n",id);
	//printf("in dispatcher %ld context address : %p\n",Current_PCB -> process_id,Current_PCB -> context);
	//try to suspend current running process
	if (id == -1)
	{
		printf("Request to come from %ld to suspend itslef \n\n",Current_PCB -> process_id );

		AddInSuspenedQueue(Current_PCB);
		GiveUpCPU();
		Dispatcher();
		return SUCCESS;
	}
	if (CheckLegalID(id) == ERROR)
	{
		printf("\n(SUSPEND_PROCESS) ERROR : Illegal process id!!!!!\n");
		return ERROR;
	}
	//Find process on ready queue / timer queue. If process founds then removes from
	//ready queue/timer queue and put it on suspended queue.

	if(IsSuspended(id) == 1)
	{
		printf("\n(SUSPEND_PROCESS) ERROR : Process is already suspened!!! \n");
		return ERROR;
	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	rPCB = DequeueFromReadyQueue(id);
	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	if (rPCB == NULL)
	{
		READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		rPCB = DequeueFromWaitingQueue(id);
		READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
	}
	if (rPCB == NULL)
	{
		rPCB = DequeueFromWatingReceiveQueue(id);
	}
	//if process id is legal and NULL returns from ready queue or waiting queue
	//then Error should have happened.
	if (rPCB == NULL)
	{
		printf("\n(SUSPEND_PROCESS) ERROR : Process is not found!!! \n");
		return ERROR;
	}
	AddInSuspenedQueue(rPCB);

	return SUCCESS;
}

/********************************************************************************************************


        ResumeProcess() :

		This routinue resumes suspended process and put back on ready queue to run.

		Return Values: 
		ERROR :			Illegal ID, try to resume itself and unsuspened process.
		SUCCESS:		upon successfull resumption of legal process

 ********************************************************************************************************/

long ResumeProcess(long id)
{

	PCB *rPCB = NULL;
	//try to resume itself
	if (id == -1)
	{
		printf("\n(RESUME_PROCESS) ERROR : Can not resume itself!!! \n");
		return ERROR;
	}
	//try to resume the current running process
	if (id == Current_PCB -> process_id)
	{
		printf("\n(RESUME_PROCESS) ERROR : Can not resume itself!!! \n");
		return ERROR;
	}
	//Error should have happened if nothing is there to resume
	if (CheckLegalID(id) == ERROR)
	{
		printf("\n(RESUME_PROCESS) ERROR : Process ID is illegal!!! \n");
		return ERROR;
	}
	else if (IsSuspended(id) == 0 )
	{
		printf("\n(RESUME_PROCESS) ERROR : Try to resume unsuspened process!!! \n");
		return ERROR;
	}
	else
	{
		READ_MODIFY(MEMORY_INTERLOCK_BASE+2, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		rPCB = DequeueFromSuspenedQueue(id);
		READ_MODIFY(MEMORY_INTERLOCK_BASE+2, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		if (rPCB != NULL)
		{
			READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
			AddInReadyQueueByPriority(rPCB);
			READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
			return SUCCESS;
		}
		else
		{
			printf("\n(RESUME_PROCESS) ERROR : Something wrong happened while removigng process from suspened queue!!! \n");
			return ERROR;
		}
	}

	return SUCCESS;
}

/********************************************************************************************************

        ChangePriority():

		This routinue changes an old priority with new priority.
		If process is not on ready queue then checks on waiting queue.
		When found it changes priority and add it on queue from where it dequeued

		Return values: 

		ERROR :			> MAX_LEGAL_PRIORITY_NUMBER, Illegal id , non-existent process
		SUCCESS :		upon successfull operation

 ********************************************************************************************************/

long ChangePriority(long id , long priority)
{
	INT32		IsEmpty;
	PCB			*PCB_to_change;

	///printProcessTable();
	if (id == -1)
	{
		Current_PCB -> priority = priority;
		//printProcessTable();
		return SUCCESS;
	}
	//printProcessTable();
	if (CheckLegalID(id) == ERROR)
	{
		printf("\n(CHANGE_PRIORITY) ERROR : Process does not exists!!!!\n");
		return ERROR;
	}
	if (priority > MAX_LEGAL_PRIORITY_NUMBER)
	{
		printf("\n(CHANGE_PRIORITY) ERROR : Illegal priority!!!!\n");
		return ERROR;
	}
	IsEmpty = IsEmptyQueue(ReadyQhead);
	if (!IsEmpty)
	{
		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		PCB_to_change = DequeueFromReadyQueue(id);

		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		/*If process is not on ready queue then check in Waiting Queue*/
		if (PCB_to_change == NULL)
		{
			READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
			PCB_to_change = DequeueFromWaitingQueue(id);
			READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
			if (PCB_to_change == NULL)
			{
				printf("\n(CHANGE_PRIORITY) ERROR : Process is not found!!!!\n\n");
				return ERROR;
			}
			else
			{
				READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
				printf("Changing priority for Process : %ld\n",id);
				PCB_to_change -> priority = priority;
				AddInWaitingQueue(PCB_to_change);
				READ_MODIFY(MEMORY_INTERLOCK_BASE+1, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
				return SUCCESS;
			}
		}
		else if(PCB_to_change!=NULL)
		{
			PCB_to_change -> priority = priority;
			printf("Changing priority for Process : %ld\n",id);
			READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
			AddInReadyQueueByPriority(PCB_to_change);
			READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

			return SUCCESS;
		}
		else
		{
			printf("\n(CHANGE_PRIORITY) ERROR : Something is wrong and process is lost!!!!\n");
		}
	}
	return SUCCESS;
}

/********************************************************************************************************

        SendMessage() :

		This routinue send message to MesageBox for target_id process to receive.

		This routine store message into msg buffer and also saves target_id and source_id if its valid. 
		Therefore only target_id can receive a message.

		if target id is -1 it means message is broadcast so allows to send message .
		After sending message to messagebox, it resumes target process if its waiting for a message
		otherwise is target id -1, resumes 1st element on WaitingToReceive queue. This is because 
		message is broadcast and any one can receive it.

		This routine only allows MAX_NUMBER_OF_MESSAGES = 10 to present in MessageBox
		othersise it should return error.

		Return values: 

		ERROR  : illegal message length, msg length is greater than buffer size,
				target_id does not exist
		SUCESS : all parameters are ok and on successfull sending


 ********************************************************************************************************/

long SendMessage(long target_id, long message_send_length, char *msg)
{

	//printf("-------------------------------------------------------->>>%ld sending msg to %ld\n",Current_PCB -> process_id,target_id);
	if(message_send_length > LEGAL_MESSAGE_LENGTH)
	{
		printf("\n(SEND) ERROR : send_length can not be more than LEGAL_MESSAGE_LENGTH (%d) !!!\n",LEGAL_MESSAGE_LENGTH);
		return ERROR;
	}
	if (strlen(msg) <= message_send_length)
	{
		if (target_id == -1 || CheckLegalID(target_id) != ERROR )
		{
			if(MessageQueue != NULL)
			{
				if(  message_counter < MAX_NUMBER_OF_MESSAGES)
				{

					strcpy(MessageQueue[message_counter] -> msg,msg);
					MessageQueue[message_counter] -> length  = message_send_length;
					MessageQueue[message_counter] -> target_pid = target_id;
					MessageQueue[message_counter] -> source_pid = Current_PCB -> process_id;
					message_counter++;
					//printf("msg conuter: %d\n", message_counter);
					if(target_id != -1)
					{
						if(!IsEmptyQueue(WaitingToReceiveQhead))
						{
							ResumeToReceive(target_id);
						}
					}
					else
					{
						if(!IsEmptyQueue(WaitingToReceiveQhead))
						{
							ResumeToReceive(WaitingToReceiveQhead -> current_PCB -> process_id);
						}
					}

					return SUCCESS;
				}
				else
				{
					printf("\n(SEND) Error: Sorry can not sent a message. OS Message Buffer is full now!!!\n");
					return ERROR;
				}
			}
			else
			{
				printf("\nError: Allocating space to MessageStrusture!!!\n\n");
				return ERROR;
			}
		}
		else
		{
			printf("\n(SEND) ERROR : Target_id is not valid!!!\n");
			return ERROR;
		}
	}
	else
	{
		printf("\n(SEND) ERROR : Sent message buffer size is too small to send message!!!\n");
		return ERROR;
	}
}


/********************************************************************************************************

        RecieveMessage() :

		This routine receives a message if found and deletes it.
		If process does not find any message for itself it suspends itself by calling SuspendToReceive()
		until message is sent to it.

		Return values: 
		ERROR	: reciever buffer size is greater than LEGAL_MESSAGE_LENGTH(64),
					source id process does not exist or -1.
		SUCCESS : on successfull receiving of message.


 ********************************************************************************************************/

long RecieveMessage(long source_id, char *msg_buffer, long lenght_of_recieved_msg, long *lenght_of_sent_msg,long *sender_id)
{

	INT32					pos = 0;


	if (lenght_of_recieved_msg > LEGAL_MESSAGE_LENGTH)
		return ERROR;
	if ( MessageQueue[pos] -> length > lenght_of_recieved_msg)
	{
		printf("\nERROR : Reciever buffer is small!!!\n");
		return ERROR;
	}

	if(source_id == -1 || CheckLegalID(source_id) != ERROR)
	{
		while(1)
		{
			//printf("-------------------------------------------------------->>>%ld receiving msg form %ld\n",Current_PCB -> process_id,source_id);
			pos = 0;
			//printf("pos  :%d msg counter : %d  \n",pos,message_counter );
			while(pos < message_counter)
			{
				//printf("%ld\t",MessageQueue[pos] -> target_pid);
				if ((MessageQueue[pos] -> source_pid == source_id || source_id == -1)&&
						(MessageQueue[pos] -> target_pid == Current_PCB -> process_id || MessageQueue[pos] -> target_pid == -1))
				{
					break;
				}
				pos++;
			}

			if (pos < message_counter && MessageQueue[pos] -> source_pid != Current_PCB -> process_id)
			{

				strcpy(msg_buffer , MessageQueue[pos] -> msg);
				*lenght_of_sent_msg = MessageQueue[pos] -> length;
				*sender_id = MessageQueue[pos] -> source_pid;
				//deleting message which process just has read

				DeleteReceivedMSG(pos);

				return SUCCESS;

			}
			else
			{
				printf("\nNo message found for %ld.Suspending itself\n ",Current_PCB -> process_id);
				SuspendToReceive(-1);
			}
		}//while loop ends

	}
	else
	{
		printf("ERROR : Source is not valid!!!\n");
		return ERROR;
	}
	return SUCCESS;
}

/********************************************************************************************************

		SetStatePrinter_Mode() :

		This routine sets State_printer mode for required uset test program. 

 ********************************************************************************************************/

int SetStatePrinter_Mode(char *process)
{
	if (strcmp(process,"test1c") == 0 || strcmp(process,"test1h") == 0 ||
			strcmp(process,"test1f") == 0 || strcmp(process,"test1j") == 0||
			strcmp(process,"test1l") == 0 || strcmp(process,"test1d") == 0 ||
		strcmp(process,"test2c") == 0||strcmp(process,"test2d") == 0)
	{
		isStatePrinter = TRUE;
		return 1;
	}
	isStatePrinter = FALSE;
	return 0;
}



/********************************************************************************************************

		DeleteReceivedMSG() :

		This routine deletes a received message form MessageBox. 

 ********************************************************************************************************/
void DeleteReceivedMSG(INT32 pos)
{

	INT32       i;

	for(i = pos; i <  message_counter-1 ; i++)
	{

		strcpy(MessageQueue[i] -> msg,MessageQueue[i+1] -> msg);
		MessageQueue[i] ->length = MessageQueue[i+1] -> length;
		MessageQueue[i] -> source_pid = MessageQueue[i+1] -> source_pid;
		MessageQueue[i] -> target_pid = MessageQueue[i+1] -> target_pid;

	}
	if ( i == message_counter-1)
	{
		strcpy(MessageQueue[i] -> msg,"");
		MessageQueue[i] -> length = 0;
		MessageQueue[i] -> source_pid = -2;
		MessageQueue[i] -> target_pid = -2;
	}

	message_counter -- ;
	if ( message_counter == 0)
	{
		strcpy(MessageQueue[message_counter] -> msg,"");
		MessageQueue[message_counter] -> length = 0;
		MessageQueue[message_counter] -> source_pid = -2;
		MessageQueue[message_counter] -> target_pid = -2;

	}

}

/********************************************************************************************************

		SuspendToReceive() :

		This routine suspends current process if it does not find any message for itself. 

		It saves Current process PCB on to WaitingToReceive queue.
		Call GiveUpCPU() and Dispatcher() to run another process.

		Process is not resumed until send is made.

 ********************************************************************************************************/

void SuspendToReceive(long pid)
{

	if(pid == -1)
	{
		printf("Request to come from %ld to suspend itslef \n\n",Current_PCB -> process_id );

		AddInWaitingReceiveQueue(Current_PCB);
		GiveUpCPU();
		Dispatcher();
	}
}
/********************************************************************************************************

		ResumeToReceive() :

		This routine resumes process if send is made for it. 
		This routine dequeue suspended process and put it on ready queue to receive a message.


 ********************************************************************************************************/
void ResumeToReceive(long id)
{
	PCB		*returnedPCB;


	returnedPCB = DequeueFromWatingReceiveQueue(id);
	if (returnedPCB != NULL)
	{
		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult);
		AddInReadyQueueByPriority(returnedPCB);
		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult);

	}
}
