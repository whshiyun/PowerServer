/*
 * container.h
 *
 *  Created on: 2017年8月23日
 *      Author: Gift
 */

#ifndef CONTAINER_CONTAINER_H_
#define CONTAINER_CONTAINER_H_

/*
 * 由于在有的操作系统下，其操作系统的API中已经定义了该函数
 * 但是这里不希望用操作系统提供的，所以若操作系统已经提供了
 * 该操作，则将其屏蔽掉。
 * （在windows下实测，若直接使用windows下的该API，且在其他工程
 * 中引用该代码，则eclipse会显示代码错误，但可以正确编译，为避免
 * 这种错误提示（应该是eclipse的bug），故采取此方式）
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
