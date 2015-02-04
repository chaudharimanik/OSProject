/*
 * page.h
 *
 *  Created on: Oct 26, 2013
 *      Author: mkchaudhari
 */

#ifndef PAGE_H_
#define PAGE_H_
#include				"global.h"
#include				"stdio.h"
#include             	"syscalls.h"
#include            	"protos.h"

#define 				PAGE_TABLE_SIZE		1024
#define					MAX_NUM_OF_FILES		10

UINT16 *pgTable;

/*frame table declaration
 * added extra pagePointer variable to point logical page of process's page table
 * */
typedef struct frameTable
{
	long		 			process_id;
	int 		 			logical_page_num;
	UINT16					*pagePointer;
	BOOL					isFree;
}FRAMETABLE;

/*Structure for File*/
typedef struct
{
	char					file_name[15];
	char					*directory;
	int						disk;
	int						disk_loc[10];
	short int 				type;
	int						time;
	int						Owner;
	char					*data;
}FILE_STRUCT;

FILE_STRUCT   		*file_table[MAX_NUM_OF_FILES];
FILE_STRUCT			*open_files[MAX_NUM_OF_FILES];
FRAMETABLE 			*frame_table[PHYS_MEM_PGS];

int frame_num[PHYS_MEM_PGS];
long sector;
//defination of disk function
void DiskRead(long disk_id,long sector, char *data);
void DiskWrite(long disk_id,long sector, char *data);
void GetDiskLocation(long *disk_id,long *disk_loc);

//phyical memory and frame table related function
int GetFrame();
void MakeFrameTableEntry(int frame_num,int logical_pg_num,long pid);
int IsPhyMemoryFull();
int GetLRUFrame();
int GetFIFOFrame();
void FreeFramesOfProcess(long id);


#endif /* PAGE_H_ */
