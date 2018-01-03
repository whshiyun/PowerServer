/***********************************************
 *  /MsgSubscribeServer/msg/msgServer/msgTimeDef.h
 *
 *  Created on: 2017年11月29日
 *  Author: gift
 *
 ***********************************************/

#ifndef MSG_MSGSERVER_MSGTIMEDEF_H_
#define MSG_MSGSERVER_MSGTIMEDEF_H_

#define TIMER_TIME_OUT_NEVER		(-1)//永不超时

typedef void (*TIMEOUT_PROC_FUNC)(void *pProcParam);//超时回调函数类型

#endif /* MSG_MSGSERVER_MSGTIMEDEF_H_ */
