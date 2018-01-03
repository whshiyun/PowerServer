/***********************************************
 *  /MsgSubscribeServer/msg/system.h
 *
 *  Created on: 2017Äê11ÔÂ27ÈÕ
 *  Author: gift
 *
 ***********************************************/

#ifndef MSG_SYSTEM_H_
#define MSG_SYSTEM_H_

#define windows

#ifdef windows

#include <windows.h>
#define HANDLE									HANDLE
#define MsgCreateTask(size, callback, args) 	CreateThread( \
													 NULL, \
													 size, \
													 (LPTHREAD_START_ROUTINE)(callback), \
													 args, \
													 0, \
													 NULL)
#define MsgDelay_ms(ms)							Sleep((ms))
#endif

#endif /* MSG_SYSTEM_H_ */
