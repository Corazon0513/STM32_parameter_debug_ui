/**
  ******************************************************************************
  * @file    parameter_manager.c
  * @author  Corazon @ CDUESTC
  * @brief   �ɱ��������ʽ����ģ��
  *          ���ĵ��ṩ��һ��궨�弰���������ڽ�Ƕ��ʽ�����еĲ�������
  *					 ���ӻ�������������ʾ���洢��
  *
  @verbatim
 ===============================================================================
############################ ģ������ע������ ################################
# 1. ʹ��ǰ����������Ҫʹ�ñ��⹦�ܵ�Դ����includeͷ�ļ�parameter_manager.h
# 2. �뽫parameter_manager.h�в��������빦��ʹ������������
# 3. ȷ��UARTȫ���жϿ���������DMA�����������·��жϻص�����ģ����ʾ���в���
# 4. ʹ�ú�pmNewParaGroupCreate()���б�����Ĵ���
# 5. ��Ҫ����ľֲ�������ʹ�ú�pmNewParaCreate()���б����Ĵ���
# 6. ��Ҫ�����ȫ�ֱ�������������ʽ������ʹ�ú�pmAddPara()����ע��
# 7. �����㷨���ջ���ƣ��ַ������ͱ����������7���ַ���
# 8. ��ģ��ʹ�þ�̬�ڴ���䷽�����������ÿ��������Ҫһ���Ŀռ����ڱ���
#    ������Ϣ����ע���ڴ�ʹ������
# 9. �����HardFault�жϣ��ܿ���������ʹ�ñ�ģ�����߳���ע���˹���ı�����
#		 ���ʵ������������ջ�Ա�����������
# 10.������FLASH�д洢�����ݽ��б������ݣ������б������ȱ���������Ϣ������
#    ������µı���ǰ����ע�⽫��������ݱ��ݣ��������ݴ�λ��ȡ������ݶ�ʧ��
################################################################################

################################ �汾������ʷ ##################################
# V1.3 (2019.7.27)
#	 ->������ˢ���޸�Ϊ����ˢ��
#  ->���ڻ����������޸�Ϊʹ��uart_struct��
# V1.2 (2019.7.25) 
#  ->[BUG][FIXED]�޸��˿�������ʱ���µ�UART����BUG
#  ->[BUG][FIXED]�޸����ޱ��������������ޱ���ʱ�����Ƿ����ʵ���HardFault����
# V1.1 (2019.7.24) 
#  ->�޸�ΪFlash�洢ģʽ
# V1.0 (2019.7.23) 
#	 ->����Release
################################################################################
 ===============================================================================

/<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
<--�жϻص�����ģ��-->
<--����ʵ�ʻ����滻�������б���-->
<--���޸ĺ���жϻص�����д��stm32h7xx_it.c��-->

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if( huart->Instance == <USARTx> ){
		BaseType_t xHigherPriorityTaskWoken;
		vTaskNotifyGiveFromISR(<TaskHandle>, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>/

*/
/* Includes ------------------------------------------------------------------*/
#include "parameter_manager.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define PM_UART_BUFFSIZE_RX uart_buffsize_rx(pmUART_PORT)
#define PM_UART_BUFFSIZE_TX uart_buffsize_tx(pmUART_PORT)
#define uart_receive uart_buff_rx(pmUART_PORT)

/* Private macros ------------------------------------------------------------*/
/*	���飺UART��ʽ�����
 *
 *	������	__UART_PORT__		���ھ��	
 *					...							��ʽ���ַ������ѡ����
 */
#define pmUartPrintf(__UART_PORT__, ...) UartPrintf(__UART_PORT__, __VA_ARGS__)
				

/*	���飺UART��ʽ������
 *
 *	������	...							��ʽ���ַ������ѡ����
 */				
