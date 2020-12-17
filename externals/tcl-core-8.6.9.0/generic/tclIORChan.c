/*
 * tclIORChan.c --
 *
 *	This file contains the implementation of Tcl's generic channel
 *	reflection code, which allows the implementation of Tcl channels in
 *	Tcl code.
 *
 *	Parts of this file are based on code contributed by Jean-Claude
 *	Wippler.
 *
 *	See TIP #219 for the specification of this functionality.
 *
 * Copyright (c) 2004-2005 ActiveState, a divison of Sophos
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
#ifdef TCL_THREADS
static void		ReflectThread(ClientData clientData, int action);
static int		ReflectEventRun(Tcl_Event *ev, int flags);
static int		ReflectEventDelete(Tcl_Event *ev, ClientData cd);
#endif
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

/*
 * The C layer channel type/driver definition used by the reflection. This is
 * a version 3 structure.
 */

static const Tcl_ChannelType tclRChannelType = {
    "tclrchannel",	   /* Type name.				  */
    TCL_CHANNEL_VERSION_5, /* v5 channel */
    ReflectClose,	   /* Close channel, clean instance data	  */
    ReflectInput,	   /* Handle read request			  */
    ReflectOutput,	   /* Handle write request			  */
    ReflectSeek,	   /* Move location of access point.	NULL'able */
    ReflectSetOption,	   /* Set options.			NULL'able */
    ReflectGetOption,	   /* Get options.			NULL'able */
    ReflectWatch,	   /* Initialize notifier			  */
    NULL,		   /* Get OS handle from the channel.	NULL'able */
    NULL,		   /* No close2 support.		NULL'able */
    ReflectBlock,	   /* Set blocking/nonblocking.		NULL'able */
    NULL,		   /* Flush channel. Not used by core.	NULL'able */
    NULL,		   /* Handle events.			NULL'able */
    ReflectSeekWide,	   /* Move access point (64 bit).	NULL'able */
#ifdef TCL_THREADS
    ReflectThread,         /* thread action, tracking owner */
#else
    NULL,		   /* thread action */
#endif
    NULL		   /* truncate */
};

/*
 * Instance data for a reflected channel. ===========================
 */

typedef struct {
    Tcl_Channel chan;		/* Back reference to generic channel
				 * structure. */
    Tcl_Interp *interp;		/* Reference to the interpreter containing the
				 * Tcl level part of the channel. NULL here
				 * signals the channel is dead because the
				 * interpreter/thread containing its Tcl
				 * command is gone.
				 */
#ifdef TCL_THREADS
    Tcl_ThreadId thread;	/* Thread the 'interp' belongs to. == Handler thread */
    Tcl_ThreadId owner;         /* Thread owning the structure.    == Channel thread */
#endif
    Tcl_Obj *cmd;		/* Callback command prefix */
    Tcl_Obj *methods;		/* Methods to append to command prefix */
    Tcl_Obj *name;		/* Name of the channel as created */

    int mode;			/* Mask of R/W mode */
    int interest;		/* Mask of events the channel is interested
				 * in. */

    int dead;			/* Boolean signal that some operations
				 * should no longer be attempted. */

    /*
     * Note regarding the usage of timers.
     *
     * Most channel implementations need a timer in the C level to ensure that
     * data in buffers is flushed out through the generation of fake file
     * events.
     *
     * See 'rechan', 'memchan', etc.
     *
     * Here this is _not_ required. Interest in events is posted to the Tcl
     * level via 'watch'. And posting of events is possible from the Tcl level
     * as well, via 'chan postevent'. This means that the generation of all
     * events, fake or not, timer based or not, is completely in the hands of
     * the Tcl level. Therefore no timer here.
     */
} ReflectedChannel;

/*
 * Structure of the table maping from channel handles to reflected
 * channels. Each interpreter which has the handler command for one or more
 * reflected channels records them in such a table, so that 'chan postevent'
 * is able to find them even if the actual channel was moved to a different
 * interpreter and/or thread.
 *
 * The table is reachable via the standard interpreter AssocData, the key is
 * defined below.
 */

typedef struct {
    Tcl_HashTable map;
} ReflectedChannelMap;

#define RCMKEY "ReflectedChannelMap"

/*
 * Event literals. ==================================================
 */

static const char *const eventOptions[] = {
    "read", "write", NULL
};
typedef enum {
    EVENT_READ, EVENT_WRITE
} EventOption;

/*
 * Method literals. ==================================================
 */

static const char *const methodNames[] = {
    "blocking",		/* OPT */
    "cget",		/* OPT \/ Together or none */
    "cgetall",		/* OPT /\ of these two     */
    "configure",	/* OPT */
    "finalize",		/*     */
    "initialize",	/*     */
    "read",		/* OPT */
    "seek",		/* OPT */
    "watch",		/*     */
    "write",		/* OPT */
    NULL
};
typedef enum {
    METH_BLOCKING,
    METH_CGET,
    METH_CGETALL,
    METH_CONFIGURE,
    METH_FINAL,
    METH_INIT,
    METH_READ,
    METH_SEEK,
    METH_WATCH,
    METH_WRITE
} MethodName;

#define FLAG(m) (1 << (m))
#define REQUIRED_METHODS \
	(FLAG(METH_INIT) | FLAG(METH_FINAL) | FLAG(METH_WATCH))
#define NULLABLE_METHODS \
	(FLAG(METH_BLOCKING) | FLAG(METH_SEEK) | \
	FLAG(METH_CONFIGURE) | FLAG(METH_CGET) | FLAG(METH_CGETALL))

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
    ForwardedClose,
    ForwardedInput,
    ForwardedOutput,
    ForwardedSeek,
    ForwardedWatch,
    ForwardedBlock,
    ForwardedSetOpt,
    ForwardedGetOpt,
    ForwardedGetOptAll
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

struct ForwardParamInput {
    ForwardParamBase base;	/* "Supertype". MUST COME FIRST. */
    char *buf;			/* O: Where to store the read bytes */
    int toRead;			/* I: #bytes to read,
				 * O: #bytes actually read */
};
struct ForwardParamOutput {
    ForwardParamBase base;	/* "Supertype". MUST COME FIRST. */
    const char *buf;		/* I: Where the bytes to write come from */
    int toWrite;		/* I: #bytes to write,
				 * O: #bytes actually written */
};
struct ForwardParamSeek {
    ForwardParamBase base;	/* "Supertype". MUST COME FIRST. */
    int seekMode;		/* I: How to seek */
    Tcl_WideInt offset;		/* I: Where to seek,
				 * O: New location */
};
struct ForwardParamWatch {
    ForwardParamBase base;	/* "Supertype". MUST COME FIRST. */
    int mask;			/* I: What events to watch for */
};
struct ForwardParamBlock {
    ForwardParamBase base;	/* "Supertype". MUST COME FIRST. */
    int nonblocking;		/* I: What mode to activate */
};
struct ForwardParamSetOpt {
    ForwardParamBase base;	/* "Supertype". MUST COME FIRST. */
    const char *name;		/* Name of option to set */
    const char *value;		/* Value to set */
};
struct ForwardParamGetOpt {
    ForwardParamBase base;	/* "Supertype". MUST COME FIRST. */
    const char *name;		/* Name of option to get, maybe NULL */
    Tcl_DString *value;		/* Result */
};

/*
 * Now join all these together in a single union for convenience.
 */

