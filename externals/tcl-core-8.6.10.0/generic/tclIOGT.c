/*
 * tclIOGT.c --
 *
 *	Implements a generic transformation exposing the underlying API at the
 *	script level. Contributed by Andreas Kupries.
 *
 * Copyright (c) 2000 Ajuba Solutions
 * Copyright (c) 1999-2000 Andreas Kupries (a.kupries@westend.com)
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclIO.h"

/*
 * Forward declarations of internal procedures. First the driver procedures of
 * the transformation.
 */

static int		TransformBlockModeProc(ClientData instanceData,
			    int mode);
static int		TransformCloseProc(ClientData instanceData,
			    Tcl_Interp *interp);
static int		TransformInputProc(ClientData instanceData, char *buf,
			    int toRead, int *errorCodePtr);
static int		TransformOutputProc(ClientData instanceData,
			    const char *buf, int toWrite, int *errorCodePtr);
static int		TransformSeekProc(ClientData instanceData, long offset,
			    int mode, int *errorCodePtr);
static int		TransformSetOptionProc(ClientData instanceData,
			    Tcl_Interp *interp, const char *optionName,
			    const char *value);
static int		TransformGetOptionProc(ClientData instanceData,
			    Tcl_Interp *interp, const char *optionName,
			    Tcl_DString *dsPtr);
static void		TransformWatchProc(ClientData instanceData, int mask);
static int		TransformGetFileHandleProc(ClientData instanceData,
			    int direction, ClientData *handlePtr);
static int		TransformNotifyProc(ClientData instanceData, int mask);
static Tcl_WideInt	TransformWideSeekProc(ClientData instanceData,
			    Tcl_WideInt offset, int mode, int *errorCodePtr);

/*
 * Forward declarations of internal procedures. Secondly the procedures for
 * handling and generating fileeevents.
 */

static void		TransformChannelHandlerTimer(ClientData clientData);

/*
 * Forward declarations of internal procedures. Third, helper procedures
 * encapsulating essential tasks.
 */

typedef struct TransformChannelData TransformChannelData;

static int		ExecuteCallback(TransformChannelData *ctrl,
			    Tcl_Interp *interp, unsigned char *op,
			    unsigned char *buf, int bufLen, int transmit,
			    int preserve);

/*
 * Action codes to give to 'ExecuteCallback' (argument 'transmit'), telling
 * the procedure what to do with the result of the script it calls.
 */

#define TRANSMIT_DONT	0	/* No transfer to do. */
#define TRANSMIT_DOWN	1	/* Transfer to the underlying channel. */
#define TRANSMIT_SELF	2	/* Transfer into our channel. */
#define TRANSMIT_IBUF	3	/* Transfer to internal input buffer. */
#define TRANSMIT_NUM	4	/* Transfer number to 'maxRead'. */

/*
 * Codes for 'preserve' of 'ExecuteCallback'.
 */

#define P_PRESERVE	1
#define P_NO_PRESERVE	0

/*
 * Strings for the action codes delivered to the script implementing a
 * transformation. Argument 'op' of 'ExecuteCallback'.
 */

#define A_CREATE_WRITE	(UCHARP("create/write"))
#define A_DELETE_WRITE	(UCHARP("delete/write"))
#define A_FLUSH_WRITE	(UCHARP("flush/write"))
#define A_WRITE		(UCHARP("write"))

#define A_CREATE_READ	(UCHARP("create/read"))
#define A_DELETE_READ	(UCHARP("delete/read"))
#define A_FLUSH_READ	(UCHARP("flush/read"))
#define A_READ		(UCHARP("read"))

#define A_QUERY_MAXREAD	(UCHARP("query/maxRead"))
#define A_CLEAR_READ	(UCHARP("clear/read"))

/*
 * Management of a simple buffer.
 */

typedef struct ResultBuffer ResultBuffer;

static inline void	ResultClear(ResultBuffer *r);
static inline void	ResultInit(ResultBuffer *r);
static inline int	ResultEmpty(ResultBuffer *r);
static inline int	ResultCopy(ResultBuffer *r, unsigned char *buf,
			    size_t toRead);
static inline void	ResultAdd(ResultBuffer *r, unsigned char *buf,
			    size_t toWrite);

/*
 * This structure describes the channel type structure for Tcl-based
 * transformations.
 */

static const Tcl_ChannelType transformChannelType = {
    "transform",		/* Type name. */
    TCL_CHANNEL_VERSION_5,	/* v5 channel */
    TransformCloseProc,		/* Close proc. */
    TransformInputProc,		/* Input proc. */
    TransformOutputProc,	/* Output proc. */
    TransformSeekProc,		/* Seek proc. */
    TransformSetOptionProc,	/* Set option proc. */
    TransformGetOptionProc,	/* Get option proc. */
    TransformWatchProc,		/* Initialize notifier. */
    TransformGetFileHandleProc,	/* Get OS handles out of channel. */
    NULL,			/* close2proc */
    TransformBlockModeProc,	/* Set blocking/nonblocking mode.*/
    NULL,			/* Flush proc. */
    TransformNotifyProc,	/* Handling of events bubbling up. */
    TransformWideSeekProc,	/* Wide seek proc. */
    NULL,			/* Thread action. */
    NULL			/* Truncate. */
};