#define pmUartScanf(...) \
				if( pmParaInput() == 1 )	{ \
					goto PM_PARA_SEL_LAB; \
				} \
				else { \
					sscanf((const char *)uart_receive, ##__VA_ARGS__); \
				}
				
/*	���飺UART�޻��Ե��ַ�����
 *
 *	������	__BUFF__				�ַ����滺����(uint8_t)
 */				
#define pmUartGetch(__BUFF__) \
				do{ \
					HAL_UART_Receive_IT(&pmUART_PORT, &__BUFF__, 1U); \
				}while( ulTaskNotifyTake(pdTRUE, 100) != 1);
				
/*	���飺UARTָ���������
 *
 *	������	__UART_PORT__		���ھ��	
 *					__ROW__					ָ����
 *          __COL__					ָ����
 *					__STRING__			��ʽ���ַ���
 *					...							��ѡ����
 */				
#define pmUartPrintfAt(__UART_PORT__, __ROW__, __COL__, __STRING__, ...) \
				UartPrintf(__UART_PORT__, "\033[%d;%dH" __STRING__, (__ROW__), (__COL__), ##__VA_ARGS__ )
				
/*	���飺UARTָ�������������
 *
 *	������	__UART_PORT__		���ھ��	
 *					__ROW__					ָ����
 *          __COL__					ָ����
 *					__STRING__			��ʽ���ַ���
 *					...							��ѡ����
 */				
#define pmUartPrintfCleanAt(__UART_PORT__, __ROW__, __COL__, __STRING__, ...) \
				UartPrintf(__UART_PORT__, "\033[%d;%dH\033[K" __STRING__, (__ROW__), (__COL__), ##__VA_ARGS__ )

/* Private variables ---------------------------------------------------------*/
extern Uart_Handle_Struct_t uart_struct(pmUART_PORT);
/* ������������ */
pmParaGroupList_t	pmParaGroupList = {0, NULL, NULL};
/* ���������� */
short							pmParaGroupSum = 0;

/* 256bit������ */
uint8_t  	dataBuff[32], dataCountBit = 0;
/* Private function prototypes -----------------------------------------------*/
/**
  * @���   ������������ȡ
  *
  * @����		��
  * @����ֵ ����ɹ���־
  */
uint8_t pmParaInput(void){
	for(int8_t residueLength = PM_UART_BUFFSIZE_RX - 1; residueLength >= 0; residueLength--) {
		if( residueLength > 0 ) {
			HAL_UART_Receive_IT(&pmUART_PORT,&uart_receive[PM_UART_BUFFSIZE_RX - 1 - residueLength], 1U);
			ulTaskNotifyTake(pdTRUE, osWaitForever);
			if( uart_receive[PM_UART_BUFFSIZE_RX - 1 - residueLength] == '\b') {
				if(residueLength < PM_UART_BUFFSIZE_RX - 1) {
					pmUartPrintf(pmUART_PORT, "%c", 127);
					residueLength += 2;
				}
				else {
					residueLength++;
				}
			}
			else if( uart_receive[PM_UART_BUFFSIZE_RX - 1 - residueLength] == 27 ){	
				return 1;
			}
			else if( uart_receive[PM_UART_BUFFSIZE_RX - 1 - residueLength] != '\r' ) {
				pmUartPrintf(pmUART_PORT, "%c", uart_receive[PM_UART_BUFFSIZE_RX - 1 - residueLength]);
			}
			else {
				uart_receive[PM_UART_BUFFSIZE_RX - residueLength] = 0;
				break;
			}
		}
		else {
			uint8_t temp;
			HAL_UART_Receive_IT(&pmUART_PORT,&temp, 1U);
			ulTaskNotifyTake(pdTRUE, osWaitForever);
			if( temp == '\b') {
				pmUartPrintf(pmUART_PORT, "%c", 127);
				residueLength += 2;
			}
			else if (temp == '\r') {
				break;
			}	
			else if( temp == 27 ){	
				return 1;
			}
			else {
				residueLength++;
			}
		}
	}
	return 0;
}
#if (PM_FLASH_SW == 1)
/**
  * @���  	�����洢��FLASH
  *
  * @����		��
  * @����ֵ ��
  */
void pmFlashSave(void){
	uint32_t	flashAddr = PM_FLASH_ADDR;
	memset(dataBuff, 0, 32);
	dataCountBit = 0;
	static FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SectorError;
	/* �������� */
	HAL_FLASH_Unlock();
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.VoltageRange = VOLTAGE_RANGE_3;
	EraseInitStruct.Sector = PM_FLASH_SECTOR;
	EraseInitStruct.NbSectors = 1;
	EraseInitStruct.Banks = PM_FLASH_BANKS;
	if( HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK ){
		ERROR("Flash erase failed");
	}
	HAL_FLASH_Lock();	
	/* �������������� */
	for(	pmParaGroupList_t* pmParaGroupListIter = pmParaGroupList.pmNextNode; ;
				pmParaGroupListIter = pmParaGroupListIter->pmNextNode) {
		/* �жϿձ����� */
		if( pmParaGroupListIter->pmParaGroup->pmParaList->pmNextNode != NULL ) {
			/* �����������ڱ���ע��� */
			for( 	pmParaList_t* pmParaListIter = pmParaGroupListIter->pmParaGroup->pmParaList->pmNextNode; ;
						pmParaListIter = pmParaListIter->pmNextNode) {
				/* �洢���� */
				switch(pmParaListIter->pmParaRegister->pmParaType) {
					/* 1Byte */
					case pm_uint8: 
					case pm_int8: { 
						if( dataCountBit > 31 ) {
							HAL_FLASH_Unlock();
							HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, flashAddr, (uint32_t)dataBuff);
							HAL_FLASH_Lock();
							flashAddr += 32;
							dataCountBit = 0;
							memset(dataBuff, 0, 32);
						}
						dataBuff[dataCountBit] = *(unsigned char*)pmParaListIter->pmParaRegister->pmParaPoint;
						dataCountBit++;
						break;
					}
					/* 2Byte */
					case pm_uint16: 
					case pm_int16: { 
						if( dataCountBit > 30 ) {
							HAL_FLASH_Unlock();
							HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, flashAddr, (uint32_t)dataBuff);
							HAL_FLASH_Lock();
							flashAddr += 32;
							dataCountBit = 0;
							memset(dataBuff, 0, 32);
						}
						for(uint8_t iter = 0; iter < 2; iter++){
							dataBuff[dataCountBit] = *((unsigned char*)pmParaListIter->pmParaRegister->pmParaPoint + iter );
							dataCountBit++;
						}
						break;
					}
					/* 4Byte */
					case pm_uint32: 
					case pm_int32:
					case pm_float: {
						if( dataCountBit > 28 ) {
							HAL_FLASH_Unlock();
							HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, flashAddr, (uint32_t)dataBuff);
							HAL_FLASH_Lock();
							flashAddr += 32;
							dataCountBit = 0;
							memset(dataBuff, 0, 32);
						}
						for(uint8_t iter = 0; iter < 4; iter++){
							dataBuff[dataCountBit] = *((unsigned char*)pmParaListIter->pmParaRegister->pmParaPoint + iter );
							dataCountBit++;
						}
						break;
					}
					/* 8Byte */
					case pm_double:
					case pm_string: { 
						if( dataCountBit > 24 ) {
							HAL_FLASH_Unlock();
							HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, flashAddr, (uint32_t)dataBuff);
							HAL_FLASH_Lock();
							flashAddr += 32;
							dataCountBit = 0;
							memset(dataBuff, 0, 32);
						}
						for(uint8_t iter = 0; iter < 8; iter++){
							dataBuff[dataCountBit] = *((unsigned char*)pmParaListIter->pmParaRegister->pmParaPoint + iter );
							dataCountBit++;
						}
						break;
					}
				}
				if( pmParaListIter->pmNextNode == NULL ) break;
			}
		}
		if( pmParaGroupListIter->pmNextNode == NULL) break;
	}
	HAL_FLASH_Unlock();
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, flashAddr, (uint32_t)dataBuff);
	HAL_FLASH_Lock();
}