typedef union ForwardParam {
    ForwardParamBase base;
    struct ForwardParamInput input;
    struct ForwardParamOutput output;
    struct ForwardParamSeek seek;
    struct ForwardParamWatch watch;
    struct ForwardParamBlock block;
    struct ForwardParamSetOpt setOpt;
    struct ForwardParamGetOpt getOpt;
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
    ReflectedChannel *rcPtr;	/* Channel instance */
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
    /*
     * Note regarding 'dsti' above: Its information is also available via the
     * chain evPtr->rcPtr->interp, however, as can be seen, two more
     * indirections are needed to retrieve it. And the evPtr may be gone,
     * breaking the chain.
     */
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
     * Table of all reflected channels owned by this thread. This is the
     * per-thread version of the per-interpreter map.
     */

    ReflectedChannelMap *rcmPtr;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * List of forwarded operations which have not completed yet, plus the mutex
 * to protect the access to this process global list.
 */

static ForwardingResult *forwardList = NULL;
TCL_DECLARE_MUTEX(rcForwardMutex)

/*
 * Function containing the generic code executing a forward, and wrapper
 * macros for the actual operations we wish to forward. Uses ForwardProc as
 * the event function executed by the thread receiving a forwarding event
 * (which executes the appropriate function and collects the result, if any).
 *
 * The ExitProc ensures that things do not deadlock when the sending thread
 * involved in the forwarding exits. It also clean things up so that we don't
 * leak resources when threads go away.
 */

static void		ForwardOpToHandlerThread(ReflectedChannel *rcPtr,
			    ForwardedOperation op, const void *param);
static int		ForwardProc(Tcl_Event *evPtr, int mask);
static void		SrcExitProc(ClientData clientData);

#define FreeReceivedError(p) \
	if ((p)->base.mustFree) {                               \
	    ckfree((p)->base.msgStr);                           \
	}
#define PassReceivedErrorInterp(i,p) \
	if ((i) != NULL) {                                      \
	    Tcl_SetChannelErrorInterp((i),                      \
		    Tcl_NewStringObj((p)->base.msgStr, -1));    \
	}                                                       \
	FreeReceivedError(p)
#define PassReceivedError(c,p) \
	Tcl_SetChannelError((c), Tcl_NewStringObj((p)->base.msgStr, -1)); \
	FreeReceivedError(p)
#define ForwardSetStaticError(p,emsg) \
	(p)->base.code = TCL_ERROR;                             \
	(p)->base.mustFree = 0;                                 \
	(p)->base.msgStr = (char *) (emsg)
#define ForwardSetDynamicError(p,emsg) \
	(p)->base.code = TCL_ERROR;                             \
	(p)->base.mustFree = 1;                                 \
	(p)->base.msgStr = (char *) (emsg)

static void		ForwardSetObjError(ForwardParam *p, Tcl_Obj *objPtr);

static ReflectedChannelMap *	GetThreadReflectedChannelMap(void);
static void		DeleteThreadReflectedChannelMap(ClientData clientData);

#endif /* TCL_THREADS */

#define SetChannelErrorStr(c,msgStr) \
	Tcl_SetChannelError((c), Tcl_NewStringObj((msgStr), -1))

static Tcl_Obj *	MarshallError(Tcl_Interp *interp);
static void		UnmarshallErrorResult(Tcl_Interp *interp,
			    Tcl_Obj *msgObj);

/*
 * Static functions for this file:
 */

static int		EncodeEventMask(Tcl_Interp *interp,
			    const char *objName, Tcl_Obj *obj, int *mask);
static Tcl_Obj *	DecodeEventMask(int mask);
static ReflectedChannel * NewReflectedChannel(Tcl_Interp *interp,
			    Tcl_Obj *cmdpfxObj, int mode, Tcl_Obj *handleObj);
static Tcl_Obj *	NextHandle(void);
static void		FreeReflectedChannel(ReflectedChannel *rcPtr);
static int		InvokeTclMethod(ReflectedChannel *rcPtr,
			    MethodName method, Tcl_Obj *argOneObj,
			    Tcl_Obj *argTwoObj, Tcl_Obj **resultObjPtr);

static ReflectedChannelMap *	GetReflectedChannelMap(Tcl_Interp *interp);
static void		DeleteReflectedChannelMap(ClientData clientData,
			    Tcl_Interp *interp);
static int		ErrnoReturn(ReflectedChannel *rcPtr, Tcl_Obj *resObj);
static void		MarkDead(ReflectedChannel *rcPtr);

/*
 * Global constant strings (messages). ==================
 * These string are used directly as bypass errors, thus they have to be valid
 * Tcl lists where the last element is the message itself. Hence the
 * list-quoting to keep the words of the message together. See also [x].
 */

static const char *msg_read_toomuch = "{read delivered more than requested}";
static const char *msg_write_toomuch = "{write wrote more than requested}";
static const char *msg_write_nothing = "{write wrote nothing}";
static const char *msg_seek_beforestart = "{Tried to seek before origin}";
#ifdef TCL_THREADS
static const char *msg_send_originlost = "{Channel thread lost}";
#endif /* TCL_THREADS */
static const char *msg_send_dstlost    = "{Owner lost}";
static const char *msg_dstlost    = "-code 1 -level 0 -errorcode NONE -errorinfo {} -errorline 1 {Owner lost}";

/*
 * Main methods to plug into the 'chan' ensemble'. ==================
 */

/*
 *----------------------------------------------------------------------
 *
 * TclChanCreateObjCmd --
 *
 *	This function is invoked to process the "chan create" Tcl command.
 *	See the user documentation for details on what it does.
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
TclChanCreateObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    ReflectedChannel *rcPtr;	/* Instance data of the new channel */
    Tcl_Obj *rcId;		/* Handle of the new channel */
    int mode;			/* R/W mode of new channel. Has to match
				 * abilities of handler commands */
    Tcl_Obj *cmdObj;		/* Command prefix, list of words */
    Tcl_Obj *cmdNameObj;	/* Command name */
    Tcl_Channel chan;		/* Token for the new channel */
    Tcl_Obj *modeObj;		/* mode in obj form for method call */
    int listc;			/* Result of 'initialize', and of */
    Tcl_Obj **listv;		/* its sublist in the 2nd element */
    int methIndex;		/* Encoded method name */
    int result;			/* Result code for 'initialize' */
    Tcl_Obj *resObj;		/* Result data for 'initialize' */
    int methods;		/* Bitmask for supported methods. */
    Channel *chanPtr;		/* 'chan' resolved to internal struct. */
    Tcl_Obj *err;		/* Error message */
    ReflectedChannelMap *rcmPtr;
				/* Map of reflected channels with handlers in
				 * this interp. */
    Tcl_HashEntry *hPtr;	/* Entry in the above map */
    int isNew;			/* Placeholder. */

    /*
     * Syntax:   chan create MODE CMDPREFIX
     *           [0]  [1]    [2]  [3]
     *
     * Actually: rCreate MODE CMDPREFIX
     *           [0]     [1]  [2]
     */

#define MODE	(1)
#define CMD	(2)

    /*
     * Number of arguments...
     */

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "mode cmdprefix");
	return TCL_ERROR;
    }

    /*
     * First argument is a list of modes. Allowed entries are "read", "write".
     * Expect at least one list element. Abbreviations are ok.
     */

    modeObj = objv[MODE];
    if (EncodeEventMask(interp, "mode", objv[MODE], &mode) != TCL_OK) {
	return TCL_ERROR;
    }

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
     * Now create the channel.
     */

    rcId = NextHandle();
    rcPtr = NewReflectedChannel(interp, cmdObj, mode, rcId);

    /*
     * Invoke 'initialize' and validate that the handler is present and ok.
     * Squash the channel if not.
     *
     * Note: The conversion of 'mode' back into a Tcl_Obj ensures that
     * 'initialize' is invoked with canonical mode names, and no
     * abbreviations. Using modeObj directly could feed abbreviations into the
     * handler, and the handler is not specified to handle such.
     */

    modeObj = DecodeEventMask(mode);
    /* assert modeObj.refCount == 1 */
    result = InvokeTclMethod(rcPtr, METH_INIT, modeObj, NULL, &resObj);
    Tcl_DecrRefCount(modeObj);

    if (result != TCL_OK) {
	UnmarshallErrorResult(interp, resObj);
	Tcl_DecrRefCount(resObj);	/* Remove reference held from invoke */
	goto error;
    }

    /*
     * Verify the result.
     * - List, of method names. Convert to mask.
     *   Check for non-optionals through the mask.
     *   Compare open mode against optional r/w.
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
	    TclNewLiteralStringObj(err, "chan handler \"");
	    Tcl_AppendObjToObj(err, cmdObj);
	    Tcl_AppendToObj(err, " initialize\" returned ", -1);
	    Tcl_AppendObjToObj(err, Tcl_GetObjResult(interp));
	    Tcl_SetObjResult(interp, err);
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

    if ((mode & TCL_READABLE) && !HAS(methods, METH_READ)) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "chan handler \"%s\" lacks a \"read\" method",
                Tcl_GetString(cmdObj)));
	goto error;
    }

    if ((mode & TCL_WRITABLE) && !HAS(methods, METH_WRITE)) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "chan handler \"%s\" lacks a \"write\" method",
                Tcl_GetString(cmdObj)));
	goto error;
    }

    if (!IMPLIES(HAS(methods, METH_CGET), HAS(methods, METH_CGETALL))) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "chan handler \"%s\" supports \"cget\" but not \"cgetall\"",
                Tcl_GetString(cmdObj)));
	goto error;
    }

    if (!IMPLIES(HAS(methods, METH_CGETALL), HAS(methods, METH_CGET))) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "chan handler \"%s\" supports \"cgetall\" but not \"cget\"",
                Tcl_GetString(cmdObj)));
	goto error;
    }

    Tcl_ResetResult(interp);

    /*
     * Everything is fine now.
     */

    chan = Tcl_CreateChannel(&tclRChannelType, TclGetString(rcId), rcPtr,
	    mode);
    rcPtr->chan = chan;
    TclChannelPreserve(chan);
    chanPtr = (Channel *) chan;

    if ((methods & NULLABLE_METHODS) != NULLABLE_METHODS) {
	/*
	 * Some of the nullable methods are not supported. We clone the
	 * channel type, null the associated C functions, and use the result
	 * as the actual channel type.
	 */

	Tcl_ChannelType *clonePtr = ckalloc(sizeof(Tcl_ChannelType));

	memcpy(clonePtr, &tclRChannelType, sizeof(Tcl_ChannelType));

	if (!(methods & FLAG(METH_CONFIGURE))) {
	    clonePtr->setOptionProc = NULL;
	}

	if (!(methods & FLAG(METH_CGET)) && !(methods & FLAG(METH_CGETALL))) {
	    clonePtr->getOptionProc = NULL;
	}
	if (!(methods & FLAG(METH_BLOCKING))) {
	    clonePtr->blockModeProc = NULL;
	}
	if (!(methods & FLAG(METH_SEEK))) {
	    clonePtr->seekProc = NULL;
	    clonePtr->wideSeekProc = NULL;
	}

	chanPtr->typePtr = clonePtr;
    }

    /*
     * Register the channel in the I/O system, and in our our map for 'chan
     * postevent'.
     */

    Tcl_RegisterChannel(interp, chan);

    rcmPtr = GetReflectedChannelMap(interp);
    hPtr = Tcl_CreateHashEntry(&rcmPtr->map, chanPtr->state->channelName,
	    &isNew);
    if (!isNew && chanPtr != Tcl_GetHashValue(hPtr)) {
	Tcl_Panic("TclChanCreateObjCmd: duplicate channel names");
    }
    Tcl_SetHashValue(hPtr, chan);
#ifdef TCL_THREADS
    rcmPtr = GetThreadReflectedChannelMap();
    hPtr = Tcl_CreateHashEntry(&rcmPtr->map, chanPtr->state->channelName,
	    &isNew);
    Tcl_SetHashValue(hPtr, chan);
#endif

    /*
     * Return handle as result of command.
     */

    Tcl_SetObjResult(interp,
            Tcl_NewStringObj(chanPtr->state->channelName, -1));
    return TCL_OK;

  error:
    Tcl_DecrRefCount(rcPtr->name);
    Tcl_DecrRefCount(rcPtr->methods);
    Tcl_DecrRefCount(rcPtr->cmd);
    ckfree((char*) rcPtr);
    return TCL_ERROR;

