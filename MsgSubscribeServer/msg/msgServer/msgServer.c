/*
 * msgServer.c
 *
 *  Created on: 2017年8月27日
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

//异步执行参数结构，该结构包含处理消息响应所需的参数
//若消息需要异步执行，则为该消息创建独立线程执行，同时将消息响应的参数传入该线程
typedef struct __msg_asyn_param {
	T_MSG_CALLBACK_FUNC pFunc;//消息响应函数指针
	T_MSG_SN msgType;//消息类型
	unsigned char *pMsg;//消息实体内存地址
	unsigned int msgLen;//消息实体长度
	void *pFuncParam;//pFunc的处理参数
	unsigned int srcPortNum;//push该消息的端口信息入口地址
	unsigned int desPortNum;//期待处理该消息的端口信息入口地址
}MSG_ASYN_PARAM, *P_MSG_ASYN_PARAM;

//当前正在执行的异步消息个数，当结束该消息服务器时，需等待所有异步消息均已执行完毕才结束
//只有这样结束消息服务器才是安全的结束方式
static volatile unsigned int asynProcessCnt = 0;

/***************************************************************************
 * msg server response msg
 ***************************************************************************/

/*******************************************************************
 * 获取消息服务器当前状态
 * 暂未实现，可以根据今后的需要获取必要的消息服务器状态
 * 该功能主要为调试及消息服务器状态监控
 *******************************************************************/