/**
  * @���  	������FLASH��ȡ
  *
  * @����		��
  * @����ֵ ��
  */
void pmFlashLoad(void){
	uint32_t flashAddr = PM_FLASH_ADDR;
	memset(dataBuff, 0, 32);
	for(dataCountBit = 0; dataCountBit < 32; dataCountBit++){
		dataBuff[dataCountBit] = *((unsigned char*)flashAddr + dataCountBit);
	}
	flashAddr += 32;
	dataCountBit = 0;
	/* �������������� */
	for(	pmParaGroupList_t* pmParaGroupListIter = pmParaGroupList.pmNextNode; ;
				pmParaGroupListIter = pmParaGroupListIter->pmNextNode) {
		/* �жϿձ����� */
		if( pmParaGroupListIter->pmParaGroup->pmParaList->pmNextNode != NULL ) {
			/* �����������ڱ���ע��� */
			for( 	pmParaList_t* pmParaListIter = pmParaGroupListIter->pmParaGroup->pmParaList->pmNextNode; ;
						pmParaListIter = pmParaListIter->pmNextNode) {
				/* ��ȡ���� */
				switch(pmParaListIter->pmParaRegister->pmParaType) {
					/* 1Byte */
					case pm_uint8: 
					case pm_int8: { 
						if( dataCountBit > 31 ) {
							memset(dataBuff, 0, 32);
							for(dataCountBit = 0; dataCountBit < 32; dataCountBit++){
								dataBuff[dataCountBit] = *((unsigned char*)flashAddr + dataCountBit);
							}
							flashAddr += 32;
							dataCountBit = 0;
						}
						*(unsigned char*)pmParaListIter->pmParaRegister->pmParaPoint = dataBuff[dataCountBit];
						dataCountBit++;
						break;
					}
					/* 2Byte */
					case pm_uint16: 
					case pm_int16: { 
						if( dataCountBit > 30 ) {
							memset(dataBuff, 0, 32);
							for(dataCountBit = 0; dataCountBit < 32; dataCountBit++){
								dataBuff[dataCountBit] = *((unsigned char*)flashAddr + dataCountBit);
							}
							flashAddr += 32;
							dataCountBit = 0;
						}
						for(uint8_t iter = 0; iter < 2; iter++){
							*((unsigned char*)pmParaListIter->pmParaRegister->pmParaPoint + iter ) = dataBuff[dataCountBit];
							dataCountBit++;
						}
						break;
					}
					/* 4Byte */
					case pm_uint32: 
					case pm_int32:
					case pm_float: {
						if( dataCountBit > 28 ) {
							memset(dataBuff, 0, 32);
							for(dataCountBit = 0; dataCountBit < 32; dataCountBit++){
								dataBuff[dataCountBit] = *((unsigned char*)flashAddr + dataCountBit);
							}
							flashAddr += 32;
							dataCountBit = 0;
						}
						for(uint8_t iter = 0; iter < 4; iter++){
							*((unsigned char*)pmParaListIter->pmParaRegister->pmParaPoint + iter ) = dataBuff[dataCountBit];
							dataCountBit++;
						}
						break;
					}
					/* 8Byte */
					case pm_double:
					case pm_string: { 
						if( dataCountBit > 24 ) {
							memset(dataBuff, 0, 32);
							for(dataCountBit = 0; dataCountBit < 32; dataCountBit++){
								dataBuff[dataCountBit] = *((unsigned char*)flashAddr + dataCountBit);
							}
							flashAddr += 32;
							dataCountBit = 0;
						}
						for(uint8_t iter = 0; iter < 8; iter++){
							*((unsigned char*)pmParaListIter->pmParaRegister->pmParaPoint + iter ) = dataBuff[dataCountBit];
							dataCountBit++;
						}
						break;
					}
				}
				if( pmParaListIter->pmNextNode == NULL ) break;
			}
		}
		if( pmParaGroupListIter->pmNextNode == NULL) break;
	}
}
#endif