#undef MODE
#undef CMD
}

/*
 *----------------------------------------------------------------------
 *
 * TclChanPostEventObjCmd --
 *
 *	This function is invoked to process the "chan postevent" Tcl command.
 *	See the user documentation for details on what it does.
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

#ifdef TCL_THREADS
typedef struct ReflectEvent {
    Tcl_Event header;
    ReflectedChannel *rcPtr;
    int events;
} ReflectEvent;

static int
ReflectEventRun(
    Tcl_Event *ev,
    int flags)
{
    /* OWNER thread
     *
     * Note: When the channel is closed any pending events of this type are
     * deleted. See ReflectClose() for the Tcl_DeleteEvents() calls
     * accomplishing that.
     */

    ReflectEvent *e = (ReflectEvent *) ev;

    Tcl_NotifyChannel(e->rcPtr->chan, e->events);
    return 1;
}

static int
ReflectEventDelete(
    Tcl_Event *ev,
    ClientData cd)
{
    /* OWNER thread
     *
     * Invoked by DeleteThreadReflectedChannelMap() and ReflectClose(). The
     * latter ensures that no pending events of this type are run on an
     * invalid channel.
     */

    ReflectEvent *e = (ReflectEvent *) ev;

    if ((ev->proc != ReflectEventRun) || ((cd != NULL) && (cd != e->rcPtr))) {
        return 0;
    }
    return 1;
}
#endif

int
TclChanPostEventObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    /*
     * Ensure -> HANDLER thread
     *
     * Syntax:   chan postevent CHANNEL EVENTSPEC
     *           [0]  [1]       [2]     [3]
     *
     * Actually: rPostevent CHANNEL EVENTSPEC
     *           [0]        [1]     [2]
     *
     * where EVENTSPEC = {read write ...} (Abbreviations allowed as well).
     */

#define CHAN	(1)
#define EVENT	(2)

    const char *chanId;		/* Tcl level channel handle */
    Tcl_Channel chan;		/* Channel associated to the handle */
    const Tcl_ChannelType *chanTypePtr;
				/* Its associated driver structure */
    ReflectedChannel *rcPtr;	/* Associated instance data */
    int events;			/* Mask of events to post */
    ReflectedChannelMap *rcmPtr;/* Map of reflected channels with handlers in
				 * this interp. */
    Tcl_HashEntry *hPtr;	/* Entry in the above map */

    /*
     * Number of arguments...
     */

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "channel eventspec");
	return TCL_ERROR;
    }

    /*
     * First argument is a channel, a reflected channel, and the call of this
     * command is done from the interp defining the channel handler cmd.
     */

    chanId = TclGetString(objv[CHAN]);

    rcmPtr = GetReflectedChannelMap(interp);
    hPtr = Tcl_FindHashEntry(&rcmPtr->map, chanId);

    if (hPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "can not find reflected channel named \"%s\"", chanId));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "CHANNEL", chanId, NULL);
	return TCL_ERROR;
    }

    /*
     * Note that the search above subsumes several of the older checks, namely:
     *
     * (1) Does the channel handle refer to a reflected channel ?
     * (2) Is the post event issued from the interpreter holding the handler
     *     of the reflected channel ?
     *
     * A successful search answers yes to both. Because the map holds only
     * handles of reflected channels, and only of such whose handler is
     * defined in this interpreter.
     *
     * We keep the old checks for both, for paranioa, but abort now instead of
     * throwing errors, as failure now means that our internal datastructures
     * have gone seriously haywire.
     */

    chan = Tcl_GetHashValue(hPtr);
    chanTypePtr = Tcl_GetChannelType(chan);

    /*
     * We use a function referenced by the channel type as our cookie to
     * detect calls to non-reflecting channels. The channel type itself is not
     * suitable, as it might not be the static definition in this file, but a
     * clone thereof. And while we have reserved the name of the type nothing
     * in the core checks against violation, so someone else might have
     * created a channel type using our name, clashing with ourselves.
     */

    if (chanTypePtr->watchProc != &ReflectWatch) {
	Tcl_Panic("TclChanPostEventObjCmd: channel is not a reflected channel");
    }

    rcPtr = Tcl_GetChannelInstanceData(chan);

    if (rcPtr->interp != interp) {
	Tcl_Panic("TclChanPostEventObjCmd: postevent accepted for call from outside interpreter");
    }

    /*
     * Second argument is a list of events. Allowed entries are "read",
     * "write". Expect at least one list element. Abbreviations are ok.
     */

    if (EncodeEventMask(interp, "event", objv[EVENT], &events) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Check that the channel is actually interested in the provided events.
     */

    if (events & ~rcPtr->interest) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "tried to post events channel \"%s\" is not interested in",
                chanId));
	return TCL_ERROR;
    }

    /*
     * We have the channel and the events to post.
     */

#ifdef TCL_THREADS
    if (rcPtr->owner == rcPtr->thread) {
#endif
        Tcl_NotifyChannel(chan, events);
#ifdef TCL_THREADS
    } else {
        ReflectEvent *ev = ckalloc(sizeof(ReflectEvent));

        ev->header.proc = ReflectEventRun;
        ev->events = events;
        ev->rcPtr = rcPtr;

        /*
         * We are not preserving the structure here. When the channel is
         * closed any pending events are deleted, see ReflectClose(), and
         * ReflectEventDelete(). Trying to preserve and later release when the
         * event is run may generate a situation where the channel structure
         * is deleted but not our structure, crashing in
         * FreeReflectedChannel().
         *
         * Force creation of the RCM, for proper cleanup on thread teardown.
         * The teardown of unprocessed events is currently coupled to the
         * thread reflected channel map
         */

        (void) GetThreadReflectedChannelMap();

        /* XXX Race condition !!
         * XXX The destination thread may not exist anymore already.
         * XXX (Delayed postevent executed after channel got removed).
         * XXX Can we detect this ? (check the validity of the owner threadid ?)
         * XXX Actually, in that case the channel should be dead also !
         */

        Tcl_ThreadQueueEvent(rcPtr->owner, (Tcl_Event *) ev, TCL_QUEUE_TAIL);
        Tcl_ThreadAlert(rcPtr->owner);
    }
#endif

    /*
     * Squash interp results left by the event script.
     */

    Tcl_ResetResult(interp);
    return TCL_OK;

#undef CHAN
#undef EVENT
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

    (void) Tcl_SetReturnOptions(interp, Tcl_NewListObj(numOptions, lv));
    ((Interp *) interp)->flags &= ~ERR_ALREADY_LOGGED;
}

int
TclChanCaughtErrorBypass(
    Tcl_Interp *interp,
    Tcl_Channel chan)
{
    Tcl_Obj *chanMsgObj = NULL;
    Tcl_Obj *interpMsgObj = NULL;
    Tcl_Obj *msgObj = NULL;

    /*
     * Get a bypassed error message from channel and/or interpreter, save the
     * reference, then kill the returned objects, if there were any. If there
     * are messages in both the channel has preference.
     */

    if ((chan == NULL) && (interp == NULL)) {
	return 0;
    }

    if (chan != NULL) {
	Tcl_GetChannelError(chan, &chanMsgObj);
    }
    if (interp != NULL) {
	Tcl_GetChannelErrorInterp(interp, &interpMsgObj);
    }

    if (chanMsgObj != NULL) {
	msgObj = chanMsgObj;
    } else if (interpMsgObj != NULL) {
	msgObj = interpMsgObj;
    }
    if (msgObj != NULL) {
	Tcl_IncrRefCount(msgObj);
    }

    if (chanMsgObj != NULL) {
	Tcl_DecrRefCount(chanMsgObj);
    }
    if (interpMsgObj != NULL) {
	Tcl_DecrRefCount(interpMsgObj);
    }

    /*
     * No message returned, nothing caught.
     */

    if (msgObj == NULL) {
	return 0;
    }

    UnmarshallErrorResult(interp, msgObj);

    Tcl_DecrRefCount(msgObj);
    return 1;
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
    ReflectedChannel *rcPtr = clientData;
    int result;			/* Result code for 'close' */
    Tcl_Obj *resObj;		/* Result data for 'close' */
    ReflectedChannelMap *rcmPtr;/* Map of reflected channels with handlers in
				 * this interp */
    Tcl_HashEntry *hPtr;	/* Entry in the above map */
    const Tcl_ChannelType *tctPtr;

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
	 * Note: DeleteThreadReflectedChannelMap() is the thread exit handler
	 * for the origin thread. Use this to clean up the structure? Except
	 * if lost?
	 */

#ifdef TCL_THREADS
	if (rcPtr->thread != Tcl_GetCurrentThread()) {
	    ForwardParam p;

	    ForwardOpToHandlerThread(rcPtr, ForwardedClose, &p);
	    result = p.base.code;

            /*
             * Now squash the pending reflection events for this channel.
             */

            Tcl_DeleteEvents(ReflectEventDelete, rcPtr);

	    if (result != TCL_OK) {
		FreeReceivedError(&p);
	    }
	}
#endif

	tctPtr = ((Channel *)rcPtr->chan)->typePtr;
	if (tctPtr && tctPtr != &tclRChannelType) {
	    ckfree((char *)tctPtr);
	    ((Channel *)rcPtr->chan)->typePtr = NULL;
	}
        Tcl_EventuallyFree(rcPtr, (Tcl_FreeProc *) FreeReflectedChannel);
	return EOK;
    }

    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rcPtr->thread != Tcl_GetCurrentThread()) {
	ForwardParam p;

	ForwardOpToHandlerThread(rcPtr, ForwardedClose, &p);
	result = p.base.code;

        /*
         * Now squash the pending reflection events for this channel.
         */

        Tcl_DeleteEvents(ReflectEventDelete, rcPtr);

	if (result != TCL_OK) {
	    PassReceivedErrorInterp(interp, &p);
	}
    } else {
#endif
	result = InvokeTclMethod(rcPtr, METH_FINAL, NULL, NULL, &resObj);
	if ((result != TCL_OK) && (interp != NULL)) {
	    Tcl_SetChannelErrorInterp(interp, resObj);
	}

	Tcl_DecrRefCount(resObj);	/* Remove reference we held from the
					 * invoke */

	/*
	 * Remove the channel from the map before releasing the memory, to
	 * prevent future accesses (like by 'postevent') from finding and
	 * dereferencing a dangling pointer.
	 *
	 * NOTE: The channel may not be in the map. This is ok, that happens
	 * when the channel was created in a different interpreter and/or
	 * thread and then was moved here.
	 *
	 * NOTE: The channel may have been removed from the map already via
	 * the per-interp DeleteReflectedChannelMap exit-handler.
	 */

	if (!rcPtr->dead) {
	    rcmPtr = GetReflectedChannelMap(rcPtr->interp);
	    hPtr = Tcl_FindHashEntry(&rcmPtr->map,
		    Tcl_GetChannelName(rcPtr->chan));
	    if (hPtr) {
		Tcl_DeleteHashEntry(hPtr);
	    }
	}
#ifdef TCL_THREADS
	rcmPtr = GetThreadReflectedChannelMap();
	hPtr = Tcl_FindHashEntry(&rcmPtr->map,
		Tcl_GetChannelName(rcPtr->chan));
	if (hPtr) {
	    Tcl_DeleteHashEntry(hPtr);
	}
    }
