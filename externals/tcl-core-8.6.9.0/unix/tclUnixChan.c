/*
 * tclUnixChan.c
 *
 *	Common channel driver for Unix channels based on files, command pipes
 *	and TCP sockets.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"	/* Internal definitions for Tcl. */
#include "tclIO.h"	/* To get Channel type declaration. */

#undef SUPPORTS_TTY
#if defined(HAVE_TERMIOS_H)
#   define SUPPORTS_TTY 1
#   include <termios.h>
#   ifdef HAVE_SYS_IOCTL_H
#	include <sys/ioctl.h>
#   endif /* HAVE_SYS_IOCTL_H */
#   ifdef HAVE_SYS_MODEM_H
#	include <sys/modem.h>
#   endif /* HAVE_SYS_MODEM_H */

#   ifdef FIONREAD
#	define GETREADQUEUE(fd, int)	ioctl((fd), FIONREAD, &(int))
#   elif defined(FIORDCHK)
#	define GETREADQUEUE(fd, int)	int = ioctl((fd), FIORDCHK, NULL)
#   else
#       define GETREADQUEUE(fd, int)    int = 0
#   endif

#   ifdef TIOCOUTQ
#	define GETWRITEQUEUE(fd, int)	ioctl((fd), TIOCOUTQ, &(int))
#   else
#	define GETWRITEQUEUE(fd, int)	int = 0
#   endif

#   if !defined(CRTSCTS) && defined(CNEW_RTSCTS)
#	define CRTSCTS CNEW_RTSCTS
#   endif /* !CRTSCTS&CNEW_RTSCTS */
#   if !defined(PAREXT) && defined(CMSPAR)
#	define PAREXT CMSPAR
#   endif /* !PAREXT&&CMSPAR */

#endif	/* HAVE_TERMIOS_H */

/*
 * Helper macros to make parts of this file clearer. The macros do exactly
 * what they say on the tin. :-) They also only ever refer to their arguments
 * once, and so can be used without regard to side effects.
 */

#define SET_BITS(var, bits)	((var) |= (bits))
#define CLEAR_BITS(var, bits)	((var) &= ~(bits))

/*
 * This structure describes per-instance state of a file based channel.
 */

typedef struct FileState {
    Tcl_Channel channel;	/* Channel associated with this file. */
    int fd;			/* File handle. */
    int validMask;		/* OR'ed combination of TCL_READABLE,
				 * TCL_WRITABLE, or TCL_EXCEPTION: indicates
				 * which operations are valid on the file. */
} FileState;

#ifdef SUPPORTS_TTY

/*
 * The following structure is used to set or get the serial port attributes in
 * a platform-independant manner.
 */

typedef struct TtyAttrs {
    int baud;
    int parity;
    int data;
    int stop;
} TtyAttrs;

#endif	/* !SUPPORTS_TTY */

#define UNSUPPORTED_OPTION(detail) \
    if (interp) {							\
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(				\
		"%s not supported for this platform", (detail)));	\
	Tcl_SetErrorCode(interp, "TCL", "UNSUPPORTED", NULL);		\
    }

/*
 * Static routines for this file:
 */

static int		FileBlockModeProc(ClientData instanceData, int mode);
static int		FileCloseProc(ClientData instanceData,
			    Tcl_Interp *interp);
static int		FileGetHandleProc(ClientData instanceData,
			    int direction, ClientData *handlePtr);
static int		FileInputProc(ClientData instanceData, char *buf,
			    int toRead, int *errorCode);
static int		FileOutputProc(ClientData instanceData,
			    const char *buf, int toWrite, int *errorCode);
static int		FileSeekProc(ClientData instanceData, long offset,
			    int mode, int *errorCode);
static int		FileTruncateProc(ClientData instanceData,
			    Tcl_WideInt length);
static Tcl_WideInt	FileWideSeekProc(ClientData instanceData,
			    Tcl_WideInt offset, int mode, int *errorCode);
static void		FileWatchProc(ClientData instanceData, int mask);
#ifdef SUPPORTS_TTY
static void		TtyGetAttributes(int fd, TtyAttrs *ttyPtr);
static int		TtyGetOptionProc(ClientData instanceData,
			    Tcl_Interp *interp, const char *optionName,
			    Tcl_DString *dsPtr);
static int		TtyGetBaud(speed_t speed);
static speed_t		TtyGetSpeed(int baud);
static void		TtyInit(int fd);
static void		TtyModemStatusStr(int status, Tcl_DString *dsPtr);
static int		TtyParseMode(Tcl_Interp *interp, const char *mode,
			    TtyAttrs *ttyPtr);
static void		TtySetAttributes(int fd, TtyAttrs *ttyPtr);
static int		TtySetOptionProc(ClientData instanceData,
			    Tcl_Interp *interp, const char *optionName,
			    const char *value);
#endif	/* SUPPORTS_TTY */

/*
 * This structure describes the channel type structure for file based IO:
 */

static const Tcl_ChannelType fileChannelType = {
    "file",			/* Type name. */
    TCL_CHANNEL_VERSION_5,	/* v5 channel */
    FileCloseProc,		/* Close proc. */
    FileInputProc,		/* Input proc. */
    FileOutputProc,		/* Output proc. */
    FileSeekProc,		/* Seek proc. */
    NULL,			/* Set option proc. */
    NULL,			/* Get option proc. */
    FileWatchProc,		/* Initialize notifier. */
    FileGetHandleProc,		/* Get OS handles out of channel. */
    NULL,			/* close2proc. */
    FileBlockModeProc,		/* Set blocking or non-blocking mode.*/
    NULL,			/* flush proc. */
    NULL,			/* handler proc. */
    FileWideSeekProc,		/* wide seek proc. */
    NULL,
    FileTruncateProc		/* truncate proc. */
};

#ifdef SUPPORTS_TTY
/*
 * This structure describes the channel type structure for serial IO.
 * Note that this type is a subclass of the "file" type.
 */

static const Tcl_ChannelType ttyChannelType = {
    "tty",			/* Type name. */
    TCL_CHANNEL_VERSION_5,	/* v5 channel */
    FileCloseProc,		/* Close proc. */
    FileInputProc,		/* Input proc. */
    FileOutputProc,		/* Output proc. */
    NULL,			/* Seek proc. */
    TtySetOptionProc,		/* Set option proc. */
    TtyGetOptionProc,		/* Get option proc. */
    FileWatchProc,		/* Initialize notifier. */
    FileGetHandleProc,		/* Get OS handles out of channel. */
    NULL,			/* close2proc. */
    FileBlockModeProc,		/* Set blocking or non-blocking mode.*/
    NULL,			/* flush proc. */
    NULL,			/* handler proc. */
    NULL,			/* wide seek proc. */
    NULL,			/* thread action proc. */
    NULL			/* truncate proc. */
};
#endif	/* SUPPORTS_TTY */

