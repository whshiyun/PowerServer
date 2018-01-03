/*
 * msgServer.c
 *
 *  Created on: 2017��8��27��
 *      Author: Gift
 */

#include <stdio.h>
#include <stdlib.h>
#include "msgServer.h"

#include "../../container/hashTable.h"
#include "../msgDef.h"
#include "../system.h"
#include "msgServerDef.h"
#include "msgTime.h"

//�첽ִ�в����ṹ���ýṹ����������Ϣ��Ӧ����Ĳ���
//����Ϣ��Ҫ�첽ִ�У���Ϊ����Ϣ���������߳�ִ�У�ͬʱ����Ϣ��Ӧ�Ĳ���������߳�
typedef struct __msg_asyn_param {
	T_MSG_CALLBACK_FUNC pFunc;//��Ϣ��Ӧ����ָ��
	T_MSG_SN msgType;//��Ϣ����
	unsigned char *pMsg;//��Ϣʵ���ڴ��ַ
	unsigned int msgLen;//��Ϣʵ�峤��
	void *pFuncParam;//pFunc�Ĵ������
	unsigned int srcPortNum;//push����Ϣ�Ķ˿���Ϣ��ڵ�ַ
	unsigned int desPortNum;//�ڴ��������Ϣ�Ķ˿���Ϣ��ڵ�ַ
}MSG_ASYN_PARAM, *P_MSG_ASYN_PARAM;

//��ǰ����ִ�е��첽��Ϣ����������������Ϣ������ʱ����ȴ������첽��Ϣ����ִ����ϲŽ���
//ֻ������������Ϣ���������ǰ�ȫ�Ľ�����ʽ
static volatile unsigned int asynProcessCnt = 0;

/***************************************************************************
 * msg server response msg
 ***************************************************************************/

/*******************************************************************
 * ��ȡ��Ϣ��������ǰ״̬
 * ��δʵ�֣����Ը��ݽ�����Ҫ��ȡ��Ҫ����Ϣ������״̬
 * �ù�����ҪΪ���Լ���Ϣ������״̬���
 *******************************************************************/
static void msgServerGetInfo(P_MSG_SERVER pServer, P_MSG_SERVER_INFO pInfo) {

	//���´���Ϊ����ʽ���룬��Ϊ���Ըù���ʹ��
	P_MSG_MAP pos = NULL;
	for(int i=0, j=0; i<pServer->htSize; i++) {
		j = 0;
		hlist_for_each_entry(pos, &pServer->ht[i], hn) {
			j ++;
		}
		if(0 != j)
			printf("msg 0x%02x, subscribe %d frequency \r\n", i, j);
	}
}

/*******************************************************************
 * ����һ�����յ�����Ϣ������״̬��ѯ����
 * ������Ϣ������״̬�ظ�����ѯ�˿�
 *******************************************************************/
static bool msgServerInfoRequestProc(T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen, P_MSG_SERVER pServer, unsigned int recvPort) {

	//��ȡ��Ϣ������״̬
	MSG_SERVER_INFO info;
	msgServerGetInfo(pServer, &info);

	//���ѯ�˿�����״̬����
	msgPushWithDes(pServer, MSG_SERVER_INFO_REPLY, (unsigned char *)&info, sizeof(MSG_SERVER_INFO), recvPort);

	printf("msgServerInfoRequestProc \r\n");

	return true;
}
/***************************************************************************
 * end msg server response msg
 ***************************************************************************/

/*******************************************************************
 * �첽��Ϣ�ص�����
 * ����Ϣ��Ҫʵ���첽��Ӧʱ��Ϊ����Ϣ�����������̣߳��ú�����Ϊ���̵߳�
 * �ص����������ڸú����ж���Ϣ������Ӧ
 *******************************************************************/
static void msgAsynProcess(void *pParam) {

	if(NULL == pParam)
		return ;

	P_MSG_ASYN_PARAM pMsgParam = (P_MSG_ASYN_PARAM)pParam;
	if(NULL == pMsgParam->pFunc)
		goto __msgAsynProcessError;

	pMsgParam->pFunc(pMsgParam->msgType, pMsgParam->pMsg, pMsgParam->msgLen,
			pMsgParam->pFuncParam, pMsgParam->srcPortNum, pMsgParam->desPortNum);

__msgAsynProcessError :
	free(pParam);
	if(0 != asynProcessCnt)
		asynProcessCnt --;
}

/*******************************************************************
 * ����һ��msg map��ϵ���ú�����Ϊ�µ�ӳ���ϵ��Ϣ�����ڴ�
 *******************************************************************/
