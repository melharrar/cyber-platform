/*
 * linkList.c
 *
 *  Created on: Feb 28, 2016
 *      Author: melharrar
 */


#include <stddef.h>
#include "llist.h"


/*! \defgroup llist linked list
 * \ingroup utils
 * @{
 */

/*! file
 * Double link list general routine.
 * insert and delete are in O(1)
 */

static void LLE_AddAfter(LL_ELEMENT_T *element, LL_ELEMENT_T *le);
static void LLE_AddBefore(LL_ELEMENT_T *element, LL_ELEMENT_T *le);


/*! Init Linked List
 * \param[in] llist - linked list header*/
void LL_Init(LL_T *llist)
{
	llist->head = NULL;
	llist->tail = NULL;
	llist->count = 0;
//#warning Implement mutex mechanism
	//llist->mutex = osLayer->osMutexAlloc();
};

/*! Init sorted linked list
 * \param[in] llist - linked list header
 * \param[in] cmp - compare function
 */

int LL_InitSort(LL_SORT_T *llist, LL_CMP_F cmp)
{
	//UTIL_ASSERT_LOG(llist==NULL, return probGeneralError, "illegal list pointer");
	//UTIL_ASSERT_LOG(cmp==NULL, return probGeneralError, "illegal compare function");

	if(llist==NULL || cmp==NULL)
		return 0;

	LL_Init(&llist->list);
	llist->cmp = cmp;
	return 1;
};

/*! Detach list elements from Linked List
 * \param[in] llist - linked list header*/

void LL_Detach(LL_T *llist)
{
	llist->head = NULL;
	llist->tail = NULL;
	llist->count = 0;
};

/*! Retach list elements to Linked List
 * \param[in] llist - linked list header
 * \param[in] element - head element to attach to list
 * */

void LL_Retach(LL_T *llist, LL_ELEMENT_T *element)
{
	llist->tail = llist->head = element;
	while(element && LL_InList(element))
	{
		llist->tail = element;
		llist->count++;
		element = LL_Next(element);
	}
};

/*! Destroy Linked List
 * \param[in] llist = linked list handle
 * \param[in] func - a callback for each element
 * \param[in] cookie - argument for callback
 */

void LL_Destroy(LL_T *llist, int (*func)(void *,void *), void *cookie)
{
	LL_ELEMENT_T *safe_next;
	LL_ELEMENT_T *element;

	if(func != NULL)
	{
		element = llist->head;
		while ((element != NULL) && LL_InList(element))
		{
			safe_next = element->next;
			(*func)(element,cookie);
			element = safe_next;
		};
	}
	llist->head = NULL;
	llist->tail = NULL;
	llist->count = 0;

//#warning Implement mutex mechanism
	//osLayer->osMutexFree(llist->mutex);
};

/*! init link list element. must be called before adding element to link list.
 * \param[in] element - link list element
 */

void LL_InitElement(LL_ELEMENT_T *element)
{
	element->next = NULL;
	element->prev = element;
};

/* add element at list tail
 * \param[in] llist - link list handle
 * \param[in] element - link list element
 */
void LListAdd(LL_T *llist, LL_ELEMENT_T *element)
{
  if (element->prev != element)
	  return;
  element->next = NULL;
  element->prev = llist->tail;
  if (llist->tail != NULL)
	  llist->tail->next = element;
  llist->tail = element;
  if (llist->head == NULL)
	  llist->head = element;
  llist->count ++;
};

/* !add element on list start
 * \param[in] list - linked list handle
 * \param[in] element - link list element
 */

void LListAddToStart(LL_T *list, LL_ELEMENT_T *element)
{

 if (element->prev != element)
	 return;
  element->prev = NULL;
 if (list->head != NULL)
  {
	  element->next = list->head;
	  list->head->prev = element;
	  list->head = element;
  }
  else
  {
	  element->next = NULL;
	  list->head = element;
	  list->tail = element;
  }
  list->count ++;
};

/*! add element in sort order
 * \param[in] llist - link list handle
 * \param[in] element - link list element
 * \param[in] cookie - cookie pass to compare function
 * 0 not allowed other wise allowed
 */
void LListAddSort(LL_SORT_T *llist, LL_ELEMENT_T *element, void *cookie)
{
	LL_ELEMENT_T *curr_elem;
	int cmp=0;

	curr_elem = LL_First(&llist->list);
	while(curr_elem && ((cmp = llist->cmp(curr_elem, element, cookie))<= 0) )
		curr_elem = LL_Next(curr_elem);

	if(curr_elem == NULL)//add at list tail
		LListAdd(&llist->list, element);
	else if(llist->list.head == curr_elem)//add at list head
		LListAddToStart(&llist->list, element);
	else//add in the middle
	{
		LLE_AddBefore(curr_elem, element);
		llist->list.count++;
	}
}

