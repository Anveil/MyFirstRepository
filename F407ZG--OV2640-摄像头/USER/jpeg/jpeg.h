#ifndef _JPEG_H
#define _JPEG_H

#include "ff.h"

#define DELETE_MODE 	1  
#define PHOTO_MODE 		2 

     					  /* �ļ��ɹ���д���� */
extern int jpg_cnt;

extern DWORD fre_clust, fre_sect, tot_sect;
extern TCHAR sys_status;


void GetFreeSpaceNoPrint(FRESULT res,FATFS *fs);
void GetFreeSpace(FRESULT res,FATFS *fs);
void JPEG_Data_Process(void);
void JPEG_Save(void);

#endif