/* Exported functions --------------------------------------------------------*/

/**
  * @���  ��ʼ���µı�����
  *
  * @����	������ָ�룬 ��������������������ڵ��ַ�� ��������ͷ
  * @����ֵ ����
  */
void pmNewParaGroupCreateF(	pmParaGroup_t* group, const char* group_name,
							pmParaGroupList_t *group_node, pmParaList_t* list_head) {
	/* �������� */
	strcpy((char *)group->pmGroupName, group_name);
	/* �����ʼ���� */
	group->pmParaSumInGroup = 0;
	/* NULL������������ͷ */
	list_head->no = 0;
	list_head->pmParaRegister = NULL;
	list_head->pmNextNode = NULL;
	/* �󶨱�������������� */
	group->pmParaList = list_head;
	group->pmParaListLast = list_head;
	/* ����������ڵ��ʼ�� */
	group_node->pmParaGroup = group;
	group_node->pmNextNode = NULL;
	/* Ѱ���������������β */
	pmParaGroupList_t* pmParaGroupListNode_p = &pmParaGroupList;
	short  pmParaGroupNo = 1;
	while (pmParaGroupListNode_p->pmNextNode != NULL) {
		pmParaGroupListNode_p = pmParaGroupListNode_p->pmNextNode;
		pmParaGroupNo++; 
	}
	/* ���ϱ��б��� */
	group_node->no = pmParaGroupNo;
	pmParaGroupListNode_p->pmNextNode = group_node;
}

/**
  * @brief  ��ʼ���µı���ע���
  *
  * @param  ����ע����ַ�� ������ַ�� �������� �������ͣ�
  *					��������ڵ��ַ�� �������ַ
  * @retval ����
  */
