/*
 * tclIORTrans.c --
 *
 *	This file contains the implementation of Tcl's generic transformation
 *	reflection code, which allows the implementation of Tcl channel
 *	transformations in Tcl code.
 *
 *	Parts of this file are based on code contributed by Jean-Claude
 *	Wippler.
 *
 *	See TIP #230 for the specification of this functionality.
 *
 * Copyright (c) 2007-2008 ActiveState.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclIO.h"
#include <assert.h>

#ifndef EINVAL
#define EINVAL	9
#endif
#ifndef EOK
#define EOK	0
#endif

/* DUPLICATE of HaveVersion() in tclIO.c // TODO - MODULE_SCOPE */
static int		HaveVersion(const Tcl_ChannelType *typePtr,
			    Tcl_ChannelTypeVersion minimumVersion);

/*
 * Signatures of all functions used in the C layer of the reflection.
 */

static int		ReflectClose(ClientData clientData,
			    Tcl_Interp *interp);
static int		ReflectInput(ClientData clientData, char *buf,
			    int toRead, int *errorCodePtr);
static int		ReflectOutput(ClientData clientData, const char *buf,
			    int toWrite, int *errorCodePtr);
static void		ReflectWatch(ClientData clientData, int mask);
static int		ReflectBlock(ClientData clientData, int mode);
static Tcl_WideInt	ReflectSeekWide(ClientData clientData,
			    Tcl_WideInt offset, int mode, int *errorCodePtr);
static int		ReflectSeek(ClientData clientData, long offset,
			    int mode, int *errorCodePtr);
static int		ReflectGetOption(ClientData clientData,
			    Tcl_Interp *interp, const char *optionName,
			    Tcl_DString *dsPtr);
static int		ReflectSetOption(ClientData clientData,
			    Tcl_Interp *interp, const char *optionName,
			    const char *newValue);
static int		ReflectHandle(ClientData clientData, int direction,
			    ClientData *handle);
static int		ReflectNotify(ClientData clientData, int mask);

/*
 * The C layer channel type/driver definition used by the reflection.
 */

static const Tcl_ChannelType tclRTransformType = {
    "tclrtransform",		/* Type name. */
    TCL_CHANNEL_VERSION_5,	/* v5 channel. */
    ReflectClose,		/* Close channel, clean instance data. */
    ReflectInput,		/* Handle read request. */
    ReflectOutput,		/* Handle write request. */
    ReflectSeek,		/* Move location of access point. */
    ReflectSetOption,		/* Set options. */
    ReflectGetOption,		/* Get options. */
    ReflectWatch,		/* Initialize notifier. */
    ReflectHandle,		/* Get OS handle from the channel. */
    NULL,			/* No close2 support. NULL'able. */
    ReflectBlock,		/* Set blocking/nonblocking. */
    NULL,			/* Flush channel. Not used by core.
				 * NULL'able. */
    ReflectNotify,		/* Handle events. */
    ReflectSeekWide,		/* Move access point (64 bit). */
    NULL,			/* thread action */
    NULL			/* truncate */
};

/*
 * Structure of the buffer to hold transform results to be consumed by higher
 * layers upon reading from the channel, plus the functions to manage such.
 */

typedef struct _ResultBuffer_ {
    unsigned char *buf;		/* Reference to the buffer area. */
    int allocated;		/* Allocated size of the buffer area. */
    int used;			/* Number of bytes in the buffer,
				 * <= allocated. */
} ResultBuffer;

#define ResultLength(r) ((r)->used)
/* static int		ResultLength(ResultBuffer *r); */

static void		ResultClear(ResultBuffer *r);
static void		ResultInit(ResultBuffer *r);
static void		ResultAdd(ResultBuffer *r, unsigned char *buf,
			    int toWrite);
static int		ResultCopy(ResultBuffer *r, unsigned char *buf,
			    int toRead);

#define RB_INCREMENT (512)

/*
 * Convenience macro to make some casts easier to use.
 */

#define UCHARP(x)	((unsigned char *) (x))

/*
 * Instance data for a reflected transformation. ===========================
 */

typedef struct {
    Tcl_Channel chan;		/* Back reference to the channel of the
				 * transformation itself. */
    Tcl_Channel parent;		/* Reference to the channel the transformation
				 * was pushed on. */
    Tcl_Interp *interp;		/* Reference to the interpreter containing the
				 * Tcl level part of the channel. */
    Tcl_Obj *handle;		/* Reference to transform handle. Also stored
				 * in the argv, see below. The separate field
				 * gives us direct access, needed when working
				 * with the reflection maps. */
#ifdef TCL_THREADS
    Tcl_ThreadId thread;	/* Thread the 'interp' belongs to. */
#endif

    Tcl_TimerToken timer;

    /* See [==] as well.
     * Storage for the command prefix and the additional words required for
     * the invocation of methods in the command handler.
     *
     * argv [0] ... [.] | [argc-2] [argc-1] | [argc]  [argc+2]
     *      cmd ... pfx | method   chan     | detail1 detail2
     *      ~~~~ CT ~~~            ~~ CT ~~
     *
     * CT = Belongs to the 'Command handler Thread'.
     */

    int argc;			/* Number of preallocated words - 2. */
    Tcl_Obj **argv;		/* Preallocated array for calling the handler.
				 * args[0] is placeholder for cmd word.
				 * Followed by the arguments in the prefix,
				 * plus 4 placeholders for method, channel,
				 * and at most two varying (method specific)
				 * words. */
    int methods;		/* Bitmask of supported methods. */

    /*
     * NOTE (9): Should we have predefined shared literals for the method
     * names?
     */

    int mode;			/* Mask of R/W mode */
    int nonblocking;		/* Flag: Channel is blocking or not. */
    int readIsDrained;		/* Flag: Read buffers are flushed. */
    int eofPending;		/* Flag: EOF seen down, but not raised up */
    int dead;			/* Boolean signal that some operations
				 * should no longer be attempted. */
    ResultBuffer result;
} ReflectedTransform;

/*
 * Structure of the table mapping from transform handles to reflected
 * transform (channels). Each interpreter which has the handler command for
 * one or more reflected transforms records them in such a table, so that we
 * are able to find them during interpreter/thread cleanup even if the actual
 * channel they belong to was moved to a different interpreter and/or thread.
 *
 * The table is reachable via the standard interpreter AssocData, the key is
 * defined below.
 */

typedef struct {
    Tcl_HashTable map;
} ReflectedTransformMap;

#define RTMKEY "ReflectedTransformMap"

/*
 * Method literals. ==================================================
 */

static const char *const methodNames[] = {
    "clear",		/* OPT */
    "drain",		/* OPT, drain => read */
    "finalize",		/*     */
    "flush",		/* OPT, flush => write */
    "initialize",	/*     */
    "limit?",		/* OPT */
    "read",		/* OPT */
    "write",		/* OPT */
    NULL
};
typedef enum {
    METH_CLEAR,
    METH_DRAIN,
    METH_FINAL,
    METH_FLUSH,
    METH_INIT,
    METH_LIMIT,
    METH_READ,
    METH_WRITE
} MethodName;

#define FLAG(m) (1 << (m))
#define REQUIRED_METHODS \
	(FLAG(METH_INIT) | FLAG(METH_FINAL))
#define RANDW \
	(TCL_READABLE | TCL_WRITABLE)

#define IMPLIES(a,b)	((!(a)) || (b))
#define NEGIMPL(a,b)
#define HAS(x,f)	(x & FLAG(f))

#ifdef TCL_THREADS
/*
 * Thread specific types and structures.
 *
 * We are here essentially creating a very specific implementation of 'thread
 * send'.
 */

/*
 * Enumeration of all operations which can be forwarded.
 */

typedef enum {
    ForwardedClear,
    ForwardedClose,
    ForwardedDrain,
    ForwardedFlush,
    ForwardedInput,
    ForwardedLimit,
    ForwardedOutput
} ForwardedOperation;

/*
 * Event used to forward driver invocations to the thread actually managing
 * the channel. We cannot construct the command to execute and forward that.
 * Because then it will contain a mixture of Tcl_Obj's belonging to both the
 * command handler thread (CT), and the thread managing the channel (MT),
 * executed in CT. Tcl_Obj's are not allowed to cross thread boundaries. So we
 * forward an operation code, the argument details, and reference to results.
 * The command is assembled in the CT and belongs fully to that thread. No
 * sharing problems.
 */

typedef struct ForwardParamBase {
    int code;			/* O: Ok/Fail of the cmd handler */
    char *msgStr;		/* O: Error message for handler failure */
    int mustFree;		/* O: True if msgStr is allocated, false if
				 * otherwise (static). */
} ForwardParamBase;

/*
 * Operation specific parameter/result structures. (These are "subtypes" of
 * ForwardParamBase. Where an operation does not need any special types, it
 * has no "subtype" and just uses ForwardParamBase, as listed above.)
 */

struct ForwardParamTransform {
    ForwardParamBase base;	/* "Supertype". MUST COME FIRST. */
    char *buf;			/* I: Bytes to transform,
				 * O: Bytes in transform result */
    int size;			/* I: #bytes to transform,
				 * O: #bytes in the transform result */
};
struct ForwardParamLimit {
    ForwardParamBase base;	/* "Supertype". MUST COME FIRST. */
    int max;			/* O: Character read limit */
};

/*
 * Now join all these together in a single union for convenience.
 */

typedef union ForwardParam {
    ForwardParamBase base;
    struct ForwardParamTransform transform;
    struct ForwardParamLimit limit;
} ForwardParam;

/*
 * Forward declaration.
 */

typedef struct ForwardingResult ForwardingResult;

/*
 * General event structure, with reference to operation specific data.
 */

typedef struct ForwardingEvent {
    Tcl_Event event;		/* Basic event data, has to be first item */
    ForwardingResult *resultPtr;
    ForwardedOperation op;	/* Forwarded driver operation */
    ReflectedTransform *rtPtr;	/* Channel instance */
    ForwardParam *param;	/* Packaged arguments and return values, a
				 * ForwardParam pointer. */
} ForwardingEvent;

/*
 * Structure to manage the result of the forwarding. This is not the result of
 * the operation itself, but about the success of the forward event itself.
 * The event can be successful, even if the operation which was forwarded
 * failed. It is also there to manage the synchronization between the involved
 * threads.
 */

struct ForwardingResult {
    Tcl_ThreadId src;		/* Originating thread. */
    Tcl_ThreadId dst;		/* Thread the op was forwarded to. */
    Tcl_Interp *dsti;		/* Interpreter in the thread the op was
				 * forwarded to. */
    Tcl_Condition done;		/* Condition variable the forwarder blocks
				 * on. */
    int result;			/* TCL_OK or TCL_ERROR */
    ForwardingEvent *evPtr;	/* Event the result belongs to. */
    ForwardingResult *prevPtr, *nextPtr;
				/* Links into the list of pending forwarded
				 * results. */
};

