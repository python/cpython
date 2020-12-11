
/*	$Id: tixList.c,v 1.1.1.1 2000/05/17 11:08:42 idiscovery Exp $	*/

/* 
 * tixList.c --
 *
 *	Implements easy-to-use link lists.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */
#include <tixPort.h>
#include <tixInt.h>

#define NEXT(info,ptr) (char*)(*(char**)((ptr+(info->nextOffset))))
#define PREV(info,ptr) (char*)(*(char**)((ptr+(info->prevOffset))))

static void 		SetNext _ANSI_ARGS_((Tix_ListInfo * info,
			    char * ptr, char * next));

static void SetNext(info, ptr, next)
    Tix_ListInfo * info;
    char * ptr;
    char * next;
{
    char ** next_ptr = (char**)((ptr+(info->nextOffset)));
    * next_ptr = next;
}

void Tix_LinkListInit(lPtr)
    Tix_LinkList * lPtr;
{
    lPtr->numItems = 0;
    lPtr->head   = (char*)NULL;
    lPtr->tail   = (char*)NULL;
}


void
Tix_LinkListAppend(infoPtr, lPtr, itemPtr, flags)
    Tix_ListInfo * infoPtr;
    Tix_LinkList * lPtr;
    char * itemPtr;
    int flags;
{
    char * ptr;

    if (flags | TIX_UNIQUE) {
	/* Check for uniqueness */
	for (ptr=lPtr->head;
	     ptr!=NULL;
	     ptr=NEXT(infoPtr,ptr)) {
	    if (ptr == itemPtr) {
		return;
	    }
	}
    }
    if (lPtr->head == NULL) {
	lPtr->head = lPtr->tail = itemPtr;
    } else {
	SetNext(infoPtr, lPtr->tail, itemPtr);
	lPtr->tail = itemPtr;
    }

    SetNext(infoPtr, itemPtr, NULL);
    ++ lPtr->numItems;
}

void Tix_LinkListIteratorInit(liPtr)
    Tix_ListIterator * liPtr;
{
    liPtr->started = 0;
}

void Tix_LinkListStart(infoPtr, lPtr, liPtr)
    Tix_ListInfo * infoPtr;
    Tix_LinkList * lPtr;
    Tix_ListIterator * liPtr;
{
    if (lPtr->head == NULL) {
	liPtr->last = NULL;
	liPtr->curr = NULL;
    } else {
	liPtr->last = liPtr->curr = lPtr->head;
    }
    liPtr->deleted = 0;
    liPtr->started = 1;
}

void Tix_LinkListNext(infoPtr, lPtr, liPtr)
    Tix_ListInfo * infoPtr;
    Tix_LinkList * lPtr;
    Tix_ListIterator * liPtr;
{
    if (liPtr->curr == NULL) {
	return;
    }
    if (liPtr->deleted == 1) {
	/* the curr pointer has already been adjusted */
	liPtr->deleted = 0;
	return;
    }

    liPtr->last = liPtr->curr;
    liPtr->curr = NEXT(infoPtr, liPtr->curr);
}

/*
 *----------------------------------------------------------------------
 * Tix_LinkListDelete --
 *
 *	Deletes an element from the linklist. The proper step of deleting
 *	an element is:
 *
 *	for (Tix_SimpleListStart(&list, &li); !Tix_SimpleListDone(&li);
 *	         Tix_SimpleListNext (&list, &li)) {
 *	    MyData * p = (MyData*)li.curr;
 *	    if (someCondition) {
 *		Tix_SimpleListDelete(&cPtr->subWDefs, &li);
 *		ckfree((char*)p);
 *          }
 *	}
 *
 *	i.e., The pointer can be freed only after Tix_SimpleListDelete().
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The pointers in the list are adjusted  and the liPtr is advanced
 *	to the next element.
 *----------------------------------------------------------------------
 */
void
Tix_LinkListDelete(infoPtr, lPtr, liPtr)
    Tix_ListInfo * infoPtr;
    Tix_LinkList * lPtr;
    Tix_ListIterator * liPtr;
{
    if (liPtr->curr == NULL) {
	/* %% probably is a mistake */
	return;
    }
    if (liPtr->deleted == 1) {
	/* %% probably is a mistake */
	return;
    }
    if (lPtr->head == lPtr->tail) {
	lPtr->head  = lPtr->tail = NULL;
	liPtr->curr = NULL;
    }
    else if (lPtr->head == liPtr->curr) {
	lPtr->head  = NEXT(infoPtr, liPtr->curr);
	liPtr->curr = lPtr->head;
	liPtr->last = lPtr->head;
    }
    else if (lPtr->tail == liPtr->curr) {
	lPtr->tail = liPtr->last;
	SetNext(infoPtr, lPtr->tail, NULL);
	liPtr->curr = NULL;
    }
    else {
	SetNext(infoPtr, liPtr->last, NEXT(infoPtr, liPtr->curr));
	liPtr->curr = NEXT(infoPtr, liPtr->last);
    }
    -- lPtr->numItems;

    liPtr->deleted = 1;
}

