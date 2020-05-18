/**
  ******************************************************************************
  * @file    uart_debug.c
  * @author  Corazon @ CDUESTC
  * @brief   串口可视化调试模块
  *          本文档提供了一组宏定义及函数，将参数调试，系统运行状态，日志输出整
	*					 合输出。
  *
  @verbatim
 ===============================================================================
############################ 模块简介与注意事项 ################################
# 
################################################################################

################################ 版本更新历史 ##################################
# V1.0 (2019.7.25) 
#	 ->初版Release
################################################################################
 ===============================================================================

/<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
<--中断回调函数模板-->
<--依据实际环境替换尖括号中变量-->
<--将修改后的中断回调函数写入stm32h7xx_it.c中-->
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
/* 初始化日志显示缓冲区 */
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
/* 移动日志显示缓冲区 */
	void dLog_RemoveLogList( void ) {
		uint8_t (*logTempPoint)[128] = logList[ dLOG_NBLINE - 1].line; 
		logList[ dLOG_NBLINE - 1 ].line = logList[0].line;
		for( uint8_t iter = 0; iter < dLOG_NBLINE - 2; iter++ ){
			logList[ iter ].line = logList[ iter + 1 ].line;
		}
		logList[ dLOG_NBLINE - 2 ].line = logTempPoint;
	}
/* 日志屏幕刷新 */
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