#endif
    tctPtr = ((Channel *)rcPtr->chan)->typePtr;
    if (tctPtr && tctPtr != &tclRChannelType) {
	    ckfree((char *)tctPtr);
	    ((Channel *)rcPtr->chan)->typePtr = NULL;
    }
    Tcl_EventuallyFree(rcPtr, (Tcl_FreeProc *) FreeReflectedChannel);
    return (result == TCL_OK) ? EOK : EINVAL;
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
    ReflectedChannel *rcPtr = clientData;
    Tcl_Obj *toReadObj;
    int bytec;			/* Number of returned bytes */
    unsigned char *bytev;	/* Array of returned bytes */
    Tcl_Obj *resObj;		/* Result data for 'read' */

    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rcPtr->thread != Tcl_GetCurrentThread()) {
	ForwardParam p;

	p.input.buf = buf;
	p.input.toRead = toRead;

	ForwardOpToHandlerThread(rcPtr, ForwardedInput, &p);

	if (p.base.code != TCL_OK) {
	    if (p.base.code < 0) {
		/* No error message, this is an errno signal. */
		*errorCodePtr = -p.base.code;
	    } else {
		PassReceivedError(rcPtr->chan, &p);
		*errorCodePtr = EINVAL;
	    }
	    p.input.toRead = -1;
	} else {
	    *errorCodePtr = EOK;
	}

	return p.input.toRead;
    }
#endif

    /* ASSERT: rcPtr->method & FLAG(METH_READ) */
    /* ASSERT: rcPtr->mode & TCL_READABLE */

    Tcl_Preserve(rcPtr);

    toReadObj = Tcl_NewIntObj(toRead);
    Tcl_IncrRefCount(toReadObj);

    if (InvokeTclMethod(rcPtr, METH_READ, toReadObj, NULL, &resObj)!=TCL_OK) {
	int code = ErrnoReturn(rcPtr, resObj);

	if (code < 0) {
	    *errorCodePtr = -code;
            goto error;
	}

	Tcl_SetChannelError(rcPtr->chan, resObj);
        goto invalid;
    }

    bytev = Tcl_GetByteArrayFromObj(resObj, &bytec);

    if (toRead < bytec) {
	SetChannelErrorStr(rcPtr->chan, msg_read_toomuch);
        goto invalid;
    }

    *errorCodePtr = EOK;

    if (bytec > 0) {
	memcpy(buf, bytev, (size_t) bytec);
    }

 stop:
    Tcl_DecrRefCount(toReadObj);
    Tcl_DecrRefCount(resObj);		/* Remove reference held from invoke */
    Tcl_Release(rcPtr);
    return bytec;
 invalid:
    *errorCodePtr = EINVAL;
 error:
    bytec = -1;
    goto stop;
}

/*
 *----------------------------------------------------------------------
 *
 * ReflectOutput --
 *
 *	This function is invoked when data is writen to the channel.
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
    ReflectedChannel *rcPtr = clientData;
    Tcl_Obj *bufObj;
    Tcl_Obj *resObj;		/* Result data for 'write' */
    int written;

    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rcPtr->thread != Tcl_GetCurrentThread()) {
	ForwardParam p;

	p.output.buf = buf;
	p.output.toWrite = toWrite;

	ForwardOpToHandlerThread(rcPtr, ForwardedOutput, &p);

	if (p.base.code != TCL_OK) {
	    if (p.base.code < 0) {
		/* No error message, this is an errno signal. */
		*errorCodePtr = -p.base.code;
	    } else {
                PassReceivedError(rcPtr->chan, &p);
                *errorCodePtr = EINVAL;
            }
	    p.output.toWrite = -1;
	} else {
	    *errorCodePtr = EOK;
	}

	return p.output.toWrite;
    }
#endif

    /* ASSERT: rcPtr->method & FLAG(METH_WRITE) */
    /* ASSERT: rcPtr->mode & TCL_WRITABLE */

    Tcl_Preserve(rcPtr);
    Tcl_Preserve(rcPtr->interp);

    bufObj = Tcl_NewByteArrayObj((unsigned char *) buf, toWrite);
    Tcl_IncrRefCount(bufObj);

    if (InvokeTclMethod(rcPtr, METH_WRITE, bufObj, NULL, &resObj) != TCL_OK) {
	int code = ErrnoReturn(rcPtr, resObj);

	if (code < 0) {
	    *errorCodePtr = -code;
            goto error;
	}

	Tcl_SetChannelError(rcPtr->chan, resObj);
        goto invalid;
    }

    if (Tcl_InterpDeleted(rcPtr->interp)) {
	/*
	 * The interp was destroyed during InvokeTclMethod().
	 */

	SetChannelErrorStr(rcPtr->chan, msg_send_dstlost);
        goto invalid;
    }
    if (Tcl_GetIntFromObj(rcPtr->interp, resObj, &written) != TCL_OK) {
	Tcl_SetChannelError(rcPtr->chan, MarshallError(rcPtr->interp));
        goto invalid;
    }

    if ((written == 0) && (toWrite > 0)) {
	/*
	 * The handler claims to have written nothing of what it was
	 * given. That is bad.
	 */

	SetChannelErrorStr(rcPtr->chan, msg_write_nothing);
        goto invalid;
    }
    if (toWrite < written) {
	/*
	 * The handler claims to have written more than it was given. That is
	 * bad. Note that the I/O core would crash if we were to return this
	 * information, trying to write -nnn bytes in the next iteration.
	 */

	SetChannelErrorStr(rcPtr->chan, msg_write_toomuch);
        goto invalid;
    }

    *errorCodePtr = EOK;
 stop:
    Tcl_DecrRefCount(bufObj);
    Tcl_DecrRefCount(resObj);		/* Remove reference held from invoke */
    Tcl_Release(rcPtr->interp);
    Tcl_Release(rcPtr);
    return written;
 invalid:
    *errorCodePtr = EINVAL;
 error:
    written = -1;
    goto stop;
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
 *	Allocates memory. Arbitrary, as it calls upon a script.
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
    ReflectedChannel *rcPtr = clientData;
    Tcl_Obj *offObj, *baseObj;
    Tcl_Obj *resObj;		/* Result for 'seek' */
    Tcl_WideInt newLoc;

    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rcPtr->thread != Tcl_GetCurrentThread()) {
	ForwardParam p;

	p.seek.seekMode = seekMode;
	p.seek.offset = offset;

	ForwardOpToHandlerThread(rcPtr, ForwardedSeek, &p);

	if (p.base.code != TCL_OK) {
	    PassReceivedError(rcPtr->chan, &p);
	    *errorCodePtr = EINVAL;
	    p.seek.offset = -1;
	} else {
	    *errorCodePtr = EOK;
	}

	return p.seek.offset;
    }
#endif

    /* ASSERT: rcPtr->method & FLAG(METH_SEEK) */

    Tcl_Preserve(rcPtr);

    offObj  = Tcl_NewWideIntObj(offset);
    baseObj = Tcl_NewStringObj(
            (seekMode == SEEK_SET) ? "start" :
            (seekMode == SEEK_CUR) ? "current" : "end", -1);
    Tcl_IncrRefCount(offObj);
    Tcl_IncrRefCount(baseObj);

    if (InvokeTclMethod(rcPtr, METH_SEEK, offObj, baseObj, &resObj)!=TCL_OK) {
	Tcl_SetChannelError(rcPtr->chan, resObj);
        goto invalid;
    }

    if (Tcl_GetWideIntFromObj(rcPtr->interp, resObj, &newLoc) != TCL_OK) {
	Tcl_SetChannelError(rcPtr->chan, MarshallError(rcPtr->interp));
        goto invalid;
    }

    if (newLoc < Tcl_LongAsWide(0)) {
	SetChannelErrorStr(rcPtr->chan, msg_seek_beforestart);
        goto invalid;
    }

    *errorCodePtr = EOK;
 stop:
    Tcl_DecrRefCount(offObj);
    Tcl_DecrRefCount(baseObj);
    Tcl_DecrRefCount(resObj);		/* Remove reference held from invoke */
    Tcl_Release(rcPtr);
    return newLoc;
 invalid:
    *errorCodePtr = EINVAL;
    newLoc = -1;
    goto stop;
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
    ReflectedChannel *rcPtr = clientData;
    Tcl_Obj *maskObj;

    /*
     * We restrict the interest to what the channel can support. IOW there
     * will never be write events for a channel which is not writable.
     * Analoguously for read events and non-readable channels.
     */

    mask &= rcPtr->mode;

    if (mask == rcPtr->interest) {
	/*
	 * Same old, same old, why should we do something?
	 */

	return;
    }

    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rcPtr->thread != Tcl_GetCurrentThread()) {
	ForwardParam p;

	p.watch.mask = mask;
	ForwardOpToHandlerThread(rcPtr, ForwardedWatch, &p);

	/*
	 * Any failure from the forward is ignored. We have no place to put
	 * this.
	 */

	return;
    }