/*
 *----------------------------------------------------------------------
 *
 * FileBlockModeProc --
 *
 *	Helper function to set blocking and nonblocking modes on a file based
 *	channel. Invoked by generic IO level code.
 *
 * Results:
 *	0 if successful, errno when failed.
 *
 * Side effects:
 *	Sets the device into blocking or non-blocking mode.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
FileBlockModeProc(
    ClientData instanceData,	/* File state. */
    int mode)			/* The mode to set. Can be TCL_MODE_BLOCKING
				 * or TCL_MODE_NONBLOCKING. */
{
    FileState *fsPtr = instanceData;

    if (TclUnixSetBlockingMode(fsPtr->fd, mode) < 0) {
	return errno;
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * FileInputProc --
 *
 *	This function is invoked from the generic IO level to read input from
 *	a file based channel.
 *
 * Results:
 *	The number of bytes read is returned or -1 on error. An output
 *	argument contains a POSIX error code if an error occurs, or zero.
 *
 * Side effects:
 *	Reads input from the input device of the channel.
 *
 *----------------------------------------------------------------------
 */

static int
FileInputProc(
    ClientData instanceData,	/* File state. */
    char *buf,			/* Where to store data read. */
    int toRead,			/* How much space is available in the
				 * buffer? */
    int *errorCodePtr)		/* Where to store error code. */
{
    FileState *fsPtr = instanceData;
    int bytesRead;		/* How many bytes were actually read from the
				 * input device? */

    *errorCodePtr = 0;

    /*
     * Assume there is always enough input available. This will block
     * appropriately, and read will unblock as soon as a short read is
     * possible, if the channel is in blocking mode. If the channel is
     * nonblocking, the read will never block.
     */

    bytesRead = read(fsPtr->fd, buf, (size_t) toRead);
    if (bytesRead > -1) {
	return bytesRead;
    }
    *errorCodePtr = errno;
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * FileOutputProc--
 *
 *	This function is invoked from the generic IO level to write output to
 *	a file channel.
 *
 * Results:
 *	The number of bytes written is returned or -1 on error. An output
 *	argument contains a POSIX error code if an error occurred, or zero.
 *
 * Side effects:
 *	Writes output on the output device of the channel.
 *
 *----------------------------------------------------------------------
 */

static int
FileOutputProc(
    ClientData instanceData,	/* File state. */
    const char *buf,		/* The data buffer. */
    int toWrite,		/* How many bytes to write? */
    int *errorCodePtr)		/* Where to store error code. */
{
    FileState *fsPtr = instanceData;
    int written;

    *errorCodePtr = 0;

    if (toWrite == 0) {
	/*
	 * SF Tcl Bug 465765. Do not try to write nothing into a file. STREAM
	 * based implementations will considers this as EOF (if there is a
	 * pipe behind the file).
	 */

	return 0;
    }
    written = write(fsPtr->fd, buf, (size_t) toWrite);
    if (written > -1) {
	return written;
    }
    *errorCodePtr = errno;
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * FileCloseProc --
 *
 *	This function is called from the generic IO level to perform
 *	channel-type-specific cleanup when a file based channel is closed.
 *
 * Results:
 *	0 if successful, errno if failed.
 *
 * Side effects:
 *	Closes the device of the channel.
 *
 *----------------------------------------------------------------------
 */

static int
FileCloseProc(
    ClientData instanceData,	/* File state. */
    Tcl_Interp *interp)		/* For error reporting - unused. */
{
    FileState *fsPtr = instanceData;
    int errorCode = 0;

    Tcl_DeleteFileHandler(fsPtr->fd);

    /*
     * Do not close standard channels while in thread-exit.
     */

    if (!TclInThreadExit()
	    || ((fsPtr->fd != 0) && (fsPtr->fd != 1) && (fsPtr->fd != 2))) {
	if (close(fsPtr->fd) < 0) {
	    errorCode = errno;
	}
    }
    ckfree(fsPtr);
    return errorCode;
}

/*
 *----------------------------------------------------------------------
 *
 * FileSeekProc --
 *
 *	This function is called by the generic IO level to move the access
 *	point in a file based channel.
 *
 * Results:
 *	-1 if failed, the new position if successful. An output argument
 *	contains the POSIX error code if an error occurred, or zero.
 *
 * Side effects:
 *	Moves the location at which the channel will be accessed in future
 *	operations.
 *
 *----------------------------------------------------------------------
 */

static int
FileSeekProc(
    ClientData instanceData,	/* File state. */
    long offset,		/* Offset to seek to. */
    int mode,			/* Relative to where should we seek? Can be
				 * one of SEEK_START, SEEK_SET or SEEK_END. */
    int *errorCodePtr)		/* To store error code. */
{
    FileState *fsPtr = instanceData;
    Tcl_WideInt oldLoc, newLoc;

    /*
     * Save our current place in case we need to roll-back the seek.
     */

    oldLoc = TclOSseek(fsPtr->fd, (Tcl_SeekOffset) 0, SEEK_CUR);
    if (oldLoc == Tcl_LongAsWide(-1)) {
	/*
	 * Bad things are happening. Error out...
	 */

	*errorCodePtr = errno;
	return -1;
    }

    newLoc = TclOSseek(fsPtr->fd, (Tcl_SeekOffset) offset, mode);

    /*
     * Check for expressability in our return type, and roll-back otherwise.
     */

    if (newLoc > Tcl_LongAsWide(INT_MAX)) {
	*errorCodePtr = EOVERFLOW;
	TclOSseek(fsPtr->fd, (Tcl_SeekOffset) oldLoc, SEEK_SET);
	return -1;
    } else {
	*errorCodePtr = (newLoc == Tcl_LongAsWide(-1)) ? errno : 0;
    }
    return (int) Tcl_WideAsLong(newLoc);
}

/*
 *----------------------------------------------------------------------
 *
 * FileWideSeekProc --
 *
 *	This function is called by the generic IO level to move the access
 *	point in a file based channel, with offsets expressed as wide
 *	integers.
 *
 * Results:
 *	-1 if failed, the new position if successful. An output argument
 *	contains the POSIX error code if an error occurred, or zero.
 *
 * Side effects:
 *	Moves the location at which the channel will be accessed in future
 *	operations.
 *
 *----------------------------------------------------------------------
 */

static Tcl_WideInt
FileWideSeekProc(
    ClientData instanceData,	/* File state. */
    Tcl_WideInt offset,		/* Offset to seek to. */
    int mode,			/* Relative to where should we seek? Can be
				 * one of SEEK_START, SEEK_CUR or SEEK_END. */
    int *errorCodePtr)		/* To store error code. */
{
    FileState *fsPtr = instanceData;
    Tcl_WideInt newLoc;

    newLoc = TclOSseek(fsPtr->fd, (Tcl_SeekOffset) offset, mode);

    *errorCodePtr = (newLoc == -1) ? errno : 0;
    return newLoc;
}

/*
 *----------------------------------------------------------------------
 *
 * FileWatchProc --
 *
 *	Initialize the notifier to watch the fd from this channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets up the notifier so that a future event on the channel will
 *	be seen by Tcl.
 *
 *----------------------------------------------------------------------
 */

static void
FileWatchProc(
    ClientData instanceData,	/* The file state. */
    int mask)			/* Events of interest; an OR-ed combination of
				 * TCL_READABLE, TCL_WRITABLE and
				 * TCL_EXCEPTION. */
{
    FileState *fsPtr = instanceData;

    /*
     * Make sure we only register for events that are valid on this file. Note
     * that we are passing Tcl_NotifyChannel directly to Tcl_CreateFileHandler
     * with the channel pointer as the client data.
     */

    mask &= fsPtr->validMask;
    if (mask) {
	Tcl_CreateFileHandler(fsPtr->fd, mask,
		(Tcl_FileProc *) Tcl_NotifyChannel, fsPtr->channel);
    } else {
	Tcl_DeleteFileHandler(fsPtr->fd);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FileGetHandleProc --
 *
 *	Called from Tcl_GetChannelHandle to retrieve OS handles from a file
 *	based channel.
 *
 * Results:
 *	Returns TCL_OK with the fd in handlePtr, or TCL_ERROR if there is no
 *	handle for the specified direction.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
FileGetHandleProc(
    ClientData instanceData,	/* The file state. */
    int direction,		/* TCL_READABLE or TCL_WRITABLE */
    ClientData *handlePtr)	/* Where to store the handle. */
{
    FileState *fsPtr = instanceData;

    if (direction & fsPtr->validMask) {
	*handlePtr = INT2PTR(fsPtr->fd);
	return TCL_OK;
    }
    return TCL_ERROR;
}

#ifdef SUPPORTS_TTY
/*
 *----------------------------------------------------------------------
 *
 * TtyModemStatusStr --
 *
 *	Converts a RS232 modem status list of readable flags
 *
 *----------------------------------------------------------------------
 */

static void
TtyModemStatusStr(
    int status,			/* RS232 modem status */
    Tcl_DString *dsPtr)		/* Where to store string */
{
#ifdef TIOCM_CTS
    Tcl_DStringAppendElement(dsPtr, "CTS");
    Tcl_DStringAppendElement(dsPtr, (status & TIOCM_CTS) ? "1" : "0");
#endif /* TIOCM_CTS */
#ifdef TIOCM_DSR
    Tcl_DStringAppendElement(dsPtr, "DSR");
    Tcl_DStringAppendElement(dsPtr, (status & TIOCM_DSR) ? "1" : "0");
#endif /* TIOCM_DSR */
#ifdef TIOCM_RNG
    Tcl_DStringAppendElement(dsPtr, "RING");
    Tcl_DStringAppendElement(dsPtr, (status & TIOCM_RNG) ? "1" : "0");
#endif /* TIOCM_RNG */
#ifdef TIOCM_CD
    Tcl_DStringAppendElement(dsPtr, "DCD");
    Tcl_DStringAppendElement(dsPtr, (status & TIOCM_CD) ? "1" : "0");
#endif /* TIOCM_CD */
}

/*
 *----------------------------------------------------------------------
 *
 * TtySetOptionProc --
 *
 *	Sets an option on a channel.
 *
 * Results:
 *	A standard Tcl result. Also sets the interp's result on error if
 *	interp is not NULL.
 *
 * Side effects:
 *	May modify an option on a device. Sets Error message if needed (by
 *	calling Tcl_BadChannelOption).
 *
 *----------------------------------------------------------------------
 */

static int
TtySetOptionProc(
    ClientData instanceData,	/* File state. */
    Tcl_Interp *interp,		/* For error reporting - can be NULL. */
    const char *optionName,	/* Which option to set? */
    const char *value)		/* New value for option. */
{
    FileState *fsPtr = instanceData;
    unsigned int len, vlen;
    TtyAttrs tty;
    int argc;
    const char **argv;
    struct termios iostate;

    len = strlen(optionName);
    vlen = strlen(value);

    /*
     * Option -mode baud,parity,databits,stopbits
     */

    if ((len > 2) && (strncmp(optionName, "-mode", len) == 0)) {
	if (TtyParseMode(interp, value, &tty) != TCL_OK) {
	    return TCL_ERROR;
	}

	/*
	 * system calls results should be checked there. - dl
	 */

	TtySetAttributes(fsPtr->fd, &tty);
	return TCL_OK;
    }


    /*
     * Option -handshake none|xonxoff|rtscts|dtrdsr
     */

    if ((len > 1) && (strncmp(optionName, "-handshake", len) == 0)) {
	/*
	 * Reset all handshake options. DTR and RTS are ON by default.
	 */

	tcgetattr(fsPtr->fd, &iostate);
	CLEAR_BITS(iostate.c_iflag, IXON | IXOFF | IXANY);
#ifdef CRTSCTS
	CLEAR_BITS(iostate.c_cflag, CRTSCTS);
#endif /* CRTSCTS */
	if (Tcl_UtfNcasecmp(value, "NONE", vlen) == 0) {
	    /*
	     * Leave all handshake options disabled.
	     */
	} else if (Tcl_UtfNcasecmp(value, "XONXOFF", vlen) == 0) {
	    SET_BITS(iostate.c_iflag, IXON | IXOFF | IXANY);
	} else if (Tcl_UtfNcasecmp(value, "RTSCTS", vlen) == 0) {
#ifdef CRTSCTS
	    SET_BITS(iostate.c_cflag, CRTSCTS);
#else /* !CRTSTS */
	    UNSUPPORTED_OPTION("-handshake RTSCTS");
	    return TCL_ERROR;
#endif /* CRTSCTS */
	} else if (Tcl_UtfNcasecmp(value, "DTRDSR", vlen) == 0) {
	    UNSUPPORTED_OPTION("-handshake DTRDSR");
	    return TCL_ERROR;
	} else {
	    if (interp) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"bad value for -handshake: must be one of"
			" xonxoff, rtscts, dtrdsr or none", -1));
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "FCONFIGURE",
			"VALUE", NULL);
	    }
	    return TCL_ERROR;
	}
	tcsetattr(fsPtr->fd, TCSADRAIN, &iostate);
	return TCL_OK;
    }

    /*
     * Option -xchar {\x11 \x13}
     */

    if ((len > 1) && (strncmp(optionName, "-xchar", len) == 0)) {
	Tcl_DString ds;

	if (Tcl_SplitList(interp, value, &argc, &argv) == TCL_ERROR) {
	    return TCL_ERROR;
	} else if (argc != 2) {
	    if (interp) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"bad value for -xchar: should be a list of"
			" two elements", -1));
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "FCONFIGURE",
			"VALUE", NULL);
	    }
	    ckfree(argv);
	    return TCL_ERROR;
	}

	tcgetattr(fsPtr->fd, &iostate);

	Tcl_UtfToExternalDString(NULL, argv[0], -1, &ds);
	iostate.c_cc[VSTART] = *(const cc_t *) Tcl_DStringValue(&ds);
	TclDStringClear(&ds);

	Tcl_UtfToExternalDString(NULL, argv[1], -1, &ds);
	iostate.c_cc[VSTOP] = *(const cc_t *) Tcl_DStringValue(&ds);
	Tcl_DStringFree(&ds);
	ckfree(argv);

	tcsetattr(fsPtr->fd, TCSADRAIN, &iostate);
	return TCL_OK;
    }

    /*
     * Option -timeout msec
     */

    if ((len > 2) && (strncmp(optionName, "-timeout", len) == 0)) {
	int msec;

	tcgetattr(fsPtr->fd, &iostate);
	if (Tcl_GetInt(interp, value, &msec) != TCL_OK) {
	    return TCL_ERROR;
	}
	iostate.c_cc[VMIN] = 0;
	iostate.c_cc[VTIME] = (msec==0) ? 0 : (msec<100) ? 1 : (msec+50)/100;
	tcsetattr(fsPtr->fd, TCSADRAIN, &iostate);
	return TCL_OK;
    }

    /*
     * Option -ttycontrol {DTR 1 RTS 0 BREAK 0}
     */
    if ((len > 4) && (strncmp(optionName, "-ttycontrol", len) == 0)) {
#if defined(TIOCMGET) && defined(TIOCMSET)
	int i, control, flag;

	if (Tcl_SplitList(interp, value, &argc, &argv) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	if ((argc % 2) == 1) {
	    if (interp) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"bad value for -ttycontrol: should be a list of"
			" signal,value pairs", -1));
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "FCONFIGURE",
			"VALUE", NULL);
	    }
	    ckfree(argv);
	    return TCL_ERROR;
	}

	ioctl(fsPtr->fd, TIOCMGET, &control);
	for (i = 0; i < argc-1; i += 2) {
	    if (Tcl_GetBoolean(interp, argv[i+1], &flag) == TCL_ERROR) {
		ckfree(argv);
		return TCL_ERROR;
	    }
	    if (Tcl_UtfNcasecmp(argv[i], "DTR", strlen(argv[i])) == 0) {
		if (flag) {
		    SET_BITS(control, TIOCM_DTR);
		} else {
		    CLEAR_BITS(control, TIOCM_DTR);
		}
	    } else if (Tcl_UtfNcasecmp(argv[i], "RTS", strlen(argv[i])) == 0) {
		if (flag) {
		    SET_BITS(control, TIOCM_RTS);
		} else {
		    CLEAR_BITS(control, TIOCM_RTS);
		}
	    } else if (Tcl_UtfNcasecmp(argv[i], "BREAK", strlen(argv[i])) == 0) {
#if defined(TIOCSBRK) && defined(TIOCCBRK)
		if (flag) {
		    ioctl(fsPtr->fd, TIOCSBRK, NULL);
		} else {
		    ioctl(fsPtr->fd, TIOCCBRK, NULL);
		}
#else /* TIOCSBRK & TIOCCBRK */
		UNSUPPORTED_OPTION("-ttycontrol BREAK");
		ckfree(argv);
		return TCL_ERROR;
#endif /* TIOCSBRK & TIOCCBRK */
	    } else {
		if (interp) {
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "bad signal \"%s\" for -ttycontrol: must be"
			    " DTR, RTS or BREAK", argv[i]));
		    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "FCONFIGURE",
			"VALUE", NULL);
		}
		ckfree(argv);
		return TCL_ERROR;
	    }
	} /* -ttycontrol options loop */

	ioctl(fsPtr->fd, TIOCMSET, &control);
	ckfree(argv);
	return TCL_OK;
