#include "system.h"	/*头文件集合*/

/********************************************************************************
 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ALIENTEK MiniFly
 * main.c
 * 包括系统初始化和创建任务
 * 正点原子@ALIENTEK
 * 技术论坛:www.openedv.com
 * 创建日期:2017/5/12
 * 版本：V1.3
 * 版权所有，盗版必究。
 * Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 * All rights reserved
********************************************************************************/

TaskHandle_t startTaskHandle;
static void startTask(void *arg);

int main()
{
    systemInit();			/*底层硬件初始化*/

    xTaskCreate(startTask, "START_TASK", 300, NULL, 2, &startTaskHandle);	/*创建起始任务*/

    vTaskStartScheduler();	/*开启任务调度*/

    while(1) {};
}
/*创建任务*/
void startTask(void *arg)
{
    taskENTER_CRITICAL();	/*进入临界区*/

    xTaskCreate(radiolinkTask, "RADIOLINK", 150, NULL, 5, NULL);		/*创建无线连接任务*/

    // xTaskCreate(usblinkRxTask, "USBLINK_RX", 150, NULL, 4, NULL);		/*创建usb接收任务*/
    // xTaskCreate(usblinkTxTask, "USBLINK_TX", 150, NULL, 3, NULL);		/*创建usb发送任务*/

    // xTaskCreate(atkpTxTask, "ATKP_TX", 150, NULL, 3, NULL);				/*创建atkp发送任务任务*/
    xTaskCreate(atkpRxAnlTask, "ATKP_RX_ANL", 300, NULL, 6, NULL);		/*创建atkp解析任务*/ 

    // xTaskCreate(configParamTask, "CONFIG_TASK", 150, NULL, 1, NULL);	/*创建参数配置任务*/

    // xTaskCreate(pmTask, "PWRMGNT", 150, NULL, 2, NULL);					/*创建电源管理任务*/

    xTaskCreate(sensorsTask, "SENSORS", 450, NULL, 4, NULL);			/*创建传感器处理任务*/

    xTaskCreate(stabilizerTask, "STABILIZER", 450, NULL, 5, NULL);		/*创建姿态任务*/

    //xTaskCreate(expModuleMgtTask, "EXP_MODULE", 150, NULL, 1, NULL);	/*创建扩展模块管理任务*/

    printf("Free heap: %d bytes\n", xPortGetFreeHeapSize());			/*打印剩余堆栈大小*/

    vTaskDelete(startTaskHandle);										/*删除开始任务*/

    taskEXIT_CRITICAL();	/*退出临界区*/
}

void vApplicationIdleHook( void )
{
    static u32 tickWatchdogReset = 0;

    portTickType tickCount = getSysTickCnt();

    if (tickCount - tickWatchdogReset > WATCHDOG_RESET_MS)
    {
        tickWatchdogReset = tickCount;
        watchdogReset();
    }

    __WFI();	/*进入低功耗模式*/
}

/*****************************radiolinkTask***************************************/
/*****************************遥控器数据解析任务***************************************/
// C ->遥控器消息队列(一帧数据)->atkp队列（解析数据）——>rcdata(全局静态变量)——>atkpSendPeriod发送数据
// 解析任务中，解析DOWN_REMOTER 这个id的数据，并将真实的遥控器数据发送到姿态解算中去
// 遥控器接收任务通过等待消息队列，等待串口2空闲中断里面发过来的字节，最后组成一包数据，发送到atkpRxAnlTask中的消息队列
//串口2中断数据--->遥控器任务收到--->打包数据发送到消息队列中
//解析任务——>遥控器任务收到——>把数据通过串口2回传回去

/*****************************atkpRxAnlTask***************************************/
/*****************************命令解析任务***************************************/
//等待消息队列（消息可以从radio link中发出，也可以是usb link中发出）
//如果是msg->id == remote_control 解析遥控器数据 
//解析的遥控数据被command.c调用，并存储在全局变量里面,变量名字是remoteCache
//atkpRxAnlTask 只解析遥控器的发送的消息队列——>解析到遥感数据——>打包返回遥感的消息队列
                                                        //|     
                                                        //--> 打包返回给usb发送队列

/*****************************sensorsTask***************************************/
//传感器数据处理任务 -->等待外部中断-->中断发出信号量,触发iic发出dma通信数据包-->获得数据发送到消息队列中，
//传感器数据的源头来自于dma传输，最终将数据放到内部的静态全局变量中（消息队列）                                       

/*****************************stabilizerTask中***************************************/
/******************************姿态控制任务***************************************/
/////////////////stabilizerTask 按照一定的频率去读取消息队列中获取数据，比如得到传感器的数据
//sensorAcquire
            // 该任务用于在imu中获取原始的数据
//imuupdata
            //该任务用于将传感器的数据转换成姿态数据
                //首先这个函数收到的是imu输出的具有物理化意义的数据，包括速度和加速度
                //将加速度与重力加速度的方向计算差值
                //使用这个差值来修正陀螺仪的偏移量
                //利用陀螺仪数据来计算四元数
                //使用四元数导出旋转矩阵和欧拉角
        //总的来说，陀螺仪数据是有偏移的，所以需要加速度计来抵消这个偏移
        //用陀螺仪数据来求出四元数，而陀螺仪是角速度，四元数是角度的表示
        //得到四元数就可以得到旋转矩阵，从而得到欧拉角
//positionestimate
            //用于在姿态数据中加入位置信息
            //通过数据融合，得到姿态信息中的速度、加速度、位置坐标信息等


//commanderGetSetpoint
            // 这个任务 根据遥控器的数据，输出期望的的姿态信息，其中有角速度、角度、速度、加速度
                //首先会调用ctrlDataUpdate 这个函数来更新控制数据
                //但是实际上，没有光流模块的时候，只有输出期望的角度和mode信息
                //关于yaw方向的控制，遥控器输出的是yaw方向的角速度，yaw就用这个速度去转就可以了

//stateControl
////根据实际的角度和期望的角度 输出期望的叫角速度
// 通过实际的角速度和期望的角速度来输出 控制量
        //pidupdate这个函数中   
        // 输入是pid的指针，和误差
        //从而更新pid的值
        // 在stabilizer这个文件中，对角度和角速度都有pid的全局变量
        //pid变量中有比例、积分、微分的K大小，也有相应的值大小、还有输出和上一次的误差











