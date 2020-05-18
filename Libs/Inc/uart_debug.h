#ifndef __UART_DEBUG_H
#define __UART_DEBUG_H

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"
#include "uart_struct.h"

/* <-----------------------�û���������-----------------------> */
/* ����UART�˿ھ�� */
#define dUART_PORT 						huart1

/* ��־�������� */
/* ��־���� */
#define dLOG_SW								1
/* ��־�������� */
#define dLOG_NBLINE						8
/* ��־������DMA�ɷ��ʵ�ַ ��0x400�ռ� */
#define dLOG_ADDRESS					0x30040080

/* ��������ͳ�ƹ������� */
/* ��������ͳ�ƹ��ܿ��� */
#define dTASK_RUNTIME_SW			1
/* ��������ͳ�ƻ�����DMA�ɷ��ʵ�ַ ��0x200�ռ� */
#define dRT_ADDRESS					0x30040480
/* <-----------------------------------------------------------> */


/**
  * @���  	�����ת��ָ��������
  *
  * @����		__UART_HANDLE_S__		UART�����ṹ��
	*					__ROW__							��	
	*					__COL__							��
  */
#define dCURSOR_JUMP(__UART_HANDLE_S__, __ROW__, __COL__) __uart_printf_dma(__UART_HANDLE_S__, "\033[%d;%dH", (__ROW__), (__COL__))
/**
  * @���  	�����ת��ָ�����в���ո���
  *
  * @����		__UART_HANDLE_S__		UART�����ṹ��
	*					__ROW__							��	
  */
#define dCURSOR_JUMPLINE_CLEANLINE(__UART_HANDLE_S__, __ROW__) __uart_printf_dma(__UART_HANDLE_S__, "\033[%d;0H\033[K", (__ROW__))

/* ��־����ʵ�� */
#if( dLOG_SW == 1 )
	void dLog_DisplayBuff_Init( void );
	void dLog_RemoveLogList( void );
	void dLog_Refresh( Uart_Handle_Struct_t* uart_port );
	/* ��־������ */
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

/* ��־��� */
#define ERROR_Handle(__STR_E__, ...) 			LOGOUT(dUART_PORT, "[ERROR]\t"	__STR_E__ "\r\n", ##__VA_ARGS__)
#define WARNING_Handle(__STR_W__, ...)		LOGOUT(dUART_PORT, "[WARNING]\t" __STR_W__ "\r\n", ##__VA_ARGS__)
#define INFO_Handle(__STR_I__, ...)				LOGOUT(dUART_PORT, "[INFO]\t" __STR_I__ "\r\n", ##__VA_ARGS__)
	
/* ��־����ӿ� */
#define ERROR(...)												ERROR_Handle(__VA_ARGS__)
#define WARNING(...)											WARNING_Handle(__VA_ARGS__)
#define INFO(...)													INFO_Handle(__VA_ARGS__)

/* ��������ͳ�ƹ��� */
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


/* ͷ�ļ���β */
#endif