static void msgServerGetInfo(P_MSG_SERVER pServer, P_MSG_SERVER_INFO pInfo) {

	//以下代码为非正式代码，仅为测试该功能使用
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
 * 处理一个接收到的消息服务器状态查询报文
 * 并将消息服务器状态回复给查询端口
 *******************************************************************/
static bool msgServerInfoRequestProc(T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen, P_MSG_SERVER pServer, unsigned int recvPort) {

	//获取消息服务器状态
	MSG_SERVER_INFO info;
	msgServerGetInfo(pServer, &info);

	//向查询端口推送状态报告
	msgPushWithDes(pServer, MSG_SERVER_INFO_REPLY, (unsigned char *)&info, sizeof(MSG_SERVER_INFO), recvPort);

	printf("msgServerInfoRequestProc \r\n");

	return true;
}
/***************************************************************************
 * end msg server response msg
 ***************************************************************************/

/*******************************************************************
 * 异步消息回调函数
 * 当消息需要实现异步响应时，为该消息创建独立的线程，该函数即为该线程的
 * 回调函数，并在该函数中对消息进行响应
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
 * 创建一个msg map关系，该函数将为新的映射关系信息申请内存
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
 * 删除一个msg map关系，该函数将释放该映射关系信息的内存
 *******************************************************************/
static void delMsgMap(P_MSG_MAP pMsgMap) {

	if(NULL == pMsgMap)
		return ;

	//从消息服务器的消息列表（hash表）中删除该消息
	hash_del(&(pMsgMap->hn));

	free(pMsgMap);
}

/*******************************************************************
 * 清空某个消息的所有绑定关系
 * 在msg server的消息列表中，msg的值为key，那么所有响应该消息的回调函数
 * 都会进入hash表的同一个hash值中，这里的hash表使用链表方式进行冲突处理，
 * 该函数为删除hash表中某个key下的所有元素
 *******************************************************************/
static void delMsgMapLink(HASH_HEAD *pHead) {

	P_MSG_MAP pos = NULL, n = NULL;
	hlist_for_each_entry_safe(pos, n, pHead, hn) {
		delMsgMap (pos);
	}
}

/*******************************************************************
 * 创建一个msg server，并未其分配内存
 *******************************************************************/
P_MSG_SERVER newMsgServer() {

	P_MSG_SERVER pServer = (P_MSG_SERVER)malloc(sizeof(MSG_SERVER));
	if(NULL == pServer)
		return NULL;

	//初始化消息列表（hash表）
	hash_init(pServer->ht);

	pServer->htSize = HASH_SIZE(pServer->ht);//计算hash表的实际大小（数组长度）
	pServer->timerHandle = newMsgTimer(pServer);//启动服务器定时器

	//msg server默认需要订阅消息服务器查询状态查询消息
	//查询式服务都要使用异步消息，以防同一个线程里面进行收发操作造成锁死
	msgSubscribe (pServer, MSG_SERVER_INFO_REQUEST, (T_MSG_CALLBACK_FUNC)msgServerInfoRequestProc,
			pServer, MSG_PRIORITY_CMD, false, false);

	return pServer;
}

/*******************************************************************
 * 删除消息服务器，并释放内存
 *******************************************************************/
int delMsgServer(P_MSG_SERVER pServer) {

	if(NULL == pServer)
		return 0;

	//等待所有异步消息都执行结束，否则直接结束是不安全的
	//今后这里可以扩展为超时式阻塞
	while(getWaitForProcAsynMsgCnt());

	//先删定时器
	delMsgTimer(pServer->timerHandle);

	//删除所有订阅的消息
	unsigned int i = 0;
	for(i=0; i<pServer->htSize; i++) {
		delMsgMapLink(&(pServer->ht[i]));
	}

	free(pServer);

	printf("msg server del !! \r\n");

	return 0;
}

/*******************************************************************
 * 向消息服务器订阅一个消息
 * P_MSG_SERVER pServer：订阅消息的服务器入口地址
 * T_MSG_SN msg：订阅的消息类型值
 * T_MSG_CALLBACK_FUNC pFunc：服务器推送该消息时的回调函数
 * void *pProcParam：pFunc的传入参数
 * unsigned int priority：该消息的优先级
 * bool isPublic：该订阅信息是否会被路由到其他消息服务器上，即：是否自动向其他服务器订阅该消息
 * bool isSyncMsg：是否为同步消息，若为异步消息，则在执行时会为其创建独立的线程进行执行，每个异步消息均拥有一个独立线程
 * unsigned int subscribePort：一般为消息的订阅端口的端口信息入口地址，这里由于不关心端口信息内容，故使用unsigned int来定义
 *******************************************************************/
bool msgSubscribeFull (P_MSG_SERVER pServer, T_MSG_SN msg,
		T_MSG_CALLBACK_FUNC pFunc, void *pProcParam, unsigned int priority,
		bool isPublic, bool isSyncMsg, unsigned int subscribePort) {

	//创建一个新的msg map
	P_MSG_MAP pMap = newMsgMap(msg, pFunc, pProcParam, priority, isPublic, isSyncMsg, subscribePort);
	if(NULL == pMap)
		return false;

	//获取该消息对应在hash表中的位置
	HASH_HEAD *pHead = hash_head_get(pServer->ht, msg);
	if((pHead > &(pServer->ht[pServer->htSize - 1])) || (pHead < &(pServer->ht[0]))) {
		delMsgMap(pMap);
		return false;
	}

	//遍历所有已经订阅过该消息的msg map
	P_MSG_MAP pos = NULL;
	bool insertFlag = false;
	hlist_for_each_entry(pos, pHead, hn) {
		//查看该消息的响应函数及参数是否已经订阅过该消息，若订阅过则不重复订阅
		//这里认为若消息的响函数及函数处理参数相同，则为同一个订阅意图，故不进行重新订阅
		if((pFunc == pos->pFunc) && (pProcParam == pos->pProcParam)) {
			delMsgMap(pMap);
			insertFlag = true;
			break;
		}

		//按消息优先级进行排序，排序过程在订阅时完成，每次消息响应时就不用再进行排序
		if(pos->priority > pMap->priority) {
			hash_add_before((HASH_NODE *)(&(pos->hn)), (HASH_NODE *)(&(pMap->hn)));
			insertFlag = true;
			break;
		}
	}

	//若没有找到比自己优先级值大的订阅，则认为自己为优先级值最大的那条订阅
	//则将自己插入链表的末尾（优先级值越大表示优先级越低）
	if(false == insertFlag) {
		hash_add_tail(pHead, &(pMap->hn));
		insertFlag = true;
	}

	return insertFlag;
}

/*******************************************************************
 * 使用本地端口订阅消息
 * 若调用该函数进行订阅，则认为订阅此消息的模块为一个本地模块
 *******************************************************************/
bool msgSubscribe (P_MSG_SERVER pServer, T_MSG_SN msg, T_MSG_CALLBACK_FUNC pFunc,
		void *pProcParam, unsigned int priority, bool isPublic, bool isSyncMsg) {

	return msgSubscribeFull(pServer, msg, pFunc, pProcParam,
			priority, isPublic, isSyncMsg, MSG_SERVER_LOCAL_SUBSCRIBE_PORT_NUM);
}

/*******************************************************************
 * 退订一条订阅
 * T_MSG_CALLBACK_FUNC pFunc：之前订阅该消息时使用的消息回调函数入口地址
 * void *pProcParam：之前订阅该消息时使用的函数参数
 * 这里认为若消息的响函数及函数处理参数相同，则为同一个订阅，即：上述两个
 * 参数唯一指定一个订阅
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
 * 向消息服务器推送一条消息
 * 若模块产生了一条消息需要进行推送，则调用该函数。
 * P_MSG_SERVER pServer：需要推送消息的服务器入口地址
 * T_MSG_SN msg：推送消息的类型
 * unsigned char *pMsgSubstance：推送消息实体的内存指针
 * unsigned int msgLen：推送消息实体的长度
 * unsigned int srcPortNum：该消息是哪个端口推送的，该字段为端口的唯一标识（一般为该端口的端口信息入口地址）
 * unsigned int desPortNum：该消息希望哪个端口处理，该字段为端口的唯一标识（一般为该端口的端口信息入口地址），
 * 该字段若为NULL，则表示所有端口都进行处理
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
 * 向服务器推送一条消息，仅指定目的端口，源端口为本地端口号
 *******************************************************************/
bool msgPushWithDes(P_MSG_SERVER pServer, T_MSG_SN msg,
		unsigned char *pMsgSubstance, unsigned int msgLen, unsigned int desPortNum) {

	return msgPushFull(pServer, msg, pMsgSubstance,
			msgLen, MSG_SERVER_LOCAL_SUBSCRIBE_PORT_NUM, desPortNum);
}

/*******************************************************************
 * 向服务器推送一条消息，仅指定源端口，目的端口为所有端口
 *******************************************************************/
bool msgPushWithSrc(P_MSG_SERVER pServer, T_MSG_SN msg,
		unsigned char *pMsgSubstance, unsigned int msgLen, unsigned int srcPortNum) {

	return msgPushFull(pServer, msg, pMsgSubstance,
			msgLen, srcPortNum, MSG_SERVER_ALL_SUBSCRIBE_PORT_NUM);
}

/*******************************************************************
 * 向服务器推送一条消息，源端口为本地端口，目的端口为所有端口
 *******************************************************************/
bool msgPush(P_MSG_SERVER pServer, T_MSG_SN msg,
		unsigned char *pMsgSubstance, unsigned int msgLen) {

	return msgPushWithDes(pServer, msg, pMsgSubstance,
			msgLen, MSG_SERVER_ALL_SUBSCRIBE_PORT_NUM);
}

/*******************************************************************
 * 获取当前正在执行的异步消息个数
 *******************************************************************/
unsigned int getWaitForProcAsynMsgCnt() {

	return asynProcessCnt;
}
















