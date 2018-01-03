/***********************************************
 *  /MsgSubscribeServer/msg/msgServer/msgTime.c
 *
 *  Created on: 2017��11��29��
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

	int timeout_s;//���õĳ�ʱʱ�䣬��λ����
	unsigned int inactivityTime_s;//�Ѿ��ж��δ�����������λ����

}TIMER_INFO, *P_TIMER_INFO;

typedef struct __msg_timer {

	LIST_NODE elementHead_s;//�붨ʱ��Ԫ���б�

	P_MSG_SERVER pServer;//��Ϣ��������ڵ�ַ
	HANDLE task;//ĿǰΪ���ʱ�̣߳����������չ�ɽ��ô���չΪ���������ʹ�������ɶ���߳�
				//���߽�timerģ������޸ģ���new��ģ��ʱ������������ø�timerģ��ļ�ʱ�����ȣ��Ƽ�ʹ�ø÷�����չ��
	bool enable;//ʹ�ܸö�ʱ��
	bool destroy;//�Ƿ���Ҫ���ٸö�ʱ��

}MSG_TIMER, *P_MSG_TIMER;

typedef struct __timer_element {

	LIST_NODE node;

	P_MSG_TIMER pTimer;
	TIMER_INFO timerInfo;//��struct __timerInfo
	TIMEOUT_PROC_FUNC timeoutProc;//��ʱ��Ԫ�س�ʱ��Ļص���������ΪNULL
	void *pProcParam;//���ݸ�timeoutProc�Ĳ���

	int timeId;//�ö�ʱ��Ԫ�ص�Ψһ��ʾ�������ⲿģ��ʶ��ö�ʱ��Ԫ������һ����ʱ����
	bool isSendTimeoutMsg;//�ڳ�ʱ���Ƿ�����������͸ö�ʱ��Ԫ�س�ʱ����Ϣ

}TIMER_ELEMENT, *P_TIMER_ELEMENT;

static bool isTimeOut(TIMER_INFO info) {

	if(TIMER_TIME_OUT_NEVER == info.timeout_s)
		return false;

	if(0 > info.timeout_s) {
		printf("isTimeOut warning : info.timeout_s with a unknow value : %d \r\n", info.timeout_s);
		return true;
	}

	//����ִ�е������Ҫȷ��info.timeout_sΪһ������
	//�����޷��������з��������Ƚ�ʱ��������
	if(info.inactivityTime_s < (unsigned int)info.timeout_s)
		return false;

	return true;
}

/***********************************************
 * ɾ��һ����ʱ��Ԫ��
 * �Ӷ�ʱ���б��н���Ԫ��ɾ�������ͷ����ڴ�
 ***********************************************/
static bool delTimerElement(P_TIMER_ELEMENT pElement) {

	if(NULL == pElement)
		return false;

	list_del(&pElement->node);
	free(pElement);

	return true;
}

/***********************************************
 * ����һ����ʱ��Ԫ�أ�ʹ�����¿�ʼ��ʱ
 ***********************************************/
static void resetTimerElement(P_TIMER_ELEMENT pElement) {

	if(NULL == pElement)
		return ;

	pElement->timerInfo.inactivityTime_s = 0;
}

/***********************************************
 * ������ĳ��ʱ��Ԫ�س�ʱ�󣬶Գ�ʱ�Ķ�ʱ�����д���
 * ���ȵ������ö�ʱ��ʱ�ⲿ�ṩ�ĳ�ʱ������
 * Ȼ���жϳ�ʱ�Ƿ���Ҫ���ͳ�ʱ��Ϣ
 * ����Ҫ��������������͸ö�ʱ����ʱ����Ϣ
 ***********************************************/
static void timeOutProc(P_TIMER_ELEMENT pElement) {

	if(NULL == pElement)
		return ;

	if(true == pElement->isSendTimeoutMsg) {
		//����ʱ�Ķ�ʱ��timeId���ͳ�ȥ
		msgPush(pElement->pTimer->pServer, MSG_TIMER_TIME_TIMEOUT,
				(unsigned char *)&pElement->timeId, sizeof(pElement->timeId));
	}

	if(NULL != pElement->timeoutProc) {
		pElement->timeoutProc(pElement->pProcParam);
	}
}

/***********************************************
 * �붨ʱ���߳�ִ�к���
 * �ú��������ڶ����߳���ִ�У�ÿ���1����
 * ����Ϣ����������һ��MSG_TIMER_TIME_TRIGGER_SECOND��Ϣ
 *
 * �ú�������MSG_TIMER_TIME_TRIGGER_SECOND��Ϣ��ʱ����
 * ��һ����׼ȷ��1���ӣ��������̵߳��ȵ���Դ��������ʱ��ƫ��
 * �ʽ����ڶ�ʱ�侫ȷ��Ҫ�����ر�ߵĵط�ʹ��
 ***********************************************/
static void timer_s(HANDLE handle) {

	if(NULL == handle)
		goto endTimer_s;

	P_MSG_TIMER pTimer = (P_MSG_TIMER)handle;
	if(NULL == pTimer->pServer)
		goto endTimer_s;

	//���timer�����٣����˳��߳�
	while(!pTimer->destroy){
		MsgDelay_ms(SECOND_WAIT);
		//����timer��ʹ�ܣ���������Ϣ������ֹͣ���ͣ��ﵽ��ʱ����ͣ��Ч��
		if(true == pTimer->enable)
			msgPush(pTimer->pServer, MSG_TIMER_TIME_TRIGGER_SECOND, NULL, 0);
	}

endTimer_s :
	if(NULL != handle)
		free(handle);

	return ;
}

