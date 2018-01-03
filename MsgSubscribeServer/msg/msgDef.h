/*
 * msgDef.h
 *
 *  Created on: 2017��8��28��
 *      Author: Gift
 */

#ifndef MSGDEF_H_
#define MSGDEF_H_

/**********************************************************************************************
 *
 *��Ϣȡֵ��Χ��0~0xFFFFFFFF
 *����0~255Ϊ������Ϣ����msgServerPkt˽��ʹ��
 *������Ϣ������Ŀ��Ҫ�������ڴ˶���
 *
 **********************************************************************************************/

/**********************************************  msg server define  ***************************************************/

/**************************************************************************************************
 * define msg server internal msg type
 **************************************************************************************************/
#define MSG_SERVER_INFO_REQUEST						(0x08) /* msgServer��Ϣ��ѯ���� */
#define MSG_SERVER_INFO_REPLY						(0x09) /* msgServer��Ϣ��ѯ���� */
#define MSG_COM_HELLO								(0x10) /* msgServer������Ϣ */
#define MSG_COM_DETECT								(0x11) /* �ӿ�̽����Ϣ��ʵ�ַ���������״̬̽�� */
#define MSG_COM_DETECT_REPLY						(0x12) /* �ӿ�̽��Ӧ����Ϣ */
#define MSG_TIMER_TIME_TRIGGER_SECOND				(0x20) /* ÿ����ʱ�䴥�� */
#define MSG_TIMER_TIME_TIMEOUT						(0x21) /* �г�ʱ����ĳ����ʱ����ʱ�ˣ��Ҷ���ʱָ����Ҫ���ͣ����͸���Ϣ */
/**************************************************************************************************
 * end - define msg server internal msg type
 **************************************************************************************************/

/**************************************************************************************************
 * define msg server priority
 **************************************************************************************************/
/**************************************************************************************************
 * end - define msg server priority
 **************************************************************************************************/




/**********************************************  custom define  ***************************************************/

/**************************************************************************************************
 * define custom msg type
 **************************************************************************************************/
#define MSG_SERVER_BASE_NUM					(0x100)

#define SERVER_MSG_READ_REG					(MSG_SERVER_BASE_NUM + 0x10)
#define SERVER_MSG_WRITE_REG				(MSG_SERVER_BASE_NUM + 0x11)
#define SERVER_MSG_SET_FILTER				(MSG_SERVER_BASE_NUM + 0x12)
#define SERVER_MSG_BEGIN_LISTEN_DATA		(MSG_SERVER_BASE_NUM + 0x13)
#define SERVER_MSG_END_LISTEN_DATA			(MSG_SERVER_BASE_NUM + 0x14)
#define SERVER_MSG_LISTEN_RAW_DATA			(MSG_SERVER_BASE_NUM + 0x15)
#define SERVER_MSG_LISTEN_FILTER_DATA		(MSG_SERVER_BASE_NUM + 0x16)

#define SERVER_MSG_REPORT_REG				(MSG_SERVER_BASE_NUM + 0x90)
#define SERVER_MSG_REPORT_FILTER_CONFIG		(MSG_SERVER_BASE_NUM + 0x92)
#define SERVER_MSG_REPORT_LISTEN_DATA		(MSG_SERVER_BASE_NUM + 0x93)

/**************************************************************************************************
 * end - define custom msg type
 **************************************************************************************************/


/**************************************************************************************************
 * define custom priority
 **************************************************************************************************/
#define MSG_PRIORITY_CMD							(0x10)
#define MSG_PRIORITY_DATA							(0x80)

#define MSG_PRIORITY_READ_REG						MSG_PRIORITY_CMD
#define MSG_PRIORITY_WRITE_REG						MSG_PRIORITY_CMD
#define MSG_PRIORITY_SET_FILTER						MSG_PRIORITY_CMD
#define MSG_PRIORITY_BEGIN_LISTEN_DATA				MSG_PRIORITY_CMD
#define MSG_PRIORITY_END_LISTEN_DATA				MSG_PRIORITY_CMD
#define MSG_PRIORITY_LISTEN_RAW_DATA				MSG_PRIORITY_CMD
#define MSG_PRIORITY_LISTEN_FILTER_DATA				MSG_PRIORITY_CMD

#define MSG_PRIORITY_REPORT_REG						MSG_PRIORITY_CMD
#define MSG_PRIORITY_REPORT_FILTER_CONFIG			MSG_PRIORITY_CMD
#define MSG_PRIORITY_REPORT_LISTEN_DATA				MSG_PRIORITY_DATA
/**************************************************************************************************
 * end - define custom priority
 **************************************************************************************************/

#endif /* MSGDEF_H_ */