void pmNewParaRegCreateF(	pmParaRegister_t* para_reg, void* para, const char* para_name,
						const char* para_type, pmParaList_t* para_node,
						pmParaGroup_t* group) {
	/* ����ָ��ָ����� */
	para_reg->pmParaPoint = para;
	/* ��������� */
	strcpy((char *)para_reg->pmParaName, para_name);
	/* �����뱣��������� */
	if
		(	strcmp(para_type, "unsigned char"	) == 0 ||
			strcmp(para_type, "uint8_t"				)	== 0	) {
		para_reg->pmParaType = pm_uint8;
	}
	else if
		(	strcmp(para_type, "char"					) == 0 ||
			strcmp(para_type, "int8_t"				)	== 0	){
		para_reg->pmParaType = pm_int8;
	}
	else if
		(	strcmp(para_type, "unsigned short") == 0 ||
			strcmp(para_type, "uint16_t"			)	== 0	){
		para_reg->pmParaType = pm_uint16;
	}
	else if
		(	strcmp(para_type, "short"					) == 0 ||
			strcmp(para_type, "int16_t"				)	== 0	){
		para_reg->pmParaType = pm_int16;
	}
	else if
		(	strcmp(para_type, "unsigned int"	) == 0 ||
			strcmp(para_type, "uint32_t"			)	== 0	){
		para_reg->pmParaType = pm_uint32;
	}
	else if
		(	strcmp(para_type, "int"						) == 0 ||
			strcmp(para_type, "int32_t"				)	== 0	){
		para_reg->pmParaType = pm_int32;
	}
	else if
		(	strcmp(para_type, "float"					) == 0 ||
			strcmp(para_type, "float_t"				)	== 0	){
		para_reg->pmParaType = pm_float;
	}
	else if
		(	strcmp(para_type, "double"				) == 0 ||
			strcmp(para_type, "double_t"			)	== 0	){
		para_reg->pmParaType = pm_double;
	}
	else if
		(	strcmp(para_type, "string"				) == 0 ){
		para_reg->pmParaType = pm_string;
	}
	else{
		pmUartPrintf(pmUART_PORT, "Unknown typedef.");
	}
	/* ��������ڵ��ʼ�� */
	para_node->no = group->pmParaSumInGroup + 1;
	para_node->pmParaRegister = para_reg;
	para_node->pmNextNode = NULL;
	/* �����������β���ر��� */
	group->pmParaListLast->pmNextNode = para_node;
	group->pmParaListLast = group->pmParaListLast->pmNextNode;
	group->pmParaSumInGroup++;
}

/**
  * @brief  ��ʾ���߼�����
  *
  * @param  ��
  *
  * @retval ��
  */
void pmShowCase(uint8_t mode) {
	pmUartPrintfCleanAt(pmUART_PORT, 1, 6, "+--------------------------------------------------------------------+");
	if( mode == 0 ){
		for (int row = 2; row < 19; row++) {
			pmUartPrintfCleanAt(pmUART_PORT, row, 6, "|");
			pmUartPrintfCleanAt(pmUART_PORT, row, 75, "|");
		}
		pmUartPrintfCleanAt(pmUART_PORT, 19, 6, "| Press <ESC> to back previous status.");
		pmUartPrintfCleanAt(pmUART_PORT, 19, 75, "|");
		pmUartPrintfCleanAt(pmUART_PORT, 20, 6, "| Press <S>   to saving parameter into Flash.");
		pmUartPrintfCleanAt(pmUART_PORT, 20, 75, "|");
		pmUartPrintfCleanAt(pmUART_PORT, 21, 6, "| Press <L>   to loading parameter from Flash.");
		pmUartPrintfCleanAt(pmUART_PORT, 21, 75, "|");
		pmUartPrintfCleanAt(pmUART_PORT, 22, 6, "+--------------------------------------------------------------------+");
	}
	else {
		for (int row = 2; row < 21; row++) {
			pmUartPrintfCleanAt(pmUART_PORT, row, 6, "|");
			pmUartPrintfCleanAt(pmUART_PORT, row, 75, "|");
		}
		pmUartPrintfCleanAt(pmUART_PORT, 21, 6, "| Press <ESC> to back previous status.");
		pmUartPrintfCleanAt(pmUART_PORT, 21, 75, "|");
		pmUartPrintfCleanAt(pmUART_PORT, 22, 6, "+--------------------------------------------------------------------+ ");
		}
	pmUartPrintfAt(pmUART_PORT, (3), (32), "PARAMETER MANAGER");
}

