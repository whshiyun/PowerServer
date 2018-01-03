/*
 * msgCom.c
 *
 *  Created on: 2017��8��28��
 *      Author: Gift
 */


#include <stdio.h>
#include <stdlib.h>


#include "../msgDef.h"
#include "../system.h"
#include "msgComDef.h"
#include "msgTime.h"
#include "msgTimeDef.h"
#include "msgCom.h"

/*
 * ���и��������⣬��Ϣ��������ĳport������Ϣʱ���ýӿڳ�ʱ��ʱ��û�и���ˢ��
 * ��������ʱ���ⲿ�˿ڵĻص����������ÿ����Ϣ������������Ϣ���ⲿ�˿ں�
 * �ⲿ�˿ڵĻص�����������ˢ�³�ʱ��ʱ��
 */

#define getLocalId __getLocalId

struct msg_bind_map {
	T_MSG_SN msgType;
	T_MSG_CALLBACK_FUNC func;
	bool isSyncMsg;
	int priority;
};

static bool __helloPktPro(T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen, void *pProcParam, unsigned int recvPort);
static bool __detectPktPro(T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen, void *pProcParam, unsigned int recvPort);
static bool __detectReplyPktPro(T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen, void *pProcParam, unsigned int recvPort);

//comĬ�϶��ĵ���Ϣ
static const struct msg_bind_map msgComBindMapTab[] = {
		{MSG_COM_HELLO, (T_MSG_CALLBACK_FUNC)__helloPktPro, true, 0x10},
		{MSG_COM_DETECT, (T_MSG_CALLBACK_FUNC)__detectPktPro, false, 0x10},
		{MSG_COM_DETECT_REPLY, (T_MSG_CALLBACK_FUNC)__detectReplyPktPro, true, 0x10},
};

/*******************************************************************
 * ��ȡ����COM��Ψһ��ַ����ʱ���Կ���������Mac��ַ
 *******************************************************************/
static void __getLocalId(unsigned char *pId, unsigned int idLen) {

}

/*******************************************************************
 * ���port�������ĵ�������Ϣ
 *******************************************************************/
static void __clearPortSubscribe(P_MSG_COM_PORT_INFO pPort) {

	if(NULL == pPort)
		return ;

	P_MSG_COM pCom = list_entry(pPort->pHead, MSG_COM, comPortListHead);
	for(int i = 0; i < pPort->msgNum; i++) {
		msgUnsubscribe(pCom->pServer, pPort->msgTable[i], pPort->sendFunc, pPort->pSendParam);
	}
}

/*******************************************************************
 * ����һ�����յ���hello����
 * ������Ϊhello�����е�������Ϣ�ڱ����������ж���
 *******************************************************************/
static bool __helloPktPro(T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen, void *pProcParam, unsigned int recvPort) {

	if(NULL == pMsg)
		return true;

	if(sizeof(MSG_HELLO_PKT) > msgLen) {
		printf("__helloPktPro error : sizeof(MSG_HELLO_PKT) > msgLen \r\n");
		printf("sizeof(MSG_HELLO_PKT) = %d, msgLen = %d \r\n", sizeof(MSG_HELLO_PKT), msgLen);
		return true;
	}

	P_MSG_HELLO_PKT pHello = (P_MSG_HELLO_PKT)pMsg;
	if(MSG_SERVER_MAX_SUBSCRIBE_NUM <= pHello->msgNum) {
		printf("__helloPktPro error : MSG_SERVER_MAX_SUBSCRIBE_NUM <= pHello->msgNum \r\n");
		printf("pHello->msgNum = %u \r\n", pHello->msgNum);
		return true;
	}

	P_MSG_COM_PORT_INFO pPort = (P_MSG_COM_PORT_INFO)recvPort;
	P_MSG_COM pCom = list_entry(pPort->pHead, MSG_COM, comPortListHead);

	//���������֮ǰ�Ķ��ģ���֤ÿ��ˢ��
	__clearPortSubscribe(pPort);

	//�������µ�Ҫ����ж���
	unsigned int i = 0;
	for(i=0; i<pHello->msgNum; i++) {
		msgSubscribeFull(pCom->pServer, pHello->msgTable[i],
				(T_MSG_CALLBACK_FUNC)pPort->sendFunc, pPort->pSendParam,
				0, false, true, (unsigned int)pPort);
	}

	//��port info�д洢���µĶ�����Ϣ
	pPort->msgNum = pHello->msgNum;
	for(i=0; i<pPort->msgNum; i++) {
		pPort->msgTable[i] = pHello->msgTable[i];
	}

	return true;
}

