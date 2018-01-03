/*
 * msgServer.h
 *
 *  Created on: 2017年8月27日
 *      Author: Gift
 */

#ifndef MSGSERVER_MSGSERVER_H_
#define MSGSERVER_MSGSERVER_H_

#include <stdbool.h>

#include "../../container/hashTable.h"
#include "../system.h"
#include "msgServerDef.h"

/* T_MSG_CALLBACK_FUNC返回true，则不需要其他模块再响应该消息，
 * 若返回false，则其他订阅该消息的模块可以继续处理该消息
 * 消息处理函数中，禁止引入延时操作，若该消息的处理操作中必须
 * 引入延时操作，则需将该消息的处理方式设置为异步处理，再其他任务中
 * 对该消息进行处理，目前异步处理操作由消息的订阅模块自行实现，
 * 后期可放入消息服务模块中统一管理 */
typedef bool (*T_MSG_CALLBACK_FUNC)(T_MSG_SN msgType,
		unsigned char *pMsg, unsigned int msgLen, ...);
		/*void *pProcParam, unsigned int recvPortNum, unsigned int desPortNum) ;*/

//消息映射关系，其最主要作用为将msg与其对应的处理回调函数进行绑定
typedef struct __msg_map {

	HASH_NODE hn;//该msg map信息的hash节点实体

	T_MSG_SN msg;//msg，同时为hash表中的key
	T_MSG_CALLBACK_FUNC pFunc;//该msg对应的处理回调函数
	void *pProcParam;//pFunc的处理参数，在调用pFunc时将该指针传入pFunc
	unsigned int priority; /* 值越小优先级越高 */
	bool isPublic;//该消息是否会通过通信模块向其他消息服务器订阅
	bool isSyncMsg;//是否为同步消息，若为异步消息，则msg server为其单独创建线程执行，每个异步消息都拥有一个独立线程
	unsigned int subscribePort;//订阅该消息的端口信息入口地址（由于这里不关心端口信息的具体内容，故使用unsigned int定义）

}MSG_MAP, *P_MSG_MAP;

typedef struct __msg_server {

	//消息服务器的消息列表，消息列表为一个链表式hash表
	HASH_HEAD ht[(1 << (MSG_SERVER_HASH_TABLE_SIZE_BIT))];
	unsigned int htSize;//hash表的大小（hash表数组的大小）

	HANDLE timerHandle;//该消息服务的定时器句柄

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
