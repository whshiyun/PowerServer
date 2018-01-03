/*
 * linkList.c
 *
 *  Created on: 2017Äê8ÔÂ23ÈÕ
 *      Author: Gift
 */

#include <stdio.h>
#include "linkList.h"


static __inline void __add(P_LIST_NODE pPrev, P_LIST_NODE pNext, P_LIST_NODE pNew) {

	if((NULL == pPrev) || (NULL == pNext) || (NULL == pNew))
		return ;

	pNew->pPrev = pPrev;
	pNew->pNext = pNext;
	pPrev->pNext = pNew;
	pNext->pPrev = pNew;
}

static __inline void __del(P_LIST_NODE pPrev, P_LIST_NODE pNext) {

	if((NULL == pPrev) || (NULL == pNext))
		return ;

	pPrev->pNext = pNext;
	pNext->pPrev = pPrev;
}

static __inline void __del_entry(P_LIST_NODE pNode) {

	if(NULL == pNode)
		return ;

	__del(pNode->pPrev, pNode->pNext);
}


void init_list_node(P_LIST_NODE pNode) {

	if(NULL == pNode)
		return ;

	pNode->pNext = pNode;
	pNode->pPrev = pNode;
}

void list_insert(P_LIST_NODE pPrev, P_LIST_NODE pNext, P_LIST_NODE pNew) {

	if((NULL == pPrev) || (NULL == pNext) || (NULL == pNew))
		return ;

	__add(pPrev, pNext, pNew);
}

void list_add(P_LIST_NODE pHead, P_LIST_NODE pNew) {

	if((NULL == pHead) || (NULL == pNew))
		return ;

	__add(pHead, pHead->pNext, pNew);
}

void list_add_tail(P_LIST_NODE pHead, P_LIST_NODE pNew) {

	if((NULL == pHead) || (NULL == pNew))
		return ;

	__add(pHead->pPrev, pHead, pNew);
}

void list_del(P_LIST_NODE pNode) {

	if(NULL == pNode)
		return ;

	__del_entry(pNode);
	pNode->pPrev = NULL;
	pNode->pNext = NULL;
}

int list_empty(const P_LIST_NODE pHead) {

	return pHead->pNext == pHead;
}