/*
 * Possible values for 'flags' field in control structure, see below.
 */

#define CHANNEL_ASYNC (1<<0)	/* Non-blocking mode. */

/*
 * Definition of the structure containing the information about the internal
 * input buffer.
 */

struct ResultBuffer {
    unsigned char *buf;		/* Reference to the buffer area. */
    size_t allocated;		/* Allocated size of the buffer area. */
    size_t used;		/* Number of bytes in the buffer, no more than
				 * number allocated. */
};

/*
 * Additional bytes to allocate during buffer expansion.
 */

#define INCREMENT	512

/*
 * Number of milliseconds to wait before firing an event to flush out
 * information waiting in buffers (fileevent support).
 */

#define FLUSH_DELAY	5

/*
 * Convenience macro to make some casts easier to use.
 */

#define UCHARP(x)	((unsigned char *) (x))

/*
 * Definition of a structure used by all transformations generated here to
 * maintain their local state.
 */

struct TransformChannelData {
    /*
     * General section. Data to integrate the transformation into the channel
     * system.
     */

    Tcl_Channel self;		/* Our own Channel handle. */
    int readIsFlushed;		/* Flag to note whether in.flushProc was
				 * called or not. */
    int eofPending;		/* Flag: EOF seen down, not raised up */
    int flags;			/* Currently CHANNEL_ASYNC or zero. */
    int watchMask;		/* Current watch/event/interest mask. */
    int mode;			/* Mode of parent channel, OR'ed combination
				 * of TCL_READABLE, TCL_WRITABLE. */
    Tcl_TimerToken timer;	/* Timer for automatic flushing of information
				 * sitting in an internal buffer. Required for
				 * full fileevent support. */

    /*
     * Transformation specific data.
     */

    int maxRead;		/* Maximum allowed number of bytes to read, as
				 * given to us by the Tcl script implementing
				 * the transformation. */
    Tcl_Interp *interp;		/* Reference to the interpreter which created
				 * the transformation. Used to execute the
				 * code below. */
    Tcl_Obj *command;		/* Tcl code to execute for a buffer */
    ResultBuffer result;	/* Internal buffer used to store the result of
				 * a transformation of incoming data. Also
				 * serves as buffer of all data not yet
				 * consumed by the reader. */
    int refCount;
};

static void
PreserveData(
    TransformChannelData *dataPtr)
{
    dataPtr->refCount++;
}