/*----------------------------------------------------------------------
 *  Tix_LinkListInsert --
 *
 *	Insert the item at the position indicated by liPtr
 *----------------------------------------------------------------------
 */
void Tix_LinkListInsert(infoPtr, lPtr, itemPtr, liPtr)
    Tix_ListInfo * infoPtr;
    Tix_LinkList * lPtr;
    char * itemPtr;
    Tix_ListIterator * liPtr;
{
    if (lPtr->numItems == 0) {
	/* Just do an append
	 */
	Tix_LinkListAppend(infoPtr, lPtr, itemPtr, 0);

	/* Fix the iterator (%% I am not sure if this is necessary)
	 */
	liPtr->curr = liPtr->last = lPtr->head;
	return;
    }

    if (liPtr->curr == NULL) {
	/* %% probably is a mistake */
	return;
    }
    if (lPtr->head == lPtr->tail) {
	lPtr->head = itemPtr;
	SetNext(infoPtr, lPtr->head, lPtr->tail);
	liPtr->last = itemPtr;
	liPtr->curr = itemPtr;
    }
    else if (liPtr->curr == lPtr->head) {
	lPtr->head = itemPtr;
	SetNext(infoPtr, lPtr->head, liPtr->curr);
	liPtr->last = itemPtr;
	liPtr->curr = itemPtr;
    }
    else {
	SetNext(infoPtr, liPtr->last, itemPtr);
	SetNext(infoPtr, itemPtr,     liPtr->curr);
	liPtr->last = itemPtr;
    }
    ++ lPtr->numItems;
}

/*----------------------------------------------------------------------
 * Tix_LinkListFindAndDelete --
 *
 *	Find an element and delete it.
 *
 * liPtr:
 *	Can be NULL.
 *	If non-NULL, the search will start from the current entry indexed
 *	by liPtr;
 *
 * Return value:
 *	1 if element is found and deleted
 *	0 if element is not found
 *----------------------------------------------------------------------
 */
int Tix_LinkListFindAndDelete(infoPtr, lPtr, itemPtr, liPtr)
    Tix_ListInfo * infoPtr;
    Tix_LinkList * lPtr;
    char * itemPtr;
    Tix_ListIterator * liPtr;
{
    Tix_ListIterator defIterator;

    if (liPtr == NULL) {
	Tix_LinkListIteratorInit(&defIterator);
	liPtr = &defIterator;
    }

    if (!liPtr->started) {
	Tix_LinkListStart(infoPtr, lPtr, liPtr);
    }

    if (Tix_LinkListFind(infoPtr, lPtr, itemPtr, liPtr)) {
	Tix_LinkListDelete(infoPtr, lPtr, liPtr);
	return 1;
    } else {
	return 0;
    }
}

int Tix_LinkListDeleteRange(infoPtr, lPtr, fromPtr, toPtr, liPtr)
    Tix_ListInfo * infoPtr;
    Tix_LinkList * lPtr;
    char * fromPtr;
    char * toPtr;
    Tix_ListIterator * liPtr;
{
    Tix_ListIterator defIterator;
    int start = 0;
    int deleted = 0;

    if (liPtr == NULL) {
	Tix_LinkListIteratorInit(&defIterator);
	liPtr = &defIterator;
    }
    if (!liPtr->started) {
	Tix_LinkListStart(infoPtr, lPtr, liPtr);
    }

    for (;
	 !Tix_LinkListDone(liPtr);
	 Tix_LinkListNext (infoPtr, lPtr, liPtr)) {

	if (liPtr->curr == fromPtr) {
	    start = 1;
	}
	if (start) {
	    Tix_LinkListDelete(infoPtr, lPtr, liPtr);
	    ++ deleted;
	}
	if (liPtr->curr == toPtr) {
	    break;
	}
    }

    return deleted;
}

int Tix_LinkListFind(infoPtr, lPtr, itemPtr, liPtr)
    Tix_ListInfo * infoPtr;
    Tix_LinkList * lPtr;
    char * itemPtr;
    Tix_ListIterator * liPtr;
{
    if (!liPtr->started) {
	Tix_LinkListStart(infoPtr, lPtr, liPtr);
    }

    for (Tix_LinkListStart(infoPtr, lPtr, liPtr);
	 !Tix_LinkListDone(liPtr);
	 Tix_LinkListNext (infoPtr, lPtr, liPtr)) {

	if (liPtr->curr == itemPtr) {
	    return 1;				/* found! */
	}
    }

    return 0;					/* Can't find */
}