/*! remove an element off the list
 * note: the element is not dealocated just removed
 * \param[in] list - link list handle
 * \param[in] element - link list element to remove
 */

void LListDelete(LL_T *list,LL_ELEMENT_T *element)
{
	if(!list)
		return;

	if(!list->count)
		return;

	if (element->prev == element)
		return;

	if (element->next != NULL)
		element->next->prev = element->prev;
	if (element->prev != NULL)
		element->prev->next = element->next;
	if (list->head == element)
		list->head = element->next;
	if (list->tail == element)
		list->tail = element->prev;
	if (list->count > 0)
		list->count--;
	LL_InitElement(element);
};

/*! return the first element of the list
 * \param[in] list - link list handle
 */

LL_ELEMENT_T *LL_First(LL_T *list)
{
	return (list->head);
};

/*! return the last element of the list
 * \param[in] list - link list handle
 */
LL_ELEMENT_T *LL_Last(LL_T *list)
{
	return (list->tail);
}



/*! return the next element after the specified element
 * \param[in] element - link list element
 */
LL_ELEMENT_T *LL_Next(LL_ELEMENT_T *element)
{
	return (element->next);
};

/*! return the element before the given element
 * \param[in] element - link list element
 */
LL_ELEMENT_T *LL_Prev(LL_ELEMENT_T *element)
{
	if ((element->prev != NULL) && (!LL_InList(element->prev)))
		return NULL;
	return element->prev;
};

/*! do a function over all element list
 * if the function return a nonzero value the iteration stop returning the
 * element where the function stop otherwise NULL is returned
 * \param[in] list - link list handle
 * \param[in] func - a given callback. get an element pointer and a cookie. The iteration stop
 * if function return anon zero value.
 * \param[in] cookie - cookie given to callback.
 */
LL_ELEMENT_T *LL_ForEach(LL_T *list, int (*func)(void *,void *), void *cookie)
{
	LL_ELEMENT_T *safe_next;
	LL_ELEMENT_T *element;

	element = list->head;
	while ((element != NULL) && LL_InList(element))
	{
		safe_next = element->next;
		if ((*func)(element,cookie))
			return (element);
		element = safe_next;
	};
    return(NULL);
};
/*! return the number of list elements
 * \param[in] list - link list handle*/

int LL_Count(LL_T *list)
{
  if (list == NULL)
    return 0;
  return list->count;
};

/* this functions set are to be used in list where there is no
 * list header or the count is not relevant
 */

/*!
 * Add element after the specified element
 */
static void LLE_AddAfter(LL_ELEMENT_T *element, LL_ELEMENT_T *le)
{
  LL_ELEMENT_T *tmp;
  tmp = element->next;
  element->next = le;
  le->next = tmp;
  le->prev = element;
  if (tmp != NULL)
   tmp->prev = le;
};

/*! add an element before the current element
 */
static void LLE_AddBefore(LL_ELEMENT_T *element, LL_ELEMENT_T *le)
{
	LL_ELEMENT_T *tmp;
	tmp = element->prev;
	element->prev = le;
	le->next = element;
	le->prev = tmp;
	if (tmp != NULL)
	  tmp->next = le;
};

/*! add an element at the end of the element list
 */
void LLE_AddAtEnd(LL_ELEMENT_T *element, LL_ELEMENT_T *le)
{
	while (element->next != NULL)
	  element = element->next;
	LLE_AddAfter(element,le);
};

/*! add an element at the beginning of the list
 */
void LLE_AddAtStart(LL_ELEMENT_T *element,LL_ELEMENT_T *le)
{
	while (element->prev != NULL)
	  element = element->prev;
	LLE_AddBefore(element,le);
};

/*! lock list (mutual exclusion)
 * \param[in] list - linked list handle
 */
void LL_Lock(LL_T *list)
{
	(void)list;
//#warning Implement mutex mechanism
	//osLayer->osMutexLock(list->mutex);
}
/*! unlock list
 * \param[in] list = linked list handle
 */
void LL_UnLock(LL_T *list)
{
	(void)list;
//#warning Implement mutex mechanism
	//osLayer->osMutexUnlock(list->mutex);
}

/* indicate element belong to any list
 * \param[in] element - link list element
 */
int LL_InList(LL_ELEMENT_T *element)
{
	return (element->prev != element);
}

/*@}*/