static P_MSG_MAP newMsgMap(T_MSG_SN msg, T_MSG_CALLBACK_FUNC pFunc,
		void *pProcParam, unsigned int priority, bool isPublic,
		bool isSyncMsg, unsigned int subscribePort) {

	P_MSG_MAP pMap = (P_MSG_MAP)malloc(sizeof(MSG_MAP));
	if(NULL == pMap)
		return NULL;

	INIT_HLIST_NODE(&(pMap->hn));
	pMap->msg = msg;
	pMap->pFunc = pFunc;
	pMap->pProcParam = pProcParam;
	pMap->priority = priority;
	pMap->isPublic = isPublic;
	pMap->isSyncMsg = isSyncMsg;
	pMap->subscribePort = subscribePort;

	return pMap;
}

/*******************************************************************
 * ɾ��һ��msg map��ϵ���ú������ͷŸ�ӳ���ϵ��Ϣ���ڴ�
 *******************************************************************/
static void delMsgMap(P_MSG_MAP pMsgMap) {

	if(NULL == pMsgMap)
		return ;

	//����Ϣ����������Ϣ�б�hash����ɾ������Ϣ
	hash_del(&(pMsgMap->hn));

	free(pMsgMap);
}

/*******************************************************************
 * ���ĳ����Ϣ�����а󶨹�ϵ
 * ��msg server����Ϣ�б��У�msg��ֵΪkey����ô������Ӧ����Ϣ�Ļص�����
 * �������hash���ͬһ��hashֵ�У������hash��ʹ������ʽ���г�ͻ����
 * �ú���Ϊɾ��hash����ĳ��key�µ�����Ԫ��
 *******************************************************************/
static void delMsgMapLink(HASH_HEAD *pHead) {

	P_MSG_MAP pos = NULL, n = NULL;
	hlist_for_each_entry_safe(pos, n, pHead, hn) {
		delMsgMap (pos);
	}
}

/*******************************************************************
 * ����һ��msg server����δ������ڴ�
 *******************************************************************/
P_MSG_SERVER newMsgServer() {

	P_MSG_SERVER pServer = (P_MSG_SERVER)malloc(sizeof(MSG_SERVER));
	if(NULL == pServer)
		return NULL;

	//��ʼ����Ϣ�б�hash��
	hash_init(pServer->ht);

	pServer->htSize = HASH_SIZE(pServer->ht);//����hash���ʵ�ʴ�С�����鳤�ȣ�
	pServer->timerHandle = newMsgTimer(pServer);//������������ʱ��

	//msg serverĬ����Ҫ������Ϣ��������ѯ״̬��ѯ��Ϣ
	//��ѯʽ����Ҫʹ���첽��Ϣ���Է�ͬһ���߳���������շ������������
	msgSubscribe (pServer, MSG_SERVER_INFO_REQUEST, (T_MSG_CALLBACK_FUNC)msgServerInfoRequestProc,
			pServer, MSG_PRIORITY_CMD, false, false);

	return pServer;
}

/*******************************************************************
 * ɾ����Ϣ�����������ͷ��ڴ�
 *******************************************************************/
int delMsgServer(P_MSG_SERVER pServer) {

	if(NULL == pServer)
		return 0;

	//�ȴ������첽��Ϣ��ִ�н���������ֱ�ӽ����ǲ���ȫ��
	//������������չΪ��ʱʽ����
	while(getWaitForProcAsynMsgCnt());

	//��ɾ��ʱ��
	delMsgTimer(pServer->timerHandle);

	//ɾ�����ж��ĵ���Ϣ
	unsigned int i = 0;
	for(i=0; i<pServer->htSize; i++) {
		delMsgMapLink(&(pServer->ht[i]));
	}

	free(pServer);

	printf("msg server del !! \r\n");

	return 0;
}

/*******************************************************************
 * ����Ϣ����������һ����Ϣ
 * P_MSG_SERVER pServer��������Ϣ�ķ�������ڵ�ַ
 * T_MSG_SN msg�����ĵ���Ϣ����ֵ
 * T_MSG_CALLBACK_FUNC pFunc�����������͸���Ϣʱ�Ļص�����
 * void *pProcParam��pFunc�Ĵ������
 * unsigned int priority������Ϣ�����ȼ�
 * bool isPublic���ö�����Ϣ�Ƿ�ᱻ·�ɵ�������Ϣ�������ϣ������Ƿ��Զ����������������ĸ���Ϣ
 * bool isSyncMsg���Ƿ�Ϊͬ����Ϣ����Ϊ�첽��Ϣ������ִ��ʱ��Ϊ�䴴���������߳̽���ִ�У�ÿ���첽��Ϣ��ӵ��һ�������߳�
 * unsigned int subscribePort��һ��Ϊ��Ϣ�Ķ��Ķ˿ڵĶ˿���Ϣ��ڵ�ַ���������ڲ����Ķ˿���Ϣ���ݣ���ʹ��unsigned int������
 *******************************************************************/
