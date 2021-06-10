#include "list.h"
#include "interrupt.h"

/* initialize doubly linked list */
void list_init(struct list* plist)
{
	plist->head.prev = NULL;
	plist->head.next = &plist->tail;
	plist->tail.prev = &plist->head;
	plist->tail.next = NULL;
}

/* add pelem before pbefore */
void list_insert_before(struct list_elem* pbefore, struct list_elem* pelem)
{
	enum intr_status old_status = intr_disable();
	
	pbefore->prev->next = pelem;	

	pelem->next = pbefore;
	pelem->prev = pbefore->prev;

	pbefore->prev = pelem;

	intr_set_status(old_status);
}

/* add pelem as the first elem in list */
void list_push(struct list* plist, struct list_elem* pelem)
{
	list_insert_before(plist->head.next, pelem);
}

/* add pelem as the last elem in the list */
void list_append(struct list* plist, struct list_elem* pelem)
{
	list_insert_before(&plist->tail, pelem);
}

/* remove pelem from list */
void list_remove(struct list_elem* pelem)
{
	enum intr_status old_status = intr_disable();
	
	pelem->prev->next = pelem->next;
	pelem->next->prev = pelem->prev;
		
	intr_set_status(old_status);
}

/* remove first elem in list and return it */
struct list_elem* list_pop(struct list* plist)
{
	struct list_elem* pelem = plist->head.next;
	list_remove(pelem);
	return pelem;
}

/* judge if list is empty */
inline bool list_empty(struct list* plist)
{
	return (plist->head.next == &plist->tail);
}

/* return length of list */
uint32_t list_len(struct list* plist)
{
 	struct list_elem* pelem = plist->head.next;
	uint32_t cnt = 0;
	
	while(pelem != &plist->tail)
	{
		cnt++;
		pelem = pelem->next;
	}

	return cnt;
}

/**/
struct list_elem* list_traversal(struct list* plist, list_traversal_func* func, int arg)
{
	struct list_elem* pelem = plist->head.next;
	
	while(pelem != &plist->tail)
	{
		if(func(pelem, arg))
		{
			return pelem;
		}
		pelem = pelem->next;
	}

	return NULL;
}

/* find if obj_elem exist in list */
bool elem_find(struct list* plist, struct list_elem* obj_elem)
{
	struct list_elem* pelem = plist->head.next;
	while(pelem != &plist->tail)
	{
		if(pelem == obj_elem)
		{
			return true;
		}
		pelem = pelem->next;
	}

	return false;
}
