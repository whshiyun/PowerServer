/*
 * container.h
 *
 *  Created on: 2017��8��23��
 *      Author: Gift
 */

#ifndef CONTAINER_CONTAINER_H_
#define CONTAINER_CONTAINER_H_

/*
 * �������еĲ���ϵͳ�£������ϵͳ��API���Ѿ������˸ú���
 * �������ﲻϣ���ò���ϵͳ�ṩ�ģ�����������ϵͳ�Ѿ��ṩ��
 * �ò������������ε���
 * ����windows��ʵ�⣬��ֱ��ʹ��windows�µĸ�API��������������
 * �����øô��룬��eclipse����ʾ������󣬵�������ȷ���룬Ϊ����
 * ���ִ�����ʾ��Ӧ����eclipse��bug�����ʲ�ȡ�˷�ʽ��
 */
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#else
#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })
#else
#undef container_of
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })
#endif

#define __must_be_array(a) (0)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

#endif /* CONTAINER_CONTAINER_H_ */
