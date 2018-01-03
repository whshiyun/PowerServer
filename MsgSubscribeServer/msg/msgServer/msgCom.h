/*
 * msgCom.h
 *
 *  Created on: 2017年8月28日
 *      Author: Gift
 */

#ifndef MSGSERVER_MSGCOM_H_
#define MSGSERVER_MSGCOM_H_
#include <stdbool.h>

#include "../../container/linkList.h"
#include "msgServer.h"
#include "msgServerDef.h"


typedef T_MSG_CALLBACK_FUNC MSG_COM_SEND_FUNC;//通信模块操作端口发送的函数类型定义
typedef T_MSG_CALLBACK_FUNC MSG_COM_RECV_FUNC;//通信模块操作端口接收的函数类型定义，暂时没用

//端口信息结构
typedef struct __msg_com_port_info {

	LIST_NODE listNode;//端口列表的链表节点实体
	LIST_NODE *pHead;//端口列表的链表头节点指针（可以通过该节点查找到该端口绑定的那个通信模块）

	bool enable;//端口信息有效性标志，若为false则表示该端口信息为无效信息
	MSG_COM_SEND_FUNC sendFunc;/*端口发送函数指针，该函数由消息服务器调用，
								该端口在订阅消息时将此函数注册给消息服务器，
								消息服务器每次收到对应的消息后将调用该函数进行消息发送*/
	void *pSendParam;/*发送参数，当消息服务器调用sendFunc函数时，给该函数传入的消息
	 	 	 	 	 	 端口发送函数往往为可重入设计，通过传入参数的不同实现不同物理接口发送，
	 	 	 	 	 	 例如网口，故这里一般需要有发送参数输入*/

	unsigned int msgNum;//msgTable的条目数
	T_MSG_SN msgTable[MSG_SERVER_MAX_SUBSCRIBE_NUM];//从该端口最近收到的hello报文里的内容，用来记录该端口总共订阅的服务

}MSG_COM_PORT_INFO, *P_MSG_COM_PORT_INFO;

typedef struct __msg_com {

	LIST_NODE comPortListHead;//端口列表的链表头节点

	P_MSG_SERVER pServer;//指向该与该通信模块挂接的消息服务器

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
