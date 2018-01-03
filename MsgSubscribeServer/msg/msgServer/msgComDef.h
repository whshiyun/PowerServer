/***********************************************
 *  /PowerPixelServer/msg/msgServer/msgComDef.h
 *
 *  Created on: 2017年11月27日
 *  Author: gift
 *
 ***********************************************/

#ifndef MSG_MSGSERVER_MSGCOMDEF_H_
#define MSG_MSGSERVER_MSGCOMDEF_H_

#include "msgServerDef.h"

/*
 * MinGW中__attribute__ ((__packed__))是无效的，要用：
 * __attribute__ ((gcc_struct, packed))或#pragma pack(1)
 * 由于MinGW中在windows下启用了一些特性，详见如下blog
 * http://blog.csdn.net/kuwater/article/details/22400929
 */

typedef struct __msg_hello_pkt {

	unsigned char locId[MSG_SERVER_LOCAL_ID_LEN];//该msgServer的Id，一般为该接口的物理地址，如Mac地址等。该字段暂时未使用
	unsigned int msgNum;
	T_MSG_SN msgTable[MSG_SERVER_MAX_SUBSCRIBE_NUM];

}__attribute__ ((gcc_struct, packed)) MSG_HELLO_PKT, *P_MSG_HELLO_PKT;

typedef struct __msg_detect_pkt {

	unsigned char locId[MSG_SERVER_LOCAL_ID_LEN];//该msgServer的Id，一般为该接口的物理地址，如Mac地址等。该字段暂时未使用
	unsigned int pktSN;

}__attribute__ ((gcc_struct, packed)) MSG_DETECT_PKT, *P_MSG_DETECT_PKT;

typedef struct __msg_detect_reply_pkt {

	unsigned char locId[MSG_SERVER_LOCAL_ID_LEN];//该msgServer的Id，一般为该接口的物理地址，如Mac地址等。该字段暂时未使用
	unsigned int pktSN;

}__attribute__ ((gcc_struct, packed)) MSG_DETECT_REPLY_PKT, *P_MSG_DETECT_REPLY_PKT;

#endif /* MSG_MSGSERVER_MSGCOMDEF_H_ */