#endif

    Tcl_Preserve(rcPtr);

    rcPtr->interest = mask;
    maskObj = DecodeEventMask(mask);
    /* assert maskObj.refCount == 1 */
    (void) InvokeTclMethod(rcPtr, METH_WATCH, maskObj, NULL, NULL);
    Tcl_DecrRefCount(maskObj);

    Tcl_Release(rcPtr);
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
    ReflectedChannel *rcPtr = clientData;
    Tcl_Obj *blockObj;
    int errorNum;		/* EINVAL or EOK (success). */
    Tcl_Obj *resObj;		/* Result data for 'blocking' */

    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rcPtr->thread != Tcl_GetCurrentThread()) {
	ForwardParam p;

	p.block.nonblocking = nonblocking;

	ForwardOpToHandlerThread(rcPtr, ForwardedBlock, &p);

	if (p.base.code != TCL_OK) {
	    PassReceivedError(rcPtr->chan, &p);
	    return EINVAL;
	}

	return EOK;
    }
#endif

    blockObj = Tcl_NewBooleanObj(!nonblocking);
    Tcl_IncrRefCount(blockObj);

    Tcl_Preserve(rcPtr);

    if (InvokeTclMethod(rcPtr,METH_BLOCKING,blockObj,NULL,&resObj)!=TCL_OK) {
	Tcl_SetChannelError(rcPtr->chan, resObj);
	errorNum = EINVAL;
    } else {
	errorNum = EOK;
    }

    Tcl_DecrRefCount(blockObj);
    Tcl_DecrRefCount(resObj);		/* Remove reference held from invoke */

    Tcl_Release(rcPtr);
    return errorNum;
}

#ifdef TCL_THREADS
/*
 *----------------------------------------------------------------------
 *
 * ReflectThread --
 *
 *	This function is invoked to tell the channel about thread movements.
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
ReflectThread(
    ClientData clientData,
    int action)
{
    ReflectedChannel *rcPtr = clientData;

    switch (action) {
    case TCL_CHANNEL_THREAD_INSERT:
        rcPtr->owner = Tcl_GetCurrentThread();
        break;
    case TCL_CHANNEL_THREAD_REMOVE:
        rcPtr->owner = NULL;
        break;
    default:
        Tcl_Panic("Unknown thread action code.");
        break;
    }
}

#endif
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
 *	Arbitrary, as it calls upon a Tcl script.
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
    ReflectedChannel *rcPtr = clientData;
    Tcl_Obj *optionObj, *valueObj;
    int result;			/* Result code for 'configure' */
    Tcl_Obj *resObj;		/* Result data for 'configure' */

    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rcPtr->thread != Tcl_GetCurrentThread()) {
	ForwardParam p;

	p.setOpt.name = optionName;
	p.setOpt.value = newValue;

	ForwardOpToHandlerThread(rcPtr, ForwardedSetOpt, &p);

	if (p.base.code != TCL_OK) {
	    Tcl_Obj *err = Tcl_NewStringObj(p.base.msgStr, -1);

	    UnmarshallErrorResult(interp, err);
	    Tcl_DecrRefCount(err);
	    FreeReceivedError(&p);
	}

	return p.base.code;
    }
#endif
    Tcl_Preserve(rcPtr);

    optionObj = Tcl_NewStringObj(optionName, -1);
    valueObj = Tcl_NewStringObj(newValue, -1);

    Tcl_IncrRefCount(optionObj);
    Tcl_IncrRefCount(valueObj);

    result = InvokeTclMethod(rcPtr, METH_CONFIGURE,optionObj,valueObj, &resObj);
    if (result != TCL_OK) {
	UnmarshallErrorResult(interp, resObj);
    }

    Tcl_DecrRefCount(optionObj);
    Tcl_DecrRefCount(valueObj);
    Tcl_DecrRefCount(resObj);		/* Remove reference held from invoke */
    Tcl_Release(rcPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ReflectGetOption --
 *
 *	This function is invoked to retrieve all or a channel option.
 *
 * Results:
 *	A standard Tcl result code.
 *
 * Side effects:
 *	Arbitrary, as it calls upon a Tcl script.
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
    /*
     * This code is special. It has regular passing of Tcl result, and errors.
     * The bypass functions are not required.
     */

    ReflectedChannel *rcPtr = clientData;
    Tcl_Obj *optionObj;
    Tcl_Obj *resObj;		/* Result data for 'configure' */
    int listc, result = TCL_OK;
    Tcl_Obj **listv;
    MethodName method;

    /*
     * Are we in the correct thread?
     */

#ifdef TCL_THREADS
    if (rcPtr->thread != Tcl_GetCurrentThread()) {
	int opcode;
	ForwardParam p;

	p.getOpt.name = optionName;
	p.getOpt.value = dsPtr;

	if (optionName == NULL) {
	    opcode = ForwardedGetOptAll;
	} else {
	    opcode = ForwardedGetOpt;
	}

	ForwardOpToHandlerThread(rcPtr, opcode, &p);

	if (p.base.code != TCL_OK) {
	    Tcl_Obj *err = Tcl_NewStringObj(p.base.msgStr, -1);

	    UnmarshallErrorResult(interp, err);
	    Tcl_DecrRefCount(err);
	    FreeReceivedError(&p);
	}

	return p.base.code;
    }
#endif

    if (optionName == NULL) {
	/*
	 * Retrieve all options.
	 */

	method = METH_CGETALL;
	optionObj = NULL;
    } else {
	/*
	 * Retrieve the value of one option.
	 */

	method = METH_CGET;
	optionObj = Tcl_NewStringObj(optionName, -1);
        Tcl_IncrRefCount(optionObj);
    }

    Tcl_Preserve(rcPtr);

    if (InvokeTclMethod(rcPtr, method, optionObj, NULL, &resObj)!=TCL_OK) {
	UnmarshallErrorResult(interp, resObj);
        goto error;
    }

    /*
     * The result has to go into the 'dsPtr' for propagation to the caller of
     * the driver.
     */

    if (optionObj != NULL) {
	TclDStringAppendObj(dsPtr, resObj);
        goto ok;
    }

    /*
     * Extract the list and append each item as element.
     */

    /*
     * NOTE (4): If we extract the string rep we can assume a properly quoted
     * string. Together with a separating space this way of simply appending
     * the whole string rep might be faster. It also doesn't check if the
     * result is a valid list. Nor that the list has an even number elements.
     */

    if (Tcl_ListObjGetElements(interp, resObj, &listc, &listv) != TCL_OK) {
        goto error;
    }

    if ((listc % 2) == 1) {
	/*
	 * Odd number of elements is wrong.
	 */

	Tcl_ResetResult(interp);
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"Expected list with even number of "
		"elements, got %d element%s instead", listc,
		(listc == 1 ? "" : "s")));
        goto error;
    } else {
	int len;
	const char *str = Tcl_GetStringFromObj(resObj, &len);

	if (len) {
	    TclDStringAppendLiteral(dsPtr, " ");
	    Tcl_DStringAppend(dsPtr, str, len);
	}
        goto ok;
    }

 ok:
    result = TCL_OK;
 stop:
    if (optionObj) {
        Tcl_DecrRefCount(optionObj);
    }
    Tcl_DecrRefCount(resObj);	/* Remove reference held from invoke */
    Tcl_Release(rcPtr);
    return result;
 error:
    result = TCL_ERROR;
    goto stop;
}

/*
 * Helpers. =========================================================
 */

/*
 *----------------------------------------------------------------------
 *
 * EncodeEventMask --
 *
 *	This function takes a list of event items and constructs the
 *	equivalent internal bitmask. The list must contain at least one
 *	element. Elements are "read", "write", or any unique abbreviation of
 *	them. Note that the bitmask is not changed if problems are
 *	encountered.
 *
 * Results:
 *	A standard Tcl error code. A bitmask where TCL_READABLE and/or
 *	TCL_WRITABLE can be set.
 *
 * Side effects:
 *	May shimmer 'obj' to a list representation. May place an error message
 *	into the interp result.
 *
 *----------------------------------------------------------------------
 */

static int
EncodeEventMask(
    Tcl_Interp *interp,
    const char *objName,
    Tcl_Obj *obj,
    int *mask)
{
    int events;			/* Mask of events to post */
    int listc;			/* #elements in eventspec list */
    Tcl_Obj **listv;		/* Elements of eventspec list */
    int evIndex;		/* Id of event for an element of the eventspec
				 * list. */

    if (Tcl_ListObjGetElements(interp, obj, &listc, &listv) != TCL_OK) {
	return TCL_ERROR;
    }

    if (listc < 1) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "bad %s list: is empty", objName));
	return TCL_ERROR;
    }

    events = 0;
    while (listc > 0) {
	if (Tcl_GetIndexFromObj(interp, listv[listc-1], eventOptions,
		objName, 0, &evIndex) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch (evIndex) {
	case EVENT_READ:
	    events |= TCL_READABLE;
	    break;
	case EVENT_WRITE:
	    events |= TCL_WRITABLE;
	    break;
	}
	listc --;
    }

    *mask = events;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DecodeEventMask --
 *
 *	This function takes an internal bitmask of events and constructs the
 *	equivalent list of event items.
 *
 * Results, Contract:
 *	A Tcl_Obj reference. The object will have a refCount of one. The user
 *	has to decrement it to release the object.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
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
    /* assert evObj.refCount == 1 */
    return evObj;
}