/*******************************************************************
 * ����һ��������̽����Ϣ
 * ���յ�������̽����Ϣ����������һ��Ӧ����
 *******************************************************************/
static bool __detectPktPro(T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen, void *pProcParam, unsigned int recvPort) {

	if(NULL == pMsg)
		return true;

	if(sizeof(MSG_DETECT_PKT) > msgLen) {
		printf("__detectPktPro error : sizeof(MSG_DETECT_PKT) > msgLen \r\n");
		printf("msgLen = %d \r\n", msgLen);
		return true;
	}

	P_MSG_DETECT_PKT pPkt = (P_MSG_DETECT_PKT)pMsg;
	getLocalId(pPkt->locId, MSG_SERVER_LOCAL_ID_LEN);
	P_MSG_COM_PORT_INFO pPort = (P_MSG_COM_PORT_INFO)recvPort;
	pPort->sendFunc(MSG_COM_DETECT_REPLY, pMsg, msgLen, pPort->pSendParam, pPort);

	return true;
}

/*******************************************************************
 * ����̽��Ӧ����
 * ����Ŀǰ������������������̽�ⱨ�ģ����Ըñ��ľ�����Ϊ��δʵ��
 *******************************************************************/
static bool __detectReplyPktPro(T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen, void *pProcParam, unsigned int recvPort) {

	printf("__detectReplyPktPro");

	///��δʵ��

	return true;
}

/*******************************************************************
 * ����һ��hello����
 *******************************************************************/
static unsigned int __createHello(P_MSG_SERVER pS, P_MSG_HELLO_PKT pHello) {

	unsigned int num = 0, i = 0;
	P_MSG_MAP pos = NULL;

	for(i=0; i<pS->htSize; i++) {
		hlist_for_each_entry(pos, &(pS->ht[i]), hn) {
			pHello->msgTable[num] = pos->msg;
			num ++;
		}
	}

	pHello->msgNum = num;

	return (sizeof(MSG_HELLO_PKT) - sizeof(pHello->msgTable) + (num * sizeof(T_MSG_SN)));
}

/*******************************************************************
 * ��pCom������ע����Ķ˿ڷ�����Ϣ
 *******************************************************************/
static void __msgPortSend(P_MSG_COM pCom, T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen) {

	P_MSG_COM_PORT_INFO pos = NULL;
	list_for_each_entry(pos, &(pCom->comPortListHead), listNode) {
		pos->sendFunc(msgType, pMsg, msgLen, pos->pSendParam);
	}
}

/*******************************************************************
 * ��ͨ�Ŷ˿��б������һ���˿�
 *******************************************************************/
static bool addMsgComPort(P_MSG_COM pCom, P_MSG_COM_PORT_INFO pPort) {

	if((NULL == pCom) || (NULL == pPort))
		return false;

//	P_MSG_COM_PORT_INFO pPort = (P_MSG_COM_PORT_INFO)malloc(sizeof(MSG_COM_PORT_INFO));
//	if(NULL == pPort)
//		return NULL;

	pPort->pHead = &(pCom->comPortListHead);
	list_add_tail(pPort->pHead, &(pPort->listNode));

	return true;
}

/*******************************************************************
 * ��ͨ�Ŷ˿��б���ɾ��һ���˿�
 *******************************************************************/
static void delMsgComPort(P_MSG_COM_PORT_INFO pPort) {

	if(NULL == pPort)
		return ;

	//����ö˿ڶ��ĵ�������Ϣ
	__clearPortSubscribe(pPort);
	list_del(&(pPort->listNode));

	pPort->msgNum = 0;
	pPort->enable = false;
	//����free,��Ϊ�˿ڳ�ʱҲ����øú���ɾ���������ʱ�Ѷ˿�free�����ⲿģ��ͨ��
	//bindMsgComPort��ȡ�Ķ˿�ָ�뽫���Ұָ�룡����
	//free��Ϊֻ�ܳ����ڱ���ʾ���õ�λ��
	//free(pPort);
}

/*******************************************************************
 * ����һ����Ϣͨ��ģ��
 * �ú�����Ϊ��ģ�������ڴ棬����������ͨ��ģ���ַָ�뷵��
 * �ڴ�����ͨ��ģ���ͬʱΪ������Ϣ����������ͨ��ģ����Ҫ��
 * Ӧ����Ϣ
 *******************************************************************/
