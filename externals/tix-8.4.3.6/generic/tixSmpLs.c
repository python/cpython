
/*	$Id: tixSmpLs.c,v 1.1.1.1 2000/05/17 11:08:42 idiscovery Exp $	*/

/* 
 * tixSmpLs.c --
 *
 *	To implement simple link lists (next is always the first
 *	member of the list).
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <tixPort.h>
#include <tixInt.h>

static Tix_ListInfo simpleListInfo = {
    0,
    TIX_UNDEFINED,
};

void Tix_SimpleListInit(lPtr)
    Tix_LinkList * lPtr;
{
    Tix_LinkListInit(lPtr);
}


void
Tix_SimpleListAppend(lPtr, itemPtr, flags)
    Tix_LinkList * lPtr;
    char * itemPtr;
    int flags;
{
    Tix_LinkListAppend(&simpleListInfo, lPtr, itemPtr, flags);
}

void Tix_SimpleListIteratorInit(liPtr)
    Tix_ListIterator * liPtr;
{
    Tix_LinkListIteratorInit(liPtr);
}

void Tix_SimpleListStart(lPtr, liPtr)
    Tix_LinkList * lPtr;
    Tix_ListIterator * liPtr;
{
    Tix_LinkListStart(&simpleListInfo, lPtr, liPtr);
}

void Tix_SimpleListNext(lPtr, liPtr)
    Tix_LinkList * lPtr;
    Tix_ListIterator * liPtr;
{
    Tix_LinkListNext(&simpleListInfo, lPtr, liPtr);
}

/*
 * To delete consecutive elements, you must delete, next, delete, next ...
 */
void Tix_SimpleListDelete(lPtr, liPtr)
    Tix_LinkList * lPtr;
    Tix_ListIterator * liPtr;
{
    Tix_LinkListDelete(&simpleListInfo, lPtr, liPtr);
}

/*----------------------------------------------------------------------
 *  Tix_SimpleListInsert --
 *
 *	Insert the item at the position indicated by liPtr
 *----------------------------------------------------------------------
 */
void Tix_SimpleListInsert(lPtr, itemPtr, liPtr)
    Tix_LinkList * lPtr;
    char * itemPtr;
    Tix_ListIterator * liPtr;
{
    Tix_LinkListInsert(&simpleListInfo, lPtr, itemPtr, liPtr);
}

/*----------------------------------------------------------------------
 * Tix_SimpleListFindAndDelete --
 *
 *	Find an element and delete it.
 *
 * liPtr:
 *	Can be zero.
 *	If non-zero, the search will start from the current entry indexed
 *	by liPtr;
 *
 * Return value:
 *	1 if element is found and deleted
 *	0 if element is not found
 *----------------------------------------------------------------------
 */
int Tix_SimpleListFindAndDelete(lPtr, itemPtr, liPtr)
    Tix_LinkList * lPtr;
    char * itemPtr;
    Tix_ListIterator * liPtr;
{
    return Tix_LinkListFindAndDelete(&simpleListInfo, lPtr, itemPtr, liPtr);
}

int Tix_SimpleListDeleteRange(lPtr, fromPtr, toPtr, liPtr)
    Tix_LinkList * lPtr;
    char * fromPtr;
    char * toPtr;
    Tix_ListIterator * liPtr;
{
    return Tix_LinkListDeleteRange(&simpleListInfo, lPtr,
	fromPtr, toPtr, liPtr);
}

int Tix_SimpleListFind(lPtr, itemPtr, liPtr)
    Tix_LinkList * lPtr;
    char * itemPtr;
    Tix_ListIterator * liPtr;
{
    return Tix_LinkListFind(&simpleListInfo, lPtr, itemPtr, liPtr);
}
