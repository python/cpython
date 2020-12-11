/*
 * ------------------------------------------------------------------------
 *      PACKAGE:  [incr Tcl]
 *  DESCRIPTION:  Object-Oriented Extensions to Tcl
 *
 *  [incr Tcl] provides object-oriented extensions to Tcl, much as
 *  C++ provides object-oriented extensions to C.  It provides a means
 *  of encapsulating related procedures together with their shared data
 *  in a local namespace that is hidden from the outside world.  It
 *  promotes code re-use through inheritance.  More than anything else,
 *  it encourages better organization of Tcl applications through the
 *  object-oriented paradigm, leading to code that is easier to
 *  understand and maintain.
 *
 *  This segment provides common utility functions used throughout
 *  the other [incr Tcl] source files.
 *
 * ========================================================================
 *  AUTHOR:  Michael J. McLennan
 *           Bell Labs Innovations for Lucent Technologies
 *           mmclennan@lucent.com
 *           http://www.tcltk.com/itcl
 *
 *  overhauled version author: Arnulf Wiedemann
 * ========================================================================
 *           Copyright (c) 1993-1998  Lucent Technologies, Inc.
 * ------------------------------------------------------------------------
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
#include "itclInt.h"
#include <limits.h>

/*
 *  POOL OF LIST ELEMENTS FOR LINKED LIST
 */
static Itcl_ListElem *listPool = NULL;
static int listPoolLen = 0;

#define ITCL_VALID_LIST 0x01face10  /* magic bit pattern for validation */
#define ITCL_LIST_POOL_SIZE 200     /* max number of elements in listPool */

/*
 *  This structure is used to take a snapshot of the interpreter
 *  state in Itcl_SaveInterpState.  You can snapshot the state,
 *  execute a command, and then back up to the result or the
 *  error that was previously in progress.
 */
typedef struct InterpState {
    int validate;                   /* validation stamp */
    int status;                     /* return code status */
    Tcl_Obj *objResult;             /* result object */
    char *errorInfo;                /* contents of errorInfo variable */
    char *errorCode;                /* contents of errorCode variable */
} InterpState;

#define TCL_STATE_VALID 0x01233210  /* magic bit pattern for validation */


/*
 * ------------------------------------------------------------------------
 *  Itcl_Assert()
 *
 *  Called whenever an assert() test fails.  Prints a diagnostic
 *  message and abruptly exits.
 * ------------------------------------------------------------------------
 */

void
Itcl_Assert(
    const char *testExpr,   /* string representing test expression */
    const char *fileName,   /* file name containing this call */
    int lineNumber)	    /* line number containing this call */
{
    Tcl_Panic("Itcl Assertion failed: \"%s\" (line %d of %s)",
	testExpr, lineNumber, fileName);
}



/*
 * ------------------------------------------------------------------------
 *  Itcl_InitStack()
 *
 *  Initializes a stack structure, allocating a certain amount of memory
 *  for the stack and setting the stack length to zero.
 * ------------------------------------------------------------------------
 */
