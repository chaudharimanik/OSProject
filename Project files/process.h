
/*
Auther	: Manik Chaudhari

This file contians declartion function and global variables
This function and golabl variables are visible in only operating system.
*/

#include				"stdio.h"
#include				"stdlib.h"
#include				"global.h"
#include				"syscalls.h"
#include				"protos.h"

#define					MAX_NUMBER_OF_PROCESSES		10
#define					SUCCESS						0L
#define					ERROR						-1L
#define					MAX_LEGAL_PRIORITY_NUMBER	100
#define					LEGAL_MESSAGE_LENGTH		64
#define					MAX_NUMBER_OF_MESSAGES		10
#define					SENDMESSAGE					1
#define					READY_TO_RECEIVE			0
#define					DEFAULT_PRIORITY			40


#define                  DO_LOCK                     1
#define                  DO_UNLOCK                   0
#define                  SUSPEND_UNTIL_LOCKED        TRUE
#define                  DO_NOT_SUSPEND              FALSE


/******************** Structure Definations **********************/
typedef struct diskInfo
{
	long 			disk_id;
	long			sector;
	char			*data;
	int				readWrite;
}DISKINFO;
typedef struct SHADOW_TABLE
{
	//int logical_pg_num;
	int disk_loc;
	int disk_id;
}SHADOW_TABLE;

/*Structure for PCB to contain process information*/
typedef struct Process_Control_Block
{
	long			process_id;
	char			process_name[15];
	char			*state;
	INT32			isDead;
	INT32			timedelay;
	long			priority;
	void			*context;
	DISKINFO		*diskInfo;
	//long			shadow_table[VIRTUAL_MEM_PGS];
	SHADOW_TABLE	*shadow_table[VIRTUAL_MEM_PGS];

}PCB;

/*Structure for process table to save existent process PCB*/
typedef struct Table
{
	PCB				*PCB[MAX_NUMBER_OF_PROCESSES];
}TABLE;



/*message structure to communicate messages between processes*/
typedef struct Message
{
	long			target_pid;
	long			source_pid;
	char			msg[LEGAL_MESSAGE_LENGTH];
	int				length;
	
		
}MESSAGE_BLOCK;



/*Structure defination for queue element*/
typedef struct Queue
{
	struct Queue	*next;
	PCB				*current_PCB;

}WaitingQueue, ReadyQueue,QueuePtr,SuspendedQueue,WaitingToReceiveQueue ,DiskQueue;

/******************************GLOBAL VARIABLES*******************************************/

int							proc_id;
int							message_counter;
char						*Done_process;

PCB							*Current_PCB;

TABLE						*ProcessTable;
int							proc_counter;
int							SuspendedProcCounter;
INT32						LockResult;
BOOL						isStatePrinter;
BOOL						waitingreceive;



//this register needs to pass process id in test1x
extern long					Z502_REG2;

INT32						ReadyQElement[MAX_NUMBER_OF_PROCESSES];
INT32						WaitingQElement[MAX_NUMBER_OF_PROCESSES];
INT32						SuspendedQElement[MAX_NUMBER_OF_PROCESSES];
INT32						Waiting_To_Recieve[MAX_NUMBER_OF_PROCESSES];
INT32						DiskQElement[MAX_NUMBER_OF_PROCESSES];

INT32						waitingMsgCounter;


MESSAGE_BLOCK				*MessageQueue[MAX_NUMBER_OF_MESSAGES];
WaitingQueue				*WaitingQhead; 
WaitingQueue				*WaitingQtail;
WaitingToReceiveQueue		*WaitingToReceiveQhead;
WaitingToReceiveQueue		*WaitingToReceiveQtail;
ReadyQueue					*ReadyQhead; 
ReadyQueue					*ReadyQtail;
SuspendedQueue				*SuspendedQhead;
SuspendedQueue				*SuspenedQtail;
DiskQueue					*DiskQHead[MAX_NUMBER_OF_DISKS];
DiskQueue					*DiskQTail[MAX_NUMBER_OF_DISKS];
DiskQueue					*diskhead,*disktail;

/*Function definations related to processes*/
//****************************************************************************************

long OsCreateProcess(char *process,void * test,INT32* p_id,long priority);
void SetPCBContents( char* process, INT32* p_id, PCB *currentPCB, long priority);
int IsDuplicateProcess(char * process_name);
void Dispatcher();
void MakeReady_To_Run();
long TerminateProc(long pid);
void PrintProcessTable();
PCB* getPCB(long pid);
long GetProcessID(char * pname, long* id);
long SuspendProcess(long id);
long ResumeProcess(long id);
int CheckParentProcess(char *name);
long CheckLegalID(long id);
long SendMessage(long target_id, long length, char *msg);
long RecieveMessage(long source_id, char *msg_buffer, long lenght_of_recieved_msg, long *lenght_of_sent_msg,long *sender_id);
void GiveUPCPU();
long ChangePriority(long id , long priority);
int SetStatePrinter_Mode(char *process);
void SuspendToReceive(long pid);
void ResumeToReceive(long id);
int SetMemoryPrinter_mode(char *process);

//Function definations related to Queues
//****************************************************************************************
void AddInWaitingQueue(PCB *current_PCB);
void AddInReadyQueue(PCB *current_PCB);
void AddInReadyQueueByPriority(PCB *current_PCB);
void AddInSuspenedQueue(PCB *current_PCB);
void AddInWaitingReceiveQueue(PCB *current_PCB);

PCB *DequeueHeadFromReadyQueue();
PCB *DequeueHeadFromWaitingQueue();
PCB* DequeueFromReadyQueue(long pid);
PCB* DequeueFromWaitingQueue(long pid);
PCB* DequeueFromSuspenedQueue(long pid);
PCB* DequeueFromWatingReceiveQueue(long pid);
void AddInDiskQueue(PCB *current_PCB,long disk_id);
PCB *DequeueFromDiskQueue(long disk_id);

int IsSuspended(long pid);
void PrintQueue (ReadyQueue *head);
int IsEmptyQueue(struct Queue *head);
void GetReadyQElements();
void GetWaitingQElements();
void GetWaitingToReceiveQElements();
void GetSuspenedQElements();
void GetDiskQElements(int disk_id);
void SetQElementsToDefault();
void DeleteReceivedMSG(INT32 pos);
void GiveUpCPU();
void Dispatcher();

