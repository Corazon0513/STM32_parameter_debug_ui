/**
  ******************************************************************************
  * @file    uart_debug.c
  * @author  Corazon @ CDUESTC
  * @brief   ���ڿ��ӻ�����ģ��
  *          ���ĵ��ṩ��һ��궨�弰���������������ԣ�ϵͳ����״̬����־�����
	*					 �������
  *
  @verbatim
 ===============================================================================
############################ ģ������ע������ ################################
# 
################################################################################

################################ �汾������ʷ ##################################
# V1.0 (2019.7.25) 
#	 ->����Release
################################################################################
 ===============================================================================

/<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
<--�жϻص�����ģ��-->
<--����ʵ�ʻ����滻�������б���-->
<--���޸ĺ���жϻص�����д��stm32h7xx_it.c��-->
void <TIMx>_IRQHandler(void)
{
	ulHighFrequencyTimerTicks++;
	<htimx>.Instance->SR &= 0xFFFE;

}
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>/

*/
/* Includes ------------------------------------------------------------------*/
#include "uart_debug.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
#if ( dLOG_SW == 1 ) 
	Uart_LogList_Struct_t		logList[dLOG_NBLINE];
	Uart_LogList_Struct_t* 	logListHeadPoint;
	uint8_t									logLine[dLOG_NBLINE][128] __attribute__((at(dLOG_ADDRESS)));
	uint8_t 								pcWriteBuffer[512] __attribute__((at(dRT_ADDRESS)));
#endif

#if ( dTASK_RUNTIME_SW == 1 )
uint64_t ulHighFrequencyTimerTicks;
#endif

/* Private function prototypes -----------------------------------------------*/
#if ( dLOG_SW == 1 ) 
/* ��ʼ����־��ʾ������ */
	void dLog_DisplayBuff_Init( void ) {
		logListHeadPoint = &logList[0];
		Uart_LogList_Struct_t* 	logListTempPoint = logListHeadPoint;
		for( uint8_t iter = 0; iter < dLOG_NBLINE - 1; iter++ ) {
			logListTempPoint->line = &logLine[iter];
			logListTempPoint->nextLine = &logList[iter + 1];
			logListTempPoint = logListTempPoint->nextLine;
		}
		logListTempPoint->line = &logLine[dLOG_NBLINE - 1];
		logListTempPoint->nextLine = NULL;
	}
/* �ƶ���־��ʾ������ */
	void dLog_RemoveLogList( void ) {
		uint8_t (*logTempPoint)[128] = logList[ dLOG_NBLINE - 1].line; 
		logList[ dLOG_NBLINE - 1 ].line = logList[0].line;
		for( uint8_t iter = 0; iter < dLOG_NBLINE - 2; iter++ ){
			logList[ iter ].line = logList[ iter + 1 ].line;
		}
		logList[ dLOG_NBLINE - 2 ].line = logTempPoint;
	}
/* ��־��Ļˢ�� */
	void dLog_Refresh( Uart_Handle_Struct_t* uart_port ) {
		for( uint8_t iter = 0; iter < dLOG_NBLINE; iter++ ){
			while(HAL_UART_Transmit_DMA(uart_port->huart, (uint8_t*)"\033[s\033[?25l\0",
												strlen((const char*)"\033[s\033[?25l\0")) == HAL_BUSY){ \
					osDelay(1); \
													
			}
			dCURSOR_JUMPLINE_CLEANLINE((*uart_port), (64 - dLOG_NBLINE + iter));
			while(HAL_UART_Transmit_DMA(uart_port->huart, *logList[iter].line,
												strlen((const char*)logList[iter].line)) == HAL_BUSY){ \
					osDelay(1); \
			}
			while(HAL_UART_Transmit_DMA(uart_port->huart, (uint8_t*)"\033[u\033[?25h\0",
												strlen((const char*)"\033[u\033[?25h\0")) == HAL_BUSY){ \
					osDelay(1); \
			}
		}
	}
#endif

#if ( dTASK_RUNTIME_SW == 1 )	
	void configureTimerForRunTimeStats(void){
		ulHighFrequencyTimerTicks = 0ul;
	}

	unsigned long getRunTimeCounterValue(void){
		return ulHighFrequencyTimerTicks;
	}

#endif
/* Private user code ---------------------------------------------------------*/
