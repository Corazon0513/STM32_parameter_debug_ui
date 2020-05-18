#ifndef __PARAMETER_MANAGER_H
#define __PARAMETER_MANAGER_H

#include <stdio.h>
#include <string.h>

#include "stm32h7xx.h"
#include "cmsis_os.h"
#include "usart.h"
#include "uart_struct.h"

/*===============参数配置与功能使能区==================*/
/* 变量名/变量组名长度上限 */
#define PM_SIZE_NAME				16
/* 存储功能开关 */
#define PM_FLASH_SW					1
/* 参数管理UART端口句柄 */
#define pmUART_PORT 						huart1
#if ( PM_FLASH_SW == 1 )
/* FLASH烧录起始地址 */
#define PM_FLASH_ADDR			0x081E0000
#define PM_FLASH_SECTOR		FLASH_SECTOR_7
#define PM_FLASH_BANKS		FLASH_BANK_2
#endif

/* 调试日志输出接口 */
#ifndef __UART_DEBUG_H
#define ERROR(...)
#define WARNING(...)
#define INFO(...)
#endif
/*=====================================================*/

/* 变量类型枚举 */
typedef enum {
	pm_uint8 					= 0x0,
	pm_int8						= 0x1,
	pm_uint16					=	0x2,
	pm_int16					= 0x3,
	pm_uint32					= 0x4,
	pm_int32					= 0x5,
	pm_float					=	0x6,
	pm_double					=	0x7,
	pm_string					=	0x8
}pmParaType_t;

/* 变量注册表 */
typedef struct pmParaRegister {
	/* 指针 指向变量	*/
	void* pmParaPoint;
	/* 变量名称		*/
	unsigned char pmParaName[PM_SIZE_NAME];
	/* 变量类型		*/
	pmParaType_t	pmParaType;
}pmParaRegister_t;

/* 变量链表 */
typedef struct pmParaList {
	/* 变量编号 */
	short no;
	/* 变量注册表 */
	pmParaRegister_t* pmParaRegister;
	/* 下一个变量注册表 */
	struct pmParaList* pmNextNode;
}pmParaList_t;

/* 变量组 */
typedef struct pmParaGroup {
	unsigned char pmGroupName[PM_SIZE_NAME];
	/* 变量链表结构体 */
	pmParaList_t* pmParaList;
	/* 链表表尾指针 */
	pmParaList_t* pmParaListLast;
	/* 变量组中变量总数 */
	int pmParaSumInGroup;
}pmParaGroup_t;

/* 变量组链表 */
typedef struct pmParaGroupList {
	/* 变量组编号 */
	short no; 
	/* 变量组 */
	pmParaGroup_t* pmParaGroup;
	/* 下一个变量组 */
	struct pmParaGroupList* pmNextNode;
}pmParaGroupList_t;

void pmNewParaGroupCreateF(	pmParaGroup_t* group, const char* group_name,
									pmParaGroupList_t *group_node, pmParaList_t* list_head);
void pmNewParaRegCreateF(pmParaRegister_t* para_reg, void* para, const char* para_name,
	const char* para_type, pmParaList_t* para_node,
	pmParaGroup_t* group);
void pmShowUserInterface(void);

/*	宏简介：创建一个变量组
 *
 *	参数：	__GROUP_NAME__		变量组名称
 */
#define pmNewParaGroupCreate(__GROUP_NAME__) \
		pmParaGroup_t pmParaGroup_##__GROUP_NAME__; \
		pmParaGroupList_t pmParaGroup_##__GROUP_NAME__##_ListNode; \
		pmParaList_t pmParaList_##__GROUP_NAME__##_Head; \
		pmNewParaGroupCreateF(	&pmParaGroup_##__GROUP_NAME__, #__GROUP_NAME__, \
								&pmParaGroup_##__GROUP_NAME__##_ListNode, &pmParaList_##__GROUP_NAME__##_Head)

 /*	宏简介：创建一个变量及此变量的变量注册表(局部变量)
  *
  *	参数：	__PARA_TYPE__			变量类型
  *					__PARA_NAME__			变量名称
  *					__PARA_VALUE__		变量初始值
  *					__GROUP_NAME__		变量所归属的变量组
  */
#define pmNewParaCreate(__PARA_TYPE__, __PARA_NAME__, __PARA_VALUE__, __GROUP_NAME__) \
		__PARA_TYPE__ __PARA_NAME__ = __PARA_VALUE__; \
		pmParaRegister_t pmParaReg_##__PARA_NAME__; \
		pmParaList_t pmParaList_##__GROUP_NAME__##_##__PARA_NAME__; \
		pmNewParaRegCreateF(	&pmParaReg_##__PARA_NAME__, &__PARA_NAME__, #__PARA_NAME__,#__PARA_TYPE__,\
							&pmParaList_##__GROUP_NAME__##_##__PARA_NAME__, &pmParaGroup_##__GROUP_NAME__)


 /*	宏简介：添加一个存在的变量及此生成变量的变量注册表(全局变量,字符串及结构体中变量等) 
  *
  *	参数：	__PARA_TYPE__			变量类型
  *					__PARA_NAME__			变量名称
  *					__PARA_ADDR__			变量地址
  *					__GROUP_NAME__		变量所归属的变量组
  */
#define pmAddPara(__PARA_TYPE__, __PARA_NAME__, __PARA_ADDR__, __GROUP_NAME__) \
		pmParaRegister_t pmParaReg_##__PARA_NAME__; \
		pmParaList_t pmParaList_##__GROUP_NAME__##_##__PARA_NAME__; \
		pmNewParaRegCreateF(	&pmParaReg_##__PARA_NAME__, __PARA_ADDR__, #__PARA_NAME__,#__PARA_TYPE__,\
											&pmParaList_##__GROUP_NAME__##_##__PARA_NAME__, &pmParaGroup_##__GROUP_NAME__)


#endif
