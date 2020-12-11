/*
 * tclIO.c --
 *
 *	This file provides the generic portions (those that are the same on
 *	all platforms and for all channel types) of Tcl's IO facilities.
 *
 * Copyright (c) 1998-2000 Ajuba Solutions
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Contributions from Don Porter, NIST, 2014. (not subject to US copyright)
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclIO.h"
#include <assert.h>

/*
 * For each channel handler registered in a call to Tcl_CreateChannelHandler,
 * there is one record of the following type. All of records for a specific
 * channel are chained together in a singly linked list which is stored in
 * the channel structure.
 */

typedef struct ChannelHandler {
    Channel *chanPtr;		/* The channel structure for this channel. */
    int mask;			/* Mask of desired events. */
    Tcl_ChannelProc *proc;	/* Procedure to call in the type of
				 * Tcl_CreateChannelHandler. */
    ClientData clientData;	/* Argument to pass to procedure. */
    struct ChannelHandler *nextPtr;
				/* Next one in list of registered handlers. */
} ChannelHandler;

/*
 * This structure keeps track of the current ChannelHandler being invoked in
 * the current invocation of Tcl_NotifyChannel. There is a potential
 * problem if a ChannelHandler is deleted while it is the current one, since
 * Tcl_NotifyChannel needs to look at the nextPtr field. To handle this
 * problem, structures of the type below indicate the next handler to be
 * processed for any (recursively nested) dispatches in progress. The
 * nextHandlerPtr field is updated if the handler being pointed to is deleted.
 * The nestedHandlerPtr field is used to chain together all recursive
 * invocations, so that Tcl_DeleteChannelHandler can find all the recursively
 * nested invocations of Tcl_NotifyChannel and compare the handler being
 * deleted against the NEXT handler to be invoked in that invocation; when it
 * finds such a situation, Tcl_DeleteChannelHandler updates the nextHandlerPtr
 * field of the structure to the next handler.
 */

typedef struct NextChannelHandler {
    ChannelHandler *nextHandlerPtr;	/* The next handler to be invoked in
					 * this invocation. */
    struct NextChannelHandler *nestedHandlerPtr;
					/* Next nested invocation of
					 * Tcl_NotifyChannel. */
} NextChannelHandler;

/*
 * The following structure is used by Tcl_GetsObj() to encapsulates the
 * state for a "gets" operation.
 */

typedef struct GetsState {
    Tcl_Obj *objPtr;		/* The object to which UTF-8 characters
				 * will be appended. */
    char **dstPtr;		/* Pointer into objPtr's string rep where
				 * next character should be stored. */
    Tcl_Encoding encoding;	/* The encoding to use to convert raw bytes
				 * to UTF-8.  */
    ChannelBuffer *bufPtr;	/* The current buffer of raw bytes being
				 * emptied. */
    Tcl_EncodingState state;	/* The encoding state just before the last
				 * external to UTF-8 conversion in
				 * FilterInputBytes(). */
    int rawRead;		/* The number of bytes removed from bufPtr
				 * in the last call to FilterInputBytes(). */
    int bytesWrote;		/* The number of bytes of UTF-8 data
				 * appended to objPtr during the last call to
				 * FilterInputBytes(). */
    int charsWrote;		/* The corresponding number of UTF-8
				 * characters appended to objPtr during the
				 * last call to FilterInputBytes(). */
    int totalChars;		/* The total number of UTF-8 characters
				 * appended to objPtr so far, just before the
				 * last call to FilterInputBytes(). */
} GetsState;

/*
 * The following structure encapsulates the state for a background channel
 * copy.  Note that the data buffer for the copy will be appended to this
 * structure.
 */

typedef struct CopyState {
    struct Channel *readPtr;	/* Pointer to input channel. */
    struct Channel *writePtr;	/* Pointer to output channel. */
    int readFlags;		/* Original read channel flags. */
    int writeFlags;		/* Original write channel flags. */
    Tcl_WideInt toRead;		/* Number of bytes to copy, or -1. */
    Tcl_WideInt total;		/* Total bytes transferred (written). */
    Tcl_Interp *interp;		/* Interp that started the copy. */
    Tcl_Obj *cmdPtr;		/* Command to be invoked at completion. */
    int bufSize;		/* Size of appended buffer. */
    char buffer[1];		/* Copy buffer, this must be the last
                                 * field. */
} CopyState;

/*
 * All static variables used in this file are collected into a single instance
 * of the following structure. For multi-threaded implementations, there is
 * one instance of this structure for each thread.
 *
 * Notice that different structures with the same name appear in other files.
 * The structure defined below is used in this file only.
 */

typedef struct ThreadSpecificData {
    NextChannelHandler *nestedHandlerPtr;
				/* This variable holds the list of nested
				 * Tcl_NotifyChannel invocations. */
    ChannelState *firstCSPtr;	/* List of all channels currently open,
				 * indexed by ChannelState, as only one
				 * ChannelState exists per set of stacked
				 * channels. */
    Tcl_Channel stdinChannel;	/* Static variable for the stdin channel. */
    int stdinInitialized;
    Tcl_Channel stdoutChannel;	/* Static variable for the stdout channel. */
    int stdoutInitialized;
    Tcl_Channel stderrChannel;	/* Static variable for the stderr channel. */
    int stderrInitialized;
    Tcl_Encoding binaryEncoding;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * Structure to record a close callback. One such record exists for
 * each close callback registered for a channel.
 */

typedef struct CloseCallback {
    Tcl_CloseProc *proc;		/* The procedure to call. */
    ClientData clientData;		/* Arbitrary one-word data to pass
					 * to the callback. */
    struct CloseCallback *nextPtr;	/* For chaining close callbacks. */
} CloseCallback;

/*
 * Static functions in this file:
 */

static ChannelBuffer *	AllocChannelBuffer(int length);
static void		PreserveChannelBuffer(ChannelBuffer *bufPtr);
static void		ReleaseChannelBuffer(ChannelBuffer *bufPtr);
static int		IsShared(ChannelBuffer *bufPtr);
static void		ChannelFree(Channel *chanPtr);
static void		ChannelTimerProc(ClientData clientData);
static int		ChanRead(Channel *chanPtr, char *dst, int dstSize);
static int		CheckChannelErrors(ChannelState *statePtr,
			    int direction);
static int		CheckForDeadChannel(Tcl_Interp *interp,
			    ChannelState *statePtr);
static void		CheckForStdChannelsBeingClosed(Tcl_Channel chan);
static void		CleanupChannelHandlers(Tcl_Interp *interp,
			    Channel *chanPtr);
static int		CloseChannel(Tcl_Interp *interp, Channel *chanPtr,
			    int errorCode);
static int		CloseChannelPart(Tcl_Interp *interp, Channel *chanPtr,
			    int errorCode, int flags);
static int		CloseWrite(Tcl_Interp *interp, Channel *chanPtr);
static void		CommonGetsCleanup(Channel *chanPtr);
static int		CopyData(CopyState *csPtr, int mask);
static int		MoveBytes(CopyState *csPtr);

static void		MBCallback(CopyState *csPtr, Tcl_Obj *errObj);
static void		MBError(CopyState *csPtr, int mask, int errorCode);
static int		MBRead(CopyState *csPtr);
static int		MBWrite(CopyState *csPtr);
static void		MBEvent(ClientData clientData, int mask);

static void		CopyEventProc(ClientData clientData, int mask);
static void		CreateScriptRecord(Tcl_Interp *interp,
			    Channel *chanPtr, int mask, Tcl_Obj *scriptPtr);
static void		DeleteChannelTable(ClientData clientData,
			    Tcl_Interp *interp);
static void		DeleteScriptRecord(Tcl_Interp *interp,
			    Channel *chanPtr, int mask);
static int		DetachChannel(Tcl_Interp *interp, Tcl_Channel chan);
static void		DiscardInputQueued(ChannelState *statePtr,
			    int discardSavedBuffers);
static void		DiscardOutputQueued(ChannelState *chanPtr);
static int		DoRead(Channel *chanPtr, char *dst, int bytesToRead,
			    int allowShortReads);
static int		DoReadChars(Channel *chan, Tcl_Obj *objPtr, int toRead,
			    int appendFlag);
static int		FilterInputBytes(Channel *chanPtr,
			    GetsState *statePtr);
static int		FlushChannel(Tcl_Interp *interp, Channel *chanPtr,
			    int calledFromAsyncFlush);
static int		TclGetsObjBinary(Tcl_Channel chan, Tcl_Obj *objPtr);
static Tcl_Encoding	GetBinaryEncoding();
static void		FreeBinaryEncoding(ClientData clientData);
static Tcl_HashTable *	GetChannelTable(Tcl_Interp *interp);
static int		GetInput(Channel *chanPtr);
static int		HaveVersion(const Tcl_ChannelType *typePtr,
			    Tcl_ChannelTypeVersion minimumVersion);
static void		PeekAhead(Channel *chanPtr, char **dstEndPtr,
			    GetsState *gsPtr);
static int		ReadBytes(ChannelState *statePtr, Tcl_Obj *objPtr,
			    int charsLeft);
static int		ReadChars(ChannelState *statePtr, Tcl_Obj *objPtr,
			    int charsLeft, int *factorPtr);
static void		RecycleBuffer(ChannelState *statePtr,
			    ChannelBuffer *bufPtr, int mustDiscard);
static int		StackSetBlockMode(Channel *chanPtr, int mode);
static int		SetBlockMode(Tcl_Interp *interp, Channel *chanPtr,
			    int mode);
static void		StopCopy(CopyState *csPtr);
static void		TranslateInputEOL(ChannelState *statePtr, char *dst,
			    const char *src, int *dstLenPtr, int *srcLenPtr);
static void		UpdateInterest(Channel *chanPtr);
static int		Write(Channel *chanPtr, const char *src,
			    int srcLen, Tcl_Encoding encoding);
static Tcl_Obj *	FixLevelCode(Tcl_Obj *msg);
static void		SpliceChannel(Tcl_Channel chan);
static void		CutChannel(Tcl_Channel chan);
static int              WillRead(Channel *chanPtr);

#define WriteChars(chanPtr, src, srcLen) \
			Write(chanPtr, src, srcLen, chanPtr->state->encoding)
#define WriteBytes(chanPtr, src, srcLen) \
			Write(chanPtr, src, srcLen, tclIdentityEncoding)

/*
 * Simplifying helper macros. All may use their argument(s) multiple times.
 * The ANSI C "prototypes" for the macros are listed below, together with a
 * short description of what the macro does.
 *
 * --------------------------------------------------------------------------
 * int BytesLeft(ChannelBuffer *bufPtr)
 *
 *	Returns the number of bytes of data remaining in the buffer.
 *
 * int SpaceLeft(ChannelBuffer *bufPtr)
 *
 *	Returns the number of bytes of space remaining at the end of the
 *	buffer.
 *
 * int IsBufferReady(ChannelBuffer *bufPtr)
 *
 *	Returns whether a buffer has bytes available within it.
 *
 * int IsBufferEmpty(ChannelBuffer *bufPtr)
 *
 *	Returns whether a buffer is entirely empty. Note that this is not the
 *	inverse of the above operation; trying to merge the two seems to lead
 *	to occasional crashes...
 *
 * int IsBufferFull(ChannelBuffer *bufPtr)
 *
 *	Returns whether more data can be added to a buffer.
 *
 * int IsBufferOverflowing(ChannelBuffer *bufPtr)
 *
 *	Returns whether a buffer has more data in it than it should.
 *
 * char *InsertPoint(ChannelBuffer *bufPtr)
 *
 *	Returns a pointer to where characters should be added to the buffer.
 *
 * char *RemovePoint(ChannelBuffer *bufPtr)
 *
 *	Returns a pointer to where characters should be removed from the
 *	buffer.
 * --------------------------------------------------------------------------
 */

#define BytesLeft(bufPtr)	((bufPtr)->nextAdded - (bufPtr)->nextRemoved)

#define SpaceLeft(bufPtr)	((bufPtr)->bufLength - (bufPtr)->nextAdded)

#define IsBufferReady(bufPtr)	((bufPtr)->nextAdded > (bufPtr)->nextRemoved)

#define IsBufferEmpty(bufPtr)	((bufPtr)->nextAdded == (bufPtr)->nextRemoved)

#define IsBufferFull(bufPtr)	((bufPtr) && (bufPtr)->nextAdded >= (bufPtr)->bufLength)

#define IsBufferOverflowing(bufPtr) ((bufPtr)->nextAdded>(bufPtr)->bufLength)

#define InsertPoint(bufPtr)	((bufPtr)->buf + (bufPtr)->nextAdded)

#define RemovePoint(bufPtr)	((bufPtr)->buf + (bufPtr)->nextRemoved)

/*
 * For working with channel state flag bits.
 */

#define SetFlag(statePtr, flag)		((statePtr)->flags |= (flag))
#define ResetFlag(statePtr, flag)	((statePtr)->flags &= ~(flag))
#define GotFlag(statePtr, flag)		((statePtr)->flags & (flag))

/*
 * Macro for testing whether a string (in optionName, length len) matches a
 * value (prefix matching rules). Arguments are the minimum length to match
 * and the value to match against. (Can't use Tcl_GetIndexFromObj as this is
 * used in a situation where no objects are available.)
 */

#define HaveOpt(minLength, nameString) \
	((len > (minLength)) && (optionName[1] == (nameString)[1]) \
		&& (strncmp(optionName, (nameString), len) == 0))

/*
 * The ChannelObjType type.  Used to store the result of looking up
 * a channel name in the context of an interp.  Saves the lookup
 * result and values needed to check its continued validity.
 */

typedef struct ResolvedChanName {
    ChannelState *statePtr;	/* The saved lookup result */
    Tcl_Interp *interp;		/* The interp in which the lookup was done. */
    int epoch;			/* The epoch of the channel when the lookup
				 * was done. Use to verify validity. */
    int refCount;		/* Share this struct among many Tcl_Obj. */
} ResolvedChanName;

static void		DupChannelIntRep(Tcl_Obj *objPtr, Tcl_Obj *copyPtr);
static void		FreeChannelIntRep(Tcl_Obj *objPtr);

static const Tcl_ObjType chanObjType = {
    "channel",			/* name for this type */
    FreeChannelIntRep,		/* freeIntRepProc */
    DupChannelIntRep,		/* dupIntRepProc */
    NULL,			/* updateStringProc */
    NULL			/* setFromAnyProc */
};

#define BUSY_STATE(st, fl) \
     ((((st)->csPtrR) && ((fl) & TCL_READABLE)) || \
      (((st)->csPtrW) && ((fl) & TCL_WRITABLE)))

#define MAX_CHANNEL_BUFFER_SIZE (1024*1024)

/*
 *---------------------------------------------------------------------------
 *
 * ChanClose, ChanRead, ChanSeek, ChanThreadAction, ChanWatch, ChanWrite --
 *
 *	Simplify the access to selected channel driver "methods" that are used
 *	in multiple places in a stereotypical fashion. These are just thin
 *	wrappers around the driver functions.
 *
 *---------------------------------------------------------------------------
 */

static inline int
ChanClose(
    Channel *chanPtr,
    Tcl_Interp *interp)
{
    if (chanPtr->typePtr->closeProc != TCL_CLOSE2PROC) {
	return chanPtr->typePtr->closeProc(chanPtr->instanceData, interp);
    } else {
	return chanPtr->typePtr->close2Proc(chanPtr->instanceData, interp, 0);
    }
}

static inline int
ChanCloseHalf(
    Channel *chanPtr,
    Tcl_Interp *interp,
    int flags)
{
    return chanPtr->typePtr->close2Proc(chanPtr->instanceData, interp, flags);
}

/*
 *---------------------------------------------------------------------------
 *
 * ChanRead --
 *
 *	Read up to dstSize bytes using the inputProc of chanPtr, store
 *	them at dst, and return the number of bytes stored.
 *
 * Results:
 *	The return value of the driver inputProc,
 *	  - number of bytes stored at dst, ot
 *	  - -1 on error, with a Posix error code available to the
 *	    caller by calling Tcl_GetErrno().
 *
 * Side effects:
 *	The CHANNEL_BLOCKED and CHANNEL_EOF flags of the channel state are
 *	set as appropriate.
 *	On EOF, the inputEncodingFlags are set to perform ending operations
 *	on decoding.
 *	TODO - Is this really the right place for that?
 *
 *---------------------------------------------------------------------------
 */
static int
ChanRead(
    Channel *chanPtr,
    char *dst,
    int dstSize)
{
    int bytesRead, result;

    /*
     * If the caller asked for zero bytes, we'd force the inputProc
     * to return zero bytes, and then misinterpret that as EOF.
     */
    assert(dstSize > 0);

    /*
     * Each read op must set the blocked and eof states anew, not let
     * the effect of prior reads leak through.
     */
    if (GotFlag(chanPtr->state, CHANNEL_EOF)) {
        chanPtr->state->inputEncodingFlags |= TCL_ENCODING_START;
    }
    ResetFlag(chanPtr->state, CHANNEL_BLOCKED | CHANNEL_EOF);
    chanPtr->state->inputEncodingFlags &= ~TCL_ENCODING_END;
    if (WillRead(chanPtr) < 0) {
        return -1;
    }

    bytesRead = chanPtr->typePtr->inputProc(chanPtr->instanceData,
	    dst, dstSize, &result);

    /* Stop any flag leakage through stacked channel levels */
    if (GotFlag(chanPtr->state, CHANNEL_EOF)) {
        chanPtr->state->inputEncodingFlags |= TCL_ENCODING_START;
    }
    ResetFlag(chanPtr->state, CHANNEL_BLOCKED | CHANNEL_EOF);
    chanPtr->state->inputEncodingFlags &= ~TCL_ENCODING_END;
    if (bytesRead > 0) {
	/*
	 * If we get a short read, signal up that we may be BLOCKED.
	 * We should avoid calling the driver because on some
	 * platforms we will block in the low level reading code even
	 * though the channel is set into nonblocking mode.
	 */

	if (bytesRead < dstSize) {
	    SetFlag(chanPtr->state, CHANNEL_BLOCKED);
	}
    } else if (bytesRead == 0) {
	SetFlag(chanPtr->state, CHANNEL_EOF);
	chanPtr->state->inputEncodingFlags |= TCL_ENCODING_END;
    } else if (bytesRead < 0) {
	if ((result == EWOULDBLOCK) || (result == EAGAIN)) {
	    SetFlag(chanPtr->state, CHANNEL_BLOCKED);
	    result = EAGAIN;
	}
	Tcl_SetErrno(result);
    }
    return bytesRead;
}

static inline Tcl_WideInt
ChanSeek(
    Channel *chanPtr,
    Tcl_WideInt offset,
    int mode,
    int *errnoPtr)
{
    /*
     * Note that we prefer the wideSeekProc if that field is available in the
     * type and non-NULL.
     */

    if (HaveVersion(chanPtr->typePtr, TCL_CHANNEL_VERSION_3) &&
	    chanPtr->typePtr->wideSeekProc != NULL) {
	return chanPtr->typePtr->wideSeekProc(chanPtr->instanceData,
		offset, mode, errnoPtr);
    }

    if (offset<Tcl_LongAsWide(LONG_MIN) || offset>Tcl_LongAsWide(LONG_MAX)) {
	*errnoPtr = EOVERFLOW;
	return Tcl_LongAsWide(-1);
    }

    return Tcl_LongAsWide(chanPtr->typePtr->seekProc(chanPtr->instanceData,
	    Tcl_WideAsLong(offset), mode, errnoPtr));
}

static inline void
ChanThreadAction(
    Channel *chanPtr,
    int action)
{
    Tcl_DriverThreadActionProc *threadActionProc =
	    Tcl_ChannelThreadActionProc(chanPtr->typePtr);

    if (threadActionProc != NULL) {
	threadActionProc(chanPtr->instanceData, action);
    }
}

static inline void
ChanWatch(
    Channel *chanPtr,
    int mask)
{
    chanPtr->typePtr->watchProc(chanPtr->instanceData, mask);
}

static inline int
ChanWrite(
    Channel *chanPtr,
    const char *src,
    int srcLen,
    int *errnoPtr)
{
    return chanPtr->typePtr->outputProc(chanPtr->instanceData, src, srcLen,
	    errnoPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclInitIOSubsystem --
 *
 *	Initialize all resources used by this subsystem on a per-process
 *	basis.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the memory subsystems.
 *
 *---------------------------------------------------------------------------
 */

void
TclInitIOSubsystem(void)
{
    /*
     * By fetching thread local storage we take care of allocating it for each
     * thread.
     */

    (void) TCL_TSD_INIT(&dataKey);
}

/*
 *-------------------------------------------------------------------------
 *
 * TclFinalizeIOSubsystem --
 *
 *	Releases all resources used by this subsystem on a per-process basis.
 *	Closes all extant channels that have not already been closed because
 *	they were not owned by any interp.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on encoding and memory subsystems.
 *
 *-------------------------------------------------------------------------
 */

	/* ARGSUSED */
void
TclFinalizeIOSubsystem(void)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    Channel *chanPtr = NULL;	/* Iterates over open channels. */
    ChannelState *statePtr;	/* State of channel stack */
    int active = 1;		/* Flag == 1 while there's still work to do */
    int doflushnb;

    /* Fetch the pre-TIP#398 compatibility flag */
    {
        const char *s;
        Tcl_DString ds;

        s = TclGetEnv("TCL_FLUSH_NONBLOCKING_ON_EXIT", &ds);
        doflushnb = ((s != NULL) && strcmp(s, "0"));
        if (s != NULL) {
            Tcl_DStringFree(&ds);
        }
    }

    /*
     * Walk all channel state structures known to this thread and close
     * corresponding channels.
     */

    while (active) {
	/*
	 * Iterate through the open channel list, and find the first channel
	 * that isn't dead. We start from the head of the list each time,
	 * because the close action on one channel can close others.
	 */

	active = 0;
	for (statePtr = tsdPtr->firstCSPtr;
		statePtr != NULL;
		statePtr = statePtr->nextCSPtr) {
	    chanPtr = statePtr->topChanPtr;
            if (GotFlag(statePtr, CHANNEL_DEAD)) {
                continue;
            }
	    if (!GotFlag(statePtr, CHANNEL_INCLOSE | CHANNEL_CLOSED )
                || GotFlag(statePtr, BG_FLUSH_SCHEDULED)) {
                ResetFlag(statePtr, BG_FLUSH_SCHEDULED);
		active = 1;
		break;
	    }
	}

	/*
	 * We've found a live (or bg-closing) channel. Close it.
	 */

	if (active) {

	    TclChannelPreserve((Tcl_Channel)chanPtr);
	    /*
	     * TIP #398:  by default, we no  longer set the  channel back into
             * blocking  mode.  To  restore  the old  blocking  behavior,  the
             * environment variable  TCL_FLUSH_NONBLOCKING_ON_EXIT must be set
             * and not be "0".
	     */
            if (doflushnb) {
                    /* Set the channel back into blocking mode to ensure that we wait
                     * for all data to flush out.
                     */

                (void) Tcl_SetChannelOption(NULL, (Tcl_Channel) chanPtr,
                                            "-blocking", "on");
            }

	    if ((chanPtr == (Channel *) tsdPtr->stdinChannel) ||
		    (chanPtr == (Channel *) tsdPtr->stdoutChannel) ||
		    (chanPtr == (Channel *) tsdPtr->stderrChannel)) {
		/*
		 * Decrement the refcount which was earlier artificially
		 * bumped up to keep the channel from being closed.
		 */

		statePtr->refCount--;
	    }

	    if (statePtr->refCount <= 0) {
		/*
		 * Close it only if the refcount indicates that the channel is
		 * not referenced from any interpreter. If it is, that
		 * interpreter will close the channel when it gets destroyed.
		 */

		(void) Tcl_Close(NULL, (Tcl_Channel) chanPtr);
	    } else {
		/*
		 * The refcount is greater than zero, so flush the channel.
		 */

		Tcl_Flush((Tcl_Channel) chanPtr);

		/*
		 * Call the device driver to actually close the underlying
		 * device for this channel.
		 */

		(void) ChanClose(chanPtr, NULL);

		/*
		 * Finally, we clean up the fields in the channel data
		 * structure since all of them have been deleted already. We
		 * mark the channel with CHANNEL_DEAD to prevent any further
		 * IO operations on it.
		 */

		chanPtr->instanceData = NULL;
		SetFlag(statePtr, CHANNEL_DEAD);
	    }
	    TclChannelRelease((Tcl_Channel)chanPtr);
	}
    }

    TclpFinalizeSockets();
    TclpFinalizePipes();
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetStdChannel --
 *
 *	This function is used to change the channels that are used for
 *	stdin/stdout/stderr in new interpreters.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetStdChannel(
    Tcl_Channel channel,
    int type)			/* One of TCL_STDIN, TCL_STDOUT, TCL_STDERR. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    int init = channel ? 1 : -1;
    switch (type) {
    case TCL_STDIN:
	tsdPtr->stdinInitialized = init;
	tsdPtr->stdinChannel = channel;
	break;
    case TCL_STDOUT:
	tsdPtr->stdoutInitialized = init;
	tsdPtr->stdoutChannel = channel;
	break;
    case TCL_STDERR:
	tsdPtr->stderrInitialized = init;
	tsdPtr->stderrChannel = channel;
	break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetStdChannel --
 *
 *	Returns the specified standard channel.
 *
 * Results:
 *	Returns the specified standard channel, or NULL.
 *
 * Side effects:
 *	May cause the creation of a standard channel and the underlying file.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_GetStdChannel(
    int type)			/* One of TCL_STDIN, TCL_STDOUT, TCL_STDERR. */
{
    Tcl_Channel channel = NULL;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    /*
     * If the channels were not created yet, create them now and store them in
     * the static variables.
     */

    switch (type) {
    case TCL_STDIN:
	if (!tsdPtr->stdinInitialized) {
	    tsdPtr->stdinInitialized = -1;
	    tsdPtr->stdinChannel = TclpGetDefaultStdChannel(TCL_STDIN);

	    /*
	     * Artificially bump the refcount to ensure that the channel is
	     * only closed on exit.
	     *
	     * NOTE: Must only do this if stdinChannel is not NULL. It can be
	     * NULL in situations where Tcl is unable to connect to the
	     * standard input.
	     */

	    if (tsdPtr->stdinChannel != NULL) {
		tsdPtr->stdinInitialized = 1;
		Tcl_RegisterChannel(NULL, tsdPtr->stdinChannel);
	    }
	}
	channel = tsdPtr->stdinChannel;
	break;
    case TCL_STDOUT:
	if (!tsdPtr->stdoutInitialized) {
	    tsdPtr->stdoutInitialized = -1;
	    tsdPtr->stdoutChannel = TclpGetDefaultStdChannel(TCL_STDOUT);
	    if (tsdPtr->stdoutChannel != NULL) {
		tsdPtr->stdoutInitialized = 1;
		Tcl_RegisterChannel(NULL, tsdPtr->stdoutChannel);
	    }
	}
	channel = tsdPtr->stdoutChannel;
	break;
    case TCL_STDERR:
	if (!tsdPtr->stderrInitialized) {
	    tsdPtr->stderrInitialized = -1;
	    tsdPtr->stderrChannel = TclpGetDefaultStdChannel(TCL_STDERR);
	    if (tsdPtr->stderrChannel != NULL) {
		tsdPtr->stderrInitialized = 1;
		Tcl_RegisterChannel(NULL, tsdPtr->stderrChannel);
	    }
	}
	channel = tsdPtr->stderrChannel;
	break;
    }
    return channel;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateCloseHandler
 *
 *	Creates a close callback which will be called when the channel is
 *	closed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Causes the callback to be called in the future when the channel will
 *	be closed.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_CreateCloseHandler(
    Tcl_Channel chan,		/* The channel for which to create the close
				 * callback. */
    Tcl_CloseProc *proc,	/* The callback routine to call when the
				 * channel will be closed. */
    ClientData clientData)	/* Arbitrary data to pass to the close
				 * callback. */
{
    ChannelState *statePtr = ((Channel *) chan)->state;
    CloseCallback *cbPtr;

    cbPtr = ckalloc(sizeof(CloseCallback));
    cbPtr->proc = proc;
    cbPtr->clientData = clientData;

    cbPtr->nextPtr = statePtr->closeCbPtr;
    statePtr->closeCbPtr = cbPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteCloseHandler --
 *
 *	Removes a callback that would have been called on closing the channel.
 *	If there is no matching callback then this function has no effect.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The callback will not be called in the future when the channel is
 *	eventually closed.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteCloseHandler(
    Tcl_Channel chan,		/* The channel for which to cancel the close
				 * callback. */
    Tcl_CloseProc *proc,	/* The procedure for the callback to
				 * remove. */
    ClientData clientData)	/* The callback data for the callback to
				 * remove. */
{
    ChannelState *statePtr = ((Channel *) chan)->state;
    CloseCallback *cbPtr, *cbPrevPtr;

    for (cbPtr = statePtr->closeCbPtr, cbPrevPtr = NULL;
	    cbPtr != NULL; cbPtr = cbPtr->nextPtr) {
	if ((cbPtr->proc == proc) && (cbPtr->clientData == clientData)) {
	    if (cbPrevPtr == NULL) {
		statePtr->closeCbPtr = cbPtr->nextPtr;
	    } else {
		cbPrevPtr->nextPtr = cbPtr->nextPtr;
	    }
	    ckfree(cbPtr);
	    break;
	}
	cbPrevPtr = cbPtr;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetChannelTable --
 *
 *	Gets and potentially initializes the channel table for an interpreter.
 *	If it is initializing the table it also inserts channels for stdin,
 *	stdout and stderr if the interpreter is trusted.
 *
 * Results:
 *	A pointer to the hash table created, for use by the caller.
 *
 * Side effects:
 *	Initializes the channel table for an interpreter. May create channels
 *	for stdin, stdout and stderr.
 *
 *----------------------------------------------------------------------
 */

static Tcl_HashTable *
GetChannelTable(
    Tcl_Interp *interp)
{
    Tcl_HashTable *hTblPtr;	/* Hash table of channels. */
    Tcl_Channel stdinChan, stdoutChan, stderrChan;

    hTblPtr = Tcl_GetAssocData(interp, "tclIO", NULL);
    if (hTblPtr == NULL) {
	hTblPtr = ckalloc(sizeof(Tcl_HashTable));
	Tcl_InitHashTable(hTblPtr, TCL_STRING_KEYS);
	Tcl_SetAssocData(interp, "tclIO",
		(Tcl_InterpDeleteProc *) DeleteChannelTable, hTblPtr);

	/*
	 * If the interpreter is trusted (not "safe"), insert channels for
	 * stdin, stdout and stderr (possibly creating them in the process).
	 */

	if (Tcl_IsSafe(interp) == 0) {
	    stdinChan = Tcl_GetStdChannel(TCL_STDIN);
	    if (stdinChan != NULL) {
		Tcl_RegisterChannel(interp, stdinChan);
	    }
	    stdoutChan = Tcl_GetStdChannel(TCL_STDOUT);
	    if (stdoutChan != NULL) {
		Tcl_RegisterChannel(interp, stdoutChan);
	    }
	    stderrChan = Tcl_GetStdChannel(TCL_STDERR);
	    if (stderrChan != NULL) {
		Tcl_RegisterChannel(interp, stderrChan);
	    }
	}
    }
    return hTblPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteChannelTable --
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
DeleteChannelTable(
    ClientData clientData,	/* The per-interpreter data structure. */
    Tcl_Interp *interp)		/* The interpreter being deleted. */
{
    Tcl_HashTable *hTblPtr;	/* The hash table. */
    Tcl_HashSearch hSearch;	/* Search variable. */
    Tcl_HashEntry *hPtr;	/* Search variable. */
    Channel *chanPtr;		/* Channel being deleted. */
    ChannelState *statePtr;	/* State of Channel being deleted. */
    EventScriptRecord *sPtr, *prevPtr, *nextPtr;
				/* Variables to loop over all channel events
				 * registered, to delete the ones that refer
				 * to the interpreter being deleted. */

    /*
     * Delete all the registered channels - this will close channels whose
     * refcount reaches zero.
     */

    hTblPtr = clientData;
    for (hPtr = Tcl_FirstHashEntry(hTblPtr, &hSearch); hPtr != NULL;
	    hPtr = Tcl_FirstHashEntry(hTblPtr, &hSearch)) {
	chanPtr = Tcl_GetHashValue(hPtr);
	statePtr = chanPtr->state;

	/*
	 * Remove any fileevents registered in this interpreter.
	 */

	for (sPtr = statePtr->scriptRecordPtr, prevPtr = NULL;
		sPtr != NULL; sPtr = nextPtr) {
	    nextPtr = sPtr->nextPtr;
	    if (sPtr->interp == interp) {
		if (prevPtr == NULL) {
		    statePtr->scriptRecordPtr = nextPtr;
		} else {
		    prevPtr->nextPtr = nextPtr;
		}

		Tcl_DeleteChannelHandler((Tcl_Channel) chanPtr,
			TclChannelEventScriptInvoker, sPtr);

		TclDecrRefCount(sPtr->scriptPtr);
		ckfree(sPtr);
	    } else {
		prevPtr = sPtr;
	    }
	}

	/*
	 * Cannot call Tcl_UnregisterChannel because that procedure calls
	 * Tcl_GetAssocData to get the channel table, which might already be
	 * inaccessible from the interpreter structure. Instead, we emulate
	 * the behavior of Tcl_UnregisterChannel directly here.
	 */

	Tcl_DeleteHashEntry(hPtr);
	statePtr->epoch++;
	if (statePtr->refCount-- <= 1) {
	    if (!GotFlag(statePtr, BG_FLUSH_SCHEDULED)) {
		(void) Tcl_Close(interp, (Tcl_Channel) chanPtr);
	    }
	}

    }
    Tcl_DeleteHashTable(hTblPtr);
    ckfree(hTblPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * CheckForStdChannelsBeingClosed --
 *
 *	Perform special handling for standard channels being closed. When
 *	given a standard channel, if the refcount is now 1, it means that the
 *	last reference to the standard channel is being explicitly closed. Now
 *	bump the refcount artificially down to 0, to ensure the normal
 *	handling of channels being closed will occur. Also reset the static
 *	pointer to the channel to NULL, to avoid dangling references.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Manipulates the refcount on standard channels. May smash the global
 *	static pointer to a standard channel.
 *
 *----------------------------------------------------------------------
 */

static void
CheckForStdChannelsBeingClosed(
    Tcl_Channel chan)
{
    ChannelState *statePtr = ((Channel *) chan)->state;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (tsdPtr->stdinInitialized == 1
	    && tsdPtr->stdinChannel != NULL
	    && statePtr == ((Channel *)tsdPtr->stdinChannel)->state) {
	if (statePtr->refCount < 2) {
	    statePtr->refCount = 0;
	    tsdPtr->stdinChannel = NULL;
	    return;
	}
    } else if (tsdPtr->stdoutInitialized == 1
	    && tsdPtr->stdoutChannel != NULL
	    && statePtr == ((Channel *)tsdPtr->stdoutChannel)->state) {
	if (statePtr->refCount < 2) {
	    statePtr->refCount = 0;
	    tsdPtr->stdoutChannel = NULL;
	    return;
	}
    } else if (tsdPtr->stderrInitialized == 1
	    && tsdPtr->stderrChannel != NULL
	    && statePtr == ((Channel *)tsdPtr->stderrChannel)->state) {
	if (statePtr->refCount < 2) {
	    statePtr->refCount = 0;
	    tsdPtr->stderrChannel = NULL;
	    return;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_IsStandardChannel --
 *
 *	Test if the given channel is a standard channel. No attempt is made to
 *	check if the channel or the standard channels are initialized or
 *	otherwise valid.
 *
 * Results:
 *	Returns 1 if true, 0 if false.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_IsStandardChannel(
    Tcl_Channel chan)		/* Channel to check. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if ((chan == tsdPtr->stdinChannel)
	    || (chan == tsdPtr->stdoutChannel)
	    || (chan == tsdPtr->stderrChannel)) {
	return 1;
    } else {
	return 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RegisterChannel --
 *
 *	Adds an already-open channel to the channel table of an interpreter.
 *	If the interpreter passed as argument is NULL, it only increments the
 *	channel refCount.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May increment the reference count of a channel.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_RegisterChannel(
    Tcl_Interp *interp,		/* Interpreter in which to add the channel. */
    Tcl_Channel chan)		/* The channel to add to this interpreter
				 * channel table. */
{
    Tcl_HashTable *hTblPtr;	/* Hash table of channels. */
    Tcl_HashEntry *hPtr;	/* Search variable. */
    int isNew;			/* Is the hash entry new or does it exist? */
    Channel *chanPtr;		/* The actual channel. */
    ChannelState *statePtr;	/* State of the actual channel. */

    /*
     * Always (un)register bottom-most channel in the stack. This makes
     * management of the channel list easier because no manipulation is
     * necessary during (un)stack operation.
     */

    chanPtr = ((Channel *) chan)->state->bottomChanPtr;
    statePtr = chanPtr->state;

    if (statePtr->channelName == NULL) {
	Tcl_Panic("Tcl_RegisterChannel: channel without name");
    }
    if (interp != NULL) {
	hTblPtr = GetChannelTable(interp);
	hPtr = Tcl_CreateHashEntry(hTblPtr, statePtr->channelName, &isNew);
	if (!isNew) {
	    if (chan == Tcl_GetHashValue(hPtr)) {
		return;
	    }

	    Tcl_Panic("Tcl_RegisterChannel: duplicate channel names");
	}
	Tcl_SetHashValue(hPtr, chanPtr);
    }
    statePtr->refCount++;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UnregisterChannel --
 *
 *	Deletes the hash entry for a channel associated with an interpreter.
 *	If the interpreter given as argument is NULL, it only decrements the
 *	reference count. (This all happens in the Tcl_DetachChannel helper
 *	function).
 *
 *	Finally, if the reference count of the channel drops to zero, it is
 *	deleted.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Calls Tcl_DetachChannel which deletes the hash entry for a channel
 *	associated with an interpreter.
 *
 *	May delete the channel, which can have a variety of consequences,
 *	especially if we are forced to close the channel.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UnregisterChannel(
    Tcl_Interp *interp,		/* Interpreter in which channel is defined. */
    Tcl_Channel chan)		/* Channel to delete. */
{
    ChannelState *statePtr;	/* State of the real channel. */

    statePtr = ((Channel *) chan)->state->bottomChanPtr->state;

    if (GotFlag(statePtr, CHANNEL_INCLOSE)) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
                    "illegal recursive call to close through close-handler"
                    " of channel", -1));
	}
	return TCL_ERROR;
    }

    if (DetachChannel(interp, chan) != TCL_OK) {
	return TCL_OK;
    }

    statePtr = ((Channel *) chan)->state->bottomChanPtr->state;

    /*
     * Perform special handling for standard channels being closed. If the
     * refCount is now 1 it means that the last reference to the standard
     * channel is being explicitly closed, so bump the refCount down
     * artificially to 0. This will ensure that the channel is actually
     * closed, below. Also set the static pointer to NULL for the channel.
     */

    CheckForStdChannelsBeingClosed(chan);

    /*
     * If the refCount reached zero, close the actual channel.
     */

    if (statePtr->refCount <= 0) {
	Tcl_Preserve(statePtr);
	if (!GotFlag(statePtr, BG_FLUSH_SCHEDULED)) {
	    /*
	     * We don't want to re-enter Tcl_Close().
	     */

	    if (!GotFlag(statePtr, CHANNEL_CLOSED)) {
		if (Tcl_Close(interp, chan) != TCL_OK) {
		    SetFlag(statePtr, CHANNEL_CLOSED);
		    Tcl_Release(statePtr);
		    return TCL_ERROR;
		}
	    }
	}
	SetFlag(statePtr, CHANNEL_CLOSED);
	Tcl_Release(statePtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DetachChannel --
 *
 *	Deletes the hash entry for a channel associated with an interpreter.
 *	If the interpreter given as argument is NULL, it only decrements the
 *	reference count. Even if the ref count drops to zero, the channel is
 *	NOT closed or cleaned up. This allows a channel to be detached from an
 *	interpreter and left in the same state it was in when it was
 *	originally returned by 'Tcl_OpenFileChannel', for example.
 *
 *	This function cannot be used on the standard channels, and will return
 *	TCL_ERROR if that is attempted.
 *
 *	This function should only be necessary for special purposes in which
 *	you need to generate a pristine channel from one that has already been
 *	used. All ordinary purposes will almost always want to use
 *	Tcl_UnregisterChannel instead.
 *
 *	Provided the channel is not attached to any other interpreter, it can
 *	then be closed with Tcl_Close, rather than with Tcl_UnregisterChannel.
 *
 * Results:
 *	A standard Tcl result. If the channel is not currently registered with
 *	the given interpreter, TCL_ERROR is returned, otherwise TCL_OK.
 *	However no error messages are left in the interp's result.
 *
 * Side effects:
 *	Deletes the hash entry for a channel associated with an interpreter.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_DetachChannel(
    Tcl_Interp *interp,		/* Interpreter in which channel is defined. */
    Tcl_Channel chan)		/* Channel to delete. */
{
    if (Tcl_IsStandardChannel(chan)) {
	return TCL_ERROR;
    }

    return DetachChannel(interp, chan);
}

/*
 *----------------------------------------------------------------------
 *
 * DetachChannel --
 *
 *	Deletes the hash entry for a channel associated with an interpreter.
 *	If the interpreter given as argument is NULL, it only decrements the
 *	reference count. Even if the ref count drops to zero, the channel is
 *	NOT closed or cleaned up. This allows a channel to be detached from an
 *	interpreter and left in the same state it was in when it was
 *	originally returned by 'Tcl_OpenFileChannel', for example.
 *
 * Results:
 *	A standard Tcl result. If the channel is not currently registered with
 *	the given interpreter, TCL_ERROR is returned, otherwise TCL_OK.
 *	However no error messages are left in the interp's result.
 *
 * Side effects:
 *	Deletes the hash entry for a channel associated with an interpreter.
 *
 *----------------------------------------------------------------------
 */

static int
DetachChannel(
    Tcl_Interp *interp,		/* Interpreter in which channel is defined. */
    Tcl_Channel chan)		/* Channel to delete. */
{
    Tcl_HashTable *hTblPtr;	/* Hash table of channels. */
    Tcl_HashEntry *hPtr;	/* Search variable. */
    Channel *chanPtr;		/* The real IO channel. */
    ChannelState *statePtr;	/* State of the real channel. */

    /*
     * Always (un)register bottom-most channel in the stack. This makes
     * management of the channel list easier because no manipulation is
     * necessary during (un)stack operation.
     */

    chanPtr = ((Channel *) chan)->state->bottomChanPtr;
    statePtr = chanPtr->state;

    if (interp != NULL) {
	hTblPtr = Tcl_GetAssocData(interp, "tclIO", NULL);
	if (hTblPtr == NULL) {
	    return TCL_ERROR;
	}
	hPtr = Tcl_FindHashEntry(hTblPtr, statePtr->channelName);
	if (hPtr == NULL) {
	    return TCL_ERROR;
	}
	if ((Channel *) Tcl_GetHashValue(hPtr) != chanPtr) {
	    return TCL_ERROR;
	}
	Tcl_DeleteHashEntry(hPtr);
	statePtr->epoch++;

	/*
	 * Remove channel handlers that refer to this interpreter, so that
	 * they will not be present if the actual close is delayed and more
	 * events happen on the channel. This may occur if the channel is
	 * shared between several interpreters, or if the channel has async
	 * flushing active.
	 */

	CleanupChannelHandlers(interp, chanPtr);
    }

    statePtr->refCount--;

    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_GetChannel --
 *
 *	Finds an existing Tcl_Channel structure by name in a given
 *	interpreter. This function is public because it is used by
 *	channel-type-specific functions.
 *
 * Results:
 *	A Tcl_Channel or NULL on failure. If failed, interp's result object
 *	contains an error message. *modePtr is filled with the modes in which
 *	the channel was opened.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Channel
Tcl_GetChannel(
    Tcl_Interp *interp,		/* Interpreter in which to find or create the
				 * channel. */
    const char *chanName,	/* The name of the channel. */
    int *modePtr)		/* Where to store the mode in which the
				 * channel was opened? Will contain an ORed
				 * combination of TCL_READABLE and
				 * TCL_WRITABLE, if non-NULL. */
{
    Channel *chanPtr;		/* The actual channel. */
    Tcl_HashTable *hTblPtr;	/* Hash table of channels. */
    Tcl_HashEntry *hPtr;	/* Search variable. */
    const char *name;		/* Translated name. */

    /*
     * Substitute "stdin", etc. Note that even though we immediately find the
     * channel using Tcl_GetStdChannel, we still need to look it up in the
     * specified interpreter to ensure that it is present in the channel
     * table. Otherwise, safe interpreters would always have access to the
     * standard channels.
     */

    name = chanName;
    if ((chanName[0] == 's') && (chanName[1] == 't')) {
	chanPtr = NULL;
	if (strcmp(chanName, "stdin") == 0) {
	    chanPtr = (Channel *) Tcl_GetStdChannel(TCL_STDIN);
	} else if (strcmp(chanName, "stdout") == 0) {
	    chanPtr = (Channel *) Tcl_GetStdChannel(TCL_STDOUT);
	} else if (strcmp(chanName, "stderr") == 0) {
	    chanPtr = (Channel *) Tcl_GetStdChannel(TCL_STDERR);
	}
	if (chanPtr != NULL) {
	    name = chanPtr->state->channelName;
	}
    }

    hTblPtr = GetChannelTable(interp);
    hPtr = Tcl_FindHashEntry(hTblPtr, name);
    if (hPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "can not find channel named \"%s\"", chanName));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "CHANNEL", chanName, NULL);
	return NULL;
    }

    /*
     * Always return bottom-most channel in the stack. This one lives the
     * longest - other channels may go away unnoticed. The other APIs
     * compensate where necessary to retrieve the topmost channel again.
     */

    chanPtr = Tcl_GetHashValue(hPtr);
    chanPtr = chanPtr->state->bottomChanPtr;
    if (modePtr != NULL) {
	*modePtr = chanPtr->state->flags & (TCL_READABLE|TCL_WRITABLE);
    }

    return (Tcl_Channel) chanPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclGetChannelFromObj --
 *
 *	Finds an existing Tcl_Channel structure by name in a given
 *	interpreter. This function is public because it is used by
 *	channel-type-specific functions.
 *
 * Results:
 *	A Tcl_Channel or NULL on failure. If failed, interp's result object
 *	contains an error message. *modePtr is filled with the modes in which
 *	the channel was opened.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TclGetChannelFromObj(
    Tcl_Interp *interp,		/* Interpreter in which to find or create the
				 * channel. */
    Tcl_Obj *objPtr,
    Tcl_Channel *channelPtr,
    int *modePtr,		/* Where to store the mode in which the
				 * channel was opened? Will contain an ORed
				 * combination of TCL_READABLE and
				 * TCL_WRITABLE, if non-NULL. */
    int flags)
{
    ChannelState *statePtr;
    ResolvedChanName *resPtr = NULL;
    Tcl_Channel chan;

    if (interp == NULL) {
	return TCL_ERROR;
    }

    if (objPtr->typePtr == &chanObjType) {
	/*
 	 * Confirm validity of saved lookup results.
 	 */

	resPtr = (ResolvedChanName *) objPtr->internalRep.twoPtrValue.ptr1;
	statePtr = resPtr->statePtr;
	if ((resPtr->interp == interp)		/* Same interp context */
			/* No epoch change in channel since lookup */
		&& (resPtr->epoch == statePtr->epoch)) {

	    /* Have a valid saved lookup. Jump to end to return it. */
	    goto valid;
	}
    }

    chan = Tcl_GetChannel(interp, TclGetString(objPtr), NULL);

    if (chan == NULL) {
	if (resPtr) {
	    FreeChannelIntRep(objPtr);
	}
	return TCL_ERROR;
    }

    if (resPtr && resPtr->refCount == 1) {
	/* Re-use the ResolvedCmdName struct */
	Tcl_Release((ClientData) resPtr->statePtr);

    } else {
	TclFreeIntRep(objPtr);

	resPtr = (ResolvedChanName *) ckalloc(sizeof(ResolvedChanName));
	resPtr->refCount = 1;
	objPtr->internalRep.twoPtrValue.ptr1 = (ClientData) resPtr;
	objPtr->typePtr = &chanObjType;
    }
    statePtr = ((Channel *)chan)->state;
    resPtr->statePtr = statePtr;
    Tcl_Preserve((ClientData) statePtr);
    resPtr->interp = interp;
    resPtr->epoch = statePtr->epoch;

  valid:
    *channelPtr = (Tcl_Channel) statePtr->bottomChanPtr;

    if (modePtr != NULL) {
	*modePtr = statePtr->flags & (TCL_READABLE|TCL_WRITABLE);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateChannel --
 *
 *	Creates a new entry in the hash table for a Tcl_Channel record.
 *
 * Results:
 *	Returns the new Tcl_Channel.
 *
 * Side effects:
 *	Creates a new Tcl_Channel instance and inserts it into the hash table.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_CreateChannel(
    const Tcl_ChannelType *typePtr, /* The channel type record. */
    const char *chanName,	/* Name of channel to record. */
    ClientData instanceData,	/* Instance specific data. */
    int mask)			/* TCL_READABLE & TCL_WRITABLE to indicate if
				 * the channel is readable, writable. */
{
    Channel *chanPtr;		/* The channel structure newly created. */
    ChannelState *statePtr;	/* The stack-level independent state info for
				 * the channel. */
    const char *name;
    char *tmp;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    /*
     * With the change of the Tcl_ChannelType structure to use a version in
     * 8.3.2+, we have to make sure that our assumption that the structure
     * remains a binary compatible size is true.
     *
     * If this assertion fails on some system, then it can be removed only if
     * the user recompiles code with older channel drivers in the new system
     * as well.
     */

    assert(sizeof(Tcl_ChannelTypeVersion) == sizeof(Tcl_DriverBlockModeProc *));
    assert(typePtr->typeName != NULL);
    if (NULL == typePtr->closeProc) {
	Tcl_Panic("channel type %s must define closeProc", typePtr->typeName);
    }
    if ((TCL_READABLE & mask) && (NULL == typePtr->inputProc)) {
	Tcl_Panic("channel type %s must define inputProc when used for reader channel", typePtr->typeName);
    }
    if ((TCL_WRITABLE & mask) &&  (NULL == typePtr->outputProc)) {
	Tcl_Panic("channel type %s must define outputProc when used for writer channel", typePtr->typeName);
    }
    if (NULL == typePtr->watchProc) {
	Tcl_Panic("channel type %s must define watchProc", typePtr->typeName);
    }
    if ((NULL!=typePtr->wideSeekProc) && (NULL == typePtr->seekProc)) {
	Tcl_Panic("channel type %s must define seekProc if defining wideSeekProc", typePtr->typeName);
    }

    /*
     * JH: We could subsequently memset these to 0 to avoid the numerous
     * assignments to 0/NULL below.
     */

    chanPtr = ckalloc(sizeof(Channel));
    statePtr = ckalloc(sizeof(ChannelState));
    chanPtr->state = statePtr;

    chanPtr->instanceData = instanceData;
    chanPtr->typePtr = typePtr;

    /*
     * Set all the bits that are part of the stack-independent state
     * information for the channel.
     */

    if (chanName != NULL) {
	unsigned len = strlen(chanName) + 1;

	/*
         * Make sure we allocate at least 7 bytes, so it fits for "stdout"
         * later.
         */

	tmp = ckalloc((len < 7) ? 7 : len);
	strcpy(tmp, chanName);
    } else {
	tmp = ckalloc(7);
	tmp[0] = '\0';
    }
    statePtr->channelName = tmp;
    statePtr->flags = mask;

    /*
     * Set the channel to system default encoding.
     *
     * Note the strange bit of protection taking place here. If the system
     * encoding name is reported back as "binary", something weird is
     * happening. Tcl provides no "binary" encoding, so someone else has
     * provided one. We ignore it so as not to interfere with the "magic"
     * interpretation that Tcl_Channels give to the "-encoding binary" option.
     */

    statePtr->encoding = NULL;
    name = Tcl_GetEncodingName(NULL);
    if (strcmp(name, "binary") != 0) {
	statePtr->encoding = Tcl_GetEncoding(NULL, name);
    }
    statePtr->inputEncodingState  = NULL;
    statePtr->inputEncodingFlags  = TCL_ENCODING_START;
    statePtr->outputEncodingState = NULL;
    statePtr->outputEncodingFlags = TCL_ENCODING_START;

    /*
     * Set the channel up initially in AUTO input translation mode to accept
     * "\n", "\r" and "\r\n". Output translation mode is set to a platform
     * specific default value. The eofChar is set to 0 for both input and
     * output, so that Tcl does not look for an in-file EOF indicator (e.g.
     * ^Z) and does not append an EOF indicator to files.
     */

    statePtr->inputTranslation	= TCL_TRANSLATE_AUTO;
    statePtr->outputTranslation	= TCL_PLATFORM_TRANSLATION;
    statePtr->inEofChar		= 0;
    statePtr->outEofChar	= 0;

    statePtr->unreportedError	= 0;
    statePtr->refCount		= 0;
    statePtr->closeCbPtr	= NULL;
    statePtr->curOutPtr		= NULL;
    statePtr->outQueueHead	= NULL;
    statePtr->outQueueTail	= NULL;
    statePtr->saveInBufPtr	= NULL;
    statePtr->inQueueHead	= NULL;
    statePtr->inQueueTail	= NULL;
    statePtr->chPtr		= NULL;
    statePtr->interestMask	= 0;
    statePtr->scriptRecordPtr	= NULL;
    statePtr->bufSize		= CHANNELBUFFER_DEFAULT_SIZE;
    statePtr->timer		= NULL;
    statePtr->csPtrR		= NULL;
    statePtr->csPtrW		= NULL;
    statePtr->outputStage	= NULL;

    /*
     * As we are creating the channel, it is obviously the top for now.
     */

    statePtr->topChanPtr	= chanPtr;
    statePtr->bottomChanPtr	= chanPtr;
    chanPtr->downChanPtr	= NULL;
    chanPtr->upChanPtr		= NULL;
    chanPtr->inQueueHead	= NULL;
    chanPtr->inQueueTail	= NULL;
    chanPtr->refCount		= 0;

    /*
     * TIP #219, Tcl Channel Reflection API
     */

    statePtr->chanMsg		= NULL;
    statePtr->unreportedMsg	= NULL;

    statePtr->epoch		= 0;

    /*
     * Link the channel into the list of all channels; create an on-exit
     * handler if there is not one already, to close off all the channels in
     * the list on exit.
     *
     * JH: Could call Tcl_SpliceChannel, but need to avoid NULL check.
     *
     * TIP #218.
     * AK: Just initialize the field to NULL before invoking Tcl_SpliceChannel
     *	   We need Tcl_SpliceChannel, for the threadAction calls. There is no
     *	   real reason to duplicate all of this.
     * NOTE: All drivers using thread actions now have to perform their TSD
     *	     manipulation only in their thread action proc. Doing it when
     *	     creating their instance structures will collide with the thread
     *	     action activity and lead to damaged lists.
     */

    statePtr->nextCSPtr = NULL;
    SpliceChannel((Tcl_Channel) chanPtr);

    /*
     * Install this channel in the first empty standard channel slot, if the
     * channel was previously closed explicitly.
     */

    if ((tsdPtr->stdinChannel == NULL) && (tsdPtr->stdinInitialized == 1)) {
	strcpy(tmp, "stdin");
	Tcl_SetStdChannel((Tcl_Channel) chanPtr, TCL_STDIN);
	Tcl_RegisterChannel(NULL, (Tcl_Channel) chanPtr);
    } else if ((tsdPtr->stdoutChannel == NULL) &&
	    (tsdPtr->stdoutInitialized == 1)) {
	strcpy(tmp, "stdout");
	Tcl_SetStdChannel((Tcl_Channel) chanPtr, TCL_STDOUT);
	Tcl_RegisterChannel(NULL, (Tcl_Channel) chanPtr);
    } else if ((tsdPtr->stderrChannel == NULL) &&
	    (tsdPtr->stderrInitialized == 1)) {
	strcpy(tmp, "stderr");
	Tcl_SetStdChannel((Tcl_Channel) chanPtr, TCL_STDERR);
	Tcl_RegisterChannel(NULL, (Tcl_Channel) chanPtr);
    }
    return (Tcl_Channel) chanPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_StackChannel --
 *
 *	Replaces an entry in the hash table for a Tcl_Channel record. The
 *	replacement is a new channel with same name, it supercedes the
 *	replaced channel. Input and output of the superceded channel is now
 *	going through the newly created channel and allows the arbitrary
 *	filtering/manipulation of the dataflow.
 *
 *	Andreas Kupries <a.kupries@westend.com>, 12/13/1998 "Trf-Patch for
 *	filtering channels"
 *
 * Results:
 *	Returns the new Tcl_Channel, which actually contains the saved
 *	information about prevChan.
 *
 * Side effects:
 *	A new channel structure is allocated and linked below the existing
 *	channel. The channel operations and client data of the existing
 *	channel are copied down to the newly created channel, and the current
 *	channel has its operations replaced by the new typePtr.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_StackChannel(
    Tcl_Interp *interp,		/* The interpreter we are working in */
    const Tcl_ChannelType *typePtr,
				/* The channel type record for the new
				 * channel. */
    ClientData instanceData,	/* Instance specific data for the new
				 * channel. */
    int mask,			/* TCL_READABLE & TCL_WRITABLE to indicate if
				 * the channel is readable, writable. */
    Tcl_Channel prevChan)	/* The channel structure to replace */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    Channel *chanPtr, *prevChanPtr;
    ChannelState *statePtr;

    /*
     * Find the given channel (prevChan) in the list of all channels. If we do
     * not find it, then it was never registered correctly.
     *
     * This operation should occur at the top of a channel stack.
     */

    statePtr = (ChannelState *) tsdPtr->firstCSPtr;
    prevChanPtr = ((Channel *) prevChan)->state->topChanPtr;

    while ((statePtr != NULL) && (statePtr->topChanPtr != prevChanPtr)) {
	statePtr = statePtr->nextCSPtr;
    }

    if (statePtr == NULL) {
	if (interp) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                    "couldn't find state for channel \"%s\"",
		    Tcl_GetChannelName(prevChan)));
	}
	return NULL;
    }

    /*
     * Here we check if the given "mask" matches the "flags" of the already
     * existing channel.
     *
     *	  | - | R | W | RW |
     *	--+---+---+---+----+	<=>  0 != (chan->mask & prevChan->mask)
     *	- |   |   |   |    |
     *	R |   | + |   | +  |	The superceding channel is allowed to restrict
     *	W |   |   | + | +  |	the capabilities of the superceded one!
     *	RW|   | + | + | +  |
     *	--+---+---+---+----+
     */

    if ((mask & (statePtr->flags & (TCL_READABLE | TCL_WRITABLE))) == 0) {
	if (interp) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "reading and writing both disallowed for channel \"%s\"",
		    Tcl_GetChannelName(prevChan)));
	}
	return NULL;
    }

    /*
     * Flush the buffers. This ensures that any data still in them at this
     * time is not handled by the new transformation. Restrict this to
     * writable channels. Take care to hide a possible bg-copy in progress
     * from Tcl_Flush and the CheckForChannelErrors inside.
     */

    if ((mask & TCL_WRITABLE) != 0) {
	CopyState *csPtrR = statePtr->csPtrR;
	CopyState *csPtrW = statePtr->csPtrW;

	statePtr->csPtrR = NULL;
	statePtr->csPtrW = NULL;

	/*
	 * TODO: Examine what can go wrong if Tcl_Flush() call disturbs
	 * the stacking state of this channel during its operations.
	 */
	if (Tcl_Flush((Tcl_Channel) prevChanPtr) != TCL_OK) {
	    statePtr->csPtrR = csPtrR;
	    statePtr->csPtrW = csPtrW;
	    if (interp) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                        "could not flush channel \"%s\"",
			Tcl_GetChannelName(prevChan)));
	    }
	    return NULL;
	}

	statePtr->csPtrR = csPtrR;
	statePtr->csPtrW = csPtrW;
    }

    /*
     * Discard any input in the buffers. They are not yet read by the user of
     * the channel, so they have to go through the new transformation before
     * reading. As the buffers contain the untransformed form their contents
     * are not only useless but actually distorts our view of the system.
     *
     * To preserve the information without having to read them again and to
     * avoid problems with the location in the channel (seeking might be
     * impossible) we move the buffers from the common state structure into
     * the channel itself. We use the buffers in the channel below the new
     * transformation to hold the data. In the future this allows us to write
     * transformations which pre-read data and push the unused part back when
     * they are going away.
     */

    if (((mask & TCL_READABLE) != 0) && (statePtr->inQueueHead != NULL)) {

	/*
	 * When statePtr->inQueueHead is not NULL, we know
	 * prevChanPtr->inQueueHead must be NULL.
	 */

	assert(prevChanPtr->inQueueHead == NULL);
	assert(prevChanPtr->inQueueTail == NULL);

	prevChanPtr->inQueueHead = statePtr->inQueueHead;
	prevChanPtr->inQueueTail = statePtr->inQueueTail;

	statePtr->inQueueHead = NULL;
	statePtr->inQueueTail = NULL;
    }

    chanPtr = ckalloc(sizeof(Channel));

    /*
     * Save some of the current state into the new structure, reinitialize the
     * parts which will stay with the transformation.
     *
     * Remarks:
     */

    chanPtr->state		= statePtr;
    chanPtr->instanceData	= instanceData;
    chanPtr->typePtr		= typePtr;
    chanPtr->downChanPtr	= prevChanPtr;
    chanPtr->upChanPtr		= NULL;
    chanPtr->inQueueHead	= NULL;
    chanPtr->inQueueTail	= NULL;
    chanPtr->refCount		= 0;

    /*
     * Place new block at the head of a possibly existing list of previously
     * stacked channels.
     */

    prevChanPtr->upChanPtr	= chanPtr;
    statePtr->topChanPtr	= chanPtr;

    /*
     * TIP #218, Channel Thread Actions.
     *
     * We call the thread actions for the new channel directly. We _cannot_
     * use SpliceChannel, because the (thread-)global list of all channels
     * always contains the _ChannelState_ for a stack of channels, not the
     * individual channels. And SpliceChannel would not only call the thread
     * actions, but also add the shared ChannelState to this list a second
     * time, mangling it.
     */

    ChanThreadAction(chanPtr, TCL_CHANNEL_THREAD_INSERT);

    return (Tcl_Channel) chanPtr;
}

void
TclChannelPreserve(
    Tcl_Channel chan)
{
    ((Channel *)chan)->refCount++;
}

void
TclChannelRelease(
    Tcl_Channel chan)
{
    Channel *chanPtr = (Channel *) chan;

    if (chanPtr->refCount == 0) {
	Tcl_Panic("Channel released more than preserved");
    }
    if (--chanPtr->refCount) {
	return;
    }
    if (chanPtr->typePtr == NULL) {
	ckfree(chanPtr);
    }
}

static void
ChannelFree(
    Channel *chanPtr)
{
    if (chanPtr->refCount == 0) {
	ckfree(chanPtr);
	return;
    }
    chanPtr->typePtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UnstackChannel --
 *
 *	Unstacks an entry in the hash table for a Tcl_Channel record. This is
 *	the reverse to 'Tcl_StackChannel'.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	If TCL_ERROR is returned, the posix error code will be set with
 *	Tcl_SetErrno. May leave a message in interp result as well.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UnstackChannel(
    Tcl_Interp *interp,		/* The interpreter we are working in */
    Tcl_Channel chan)		/* The channel to unstack */
{
    Channel *chanPtr = (Channel *) chan;
    ChannelState *statePtr = chanPtr->state;
    int result = 0;

    /*
     * This operation should occur at the top of a channel stack.
     */

    chanPtr = statePtr->topChanPtr;

    if (chanPtr->downChanPtr != NULL) {
	/*
	 * Instead of manipulating the per-thread / per-interp list/hashtable
	 * of registered channels we wind down the state of the
	 * transformation, and then restore the state of underlying channel
	 * into the old structure.
	 */

	/*
	 * TODO: Figure out how to handle the situation where the chan
	 * operations called below by this unstacking operation cause
	 * another unstacking recursively.  In that case the downChanPtr
	 * value we're holding on to will not be the right thing.
	 */

	Channel *downChanPtr = chanPtr->downChanPtr;

	/*
	 * Flush the buffers. This ensures that any data still in them at this
	 * time _is_ handled by the transformation we are unstacking right
	 * now. Restrict this to writable channels. Take care to hide a
	 * possible bg-copy in progress from Tcl_Flush and the
	 * CheckForChannelErrors inside.
	 */

	if (GotFlag(statePtr, TCL_WRITABLE)) {
	    CopyState *csPtrR = statePtr->csPtrR;
	    CopyState *csPtrW = statePtr->csPtrW;

	    statePtr->csPtrR = NULL;
	    statePtr->csPtrW = NULL;

	    if (Tcl_Flush((Tcl_Channel) chanPtr) != TCL_OK) {
		statePtr->csPtrR = csPtrR;
		statePtr->csPtrW = csPtrW;

		/*
		 * TIP #219, Tcl Channel Reflection API.
		 * Move error messages put by the driver into the chan/ip
		 * bypass area into the regular interpreter result. Fall back
		 * to the regular message if nothing was found in the
		 * bypasses.
		 */

		if (!TclChanCaughtErrorBypass(interp, chan) && interp) {
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                            "could not flush channel \"%s\"",
			    Tcl_GetChannelName((Tcl_Channel) chanPtr)));
		}
		return TCL_ERROR;
	    }

	    statePtr->csPtrR  = csPtrR;
	    statePtr->csPtrW = csPtrW;
	}

	/*
	 * Anything in the input queue and the push-back buffers of the
	 * transformation going away is transformed data, but not yet read. As
	 * unstacking means that the caller does not want to see transformed
	 * data any more we have to discard these bytes. To avoid writing an
	 * analogue to 'DiscardInputQueued' we move the information in the
	 * push back buffers to the input queue and then call
	 * 'DiscardInputQueued' on that.
	 */

	if (GotFlag(statePtr, TCL_READABLE) &&
		((statePtr->inQueueHead != NULL) ||
		(chanPtr->inQueueHead != NULL))) {
	    if ((statePtr->inQueueHead != NULL) &&
		    (chanPtr->inQueueHead != NULL)) {
		statePtr->inQueueTail->nextPtr = chanPtr->inQueueHead;
		statePtr->inQueueTail = chanPtr->inQueueTail;
		statePtr->inQueueHead = statePtr->inQueueTail;
	    } else if (chanPtr->inQueueHead != NULL) {
		statePtr->inQueueHead = chanPtr->inQueueHead;
		statePtr->inQueueTail = chanPtr->inQueueTail;
	    }

	    chanPtr->inQueueHead = NULL;
	    chanPtr->inQueueTail = NULL;

	    DiscardInputQueued(statePtr, 0);
	}

	/*
	 * TIP #218, Channel Thread Actions.
	 *
	 * We call the thread actions for the new channel directly. We
	 * _cannot_ use CutChannel, because the (thread-)global list of all
	 * channels always contains the _ChannelState_ for a stack of
	 * channels, not the individual channels. And SpliceChannel would not
	 * only call the thread actions, but also remove the shared
	 * ChannelState from this list despite there being more channels for
	 * the state which are still active.
	 */

	ChanThreadAction(chanPtr, TCL_CHANNEL_THREAD_REMOVE);

	statePtr->topChanPtr = downChanPtr;
	downChanPtr->upChanPtr = NULL;

	/*
	 * Leave this link intact for closeproc
	 *  chanPtr->downChanPtr = NULL;
	 */

	/*
	 * Close and free the channel driver state.
	 */

	result = ChanClose(chanPtr, interp);
	ChannelFree(chanPtr);

	UpdateInterest(statePtr->topChanPtr);

	if (result != 0) {
	    Tcl_SetErrno(result);

	    /*
	     * TIP #219, Tcl Channel Reflection API.
	     * Move error messages put by the driver into the chan/ip bypass
	     * area into the regular interpreter result.
	     */

	    TclChanCaughtErrorBypass(interp, chan);
	    return TCL_ERROR;
	}
    } else {
	/*
	 * This channel does not cover another one. Simply do a close, if
	 * necessary.
	 */

	if (statePtr->refCount <= 0) {
	    if (Tcl_Close(interp, chan) != TCL_OK) {
		/*
		 * TIP #219, Tcl Channel Reflection API.
		 * "TclChanCaughtErrorBypass" is not required here, it was
		 * done already by "Tcl_Close".
		 */

		return TCL_ERROR;
	    }
	}

	/*
	 * TIP #218, Channel Thread Actions.
	 * Not required in this branch, this is done by Tcl_Close. If
	 * Tcl_Close is not called then the ChannelState is still active in
	 * the thread and no action has to be taken either.
	 */
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetStackedChannel --
 *
 *	Determines whether the specified channel is stacked upon another.
 *
 * Results:
 *	NULL if the channel is not stacked upon another one, or a reference to
 *	the channel it is stacked upon. This reference can be used in queries,
 *	but modification is not allowed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_GetStackedChannel(
    Tcl_Channel chan)
{
    Channel *chanPtr = (Channel *) chan;
				/* The actual channel. */

    return (Tcl_Channel) chanPtr->downChanPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetTopChannel --
 *
 *	Returns the top channel of a channel stack.
 *
 * Results:
 *	NULL if the channel is not stacked upon another one, or a reference to
 *	the channel it is stacked upon. This reference can be used in queries,
 *	but modification is not allowed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_GetTopChannel(
    Tcl_Channel chan)
{
    Channel *chanPtr = (Channel *) chan;
				/* The actual channel. */

    return (Tcl_Channel) chanPtr->state->topChanPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelInstanceData --
 *
 *	Returns the client data associated with a channel.
 *
 * Results:
 *	The client data.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ClientData
Tcl_GetChannelInstanceData(
    Tcl_Channel chan)		/* Channel for which to return client data. */
{
    Channel *chanPtr = (Channel *) chan;
				/* The actual channel. */

    return chanPtr->instanceData;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelThread --
 *
 *	Given a channel structure, returns the thread managing it. TIP #10
 *
 * Results:
 *	Returns the id of the thread managing the channel.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_ThreadId
Tcl_GetChannelThread(
    Tcl_Channel chan)		/* The channel to return the managing thread
				 * for. */
{
    Channel *chanPtr = (Channel *) chan;
				/* The actual channel. */

    return chanPtr->state->managingThread;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelType --
 *
 *	Given a channel structure, returns the channel type structure.
 *
 * Results:
 *	Returns a pointer to the channel type structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const Tcl_ChannelType *
Tcl_GetChannelType(
    Tcl_Channel chan)		/* The channel to return type for. */
{
    Channel *chanPtr = (Channel *) chan;
				/* The actual channel. */

    return chanPtr->typePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelMode --
 *
 *	Computes a mask indicating whether the channel is open for reading and
 *	writing.
 *
 * Results:
 *	An OR-ed combination of TCL_READABLE and TCL_WRITABLE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetChannelMode(
    Tcl_Channel chan)		/* The channel for which the mode is being
				 * computed. */
{
    ChannelState *statePtr = ((Channel *) chan)->state;
				/* State of actual channel. */

    return (statePtr->flags & (TCL_READABLE | TCL_WRITABLE));
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelName --
 *
 *	Returns the string identifying the channel name.
 *
 * Results:
 *	The string containing the channel name. This memory is owned by the
 *	generic layer and should not be modified by the caller.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
Tcl_GetChannelName(
    Tcl_Channel chan)		/* The channel for which to return the name. */
{
    ChannelState *statePtr = ((Channel *) chan)->state;
				/* State of actual channel. */

    return statePtr->channelName;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelHandle --
 *
 *	Returns an OS handle associated with a channel.
 *
 * Results:
 *	Returns TCL_OK and places the handle in handlePtr, or returns
 *	TCL_ERROR on failure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetChannelHandle(
    Tcl_Channel chan,		/* The channel to get file from. */
    int direction,		/* TCL_WRITABLE or TCL_READABLE. */
    ClientData *handlePtr)	/* Where to store handle */
{
    Channel *chanPtr;		/* The actual channel. */
    ClientData handle;
    int result;

    chanPtr = ((Channel *) chan)->state->bottomChanPtr;
    if (!chanPtr->typePtr->getHandleProc) {
        Tcl_SetChannelError(chan, Tcl_ObjPrintf(
                "channel \"%s\" does not support OS handles",
                Tcl_GetChannelName(chan)));
	return TCL_ERROR;
    }
    result = chanPtr->typePtr->getHandleProc(chanPtr->instanceData, direction,
	    &handle);
    if (handlePtr) {
	*handlePtr = handle;
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * AllocChannelBuffer --
 *
 *	A channel buffer has BUFFER_PADDING bytes extra at beginning to hold
 *	any bytes of a native-encoding character that got split by the end of
 *	the previous buffer and need to be moved to the beginning of the next
 *	buffer to make a contiguous string so it can be converted to UTF-8.
 *
 *	A channel buffer has BUFFER_PADDING bytes extra at the end to hold any
 *	bytes of a native-encoding character (generated from a UTF-8
 *	character) that overflow past the end of the buffer and need to be
 *	moved to the next buffer.
 *
 * Results:
 *	A newly allocated channel buffer.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static ChannelBuffer *
AllocChannelBuffer(
    int length)			/* Desired length of channel buffer. */
{
    ChannelBuffer *bufPtr;
    int n;

    n = length + CHANNELBUFFER_HEADER_SIZE + BUFFER_PADDING + BUFFER_PADDING;
    bufPtr = ckalloc(n);
    bufPtr->nextAdded	= BUFFER_PADDING;
    bufPtr->nextRemoved	= BUFFER_PADDING;
    bufPtr->bufLength	= length + BUFFER_PADDING;
    bufPtr->nextPtr	= NULL;
    bufPtr->refCount	= 1;
    return bufPtr;
}

static void
PreserveChannelBuffer(
    ChannelBuffer *bufPtr)
{
    if (bufPtr->refCount == 0) {
	Tcl_Panic("Reuse of ChannelBuffer! %p", bufPtr);
    }
    bufPtr->refCount++;
}

static void
ReleaseChannelBuffer(
    ChannelBuffer *bufPtr)
{
    if (--bufPtr->refCount) {
	return;
    }
    ckfree(bufPtr);
}

static int
IsShared(
    ChannelBuffer *bufPtr)
{
    return bufPtr->refCount > 1;
}

/*
 *----------------------------------------------------------------------
 *
 * RecycleBuffer --
 *
 *	Helper function to recycle input and output buffers. Ensures that two
 *	input buffers are saved (one in the input queue and another in the
 *	saveInBufPtr field) and that curOutPtr is set to a buffer. Only if
 *	these conditions are met is the buffer freed to the OS.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May free a buffer to the OS.
 *
 *----------------------------------------------------------------------
 */

static void
RecycleBuffer(
    ChannelState *statePtr,	/* ChannelState in which to recycle buffers. */
    ChannelBuffer *bufPtr,	/* The buffer to recycle. */
    int mustDiscard)		/* If nonzero, free the buffer to the OS,
				 * always. */
{
    /*
     * Do we have to free the buffer to the OS?
     */
    if (IsShared(bufPtr)) {
	mustDiscard = 1;
    }

    if (mustDiscard) {
	ReleaseChannelBuffer(bufPtr);
	return;
    }

    /*
     * Only save buffers which have the requested buffersize for the
     * channel. This is to honor dynamic changes of the buffersize
     * made by the user.
     */

    if ((bufPtr->bufLength - BUFFER_PADDING) != statePtr->bufSize) {
	ReleaseChannelBuffer(bufPtr);
	return;
    }

    /*
     * Only save buffers for the input queue if the channel is readable.
     */

    if (GotFlag(statePtr, TCL_READABLE)) {
	if (statePtr->inQueueHead == NULL) {
	    statePtr->inQueueHead = bufPtr;
	    statePtr->inQueueTail = bufPtr;
	    goto keepBuffer;
	}
	if (statePtr->saveInBufPtr == NULL) {
	    statePtr->saveInBufPtr = bufPtr;
	    goto keepBuffer;
	}
    }

    /*
     * Only save buffers for the output queue if the channel is writable.
     */

    if (GotFlag(statePtr, TCL_WRITABLE)) {
	if (statePtr->curOutPtr == NULL) {
	    statePtr->curOutPtr = bufPtr;
	    goto keepBuffer;
	}
    }

    /*
     * If we reached this code we return the buffer to the OS.
     */

    ReleaseChannelBuffer(bufPtr);
    return;

  keepBuffer:
    bufPtr->nextRemoved = BUFFER_PADDING;
    bufPtr->nextAdded = BUFFER_PADDING;
    bufPtr->nextPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DiscardOutputQueued --
 *
 *	Discards all output queued in the output queue of a channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Recycles buffers.
 *
 *----------------------------------------------------------------------
 */

static void
DiscardOutputQueued(
    ChannelState *statePtr)	/* ChannelState for which to discard output. */
{
    ChannelBuffer *bufPtr;

    while (statePtr->outQueueHead != NULL) {
	bufPtr = statePtr->outQueueHead;
	statePtr->outQueueHead = bufPtr->nextPtr;
	RecycleBuffer(statePtr, bufPtr, 0);
    }
    statePtr->outQueueHead = NULL;
    statePtr->outQueueTail = NULL;
    bufPtr = statePtr->curOutPtr;
    if (bufPtr && BytesLeft(bufPtr)) {
	statePtr->curOutPtr = NULL;
	RecycleBuffer(statePtr, bufPtr, 0);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CheckForDeadChannel --
 *
 *	This function checks is a given channel is Dead (a channel that has
 *	been closed but not yet deallocated.)
 *
 * Results:
 *	True (1) if channel is Dead, False (0) if channel is Ok
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static int
CheckForDeadChannel(
    Tcl_Interp *interp,		/* For error reporting (can be NULL) */
    ChannelState *statePtr)	/* The channel state to check. */
{
    if (!GotFlag(statePtr, CHANNEL_DEAD)) {
	return 0;
    }

    Tcl_SetErrno(EINVAL);
    if (interp) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
                "unable to access channel: invalid channel", -1));
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * FlushChannel --
 *
 *	This function flushes as much of the queued output as is possible now.
 *	If calledFromAsyncFlush is nonzero, it is being called in an event
 *	handler to flush channel output asynchronously.
 *
 * Results:
 *	0 if successful, else the error code that was returned by the channel
 *	type operation. May leave a message in the interp result.
 *
 * Side effects:
 *	May produce output on a channel. May block indefinitely if the channel
 *	is synchronous. May schedule an async flush on the channel. May
 *	recycle memory for buffers in the output queue.
 *
 *----------------------------------------------------------------------
 */

static int
FlushChannel(
    Tcl_Interp *interp,		/* For error reporting during close. */
    Channel *chanPtr,		/* The channel to flush on. */
    int calledFromAsyncFlush)	/* If nonzero then we are being called from an
				 * asynchronous flush callback. */
{
    ChannelState *statePtr = chanPtr->state;
				/* State of the channel stack. */
    ChannelBuffer *bufPtr;	/* Iterates over buffered output queue. */
    int written;		/* Amount of output data actually written in
				 * current round. */
    int errorCode = 0;		/* Stores POSIX error codes from channel
				 * driver operations. */
    int wroteSome = 0;		/* Set to one if any data was written to the
				 * driver. */

    /*
     * Prevent writing on a dead channel -- a channel that has been closed but
     * not yet deallocated. This can occur if the exit handler for the channel
     * deallocation runs before all channels are deregistered in all
     * interpreters.
     */

    if (CheckForDeadChannel(interp, statePtr)) {
	return -1;
    }

    /*
     * Should we shift the current output buffer over to the output queue?
     * First check that there are bytes in it.  If so then...
     * If the output queue is empty, then yes, trusting the caller called
     * us only when written bytes ought to be flushed.
     * If the current output buffer is full, then yes, so we can meet
     * the post-condition that on a successful return to caller we've
     * left space in the current output buffer for more writing (the flush
     * call was to make new room).
     * If the channel is blocking, then yes, so we guarantee that
     * blocking flushes actually flush all pending data.
     * Otherwise, no.  Keep the current output buffer where it is so more
     * can be written to it, possibly filling it, to promote more efficient
     * buffer usage.
     */

    bufPtr = statePtr->curOutPtr;
    if (bufPtr && BytesLeft(bufPtr) && /* Keep empties off queue */
	    (statePtr->outQueueHead == NULL || IsBufferFull(bufPtr)
		    || !GotFlag(statePtr, CHANNEL_NONBLOCKING))) {
	if (statePtr->outQueueHead == NULL) {
	    statePtr->outQueueHead = bufPtr;
	} else {
	    statePtr->outQueueTail->nextPtr = bufPtr;
	}
	statePtr->outQueueTail = bufPtr;
	statePtr->curOutPtr = NULL;
    }

    assert(!IsBufferFull(statePtr->curOutPtr));

    /*
     * If we are not being called from an async flush and an async flush
     * is active, we just return without producing any output.
     */

    if (!calledFromAsyncFlush && GotFlag(statePtr, BG_FLUSH_SCHEDULED)) {
	return 0;
    }

    /*
     * Loop over the queued buffers and attempt to flush as much as possible
     * of the queued output to the channel.
     */

    TclChannelPreserve((Tcl_Channel)chanPtr);
    while (statePtr->outQueueHead) {
	bufPtr = statePtr->outQueueHead;

	/*
	 * Produce the output on the channel.
	 */

	PreserveChannelBuffer(bufPtr);
	written = ChanWrite(chanPtr, RemovePoint(bufPtr), BytesLeft(bufPtr),
		&errorCode);

	/*
	 * If the write failed completely attempt to start the asynchronous
	 * flush mechanism and break out of this loop - do not attempt to
	 * write any more output at this time.
	 */

	if (written < 0) {
	    /*
	     * If the last attempt to write was interrupted, simply retry.
	     */

	    if (errorCode == EINTR) {
		errorCode = 0;
		ReleaseChannelBuffer(bufPtr);
		continue;
	    }

	    /*
	     * If the channel is non-blocking and we would have blocked, start
	     * a background flushing handler and break out of the loop.
	     */

	    if ((errorCode == EWOULDBLOCK) || (errorCode == EAGAIN)) {
		/*
		 * This used to check for CHANNEL_NONBLOCKING, and panic if
		 * the channel was blocking. However, it appears that setting
		 * stdin to -blocking 0 has some effect on the stdout when
		 * it's a tty channel (dup'ed underneath)
		 */

		if (!GotFlag(statePtr, BG_FLUSH_SCHEDULED) && !TclInExit()) {
		    SetFlag(statePtr, BG_FLUSH_SCHEDULED);
		    UpdateInterest(chanPtr);
		}
		errorCode = 0;
		ReleaseChannelBuffer(bufPtr);
		break;
	    }

	    /*
	     * Decide whether to report the error upwards or defer it.
	     */

	    if (calledFromAsyncFlush) {
		/*
		 * TIP #219, Tcl Channel Reflection API.
		 * When defering the error copy a message from the bypass into
		 * the unreported area. Or discard it if the new error is to be
		 * ignored in favor of an earlier defered error.
		 */

		Tcl_Obj *msg = statePtr->chanMsg;

		if (statePtr->unreportedError == 0) {
		    statePtr->unreportedError = errorCode;
		    statePtr->unreportedMsg = msg;
		    if (msg != NULL) {
			Tcl_IncrRefCount(msg);
		    }
		} else {
		    /*
		     * An old unreported error is kept, and this error thrown
		     * away.
		     */

		    statePtr->chanMsg = NULL;
		    if (msg != NULL) {
			TclDecrRefCount(msg);
		    }
		}
	    } else {
		/*
		 * TIP #219, Tcl Channel Reflection API.
		 * Move error messages put by the driver into the chan bypass
		 * area into the regular interpreter result. Fall back to the
		 * regular message if nothing was found in the bypasses.
		 */

		Tcl_SetErrno(errorCode);
		if (interp != NULL && !TclChanCaughtErrorBypass(interp,
			(Tcl_Channel) chanPtr)) {
		    Tcl_SetObjResult(interp,
			    Tcl_NewStringObj(Tcl_PosixError(interp), -1));
		}

		/*
		 * An unreportable bypassed message is kept, for the caller of
		 * Tcl_Seek, Tcl_Write, etc.
		 */
	    }

	    /*
	     * When we get an error we throw away all the output currently
	     * queued.
	     */

	    DiscardOutputQueued(statePtr);
	    ReleaseChannelBuffer(bufPtr);
	    break;
	} else {
	    /* TODO: Consider detecting and reacting to short writes
	     * on blocking channels.  Ought not happen.  See iocmd-24.2. */
	    wroteSome = 1;
	}

	bufPtr->nextRemoved += written;

	/*
	 * If this buffer is now empty, recycle it.
	 */

	if (IsBufferEmpty(bufPtr)) {
	    statePtr->outQueueHead = bufPtr->nextPtr;
	    if (statePtr->outQueueHead == NULL) {
		statePtr->outQueueTail = NULL;
	    }
	    RecycleBuffer(statePtr, bufPtr, 0);
	}
	ReleaseChannelBuffer(bufPtr);
    }	/* Closes "while". */

    /*
     * If we wrote some data while flushing in the background, we are done.
     * We can't finish the background flush until we run out of data and the
     * channel becomes writable again. This ensures that all of the pending
     * data has been flushed at the system level.
     */

    if (GotFlag(statePtr, BG_FLUSH_SCHEDULED)) {
	if (wroteSome) {
	    goto done;
	} else if (statePtr->outQueueHead == NULL) {
	    ResetFlag(statePtr, BG_FLUSH_SCHEDULED);
	    ChanWatch(chanPtr, statePtr->interestMask);
	} else {

	    /*
	     * When we are calledFromAsyncFlush, that means a writable
	     * state on the channel triggered the call, so we should be
	     * able to write something.  Either we did write something
	     * and wroteSome should be set, or there was nothing left to
	     * write in this call, and we've completed the BG flush.
	     * These are the two cases above.  If we get here, that means
	     * there is some kind failure in the writable event machinery.
	     *
	     * The tls extension indeed suffers from flaws in its channel
	     * event mgmt.  See https://core.tcl-lang.org/tcl/info/c31ca233ca.
	     * Until that patch is broadly distributed, disable the
	     * assertion checking here, so that programs using Tcl and
	     * tls can be debugged.

	    assert(!calledFromAsyncFlush);
	     */
	}
    }

    /*
     * If the channel is flagged as closed, delete it when the refCount drops
     * to zero, the output queue is empty and there is no output in the
     * current output buffer.
     */

    if (GotFlag(statePtr, CHANNEL_CLOSED) && (statePtr->refCount <= 0) &&
	    (statePtr->outQueueHead == NULL) &&
	    ((statePtr->curOutPtr == NULL) ||
	    IsBufferEmpty(statePtr->curOutPtr))) {
	errorCode = CloseChannel(interp, chanPtr, errorCode);
	goto done;
    }

    /*
     * If the write-side of the channel is flagged as closed, delete it when
     * the output queue is empty and there is no output in the current output
     * buffer.
     */

    if (GotFlag(statePtr, CHANNEL_CLOSEDWRITE) &&
	    (statePtr->outQueueHead == NULL) &&
	    ((statePtr->curOutPtr == NULL) ||
	    IsBufferEmpty(statePtr->curOutPtr))) {
	errorCode = CloseChannelPart(interp, chanPtr, errorCode, TCL_CLOSE_WRITE);
	goto done;
    }

  done:
    TclChannelRelease((Tcl_Channel)chanPtr);
    return errorCode;
}

/*
 *----------------------------------------------------------------------
 *
 * CloseChannel --
 *
 *	Utility procedure to close a channel and free associated resources.
 *
 *	If the channel was stacked, then the it will copy the necessary
 *	elements of the NEXT channel into the TOP channel, in essence
 *	unstacking the channel. The NEXT channel will then be freed.
 *
 *	If the channel was not stacked, then we will free all the bits for the
 *	TOP channel, including the data structure itself.
 *
 * Results:
 *	Error code from an unreported error or the driver close operation.
 *
 * Side effects:
 *	May close the actual channel, may free memory, may change the value of
 *	errno.
 *
 *----------------------------------------------------------------------
 */

static int
CloseChannel(
    Tcl_Interp *interp,		/* For error reporting. */
    Channel *chanPtr,		/* The channel to close. */
    int errorCode)		/* Status of operation so far. */
{
    int result = 0;		/* Of calling driver close operation. */
    ChannelState *statePtr;	/* State of the channel stack. */
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (chanPtr == NULL) {
	return result;
    }
    statePtr = chanPtr->state;

    /*
     * No more input can be consumed so discard any leftover input.
     */

    DiscardInputQueued(statePtr, 1);

    /*
     * Discard a leftover buffer in the current output buffer field.
     */

    if (statePtr->curOutPtr != NULL) {
	ReleaseChannelBuffer(statePtr->curOutPtr);
	statePtr->curOutPtr = NULL;
    }

    /*
     * The caller guarantees that there are no more buffers queued for output.
     */

    if (statePtr->outQueueHead != NULL) {
	Tcl_Panic("TclFlush, closed channel: queued output left");
    }

    /*
     * If the EOF character is set in the channel, append that to the output
     * device.
     */

    if ((statePtr->outEofChar != 0) && GotFlag(statePtr, TCL_WRITABLE)) {
	int dummy;
	char c = (char) statePtr->outEofChar;

	(void) ChanWrite(chanPtr, &c, 1, &dummy);
    }

    /*
     * TIP #219, Tcl Channel Reflection API.
     * Move a leftover error message in the channel bypass into the
     * interpreter bypass. Just clear it if there is no interpreter.
     */

    if (statePtr->chanMsg != NULL) {
	if (interp != NULL) {
	    Tcl_SetChannelErrorInterp(interp, statePtr->chanMsg);
	}
	TclDecrRefCount(statePtr->chanMsg);
	statePtr->chanMsg = NULL;
    }

    /*
     * Remove this channel from of the list of all channels.
     */

    CutChannel((Tcl_Channel) chanPtr);

    /*
     * Close and free the channel driver state.
     * This may leave a TIP #219 error message in the interp.
     */

    result = ChanClose(chanPtr, interp);

    /*
     * Some resources can be cleared only if the bottom channel in a stack is
     * closed. All the other channels in the stack are not allowed to remove.
     */

    if (chanPtr == statePtr->bottomChanPtr) {
	if (statePtr->channelName != NULL) {
	    ckfree(statePtr->channelName);
	    statePtr->channelName = NULL;
	}

	Tcl_FreeEncoding(statePtr->encoding);
    }

    /*
     * If we are being called synchronously, report either any latent error on
     * the channel or the current error.
     */

    if (statePtr->unreportedError != 0) {
	errorCode = statePtr->unreportedError;

	/*
	 * TIP #219, Tcl Channel Reflection API.
	 * Move an error message found in the unreported area into the regular
	 * bypass (interp). This kills any message in the channel bypass area.
	 */

	if (statePtr->chanMsg != NULL) {
	    TclDecrRefCount(statePtr->chanMsg);
	    statePtr->chanMsg = NULL;
	}
	if (interp) {
	    Tcl_SetChannelErrorInterp(interp, statePtr->unreportedMsg);
	}
    }
    if (errorCode == 0) {
	errorCode = result;
	if (errorCode != 0) {
	    Tcl_SetErrno(errorCode);
	}
    }

    /*
     * Cancel any outstanding timer.
     */

    Tcl_DeleteTimerHandler(statePtr->timer);

    /*
     * Mark the channel as deleted by clearing the type structure.
     */

    if (chanPtr->downChanPtr != NULL) {
	Channel *downChanPtr = chanPtr->downChanPtr;

	statePtr->nextCSPtr = tsdPtr->firstCSPtr;
	tsdPtr->firstCSPtr = statePtr;

	statePtr->topChanPtr = downChanPtr;
	downChanPtr->upChanPtr = NULL;

	ChannelFree(chanPtr);

	return Tcl_Close(interp, (Tcl_Channel) downChanPtr);
    }

    /*
     * There is only the TOP Channel, so we free the remaining pointers we
     * have and then ourselves. Since this is the last of the channels in the
     * stack, make sure to free the ChannelState structure associated with it.
     */

    ChannelFree(chanPtr);

    Tcl_EventuallyFree(statePtr, TCL_DYNAMIC);

    return errorCode;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CutChannel --
 * CutChannel --
 *
 *	Removes a channel from the (thread-)global list of all channels (in
 *	that thread). This is actually the statePtr for the stack of channel.
 *
 * Results:
 *	Nothing.
 *
 * Side effects:
 *	Resets the field 'nextCSPtr' of the specified channel state to NULL.
 *
 * NOTE:
 *	The channel to cut out of the list must not be referenced in any
 *	interpreter. This is something this procedure cannot check (despite
 *	the refcount) because the caller usually wants fiddle with the channel
 *	(like transfering it to a different thread) and thus keeps the
 *	refcount artifically high to prevent its destruction.
 *
 *----------------------------------------------------------------------
 */

static void
CutChannel(
    Tcl_Channel chan)		/* The channel being removed. Must not be
				 * referenced in any interpreter. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    ChannelState *prevCSPtr;	/* Preceding channel state in list of all
				 * states - used to splice a channel out of
				 * the list on close. */
    ChannelState *statePtr = ((Channel *) chan)->state;
				/* State of the channel stack. */

    /*
     * Remove this channel from of the list of all channels (in the current
     * thread).
     */

    if (tsdPtr->firstCSPtr && (statePtr == tsdPtr->firstCSPtr)) {
	tsdPtr->firstCSPtr = statePtr->nextCSPtr;
    } else {
	for (prevCSPtr = tsdPtr->firstCSPtr;
		prevCSPtr && (prevCSPtr->nextCSPtr != statePtr);
		prevCSPtr = prevCSPtr->nextCSPtr) {
	    /* Empty loop body. */
	}
	if (prevCSPtr == NULL) {
	    Tcl_Panic("FlushChannel: damaged channel list");
	}
	prevCSPtr->nextCSPtr = statePtr->nextCSPtr;
    }

    statePtr->nextCSPtr = NULL;

    /*
     * TIP #218, Channel Thread Actions
     */

    ChanThreadAction((Channel *) chan, TCL_CHANNEL_THREAD_REMOVE);
}

void
Tcl_CutChannel(
    Tcl_Channel chan)		/* The channel being added. Must not be
				 * referenced in any interpreter. */
{
    Channel *chanPtr = ((Channel *) chan)->state->bottomChanPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    ChannelState *prevCSPtr;	/* Preceding channel state in list of all
				 * states - used to splice a channel out of
				 * the list on close. */
    ChannelState *statePtr = chanPtr->state;
				/* State of the channel stack. */

    /*
     * Remove this channel from of the list of all channels (in the current
     * thread).
     */

    if (tsdPtr->firstCSPtr && (statePtr == tsdPtr->firstCSPtr)) {
	tsdPtr->firstCSPtr = statePtr->nextCSPtr;
    } else {
	for (prevCSPtr = tsdPtr->firstCSPtr;
		prevCSPtr && (prevCSPtr->nextCSPtr != statePtr);
		prevCSPtr = prevCSPtr->nextCSPtr) {
	    /* Empty loop body. */
	}
	if (prevCSPtr == NULL) {
	    Tcl_Panic("FlushChannel: damaged channel list");
	}
	prevCSPtr->nextCSPtr = statePtr->nextCSPtr;
    }

    statePtr->nextCSPtr = NULL;

    /*
     * TIP #218, Channel Thread Actions
     * For all transformations and the base channel.
     */

    for (; chanPtr != NULL ; chanPtr = chanPtr->upChanPtr) {
	ChanThreadAction(chanPtr, TCL_CHANNEL_THREAD_REMOVE);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SpliceChannel --
 * SpliceChannel --
 *
 *	Adds a channel to the (thread-)global list of all channels (in that
 *	thread). Expects that the field 'nextChanPtr' in the channel is set to
 *	NULL.
 *
 * Results:
 *	Nothing.
 *
 * Side effects:
 *	Nothing.
 *
 * NOTE:
 *	The channel to splice into the list must not be referenced in any
 *	interpreter. This is something this procedure cannot check (despite
 *	the refcount) because the caller usually wants figgle with the channel
 *	(like transfering it to a different thread) and thus keeps the
 *	refcount artifically high to prevent its destruction.
 *
 *----------------------------------------------------------------------
 */

static void
SpliceChannel(
    Tcl_Channel chan)		/* The channel being added. Must not be
				 * referenced in any interpreter. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    ChannelState *statePtr = ((Channel *) chan)->state;

    if (statePtr->nextCSPtr != NULL) {
	Tcl_Panic("SpliceChannel: trying to add channel used in different list");
    }

    statePtr->nextCSPtr = tsdPtr->firstCSPtr;
    tsdPtr->firstCSPtr = statePtr;

    /*
     * TIP #10. Mark the current thread as the new one managing this channel.
     *		Note: 'Tcl_GetCurrentThread' returns sensible values even for
     *		a non-threaded core.
     */

    statePtr->managingThread = Tcl_GetCurrentThread();

    /*
     * TIP #218, Channel Thread Actions
     */

    ChanThreadAction((Channel *) chan, TCL_CHANNEL_THREAD_INSERT);
}

void
Tcl_SpliceChannel(
    Tcl_Channel chan)		/* The channel being added. Must not be
				 * referenced in any interpreter. */
{
    Channel *chanPtr = ((Channel *) chan)->state->bottomChanPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    ChannelState *statePtr = chanPtr->state;

    if (statePtr->nextCSPtr != NULL) {
	Tcl_Panic("SpliceChannel: trying to add channel used in different list");
    }

    statePtr->nextCSPtr = tsdPtr->firstCSPtr;
    tsdPtr->firstCSPtr = statePtr;

    /*
     * TIP #10. Mark the current thread as the new one managing this channel.
     *		Note: 'Tcl_GetCurrentThread' returns sensible values even for
     *		a non-threaded core.
     */

    statePtr->managingThread = Tcl_GetCurrentThread();

    /*
     * TIP #218, Channel Thread Actions
     * For all transformations and the base channel.
     */

    for (; chanPtr != NULL ; chanPtr = chanPtr->upChanPtr) {
	ChanThreadAction(chanPtr, TCL_CHANNEL_THREAD_INSERT);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Close --
 *
 *	Closes a channel.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Closes the channel if this is the last reference.
 *
 * NOTE:
 *	Tcl_Close removes the channel as far as the user is concerned.
 *	However, it may continue to exist for a while longer if it has a
 *	background flush scheduled. The device itself is eventually closed and
 *	the channel record removed, in CloseChannel, above.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_Close(
    Tcl_Interp *interp,		/* Interpreter for errors. */
    Tcl_Channel chan)		/* The channel being closed. Must not be
				 * referenced in any interpreter. */
{
    CloseCallback *cbPtr;	/* Iterate over close callbacks for this
				 * channel. */
    Channel *chanPtr;		/* The real IO channel. */
    ChannelState *statePtr;	/* State of real IO channel. */
    int result;			/* Of calling FlushChannel. */
    int flushcode;
    int stickyError;

    if (chan == NULL) {
	return TCL_OK;
    }

    /*
     * Perform special handling for standard channels being closed. If the
     * refCount is now 1 it means that the last reference to the standard
     * channel is being explicitly closed, so bump the refCount down
     * artificially to 0. This will ensure that the channel is actually
     * closed, below. Also set the static pointer to NULL for the channel.
     */

    CheckForStdChannelsBeingClosed(chan);

    /*
     * This operation should occur at the top of a channel stack.
     */

    chanPtr = (Channel *) chan;
    statePtr = chanPtr->state;
    chanPtr = statePtr->topChanPtr;

    if (statePtr->refCount > 0) {
	Tcl_Panic("called Tcl_Close on channel with refCount > 0");
    }

    if (GotFlag(statePtr, CHANNEL_INCLOSE)) {
	if (interp) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
                    "illegal recursive call to close through close-handler"
                    " of channel", -1));
	}
	return TCL_ERROR;
    }
    SetFlag(statePtr, CHANNEL_INCLOSE);

    /*
     * When the channel has an escape sequence driven encoding such as
     * iso2022, the terminated escape sequence must write to the buffer.
     */

    stickyError = 0;

    if (GotFlag(statePtr, TCL_WRITABLE) && (statePtr->encoding != NULL)
	    && !(statePtr->outputEncodingFlags & TCL_ENCODING_START)) {

	int code = CheckChannelErrors(statePtr, TCL_WRITABLE);

	if (code == 0) {
	    statePtr->outputEncodingFlags |= TCL_ENCODING_END;
	    code = WriteChars(chanPtr, "", 0);
	    statePtr->outputEncodingFlags &= ~TCL_ENCODING_END;
	    statePtr->outputEncodingFlags |= TCL_ENCODING_START;
	}
	if (code < 0) {
	    stickyError = Tcl_GetErrno();
	}

	/*
	 * TIP #219, Tcl Channel Reflection API.
	 * Move an error message found in the channel bypass into the
	 * interpreter bypass. Just clear it if there is no interpreter.
	 */

	if (statePtr->chanMsg != NULL) {
	    if (interp != NULL) {
		Tcl_SetChannelErrorInterp(interp, statePtr->chanMsg);
	    }
	    TclDecrRefCount(statePtr->chanMsg);
	    statePtr->chanMsg = NULL;
	}
    }

    Tcl_ClearChannelHandlers(chan);

    /*
     * Invoke the registered close callbacks and delete their records.
     */

    while (statePtr->closeCbPtr != NULL) {
	cbPtr = statePtr->closeCbPtr;
	statePtr->closeCbPtr = cbPtr->nextPtr;
	cbPtr->proc(cbPtr->clientData);
	ckfree(cbPtr);
    }

    ResetFlag(statePtr, CHANNEL_INCLOSE);

    /*
     * If this channel supports it, close the read side, since we don't need
     * it anymore and this will help avoid deadlocks on some channel types.
     */

    if (chanPtr->typePtr->closeProc == TCL_CLOSE2PROC) {
	result = chanPtr->typePtr->close2Proc(chanPtr->instanceData, interp,
		TCL_CLOSE_READ);
    } else {
	result = 0;
    }

    /*
     * The call to FlushChannel will flush any queued output and invoke the
     * close function of the channel driver, or it will set up the channel to
     * be flushed and closed asynchronously.
     */

    SetFlag(statePtr, CHANNEL_CLOSED);

    flushcode = FlushChannel(interp, chanPtr, 0);

    /*
     * TIP #219.
     * Capture error messages put by the driver into the bypass area and put
     * them into the regular interpreter result.
     *
     * Notes: Due to the assertion of CHANNEL_CLOSED in the flags
     * FlushChannel() has called CloseChannel() and thus freed all the channel
     * structures. We must not try to access "chan" anymore, hence the NULL
     * argument in the call below. The only place which may still contain a
     * message is the interpreter itself, and "CloseChannel" made sure to lift
     * any channel message it generated into it.
     */

    if (TclChanCaughtErrorBypass(interp, NULL)) {
	result = EINVAL;
    }

    if (stickyError != 0) {
	Tcl_SetErrno(stickyError);
	if (interp != NULL) {
	    Tcl_SetObjResult(interp,
			     Tcl_NewStringObj(Tcl_PosixError(interp), -1));
	}
	return TCL_ERROR;
    }
    /*
     * Bug 97069ea11a: set error message if a flush code is set and no error
     * message set up to now.
     */
    if (flushcode != 0 && interp != NULL
	    && 0 == Tcl_GetCharLength(Tcl_GetObjResult(interp)) ) {
	Tcl_SetErrno(flushcode);
	Tcl_SetObjResult(interp,
		Tcl_NewStringObj(Tcl_PosixError(interp), -1));
    }
    if ((flushcode != 0) || (result != 0)) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CloseEx --
 *
 *	Closes one side of a channel, read or write.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Closes one direction of the channel.
 *
 * NOTE:
 *	Tcl_CloseEx closes the specified direction of the channel as far as
 *	the user is concerned. The channel keeps existing however. You cannot
 *	calls this function to close the last possible direction of the
 *	channel. Use Tcl_Close for that.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_CloseEx(
    Tcl_Interp *interp,		/* Interpreter for errors. */
    Tcl_Channel chan,		/* The channel being closed. May still be used
				 * by some interpreter. */
    int flags)			/* Flags telling us which side to close. */
{
    Channel *chanPtr;		/* The real IO channel. */
    ChannelState *statePtr;	/* State of real IO channel. */

    if (chan == NULL) {
	return TCL_OK;
    }

    /* TODO: assert flags validity ? */

    chanPtr = (Channel *) chan;
    statePtr = chanPtr->state;

    /*
     * Does the channel support half-close anyway? Error if not.
     */

    if (!chanPtr->typePtr->close2Proc) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "half-close of channels not supported by %ss",
		chanPtr->typePtr->typeName));
	return TCL_ERROR;
    }

    /*
     * Is the channel unstacked ? If not we fail.
     */

    if (chanPtr != statePtr->topChanPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"half-close not applicable to stack of transformations", -1));
	return TCL_ERROR;
    }

    /*
     * Check direction against channel mode. It is an error if we try to close
     * a direction not supported by the channel (already closed, or never
     * opened for that direction).
     */

    if (!(statePtr->flags & (TCL_READABLE | TCL_WRITABLE) & flags)) {
	const char *msg;

	if (flags & TCL_CLOSE_READ) {
	    msg = "read";
	} else {
	    msg = "write";
	}
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "Half-close of %s-side not possible, side not opened or"
                " already closed", msg));
	return TCL_ERROR;
    }

    /*
     * A user may try to call half-close from within a channel close
     * handler. That won't do.
     */

    if (statePtr->flags & CHANNEL_INCLOSE) {
	if (interp) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
                    "illegal recursive call to close through close-handler"
                    " of channel", -1));
	}
	return TCL_ERROR;
    }

    if (flags & TCL_CLOSE_READ) {
	/*
	 * Call the finalization code directly. There are no events to handle,
	 * there cannot be for the read-side.
	 */

	return CloseChannelPart(interp, chanPtr, 0, flags);
    } else if (flags & TCL_CLOSE_WRITE) {
	Tcl_Preserve(statePtr);
	if (!GotFlag(statePtr, BG_FLUSH_SCHEDULED)) {
	    /*
	     * We don't want to re-enter CloseWrite().
	     */

	    if (!GotFlag(statePtr, CHANNEL_CLOSEDWRITE)) {
		if (CloseWrite(interp, chanPtr) != TCL_OK) {
		    SetFlag(statePtr, CHANNEL_CLOSEDWRITE);
		    Tcl_Release(statePtr);
		    return TCL_ERROR;
		}
	    }
	}
	SetFlag(statePtr, CHANNEL_CLOSEDWRITE);
	Tcl_Release(statePtr);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CloseWrite --
 *
 *	Closes the write side a channel.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Closes the write side of the channel.
 *
 * NOTE:
 *	CloseWrite removes the channel as far as the user is concerned.
 *	However, the ooutput data structures may continue to exist for a while
 *	longer if it has a background flush scheduled. The device itself is
 *	eventually closed and the channel structures modified, in
 *	CloseChannelPart, below.
 *
 *----------------------------------------------------------------------
 */

static int
CloseWrite(
    Tcl_Interp *interp,		/* Interpreter for errors. */
    Channel *chanPtr)		/* The channel whose write side is being
                                 * closed. May still be used by some
                                 * interpreter */
{
    /* Notes: clear-channel-handlers - write side only ? or keep around, just
     * not called. */
    /* No close cllbacks are run - channel is still open (read side) */

    ChannelState *statePtr = chanPtr->state;
                                /* State of real IO channel. */
    int flushcode;
    int result = 0;

    /*
     * The call to FlushChannel will flush any queued output and invoke the
     * close function of the channel driver, or it will set up the channel to
     * be flushed and closed asynchronously.
     */

    SetFlag(statePtr, CHANNEL_CLOSEDWRITE);

    flushcode = FlushChannel(interp, chanPtr, 0);

    /*
     * TIP #219.
     * Capture error messages put by the driver into the bypass area and put
     * them into the regular interpreter result.
     *
     * Notes: Due to the assertion of CHANNEL_CLOSEDWRITE in the flags
     * FlushChannel() has called CloseChannelPart(). While we can still access
     * "chan" (no structures were freed), the only place which may still
     * contain a message is the interpreter itself, and "CloseChannelPart" made
     * sure to lift any channel message it generated into it. Hence the NULL
     * argument in the call below.
     */

    if (TclChanCaughtErrorBypass(interp, NULL)) {
	result = EINVAL;
    }

    if ((flushcode != 0) || (result != 0)) {
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CloseChannelPart --
 *
 *	Utility procedure to close a channel partially and free associated
 *	resources. If the channel was stacked it will never be run (The higher
 *	level forbid this). If the channel was not stacked, then we will free
 *	all the bits of the chosen side (read, or write) for the TOP channel.
 *
 * Results:
 *	Error code from an unreported error or the driver close2 operation.
 *
 * Side effects:
 *	May free memory, may change the value of errno.
 *
 *----------------------------------------------------------------------
 */

static int
CloseChannelPart(
    Tcl_Interp *interp,		/* Interpreter for errors. */
    Channel *chanPtr,		/* The channel being closed. May still be used
				 * by some interpreter. */
    int errorCode,		/* Status of operation so far. */
    int flags)			/* Flags telling us which side to close. */
{
    ChannelState *statePtr;	/* State of real IO channel. */
    int result;			/* Of calling the close2proc. */

    statePtr = chanPtr->state;

    if (flags & TCL_CLOSE_READ) {
	/*
	 * No more input can be consumed so discard any leftover input.
	 */

	DiscardInputQueued(statePtr, 1);
    } else if (flags & TCL_CLOSE_WRITE) {
	/*
	 * The caller guarantees that there are no more buffers queued for
	 * output.
	 */

	if (statePtr->outQueueHead != NULL) {
	    Tcl_Panic("ClosechanHalf, closed write-side of channel: "
		    "queued output left");
	}

	/*
	 * If the EOF character is set in the channel, append that to the
	 * output device.
	 */

	if ((statePtr->outEofChar != 0) && GotFlag(statePtr, TCL_WRITABLE)) {
	    int dummy;
	    char c = (char) statePtr->outEofChar;

	    (void) ChanWrite(chanPtr, &c, 1, &dummy);
	}

	/*
	 * TIP #219, Tcl Channel Reflection API.
	 * Move a leftover error message in the channel bypass into the
	 * interpreter bypass. Just clear it if there is no interpreter.
	 */

	if (statePtr->chanMsg != NULL) {
	    if (interp != NULL) {
		Tcl_SetChannelErrorInterp(interp, statePtr->chanMsg);
	    }
	    TclDecrRefCount(statePtr->chanMsg);
	    statePtr->chanMsg = NULL;
	}
    }

    /*
     * Finally do what is asked of us. Close and free the channel driver state
     * for the chosen side of the channel. This may leave a TIP #219 error
     * message in the interp.
     */

    result = ChanCloseHalf(chanPtr, interp, flags);

    /*
     * If we are being called synchronously, report either any latent error on
     * the channel or the current error.
     */

    if (statePtr->unreportedError != 0) {
	errorCode = statePtr->unreportedError;

	/*
	 * TIP #219, Tcl Channel Reflection API.
	 * Move an error message found in the unreported area into the regular
	 * bypass (interp). This kills any message in the channel bypass area.
	 */

	if (statePtr->chanMsg != NULL) {
	    TclDecrRefCount(statePtr->chanMsg);
	    statePtr->chanMsg = NULL;
	}
	if (interp) {
	    Tcl_SetChannelErrorInterp(interp, statePtr->unreportedMsg);
	}
    }
    if (errorCode == 0) {
	errorCode = result;
	if (errorCode != 0) {
	    Tcl_SetErrno(errorCode);
	}
    }

    /*
     * TIP #219.
     * Capture error messages put by the driver into the bypass area and put
     * them into the regular interpreter result. See also the bottom of
     * CloseWrite().
     */

    if (TclChanCaughtErrorBypass(interp, (Tcl_Channel) chanPtr)) {
	result = EINVAL;
    }

    if (result != 0) {
	return TCL_ERROR;
    }

    /*
     * Remove the closed side from the channel mode/flags.
     */

    ResetFlag(statePtr, flags & (TCL_READABLE | TCL_WRITABLE));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ClearChannelHandlers --
 *
 *	Removes all channel handlers and event scripts from the channel,
 *	cancels all background copies involving the channel and any interest
 *	in events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above. Deallocates memory.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_ClearChannelHandlers(
    Tcl_Channel channel)
{
    ChannelHandler *chPtr, *chNext;	/* Iterate over channel handlers. */
    EventScriptRecord *ePtr, *eNextPtr;	/* Iterate over eventscript records. */
    Channel *chanPtr;			/* The real IO channel. */
    ChannelState *statePtr;		/* State of real IO channel. */
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    NextChannelHandler *nhPtr;

    /*
     * This operation should occur at the top of a channel stack.
     */

    chanPtr = (Channel *) channel;
    statePtr = chanPtr->state;
    chanPtr = statePtr->topChanPtr;

    /*
     * Cancel any outstanding timer.
     */

    Tcl_DeleteTimerHandler(statePtr->timer);

    /*
     * Remove any references to channel handlers for this channel that may be
     * about to be invoked.
     */

    for (nhPtr = tsdPtr->nestedHandlerPtr; nhPtr != NULL;
	    nhPtr = nhPtr->nestedHandlerPtr) {
	if (nhPtr->nextHandlerPtr &&
		(nhPtr->nextHandlerPtr->chanPtr == chanPtr)) {
	    nhPtr->nextHandlerPtr = NULL;
	}
    }

    /*
     * Remove all the channel handler records attached to the channel itself.
     */

    for (chPtr = statePtr->chPtr; chPtr != NULL; chPtr = chNext) {
	chNext = chPtr->nextPtr;
	ckfree(chPtr);
    }
    statePtr->chPtr = NULL;

    /*
     * Cancel any pending copy operation.
     */

    StopCopy(statePtr->csPtrR);
    StopCopy(statePtr->csPtrW);

    /*
     * Must set the interest mask now to 0, otherwise infinite loops
     * will occur if Tcl_DoOneEvent is called before the channel is
     * finally deleted in FlushChannel. This can happen if the channel
     * has a background flush active.
     */

    statePtr->interestMask = 0;

    /*
     * Remove any EventScript records for this channel.
     */

    for (ePtr = statePtr->scriptRecordPtr; ePtr != NULL; ePtr = eNextPtr) {
	eNextPtr = ePtr->nextPtr;
	TclDecrRefCount(ePtr->scriptPtr);
	ckfree(ePtr);
    }
    statePtr->scriptRecordPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Write --
 *
 *	Puts a sequence of bytes into an output buffer, may queue the buffer
 *	for output if it gets full, and also remembers whether the current
 *	buffer is ready e.g. if it contains a newline and we are in line
 *	buffering mode. Compensates stacking, i.e. will redirect the data from
 *	the specified channel to the topmost channel in a stack.
 *
 *	No encoding conversions are applied to the bytes being read.
 *
 * Results:
 *	The number of bytes written or -1 in case of error. If -1,
 *	Tcl_GetErrno will return the error code.
 *
 * Side effects:
 *	May buffer up output and may cause output to be produced on the
 *	channel.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Write(
    Tcl_Channel chan,		/* The channel to buffer output for. */
    const char *src,		/* Data to queue in output buffer. */
    int srcLen)			/* Length of data in bytes, or < 0 for
				 * strlen(). */
{
    /*
     * Always use the topmost channel of the stack
     */

    Channel *chanPtr;
    ChannelState *statePtr;	/* State info for channel */

    statePtr = ((Channel *) chan)->state;
    chanPtr = statePtr->topChanPtr;

    if (CheckChannelErrors(statePtr, TCL_WRITABLE) != 0) {
	return -1;
    }

    if (srcLen < 0) {
	srcLen = strlen(src);
    }
    if (WriteBytes(chanPtr, src, srcLen) < 0) {
	return -1;
    }
    return srcLen;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_WriteRaw --
 *
 *	Puts a sequence of bytes into an output buffer, may queue the buffer
 *	for output if it gets full, and also remembers whether the current
 *	buffer is ready e.g. if it contains a newline and we are in line
 *	buffering mode. Writes directly to the driver of the channel, does not
 *	compensate for stacking.
 *
 *	No encoding conversions are applied to the bytes being read.
 *
 * Results:
 *	The number of bytes written or -1 in case of error. If -1,
 *	Tcl_GetErrno will return the error code.
 *
 * Side effects:
 *	May buffer up output and may cause output to be produced on the
 *	channel.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_WriteRaw(
    Tcl_Channel chan,		/* The channel to buffer output for. */
    const char *src,		/* Data to queue in output buffer. */
    int srcLen)			/* Length of data in bytes, or < 0 for
				 * strlen(). */
{
    Channel *chanPtr = ((Channel *) chan);
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    int errorCode, written;

    if (CheckChannelErrors(statePtr, TCL_WRITABLE | CHANNEL_RAW_MODE) != 0) {
	return -1;
    }

    if (srcLen < 0) {
	srcLen = strlen(src);
    }

    /*
     * Go immediately to the driver, do all the error handling by ourselves.
     * The code was stolen from 'FlushChannel'.
     */

    written = ChanWrite(chanPtr, src, srcLen, &errorCode);
    if (written < 0) {
	Tcl_SetErrno(errorCode);
    }

    return written;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_WriteChars --
 *
 *	Takes a sequence of UTF-8 characters and converts them for output
 *	using the channel's current encoding, may queue the buffer for output
 *	if it gets full, and also remembers whether the current buffer is
 *	ready e.g. if it contains a newline and we are in line buffering
 *	mode. Compensates stacking, i.e. will redirect the data from the
 *	specified channel to the topmost channel in a stack.
 *
 * Results:
 *	The number of bytes written or -1 in case of error. If -1,
 *	Tcl_GetErrno will return the error code.
 *
 * Side effects:
 *	May buffer up output and may cause output to be produced on the
 *	channel.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_WriteChars(
    Tcl_Channel chan,		/* The channel to buffer output for. */
    const char *src,		/* UTF-8 characters to queue in output
				 * buffer. */
    int len)			/* Length of string in bytes, or < 0 for
				 * strlen(). */
{
    Channel *chanPtr = (Channel *) chan;
    ChannelState *statePtr = chanPtr->state;	/* State info for channel */
    int result;
    Tcl_Obj *objPtr;

    if (CheckChannelErrors(statePtr, TCL_WRITABLE) != 0) {
	return -1;
    }

    chanPtr = statePtr->topChanPtr;

    if (len < 0) {
	len = strlen(src);
    }
    if (statePtr->encoding) {
	return WriteChars(chanPtr, src, len);
    }

    /*
     * Inefficient way to convert UTF-8 to byte-array, but the code
     * parallels the way it is done for objects.  Special case for 1-byte
     * (used by eg [puts] for the \n) could be extended to more efficient
     * translation of the src string.
     */

    if ((len == 1) && (UCHAR(*src) < 0xC0)) {
	return WriteBytes(chanPtr, src, len);
    }

    objPtr = Tcl_NewStringObj(src, len);
    src = (char *) Tcl_GetByteArrayFromObj(objPtr, &len);
    result = WriteBytes(chanPtr, src, len);
    TclDecrRefCount(objPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_WriteObj --
 *
 *	Takes the Tcl object and queues its contents for output. If the
 *	encoding of the channel is NULL, takes the byte-array representation
 *	of the object and queues those bytes for output. Otherwise, takes the
 *	characters in the UTF-8 (string) representation of the object and
 *	converts them for output using the channel's current encoding. May
 *	flush internal buffers to output if one becomes full or is ready for
 *	some other reason, e.g. if it contains a newline and the channel is in
 *	line buffering mode.
 *
 * Results:
 *	The number of bytes written or -1 in case of error. If -1,
 *	Tcl_GetErrno() will return the error code.
 *
 * Side effects:
 *	May buffer up output and may cause output to be produced on the
 *	channel.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_WriteObj(
    Tcl_Channel chan,		/* The channel to buffer output for. */
    Tcl_Obj *objPtr)		/* The object to write. */
{
    /*
     * Always use the topmost channel of the stack
     */

    Channel *chanPtr;
    ChannelState *statePtr;	/* State info for channel */
    const char *src;
    int srcLen;

    statePtr = ((Channel *) chan)->state;
    chanPtr = statePtr->topChanPtr;

    if (CheckChannelErrors(statePtr, TCL_WRITABLE) != 0) {
	return -1;
    }
    if (statePtr->encoding == NULL) {
	src = (char *) Tcl_GetByteArrayFromObj(objPtr, &srcLen);
	return WriteBytes(chanPtr, src, srcLen);
    } else {
	src = TclGetStringFromObj(objPtr, &srcLen);
	return WriteChars(chanPtr, src, srcLen);
    }
}

static void
WillWrite(
    Channel *chanPtr)
{
    int inputBuffered;

    if ((chanPtr->typePtr->seekProc != NULL) &&
            ((inputBuffered = Tcl_InputBuffered((Tcl_Channel) chanPtr)) > 0)){
        int ignore;

        DiscardInputQueued(chanPtr->state, 0);
        ChanSeek(chanPtr, -inputBuffered, SEEK_CUR, &ignore);
    }
}

static int
WillRead(
    Channel *chanPtr)
{
    if (chanPtr->typePtr == NULL) {
	/* Prevent read attempts on a closed channel */
        DiscardInputQueued(chanPtr->state, 0);
	Tcl_SetErrno(EINVAL);
	return -1;
    }
    if ((chanPtr->typePtr->seekProc != NULL)
            && (Tcl_OutputBuffered((Tcl_Channel) chanPtr) > 0)) {

	/*
	 * CAVEAT - The assumption here is that FlushChannel() will
	 * push out the bytes of any writes that are in progress.
	 * Since this is a seekable channel, we assume it is not one
	 * that can block and force bg flushing.  Channels we know that
	 * can do that -- sockets, pipes -- are not seekable.  If the
	 * assumption is wrong, more drastic measures may be required here
	 * like temporarily setting the channel into blocking mode.
	 */

        if (FlushChannel(NULL, chanPtr, 0) != 0) {
            return -1;
        }
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Write --
 *
 *	Convert srcLen bytes starting at src according to encoding and write
 *	produced bytes into an output buffer, may queue the buffer for output
 *	if it gets full, and also remembers whether the current buffer is
 *	ready e.g. if it contains a newline and we are in line buffering mode.
 *
 * Results:
 *	The number of bytes written or -1 in case of error. If -1,
 *	Tcl_GetErrno will return the error code.
 *
 * Side effects:
 *	May buffer up output and may cause output to be produced on the
 *	channel.
 *
 *----------------------------------------------------------------------
 */

static int
Write(
    Channel *chanPtr,		/* The channel to buffer output for. */
    const char *src,		/* UTF-8 string to write. */
    int srcLen,			/* Length of UTF-8 string in bytes. */
    Tcl_Encoding encoding)
{
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    char *nextNewLine = NULL;
    int endEncoding, saved = 0, total = 0, flushed = 0, needNlFlush = 0;

    if (srcLen) {
        WillWrite(chanPtr);
    }

    /*
     * Write the terminated escape sequence even if srcLen is 0.
     */

    endEncoding = ((statePtr->outputEncodingFlags & TCL_ENCODING_END) != 0);

    if (GotFlag(statePtr, CHANNEL_LINEBUFFERED)
	    || (statePtr->outputTranslation != TCL_TRANSLATE_LF)) {
	nextNewLine = memchr(src, '\n', srcLen);
    }

    while (srcLen + saved + endEncoding > 0) {
	ChannelBuffer *bufPtr;
	char *dst, safe[BUFFER_PADDING];
	int result, srcRead, dstLen, dstWrote, srcLimit = srcLen;

	if (nextNewLine) {
	    srcLimit = nextNewLine - src;
	}

	/* Get space to write into */
	bufPtr = statePtr->curOutPtr;
	if (bufPtr == NULL) {
	    bufPtr = AllocChannelBuffer(statePtr->bufSize);
	    statePtr->curOutPtr = bufPtr;
	}
	if (saved) {
	    /*
	     * Here's some translated bytes left over from the last buffer
	     * that we need to stick at the beginning of this buffer.
	     */

	    memcpy(InsertPoint(bufPtr), safe, (size_t) saved);
	    bufPtr->nextAdded += saved;
	    saved = 0;
	}
	PreserveChannelBuffer(bufPtr);
	dst = InsertPoint(bufPtr);
	dstLen = SpaceLeft(bufPtr);

	result = Tcl_UtfToExternal(NULL, encoding, src, srcLimit,
		statePtr->outputEncodingFlags,
		&statePtr->outputEncodingState, dst,
		dstLen + BUFFER_PADDING, &srcRead, &dstWrote, NULL);

	/* See chan-io-1.[89]. Tcl Bug 506297. */
	statePtr->outputEncodingFlags &= ~TCL_ENCODING_START;

	if ((result != TCL_OK) && (srcRead + dstWrote == 0)) {
	    /* We're reading from invalid/incomplete UTF-8 */
	    ReleaseChannelBuffer(bufPtr);
	    if (total == 0) {
		Tcl_SetErrno(EINVAL);
		return -1;
	    }
	    break;
	}

	bufPtr->nextAdded += dstWrote;
	src += srcRead;
	srcLen -= srcRead;
	total += dstWrote;
	dst += dstWrote;
	dstLen -= dstWrote;

	if (src == nextNewLine && dstLen > 0) {
	    static char crln[3] = "\r\n";
	    char *nl = NULL;
	    int nlLen = 0;

	    switch (statePtr->outputTranslation) {
	    case TCL_TRANSLATE_LF:
		nl = crln + 1;
		nlLen = 1;
		break;
	    case TCL_TRANSLATE_CR:
		nl = crln;
		nlLen = 1;
		break;
	    case TCL_TRANSLATE_CRLF:
		nl = crln;
		nlLen = 2;
		break;
	    default:
		Tcl_Panic("unknown output translation requested");
		break;
	    }

	    result |= Tcl_UtfToExternal(NULL, encoding, nl, nlLen,
		statePtr->outputEncodingFlags,
		&statePtr->outputEncodingState, dst,
		dstLen + BUFFER_PADDING, &srcRead, &dstWrote, NULL);

	    assert (srcRead == nlLen);

	    bufPtr->nextAdded += dstWrote;
	    src++;
	    srcLen--;
	    total += dstWrote;
	    dst += dstWrote;
	    dstLen -= dstWrote;
	    nextNewLine = memchr(src, '\n', srcLen);
	    needNlFlush = 1;
	}

	if (IsBufferOverflowing(bufPtr)) {
	    /*
	     * When translating from UTF-8 to external encoding, we
	     * allowed the translation to produce a character that crossed
	     * the end of the output buffer, so that we would get a
	     * completely full buffer before flushing it. The extra bytes
	     * will be moved to the beginning of the next buffer.
	     */

	    saved = -SpaceLeft(bufPtr);
	    memcpy(safe, dst + dstLen, (size_t) saved);
	    bufPtr->nextAdded = bufPtr->bufLength;
	}

	if ((srcLen + saved == 0) && (result == TCL_OK)) {
	    endEncoding = 0;
	}

	if (IsBufferFull(bufPtr)) {
	    if (FlushChannel(NULL, chanPtr, 0) != 0) {
		ReleaseChannelBuffer(bufPtr);
		return -1;
	    }
	    flushed += statePtr->bufSize;

	    /*
 	     * We just flushed.  So if we have needNlFlush set to record
 	     * that we need to flush because theres a (translated) newline
 	     * in the buffer, that's likely not true any more.  But there
 	     * is a tricky exception.  If we have saved bytes that did not
 	     * really get flushed and those bytes came from a translation
 	     * of a newline as the last thing taken from the src array,
 	     * then needNlFlush needs to remain set to flag that the
 	     * next buffer still needs a newline flush.
 	     */
	    if (needNlFlush && (saved == 0 || src[-1] != '\n')) {
		needNlFlush = 0;
	    }
	}
	ReleaseChannelBuffer(bufPtr);
    }
    if ((flushed < total) && (GotFlag(statePtr, CHANNEL_UNBUFFERED) ||
	    (needNlFlush && GotFlag(statePtr, CHANNEL_LINEBUFFERED)))) {
	if (FlushChannel(NULL, chanPtr, 0) != 0) {
	    return -1;
	}
    }

    return total;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_Gets --
 *
 *	Reads a complete line of input from the channel into a Tcl_DString.
 *
 * Results:
 *	Length of line read (in characters) or -1 if error, EOF, or blocked.
 *	If -1, use Tcl_GetErrno() to retrieve the POSIX error code for the
 *	error or condition that occurred.
 *
 * Side effects:
 *	May flush output on the channel. May cause input to be consumed from
 *	the channel.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_Gets(
    Tcl_Channel chan,		/* Channel from which to read. */
    Tcl_DString *lineRead)	/* The line read will be appended to this
				 * DString as UTF-8 characters. The caller
				 * must have initialized it and is responsible
				 * for managing the storage. */
{
    Tcl_Obj *objPtr;
    int charsStored;

    TclNewObj(objPtr);
    charsStored = Tcl_GetsObj(chan, objPtr);
    if (charsStored > 0) {
	TclDStringAppendObj(lineRead, objPtr);
    }
    TclDecrRefCount(objPtr);
    return charsStored;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_GetsObj --
 *
 *	Accumulate input from the input channel until end-of-line or
 *	end-of-file has been seen. Bytes read from the input channel are
 *	converted to UTF-8 using the encoding specified by the channel.
 *
 * Results:
 *	Number of characters accumulated in the object or -1 if error,
 *	blocked, or EOF. If -1, use Tcl_GetErrno() to retrieve the POSIX error
 *	code for the error or condition that occurred.
 *
 * Side effects:
 *	Consumes input from the channel.
 *
 *	On reading EOF, leave channel pointing at EOF char. On reading EOL,
 *	leave channel pointing after EOL, but don't return EOL in dst buffer.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_GetsObj(
    Tcl_Channel chan,		/* Channel from which to read. */
    Tcl_Obj *objPtr)		/* The line read will be appended to this
				 * object as UTF-8 characters. */
{
    GetsState gs;
    Channel *chanPtr = (Channel *) chan;
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    ChannelBuffer *bufPtr;
    int inEofChar, skip, copiedTotal, oldLength, oldFlags, oldRemoved;
    Tcl_Encoding encoding;
    char *dst, *dstEnd, *eol, *eof;
    Tcl_EncodingState oldState;

    if (CheckChannelErrors(statePtr, TCL_READABLE) != 0) {
	return -1;
    }

    /*
     * If we're sitting ready to read the eofchar, there's no need to
     * do it.
     */

    if (GotFlag(statePtr, CHANNEL_STICKY_EOF)) {
	SetFlag(statePtr, CHANNEL_EOF);
	assert( statePtr->inputEncodingFlags & TCL_ENCODING_END );
	assert( !GotFlag(statePtr, CHANNEL_BLOCKED|INPUT_SAW_CR) );

	/* TODO: Do we need this? */
	UpdateInterest(chanPtr);
	return -1;
    }

    /*
     * A binary version of Tcl_GetsObj. This could also handle encodings that
     * are ascii-7 pure (iso8859, utf-8, ...) with a final encoding conversion
     * done on objPtr.
     */

    if ((statePtr->encoding == NULL)
	    && ((statePtr->inputTranslation == TCL_TRANSLATE_LF)
		    || (statePtr->inputTranslation == TCL_TRANSLATE_CR))) {
	return TclGetsObjBinary(chan, objPtr);
    }

    /*
     * This operation should occur at the top of a channel stack.
     */

    chanPtr = statePtr->topChanPtr;
    TclChannelPreserve((Tcl_Channel)chanPtr);

    bufPtr = statePtr->inQueueHead;
    encoding = statePtr->encoding;

    /*
     * Preserved so we can restore the channel's state in case we don't find a
     * newline in the available input.
     */

    TclGetStringFromObj(objPtr, &oldLength);
    oldFlags = statePtr->inputEncodingFlags;
    oldState = statePtr->inputEncodingState;
    oldRemoved = BUFFER_PADDING;
    if (bufPtr != NULL) {
	oldRemoved = bufPtr->nextRemoved;
    }

    /*
     * If there is no encoding, use "iso8859-1" -- Tcl_GetsObj() doesn't
     * produce ByteArray objects.
     */

    if (encoding == NULL) {
	encoding = GetBinaryEncoding();
    }

    /*
     * Object used by FilterInputBytes to keep track of how much data has been
     * consumed from the channel buffers.
     */

    gs.objPtr		= objPtr;
    gs.dstPtr		= &dst;
    gs.encoding		= encoding;
    gs.bufPtr		= bufPtr;
    gs.state		= oldState;
    gs.rawRead		= 0;
    gs.bytesWrote	= 0;
    gs.charsWrote	= 0;
    gs.totalChars	= 0;

    dst = objPtr->bytes + oldLength;
    dstEnd = dst;

    skip = 0;
    eof = NULL;
    inEofChar = statePtr->inEofChar;

    ResetFlag(statePtr, CHANNEL_BLOCKED);
    while (1) {
	if (dst >= dstEnd) {
	    if (FilterInputBytes(chanPtr, &gs) != 0) {
		goto restore;
	    }
	    dstEnd = dst + gs.bytesWrote;
	}

	/*
	 * Remember if EOF char is seen, then look for EOL anyhow, because the
	 * EOL might be before the EOF char.
	 */

	if (inEofChar != '\0') {
	    for (eol = dst; eol < dstEnd; eol++) {
		if (*eol == inEofChar) {
		    dstEnd = eol;
		    eof = eol;
		    break;
		}
	    }
	}

	/*
	 * On EOL, leave current file position pointing after the EOL, but
	 * don't store the EOL in the output string.
	 */

	switch (statePtr->inputTranslation) {
	case TCL_TRANSLATE_LF:
	    for (eol = dst; eol < dstEnd; eol++) {
		if (*eol == '\n') {
		    skip = 1;
		    goto gotEOL;
		}
	    }
	    break;
	case TCL_TRANSLATE_CR:
	    for (eol = dst; eol < dstEnd; eol++) {
		if (*eol == '\r') {
		    skip = 1;
		    goto gotEOL;
		}
	    }
	    break;
	case TCL_TRANSLATE_CRLF:
	    for (eol = dst; eol < dstEnd; eol++) {
		if (*eol == '\r') {
		    eol++;

		    /*
		     * If a CR is at the end of the buffer, then check for a
		     * LF at the begining of the next buffer, unless EOF char
		     * was found already.
		     */

		    if (eol >= dstEnd) {
			int offset;

			if (eol != eof) {
			    offset = eol - objPtr->bytes;
			    dst = dstEnd;
			    if (FilterInputBytes(chanPtr, &gs) != 0) {
				goto restore;
			    }
			    dstEnd = dst + gs.bytesWrote;
			    eol = objPtr->bytes + offset;
			}
			if (eol >= dstEnd) {
			    skip = 0;
			    goto gotEOL;
			}
		    }
		    if (*eol == '\n') {
			eol--;
			skip = 2;
			goto gotEOL;
		    }
		}
	    }
	    break;
	case TCL_TRANSLATE_AUTO:
	    eol = dst;
	    skip = 1;
	    if (GotFlag(statePtr, INPUT_SAW_CR)) {
		ResetFlag(statePtr, INPUT_SAW_CR);
		if ((eol < dstEnd) && (*eol == '\n')) {
		    /*
		     * Skip the raw bytes that make up the '\n'.
		     */

		    char tmp[TCL_UTF_MAX];
		    int rawRead;

		    bufPtr = gs.bufPtr;
		    Tcl_ExternalToUtf(NULL, gs.encoding, RemovePoint(bufPtr),
			    gs.rawRead, statePtr->inputEncodingFlags
				| TCL_ENCODING_NO_TERMINATE, &gs.state, tmp,
			    TCL_UTF_MAX, &rawRead, NULL, NULL);
		    bufPtr->nextRemoved += rawRead;
		    gs.rawRead -= rawRead;
		    gs.bytesWrote--;
		    gs.charsWrote--;
		    memmove(dst, dst + 1, (size_t) (dstEnd - dst));
		    dstEnd--;
		}
	    }
	    for (eol = dst; eol < dstEnd; eol++) {
		if (*eol == '\r') {
		    eol++;
		    if (eol == dstEnd) {
			/*
			 * If buffer ended on \r, peek ahead to see if a \n is
			 * available, unless EOF char was found already.
			 */

			if (eol != eof) {
			    int offset;

			    offset = eol - objPtr->bytes;
			    dst = dstEnd;
			    PeekAhead(chanPtr, &dstEnd, &gs);
			    eol = objPtr->bytes + offset;
			}

			if (eol >= dstEnd) {
			    eol--;
			    SetFlag(statePtr, INPUT_SAW_CR);
			    goto gotEOL;
			}
		    }
		    if (*eol == '\n') {
			skip++;
		    }
		    eol--;
		    goto gotEOL;
		} else if (*eol == '\n') {
		    goto gotEOL;
		}
	    }
	}
	if (eof != NULL) {
	    /*
	     * EOF character was seen. On EOF, leave current file position
	     * pointing at the EOF character, but don't store the EOF
	     * character in the output string.
	     */

	    dstEnd = eof;
	    SetFlag(statePtr, CHANNEL_EOF | CHANNEL_STICKY_EOF);
	    statePtr->inputEncodingFlags |= TCL_ENCODING_END;
	    ResetFlag(statePtr, CHANNEL_BLOCKED|INPUT_SAW_CR);
	}
	if (GotFlag(statePtr, CHANNEL_EOF)) {
	    skip = 0;
	    eol = dstEnd;
	    if (eol == objPtr->bytes + oldLength) {
		/*
		 * If we didn't append any bytes before encountering EOF,
		 * caller needs to see -1.
		 */

		Tcl_SetObjLength(objPtr, oldLength);
		CommonGetsCleanup(chanPtr);
		copiedTotal = -1;
		ResetFlag(statePtr, CHANNEL_BLOCKED);
		goto done;
	    }
	    goto gotEOL;
	}
	dst = dstEnd;
    }

    /*
     * Found EOL or EOF, but the output buffer may now contain too many UTF-8
     * characters. We need to know how many raw bytes correspond to the number
     * of UTF-8 characters we want, plus how many raw bytes correspond to the
     * character(s) making up EOL (if any), so we can remove the correct
     * number of bytes from the channel buffer.
     */

  gotEOL:
    /*
     * Regenerate the top channel, in case it was changed due to
     * self-modifying reflected transforms.
     */

    if (chanPtr != statePtr->topChanPtr) {
	TclChannelRelease((Tcl_Channel)chanPtr);
	chanPtr = statePtr->topChanPtr;
	TclChannelPreserve((Tcl_Channel)chanPtr);
    }

    bufPtr = gs.bufPtr;
    if (bufPtr == NULL) {
	Tcl_Panic("Tcl_GetsObj: gotEOL reached with bufPtr==NULL");
    }
    statePtr->inputEncodingState = gs.state;
    Tcl_ExternalToUtf(NULL, gs.encoding, RemovePoint(bufPtr), gs.rawRead,
	    statePtr->inputEncodingFlags | TCL_ENCODING_NO_TERMINATE,
	    &statePtr->inputEncodingState, dst,
	    eol - dst + skip + TCL_UTF_MAX - 1, &gs.rawRead, NULL,
	    &gs.charsWrote);
    bufPtr->nextRemoved += gs.rawRead;

    /*
     * Recycle all the emptied buffers.
     */

    Tcl_SetObjLength(objPtr, eol - objPtr->bytes);
    CommonGetsCleanup(chanPtr);
    ResetFlag(statePtr, CHANNEL_BLOCKED);
    copiedTotal = gs.totalChars + gs.charsWrote - skip;
    goto done;

    /*
     * Couldn't get a complete line. This only happens if we get a error
     * reading from the channel or we are non-blocking and there wasn't an EOL
     * or EOF in the data available.
     */

  restore:
    /*
     * Regenerate the top channel, in case it was changed due to
     * self-modifying reflected transforms.
     */
    if (chanPtr != statePtr->topChanPtr) {
	TclChannelRelease((Tcl_Channel)chanPtr);
	chanPtr = statePtr->topChanPtr;
	TclChannelPreserve((Tcl_Channel)chanPtr);
    }
    bufPtr = statePtr->inQueueHead;
    if (bufPtr != NULL) {
	bufPtr->nextRemoved = oldRemoved;
	bufPtr = bufPtr->nextPtr;
    }

    for ( ; bufPtr != NULL; bufPtr = bufPtr->nextPtr) {
	bufPtr->nextRemoved = BUFFER_PADDING;
    }
    CommonGetsCleanup(chanPtr);

    statePtr->inputEncodingState = oldState;
    statePtr->inputEncodingFlags = oldFlags;
    Tcl_SetObjLength(objPtr, oldLength);

    /*
     * We didn't get a complete line so we need to indicate to UpdateInterest
     * that the gets blocked. It will wait for more data instead of firing a
     * timer, avoiding a busy wait. This is where we are assuming that the
     * next operation is a gets. No more file events will be delivered on this
     * channel until new data arrives or some operation is performed on the
     * channel (e.g. gets, read, fconfigure) that changes the blocking state.
     * Note that this means a file event will not be delivered even though a
     * read would be able to consume the buffered data.
     */

    SetFlag(statePtr, CHANNEL_NEED_MORE_DATA);
    copiedTotal = -1;

    /*
     * Update the notifier state so we don't block while there is still data
     * in the buffers.
     */

  done:
	assert(!GotFlag(statePtr, CHANNEL_EOF)
		|| GotFlag(statePtr, CHANNEL_STICKY_EOF)
		|| Tcl_InputBuffered((Tcl_Channel)chanPtr) == 0);

	assert( !(GotFlag(statePtr, CHANNEL_EOF|CHANNEL_BLOCKED)
		== (CHANNEL_EOF|CHANNEL_BLOCKED)) );

    /*
     * Regenerate the top channel, in case it was changed due to
     * self-modifying reflected transforms.
     */
    if (chanPtr != statePtr->topChanPtr) {
	TclChannelRelease((Tcl_Channel)chanPtr);
	chanPtr = statePtr->topChanPtr;
	TclChannelPreserve((Tcl_Channel)chanPtr);
    }
    UpdateInterest(chanPtr);
    TclChannelRelease((Tcl_Channel)chanPtr);
    return copiedTotal;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclGetsObjBinary --
 *
 *	A variation of Tcl_GetsObj that works directly on the buffers until
 *	end-of-line or end-of-file has been seen. Bytes read from the input
 *	channel return as a ByteArray obj.
 *
 *	WARNING!  The notion of "binary" used here is different from
 *	notions of "binary" used in other places.  In particular, this
 *	"binary" routine may be called when an -eofchar is set on the
 * 	channel.
 *
 * Results:
 *	Number of characters accumulated in the object or -1 if error,
 *	blocked, or EOF. If -1, use Tcl_GetErrno() to retrieve the POSIX error
 *	code for the error or condition that occurred.
 *
 * Side effects:
 *	Consumes input from the channel.
 *
 *	On reading EOF, leave channel pointing at EOF char. On reading EOL,
 *	leave channel pointing after EOL, but don't return EOL in dst buffer.
 *
 *---------------------------------------------------------------------------
 */

static int
TclGetsObjBinary(
    Tcl_Channel chan,		/* Channel from which to read. */
    Tcl_Obj *objPtr)		/* The line read will be appended to this
				 * object as UTF-8 characters. */
{
    Channel *chanPtr = (Channel *) chan;
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    ChannelBuffer *bufPtr;
    int inEofChar, skip, copiedTotal, oldLength, oldFlags, oldRemoved;
    int rawLen, byteLen, eolChar;
    unsigned char *dst, *dstEnd, *eol, *eof, *byteArray;

    /*
     * This operation should occur at the top of a channel stack.
     */

    chanPtr = statePtr->topChanPtr;
    TclChannelPreserve((Tcl_Channel)chanPtr);

    bufPtr = statePtr->inQueueHead;

    /*
     * Preserved so we can restore the channel's state in case we don't find a
     * newline in the available input.
     */

    byteArray = Tcl_GetByteArrayFromObj(objPtr, &byteLen);
    oldFlags = statePtr->inputEncodingFlags;
    oldRemoved = BUFFER_PADDING;
    oldLength = byteLen;
    if (bufPtr != NULL) {
	oldRemoved = bufPtr->nextRemoved;
    }

    rawLen = 0;
    skip = 0;
    eof = NULL;
    inEofChar = statePtr->inEofChar;

    /*
     * Only handle TCL_TRANSLATE_LF and TCL_TRANSLATE_CR.
     */

    eolChar = (statePtr->inputTranslation == TCL_TRANSLATE_LF) ? '\n' : '\r';

    ResetFlag(statePtr, CHANNEL_BLOCKED);
    while (1) {
	/*
	 * Subtract the number of bytes that were removed from channel
	 * buffer during last call.
	 */

	if (bufPtr != NULL) {
	    bufPtr->nextRemoved += rawLen;
	    if (!IsBufferReady(bufPtr)) {
		bufPtr = bufPtr->nextPtr;
	    }
	}

	if ((bufPtr == NULL) || (bufPtr->nextAdded == BUFFER_PADDING)) {
	    /*
	     * All channel buffers were exhausted and the caller still
	     * hasn't seen EOL. Need to read more bytes from the channel
	     * device. Side effect is to allocate another channel buffer.
	     */
	    if (GetInput(chanPtr) != 0) {
		goto restore;
	    }
	    bufPtr = statePtr->inQueueTail;
	    if (bufPtr == NULL) {
		goto restore;
	    }
	} else {
	    /*
	     * Incoming CHANNEL_STICKY_EOF is filtered out on entry.
	     * A new CHANNEL_STICKY_EOF set in this routine leads to
	     * return before coming back here.  When we are not dealing
	     * with CHANNEL_STICKY_EOF, a CHANNEL_EOF implies an
	     * empty buffer.  Here the buffer is non-empty so we know
	     * we're a non-EOF */

	    assert ( !GotFlag(statePtr, CHANNEL_STICKY_EOF) );
	    assert ( !GotFlag(statePtr, CHANNEL_EOF) );
	}

	dst = (unsigned char *) RemovePoint(bufPtr);
	dstEnd = dst + BytesLeft(bufPtr);

	/*
	 * Remember if EOF char is seen, then look for EOL anyhow, because the
	 * EOL might be before the EOF char.
	 * XXX - in the binary case, consider coincident search for eol/eof.
	 */

	if (inEofChar != '\0') {
	    for (eol = dst; eol < dstEnd; eol++) {
		if (*eol == inEofChar) {
		    dstEnd = eol;
		    eof = eol;
		    break;
		}
	    }
	}

	/*
	 * On EOL, leave current file position pointing after the EOL, but
	 * don't store the EOL in the output string.
	 */

	for (eol = dst; eol < dstEnd; eol++) {
	    if (*eol == eolChar) {
		skip = 1;
		goto gotEOL;
	    }
	}
	if (eof != NULL) {
	    /*
	     * EOF character was seen. On EOF, leave current file position
	     * pointing at the EOF character, but don't store the EOF
	     * character in the output string.
	     */

	    SetFlag(statePtr, CHANNEL_EOF | CHANNEL_STICKY_EOF);
	    statePtr->inputEncodingFlags |= TCL_ENCODING_END;
	    ResetFlag(statePtr, CHANNEL_BLOCKED|INPUT_SAW_CR);
	}
	if (GotFlag(statePtr, CHANNEL_EOF)) {
	    skip = 0;
	    eol = dstEnd;
	    if ((dst == dstEnd) && (byteLen == oldLength)) {
		/*
		 * If we didn't append any bytes before encountering EOF,
		 * caller needs to see -1.
		 */

		byteArray = Tcl_SetByteArrayLength(objPtr, oldLength);
		CommonGetsCleanup(chanPtr);
		copiedTotal = -1;
		ResetFlag(statePtr, CHANNEL_BLOCKED);
		goto done;
	    }
	    goto gotEOL;
	}
	if (GotFlag(statePtr, CHANNEL_BLOCKED|CHANNEL_NONBLOCKING)
		== (CHANNEL_BLOCKED|CHANNEL_NONBLOCKING)) {
	    goto restore;
	}

	/*
	 * Copy bytes from the channel buffer to the ByteArray.
	 * This may realloc space, so keep track of result.
	 */

	rawLen = dstEnd - dst;
	byteArray = Tcl_SetByteArrayLength(objPtr, byteLen + rawLen);
	memcpy(byteArray + byteLen, dst, (size_t) rawLen);
	byteLen += rawLen;
    }

    /*
     * Found EOL or EOF, but the output buffer may now contain too many bytes.
     * We need to know how many bytes correspond to the number we want, so we
     * can remove the correct number of bytes from the channel buffer.
     */

  gotEOL:
    if (bufPtr == NULL) {
	Tcl_Panic("TclGetsObjBinary: gotEOL reached with bufPtr==NULL");
    }

    rawLen = eol - dst;
    byteArray = Tcl_SetByteArrayLength(objPtr, byteLen + rawLen);
    memcpy(byteArray + byteLen, dst, (size_t) rawLen);
    byteLen += rawLen;
    bufPtr->nextRemoved += rawLen + skip;

    /*
     * Convert the buffer if there was an encoding.
     * XXX - unimplemented.
     */

    if (statePtr->encoding != NULL) {
    }

    /*
     * Recycle all the emptied buffers.
     */

    CommonGetsCleanup(chanPtr);
    ResetFlag(statePtr, CHANNEL_BLOCKED);
    copiedTotal = byteLen;
    goto done;

    /*
     * Couldn't get a complete line. This only happens if we get a error
     * reading from the channel or we are non-blocking and there wasn't an EOL
     * or EOF in the data available.
     */

  restore:
    bufPtr = statePtr->inQueueHead;
    if (bufPtr) {
	bufPtr->nextRemoved = oldRemoved;
	bufPtr = bufPtr->nextPtr;
    }

    for ( ; bufPtr != NULL; bufPtr = bufPtr->nextPtr) {
	bufPtr->nextRemoved = BUFFER_PADDING;
    }
    CommonGetsCleanup(chanPtr);

    statePtr->inputEncodingFlags = oldFlags;
    byteArray = Tcl_SetByteArrayLength(objPtr, oldLength);

    /*
     * We didn't get a complete line so we need to indicate to UpdateInterest
     * that the gets blocked. It will wait for more data instead of firing a
     * timer, avoiding a busy wait. This is where we are assuming that the
     * next operation is a gets. No more file events will be delivered on this
     * channel until new data arrives or some operation is performed on the
     * channel (e.g. gets, read, fconfigure) that changes the blocking state.
     * Note that this means a file event will not be delivered even though a
     * read would be able to consume the buffered data.
     */

    SetFlag(statePtr, CHANNEL_NEED_MORE_DATA);
    copiedTotal = -1;

    /*
     * Update the notifier state so we don't block while there is still data
     * in the buffers.
     */

  done:
	assert(!GotFlag(statePtr, CHANNEL_EOF)
		|| GotFlag(statePtr, CHANNEL_STICKY_EOF)
		|| Tcl_InputBuffered((Tcl_Channel)chanPtr) == 0);
	assert( !(GotFlag(statePtr, CHANNEL_EOF|CHANNEL_BLOCKED)
		== (CHANNEL_EOF|CHANNEL_BLOCKED)) );
    UpdateInterest(chanPtr);
    TclChannelRelease((Tcl_Channel)chanPtr);
    return copiedTotal;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeBinaryEncoding --
 *
 *	Frees any "iso8859-1" Tcl_Encoding created by [gets] on a binary
 *	channel in a thread as part of that thread's finalization.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
FreeBinaryEncoding(
    ClientData dummy)	/* Not used */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (tsdPtr->binaryEncoding != NULL) {
	Tcl_FreeEncoding(tsdPtr->binaryEncoding);
	tsdPtr->binaryEncoding = NULL;
    }
}

static Tcl_Encoding
GetBinaryEncoding()
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (tsdPtr->binaryEncoding == NULL) {
	tsdPtr->binaryEncoding = Tcl_GetEncoding(NULL, "iso8859-1");
	Tcl_CreateThreadExitHandler(FreeBinaryEncoding, NULL);
    }
    if (tsdPtr->binaryEncoding == NULL) {
	Tcl_Panic("binary encoding is not available");
    }
    return tsdPtr->binaryEncoding;
}

/*
 *---------------------------------------------------------------------------
 *
 * FilterInputBytes --
 *
 *	Helper function for Tcl_GetsObj. Produces UTF-8 characters from raw
 *	bytes read from the channel.
 *
 *	Consumes available bytes from channel buffers. When channel buffers
 *	are exhausted, reads more bytes from channel device into a new channel
 *	buffer. It is the caller's responsibility to free the channel buffers
 *	that have been exhausted.
 *
 * Results:
 *	The return value is -1 if there was an error reading from the channel,
 *	0 otherwise.
 *
 * Side effects:
 *	Status object keeps track of how much data from channel buffers has
 *	been consumed and where UTF-8 bytes should be stored.
 *
 *---------------------------------------------------------------------------
 */

static int
FilterInputBytes(
    Channel *chanPtr,		/* Channel to read. */
    GetsState *gsPtr)		/* Current state of gets operation. */
{
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    ChannelBuffer *bufPtr;
    char *raw, *dst;
    int offset, toRead, dstNeeded, spaceLeft, result, rawLen;
    Tcl_Obj *objPtr;
#define ENCODING_LINESIZE 20	/* Lower bound on how many bytes to convert at
				 * a time. Since we don't know a priori how
				 * many bytes of storage this many source
				 * bytes will use, we actually need at least
				 * ENCODING_LINESIZE * TCL_MAX_UTF bytes of
				 * room. */

    objPtr = gsPtr->objPtr;

    /*
     * Subtract the number of bytes that were removed from channel buffer
     * during last call.
     */

    bufPtr = gsPtr->bufPtr;
    if (bufPtr != NULL) {
	bufPtr->nextRemoved += gsPtr->rawRead;
	if (!IsBufferReady(bufPtr)) {
	    bufPtr = bufPtr->nextPtr;
	}
    }
    gsPtr->totalChars += gsPtr->charsWrote;

    if ((bufPtr == NULL) || (bufPtr->nextAdded == BUFFER_PADDING)) {
	/*
	 * All channel buffers were exhausted and the caller still hasn't seen
	 * EOL. Need to read more bytes from the channel device. Side effect
	 * is to allocate another channel buffer.
	 */

    read:
	if (GotFlag(statePtr, CHANNEL_NONBLOCKING|CHANNEL_BLOCKED)
		== (CHANNEL_NONBLOCKING|CHANNEL_BLOCKED)) {
	    gsPtr->charsWrote = 0;
	    gsPtr->rawRead = 0;
	    return -1;
	}
	if (GetInput(chanPtr) != 0) {
	    gsPtr->charsWrote = 0;
	    gsPtr->rawRead = 0;
	    return -1;
	}
	bufPtr = statePtr->inQueueTail;
	gsPtr->bufPtr = bufPtr;
	if (bufPtr == NULL) {
	    gsPtr->charsWrote = 0;
	    gsPtr->rawRead = 0;
	    return -1;
	}
    } else {
	/*
	 * Incoming CHANNEL_STICKY_EOF is filtered out on entry.
	 * A new CHANNEL_STICKY_EOF set in this routine leads to
	 * return before coming back here.  When we are not dealing
	 * with CHANNEL_STICKY_EOF, a CHANNEL_EOF implies an
	 * empty buffer.  Here the buffer is non-empty so we know
	 * we're a non-EOF */

	assert ( !GotFlag(statePtr, CHANNEL_STICKY_EOF) );
	assert ( !GotFlag(statePtr, CHANNEL_EOF) );
    }

    /*
     * Convert some of the bytes from the channel buffer to UTF-8. Space in
     * objPtr's string rep is used to hold the UTF-8 characters. Grow the
     * string rep if we need more space.
     */

    raw = RemovePoint(bufPtr);
    rawLen = BytesLeft(bufPtr);

    dst = *gsPtr->dstPtr;
    offset = dst - objPtr->bytes;
    toRead = ENCODING_LINESIZE;
    if (toRead > rawLen) {
	toRead = rawLen;
    }
    dstNeeded = toRead * TCL_UTF_MAX;
    spaceLeft = objPtr->length - offset;
    if (dstNeeded > spaceLeft) {
	int length = offset + ((offset < dstNeeded) ? dstNeeded : offset);

	if (Tcl_AttemptSetObjLength(objPtr, length) == 0) {
	    length = offset + dstNeeded;
	    if (Tcl_AttemptSetObjLength(objPtr, length) == 0) {
		dstNeeded = TCL_UTF_MAX - 1 + toRead;
		length = offset + dstNeeded;
		Tcl_SetObjLength(objPtr, length);
	    }
	}
	spaceLeft = length - offset;
	dst = objPtr->bytes + offset;
	*gsPtr->dstPtr = dst;
    }
    gsPtr->state = statePtr->inputEncodingState;
    result = Tcl_ExternalToUtf(NULL, gsPtr->encoding, raw, rawLen,
	    statePtr->inputEncodingFlags | TCL_ENCODING_NO_TERMINATE,
	    &statePtr->inputEncodingState, dst, spaceLeft, &gsPtr->rawRead,
	    &gsPtr->bytesWrote, &gsPtr->charsWrote);

    /*
     * Make sure that if we go through 'gets', that we reset the
     * TCL_ENCODING_START flag still. [Bug #523988]
     */

    statePtr->inputEncodingFlags &= ~TCL_ENCODING_START;

    if (result == TCL_CONVERT_MULTIBYTE) {
	/*
	 * The last few bytes in this channel buffer were the start of a
	 * multibyte sequence. If this buffer was full, then move them to the
	 * next buffer so the bytes will be contiguous.
	 */

	ChannelBuffer *nextPtr;
	int extra;

	nextPtr = bufPtr->nextPtr;
	if (!IsBufferFull(bufPtr)) {
	    if (gsPtr->rawRead > 0) {
		/*
		 * Some raw bytes were converted to UTF-8. Fall through,
		 * returning those UTF-8 characters because a EOL might be
		 * present in them.
		 */
	    } else if (GotFlag(statePtr, CHANNEL_EOF)) {
		/*
		 * There was a partial character followed by EOF on the
		 * device. Fall through, returning that nothing was found.
		 */

		bufPtr->nextRemoved = bufPtr->nextAdded;
	    } else {
		/*
		 * There are no more cached raw bytes left. See if we can get
		 * some more, but avoid blocking on a non-blocking channel.
		 */

		goto read;
	    }
	} else {
	    if (nextPtr == NULL) {
		nextPtr = AllocChannelBuffer(statePtr->bufSize);
		bufPtr->nextPtr = nextPtr;
		statePtr->inQueueTail = nextPtr;
	    }
	    extra = rawLen - gsPtr->rawRead;
	    memcpy(nextPtr->buf + (BUFFER_PADDING - extra),
		    raw + gsPtr->rawRead, (size_t) extra);
	    nextPtr->nextRemoved -= extra;
	    bufPtr->nextAdded -= extra;
	}
    }

    gsPtr->bufPtr = bufPtr;
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * PeekAhead --
 *
 *	Helper function used by Tcl_GetsObj(). Called when we've seen a \r at
 *	the end of the UTF-8 string and want to look ahead one character to
 *	see if it is a \n.
 *
 * Results:
 *	*gsPtr->dstPtr is filled with a pointer to the start of the range of
 *	UTF-8 characters that were found by peeking and *dstEndPtr is filled
 *	with a pointer to the bytes just after the end of the range.
 *
 * Side effects:
 *	If no more raw bytes were available in one of the channel buffers,
 *	tries to perform a non-blocking read to get more bytes from the
 *	channel device.
 *
 *---------------------------------------------------------------------------
 */

static void
PeekAhead(
    Channel *chanPtr,		/* The channel to read. */
    char **dstEndPtr,		/* Filled with pointer to end of new range of
				 * UTF-8 characters. */
    GetsState *gsPtr)		/* Current state of gets operation. */
{
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    ChannelBuffer *bufPtr;
    Tcl_DriverBlockModeProc *blockModeProc;
    int bytesLeft;

    bufPtr = gsPtr->bufPtr;

    /*
     * If there's any more raw input that's still buffered, we'll peek into
     * that. Otherwise, only get more data from the channel driver if it looks
     * like there might actually be more data. The assumption is that if the
     * channel buffer is filled right up to the end, then there might be more
     * data to read.
     */

    blockModeProc = NULL;
    if (bufPtr->nextPtr == NULL) {
	bytesLeft = BytesLeft(bufPtr) - gsPtr->rawRead;
	if (bytesLeft == 0) {
	    if (!IsBufferFull(bufPtr)) {
		/*
		 * Don't peek ahead if last read was short read.
		 */

		goto cleanup;
	    }
	    if (!GotFlag(statePtr, CHANNEL_NONBLOCKING)) {
		blockModeProc = Tcl_ChannelBlockModeProc(chanPtr->typePtr);
		if (blockModeProc == NULL) {
		    /*
		     * Don't peek ahead if cannot set non-blocking mode.
		     */

		    goto cleanup;
		}
		StackSetBlockMode(chanPtr, TCL_MODE_NONBLOCKING);
	    }
	}
    }
    if (FilterInputBytes(chanPtr, gsPtr) == 0) {
	*dstEndPtr = *gsPtr->dstPtr + gsPtr->bytesWrote;
    }
    if (blockModeProc != NULL) {
	StackSetBlockMode(chanPtr, TCL_MODE_BLOCKING);
    }
    return;

  cleanup:
    bufPtr->nextRemoved += gsPtr->rawRead;
    gsPtr->rawRead = 0;
    gsPtr->totalChars += gsPtr->charsWrote;
    gsPtr->bytesWrote = 0;
    gsPtr->charsWrote = 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * CommonGetsCleanup --
 *
 *	Helper function for Tcl_GetsObj() to restore the channel after a
 *	"gets" operation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Encoding may be freed.
 *
 *---------------------------------------------------------------------------
 */

static void
CommonGetsCleanup(
    Channel *chanPtr)
{
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    ChannelBuffer *bufPtr, *nextPtr;

    bufPtr = statePtr->inQueueHead;
    for ( ; bufPtr != NULL; bufPtr = nextPtr) {
	nextPtr = bufPtr->nextPtr;
	if (IsBufferReady(bufPtr)) {
	    break;
	}
	RecycleBuffer(statePtr, bufPtr, 0);
    }
    statePtr->inQueueHead = bufPtr;
    if (bufPtr == NULL) {
	statePtr->inQueueTail = NULL;
    } else {
	/*
	 * If any multi-byte characters were split across channel buffer
	 * boundaries, the split-up bytes were moved to the next channel
	 * buffer by FilterInputBytes(). Move the bytes back to their original
	 * buffer because the caller could change the channel's encoding which
	 * could change the interpretation of whether those bytes really made
	 * up multi-byte characters after all.
	 */

	nextPtr = bufPtr->nextPtr;
	for ( ; nextPtr != NULL; nextPtr = bufPtr->nextPtr) {
	    int extra;

	    extra = SpaceLeft(bufPtr);
	    if (extra > 0) {
		memcpy(InsertPoint(bufPtr),
			nextPtr->buf + (BUFFER_PADDING - extra),
			(size_t) extra);
		bufPtr->nextAdded += extra;
		nextPtr->nextRemoved = BUFFER_PADDING;
	    }
	    bufPtr = nextPtr;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Read --
 *
 *	Reads a given number of bytes from a channel. EOL and EOF translation
 *	is done on the bytes being read, so the number of bytes consumed from
 *	the channel may not be equal to the number of bytes stored in the
 *	destination buffer.
 *
 *	No encoding conversions are applied to the bytes being read.
 *
 * Results:
 *	The number of bytes read, or -1 on error. Use Tcl_GetErrno() to
 *	retrieve the error code for the error that occurred.
 *
 * Side effects:
 *	May cause input to be buffered.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Read(
    Tcl_Channel chan,		/* The channel from which to read. */
    char *dst,			/* Where to store input read. */
    int bytesToRead)		/* Maximum number of bytes to read. */
{
    Channel *chanPtr = (Channel *) chan;
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */

    /*
     * This operation should occur at the top of a channel stack.
     */

    chanPtr = statePtr->topChanPtr;

    if (CheckChannelErrors(statePtr, TCL_READABLE) != 0) {
	return -1;
    }

    return DoRead(chanPtr, dst, bytesToRead, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ReadRaw --
 *
 *	Reads a given number of bytes from a channel. EOL and EOF translation
 *	is done on the bytes being read, so the number of bytes consumed from
 *	the channel may not be equal to the number of bytes stored in the
 *	destination buffer.
 *
 *	No encoding conversions are applied to the bytes being read.
 *
 * Results:
 *	The number of bytes read, or -1 on error. Use Tcl_GetErrno() to
 *	retrieve the error code for the error that occurred.
 *
 * Side effects:
 *	May cause input to be buffered.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ReadRaw(
    Tcl_Channel chan,		/* The channel from which to read. */
    char *readBuf,		/* Where to store input read. */
    int bytesToRead)		/* Maximum number of bytes to read. */
{
    Channel *chanPtr = (Channel *) chan;
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    int copied = 0;

    assert(bytesToRead > 0);
    if (CheckChannelErrors(statePtr, TCL_READABLE | CHANNEL_RAW_MODE) != 0) {
	return -1;
    }

    /* First read bytes from the push-back buffers. */

    while (chanPtr->inQueueHead && bytesToRead > 0) {
	ChannelBuffer *bufPtr = chanPtr->inQueueHead;
	int bytesInBuffer = BytesLeft(bufPtr);
	int toCopy = (bytesInBuffer < bytesToRead) ? bytesInBuffer
		: bytesToRead;

	/* Copy the current chunk into the read buffer. */

	memcpy(readBuf, RemovePoint(bufPtr), (size_t) toCopy);
	bufPtr->nextRemoved += toCopy;
	copied += toCopy;
	readBuf += toCopy;
	bytesToRead -= toCopy;

	/* If the current buffer is empty recycle it. */

	if (IsBufferEmpty(bufPtr)) {
	    chanPtr->inQueueHead = bufPtr->nextPtr;
	    if (chanPtr->inQueueHead == NULL) {
		chanPtr->inQueueTail = NULL;
	    }
	    RecycleBuffer(chanPtr->state, bufPtr, 0);
	}
    }

    /*
     * Go to the driver only if we got nothing from pushback.
     * Have to do it this way to avoid EOF mis-timings when we
     * consider the ability that EOF may not be a permanent
     * condition in the driver, and in that case we have to
     * synchronize.
     */

    if (copied) {
	return copied;
    }

    /* This test not needed. */
    if (bytesToRead > 0) {

	int nread = ChanRead(chanPtr, readBuf, bytesToRead);

	if (nread > 0) {
	    /* Successful read (short is OK) - add to bytes copied */
	    copied += nread;
	} else if (nread < 0) {
	    /*
	     * An error signaled.  If CHANNEL_BLOCKED, then the error
	     * is not real, but an indication of blocked state.  In
	     * that case, retain the flag and let caller receive the
	     * short read of copied bytes from the pushback.
	     * HOWEVER, if copied==0 bytes from pushback then repeat
	     * signalling the blocked state as an error to caller so
	     * there is no false report of an EOF.
	     * When !CHANNEL_BLOCKED, the error is real and passes on
	     * to caller.
	     */
	    if (!GotFlag(statePtr, CHANNEL_BLOCKED) || copied == 0) {
		copied = -1;
	    }
	} else {
	    /*
	     * nread == 0.  Driver is at EOF. Let that state filter up.
	     */
	}
    }
    return copied;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_ReadChars --
 *
 *	Reads from the channel until the requested number of characters have
 *	been seen, EOF is seen, or the channel would block. EOL and EOF
 *	translation is done. If reading binary data, the raw bytes are wrapped
 *	in a Tcl byte array object. Otherwise, the raw bytes are converted to
 *	UTF-8 using the channel's current encoding and stored in a Tcl string
 *	object.
 *
 * Results:
 *	The number of characters read, or -1 on error. Use Tcl_GetErrno() to
 *	retrieve the error code for the error that occurred.
 *
 * Side effects:
 *	May cause input to be buffered.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_ReadChars(
    Tcl_Channel chan,		/* The channel to read. */
    Tcl_Obj *objPtr,		/* Input data is stored in this object. */
    int toRead,			/* Maximum number of characters to store, or
				 * -1 to read all available data (up to EOF or
				 * when channel blocks). */
    int appendFlag)		/* If non-zero, data read from the channel
				 * will be appended to the object. Otherwise,
				 * the data will replace the existing contents
				 * of the object. */
{
    Channel *chanPtr = (Channel *) chan;
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */

    /*
     * This operation should occur at the top of a channel stack.
     */

    chanPtr = statePtr->topChanPtr;

    if (CheckChannelErrors(statePtr, TCL_READABLE) != 0) {
	/*
	 * Update the notifier state so we don't block while there is still
	 * data in the buffers.
	 */

	UpdateInterest(chanPtr);
	return -1;
    }

    return DoReadChars(chanPtr, objPtr, toRead, appendFlag);
}
/*
 *---------------------------------------------------------------------------
 *
 * DoReadChars --
 *
 *	Reads from the channel until the requested number of characters have
 *	been seen, EOF is seen, or the channel would block. EOL and EOF
 *	translation is done. If reading binary data, the raw bytes are wrapped
 *	in a Tcl byte array object. Otherwise, the raw bytes are converted to
 *	UTF-8 using the channel's current encoding and stored in a Tcl string
 *	object.
 *
 * Results:
 *	The number of characters read, or -1 on error. Use Tcl_GetErrno() to
 *	retrieve the error code for the error that occurred.
 *
 * Side effects:
 *	May cause input to be buffered.
 *
 *---------------------------------------------------------------------------
 */

static int
DoReadChars(
    Channel *chanPtr,		/* The channel to read. */
    Tcl_Obj *objPtr,		/* Input data is stored in this object. */
    int toRead,			/* Maximum number of characters to store, or
				 * -1 to read all available data (up to EOF or
				 * when channel blocks). */
    int appendFlag)		/* If non-zero, data read from the channel
				 * will be appended to the object. Otherwise,
				 * the data will replace the existing contents
				 * of the object. */
{
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    ChannelBuffer *bufPtr;
    int copied, copiedNow, result;
    Tcl_Encoding encoding = statePtr->encoding;
    int binaryMode;
#define UTF_EXPANSION_FACTOR	1024
    int factor = UTF_EXPANSION_FACTOR;

    binaryMode = (encoding == NULL)
	    && (statePtr->inputTranslation == TCL_TRANSLATE_LF)
	    && (statePtr->inEofChar == '\0');

    if (appendFlag == 0) {
	if (binaryMode) {
	    Tcl_SetByteArrayLength(objPtr, 0);
	} else {
	    Tcl_SetObjLength(objPtr, 0);

	    /*
	     * We're going to access objPtr->bytes directly, so we must ensure
	     * that this is actually a string object (otherwise it might have
	     * been pure Unicode).
	     *
	     * Probably not needed anymore.
	     */

	    TclGetString(objPtr);
	}
    }

    /*
     * Early out when next read will see eofchar.
     *
     * NOTE: See DoRead for argument that it's a bug (one we're keeping)
     * to have this escape before the one for zero-char read request.
     */

    if (GotFlag(statePtr, CHANNEL_STICKY_EOF)) {
	SetFlag(statePtr, CHANNEL_EOF);
	assert( statePtr->inputEncodingFlags & TCL_ENCODING_END );
	assert( !GotFlag(statePtr, CHANNEL_BLOCKED|INPUT_SAW_CR) );

	/* TODO: We don't need this call? */
	UpdateInterest(chanPtr);
	return 0;
    }

    /* Special handling for zero-char read request. */
    if (toRead == 0) {
	if (GotFlag(statePtr, CHANNEL_EOF)) {
	    statePtr->inputEncodingFlags |= TCL_ENCODING_START;
	}
	ResetFlag(statePtr, CHANNEL_BLOCKED|CHANNEL_EOF);
	statePtr->inputEncodingFlags &= ~TCL_ENCODING_END;
	/* TODO: We don't need this call? */
	UpdateInterest(chanPtr);
	return 0;
    }

    /*
     * This operation should occur at the top of a channel stack.
     */

    chanPtr = statePtr->topChanPtr;
    TclChannelPreserve((Tcl_Channel)chanPtr);

    /* Must clear the BLOCKED|EOF flags here since we check before reading */
    if (GotFlag(statePtr, CHANNEL_EOF)) {
	statePtr->inputEncodingFlags |= TCL_ENCODING_START;
    }
    ResetFlag(statePtr, CHANNEL_BLOCKED|CHANNEL_EOF);
    statePtr->inputEncodingFlags &= ~TCL_ENCODING_END;
    for (copied = 0; (unsigned) toRead > 0; ) {
	copiedNow = -1;
	if (statePtr->inQueueHead != NULL) {
	    if (binaryMode) {
		copiedNow = ReadBytes(statePtr, objPtr, toRead);
	    } else {
		copiedNow = ReadChars(statePtr, objPtr, toRead, &factor);
	    }

	    /*
	     * If the current buffer is empty recycle it.
	     */

	    bufPtr = statePtr->inQueueHead;
	    if (IsBufferEmpty(bufPtr)) {
		ChannelBuffer *nextPtr = bufPtr->nextPtr;

		RecycleBuffer(statePtr, bufPtr, 0);
		statePtr->inQueueHead = nextPtr;
		if (nextPtr == NULL) {
		    statePtr->inQueueTail = NULL;
		}
	    }
	}

	if (copiedNow < 0) {
	    if (GotFlag(statePtr, CHANNEL_EOF)) {
		break;
	    }
	    if (GotFlag(statePtr, CHANNEL_NONBLOCKING|CHANNEL_BLOCKED)
		    == (CHANNEL_NONBLOCKING|CHANNEL_BLOCKED)) {
		break;
	    }
	    result = GetInput(chanPtr);
	    if (chanPtr != statePtr->topChanPtr) {
		TclChannelRelease((Tcl_Channel)chanPtr);
		chanPtr = statePtr->topChanPtr;
		TclChannelPreserve((Tcl_Channel)chanPtr);
	    }
	    if (result != 0) {
		if (!GotFlag(statePtr, CHANNEL_BLOCKED)) {
		    copied = -1;
		}
		break;
	    }
	} else {
	    copied += copiedNow;
	    toRead -= copiedNow;
	}
    }

    /*
     * Failure to fill a channel buffer may have left channel reporting
     * a "blocked" state, but so long as we fulfilled the request here,
     * the caller does not consider us blocked.
     */
    if (toRead == 0) {
	ResetFlag(statePtr, CHANNEL_BLOCKED);
    }

    /*
     * Regenerate the top channel, in case it was changed due to
     * self-modifying reflected transforms.
     */
    if (chanPtr != statePtr->topChanPtr) {
	TclChannelRelease((Tcl_Channel)chanPtr);
	chanPtr = statePtr->topChanPtr;
	TclChannelPreserve((Tcl_Channel)chanPtr);
    }

    /*
     * Update the notifier state so we don't block while there is still data
     * in the buffers.
     */
	assert(!GotFlag(statePtr, CHANNEL_EOF)
		|| GotFlag(statePtr, CHANNEL_STICKY_EOF)
		|| Tcl_InputBuffered((Tcl_Channel)chanPtr) == 0);
	assert( !(GotFlag(statePtr, CHANNEL_EOF|CHANNEL_BLOCKED)
		== (CHANNEL_EOF|CHANNEL_BLOCKED)) );
    UpdateInterest(chanPtr);
    TclChannelRelease((Tcl_Channel)chanPtr);
    return copied;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadBytes --
 *
 *	Reads from the channel until the requested number of bytes have been
 *	seen, EOF is seen, or the channel would block. Bytes from the channel
 *	are stored in objPtr as a ByteArray object. EOL and EOF translation
 *	are done.
 *
 *	'bytesToRead' can safely be a very large number because space is only
 *	allocated to hold data read from the channel as needed.
 *
 * Results:
 *	The return value is the number of bytes appended to the object, or
 *	-1 to indicate that zero bytes were read due to an EOF.
 *
 * Side effects:
 *	The storage of bytes in objPtr can cause (re-)allocation of memory.
 *
 *---------------------------------------------------------------------------
 */

static int
ReadBytes(
    ChannelState *statePtr,	/* State of the channel to read. */
    Tcl_Obj *objPtr,		/* Input data is appended to this ByteArray
				 * object. Its length is how much space has
				 * been allocated to hold data, not how many
				 * bytes of data have been stored in the
				 * object. */
    int bytesToRead)		/* Maximum number of bytes to store, or < 0 to
				 * get all available bytes. Bytes are obtained
				 * from the first buffer in the queue - even
				 * if this number is larger than the number of
				 * bytes available in the first buffer, only
				 * the bytes from the first buffer are
				 * returned. */
{
    ChannelBuffer *bufPtr = statePtr->inQueueHead;
    int srcLen = BytesLeft(bufPtr);
    int toRead = bytesToRead>srcLen || bytesToRead<0 ? srcLen : bytesToRead;

    TclAppendBytesToByteArray(objPtr, (unsigned char *) RemovePoint(bufPtr),
	    toRead);
    bufPtr->nextRemoved += toRead;
    return toRead;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadChars --
 *
 *	Reads from the channel until the requested number of UTF-8 characters
 *	have been seen, EOF is seen, or the channel would block. Raw bytes
 *	from the channel are converted to UTF-8 and stored in objPtr. EOL and
 *	EOF translation is done.
 *
 *	'charsToRead' can safely be a very large number because space is only
 *	allocated to hold data read from the channel as needed.
 *
 *	'charsToRead' may *not* be 0.
 *
 * Results:
 *	The return value is the number of characters appended to the object,
 *	*offsetPtr is filled with the number of bytes that were appended, and
 *	*factorPtr is filled with the expansion factor used to guess how many
 *	bytes of UTF-8 to allocate to hold N source bytes.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
ReadChars(
    ChannelState *statePtr,	/* State of channel to read. */
    Tcl_Obj *objPtr,		/* Input data is appended to this object.
				 * objPtr->length is how much space has been
				 * allocated to hold data, not how many bytes
				 * of data have been stored in the object. */
    int charsToRead,		/* Maximum number of characters to store, or
				 * -1 to get all available characters.
				 * Characters are obtained from the first
				 * buffer in the queue -- even if this number
				 * is larger than the number of characters
				 * available in the first buffer, only the
				 * characters from the first buffer are
				 * returned. The execption is when there is
				 * not any complete character in the first
				 * buffer.  In that case, a recursive call
				 * effectively obtains chars from the
				 * second buffer. */
    int *factorPtr)		/* On input, contains a guess of how many
				 * bytes need to be allocated to hold the
				 * result of converting N source bytes to
				 * UTF-8. On output, contains another guess
				 * based on the data seen so far. */
{
    Tcl_Encoding encoding = statePtr->encoding? statePtr->encoding
	    : GetBinaryEncoding();
    Tcl_EncodingState savedState = statePtr->inputEncodingState;
    ChannelBuffer *bufPtr = statePtr->inQueueHead;
    int savedIEFlags = statePtr->inputEncodingFlags;
    int savedFlags = statePtr->flags;
    char *dst, *src = RemovePoint(bufPtr);
    int numBytes, srcLen = BytesLeft(bufPtr);

    /*
     * One src byte can yield at most one character.  So when the
     * number of src bytes we plan to read is less than the limit on
     * character count to be read, clearly we will remain within that
     * limit, and we can use the value of "srcLen" as a tighter limit
     * for sizing receiving buffers.
     */

    int toRead = ((charsToRead<0)||(charsToRead > srcLen)) ? srcLen : charsToRead;

    /*
     * 'factor' is how much we guess that the bytes in the source buffer will
     * expand when converted to UTF-8 chars. This guess comes from analyzing
     * how many characters were produced by the previous pass.
     */

    int factor = *factorPtr;
    int dstLimit = TCL_UTF_MAX - 1 + toRead * factor / UTF_EXPANSION_FACTOR;

    (void) TclGetStringFromObj(objPtr, &numBytes);
    Tcl_AppendToObj(objPtr, NULL, dstLimit);
    if (toRead == srcLen) {
	unsigned int size;
	dst = TclGetStringStorage(objPtr, &size) + numBytes;
	dstLimit = size - numBytes;
    } else {
	dst = TclGetString(objPtr) + numBytes;
    }

    /*
     * This routine is burdened with satisfying several constraints.
     * It cannot append more than 'charsToRead` chars onto objPtr.
     * This is measured after encoding and translation transformations
     * are completed.  There is no precise number of src bytes that can
     * be associated with the limit.  Yet, when we are done, we must know
     * precisely the number of src bytes that were consumed to produce
     * the appended chars, so that all subsequent bytes are left in
     * the buffers for future read operations.
     *
     * The consequence is that we have no choice but to implement a
     * "trial and error" approach, where in general we may need to
     * perform transformations and copies multiple times to achieve
     * a consistent set of results.  This takes the shape of a loop.
     */

    while (1) {
	int dstDecoded, dstRead, dstWrote, srcRead, numChars, code;
	int flags = statePtr->inputEncodingFlags | TCL_ENCODING_NO_TERMINATE;

	if (charsToRead > 0) {
	    flags |= TCL_ENCODING_CHAR_LIMIT;
	    numChars = charsToRead;
	}

	/*
	 * Perform the encoding transformation.  Read no more than
	 * srcLen bytes, write no more than dstLimit bytes.
	 *
	 * Some trickiness with encoding flags here.  We do not want
	 * the end of a buffer to be treated as the end of all input
	 * when the presence of bytes in a next buffer are already
	 * known to exist.  This is checked with an assert() because
	 * so far no test case causing the assertion to be false has
	 * been created.  The normal operations of channel reading
	 * appear to cause EOF and TCL_ENCODING_END setting to appear
	 * only in situations where there are no further bytes in
	 * any buffers.
	 */

	assert(bufPtr->nextPtr == NULL || BytesLeft(bufPtr->nextPtr) == 0
		|| (statePtr->inputEncodingFlags & TCL_ENCODING_END) == 0);

	code = Tcl_ExternalToUtf(NULL, encoding, src, srcLen,
		flags, &statePtr->inputEncodingState,
		dst, dstLimit, &srcRead, &dstDecoded, &numChars);

	/*
	 * Perform the translation transformation in place.  Read no more
	 * than the dstDecoded bytes the encoding transformation actually
	 * produced.  Capture the number of bytes written in dstWrote.
	 * Capture the number of bytes actually consumed in dstRead.
	 */

	dstWrote = dstLimit;
	dstRead = dstDecoded;
	TranslateInputEOL(statePtr, dst, dst, &dstWrote, &dstRead);

	if (dstRead < dstDecoded) {

	    /*
	     * The encoding transformation produced bytes that the
	     * translation transformation did not consume.  Why did
	     * this happen?
	     */

	    if (statePtr->inEofChar && dst[dstRead] == statePtr->inEofChar) {
		/*
		 * 1) There's an eof char set on the channel, and
		 *    we saw it and stopped translating at that point.
		 *
		 * NOTE the bizarre spec of TranslateInputEOL in this case.
		 * Clearly the eof char had to be read in order to account
		 * for the stopping, but the value of dstRead does not
		 * include it.
		 *
		 * Also rather bizarre, our caller can only notice an
		 * EOF condition if we return the value -1 as the number
		 * of chars read.  This forces us to perform a 2-call
		 * dance where the first call can read all the chars
		 * up to the eof char, and the second call is solely
		 * for consuming the encoded eof char then pointed at
		 * by src so that we can return that magic -1 value.
		 * This seems really wasteful, especially since
		 * the first decoding pass of each call is likely to
		 * decode many bytes beyond that eof char that's all we
		 * care about.
		 */

		if (dstRead == 0) {
		    /*
		     * Curious choice in the eof char handling.  We leave
		     * the eof char in the buffer.  So, no need to compute
		     * a proper srcRead value.  At this point, there
		     * are no chars before the eof char in the buffer.
		     */
		    Tcl_SetObjLength(objPtr, numBytes);
		    return -1;
		}

		{
		    /*
		     * There are chars leading the buffer before the eof
		     * char.  Adjust the dstLimit so we go back and read
		     * only those and do not encounter the eof char this
		     * time.
		     */

		    dstLimit = dstRead - 1 + TCL_UTF_MAX;
		    statePtr->flags = savedFlags;
		    statePtr->inputEncodingFlags = savedIEFlags;
		    statePtr->inputEncodingState = savedState;
		    continue;
		}
	    }

	    /*
	     * 2) The other way to read fewer bytes than are decoded
	     *    is when the final byte is \r and we're in a CRLF
	     *    translation mode so we cannot decide whether to
	     *	  record \r or \n yet.
	     */

	    assert(dst[dstRead] == '\r');
	    assert(statePtr->inputTranslation == TCL_TRANSLATE_CRLF);

	    if (dstWrote > 0) {
		/*
		 * There are chars we can read before we hit the bare cr.
		 * Go back with a smaller dstLimit so we get them in the
		 * next pass, compute a matching srcRead, and don't end
		 * up back here in this call.
		 */

		dstLimit = dstRead - 1 + TCL_UTF_MAX;
		statePtr->flags = savedFlags;
		statePtr->inputEncodingFlags = savedIEFlags;
		statePtr->inputEncodingState = savedState;
		continue;
	    }

	    assert(dstWrote == 0);
	    assert(dstRead == 0);

	    /*
	     * We decoded only the bare cr, and we cannot read a
	     * translated char from that alone.  We have to know what's
	     * next.  So why do we only have the one decoded char?
	     */

	    if (code != TCL_OK) {
		char buffer[TCL_UTF_MAX + 1];
		int read, decoded, count;

		/*
		 * Didn't get everything the buffer could offer
		 */

		statePtr->flags = savedFlags;
		statePtr->inputEncodingFlags = savedIEFlags;
		statePtr->inputEncodingState = savedState;

		assert(bufPtr->nextPtr == NULL
			|| BytesLeft(bufPtr->nextPtr) == 0 || 0 ==
			(statePtr->inputEncodingFlags & TCL_ENCODING_END));

		Tcl_ExternalToUtf(NULL, encoding, src, srcLen,
		(statePtr->inputEncodingFlags | TCL_ENCODING_NO_TERMINATE),
		&statePtr->inputEncodingState, buffer, TCL_UTF_MAX + 1,
		&read, &decoded, &count);

		if (count == 2) {
		    if (buffer[1] == '\n') {
			/* \r\n translate to \n */
			dst[0] = '\n';
			bufPtr->nextRemoved += read;
		    } else {
			dst[0] = '\r';
			bufPtr->nextRemoved += srcRead;
		    }

		    statePtr->inputEncodingFlags &= ~TCL_ENCODING_START;

		    Tcl_SetObjLength(objPtr, numBytes + 1);
		    return 1;
		}

	    } else if (statePtr->flags & CHANNEL_EOF) {

		/*
		 * The bare \r is the only char and we will never read
		 * a subsequent char to make the determination.
		 */

		dst[0] = '\r';
		bufPtr->nextRemoved = bufPtr->nextAdded;
		Tcl_SetObjLength(objPtr, numBytes + 1);
		return 1;
	    }

	    /*
	     * Revise the dstRead value so that the numChars calc
	     * below correctly computes zero characters read.
	     */

	    dstRead = numChars;

	    /* FALL THROUGH - get more data (dstWrote == 0) */
	}

	/*
	 * The translation transformation can only reduce the number
	 * of chars when it converts \r\n into \n.  The reduction in
	 * the number of chars is the difference in bytes read and written.
	 */

	numChars -= (dstRead - dstWrote);

	if (charsToRead > 0 && numChars > charsToRead) {

	    /*
	     * TODO: This cannot happen anymore.
	     *
	     * We read more chars than allowed.  Reset limits to
	     * prevent that and try again.  Don't forget the extra
	     * padding of TCL_UTF_MAX bytes demanded by the
	     * Tcl_ExternalToUtf() call!
	     */

	    dstLimit = Tcl_UtfAtIndex(dst, charsToRead) - 1 + TCL_UTF_MAX - dst;
	    statePtr->flags = savedFlags;
	    statePtr->inputEncodingFlags = savedIEFlags;
	    statePtr->inputEncodingState = savedState;
	    continue;
	}

	if (dstWrote == 0) {
	    ChannelBuffer *nextPtr;

	    /* We were not able to read any chars. */

	    assert (numChars == 0);

	    /*
	     * There is one situation where this is the correct final
	     * result.  If the src buffer contains only a single \n
	     * byte, and we are in TCL_TRANSLATE_AUTO mode, and
	     * when the translation pass was made the INPUT_SAW_CR
	     * flag was set on the channel.  In that case, the
	     * correct behavior is to consume that \n and produce the
	     * empty string.
	     */

	    if (dstRead == 1 && dst[0] == '\n') {
		assert(statePtr->inputTranslation == TCL_TRANSLATE_AUTO);

		goto consume;
	    }

	    /* Otherwise, reading zero characters indicates there's
	     * something incomplete at the end of the src buffer.
	     * Maybe there were not enough src bytes to decode into
	     * a char.  Maybe a lone \r could not be translated (crlf
	     * mode).  Need to combine any unused src bytes we have
	     * in the first buffer with subsequent bytes to try again.
	     */

	    nextPtr = bufPtr->nextPtr;

	    if (nextPtr == NULL) {
		if (srcLen > 0) {
		    SetFlag(statePtr, CHANNEL_NEED_MORE_DATA);
		}
		Tcl_SetObjLength(objPtr, numBytes);
		return -1;
	    }

	    /*
	     * Space is made at the beginning of the buffer to copy the
	     * previous unused bytes there. Check first if the buffer we
	     * are using actually has enough space at its beginning for
	     * the data we are copying.  Because if not we will write over
	     * the buffer management information, especially the 'nextPtr'.
	     *
	     * Note that the BUFFER_PADDING (See AllocChannelBuffer) is
	     * used to prevent exactly this situation. I.e. it should never
	     * happen.  Therefore it is ok to panic should it happen despite
	     * the precautions.
	     */

	    if (nextPtr->nextRemoved - srcLen < 0) {
		Tcl_Panic("Buffer Underflow, BUFFER_PADDING not enough");
	    }

	    nextPtr->nextRemoved -= srcLen;
	    memcpy(RemovePoint(nextPtr), src, (size_t) srcLen);
	    RecycleBuffer(statePtr, bufPtr, 0);
	    statePtr->inQueueHead = nextPtr;
	    Tcl_SetObjLength(objPtr, numBytes);
	    return ReadChars(statePtr, objPtr, charsToRead, factorPtr);
	}

	statePtr->inputEncodingFlags &= ~TCL_ENCODING_START;

    consume:
	bufPtr->nextRemoved += srcRead;
	/*
	 * If this read contained multibyte characters, revise factorPtr
	 * so the next read will allocate bigger buffers.
	 */
	if (numChars && numChars < srcRead) {
	    *factorPtr = srcRead * UTF_EXPANSION_FACTOR / numChars;
	}
	Tcl_SetObjLength(objPtr, numBytes + dstWrote);
	return numChars;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TranslateInputEOL --
 *
 *	Perform input EOL and EOF translation on the source buffer, leaving
 *	the translated result in the destination buffer.
 *
 * Results:
 *	The return value is 1 if the EOF character was found when copying
 *	bytes to the destination buffer, 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
TranslateInputEOL(
    ChannelState *statePtr,	/* Channel being read, for EOL translation and
				 * EOF character. */
    char *dstStart,		/* Output buffer filled with chars by applying
				 * appropriate EOL translation to source
				 * characters. */
    const char *srcStart,	/* Source characters. */
    int *dstLenPtr,		/* On entry, the maximum length of output
				 * buffer in bytes. On exit, the number of
				 * bytes actually used in output buffer. */
    int *srcLenPtr)		/* On entry, the length of source buffer. On
				 * exit, the number of bytes read from the
				 * source buffer. */
{
    const char *eof = NULL;
    int dstLen = *dstLenPtr;
    int srcLen = *srcLenPtr;
    int inEofChar = statePtr->inEofChar;

    /*
     * Depending on the translation mode in use, there's no need
     * to scan more srcLen bytes at srcStart than can possibly transform
     * to dstLen bytes.  This keeps the scan for eof char below from
     * being pointlessly long.
     */

    switch (statePtr->inputTranslation) {
    case TCL_TRANSLATE_LF:
    case TCL_TRANSLATE_CR:
	if (srcLen > dstLen) {
	/* In these modes, each src byte become a dst byte. */
	    srcLen = dstLen;
	}
	break;
    default:
	/* In other modes, at most 2 src bytes become a dst byte. */
	if (srcLen/2 > dstLen) {
	    srcLen = 2 * dstLen;
	}
	break;
    }

    if (inEofChar != '\0') {
	/*
	 * Make sure we do not read past any logical end of channel input
	 * created by the presence of the input eof char.
	 */

	if ((eof = memchr(srcStart, inEofChar, srcLen))) {
	    srcLen = eof - srcStart;
	}
    }

    switch (statePtr->inputTranslation) {
    case TCL_TRANSLATE_LF:
    case TCL_TRANSLATE_CR:
	if (dstStart != srcStart) {
	    memcpy(dstStart, srcStart, (size_t) srcLen);
	}
	if (statePtr->inputTranslation == TCL_TRANSLATE_CR) {
	    char *dst = dstStart;
	    char *dstEnd = dstStart + srcLen;

	    while ((dst = memchr(dst, '\r', dstEnd - dst))) {
		*dst++ = '\n';
	    }
	}
	dstLen = srcLen;
	break;
    case TCL_TRANSLATE_CRLF: {
	const char *crFound, *src = srcStart;
	char *dst = dstStart;
	int lesser = (dstLen < srcLen) ? dstLen : srcLen;

	while ((crFound = memchr(src, '\r', lesser))) {
	    int numBytes = crFound - src;
	    memmove(dst, src, numBytes);

	    dst += numBytes; dstLen -= numBytes;
	    src += numBytes; srcLen -= numBytes;
	    if (srcLen == 1) {
		/* valid src bytes end in \r */
		if (eof) {
		    *dst++ = '\r';
		    src++; srcLen--;
		} else {
		    lesser = 0;
		    break;
		}
	    } else if (src[1] == '\n') {
		*dst++ = '\n';
		src += 2; srcLen -= 2;
	    } else {
		*dst++ = '\r';
		src++; srcLen--;
	    }
	    dstLen--;
	    lesser = (dstLen < srcLen) ? dstLen : srcLen;
	}
	memmove(dst, src, lesser);
	srcLen = src + lesser - srcStart;
	dstLen = dst + lesser - dstStart;
	break;
    }
    case TCL_TRANSLATE_AUTO: {
	const char *crFound, *src = srcStart;
	char *dst = dstStart;
	int lesser;

	if ((statePtr->flags & INPUT_SAW_CR) && srcLen) {
	    if (*src == '\n') { src++; srcLen--; }
	    ResetFlag(statePtr, INPUT_SAW_CR);
	}
	lesser = (dstLen < srcLen) ? dstLen : srcLen;
	while ((crFound = memchr(src, '\r', lesser))) {
	    int numBytes = crFound - src;
	    memmove(dst, src, numBytes);

	    dst[numBytes] = '\n';
	    dst += numBytes + 1; dstLen -= numBytes + 1;
	    src += numBytes + 1; srcLen -= numBytes + 1;
	    if (srcLen == 0) {
		SetFlag(statePtr, INPUT_SAW_CR);
	    } else if (*src == '\n') {
		src++; srcLen--;
	    }
	    lesser = (dstLen < srcLen) ? dstLen : srcLen;
	}
	memmove(dst, src, lesser);
	srcLen = src + lesser - srcStart;
	dstLen = dst + lesser - dstStart;
	break;
    }
    default:
	Tcl_Panic("unknown input translation %d", statePtr->inputTranslation);
    }
    *dstLenPtr = dstLen;
    *srcLenPtr = srcLen;

    if (srcStart + srcLen == eof) {
	/*
	 * EOF character was seen in EOL translated range. Leave current file
	 * position pointing at the EOF character, but don't store the EOF
	 * character in the output string.
	 */

	SetFlag(statePtr, CHANNEL_EOF | CHANNEL_STICKY_EOF);
	statePtr->inputEncodingFlags |= TCL_ENCODING_END;
	ResetFlag(statePtr, CHANNEL_BLOCKED|INPUT_SAW_CR);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Ungets --
 *
 *	Causes the supplied string to be added to the input queue of the
 *	channel, at either the head or tail of the queue.
 *
 * Results:
 *	The number of bytes stored in the channel, or -1 on error.
 *
 * Side effects:
 *	Adds input to the input queue of a channel.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Ungets(
    Tcl_Channel chan,		/* The channel for which to add the input. */
    const char *str,		/* The input itself. */
    int len,			/* The length of the input. */
    int atEnd)			/* If non-zero, add at end of queue; otherwise
				 * add at head of queue. */
{
    Channel *chanPtr;		/* The real IO channel. */
    ChannelState *statePtr;	/* State of actual channel. */
    ChannelBuffer *bufPtr;	/* Buffer to contain the data. */
    int flags;

    chanPtr = (Channel *) chan;
    statePtr = chanPtr->state;

    /*
     * This operation should occur at the top of a channel stack.
     */

    chanPtr = statePtr->topChanPtr;

    /*
     * CheckChannelErrors clears too many flag bits in this one case.
     */

    flags = statePtr->flags;
    if (CheckChannelErrors(statePtr, TCL_READABLE) != 0) {
	len = -1;
	goto done;
    }
    statePtr->flags = flags;

    /*
     * Clear the EOF flags, and clear the BLOCKED bit.
     */

    if (GotFlag(statePtr, CHANNEL_EOF)) {
	statePtr->inputEncodingFlags |= TCL_ENCODING_START;
    }
    ResetFlag(statePtr,
	    CHANNEL_BLOCKED | CHANNEL_STICKY_EOF | CHANNEL_EOF | INPUT_SAW_CR);
    statePtr->inputEncodingFlags &= ~TCL_ENCODING_END;

    bufPtr = AllocChannelBuffer(len);
    memcpy(InsertPoint(bufPtr), str, (size_t) len);
    bufPtr->nextAdded += len;

    if (statePtr->inQueueHead == NULL) {
	bufPtr->nextPtr = NULL;
	statePtr->inQueueHead = bufPtr;
	statePtr->inQueueTail = bufPtr;
    } else if (atEnd) {
	bufPtr->nextPtr = NULL;
	statePtr->inQueueTail->nextPtr = bufPtr;
	statePtr->inQueueTail = bufPtr;
    } else {
	bufPtr->nextPtr = statePtr->inQueueHead;
	statePtr->inQueueHead = bufPtr;
    }

    /*
     * Update the notifier state so we don't block while there is still data
     * in the buffers.
     */

  done:
    UpdateInterest(chanPtr);
    return len;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Flush --
 *
 *	Flushes output data on a channel.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May flush output queued on this channel.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Flush(
    Tcl_Channel chan)		/* The Channel to flush. */
{
    int result;			/* Of calling FlushChannel. */
    Channel *chanPtr = (Channel *) chan;
				/* The actual channel. */
    ChannelState *statePtr = chanPtr->state;
				/* State of actual channel. */

    /*
     * This operation should occur at the top of a channel stack.
     */

    chanPtr = statePtr->topChanPtr;

    if (CheckChannelErrors(statePtr, TCL_WRITABLE) != 0) {
	return TCL_ERROR;
    }

    result = FlushChannel(NULL, chanPtr, 0);
    if (result != 0) {
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DiscardInputQueued --
 *
 *	Discards any input read from the channel but not yet consumed by Tcl
 *	reading commands.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May discard input from the channel. If discardLastBuffer is zero,
 *	leaves one buffer in place for back-filling.
 *
 *----------------------------------------------------------------------
 */

static void
DiscardInputQueued(
    ChannelState *statePtr,	/* Channel on which to discard the queued
				 * input. */
    int discardSavedBuffers)	/* If non-zero, discard all buffers including
				 * last one. */
{
    ChannelBuffer *bufPtr, *nxtPtr;
				/* Loop variables. */

    bufPtr = statePtr->inQueueHead;
    statePtr->inQueueHead = NULL;
    statePtr->inQueueTail = NULL;
    for (; bufPtr != NULL; bufPtr = nxtPtr) {
	nxtPtr = bufPtr->nextPtr;
	RecycleBuffer(statePtr, bufPtr, discardSavedBuffers);
    }

    /*
     * If discardSavedBuffers is nonzero, must also discard any previously
     * saved buffer in the saveInBufPtr field.
     */

    if (discardSavedBuffers && statePtr->saveInBufPtr != NULL) {
	ReleaseChannelBuffer(statePtr->saveInBufPtr);
	statePtr->saveInBufPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetInput --
 *
 *	Reads input data from a device into a channel buffer.
 *
 *	IMPORTANT!  This routine is only called on a chanPtr argument
 *	that is the top channel of a stack!
 *
 * Results:
 *	The return value is the Posix error code if an error occurred while
 *	reading from the file, or 0 otherwise.
 *
 * Side effects:
 *	Reads from the underlying device.
 *
 *---------------------------------------------------------------------------
 */

static int
GetInput(
    Channel *chanPtr)		/* Channel to read input from. */
{
    int toRead;			/* How much to read? */
    int result;			/* Of calling driver. */
    int nread;			/* How much was read from channel? */
    ChannelBuffer *bufPtr;	/* New buffer to add to input queue. */
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */

    /*
     * Verify that all callers know better than to call us when
     * it's recorded that the next char waiting to be read is the
     * eofchar.
     */

    assert( !GotFlag(statePtr, CHANNEL_STICKY_EOF) );

    /*
     * Prevent reading from a dead channel -- a channel that has been closed
     * but not yet deallocated, which can happen if the exit handler for
     * channel cleanup has run but the channel is still registered in some
     * interpreter.
     */

    if (CheckForDeadChannel(NULL, statePtr)) {
	return EINVAL;
    }

    /*
     * WARNING: There was once a comment here claiming that it was
     * a bad idea to make another call to the inputproc of a channel
     * driver when EOF has already been detected on the channel.  Through
     * much of Tcl's history, this warning was then completely negated
     * by having all (most?) read paths clear the EOF setting before
     * reaching here.  So we had a guard that was never triggered.
     *
     * Don't be tempted to restore the guard.  Even if EOF is set on
     * the channel, continue through and call the inputproc again.  This
     * is the way to enable the ability to [read] again beyond the EOF,
     * which seems a strange thing to do, but for which use cases exist
     * [Tcl Bug 5adc350683] and which may even be essential for channels
     * representing things like ttys or other devices where the stream
     * might take the logical form of a series of 'files' separated by
     * an EOF condition.
     */

    /*
     * First check for more buffers in the pushback area of the topmost
     * channel in the stack and use them. They can be the result of a
     * transformation which went away without reading all the information
     * placed in the area when it was stacked.
     */

    if (chanPtr->inQueueHead != NULL) {

	/* TODO: Tests to cover this. */
	assert(statePtr->inQueueHead == NULL);

	statePtr->inQueueHead = chanPtr->inQueueHead;
	statePtr->inQueueTail = chanPtr->inQueueTail;
	chanPtr->inQueueHead = NULL;
	chanPtr->inQueueTail = NULL;
	return 0;
    }

    /*
     * Nothing in the pushback area, fall back to the usual handling (driver,
     * etc.)
     */

    /*
     * See if we can fill an existing buffer. If we can, read only as much as
     * will fit in it. Otherwise allocate a new buffer, add it to the input
     * queue and attempt to fill it to the max.
     */

    bufPtr = statePtr->inQueueTail;

    if ((bufPtr == NULL) || IsBufferFull(bufPtr)) {
	bufPtr = statePtr->saveInBufPtr;
	statePtr->saveInBufPtr = NULL;

	/*
	 * Check the actual buffersize against the requested buffersize.
	 * Saved buffers of the wrong size are squashed. This is done
	 * to honor dynamic changes of the buffersize made by the user.
	 * TODO: Tests to cover this.
	 */

	if ((bufPtr != NULL)
		&& (bufPtr->bufLength - BUFFER_PADDING != statePtr->bufSize)) {
	    ReleaseChannelBuffer(bufPtr);
	    bufPtr = NULL;
	}

	if (bufPtr == NULL) {
	    bufPtr = AllocChannelBuffer(statePtr->bufSize);
	}
	bufPtr->nextPtr = NULL;

	toRead = SpaceLeft(bufPtr);
	assert(toRead == statePtr->bufSize);

	if (statePtr->inQueueTail == NULL) {
	    statePtr->inQueueHead = bufPtr;
	} else {
	    statePtr->inQueueTail->nextPtr = bufPtr;
	}
	statePtr->inQueueTail = bufPtr;
    } else {
	toRead = SpaceLeft(bufPtr);
    }

    PreserveChannelBuffer(bufPtr);
    nread = ChanRead(chanPtr, InsertPoint(bufPtr), toRead);

    if (nread < 0) {
	result = Tcl_GetErrno();
    } else {
	result = 0;
	bufPtr->nextAdded += nread;
    }

    ReleaseChannelBuffer(bufPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Seek --
 *
 *	Implements seeking on Tcl Channels. This is a public function so that
 *	other C facilities may be implemented on top of it.
 *
 * Results:
 *	The new access point or -1 on error. If error, use Tcl_GetErrno() to
 *	retrieve the POSIX error code for the error that occurred.
 *
 * Side effects:
 *	May flush output on the channel. May discard queued input.
 *
 *----------------------------------------------------------------------
 */

Tcl_WideInt
Tcl_Seek(
    Tcl_Channel chan,		/* The channel on which to seek. */
    Tcl_WideInt offset,		/* Offset to seek to. */
    int mode)			/* Relative to which location to seek? */
{
    Channel *chanPtr = (Channel *) chan;
				/* The real IO channel. */
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    int inputBuffered, outputBuffered;
				/* # bytes held in buffers. */
    int result;			/* Of device driver operations. */
    Tcl_WideInt curPos;		/* Position on the device. */
    int wasAsync;		/* Was the channel nonblocking before the seek
				 * operation? If so, must restore to
				 * non-blocking mode after the seek. */

    if (CheckChannelErrors(statePtr, TCL_WRITABLE | TCL_READABLE) != 0) {
	return Tcl_LongAsWide(-1);
    }

    /*
     * Disallow seek on dead channels - channels that have been closed but not
     * yet been deallocated. Such channels can be found if the exit handler
     * for channel cleanup has run but the channel is still registered in an
     * interpreter.
     */

    if (CheckForDeadChannel(NULL, statePtr)) {
	return Tcl_LongAsWide(-1);
    }

    /*
     * This operation should occur at the top of a channel stack.
     */

    chanPtr = statePtr->topChanPtr;

    /*
     * Disallow seek on channels whose type does not have a seek procedure
     * defined. This means that the channel does not support seeking.
     */

    if (chanPtr->typePtr->seekProc == NULL) {
	Tcl_SetErrno(EINVAL);
	return Tcl_LongAsWide(-1);
    }

    /*
     * Compute how much input and output is buffered. If both input and output
     * is buffered, cannot compute the current position.
     */

    inputBuffered = Tcl_InputBuffered(chan);
    outputBuffered = Tcl_OutputBuffered(chan);

    if ((inputBuffered != 0) && (outputBuffered != 0)) {
	Tcl_SetErrno(EFAULT);
	return Tcl_LongAsWide(-1);
    }

    /*
     * If we are seeking relative to the current position, compute the
     * corrected offset taking into account the amount of unread input.
     */

    if (mode == SEEK_CUR) {
	offset -= inputBuffered;
    }

    /*
     * Discard any queued input - this input should not be read after the
     * seek.
     */

    DiscardInputQueued(statePtr, 0);

    /*
     * Reset EOF and BLOCKED flags. We invalidate them by moving the access
     * point. Also clear CR related flags.
     */

    if (GotFlag(statePtr, CHANNEL_EOF)) {
	statePtr->inputEncodingFlags |= TCL_ENCODING_START;
    }
    ResetFlag(statePtr, CHANNEL_EOF | CHANNEL_STICKY_EOF | CHANNEL_BLOCKED |
	    INPUT_SAW_CR);
    statePtr->inputEncodingFlags &= ~TCL_ENCODING_END;

    /*
     * If the channel is in asynchronous output mode, switch it back to
     * synchronous mode and cancel any async flush that may be scheduled.
     * After the flush, the channel will be put back into asynchronous output
     * mode.
     */

    wasAsync = 0;
    if (GotFlag(statePtr, CHANNEL_NONBLOCKING)) {
	wasAsync = 1;
	result = StackSetBlockMode(chanPtr, TCL_MODE_BLOCKING);
	if (result != 0) {
	    return Tcl_LongAsWide(-1);
	}
	ResetFlag(statePtr, CHANNEL_NONBLOCKING);
	if (GotFlag(statePtr, BG_FLUSH_SCHEDULED)) {
	    ResetFlag(statePtr, BG_FLUSH_SCHEDULED);
	}
    }

    /*
     * If the flush fails we cannot recover the original position. In that
     * case the seek is not attempted because we do not know where the access
     * position is - instead we return the error. FlushChannel has already
     * called Tcl_SetErrno() to report the error upwards. If the flush
     * succeeds we do the seek also.
     */

    if (FlushChannel(NULL, chanPtr, 0) != 0) {
	curPos = -1;
    } else {
	/*
	 * Now seek to the new position in the channel as requested by the
	 * caller.
	 */

	curPos = ChanSeek(chanPtr, offset, mode, &result);
	if (curPos == Tcl_LongAsWide(-1)) {
	    Tcl_SetErrno(result);
	}
    }

    /*
     * Restore to nonblocking mode if that was the previous behavior.
     *
     * NOTE: Even if there was an async flush active we do not restore it now
     * because we already flushed all the queued output, above.
     */

    if (wasAsync) {
	SetFlag(statePtr, CHANNEL_NONBLOCKING);
	result = StackSetBlockMode(chanPtr, TCL_MODE_NONBLOCKING);
	if (result != 0) {
	    return Tcl_LongAsWide(-1);
	}
    }

    return curPos;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Tell --
 *
 *	Returns the position of the next character to be read/written on this
 *	channel.
 *
 * Results:
 *	A nonnegative integer on success, -1 on failure. If failed, use
 *	Tcl_GetErrno() to retrieve the POSIX error code for the error that
 *	occurred.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_WideInt
Tcl_Tell(
    Tcl_Channel chan)		/* The channel to return pos for. */
{
    Channel *chanPtr = (Channel *) chan;
				/* The real IO channel. */
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    int inputBuffered, outputBuffered;
				/* # bytes held in buffers. */
    int result;			/* Of calling device driver. */
    Tcl_WideInt curPos;		/* Position on device. */

    if (CheckChannelErrors(statePtr, TCL_WRITABLE | TCL_READABLE) != 0) {
	return Tcl_LongAsWide(-1);
    }

    /*
     * Disallow tell on dead channels -- channels that have been closed but
     * not yet been deallocated. Such channels can be found if the exit
     * handler for channel cleanup has run but the channel is still registered
     * in an interpreter.
     */

    if (CheckForDeadChannel(NULL, statePtr)) {
	return Tcl_LongAsWide(-1);
    }

    /*
     * This operation should occur at the top of a channel stack.
     */

    chanPtr = statePtr->topChanPtr;

    /*
     * Disallow tell on channels whose type does not have a seek procedure
     * defined. This means that the channel does not support seeking.
     */

    if (chanPtr->typePtr->seekProc == NULL) {
	Tcl_SetErrno(EINVAL);
	return Tcl_LongAsWide(-1);
    }

    /*
     * Compute how much input and output is buffered. If both input and output
     * is buffered, cannot compute the current position.
     */

    inputBuffered = Tcl_InputBuffered(chan);
    outputBuffered = Tcl_OutputBuffered(chan);

    /*
     * Get the current position in the device and compute the position where
     * the next character will be read or written. Note that we prefer the
     * wideSeekProc if that is available and non-NULL...
     */

    curPos = ChanSeek(chanPtr, Tcl_LongAsWide(0), SEEK_CUR, &result);
    if (curPos == Tcl_LongAsWide(-1)) {
	Tcl_SetErrno(result);
	return Tcl_LongAsWide(-1);
    }

    if (inputBuffered != 0) {
	return curPos - inputBuffered;
    }
    return curPos + outputBuffered;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_SeekOld, Tcl_TellOld --
 *
 *	Backward-compatibility versions of the seek/tell interface that do not
 *	support 64-bit offsets. This interface is not documented or expected
 *	to be supported indefinitely.
 *
 * Results:
 *	As for Tcl_Seek and Tcl_Tell respectively, except truncated to
 *	whatever value will fit in an 'int'.
 *
 * Side effects:
 *	As for Tcl_Seek and Tcl_Tell respectively.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_SeekOld(
    Tcl_Channel chan,		/* The channel on which to seek. */
    int offset,			/* Offset to seek to. */
    int mode)			/* Relative to which location to seek? */
{
    Tcl_WideInt wOffset, wResult;

    wOffset = Tcl_LongAsWide((long) offset);
    wResult = Tcl_Seek(chan, wOffset, mode);
    return (int) Tcl_WideAsLong(wResult);
}

int
Tcl_TellOld(
    Tcl_Channel chan)		/* The channel to return pos for. */
{
    Tcl_WideInt wResult = Tcl_Tell(chan);

    return (int) Tcl_WideAsLong(wResult);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_TruncateChannel --
 *
 *	Truncate a channel to the given length.
 *
 * Results:
 *	TCL_OK on success, TCL_ERROR if the operation failed (e.g. is not
 *	supported by the type of channel, or the underlying OS operation
 *	failed in some way).
 *
 * Side effects:
 *	Seeks the channel to the current location. Sets errno on OS error.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_TruncateChannel(
    Tcl_Channel chan,		/* Channel to truncate. */
    Tcl_WideInt length)		/* Length to truncate it to. */
{
    Channel *chanPtr = (Channel *) chan;
    Tcl_DriverTruncateProc *truncateProc =
	    Tcl_ChannelTruncateProc(chanPtr->typePtr);
    int result;

    if (truncateProc == NULL) {
	/*
	 * Feature not supported and it's not emulatable. Pretend it's
	 * returned an EINVAL, a very generic error!
	 */

	Tcl_SetErrno(EINVAL);
	return TCL_ERROR;
    }

    if (!GotFlag(chanPtr->state, TCL_WRITABLE)) {
	/*
	 * We require that the file was opened of writing. Do that check now
	 * so that we only flush if we think we're going to succeed.
	 */

	Tcl_SetErrno(EINVAL);
	return TCL_ERROR;
    }

    /*
     * Seek first to force a total flush of all pending buffers and ditch any
     * pre-read input data.
     */

    WillWrite(chanPtr);

    if (WillRead(chanPtr) < 0) {
        return TCL_ERROR;
    }

    /*
     * We're all flushed to disk now and we also don't have any unfortunate
     * input baggage around either; can truncate with impunity.
     */

    result = truncateProc(chanPtr->instanceData, length);
    if (result != 0) {
	Tcl_SetErrno(result);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckChannelErrors --
 *
 *	See if the channel is in an ready state and can perform the desired
 *	operation.
 *
 * Results:
 *	The return value is 0 if the channel is OK, otherwise the return value
 *	is -1 and errno is set to indicate the error.
 *
 * Side effects:
 *	May clear the EOF and/or BLOCKED bits if reading from channel.
 *
 *---------------------------------------------------------------------------
 */

static int
CheckChannelErrors(
    ChannelState *statePtr,	/* Channel to check. */
    int flags)			/* Test if channel supports desired operation:
				 * TCL_READABLE, TCL_WRITABLE. Also indicates
				 * Raw read or write for special close
				 * processing */
{
    int direction = flags & (TCL_READABLE|TCL_WRITABLE);

    /*
     * Check for unreported error.
     */

    if (statePtr->unreportedError != 0) {
	Tcl_SetErrno(statePtr->unreportedError);
	statePtr->unreportedError = 0;

	/*
	 * TIP #219, Tcl Channel Reflection API.
	 * Move a defered error message back into the channel bypass.
	 */

	if (statePtr->chanMsg != NULL) {
	    TclDecrRefCount(statePtr->chanMsg);
	}
	statePtr->chanMsg = statePtr->unreportedMsg;
	statePtr->unreportedMsg = NULL;
	return -1;
    }

    /*
     * Only the raw read and write operations are allowed during close in
     * order to drain data from stacked channels.
     */

    if (GotFlag(statePtr, CHANNEL_CLOSED) && !(flags & CHANNEL_RAW_MODE)) {
	Tcl_SetErrno(EACCES);
	return -1;
    }

    /*
     * Fail if the channel is not opened for desired operation.
     */

    if ((statePtr->flags & direction) == 0) {
	Tcl_SetErrno(EACCES);
	return -1;
    }

    /*
     * Fail if the channel is in the middle of a background copy.
     *
     * Don't do this tests for raw channels here or else the chaining in the
     * transformation drivers will fail with 'file busy' error instead of
     * retrieving and transforming the data to copy.
     */

    if (BUSY_STATE(statePtr, flags) && ((flags & CHANNEL_RAW_MODE) == 0)) {
	Tcl_SetErrno(EBUSY);
	return -1;
    }

    if (direction == TCL_READABLE) {
	ResetFlag(statePtr, CHANNEL_NEED_MORE_DATA);
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Eof --
 *
 *	Returns 1 if the channel is at EOF, 0 otherwise.
 *
 * Results:
 *	1 or 0, always.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Eof(
    Tcl_Channel chan)		/* Does this channel have EOF? */
{
    ChannelState *statePtr = ((Channel *) chan)->state;
				/* State of real channel structure. */

    return GotFlag(statePtr, CHANNEL_EOF) ? 1 : 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InputBlocked --
 *
 *	Returns 1 if input is blocked on this channel, 0 otherwise.
 *
 * Results:
 *	0 or 1, always.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_InputBlocked(
    Tcl_Channel chan)		/* Is this channel blocked? */
{
    ChannelState *statePtr = ((Channel *) chan)->state;
				/* State of real channel structure. */

    return GotFlag(statePtr, CHANNEL_BLOCKED) ? 1 : 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InputBuffered --
 *
 *	Returns the number of bytes of input currently buffered in the common
 *	internal buffer of a channel.
 *
 * Results:
 *	The number of input bytes buffered, or zero if the channel is not open
 *	for reading.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_InputBuffered(
    Tcl_Channel chan)		/* The channel to query. */
{
    ChannelState *statePtr = ((Channel *) chan)->state;
				/* State of real channel structure. */
    ChannelBuffer *bufPtr;
    int bytesBuffered;

    for (bytesBuffered = 0, bufPtr = statePtr->inQueueHead; bufPtr != NULL;
	    bufPtr = bufPtr->nextPtr) {
	bytesBuffered += BytesLeft(bufPtr);
    }

    /*
     * Don't forget the bytes in the topmost pushback area.
     */

    for (bufPtr = statePtr->topChanPtr->inQueueHead; bufPtr != NULL;
	    bufPtr = bufPtr->nextPtr) {
	bytesBuffered += BytesLeft(bufPtr);
    }

    return bytesBuffered;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_OutputBuffered --
 *
 *    Returns the number of bytes of output currently buffered in the common
 *    internal buffer of a channel.
 *
 * Results:
 *    The number of output bytes buffered, or zero if the channel is not open
 *    for writing.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_OutputBuffered(
    Tcl_Channel chan)		/* The channel to query. */
{
    ChannelState *statePtr = ((Channel *) chan)->state;
				/* State of real channel structure. */
    ChannelBuffer *bufPtr;
    int bytesBuffered;

    for (bytesBuffered = 0, bufPtr = statePtr->outQueueHead; bufPtr != NULL;
	    bufPtr = bufPtr->nextPtr) {
	bytesBuffered += BytesLeft(bufPtr);
    }
    if (statePtr->curOutPtr != NULL) {
	register ChannelBuffer *curOutPtr = statePtr->curOutPtr;

	if (IsBufferReady(curOutPtr)) {
	    bytesBuffered += BytesLeft(curOutPtr);
	}
    }

    return bytesBuffered;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelBuffered --
 *
 *	Returns the number of bytes of input currently buffered in the
 *	internal buffer (push back area) of a channel.
 *
 * Results:
 *	The number of input bytes buffered, or zero if the channel is not open
 *	for reading.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ChannelBuffered(
    Tcl_Channel chan)		/* The channel to query. */
{
    Channel *chanPtr = (Channel *) chan;
				/* Real channel structure. */
    ChannelBuffer *bufPtr;
    int bytesBuffered = 0;

    for (bufPtr = chanPtr->inQueueHead; bufPtr != NULL;
	    bufPtr = bufPtr->nextPtr) {
	bytesBuffered += BytesLeft(bufPtr);
    }

    return bytesBuffered;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetChannelBufferSize --
 *
 *	Sets the size of buffers to allocate to store input or output in the
 *	channel. The size must be between 1 byte and 1 MByte.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the size of buffers subsequently allocated for this channel.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetChannelBufferSize(
    Tcl_Channel chan,		/* The channel whose buffer size to set. */
    int sz)			/* The size to set. */
{
    ChannelState *statePtr;	/* State of real channel structure. */

    /*
     * Clip the buffer size to force it into the [1,1M] range
     */

    if (sz < 1) {
	sz = 1;
    } else if (sz > MAX_CHANNEL_BUFFER_SIZE) {
	sz = MAX_CHANNEL_BUFFER_SIZE;
    }

    statePtr = ((Channel *) chan)->state;

    if (statePtr->bufSize == sz) {
	return;
    }
    statePtr->bufSize = sz;

    /*
     * If bufsize changes, need to get rid of old utility buffer.
     */

    if (statePtr->saveInBufPtr != NULL) {
	RecycleBuffer(statePtr, statePtr->saveInBufPtr, 1);
	statePtr->saveInBufPtr = NULL;
    }
    if ((statePtr->inQueueHead != NULL)
	    && (statePtr->inQueueHead->nextPtr == NULL)
	    && IsBufferEmpty(statePtr->inQueueHead)) {
	RecycleBuffer(statePtr, statePtr->inQueueHead, 1);
	statePtr->inQueueHead = NULL;
	statePtr->inQueueTail = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelBufferSize --
 *
 *	Retrieves the size of buffers to allocate for this channel.
 *
 * Results:
 *	The size.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetChannelBufferSize(
    Tcl_Channel chan)		/* The channel for which to find the buffer
				 * size. */
{
    ChannelState *statePtr = ((Channel *) chan)->state;
				/* State of real channel structure. */

    return statePtr->bufSize;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_BadChannelOption --
 *
 *	This procedure generates a "bad option" error message in an (optional)
 *	interpreter. It is used by channel drivers when a invalid Set/Get
 *	option is requested. Its purpose is to concatenate the generic options
 *	list to the specific ones and factorize the generic options error
 *	message string.
 *
 * Results:
 *	TCL_ERROR.
 *
 * Side effects:

 *	An error message is generated in interp's result object to indicate
 *	that a command was invoked with the a bad option. The message has the
 *	form:
 *		bad option "blah": should be one of
 *		<...generic options...>+<...specific options...>
 *	"blah" is the optionName argument and "<specific options>" is a space
 *	separated list of specific option words. The function takes good care
 *	of inserting minus signs before each option, commas after, and an "or"
 *	before the last option.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_BadChannelOption(
    Tcl_Interp *interp,		/* Current interpreter (can be NULL).*/
    const char *optionName,	/* 'bad option' name */
    const char *optionList)	/* Specific options list to append to the
				 * standard generic options. Can be NULL for
				 * generic options only. */
{
    if (interp != NULL) {
	const char *genericopt =
		"blocking buffering buffersize encoding eofchar translation";
	const char **argv;
	int argc, i;
	Tcl_DString ds;
        Tcl_Obj *errObj;

	Tcl_DStringInit(&ds);
	Tcl_DStringAppend(&ds, genericopt, -1);
	if (optionList && (*optionList)) {
	    TclDStringAppendLiteral(&ds, " ");
	    Tcl_DStringAppend(&ds, optionList, -1);
	}
	if (Tcl_SplitList(interp, Tcl_DStringValue(&ds),
		&argc, &argv) != TCL_OK) {
	    Tcl_Panic("malformed option list in channel driver");
	}
	Tcl_ResetResult(interp);
	errObj = Tcl_ObjPrintf("bad option \"%s\": should be one of ",
                optionName);
	argc--;
	for (i = 0; i < argc; i++) {
	    Tcl_AppendPrintfToObj(errObj, "-%s, ", argv[i]);
	}
	Tcl_AppendPrintfToObj(errObj, "or -%s", argv[i]);
        Tcl_SetObjResult(interp, errObj);
	Tcl_DStringFree(&ds);
	ckfree(argv);
    }
    Tcl_SetErrno(EINVAL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelOption --
 *
 *	Gets a mode associated with an IO channel. If the optionName arg is
 *	non NULL, retrieves the value of that option. If the optionName arg is
 *	NULL, retrieves a list of alternating option names and values for the
 *	given channel.
 *
 * Results:
 *	A standard Tcl result. Also sets the supplied DString to the string
 *	value of the option(s) returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetChannelOption(
    Tcl_Interp *interp,		/* For error reporting - can be NULL. */
    Tcl_Channel chan,		/* Channel on which to get option. */
    const char *optionName,	/* Option to get. */
    Tcl_DString *dsPtr)		/* Where to store value(s). */
{
    size_t len;			/* Length of optionName string. */
    char optionVal[128];	/* Buffer for sprintf. */
    Channel *chanPtr = (Channel *) chan;
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    int flags;

    /*
     * Disallow options on dead channels -- channels that have been closed but
     * not yet been deallocated. Such channels can be found if the exit
     * handler for channel cleanup has run but the channel is still registered
     * in an interpreter.
     */

    if (CheckForDeadChannel(interp, statePtr)) {
	return TCL_ERROR;
    }

    /*
     * This operation should occur at the top of a channel stack.
     */

    chanPtr = statePtr->topChanPtr;

    /*
     * If we are in the middle of a background copy, use the saved flags.
     */

    if (statePtr->csPtrR) {
	flags = statePtr->csPtrR->readFlags;
    } else if (statePtr->csPtrW) {
	flags = statePtr->csPtrW->writeFlags;
    } else {
	flags = statePtr->flags;
    }

    /*
     * If the optionName is NULL it means that we want a list of all options
     * and values.
     */

    if (optionName == NULL) {
	len = 0;
    } else {
	len = strlen(optionName);
    }

    if (len == 0 || HaveOpt(2, "-blocking")) {
	if (len == 0) {
	    Tcl_DStringAppendElement(dsPtr, "-blocking");
	}
	Tcl_DStringAppendElement(dsPtr,
		(flags & CHANNEL_NONBLOCKING) ? "0" : "1");
	if (len > 0) {
	    return TCL_OK;
	}
    }
    if (len == 0 || HaveOpt(7, "-buffering")) {
	if (len == 0) {
	    Tcl_DStringAppendElement(dsPtr, "-buffering");
	}
	if (flags & CHANNEL_LINEBUFFERED) {
	    Tcl_DStringAppendElement(dsPtr, "line");
	} else if (flags & CHANNEL_UNBUFFERED) {
	    Tcl_DStringAppendElement(dsPtr, "none");
	} else {
	    Tcl_DStringAppendElement(dsPtr, "full");
	}
	if (len > 0) {
	    return TCL_OK;
	}
    }
    if (len == 0 || HaveOpt(7, "-buffersize")) {
	if (len == 0) {
	    Tcl_DStringAppendElement(dsPtr, "-buffersize");
	}
	TclFormatInt(optionVal, statePtr->bufSize);
	Tcl_DStringAppendElement(dsPtr, optionVal);
	if (len > 0) {
	    return TCL_OK;
	}
    }
    if (len == 0 || HaveOpt(2, "-encoding")) {
	if (len == 0) {
	    Tcl_DStringAppendElement(dsPtr, "-encoding");
	}
	if (statePtr->encoding == NULL) {
	    Tcl_DStringAppendElement(dsPtr, "binary");
	} else {
	    Tcl_DStringAppendElement(dsPtr,
		    Tcl_GetEncodingName(statePtr->encoding));
	}
	if (len > 0) {
	    return TCL_OK;
	}
    }
    if (len == 0 || HaveOpt(2, "-eofchar")) {
	if (len == 0) {
	    Tcl_DStringAppendElement(dsPtr, "-eofchar");
	}
	if (((flags & (TCL_READABLE|TCL_WRITABLE)) ==
		(TCL_READABLE|TCL_WRITABLE)) && (len == 0)) {
	    Tcl_DStringStartSublist(dsPtr);
	}
	if (flags & TCL_READABLE) {
	    if (statePtr->inEofChar == 0) {
		Tcl_DStringAppendElement(dsPtr, "");
	    } else {
		char buf[4];

		sprintf(buf, "%c", statePtr->inEofChar);
		Tcl_DStringAppendElement(dsPtr, buf);
	    }
	}
	if (flags & TCL_WRITABLE) {
	    if (statePtr->outEofChar == 0) {
		Tcl_DStringAppendElement(dsPtr, "");
	    } else {
		char buf[4];

		sprintf(buf, "%c", statePtr->outEofChar);
		Tcl_DStringAppendElement(dsPtr, buf);
	    }
	}
	if (!(flags & (TCL_READABLE|TCL_WRITABLE))) {
	    /*
	     * Not readable or writable (e.g. server socket)
	     */

	    Tcl_DStringAppendElement(dsPtr, "");
	}
	if (((flags & (TCL_READABLE|TCL_WRITABLE)) ==
		(TCL_READABLE|TCL_WRITABLE)) && (len == 0)) {
	    Tcl_DStringEndSublist(dsPtr);
	}
	if (len > 0) {
	    return TCL_OK;
	}
    }
    if (len == 0 || HaveOpt(1, "-translation")) {
	if (len == 0) {
	    Tcl_DStringAppendElement(dsPtr, "-translation");
	}
	if (((flags & (TCL_READABLE|TCL_WRITABLE)) ==
		(TCL_READABLE|TCL_WRITABLE)) && (len == 0)) {
	    Tcl_DStringStartSublist(dsPtr);
	}
	if (flags & TCL_READABLE) {
	    if (statePtr->inputTranslation == TCL_TRANSLATE_AUTO) {
		Tcl_DStringAppendElement(dsPtr, "auto");
	    } else if (statePtr->inputTranslation == TCL_TRANSLATE_CR) {
		Tcl_DStringAppendElement(dsPtr, "cr");
	    } else if (statePtr->inputTranslation == TCL_TRANSLATE_CRLF) {
		Tcl_DStringAppendElement(dsPtr, "crlf");
	    } else {
		Tcl_DStringAppendElement(dsPtr, "lf");
	    }
	}
	if (flags & TCL_WRITABLE) {
	    if (statePtr->outputTranslation == TCL_TRANSLATE_AUTO) {
		Tcl_DStringAppendElement(dsPtr, "auto");
	    } else if (statePtr->outputTranslation == TCL_TRANSLATE_CR) {
		Tcl_DStringAppendElement(dsPtr, "cr");
	    } else if (statePtr->outputTranslation == TCL_TRANSLATE_CRLF) {
		Tcl_DStringAppendElement(dsPtr, "crlf");
	    } else {
		Tcl_DStringAppendElement(dsPtr, "lf");
	    }
	}
	if (!(flags & (TCL_READABLE|TCL_WRITABLE))) {
	    /*
	     * Not readable or writable (e.g. server socket)
	     */

	    Tcl_DStringAppendElement(dsPtr, "auto");
	}
	if (((flags & (TCL_READABLE|TCL_WRITABLE)) ==
		(TCL_READABLE|TCL_WRITABLE)) && (len == 0)) {
	    Tcl_DStringEndSublist(dsPtr);
	}
	if (len > 0) {
	    return TCL_OK;
	}
    }

    if (chanPtr->typePtr->getOptionProc != NULL) {
	/*
	 * Let the driver specific handle additional options and result code
	 * and message.
	 */

	return chanPtr->typePtr->getOptionProc(chanPtr->instanceData, interp,
		optionName, dsPtr);
    } else {
	/*
	 * No driver specific options case.
	 */

	if (len == 0) {
	    return TCL_OK;
	}
	return Tcl_BadChannelOption(interp, optionName, NULL);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_SetChannelOption --
 *
 *	Sets an option on a channel.
 *
 * Results:
 *	A standard Tcl result. On error, sets interp's result object if
 *	interp is not NULL.
 *
 * Side effects:
 *	May modify an option on a device.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_SetChannelOption(
    Tcl_Interp *interp,		/* For error reporting - can be NULL. */
    Tcl_Channel chan,		/* Channel on which to set mode. */
    const char *optionName,	/* Which option to set? */
    const char *newValue)	/* New value for option. */
{
    Channel *chanPtr = (Channel *) chan;
				/* The real IO channel. */
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    size_t len;			/* Length of optionName string. */
    int argc;
    const char **argv;

    /*
     * If the channel is in the middle of a background copy, fail.
     */

    if (statePtr->csPtrR || statePtr->csPtrW) {
	if (interp) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
                    "unable to set channel options: background copy in"
                    " progress", -1));
	}
	return TCL_ERROR;
    }

    /*
     * Disallow options on dead channels -- channels that have been closed but
     * not yet been deallocated. Such channels can be found if the exit
     * handler for channel cleanup has run but the channel is still registered
     * in an interpreter.
     */

    if (CheckForDeadChannel(NULL, statePtr)) {
	return TCL_ERROR;
    }

    /*
     * This operation should occur at the top of a channel stack.
     */

    chanPtr = statePtr->topChanPtr;

    len = strlen(optionName);

    if (HaveOpt(2, "-blocking")) {
	int newMode;

	if (Tcl_GetBoolean(interp, newValue, &newMode) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	if (newMode) {
	    newMode = TCL_MODE_BLOCKING;
	} else {
	    newMode = TCL_MODE_NONBLOCKING;
	}
	return SetBlockMode(interp, chanPtr, newMode);
    } else if (HaveOpt(7, "-buffering")) {
	len = strlen(newValue);
	if ((newValue[0] == 'f') && (strncmp(newValue, "full", len) == 0)) {
	    ResetFlag(statePtr, CHANNEL_UNBUFFERED | CHANNEL_LINEBUFFERED);
	} else if ((newValue[0] == 'l') &&
		(strncmp(newValue, "line", len) == 0)) {
	    ResetFlag(statePtr, CHANNEL_UNBUFFERED);
	    SetFlag(statePtr, CHANNEL_LINEBUFFERED);
	} else if ((newValue[0] == 'n') &&
		(strncmp(newValue, "none", len) == 0)) {
	    ResetFlag(statePtr, CHANNEL_LINEBUFFERED);
	    SetFlag(statePtr, CHANNEL_UNBUFFERED);
	} else if (interp) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(
                    "bad value for -buffering: must be one of"
                    " full, line, or none", -1));
            return TCL_ERROR;
	}
	return TCL_OK;
    } else if (HaveOpt(7, "-buffersize")) {
	int newBufferSize;

	if (Tcl_GetInt(interp, newValue, &newBufferSize) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	Tcl_SetChannelBufferSize(chan, newBufferSize);
	return TCL_OK;
    } else if (HaveOpt(2, "-encoding")) {
	Tcl_Encoding encoding;

	if ((newValue[0] == '\0') || (strcmp(newValue, "binary") == 0)) {
	    encoding = NULL;
	} else {
	    encoding = Tcl_GetEncoding(interp, newValue);
	    if (encoding == NULL) {
		return TCL_ERROR;
	    }
	}

	/*
	 * When the channel has an escape sequence driven encoding such as
	 * iso2022, the terminated escape sequence must write to the buffer.
	 */

	if ((statePtr->encoding != NULL)
		&& !(statePtr->outputEncodingFlags & TCL_ENCODING_START)
		&& (CheckChannelErrors(statePtr, TCL_WRITABLE) == 0)) {
	    statePtr->outputEncodingFlags |= TCL_ENCODING_END;
	    WriteChars(chanPtr, "", 0);
	}
	Tcl_FreeEncoding(statePtr->encoding);
	statePtr->encoding = encoding;
	statePtr->inputEncodingState = NULL;
	statePtr->inputEncodingFlags = TCL_ENCODING_START;
	statePtr->outputEncodingState = NULL;
	statePtr->outputEncodingFlags = TCL_ENCODING_START;
	ResetFlag(statePtr, CHANNEL_NEED_MORE_DATA);
	UpdateInterest(chanPtr);
	return TCL_OK;
    } else if (HaveOpt(2, "-eofchar")) {
	if (Tcl_SplitList(interp, newValue, &argc, &argv) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	if (argc == 0) {
	    statePtr->inEofChar = 0;
	    statePtr->outEofChar = 0;
	} else if (argc == 1 || argc == 2) {
	    int outIndex = (argc - 1);
	    int inValue = (int) argv[0][0];
	    int outValue = (int) argv[outIndex][0];

	    if (inValue & 0x80 || outValue & 0x80) {
		if (interp) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
                            "bad value for -eofchar: must be non-NUL ASCII"
                            " character", -1));
		}
		ckfree(argv);
		return TCL_ERROR;
	    }
	    if (GotFlag(statePtr, TCL_READABLE)) {
		statePtr->inEofChar = inValue;
	    }
	    if (GotFlag(statePtr, TCL_WRITABLE)) {
		statePtr->outEofChar = outValue;
	    }
	} else {
	    if (interp) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"bad value for -eofchar: should be a list of zero,"
			" one, or two elements", -1));
	    }
	    ckfree(argv);
	    return TCL_ERROR;
	}
	if (argv != NULL) {
	    ckfree(argv);
	}

	/*
	 * [Bug 930851] Reset EOF and BLOCKED flags. Changing the character
	 * which signals eof can transform a current eof condition into a 'go
	 * ahead'. Ditto for blocked.
	 */

	if (GotFlag(statePtr, CHANNEL_EOF)) {
	    statePtr->inputEncodingFlags |= TCL_ENCODING_START;
	}
	ResetFlag(statePtr, CHANNEL_EOF|CHANNEL_STICKY_EOF|CHANNEL_BLOCKED);
	statePtr->inputEncodingFlags &= ~TCL_ENCODING_END;
	return TCL_OK;
    } else if (HaveOpt(1, "-translation")) {
	const char *readMode, *writeMode;

	if (Tcl_SplitList(interp, newValue, &argc, &argv) == TCL_ERROR) {
	    return TCL_ERROR;
	}

	if (argc == 1) {
	    readMode = GotFlag(statePtr, TCL_READABLE) ? argv[0] : NULL;
	    writeMode = GotFlag(statePtr, TCL_WRITABLE) ? argv[0] : NULL;
	} else if (argc == 2) {
	    readMode = GotFlag(statePtr, TCL_READABLE) ? argv[0] : NULL;
	    writeMode = GotFlag(statePtr, TCL_WRITABLE) ? argv[1] : NULL;
	} else {
	    if (interp) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"bad value for -translation: must be a one or two"
			" element list", -1));
	    }
	    ckfree(argv);
	    return TCL_ERROR;
	}

	if (readMode) {
	    TclEolTranslation translation;

	    if (*readMode == '\0') {
		translation = statePtr->inputTranslation;
	    } else if (strcmp(readMode, "auto") == 0) {
		translation = TCL_TRANSLATE_AUTO;
	    } else if (strcmp(readMode, "binary") == 0) {
		translation = TCL_TRANSLATE_LF;
		statePtr->inEofChar = 0;
		Tcl_FreeEncoding(statePtr->encoding);
		statePtr->encoding = NULL;
	    } else if (strcmp(readMode, "lf") == 0) {
		translation = TCL_TRANSLATE_LF;
	    } else if (strcmp(readMode, "cr") == 0) {
		translation = TCL_TRANSLATE_CR;
	    } else if (strcmp(readMode, "crlf") == 0) {
		translation = TCL_TRANSLATE_CRLF;
	    } else if (strcmp(readMode, "platform") == 0) {
		translation = TCL_PLATFORM_TRANSLATION;
	    } else {
		if (interp) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "bad value for -translation: must be one of "
                            "auto, binary, cr, lf, crlf, or platform", -1));
		}
		ckfree(argv);
		return TCL_ERROR;
	    }

	    /*
	     * Reset the EOL flags since we need to look at any buffered data
	     * to see if the new translation mode allows us to complete the
	     * line.
	     */

	    if (translation != statePtr->inputTranslation) {
		statePtr->inputTranslation = translation;
		ResetFlag(statePtr, INPUT_SAW_CR | CHANNEL_NEED_MORE_DATA);
		UpdateInterest(chanPtr);
	    }
	}
	if (writeMode) {
	    if (*writeMode == '\0') {
		/* Do nothing. */
	    } else if (strcmp(writeMode, "auto") == 0) {
		/*
		 * This is a hack to get TCP sockets to produce output in CRLF
		 * mode if they are being set into AUTO mode. A better
		 * solution for achieving this effect will be coded later.
		 */

		if (strcmp(Tcl_ChannelName(chanPtr->typePtr), "tcp") == 0) {
		    statePtr->outputTranslation = TCL_TRANSLATE_CRLF;
		} else {
		    statePtr->outputTranslation = TCL_PLATFORM_TRANSLATION;
		}
	    } else if (strcmp(writeMode, "binary") == 0) {
		statePtr->outEofChar = 0;
		statePtr->outputTranslation = TCL_TRANSLATE_LF;
		Tcl_FreeEncoding(statePtr->encoding);
		statePtr->encoding = NULL;
	    } else if (strcmp(writeMode, "lf") == 0) {
		statePtr->outputTranslation = TCL_TRANSLATE_LF;
	    } else if (strcmp(writeMode, "cr") == 0) {
		statePtr->outputTranslation = TCL_TRANSLATE_CR;
	    } else if (strcmp(writeMode, "crlf") == 0) {
		statePtr->outputTranslation = TCL_TRANSLATE_CRLF;
	    } else if (strcmp(writeMode, "platform") == 0) {
		statePtr->outputTranslation = TCL_PLATFORM_TRANSLATION;
	    } else {
		if (interp) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "bad value for -translation: must be one of "
                            "auto, binary, cr, lf, crlf, or platform", -1));
		}
		ckfree(argv);
		return TCL_ERROR;
	    }
	}
	ckfree(argv);
	return TCL_OK;
    } else if (chanPtr->typePtr->setOptionProc != NULL) {
	return chanPtr->typePtr->setOptionProc(chanPtr->instanceData, interp,
		optionName, newValue);
    } else {
	return Tcl_BadChannelOption(interp, optionName, NULL);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CleanupChannelHandlers --
 *
 *	Removes channel handlers that refer to the supplied interpreter, so
 *	that if the actual channel is not closed now, these handlers will not
 *	run on subsequent events on the channel. This would be erroneous,
 *	because the interpreter no longer has a reference to this channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes channel handlers.
 *
 *----------------------------------------------------------------------
 */

static void
CleanupChannelHandlers(
    Tcl_Interp *interp,
    Channel *chanPtr)
{
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    EventScriptRecord *sPtr, *prevPtr, *nextPtr;

    /*
     * Remove fileevent records on this channel that refer to the given
     * interpreter.
     */

    for (sPtr = statePtr->scriptRecordPtr, prevPtr = NULL;
	    sPtr != NULL; sPtr = nextPtr) {
	nextPtr = sPtr->nextPtr;
	if (sPtr->interp == interp) {
	    if (prevPtr == NULL) {
		statePtr->scriptRecordPtr = nextPtr;
	    } else {
		prevPtr->nextPtr = nextPtr;
	    }

	    Tcl_DeleteChannelHandler((Tcl_Channel) chanPtr,
		    TclChannelEventScriptInvoker, sPtr);

	    TclDecrRefCount(sPtr->scriptPtr);
	    ckfree(sPtr);
	} else {
	    prevPtr = sPtr;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_NotifyChannel --
 *
 *	This procedure is called by a channel driver when a driver detects an
 *	event on a channel. This procedure is responsible for actually
 *	handling the event by invoking any channel handler callbacks.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Whatever the channel handler callback procedure does.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_NotifyChannel(
    Tcl_Channel channel,	/* Channel that detected an event. */
    int mask)			/* OR'ed combination of TCL_READABLE,
				 * TCL_WRITABLE, or TCL_EXCEPTION: indicates
				 * which events were detected. */
{
    Channel *chanPtr = (Channel *) channel;
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    ChannelHandler *chPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    NextChannelHandler nh;
    Channel *upChanPtr;
    const Tcl_ChannelType *upTypePtr;

    /*
     * In contrast to the other API functions this procedure walks towards the
     * top of a stack and not down from it.
     *
     * The channel calling this procedure is the one who generated the event,
     * and thus does not take part in handling it. IOW, its HandlerProc is not
     * called, instead we begin with the channel above it.
     *
     * This behaviour also allows the transformation channels to generate
     * their own events and pass them upward.
     */

    while (mask && (chanPtr->upChanPtr != NULL)) {
	Tcl_DriverHandlerProc *upHandlerProc;

	upChanPtr = chanPtr->upChanPtr;
	upTypePtr = upChanPtr->typePtr;
	upHandlerProc = Tcl_ChannelHandlerProc(upTypePtr);
	if (upHandlerProc != NULL) {
	    mask = upHandlerProc(upChanPtr->instanceData, mask);
	}

	/*
	 * ELSE: Ignore transformations which are unable to handle the event
	 * coming from below. Assume that they don't change the mask and pass
	 * it on.
	 */

	chanPtr = upChanPtr;
    }

    channel = (Tcl_Channel) chanPtr;

    /*
     * Here we have either reached the top of the stack or the mask is empty.
     * We break out of the procedure if it is the latter.
     */

    if (!mask) {
	return;
    }

    /*
     * We are now above the topmost channel in a stack and have events left.
     * Now call the channel handlers as usual.
     *
     * Preserve the channel struct in case the script closes it.
     */

    TclChannelPreserve((Tcl_Channel)channel);
    Tcl_Preserve(statePtr);

    /*
     * If we are flushing in the background, be sure to call FlushChannel for
     * writable events. Note that we have to discard the writable event so we
     * don't call any write handlers before the flush is complete.
     */

    if (GotFlag(statePtr, BG_FLUSH_SCHEDULED) && (mask & TCL_WRITABLE)) {
	if (0 == FlushChannel(NULL, chanPtr, 1)) {
	    mask &= ~TCL_WRITABLE;
	}
    }

    /*
     * Add this invocation to the list of recursive invocations of
     * Tcl_NotifyChannel.
     */

    nh.nextHandlerPtr = NULL;
    nh.nestedHandlerPtr = tsdPtr->nestedHandlerPtr;
    tsdPtr->nestedHandlerPtr = &nh;

    for (chPtr = statePtr->chPtr; chPtr != NULL; ) {
	/*
	 * If this channel handler is interested in any of the events that
	 * have occurred on the channel, invoke its procedure.
	 */

	if ((chPtr->mask & mask) != 0) {
	    nh.nextHandlerPtr = chPtr->nextPtr;
	    chPtr->proc(chPtr->clientData, chPtr->mask & mask);
	    chPtr = nh.nextHandlerPtr;
	} else {
	    chPtr = chPtr->nextPtr;
	}
    }

    /*
     * Update the notifier interest, since it may have changed after invoking
     * event handlers. Skip that if the channel was deleted in the call to the
     * channel handler.
     */

    if (chanPtr->typePtr != NULL) {
	/*
	 * TODO: This call may not be needed.  If a handler induced a
	 * change in interest, that handler should have made its own
	 * UpdateInterest() call, one would think.
	 */
	UpdateInterest(chanPtr);
    }

    Tcl_Release(statePtr);
    TclChannelRelease(channel);

    tsdPtr->nestedHandlerPtr = nh.nestedHandlerPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateInterest --
 *
 *	Arrange for the notifier to call us back at appropriate times based on
 *	the current state of the channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May schedule a timer or driver handler.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateInterest(
    Channel *chanPtr)		/* Channel to update. */
{
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    int mask = statePtr->interestMask;

    if (chanPtr->typePtr == NULL) {
	/* Do not update interest on a closed channel */
	return;
    }

    /*
     * If there are flushed buffers waiting to be written, then we need to
     * watch for the channel to become writable.
     */

    if (GotFlag(statePtr, BG_FLUSH_SCHEDULED)) {
	mask |= TCL_WRITABLE;
    }

    /*
     * If there is data in the input queue, and we aren't waiting for more
     * data, then we need to schedule a timer so we don't block in the
     * notifier. Also, cancel the read interest so we don't get duplicate
     * events.
     */

    if (mask & TCL_READABLE) {
	if (!GotFlag(statePtr, CHANNEL_NEED_MORE_DATA)
		&& (statePtr->inQueueHead != NULL)
		&& IsBufferReady(statePtr->inQueueHead)) {
	    mask &= ~TCL_READABLE;

	    /*
	     * Andreas Kupries, April 11, 2003
	     *
	     * Some operating systems (Solaris 2.6 and higher (but not Solaris
	     * 2.5, go figure)) generate READABLE and EXCEPTION events when
	     * select()'ing [*] on a plain file, even if EOF was not yet
	     * reached. This is a problem in the following situation:
	     *
	     * - An extension asks to get both READABLE and EXCEPTION events.
	     * - It reads data into a buffer smaller than the buffer used by
	     *	 Tcl itself.
	     * - It does not process all events in the event queue, but only
	     *	 one, at least in some situations.
	     *
	     * In that case we can get into a situation where
	     *
	     * - Tcl drops READABLE here, because it has data in its own
	     *	 buffers waiting to be read by the extension.
	     * - A READABLE event is syntesized via timer.
	     * - The OS still reports the EXCEPTION condition on the file.
	     * - And the extension gets the EXCPTION event first, and handles
	     *	 this as EOF.
	     *
	     * End result ==> Premature end of reading from a file.
	     *
	     * The concrete example is 'Expect', and its [expect] command
	     * (and at the C-level, deep in the bowels of Expect,
	     * 'exp_get_next_event'. See marker 'SunOS' for commentary in
	     * that function too).
	     *
	     * [*] As the Tcl notifier does. See also for marker 'SunOS' in
	     * file 'exp_event.c' of Expect.
	     *
	     * Our solution here is to drop the interest in the EXCEPTION
	     * events too. This compiles on all platforms, and also passes the
	     * testsuite on all of them.
	     */

	    mask &= ~TCL_EXCEPTION;

	    if (!statePtr->timer) {
		statePtr->timer = Tcl_CreateTimerHandler(SYNTHETIC_EVENT_TIME,
                        ChannelTimerProc, chanPtr);
	    }
	}
    }
    ChanWatch(chanPtr, mask);
}

/*
 *----------------------------------------------------------------------
 *
 * ChannelTimerProc --
 *
 *	Timer handler scheduled by UpdateInterest to monitor the channel
 *	buffers until they are empty.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May invoke channel handlers.
 *
 *----------------------------------------------------------------------
 */

static void
ChannelTimerProc(
    ClientData clientData)
{
    Channel *chanPtr = clientData;
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */

    if (!GotFlag(statePtr, CHANNEL_NEED_MORE_DATA)
	    && (statePtr->interestMask & TCL_READABLE)
	    && (statePtr->inQueueHead != NULL)
	    && IsBufferReady(statePtr->inQueueHead)) {
	/*
	 * Restart the timer in case a channel handler reenters the event loop
	 * before UpdateInterest gets called by Tcl_NotifyChannel.
	 */

	statePtr->timer = Tcl_CreateTimerHandler(SYNTHETIC_EVENT_TIME,
                ChannelTimerProc,chanPtr);
	Tcl_Preserve(statePtr);
	Tcl_NotifyChannel((Tcl_Channel) chanPtr, TCL_READABLE);
	Tcl_Release(statePtr);
    } else {
	statePtr->timer = NULL;
	UpdateInterest(chanPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateChannelHandler --
 *
 *	Arrange for a given procedure to be invoked whenever the channel
 *	indicated by the chanPtr arg becomes readable or writable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	From now on, whenever the I/O channel given by chanPtr becomes ready
 *	in the way indicated by mask, proc will be invoked. See the manual
 *	entry for details on the calling sequence to proc. If there is already
 *	an event handler for chan, proc and clientData, then the mask will be
 *	updated.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_CreateChannelHandler(
    Tcl_Channel chan,		/* The channel to create the handler for. */
    int mask,			/* OR'ed combination of TCL_READABLE,
				 * TCL_WRITABLE, and TCL_EXCEPTION: indicates
				 * conditions under which proc should be
				 * called. Use 0 to disable a registered
				 * handler. */
    Tcl_ChannelProc *proc,	/* Procedure to call for each selected
				 * event. */
    ClientData clientData)	/* Arbitrary data to pass to proc. */
{
    ChannelHandler *chPtr;
    Channel *chanPtr = (Channel *) chan;
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */

    /*
     * Check whether this channel handler is not already registered. If it is
     * not, create a new record, else reuse existing record (smash current
     * values).
     */

    for (chPtr = statePtr->chPtr; chPtr != NULL; chPtr = chPtr->nextPtr) {
	if ((chPtr->chanPtr == chanPtr) && (chPtr->proc == proc) &&
		(chPtr->clientData == clientData)) {
	    break;
	}
    }
    if (chPtr == NULL) {
	chPtr = ckalloc(sizeof(ChannelHandler));
	chPtr->mask = 0;
	chPtr->proc = proc;
	chPtr->clientData = clientData;
	chPtr->chanPtr = chanPtr;
	chPtr->nextPtr = statePtr->chPtr;
	statePtr->chPtr = chPtr;
    }

    /*
     * The remainder of the initialization below is done regardless of whether
     * or not this is a new record or a modification of an old one.
     */

    chPtr->mask = mask;

    /*
     * Recompute the interest mask for the channel - this call may actually be
     * disabling an existing handler.
     */

    statePtr->interestMask = 0;
    for (chPtr = statePtr->chPtr; chPtr != NULL; chPtr = chPtr->nextPtr) {
	statePtr->interestMask |= chPtr->mask;
    }

    UpdateInterest(statePtr->topChanPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteChannelHandler --
 *
 *	Cancel a previously arranged callback arrangement for an IO channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If a callback was previously registered for this chan, proc and
 *	clientData, it is removed and the callback will no longer be called
 *	when the channel becomes ready for IO.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteChannelHandler(
    Tcl_Channel chan,		/* The channel for which to remove the
				 * callback. */
    Tcl_ChannelProc *proc,	/* The procedure in the callback to delete. */
    ClientData clientData)	/* The client data in the callback to
				 * delete. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    ChannelHandler *chPtr, *prevChPtr;
    Channel *chanPtr = (Channel *) chan;
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    NextChannelHandler *nhPtr;

    /*
     * Find the entry and the previous one in the list.
     */

    for (prevChPtr = NULL, chPtr = statePtr->chPtr; chPtr != NULL;
	    chPtr = chPtr->nextPtr) {
	if ((chPtr->chanPtr == chanPtr) && (chPtr->clientData == clientData)
		&& (chPtr->proc == proc)) {
	    break;
	}
	prevChPtr = chPtr;
    }

    /*
     * If not found, return without doing anything.
     */

    if (chPtr == NULL) {
	return;
    }

    /*
     * If Tcl_NotifyChannel is about to process this handler, tell it to
     * process the next one instead - we are going to delete *this* one.
     */

    for (nhPtr = tsdPtr->nestedHandlerPtr; nhPtr != NULL;
	    nhPtr = nhPtr->nestedHandlerPtr) {
	if (nhPtr->nextHandlerPtr == chPtr) {
	    nhPtr->nextHandlerPtr = chPtr->nextPtr;
	}
    }

    /*
     * Splice it out of the list of channel handlers.
     */

    if (prevChPtr == NULL) {
	statePtr->chPtr = chPtr->nextPtr;
    } else {
	prevChPtr->nextPtr = chPtr->nextPtr;
    }
    ckfree(chPtr);

    /*
     * Recompute the interest list for the channel, so that infinite loops
     * will not result if Tcl_DeleteChannelHandler is called inside an event.
     */

    statePtr->interestMask = 0;
    for (chPtr = statePtr->chPtr; chPtr != NULL; chPtr = chPtr->nextPtr) {
	statePtr->interestMask |= chPtr->mask;
    }

    UpdateInterest(statePtr->topChanPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteScriptRecord --
 *
 *	Delete a script record for this combination of channel, interp and
 *	mask.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deletes a script record and cancels a channel event handler.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteScriptRecord(
    Tcl_Interp *interp,		/* Interpreter in which script was to be
				 * executed. */
    Channel *chanPtr,		/* The channel for which to delete the script
				 * record (if any). */
    int mask)			/* Events in mask must exactly match mask of
				 * script to delete. */
{
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    EventScriptRecord *esPtr, *prevEsPtr;

    for (esPtr = statePtr->scriptRecordPtr, prevEsPtr = NULL; esPtr != NULL;
	    prevEsPtr = esPtr, esPtr = esPtr->nextPtr) {
	if ((esPtr->interp == interp) && (esPtr->mask == mask)) {
	    if (esPtr == statePtr->scriptRecordPtr) {
		statePtr->scriptRecordPtr = esPtr->nextPtr;
	    } else {
		CLANG_ASSERT(prevEsPtr);
		prevEsPtr->nextPtr = esPtr->nextPtr;
	    }

	    Tcl_DeleteChannelHandler((Tcl_Channel) chanPtr,
		    TclChannelEventScriptInvoker, esPtr);

	    TclDecrRefCount(esPtr->scriptPtr);
	    ckfree(esPtr);

	    break;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CreateScriptRecord --
 *
 *	Creates a record to store a script to be executed when a specific
 *	event fires on a specific channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Causes the script to be stored for later execution.
 *
 *----------------------------------------------------------------------
 */

static void
CreateScriptRecord(
    Tcl_Interp *interp,		/* Interpreter in which to execute the stored
				 * script. */
    Channel *chanPtr,		/* Channel for which script is to be stored */
    int mask,			/* Set of events for which script will be
				 * invoked. */
    Tcl_Obj *scriptPtr)		/* Pointer to script object. */
{
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */
    EventScriptRecord *esPtr;
    int makeCH;

    for (esPtr=statePtr->scriptRecordPtr; esPtr!=NULL; esPtr=esPtr->nextPtr) {
	if ((esPtr->interp == interp) && (esPtr->mask == mask)) {
	    TclDecrRefCount(esPtr->scriptPtr);
	    esPtr->scriptPtr = NULL;
	    break;
	}
    }

    makeCH = (esPtr == NULL);

    if (makeCH) {
	esPtr = ckalloc(sizeof(EventScriptRecord));
    }

    /*
     * Initialize the structure before calling Tcl_CreateChannelHandler,
     * because a reflected channel calling 'chan postevent' aka
     * 'Tcl_NotifyChannel' in its 'watch'Proc will invoke
     * 'TclChannelEventScriptInvoker' immediately, and we do not wish it to
     * see uninitialized memory and crash. See [Bug 2918110].
     */

    esPtr->chanPtr = chanPtr;
    esPtr->interp = interp;
    esPtr->mask = mask;
    Tcl_IncrRefCount(scriptPtr);
    esPtr->scriptPtr = scriptPtr;

    if (makeCH) {
	esPtr->nextPtr = statePtr->scriptRecordPtr;
	statePtr->scriptRecordPtr = esPtr;

	Tcl_CreateChannelHandler((Tcl_Channel) chanPtr, mask,
		TclChannelEventScriptInvoker, esPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclChannelEventScriptInvoker --
 *
 *	Invokes a script scheduled by "fileevent" for when the channel becomes
 *	ready for IO. This function is invoked by the channel handler which
 *	was created by the Tcl "fileevent" command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Whatever the script does.
 *
 *----------------------------------------------------------------------
 */

void
TclChannelEventScriptInvoker(
    ClientData clientData,	/* The script+interp record. */
    int mask)			/* Not used. */
{
    Tcl_Interp *interp;		/* Interpreter in which to eval the script. */
    Channel *chanPtr;		/* The channel for which this handler is
				 * registered. */
    EventScriptRecord *esPtr;	/* The event script + interpreter to eval it
				 * in. */
    int result;			/* Result of call to eval script. */

    esPtr = clientData;
    chanPtr = esPtr->chanPtr;
    mask = esPtr->mask;
    interp = esPtr->interp;

    /*
     * We must preserve the interpreter so we can report errors on it later.
     * Note that we do not need to preserve the channel because that is done
     * by Tcl_NotifyChannel before calling channel handlers.
     */

    Tcl_Preserve(interp);
    TclChannelPreserve((Tcl_Channel)chanPtr);
    result = Tcl_EvalObjEx(interp, esPtr->scriptPtr, TCL_EVAL_GLOBAL);

    /*
     * On error, cause a background error and remove the channel handler and
     * the script record.
     *
     * NOTE: Must delete channel handler before causing the background error
     * because the background error may want to reinstall the handler.
     */

    if (result != TCL_OK) {
	if (chanPtr->typePtr != NULL) {
	    DeleteScriptRecord(interp, chanPtr, mask);
	}
	Tcl_BackgroundException(interp, result);
    }
    TclChannelRelease((Tcl_Channel)chanPtr);
    Tcl_Release(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FileEventObjCmd --
 *
 *	This procedure implements the "fileevent" Tcl command. See the user
 *	documentation for details on what it does. This command is based on
 *	the Tk command "fileevent" which in turn is based on work contributed
 *	by Mark Diekhans.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May create a channel handler for the specified channel.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_FileEventObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter in which the channel for which
				 * to create the handler is found. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Channel *chanPtr;		/* The channel to create the handler for. */
    ChannelState *statePtr;	/* State info for channel */
    Tcl_Channel chan;		/* The opaque type for the channel. */
    const char *chanName;
    int modeIndex;		/* Index of mode argument. */
    int mask;
    static const char *const modeOptions[] = {"readable", "writable", NULL};
    static const int maskArray[] = {TCL_READABLE, TCL_WRITABLE};

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId event ?script?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[2], modeOptions, "event name", 0,
	    &modeIndex) != TCL_OK) {
	return TCL_ERROR;
    }
    mask = maskArray[modeIndex];

    chanName = TclGetString(objv[1]);
    chan = Tcl_GetChannel(interp, chanName, NULL);
    if (chan == NULL) {
	return TCL_ERROR;
    }
    chanPtr = (Channel *) chan;
    statePtr = chanPtr->state;
    if ((statePtr->flags & mask) == 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("channel is not %s",
		(mask == TCL_READABLE) ? "readable" : "writable"));
	return TCL_ERROR;
    }

    /*
     * If we are supposed to return the script, do so.
     */

    if (objc == 3) {
	EventScriptRecord *esPtr;

	for (esPtr = statePtr->scriptRecordPtr; esPtr != NULL;
		esPtr = esPtr->nextPtr) {
	    if ((esPtr->interp == interp) && (esPtr->mask == mask)) {
		Tcl_SetObjResult(interp, esPtr->scriptPtr);
		break;
	    }
	}
	return TCL_OK;
    }

    /*
     * If we are supposed to delete a stored script, do so.
     */

    if (*(TclGetString(objv[3])) == '\0') {
	DeleteScriptRecord(interp, chanPtr, mask);
	return TCL_OK;
    }

    /*
     * Make the script record that will link between the event and the script
     * to invoke. This also creates a channel event handler which will
     * evaluate the script in the supplied interpreter.
     */

    CreateScriptRecord(interp, chanPtr, mask, objv[3]);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ZeroTransferTimerProc --
 *
 *	Timer handler scheduled by TclCopyChannel so that -command is
 *	called asynchronously even when -size is 0.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls CopyData for -command invocation.
 *
 *----------------------------------------------------------------------
 */

static void
ZeroTransferTimerProc(
    ClientData clientData)
{
    /* calling CopyData with mask==0 still implies immediate invocation of the
     *  -command callback, and completion of the fcopy.
     */
    CopyData(clientData, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * TclCopyChannel --
 *
 *	This routine copies data from one channel to another, either
 *	synchronously or asynchronously. If a command script is supplied, the
 *	operation runs in the background. The script is invoked when the copy
 *	completes. Otherwise the function waits until the copy is completed
 *	before returning.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May schedule a background copy operation that causes both channels to
 *	be marked busy.
 *
 *----------------------------------------------------------------------
 */

int
TclCopyChannelOld(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Channel inChan,		/* Channel to read from. */
    Tcl_Channel outChan,	/* Channel to write to. */
    int toRead,			/* Amount of data to copy, or -1 for all. */
    Tcl_Obj *cmdPtr)		/* Pointer to script to execute or NULL. */
{
    return TclCopyChannel(interp, inChan, outChan, (Tcl_WideInt) toRead,
            cmdPtr);
}

int
TclCopyChannel(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Channel inChan,		/* Channel to read from. */
    Tcl_Channel outChan,	/* Channel to write to. */
    Tcl_WideInt toRead,		/* Amount of data to copy, or -1 for all. */
    Tcl_Obj *cmdPtr)		/* Pointer to script to execute or NULL. */
{
    Channel *inPtr = (Channel *) inChan;
    Channel *outPtr = (Channel *) outChan;
    ChannelState *inStatePtr, *outStatePtr;
    int readFlags, writeFlags;
    CopyState *csPtr;
    int nonBlocking = (cmdPtr) ? CHANNEL_NONBLOCKING : 0;
    int moveBytes;

    inStatePtr = inPtr->state;
    outStatePtr = outPtr->state;

    if (BUSY_STATE(inStatePtr, TCL_READABLE)) {
	if (interp) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                    "channel \"%s\" is busy", Tcl_GetChannelName(inChan)));
	}
	return TCL_ERROR;
    }
    if (BUSY_STATE(outStatePtr, TCL_WRITABLE)) {
	if (interp) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                    "channel \"%s\" is busy", Tcl_GetChannelName(outChan)));
	}
	return TCL_ERROR;
    }

    readFlags = inStatePtr->flags;
    writeFlags = outStatePtr->flags;

    /*
     * Set up the blocking mode appropriately. Background copies need
     * non-blocking channels. Foreground copies need blocking channels. If
     * there is an error, restore the old blocking mode.
     */

    if (nonBlocking != (readFlags & CHANNEL_NONBLOCKING)) {
	if (SetBlockMode(interp, inPtr, nonBlocking ?
		TCL_MODE_NONBLOCKING : TCL_MODE_BLOCKING) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if ((inPtr!=outPtr) && (nonBlocking!=(writeFlags&CHANNEL_NONBLOCKING)) &&
	    (SetBlockMode(NULL, outPtr, nonBlocking ?
		    TCL_MODE_NONBLOCKING : TCL_MODE_BLOCKING) != TCL_OK) &&
	    (nonBlocking != (readFlags & CHANNEL_NONBLOCKING))) {
	SetBlockMode(NULL, inPtr, (readFlags & CHANNEL_NONBLOCKING)
		? TCL_MODE_NONBLOCKING : TCL_MODE_BLOCKING);
	return TCL_ERROR;
    }

    /*
     * Make sure the output side is unbuffered.
     */

    outStatePtr->flags = (outStatePtr->flags & ~CHANNEL_LINEBUFFERED)
	    | CHANNEL_UNBUFFERED;

    /*
     * Test for conditions where we know we can just move bytes from input
     * channel to output channel with no transformation or even examination
     * of the bytes themselves.
     */

    moveBytes = inStatePtr->inEofChar == '\0'	/* No eofChar to stop input */
	    && inStatePtr->inputTranslation == TCL_TRANSLATE_LF
	    && outStatePtr->outputTranslation == TCL_TRANSLATE_LF
	    && inStatePtr->encoding == outStatePtr->encoding;

    /*
     * Allocate a new CopyState to maintain info about the current copy in
     * progress. This structure will be deallocated when the copy is
     * completed.
     */

    csPtr = ckalloc(sizeof(CopyState) + !moveBytes * inStatePtr->bufSize);
    csPtr->bufSize = !moveBytes * inStatePtr->bufSize;
    csPtr->readPtr = inPtr;
    csPtr->writePtr = outPtr;
    csPtr->readFlags = readFlags;
    csPtr->writeFlags = writeFlags;
    csPtr->toRead = toRead;
    csPtr->total = (Tcl_WideInt) 0;
    csPtr->interp = interp;
    if (cmdPtr) {
	Tcl_IncrRefCount(cmdPtr);
    }
    csPtr->cmdPtr = cmdPtr;

    inStatePtr->csPtrR  = csPtr;
    outStatePtr->csPtrW = csPtr;

    if (moveBytes) {
	return MoveBytes(csPtr);
    }

    /*
     * Special handling of -size 0 async transfers, so that the -command is
     * still called asynchronously.
     */

    if ((nonBlocking == CHANNEL_NONBLOCKING) && (toRead == 0)) {
        Tcl_CreateTimerHandler(0, ZeroTransferTimerProc, csPtr);
        return 0;
    }

    /*
     * Start copying data between the channels.
     */

    return CopyData(csPtr, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * CopyData --
 *
 *	This function implements the lowest level of the copying mechanism for
 *	TclCopyChannel.
 *
 * Results:
 *	Returns TCL_OK on success, else TCL_ERROR.
 *
 * Side effects:
 *	Moves data between channels, may create channel handlers.
 *
 *----------------------------------------------------------------------
 */

static void
MBCallback(
    CopyState *csPtr,
    Tcl_Obj *errObj)
{
    Tcl_Obj *cmdPtr = Tcl_DuplicateObj(csPtr->cmdPtr);
    Tcl_WideInt total = csPtr->total;
    Tcl_Interp *interp = csPtr->interp;
    int code;

    Tcl_IncrRefCount(cmdPtr);
    StopCopy(csPtr);

    /* TODO: What if cmdPtr is not a list?! */

    Tcl_ListObjAppendElement(NULL, cmdPtr, Tcl_NewWideIntObj(total));
    if (errObj) {
	Tcl_ListObjAppendElement(NULL, cmdPtr, errObj);
    }

    Tcl_Preserve(interp);
    code = Tcl_EvalObjEx(interp, cmdPtr, TCL_EVAL_GLOBAL);
    if (code != TCL_OK) {
	Tcl_BackgroundException(interp, code);
    }
    Tcl_Release(interp);
    TclDecrRefCount(cmdPtr);
}

static void
MBError(
    CopyState *csPtr,
    int mask,
    int errorCode)
{
    Tcl_Channel inChan = (Tcl_Channel) csPtr->readPtr;
    Tcl_Channel outChan = (Tcl_Channel) csPtr->writePtr;
    Tcl_Obj *errObj;

    Tcl_SetErrno(errorCode);

    errObj = Tcl_ObjPrintf( "error %sing \"%s\": %s",
	    (mask & TCL_READABLE) ? "read" : "writ",
	    Tcl_GetChannelName((mask & TCL_READABLE) ? inChan : outChan),
	    Tcl_PosixError(csPtr->interp));

    if (csPtr->cmdPtr) {
	MBCallback(csPtr, errObj);
    } else {
	Tcl_SetObjResult(csPtr->interp, errObj);
	StopCopy(csPtr);
    }
}

static void
MBEvent(
    ClientData clientData,
    int mask)
{
    CopyState *csPtr = (CopyState *) clientData;
    Tcl_Channel inChan = (Tcl_Channel) csPtr->readPtr;
    Tcl_Channel outChan = (Tcl_Channel) csPtr->writePtr;
    ChannelState *inStatePtr = csPtr->readPtr->state;

    if (mask & TCL_WRITABLE) {
	Tcl_DeleteChannelHandler(inChan, MBEvent, csPtr);
	Tcl_DeleteChannelHandler(outChan, MBEvent, csPtr);
	switch (MBWrite(csPtr)) {
	case TCL_OK:
	    MBCallback(csPtr, NULL);
	    break;
	case TCL_CONTINUE:
	    Tcl_CreateChannelHandler(inChan, TCL_READABLE, MBEvent, csPtr);
	    break;
	}
    } else if (mask & TCL_READABLE) {
	if (TCL_OK == MBRead(csPtr)) {
	    /* When at least one full buffer is present, stop reading. */
	    if (IsBufferFull(inStatePtr->inQueueHead)
		    || !Tcl_InputBlocked(inChan)) {
		Tcl_DeleteChannelHandler(inChan, MBEvent, csPtr);
	    }

	    /* Successful read -- set up to write the bytes we read */
	    Tcl_CreateChannelHandler(outChan, TCL_WRITABLE, MBEvent, csPtr);
	}
    }
}

static int
MBRead(
    CopyState *csPtr)
{
    ChannelState *inStatePtr = csPtr->readPtr->state;
    ChannelBuffer *bufPtr = inStatePtr->inQueueHead;
    int code;

    if (bufPtr && BytesLeft(bufPtr) > 0) {
	return TCL_OK;
    }

    code = GetInput(inStatePtr->topChanPtr);
    if (code == 0 || GotFlag(inStatePtr, CHANNEL_BLOCKED)) {
	return TCL_OK;
    } else {
	MBError(csPtr, TCL_READABLE, code);
	return TCL_ERROR;
    }
}

static int
MBWrite(
    CopyState *csPtr)
{
    ChannelState *inStatePtr = csPtr->readPtr->state;
    ChannelState *outStatePtr = csPtr->writePtr->state;
    ChannelBuffer *bufPtr = inStatePtr->inQueueHead;
    ChannelBuffer *tail = NULL;
    int code;
    Tcl_WideInt inBytes = 0;

    /* Count up number of bytes waiting in the input queue */
    while (bufPtr) {
	inBytes += BytesLeft(bufPtr);
	tail = bufPtr;
	if (csPtr->toRead != -1 && csPtr->toRead < inBytes) {
	    /* Queue has enough bytes to complete the copy */
	    break;
	}
	bufPtr = bufPtr->nextPtr;
    }

    if (bufPtr) {
	/* Split the overflowing buffer in two */
	int extra = (int) (inBytes - csPtr->toRead);
        /* Note that going with int for extra assumes that inBytes is not too
         * much over toRead to require a wide itself. If that gets violated
         * then the calculations involving extra must be made wide too.
         *
         * Noted with Win32/MSVC debug build treating the warning (possible of
         * data in __int64 to int conversion) as error.
         */

	bufPtr = AllocChannelBuffer(extra);

	tail->nextAdded -= extra;
	memcpy(InsertPoint(bufPtr), InsertPoint(tail), extra);
	bufPtr->nextAdded += extra;
	bufPtr->nextPtr = tail->nextPtr;
	tail->nextPtr = NULL;
	inBytes = csPtr->toRead;
    }

    /* Update the byte counts */
    if (csPtr->toRead != -1) {
	csPtr->toRead -= inBytes;
    }
    csPtr->total += inBytes;

    /* Move buffers from input to output channels */
    if (outStatePtr->outQueueTail) {
	outStatePtr->outQueueTail->nextPtr = inStatePtr->inQueueHead;
    } else {
	outStatePtr->outQueueHead = inStatePtr->inQueueHead;
    }
    outStatePtr->outQueueTail = tail;
    inStatePtr->inQueueHead = bufPtr;
    if (inStatePtr->inQueueTail == tail) {
	inStatePtr->inQueueTail = bufPtr;
    }
    if (bufPtr == NULL) {
	inStatePtr->inQueueTail = NULL;
    }

    code = FlushChannel(csPtr->interp, outStatePtr->topChanPtr, 0);
    if (code) {
	MBError(csPtr, TCL_WRITABLE, code);
	return TCL_ERROR;
    }
    if (csPtr->toRead == 0 || GotFlag(inStatePtr, CHANNEL_EOF)) {
	return TCL_OK;
    }
    return TCL_CONTINUE;
}

static int
MoveBytes(
    CopyState *csPtr)		/* State of copy operation. */
{
    ChannelState *outStatePtr = csPtr->writePtr->state;
    ChannelBuffer *bufPtr = outStatePtr->curOutPtr;
    int errorCode;

    if (bufPtr && BytesLeft(bufPtr)) {
	/* If we start with unflushed bytes in the destination
	 * channel, flush them out of the way first. */

	errorCode = FlushChannel(csPtr->interp, outStatePtr->topChanPtr, 0);
	if (errorCode != 0) {
	    MBError(csPtr, TCL_WRITABLE, errorCode);
	    return TCL_ERROR;
	}
    }

    if (csPtr->cmdPtr) {
	Tcl_Channel inChan = (Tcl_Channel) csPtr->readPtr;
	Tcl_CreateChannelHandler(inChan, TCL_READABLE, MBEvent, csPtr);
	return TCL_OK;
    }

    while (1) {
	int code;

	if (TCL_ERROR == MBRead(csPtr)) {
	    return TCL_ERROR;
	}
	code = MBWrite(csPtr);
	if (code == TCL_OK) {
	    Tcl_SetObjResult(csPtr->interp, Tcl_NewWideIntObj(csPtr->total));
	    StopCopy(csPtr);
	    return TCL_OK;
	}
	if (code == TCL_ERROR) {
	    return TCL_ERROR;
	}
	/* code == TCL_CONTINUE --> continue the loop */
    }
    return TCL_OK;	/* Silence compiler warnings */
}

static int
CopyData(
    CopyState *csPtr,		/* State of copy operation. */
    int mask)			/* Current channel event flags. */
{
    Tcl_Interp *interp;
    Tcl_Obj *cmdPtr, *errObj = NULL, *bufObj = NULL, *msg = NULL;
    Tcl_Channel inChan, outChan;
    ChannelState *inStatePtr, *outStatePtr;
    int result = TCL_OK, size, sizeb;
    Tcl_WideInt total;
    const char *buffer;
    int inBinary, outBinary, sameEncoding;
				/* Encoding control */
    int underflow;		/* Input underflow */

    inChan	= (Tcl_Channel) csPtr->readPtr;
    outChan	= (Tcl_Channel) csPtr->writePtr;
    inStatePtr	= csPtr->readPtr->state;
    outStatePtr	= csPtr->writePtr->state;
    interp	= csPtr->interp;
    cmdPtr	= csPtr->cmdPtr;

    /*
     * Copy the data the slow way, using the translation mechanism.
     *
     * Note: We have make sure that we use the topmost channel in a stack for
     * the copying. The caller uses Tcl_GetChannel to access it, and thus gets
     * the bottom of the stack.
     */

    inBinary = (inStatePtr->encoding == NULL);
    outBinary = (outStatePtr->encoding == NULL);
    sameEncoding = (inStatePtr->encoding == outStatePtr->encoding);

    if (!(inBinary || sameEncoding)) {
	TclNewObj(bufObj);
	Tcl_IncrRefCount(bufObj);
    }

    while (csPtr->toRead != (Tcl_WideInt) 0) {
	/*
	 * Check for unreported background errors.
	 */

	Tcl_GetChannelError(inChan, &msg);
	if ((inStatePtr->unreportedError != 0) || (msg != NULL)) {
	    Tcl_SetErrno(inStatePtr->unreportedError);
	    inStatePtr->unreportedError = 0;
	    goto readError;
	}
	Tcl_GetChannelError(outChan, &msg);
	if ((outStatePtr->unreportedError != 0) || (msg != NULL)) {
	    Tcl_SetErrno(outStatePtr->unreportedError);
	    outStatePtr->unreportedError = 0;
	    goto writeError;
	}

	if (cmdPtr && (mask == 0)) {
	    /*
	     * In async mode, we skip reading synchronously and fake an
	     * underflow instead to prime the readable fileevent.
	     */

	    size = 0;
	    underflow = 1;
	} else {
	    /*
	     * Read up to bufSize bytes.
	     */

	    if ((csPtr->toRead == (Tcl_WideInt) -1)
                    || (csPtr->toRead > (Tcl_WideInt) csPtr->bufSize)) {
		sizeb = csPtr->bufSize;
	    } else {
		sizeb = (int) csPtr->toRead;
	    }

	    if (inBinary || sameEncoding) {
		size = DoRead(inStatePtr->topChanPtr, csPtr->buffer, sizeb,
                              !GotFlag(inStatePtr, CHANNEL_NONBLOCKING));
	    } else {
		size = DoReadChars(inStatePtr->topChanPtr, bufObj, sizeb,
			0 /* No append */);
	    }
	    underflow = (size >= 0) && (size < sizeb);	/* Input underflow */
	}

	if (size < 0) {
	readError:
	    if (interp) {
		TclNewObj(errObj);
		Tcl_AppendStringsToObj(errObj, "error reading \"",
			Tcl_GetChannelName(inChan), "\": ", NULL);
		if (msg != NULL) {
		    Tcl_AppendObjToObj(errObj, msg);
		} else {
		    Tcl_AppendStringsToObj(errObj, Tcl_PosixError(interp),
			    NULL);
		}
	    }
	    if (msg != NULL) {
		Tcl_DecrRefCount(msg);
	    }
	    break;
	} else if (underflow) {
	    /*
	     * We had an underflow on the read side. If we are at EOF, and not
	     * in the synchronous part of an asynchronous fcopy, then the
	     * copying is done, otherwise set up a channel handler to detect
	     * when the channel becomes readable again.
	     */

	    if ((size == 0) && Tcl_Eof(inChan) && !(cmdPtr && (mask == 0))) {
		break;
	    }
	    if (cmdPtr && (!Tcl_Eof(inChan) || (mask == 0)) &&
                !(mask & TCL_READABLE)) {
		if (mask & TCL_WRITABLE) {
		    Tcl_DeleteChannelHandler(outChan, CopyEventProc, csPtr);
		}
		Tcl_CreateChannelHandler(inChan, TCL_READABLE, CopyEventProc,
			csPtr);
	    }
	    if (size == 0) {
		if (!GotFlag(inStatePtr, CHANNEL_NONBLOCKING)) {
		    /* We allowed a short read.  Keep trying. */
		    continue;
		}
		if (bufObj != NULL) {
		    TclDecrRefCount(bufObj);
		    bufObj = NULL;
		}
		return TCL_OK;
	    }
	}

	/*
	 * Now write the buffer out.
	 */

	if (inBinary || sameEncoding) {
	    buffer = csPtr->buffer;
	    sizeb = size;
	} else {
	    buffer = TclGetStringFromObj(bufObj, &sizeb);
	}

	if (outBinary || sameEncoding) {
	    sizeb = WriteBytes(outStatePtr->topChanPtr, buffer, sizeb);
	} else {
	    sizeb = WriteChars(outStatePtr->topChanPtr, buffer, sizeb);
	}

	/*
	 * [Bug 2895565]. At this point 'size' still contains the number of
	 * bytes or characters which have been read. We keep this to later to
	 * update the totals and toRead information, see marker (UP) below. We
	 * must not overwrite it with 'sizeb', which is the number of written
	 * bytes or characters, and both EOL translation and encoding
	 * conversion may have changed this number unpredictably in relation
	 * to 'size' (It can be smaller or larger, in the latter case able to
	 * drive toRead below -1, causing infinite looping). Completely
	 * unsuitable for updating totals and toRead.
	 */

	if (sizeb < 0) {
	writeError:
	    if (interp) {
		TclNewObj(errObj);
		Tcl_AppendStringsToObj(errObj, "error writing \"",
			Tcl_GetChannelName(outChan), "\": ", NULL);
		if (msg != NULL) {
		    Tcl_AppendObjToObj(errObj, msg);
		} else {
		    Tcl_AppendStringsToObj(errObj, Tcl_PosixError(interp),
			    NULL);
		}
	    }
	    if (msg != NULL) {
		Tcl_DecrRefCount(msg);
	    }
	    break;
	}

	/*
	 * Update the current byte count. Do it now so the count is valid
	 * before a return or break takes us out of the loop. The invariant at
	 * the top of the loop should be that csPtr->toRead holds the number
	 * of bytes left to copy.
	 */

	if (csPtr->toRead != -1) {
	    csPtr->toRead -= size;
	}
	csPtr->total += size;

	/*
	 * Break loop if EOF && (size>0)
	 */

	if (Tcl_Eof(inChan)) {
	    break;
	}

	/*
	 * Check to see if the write is happening in the background. If so,
	 * stop copying and wait for the channel to become writable again.
	 * After input underflow we already installed a readable handler
	 * therefore we don't need a writable handler.
	 */

	if (!underflow && GotFlag(outStatePtr, BG_FLUSH_SCHEDULED)) {
	    if (!(mask & TCL_WRITABLE)) {
		if (mask & TCL_READABLE) {
		    Tcl_DeleteChannelHandler(inChan, CopyEventProc, csPtr);
		}
		Tcl_CreateChannelHandler(outChan, TCL_WRITABLE,
			CopyEventProc, csPtr);
	    }
	    if (bufObj != NULL) {
		TclDecrRefCount(bufObj);
		bufObj = NULL;
	    }
	    return TCL_OK;
	}

	/*
	 * For background copies, we only do one buffer per invocation so we
	 * don't starve the rest of the system.
	 */

	if (cmdPtr && (csPtr->toRead != 0)) {
	    /*
	     * The first time we enter this code, there won't be a channel
	     * handler established yet, so do it here.
	     */

	    if (mask == 0) {
		Tcl_CreateChannelHandler(outChan, TCL_WRITABLE, CopyEventProc,
			csPtr);
	    }
	    if (bufObj != NULL) {
		TclDecrRefCount(bufObj);
		bufObj = NULL;
	    }
	    return TCL_OK;
	}
    } /* while */

    if (bufObj != NULL) {
	TclDecrRefCount(bufObj);
	bufObj = NULL;
    }

    /*
     * Make the callback or return the number of bytes transferred. The local
     * total is used because StopCopy frees csPtr.
     */

    total = csPtr->total;
    if (cmdPtr && interp) {
	int code;

	/*
	 * Get a private copy of the command so we can mutate it by adding
	 * arguments. Note that StopCopy frees our saved reference to the
	 * original command obj.
	 */

	cmdPtr = Tcl_DuplicateObj(cmdPtr);
	Tcl_IncrRefCount(cmdPtr);
	StopCopy(csPtr);
	Tcl_Preserve(interp);

	Tcl_ListObjAppendElement(interp, cmdPtr, Tcl_NewWideIntObj(total));
	if (errObj) {
	    Tcl_ListObjAppendElement(interp, cmdPtr, errObj);
	}
	code = Tcl_EvalObjEx(interp, cmdPtr, TCL_EVAL_GLOBAL);
	if (code != TCL_OK) {
	    Tcl_BackgroundException(interp, code);
	    result = TCL_ERROR;
	}
	TclDecrRefCount(cmdPtr);
	Tcl_Release(interp);
    } else {
	StopCopy(csPtr);
	if (interp) {
	    if (errObj) {
		Tcl_SetObjResult(interp, errObj);
		result = TCL_ERROR;
	    } else {
		Tcl_ResetResult(interp);
		Tcl_SetObjResult(interp, Tcl_NewWideIntObj(total));
	    }
	}
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * DoRead --
 *
 *	Stores up to "bytesToRead" bytes in memory pointed to by "dst".
 *	These bytes come from reading the channel "chanPtr" and
 *	performing the configured translations.  No encoding conversions
 *	are applied to the bytes being read.
 *
 * Results:
 *	The number of bytes actually stored (<= bytesToRead),
 * 	or -1 if there is an error in reading the channel.  Use
 * 	Tcl_GetErrno() to retrieve the error code for the error
 *	that occurred.
 *
 *	The number of bytes stored can be less than the number
 * 	requested when
 *	  - EOF is reached on the channel; or
 *	  - the channel is non-blocking, and we've read all we can
 *	    without blocking.
 *	  - a channel reading error occurs (and we return -1)
 *
 * Side effects:
 *	May cause input to be buffered.
 *
 *----------------------------------------------------------------------
 */

static int
DoRead(
    Channel *chanPtr,		/* The channel from which to read. */
    char *dst,			/* Where to store input read. */
    int bytesToRead,		/* Maximum number of bytes to read. */
    int allowShortReads)	/* Allow half-blocking (pipes,sockets) */
{
    ChannelState *statePtr = chanPtr->state;
    char *p = dst;

    assert (bytesToRead >= 0);

    /*
     * Early out when we know a read will get the eofchar.
     *
     * NOTE: This seems to be a bug.  The special handling for
     * a zero-char read request ought to come first.  As coded
     * the EOF due to eofchar has distinguishing behavior from
     * the EOF due to reported EOF on the underlying device, and
     * that seems undesirable.  However recent history indicates
     * that new inconsistent behavior in a patchlevel has problems
     * too.  Keep on keeping on for now.
     */

    if (GotFlag(statePtr, CHANNEL_STICKY_EOF)) {
	SetFlag(statePtr, CHANNEL_EOF);
	assert( statePtr->inputEncodingFlags & TCL_ENCODING_END );
	assert( !GotFlag(statePtr, CHANNEL_BLOCKED|INPUT_SAW_CR) );

	/* TODO: Don't need this call */
	UpdateInterest(chanPtr);
	return 0;
    }

    /* Special handling for zero-char read request. */
    if (bytesToRead == 0) {
	if (GotFlag(statePtr, CHANNEL_EOF)) {
	    statePtr->inputEncodingFlags |= TCL_ENCODING_START;
	}
	ResetFlag(statePtr, CHANNEL_BLOCKED|CHANNEL_EOF);
	statePtr->inputEncodingFlags &= ~TCL_ENCODING_END;
	/* TODO: Don't need this call */
	UpdateInterest(chanPtr);
	return 0;
    }

    TclChannelPreserve((Tcl_Channel)chanPtr);
    while (bytesToRead) {
	/*
	 * Each pass through the loop is intended to process up to
	 * one channel buffer.
	 */

	int bytesRead, bytesWritten;
	ChannelBuffer *bufPtr = statePtr->inQueueHead;

	/*
	 * Don't read more data if we have what we need.
	 */

	while (!bufPtr ||			/* We got no buffer!   OR */
		(!IsBufferFull(bufPtr) && 	/* Our buffer has room AND */
		(BytesLeft(bufPtr) < bytesToRead) ) ) {
						/* Not enough bytes in it
						 * yet to fill the dst */
	    int code;

	moreData:
	    code = GetInput(chanPtr);
	    bufPtr = statePtr->inQueueHead;

	    assert (bufPtr != NULL);

	    if (GotFlag(statePtr, CHANNEL_EOF|CHANNEL_BLOCKED)) {
		/* Further reads cannot do any more */
		break;
	    }

	    if (code) {
		/* Read error */
		UpdateInterest(chanPtr);
		TclChannelRelease((Tcl_Channel)chanPtr);
		return -1;
	    }

	    assert (IsBufferFull(bufPtr));
	}

	assert (bufPtr != NULL);

	bytesRead = BytesLeft(bufPtr);
	bytesWritten = bytesToRead;

	TranslateInputEOL(statePtr, p, RemovePoint(bufPtr),
		&bytesWritten, &bytesRead);
	bufPtr->nextRemoved += bytesRead;
	p += bytesWritten;
	bytesToRead -= bytesWritten;

	if (!IsBufferEmpty(bufPtr)) {
	    /*
	     * Buffer is not empty.  How can that be?
	     *
	     * 0) We stopped early because we got all the bytes
	     *    we were seeking.  That's fine.
	     */

	    if (bytesToRead == 0) {
		break;
	    }

	    /*
	     * 1) We're @EOF because we saw eof char.
	     */

	    if (GotFlag(statePtr, CHANNEL_STICKY_EOF)) {
		break;
	    }

	    /*
	     * 2) The buffer holds a \r while in CRLF translation,
	     *    followed by the end of the buffer.
	     */

	    assert(statePtr->inputTranslation == TCL_TRANSLATE_CRLF);
	    assert(RemovePoint(bufPtr)[0] == '\r');
	    assert(BytesLeft(bufPtr) == 1);

	    if (bufPtr->nextPtr == NULL) {
		/* There's no more buffered data.... */

		if (statePtr->flags & CHANNEL_EOF) {
		    /* ...and there never will be. */

		    *p++ = '\r';
		    bytesToRead--;
		    bufPtr->nextRemoved++;
		} else if (statePtr->flags & CHANNEL_BLOCKED) {
		    /* ...and we cannot get more now. */
		    SetFlag(statePtr, CHANNEL_NEED_MORE_DATA);
		    break;
		} else {
		    /* ... so we need to get some. */
		    goto moreData;
		}
	    }

	    if (bufPtr->nextPtr) {
		/* There's a next buffer.  Shift orphan \r to it. */

		ChannelBuffer *nextPtr = bufPtr->nextPtr;

		nextPtr->nextRemoved -= 1;
		RemovePoint(nextPtr)[0] = '\r';
		bufPtr->nextRemoved++;
	    }
	}

	if (IsBufferEmpty(bufPtr)) {
	    statePtr->inQueueHead = bufPtr->nextPtr;
	    if (statePtr->inQueueHead == NULL) {
		statePtr->inQueueTail = NULL;
	    }
	    RecycleBuffer(statePtr, bufPtr, 0);
	    bufPtr = statePtr->inQueueHead;
	}

	if ((GotFlag(statePtr, CHANNEL_NONBLOCKING) || allowShortReads)
		&& GotFlag(statePtr, CHANNEL_BLOCKED)) {
	    break;
	}

	/*
	 * When there's no buffered data to read, and we're at EOF,
	 * escape to the caller.
	 */

	if (GotFlag(statePtr, CHANNEL_EOF)
		&& (bufPtr == NULL || IsBufferEmpty(bufPtr))) {
	    break;
	}
    }
    if (bytesToRead == 0) {
	ResetFlag(statePtr, CHANNEL_BLOCKED);
    }

	assert(!GotFlag(statePtr, CHANNEL_EOF)
		|| GotFlag(statePtr, CHANNEL_STICKY_EOF)
		|| Tcl_InputBuffered((Tcl_Channel)chanPtr) == 0);
	assert( !(GotFlag(statePtr, CHANNEL_EOF|CHANNEL_BLOCKED)
		== (CHANNEL_EOF|CHANNEL_BLOCKED)) );
    UpdateInterest(chanPtr);
    TclChannelRelease((Tcl_Channel)chanPtr);
    return (int)(p - dst);
}

/*
 *----------------------------------------------------------------------
 *
 * CopyEventProc --
 *
 *	This routine is invoked as a channel event handler for the background
 *	copy operation. It is just a trivial wrapper around the CopyData
 *	routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
CopyEventProc(
    ClientData clientData,
    int mask)
{
    (void) CopyData(clientData, mask);
}

/*
 *----------------------------------------------------------------------
 *
 * StopCopy --
 *
 *	This routine halts a copy that is in progress.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes any pending channel handlers and restores the blocking and
 *	buffering modes of the channels. The CopyState is freed.
 *
 *----------------------------------------------------------------------
 */

static void
StopCopy(
    CopyState *csPtr)		/* State for bg copy to stop . */
{
    ChannelState *inStatePtr, *outStatePtr;
    Tcl_Channel inChan, outChan;

    int nonBlocking;

    if (!csPtr) {
	return;
    }

    inChan = (Tcl_Channel) csPtr->readPtr;
    outChan = (Tcl_Channel) csPtr->writePtr;
    inStatePtr = csPtr->readPtr->state;
    outStatePtr = csPtr->writePtr->state;

    /*
     * Restore the old blocking mode and output buffering mode.
     */

    nonBlocking = csPtr->readFlags & CHANNEL_NONBLOCKING;
    if (nonBlocking != (inStatePtr->flags & CHANNEL_NONBLOCKING)) {
	SetBlockMode(NULL, csPtr->readPtr,
		nonBlocking ? TCL_MODE_NONBLOCKING : TCL_MODE_BLOCKING);
    }
    if (csPtr->readPtr != csPtr->writePtr) {
	nonBlocking = csPtr->writeFlags & CHANNEL_NONBLOCKING;
	if (nonBlocking != (outStatePtr->flags & CHANNEL_NONBLOCKING)) {
	    SetBlockMode(NULL, csPtr->writePtr,
		    nonBlocking ? TCL_MODE_NONBLOCKING : TCL_MODE_BLOCKING);
	}
    }
    ResetFlag(outStatePtr, CHANNEL_LINEBUFFERED | CHANNEL_UNBUFFERED);
    outStatePtr->flags |=
	    csPtr->writeFlags & (CHANNEL_LINEBUFFERED | CHANNEL_UNBUFFERED);

    if (csPtr->cmdPtr) {
	Tcl_DeleteChannelHandler(inChan, CopyEventProc, csPtr);
	if (inChan != outChan) {
	    Tcl_DeleteChannelHandler(outChan, CopyEventProc, csPtr);
	}
	Tcl_DeleteChannelHandler(inChan, MBEvent, csPtr);
	Tcl_DeleteChannelHandler(outChan, MBEvent, csPtr);
	TclDecrRefCount(csPtr->cmdPtr);
    }
    inStatePtr->csPtrR = NULL;
    outStatePtr->csPtrW = NULL;
    ckfree(csPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * StackSetBlockMode --
 *
 *	This function sets the blocking mode for a channel, iterating through
 *	each channel in a stack and updates the state flags.
 *
 * Results:
 *	0 if OK, result code from failed blockModeProc otherwise.
 *
 * Side effects:
 *	Modifies the blocking mode of the channel and possibly generates an
 *	error.
 *
 *----------------------------------------------------------------------
 */

static int
StackSetBlockMode(
    Channel *chanPtr,		/* Channel to modify. */
    int mode)			/* One of TCL_MODE_BLOCKING or
				 * TCL_MODE_NONBLOCKING. */
{
    int result = 0;
    Tcl_DriverBlockModeProc *blockModeProc;
    ChannelState *statePtr = chanPtr->state;

    /*
     * Start at the top of the channel stack
     * TODO: Examine what can go wrong when blockModeProc calls
     * disturb the stacking state of the channel.
     */

    chanPtr = statePtr->topChanPtr;
    while (chanPtr != NULL) {
	blockModeProc = Tcl_ChannelBlockModeProc(chanPtr->typePtr);
	if (blockModeProc != NULL) {
	    result = blockModeProc(chanPtr->instanceData, mode);
	    if (result != 0) {
		Tcl_SetErrno(result);
		return result;
	    }
	}
	chanPtr = chanPtr->downChanPtr;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * SetBlockMode --
 *
 *	This function sets the blocking mode for a channel and updates the
 *	state flags.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Modifies the blocking mode of the channel and possibly generates an
 *	error.
 *
 *----------------------------------------------------------------------
 */

static int
SetBlockMode(
    Tcl_Interp *interp,		/* Interp for error reporting. */
    Channel *chanPtr,		/* Channel to modify. */
    int mode)			/* One of TCL_MODE_BLOCKING or
				 * TCL_MODE_NONBLOCKING. */
{
    int result = 0;
    ChannelState *statePtr = chanPtr->state;
				/* State info for channel */

    result = StackSetBlockMode(chanPtr, mode);
    if (result != 0) {
	if (interp != NULL) {
	    /*
	     * TIP #219.
	     * Move error messages put by the driver into the bypass area and
	     * put them into the regular interpreter result. Fall back to the
	     * regular message if nothing was found in the bypass.
	     *
	     * Note that we cannot have a message in the interpreter bypass
	     * area, StackSetBlockMode is restricted to the channel bypass.
	     * We still need the interp as the destination of the move.
	     */

	    if (!TclChanCaughtErrorBypass(interp, (Tcl_Channel) chanPtr)) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                        "error setting blocking mode: %s",
			Tcl_PosixError(interp)));
	    }
	} else {
	    /*
	     * TIP #219.
	     * If we have no interpreter to put a bypass message into we have
	     * to clear it, to prevent its propagation and use in other places
	     * unrelated to the actual occurence of the problem.
	     */

	    Tcl_SetChannelError((Tcl_Channel) chanPtr, NULL);
	}
	return TCL_ERROR;
    }
    if (mode == TCL_MODE_BLOCKING) {
	ResetFlag(statePtr, CHANNEL_NONBLOCKING | BG_FLUSH_SCHEDULED);
    } else {
	SetFlag(statePtr, CHANNEL_NONBLOCKING);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelNames --
 *
 *	Return the names of all open channels in the interp.
 *
 * Results:
 *	TCL_OK or TCL_ERROR.
 *
 * Side effects:
 *	Interp result modified with list of channel names.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetChannelNames(
    Tcl_Interp *interp)		/* Interp for error reporting. */
{
    return Tcl_GetChannelNamesEx(interp, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelNamesEx --
 *
 *	Return the names of open channels in the interp filtered filtered
 *	through a pattern. If pattern is NULL, it returns all the open
 *	channels.
 *
 * Results:
 *	TCL_OK or TCL_ERROR.
 *
 * Side effects:
 *	Interp result modified with list of channel names.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetChannelNamesEx(
    Tcl_Interp *interp,		/* Interp for error reporting. */
    const char *pattern)	/* Pattern to filter on. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    ChannelState *statePtr;
    const char *name;		/* Name for channel */
    Tcl_Obj *resultPtr;		/* Pointer to result object */
    Tcl_HashTable *hTblPtr;	/* Hash table of channels. */
    Tcl_HashEntry *hPtr;	/* Search variable. */
    Tcl_HashSearch hSearch;	/* Search variable. */

    if (interp == NULL) {
	return TCL_OK;
    }

    /*
     * Get the channel table that stores the channels registered for this
     * interpreter.
     */

    hTblPtr = GetChannelTable(interp);
    TclNewObj(resultPtr);
    if ((pattern != NULL) && TclMatchIsTrivial(pattern)
	    && !((pattern[0] == 's') && (pattern[1] == 't')
	    && (pattern[2] == 'd'))) {
	if ((Tcl_FindHashEntry(hTblPtr, pattern) != NULL)
		&& (Tcl_ListObjAppendElement(interp, resultPtr,
		Tcl_NewStringObj(pattern, -1)) != TCL_OK)) {
	    goto error;
	}
	goto done;
    }

    for (hPtr = Tcl_FirstHashEntry(hTblPtr, &hSearch); hPtr != NULL;
	    hPtr = Tcl_NextHashEntry(&hSearch)) {
	statePtr = ((Channel *) Tcl_GetHashValue(hPtr))->state;

	if (statePtr->topChanPtr == (Channel *) tsdPtr->stdinChannel) {
	    name = "stdin";
	} else if (statePtr->topChanPtr == (Channel *) tsdPtr->stdoutChannel) {
	    name = "stdout";
	} else if (statePtr->topChanPtr == (Channel *) tsdPtr->stderrChannel) {
	    name = "stderr";
	} else {
	    /*
	     * This is also stored in Tcl_GetHashKey(hTblPtr, hPtr), but it's
	     * simpler to just grab the name from the statePtr.
	     */

	    name = statePtr->channelName;
	}

	if (((pattern == NULL) || Tcl_StringMatch(name, pattern)) &&
		(Tcl_ListObjAppendElement(interp, resultPtr,
			Tcl_NewStringObj(name, -1)) != TCL_OK)) {
	error:
	    TclDecrRefCount(resultPtr);
	    return TCL_ERROR;
	}
    }

  done:
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_IsChannelRegistered --
 *
 *	Checks whether the channel is associated with the interp. See also
 *	Tcl_RegisterChannel and Tcl_UnregisterChannel.
 *
 * Results:
 *	0 if the channel is not registered in the interpreter, 1 else.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_IsChannelRegistered(
    Tcl_Interp *interp,		/* The interp to query of the channel */
    Tcl_Channel chan)		/* The channel to check */
{
    Tcl_HashTable *hTblPtr;	/* Hash table of channels. */
    Tcl_HashEntry *hPtr;	/* Search variable. */
    Channel *chanPtr;		/* The real IO channel. */
    ChannelState *statePtr;	/* State of the real channel. */

    /*
     * Always check bottom-most channel in the stack. This is the one that
     * gets registered.
     */

    chanPtr = ((Channel *) chan)->state->bottomChanPtr;
    statePtr = chanPtr->state;

    hTblPtr = Tcl_GetAssocData(interp, "tclIO", NULL);
    if (hTblPtr == NULL) {
	return 0;
    }
    hPtr = Tcl_FindHashEntry(hTblPtr, statePtr->channelName);
    if (hPtr == NULL) {
	return 0;
    }
    if ((Channel *) Tcl_GetHashValue(hPtr) != chanPtr) {
	return 0;
    }

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_IsChannelShared --
 *
 *	Checks whether the channel is shared by multiple interpreters.
 *
 * Results:
 *	A boolean value (0 = Not shared, 1 = Shared).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_IsChannelShared(
    Tcl_Channel chan)		/* The channel to query */
{
    ChannelState *statePtr = ((Channel *) chan)->state;
				/* State of real channel structure. */

    return ((statePtr->refCount > 1) ? 1 : 0);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_IsChannelExisting --
 *
 *	Checks whether a channel of the given name exists in the
 *	(thread)-global list of all channels. See Tcl_GetChannelNamesEx for
 *	function exposed at the Tcl level.
 *
 * Results:
 *	A boolean value (0 = Does not exist, 1 = Does exist).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_IsChannelExisting(
    const char *chanName)	/* The name of the channel to look for. */
{
    ChannelState *statePtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    const char *name;
    int chanNameLen;

    chanNameLen = strlen(chanName);
    for (statePtr = tsdPtr->firstCSPtr; statePtr != NULL;
	    statePtr = statePtr->nextCSPtr) {
	if (statePtr->topChanPtr == (Channel *) tsdPtr->stdinChannel) {
	    name = "stdin";
	} else if (statePtr->topChanPtr == (Channel *) tsdPtr->stdoutChannel) {
	    name = "stdout";
	} else if (statePtr->topChanPtr == (Channel *) tsdPtr->stderrChannel) {
	    name = "stderr";
	} else {
	    name = statePtr->channelName;
	}

	if ((*chanName == *name) &&
		(memcmp(name, chanName, (size_t) chanNameLen + 1) == 0)) {
	    return 1;
	}
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelName --
 *
 *	Return the name of the channel type.
 *
 * Results:
 *	A pointer the name of the channel type.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
Tcl_ChannelName(
    const Tcl_ChannelType *chanTypePtr) /* Pointer to channel type. */
{
    return chanTypePtr->typeName;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelVersion --
 *
 *	Return the of version of the channel type.
 *
 * Results:
 *	One of the TCL_CHANNEL_VERSION_* constants from tcl.h
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_ChannelTypeVersion
Tcl_ChannelVersion(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    if (chanTypePtr->version == TCL_CHANNEL_VERSION_2) {
	return TCL_CHANNEL_VERSION_2;
    } else if (chanTypePtr->version == TCL_CHANNEL_VERSION_3) {
	return TCL_CHANNEL_VERSION_3;
    } else if (chanTypePtr->version == TCL_CHANNEL_VERSION_4) {
	return TCL_CHANNEL_VERSION_4;
    } else if (chanTypePtr->version == TCL_CHANNEL_VERSION_5) {
	return TCL_CHANNEL_VERSION_5;
    } else {
	/*
	 * In <v2 channel versions, the version field is occupied by the
	 * Tcl_DriverBlockModeProc
	 */

	return TCL_CHANNEL_VERSION_1;
    }
}

/*
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

    return (PTR2INT(actualVersion)) >= (PTR2INT(minimumVersion));
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelBlockModeProc --
 *
 *	Return the Tcl_DriverBlockModeProc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------- */

Tcl_DriverBlockModeProc *
Tcl_ChannelBlockModeProc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    if (HaveVersion(chanTypePtr, TCL_CHANNEL_VERSION_2)) {
	return chanTypePtr->blockModeProc;
    }

    /*
     * The v1 structure had the blockModeProc in a different place.
     */

    return (Tcl_DriverBlockModeProc *) chanTypePtr->version;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelCloseProc --
 *
 *	Return the Tcl_DriverCloseProc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_DriverCloseProc *
Tcl_ChannelCloseProc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    return chanTypePtr->closeProc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelClose2Proc --
 *
 *	Return the Tcl_DriverClose2Proc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_DriverClose2Proc *
Tcl_ChannelClose2Proc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    return chanTypePtr->close2Proc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelInputProc --
 *
 *	Return the Tcl_DriverInputProc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_DriverInputProc *
Tcl_ChannelInputProc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    return chanTypePtr->inputProc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelOutputProc --
 *
 *	Return the Tcl_DriverOutputProc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_DriverOutputProc *
Tcl_ChannelOutputProc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    return chanTypePtr->outputProc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelSeekProc --
 *
 *	Return the Tcl_DriverSeekProc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_DriverSeekProc *
Tcl_ChannelSeekProc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    return chanTypePtr->seekProc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelSetOptionProc --
 *
 *	Return the Tcl_DriverSetOptionProc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_DriverSetOptionProc *
Tcl_ChannelSetOptionProc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    return chanTypePtr->setOptionProc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelGetOptionProc --
 *
 *	Return the Tcl_DriverGetOptionProc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_DriverGetOptionProc *
Tcl_ChannelGetOptionProc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    return chanTypePtr->getOptionProc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelWatchProc --
 *
 *	Return the Tcl_DriverWatchProc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_DriverWatchProc *
Tcl_ChannelWatchProc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    return chanTypePtr->watchProc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelGetHandleProc --
 *
 *	Return the Tcl_DriverGetHandleProc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_DriverGetHandleProc *
Tcl_ChannelGetHandleProc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    return chanTypePtr->getHandleProc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelFlushProc --
 *
 *	Return the Tcl_DriverFlushProc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_DriverFlushProc *
Tcl_ChannelFlushProc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    if (HaveVersion(chanTypePtr, TCL_CHANNEL_VERSION_2)) {
	return chanTypePtr->flushProc;
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelHandlerProc --
 *
 *	Return the Tcl_DriverHandlerProc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_DriverHandlerProc *
Tcl_ChannelHandlerProc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    if (HaveVersion(chanTypePtr, TCL_CHANNEL_VERSION_2)) {
	return chanTypePtr->handlerProc;
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelWideSeekProc --
 *
 *	Return the Tcl_DriverWideSeekProc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_DriverWideSeekProc *
Tcl_ChannelWideSeekProc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    if (HaveVersion(chanTypePtr, TCL_CHANNEL_VERSION_3)) {
	return chanTypePtr->wideSeekProc;
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelThreadActionProc --
 *
 *	TIP #218, Channel Thread Actions. Return the
 *	Tcl_DriverThreadActionProc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_DriverThreadActionProc *
Tcl_ChannelThreadActionProc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    if (HaveVersion(chanTypePtr, TCL_CHANNEL_VERSION_4)) {
	return chanTypePtr->threadActionProc;
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetChannelErrorInterp --
 *
 *	TIP #219, Tcl Channel Reflection API.
 *	Store an error message for the I/O system.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Discards a previously stored message.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetChannelErrorInterp(
    Tcl_Interp *interp,		/* Interp to store the data into. */
    Tcl_Obj *msg)		/* Error message to store. */
{
    Interp *iPtr = (Interp *) interp;

    if (iPtr->chanMsg != NULL) {
	TclDecrRefCount(iPtr->chanMsg);
	iPtr->chanMsg = NULL;
    }

    if (msg != NULL) {
	iPtr->chanMsg = FixLevelCode(msg);
	Tcl_IncrRefCount(iPtr->chanMsg);
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetChannelError --
 *
 *	TIP #219, Tcl Channel Reflection API.
 *	Store an error message for the I/O system.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Discards a previously stored message.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetChannelError(
    Tcl_Channel chan,		/* Channel to store the data into. */
    Tcl_Obj *msg)		/* Error message to store. */
{
    ChannelState *statePtr = ((Channel *) chan)->state;

    if (statePtr->chanMsg != NULL) {
	TclDecrRefCount(statePtr->chanMsg);
	statePtr->chanMsg = NULL;
    }

    if (msg != NULL) {
	statePtr->chanMsg = FixLevelCode(msg);
	Tcl_IncrRefCount(statePtr->chanMsg);
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * FixLevelCode --
 *
 *	TIP #219, Tcl Channel Reflection API.
 *	Scans an error message for bad -code / -level directives. Returns a
 *	modified copy with such directives corrected, and the input if it had
 *	no problems.
 *
 * Results:
 *	A Tcl_Obj*
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
FixLevelCode(
    Tcl_Obj *msg)
{
    int explicitResult, numOptions, lc, lcn;
    Tcl_Obj **lv, **lvn;
    int res, i, j, val, lignore, cignore;
    int newlevel = -1, newcode = -1;

    /* ASSERT msg != NULL */

    /*
     * Process the caught message.
     *
     * Syntax = (option value)... ?message?
     *
     * Bad message syntax causes a panic, because the other side uses
     * Tcl_GetReturnOptions and list construction functions to marshall the
     * information. Hence an error means that we've got serious breakage.
     */

    res = Tcl_ListObjGetElements(NULL, msg, &lc, &lv);
    if (res != TCL_OK) {
	Tcl_Panic("Tcl_SetChannelError: bad syntax of message");
    }

    explicitResult = (1 == (lc % 2));
    numOptions = lc - explicitResult;

    /*
     * No options, nothing to do.
     */

    if (numOptions == 0) {
	return msg;
    }

    /*
     * Check for -code x, x != 1|error, and -level x, x != 0
     */

    for (i = 0; i < numOptions; i += 2) {
	if (0 == strcmp(TclGetString(lv[i]), "-code")) {
	    /*
	     * !"error", !integer, integer != 1 (numeric code for error)
	     */

	    res = TclGetIntFromObj(NULL, lv[i+1], &val);
	    if (((res == TCL_OK) && (val != 1)) || ((res != TCL_OK) &&
		    (0 != strcmp(TclGetString(lv[i+1]), "error")))) {
		newcode = 1;
	    }
	} else if (0 == strcmp(TclGetString(lv[i]), "-level")) {
	    /*
	     * !integer, integer != 0
	     */

	    res = TclGetIntFromObj(NULL, lv [i+1], &val);
	    if ((res != TCL_OK) || (val != 0)) {
		newlevel = 0;
	    }
	}
    }

    /*
     * -code, -level are either not present or ok. Nothing to do.
     */

    if ((newlevel < 0) && (newcode < 0)) {
	return msg;
    }

    lcn = numOptions;
    if (explicitResult) {
	lcn ++;
    }
    if (newlevel >= 0) {
	lcn += 2;
    }
    if (newcode >= 0) {
	lcn += 2;
    }

    lvn = ckalloc(lcn * sizeof(Tcl_Obj *));

    /*
     * New level/code information is spliced into the first occurence of
     * -level, -code, further occurences are ignored. The options cannot be
     * not present, we would not come here. Options which are ok are simply
     * copied over.
     */

    lignore = cignore = 0;
    for (i=0, j=0; i<numOptions; i+=2) {
	if (0 == strcmp(TclGetString(lv[i]), "-level")) {
	    if (newlevel >= 0) {
		lvn[j++] = lv[i];
		lvn[j++] = Tcl_NewIntObj(newlevel);
		newlevel = -1;
		lignore = 1;
		continue;
	    } else if (lignore) {
		continue;
	    }
	} else if (0 == strcmp(TclGetString(lv[i]), "-code")) {
	    if (newcode >= 0) {
		lvn[j++] = lv[i];
		lvn[j++] = Tcl_NewIntObj(newcode);
		newcode = -1;
		cignore = 1;
		continue;
	    } else if (cignore) {
		continue;
	    }
	}

	/*
	 * Keep everything else, possibly copied down.
	 */

	lvn[j++] = lv[i];
	lvn[j++] = lv[i+1];
    }
    if (newlevel >= 0) {
	Tcl_Panic("Defined newlevel not used in rewrite");
    }
    if (newcode >= 0) {
	Tcl_Panic("Defined newcode not used in rewrite");
    }

    if (explicitResult) {
	lvn[j++] = lv[i];
    }

    msg = Tcl_NewListObj(j, lvn);

    ckfree(lvn);
    return msg;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelErrorInterp --
 *
 *	TIP #219, Tcl Channel Reflection API.
 *	Return the message stored by the channel driver.
 *
 * Results:
 *	Tcl error message object.
 *
 * Side effects:
 *	Resets the stored data to NULL.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_GetChannelErrorInterp(
    Tcl_Interp *interp,		/* Interp to query. */
    Tcl_Obj **msg)		/* Place for error message. */
{
    Interp *iPtr = (Interp *) interp;

    *msg = iPtr->chanMsg;
    iPtr->chanMsg = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelError --
 *
 *	TIP #219, Tcl Channel Reflection API.
 *	Return the message stored by the channel driver.
 *
 * Results:
 *	Tcl error message object.
 *
 * Side effects:
 *	Resets the stored data to NULL.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_GetChannelError(
    Tcl_Channel chan,		/* Channel to query. */
    Tcl_Obj **msg)		/* Place for error message. */
{
    ChannelState *statePtr = ((Channel *) chan)->state;

    *msg = statePtr->chanMsg;
    statePtr->chanMsg = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChannelTruncateProc --
 *
 *	TIP #208 (subsection relating to truncation, based on TIP #206).
 *	Return the Tcl_DriverTruncateProc of the channel type.
 *
 * Results:
 *	A pointer to the proc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_DriverTruncateProc *
Tcl_ChannelTruncateProc(
    const Tcl_ChannelType *chanTypePtr)
				/* Pointer to channel type. */
{
    if (HaveVersion(chanTypePtr, TCL_CHANNEL_VERSION_5)) {
	return chanTypePtr->truncateProc;
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DupChannelIntRep --
 *
 *	Initialize the internal representation of a new Tcl_Obj to a copy of
 *	the internal representation of an existing string object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	copyPtr's internal rep is set to a copy of srcPtr's internal
 *	representation.
 *
 *----------------------------------------------------------------------
 */

static void
DupChannelIntRep(
    register Tcl_Obj *srcPtr,	/* Object with internal rep to copy. Must have
				 * an internal rep of type "Channel". */
    register Tcl_Obj *copyPtr)	/* Object with internal rep to set. Must not
				 * currently have an internal rep.*/
{
    ResolvedChanName *resPtr = srcPtr->internalRep.twoPtrValue.ptr1;

    resPtr->refCount++;
    copyPtr->internalRep.twoPtrValue.ptr1 = resPtr;
    copyPtr->typePtr = srcPtr->typePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeChannelIntRep --
 *
 *	Release statePtr storage.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May cause state to be freed.
 *
 *----------------------------------------------------------------------
 */

static void
FreeChannelIntRep(
    Tcl_Obj *objPtr)		/* Object with internal rep to free. */
{
    ResolvedChanName *resPtr = objPtr->internalRep.twoPtrValue.ptr1;

    objPtr->typePtr = NULL;
    if (--resPtr->refCount) {
	return;
    }
    Tcl_Release(resPtr->statePtr);
    ckfree(resPtr);
}

#if 0
/*
 * For future debugging work, a simple function to print the flags of a
 * channel in semi-readable form.
 */

static int
DumpFlags(
    char *str,
    int flags)
{
    char buf[20];
    int i = 0;

#define ChanFlag(chr, bit)      (buf[i++] = ((flags & (bit)) ? (chr) : '_'))

    ChanFlag('r', TCL_READABLE);
    ChanFlag('w', TCL_WRITABLE);
    ChanFlag('n', CHANNEL_NONBLOCKING);
    ChanFlag('l', CHANNEL_LINEBUFFERED);
    ChanFlag('u', CHANNEL_UNBUFFERED);
    ChanFlag('F', BG_FLUSH_SCHEDULED);
    ChanFlag('c', CHANNEL_CLOSED);
    ChanFlag('E', CHANNEL_EOF);
    ChanFlag('S', CHANNEL_STICKY_EOF);
    ChanFlag('B', CHANNEL_BLOCKED);
    ChanFlag('/', INPUT_SAW_CR);
    ChanFlag('D', CHANNEL_DEAD);
    ChanFlag('R', CHANNEL_RAW_MODE);
    ChanFlag('x', CHANNEL_INCLOSE);

    buf[i] ='\0';

    fprintf(stderr, "%s: %s\n", str, buf);
    return 0;
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