typedef struct ThreadSpecificData {
    /*
     * Table of all reflected transformations owned by this thread.
     */

    ReflectedTransformMap *rtmPtr;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * List of forwarded operations which have not completed yet, plus the mutex
 * to protect the access to this process global list.
 */

static ForwardingResult *forwardList = NULL;
TCL_DECLARE_MUTEX(rtForwardMutex)

/*
 * Function containing the generic code executing a forward, and wrapper
 * macros for the actual operations we wish to forward. Uses ForwardProc as
 * the event function executed by the thread receiving a forwarding event
 * (which executes the appropriate function and collects the result, if any).
 *
 * The two ExitProcs are handlers so that things do not deadlock when either
 * thread involved in the forwarding exits. They also clean things up so that
 * we don't leak resources when threads go away.
 */

static void		ForwardOpToOwnerThread(ReflectedTransform *rtPtr,
			    ForwardedOperation op, const void *param);
static int		ForwardProc(Tcl_Event *evPtr, int mask);
static void		SrcExitProc(ClientData clientData);

#define FreeReceivedError(p) \
	do {								\
	    if ((p)->base.mustFree) {					\
		ckfree((p)->base.msgStr);				\
	    }								\
	} while (0)
#define PassReceivedErrorInterp(i,p) \
	do {								\
	    if ((i) != NULL) {						\
		Tcl_SetChannelErrorInterp((i),				\
			Tcl_NewStringObj((p)->base.msgStr, -1));	\
	    }								\
	    FreeReceivedError(p);					\
	} while (0)
#define PassReceivedError(c,p) \
	do {								\
	    Tcl_SetChannelError((c),					\
		    Tcl_NewStringObj((p)->base.msgStr, -1));		\
	    FreeReceivedError(p);					\
	} while (0)
#define ForwardSetStaticError(p,emsg) \
	do {								\
	    (p)->base.code = TCL_ERROR;					\
	    (p)->base.mustFree = 0;					\
	    (p)->base.msgStr = (char *) (emsg);				\
	} while (0)
#define ForwardSetDynamicError(p,emsg) \
	do {								\
	    (p)->base.code = TCL_ERROR;					\
	    (p)->base.mustFree = 1;					\
	    (p)->base.msgStr = (char *) (emsg);				\
	} while (0)

static void		ForwardSetObjError(ForwardParam *p,
			    Tcl_Obj *objPtr);
static ReflectedTransformMap *	GetThreadReflectedTransformMap(void);
static void		DeleteThreadReflectedTransformMap(
			    ClientData clientData);
#endif /* TCL_THREADS */

#define SetChannelErrorStr(c,msgStr) \
	Tcl_SetChannelError((c), Tcl_NewStringObj((msgStr), -1))

static Tcl_Obj *	MarshallError(Tcl_Interp *interp);
static void		UnmarshallErrorResult(Tcl_Interp *interp,
			    Tcl_Obj *msgObj);

/*
 * Static functions for this file:
 */

static Tcl_Obj *	DecodeEventMask(int mask);
static ReflectedTransform * NewReflectedTransform(Tcl_Interp *interp,
			    Tcl_Obj *cmdpfxObj, int mode, Tcl_Obj *handleObj,
			    Tcl_Channel parentChan);
static Tcl_Obj *	NextHandle(void);
static void		FreeReflectedTransform(ReflectedTransform *rtPtr);
static void		FreeReflectedTransformArgs(ReflectedTransform *rtPtr);
static int		InvokeTclMethod(ReflectedTransform *rtPtr,
			    const char *method, Tcl_Obj *argOneObj,
			    Tcl_Obj *argTwoObj, Tcl_Obj **resultObjPtr);

static ReflectedTransformMap *	GetReflectedTransformMap(Tcl_Interp *interp);
static void		DeleteReflectedTransformMap(ClientData clientData,
			    Tcl_Interp *interp);

/*
 * Global constant strings (messages). ==================
 * These string are used directly as bypass errors, thus they have to be valid
 * Tcl lists where the last element is the message itself. Hence the
 * list-quoting to keep the words of the message together. See also [x].
 */

static const char *msg_read_unsup = "{read not supported by Tcl driver}";
static const char *msg_write_unsup = "{write not supported by Tcl driver}";
#ifdef TCL_THREADS
static const char *msg_send_originlost = "{Channel thread lost}";
static const char *msg_send_dstlost = "{Owner lost}";
#endif /* TCL_THREADS */
static const char *msg_dstlost =
    "-code 1 -level 0 -errorcode NONE -errorinfo {} -errorline 1 {Owner lost}";

/*
 * Timer management (flushing out buffered data via artificial events).
 */

/*
 * Helper functions encapsulating some of the thread forwarding to make the
 * control flow in callers easier.
 */

static void		TimerKill(ReflectedTransform *rtPtr);
static void		TimerSetup(ReflectedTransform *rtPtr);
static void		TimerRun(ClientData clientData);
static int		TransformRead(ReflectedTransform *rtPtr,
			    int *errorCodePtr, Tcl_Obj *bufObj);
static int		TransformWrite(ReflectedTransform *rtPtr,
			    int *errorCodePtr, unsigned char *buf,
			    int toWrite);
static int		TransformDrain(ReflectedTransform *rtPtr,
			    int *errorCodePtr);
static int		TransformFlush(ReflectedTransform *rtPtr,
			    int *errorCodePtr, int op);
static void		TransformClear(ReflectedTransform *rtPtr);
static int		TransformLimit(ReflectedTransform *rtPtr,
			    int *errorCodePtr, int *maxPtr);

/*
 * Operation codes for TransformFlush().
 */

#define FLUSH_WRITE	1
#define FLUSH_DISCARD	0

/*
 * Main methods to plug into the 'chan' ensemble'. ==================
 */

/*
 *----------------------------------------------------------------------
 *
 * TclChanPushObjCmd --
 *
 *	This function is invoked to process the "chan push" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result. The handle of the new channel is placed in the
 *	interp result.
 *
 * Side effects:
 *	Creates a new channel.
 *
 *----------------------------------------------------------------------
 */

int
TclChanPushObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    ReflectedTransform *rtPtr;	/* Instance data of the new (transform)
				 * channel. */
    Tcl_Obj *chanObj;		/* Handle of parent channel */
    Tcl_Channel parentChan;	/* Token of parent channel */
    int mode;			/* R/W mode of parent, later the new channel.
				 * Has to match the abilities of the handler
				 * commands */
    Tcl_Obj *cmdObj;		/* Command prefix, list of words */
    Tcl_Obj *cmdNameObj;	/* Command name */
    Tcl_Obj *rtId;		/* Handle of the new transform (channel) */
    Tcl_Obj *modeObj;		/* mode in obj form for method call */
    int listc;			/* Result of 'initialize', and of */
    Tcl_Obj **listv;		/* its sublist in the 2nd element */
    int methIndex;		/* Encoded method name */
    int result;			/* Result code for 'initialize' */
    Tcl_Obj *resObj;		/* Result data for 'initialize' */
    int methods;		/* Bitmask for supported methods. */
    ReflectedTransformMap *rtmPtr;
				/* Map of reflected transforms with handlers
				 * in this interp. */
    Tcl_HashEntry *hPtr;	/* Entry in the above map */
    int isNew;			/* Placeholder. */

    /*
     * Syntax:   chan push CHANNEL CMDPREFIX
     *           [0]  [1]  [2]     [3]
     *
     * Actually: rPush CHANNEL CMDPREFIX
     *           [0]   [1]     [2]
     */

#define CHAN	(1)
#define CMD	(2)

    /*
     * Number of arguments...
     */

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "channel cmdprefix");
	return TCL_ERROR;
    }

    /*
     * First argument is a channel handle.
     */

    chanObj = objv[CHAN];
    parentChan = Tcl_GetChannel(interp, Tcl_GetString(chanObj), &mode);
    if (parentChan == NULL) {
	return TCL_ERROR;
    }
    parentChan = Tcl_GetTopChannel(parentChan);

    /*
     * Second argument is command prefix, i.e. list of words, first word is
     * name of handler command, other words are fixed arguments. Run the
     * 'initialize' method to get the list of supported methods. Validate
     * this.
     */

    cmdObj = objv[CMD];

    /*
     * Basic check that the command prefix truly is a list.
     */

    if (Tcl_ListObjIndex(interp, cmdObj, 0, &cmdNameObj) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Now create the transformation (channel).
     */

    rtId = NextHandle();
    rtPtr = NewReflectedTransform(interp, cmdObj, mode, rtId, parentChan);

    /*
     * Invoke 'initialize' and validate that the handler is present and ok.
     * Squash the transformation if not.
     */

    modeObj = DecodeEventMask(mode);
    /* assert modeObj.refCount == 1 */
    result = InvokeTclMethod(rtPtr, "initialize", modeObj, NULL, &resObj);
    Tcl_DecrRefCount(modeObj);
    if (result != TCL_OK) {
	UnmarshallErrorResult(interp, resObj);
	Tcl_DecrRefCount(resObj);	/* Remove reference held from invoke */
	goto error;
    }

    /*
     * Verify the result.
     * - List, of method names. Convert to mask. Check for non-optionals
     *   through the mask. Compare open mode against optional r/w.
     */

    if (Tcl_ListObjGetElements(NULL, resObj, &listc, &listv) != TCL_OK) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "chan handler \"%s initialize\" returned non-list: %s",
                Tcl_GetString(cmdObj), Tcl_GetString(resObj)));
	Tcl_DecrRefCount(resObj);
	goto error;
    }

    methods = 0;
    while (listc > 0) {
	if (Tcl_GetIndexFromObj(interp, listv[listc-1], methodNames,
		"method", TCL_EXACT, &methIndex) != TCL_OK) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "chan handler \"%s initialize\" returned %s",
		    Tcl_GetString(cmdObj),
		    Tcl_GetString(Tcl_GetObjResult(interp))));
	    Tcl_DecrRefCount(resObj);
	    goto error;
	}

	methods |= FLAG(methIndex);
	listc--;
    }
    Tcl_DecrRefCount(resObj);

    if ((REQUIRED_METHODS & methods) != REQUIRED_METHODS) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "chan handler \"%s\" does not support all required methods",
                Tcl_GetString(cmdObj)));
	goto error;
    }

    /*
     * Mode tell us what the parent channel supports. The methods tell us what
     * the handler supports. We remove the non-supported bits from the mode
     * and check that the channel is not completely inacessible. Afterward the
     * mode tells us which methods are still required, and these methods will
     * also be supported by the handler, by design of the check.
     */

    if (!HAS(methods, METH_READ)) {
	mode &= ~TCL_READABLE;
    }
    if (!HAS(methods, METH_WRITE)) {
	mode &= ~TCL_WRITABLE;
    }

    if (!mode) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "chan handler \"%s\" makes the channel inaccessible",
                Tcl_GetString(cmdObj)));
	goto error;
    }

    /*
     * The mode and support for it is ok, now check the internal constraints.
     */

    if (!IMPLIES(HAS(methods, METH_DRAIN), HAS(methods, METH_READ))) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "chan handler \"%s\" supports \"drain\" but not \"read\"",
                Tcl_GetString(cmdObj)));
	goto error;
    }

    if (!IMPLIES(HAS(methods, METH_FLUSH), HAS(methods, METH_WRITE))) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "chan handler \"%s\" supports \"flush\" but not \"write\"",
                Tcl_GetString(cmdObj)));
	goto error;
    }

    Tcl_ResetResult(interp);

    /*
     * Everything is fine now.
     */

    rtPtr->methods = methods;
    rtPtr->mode = mode;
    rtPtr->chan = Tcl_StackChannel(interp, &tclRTransformType, rtPtr, mode,
	    rtPtr->parent);

    /*
     * Register the transform in our our map for proper handling of deleted
     * interpreters and/or threads.
     */

    rtmPtr = GetReflectedTransformMap(interp);
    hPtr = Tcl_CreateHashEntry(&rtmPtr->map, Tcl_GetString(rtId), &isNew);
    if (!isNew && rtPtr != Tcl_GetHashValue(hPtr)) {
	Tcl_Panic("TclChanPushObjCmd: duplicate transformation handle");
    }
    Tcl_SetHashValue(hPtr, rtPtr);
#ifdef TCL_THREADS
    rtmPtr = GetThreadReflectedTransformMap();
    hPtr = Tcl_CreateHashEntry(&rtmPtr->map, Tcl_GetString(rtId), &isNew);
    Tcl_SetHashValue(hPtr, rtPtr);
#endif /* TCL_THREADS */

    /*
     * Return the channel as the result of the command.
     */

    Tcl_SetObjResult(interp, Tcl_NewStringObj(
	    Tcl_GetChannelName(rtPtr->chan), -1));
    return TCL_OK;

  error:
    /*
     * We are not going through ReflectClose as we never had a channel
     * structure.
     */

    Tcl_EventuallyFree(rtPtr, (Tcl_FreeProc *) FreeReflectedTransform);
    return TCL_ERROR;

#undef CHAN
#undef CMD
}

/*
 *----------------------------------------------------------------------
 *
 * TclChanPopObjCmd --
 *
 *	This function is invoked to process the "chan pop" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Posts events to a reflected channel, invokes event handlers. The
 *	latter implies that arbitrary side effects are possible.
 *
 *----------------------------------------------------------------------
 */

int
TclChanPopObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    /*
     * Syntax:   chan pop CHANNEL
     *           [0]  [1] [2]
     *
     * Actually: rPop CHANNEL
     *           [0]  [1]
     */