#else /* TIOCMGET&TIOCMSET */
	UNSUPPORTED_OPTION("-ttycontrol");
#endif /* TIOCMGET&TIOCMSET */
    }

    return Tcl_BadChannelOption(interp, optionName,
	    "mode handshake timeout ttycontrol xchar");
}

/*
 *----------------------------------------------------------------------
 *
 * TtyGetOptionProc --
 *
 *	Gets a mode associated with an IO channel. If the optionName arg is
 *	non-NULL, retrieves the value of that option. If the optionName arg is
 *	NULL, retrieves a list of alternating option names and values for the
 *	given channel.
 *
 * Results:
 *	A standard Tcl result. Also sets the supplied DString to the string
 *	value of the option(s) returned.  Sets error message if needed
 *	(by calling Tcl_BadChannelOption).
 *
 *----------------------------------------------------------------------
 */

static int
TtyGetOptionProc(
    ClientData instanceData,	/* File state. */
    Tcl_Interp *interp,		/* For error reporting - can be NULL. */
    const char *optionName,	/* Option to get. */
    Tcl_DString *dsPtr)		/* Where to store value(s). */
{
    FileState *fsPtr = instanceData;
    unsigned int len;
    char buf[3*TCL_INTEGER_SPACE + 16];
    int valid = 0;		/* Flag if valid option parsed. */

    if (optionName == NULL) {
	len = 0;
    } else {
	len = strlen(optionName);
    }
    if (len == 0) {
	Tcl_DStringAppendElement(dsPtr, "-mode");
    }
    if (len==0 || (len>2 && strncmp(optionName, "-mode", len)==0)) {
	TtyAttrs tty;

	valid = 1;
	TtyGetAttributes(fsPtr->fd, &tty);
	sprintf(buf, "%d,%c,%d,%d", tty.baud, tty.parity, tty.data, tty.stop);
	Tcl_DStringAppendElement(dsPtr, buf);
    }

    /*
     * Get option -xchar
     */

    if (len == 0) {
	Tcl_DStringAppendElement(dsPtr, "-xchar");
	Tcl_DStringStartSublist(dsPtr);
    }
    if (len==0 || (len>1 && strncmp(optionName, "-xchar", len)==0)) {
	struct termios iostate;
	Tcl_DString ds;

	valid = 1;
	tcgetattr(fsPtr->fd, &iostate);
	Tcl_DStringInit(&ds);

	Tcl_ExternalToUtfDString(NULL, (char *) &iostate.c_cc[VSTART], 1, &ds);
	Tcl_DStringAppendElement(dsPtr, Tcl_DStringValue(&ds));
	TclDStringClear(&ds);

	Tcl_ExternalToUtfDString(NULL, (char *) &iostate.c_cc[VSTOP], 1, &ds);
	Tcl_DStringAppendElement(dsPtr, Tcl_DStringValue(&ds));
	Tcl_DStringFree(&ds);
    }
    if (len == 0) {
	Tcl_DStringEndSublist(dsPtr);
    }

    /*
     * Get option -queue
     * Option is readonly and returned by [fconfigure chan -queue] but not
     * returned by unnamed [fconfigure chan].
     */

    if ((len > 1) && (strncmp(optionName, "-queue", len) == 0)) {
	int inQueue=0, outQueue=0, inBuffered, outBuffered;

	valid = 1;
	GETREADQUEUE(fsPtr->fd, inQueue);
	GETWRITEQUEUE(fsPtr->fd, outQueue);
	inBuffered = Tcl_InputBuffered(fsPtr->channel);
	outBuffered = Tcl_OutputBuffered(fsPtr->channel);

	sprintf(buf, "%d", inBuffered+inQueue);
	Tcl_DStringAppendElement(dsPtr, buf);
	sprintf(buf, "%d", outBuffered+outQueue);
	Tcl_DStringAppendElement(dsPtr, buf);
    }

#if defined(TIOCMGET)
    /*
     * Get option -ttystatus
     * Option is readonly and returned by [fconfigure chan -ttystatus] but not
     * returned by unnamed [fconfigure chan].
     */
    if ((len > 4) && (strncmp(optionName, "-ttystatus", len) == 0)) {
	int status;

	valid = 1;
	ioctl(fsPtr->fd, TIOCMGET, &status);
	TtyModemStatusStr(status, dsPtr);
    }
#endif /* TIOCMGET */

    if (valid) {
	return TCL_OK;
    }
    return Tcl_BadChannelOption(interp, optionName, "mode"
	    " queue ttystatus xchar"
	    );
}