P_MSG_COM newMsgCom(P_MSG_SERVER pServer) {

	P_MSG_COM pCom = (P_MSG_COM)malloc(sizeof(MSG_COM));
	if(NULL == pCom)
		return NULL;

	init_list_node(&(pCom->comPortListHead));
	pCom->pServer = pServer;
//	msgSubscribe(pCom->pServer, MSG_COM_HELLO, (T_MSG_CALLBACK_FUNC)__helloPktPro,
//			NULL, 0, false, true);
	int n = ARRAY_SIZE(msgComBindMapTab);
	for(int i = 0; i < n; i++) {
		msgSubscribe(pCom->pServer, msgComBindMapTab[i].msgType, (T_MSG_CALLBACK_FUNC)msgComBindMapTab[i].func,
				pCom, msgComBindMapTab[i].priority, false, msgComBindMapTab[i].isSyncMsg);
	}

	return pCom;
}

/*******************************************************************
 * ɾ��һ��ͨ��ģ��
 *******************************************************************/
void delMsgCom(P_MSG_COM pCom) {

	if(NULL == pCom)
		return ;

	//����ע��˿��б��ֱ�ɾ������ע��Ķ˿�
	P_MSG_COM_PORT_INFO pos = NULL;
	list_for_each_entry(pos, &pCom->comPortListHead, listNode) {
		unbindMsgComPort (pos);
	}

	//��������˶�����ͨ��ģ�鶩�ĵ���Ϣ�������˿ڶ��ĵ���Ϣ���˿ڶ��ĵ���Ϣ��unbindMsgComPort���˶���
	int n = ARRAY_SIZE(msgComBindMapTab);
	for(int i = 0; i < n; i++) {
		msgUnsubscribe(pCom->pServer, msgComBindMapTab[i].msgType, (T_MSG_CALLBACK_FUNC)msgComBindMapTab[i].func, pCom);
	}

	free(pCom);

	printf("msg com del !! \r\n");
}

/*******************************************************************
 * ��ͨ��ģ��󶨣�ע�ᣩһ���˿�
 * �ú������Զ�Ϊ��ע��˿�����һ����ʱ��ʱ��
 * �������õĳ�ʱʱ����û�дӸö˿��յ��κ���Ϣ����ɾ���ö˿�
 *******************************************************************/
bool bindMsgComPort(P_MSG_COM pCom, P_MSG_COM_PORT_INFO pPort,
		MSG_COM_SEND_FUNC sendFunc, void *pSendParam, int timeout) {

	if((NULL == pCom) || (NULL == pPort))
		return false;

	if(true == pPort->enable)
		return false;

	addMsgComPort(pCom, pPort);

	pPort->sendFunc = sendFunc;
	pPort->pSendParam = pSendParam;
	pPort->msgNum = 0;
	pPort->enable = true;

	//���øö˿ڵĳ�ʱ��ʱ�����øö˿ڵĶ˿���Ϣ��ڵ�ַ��pPort����Ϊ�ö�ʱ����timeId
	if(false == addTimerElement_s(pCom->pServer->timerHandle, timeout,
			(int)pPort, (TIMEOUT_PROC_FUNC)unbindMsgComPort, pPort, true)) {
		printf("bindMsgComPort warning : addTimerElement_s fail \r\n");
	}

	return pPort;
}

/*******************************************************************
 * ���һ���˿ڵİ�
 *******************************************************************/
bool unbindMsgComPort(P_MSG_COM_PORT_INFO pPort) {

	if(NULL == pPort)
		return false;

	//ͨ���ö˿ڵ���Ϣ����ȡ�ö˿�ע���ͨ��ģ��
	P_MSG_COM pCom = list_entry(pPort->pHead, MSG_COM, comPortListHead);
	//ɾ���ö˿ڵĳ�ʱ��ʱ��
	delTimerElement_s(pCom->pServer->timerHandle, (int)pPort);
	//��ͨ��ģ����ɾ���ö˿�
	delMsgComPort(pPort);
	printf("unbindMsgComPort port : 0x%x \r\n", (unsigned int)pPort);

	return true;
}

/*******************************************************************
 * ��ĳ���Ѿ�ע��Ķ˿ڴӶ˿��Ͻ��յ������ݣ�����øú����������յ���
 * ���ݷ��͸�ͨ��ģ����д���
 * �ú���Ӧ�ڶ˿ڵĽ��ջص������б����ã���ɶ˿���Ϣ���͹���
 * T_MSG_SN msgType���˿ڽ��յ�����Ϣ����
 * unsigned char *pMsg����Ϣʵ��ָ��
 * unsigned int msgLen����Ϣʵ�峤��
 * P_MSG_COM_PORT_INFO srcPort�����ݽ��ն˿ڵĶ˿���Ϣ��ڵ�ַ
 * P_MSG_COM_PORT_INFO desProt��ϣ������Ϣ�����͵��ĸ��˿ڣ���ϣ������
 * �����˸���Ϣ�Ķ˿ڶ��ܽ��յ�����Ϣ������NULL
 *******************************************************************/
