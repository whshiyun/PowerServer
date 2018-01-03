/***********************************************
 *  /PowerPixelServer/msg/msgServer/msgComDef.h
 *
 *  Created on: 2017��11��27��
 *  Author: gift
 *
 ***********************************************/

#ifndef MSG_MSGSERVER_MSGCOMDEF_H_
#define MSG_MSGSERVER_MSGCOMDEF_H_

#include "msgServerDef.h"

/*
 * MinGW��__attribute__ ((__packed__))����Ч�ģ�Ҫ�ã�
 * __attribute__ ((gcc_struct, packed))��#pragma pack(1)
 * ����MinGW����windows��������һЩ���ԣ��������blog
 * http://blog.csdn.net/kuwater/article/details/22400929
 */

typedef struct __msg_hello_pkt {

	unsigned char locId[MSG_SERVER_LOCAL_ID_LEN];//��msgServer��Id��һ��Ϊ�ýӿڵ������ַ����Mac��ַ�ȡ����ֶ���ʱδʹ��
	unsigned int msgNum;
	T_MSG_SN msgTable[MSG_SERVER_MAX_SUBSCRIBE_NUM];

}__attribute__ ((gcc_struct, packed)) MSG_HELLO_PKT, *P_MSG_HELLO_PKT;

typedef struct __msg_detect_pkt {

	unsigned char locId[MSG_SERVER_LOCAL_ID_LEN];//��msgServer��Id��һ��Ϊ�ýӿڵ������ַ����Mac��ַ�ȡ����ֶ���ʱδʹ��
	unsigned int pktSN;

}__attribute__ ((gcc_struct, packed)) MSG_DETECT_PKT, *P_MSG_DETECT_PKT;

typedef struct __msg_detect_reply_pkt {

	unsigned char locId[MSG_SERVER_LOCAL_ID_LEN];//��msgServer��Id��һ��Ϊ�ýӿڵ������ַ����Mac��ַ�ȡ����ֶ���ʱδʹ��
	unsigned int pktSN;

}__attribute__ ((gcc_struct, packed)) MSG_DETECT_REPLY_PKT, *P_MSG_DETECT_REPLY_PKT;

#endif /* MSG_MSGSERVER_MSGCOMDEF_H_ */