static const struct {int baud; speed_t speed;} speeds[] = {
#ifdef B0
    {0, B0},
#endif
#ifdef B50
    {50, B50},
#endif
#ifdef B75
    {75, B75},
#endif
#ifdef B110
    {110, B110},
#endif
#ifdef B134
    {134, B134},
#endif
#ifdef B150
    {150, B150},
#endif
#ifdef B200
    {200, B200},
#endif
#ifdef B300
    {300, B300},
#endif
#ifdef B600
    {600, B600},
#endif
#ifdef B1200
    {1200, B1200},
#endif
#ifdef B1800
    {1800, B1800},
#endif
#ifdef B2400
    {2400, B2400},
#endif
#ifdef B4800
    {4800, B4800},
#endif
#ifdef B9600
    {9600, B9600},
#endif
#ifdef B14400
    {14400, B14400},
#endif
#ifdef B19200
    {19200, B19200},
#endif
#ifdef EXTA
    {19200, EXTA},
#endif
#ifdef B28800
    {28800, B28800},
#endif
#ifdef B38400
    {38400, B38400},
#endif
#ifdef EXTB
    {38400, EXTB},
#endif
#ifdef B57600
    {57600, B57600},
#endif
#ifdef _B57600
    {57600, _B57600},
#endif
#ifdef B76800
    {76800, B76800},
#endif
#ifdef B115200
    {115200, B115200},
#endif
#ifdef _B115200
    {115200, _B115200},
#endif
#ifdef B153600
    {153600, B153600},
#endif
#ifdef B230400
    {230400, B230400},
#endif
#ifdef B307200
    {307200, B307200},
#endif
#ifdef B460800
    {460800, B460800},
#endif
#ifdef B500000
    {500000, B500000},
#endif
#ifdef B576000
    {576000, B576000},
#endif
#ifdef B921600
    {921600, B921600},
#endif
#ifdef B1000000
    {1000000, B1000000},
#endif
#ifdef B1152000
    {1152000, B1152000},
#endif
#ifdef B1500000
    {1500000,B1500000},
#endif
#ifdef B2000000
    {2000000, B2000000},
#endif
#ifdef B2500000
    {2500000,B2500000},
#endif
#ifdef B3000000
    {3000000,B3000000},
#endif
#ifdef B3500000
    {3500000,B3500000},
#endif
#ifdef B4000000
    {4000000,B4000000},
#endif
    {-1, 0}
};