void msgComPortRecvCallback(T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen, P_MSG_COM_PORT_INFO srcPort, P_MSG_COM_PORT_INFO desProt) {

	if(NULL == srcPort)
		return ;

	//�жϽ��ո����ݵĶ˿��Ƿ���ͨ��ģ������Ч
	if(false == srcPort->enable)
		return ;

	//��Ŀ�Ķ˿���Ϣ��ڵ�ַ��NULL�����ʾ����Ϣ�������ж˿�
	if(NULL == desProt)
		desProt = (P_MSG_COM_PORT_INFO)MSG_SERVER_ALL_SUBSCRIBE_PORT_NUM;

	//����Ϣ���������ʹӶ˿��յ�����Ϣ
	P_MSG_COM pCom = list_entry(srcPort->pHead, MSG_COM, comPortListHead);
	msgPushFull(pCom->pServer, msgType, pMsg, msgLen, (unsigned int)srcPort, (unsigned int)desProt);

	keepPortActive(srcPort);//���Ӹö˿��յ������ݣ��򱣳ָö˿�Ϊ��Ծ�˿�
}

/*******************************************************************
 * ��ʼ��һ���˿�
 * �ú���ֻ�Զ˿ڽ��г�ʼ������Ϊ������ڴ�
 * �˿���Ϣ���ڴ����Ӧ�ɸö˿ڵ�ʵ�������з���
 *
 * ��������Ŀ�ģ����˿ڷ�����ʱ������Ҫɾ���˿ڣ������ͨ��ģ�鸺��Ϊ
 * �˿���Ϣ�����ڴ棬��������������˵ҲӦ����ͨ��ģ�鸺���ͷ��ڴ档
 * ��ͨ��ģ���Լ����ڴ��ͷ��ˣ���û��֪ͨ�ⲿ�˿�ʵ��������ڴ����˿�ʱ
 * �ⲿ�˿�ʵ������õĶ˿���Ϣָ�뽫��ΪҰָ�룬������ڴ�����������
 * ��ǿ��Ҫ���ⲿ�˿�ʵ������ṩ�˿����ٵ���Ӧ�������¼���Ӧ��������
 * �����ⲿ�˿�ʵ�����ĸ��Ӷȡ�
 *
 * �ֽ׶ν��������ķ������˿���Ϣ���ڴ���估�ͷŶ����ⲿ�˿�ʵ�����
 * ����ͨ��ģ���ڶ˿ڳ�ʱ����Ӷ˿��б���ɾ���������ö˿���Ϣ��enable
 * �ֶ���λfalse���Ӷ���ʹ�ⲿ�ӿ���ȫ��֪���Լ��Ѿ���ɾ����Ҳ��������
 * ϵͳ�����󣬶�ͨ��ģ��ÿ����ʹ�øö˿���Ϣʱ�����ж���enableλ����Ϊ
 * false����Ӧ�ö˿�����
 *******************************************************************/
void msgComInitPort(P_MSG_COM_PORT_INFO pPort) {

	if(NULL == pPort)
		return ;

	init_list_node(&pPort->listNode);
	pPort->pHead = NULL;
	pPort->sendFunc = NULL;
	pPort->pSendParam = NULL;
	pPort->msgNum = 0;
	pPort->enable = false;
}

/*******************************************************************
 * ����hello����
 * �ú�������Ϣ���ķ���ģ���ʹ���߸��ݾ������������ʱ����
 *******************************************************************/
void msgSendHello (P_MSG_COM pCom) {

	//����һ��hello����
	static MSG_HELLO_PKT hello;
	unsigned int msgLen = __createHello(pCom->pServer, &hello);

	//�������Ѿ��󶨵Ķ˿ڷ��͸�hello����
	__msgPortSend(pCom, MSG_COM_HELLO, (unsigned char *)(&hello), msgLen);
}

/*******************************************************************
 * ˢ��һ���˿ڵļ���״̬
 * �ú����������뿴�Ź���ι��������ͬ
 *******************************************************************/
void keepPortActive(P_MSG_COM_PORT_INFO port) {

	P_MSG_COM pCom = list_entry(port->pHead, MSG_COM, comPortListHead);
	if(NULL == pCom)
		return ;

	//��ˢ�¸ö˿ڵĳ�ʱʱ��
	resetTimerElement_s(pCom->pServer->timerHandle, (int)port);
}







