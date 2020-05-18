/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */     
#include "uart_debug.h"
#include "parameter_manager.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
Uart_Handle_Init_DMA(huart1, 128, 8, 0x30040000);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern TIM_HandleTypeDef htim16;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId pmTaskHandle;
osThreadId udTaskHandle;
osThreadId rtTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
   
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void pmTaskF(void const * argument);
void udTaskF(void const * argument);
void rtTaskF(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{

}

__weak unsigned long getRunTimeCounterValue(void)
{
return 0;
}
/* USER CODE END 1 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
       
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of pmTask */
  osThreadDef(pmTask, pmTaskF, osPriorityBelowNormal, 0, 512);
  pmTaskHandle = osThreadCreate(osThread(pmTask), NULL);

  /* definition and creation of udTask */
  osThreadDef(udTask, udTaskF, osPriorityBelowNormal, 0, 128);
  udTaskHandle = osThreadCreate(osThread(udTask), NULL);

  /* definition and creation of rtTask */
  osThreadDef(rtTask, rtTaskF, osPriorityBelowNormal, 0, 512);
  rtTaskHandle = osThreadCreate(osThread(rtTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{

  /* USER CODE BEGIN StartDefaultTask */

  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_pmTaskF */
/**
* @brief Function implementing the pmTask thread.
* @param argument: Not used
* @retval None
*/
typedef struct test{
	int STR1;
	float STR2;
	uint8_t STR3[10];
}test_t;
test_t teststr = {12,33.33,"sadasd"};
uint8_t STR[5][8];
/* USER CODE END Header_pmTaskF */
void pmTaskF(void const * argument)
{
  /* USER CODE BEGIN pmTaskF */
	pmNewParaGroupCreate(TestGroup);
	pmNewParaCreate(int, test1, 0, TestGroup);
	pmNewParaGroupCreate(NULLGROUP);
	pmNewParaCreate(float, test2, 0, TestGroup);
	pmNewParaGroupCreate(PGroup);
	pmNewParaGroupCreate(STRGroup);
	pmNewParaCreate(int, qtest1, 0, PGroup);
	uint8_t test5 = 4;
	pmAddPara(uint8_t, test5, &test5, PGroup);
	pmAddPara(int, STR1, &teststr.STR1,  PGroup);
	pmAddPara(float, STR2, &teststr.STR2,  PGroup);
	pmAddPara(string, STR3, &teststr.STR3,  PGroup);
	pmAddPara(string, STR_1, STR[0],  STRGroup);
	pmAddPara(string, STR_2, STR[1],  STRGroup);
	pmAddPara(string, STR_3, STR[2],  STRGroup);
	pmAddPara(string, STR_4, STR[3],  STRGroup);
	pmAddPara(string, STR_5, STR[4],  STRGroup);
	osDelay(1000);
	/* Infinite loop */
  for(;;)
  {
		pmShowUserInterface();
    osDelay(1);
  }
  /* USER CODE END pmTaskF */
}

/* USER CODE BEGIN Header_udTaskF */
/**
* @brief Function implementing the udTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_udTaskF */
void udTaskF(void const * argument)
{
  /* USER CODE BEGIN udTaskF */
	dLog_DisplayBuff_Init();
	ERROR("TEST");
	WARNING("TEST");
	INFO("TEST");
  /* Infinite loop */
  for(;;)
  {
		for(uint16_t iter = 0; ; iter++){
			INFO("This is %d infomation(s)", iter);
			osDelay(1000);
		}
  }
  /* USER CODE END udTaskF */
}

/* USER CODE BEGIN Header_rtTaskF */
/**
* @brief Function implementing the rtTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_rtTaskF */
void rtTaskF(void const * argument)
{
  /* USER CODE BEGIN rtTaskF */
	osDelay(250);
	HAL_TIM_Base_Start_IT(&htim16);
  /* Infinite loop */
  for(;;)
  {
		rtShow();
    osDelay(5000);
  }
  /* USER CODE END rtTaskF */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
     
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