/***********************************************
 * timerMsgProc_s����Ϊtimerģ���Լ���Ӧ
 * MSG_TIMER_TIME_TRIGGER_SECOND��Ϣ
 * �Ļص�������ÿ��һ����timerģ�����Լ����е�
 * timer_element�Ƿ��г�ʱ��ͬʱ��timer_element�е�
 * timerInfo.inactivityTime_s ��������
 *
 * ���жϳ�ʱ��ʱ������ÿ����ʱ����
 * pos->timerInfo.inactivityTime_s����ʱ�仯��
 * �������ʹ��������ƣ�
 * ֻ�жϵ�һ����ʱ���Ƿ�ʱ���ַ��������ò���ʧ
 * ����linux����Ķ�ʱ�����Ʋ�ͬ
 ***********************************************/
static bool timerMsgProc_s(T_MSG_SN msgType, unsigned char *pMsg,
		unsigned int msgLen, void *pParam) {

	if((MSG_TIMER_TIME_TRIGGER_SECOND != msgType) || (NULL == pParam))
		return false;

	//����Ҫ���ⲿģ�鲻��timeOutProc(pos);�е���
	//delTimerElement(pos)����������Ҫ��list_for_each_entry_safe
	P_MSG_TIMER pTimer = pParam;
	P_TIMER_ELEMENT pos = NULL, n = NULL;
	list_for_each_entry_safe(pos, n, &pTimer->elementHead_s, node) {
		pos->timerInfo.inactivityTime_s ++;
		if(true == isTimeOut(pos->timerInfo)) {
			timeOutProc(pos);
			/* ��ʱ��Ҳ���Լ�ɾ����ʱ��������ʱ����Ϣ֪ͨ��ȥ�������øö�ʱ����ģ���Լ�ɾ�� */
			//delTimerElement(pos);//�Ƚ��г�ʱ�����������ֱ�ӽ���ʱ��ɾ��
		}
	}

	return false;
}

/***********************************************
 * ����һ��timerģ��
 * �ڸú����л�Ϊһ��timer�����ڴ棬ͬʱ����timer��ʱ
 * ��Ϣ�����̣߳������붨ʱ����Ὺ���붨ʱ�̣߳�timer_s����������
 * ʹtimerģ����������Ϣ���������Ͷ�ʱ����Ϣ
 * �������⣬��timerģ����Ҫ����ʱ��Ϣ��
 * �����ڴ˺����ж����Լ��������͵Ķ�ʱ��Ϣ�����磺
 * MSG_TIMER_TIME_TRIGGER_SECOND��Ϣ�Ķ���
 *
 * Ŀǰ��ʱʵ���뼶��ʱ�����������ȶ�ʱ�������ӣ�
 * ��һ���ڴ˴������߳̿�������Ϣ����
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

	//��������Ϣ��Ӧ
	msgSubscribe(pServer, MSG_TIMER_TIME_TRIGGER_SECOND, (T_MSG_CALLBACK_FUNC)timerMsgProc_s,
			pTimer, MSG_PRIORITY_CMD + 1, false, true);

	return pTimer;

newMsgTimeError :
	if(NULL != pTimer)
		free(pTimer);

	return NULL;
}

/***********************************************
 * ɾ��timer
 ***********************************************/
void delMsgTimer(HANDLE handle) {

	if(NULL == handle)
		return ;

	P_MSG_TIMER pTimer = (P_MSG_TIMER)handle;

	//ɾ�����ĵ�����Ϣ��Ӧ
	msgUnsubscribe(pTimer->pServer, MSG_TIMER_TIME_TRIGGER_SECOND, (T_MSG_CALLBACK_FUNC)timerMsgProc_s, pTimer);

	//ɾ�����õĶ�ʱ��Ԫ��
	P_TIMER_ELEMENT pos = NULL, n = NULL;
	list_for_each_entry_safe(pos, n, &pTimer->elementHead_s, node) {
		delTimerElement(pos);
	}

	//������ڶ�ʱ�̣߳������߳�������pTimer���ͷ��ڴ�
	//����ֱ���ͷ�timer���ڴ�
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
 * ��timer�����һ����ʱ��Ԫ��
 * time����ʱ����ʱʱ�䣬����Ϊ��λ
 * timeId���ö�ʱ��Ԫ�ص�Ψһ��ʾ�������ⲿģ��ʶ��
 * �ö�ʱ��Ԫ������һ����ʱ����
 * func����ʱ��Ԫ�س�ʱ��Ļص���������ΪNULL
 * funcParam�����ݸ�func�Ĳ���
 * isSendTimeoutMsg���ڳ�ʱ���Ƿ�����������͸ö�ʱ��
 * Ԫ�س�ʱ����Ϣ
 *
 * �ú�����Ϊelement�����ڴ棬���ڴ���delTimerElement����
 * �����ͷ�
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
 * ɾ��ָ�����뼶��ʱ��Ԫ��
 * ���ͷŸö�ʱ��Ԫ�ص��ڴ棬�����Ὣ��ʱ��Ԫ��
 * ��ɾ���Ĳ���֪ͨ�������ⲿģ��
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
 * ����һ���붨ʱ��Ԫ�أ�ʹ�����¿�ʼ��ʱ
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