/*
 *---------------------------------------------------------------------------
 *
 * TtyGetSpeed --
 *
 *	Given an integer baud rate, get the speed_t value that should be
 *	used to select that baud rate.
 *
 * Results:
 *	As above.
 *
 *---------------------------------------------------------------------------
 */

static speed_t
TtyGetSpeed(
    int baud)			/* The baud rate to look up. */
{
    int bestIdx, bestDiff, i, diff;

    bestIdx = 0;
    bestDiff = 1000000;

    /*
     * If the baud rate does not correspond to one of the known mask values,
     * choose the mask value whose baud rate is closest to the specified baud
     * rate.
     */

    for (i = 0; speeds[i].baud >= 0; i++) {
	diff = speeds[i].baud - baud;
	if (diff < 0) {
	    diff = -diff;
	}
	if (diff < bestDiff) {
	    bestIdx = i;
	    bestDiff = diff;
	}
    }
    return speeds[bestIdx].speed;
}

/*
 *---------------------------------------------------------------------------
 *
 * TtyGetBaud --
 *
 *	Return the integer baud rate corresponding to a given speed_t value.
 *
 * Results:
 *	As above. If the mask value was not recognized, 0 is returned.
 *
 *---------------------------------------------------------------------------
 */

static int
TtyGetBaud(
    speed_t speed)		/* Speed mask value to look up. */
{
    int i;

    for (i = 0; speeds[i].baud >= 0; i++) {
	if (speeds[i].speed == speed) {
	    return speeds[i].baud;
	}
    }
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * TtyGetAttributes --
 *
 *	Get the current attributes of the specified serial device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
TtyGetAttributes(
    int fd,			/* Open file descriptor for serial port to be
				 * queried. */
    TtyAttrs *ttyPtr)		/* Buffer filled with serial port
				 * attributes. */
{
    struct termios iostate;
    int baud, parity, data, stop;

    tcgetattr(fd, &iostate);

    baud = TtyGetBaud(cfgetospeed(&iostate));

    parity = 'n';
#ifdef PAREXT
    switch ((int) (iostate.c_cflag & (PARENB | PARODD | PAREXT))) {
    case PARENB			  : parity = 'e'; break;
    case PARENB | PARODD	  : parity = 'o'; break;
    case PARENB |	   PAREXT : parity = 's'; break;
    case PARENB | PARODD | PAREXT : parity = 'm'; break;
    }
#else /* !PAREXT */
    switch ((int) (iostate.c_cflag & (PARENB | PARODD))) {
    case PARENB		 : parity = 'e'; break;
    case PARENB | PARODD : parity = 'o'; break;
    }
#endif /* PAREXT */

    data = iostate.c_cflag & CSIZE;
    data = (data == CS5) ? 5 : (data == CS6) ? 6 : (data == CS7) ? 7 : 8;

    stop = (iostate.c_cflag & CSTOPB) ? 2 : 1;

    ttyPtr->baud    = baud;
    ttyPtr->parity  = parity;
    ttyPtr->data    = data;
    ttyPtr->stop    = stop;
}

/*
 *---------------------------------------------------------------------------
 *
 * TtySetAttributes --
 *
 *	Set the current attributes of the specified serial device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
TtySetAttributes(
    int fd,			/* Open file descriptor for serial port to be
				 * modified. */
    TtyAttrs *ttyPtr)		/* Buffer containing new attributes for serial
				 * port. */
{
    struct termios iostate;
    int parity, data, flag;

    tcgetattr(fd, &iostate);
    cfsetospeed(&iostate, TtyGetSpeed(ttyPtr->baud));
    cfsetispeed(&iostate, TtyGetSpeed(ttyPtr->baud));

    flag = 0;
    parity = ttyPtr->parity;
    if (parity != 'n') {
	SET_BITS(flag, PARENB);
#ifdef PAREXT
	CLEAR_BITS(iostate.c_cflag, PAREXT);
	if ((parity == 'm') || (parity == 's')) {
	    SET_BITS(flag, PAREXT);
	}
#endif /* PAREXT */
	if ((parity == 'm') || (parity == 'o')) {
	    SET_BITS(flag, PARODD);
	}
    }
    data = ttyPtr->data;
    SET_BITS(flag,
	    (data == 5) ? CS5 :
	    (data == 6) ? CS6 :
	    (data == 7) ? CS7 : CS8);
    if (ttyPtr->stop == 2) {
	SET_BITS(flag, CSTOPB);
    }

    CLEAR_BITS(iostate.c_cflag, PARENB | PARODD | CSIZE | CSTOPB);
    SET_BITS(iostate.c_cflag, flag);

    tcsetattr(fd, TCSADRAIN, &iostate);
}

