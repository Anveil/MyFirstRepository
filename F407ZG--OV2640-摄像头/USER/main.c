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




//STM32F407������
//����ͷ ʵ�� -�⺯���汾


//D1��˸˵��DCMI_IRQHandler�жϱ�������������ͼ��

u32 jpg_cnt=0;
extern u8 ov_frame;  						//֡��
extern uint8_t Key_Flag;//������ֵ
u8 Com1SendFlag;//����1�������ݱ��






//0,����û�вɼ���;
//1,���ݲɼ�����,���ǻ�û����;
//2,�����Ѿ����������,���Կ�ʼ��һ֡����
//JPEG�ߴ�֧���б�
//const u16 jpeg_img_size_tbl[][2] =
//{
//    176, 144,	//QCIF
//    160, 120,	//QQVGA
//    352, 288,	//CIF
//    320, 240,	//QVGA
//    640, 480,	//VGA
//    800, 600,	//SVGA
//    1024, 768,	//XGA
//    1280, 1024,	//SXGA
//    1600, 1200,	//UXGA
//};


//const u8*JPEG_SIZE_TBL[9] = {"QCIF", "QQVGA", "CIF", "QVGA", "VGA", "SVGA", "XGA", "SXGA", "UXGA"};	//JPEGͼƬ 9�ֳߴ�





int main(void)
{
    FATFS fs;													/* FatFs�ļ�ϵͳ���� */
    FIL fnew;													/* �ļ����� */
    FRESULT res_sd;                /* �ļ�������� */
    UINT fnum;            					  /* �ļ��ɹ���д���� */
    DIR dir;
    
    

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
    delay_init(168);  //��ʼ����ʱ����
    
    RTC_TimeInit();
    
	uart_init(115200);		///��ʼ�����ڲ�����Ϊ921600   ʹͨ���ٶȸ��죬PC������ܸ����ˢ��ͼƬ��̫���Ļ������쳣

    LED_Init();					//��ʼ��LED
    KEY_Init();					//������ʼ��
    res_sd = f_mount(&fs,"0:",1);//�����ļ�ϵͳ
    
    
    
		
    TIM3_Int_Init(10000 - 1, 84 - 1); //���ö�ʱ���Ķ�ʱƵ��Ϊ10ms  1�����ж�100��
    
    if(res_sd == FR_NO_FILESYSTEM)  //��ʼ���ļ�ϵͳ
    {
        printf("��SD����û���ļ�ϵͳ���������и�ʽ��...\r\n");
        /* ��ʽ�� */
        res_sd=f_mkfs("0:",0,0);							
        
        if(res_sd == FR_OK)
        {
            printf("��SD���ѳɹ���ʽ���ļ�ϵͳ��\r\n");
            /* ��ʽ������ȡ������ */
            res_sd = f_mount(NULL,"0:",1);			
            /* ���¹���	*/			
            res_sd = f_mount(&fs,"0:",1);
        }
        else
        {
            printf("������ʽ��ʧ�ܡ�����\r\n");
            while(1);
        }
    }
    else if(res_sd!=FR_OK)
    {
        printf("����SD�������ļ�ϵͳʧ�ܡ�(%d)\r\n",res_sd);
        printf("��������ԭ��SD����ʼ�����ɹ���\r\n");
        while(1);
    }
    else
    {
        printf("���ļ�ϵͳ���سɹ������Խ��ж�д����\r\n");
    }
    while(1)
    {
            res_sd=f_opendir(&dir,"0:CAM"); //��һ��Ŀ¼
        if(res_sd==FR_NO_PATH)
        {
            res_sd=f_mkdir("0:CAM");
            if(res_sd==FR_OK)
                printf("%s\r\n","FR_EXIST");
            
            delay_ms(500);
        }
    }
    
//    f_deldir("0:");
//    while(OV2640_Init())//��ʼ��OV2640
//    {
//        printf("OV2640��ʼ��ʧ��");
//    }
//	printf("OV2640��ʼ���ɹ�");
	
//	JPEG_Save(res_sd,fnew,fnum);
}