static void
ReleaseData(
    TransformChannelData *dataPtr)
{
    if (--dataPtr->refCount) {
	return;
    }
    ResultClear(&dataPtr->result);
    Tcl_DecrRefCount(dataPtr->command);
    ckfree(dataPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclChannelTransform --
 *
 *	Implements the Tcl "testchannel transform" debugging command. This is
 *	part of the testing environment. This sets up a tcl script (cmdObjPtr)
 *	to be used as a transform on the channel.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
TclChannelTransform(
    Tcl_Interp *interp,		/* Interpreter for result. */
    Tcl_Channel chan,		/* Channel to transform. */
    Tcl_Obj *cmdObjPtr)		/* Script to use for transform. */
{
    Channel *chanPtr;		/* The actual channel. */
    ChannelState *statePtr;	/* State info for channel. */
    int mode;			/* Read/write mode of the channel. */
    int objc;
    TransformChannelData *dataPtr;
    Tcl_DString ds;

    if (chan == NULL) {
	return TCL_ERROR;
    }

    if (TCL_OK != Tcl_ListObjLength(interp, cmdObjPtr, &objc)) {
	Tcl_SetObjResult(interp,
		Tcl_NewStringObj("-command value is not a list", -1));
	return TCL_ERROR;
    }

    chanPtr = (Channel *) chan;
    statePtr = chanPtr->state;
    chanPtr = statePtr->topChanPtr;
    chan = (Tcl_Channel) chanPtr;
    mode = (statePtr->flags & (TCL_READABLE|TCL_WRITABLE));

    /*
     * Now initialize the transformation state and stack it upon the specified
     * channel. One of the necessary things to do is to retrieve the blocking
     * regime of the underlying channel and to use the same for us too.
     */

    dataPtr = ckalloc(sizeof(TransformChannelData));

    dataPtr->refCount = 1;
    Tcl_DStringInit(&ds);
    Tcl_GetChannelOption(interp, chan, "-blocking", &ds);
    dataPtr->readIsFlushed = 0;
    dataPtr->eofPending = 0;
    dataPtr->flags = 0;
    if (ds.string[0] == '0') {
	dataPtr->flags |= CHANNEL_ASYNC;
    }
    Tcl_DStringFree(&ds);

    dataPtr->watchMask = 0;
    dataPtr->mode = mode;
    dataPtr->timer = NULL;
    dataPtr->maxRead = 4096;	/* Initial value not relevant. */
    dataPtr->interp = interp;
    dataPtr->command = cmdObjPtr;
    Tcl_IncrRefCount(dataPtr->command);

    ResultInit(&dataPtr->result);

    dataPtr->self = Tcl_StackChannel(interp, &transformChannelType, dataPtr,
	    mode, chan);
    if (dataPtr->self == NULL) {
	Tcl_AppendPrintfToObj(Tcl_GetObjResult(interp),
		"\nfailed to stack channel \"%s\"", Tcl_GetChannelName(chan));
	ReleaseData(dataPtr);
	return TCL_ERROR;
    }
    Tcl_Preserve(dataPtr->self);

    /*
     * At last initialize the transformation at the script level.
     */

    PreserveData(dataPtr);
    if ((dataPtr->mode & TCL_WRITABLE) && ExecuteCallback(dataPtr, NULL,
	    A_CREATE_WRITE, NULL, 0, TRANSMIT_DONT, P_NO_PRESERVE) != TCL_OK){
	Tcl_UnstackChannel(interp, chan);
	ReleaseData(dataPtr);
	return TCL_ERROR;
    }

    if ((dataPtr->mode & TCL_READABLE) && ExecuteCallback(dataPtr, NULL,
	    A_CREATE_READ, NULL, 0, TRANSMIT_DONT, P_NO_PRESERVE) != TCL_OK) {
	ExecuteCallback(dataPtr, NULL, A_DELETE_WRITE, NULL, 0, TRANSMIT_DONT,
		P_NO_PRESERVE);
	Tcl_UnstackChannel(interp, chan);
	ReleaseData(dataPtr);
	return TCL_ERROR;
    }

    ReleaseData(dataPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ExecuteCallback --
 *
 *	Executes the defined callback for buffer and operation.
 *
 * Side effects:
 *	As of the executed tcl script.
 *
 * Result:
 *	A standard TCL error code. In case of an error a message is left in
 *	the result area of the specified interpreter.
 *
 *----------------------------------------------------------------------
 */

static int
ExecuteCallback(
    TransformChannelData *dataPtr,
				/* Transformation with the callback. */
    Tcl_Interp *interp,		/* Current interpreter, possibly NULL. */
    unsigned char *op,		/* Operation invoking the callback. */
    unsigned char *buf,		/* Buffer to give to the script. */
    int bufLen,			/* And its length. */
    int transmit,		/* Flag, determines whether the result of the
				 * callback is sent to the underlying channel
				 * or not. */
    int preserve)		/* Flag. If true the procedure will preserve
				 * the result state of all accessed
				 * interpreters. */
{
    Tcl_Obj *resObj;		/* See below, switch (transmit). */
    int resLen;
    unsigned char *resBuf;
    Tcl_InterpState state = NULL;
    int res = TCL_OK;
    Tcl_Obj *command = TclListObjCopy(NULL, dataPtr->command);
    Tcl_Interp *eval = dataPtr->interp;

    Tcl_Preserve(eval);

    /*
     * Step 1, create the complete command to execute. Do this by appending
     * operation and buffer to operate upon to a copy of the callback
     * definition. We *cannot* create a list containing 3 objects and then use
     * 'Tcl_EvalObjv', because the command may contain additional prefixed
     * arguments. Feather's curried commands would come in handy here.
     */

    if (preserve == P_PRESERVE) {
	state = Tcl_SaveInterpState(eval, res);
    }

    Tcl_IncrRefCount(command);
    Tcl_ListObjAppendElement(NULL, command, Tcl_NewStringObj((char *) op, -1));

    /*
     * Use a byte-array to prevent the misinterpretation of binary data coming
     * through as UTF while at the tcl level.
     */

    Tcl_ListObjAppendElement(NULL, command, Tcl_NewByteArrayObj(buf, bufLen));

    /*
     * Step 2, execute the command at the global level of the interpreter used
     * to create the transformation. Destroy the command afterward. If an
     * error occured and the current interpreter is defined and not equal to
     * the interpreter for the callback, then copy the error message into
     * current interpreter. Don't copy if in preservation mode.
     */

    res = Tcl_EvalObjEx(eval, command, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(command);
    command = NULL;

    if ((res != TCL_OK) && (interp != NULL) && (eval != interp)
	    && (preserve == P_NO_PRESERVE)) {
	Tcl_SetObjResult(interp, Tcl_GetObjResult(eval));
	Tcl_Release(eval);
	return res;
    }

    /*
     * Step 3, transmit a possible conversion result to the underlying
     * channel, or ourselves.
     */

    switch (transmit) {
    case TRANSMIT_DONT:
	/* nothing to do */
	break;

    case TRANSMIT_DOWN:
	if (dataPtr->self == NULL) {
	    break;
	}
	resObj = Tcl_GetObjResult(eval);
	resBuf = Tcl_GetByteArrayFromObj(resObj, &resLen);
	Tcl_WriteRaw(Tcl_GetStackedChannel(dataPtr->self), (char *) resBuf,
		resLen);
	break;

    case TRANSMIT_SELF:
	if (dataPtr->self == NULL) {
	    break;
	}
	resObj = Tcl_GetObjResult(eval);
	resBuf = Tcl_GetByteArrayFromObj(resObj, &resLen);
	Tcl_WriteRaw(dataPtr->self, (char *) resBuf, resLen);
	break;

    case TRANSMIT_IBUF:
	resObj = Tcl_GetObjResult(eval);
	resBuf = Tcl_GetByteArrayFromObj(resObj, &resLen);
	ResultAdd(&dataPtr->result, resBuf, resLen);
	break;

    case TRANSMIT_NUM:
	/*
	 * Interpret result as integer number.
	 */

	resObj = Tcl_GetObjResult(eval);
	TclGetIntFromObj(eval, resObj, &dataPtr->maxRead);
	break;
    }

    Tcl_ResetResult(eval);
    if (preserve == P_PRESERVE) {
	(void) Tcl_RestoreInterpState(eval, state);
    }
    Tcl_Release(eval);
    return res;
}

/*
 *----------------------------------------------------------------------
 *
 * TransformBlockModeProc --
 *
 *	Trap handler. Called by the generic IO system during option processing
 *	to change the blocking mode of the channel.
 *
 * Side effects:
 *	Forwards the request to the underlying channel.
 *
 * Result:
 *	0 if successful, errno when failed.
 *
 *----------------------------------------------------------------------
 */

static int
TransformBlockModeProc(
    ClientData instanceData,	/* State of transformation. */
    int mode)			/* New blocking mode. */
{
    TransformChannelData *dataPtr = instanceData;

    if (mode == TCL_MODE_NONBLOCKING) {
	dataPtr->flags |= CHANNEL_ASYNC;
    } else {
	dataPtr->flags &= ~CHANNEL_ASYNC;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TransformCloseProc --
 *
 *	Trap handler. Called by the generic IO system during destruction of
 *	the transformation channel.
 *
 * Side effects:
 *	Releases the memory allocated in 'Tcl_TransformObjCmd'.
 *
 * Result:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TransformCloseProc(
    ClientData instanceData,
    Tcl_Interp *interp)
{
    TransformChannelData *dataPtr = instanceData;

    /*
     * Important: In this procedure 'dataPtr->self' already points to the
     * underlying channel.
     *
     * There is no need to cancel an existing channel handler, this is already
     * done. Either by 'Tcl_UnstackChannel' or by the general cleanup in
     * 'Tcl_Close'.
     *
     * But we have to cancel an active timer to prevent it from firing on the
     * removed channel.
     */

    if (dataPtr->timer != NULL) {
	Tcl_DeleteTimerHandler(dataPtr->timer);
	dataPtr->timer = NULL;
    }

    /*
     * Now flush data waiting in internal buffers to output and input. The
     * input must be done despite the fact that there is no real receiver for
     * it anymore. But the scripts might have sideeffects other parts of the
     * system rely on (f.e. signaling the close to interested parties).
     */

    PreserveData(dataPtr);
    if (dataPtr->mode & TCL_WRITABLE) {
	ExecuteCallback(dataPtr, interp, A_FLUSH_WRITE, NULL, 0,
		TRANSMIT_DOWN, P_PRESERVE);
    }

    if ((dataPtr->mode & TCL_READABLE) && !dataPtr->readIsFlushed) {
	dataPtr->readIsFlushed = 1;
	ExecuteCallback(dataPtr, interp, A_FLUSH_READ, NULL, 0, TRANSMIT_IBUF,
		P_PRESERVE);
    }

    if (dataPtr->mode & TCL_WRITABLE) {
	ExecuteCallback(dataPtr, interp, A_DELETE_WRITE, NULL, 0,
		TRANSMIT_DONT, P_PRESERVE);
    }
    if (dataPtr->mode & TCL_READABLE) {
	ExecuteCallback(dataPtr, interp, A_DELETE_READ, NULL, 0,
		TRANSMIT_DONT, P_PRESERVE);
    }
    ReleaseData(dataPtr);

    /*
     * General cleanup.
     */

    Tcl_Release(dataPtr->self);
    dataPtr->self = NULL;
    ReleaseData(dataPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TransformInputProc --
 *
 *	Called by the generic IO system to convert read data.
 *
 * Side effects:
 *	As defined by the conversion.
 *
 * Result:
 *	A transformed buffer.
 *
 *----------------------------------------------------------------------
 */

static int
TransformInputProc(
    ClientData instanceData,
    char *buf,
    int toRead,
    int *errorCodePtr)
{
    TransformChannelData *dataPtr = instanceData;
    int gotBytes, read, copied;
    Tcl_Channel downChan;

    /*
     * Should assert(dataPtr->mode & TCL_READABLE);
     */

    if (toRead == 0 || dataPtr->self == NULL) {
	/*
	 * Catch a no-op. TODO: Is this a panic()?
	 */
	return 0;
    }

    gotBytes = 0;
    downChan = Tcl_GetStackedChannel(dataPtr->self);

    PreserveData(dataPtr);
    while (toRead > 0) {
	/*
	 * Loop until the request is satisfied (or no data is available from
	 * below, possibly EOF).
	 */

	copied = ResultCopy(&dataPtr->result, UCHARP(buf), toRead);
	toRead -= copied;
	buf += copied;
	gotBytes += copied;

	if (toRead == 0) {
	    /*
	     * The request was completely satisfied from our buffers. We can
	     * break out of the loop and return to the caller.
	     */

	    break;
	}

	/*
	 * Length (dataPtr->result) == 0, toRead > 0 here. Use the incoming
	 * 'buf'! as target to store the intermediary information read from
	 * the underlying channel.
	 *
	 * Ask the tcl level how much data it allows us to read from the
	 * underlying channel. This feature allows the transform to signal EOF
	 * upstream although there is none downstream. Useful to control an
	 * unbounded 'fcopy', either through counting bytes, or by pattern
	 * matching.
	 */

	ExecuteCallback(dataPtr, NULL, A_QUERY_MAXREAD, NULL, 0,
		TRANSMIT_NUM /* -> maxRead */, P_PRESERVE);

	if (dataPtr->maxRead >= 0) {
	    if (dataPtr->maxRead < toRead) {
		toRead = dataPtr->maxRead;
	    }
	} /* else: 'maxRead < 0' == Accept the current value of toRead. */
	if (toRead <= 0) {
	    break;
	}
	if (dataPtr->eofPending) {
	    /*
	     * Already saw EOF from downChan; don't ask again.
	     * NOTE: Could move this up to avoid the last maxRead
	     * execution.  Believe this would still be correct behavior,
	     * but the test suite tests the whole command callback
	     * sequence, so leave it unchanged for now.
	     */

	    break;
	}

	/*
	 * Get bytes from the underlying channel.
	 */

	read = Tcl_ReadRaw(downChan, buf, toRead);
	if (read < 0) {
	    if (Tcl_InputBlocked(downChan) && (gotBytes > 0)) {
		/*
		 * Zero bytes available from downChan because blocked.
		 * But nonzero bytes already copied, so total is a
		 * valid blocked short read. Return to caller.
		 */

		break;
	    }

	    /*
	     * Either downChan is not blocked (there's a real error).
	     * or it is and there are no bytes copied yet.  In either
	     * case we want to pass the "error" along to the caller,
	     * either to report an error, or to signal to the caller
	     * that zero bytes are available because blocked.
	     */

	    *errorCodePtr = Tcl_GetErrno();
	    gotBytes = -1;
	    break;
	} else if (read == 0) {

	    /*
	     * Zero returned from Tcl_ReadRaw() always indicates EOF
	     * on the down channel.
	     */

	    dataPtr->eofPending = 1;
	    dataPtr->readIsFlushed = 1;
	    ExecuteCallback(dataPtr, NULL, A_FLUSH_READ, NULL, 0,
		    TRANSMIT_IBUF, P_PRESERVE);

	    if (ResultEmpty(&dataPtr->result)) {
		/*
		 * We had nothing to flush.
		 */

		break;
	    }

	    continue;		/* at: while (toRead > 0) */
	} /* read == 0 */

	/*
	 * Transform the read chunk and add the result to our read buffer
	 * (dataPtr->result).
	 */

	if (ExecuteCallback(dataPtr, NULL, A_READ, UCHARP(buf), read,
		TRANSMIT_IBUF, P_PRESERVE) != TCL_OK) {
	    *errorCodePtr = EINVAL;
	    gotBytes = -1;
	    break;
	}
    } /* while toRead > 0 */

    if (gotBytes == 0) {
	dataPtr->eofPending = 0;
    }
    ReleaseData(dataPtr);
    return gotBytes;
}

/*
 *----------------------------------------------------------------------
 *
 * TransformOutputProc --
 *
 *	Called by the generic IO system to convert data waiting to be written.
 *
 * Side effects:
 *	As defined by the transformation.
 *
 * Result:
 *	A transformed buffer.
 *
 *----------------------------------------------------------------------
 */

static int
TransformOutputProc(
    ClientData instanceData,
    const char *buf,
    int toWrite,
    int *errorCodePtr)
{
    TransformChannelData *dataPtr = instanceData;

    /*
     * Should assert(dataPtr->mode & TCL_WRITABLE);
     */

    if (toWrite == 0) {
	/*
	 * Catch a no-op.
	 */

	return 0;
    }

    PreserveData(dataPtr);
    if (ExecuteCallback(dataPtr, NULL, A_WRITE, UCHARP(buf), toWrite,
	    TRANSMIT_DOWN, P_NO_PRESERVE) != TCL_OK) {
	*errorCodePtr = EINVAL;
	toWrite = -1;
    }
    ReleaseData(dataPtr);

    return toWrite;
}

/*
 *----------------------------------------------------------------------
 *
 * TransformSeekProc --
 *
 *	This procedure is called by the generic IO level to move the access
 *	point in a channel.
 *
 * Side effects:
 *	Moves the location at which the channel will be accessed in future
 *	operations. Flushes all transformation buffers, then forwards it to
 *	the underlying channel.
 *
 * Result:
 *	-1 if failed, the new position if successful. An output argument
 *	contains the POSIX error code if an error occurred, or zero.
 *
 *----------------------------------------------------------------------
 */

static int
TransformSeekProc(
    ClientData instanceData,	/* The channel to manipulate. */
    long offset,		/* Size of movement. */
    int mode,			/* How to move. */
    int *errorCodePtr)		/* Location of error flag. */
{
    TransformChannelData *dataPtr = instanceData;
    Tcl_Channel parent = Tcl_GetStackedChannel(dataPtr->self);
    const Tcl_ChannelType *parentType = Tcl_GetChannelType(parent);
    Tcl_DriverSeekProc *parentSeekProc = Tcl_ChannelSeekProc(parentType);

    if ((offset == 0) && (mode == SEEK_CUR)) {
	/*
	 * This is no seek but a request to tell the caller the current
	 * location. Simply pass the request down.
	 */

	return parentSeekProc(Tcl_GetChannelInstanceData(parent), offset,
		mode, errorCodePtr);
    }

    /*
     * It is a real request to change the position. Flush all data waiting for
     * output and discard everything in the input buffers. Then pass the
     * request down, unchanged.
     */

    PreserveData(dataPtr);
    if (dataPtr->mode & TCL_WRITABLE) {
	ExecuteCallback(dataPtr, NULL, A_FLUSH_WRITE, NULL, 0, TRANSMIT_DOWN,
		P_NO_PRESERVE);
    }

    if (dataPtr->mode & TCL_READABLE) {
	ExecuteCallback(dataPtr, NULL, A_CLEAR_READ, NULL, 0, TRANSMIT_DONT,
		P_NO_PRESERVE);
	ResultClear(&dataPtr->result);
	dataPtr->readIsFlushed = 0;
	dataPtr->eofPending = 0;
    }
    ReleaseData(dataPtr);

    return parentSeekProc(Tcl_GetChannelInstanceData(parent), offset, mode,
	    errorCodePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TransformWideSeekProc --
 *
 *	This procedure is called by the generic IO level to move the access
 *	point in a channel, with a (potentially) 64-bit offset.
 *
 * Side effects:
 *	Moves the location at which the channel will be accessed in future
 *	operations. Flushes all transformation buffers, then forwards it to
 *	the underlying channel.
 *
 * Result:
 *	-1 if failed, the new position if successful. An output argument
 *	contains the POSIX error code if an error occurred, or zero.
 *
 *----------------------------------------------------------------------
 */

static Tcl_WideInt
TransformWideSeekProc(
    ClientData instanceData,	/* The channel to manipulate. */
    Tcl_WideInt offset,		/* Size of movement. */
    int mode,			/* How to move. */
    int *errorCodePtr)		/* Location of error flag. */
{
    TransformChannelData *dataPtr = instanceData;
    Tcl_Channel parent = Tcl_GetStackedChannel(dataPtr->self);
    const Tcl_ChannelType *parentType	= Tcl_GetChannelType(parent);
    Tcl_DriverSeekProc *parentSeekProc = Tcl_ChannelSeekProc(parentType);
    Tcl_DriverWideSeekProc *parentWideSeekProc =
	    Tcl_ChannelWideSeekProc(parentType);
    ClientData parentData = Tcl_GetChannelInstanceData(parent);

    if ((offset == Tcl_LongAsWide(0)) && (mode == SEEK_CUR)) {
	/*
	 * This is no seek but a request to tell the caller the current
	 * location. Simply pass the request down.
	 */

	if (parentWideSeekProc != NULL) {
	    return parentWideSeekProc(parentData, offset, mode, errorCodePtr);
	}

	return Tcl_LongAsWide(parentSeekProc(parentData, 0, mode,
		errorCodePtr));
    }

    /*
     * It is a real request to change the position. Flush all data waiting for
     * output and discard everything in the input buffers. Then pass the
     * request down, unchanged.
     */

    PreserveData(dataPtr);
    if (dataPtr->mode & TCL_WRITABLE) {
	ExecuteCallback(dataPtr, NULL, A_FLUSH_WRITE, NULL, 0, TRANSMIT_DOWN,
		P_NO_PRESERVE);
    }

    if (dataPtr->mode & TCL_READABLE) {
	ExecuteCallback(dataPtr, NULL, A_CLEAR_READ, NULL, 0, TRANSMIT_DONT,
		P_NO_PRESERVE);
	ResultClear(&dataPtr->result);
	dataPtr->readIsFlushed = 0;
	dataPtr->eofPending = 0;
    }
    ReleaseData(dataPtr);

    /*
     * If we have a wide seek capability, we should stick with that.
     */

    if (parentWideSeekProc != NULL) {
	return parentWideSeekProc(parentData, offset, mode, errorCodePtr);
    }

    /*
     * We're transferring to narrow seeks at this point; this is a bit complex
     * because we have to check whether the seek is possible first (i.e.
     * whether we are losing information in truncating the bits of the
     * offset). Luckily, there's a defined error for what happens when trying
     * to go out of the representable range.
     */

    if (offset<Tcl_LongAsWide(LONG_MIN) || offset>Tcl_LongAsWide(LONG_MAX)) {
	*errorCodePtr = EOVERFLOW;
	return Tcl_LongAsWide(-1);
    }

    return Tcl_LongAsWide(parentSeekProc(parentData, Tcl_WideAsLong(offset),
	    mode, errorCodePtr));
}

/*
 *----------------------------------------------------------------------
 *
 * TransformSetOptionProc --
 *
 *	Called by generic layer to handle the reconfiguration of channel
 *	specific options. As this channel type does not have such, it simply
 *	passes all requests downstream.
 *
 * Side effects:
 *	As defined by the channel downstream.
 *
 * Result:
 *	A standard TCL error code.
 *
 *----------------------------------------------------------------------
 */

static int
TransformSetOptionProc(
    ClientData instanceData,
    Tcl_Interp *interp,
    const char *optionName,
    const char *value)
{
    TransformChannelData *dataPtr = instanceData;
    Tcl_Channel downChan = Tcl_GetStackedChannel(dataPtr->self);
    Tcl_DriverSetOptionProc *setOptionProc;

    setOptionProc = Tcl_ChannelSetOptionProc(Tcl_GetChannelType(downChan));
    if (setOptionProc == NULL) {
	return TCL_ERROR;
    }

    return setOptionProc(Tcl_GetChannelInstanceData(downChan), interp,
	    optionName, value);
}

/*
 *----------------------------------------------------------------------
 *
 * TransformGetOptionProc --
 *
 *	Called by generic layer to handle requests for the values of channel
 *	specific options. As this channel type does not have such, it simply
 *	passes all requests downstream.
 *
 * Side effects:
 *	As defined by the channel downstream.
 *
 * Result:
 *	A standard TCL error code.
 *
 *----------------------------------------------------------------------
 */

static int
TransformGetOptionProc(
    ClientData instanceData,
    Tcl_Interp *interp,
    const char *optionName,
    Tcl_DString *dsPtr)
{
    TransformChannelData *dataPtr = instanceData;
    Tcl_Channel downChan = Tcl_GetStackedChannel(dataPtr->self);
    Tcl_DriverGetOptionProc *getOptionProc;

    getOptionProc = Tcl_ChannelGetOptionProc(Tcl_GetChannelType(downChan));
    if (getOptionProc != NULL) {
	return getOptionProc(Tcl_GetChannelInstanceData(downChan), interp,
		optionName, dsPtr);
    } else if (optionName == NULL) {
	/*
	 * Request is query for all options, this is ok.
	 */

	return TCL_OK;
    }

    /*
     * Request for a specific option has to fail, since we don't have any.
     */

    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TransformWatchProc --
 *
 *	Initialize the notifier to watch for events from this channel.
 *
 * Side effects:
 *	Sets up the notifier so that a future event on the channel will be
 *	seen by Tcl.
 *
 * Result:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
TransformWatchProc(
    ClientData instanceData,	/* Channel to watch. */
    int mask)			/* Events of interest. */
{
    TransformChannelData *dataPtr = instanceData;
    Tcl_Channel downChan;

    /*
     * The caller expressed interest in events occuring for this channel. We
     * are forwarding the call to the underlying channel now.
     */

    dataPtr->watchMask = mask;

    /*
     * No channel handlers any more. We will be notified automatically about
     * events on the channel below via a call to our 'TransformNotifyProc'.
     * But we have to pass the interest down now. We are allowed to add
     * additional 'interest' to the mask if we want to. But this
     * transformation has no such interest. It just passes the request down,
     * unchanged.
     */

    if (dataPtr->self == NULL) {
	return;
    }
    downChan = Tcl_GetStackedChannel(dataPtr->self);

    Tcl_GetChannelType(downChan)->watchProc(
	    Tcl_GetChannelInstanceData(downChan), mask);

    /*
     * Management of the internal timer.
     */

    if ((dataPtr->timer != NULL) &&
	    (!(mask & TCL_READABLE) || ResultEmpty(&dataPtr->result))) {
	/*
	 * A pending timer exists, but either is there no (more) interest in
	 * the events it generates or nothing is available for reading, so
	 * remove it.
	 */

	Tcl_DeleteTimerHandler(dataPtr->timer);
	dataPtr->timer = NULL;
    }

    if ((dataPtr->timer == NULL) && (mask & TCL_READABLE)
	    && !ResultEmpty(&dataPtr->result)) {
	/*
	 * There is no pending timer, but there is interest in readable events
	 * and we actually have data waiting, so generate a timer to flush
	 * that.
	 */

	dataPtr->timer = Tcl_CreateTimerHandler(FLUSH_DELAY,
		TransformChannelHandlerTimer, dataPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TransformGetFileHandleProc --
 *
 *	Called from Tcl_GetChannelHandle to retrieve OS specific file handle
 *	from inside this channel.
 *
 * Side effects:
 *	None.
 *
 * Result:
 *	The appropriate Tcl_File or NULL if not present.
 *
 *----------------------------------------------------------------------
 */

static int
TransformGetFileHandleProc(
    ClientData instanceData,	/* Channel to query. */
    int direction,		/* Direction of interest. */
    ClientData *handlePtr)	/* Place to store the handle into. */
{
    TransformChannelData *dataPtr = instanceData;

    /*
     * Return the handle belonging to parent channel. IOW, pass the request
     * down and the result up.
     */

    return Tcl_GetChannelHandle(Tcl_GetStackedChannel(dataPtr->self),
	    direction, handlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TransformNotifyProc --
 *
 *	Handler called by Tcl to inform us of activity on the underlying
 *	channel.
 *
 * Side effects:
 *	May process the incoming event by itself.
 *
 * Result:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TransformNotifyProc(
    ClientData clientData,	/* The state of the notified
				 * transformation. */
    int mask)			/* The mask of occuring events. */
{
    TransformChannelData *dataPtr = clientData;

    /*
     * An event occured in the underlying channel. This transformation doesn't
     * process such events thus returns the incoming mask unchanged.
     */

    if (dataPtr->timer != NULL) {
	/*
	 * Delete an existing timer. It was not fired, yet we are here, so the
	 * channel below generated such an event and we don't have to. The
	 * renewal of the interest after the execution of channel handlers
	 * will eventually cause us to recreate the timer (in
	 * TransformWatchProc).
	 */

	Tcl_DeleteTimerHandler(dataPtr->timer);
	dataPtr->timer = NULL;
    }
    return mask;
}

/*
 *----------------------------------------------------------------------
 *
 * TransformChannelHandlerTimer --
 *
 *	Called by the notifier (-> timer) to flush out information waiting in
 *	the input buffer.
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
TransformChannelHandlerTimer(
    ClientData clientData)	/* Transformation to query. */
{
    TransformChannelData *dataPtr = clientData;

    dataPtr->timer = NULL;
    if (!(dataPtr->watchMask&TCL_READABLE) || ResultEmpty(&dataPtr->result)) {
	/*
	 * The timer fired, but either is there no (more) interest in the
	 * events it generates or nothing is available for reading, so ignore
	 * it and don't recreate it.
	 */

	return;
    }
    Tcl_NotifyChannel(dataPtr->self, TCL_READABLE);
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

static inline void
ResultClear(
    ResultBuffer *r)		/* Reference to the buffer to clear out. */
{
    r->used = 0;

    if (r->allocated) {
	ckfree(r->buf);
	r->buf = NULL;
	r->allocated = 0;
    }
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

static inline void
ResultInit(
    ResultBuffer *r)		/* Reference to the structure to
				 * initialize. */
{
    r->used = 0;
    r->allocated = 0;
    r->buf = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * ResultEmpty --
 *
 *	Returns whether the number of bytes stored in the buffer is zero.
 *
 * Side effects:
 *	None.
 *
 * Result:
 *	A boolean.
 *
 *----------------------------------------------------------------------
 */

static inline int
ResultEmpty(
    ResultBuffer *r)		/* The structure to query. */
{
    return r->used == 0;
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

static inline int
ResultCopy(
    ResultBuffer *r,		/* The buffer to read from. */
    unsigned char *buf,		/* The buffer to copy into. */
    size_t toRead)		/* Number of requested bytes. */
{
    if (r->used == 0) {
	/*
	 * Nothing to copy in the case of an empty buffer.
	 */

	return 0;
    } else if (r->used == toRead) {
	/*
	 * We have just enough. Copy everything to the caller.
	 */

	memcpy(buf, r->buf, toRead);
	r->used = 0;
    } else if (r->used > toRead) {
	/*
	 * The internal buffer contains more than requested. Copy the
	 * requested subset to the caller, and shift the remaining bytes down.
	 */

	memcpy(buf, r->buf, toRead);
	memmove(r->buf, r->buf + toRead, r->used - toRead);
	r->used -= toRead;
    } else {
	/*
	 * There is not enough in the buffer to satisfy the caller, so take
	 * everything.
	 */

	memcpy(buf, r->buf, r->used);
	toRead = r->used;
	r->used = 0;
    }
    return toRead;
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

static inline void
ResultAdd(
    ResultBuffer *r,		/* The buffer to extend. */
    unsigned char *buf,		/* The buffer to read from. */
    size_t toWrite)		/* The number of bytes in 'buf'. */
{
    if (r->used + toWrite > r->allocated) {
	/*
	 * Extension of the internal buffer is required.
	 */

	if (r->allocated == 0) {
	    r->allocated = toWrite + INCREMENT;
	    r->buf = ckalloc(r->allocated);
	} else {
	    r->allocated += toWrite + INCREMENT;
	    r->buf = ckrealloc(r->buf, r->allocated);
	}
    }

    /*
     * Now we may copy the data.
     */

    memcpy(r->buf + r->used, buf, toWrite);
    r->used += toWrite;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
