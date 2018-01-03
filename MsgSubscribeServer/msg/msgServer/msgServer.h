/*
 * msgServer.h
 *
 *  Created on: 2017��8��27��
 *      Author: Gift
 */

#ifndef MSGSERVER_MSGSERVER_H_
#define MSGSERVER_MSGSERVER_H_

#include <stdbool.h>

#include "../../container/hashTable.h"
#include "../system.h"
#include "msgServerDef.h"

/* T_MSG_CALLBACK_FUNC����true������Ҫ����ģ������Ӧ����Ϣ��
 * ������false�����������ĸ���Ϣ��ģ����Լ����������Ϣ
 * ��Ϣ�������У���ֹ������ʱ������������Ϣ�Ĵ�������б���
 * ������ʱ���������轫����Ϣ�Ĵ���ʽ����Ϊ�첽����������������
 * �Ը���Ϣ���д���Ŀǰ�첽�����������Ϣ�Ķ���ģ������ʵ�֣�
 * ���ڿɷ�����Ϣ����ģ����ͳһ���� */
typedef bool (*T_MSG_CALLBACK_FUNC)(T_MSG_SN msgType,
		unsigned char *pMsg, unsigned int msgLen, ...);
		/*void *pProcParam, unsigned int recvPortNum, unsigned int desPortNum) ;*/

//��Ϣӳ���ϵ��������Ҫ����Ϊ��msg�����Ӧ�Ĵ���ص��������а�
typedef struct __msg_map {

	HASH_NODE hn;//��msg map��Ϣ��hash�ڵ�ʵ��

	T_MSG_SN msg;//msg��ͬʱΪhash���е�key
	T_MSG_CALLBACK_FUNC pFunc;//��msg��Ӧ�Ĵ���ص�����
	void *pProcParam;//pFunc�Ĵ���������ڵ���pFuncʱ����ָ�봫��pFunc
	unsigned int priority; /* ֵԽС���ȼ�Խ�� */
	bool isPublic;//����Ϣ�Ƿ��ͨ��ͨ��ģ����������Ϣ����������
	bool isSyncMsg;//�Ƿ�Ϊͬ����Ϣ����Ϊ�첽��Ϣ����msg serverΪ�䵥�������߳�ִ�У�ÿ���첽��Ϣ��ӵ��һ�������߳�
	unsigned int subscribePort;//���ĸ���Ϣ�Ķ˿���Ϣ��ڵ�ַ���������ﲻ���Ķ˿���Ϣ�ľ������ݣ���ʹ��unsigned int���壩

}MSG_MAP, *P_MSG_MAP;

typedef struct __msg_server {

	//��Ϣ����������Ϣ�б���Ϣ�б�Ϊһ������ʽhash��
	HASH_HEAD ht[(1 << (MSG_SERVER_HASH_TABLE_SIZE_BIT))];
	unsigned int htSize;//hash��Ĵ�С��hash������Ĵ�С��

	HANDLE timerHandle;//����Ϣ����Ķ�ʱ�����

}MSG_SERVER, *P_MSG_SERVER;


P_MSG_SERVER newMsgServer();
int delMsgServer(P_MSG_SERVER pServer);
bool msgSubscribeFull (P_MSG_SERVER pServer, T_MSG_SN msg, T_MSG_CALLBACK_FUNC pFunc,
		void *pProcParam, unsigned int priority, bool isPublic, bool isSyncMsg, unsigned int subscribePort);
bool msgSubscribe (P_MSG_SERVER pServer, T_MSG_SN msg, T_MSG_CALLBACK_FUNC pFunc,
		void *pProcParam, unsigned int priority, bool isPublic, bool isSyncMsg);
bool msgUnsubscribe(P_MSG_SERVER pServer, T_MSG_SN msg, T_MSG_CALLBACK_FUNC pFunc, void *pProcParam);
bool msgPushFull(P_MSG_SERVER pServer, T_MSG_SN msg, unsigned char *pMsgSubstance,
		unsigned int msgLen, unsigned int sendPortNum, unsigned int desPortNum);
bool msgPushWithDes(P_MSG_SERVER pServer, T_MSG_SN msg,	unsigned char *pMsgSubstance,
		unsigned int msgLen, unsigned int desPortNum);
bool msgPushWithSrc(P_MSG_SERVER pServer, T_MSG_SN msg,
		unsigned char *pMsgSubstance, unsigned int msgLen, unsigned int srcPortNum);
bool msgPush(P_MSG_SERVER pServer, T_MSG_SN msg, unsigned char *pMsgSubstance, unsigned int msgLen);
unsigned int getWaitForProcAsynMsgCnt();

#endif /* MSGSERVER_MSGSERVER_H_ */