/*
 *---------------------------------------------------------------------------
 *
 * TtyParseMode --
 *
 *	Parse the "-mode" argument to the fconfigure command. The argument is
 *	of the form baud,parity,data,stop.
 *
 * Results:
 *	The return value is TCL_OK if the argument was successfully parsed,
 *	TCL_ERROR otherwise. If TCL_ERROR is returned, an error message is
 *	left in the interp's result (if interp is non-NULL).
 *
 *---------------------------------------------------------------------------
 */

static int
TtyParseMode(
    Tcl_Interp *interp,		/* If non-NULL, interp for error return. */
    const char *mode,		/* Mode string to be parsed. */
    TtyAttrs *ttyPtr)		/* Filled with data from mode string */
{
    int i, end;
    char parity;
    const char *bad = "bad value for -mode";

    i = sscanf(mode, "%d,%c,%d,%d%n",
	    &ttyPtr->baud,
	    &parity,
	    &ttyPtr->data,
	    &ttyPtr->stop, &end);
    if ((i != 4) || (mode[end] != '\0')) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "%s: should be baud,parity,data,stop", bad));
	    Tcl_SetErrorCode(interp, "TCL", "VALUE", "SERIALMODE", NULL);
	}
	return TCL_ERROR;
    }

    /*
     * Only allow setting mark/space parity on platforms that support it Make
     * sure to allow for the case where strchr is a macro. [Bug: 5089]
     *
     * We cannot if/else/endif the strchr arguments, it has to be the whole
     * function. On AIX this function is apparently a macro, and macros do
     * not allow pre-processor directives in their arguments.
     */

    if (
#if defined(PAREXT)
        strchr("noems", parity)
#else
        strchr("noe", parity)
#endif /* PAREXT */
                               == NULL) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "%s parity: should be %s", bad,
#if defined(PAREXT)
		    "n, o, e, m, or s"
#else
		    "n, o, or e"
#endif /* PAREXT */
		    ));
	    Tcl_SetErrorCode(interp, "TCL", "VALUE", "SERIALMODE", NULL);
	}
	return TCL_ERROR;
    }
    ttyPtr->parity = parity;
    if ((ttyPtr->data < 5) || (ttyPtr->data > 8)) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "%s data: should be 5, 6, 7, or 8", bad));
	    Tcl_SetErrorCode(interp, "TCL", "VALUE", "SERIALMODE", NULL);
	}
	return TCL_ERROR;
    }
    if ((ttyPtr->stop < 0) || (ttyPtr->stop > 2)) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "%s stop: should be 1 or 2", bad));
	    Tcl_SetErrorCode(interp, "TCL", "VALUE", "SERIALMODE", NULL);
	}
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TtyInit --
 *
 *	Given file descriptor that refers to a serial port, initialize the
 *	serial port to a set of sane values so that Tcl can talk to a device
 *	located on the serial port.
 *
 * Side effects:
 *	Serial device initialized to non-blocking raw mode, similar to sockets
 *	All other modes can be simulated on top of this in Tcl.
 *
 *---------------------------------------------------------------------------
 */

static void
TtyInit(
    int fd)	/* Open file descriptor for serial port to be initialized. */
{
    struct termios iostate;
    tcgetattr(fd, &iostate);

    if (iostate.c_iflag != IGNBRK
	    || iostate.c_oflag != 0
	    || iostate.c_lflag != 0
	    || iostate.c_cflag & CREAD
	    || iostate.c_cc[VMIN] != 1
	    || iostate.c_cc[VTIME] != 0)
    {
	iostate.c_iflag = IGNBRK;
	iostate.c_oflag = 0;
	iostate.c_lflag = 0;
	iostate.c_cflag |= CREAD;
	iostate.c_cc[VMIN] = 1;
	iostate.c_cc[VTIME] = 0;

	tcsetattr(fd, TCSADRAIN, &iostate);
    }
}
#endif	/* SUPPORTS_TTY */

/*
 *----------------------------------------------------------------------
 *
 * TclpOpenFileChannel --
 *
 *	Open an file based channel on Unix systems.
 *
 * Results:
 *	The new channel or NULL. If NULL, the output argument errorCodePtr is
 *	set to a POSIX error and an error message is left in the interp's
 *	result if interp is not NULL.
 *
 * Side effects:
 *	May open the channel and may cause creation of a file on the file
 *	system.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
TclpOpenFileChannel(
    Tcl_Interp *interp,		/* Interpreter for error reporting; can be
				 * NULL. */
    Tcl_Obj *pathPtr,		/* Name of file to open. */
    int mode,			/* POSIX open mode. */
    int permissions)		/* If the open involves creating a file, with
				 * what modes to create it? */
{
    int fd, channelPermissions;
    FileState *fsPtr;
    const char *native, *translation;
    char channelName[16 + TCL_INTEGER_SPACE];
    const Tcl_ChannelType *channelTypePtr;

    switch (mode & (O_RDONLY | O_WRONLY | O_RDWR)) {
    case O_RDONLY:
	channelPermissions = TCL_READABLE;
	break;
    case O_WRONLY:
	channelPermissions = TCL_WRITABLE;
	break;
    case O_RDWR:
	channelPermissions = (TCL_READABLE | TCL_WRITABLE);
	break;
    default:
	/*
	 * This may occurr if modeString was "", for example.
	 */

	Tcl_Panic("TclpOpenFileChannel: invalid mode value");
	return NULL;
    }

    native = Tcl_FSGetNativePath(pathPtr);
    if (native == NULL) {
	if (interp != (Tcl_Interp *) NULL) {
	    Tcl_AppendResult(interp, "couldn't open \"",
	    TclGetString(pathPtr), "\": filename is invalid on this platform",
	    NULL);
	}
	return NULL;
    }

#ifdef DJGPP
    SET_BITS(mode, O_BINARY);
#endif

    fd = TclOSopen(native, mode, permissions);

    if (fd < 0) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "couldn't open \"%s\": %s",
		    TclGetString(pathPtr), Tcl_PosixError(interp)));
	}
	return NULL;
    }

    /*
     * Set close-on-exec flag on the fd so that child processes will not
     * inherit this fd.
     */

    fcntl(fd, F_SETFD, FD_CLOEXEC);

    sprintf(channelName, "file%d", fd);

#ifdef SUPPORTS_TTY
    if (strcmp(native, "/dev/tty") != 0 && isatty(fd)) {
	/*
	 * Initialize the serial port to a set of sane parameters. Especially
	 * important if the remote device is set to echo and the serial port
	 * driver was also set to echo -- as soon as a char were sent to the
	 * serial port, the remote device would echo it, then the serial
	 * driver would echo it back to the device, etc.
	 *
	 * Note that we do not do this if we're dealing with /dev/tty itself,
	 * as that tends to cause Bad Things To Happen when you're working
	 * interactively. Strictly a better check would be to see if the FD
	 * being set up is a device and has the same major/minor as the
	 * initial std FDs (beware reopening!) but that's nearly as messy.
	 */

	translation = "auto crlf";
	channelTypePtr = &ttyChannelType;
	TtyInit(fd);
    } else