#define CHAN	(1)

    const char *chanId;		/* Tcl level channel handle */
    Tcl_Channel chan;		/* Channel associated to the handle */
    int mode;			/* Channel r/w mode */

    /*
     * Number of arguments...
     */

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "channel");
	return TCL_ERROR;
    }

    /*
     * First argument is a channel, which may have a (reflected)
     * transformation.
     */

    chanId = TclGetString(objv[CHAN]);
    chan = Tcl_GetChannel(interp, chanId, &mode);

    if (chan == NULL) {
	return TCL_ERROR;
    }

    /*
     * Removing transformations is generic, and not restricted to reflected
     * transformations.
     */

    Tcl_UnstackChannel(interp, chan);
    return TCL_OK;

#undef CHAN
}

/*
 * Channel error message marshalling utilities.
 */

static Tcl_Obj *
MarshallError(
    Tcl_Interp *interp)
{
    /*
     * Capture the result status of the interpreter into a string. => List of
     * options and values, followed by the error message. The result has
     * refCount 0.
     */

    Tcl_Obj *returnOpt = Tcl_GetReturnOptions(interp, TCL_ERROR);

    /*
     * => returnOpt.refCount == 0. We can append directly.
     */

    Tcl_ListObjAppendElement(NULL, returnOpt, Tcl_GetObjResult(interp));
    return returnOpt;
}

static void
UnmarshallErrorResult(
    Tcl_Interp *interp,
    Tcl_Obj *msgObj)
{
    int lc;
    Tcl_Obj **lv;
    int explicitResult;
    int numOptions;

    /*
     * Process the caught message.
     *
     * Syntax = (option value)... ?message?
     *
     * Bad syntax causes a panic. This is OK because the other side uses
     * Tcl_GetReturnOptions and list construction functions to marshall the
     * information; if we panic here, something has gone badly wrong already.
     */

    if (Tcl_ListObjGetElements(interp, msgObj, &lc, &lv) != TCL_OK) {
	Tcl_Panic("TclChanCaughtErrorBypass: Bad syntax of caught result");
    }
    if (interp == NULL) {
	return;
    }

    explicitResult = lc & 1;		/* Odd number of values? */
    numOptions = lc - explicitResult;

    if (explicitResult) {
	Tcl_SetObjResult(interp, lv[lc-1]);
    }

    Tcl_SetReturnOptions(interp, Tcl_NewListObj(numOptions, lv));
    ((Interp *) interp)->flags &= ~ERR_ALREADY_LOGGED;
}

/*
 * Driver functions. ================================================
 */

/*
 *----------------------------------------------------------------------
 *
 * ReflectClose --
 *
 *	This function is invoked when the channel is closed, to delete the
 *	driver specific instance data.
 *
 * Results:
 *	A posix error.
 *
 * Side effects:
 *	Releases memory. Arbitrary, as it calls upon a script.
 *
 *----------------------------------------------------------------------
 */

static int
ReflectClose(
    ClientData clientData,
    Tcl_Interp *interp)
{
    ReflectedTransform *rtPtr = clientData;
    int errorCode, errorCodeSet = 0;
    int result = TCL_OK;	/* Result code for 'close' */
    Tcl_Obj *resObj;		/* Result data for 'close' */
    ReflectedTransformMap *rtmPtr;
				/* Map of reflected transforms with handlers
				 * in this interp. */
    Tcl_HashEntry *hPtr;	/* Entry in the above map */

    if (TclInThreadExit()) {
	/*
	 * This call comes from TclFinalizeIOSystem. There are no
	 * interpreters, and therefore we cannot call upon the handler command
	 * anymore. Threading is irrelevant as well. We simply clean up all
	 * our C level data structures and leave the Tcl level to the other
	 * finalization functions.
	 */

	/*
	 * THREADED => Forward this to the origin thread
	 *
	 * Note: DeleteThreadReflectedTransformMap() is the thread exit handler
	 * for the origin thread. Use this to clean up the structure? Except
	 * if lost?
	 */

#ifdef TCL_THREADS
	if (rtPtr->thread != Tcl_GetCurrentThread()) {
	    ForwardParam p;

	    ForwardOpToOwnerThread(rtPtr, ForwardedClose, &p);
	    result = p.base.code;

	    if (result != TCL_OK) {
		FreeReceivedError(&p);
	    }
	}
#endif /* TCL_THREADS */

	Tcl_EventuallyFree(rtPtr, (Tcl_FreeProc *) FreeReflectedTransform);
	return EOK;
    }

    /*
     * In the reflected channel implementation a cleaned method mask here
     * implies that the channel creation was aborted, and "finalize" must not
     * be called. for transformations however we are not going through here on
     * such an abort, but directly through FreeReflectedTransform. So for us
     * that check is not necessary. We always go through 'finalize'.
     */

    if (HAS(rtPtr->methods, METH_DRAIN) && !rtPtr->readIsDrained) {
	if (!TransformDrain(rtPtr, &errorCode)) {
#ifdef TCL_THREADS
	    if (rtPtr->thread != Tcl_GetCurrentThread()) {
		Tcl_EventuallyFree(rtPtr,
			(Tcl_FreeProc *) FreeReflectedTransform);
		return errorCode;
	    }
#endif /* TCL_THREADS */
	    errorCodeSet = 1;
	    goto cleanup;
	}
    }

    if (HAS(rtPtr->methods, METH_FLUSH)) {
	if (!TransformFlush(rtPtr, &errorCode, FLUSH_WRITE)) {
#ifdef TCL_THREADS
	    if (rtPtr->thread != Tcl_GetCurrentThread()) {
		Tcl_EventuallyFree(rtPtr,
			(Tcl_FreeProc *) FreeReflectedTransform);
		return errorCode;
	    }
#endif /* TCL_THREADS */
	    errorCodeSet = 1;
	    goto cleanup;
	}
    }

    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rtPtr->thread != Tcl_GetCurrentThread()) {
	ForwardParam p;

	ForwardOpToOwnerThread(rtPtr, ForwardedClose, &p);
	result = p.base.code;

	Tcl_EventuallyFree(rtPtr, (Tcl_FreeProc *) FreeReflectedTransform);

	if (result != TCL_OK) {
	    PassReceivedErrorInterp(interp, &p);
	    return EINVAL;
	}
	return EOK;
    }
#endif /* TCL_THREADS */

    /*
     * Do the actual invokation of "finalize" now; we're in the right thread.
     */

    result = InvokeTclMethod(rtPtr, "finalize", NULL, NULL, &resObj);
    if ((result != TCL_OK) && (interp != NULL)) {
	Tcl_SetChannelErrorInterp(interp, resObj);
    }

    Tcl_DecrRefCount(resObj);	/* Remove reference we held from the
				 * invoke. */

  cleanup:

    /*
     * Remove the transform from the map before releasing the memory, to
     * prevent future accesses from finding and dereferencing a dangling
     * pointer.
     *
     * NOTE: The transform may not be in the map. This is ok, that happens
     * when the transform was created in a different interpreter and/or thread
     * and then was moved here.
     *
     * NOTE: The channel may have been removed from the map already via
     * the per-interp DeleteReflectedTransformMap exit-handler.
     */

    if (!rtPtr->dead) {
	rtmPtr = GetReflectedTransformMap(rtPtr->interp);
	hPtr = Tcl_FindHashEntry(&rtmPtr->map, Tcl_GetString(rtPtr->handle));
	if (hPtr) {
	    Tcl_DeleteHashEntry(hPtr);
	}

	/*
	 * In a threaded interpreter we manage a per-thread map as well,
	 * to allow us to survive if the script level pulls the rug out
	 * under a channel by deleting the owning thread.
	 */

#ifdef TCL_THREADS
	rtmPtr = GetThreadReflectedTransformMap();
	hPtr = Tcl_FindHashEntry(&rtmPtr->map, Tcl_GetString(rtPtr->handle));
	if (hPtr) {
	    Tcl_DeleteHashEntry(hPtr);
	}
#endif /* TCL_THREADS */
    }

    Tcl_EventuallyFree (rtPtr, (Tcl_FreeProc *) FreeReflectedTransform);
    return errorCodeSet ? errorCode : ((result == TCL_OK) ? EOK : EINVAL);
}

/*
 *----------------------------------------------------------------------
 *
 * ReflectInput --
 *
 *	This function is invoked when more data is requested from the channel.
 *
 * Results:
 *	The number of bytes read.
 *
 * Side effects:
 *	Allocates memory. Arbitrary, as it calls upon a script.
 *
 *----------------------------------------------------------------------
 */

static int
ReflectInput(
    ClientData clientData,
    char *buf,
    int toRead,
    int *errorCodePtr)
{
    ReflectedTransform *rtPtr = clientData;
    int gotBytes, copied, readBytes;
    Tcl_Obj *bufObj;

    /*
     * The following check can be done before thread redirection, because we
     * are reading from an item which is readonly, i.e. will never change
     * during the lifetime of the channel.
     */

    if (!(rtPtr->methods & FLAG(METH_READ))) {
	SetChannelErrorStr(rtPtr->chan, msg_read_unsup);
	*errorCodePtr = EINVAL;
	return -1;
    }

    Tcl_Preserve(rtPtr);

    /* TODO: Consider a more appropriate buffer size. */
    bufObj = Tcl_NewByteArrayObj(NULL, toRead);
    Tcl_IncrRefCount(bufObj);
    gotBytes = 0;
    if (rtPtr->eofPending) {
	goto stop;
    }
    rtPtr->readIsDrained = 0;
    while (toRead > 0) {
	/*
	 * Loop until the request is satisfied (or no data available from
	 * below, possibly EOF).
	 */

	copied = ResultCopy(&rtPtr->result, UCHARP(buf), toRead);
	toRead -= copied;
	buf += copied;
	gotBytes += copied;

	if (toRead == 0) {
	    goto stop;
	}

	if (rtPtr->eofPending) {
	    goto stop;
	}


	/*
	 * The buffer is exhausted, but the caller wants even more. We now
	 * have to go to the underlying channel, get more bytes and then
	 * transform them for delivery. We may not get what we want (full EOF
	 * or temporarily out of data).
	 *
	 * Length (rtPtr->result) == 0, toRead > 0 here. Use 'buf'! as target
	 * to store the intermediary information read from the parent channel.
	 *
	 * Ask the transform how much data it allows us to read from the
	 * underlying channel. This feature allows the transform to signal EOF
	 * upstream although there is none downstream. Useful to control an
	 * unbounded 'fcopy' for example, either through counting bytes, or by
	 * pattern matching.
	 */

	if ((rtPtr->methods & FLAG(METH_LIMIT))) {
	    int maxRead = -1;

	    if (!TransformLimit(rtPtr, errorCodePtr, &maxRead)) {
		goto error;
	    }
	    if (maxRead == 0) {
		goto stop;
	    } else if (maxRead > 0) {
		if (maxRead < toRead) {
		    toRead = maxRead;
		}
	    } /* else: 'maxRead < 0' == Accept the current value of toRead */
	}

	if (toRead <= 0) {
	    goto stop;
	}


	readBytes = Tcl_ReadRaw(rtPtr->parent,
		(char *) Tcl_SetByteArrayLength(bufObj, toRead), toRead);
	if (readBytes < 0) {
	    if (Tcl_InputBlocked(rtPtr->parent) && (gotBytes > 0)) {

		/*
		 * Down channel is blocked and offers zero additional bytes.
		 * The nonzero gotBytes already returned makes the total
		 * operation a valid short read.  Return to caller.
		 */

		goto stop;
	    }

	    /*
	     * Either the down channel is not blocked (a real error)
	     * or it is and there are gotBytes==0 byte copied so far.
	     * In either case, pass up the error, so we either report
	     * any real error, or do not mistakenly signal EOF by
	     * returning 0 to the caller.
	     */

	    *errorCodePtr = Tcl_GetErrno();
	    goto error;
	}

	if (readBytes == 0) {

	    /*
	     * Zero returned from Tcl_ReadRaw() always indicates EOF
	     * on the down channel.
	     */

	    rtPtr->eofPending = 1;

		/*
		 * Now this is a bit different. The partial data waiting is
		 * converted and returned.
		 */

		if (HAS(rtPtr->methods, METH_DRAIN)) {
		    if (!TransformDrain(rtPtr, errorCodePtr)) {
			goto error;
		    }
		}

		if (ResultLength(&rtPtr->result) == 0) {
		    /*
		     * The drain delivered nothing.
		     */

		    goto stop;
		}

		continue; /* at: while (toRead > 0) */
	} /* readBytes == 0 */

	/*
	 * Transform the read chunk, which was not empty. Anything we got back
	 * is a transformation result is put into our buffers, and the next
	 * iteration will put it into the result.
	 */

	Tcl_SetByteArrayLength(bufObj, readBytes);
	if (!TransformRead(rtPtr, errorCodePtr, bufObj)) {
	    goto error;
	}
	if (Tcl_IsShared(bufObj)) {
	    Tcl_DecrRefCount(bufObj);
	    bufObj = Tcl_NewObj();
	    Tcl_IncrRefCount(bufObj);
	}
	Tcl_SetByteArrayLength(bufObj, 0);
    } /* while toRead > 0 */

 stop:
    if (gotBytes == 0) {
	rtPtr->eofPending = 0;
    }
    Tcl_DecrRefCount(bufObj);
    Tcl_Release(rtPtr);
    return gotBytes;

 error:
    gotBytes = -1;
    goto stop;
}

