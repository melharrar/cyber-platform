/*
 * linkedList.h
 *
 *  Created on: Feb 28, 2016
 *      Author: melharrar
 */

#ifndef _LINKED_LIST_H
#define _LINKED_LIST_H

#include <stdint.h>

typedef struct ll_element_t
{
  struct ll_element_t *next;
  struct ll_element_t *prev;
}LL_ELEMENT_T;

typedef struct ll_t
{
	struct ll_element_t *head;
	struct ll_element_t *tail;
	int count;

	//TODO: do it portable
	//pthread_mutex_t mutex;
	void * mutex;
}LL_T;

//compare function (return -1 if b > a, return 0 if a==b, return 1 if b < a)
typedef int LL_CMP_F(LL_ELEMENT_T *a, LL_ELEMENT_T *b, void *cookie);

typedef struct ll_sort_t
{
	LL_T list;
	LL_CMP_F *cmp;
}LL_SORT_T;

#define LL_T_INIT  				{NULL,NULL,0,PTHREAD_MUTEX_INITIALIZER}
#define LL_T_INIT_ORDER(__cmp)  {LL_T_INIT, __cmp}


#define LL_ENTRY(ptr ,name, type) \
((type *)((char *)(ptr) - (uintptr_t)(&(((type *)0)->name))))
void LL_Init(LL_T *llist);
void LL_Detach(LL_T *llist);
void LL_Retach(LL_T *llist, LL_ELEMENT_T *element);
void LL_Destroy(LL_T *llist, int (*func)(void *,void *), void *cookie);
void LL_InitElement(LL_ELEMENT_T *element);
void LListAdd(LL_T *llist, LL_ELEMENT_T *element);
void LListAddToStart(LL_T *list, LL_ELEMENT_T *element);
void LListDelete(LL_T *list,LL_ELEMENT_T *element);
LL_ELEMENT_T *LL_First(LL_T *list);
LL_ELEMENT_T *LL_Next(LL_ELEMENT_T *element);
LL_ELEMENT_T *LL_Prev(LL_ELEMENT_T *element);
LL_ELEMENT_T *LL_ForEach(LL_T *list, int (*func)(void *,void *), void *cookie);
LL_ELEMENT_T *LL_Last(LL_T *list);
int LL_Count(LL_T *list);
void LL_Lock(LL_T *list);
void LL_UnLock(LL_T *list);
int LL_InList(LL_ELEMENT_T *element);
int LL_InitSort(LL_SORT_T *llist, LL_CMP_F cmp);
void LListAddSort(LL_SORT_T *llist, LL_ELEMENT_T *element, void *cookie);
void LLE_AddAtEnd(LL_ELEMENT_T *element, LL_ELEMENT_T *le);
void LLE_AddAtStart(LL_ELEMENT_T *element,LL_ELEMENT_T *le);

#endif
