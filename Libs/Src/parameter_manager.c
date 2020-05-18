/**
  ******************************************************************************
  * @file    parameter_manager.c
  * @author  Corazon @ CDUESTC
  * @brief   可变参数交互式管理模块
  *          本文档提供了一组宏定义及函数，用于将嵌入式开发中的参数进行
  *					 可视化管理，调整，显示，存储。
  *
  @verbatim
 ===============================================================================
############################ 模块简介与注意事项 ################################
# 1. 使用前，请先在需要使用本库功能的源码中include头文件parameter_manager.h
# 2. 请将parameter_manager.h中参数配置与功能使能区按需配置
# 3. 确保UART全局中断开启，发送DMA开启，按照下方中断回调函数模板提示进行操作
# 4. 使用宏pmNewParaGroupCreate()进行变量组的创建
# 5. 需要管理的局部变量，使用宏pmNewParaCreate()进行变量的创建
# 6. 需要管理的全局变量，在正常方式声明后，使用宏pmAddPara()进行注册
# 7. 由于算法与堆栈限制，字符串类型变量请控制在7个字符内
# 8. 本模块使用静态内存分配方法管理变量，每个变量需要一定的空间用于保存
#    变量信息，请注意内存使用量。
# 9. 如出现HardFault中断，很可能是由于使用本模块在线程中注册了过多的变量，
#		 请适当的增加任务堆栈以避免此问题出现
# 10.由于在FLASH中存储的数据仅有变量内容，不含有变量名等变量其余信息，所以
#    在添加新的变量前请先注意将保存的数据备份，避免数据错位读取造成数据丢失。
################################################################################

################################ 版本更新历史 ##################################
# V1.3 (2019.7.27)
#	 ->将清屏刷新修改为清行刷新
#  ->串口缓冲区配置修改为使用uart_struct库
# V1.2 (2019.7.25) 
#  ->[BUG][FIXED]修复了快速输入时导致的UART假死BUG
#  ->[BUG][FIXED]修复了无变量组或变量组内无变量时产生非法访问导致HardFault问题
# V1.1 (2019.7.24) 
#  ->修改为Flash存储模式
# V1.0 (2019.7.23) 
#	 ->初版Release
################################################################################
 ===============================================================================

/<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
<--中断回调函数模板-->
<--依据实际环境替换尖括号中变量-->
<--将修改后的中断回调函数写入stm32h7xx_it.c中-->

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
/*	宏简介：UART格式化输出
 *
 *	参数：	__UART_PORT__		串口句柄	
 *					...							格式化字符串与可选参数
 */
#define pmUartPrintf(__UART_PORT__, ...) UartPrintf(__UART_PORT__, __VA_ARGS__)
				

/*	宏简介：UART格式化输入
 *
 *	参数：	...							格式化字符串与可选参数
 */				
