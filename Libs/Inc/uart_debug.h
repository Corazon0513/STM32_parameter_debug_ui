#ifndef __UART_DEBUG_H
#define __UART_DEBUG_H

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"
#include "uart_struct.h"

/* <-----------------------用户配置区域-----------------------> */
/* 调试UART端口句柄 */
#define dUART_PORT 						huart1

/* 日志功能配置 */
/* 日志开关 */
#define dLOG_SW								1
/* 日志保留行数 */
#define dLOG_NBLINE						8
/* 日志缓冲区DMA可访问地址 需0x400空间 */
#define dLOG_ADDRESS					0x30040080

/* 任务运行统计功能配置 */
/* 任务运行统计功能开关 */
#define dTASK_RUNTIME_SW			1
/* 任务运行统计缓冲区DMA可访问地址 需0x200空间 */
#define dRT_ADDRESS					0x30040480
/* <-----------------------------------------------------------> */


/**
  * @简介  	光标跳转到指定的行列
  *
  * @参数		__UART_HANDLE_S__		UART操作结构体
	*					__ROW__							行	
	*					__COL__							列
  */
#define dCURSOR_JUMP(__UART_HANDLE_S__, __ROW__, __COL__) __uart_printf_dma(__UART_HANDLE_S__, "\033[%d;%dH", (__ROW__), (__COL__))
/**
  * @简介  	光标跳转到指定的行并清空该行
  *
  * @参数		__UART_HANDLE_S__		UART操作结构体
	*					__ROW__							行	
  */
#define dCURSOR_JUMPLINE_CLEANLINE(__UART_HANDLE_S__, __ROW__) __uart_printf_dma(__UART_HANDLE_S__, "\033[%d;0H\033[K", (__ROW__))

/* 日志功能实现 */
#if( dLOG_SW == 1 )
	void dLog_DisplayBuff_Init( void );
	void dLog_RemoveLogList( void );
	void dLog_Refresh( Uart_Handle_Struct_t* uart_port );
	/* 日志行链表 */
	typedef struct Uart_LogList_Struct{
		uint8_t (*line)[128];
		struct Uart_LogList_Struct* nextLine;
	}Uart_LogList_Struct_t;
	extern Uart_LogList_Struct_t		logList[dLOG_NBLINE];
	extern Uart_LogList_Struct_t* 	logListPoint;
	#define dTIME 													(float)(HAL_GetTick())/1000
	#define LOGOUT(__UART_PORT__, __LOG__, ...) \
					dLog_RemoveLogList(); \
					snprintf(	(char *)logList[dLOG_NBLINE - 1].line, 128,	\
										"[%f] " __LOG__, dTIME, ##__VA_ARGS__); \
					dLog_Refresh(&__ADD_S(__UART_PORT__))
#else
	#define LOGOUT(...) 
#endif

/* 日志句柄 */
#define ERROR_Handle(__STR_E__, ...) 			LOGOUT(dUART_PORT, "[ERROR]\t"	__STR_E__ "\r\n", ##__VA_ARGS__)
#define WARNING_Handle(__STR_W__, ...)		LOGOUT(dUART_PORT, "[WARNING]\t" __STR_W__ "\r\n", ##__VA_ARGS__)
#define INFO_Handle(__STR_I__, ...)				LOGOUT(dUART_PORT, "[INFO]\t" __STR_I__ "\r\n", ##__VA_ARGS__)
	
/* 日志输出接口 */
#define ERROR(...)												ERROR_Handle(__VA_ARGS__)
#define WARNING(...)											WARNING_Handle(__VA_ARGS__)
#define INFO(...)													INFO_Handle(__VA_ARGS__)

/* 任务运行统计功能 */
#if ( dTASK_RUNTIME_SW == 1)
	#if ( (configGENERATE_RUN_TIME_STATS == 1) && \
				(configUSE_TRACE_FACILITY == 1) && \
				(configUSE_STATS_FORMATTING_FUNCTIONS == 1))
		void configureTimerForRunTimeStats(void);
		unsigned long getRunTimeCounterValue(void);
		extern uint64_t ulHighFrequencyTimerTicks;
		extern uint8_t 	pcWriteBuffer[512];
			
		#define rtShow( void ) \
			while(HAL_UART_Transmit_DMA(&dUART_PORT, (uint8_t*)"\033[s\033[?25l\0", \
												strlen((const char*)"\033[s\033[?25l\0")) == HAL_BUSY){ \
					osDelay(1); \
			} \
			UartPrintfCleanAt(dUART_PORT, 26, 0, "task_name  \tstate\t prior\trtack\t Id\r\n"); \
			vTaskList((char *)&pcWriteBuffer); \
			while(HAL_UART_Transmit_DMA(&dUART_PORT, pcWriteBuffer, \
												strlen((const char*)pcWriteBuffer)) == HAL_BUSY){ \
					osDelay(1); \
			} \
			UartPrintfCleanAt(dUART_PORT, 40, 0, "task_name    \ttime_count \tusage_pec\r\n"); \
			vTaskGetRunTimeStats((char *)&pcWriteBuffer); \
			while(HAL_UART_Transmit_DMA(&dUART_PORT, pcWriteBuffer, \
												strlen((const char*)pcWriteBuffer)) == HAL_BUSY){ \
					osDelay(1); \
			} \
			while(HAL_UART_Transmit_DMA(&dUART_PORT, (uint8_t*)"\033[u\033[?25h\0", \
												strlen((const char*)"\033[u\033[?25h\0")) == HAL_BUSY){ \
					osDelay(1); \
			} 
	#else
		#error "dTASK_RUNTIME_SW need" enable 3 function marco in STM32Cube "Run time and task stats gathering related definitions".
	
	#endif
#endif


/* 头文件结尾 */
#endif
