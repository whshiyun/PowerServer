/*
 * msgCom.c
 *
 *  Created on: 2017年8月28日
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
 * 还有个遗留问题，消息服务器向某port推送消息时，该接口超时计时器没有跟随刷新
 * 该问题暂时由外部端口的回调函数解决，每次消息服务器推送消息到外部端口后，
 * 外部端口的回调服务函数中需刷新超时计时器
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

//com默认订阅的消息
static const struct msg_bind_map msgComBindMapTab[] = {
		{MSG_COM_HELLO, (T_MSG_CALLBACK_FUNC)__helloPktPro, true, 0x10},
		{MSG_COM_DETECT, (T_MSG_CALLBACK_FUNC)__detectPktPro, false, 0x10},
		{MSG_COM_DETECT_REPLY, (T_MSG_CALLBACK_FUNC)__detectReplyPktPro, true, 0x10},
};

/*******************************************************************
 * 获取本地COM的唯一地址，暂时可以考虑用网口Mac地址
 *******************************************************************/
static void __getLocalId(unsigned char *pId, unsigned int idLen) {

}

/*******************************************************************
 * 清空port上所订阅的所有消息
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
 * 处理一个接收到的hello报文
 * 函数内为hello报文中的所有消息在本服务器进行订阅
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

	//先清空所有之前的订阅，保证每次刷新
	__clearPortSubscribe(pPort);

	//根据最新的要求进行订阅
	unsigned int i = 0;
	for(i=0; i<pHello->msgNum; i++) {
		msgSubscribeFull(pCom->pServer, pHello->msgTable[i],
				(T_MSG_CALLBACK_FUNC)pPort->sendFunc, pPort->pSendParam,
				0, false, true, (unsigned int)pPort);
	}

	//在port info中存储最新的订阅信息
	pPort->msgNum = pHello->msgNum;
	for(i=0; i<pPort->msgNum; i++) {
		pPort->msgTable[i] = pHello->msgTable[i];
	}

	return true;
}

/*******************************************************************
 * 处理一个服务器探测消息
 * 若收到服务器探测消息，则立马返回一个应答报文
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
 * 处理探测应答报文
 * 由于目前服务器不会主动发送探测报文，所以该报文具体行为暂未实现
 *******************************************************************/
static bool __detectReplyPktPro(T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen, void *pProcParam, unsigned int recvPort) {

	printf("__detectReplyPktPro");

	///暂未实现

	return true;
}

/*******************************************************************
 * 创建一个hello报文
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
 * 向pCom中所有注册过的端口发送消息
 *******************************************************************/
static void __msgPortSend(P_MSG_COM pCom, T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen) {

	P_MSG_COM_PORT_INFO pos = NULL;
	list_for_each_entry(pos, &(pCom->comPortListHead), listNode) {
		pos->sendFunc(msgType, pMsg, msgLen, pos->pSendParam);
	}
}

/*******************************************************************
 * 向通信端口列表中添加一个端口
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
 * 在通信端口列表中删除一个端口
 *******************************************************************/
static void delMsgComPort(P_MSG_COM_PORT_INFO pPort) {

	if(NULL == pPort)
		return ;

	//清除该端口订阅的所有消息
	__clearPortSubscribe(pPort);
	list_del(&(pPort->listNode));

	pPort->msgNum = 0;
	pPort->enable = false;
	//不能free,因为端口超时也会调用该函数删除，如果这时把端口free掉，外部模块通过
	//bindMsgComPort获取的端口指针将变成野指针！！！
	//free行为只能出现在被显示调用的位置
	//free(pPort);
}

/*******************************************************************
 * 创建一个消息通信模块
 * 该函数会为该模块申请内存，并将创建的通信模块地址指针返回
 * 在创建该通信模块的同时为其向消息服务器订阅通信模块需要响
 * 应的消息
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
 * 删除一个通信模块
 *******************************************************************/
void delMsgCom(P_MSG_COM pCom) {

	if(NULL == pCom)
		return ;

	//遍历注册端口列表，分别删除所有注册的端口
	P_MSG_COM_PORT_INFO pos = NULL;
	list_for_each_entry(pos, &pCom->comPortListHead, listNode) {
		unbindMsgComPort (pos);
	}

	//向服务器退订所有通信模块订阅的消息（不含端口订阅的消息，端口订阅的消息在unbindMsgComPort中退订）
	int n = ARRAY_SIZE(msgComBindMapTab);
	for(int i = 0; i < n; i++) {
		msgUnsubscribe(pCom->pServer, msgComBindMapTab[i].msgType, (T_MSG_CALLBACK_FUNC)msgComBindMapTab[i].func, pCom);
	}

	free(pCom);

	printf("msg com del !! \r\n");
}