#define pmUartScanf(...) \
				if( pmParaInput() == 1 )	{ \
					goto PM_PARA_SEL_LAB; \
				} \
				else { \
					sscanf((const char *)uart_receive, ##__VA_ARGS__); \
				}
				
/*	宏简介：UART无回显单字符输入
 *
 *	参数：	__BUFF__				字符保存缓冲区(uint8_t)
 */				
#define pmUartGetch(__BUFF__) \
				do{ \
					HAL_UART_Receive_IT(&pmUART_PORT, &__BUFF__, 1U); \
				}while( ulTaskNotifyTake(pdTRUE, 100) != 1);
				
/*	宏简介：UART指定行列输出
 *
 *	参数：	__UART_PORT__		串口句柄	
 *					__ROW__					指定行
 *          __COL__					指定列
 *					__STRING__			格式化字符串
 *					...							可选参数
 */				
#define pmUartPrintfAt(__UART_PORT__, __ROW__, __COL__, __STRING__, ...) \
				UartPrintf(__UART_PORT__, "\033[%d;%dH" __STRING__, (__ROW__), (__COL__), ##__VA_ARGS__ )
				
/*	宏简介：UART指定行列清行输出
 *
 *	参数：	__UART_PORT__		串口句柄	
 *					__ROW__					指定行
 *          __COL__					指定列
 *					__STRING__			格式化字符串
 *					...							可选参数
 */				
#define pmUartPrintfCleanAt(__UART_PORT__, __ROW__, __COL__, __STRING__, ...) \
				UartPrintf(__UART_PORT__, "\033[%d;%dH\033[K" __STRING__, (__ROW__), (__COL__), ##__VA_ARGS__ )

/* Private variables ---------------------------------------------------------*/
extern Uart_Handle_Struct_t uart_struct(pmUART_PORT);
/* 主变量组链表 */
pmParaGroupList_t	pmParaGroupList = {0, NULL, NULL};
/* 变量组数量 */
short							pmParaGroupSum = 0;

/* 256bit缓冲区 */
uint8_t  	dataBuff[32], dataCountBit = 0;
/* Private function prototypes -----------------------------------------------*/
/**
  * @简介   不定长参数读取
  *
  * @参数		无
  * @返回值 输入成功标志
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
  * @简介  	变量存储至FLASH
  *
  * @参数		无
  * @返回值 无
  */
void pmFlashSave(void){
	uint32_t	flashAddr = PM_FLASH_ADDR;
	memset(dataBuff, 0, 32);
	dataCountBit = 0;
	static FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SectorError;
	/* 擦除扇区 */
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
	/* 遍历变量组链表 */
	for(	pmParaGroupList_t* pmParaGroupListIter = pmParaGroupList.pmNextNode; ;
				pmParaGroupListIter = pmParaGroupListIter->pmNextNode) {
		/* 判断空变量组 */
		if( pmParaGroupListIter->pmParaGroup->pmParaList->pmNextNode != NULL ) {
			/* 遍历变量组内变量注册表 */
			for( 	pmParaList_t* pmParaListIter = pmParaGroupListIter->pmParaGroup->pmParaList->pmNextNode; ;
						pmParaListIter = pmParaListIter->pmNextNode) {
				/* 存储变量 */
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
  * @简介  	变量从FLASH读取
  *
  * @参数		无
  * @返回值 无
  */
void pmFlashLoad(void){
	uint32_t flashAddr = PM_FLASH_ADDR;
	memset(dataBuff, 0, 32);
	for(dataCountBit = 0; dataCountBit < 32; dataCountBit++){
		dataBuff[dataCountBit] = *((unsigned char*)flashAddr + dataCountBit);
	}
	flashAddr += 32;
	dataCountBit = 0;
	/* 遍历变量组链表 */
	for(	pmParaGroupList_t* pmParaGroupListIter = pmParaGroupList.pmNextNode; ;
				pmParaGroupListIter = pmParaGroupListIter->pmNextNode) {
		/* 判断空变量组 */
		if( pmParaGroupListIter->pmParaGroup->pmParaList->pmNextNode != NULL ) {
			/* 遍历变量组内变量注册表 */
			for( 	pmParaList_t* pmParaListIter = pmParaGroupListIter->pmParaGroup->pmParaList->pmNextNode; ;
						pmParaListIter = pmParaListIter->pmNextNode) {
				/* 读取变量 */
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
  * @简介  初始化新的变量组
  *
  * @参数	变量组指针， 变量组名，变量组链表节点地址， 变量链表头
  * @返回值 暂无
  */
void pmNewParaGroupCreateF(	pmParaGroup_t* group, const char* group_name,
							pmParaGroupList_t *group_node, pmParaList_t* list_head) {
	/* 保存组名 */
	strcpy((char *)group->pmGroupName, group_name);
	/* 置零初始化量 */
	group->pmParaSumInGroup = 0;
	/* NULL掉变量组链表头 */
	list_head->no = 0;
	list_head->pmParaRegister = NULL;
	list_head->pmNextNode = NULL;
	/* 绑定变量组与变量链表 */
	group->pmParaList = list_head;
	group->pmParaListLast = list_head;
	/* 变量组链表节点初始化 */
	group_node->pmParaGroup = group;
	group_node->pmNextNode = NULL;
	/* 寻找主变量组链表表尾 */
	pmParaGroupList_t* pmParaGroupListNode_p = &pmParaGroupList;
	short  pmParaGroupNo = 1;
	while (pmParaGroupListNode_p->pmNextNode != NULL) {
		pmParaGroupListNode_p = pmParaGroupListNode_p->pmNextNode;
		pmParaGroupNo++; 
	}
	/* 挂上本列表组 */
	group_node->no = pmParaGroupNo;
	pmParaGroupListNode_p->pmNextNode = group_node;
}

/**
  * @brief  初始化新的变量注册表
  *
  * @param  变量注册表地址， 变量地址， 变量名， 变量类型，
  *					变量链表节点地址， 变量组地址
  * @retval 暂无
  */
void pmNewParaRegCreateF(	pmParaRegister_t* para_reg, void* para, const char* para_name,
						const char* para_type, pmParaList_t* para_node,
						pmParaGroup_t* group) {
	/* 变量指针指向变量 */
	para_reg->pmParaPoint = para;
	/* 保存变量名 */
	strcpy((char *)para_reg->pmParaName, para_name);
	/* 分析与保存变量类型 */
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
	/* 变量链表节点初始化 */
	para_node->no = group->pmParaSumInGroup + 1;
	para_node->pmParaRegister = para_reg;
	para_node->pmNextNode = NULL;
	/* 变量组链表表尾挂载变量 */
	group->pmParaListLast->pmNextNode = para_node;
	group->pmParaListLast = group->pmParaListLast->pmNextNode;
	group->pmParaSumInGroup++;
}

/**
  * @brief  显示框线及标题
  *
  * @param  无
  *
  * @retval 无
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
  * @brief  显示控制界面 68列可用
  *
  * @param  无
  *			
  * @retval 无
  */
void pmShowUserInterface(void) {
	/* 输入选择项 */
	uint8_t choose;
	/* 变量组指针 初始化指向变量组列表中第一个变量组 */
	pmParaGroupList_t* group_p = pmParaGroupList.pmNextNode;
	for(;;){
		/* 选择变量组标签 */
		PM_GROUP_SEL_LAB:
		/* 检查变量组列表中是否存在变量组 */
		if( group_p == NULL ){
			/* 显示外框(无存储提示) */
			pmShowCase( 1 );
			pmUartPrintfAt(pmUART_PORT, (7), (10), " No any parameter group! Press ESC to turn back.");
			do{
				pmUartGetch(choose);
			}while( choose != 27 );
			return ;
		}
		/* 复位变量组指针 */
		group_p = pmParaGroupList.pmNextNode;
		/* 显示外框(带存储提示) */
		pmShowCase( 0 );
		/* 选变量组界面 */
		pmUartPrintfAt(pmUART_PORT, (5), (8), "Group select:");
		/* 遍历打印变量组列表中成员变量组 */
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
		/* 显示选择提示 */
		pmUartPrintfAt(pmUART_PORT, (18), (8), "Your choose:");
		/* 变量组指针指向变量组列表表头 */

		/* 变量列表指针 */
		pmParaList_t* list_p;
		/* 输入处理 */
		for(;;){
			pmUartGetch(choose);
			group_p = &pmParaGroupList;
			if( choose == 27 ) {
				/* 退出 */
				return ;
			}
			/* 保存至FLASH */
			else if ( choose == 'S' || choose == 's' ){
				pmFlashSave();
				pmUartPrintfAt(pmUART_PORT, (18), (8), "Successed save parameter value.");
				osDelay(250);
				goto PM_GROUP_SEL_LAB;
			}
			/* 从FLASH读取 */
			else if ( choose == 'L' || choose == 'l' ){
				pmFlashLoad();
				pmUartPrintfAt(pmUART_PORT, (18), (8), "Successed load parameter value.");
				osDelay(250);
				goto PM_GROUP_SEL_LAB;
			}
			/* 输入范围判断 */
			else if( choose > '0' && choose <= '9'){
				/* 寻找选择项 */
				for (int i = '1'; i < choose && group_p->pmNextNode != NULL; i++, group_p = group_p->pmNextNode);
				/* 正确输入 */
				if (group_p->pmNextNode != NULL) {
					break;
				}
			}
		}
		for(;;){
			PM_PARA_SEL_LAB:
			/* 指向待修改变量链表 */
			list_p = group_p->pmNextNode->pmParaGroup->pmParaList->pmNextNode;
			/* 清屏 */
			pmShowCase( 1 );
			/* 检查变量组列表中是否存在变量组 */
			if( list_p == NULL ){
				pmUartPrintfAt(pmUART_PORT, (7), (10), " No any parameter in this group! Press ESC to turn back.");
				do{
					pmUartGetch(choose);
				}while( choose != 27 );
				goto PM_GROUP_SEL_LAB;
			}
			/* 选择变量 */
			pmUartPrintfAt(pmUART_PORT, (5), (8), "Parameter select:");
			/* 遍历打印变量组中成员变量及变量值*/
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
			/* 显示选择提示 */
			pmUartPrintfAt(pmUART_PORT, (20), (8), "Your choose:");
			/* 变量注册表指针 */
			pmParaRegister_t* para_p;
			/* 输入处理 */
			for(;;){
				pmUartGetch(choose);
				/* 变量列表指针指向变量列表头 */
				list_p = group_p->pmNextNode->pmParaGroup->pmParaList;
				if ( choose == 27 ) goto PM_GROUP_SEL_LAB;
				/* 输入范围判断 */
				if( choose > '0' && choose <= '9'){
					/* 寻找选择项 */
					for (int i = '1'; i < choose && list_p->pmNextNode != NULL; i++, list_p = list_p->pmNextNode);
					/* 选中选择项 */
					if (list_p->pmNextNode != NULL) {
						para_p = list_p->pmNextNode->pmParaRegister;
						break ;
					}
				}
			}
			/* 显示赋值提示 */
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
			/* 光标下移 */
			pmUartPrintfAt(pmUART_PORT, (25), (8), "");
		}
	}
}
