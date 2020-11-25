#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "ff.h"
#include "timer.h"
#include "ov2640.h"
#include "dcmi.h"
#include "bsp_rtc.h"
#include "jpeg.h"
#include "string.h"


#define jpeg_buf_size 31*1024  			//定义JPEG数据缓存jpeg_buf的大小(*4字节)



int jpg_cnt=0;

u8 lastHour;
u8 needMakeDir=0;
u8 dirNameBuffer[50];
__align(4)  u32 jpeg_buf[jpeg_buf_size];	//JPEG数据缓存buf
DWORD fre_clust, fre_sect, tot_sect;

volatile u32 jpeg_data_len = 0; 			//buf中的JPEG有效数据长度
volatile u8 jpeg_data_ok = 0;				//JPEG数据采集完成标志




void JpegInit()
{
    OV2640_JPEG_Mode();		//JPEG模式
    My_DCMI_Init();			//DCMI配置
	
    DCMI_DMA_Init((u32)&jpeg_buf, jpeg_buf_size, DMA_MemoryDataSize_Word, DMA_MemoryInc_Enable); //DCMI DMA配置
		
    OV2640_OutSize_Set(800, 600); //设置输出尺寸
		
    DCMI_Start(); 		//启动传输
}


//处理JPEG数据
//当采集完一帧JPEG数据后,调用此函数,切换JPEG BUF.开始下一帧采集.
//在DCMI_IRQHandler中断里调用

void JpegSaveHandler(void)
{

	if(jpeg_data_ok == 0)	//jpeg数据还未采集完?
	{
			DMA_Cmd(DMA2_Stream1, DISABLE);//停止当前传输
			while (DMA_GetCmdStatus(DMA2_Stream1) != DISABLE) {} //等待DMA2_Stream1可配置
			jpeg_data_len = jpeg_buf_size - DMA_GetCurrDataCounter(DMA2_Stream1); //得到此次数据传输的长度

			jpeg_data_ok = 1; 				//标记JPEG数据采集完按成,等待其他函数处理
	}
	if(jpeg_data_ok == 2)	//上一次的jpeg数据已经被处理了
	{
			DMA2_Stream1->NDTR = jpeg_buf_size;
			DMA_SetCurrDataCounter(DMA2_Stream1, jpeg_buf_size); //传输长度为jpeg_buf_size*4字节
			DMA_Cmd(DMA2_Stream1, ENABLE);			//重新传输
			jpeg_data_ok = 0;						//标记数据未采集
	}
		
}


void GetFreeSpaceNoPrint(FRESULT res,FATFS *fs)
{
    /*获取剩余扇区计算剩余空间*/
    
    /* Get volume information and free clusters of drive 1 */
    res = f_getfree("0:", &fre_clust, &fs);
    if (res)
    {
        printf("出现错误（%d）",res);
    }

    /* Get total sectors and free sectors */
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;
     
}


void GetFreeSpace(FRESULT res,FATFS *fs)
{
    /*获取剩余扇区计算剩余空间*/
    
    /* Get volume information and free clusters of drive 1 */
    res = f_getfree("0:", &fre_clust, &fs);
    if (res)
    {
        printf("出现错误（%d）",res);
    }

    /* Get total sectors and free sectors */
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;

    /* Print the free space (assuming 512 bytes/sector) */
    printf("可用容量:  %u KB\t已用容量: %u KB\n",fre_sect / 2, tot_sect / 2-fre_sect / 2);
}
            
            