bool msgSubscribeFull (P_MSG_SERVER pServer, T_MSG_SN msg,
		T_MSG_CALLBACK_FUNC pFunc, void *pProcParam, unsigned int priority,
		bool isPublic, bool isSyncMsg, unsigned int subscribePort) {

	//����һ���µ�msg map
	P_MSG_MAP pMap = newMsgMap(msg, pFunc, pProcParam, priority, isPublic, isSyncMsg, subscribePort);
	if(NULL == pMap)
		return false;

	//��ȡ����Ϣ��Ӧ��hash���е�λ��
	HASH_HEAD *pHead = hash_head_get(pServer->ht, msg);
	if((pHead > &(pServer->ht[pServer->htSize - 1])) || (pHead < &(pServer->ht[0]))) {
		delMsgMap(pMap);
		return false;
	}

	//���������Ѿ����Ĺ�����Ϣ��msg map
	P_MSG_MAP pos = NULL;
	bool insertFlag = false;
	hlist_for_each_entry(pos, pHead, hn) {
		//�鿴����Ϣ����Ӧ�����������Ƿ��Ѿ����Ĺ�����Ϣ�������Ĺ����ظ�����
		//������Ϊ����Ϣ���캯�����������������ͬ����Ϊͬһ��������ͼ���ʲ��������¶���
		if((pFunc == pos->pFunc) && (pProcParam == pos->pProcParam)) {
			delMsgMap(pMap);
			insertFlag = true;
			break;
		}

		//����Ϣ���ȼ�����������������ڶ���ʱ��ɣ�ÿ����Ϣ��Ӧʱ�Ͳ����ٽ�������
		if(pos->priority > pMap->priority) {
			hash_add_before((HASH_NODE *)(&(pos->hn)), (HASH_NODE *)(&(pMap->hn)));
			insertFlag = true;
			break;
		}
	}

	//��û���ҵ����Լ����ȼ�ֵ��Ķ��ģ�����Ϊ�Լ�Ϊ���ȼ�ֵ������������
	//���Լ����������ĩβ�����ȼ�ֵԽ���ʾ���ȼ�Խ�ͣ�
	if(false == insertFlag) {
		hash_add_tail(pHead, &(pMap->hn));
		insertFlag = true;
	}

	return insertFlag;
}

/*******************************************************************
 * ʹ�ñ��ض˿ڶ�����Ϣ
 * �����øú������ж��ģ�����Ϊ���Ĵ���Ϣ��ģ��Ϊһ������ģ��
 *******************************************************************/
bool msgSubscribe (P_MSG_SERVER pServer, T_MSG_SN msg, T_MSG_CALLBACK_FUNC pFunc,
		void *pProcParam, unsigned int priority, bool isPublic, bool isSyncMsg) {

	return msgSubscribeFull(pServer, msg, pFunc, pProcParam,
			priority, isPublic, isSyncMsg, MSG_SERVER_LOCAL_SUBSCRIBE_PORT_NUM);
}

/*******************************************************************
 * �˶�һ������
 * T_MSG_CALLBACK_FUNC pFunc��֮ǰ���ĸ���Ϣʱʹ�õ���Ϣ�ص�������ڵ�ַ
 * void *pProcParam��֮ǰ���ĸ���Ϣʱʹ�õĺ�������
 * ������Ϊ����Ϣ���캯�����������������ͬ����Ϊͬһ�����ģ�������������
 * ����Ψһָ��һ������
 *******************************************************************/
bool msgUnsubscribe(P_MSG_SERVER pServer, T_MSG_SN msg, T_MSG_CALLBACK_FUNC pFunc, void *pProcParam) {

	HASH_HEAD *pHead = hash_head_get(pServer->ht, msg);
	if((pHead > &(pServer->ht[pServer->htSize - 1])) || (pHead < &(pServer->ht[0]))) {
		return false;
	}

	P_MSG_MAP pos = NULL, n = NULL;
	hlist_for_each_entry_safe(pos, n, pHead, hn) {
		if((pFunc == pos->pFunc) && (pProcParam == pos->pProcParam)) {
			delMsgMap(pos);
		}
	}

	return true;
}

