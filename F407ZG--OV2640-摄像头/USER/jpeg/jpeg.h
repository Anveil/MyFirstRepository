#ifndef _JPEG_H
#define _JPEG_H

#include "ff.h"

#define DELETE_MODE 	1  
#define PHOTO_MODE 		2 

     					  /* 文件成功读写数量 */
extern int jpg_cnt;

extern DWORD fre_clust, fre_sect, tot_sect;
extern TCHAR sys_status;



void JpegInit(void);
void GetFreeSpaceNoPrint(FRESULT res,FATFS *fs);
void GetFreeSpace(FRESULT res,FATFS *fs);
void JpegSaveHandler(void);
void SaveJpeg(FATFS *fs,FIL *fnew);

#endif