void GetTimeAndDateString(u8 *namebuf)
{
    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_DateTypeDef RTC_DateStructure;
    char timeTemp[50],dateTemp[50],dirNameTemp[50];
    
    /*获取当前日期时间*/
    RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
    sprintf(dateTemp,"20%0.2d年%0.2d月%0.2d日", 
        RTC_DateStructure.RTC_Year,
        RTC_DateStructure.RTC_Month, 
        RTC_DateStructure.RTC_Date);

    //void RTC_GetTime(uint32_t RTC_Format, RTC_TimeTypeDef* RTC_TimeStruct)
    RTC_GetTime(RTC_Format_BIN,&RTC_TimeStructure);
    sprintf(timeTemp,"%0.2d时%0.2d分%0.2d秒", 
        RTC_TimeStructure.RTC_Hours, 
        RTC_TimeStructure.RTC_Minutes, 
        RTC_TimeStructure.RTC_Seconds);
    
    //拼接文件夹名字
    sprintf(dirNameTemp,"0:20%0.2d年%0.2d月%0.2d日%0.2d时--%0.2d时", 
        RTC_DateStructure.RTC_Year,
        RTC_DateStructure.RTC_Month, 
        RTC_DateStructure.RTC_Date,
        RTC_TimeStructure.RTC_Hours,
        RTC_TimeStructure.RTC_Hours+1);
    
    if(lastHour!=RTC_TimeStructure.RTC_Hours)
    {
        
        printf("建立文件夹\n");
        f_mkdir(dirNameTemp);
    }
    
    lastHour=RTC_TimeStructure.RTC_Hours;

    sprintf((char *)namebuf,"%s/%s%s.jpg",dirNameTemp,dateTemp,timeTemp);
    puts((char *)namebuf);
}




//JPEG保存

void SaveJpeg(FATFS *fs,FIL *fnew)
{
    													/* 文件对象 */
    FRESULT res_sd;                /* 文件操作结果 */
    UINT fnum;   
    u32 i,jpgstart,jpglen; 
    u8 *p;
    u8 headok=0;
    u8 namebuf[100];
    char *dirname;
    char allRootDirName[50][200];
    
    
    FRESULT res;
    DIR   dir;     /* 文件夹对象 */ //36  bytes
    FILINFO fno;   /* 文件属性 */   //32  bytes
    u32 dirCnt=0;
    static TCHAR file[_MAX_LFN + 2] = {0};
    static TCHAR filename[100];
#if _USE_LFN
    TCHAR lname[_MAX_LFN + 2] = {0};
#endif
    
#if _USE_LFN
    fno.lfsize = _MAX_LFN;
    fno.lfname = lname;    //必须赋初值
#endif

    p = (u8*)jpeg_buf;
    
    
    

    
    /*找头尾，记录长度*/
    jpglen=0;	//设置jpg文件大小为0
    headok=0;	//清除jpg头标记
    for(i=0;i<jpeg_data_len*4;i++)//查找0XFF,0XD8和0XFF,0XD9,获取jpg文件大小
    {
        if((p[i]==0XFF)&&(p[i+1]==0XD8))//找到FF D8
        {
            jpgstart=i;
            headok=1;	//标记找到jpg头(FF D8)
        }
        if((p[i]==0XFF)&&(p[i+1]==0XD9)&&headok)//找到头以后,再找FF D9
        {
            jpglen=i-jpgstart+2;
            break;
        }
    }
    
    
    if((tot_sect/2-fre_sect/2)>30000000 )        //判断容量是否足
    {
        dirname=GetDirNameToDel("");
        f_deldir(dirname);
    }

    GetTimeAndDateString(namebuf);
    
    /*保存数据部分*/
    p+=jpgstart;			//偏移到0XFF,0XD8处
    if(jpglen)	//正常的jpeg数据
    {
//                printf("%d",jpglen);
        res_sd = f_open(fnew,(TCHAR *)namebuf,FA_CREATE_ALWAYS | FA_WRITE);     //打开文件
        if ( res_sd == FR_OK )
        {
                printf("》打开/创建测试图片成功\r\n");
        /* 将指定存储区内容写入到文件内 */
            res_sd=f_write(fnew,p,sizeof(u8)*jpglen,&fnum);
            if(res_sd==FR_OK)
            {
                printf("写入完成:(%d)\n",res_sd);
            }
            else
            {
                printf("！！文件写入失败：(%d)\n",res_sd);
            }    
                /* 不再读写，关闭文件 */
            f_close(fnew);
        }
        else
        {	
            printf("！！打开/创建文件失败。%d\r\n",res_sd);
        }
    }
        jpeg_data_ok = 2;	//标记jpeg数据处理完了,可以让DMA去采集下一帧了.
			delay_ms(1000);
}







