/*
 * msgCom.h
 *
 *  Created on: 2017��8��28��
 *      Author: Gift
 */

#ifndef MSGSERVER_MSGCOM_H_
#define MSGSERVER_MSGCOM_H_
#include <stdbool.h>

#include "../../container/linkList.h"
#include "msgServer.h"
#include "msgServerDef.h"


typedef T_MSG_CALLBACK_FUNC MSG_COM_SEND_FUNC;//ͨ��ģ������˿ڷ��͵ĺ������Ͷ���
typedef T_MSG_CALLBACK_FUNC MSG_COM_RECV_FUNC;//ͨ��ģ������˿ڽ��յĺ������Ͷ��壬��ʱû��

//�˿���Ϣ�ṹ
typedef struct __msg_com_port_info {

	LIST_NODE listNode;//�˿��б������ڵ�ʵ��
	LIST_NODE *pHead;//�˿��б������ͷ�ڵ�ָ�루����ͨ���ýڵ���ҵ��ö˿ڰ󶨵��Ǹ�ͨ��ģ�飩

	bool enable;//�˿���Ϣ��Ч�Ա�־����Ϊfalse���ʾ�ö˿���ϢΪ��Ч��Ϣ
	MSG_COM_SEND_FUNC sendFunc;/*�˿ڷ��ͺ���ָ�룬�ú�������Ϣ���������ã�
								�ö˿��ڶ�����Ϣʱ���˺���ע�����Ϣ��������
								��Ϣ������ÿ���յ���Ӧ����Ϣ�󽫵��øú���������Ϣ����*/
	void *pSendParam;/*���Ͳ���������Ϣ����������sendFunc����ʱ�����ú����������Ϣ
	 	 	 	 	 	 �˿ڷ��ͺ�������Ϊ��������ƣ�ͨ����������Ĳ�ͬʵ�ֲ�ͬ����ӿڷ��ͣ�
	 	 	 	 	 	 �������ڣ�������һ����Ҫ�з��Ͳ�������*/

	unsigned int msgNum;//msgTable����Ŀ��
	T_MSG_SN msgTable[MSG_SERVER_MAX_SUBSCRIBE_NUM];//�Ӹö˿�����յ���hello����������ݣ�������¼�ö˿��ܹ����ĵķ���

}MSG_COM_PORT_INFO, *P_MSG_COM_PORT_INFO;

typedef struct __msg_com {

	LIST_NODE comPortListHead;//�˿��б������ͷ�ڵ�

	P_MSG_SERVER pServer;//ָ������ͨ��ģ��ҽӵ���Ϣ������

}MSG_COM, *P_MSG_COM;

P_MSG_COM newMsgCom(P_MSG_SERVER pServer);
void delMsgCom(P_MSG_COM pCom);
bool bindMsgComPort(P_MSG_COM pCom, P_MSG_COM_PORT_INFO pPort,
		MSG_COM_SEND_FUNC sendFunc, void *pSendParam, int timeout);
bool unbindMsgComPort(P_MSG_COM_PORT_INFO pPort);
void msgComPortRecvCallback(T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen, P_MSG_COM_PORT_INFO srcPort, P_MSG_COM_PORT_INFO desProt);
void msgComInitPort(P_MSG_COM_PORT_INFO pPort);
void msgSendHello (P_MSG_COM pCom);
void keepPortActive(P_MSG_COM_PORT_INFO port);

#endif /* MSGSERVER_MSGCOM_H_ */