/*
 *----------------------------------------------------------------------
 *
 * ReflectOutput --
 *
 *	This function is invoked when data is written to the channel.
 *
 * Results:
 *	The number of bytes actually written.
 *
 * Side effects:
 *	Allocates memory. Arbitrary, as it calls upon a script.
 *
 *----------------------------------------------------------------------
 */

static int
ReflectOutput(
    ClientData clientData,
    const char *buf,
    int toWrite,
    int *errorCodePtr)
{
    ReflectedTransform *rtPtr = clientData;

    /*
     * The following check can be done before thread redirection, because we
     * are reading from an item which is readonly, i.e. will never change
     * during the lifetime of the channel.
     */

    if (!(rtPtr->methods & FLAG(METH_WRITE))) {
	SetChannelErrorStr(rtPtr->chan, msg_write_unsup);
	*errorCodePtr = EINVAL;
	return -1;
    }

    if (toWrite == 0) {
	/*
	 * Nothing came in to write, ignore the call
	 */

	return 0;
    }

    /*
     * Discard partial data in the input buffers, i.e. on the read side. Like
     * we do when explicitly seeking as well.
     */

    Tcl_Preserve(rtPtr);

    if ((rtPtr->methods & FLAG(METH_CLEAR))) {
	TransformClear(rtPtr);
    }

    /*
     * Hand the data to the transformation itself. Anything it deigned to
     * return to us is a (partial) transformation result and written to the
     * parent channel for further processing.
     */

    if (!TransformWrite(rtPtr, errorCodePtr, UCHARP(buf), toWrite)) {
	Tcl_Release(rtPtr);
	return -1;
    }

    *errorCodePtr = EOK;
    Tcl_Release(rtPtr);
    return toWrite;
}

/*
 *----------------------------------------------------------------------
 *
 * ReflectSeekWide / ReflectSeek --
 *
 *	This function is invoked when the user wishes to seek on the channel.
 *
 * Results:
 *	The new location of the access point.
 *
 * Side effects:
 *	Allocates memory. Arbitrary, per the parent channel, and the called
 *	scripts.
 *
 *----------------------------------------------------------------------
 */

static Tcl_WideInt
ReflectSeekWide(
    ClientData clientData,
    Tcl_WideInt offset,
    int seekMode,
    int *errorCodePtr)
{
    ReflectedTransform *rtPtr = clientData;
    Channel *parent = (Channel *) rtPtr->parent;
    Tcl_WideInt curPos;		/* Position on the device. */

    Tcl_DriverSeekProc *seekProc =
	    Tcl_ChannelSeekProc(Tcl_GetChannelType(rtPtr->parent));

    /*
     * Fail if the parent channel is not seekable.
     */

    if (seekProc == NULL) {
	Tcl_SetErrno(EINVAL);
	return Tcl_LongAsWide(-1);
    }

    /*
     * Check if we can leave out involving the Tcl level, i.e. transformation
     * handler. This is true for tell requests, and transformations which
     * support neither flush, nor drain. For these cases we can pass the
     * request down and the result back up unchanged.
     */

    Tcl_Preserve(rtPtr);

    if (((seekMode != SEEK_CUR) || (offset != 0))
	    && (HAS(rtPtr->methods, METH_CLEAR)
	    || HAS(rtPtr->methods, METH_FLUSH))) {
	/*
	 * Neither a tell request, nor clear/flush both not supported. We have
	 * to go through the Tcl level to clear and/or flush the
	 * transformation.
	 */

	if (rtPtr->methods & FLAG(METH_CLEAR)) {
	    TransformClear(rtPtr);
	}

	/*
	 * When flushing the transform for seeking the generated results are
	 * irrelevant. We cannot put them into the channel, this would move
	 * the location, throwing it off with regard to where we are and are
	 * seeking to.
	 */

	if (HAS(rtPtr->methods, METH_FLUSH)) {
	    if (!TransformFlush(rtPtr, errorCodePtr, FLUSH_DISCARD)) {
		Tcl_Release(rtPtr);
		return -1;
	    }
	}
    }

    /*
     * Now seek to the new position in the channel as requested by the
     * caller. Note that we prefer the wideSeekProc if that is available and
     * non-NULL...
     */

    if (HaveVersion(parent->typePtr, TCL_CHANNEL_VERSION_3) &&
	parent->typePtr->wideSeekProc != NULL) {
	curPos = parent->typePtr->wideSeekProc(parent->instanceData, offset,
		seekMode, errorCodePtr);
    } else if (offset < Tcl_LongAsWide(LONG_MIN) ||
	    offset > Tcl_LongAsWide(LONG_MAX)) {
	*errorCodePtr = EOVERFLOW;
	curPos = Tcl_LongAsWide(-1);
    } else {
	curPos = Tcl_LongAsWide(parent->typePtr->seekProc(
		parent->instanceData, Tcl_WideAsLong(offset), seekMode,
		errorCodePtr));
    }
    if (curPos == Tcl_LongAsWide(-1)) {
	Tcl_SetErrno(*errorCodePtr);
    }

    *errorCodePtr = EOK;
    Tcl_Release(rtPtr);
    return curPos;
}

