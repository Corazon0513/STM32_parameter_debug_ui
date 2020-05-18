#ifndef __PARAMETER_MANAGER_H
#define __PARAMETER_MANAGER_H

#include <stdio.h>
#include <string.h>

#include "stm32h7xx.h"
#include "cmsis_os.h"
#include "usart.h"
#include "uart_struct.h"

/*===============���������빦��ʹ����==================*/
/* ������/���������������� */
#define PM_SIZE_NAME				16
/* �洢���ܿ��� */
#define PM_FLASH_SW					1
/* ��������UART�˿ھ�� */
#define pmUART_PORT 						huart1
#if ( PM_FLASH_SW == 1 )
/* FLASH��¼��ʼ��ַ */
#define PM_FLASH_ADDR			0x081E0000
#define PM_FLASH_SECTOR		FLASH_SECTOR_7
#define PM_FLASH_BANKS		FLASH_BANK_2
#endif

/* ������־����ӿ� */
#ifndef __UART_DEBUG_H
#define ERROR(...)
#define WARNING(...)
#define INFO(...)
#endif
/*=====================================================*/

/* ��������ö�� */
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

/* ����ע��� */
typedef struct pmParaRegister {
	/* ָ�� ָ�����	*/
	void* pmParaPoint;
	/* ��������		*/
	unsigned char pmParaName[PM_SIZE_NAME];
	/* ��������		*/
	pmParaType_t	pmParaType;
}pmParaRegister_t;

/* �������� */
typedef struct pmParaList {
	/* ������� */
	short no;
	/* ����ע��� */
	pmParaRegister_t* pmParaRegister;
	/* ��һ������ע��� */
	struct pmParaList* pmNextNode;
}pmParaList_t;

/* ������ */
typedef struct pmParaGroup {
	unsigned char pmGroupName[PM_SIZE_NAME];
	/* ��������ṹ�� */
	pmParaList_t* pmParaList;
	/* �����βָ�� */
	pmParaList_t* pmParaListLast;
	/* �������б������� */
	int pmParaSumInGroup;
}pmParaGroup_t;

/* ���������� */
typedef struct pmParaGroupList {
	/* �������� */
	short no; 
	/* ������ */
	pmParaGroup_t* pmParaGroup;
	/* ��һ�������� */
	struct pmParaGroupList* pmNextNode;
}pmParaGroupList_t;

void pmNewParaGroupCreateF(	pmParaGroup_t* group, const char* group_name,
									pmParaGroupList_t *group_node, pmParaList_t* list_head);
void pmNewParaRegCreateF(pmParaRegister_t* para_reg, void* para, const char* para_name,
	const char* para_type, pmParaList_t* para_node,
	pmParaGroup_t* group);
void pmShowUserInterface(void);

/*	���飺����һ��������
 *
 *	������	__GROUP_NAME__		����������
 */
#define pmNewParaGroupCreate(__GROUP_NAME__) \
		pmParaGroup_t pmParaGroup_##__GROUP_NAME__; \
		pmParaGroupList_t pmParaGroup_##__GROUP_NAME__##_ListNode; \
		pmParaList_t pmParaList_##__GROUP_NAME__##_Head; \
		pmNewParaGroupCreateF(	&pmParaGroup_##__GROUP_NAME__, #__GROUP_NAME__, \
								&pmParaGroup_##__GROUP_NAME__##_ListNode, &pmParaList_##__GROUP_NAME__##_Head)

 /*	���飺����һ���������˱����ı���ע���(�ֲ�����)
  *
  *	������	__PARA_TYPE__			��������
  *					__PARA_NAME__			��������
  *					__PARA_VALUE__		������ʼֵ
  *					__GROUP_NAME__		�����������ı�����
  */
#define pmNewParaCreate(__PARA_TYPE__, __PARA_NAME__, __PARA_VALUE__, __GROUP_NAME__) \
		__PARA_TYPE__ __PARA_NAME__ = __PARA_VALUE__; \
		pmParaRegister_t pmParaReg_##__PARA_NAME__; \
		pmParaList_t pmParaList_##__GROUP_NAME__##_##__PARA_NAME__; \
		pmNewParaRegCreateF(	&pmParaReg_##__PARA_NAME__, &__PARA_NAME__, #__PARA_NAME__,#__PARA_TYPE__,\
							&pmParaList_##__GROUP_NAME__##_##__PARA_NAME__, &pmParaGroup_##__GROUP_NAME__)


 /*	���飺���һ�����ڵı����������ɱ����ı���ע���(ȫ�ֱ���,�ַ������ṹ���б�����) 
  *
  *	������	__PARA_TYPE__			��������
  *					__PARA_NAME__			��������
  *					__PARA_ADDR__			������ַ
  *					__GROUP_NAME__		�����������ı�����
  */
#define pmAddPara(__PARA_TYPE__, __PARA_NAME__, __PARA_ADDR__, __GROUP_NAME__) \
		pmParaRegister_t pmParaReg_##__PARA_NAME__; \
		pmParaList_t pmParaList_##__GROUP_NAME__##_##__PARA_NAME__; \
		pmNewParaRegCreateF(	&pmParaReg_##__PARA_NAME__, __PARA_ADDR__, #__PARA_NAME__,#__PARA_TYPE__,\
											&pmParaList_##__GROUP_NAME__##_##__PARA_NAME__, &pmParaGroup_##__GROUP_NAME__)


#endif