/**
  * @brief  ��ʾ���ƽ��� 68�п���
  *
  * @param  ��
  *			
  * @retval ��
  */
void pmShowUserInterface(void) {
	/* ����ѡ���� */
	uint8_t choose;
	/* ������ָ�� ��ʼ��ָ��������б��е�һ�������� */
	pmParaGroupList_t* group_p = pmParaGroupList.pmNextNode;
	for(;;){
		/* ѡ��������ǩ */
		PM_GROUP_SEL_LAB:
		/* ���������б����Ƿ���ڱ����� */
		if( group_p == NULL ){
			/* ��ʾ���(�޴洢��ʾ) */
			pmShowCase( 1 );
			pmUartPrintfAt(pmUART_PORT, (7), (10), " No any parameter group! Press ESC to turn back.");
			do{
				pmUartGetch(choose);
			}while( choose != 27 );
			return ;
		}
		/* ��λ������ָ�� */
		group_p = pmParaGroupList.pmNextNode;
		/* ��ʾ���(���洢��ʾ) */
		pmShowCase( 0 );
		/* ѡ��������� */
		pmUartPrintfAt(pmUART_PORT, (5), (8), "Group select:");
		/* ������ӡ�������б��г�Ա������ */
		int group_i = 1;
		while (group_p->pmParaGroup != NULL) {
			pmUartPrintfAt(pmUART_PORT, (6 + group_i), (10), "%d:%s", group_i, group_p->pmParaGroup->pmGroupName);
			if (group_p->pmNextNode == NULL) {
				break;
			}
			else {
				group_p = group_p->pmNextNode;
				group_i++;
			}
		}
		/* ��ʾѡ����ʾ */
		pmUartPrintfAt(pmUART_PORT, (18), (8), "Your choose:");
		/* ������ָ��ָ��������б��ͷ */

		/* �����б�ָ�� */
		pmParaList_t* list_p;
		/* ���봦�� */
		for(;;){
			pmUartGetch(choose);
			group_p = &pmParaGroupList;
			if( choose == 27 ) {
				/* �˳� */
				return ;
			}
			/* ������FLASH */
			else if ( choose == 'S' || choose == 's' ){
				pmFlashSave();
				pmUartPrintfAt(pmUART_PORT, (18), (8), "Successed save parameter value.");
				osDelay(250);
				goto PM_GROUP_SEL_LAB;
			}
			/* ��FLASH��ȡ */
			else if ( choose == 'L' || choose == 'l' ){
				pmFlashLoad();
				pmUartPrintfAt(pmUART_PORT, (18), (8), "Successed load parameter value.");
				osDelay(250);
				goto PM_GROUP_SEL_LAB;
			}
			/* ���뷶Χ�ж� */
			else if( choose > '0' && choose <= '9'){
				/* Ѱ��ѡ���� */
				for (int i = '1'; i < choose && group_p->pmNextNode != NULL; i++, group_p = group_p->pmNextNode);
				/* ��ȷ���� */
				if (group_p->pmNextNode != NULL) {
					break;
				}
			}
		}
		for(;;){
			PM_PARA_SEL_LAB:
			/* ָ����޸ı������� */
			list_p = group_p->pmNextNode->pmParaGroup->pmParaList->pmNextNode;
			/* ���� */
			pmShowCase( 1 );
			/* ���������б����Ƿ���ڱ����� */
			if( list_p == NULL ){
				pmUartPrintfAt(pmUART_PORT, (7), (10), " No any parameter in this group! Press ESC to turn back.");
				do{
					pmUartGetch(choose);
				}while( choose != 27 );
				goto PM_GROUP_SEL_LAB;
			}
			/* ѡ����� */
			pmUartPrintfAt(pmUART_PORT, (5), (8), "Parameter select:");
			/* ������ӡ�������г�Ա����������ֵ*/
			short list_i = 1;
			while (list_p->pmParaRegister != NULL) {
				pmUartPrintfAt(pmUART_PORT, (6 + list_i), (10), "%d:[%s", list_i, list_p->pmParaRegister->pmParaName);
				switch(list_p->pmParaRegister->pmParaType){
					case pm_uint8: { 
						pmUartPrintfAt(pmUART_PORT, (6 + list_i), (28), "%hhu]", *(unsigned char*)list_p->pmParaRegister->pmParaPoint); 
						break;
					}
					case pm_int8: { 
						pmUartPrintfAt(pmUART_PORT, (6 + list_i), (28), "%+hhd]", *(char*)list_p->pmParaRegister->pmParaPoint); 
						break;
					}
					case pm_uint16: { 
						pmUartPrintfAt(pmUART_PORT, (6 + list_i), (28), "%hu]", *(unsigned short*)list_p->pmParaRegister->pmParaPoint); 
						break;
					}
					case pm_int16: { 
						pmUartPrintfAt(pmUART_PORT, (6 + list_i), (28), "%+hd]", *(short*)list_p->pmParaRegister->pmParaPoint); 
						break;
					}
					case pm_uint32: { 
						pmUartPrintfAt(pmUART_PORT, (6 + list_i), (28), "%u]", *(unsigned int*)list_p->pmParaRegister->pmParaPoint); 
						break;
					}
					case pm_int32: { 
						pmUartPrintfAt(pmUART_PORT, (6 + list_i), (28), "%+d]", *(int*)list_p->pmParaRegister->pmParaPoint); 
						break;
					}
					case pm_float: { 
						pmUartPrintfAt(pmUART_PORT, (6 + list_i), (28), "%+f]", *(float*)list_p->pmParaRegister->pmParaPoint); 
						break;
					}
					case pm_double: { 
						pmUartPrintfAt(pmUART_PORT, (6 + list_i), (28), "%+lf]", *(double*)list_p->pmParaRegister->pmParaPoint); 
						break;
					}
					case pm_string: { 
						pmUartPrintfAt(pmUART_PORT, (6 + list_i), (28), "%s]", (unsigned char*)list_p->pmParaRegister->pmParaPoint); 
						break;
					}
				}
				if (list_p->pmNextNode == NULL) {
					break;
				}
				else {
					list_p = list_p->pmNextNode;
					list_i++;
				}
			}
			/* ��ʾѡ����ʾ */
			pmUartPrintfAt(pmUART_PORT, (20), (8), "Your choose:");
			/* ����ע���ָ�� */
			pmParaRegister_t* para_p;
			/* ���봦�� */
			for(;;){
				pmUartGetch(choose);
				/* �����б�ָ��ָ������б�ͷ */
				list_p = group_p->pmNextNode->pmParaGroup->pmParaList;
				if ( choose == 27 ) goto PM_GROUP_SEL_LAB;
				/* ���뷶Χ�ж� */
				if( choose > '0' && choose <= '9'){
					/* Ѱ��ѡ���� */
					for (int i = '1'; i < choose && list_p->pmNextNode != NULL; i++, list_p = list_p->pmNextNode);
					/* ѡ��ѡ���� */
					if (list_p->pmNextNode != NULL) {
						para_p = list_p->pmNextNode->pmParaRegister;
						break ;
					}
				}
			}
			/* ��ʾ��ֵ��ʾ */
			pmUartPrintfAt(pmUART_PORT, (20), (8), "%s%s:", "New value of ", para_p->pmParaName);
			switch(para_p->pmParaType){
				case pm_uint8: { 
					pmUartScanf("%hhu", (unsigned char*)para_p->pmParaPoint);
					break;
				}
				case pm_int8: { 
					pmUartScanf("%hhd", (char*)para_p->pmParaPoint);
					break;
				}
				case pm_uint16: { 
					pmUartScanf("%hu", (unsigned short*)para_p->pmParaPoint);
					break;
				}
				case pm_int16: { 
					pmUartScanf("%hd", (short*)para_p->pmParaPoint);
					break;
				}
				case pm_uint32: { 
					pmUartScanf("%u", (unsigned int*)para_p->pmParaPoint);
					break;
				}
				case pm_int32: { 
					pmUartScanf("%d", (int*)para_p->pmParaPoint);
					break;
				}
				case pm_float: { 
					pmUartScanf("%f", (float*)para_p->pmParaPoint);
					break;
				}
				case pm_double: { 
					pmUartScanf("%lf", (double*)para_p->pmParaPoint);
					break;
				}
				case pm_string: { 
					pmUartScanf("%s", (unsigned char*)para_p->pmParaPoint);
					break;
				}
			}
			/* ������� */
			pmUartPrintfAt(pmUART_PORT, (25), (8), "");
		}
	}
}