/*******************************************************************
 * ����Ϣ����������һ����Ϣ
 * ��ģ�������һ����Ϣ��Ҫ�������ͣ�����øú�����
 * P_MSG_SERVER pServer����Ҫ������Ϣ�ķ�������ڵ�ַ
 * T_MSG_SN msg��������Ϣ������
 * unsigned char *pMsgSubstance��������Ϣʵ����ڴ�ָ��
 * unsigned int msgLen��������Ϣʵ��ĳ���
 * unsigned int srcPortNum������Ϣ���ĸ��˿����͵ģ����ֶ�Ϊ�˿ڵ�Ψһ��ʶ��һ��Ϊ�ö˿ڵĶ˿���Ϣ��ڵ�ַ��
 * unsigned int desPortNum������Ϣϣ���ĸ��˿ڴ������ֶ�Ϊ�˿ڵ�Ψһ��ʶ��һ��Ϊ�ö˿ڵĶ˿���Ϣ��ڵ�ַ����
 * ���ֶ���ΪNULL�����ʾ���ж˿ڶ����д���
 *******************************************************************/
bool msgPushFull(P_MSG_SERVER pServer, T_MSG_SN msg, unsigned char *pMsgSubstance,
		unsigned int msgLen, unsigned int srcPortNum, unsigned int desPortNum) {

	HASH_HEAD *pHead = hash_head_get(pServer->ht, msg);
	if((pHead > &(pServer->ht[pServer->htSize - 1])) || (pHead < &(pServer->ht[0]))) {
		return false;
	}

	P_MSG_MAP pos = NULL, n = NULL;
	hlist_for_each_entry_safe(pos, n, pHead, hn) {
		if(true == pos->isSyncMsg) {
			if(NULL == pos->pFunc)
				continue;
			if((MSG_SERVER_ALL_SUBSCRIBE_PORT_NUM != desPortNum) && (desPortNum != pos->subscribePort))
				continue;
			if(true == pos->pFunc(msg, pMsgSubstance, msgLen, pos->pProcParam, srcPortNum, desPortNum))
				break;
		} else {
			P_MSG_ASYN_PARAM pParam = (P_MSG_ASYN_PARAM)malloc(sizeof(MSG_ASYN_PARAM));
			pParam->pFunc = pos->pFunc;
			pParam->msgType = msg;
			pParam->pMsg = pMsgSubstance;
			pParam->msgLen = msgLen;
			pParam->pFuncParam = pos->pProcParam;
			pParam->srcPortNum = srcPortNum;
			pParam->desPortNum = desPortNum;

			if(NULL != MsgCreateTask(0, msgAsynProcess, pParam))
				asynProcessCnt ++;
		}
	}

	return true;
}

/*******************************************************************
 * �����������һ����Ϣ����ָ��Ŀ�Ķ˿ڣ�Դ�˿�Ϊ���ض˿ں�
 *******************************************************************/
bool msgPushWithDes(P_MSG_SERVER pServer, T_MSG_SN msg,
		unsigned char *pMsgSubstance, unsigned int msgLen, unsigned int desPortNum) {

	return msgPushFull(pServer, msg, pMsgSubstance,
			msgLen, MSG_SERVER_LOCAL_SUBSCRIBE_PORT_NUM, desPortNum);
}

/*******************************************************************
 * �����������һ����Ϣ����ָ��Դ�˿ڣ�Ŀ�Ķ˿�Ϊ���ж˿�
 *******************************************************************/
bool msgPushWithSrc(P_MSG_SERVER pServer, T_MSG_SN msg,
		unsigned char *pMsgSubstance, unsigned int msgLen, unsigned int srcPortNum) {

	return msgPushFull(pServer, msg, pMsgSubstance,
			msgLen, srcPortNum, MSG_SERVER_ALL_SUBSCRIBE_PORT_NUM);
}

/*******************************************************************
 * �����������һ����Ϣ��Դ�˿�Ϊ���ض˿ڣ�Ŀ�Ķ˿�Ϊ���ж˿�
 *******************************************************************/
bool msgPush(P_MSG_SERVER pServer, T_MSG_SN msg,
		unsigned char *pMsgSubstance, unsigned int msgLen) {

	return msgPushWithDes(pServer, msg, pMsgSubstance,
			msgLen, MSG_SERVER_ALL_SUBSCRIBE_PORT_NUM);
}

/*******************************************************************
 * ��ȡ��ǰ����ִ�е��첽��Ϣ����
 *******************************************************************/
unsigned int getWaitForProcAsynMsgCnt() {

	return asynProcessCnt;
}
