#endif	/* SUPPORTS_TTY */
    {
	translation = NULL;
	channelTypePtr = &fileChannelType;
    }

    fsPtr = ckalloc(sizeof(FileState));
    fsPtr->validMask = channelPermissions | TCL_EXCEPTION;
    fsPtr->fd = fd;

    fsPtr->channel = Tcl_CreateChannel(channelTypePtr, channelName,
	    fsPtr, channelPermissions);

    if (translation != NULL) {
	/*
	 * Gotcha. Most modems need a "\r" at the end of the command sequence.
	 * If you just send "at\n", the modem will not respond with "OK"
	 * because it never got a "\r" to actually invoke the command. So, by
	 * default, newlines are translated to "\r\n" on output to avoid "bug"
	 * reports that the serial port isn't working.
	 */

	if (Tcl_SetChannelOption(interp, fsPtr->channel, "-translation",
		translation) != TCL_OK) {
	    Tcl_Close(NULL, fsPtr->channel);
	    return NULL;
	}
    }

    return fsPtr->channel;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MakeFileChannel --
 *
 *	Makes a Tcl_Channel from an existing OS level file handle.
 *
 * Results:
 *	The Tcl_Channel created around the preexisting OS level file handle.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_MakeFileChannel(
    ClientData handle,		/* OS level handle. */
    int mode)			/* ORed combination of TCL_READABLE and
				 * TCL_WRITABLE to indicate file mode. */
{
    FileState *fsPtr;
    char channelName[16 + TCL_INTEGER_SPACE];
    int fd = PTR2INT(handle);
    const Tcl_ChannelType *channelTypePtr;
    struct sockaddr sockaddr;
    socklen_t sockaddrLen = sizeof(sockaddr);

    if (mode == 0) {
	return NULL;
    }

    sockaddr.sa_family = AF_UNSPEC;

#ifdef SUPPORTS_TTY
    if (isatty(fd)) {
	channelTypePtr = &ttyChannelType;
	sprintf(channelName, "serial%d", fd);
    } else
#endif /* SUPPORTS_TTY */
    if ((getsockname(fd, (struct sockaddr *)&sockaddr, &sockaddrLen) == 0)
	&& (sockaddrLen > 0)
	&& (sockaddr.sa_family == AF_INET || sockaddr.sa_family == AF_INET6)) {
	return TclpMakeTcpClientChannelMode(INT2PTR(fd), mode);
    } else {
	channelTypePtr = &fileChannelType;
	sprintf(channelName, "file%d", fd);
    }

    fsPtr = ckalloc(sizeof(FileState));
    fsPtr->fd = fd;
    fsPtr->validMask = mode | TCL_EXCEPTION;
    fsPtr->channel = Tcl_CreateChannel(channelTypePtr, channelName,
	    fsPtr, mode);

    return fsPtr->channel;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetDefaultStdChannel --
 *
 *	Creates channels for standard input, standard output or standard error
 *	output if they do not already exist.
 *
 * Results:
 *	Returns the specified default standard channel, or NULL.
 *
 * Side effects:
 *	May cause the creation of a standard channel and the underlying file.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
TclpGetDefaultStdChannel(
    int type)			/* One of TCL_STDIN, TCL_STDOUT, TCL_STDERR. */
{
    Tcl_Channel channel = NULL;
    int fd = 0;			/* Initializations needed to prevent */
    int mode = 0;		/* compiler warning (used before set). */
    const char *bufMode = NULL;

    /*
     * Some #def's to make the code a little clearer!
     */

#define ZERO_OFFSET	((Tcl_SeekOffset) 0)
#define ERROR_OFFSET	((Tcl_SeekOffset) -1)

    switch (type) {
    case TCL_STDIN:
	if ((TclOSseek(0, ZERO_OFFSET, SEEK_CUR) == ERROR_OFFSET)
		&& (errno == EBADF)) {
	    return NULL;
	}
	fd = 0;
	mode = TCL_READABLE;
	bufMode = "line";
	break;
    case TCL_STDOUT:
	if ((TclOSseek(1, ZERO_OFFSET, SEEK_CUR) == ERROR_OFFSET)
		&& (errno == EBADF)) {
	    return NULL;
	}
	fd = 1;
	mode = TCL_WRITABLE;
	bufMode = "line";
	break;
    case TCL_STDERR:
	if ((TclOSseek(2, ZERO_OFFSET, SEEK_CUR) == ERROR_OFFSET)
		&& (errno == EBADF)) {
	    return NULL;
	}
	fd = 2;
	mode = TCL_WRITABLE;
	bufMode = "none";
	break;
    default:
	Tcl_Panic("TclGetDefaultStdChannel: Unexpected channel type");
	break;
    }

#undef ZERO_OFFSET
#undef ERROR_OFFSET

    channel = Tcl_MakeFileChannel(INT2PTR(fd), mode);
    if (channel == NULL) {
	return NULL;
    }

    /*
     * Set up the normal channel options for stdio handles.
     */

    if (Tcl_GetChannelType(channel) == &fileChannelType) {
	Tcl_SetChannelOption(NULL, channel, "-translation", "auto");
    } else {
	Tcl_SetChannelOption(NULL, channel, "-translation", "auto crlf");
    }
    Tcl_SetChannelOption(NULL, channel, "-buffering", bufMode);
    return channel;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetOpenFile --
 *
 *	Given a name of a channel registered in the given interpreter, returns
 *	a FILE * for it.
 *
 * Results:
 *	A standard Tcl result. If the channel is registered in the given
 *	interpreter and it is managed by the "file" channel driver, and it is
 *	open for the requested mode, then the output parameter filePtr is set
 *	to a FILE * for the underlying file. On error, the filePtr is not set,
 *	TCL_ERROR is returned and an error message is left in the interp's
 *	result.
 *
 * Side effects:
 *	May invoke fdopen to create the FILE * for the requested file.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetOpenFile(
    Tcl_Interp *interp,		/* Interpreter in which to find file. */
    const char *chanID,		/* String that identifies file. */
    int forWriting,		/* 1 means the file is going to be used for
				 * writing, 0 means for reading. */
    int checkUsage,		/* 1 means verify that the file was opened in
				 * a mode that allows the access specified by
				 * "forWriting". Ignored, we always check that
				 * the channel is open for the requested
				 * mode. */
    ClientData *filePtr)	/* Store pointer to FILE structure here. */
{
    Tcl_Channel chan;
    int chanMode, fd;
    const Tcl_ChannelType *chanTypePtr;
    ClientData data;
    FILE *f;

    chan = Tcl_GetChannel(interp, chanID, &chanMode);
    if (chan == NULL) {
	return TCL_ERROR;
    }
    if (forWriting && !(chanMode & TCL_WRITABLE)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"\"%s\" wasn't opened for writing", chanID));
	Tcl_SetErrorCode(interp, "TCL", "VALUE", "CHANNEL", "NOT_WRITABLE",
		NULL);
	return TCL_ERROR;
    } else if (!forWriting && !(chanMode & TCL_READABLE)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"\"%s\" wasn't opened for reading", chanID));
	Tcl_SetErrorCode(interp, "TCL", "VALUE", "CHANNEL", "NOT_READABLE",
		NULL);
	return TCL_ERROR;
    }

    /*
     * We allow creating a FILE * out of file based, pipe based and socket
     * based channels. We currently do not allow any other channel types,
     * because it is likely that stdio will not know what to do with them.
     */

    chanTypePtr = Tcl_GetChannelType(chan);
    if ((chanTypePtr == &fileChannelType)
#ifdef SUPPORTS_TTY
	    || (chanTypePtr == &ttyChannelType)
#endif /* SUPPORTS_TTY */
	    || (strcmp(chanTypePtr->typeName, "tcp") == 0)
	    || (strcmp(chanTypePtr->typeName, "pipe") == 0)) {
	if (Tcl_GetChannelHandle(chan,
		(forWriting ? TCL_WRITABLE : TCL_READABLE), &data) == TCL_OK) {
	    fd = PTR2INT(data);

	    /*
	     * The call to fdopen below is probably dangerous, since it will
	     * truncate an existing file if the file is being opened for
	     * writing....
	     */

	    f = fdopen(fd, (forWriting ? "w" : "r"));
	    if (f == NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"cannot get a FILE * for \"%s\"", chanID));
		Tcl_SetErrorCode(interp, "TCL", "VALUE", "CHANNEL",
			"FILE_FAILURE", NULL);
		return TCL_ERROR;
	    }
	    *filePtr = f;
	    return TCL_OK;
	}
    }

    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "\"%s\" cannot be used to get a FILE *", chanID));
    Tcl_SetErrorCode(interp, "TCL", "VALUE", "CHANNEL", "NO_DESCRIPTOR",
	    NULL);
    return TCL_ERROR;
}