/*
 *----------------------------------------------------------------------
 *
 * NewReflectedChannel --
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

static ReflectedChannel *
NewReflectedChannel(
    Tcl_Interp *interp,
    Tcl_Obj *cmdpfxObj,
    int mode,
    Tcl_Obj *handleObj)
{
    ReflectedChannel *rcPtr;
    MethodName mn = METH_BLOCKING;

    rcPtr = ckalloc(sizeof(ReflectedChannel));

    /* rcPtr->chan: Assigned by caller. Dummy data here. */

    rcPtr->chan = NULL;
    rcPtr->interp = interp;
    rcPtr->dead = 0;
#ifdef TCL_THREADS
    rcPtr->thread = Tcl_GetCurrentThread();
#endif
    rcPtr->mode = mode;
    rcPtr->interest = 0;		/* Initially no interest registered */

    /* ASSERT: cmdpfxObj is a Tcl List */
    rcPtr->cmd = TclListObjCopy(NULL, cmdpfxObj);
    Tcl_IncrRefCount(rcPtr->cmd);
    rcPtr->methods = Tcl_NewListObj(METH_WRITE + 1, NULL);
    while (mn <= METH_WRITE) {
	Tcl_ListObjAppendElement(NULL, rcPtr->methods,
		Tcl_NewStringObj(methodNames[mn++], -1));
    }
    Tcl_IncrRefCount(rcPtr->methods);
    rcPtr->name = handleObj;
    Tcl_IncrRefCount(rcPtr->name);
    return rcPtr;
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

    TCL_DECLARE_MUTEX(rcCounterMutex)
    static unsigned long rcCounter = 0;
    Tcl_Obj *resObj;

    Tcl_MutexLock(&rcCounterMutex);
    resObj = Tcl_ObjPrintf("rc%lu", rcCounter);
    rcCounter++;
    Tcl_MutexUnlock(&rcCounterMutex);

    return resObj;
}

static void
FreeReflectedChannel(
    ReflectedChannel *rcPtr)
{
    Channel *chanPtr = (Channel *) rcPtr->chan;

    TclChannelRelease((Tcl_Channel)chanPtr);
    if (rcPtr->name) {
	Tcl_DecrRefCount(rcPtr->name);
    }
    if (rcPtr->methods) {
	Tcl_DecrRefCount(rcPtr->methods);
    }
    if (rcPtr->cmd) {
	Tcl_DecrRefCount(rcPtr->cmd);
    }
    ckfree(rcPtr);
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
 */

static int
InvokeTclMethod(
    ReflectedChannel *rcPtr,
    MethodName method,
    Tcl_Obj *argOneObj,		/* NULL'able */
    Tcl_Obj *argTwoObj,		/* NULL'able */
    Tcl_Obj **resultObjPtr)	/* NULL'able */
{
    Tcl_Obj *methObj = NULL;	/* Method name in object form */
    Tcl_InterpState sr;		/* State of handler interp */
    int result;			/* Result code of method invokation */
    Tcl_Obj *resObj = NULL;	/* Result of method invokation. */
    Tcl_Obj *cmd;

    if (rcPtr->dead) {
	/*
	 * The channel is marked as dead. Bail out immediately, with an
	 * appropriate error.
	 */

	if (resultObjPtr != NULL) {
	    resObj = Tcl_NewStringObj(msg_dstlost,-1);
	    *resultObjPtr = resObj;
	    Tcl_IncrRefCount(resObj);
	}

        /*
         * Not touching argOneObj, argTwoObj, they have not been used.
         * See the contract as well.
         */

	return TCL_ERROR;
    }

    /*
     * Insert method into the callback command, after the command prefix,
     * before the channel id.
     */

    cmd = TclListObjCopy(NULL, rcPtr->cmd);

    Tcl_ListObjIndex(NULL, rcPtr->methods, method, &methObj);
    Tcl_ListObjAppendElement(NULL, cmd, methObj);
    Tcl_ListObjAppendElement(NULL, cmd, rcPtr->name);

    /*
     * Append the additional argument containing method specific details
     * behind the channel id. If specified.
     *
     * Because of the contract there is no need to increment the refcounts.
     * The objects will survive the Tcl_EvalObjv without change.
     */

    if (argOneObj) {
	Tcl_ListObjAppendElement(NULL, cmd, argOneObj);
	if (argTwoObj) {
	    Tcl_ListObjAppendElement(NULL, cmd, argTwoObj);
	}
    }

    /*
     * And run the handler... This is done in auch a manner which leaves any
     * existing state intact.
     */

    Tcl_IncrRefCount(cmd);
    sr = Tcl_SaveInterpState(rcPtr->interp, 0 /* Dummy */);
    Tcl_Preserve(rcPtr->interp);
    result = Tcl_EvalObjEx(rcPtr->interp, cmd, TCL_EVAL_GLOBAL);

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

	    resObj = Tcl_GetObjResult(rcPtr->interp);
	} else {
	    /*
	     * Non-ok result is always treated as an error. We have to capture
	     * the full state of the result, including additional options.
	     *
	     * This is complex and ugly, and would be completely unnecessary
	     * if we only added support for a TCL_FORBID_EXCEPTIONS flag.
	     */

	    if (result != TCL_ERROR) {
		int cmdLen;
		const char *cmdString = Tcl_GetStringFromObj(cmd, &cmdLen);

		Tcl_IncrRefCount(cmd);
		Tcl_ResetResult(rcPtr->interp);
		Tcl_SetObjResult(rcPtr->interp, Tcl_ObjPrintf(
			"chan handler returned bad code: %d", result));
		Tcl_LogCommandInfo(rcPtr->interp, cmdString, cmdString,
			cmdLen);
		Tcl_DecrRefCount(cmd);
		result = TCL_ERROR;
	    }
	    Tcl_AppendObjToErrorInfo(rcPtr->interp, Tcl_ObjPrintf(
		    "\n    (chan handler subcommand \"%s\")",
		    methodNames[method]));
	    resObj = MarshallError(rcPtr->interp);
	}
	Tcl_IncrRefCount(resObj);
    }
    Tcl_DecrRefCount(cmd);
    Tcl_RestoreInterpState(rcPtr->interp, sr);
    Tcl_Release(rcPtr->interp);

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
 * ErrnoReturn --
 *
 *	Checks a method error result if it returned an 'errno'.
 *
 * Results:
 *	The negative errno found in the error result, or 0.
 *
 * Side effects:
 *	None.
 *
 * Users:
 *	ReflectInput/Output(), to enable the signaling of EAGAIN
 *	on 0-sized short reads/writes.
 *
 *----------------------------------------------------------------------
 */

