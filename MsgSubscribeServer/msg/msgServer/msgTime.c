/***********************************************
 *  /MsgSubscribeServer/msg/msgServer/msgTime.c
 *
 *  Created on: 2017年11月29日
 *  Author: gift
 *
 ***********************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "msgTimeDef.h"
#include "msgTime.h"
#include "../msgDef.h"

#define SECOND_WAIT			(1000)

typedef struct __timerInfo {

	int timeout_s;//设置的超时时间，单位：秒
	unsigned int inactivityTime_s;//已经有多久未被激活过，单位：秒

}TIMER_INFO, *P_TIMER_INFO;

typedef struct __msg_timer {

	LIST_NODE elementHead_s;//秒定时器元素列表

	P_MSG_SERVER pServer;//消息服务器入口地址
	HANDLE task;//目前为秒计时线程，今后若需扩展可将该处扩展为数组或链表，使其能容纳多个线程
				//或者将timer模块进行修改，在new此模块时传入参数，设置该timer模块的计时颗粒度（推荐使用该方法扩展）
	bool enable;//使能该定时器
	bool destroy;//是否需要销毁该定时器

}MSG_TIMER, *P_MSG_TIMER;

typedef struct __timer_element {

	LIST_NODE node;

	P_MSG_TIMER pTimer;
	TIMER_INFO timerInfo;//见struct __timerInfo
	TIMEOUT_PROC_FUNC timeoutProc;//定时器元素超时后的回调处理，可以为NULL
	void *pProcParam;//传递给timeoutProc的参数

	int timeId;//该定时器元素的唯一表示符，供外部模块识别该定时器元素是哪一个定时操作
	bool isSendTimeoutMsg;//在超时后是否想服务器推送该定时器元素超时的消息

}TIMER_ELEMENT, *P_TIMER_ELEMENT;

static bool isTimeOut(TIMER_INFO info) {

	if(TIMER_TIME_OUT_NEVER == info.timeout_s)
		return false;

	if(0 > info.timeout_s) {
		printf("isTimeOut warning : info.timeout_s with a unknow value : %d \r\n", info.timeout_s);
		return true;
	}

	//程序执行到这里，需要确保info.timeout_s为一个正数
	//否则无符号数与有符号数做比较时会有问题
	if(info.inactivityTime_s < (unsigned int)info.timeout_s)
		return false;

	return true;
}

/***********************************************
 * 删除一个定时器元素
 * 从定时器列表中将该元素删除，并释放其内存
 ***********************************************/
static bool delTimerElement(P_TIMER_ELEMENT pElement) {

	if(NULL == pElement)
		return false;

	list_del(&pElement->node);
	free(pElement);

	return true;
}

/***********************************************
 * 重置一个定时器元素，使其重新开始计时
 ***********************************************/
static void resetTimerElement(P_TIMER_ELEMENT pElement) {

	if(NULL == pElement)
		return ;

	pElement->timerInfo.inactivityTime_s = 0;
}

/***********************************************
 * 若发现某定时器元素超时后，对超时的定时器进行处理
 * 首先调用设置定时器时外部提供的超时处理方法
 * 然后判断超时是否需要发送超时消息
 * 若需要，则想服务器发送该定时器超时的消息
 ***********************************************/
static void timeOutProc(P_TIMER_ELEMENT pElement) {

	if(NULL == pElement)
		return ;

	if(true == pElement->isSendTimeoutMsg) {
		//将超时的定时器timeId发送出去
		msgPush(pElement->pTimer->pServer, MSG_TIMER_TIME_TIMEOUT,
				(unsigned char *)&pElement->timeId, sizeof(pElement->timeId));
	}

	if(NULL != pElement->timeoutProc) {
		pElement->timeoutProc(pElement->pProcParam);
	}
}

/***********************************************
 * 秒定时器线程执行函数
 * 该函数在需在独立线程中执行，每间隔1秒钟
 * 向消息服务器发送一条MSG_TIMER_TIME_TRIGGER_SECOND消息
 *
 * 该函数发送MSG_TIMER_TIME_TRIGGER_SECOND消息的时间间隔
 * 不一定是准确的1秒钟，会由于线程调度的资源开销导致时间偏差
 * 故仅用于对时间精确度要求不是特别高的地方使用
 ***********************************************/