#ifndef HAVE_COREFOUNDATION	/* Darwin/Mac OS X CoreFoundation notifier is
				 * in tclMacOSXNotify.c */
/*
 *----------------------------------------------------------------------
 *
 * TclUnixWaitForFile --
 *
 *	This function waits synchronously for a file to become readable or
 *	writable, with an optional timeout.
 *
 * Results:
 *	The return value is an OR'ed combination of TCL_READABLE,
 *	TCL_WRITABLE, and TCL_EXCEPTION, indicating the conditions that are
 *	present on file at the time of the return. This function will not
 *	return until either "timeout" milliseconds have elapsed or at least
 *	one of the conditions given by mask has occurred for file (a return
 *	value of 0 means that a timeout occurred). No normal events will be
 *	serviced during the execution of this function.
 *
 * Side effects:
 *	Time passes.
 *
 *----------------------------------------------------------------------
 */

int
TclUnixWaitForFile(
    int fd,			/* Handle for file on which to wait. */
    int mask,			/* What to wait for: OR'ed combination of
				 * TCL_READABLE, TCL_WRITABLE, and
				 * TCL_EXCEPTION. */
    int timeout)		/* Maximum amount of time to wait for one of
				 * the conditions in mask to occur, in
				 * milliseconds. A value of 0 means don't wait
				 * at all, and a value of -1 means wait
				 * forever. */
{
    Tcl_Time abortTime = {0, 0}, now; /* silence gcc 4 warning */
    struct timeval blockTime, *timeoutPtr;
    int numFound, result = 0;
    fd_set readableMask;
    fd_set writableMask;
    fd_set exceptionMask;

#ifndef _DARWIN_C_SOURCE
    /*
     * Sanity check fd.
     */

    if (fd >= FD_SETSIZE) {
	Tcl_Panic("TclUnixWaitForFile can't handle file id %d", fd);
	/* must never get here, or select masks overrun will occur below */
    }
#endif

    /*
     * If there is a non-zero finite timeout, compute the time when we give
     * up.
     */

    if (timeout > 0) {
	Tcl_GetTime(&now);
	abortTime.sec = now.sec + timeout/1000;
	abortTime.usec = now.usec + (timeout%1000)*1000;
	if (abortTime.usec >= 1000000) {
	    abortTime.usec -= 1000000;
	    abortTime.sec += 1;
	}
	timeoutPtr = &blockTime;
    } else if (timeout == 0) {
	timeoutPtr = &blockTime;
	blockTime.tv_sec = 0;
	blockTime.tv_usec = 0;
    } else {
	timeoutPtr = NULL;
    }

    /*
     * Initialize the select masks.
     */

    FD_ZERO(&readableMask);
    FD_ZERO(&writableMask);
    FD_ZERO(&exceptionMask);

    /*
     * Loop in a mini-event loop of our own, waiting for either the file to
     * become ready or a timeout to occur.
     */

    while (1) {
	if (timeout > 0) {
	    blockTime.tv_sec = abortTime.sec - now.sec;
	    blockTime.tv_usec = abortTime.usec - now.usec;
	    if (blockTime.tv_usec < 0) {
		blockTime.tv_sec -= 1;
		blockTime.tv_usec += 1000000;
	    }
	    if (blockTime.tv_sec < 0) {
		blockTime.tv_sec = 0;
		blockTime.tv_usec = 0;
	    }
	}

	/*
	 * Setup the select masks for the fd.
	 */

	if (mask & TCL_READABLE) {
	    FD_SET(fd, &readableMask);
	}
	if (mask & TCL_WRITABLE) {
	    FD_SET(fd, &writableMask);
	}
	if (mask & TCL_EXCEPTION) {
	    FD_SET(fd, &exceptionMask);
	}

	/*
	 * Wait for the event or a timeout.
	 */

	numFound = select(fd + 1, &readableMask, &writableMask,
		&exceptionMask, timeoutPtr);
	if (numFound == 1) {
	    if (FD_ISSET(fd, &readableMask)) {
		SET_BITS(result, TCL_READABLE);
	    }
	    if (FD_ISSET(fd, &writableMask)) {
		SET_BITS(result, TCL_WRITABLE);
	    }
	    if (FD_ISSET(fd, &exceptionMask)) {
		SET_BITS(result, TCL_EXCEPTION);
	    }
	    result &= mask;
	    if (result) {
		break;
	    }
	}
	if (timeout == 0) {
	    break;
	}
	if (timeout < 0) {
	    continue;
	}

	/*
	 * The select returned early, so we need to recompute the timeout.
	 */

	Tcl_GetTime(&now);
	if ((abortTime.sec < now.sec)
		|| (abortTime.sec==now.sec && abortTime.usec<=now.usec)) {
	    break;
	}
    }
    return result;
}
#endif /* HAVE_COREFOUNDATION */

/*
 *----------------------------------------------------------------------
 *
 * FileTruncateProc --
 *
 *	Truncates a file to a given length.
 *
 * Results:
 *	0 if the operation succeeded, and -1 if it failed (in which case
 *	*errorCodePtr will be set to errno).
 *
 * Side effects:
 *	The underlying file is potentially truncated. This can have a wide
 *	variety of side effects, including moving file pointers that point at
 *	places later in the file than the truncate point.
 *
 *----------------------------------------------------------------------
 */

static int
FileTruncateProc(
    ClientData instanceData,
    Tcl_WideInt length)
{
    FileState *fsPtr = instanceData;
    int result;

#ifdef HAVE_TYPE_OFF64_T
    /*
     * We assume this goes with the type for now...
     */

    result = ftruncate64(fsPtr->fd, (off64_t) length);
#else
    result = ftruncate(fsPtr->fd, (off_t) length);
#endif
    if (result) {
	return errno;
    }
    return 0;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