void
Itcl_InitStack(
    Itcl_Stack *stack)     /* stack to be initialized */
{
    stack->values = stack->space;
    stack->max = sizeof(stack->space)/sizeof(ClientData);
    stack->len = 0;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_DeleteStack()
 *
 *  Destroys a stack structure, freeing any memory that may have been
 *  allocated to represent it.
 * ------------------------------------------------------------------------
 */
void
Itcl_DeleteStack(
    Itcl_Stack *stack)     /* stack to be deleted */
{
    /*
     *  If memory was explicitly allocated (instead of using the
     *  built-in buffer) then free it.
     */
    if (stack->values != stack->space) {
        ckfree((char*)stack->values);
    }
    stack->values = NULL;
    stack->len = stack->max = 0;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_PushStack()
 *
 *  Pushes a piece of client data onto the top of the given stack.
 *  If the stack is not large enough, it is automatically resized.
 * ------------------------------------------------------------------------
 */
void
Itcl_PushStack(
    ClientData cdata,      /* data to be pushed onto stack */
    Itcl_Stack *stack)     /* stack */
{
    ClientData *newStack;

    if (stack->len+1 >= stack->max) {
        stack->max = 2*stack->max;
        newStack = (ClientData*)
            ckalloc((unsigned)(stack->max*sizeof(ClientData)));

        if (stack->values) {
            memcpy((char*)newStack, (char*)stack->values,
                (size_t)(stack->len*sizeof(ClientData)));

            if (stack->values != stack->space)
                ckfree((char*)stack->values);
        }
        stack->values = newStack;
    }
    stack->values[stack->len++] = cdata;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_PopStack()
 *
 *  Pops a bit of client data from the top of the given stack.
 * ------------------------------------------------------------------------
 */
ClientData
Itcl_PopStack(
    Itcl_Stack *stack)  /* stack to be manipulated */
{
    if (stack->values && (stack->len > 0)) {
        stack->len--;
        return stack->values[stack->len];
    }
    return NULL;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_PeekStack()
 *
 *  Gets the current value from the top of the given stack.
 * ------------------------------------------------------------------------
 */
ClientData
Itcl_PeekStack(
    Itcl_Stack *stack)  /* stack to be examined */
{
    if (stack->values && (stack->len > 0)) {
        return stack->values[stack->len-1];
    }
    return NULL;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_GetStackValue()
 *
 *  Gets a value at some index within the stack.  Index "0" is the
 *  first value pushed onto the stack.
 * ------------------------------------------------------------------------
 */
ClientData
Itcl_GetStackValue(
    Itcl_Stack *stack,  /* stack to be examined */
    int pos)            /* get value at this index */
{
    if (stack->values && (stack->len > 0)) {
        assert(pos < stack->len);
        return stack->values[pos];
    }
    return NULL;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_InitList()
 *
 *  Initializes a linked list structure, setting the list to the empty
 *  state.
 * ------------------------------------------------------------------------
 */
void
Itcl_InitList(
    Itcl_List *listPtr)     /* list to be initialized */
{
    listPtr->validate = ITCL_VALID_LIST;
    listPtr->num      = 0;
    listPtr->head     = NULL;
    listPtr->tail     = NULL;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_DeleteList()
 *
 *  Destroys a linked list structure, deleting all of its elements and
 *  setting it to an empty state.  If the elements have memory associated
 *  with them, this memory must be freed before deleting the list or it
 *  will be lost.
 * ------------------------------------------------------------------------
 */
void
Itcl_DeleteList(
    Itcl_List *listPtr)     /* list to be deleted */
{
    Itcl_ListElem *elemPtr;

    assert(listPtr->validate == ITCL_VALID_LIST);

    elemPtr = listPtr->head;
    while (elemPtr) {
        elemPtr = Itcl_DeleteListElem(elemPtr);
    }
    listPtr->validate = 0;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_CreateListElem()
 *
 *  Low-level routined used by procedures like Itcl_InsertList() and
 *  Itcl_AppendList() to create new list elements.  If elements are
 *  available, one is taken from the list element pool.  Otherwise,
 *  a new one is allocated.
 * ------------------------------------------------------------------------
 */
Itcl_ListElem*
Itcl_CreateListElem(
    Itcl_List *listPtr)     /* list that will contain this new element */
{
    Itcl_ListElem *elemPtr;

    if (listPoolLen > 0) {
        elemPtr = listPool;
        listPool = elemPtr->next;
        --listPoolLen;
    } else {
        elemPtr = (Itcl_ListElem*)ckalloc((unsigned)sizeof(Itcl_ListElem));
    }
    elemPtr->owner = listPtr;
    elemPtr->value = NULL;
    elemPtr->next  = NULL;
    elemPtr->prev  = NULL;

    return elemPtr;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_DeleteListElem()
 *
 *  Destroys a single element in a linked list, returning it to a pool of
 *  elements that can be later reused.  Returns a pointer to the next
 *  element in the list.
 * ------------------------------------------------------------------------
 */
Itcl_ListElem*
Itcl_DeleteListElem(
    Itcl_ListElem *elemPtr)     /* list element to be deleted */
{
    Itcl_List *listPtr;
    Itcl_ListElem *nextPtr;

    nextPtr = elemPtr->next;

    if (elemPtr->prev) {
        elemPtr->prev->next = elemPtr->next;
    }
    if (elemPtr->next) {
        elemPtr->next->prev = elemPtr->prev;
    }

    listPtr = elemPtr->owner;
    if (elemPtr == listPtr->head) {
        listPtr->head = elemPtr->next;
    }
    if (elemPtr == listPtr->tail) {
        listPtr->tail = elemPtr->prev;
    }
    --listPtr->num;

    if (listPoolLen < ITCL_LIST_POOL_SIZE) {
        elemPtr->next = listPool;
        listPool = elemPtr;
        ++listPoolLen;
    } else {
        ckfree((char*)elemPtr);
    }
    return nextPtr;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_InsertList()
 *
 *  Creates a new list element containing the given value and returns
 *  a pointer to it.  The element is inserted at the beginning of the
 *  specified list.
 * ------------------------------------------------------------------------
 */
Itcl_ListElem*
Itcl_InsertList(
    Itcl_List *listPtr,     /* list being modified */
    ClientData val)         /* value associated with new element */
{
    Itcl_ListElem *elemPtr;
    assert(listPtr->validate == ITCL_VALID_LIST);

    elemPtr = Itcl_CreateListElem(listPtr);

    elemPtr->value = val;
    elemPtr->next  = listPtr->head;
    elemPtr->prev  = NULL;
    if (listPtr->head) {
        listPtr->head->prev = elemPtr;
    }
    listPtr->head  = elemPtr;
    if (listPtr->tail == NULL) {
        listPtr->tail = elemPtr;
    }
    ++listPtr->num;

    return elemPtr;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_InsertListElem()
 *
 *  Creates a new list element containing the given value and returns
 *  a pointer to it.  The element is inserted in the list just before
 *  the specified element.
 * ------------------------------------------------------------------------
 */
Itcl_ListElem*
Itcl_InsertListElem(
    Itcl_ListElem *pos,     /* insert just before this element */
    ClientData val)         /* value associated with new element */
{
    Itcl_List *listPtr;
    Itcl_ListElem *elemPtr;

    listPtr = pos->owner;
    assert(listPtr->validate == ITCL_VALID_LIST);
    assert(pos != NULL);

    elemPtr = Itcl_CreateListElem(listPtr);
    elemPtr->value = val;

    elemPtr->prev = pos->prev;
    if (elemPtr->prev) {
        elemPtr->prev->next = elemPtr;
    }
    elemPtr->next = pos;
    pos->prev     = elemPtr;

    if (listPtr->head == pos) {
        listPtr->head = elemPtr;
    }
    if (listPtr->tail == NULL) {
        listPtr->tail = elemPtr;
    }
    ++listPtr->num;

    return elemPtr;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_AppendList()
 *
 *  Creates a new list element containing the given value and returns
 *  a pointer to it.  The element is appended at the end of the
 *  specified list.
 * ------------------------------------------------------------------------
 */
Itcl_ListElem*
Itcl_AppendList(
    Itcl_List *listPtr,     /* list being modified */
    ClientData val)         /* value associated with new element */
{
    Itcl_ListElem *elemPtr;
    assert(listPtr->validate == ITCL_VALID_LIST);

    elemPtr = Itcl_CreateListElem(listPtr);

    elemPtr->value = val;
    elemPtr->prev  = listPtr->tail;
    elemPtr->next  = NULL;
    if (listPtr->tail) {
        listPtr->tail->next = elemPtr;
    }
    listPtr->tail  = elemPtr;
    if (listPtr->head == NULL) {
        listPtr->head = elemPtr;
    }
    ++listPtr->num;

    return elemPtr;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_AppendListElem()
 *
 *  Creates a new list element containing the given value and returns
 *  a pointer to it.  The element is inserted in the list just after
 *  the specified element.
 * ------------------------------------------------------------------------
 */
Itcl_ListElem*
Itcl_AppendListElem(
    Itcl_ListElem *pos,     /* insert just after this element */
    ClientData val)         /* value associated with new element */
{
    Itcl_List *listPtr;
    Itcl_ListElem *elemPtr;

    listPtr = pos->owner;
    assert(listPtr->validate == ITCL_VALID_LIST);
    assert(pos != NULL);

    elemPtr = Itcl_CreateListElem(listPtr);
    elemPtr->value = val;

    elemPtr->next = pos->next;
    if (elemPtr->next) {
        elemPtr->next->prev = elemPtr;
    }
    elemPtr->prev = pos;
    pos->next     = elemPtr;

    if (listPtr->tail == pos) {
        listPtr->tail = elemPtr;
    }
    if (listPtr->head == NULL) {
        listPtr->head = elemPtr;
    }
    ++listPtr->num;

    return elemPtr;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_SetListValue()
 *
 *  Modifies the value associated with a list element.
 * ------------------------------------------------------------------------
 */
void
Itcl_SetListValue(
    Itcl_ListElem *elemPtr, /* list element being modified */
    ClientData val)         /* new value associated with element */
{
    assert(elemPtr != NULL);
    assert(elemPtr->owner->validate == ITCL_VALID_LIST);
    elemPtr->value = val;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_FinishList()
 *
 *  free all memory used in the list pool
 * ------------------------------------------------------------------------
 */
void
Itcl_FinishList()
{
    Itcl_ListElem *listPtr;
    Itcl_ListElem *elemPtr;

    listPtr = listPool;
    while (listPtr != NULL) {
        elemPtr = listPtr;
	listPtr = elemPtr->next;
	ckfree((char *)elemPtr);
        elemPtr = NULL;
    }
    listPool = NULL;
    listPoolLen = 0;
}


/*
 * ========================================================================
 *  REFERENCE-COUNTED DATA
 *
 *  The following procedures manage generic reference-counted data.
 *  They are similar in spirit to the Tcl_Preserve/Tcl_Release
 *  procedures defined in the Tcl/Tk core.  But these procedures attach a
 *  refcount directly to the allocated memory, and then use it to govern
 *  shared access and properly timed release.
 */

typedef struct PresMemoryPrefix {
    Tcl_FreeProc *freeProc;     /* called by last Itcl_ReleaseData */
    size_t refCount;            /* refernce (resp preserving) counter */
} PresMemoryPrefix;

/*
 * ------------------------------------------------------------------------
 *  Itcl_EventuallyFree()
 *
 *  Asscociates with cdata (allocated by Itcl_Alloc()) a routine to
 *  be called when cdata should be freed. This routine will be called
 *  when the number of Itcl_ReleaseData() calls on cdata  matches the
 *  number of Itcl_PreserveData() calls on cdata.
 * ------------------------------------------------------------------------
 */
void
Itcl_EventuallyFree(
    ClientData cdata,          /* data to be freed when not in use */
    Tcl_FreeProc *fproc)       /* procedure called to free data */
{
    PresMemoryPrefix *blk;

    if (cdata == NULL) {
        return;
    }

    /* Itcl memory block to ckalloc block */
    blk = ((PresMemoryPrefix *)cdata)-1;

    /* Set new free proc */
    blk->freeProc = fproc;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_PreserveData()
 *
 *  Increases the usage count for a piece of data that will be freed
 *  later when no longer needed.  Each call to Itcl_PreserveData()
 *  puts one claim on a piece of data, and subsequent calls to
 *  Itcl_ReleaseData() remove those claims.  When Itcl_EventuallyFree()
 *  is called, and when the usage count reaches zero, the data is
 *  freed.
 * ------------------------------------------------------------------------
 */
void
Itcl_PreserveData(
    ClientData cdata)     /* data to be preserved */
{
    PresMemoryPrefix *blk;

    if (cdata == NULL) {
        return;
    }

    /* Itcl memory block to ckalloc block */
    blk = ((PresMemoryPrefix *)cdata)-1;

    /* Increment preservation count */
    ++blk->refCount;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ReleaseData()
 *
 *  Decreases the usage count for a piece of data that was registered
 *  previously via Itcl_PreserveData().  After Itcl_EventuallyFree()
 *  is called and the usage count reaches zero, the data is
 *  automatically freed.
 * ------------------------------------------------------------------------
 */
void
Itcl_ReleaseData(
    ClientData cdata)      /* data to be released */
{
    PresMemoryPrefix *blk;
    Tcl_FreeProc *freeProc;

    if (cdata == NULL) {
        return;
    }

    /* Itcl memory block to ckalloc block */
    blk = ((PresMemoryPrefix *)cdata)-1;

    /* Usage sanity check */
    assert(blk->refCount != 0); /* must call Itcl_PreserveData() first */
    assert(blk->freeProc);	/* must call Itcl_EventuallyFree() first */

    /* Decrement preservation count */
    if (--blk->refCount) {
	return;
    }

    /* Free cdata now */
    freeProc = blk->freeProc;
    blk->freeProc = NULL;
    freeProc(cdata);
}

/*
 * ------------------------------------------------------------------------
 * Itcl_Alloc()
 *
 *	Allocate preservable memory. In opposite to ckalloc the result can be
 *	supplied to preservation facilities of Itcl (Itcl_PreserveData).
 *
 * Results:
 *	Pointer to new allocated memory.
 * ------------------------------------------------------------------------
 */
void * Itcl_Alloc(
    size_t size)	/* Size of memory to allocate */
{
    size_t numBytes;
    PresMemoryPrefix *blk;

    /* The ckalloc() in Tcl 8 can alloc at most UINT_MAX bytes */
    assert (size <= UINT_MAX - sizeof(PresMemoryPrefix));
    numBytes = size + sizeof(PresMemoryPrefix);

    /* This will panic on allocation failure. No need to check return value. */
    blk = (PresMemoryPrefix *)ckalloc(numBytes);

    /* Itcl_Alloc defined to zero-init memory it allocates */
    memset(blk, 0, numBytes);

    /* ckalloc block to Itcl memory block */
    return blk+1;
}
/*
 * ------------------------------------------------------------------------
 * ItclFree()
 *
 *	Release memory allocated by Itcl_Alloc() that was never preserved.
 *
 * Results:
 *	None.
 *
 * ------------------------------------------------------------------------
 */
void Itcl_Free(void *ptr) {
    PresMemoryPrefix *blk;
    
    if (ptr == NULL) {
	return;
    }
    /* Itcl memory block to ckalloc block */
    blk = ((PresMemoryPrefix *)ptr)-1;

    assert(blk->refCount == 0); /* it should be not preserved */
    assert(blk->freeProc == NULL); /* it should be released */
    ckfree(blk);
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_SaveInterpState()
 *
 *  Takes a snapshot of the current result state of the interpreter.
 *  The snapshot can be restored at any point by Itcl_RestoreInterpState.
 *  So if you are in the middle of building a return result, you can
 *  snapshot the interpreter, execute a command that might generate an
 *  error, restore the snapshot, and continue building the result string.
 *
 *  Once a snapshot is saved, it must be restored by calling
 *  Itcl_RestoreInterpState, or discarded by calling
 *  Itcl_DiscardInterpState.  Otherwise, memory will be leaked.
 *
 *  Returns a token representing the state of the interpreter.
 * ------------------------------------------------------------------------
 */
Itcl_InterpState
Itcl_SaveInterpState(
    Tcl_Interp* interp,     /* interpreter being modified */
    int status)             /* integer status code for current operation */
{
    return (Itcl_InterpState) Tcl_SaveInterpState(interp, status);
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_RestoreInterpState()
 *
 *  Restores the state of the interpreter to a snapshot taken by
 *  Itcl_SaveInterpState.  This affects variables such as "errorInfo"
 *  and "errorCode".  After this call, the token for the interpreter
 *  state is no longer valid.
 *
 *  Returns the status code that was pending at the time the state was
 *  captured.
 * ------------------------------------------------------------------------
 */
int
Itcl_RestoreInterpState(
    Tcl_Interp* interp,       /* interpreter being modified */
    Itcl_InterpState state)   /* token representing interpreter state */
{
    return Tcl_RestoreInterpState(interp, (Tcl_InterpState)state);
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_DiscardInterpState()
 *
 *  Frees the memory associated with an interpreter snapshot taken by
 *  Itcl_SaveInterpState.  If the snapshot is not restored, this
 *  procedure must be called to discard it, or the memory will be lost.
 *  After this call, the token for the interpreter state is no longer
 *  valid.
 * ------------------------------------------------------------------------
 */
void
Itcl_DiscardInterpState(
    Itcl_InterpState state)  /* token representing interpreter state */
{
    Tcl_DiscardInterpState((Tcl_InterpState)state);
    return;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_Protection()
 *
 *  Used to query/set the protection level used when commands/variables
 *  are defined within a class.  The default protection level (when
 *  no public/protected/private command is active) is ITCL_DEFAULT_PROTECT.
 *  In the default case, new commands are treated as public, while new
 *  variables are treated as protected.
 *
 *  If the specified level is 0, then this procedure returns the
 *  current value without changing it.  Otherwise, it sets the current
 *  value to the specified protection level, and returns the previous
 *  value.
 * ------------------------------------------------------------------------
 */
int
Itcl_Protection(
    Tcl_Interp *interp,  /* interpreter being queried */
    int newLevel)        /* new protection level or 0 */
{
    int oldVal;
    ItclObjectInfo *infoPtr;

    /*
     *  If a new level was specified, then set the protection level.
     *  In any case, return the protection level as it stands right now.
     */
    infoPtr = (ItclObjectInfo*) Tcl_GetAssocData(interp, ITCL_INTERP_DATA,
        NULL);

    assert(infoPtr != NULL);
    oldVal = infoPtr->protection;

    if (newLevel != 0) {
        assert(newLevel == ITCL_PUBLIC ||
            newLevel == ITCL_PROTECTED ||
            newLevel == ITCL_PRIVATE ||
            newLevel == ITCL_DEFAULT_PROTECT);
        infoPtr->protection = newLevel;
    }
    return oldVal;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ParseNamespPath()
 *
 *  Parses a reference to a namespace element of the form:
 *
 *      namesp::namesp::namesp::element
 *
 *  Returns pointers to the head part ("namesp::namesp::namesp")
 *  and the tail part ("element").  If the head part is missing,
 *  a NULL pointer is returned and the rest of the string is taken
 *  as the tail.
 *
 *  Both head and tail point to locations within the given dynamic
 *  string buffer.  This buffer must be uninitialized when passed
 *  into this procedure, and it must be freed later on, when the
 *  strings are no longer needed.
 * ------------------------------------------------------------------------
 */
void
Itcl_ParseNamespPath(
    const char *name,    /* path name to class member */
    Tcl_DString *buffer, /* dynamic string buffer (uninitialized) */
    const char **head,   /* returns "namesp::namesp::namesp" part */
    const char **tail)   /* returns "element" part */
{
    char *sep, *newname;

    Tcl_DStringInit(buffer);

    /*
     *  Copy the name into the buffer and parse it.  Look
     *  backward from the end of the string to the first '::'
     *  scope qualifier.
     */
    Tcl_DStringAppend(buffer, name, -1);
    newname = Tcl_DStringValue(buffer);

    for (sep=newname; *sep != '\0'; sep++)
        ;

    while (--sep > newname) {
        if (*sep == ':' && *(sep-1) == ':') {
            break;
        }
    }

    /*
     *  Found head/tail parts.  If there are extra :'s, keep backing
     *  up until the head is found.  This supports the Tcl namespace
     *  behavior, which allows names like "foo:::bar".
     */
    if (sep > newname) {
        *tail = sep+1;
        while (sep > newname && *(sep-1) == ':') {
            sep--;
        }
        *sep = '\0';
        *head = newname;
    } else {

        /*
         *  No :: separators--the whole name is treated as a tail.
         */
        *tail = newname;
        *head = NULL;
    }
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_CanAccess2()
 *
 *  Checks to see if a class member can be accessed from a particular
 *  namespace context.  Public things can always be accessed.  Protected
 *  things can be accessed if the "from" namespace appears in the
 *  inheritance hierarchy of the class namespace.  Private things
 *  can be accessed only if the "from" namespace is the same as the
 *  class that contains them.
 *
 *  Returns 1/0 indicating true/false.
 * ------------------------------------------------------------------------
 */
int
Itcl_CanAccess2(
    ItclClass *iclsPtr,        /* class being tested */
    int protection,            /* protection level being tested */
    Tcl_Namespace* fromNsPtr)  /* namespace requesting access */
{
    ItclClass* fromIclsPtr;
    Tcl_HashEntry *entry;

    /*
     *  If the protection level is "public" or "private", then the
     *  answer is known immediately.
     */
    if (protection == ITCL_PUBLIC) {
        return 1;
    } else {
        if (protection == ITCL_PRIVATE) {
	    entry = Tcl_FindHashEntry(&iclsPtr->infoPtr->namespaceClasses,
		fromNsPtr);
	    if (entry == NULL) {
		return 0;
	    }
	    return (iclsPtr == Tcl_GetHashValue(entry));
        }
    }

    /*
     *  If the protection level is "protected", then check the
     *  heritage of the namespace requesting access.  If cdefnPtr
     *  is in the heritage, then access is allowed.
     */
    assert (protection == ITCL_PROTECTED);

    if (Itcl_IsClassNamespace(fromNsPtr)) {
	entry = Tcl_FindHashEntry(&iclsPtr->infoPtr->namespaceClasses,
		fromNsPtr);
	if (entry == NULL) {
	    return 0;
	}
	fromIclsPtr = (ItclClass *)Tcl_GetHashValue(entry);

        entry = Tcl_FindHashEntry(&fromIclsPtr->heritage,
	        (char*)iclsPtr);

        if (entry) {
            return 1;
        }
    }
    return 0;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_CanAccess()
 *
 *  Checks to see if a class member can be accessed from a particular
 *  namespace context.  Public things can always be accessed.  Protected
 *  things can be accessed if the "from" namespace appears in the
 *  inheritance hierarchy of the class namespace.  Private things
 *  can be accessed only if the "from" namespace is the same as the
 *  class that contains them.
 *
 *  Returns 1/0 indicating true/false.
 * ------------------------------------------------------------------------
 */
int
Itcl_CanAccess(
    ItclMemberFunc* imPtr,     /* class member being tested */
    Tcl_Namespace* fromNsPtr)  /* namespace requesting access */
{
    return Itcl_CanAccess2(imPtr->iclsPtr, imPtr->protection, fromNsPtr);
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_CanAccessFunc()
 *
 *  Checks to see if a member function with the specified protection
 *  level can be accessed from a particular namespace context.  This
 *  follows the same rules enforced by Itcl_CanAccess, but adds one
 *  special case:  If the function is a protected method, and if the
 *  current context is a base class that has the same method, then
 *  access is allowed.
 *
 *  Returns 1/0 indicating true/false.
 * ------------------------------------------------------------------------
 */
int
Itcl_CanAccessFunc(
    ItclMemberFunc* imPtr,     /* member function being tested */
    Tcl_Namespace* fromNsPtr)  /* namespace requesting access */
{
    ItclClass *iclsPtr;
    ItclClass *fromIclsPtr;
    ItclMemberFunc *ovlfunc;
    Tcl_HashEntry *entry;

    /*
     *  Apply the usual rules first.
     */
    if (Itcl_CanAccess(imPtr, fromNsPtr)) {
        return 1;
    }

    /*
     *  As a last resort, see if the namespace is really a base
     *  class of the class containing the method.  Look for a
     *  method with the same name in the base class.  If there
     *  is one, then this method overrides it, and the base class
     *  has access.
     */
    if ((imPtr->flags & ITCL_COMMON) == 0 &&
            Itcl_IsClassNamespace(fromNsPtr)) {
        Tcl_HashEntry *hPtr;

        iclsPtr = imPtr->iclsPtr;
	hPtr = Tcl_FindHashEntry(&iclsPtr->infoPtr->namespaceClasses,
	        (char *)fromNsPtr);
	if (hPtr == NULL) {
	    return 0;
	}
        fromIclsPtr = (ItclClass *)Tcl_GetHashValue(hPtr);

        if (Tcl_FindHashEntry(&iclsPtr->heritage, (char*)fromIclsPtr)) {
            entry = Tcl_FindHashEntry(&fromIclsPtr->resolveCmds,
                (char *)imPtr->namePtr);

            if (entry) {
		ItclCmdLookup *clookup;
		clookup = (ItclCmdLookup *)Tcl_GetHashValue(entry);
		ovlfunc = clookup->imPtr;
                if ((ovlfunc->flags & ITCL_COMMON) == 0 &&
                     ovlfunc->protection < ITCL_PRIVATE) {
                    return 1;
                }
            }
        }
    }
    return 0;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_DecodeScopedCommand()
 *
 *  Decodes a scoped command of the form:
 *
 *      namespace inscope <namesp> <command>
 *
 *  If the given string is not a scoped value, this procedure does
 *  nothing and returns TCL_OK.  If the string is a scoped value,
 *  then it is decoded, and the namespace, and the simple command
 *  string are returned as arguments; the simple command should
 *  be freed when no longer in use.  If anything goes wrong, this
 *  procedure returns TCL_ERROR, along with an error message in
 *  the interpreter.
 * ------------------------------------------------------------------------
 */
int
Itcl_DecodeScopedCommand(
    Tcl_Interp *interp,		/* current interpreter */
    const char *name,		/* string to be decoded */
    Tcl_Namespace **rNsPtr,	/* returns: namespace for scoped value */
    char **rCmdPtr)		/* returns: simple command word */
{
    Tcl_Namespace *nsPtr;
    char *cmdName;
    const char *pos;
    const char **listv;
    int listc;
    int result;
    int len;

    nsPtr = NULL;
    len = strlen(name);
    cmdName = (char *)ckalloc(strlen(name)+1);
    strcpy(cmdName, name);

    if ((*name == 'n') && (len > 17) && (strncmp(name, "namespace", 9) == 0)) {
	for (pos = (name + 9);  (*pos == ' ');  pos++) {
	    /* empty body: skip over spaces */
	}
	if ((*pos == 'i') && ((pos + 7) <= (name + len))
	        && (strncmp(pos, "inscope", 7) == 0)) {

            result = Tcl_SplitList(interp, (const char *)name, &listc,
		    &listv);
            if (result == TCL_OK) {
                if (listc != 4) {
                    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                        "malformed command \"", name, "\": should be \"",
                        "namespace inscope namesp command\"",
                        NULL);
                    result = TCL_ERROR;
                } else {
                    nsPtr = Tcl_FindNamespace(interp, listv[2],
                        NULL, TCL_LEAVE_ERR_MSG);

                    if (nsPtr == NULL) {
                        result = TCL_ERROR;
                    } else {
		        ckfree(cmdName);
                        cmdName = (char *)ckalloc(strlen(listv[3])+1);
                        strcpy(cmdName, listv[3]);
                    }
                }
            }
            ckfree((char*)listv);

            if (result != TCL_OK) {
                Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
                        "\n    (while decoding scoped command \"%s\")",
                        name));
		ckfree(cmdName);
                return TCL_ERROR;
            }
	}
    }

    *rNsPtr = nsPtr;
    *rCmdPtr = cmdName;
    return TCL_OK;
}