static int
ErrnoReturn(
    ReflectedChannel *rcPtr,
    Tcl_Obj *resObj)
{
    int code;
    Tcl_InterpState sr;		/* State of handler interp */

    if (rcPtr->dead) {
	return 0;
    }

    sr = Tcl_SaveInterpState(rcPtr->interp, 0 /* Dummy */);
    UnmarshallErrorResult(rcPtr->interp, resObj);

    resObj = Tcl_GetObjResult(rcPtr->interp);

    if (((Tcl_GetIntFromObj(rcPtr->interp, resObj, &code) != TCL_OK)
	    || (code >= 0))) {
	if (strcmp("EAGAIN", Tcl_GetString(resObj)) == 0) {
	    code = -EAGAIN;
	} else {
	    code = 0;
	}
    }

    Tcl_RestoreInterpState(rcPtr->interp, sr);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * GetReflectedChannelMap --
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

static ReflectedChannelMap *
GetReflectedChannelMap(
    Tcl_Interp *interp)
{
    ReflectedChannelMap *rcmPtr = Tcl_GetAssocData(interp, RCMKEY, NULL);

    if (rcmPtr == NULL) {
	rcmPtr = ckalloc(sizeof(ReflectedChannelMap));
	Tcl_InitHashTable(&rcmPtr->map, TCL_STRING_KEYS);
	Tcl_SetAssocData(interp, RCMKEY,
		(Tcl_InterpDeleteProc *) DeleteReflectedChannelMap, rcmPtr);
    }
    return rcmPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteReflectedChannelMap --
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
MarkDead(
    ReflectedChannel *rcPtr)
{
    if (rcPtr->dead) {
	return;
    }
    if (rcPtr->name) {
	Tcl_DecrRefCount(rcPtr->name);
	rcPtr->name = NULL;
    }
    if (rcPtr->methods) {
	Tcl_DecrRefCount(rcPtr->methods);
	rcPtr->methods = NULL;
    }
    if (rcPtr->cmd) {
	Tcl_DecrRefCount(rcPtr->cmd);
	rcPtr->cmd = NULL;
    }
    rcPtr->dead = 1;
}

static void
DeleteReflectedChannelMap(
    ClientData clientData,	/* The per-interpreter data structure. */
    Tcl_Interp *interp)		/* The interpreter being deleted. */
{
    ReflectedChannelMap *rcmPtr = clientData;
				/* The map */
    Tcl_HashSearch hSearch;	 /* Search variable. */
    Tcl_HashEntry *hPtr;	 /* Search variable. */
    ReflectedChannel *rcPtr;
    Tcl_Channel chan;
#ifdef TCL_THREADS
    ForwardingResult *resultPtr;
    ForwardingEvent *evPtr;
    ForwardParam *paramPtr;
#endif

    /*
     * Delete all entries. The channels may have been closed already, or will
     * be closed later, by the standard IO finalization of an interpreter
     * under destruction. Except for the channels which were moved to a
     * different interpreter and/or thread. They do not exist from the IO
     * systems point of view and will not get closed. Therefore mark all as
     * dead so that any future access will cause a proper error. For channels
     * in a different thread we actually do the same as
     * DeleteThreadReflectedChannelMap(), just restricted to the channels of
     * this interp.
     */

    for (hPtr = Tcl_FirstHashEntry(&rcmPtr->map, &hSearch);
	    hPtr != NULL;
	    hPtr = Tcl_FirstHashEntry(&rcmPtr->map, &hSearch)) {
	chan = Tcl_GetHashValue(hPtr);
	rcPtr = Tcl_GetChannelInstanceData(chan);

	MarkDead(rcPtr);
	Tcl_DeleteHashEntry(hPtr);
    }
    Tcl_DeleteHashTable(&rcmPtr->map);
    ckfree(&rcmPtr->map);

#ifdef TCL_THREADS
    /*
     * The origin interpreter for one or more reflected channels is gone.
     */

    /*
     * Go through the list of pending results and cancel all whose events were
     * destined for this interpreter. While this is in progress we block any
     * other access to the list of pending results.
     */

    Tcl_MutexLock(&rcForwardMutex);

    for (resultPtr = forwardList;
	    resultPtr != NULL;
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
         *
         * Attention: Results may have been detached already, by either the
         * receiver, or this thread, as part of other parts in the thread
         * teardown. Such results are ignored. See ticket [b47b176adf] for the
         * identical race condition in Tcl 8.6 IORTrans.
	 */

	evPtr = resultPtr->evPtr;

	/* Basic crash safety until this routine can get revised [3411310] */
	if (evPtr == NULL) {
	    continue;
	}
	paramPtr = evPtr->param;
	if (!evPtr) {
	    continue;
	}

	evPtr->resultPtr = NULL;
	resultPtr->evPtr = NULL;
	resultPtr->result = TCL_ERROR;

	ForwardSetStaticError(paramPtr, msg_send_dstlost);

	Tcl_ConditionNotify(&resultPtr->done);
    }
    Tcl_MutexUnlock(&rcForwardMutex);

    /*
     * Get the map of all channels handled by the current thread. This is a
     * ReflectedChannelMap, but on a per-thread basis, not per-interp. Go
     * through the channels and remove all which were handled by this
     * interpreter. They have already been marked as dead.
     */

    rcmPtr = GetThreadReflectedChannelMap();
    for (hPtr = Tcl_FirstHashEntry(&rcmPtr->map, &hSearch);
	    hPtr != NULL;
	    hPtr = Tcl_NextHashEntry(&hSearch)) {
	chan = Tcl_GetHashValue(hPtr);
	rcPtr = Tcl_GetChannelInstanceData(chan);

	if (rcPtr->interp != interp) {
	    /*
	     * Ignore entries for other interpreters.
	     */

	    continue;
	}

	MarkDead(rcPtr);
	Tcl_DeleteHashEntry(hPtr);
    }
#endif
}

#ifdef TCL_THREADS
/*
 *----------------------------------------------------------------------
 *
 * GetThreadReflectedChannelMap --
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

static ReflectedChannelMap *
GetThreadReflectedChannelMap(void)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!tsdPtr->rcmPtr) {
	tsdPtr->rcmPtr = ckalloc(sizeof(ReflectedChannelMap));
	Tcl_InitHashTable(&tsdPtr->rcmPtr->map, TCL_STRING_KEYS);
	Tcl_CreateThreadExitHandler(DeleteThreadReflectedChannelMap, NULL);
    }

    return tsdPtr->rcmPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteThreadReflectedChannelMap --
 *
 *	Deletes the channel table for a thread. This procedure is invoked when
 *	a thread is deleted. The channels have already been marked as dead, in
 *	DeleteReflectedChannelMap().
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
DeleteThreadReflectedChannelMap(
    ClientData clientData)	/* The per-thread data structure. */
{
    Tcl_HashSearch hSearch;	 /* Search variable. */
    Tcl_HashEntry *hPtr;	 /* Search variable. */
    Tcl_ThreadId self = Tcl_GetCurrentThread();
    ReflectedChannelMap *rcmPtr; /* The map */
    ForwardingResult *resultPtr;

    /*
     * The origin thread for one or more reflected channels is gone.
     * NOTE: If this function is called due to a thread getting killed the
     *       per-interp DeleteReflectedChannelMap is apparently not called.
     */

    /*
     * Go through the list of pending results and cancel all whose events were
     * destined for this thread. While this is in progress we block any
     * other access to the list of pending results.
     */

    Tcl_MutexLock(&rcForwardMutex);

    for (resultPtr = forwardList;
	    resultPtr != NULL;
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
         *
         * Attention: Results may have been detached already, by either the
         * receiver, or this thread, as part of other parts in the thread
         * teardown. Such results are ignored. See ticket [b47b176adf] for the
         * identical race condition in Tcl 8.6 IORTrans.
	 */

	evPtr = resultPtr->evPtr;

	/* Basic crash safety until this routine can get revised [3411310] */
	if (evPtr == NULL ) {
	    continue;
	}
	paramPtr = evPtr->param;
	if (!evPtr) {
	    continue;
	}

	evPtr->resultPtr = NULL;
	resultPtr->evPtr = NULL;
	resultPtr->result = TCL_ERROR;

	ForwardSetStaticError(paramPtr, msg_send_dstlost);

	Tcl_ConditionNotify(&resultPtr->done);
    }
    Tcl_MutexUnlock(&rcForwardMutex);

    /*
     * Run over the event queue of this thread and remove all ReflectEvent's
     * still pending. These are inbound events for reflected channels this
     * thread owns but doesn't handle. The inverse of the channel map
     * actually.
     */

    Tcl_DeleteEvents(ReflectEventDelete, NULL);

    /*
     * Get the map of all channels handled by the current thread. This is a
     * ReflectedChannelMap, but on a per-thread basis, not per-interp. Go
     * through the channels, remove all, mark them as dead.
     */

    rcmPtr = GetThreadReflectedChannelMap();
    for (hPtr = Tcl_FirstHashEntry(&rcmPtr->map, &hSearch);
	    hPtr != NULL;
	    hPtr = Tcl_FirstHashEntry(&rcmPtr->map, &hSearch)) {
	Tcl_Channel chan = Tcl_GetHashValue(hPtr);
	ReflectedChannel *rcPtr = Tcl_GetChannelInstanceData(chan);

	MarkDead(rcPtr);
	Tcl_DeleteHashEntry(hPtr);
    }
    ckfree(rcmPtr);
}

static void
ForwardOpToHandlerThread(
    ReflectedChannel *rcPtr,	/* Channel instance */
    ForwardedOperation op,	/* Forwarded driver operation */
    const void *param)		/* Arguments */
{
    /*
     * Core of the communication from OWNER to HANDLER thread.
     * The receiver is ForwardProc() below.
     */

    Tcl_ThreadId dst = rcPtr->thread;
    ForwardingEvent *evPtr;
    ForwardingResult *resultPtr;

    /*
     * We gather the lock early. This allows us to check the liveness of the
     * channel without interference from DeleteThreadReflectedChannelMap().
     */

    Tcl_MutexLock(&rcForwardMutex);

    if (rcPtr->dead) {
	/*
	 * The channel is marked as dead. Bail out immediately, with an
	 * appropriate error. Do not forget to unlock the mutex on this path.
	 */

	ForwardSetStaticError((ForwardParam *) param, msg_send_dstlost);
	Tcl_MutexUnlock(&rcForwardMutex);
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
    evPtr->rcPtr = rcPtr;
    evPtr->param = (ForwardParam *) param;

    resultPtr->src = Tcl_GetCurrentThread();
    resultPtr->dst = dst;
    resultPtr->dsti = rcPtr->interp;
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
     * DeleteThreadReflectedChannelMap(), this is set up by
     * GetThreadReflectedChannelMap(). This is what we use the 'forwardList'
     * (see above) for.
     */

    Tcl_CreateThreadExitHandler(SrcExitProc, evPtr);

    /*
     * Queue the event and poke the other thread's notifier.
     */

    Tcl_ThreadQueueEvent(dst, (Tcl_Event *) evPtr, TCL_QUEUE_TAIL);
    Tcl_ThreadAlert(dst);

    /*
     * (*) Block until the handler thread has either processed the transfer or
     * rejected it.
     */

    while (resultPtr->result < 0) {
	/*
	 * NOTE (1): Is it possible that the current thread goes away while
	 * waiting here? IOW Is it possible that "SrcExitProc" is called while
	 * we are here? See complementary note (2) in "SrcExitProc"
	 *
	 * The ConditionWait unlocks the mutex during the wait and relocks it
	 * immediately after.
	 */

	Tcl_ConditionWait(&resultPtr->done, &rcForwardMutex, NULL);
    }

    /*
     * Unlink result from the forwarder list. No need to lock. Either still
     * locked, or locked by the ConditionWait
     */

    TclSpliceOut(resultPtr, forwardList);

    resultPtr->nextPtr = NULL;
    resultPtr->prevPtr = NULL;

    Tcl_MutexUnlock(&rcForwardMutex);
    Tcl_ConditionFinalize(&resultPtr->done);

    /*
     * Kill the cleanup handler now, and the result structure as well, before
     * returning the success code.
     *
     * Note: The event structure has already been deleted.
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
     * HANDLER thread.

     * The receiver part for the operations coming from the OWNER thread.
     * See ForwardOpToHandlerThread() for the transmitter.
     *
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
    ReflectedChannel *rcPtr = evPtr->rcPtr;
    Tcl_Interp *interp = rcPtr->interp;
    ForwardParam *paramPtr = evPtr->param;
    Tcl_Obj *resObj = NULL;	/* Interp result of InvokeTclMethod */
    ReflectedChannelMap *rcmPtr;/* Map of reflected channels with handlers in
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
	 * rcPtr->thread, which contains rcPtr->interp, the interp we have to
	 * call upon for the driver.
	 */

    case ForwardedClose: {
	/*
	 * No parameters/results.
	 */

	if (InvokeTclMethod(rcPtr, METH_FINAL, NULL, NULL, &resObj)!=TCL_OK) {
	    ForwardSetObjError(paramPtr, resObj);
	}

	/*
	 * Freeing is done here, in the origin thread, callback command
	 * objects belong to this thread. Deallocating them in a different
	 * thread is not allowed
	 *
	 * We remove the channel from both interpreter and thread maps before
	 * releasing the memory, to prevent future accesses (like by
	 * 'postevent') from finding and dereferencing a dangling pointer.
	 */

	rcmPtr = GetReflectedChannelMap(interp);
	hPtr = Tcl_FindHashEntry(&rcmPtr->map,
                Tcl_GetChannelName(rcPtr->chan));
	Tcl_DeleteHashEntry(hPtr);

	rcmPtr = GetThreadReflectedChannelMap();
	hPtr = Tcl_FindHashEntry(&rcmPtr->map,
                Tcl_GetChannelName(rcPtr->chan));
	Tcl_DeleteHashEntry(hPtr);
	MarkDead(rcPtr);
	break;
    }

    case ForwardedInput: {
	Tcl_Obj *toReadObj = Tcl_NewIntObj(paramPtr->input.toRead);
        Tcl_IncrRefCount(toReadObj);

        Tcl_Preserve(rcPtr);
	if (InvokeTclMethod(rcPtr, METH_READ, toReadObj, NULL, &resObj)!=TCL_OK){
	    int code = ErrnoReturn(rcPtr, resObj);

	    if (code < 0) {
		paramPtr->base.code = code;
	    } else {
		ForwardSetObjError(paramPtr, resObj);
	    }
	    paramPtr->input.toRead = -1;
	} else {
	    /*
	     * Process a regular result.
	     */

	    int bytec;			/* Number of returned bytes */
	    unsigned char *bytev;	/* Array of returned bytes */

	    bytev = Tcl_GetByteArrayFromObj(resObj, &bytec);

	    if (paramPtr->input.toRead < bytec) {
		ForwardSetStaticError(paramPtr, msg_read_toomuch);
		paramPtr->input.toRead = -1;
	    } else {
		if (bytec > 0) {
		    memcpy(paramPtr->input.buf, bytev, (size_t) bytec);
		}
		paramPtr->input.toRead = bytec;
	    }
	}
        Tcl_Release(rcPtr);
        Tcl_DecrRefCount(toReadObj);
	break;
    }

    case ForwardedOutput: {
	Tcl_Obj *bufObj = Tcl_NewByteArrayObj((unsigned char *)
                paramPtr->output.buf, paramPtr->output.toWrite);
        Tcl_IncrRefCount(bufObj);

        Tcl_Preserve(rcPtr);
	if (InvokeTclMethod(rcPtr, METH_WRITE, bufObj, NULL, &resObj) != TCL_OK) {
	    int code = ErrnoReturn(rcPtr, resObj);

	    if (code < 0) {
		paramPtr->base.code = code;
	    } else {
		ForwardSetObjError(paramPtr, resObj);
	    }
	    paramPtr->output.toWrite = -1;
	} else {
	    /*
	     * Process a regular result.
	     */

	    int written;

	    if (Tcl_GetIntFromObj(interp, resObj, &written) != TCL_OK) {
		Tcl_DecrRefCount(resObj);
		resObj = MarshallError(interp);
		ForwardSetObjError(paramPtr, resObj);
		paramPtr->output.toWrite = -1;
	    } else if (written==0 || paramPtr->output.toWrite<written) {
		ForwardSetStaticError(paramPtr, msg_write_toomuch);
		paramPtr->output.toWrite = -1;
	    } else {
		paramPtr->output.toWrite = written;
	    }
	}
        Tcl_Release(rcPtr);
        Tcl_DecrRefCount(bufObj);
	break;
    }

    case ForwardedSeek: {
	Tcl_Obj *offObj = Tcl_NewWideIntObj(paramPtr->seek.offset);
	Tcl_Obj *baseObj = Tcl_NewStringObj(
                (paramPtr->seek.seekMode==SEEK_SET) ? "start" :
                (paramPtr->seek.seekMode==SEEK_CUR) ? "current" : "end", -1);

        Tcl_IncrRefCount(offObj);
        Tcl_IncrRefCount(baseObj);

        Tcl_Preserve(rcPtr);
	if (InvokeTclMethod(rcPtr, METH_SEEK, offObj, baseObj, &resObj)!=TCL_OK){
	    ForwardSetObjError(paramPtr, resObj);
	    paramPtr->seek.offset = -1;
	} else {
	    /*
	     * Process a regular result. If the type is wrong this may change
	     * into an error.
	     */

	    Tcl_WideInt newLoc;

	    if (Tcl_GetWideIntFromObj(interp, resObj, &newLoc) == TCL_OK) {
		if (newLoc < Tcl_LongAsWide(0)) {
		    ForwardSetStaticError(paramPtr, msg_seek_beforestart);
		    paramPtr->seek.offset = -1;
		} else {
		    paramPtr->seek.offset = newLoc;
		}
	    } else {
		Tcl_DecrRefCount(resObj);
		resObj = MarshallError(interp);
		ForwardSetObjError(paramPtr, resObj);
		paramPtr->seek.offset = -1;
	    }
	}
        Tcl_Release(rcPtr);
        Tcl_DecrRefCount(offObj);
        Tcl_DecrRefCount(baseObj);
	break;
    }

    case ForwardedWatch: {
	Tcl_Obj *maskObj = DecodeEventMask(paramPtr->watch.mask);
        /* assert maskObj.refCount == 1 */

        Tcl_Preserve(rcPtr);
	rcPtr->interest = paramPtr->watch.mask;
	(void) InvokeTclMethod(rcPtr, METH_WATCH, maskObj, NULL, NULL);
	Tcl_DecrRefCount(maskObj);
        Tcl_Release(rcPtr);
	break;
    }

    case ForwardedBlock: {
	Tcl_Obj *blockObj = Tcl_NewBooleanObj(!paramPtr->block.nonblocking);

        Tcl_IncrRefCount(blockObj);
        Tcl_Preserve(rcPtr);
	if (InvokeTclMethod(rcPtr, METH_BLOCKING, blockObj, NULL,
                &resObj) != TCL_OK) {
	    ForwardSetObjError(paramPtr, resObj);
	}
        Tcl_Release(rcPtr);
        Tcl_DecrRefCount(blockObj);
	break;
    }

    case ForwardedSetOpt: {
	Tcl_Obj *optionObj = Tcl_NewStringObj(paramPtr->setOpt.name, -1);
	Tcl_Obj *valueObj  = Tcl_NewStringObj(paramPtr->setOpt.value, -1);

        Tcl_IncrRefCount(optionObj);
        Tcl_IncrRefCount(valueObj);
        Tcl_Preserve(rcPtr);
	if (InvokeTclMethod(rcPtr, METH_CONFIGURE, optionObj, valueObj,
                &resObj) != TCL_OK) {
	    ForwardSetObjError(paramPtr, resObj);
	}
        Tcl_Release(rcPtr);
        Tcl_DecrRefCount(optionObj);
        Tcl_DecrRefCount(valueObj);
	break;
    }

    case ForwardedGetOpt: {
	/*
	 * Retrieve the value of one option.
	 */

	Tcl_Obj *optionObj = Tcl_NewStringObj(paramPtr->getOpt.name, -1);

        Tcl_IncrRefCount(optionObj);
        Tcl_Preserve(rcPtr);
	if (InvokeTclMethod(rcPtr, METH_CGET, optionObj, NULL, &resObj)!=TCL_OK){
	    ForwardSetObjError(paramPtr, resObj);
	} else {
	    TclDStringAppendObj(paramPtr->getOpt.value, resObj);
	}
        Tcl_Release(rcPtr);
        Tcl_DecrRefCount(optionObj);
	break;
    }

    case ForwardedGetOptAll:
	/*
	 * Retrieve all options.
	 */

        Tcl_Preserve(rcPtr);
	if (InvokeTclMethod(rcPtr, METH_CGETALL, NULL, NULL, &resObj) != TCL_OK){
	    ForwardSetObjError(paramPtr, resObj);
	} else {
	    /*
	     * Extract list, validate that it is a list, and #elements. See
	     * NOTE (4) as well.
	     */

	    int listc;
	    Tcl_Obj **listv;

	    if (Tcl_ListObjGetElements(interp, resObj, &listc,
                    &listv) != TCL_OK) {
		Tcl_DecrRefCount(resObj);
		resObj = MarshallError(interp);
		ForwardSetObjError(paramPtr, resObj);
	    } else if ((listc % 2) == 1) {
		/*
		 * Odd number of elements is wrong. [x].
		 */

		char *buf = ckalloc(200);
		sprintf(buf,
			"{Expected list with even number of elements, got %d %s instead}",
			listc, (listc == 1 ? "element" : "elements"));

		ForwardSetDynamicError(paramPtr, buf);
	    } else {
		int len;
		const char *str = Tcl_GetStringFromObj(resObj, &len);

		if (len) {
		    TclDStringAppendLiteral(paramPtr->getOpt.value, " ");
		    Tcl_DStringAppend(paramPtr->getOpt.value, str, len);
		}
	    }
	}
        Tcl_Release(rcPtr);
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

	Tcl_MutexLock(&rcForwardMutex);
	resultPtr->result = TCL_OK;
	Tcl_ConditionNotify(&resultPtr->done);
	Tcl_MutexUnlock(&rcForwardMutex);
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

    Tcl_MutexLock(&rcForwardMutex);

    resultPtr = evPtr->resultPtr;
    paramPtr = evPtr->param;

    evPtr->resultPtr = NULL;
    resultPtr->evPtr = NULL;
    resultPtr->result = TCL_ERROR;

    ForwardSetStaticError(paramPtr, msg_send_originlost);

    /*
     * See below: TclSpliceOut(resultPtr, forwardList);
     */

    Tcl_MutexUnlock(&rcForwardMutex);

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
#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 */
