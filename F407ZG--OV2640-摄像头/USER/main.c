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




//STM32F407开发板
//摄像头 实验 -库函数版本


//D1闪烁说明DCMI_IRQHandler中断被触发，捕获到了图像




extern u8 ov_frame;  						//帧率
extern uint8_t Key_Flag;//按键键值
u8 Com1SendFlag;//串口1发送数据标记



TCHAR sys_status=PHOTO_MODE;


//0,数据没有采集完;
//1,数据采集完了,但是还没处理;
//2,数据已经处理完成了,可以开始下一帧接收
//JPEG尺寸支持列表
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


//const u8*JPEG_SIZE_TBL[9] = {"QCIF", "QQVGA", "CIF", "QVGA", "VGA", "SVGA", "XGA", "SXGA", "UXGA"};	//JPEG图片 9种尺寸





int main(void)
{

    FATFS *fs;													/* FatFs文件系统对象 */
    FIL fnew;													/* 文件对象 */
    FRESULT res_sd;                /* 文件操作结果 */
    UINT fnum;            					  /* 文件成功读写数量 */
    
    

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
    delay_init(168);  //初始化延时函数
    
    RTC_TimeInit();
    
	uart_init(115200);		///初始化串口波特率为921600   使通信速度更快，PC软件才能更快的刷新图片，太慢的话，会异常
    
    LED_Init();					//初始化LED
    KEY_Init();					//按键初始化
    res_sd = f_mount(fs,"0:",1);//挂载文件系统

    TIM3_Int_Init(10000 - 1, 84 - 1); //设置定时器的定时频率为10ms  1秒钟中断100次
    
    

    
    if(res_sd == FR_NO_FILESYSTEM)  //初始化文件系统
    {
        printf("》即将进行格式化...\r\n");
        /* 格式化 */
        res_sd=f_mkfs("0:",0,0);							
        
        if(res_sd == FR_OK)
        {
            printf("》SD卡已成功格式化文件系统。\r\n");
            /* 格式化后，先取消挂载 */
            res_sd = f_mount(NULL,"0:",1);			
            /* 重新挂载	*/			
            res_sd = f_mount(fs,"0:",1);
        }
        else
        {
            printf("《《格式化失败。》》\r\n");
            while(1);
        }
    }
    else if(res_sd!=FR_OK)
    {
        printf("！！SD卡挂载文件系统失败。(%d)\r\n",res_sd);
        printf("！！可能原因：SD卡初始化不成功。\r\n");
        while(1);
    }
    else
    {
        printf("》文件系统挂载成功，可以进行读写测试\r\n");
    }
        while(OV2640_Init())//初始化OV2640
    {
        printf("OV2640初始化失败");
    }
	printf("OV2640初始化成功\n");
    
    OV2640_LED_light=0;//打开补光LED
//    FindOldestFile(fs,"0:");
    
    
    JPEG_Save();
//    while(1)
//    {
//        switch(sys_status)
//        {
//            case DELETE_MODE:
//                f_del_oldestfile("0:");
//                break;
//            case PHOTO_MODE:
//                
//                break;
//            default:
//                printf("信号量错误");
//                while(1);
//        }
//        
//    }
    
}