static void timer_s(HANDLE handle) {

	if(NULL == handle)
		goto endTimer_s;

	P_MSG_TIMER pTimer = (P_MSG_TIMER)handle;
	if(NULL == pTimer->pServer)
		goto endTimer_s;

	//如果timer被销毁，则退出线程
	while(!pTimer->destroy){
		MsgDelay_ms(SECOND_WAIT);
		//若该timer被使能，则推送消息，否则停止推送，达到定时器暂停的效果
		if(true == pTimer->enable)
			msgPush(pTimer->pServer, MSG_TIMER_TIME_TRIGGER_SECOND, NULL, 0);
	}

endTimer_s :
	if(NULL != handle)
		free(handle);

	return ;
}

/***********************************************
 * timerMsgProc_s函数为timer模块自己响应
 * MSG_TIMER_TIME_TRIGGER_SECOND消息
 * 的回调函数，每隔一秒钟timer模块检查自己所有的
 * timer_element是否有超时，同时将timer_element中的
 * timerInfo.inactivityTime_s 进行增加
 *
 * 在判断超时的时候，由于每个定时器的
 * pos->timerInfo.inactivityTime_s是随时变化的
 * 所以如果使用排序机制，
 * 只判断第一个定时器是否超时这种方法反而得不偿失
 * 这点跟linux里面的定时器机制不同
 ***********************************************/
static bool timerMsgProc_s(T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen, void *pParam) {

	if((MSG_TIMER_TIME_TRIGGER_SECOND != msgType) || (NULL == pParam))
		return false;

	//不能要求外部模块不在timeOutProc(pos);中调用
	//delTimerElement(pos)，所以这里要用list_for_each_entry_safe
	P_MSG_TIMER pTimer = pParam;
	P_TIMER_ELEMENT pos = NULL, n = NULL;
	list_for_each_entry_safe(pos, n, &pTimer->elementHead_s, node) {
		pos->timerInfo.inactivityTime_s ++;
		if(true == isTimeOut(pos->timerInfo)) {
			timeOutProc(pos);
			/* 超时了也不自己删除定时器，将超时的消息通知出去，让设置该定时器的模块自己删除 */
			//delTimerElement(pos);//先进行超时处理，处理完后直接将定时器删除
		}
	}

	return false;
}

/***********************************************
 * 创建一个timer模块
 * 在该函数中会为一个timer分配内存，同时开启timer定时
 * 消息触发线程，例如秒定时，则会开启秒定时线程（timer_s（）函数）
 * 使timer模块周期向消息服务器推送定时器消息
 * 除此以外，若timer模块需要处理定时消息，
 * 则还需在此函数中订阅自己周期推送的定时消息，例如：
 * MSG_TIMER_TIME_TRIGGER_SECOND消息的订阅
 *
 * 目前暂时实现秒级定时，其他颗粒度定时若需增加，
 * 则一律在此处进行线程开启及消息订阅
 ***********************************************/
HANDLE newMsgTimer(P_MSG_SERVER pServer) {

	if(NULL == pServer) {
		printf("newMsgTime error : NULL == pServer \r\n");
		return NULL;
	}

	P_MSG_TIMER pTimer = (P_MSG_TIMER)malloc(sizeof(MSG_TIMER));
	init_list_node(&pTimer->elementHead_s);
	pTimer->pServer = pServer;
	pTimer->enable = true;
	pTimer->destroy = false;
	if(NULL == (pTimer->task = MsgCreateTask(0, timer_s, pTimer))) {
		goto newMsgTimeError;
	}

	//订阅秒消息响应
	msgSubscribe(pServer, MSG_TIMER_TIME_TRIGGER_SECOND, (T_MSG_CALLBACK_FUNC)timerMsgProc_s,
			pTimer, MSG_PRIORITY_CMD + 1, false, true);

	return pTimer;

newMsgTimeError :
	if(NULL != pTimer)
		free(pTimer);

	return NULL;
}

