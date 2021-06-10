#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H

#include "global.h"
#include "stdint.h"

struct list_elem
{
	struct list_elem* prev;
	struct list_elem* next;
};

struct list
{
	/* the head and tail of list are empty and fixed */
	struct list_elem head;
	struct list_elem tail;
};

/* universal function for traverse */
typedef bool (list_traversal_func) (struct list_elem* elem, int arg);

/* operation of list */
void list_init(struct list* plist);
void list_insert_before(struct list_elem* pbefore, struct list_elem* pelem);
void list_push(struct list* plist, struct list_elem* pelem);
void list_iterate(struct list* plist);
void list_append(struct list* plist, struct list_elem* pelem);
void list_remove(struct list_elem* pelem);
struct list_elem* list_pop(struct list* plist);
bool list_empty(struct list* plist);
uint32_t list_len(struct list* plist);
struct list_elem* list_traversal(struct list* plist, list_traversal_func* func, int arg);
bool elem_find(struct list* plist, struct list_elem* obj_elem);

#endif // __LIB_KERNEL_LIST_H
