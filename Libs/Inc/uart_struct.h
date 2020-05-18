#ifndef __UART_STRUCT_H
#define __UART_STRUCT_H

#include "stm32h7xx_hal.h"
#include "string.h"
#include "stdio.h"

/* ���ڲ����ṹ�嶨�� */
typedef struct Uart_Handle_Struct {
	UART_HandleTypeDef* huart;
	uint16_t uart_buffsize_tx;
	uint16_t uart_buffsize_rx;
	uint8_t* uart_buff_tx;
	uint8_t* uart_buff_rx;
}Uart_Handle_Struct_t;

/**
  * @���  	��ʼ�����ڲ����ṹ��
  *
  * @����		__UART_HANDLE__					UART���
	*					__BUFFSIZE_TX__					���ͻ�������С
	*					__BUFFSIZE_RX__					���ջ�������С
  */
#define Uart_Handle_Init(__UART_HANDLE__, __BUFFSIZE_TX__, __BUFFSIZE_RX__) \
				uint8_t  __##__UART_HANDLE__##buff_tx[__BUFFSIZE_TX__] = {0}; \
				uint8_t  __##__UART_HANDLE__##buff_rx[__BUFFSIZE_RX__] = {0}; \
				Uart_Handle_Struct_t	__UART_HANDLE__##_S = { \
					&__UART_HANDLE__, \
					__BUFFSIZE_TX__, \
					__BUFFSIZE_RX__, \
					__##__UART_HANDLE__##buff_tx, \
					__##__UART_HANDLE__##buff_rx, \
				};
				
/**
	* @���  	��ʼ�����ڲ����ṹ��(DMA����)
  *
  * @����		__UART_HANDLE__					UART���
	*					__BUFFSIZE_TX__					���ͻ�������С
	*					__BUFFSIZE_RX__					���ջ�������С
	*					__TX_ADDRESS__					���ͻ�������ʼ��ַ
  */
#define Uart_Handle_Init_DMA(__UART_HANDLE__, __BUFFSIZE_TX__, __BUFFSIZE_RX__, __TX_ADDRESS__) \
				uint8_t  __##__UART_HANDLE__##buff_tx[__BUFFSIZE_TX__] __attribute__((at(__TX_ADDRESS__))) = {0}; \
				uint8_t  __##__UART_HANDLE__##buff_rx[__BUFFSIZE_RX__] = {0}; \
				Uart_Handle_Struct_t	__UART_HANDLE__##_S = { \
					&__UART_HANDLE__, \
					__BUFFSIZE_TX__, \
					__BUFFSIZE_RX__, \
					__##__UART_HANDLE__##buff_tx, \
					__##__UART_HANDLE__##buff_rx, \
				};				
				
/**
  * @���  	ָ�����ڵ�printf����
  *
  * @����		__UART_HANDLE__				UART���	
	*					...										printf()�ɱ����
  */
#define uart_printf( __UART_HANDLE__, ...) __uart_printf(__ADD_S(__UART_HANDLE__), __VA_ARGS__)	

#define __uart_printf( __UART_STRUCT_S__, ...) \
				while(__UART_STRUCT_S__.huart->gState != HAL_UART_STATE_READY) osDelay(1); \
				snprintf((char *)__UART_STRUCT_S__.uart_buff_tx, __UART_STRUCT_S__.uart_buffsize_tx, ##__VA_ARGS__); \
				while(HAL_UART_Transmit(__UART_STRUCT_S__.huart, __UART_STRUCT_S__.uart_buff_tx, \
																strlen((char *)__UART_STRUCT_S__.uart_buff_tx), 0xFFFF) != HAL_OK){ \
					osDelay(1); \
				}
																		
/**
  * @���  	ָ�����ڵ�printf����(DMA��ʽ)
  *
  * @����		__UART_HANDLE__				UART���	
	*					...										printf()�ɱ����
  */
#define uart_printf_dma( __UART_HANDLE__, ...) __uart_printf_dma(__ADD_S(__UART_HANDLE__), __VA_ARGS__)	

#define __uart_printf_dma( __UART_STRUCT_S__, ...) \
				while(__UART_STRUCT_S__.huart->gState != HAL_UART_STATE_READY) osDelay(1); \
				snprintf((char *)__UART_STRUCT_S__.uart_buff_tx, __UART_STRUCT_S__.uart_buffsize_tx, ##__VA_ARGS__); \
				while(HAL_UART_Transmit_DMA(__UART_STRUCT_S__.huart, __UART_STRUCT_S__.uart_buff_tx, \
																		strlen((char *)__UART_STRUCT_S__.uart_buff_tx)) != HAL_OK){ \
					osDelay(1); \
				}

/**
	* @���  	��ȡ���ڲ����ṹ��
  *
  * @����		__UART_HANDLE__				UART���	
  */	
#define uart_struct( __UART_HANDLE__ ) __uart_struct( __ADD_S(__UART_HANDLE__))
#define __uart_struct( __UART_STRUCT_S__ ) ( __UART_STRUCT_S__ )

/**
	* @���  	��ȡ���ڽ��ջ�������С
  *
  * @����		__UART_HANDLE__				UART���	
  */	
#define uart_buffsize_rx( __UART_HANDLE__ ) __uart_buffsize_rx( __ADD_S(__UART_HANDLE__))
#define __uart_buffsize_rx( __UART_STRUCT_S__ ) (__UART_STRUCT_S__.uart_buffsize_rx)

/**
	* @���  	��ȡ���ڷ��ͻ�������С
  *
  * @����		__UART_HANDLE__				UART���	
  */	
#define uart_buffsize_tx( __UART_HANDLE__ ) __uart_buffsize_tx( __ADD_S(__UART_HANDLE__))
#define __uart_buffsize_tx( __UART_STRUCT_S__ ) (__UART_STRUCT_S__.uart_buffsize_tx)
				
/**
	* @���  	��ȡ���ڷ��ͻ�����ָ��
  *
  * @����		__UART_HANDLE__				UART���	
  */	
#define uart_buff_tx( __UART_HANDLE__ ) __uart_buff_tx( __ADD_S(__UART_HANDLE__))
#define __uart_buff_tx( __UART_STRUCT_S__ ) (__UART_STRUCT_S__.uart_buff_tx)
				
/**
	* @���  	��ȡ���ڽ��ջ�����ָ��
  *
  * @����		__UART_HANDLE__				UART���	
  */	
#define uart_buff_rx( __UART_HANDLE__ ) __uart_buff_rx( __ADD_S(__UART_HANDLE__))
#define __uart_buff_rx( __UART_STRUCT_S__ ) (__UART_STRUCT_S__.uart_buff_rx)

/* ���"_S"��׺ ��UART���ת��ΪUART�����ṹ�� */
#define __ADD_S(__UART_PORT__) (__UART_PORT__##_S)

/*	���飺UART��ʽ�����
 *
 *	������	__UART_PORT__		���ھ��	
 *					...							��ʽ���ַ������ѡ����
 */
#define UartPrintf(__UART_PORT__, ...) uart_printf_dma(__UART_PORT__, __VA_ARGS__)

#define UartPrintfAt(__UART_PORT__, __ROW__, __COL__, __STRING__, ...) \
				UartPrintf(__UART_PORT__, "\033[%d;%dH" __STRING__, (__ROW__), (__COL__), ##__VA_ARGS__ )
				
#define UartPrintfCleanAt(__UART_PORT__, __ROW__, __COL__, __STRING__, ...) \
				UartPrintf(__UART_PORT__, "\033[%d;%dH\033[K" __STRING__, (__ROW__), (__COL__), ##__VA_ARGS__ )

#endif