/***********************************************
 * 删除timer
 ***********************************************/
void delMsgTimer(HANDLE handle) {

	if(NULL == handle)
		return ;

	P_MSG_TIMER pTimer = (P_MSG_TIMER)handle;

	//删除订阅的秒消息响应
	msgUnsubscribe(pTimer->pServer, MSG_TIMER_TIME_TRIGGER_SECOND, (T_MSG_CALLBACK_FUNC)timerMsgProc_s, pTimer);

	//删除设置的定时器元素
	P_TIMER_ELEMENT pos = NULL, n = NULL;
	list_for_each_entry_safe(pos, n, &pTimer->elementHead_s, node) {
		delTimerElement(pos);
	}

	//如果存在定时线程，则在线程中销毁pTimer，释放内存
	//否则，直接释放timer的内存
	if(NULL == pTimer->task) {
		free(pTimer);
	} else {
		pTimer->destroy = true;
		if(WAIT_OBJECT_0 != WaitForSingleObject(pTimer->task, 2 * SECOND_WAIT)) {
			printf("delMsgTimer fail !!!\r\n");
		}
	}
}

/***********************************************
 * 向timer中添加一个定时器元素
 * time：定时器超时时间，以秒为单位
 * timeId：该定时器元素的唯一表示符，供外部模块识别
 * 该定时器元素是哪一个定时操作
 * func：定时器元素超时后的回调处理，可以为NULL
 * funcParam：传递给func的参数
 * isSendTimeoutMsg：在超时后是否想服务器推送该定时器
 * 元素超时的消息
 *
 * 该函数会为element申请内存，该内存由delTimerElement（）
 * 函数释放
 ***********************************************/
bool addTimerElement_s(HANDLE handle, int time, int timeId, TIMEOUT_PROC_FUNC func,
		void *funcParam, bool isSendTimeoutMsg) {

	P_MSG_TIMER pTimer = (P_MSG_TIMER)handle;
	if(NULL == pTimer)
		return false;

	P_TIMER_ELEMENT pElement = (P_TIMER_ELEMENT)malloc(sizeof(TIMER_ELEMENT));
	if(NULL == pElement)
		return false;

	init_list_node(&pElement->node);
	pElement->pTimer = pTimer;
	pElement->timeoutProc = func;
	pElement->pProcParam = funcParam;
	pElement->timerInfo.inactivityTime_s = 0;
	pElement->timerInfo.timeout_s = time;
	pElement->timeId = timeId;
	pElement->isSendTimeoutMsg = isSendTimeoutMsg;

	list_add(&pTimer->elementHead_s, &pElement->node);

	return true;
}

/***********************************************
 * 删除指定的秒级定时器元素
 * 会释放该定时器元素的内存，但不会将定时器元素
 * 被删除的操作通知给其他外部模块
 ***********************************************/
bool delTimerElement_s(HANDLE handle, int timeId) {

	P_MSG_TIMER pTimer = (P_MSG_TIMER)handle;
	if(NULL == pTimer)
		return false;

	int ret = 0;

	P_TIMER_ELEMENT pos = NULL, n = NULL;
	list_for_each_entry_safe(pos, n, &pTimer->elementHead_s, node) {
		if(timeId == pos->timeId) {
			if(false == delTimerElement(pos))
				ret ++;
			break;
		}
	}

	if(0 != ret)
		return false;
	return true;
}

/***********************************************
 * 重置一个秒定时器元素，使其重新开始计时
 ***********************************************/
void resetTimerElement_s(HANDLE handle, int timeId) {

	P_MSG_TIMER pTimer = (P_MSG_TIMER)handle;
	if(NULL == pTimer)
		return ;

	P_TIMER_ELEMENT pos = NULL;
	list_for_each_entry(pos, &pTimer->elementHead_s, node) {
		if(timeId == pos->timeId) {
			resetTimerElement(pos);
			break;
		}
	}
}