static int
ReflectSeek(
    ClientData clientData,
    long offset,
    int seekMode,
    int *errorCodePtr)
{
    /*
     * This function can be invoked from a transformation which is based on
     * standard seeking, i.e. non-wide. Because of this we have to implement
     * it, a dummy is not enough. We simply delegate the call to the wide
     * routine.
     */

    return (int) ReflectSeekWide(clientData, Tcl_LongAsWide(offset), seekMode,
	    errorCodePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ReflectWatch --
 *
 *	This function is invoked to tell the channel what events the I/O
 *	system is interested in.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates memory. Arbitrary, as it calls upon a script.
 *
 *----------------------------------------------------------------------
 */

static void
ReflectWatch(
    ClientData clientData,
    int mask)
{
    ReflectedTransform *rtPtr = clientData;
    Tcl_DriverWatchProc *watchProc;

    watchProc = Tcl_ChannelWatchProc(Tcl_GetChannelType(rtPtr->parent));
    watchProc(Tcl_GetChannelInstanceData(rtPtr->parent), mask);

    /*
     * Management of the internal timer.
     */

    if (!(mask & TCL_READABLE) || (ResultLength(&rtPtr->result) == 0)) {
	/*
	 * A pending timer may exist, but either is there no (more) interest
	 * in the events it generates or nothing is available for reading.
	 * Remove it, if existing.
	 */

	TimerKill(rtPtr);
    } else {
	/*
	 * There might be no pending timer, but there is interest in readable
	 * events and we actually have data waiting, so generate a timer to
	 * flush that if it does not exist.
	 */

	TimerSetup(rtPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ReflectBlock --
 *
 *	This function is invoked to tell the channel which blocking behaviour
 *	is required of it.
 *
 * Results:
 *	A posix error number.
 *
 * Side effects:
 *	Allocates memory. Arbitrary, as it calls upon a script.
 *
 *----------------------------------------------------------------------
 */

static int
ReflectBlock(
    ClientData clientData,
    int nonblocking)
{
    ReflectedTransform *rtPtr = clientData;

    /*
     * Transformations simply record the blocking mode in their C level
     * structure for use by --> ReflectInput. The Tcl level doesn't see this
     * information or change. As such thread forwarding is not required.
     */

    rtPtr->nonblocking = nonblocking;
    return EOK;
}

/*
 *----------------------------------------------------------------------
 *
 * ReflectSetOption --
 *
 *	This function is invoked to configure a channel option.
 *
 * Results:
 *	A standard Tcl result code.
 *
 * Side effects:
 *	Arbitrary, per the parent channel.
 *
 *----------------------------------------------------------------------
 */

static int
ReflectSetOption(
    ClientData clientData,	/* Channel to query */
    Tcl_Interp *interp,		/* Interpreter to leave error messages in */
    const char *optionName,	/* Name of requested option */
    const char *newValue)	/* The new value */
{
    ReflectedTransform *rtPtr = clientData;

    /*
     * Transformations have no options. Thus the call is passed down unchanged
     * to the parent channel for processing. Its results are passed back
     * unchanged as well. This all happens in the thread we are in. As the Tcl
     * level is not involved there is no need for thread forwarding.
     */

    Tcl_DriverSetOptionProc *setOptionProc =
	    Tcl_ChannelSetOptionProc(Tcl_GetChannelType(rtPtr->parent));

    if (setOptionProc == NULL) {
	return TCL_ERROR;
    }
    return setOptionProc(Tcl_GetChannelInstanceData(rtPtr->parent), interp,
	    optionName, newValue);
}

/*
 *----------------------------------------------------------------------
 *
 * ReflectGetOption --
 *
 *	This function is invoked to retrieve all or a channel options.
 *
 * Results:
 *	A standard Tcl result code.
 *
 * Side effects:
 *	Arbitrary, per the parent channel.
 *
 *----------------------------------------------------------------------
 */

static int
ReflectGetOption(
    ClientData clientData,	/* Channel to query */
    Tcl_Interp *interp,		/* Interpreter to leave error messages in */
    const char *optionName,	/* Name of reuqested option */
    Tcl_DString *dsPtr)		/* String to place the result into */
{
    ReflectedTransform *rtPtr = clientData;

    /*
     * Transformations have no options. Thus the call is passed down unchanged
     * to the parent channel for processing. Its results are passed back
     * unchanged as well. This all happens in the thread we are in. As the Tcl
     * level is not involved there is no need for thread forwarding.
     *
     * Note that the parent not having a driver for option retrieval is not an
     * immediate error. A query for all options is ok. Only a request for a
     * specific option has to fail.
     */

    Tcl_DriverGetOptionProc *getOptionProc =
	    Tcl_ChannelGetOptionProc(Tcl_GetChannelType(rtPtr->parent));

    if (getOptionProc != NULL) {
	return getOptionProc(Tcl_GetChannelInstanceData(rtPtr->parent),
		interp, optionName, dsPtr);
    } else if (optionName == NULL) {
	return TCL_OK;
    } else {
	return TCL_ERROR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ReflectHandle --
 *
 *	This function is invoked to retrieve the associated file handle.
 *
 * Results:
 *	A standard Tcl result code.
 *
 * Side effects:
 *	Arbitrary, per the parent channel.
 *
 *----------------------------------------------------------------------
 */

static int
ReflectHandle(
    ClientData clientData,
    int direction,
    ClientData *handlePtr)
{
    ReflectedTransform *rtPtr = clientData;

    /*
     * Transformations have no handle of their own. As such we simply query
     * the parent channel for it. This way the qery will ripple down through
     * all transformations until reaches the base channel. Which then returns
     * its handle, or fails. The former will then ripple up the stack.
     *
     * This all happens in the thread we are in. As the Tcl level is not
     * involved no forwarding is required.
     */

    return Tcl_GetChannelHandle(rtPtr->parent, direction, handlePtr);
}
/*
 *----------------------------------------------------------------------
 *
 * ReflectNotify --
 *
 *	This function is invoked to reported incoming events.
 *
 * Results:
 *	A standard Tcl result code.
 *
 * Side effects:
 *	Arbitrary, per the parent channel.
 *
 *----------------------------------------------------------------------
 */

static int
ReflectNotify(
    ClientData clientData,
    int mask)
{
    ReflectedTransform *rtPtr = clientData;

    /*
     * An event occured in the underlying channel.
     *
     * We delete our timer. It was not fired, yet we are here, so the channel
     * below generated such an event and we don't have to. The renewal of the
     * interest after the execution of channel handlers will eventually cause
     * us to recreate the timer (in ReflectWatch).
     */

    TimerKill(rtPtr);

    /*
     * Pass to higher layers.
     */

    return mask;
}

/*
 * Helpers. =========================================================
 */


/*
 *----------------------------------------------------------------------
 *
 * DecodeEventMask --
 *
 *	This function takes an internal bitmask of events and constructs the
 *	equivalent list of event items.
 *
 * Results:
 *	A Tcl_Obj reference. The object will have a refCount of one. The user
 *	has to decrement it to release the object.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 * DUPLICATE of 'DecodeEventMask' in tclIORChan.c
 */

static Tcl_Obj *
DecodeEventMask(
    int mask)
{
    register const char *eventStr;
    Tcl_Obj *evObj;

    switch (mask & RANDW) {
    case RANDW:
	eventStr = "read write";
	break;
    case TCL_READABLE:
	eventStr = "read";
	break;
    case TCL_WRITABLE:
	eventStr = "write";
	break;
    default:
	eventStr = "";
	break;
    }

    evObj = Tcl_NewStringObj(eventStr, -1);
    Tcl_IncrRefCount(evObj);
    return evObj;
}

/*
 *----------------------------------------------------------------------
 *
 * NewReflectedTransform --
 *
 *	This function is invoked to allocate and initialize the instance data
 *	of a new reflected channel.
 *
 * Results:
 *	A heap-allocated channel instance.
 *
 * Side effects:
 *	Allocates memory.
 *
 *----------------------------------------------------------------------
 */

static ReflectedTransform *
NewReflectedTransform(
    Tcl_Interp *interp,
    Tcl_Obj *cmdpfxObj,
    int mode,
    Tcl_Obj *handleObj,
    Tcl_Channel parentChan)
{
    ReflectedTransform *rtPtr;
    int listc;
    Tcl_Obj **listv;
    int i;

    rtPtr = ckalloc(sizeof(ReflectedTransform));

    /* rtPtr->chan: Assigned by caller. Dummy data here. */
    /* rtPtr->methods: Assigned by caller. Dummy data here. */

    rtPtr->chan = NULL;
    rtPtr->methods = 0;
#ifdef TCL_THREADS
    rtPtr->thread = Tcl_GetCurrentThread();
#endif
    rtPtr->parent = parentChan;
    rtPtr->interp = interp;
    rtPtr->handle = handleObj;
    Tcl_IncrRefCount(handleObj);
    rtPtr->timer = NULL;
    rtPtr->mode = 0;
    rtPtr->readIsDrained = 0;
    rtPtr->eofPending = 0;
    rtPtr->nonblocking =
	    (((Channel *) parentChan)->state->flags & CHANNEL_NONBLOCKING);
    rtPtr->dead = 0;

    /*
     * Query parent for current blocking mode.
     */

    ResultInit(&rtPtr->result);

    /*
     * Method placeholder.
     */

    /* ASSERT: cmdpfxObj is a Tcl List */

    Tcl_ListObjGetElements(interp, cmdpfxObj, &listc, &listv);

    /*
     * See [==] as well.
     * Storage for the command prefix and the additional words required for
     * the invocation of methods in the command handler.
     *
     * listv [0] [listc-1] | [listc]  [listc+1] |
     * argv  [0]   ... [.] | [argc-2] [argc-1]  | [argc]  [argc+2]
     *       cmd   ... pfx | method   chan      | detail1 detail2
     */

    rtPtr->argc = listc + 2;
    rtPtr->argv = ckalloc(sizeof(Tcl_Obj *) * (listc+4));

    /*
     * Duplicate object references.
     */

    for (i=0; i<listc ; i++) {
	Tcl_Obj *word = rtPtr->argv[i] = listv[i];

	Tcl_IncrRefCount(word);
    }

    i++;				/* Skip placeholder for method */

    /*
     * See [x] in FreeReflectedTransform for release
     */
    rtPtr->argv[i] = handleObj;
    Tcl_IncrRefCount(handleObj);

    /*
     * The next two objects are kept empty, varying arguments.
     */

    /*
     * Initialization complete.
     */

    return rtPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * NextHandle --
 *
 *	This function is invoked to generate a channel handle for a new
 *	reflected channel.
 *
 * Results:
 *	A Tcl_Obj containing the string of the new channel handle. The
 *	refcount of the returned object is -- zero --.
 *
 * Side effects:
 *	May allocate memory. Mutex protected critical section locks out other
 *	threads for a short time.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
NextHandle(void)
{
    /*
     * Count number of generated reflected channels. Used for id generation.
     * Ids are never reclaimed and there is no dealing with wrap around. On
     * the other hand, "unsigned long" should be big enough except for
     * absolute longrunners (generate a 100 ids per second => overflow will
     * occur in 1 1/3 years).
     */

    TCL_DECLARE_MUTEX(rtCounterMutex)
    static unsigned long rtCounter = 0;
    Tcl_Obj *resObj;

    Tcl_MutexLock(&rtCounterMutex);
    resObj = Tcl_ObjPrintf("rt%lu", rtCounter);
    rtCounter++;
    Tcl_MutexUnlock(&rtCounterMutex);

    return resObj;
}

static void
FreeReflectedTransformArgs(
    ReflectedTransform *rtPtr)
{
    int i, n = rtPtr->argc - 2;

    if (n < 0) {
	return;
    }

    Tcl_DecrRefCount(rtPtr->handle);
    rtPtr->handle = NULL;

    for (i=0; i<n; i++) {
	Tcl_DecrRefCount(rtPtr->argv[i]);
    }

    /*
     * See [x] in NewReflectedTransform for lock
     * n+1 = argc-1.
     */
    Tcl_DecrRefCount(rtPtr->argv[n+1]);

    rtPtr->argc = 1;
}

static void
FreeReflectedTransform(
    ReflectedTransform *rtPtr)
{
    TimerKill(rtPtr);
    ResultClear(&rtPtr->result);

    FreeReflectedTransformArgs(rtPtr);

    ckfree(rtPtr->argv);
    ckfree(rtPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * InvokeTclMethod --
 *
 *	This function is used to invoke the Tcl level of a reflected channel.
 *	It handles all the command assembly, invokation, and generic state and
 *	result mgmt. It does *not* handle thread redirection; that is the
 *	responsibility of clients of this function.
 *
 * Results:
 *	Result code and data as returned by the method.
 *
 * Side effects:
 *	Arbitrary, as it calls upon a Tcl script.
 *
 * Contract:
 *	argOneObj.refCount >= 1 on entry and exit, if argOneObj != NULL
 *	argTwoObj.refCount >= 1 on entry and exit, if argTwoObj != NULL
 *	resObj.refCount in {0, 1, ...}
 *
 *----------------------------------------------------------------------
 * Semi-DUPLICATE of 'InvokeTclMethod' in tclIORChan.c
 * - Semi because different structures are used.
 * - Still possible to factor out the commonalities into a separate structure.
 */

static int
InvokeTclMethod(
    ReflectedTransform *rtPtr,
    const char *method,
    Tcl_Obj *argOneObj,		/* NULL'able */
    Tcl_Obj *argTwoObj,		/* NULL'able */
    Tcl_Obj **resultObjPtr)	/* NULL'able */
{
    int cmdc;			/* #words in constructed command */
    Tcl_Obj *methObj = NULL;	/* Method name in object form */
    Tcl_InterpState sr;		/* State of handler interp */
    int result;			/* Result code of method invokation */
    Tcl_Obj *resObj = NULL;	/* Result of method invokation. */

    if (rtPtr->dead) {
	/*
	 * The transform is marked as dead. Bail out immediately, with an
	 * appropriate error.
	 */

	if (resultObjPtr != NULL) {
	    resObj = Tcl_NewStringObj(msg_dstlost,-1);
	    *resultObjPtr = resObj;
	    Tcl_IncrRefCount(resObj);
	}
	return TCL_ERROR;
    }

    /*
     * NOTE (5): Decide impl. issue: Cache objects with method names?
     * Requires TSD data as reflections can be created in many different
     * threads.
     * NO: Caching of command resolutions means storage per channel.
     */

    /*
     * Insert method into the pre-allocated area, after the command prefix,
     * before the channel id.
     */

    methObj = Tcl_NewStringObj(method, -1);
    Tcl_IncrRefCount(methObj);
    rtPtr->argv[rtPtr->argc - 2] = methObj;

    /*
     * Append the additional argument containing method specific details
     * behind the channel id. If specified.
     *
     * Because of the contract there is no need to increment the refcounts.
     * The objects will survive the Tcl_EvalObjv without change.
     */

    cmdc = rtPtr->argc;
    if (argOneObj) {
	rtPtr->argv[cmdc] = argOneObj;
	cmdc++;
	if (argTwoObj) {
	    rtPtr->argv[cmdc] = argTwoObj;
	    cmdc++;
	}
    }

    /*
     * And run the handler... This is done in auch a manner which leaves any
     * existing state intact.
     */

    sr = Tcl_SaveInterpState(rtPtr->interp, 0 /* Dummy */);
    Tcl_Preserve(rtPtr);
    Tcl_Preserve(rtPtr->interp);
    result = Tcl_EvalObjv(rtPtr->interp, cmdc, rtPtr->argv, TCL_EVAL_GLOBAL);

    /*
     * We do not try to extract the result information if the caller has no
     * interest in it. I.e. there is no need to put effort into creating
     * something which is discarded immediately after.
     */

    if (resultObjPtr) {
	if (result == TCL_OK) {
	    /*
	     * Ok result taken as is, also if the caller requests that there
	     * is no capture.
	     */

	    resObj = Tcl_GetObjResult(rtPtr->interp);
	} else {
	    /*
	     * Non-ok result is always treated as an error. We have to capture
	     * the full state of the result, including additional options.
	     *
	     * This is complex and ugly, and would be completely unnecessary
	     * if we only added support for a TCL_FORBID_EXCEPTIONS flag.
	     */
	    if (result != TCL_ERROR) {
		Tcl_Obj *cmd = Tcl_NewListObj(cmdc, rtPtr->argv);
		int cmdLen;
		const char *cmdString = Tcl_GetStringFromObj(cmd, &cmdLen);

		Tcl_IncrRefCount(cmd);
		Tcl_ResetResult(rtPtr->interp);
		Tcl_SetObjResult(rtPtr->interp, Tcl_ObjPrintf(
			"chan handler returned bad code: %d", result));
		Tcl_LogCommandInfo(rtPtr->interp, cmdString, cmdString, cmdLen);
		Tcl_DecrRefCount(cmd);
		result = TCL_ERROR;
	    }
	    Tcl_AppendObjToErrorInfo(rtPtr->interp, Tcl_ObjPrintf(
		    "\n    (chan handler subcommand \"%s\")", method));
	    resObj = MarshallError(rtPtr->interp);
	}
	Tcl_IncrRefCount(resObj);
    }
    Tcl_RestoreInterpState(rtPtr->interp, sr);
    Tcl_Release(rtPtr->interp);
    Tcl_Release(rtPtr);

    /*
     * Cleanup of the dynamic parts of the command.
     *
     * The detail objects survived the Tcl_EvalObjv without change because of
     * the contract. Therefore there is no need to decrement the refcounts. Only
     * the internal method object has to be disposed of.
     */

    Tcl_DecrRefCount(methObj);

    /*
     * The resObj has a ref count of 1 at this location. This means that the
     * caller of InvokeTclMethod has to dispose of it (but only if it was
     * returned to it).
     */

    if (resultObjPtr != NULL) {
	*resultObjPtr = resObj;
    }

    /*
     * There no need to handle the case where nothing is returned, because for
     * that case resObj was not set anyway.
     */

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * GetReflectedTransformMap --
 *
 *	Gets and potentially initializes the reflected channel map for an
 *	interpreter.
 *
 * Results:
 *	A pointer to the map created, for use by the caller.
 *
 * Side effects:
 *	Initializes the reflected channel map for an interpreter.
 *
 *----------------------------------------------------------------------
 */

static ReflectedTransformMap *
GetReflectedTransformMap(
    Tcl_Interp *interp)
{
    ReflectedTransformMap *rtmPtr = Tcl_GetAssocData(interp, RTMKEY, NULL);

    if (rtmPtr == NULL) {
	rtmPtr = ckalloc(sizeof(ReflectedTransformMap));
	Tcl_InitHashTable(&rtmPtr->map, TCL_STRING_KEYS);
	Tcl_SetAssocData(interp, RTMKEY,
		(Tcl_InterpDeleteProc *) DeleteReflectedTransformMap, rtmPtr);
    }
    return rtmPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteReflectedTransformMap --
 *
 *	Deletes the channel table for an interpreter, closing any open
 *	channels whose refcount reaches zero. This procedure is invoked when
 *	an interpreter is deleted, via the AssocData cleanup mechanism.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deletes the hash table of channels. May close channels. May flush
 *	output on closed channels. Removes any channeEvent handlers that were
 *	registered in this interpreter.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteReflectedTransformMap(
    ClientData clientData,	/* The per-interpreter data structure. */
    Tcl_Interp *interp)		/* The interpreter being deleted. */
{
    ReflectedTransformMap *rtmPtr; /* The map */
    Tcl_HashSearch hSearch;	 /* Search variable. */
    Tcl_HashEntry *hPtr;	 /* Search variable. */
    ReflectedTransform *rtPtr;
#ifdef TCL_THREADS
    ForwardingResult *resultPtr;
    ForwardingEvent *evPtr;
    ForwardParam *paramPtr;
#endif /* TCL_THREADS */

    /*
     * Delete all entries. The channels may have been closed already, or will
     * be closed later, by the standard IO finalization of an interpreter
     * under destruction. Except for the channels which were moved to a
     * different interpreter and/or thread. They do not exist from the IO
     * systems point of view and will not get closed. Therefore mark all as
     * dead so that any future access will cause a proper error. For channels
     * in a different thread we actually do the same as
     * DeleteThreadReflectedTransformMap(), just restricted to the channels of
     * this interp.
     */

    rtmPtr = clientData;
    for (hPtr = Tcl_FirstHashEntry(&rtmPtr->map, &hSearch);
	    hPtr != NULL;
	    hPtr = Tcl_FirstHashEntry(&rtmPtr->map, &hSearch)) {
	rtPtr = Tcl_GetHashValue(hPtr);

	rtPtr->dead = 1;
	Tcl_DeleteHashEntry(hPtr);
    }
    Tcl_DeleteHashTable(&rtmPtr->map);
    ckfree(&rtmPtr->map);

#ifdef TCL_THREADS
    /*
     * The origin interpreter for one or more reflected channels is gone.
     */

    /*
     * Get the map of all channels handled by the current thread. This is a
     * ReflectedTransformMap, but on a per-thread basis, not per-interp. Go
     * through the channels and remove all which were handled by this
     * interpreter. They have already been marked as dead.
     */

    rtmPtr = GetThreadReflectedTransformMap();
    for (hPtr = Tcl_FirstHashEntry(&rtmPtr->map, &hSearch);
	    hPtr != NULL;
	    hPtr = Tcl_NextHashEntry(&hSearch)) {
	rtPtr = Tcl_GetHashValue(hPtr);

	if (rtPtr->interp != interp) {
	    /*
	     * Ignore entries for other interpreters.
	     */

	    continue;
	}

	rtPtr->dead = 1;
	FreeReflectedTransformArgs(rtPtr);
	Tcl_DeleteHashEntry(hPtr);
    }

    /*
     * Go through the list of pending results and cancel all whose events were
     * destined for this interpreter. While this is in progress we block any
     * other access to the list of pending results.
     */

    Tcl_MutexLock(&rtForwardMutex);

    for (resultPtr = forwardList; resultPtr != NULL;
	    resultPtr = resultPtr->nextPtr) {
	if (resultPtr->dsti != interp) {
	    /*
	     * Ignore results/events for other interpreters.
	     */

	    continue;
	}

	/*
	 * The receiver for the event exited, before processing the event. We
	 * detach the result now, wake the originator up and signal failure.
	 */

	evPtr = resultPtr->evPtr;
	if (evPtr == NULL) {
	    continue;
	}
	paramPtr = evPtr->param;

	evPtr->resultPtr = NULL;
	resultPtr->evPtr = NULL;
	resultPtr->result = TCL_ERROR;

	ForwardSetStaticError(paramPtr, msg_send_dstlost);

	Tcl_ConditionNotify(&resultPtr->done);
    }
    Tcl_MutexUnlock(&rtForwardMutex);
#endif /* TCL_THREADS */
}

#ifdef TCL_THREADS
/*
 *----------------------------------------------------------------------
 *
 * GetThreadReflectedTransformMap --
 *
 *	Gets and potentially initializes the reflected channel map for a
 *	thread.
 *
 * Results:
 *	A pointer to the map created, for use by the caller.
 *
 * Side effects:
 *	Initializes the reflected channel map for a thread.
 *
 *----------------------------------------------------------------------
 */

static ReflectedTransformMap *
GetThreadReflectedTransformMap(void)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!tsdPtr->rtmPtr) {
	tsdPtr->rtmPtr = ckalloc(sizeof(ReflectedTransformMap));
	Tcl_InitHashTable(&tsdPtr->rtmPtr->map, TCL_STRING_KEYS);
	Tcl_CreateThreadExitHandler(DeleteThreadReflectedTransformMap, NULL);
    }

    return tsdPtr->rtmPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteThreadReflectedTransformMap --
 *
 *	Deletes the channel table for a thread. This procedure is invoked when
 *	a thread is deleted. The channels have already been marked as dead, in
 *	DeleteReflectedTransformMap().
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deletes the hash table of channels.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteThreadReflectedTransformMap(
    ClientData clientData)	/* The per-thread data structure. */
{
    Tcl_HashSearch hSearch;	 /* Search variable. */
    Tcl_HashEntry *hPtr;	 /* Search variable. */
    Tcl_ThreadId self = Tcl_GetCurrentThread();
    ReflectedTransformMap *rtmPtr; /* The map */
    ForwardingResult *resultPtr;

    /*
     * The origin thread for one or more reflected channels is gone.
     * NOTE: If this function is called due to a thread getting killed the
     *       per-interp DeleteReflectedTransformMap is apparently not called.
     */

    /*
     * Get the map of all channels handled by the current thread. This is a
     * ReflectedTransformMap, but on a per-thread basis, not per-interp. Go
     * through the channels, remove all, mark them as dead.
     */

    rtmPtr = GetThreadReflectedTransformMap();
    for (hPtr = Tcl_FirstHashEntry(&rtmPtr->map, &hSearch);
	    hPtr != NULL;
	    hPtr = Tcl_FirstHashEntry(&rtmPtr->map, &hSearch)) {
	ReflectedTransform *rtPtr = Tcl_GetHashValue(hPtr);

	rtPtr->dead = 1;
	FreeReflectedTransformArgs(rtPtr);
	Tcl_DeleteHashEntry(hPtr);
    }
    ckfree(rtmPtr);

    /*
     * Go through the list of pending results and cancel all whose events were
     * destined for this thread. While this is in progress we block any
     * other access to the list of pending results.
     */

    Tcl_MutexLock(&rtForwardMutex);

    for (resultPtr = forwardList; resultPtr != NULL;
	    resultPtr = resultPtr->nextPtr) {
	ForwardingEvent *evPtr;
	ForwardParam *paramPtr;

	if (resultPtr->dst != self) {
	    /*
	     * Ignore results/events for other threads.
	     */

	    continue;
	}

	/*
	 * The receiver for the event exited, before processing the event. We
	 * detach the result now, wake the originator up and signal failure.
	 */

	evPtr = resultPtr->evPtr;
	if (evPtr == NULL) {
	    continue;
	}
	paramPtr = evPtr->param;

	evPtr->resultPtr = NULL;
	resultPtr->evPtr = NULL;
	resultPtr->result = TCL_ERROR;

	ForwardSetStaticError(paramPtr, msg_send_dstlost);

	Tcl_ConditionNotify(&resultPtr->done);
    }
    Tcl_MutexUnlock(&rtForwardMutex);
}

static void
ForwardOpToOwnerThread(
    ReflectedTransform *rtPtr,	/* Channel instance */
    ForwardedOperation op,	/* Forwarded driver operation */
    const void *param)		/* Arguments */
{
    Tcl_ThreadId dst = rtPtr->thread;
    ForwardingEvent *evPtr;
    ForwardingResult *resultPtr;

    /*
     * We gather the lock early. This allows us to check the liveness of the
     * channel without interference from DeleteThreadReflectedTransformMap().
     */

    Tcl_MutexLock(&rtForwardMutex);

    if (rtPtr->dead) {
	/*
	 * The channel is marked as dead. Bail out immediately, with an
	 * appropriate error. Do not forget to unlock the mutex on this path.
	 */

	ForwardSetStaticError((ForwardParam *) param, msg_send_dstlost);
	Tcl_MutexUnlock(&rtForwardMutex);
	return;
    }

    /*
     * Create and initialize the event and data structures.
     */

    evPtr = ckalloc(sizeof(ForwardingEvent));
    resultPtr = ckalloc(sizeof(ForwardingResult));

    evPtr->event.proc = ForwardProc;
    evPtr->resultPtr = resultPtr;
    evPtr->op = op;
    evPtr->rtPtr = rtPtr;
    evPtr->param = (ForwardParam *) param;

    resultPtr->src = Tcl_GetCurrentThread();
    resultPtr->dst = dst;
    resultPtr->dsti = rtPtr->interp;
    resultPtr->done = NULL;
    resultPtr->result = -1;
    resultPtr->evPtr = evPtr;

    /*
     * Now execute the forward.
     */

    TclSpliceIn(resultPtr, forwardList);
    /* Do not unlock here. That is done by the ConditionWait */

    /*
     * Ensure cleanup of the event if the origin thread exits while this event
     * is pending or in progress. Exit of the destination thread is handled by
     * DeleteThreadReflectionChannelMap(), this is set up by
     * GetThreadReflectedTransformMap(). This is what we use the 'forwardList'
     * (see above) for.
     */

    Tcl_CreateThreadExitHandler(SrcExitProc, evPtr);

    /*
     * Queue the event and poke the other thread's notifier.
     */

    Tcl_ThreadQueueEvent(dst, (Tcl_Event *) evPtr, TCL_QUEUE_TAIL);
    Tcl_ThreadAlert(dst);

    /*
     * (*) Block until the other thread has either processed the transfer or
     * rejected it.
     */

    while (resultPtr->result < 0) {
	/*
	 * NOTE (1): Is it possible that the current thread goes away while
	 * waiting here? IOW Is it possible that "SrcExitProc" is called
	 * while we are here? See complementary note (2) in "SrcExitProc"
	 *
	 * The ConditionWait unlocks the mutex during the wait and relocks it
	 * immediately after.
	 */

	Tcl_ConditionWait(&resultPtr->done, &rtForwardMutex, NULL);
    }

    /*
     * Unlink result from the forwarder list. No need to lock. Either still
     * locked, or locked by the ConditionWait
     */

    TclSpliceOut(resultPtr, forwardList);

    resultPtr->nextPtr = NULL;
    resultPtr->prevPtr = NULL;

    Tcl_MutexUnlock(&rtForwardMutex);
    Tcl_ConditionFinalize(&resultPtr->done);

    /*
     * Kill the cleanup handler now, and the result structure as well, before
     * returning the success code.
     *
     * Note: The event structure has already been deleted by the destination
     * notifier, after it serviced the event.
     */

    Tcl_DeleteThreadExitHandler(SrcExitProc, evPtr);

    ckfree(resultPtr);
}

static int
ForwardProc(
    Tcl_Event *evGPtr,
    int mask)
{
    /*
     * Notes regarding access to the referenced data.
     *
     * In principle the data belongs to the originating thread (see
     * evPtr->src), however this thread is currently blocked at (*), i.e.
     * quiescent. Because of this we can treat the data as belonging to us,
     * without fear of race conditions. I.e. we can read and write as we like.
     *
     * The only thing we cannot be sure of is the resultPtr. This can be be
     * NULLed if the originating thread went away while the event is handled
     * here now.
     */

    ForwardingEvent *evPtr = (ForwardingEvent *) evGPtr;
    ForwardingResult *resultPtr = evPtr->resultPtr;
    ReflectedTransform *rtPtr = evPtr->rtPtr;
    Tcl_Interp *interp = rtPtr->interp;
    ForwardParam *paramPtr = evPtr->param;
    Tcl_Obj *resObj = NULL;	/* Interp result of InvokeTclMethod */
    ReflectedTransformMap *rtmPtr;
				/* Map of reflected channels with handlers in
				 * this interp. */
    Tcl_HashEntry *hPtr;	/* Entry in the above map */

    /*
     * Ignore the event if no one is waiting for its result anymore.
     */

    if (!resultPtr) {
	return 1;
    }

    paramPtr->base.code = TCL_OK;
    paramPtr->base.msgStr = NULL;
    paramPtr->base.mustFree = 0;

    switch (evPtr->op) {
	/*
	 * The destination thread for the following operations is
	 * rtPtr->thread, which contains rtPtr->interp, the interp we have to
	 * call upon for the driver.
	 */

    case ForwardedClose:
	/*
	 * No parameters/results.
	 */

	if (InvokeTclMethod(rtPtr, "finalize", NULL, NULL,
		&resObj) != TCL_OK) {
	    ForwardSetObjError(paramPtr, resObj);
	}

	/*
	 * Freeing is done here, in the origin thread, because the argv[]
	 * objects belong to this thread. Deallocating them in a different
	 * thread is not allowed
	 */

	/*
	 * Remove the channel from the map before releasing the memory, to
	 * prevent future accesses (like by 'postevent') from finding and
	 * dereferencing a dangling pointer.
	 */

	rtmPtr = GetReflectedTransformMap(interp);
	hPtr = Tcl_FindHashEntry(&rtmPtr->map, Tcl_GetString(rtPtr->handle));
	Tcl_DeleteHashEntry(hPtr);

	/*
	 * In a threaded interpreter we manage a per-thread map as well, to
	 * allow us to survive if the script level pulls the rug out under a
	 * channel by deleting the owning thread.
	 */

	rtmPtr = GetThreadReflectedTransformMap();
	hPtr = Tcl_FindHashEntry(&rtmPtr->map, Tcl_GetString(rtPtr->handle));
	Tcl_DeleteHashEntry(hPtr);

	FreeReflectedTransformArgs(rtPtr);
	break;

    case ForwardedInput: {
	Tcl_Obj *bufObj = Tcl_NewByteArrayObj((unsigned char *)
		paramPtr->transform.buf, paramPtr->transform.size);
	Tcl_IncrRefCount(bufObj);

	if (InvokeTclMethod(rtPtr, "read", bufObj, NULL, &resObj) != TCL_OK) {
	    ForwardSetObjError(paramPtr, resObj);
	    paramPtr->transform.size = -1;
	} else {
	    /*
	     * Process a regular return. Contains the transformation result.
	     * Sent it back to the request originator.
	     */

	    int bytec;		/* Number of returned bytes */
	    unsigned char *bytev;
				/* Array of returned bytes */

	    bytev = Tcl_GetByteArrayFromObj(resObj, &bytec);

	    paramPtr->transform.size = bytec;

	    if (bytec > 0) {
		paramPtr->transform.buf = ckalloc(bytec);
		memcpy(paramPtr->transform.buf, bytev, (size_t)bytec);
	    } else {
		paramPtr->transform.buf = NULL;
	    }
	}

	Tcl_DecrRefCount(bufObj);
	break;
    }

    case ForwardedOutput: {
	Tcl_Obj *bufObj = Tcl_NewByteArrayObj((unsigned char *)
		paramPtr->transform.buf, paramPtr->transform.size);
	Tcl_IncrRefCount(bufObj);

	if (InvokeTclMethod(rtPtr, "write", bufObj, NULL, &resObj) != TCL_OK) {
	    ForwardSetObjError(paramPtr, resObj);
	    paramPtr->transform.size = -1;
	} else {
	    /*
	     * Process a regular return. Contains the transformation result.
	     * Sent it back to the request originator.
	     */

	    int bytec;		/* Number of returned bytes */
	    unsigned char *bytev;
				/* Array of returned bytes */

	    bytev = Tcl_GetByteArrayFromObj(resObj, &bytec);

	    paramPtr->transform.size = bytec;

	    if (bytec > 0) {
		paramPtr->transform.buf = ckalloc(bytec);
		memcpy(paramPtr->transform.buf, bytev, (size_t)bytec);
	    } else {
		paramPtr->transform.buf = NULL;
	    }
	}

	Tcl_DecrRefCount(bufObj);
	break;
    }

    case ForwardedDrain:
	if (InvokeTclMethod(rtPtr, "drain", NULL, NULL, &resObj) != TCL_OK) {
	    ForwardSetObjError(paramPtr, resObj);
	    paramPtr->transform.size = -1;
	} else {
	    /*
	     * Process a regular return. Contains the transformation result.
	     * Sent it back to the request originator.
	     */

	    int bytec;		/* Number of returned bytes */
	    unsigned char *bytev; /* Array of returned bytes */

	    bytev = Tcl_GetByteArrayFromObj(resObj, &bytec);

	    paramPtr->transform.size = bytec;

	    if (bytec > 0) {
		paramPtr->transform.buf = ckalloc(bytec);
		memcpy(paramPtr->transform.buf, bytev, (size_t)bytec);
	    } else {
		paramPtr->transform.buf = NULL;
	    }
	}
	break;

    case ForwardedFlush:
	if (InvokeTclMethod(rtPtr, "flush", NULL, NULL, &resObj) != TCL_OK) {
	    ForwardSetObjError(paramPtr, resObj);
	    paramPtr->transform.size = -1;
	} else {
	    /*
	     * Process a regular return. Contains the transformation result.
	     * Sent it back to the request originator.
	     */

	    int bytec;		/* Number of returned bytes */
	    unsigned char *bytev;
				/* Array of returned bytes */

	    bytev = Tcl_GetByteArrayFromObj(resObj, &bytec);

	    paramPtr->transform.size = bytec;

	    if (bytec > 0) {
		paramPtr->transform.buf = ckalloc(bytec);
		memcpy(paramPtr->transform.buf, bytev, (size_t)bytec);
	    } else {
		paramPtr->transform.buf = NULL;
	    }
	}
	break;

    case ForwardedClear:
	(void) InvokeTclMethod(rtPtr, "clear", NULL, NULL, NULL);
	break;

    case ForwardedLimit:
	if (InvokeTclMethod(rtPtr, "limit?", NULL, NULL, &resObj) != TCL_OK) {
	    ForwardSetObjError(paramPtr, resObj);
	    paramPtr->limit.max = -1;
	} else if (Tcl_GetIntFromObj(interp, resObj,
		&paramPtr->limit.max) != TCL_OK) {
	    ForwardSetObjError(paramPtr, MarshallError(interp));
	    paramPtr->limit.max = -1;
	}
	break;

    default:
	/*
	 * Bad operation code.
	 */
	Tcl_Panic("Bad operation code in ForwardProc");
	break;
    }

    /*
     * Remove the reference we held on the result of the invoke, if we had
     * such.
     */

    if (resObj != NULL) {
	Tcl_DecrRefCount(resObj);
    }

    if (resultPtr) {
	/*
	 * Report the forwarding result synchronously to the waiting caller.
	 * This unblocks (*) as well. This is wrapped into a conditional
	 * because the caller may have exited in the mean time.
	 */

	Tcl_MutexLock(&rtForwardMutex);
	resultPtr->result = TCL_OK;
	Tcl_ConditionNotify(&resultPtr->done);
	Tcl_MutexUnlock(&rtForwardMutex);
    }

    return 1;
}

static void
SrcExitProc(
    ClientData clientData)
{
    ForwardingEvent *evPtr = clientData;
    ForwardingResult *resultPtr;
    ForwardParam *paramPtr;

    /*
     * NOTE (2): Can this handler be called with the originator blocked?
     */

    /*
     * The originator for the event exited. It is not sure if this can happen,
     * as the originator should be blocked at (*) while the event is in
     * transit/pending.
     *
     * We make sure that the event cannot refer to the result anymore, remove
     * it from the list of pending results and free the structure. Locking the
     * access ensures that we cannot get in conflict with "ForwardProc",
     * should it already execute the event.
     */

    Tcl_MutexLock(&rtForwardMutex);

    resultPtr = evPtr->resultPtr;
    paramPtr = evPtr->param;

    evPtr->resultPtr = NULL;
    resultPtr->evPtr = NULL;
    resultPtr->result = TCL_ERROR;

    ForwardSetStaticError(paramPtr, msg_send_originlost);

    /*
     * See below: TclSpliceOut(resultPtr, forwardList);
     */

    Tcl_MutexUnlock(&rtForwardMutex);

    /*
     * This unlocks (*). The structure will be spliced out and freed by
     * "ForwardProc". Maybe.
     */

    Tcl_ConditionNotify(&resultPtr->done);
}

static void
ForwardSetObjError(
    ForwardParam *paramPtr,
    Tcl_Obj *obj)
{
    int len;
    const char *msgStr = Tcl_GetStringFromObj(obj, &len);

    len++;
    ForwardSetDynamicError(paramPtr, ckalloc(len));
    memcpy(paramPtr->base.msgStr, msgStr, (unsigned) len);
}
#endif /* TCL_THREADS */

/*
 *----------------------------------------------------------------------
 *
 * TimerKill --
 *
 *	Timer management. Removes the internal timer if it exists.
 *
 * Side effects:
 *	See above.
 *
 * Result:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
TimerKill(
    ReflectedTransform *rtPtr)
{
    if (rtPtr->timer == NULL) {
	return;
    }

    /*
     * Delete an existing flush-out timer, prevent it from firing on a
     * removed/dead channel.
     */

    Tcl_DeleteTimerHandler(rtPtr->timer);
    rtPtr->timer = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TimerSetup --
 *
 *	Timer management. Creates the internal timer if it does not exist.
 *
 * Side effects:
 *	See above.
 *
 * Result:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
TimerSetup(
    ReflectedTransform *rtPtr)
{
    if (rtPtr->timer != NULL) {
	return;
    }

    rtPtr->timer = Tcl_CreateTimerHandler(SYNTHETIC_EVENT_TIME,
	    TimerRun, rtPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TimerRun --
 *
 *	Called by the notifier (-> timer) to flush out information waiting in
 *	channel buffers.
 *
 * Side effects:
 *	As of 'Tcl_NotifyChannel'.
 *
 * Result:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
TimerRun(
    ClientData clientData)
{
    ReflectedTransform *rtPtr = clientData;

    rtPtr->timer = NULL;
    Tcl_NotifyChannel(rtPtr->chan, TCL_READABLE);
}

/*
 *----------------------------------------------------------------------
 *
 * ResultInit --
 *
 *	Initializes the specified buffer structure. The structure will contain
 *	valid information for an emtpy buffer.
 *
 * Side effects:
 *	See above.
 *
 * Result:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
ResultInit(
    ResultBuffer *rPtr)		/* Reference to the structure to
				 * initialize. */
{
    rPtr->used = 0;
    rPtr->allocated = 0;
    rPtr->buf = NULL;
}
/*
 *----------------------------------------------------------------------
 *
 * ResultClear --
 *
 *	Deallocates any memory allocated by 'ResultAdd'.
 *
 * Side effects:
 *	See above.
 *
 * Result:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
ResultClear(
    ResultBuffer *rPtr)		/* Reference to the buffer to clear out */
{
    rPtr->used = 0;

    if (!rPtr->allocated) {
	return;
    }

    ckfree((char *) rPtr->buf);
    rPtr->buf = NULL;
    rPtr->allocated = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * ResultAdd --
 *
 *	Adds the bytes in the specified array to the buffer, by appending it.
 *
 * Side effects:
 *	See above.
 *
 * Result:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
ResultAdd(
    ResultBuffer *rPtr,		/* The buffer to extend */
    unsigned char *buf,		/* The buffer to read from */
    int toWrite)		/* The number of bytes in 'buf' */
{
    if ((rPtr->used + toWrite + 1) > rPtr->allocated) {
	/*
	 * Extension of the internal buffer is required.
	 * NOTE: Currently linear. Should be doubling to amortize.
	 */

	if (rPtr->allocated == 0) {
	    rPtr->allocated = toWrite + RB_INCREMENT;
	    rPtr->buf = UCHARP(ckalloc(rPtr->allocated));
	} else {
	    rPtr->allocated += toWrite + RB_INCREMENT;
	    rPtr->buf = UCHARP(ckrealloc((char *) rPtr->buf,
		    rPtr->allocated));
	}
    }

    /*
     * Now copy data.
     */

    memcpy(rPtr->buf + rPtr->used, buf, toWrite);
    rPtr->used += toWrite;
}

/*
 *----------------------------------------------------------------------
 *
 * ResultCopy --
 *
 *	Copies the requested number of bytes from the buffer into the
 *	specified array and removes them from the buffer afterward. Copies
 *	less if there is not enough data in the buffer.
 *
 * Side effects:
 *	See above.
 *
 * Result:
 *	The number of actually copied bytes, possibly less than 'toRead'.
 *
 *----------------------------------------------------------------------
 */

static int
ResultCopy(
    ResultBuffer *rPtr,		/* The buffer to read from */
    unsigned char *buf,		/* The buffer to copy into */
    int toRead)			/* Number of requested bytes */
{
    int copied;

    if (rPtr->used == 0) {
	/*
	 * Nothing to copy in the case of an empty buffer.
	 */

	copied = 0;
    } else if (rPtr->used == toRead) {
	/*
	 * We have just enough. Copy everything to the caller.
	 */

	memcpy(buf, rPtr->buf, toRead);
	rPtr->used = 0;
	copied = toRead;
    } else if (rPtr->used > toRead) {
	/*
	 * The internal buffer contains more than requested. Copy the
	 * requested subset to the caller, and shift the remaining bytes down.
	 */

	memcpy(buf, rPtr->buf, toRead);
	memmove(rPtr->buf, rPtr->buf + toRead, rPtr->used - toRead);

	rPtr->used -= toRead;
	copied = toRead;
    } else {
	/*
	 * There is not enough in the buffer to satisfy the caller, so take
	 * everything.
	 */

	memcpy(buf, rPtr->buf, rPtr->used);
	toRead = rPtr->used;
	rPtr->used = 0;
	copied = toRead;
    }

    /* -- common postwork code ------- */

    return copied;
}

static int
TransformRead(
    ReflectedTransform *rtPtr,
    int *errorCodePtr,
    Tcl_Obj *bufObj)
{
    Tcl_Obj *resObj;
    int bytec;			/* Number of returned bytes */
    unsigned char *bytev;	/* Array of returned bytes */

    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rtPtr->thread != Tcl_GetCurrentThread()) {
	ForwardParam p;

	p.transform.buf = (char *) Tcl_GetByteArrayFromObj(bufObj,
		&(p.transform.size));

	ForwardOpToOwnerThread(rtPtr, ForwardedInput, &p);

	if (p.base.code != TCL_OK) {
	    PassReceivedError(rtPtr->chan, &p);
	    *errorCodePtr = EINVAL;
	    return 0;
	}

	*errorCodePtr = EOK;
	ResultAdd(&rtPtr->result, UCHARP(p.transform.buf), p.transform.size);
	ckfree(p.transform.buf);
	return 1;
    }
#endif /* TCL_THREADS */

    /* ASSERT: rtPtr->method & FLAG(METH_READ) */
    /* ASSERT: rtPtr->mode & TCL_READABLE */

    if (InvokeTclMethod(rtPtr, "read", bufObj, NULL, &resObj) != TCL_OK) {
	Tcl_SetChannelError(rtPtr->chan, resObj);
	Tcl_DecrRefCount(resObj);	/* Remove reference held from invoke */
	*errorCodePtr = EINVAL;
	return 0;
    }

    bytev = Tcl_GetByteArrayFromObj(resObj, &bytec);
    ResultAdd(&rtPtr->result, bytev, bytec);

    Tcl_DecrRefCount(resObj);		/* Remove reference held from invoke */
    return 1;
}

static int
TransformWrite(
    ReflectedTransform *rtPtr,
    int *errorCodePtr,
    unsigned char *buf,
    int toWrite)
{
    Tcl_Obj *bufObj;
    Tcl_Obj *resObj;
    int bytec;			/* Number of returned bytes */
    unsigned char *bytev;	/* Array of returned bytes */
    int res;

    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rtPtr->thread != Tcl_GetCurrentThread()) {
	ForwardParam p;

	p.transform.buf = (char *) buf;
	p.transform.size = toWrite;

	ForwardOpToOwnerThread(rtPtr, ForwardedOutput, &p);

	if (p.base.code != TCL_OK) {
	    PassReceivedError(rtPtr->chan, &p);
	    *errorCodePtr = EINVAL;
	    return 0;
	}

	*errorCodePtr = EOK;
	res = Tcl_WriteRaw(rtPtr->parent, (char *) p.transform.buf,
		p.transform.size);
	ckfree(p.transform.buf);
    } else
#endif /* TCL_THREADS */
    {
	/* ASSERT: rtPtr->method & FLAG(METH_WRITE) */
	/* ASSERT: rtPtr->mode & TCL_WRITABLE */

	bufObj = Tcl_NewByteArrayObj((unsigned char *) buf, toWrite);
	Tcl_IncrRefCount(bufObj);
	if (InvokeTclMethod(rtPtr, "write", bufObj, NULL, &resObj) != TCL_OK) {
	    *errorCodePtr = EINVAL;
	    Tcl_SetChannelError(rtPtr->chan, resObj);

	    Tcl_DecrRefCount(bufObj);
	    Tcl_DecrRefCount(resObj);	/* Remove reference held from invoke */
	    return 0;
	}

	*errorCodePtr = EOK;

	bytev = Tcl_GetByteArrayFromObj(resObj, &bytec);
	res = Tcl_WriteRaw(rtPtr->parent, (char *) bytev, bytec);

	Tcl_DecrRefCount(bufObj);
	Tcl_DecrRefCount(resObj);	/* Remove reference held from invoke */
    }

    if (res < 0) {
	*errorCodePtr = Tcl_GetErrno();
	return 0;
    }

    return 1;
}

static int
TransformDrain(
    ReflectedTransform *rtPtr,
    int *errorCodePtr)
{
    Tcl_Obj *resObj;
    int bytec;			/* Number of returned bytes */
    unsigned char *bytev;	/* Array of returned bytes */

    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rtPtr->thread != Tcl_GetCurrentThread()) {
	ForwardParam p;

	ForwardOpToOwnerThread(rtPtr, ForwardedDrain, &p);

	if (p.base.code != TCL_OK) {
	    PassReceivedError(rtPtr->chan, &p);
	    *errorCodePtr = EINVAL;
	    return 0;
	}

	*errorCodePtr = EOK;
	ResultAdd(&rtPtr->result, UCHARP(p.transform.buf), p.transform.size);
	ckfree(p.transform.buf);
    } else
#endif /* TCL_THREADS */
    {
	if (InvokeTclMethod(rtPtr, "drain", NULL, NULL, &resObj)!=TCL_OK) {
	    Tcl_SetChannelError(rtPtr->chan, resObj);
	    Tcl_DecrRefCount(resObj);	/* Remove reference held from invoke */
	    *errorCodePtr = EINVAL;
	    return 0;
	}

	bytev = Tcl_GetByteArrayFromObj(resObj, &bytec);
	ResultAdd(&rtPtr->result, bytev, bytec);

	Tcl_DecrRefCount(resObj);	/* Remove reference held from invoke */
    }

    rtPtr->readIsDrained = 1;
    return 1;
}

static int
TransformFlush(
    ReflectedTransform *rtPtr,
    int *errorCodePtr,
    int op)
{
    Tcl_Obj *resObj;
    int bytec;			/* Number of returned bytes */
    unsigned char *bytev;	/* Array of returned bytes */
    int res;

    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rtPtr->thread != Tcl_GetCurrentThread()) {
	ForwardParam p;

	ForwardOpToOwnerThread(rtPtr, ForwardedFlush, &p);

	if (p.base.code != TCL_OK) {
	    PassReceivedError(rtPtr->chan, &p);
	    *errorCodePtr = EINVAL;
	    return 0;
	}

	*errorCodePtr = EOK;
	if (op == FLUSH_WRITE) {
	    res = Tcl_WriteRaw(rtPtr->parent, (char *) p.transform.buf,
		    p.transform.size);
	} else {
	    res = 0;
	}
	ckfree(p.transform.buf);
    } else
#endif /* TCL_THREADS */
    {
	if (InvokeTclMethod(rtPtr, "flush", NULL, NULL, &resObj)!=TCL_OK) {
	    Tcl_SetChannelError(rtPtr->chan, resObj);
	    Tcl_DecrRefCount(resObj);	/* Remove reference held from invoke */
	    *errorCodePtr = EINVAL;
	    return 0;
	}

	if (op == FLUSH_WRITE) {
	    bytev = Tcl_GetByteArrayFromObj(resObj, &bytec);
	    res = Tcl_WriteRaw(rtPtr->parent, (char *) bytev, bytec);
	} else {
	    res = 0;
	}
	Tcl_DecrRefCount(resObj);	/* Remove reference held from invoke */
    }

    if (res < 0) {
	*errorCodePtr = Tcl_GetErrno();
	return 0;
    }

    return 1;
}

static void
TransformClear(
    ReflectedTransform *rtPtr)
{
    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rtPtr->thread != Tcl_GetCurrentThread()) {
	ForwardParam p;

	ForwardOpToOwnerThread(rtPtr, ForwardedClear, &p);
	return;
    }
#endif /* TCL_THREADS */

    /* ASSERT: rtPtr->method & FLAG(METH_READ) */
    /* ASSERT: rtPtr->mode & TCL_READABLE */

    (void) InvokeTclMethod(rtPtr, "clear", NULL, NULL, NULL);

    rtPtr->readIsDrained = 0;
    rtPtr->eofPending = 0;
    ResultClear(&rtPtr->result);
}

static int
TransformLimit(
    ReflectedTransform *rtPtr,
    int *errorCodePtr,
    int *maxPtr)
{
    Tcl_Obj *resObj;
    Tcl_InterpState sr;		/* State of handler interp */

    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rtPtr->thread != Tcl_GetCurrentThread()) {
	ForwardParam p;

	ForwardOpToOwnerThread(rtPtr, ForwardedLimit, &p);

	if (p.base.code != TCL_OK) {
	    PassReceivedError(rtPtr->chan, &p);
	    *errorCodePtr = EINVAL;
	    return 0;
	}

	*errorCodePtr = EOK;
	*maxPtr = p.limit.max;
	return 1;
    }
#endif

    /* ASSERT: rtPtr->method & FLAG(METH_WRITE) */
    /* ASSERT: rtPtr->mode & TCL_WRITABLE */

    if (InvokeTclMethod(rtPtr, "limit?", NULL, NULL, &resObj) != TCL_OK) {
	Tcl_SetChannelError(rtPtr->chan, resObj);
	Tcl_DecrRefCount(resObj);	/* Remove reference held from invoke */
	*errorCodePtr = EINVAL;
	return 0;
    }

    sr = Tcl_SaveInterpState(rtPtr->interp, 0 /* Dummy */);

    if (Tcl_GetIntFromObj(rtPtr->interp, resObj, maxPtr) != TCL_OK) {
	Tcl_DecrRefCount(resObj);	/* Remove reference held from invoke */
	Tcl_SetChannelError(rtPtr->chan, MarshallError(rtPtr->interp));
	*errorCodePtr = EINVAL;

	Tcl_RestoreInterpState(rtPtr->interp, sr);
	return 0;
    }

    Tcl_DecrRefCount(resObj);		/* Remove reference held from invoke */
    Tcl_RestoreInterpState(rtPtr->interp, sr);
    return 1;
}

/* DUPLICATE of HaveVersion() in tclIO.c
 *----------------------------------------------------------------------
 *
 * HaveVersion --
 *
 *	Return whether a channel type is (at least) of a given version.
 *
 * Results:
 *	True if the minimum version is exceeded by the version actually
 *	present.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
HaveVersion(
    const Tcl_ChannelType *chanTypePtr,
    Tcl_ChannelTypeVersion minimumVersion)
{
    Tcl_ChannelTypeVersion actualVersion = Tcl_ChannelVersion(chanTypePtr);

    return PTR2INT(actualVersion) >= PTR2INT(minimumVersion);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
