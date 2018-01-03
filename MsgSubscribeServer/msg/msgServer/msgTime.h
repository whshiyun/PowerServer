/***********************************************
 *  /MsgSubscribeServer/msg/msgServer/msgTime.h
 *
 *  Created on: 2017Äê11ÔÂ29ÈÕ
 *  Author: gift
 *
 ***********************************************/

#ifndef MSG_MSGSERVER_MSGTIME_H_
#define MSG_MSGSERVER_MSGTIME_H_
#include "../system.h"
#include "msgServer.h"
#include "msgTimeDef.h"

HANDLE newMsgTimer(P_MSG_SERVER pServer);
void delMsgTimer(HANDLE handle);

bool addTimerElement_s(HANDLE handle, int time, int timeId, TIMEOUT_PROC_FUNC func,
		void *funcParam, bool isSendTimeoutMsg);
bool delTimerElement_s(HANDLE handle, int timeId);
void resetTimerElement_s(HANDLE handle, int timeId);

#endif /* MSG_MSGSERVER_MSGTIME_H_ */
