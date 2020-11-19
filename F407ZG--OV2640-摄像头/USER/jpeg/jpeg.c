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



#define jpeg_buf_size 31*1024  			//����JPEG���ݻ���jpeg_buf�Ĵ�С(*4�ֽ�)



int jpg_cnt=0;

__align(4) u32 jpeg_buf[jpeg_buf_size];	//JPEG���ݻ���buf
DWORD fre_clust, fre_sect, tot_sect;

volatile u32 jpeg_data_len = 0; 			//buf�е�JPEG��Ч���ݳ���
volatile u8 jpeg_data_ok = 0;				//JPEG���ݲɼ���ɱ�־





//����JPEG����
//���ɼ���һ֡JPEG���ݺ�,���ô˺���,�л�JPEG BUF.��ʼ��һ֡�ɼ�.
//��DCMI_IRQHandler�ж������

void JPEG_Data_Process(void)
{

	if(jpeg_data_ok == 0)	//jpeg���ݻ�δ�ɼ���?
	{
			DMA_Cmd(DMA2_Stream1, DISABLE);//ֹͣ��ǰ����
			while (DMA_GetCmdStatus(DMA2_Stream1) != DISABLE) {} //�ȴ�DMA2_Stream1������
			jpeg_data_len = jpeg_buf_size - DMA_GetCurrDataCounter(DMA2_Stream1); //�õ��˴����ݴ���ĳ���

			jpeg_data_ok = 1; 				//���JPEG���ݲɼ��갴��,�ȴ�������������
	}
	if(jpeg_data_ok == 2)	//��һ�ε�jpeg�����Ѿ���������
	{
			DMA2_Stream1->NDTR = jpeg_buf_size;
			DMA_SetCurrDataCounter(DMA2_Stream1, jpeg_buf_size); //���䳤��Ϊjpeg_buf_size*4�ֽ�
			DMA_Cmd(DMA2_Stream1, ENABLE);			//���´���
			jpeg_data_ok = 0;						//�������δ�ɼ�
	}
		
}


void GetFreeSpaceNoPrint(FRESULT res,FATFS *fs)
{
    /*��ȡʣ����������ʣ��ռ�*/
    
    /* Get volume information and free clusters of drive 1 */
    res = f_getfree("0:", &fre_clust, &fs);
    if (res)
    {
        printf("���ִ���%d��",res);
    }

    /* Get total sectors and free sectors */
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;
     
}


void GetFreeSpace(FRESULT res,FATFS *fs)
{
    /*��ȡʣ����������ʣ��ռ�*/
    
    /* Get volume information and free clusters of drive 1 */
    res = f_getfree("0:", &fre_clust, &fs);
    if (res)
    {
        printf("���ִ���%d��",res);
    }

    /* Get total sectors and free sectors */
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;

    /* Print the free space (assuming 512 bytes/sector) */
    printf("��������:  %u KB\t��������: %u KB\n",fre_sect / 2, tot_sect / 2-fre_sect / 2);
            
}
            
            



void GetTimeAndDateString(u8 *namebuf)
{
    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_DateTypeDef RTC_DateStructure;
    char TimeTemp[20],DateTemp[20];
    
    /*��ȡ��ǰ����ʱ��*/
    RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
    sprintf(DateTemp,"20%0.2d-%0.2d-%0.2d", 
        RTC_DateStructure.RTC_Year,
        RTC_DateStructure.RTC_Month, 
        RTC_DateStructure.RTC_Date);

    //void RTC_GetTime(uint32_t RTC_Format, RTC_TimeTypeDef* RTC_TimeStruct)
    RTC_GetTime(RTC_Format_BIN,&RTC_TimeStructure);
    sprintf(TimeTemp,"%0.2d-%0.2d-%0.2d", 
        RTC_TimeStructure.RTC_Hours, 
        RTC_TimeStructure.RTC_Minutes, 
        RTC_TimeStructure.RTC_Seconds);

    sprintf((char *)namebuf,"%s.%s(%d).jpg",DateTemp,TimeTemp,jpg_cnt++);
    puts((char *)namebuf);
}




//JPEG����

void JPEG_Save()
{
    FATFS *fs;													/* FatFs�ļ�ϵͳ���� */
    FIL fnew;													/* �ļ����� */
    FRESULT res_sd;                /* �ļ�������� */
    UINT fnum;   
    u32 i,jpgstart,jpglen; 
    u8 *p;
    u8 headok=0;
    u8 namebuf[50];
    
    
    
    OV2640_JPEG_Mode();		//JPEGģʽ
    My_DCMI_Init();			//DCMI����
	
    DCMI_DMA_Init((u32)&jpeg_buf, jpeg_buf_size, DMA_MemoryDataSize_Word, DMA_MemoryInc_Enable); //DCMI DMA����
		
    OV2640_OutSize_Set(800, 600); //��������ߴ�
		
    DCMI_Start(); 		//��������
    
    
    

    while(1)
    {
        if(jpeg_data_ok == 1)	//�Ѿ��ɼ���һ֡ͼ����
        {
            p = (u8*)jpeg_buf;
            
            GetFreeSpace(res_sd,fs);

            if((tot_sect/2-fre_sect/2)>1048576)        //�ж������Ƿ���
            {
                printf("��������,��Ҫɾ��\n");
                FindOldestFile(fs,"0:");
                continue;
            }
            
            

            GetTimeAndDateString(namebuf);
            
            /*��ͷβ����¼����*/
            jpglen=0;	//����jpg�ļ���СΪ0
            headok=0;	//���jpgͷ���
            for(i=0;i<jpeg_data_len*4;i++)//����0XFF,0XD8��0XFF,0XD9,��ȡjpg�ļ���С
            {
                if((p[i]==0XFF)&&(p[i+1]==0XD8))//�ҵ�FF D8
                {
                    jpgstart=i;
                    headok=1;	//����ҵ�jpgͷ(FF D8)
                }
                if((p[i]==0XFF)&&(p[i+1]==0XD9)&&headok)//�ҵ�ͷ�Ժ�,����FF D9
                {
                    jpglen=i-jpgstart+2;
                    break;
                }
            }
            
            /*�������ݲ���*/
            p+=jpgstart;			//ƫ�Ƶ�0XFF,0XD8��
            if(jpglen)	//������jpeg����
            {
//                printf("%d",jpglen);
                res_sd = f_open(&fnew,(TCHAR *)namebuf,FA_CREATE_ALWAYS | FA_WRITE);     //���ļ�
                if ( res_sd == FR_OK )
                {
//                    printf("����/��������ͼƬ�ɹ�\r\n");
                /* ��ָ���洢������д�뵽�ļ��� */
                    res_sd=f_write(&fnew,p,sizeof(u8)*jpglen,&fnum);
                    if(res_sd==FR_OK)
                    {
//                            printf("д�����:(%d)\n",res_sd);
                    }
                    else
                    {
                        printf("�����ļ�д��ʧ�ܣ�(%d)\n",res_sd);
                    }    
                        /* ���ٶ�д���ر��ļ� */
                    f_close(&fnew);
                }
                else
                {	
                    printf("������/�����ļ�ʧ�ܡ�\r\n");
                }
            }
                jpeg_data_ok = 2;	//���jpeg���ݴ�������,������DMAȥ�ɼ���һ֡��.
    //			delay_ms(100);
        }
    }
	
}