/*******************************************************************
 * 向通信模块绑定（注册）一个端口
 * 该函数将自动为该注册端口设置一个超时定时器
 * 若在设置的超时时长内没有从该端口收到任何消息，则删除该端口
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

	//设置该端口的超时定时器，用该端口的端口信息入口地址（pPort）作为该定时器的timeId
	if(false == addTimerElement_s(pCom->pServer->timerHandle, timeout,
			(int)pPort, (TIMEOUT_PROC_FUNC)unbindMsgComPort, pPort, true)) {
		printf("bindMsgComPort warning : addTimerElement_s fail \r\n");
	}

	return pPort;
}

/*******************************************************************
 * 解除一个端口的绑定
 *******************************************************************/
bool unbindMsgComPort(P_MSG_COM_PORT_INFO pPort) {

	if(NULL == pPort)
		return false;

	//通过该端口的信息，获取该端口注册的通信模块
	P_MSG_COM pCom = list_entry(pPort->pHead, MSG_COM, comPortListHead);
	//删除该端口的超时定时器
	delTimerElement_s(pCom->pServer->timerHandle, (int)pPort);
	//从通信模块中删除该端口
	delMsgComPort(pPort);
	printf("unbindMsgComPort port : 0x%x \r\n", (unsigned int)pPort);

	return true;
}

/*******************************************************************
 * 若某个已经注册的端口从端口上接收到了数据，则调用该函数，将接收到的
 * 数据发送给通信模块进行处理。
 * 该函数应在端口的接收回调函数中被调用，完成端口消息推送功能
 * T_MSG_SN msgType：端口接收到的消息类型
 * unsigned char *pMsg：消息实体指针
 * unsigned int msgLen：消息实体长度
 * P_MSG_COM_PORT_INFO srcPort：数据接收端口的端口信息入口地址
 * P_MSG_COM_PORT_INFO desProt：希望该消息被推送到哪个端口，若希望所有
 * 订阅了该消息的端口都能接收到该消息，则填NULL
 *******************************************************************/
void msgComPortRecvCallback(T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen, P_MSG_COM_PORT_INFO srcPort, P_MSG_COM_PORT_INFO desProt) {

	if(NULL == srcPort)
		return ;

	//判断接收该数据的端口是否在通信模块内有效
	if(false == srcPort->enable)
		return ;

	//若目的端口信息入口地址填NULL，则表示该消息发给所有端口
	if(NULL == desProt)
		desProt = (P_MSG_COM_PORT_INFO)MSG_SERVER_ALL_SUBSCRIBE_PORT_NUM;

	//向消息服务器推送从端口收到的消息
	P_MSG_COM pCom = list_entry(srcPort->pHead, MSG_COM, comPortListHead);
	msgPushFull(pCom->pServer, msgType, pMsg, msgLen, (unsigned int)srcPort, (unsigned int)desProt);

	keepPortActive(srcPort);//若从该端口收到了数据，则保持该端口为活跃端口
}

/*******************************************************************
 * 初始化一个端口
 * 该函数只对端口进行初始化，不为其分配内存
 * 端口信息的内存分配应由该端口的实体程序进行分配
 *
 * 这样做的目的：若端口发生超时，则需要删除端口，如果是通信模块负责为
 * 端口信息分配内存，则从设计理念上来说也应该由通信模块负责释放内存。
 * 若通信模块自己将内存释放了，而没有通知外部端口实体程序，则在创建端口时
 * 外部端口实体程序获得的端口信息指针将成为野指针，造成了内存错误的隐患。
 * 若强行要求外部端口实体程序提供端口销毁的响应函数或事件响应函数，将
 * 增加外部端口实体程序的复杂度。
 *
 * 现阶段解决该问题的方法：端口信息的内存分配及释放都由外部端口实体程序
 * 管理，通信模块在端口超时后将其从端口列表中删除，并将该端口信息的enable
 * 字段置位false，从而即使外部接口完全不知道自己已经被删除，也不会引起
 * 系统级错误，而通信模块每次在使用该端口信息时首先判断其enable位，若为
 * false则不响应该端口请求。
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
 * 发送hello报文
 * 该函数由消息订阅服务模块的使用者根据具体需求决定何时调用
 *******************************************************************/
void msgSendHello (P_MSG_COM pCom) {

	//创建一条hello报文
	static MSG_HELLO_PKT hello;
	unsigned int msgLen = __createHello(pCom->pServer, &hello);

	//向所有已经绑定的端口发送该hello报文
	__msgPortSend(pCom, MSG_COM_HELLO, (unsigned char *)(&hello), msgLen);
}

/*******************************************************************
 * 刷新一个端口的激活状态
 * 该函数的作用与看门狗的喂狗操作相同
 *******************************************************************/
void keepPortActive(P_MSG_COM_PORT_INFO port) {

	P_MSG_COM pCom = list_entry(port->pHead, MSG_COM, comPortListHead);
	if(NULL == pCom)
		return ;

	//则刷新该端口的超时时间
	resetTimerElement_s(pCom->pServer->timerHandle, (int)port);
}







