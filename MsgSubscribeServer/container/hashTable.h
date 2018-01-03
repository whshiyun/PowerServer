/*
 * hashTable.h
 *
 *  Created on: 2017年8月25日
 *      Author: Gift
 *
 *  有_save的遍历，是对删除安全的，如果只是查询则可以使用不带_save的遍历
 */

#ifndef CONTAINER_HASHTABLE_H_
#define CONTAINER_HASHTABLE_H_

#include "linkList.h"

typedef LIST_NODE HASH_NODE, *P_HASH_NODE;
typedef HASH_NODE HASH_HEAD;
typedef HASH_HEAD *P_HASH_TABLE;

#define HLIST_NODE_INIT(type, node) { .pPrev = ((type *)(&node)), \
									.pNext = ((type *)(&node)) }

#define INIT_HLIST_NODE(ptr) { ((ptr)->pPrev = ptr); 		\
							((ptr)->pNext = ptr); }

#define DEFINE_HASHTABLE(name, bits)						\
		HASH_HEAD name[1 << (bits)] =					\
			{ [0 ... ((1 << (bits)) - 1)] = HLIST_NODE_INIT(HASH_HEAD, name) }

#define NEW_HASHTABLE(bits) (malloc(sizeof(HASH_HEAD) * (1 << (bits))))

#define HASH_SIZE(name) (ARRAY_SIZE(name))

#define HASH_FUNC(sz, key) ((key) & ((sz) - 1))//((key) % (sz))

static inline void __hash_init(P_HASH_TABLE pHt, unsigned int sz)
{
	unsigned int i;

	for (i = 0; i < sz; i++)
		INIT_HLIST_NODE(&(pHt[i]));
}

static inline void __hash_del(HASH_NODE *pHn) {

	list_del((P_LIST_NODE)pHn);
}

#define hash_init(ht) 							\
		__hash_init(ht, HASH_SIZE(ht))

#define hash_add(ht, key, pNew) 							\
		list_add(&(ht[HASH_FUNC(HASH_SIZE(ht), key)]), pNew)

#define hash_add_tail(pHead, pNew) 							\
		list_add_tail(pHead, pNew)

#define hash_add_before(pNode, pNew) 							\
		list_insert(pNode->pPrev, pNode, pNew)

#define hash_del(pHn)							\
		__hash_del(pHn)

#define hash_head_get(ht, key) (&(ht[HASH_FUNC(HASH_SIZE(ht), key)]))


#define hlist_for_each(pos, head) \
		list_for_each(pos, head)

#define hlist_for_each_safe(pos, n, head)	\
		list_for_each_safe(pos, n, head)

/**
 * hlist_for_each_entry	- iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry(pos, head, member)	\
		list_for_each_entry(pos, head, member)

/**
 * hlist_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another &struct hlist_node to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry_safe(pos, n, head, member) 		\
		list_for_each_entry_safe(pos, n, head, member)

#define hlist_empty(head) \
		list_empty(head)

#endif /* CONTAINER_HASHTABLE_H_ */
