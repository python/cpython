/*
 * tclZlib.c --
 *
 *	This file provides the interface to the Zlib library.
 *
 * Copyright (C) 2004-2005 Pascal Scheffers <pascal@scheffers.net>
 * Copyright (C) 2005 Unitas Software B.V.
 * Copyright (c) 2008-2012 Donal K. Fellows
 *
 * Parts written by Jean-Claude Wippler, as part of Tclkit, placed in the
 * public domain March 2003.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#ifdef HAVE_ZLIB
#include <zlib.h>
#include "tclIO.h"

/*
 * The version of the zlib "package" that this implements. Note that this
 * thoroughly supersedes the versions included with tclkit, which are "1.1",
 * so this is at least "2.0" (there's no general *commitment* to have the same
 * interface, even if that is mostly true).
 */

#define TCL_ZLIB_VERSION	"2.0.1"

/*
 * Magic flags used with wbits fields to indicate that we're handling the gzip
 * format or automatic detection of format. Putting it here is slightly less
 * gross!
 */

#define WBITS_RAW		(-MAX_WBITS)
#define WBITS_ZLIB		(MAX_WBITS)
#define WBITS_GZIP		(MAX_WBITS | 16)
#define WBITS_AUTODETECT	(MAX_WBITS | 32)

/*
 * Structure used for handling gzip headers that are generated from a
 * dictionary. It comprises the header structure itself plus some working
 * space that it is very convenient to have attached.
 */

#define MAX_COMMENT_LEN		256

typedef struct {
    gz_header header;
    char nativeFilenameBuf[MAXPATHLEN];
    char nativeCommentBuf[MAX_COMMENT_LEN];
} GzipHeader;

/*
 * Structure used for the Tcl_ZlibStream* commands and [zlib stream ...]
 */

typedef struct {
    Tcl_Interp *interp;
    z_stream stream;		/* The interface to the zlib library. */
    int streamEnd;		/* If we've got to end-of-stream. */
    Tcl_Obj *inData, *outData;	/* Input / output buffers (lists) */
    Tcl_Obj *currentInput;	/* Pointer to what is currently being
				 * inflated. */
    int outPos;
    int mode;			/* Either TCL_ZLIB_STREAM_DEFLATE or
				 * TCL_ZLIB_STREAM_INFLATE. */
    int format;			/* Flags from the TCL_ZLIB_FORMAT_* */
    int level;			/* Default 5, 0-9 */
    int flush;			/* Stores the flush param for deferred the
				 * decompression. */
    int wbits;			/* The encoded compression mode, so we can
				 * restart the stream if necessary. */
    Tcl_Command cmd;		/* Token for the associated Tcl command. */
    Tcl_Obj *compDictObj;	/* Byte-array object containing compression
				 * dictionary (not dictObj!) to use if
				 * necessary. */
    int flags;			/* Miscellaneous flag bits. */
    GzipHeader *gzHeaderPtr;	/* If we've allocated a gzip header
				 * structure. */
} ZlibStreamHandle;

#define DICT_TO_SET	0x1	/* If we need to set a compression dictionary
				 * in the low-level engine at the next
				 * opportunity. */

/*
 * Macros to make it clearer in some of the twiddlier accesses what is
 * happening.
 */

#define IsRawStream(zshPtr)	((zshPtr)->format == TCL_ZLIB_FORMAT_RAW)
#define HaveDictToSet(zshPtr)	((zshPtr)->flags & DICT_TO_SET)
#define DictWasSet(zshPtr)	((zshPtr)->flags |= ~DICT_TO_SET)

/*
 * Structure used for stacked channel compression and decompression.
 */

typedef struct {
    Tcl_Channel chan;		/* Reference to the channel itself. */
    Tcl_Channel parent;		/* The underlying source and sink of bytes. */
    int flags;			/* General flag bits, see below... */
    int mode;			/* Either the value TCL_ZLIB_STREAM_DEFLATE
				 * for compression on output, or
				 * TCL_ZLIB_STREAM_INFLATE for decompression
				 * on input. */
    int format;			/* What format of data is going on the wire.
				 * Needed so that the correct [fconfigure]
				 * options can be enabled. */
    int readAheadLimit;		/* The maximum number of bytes to read from
				 * the underlying stream in one go. */
    z_stream inStream;		/* Structure used by zlib for decompression of
				 * input. */
    z_stream outStream;		/* Structure used by zlib for compression of
				 * output. */
    char *inBuffer, *outBuffer;	/* Working buffers. */
    int inAllocated, outAllocated;
				/* Sizes of working buffers. */
    GzipHeader inHeader;	/* Header read from input stream, when
				 * decompressing a gzip stream. */
    GzipHeader outHeader;	/* Header to write to an output stream, when
				 * compressing a gzip stream. */
    Tcl_TimerToken timer;	/* Timer used for keeping events fresh. */
    Tcl_DString decompressed;	/* Buffer for decompression results. */
    Tcl_Obj *compDictObj;	/* Byte-array object containing compression
				 * dictionary (not dictObj!) to use if
				 * necessary. */
} ZlibChannelData;

/*
 * Value bits for the flags field. Definitions are:
 *	ASYNC -		Whether this is an asynchronous channel.
 *	IN_HEADER -	Whether the inHeader field has been registered with
 *			the input compressor.
 *	OUT_HEADER -	Whether the outputHeader field has been registered
 *			with the output decompressor.
 */

#define ASYNC			0x1
#define IN_HEADER		0x2
#define OUT_HEADER		0x4

/*
 * Size of buffers allocated by default, and the range it can be set to.  The
 * same sorts of values apply to streams, except with different limits (they
 * permit byte-level activity). Channels always use bytes unless told to use
 * larger buffers.
 */

#define DEFAULT_BUFFER_SIZE	4096
#define MIN_NONSTREAM_BUFFER_SIZE 16
#define MAX_BUFFER_SIZE		65536

/*
 * Prototypes for private procedures defined later in this file:
 */

static Tcl_CmdDeleteProc	ZlibStreamCmdDelete;
static Tcl_DriverBlockModeProc	ZlibTransformBlockMode;
static Tcl_DriverCloseProc	ZlibTransformClose;
static Tcl_DriverGetHandleProc	ZlibTransformGetHandle;
static Tcl_DriverGetOptionProc	ZlibTransformGetOption;
static Tcl_DriverHandlerProc	ZlibTransformEventHandler;
static Tcl_DriverInputProc	ZlibTransformInput;
static Tcl_DriverOutputProc	ZlibTransformOutput;
static Tcl_DriverSetOptionProc	ZlibTransformSetOption;
static Tcl_DriverWatchProc	ZlibTransformWatch;
static Tcl_ObjCmdProc		ZlibCmd;
static Tcl_ObjCmdProc		ZlibStreamCmd;
static Tcl_ObjCmdProc		ZlibStreamAddCmd;
static Tcl_ObjCmdProc		ZlibStreamHeaderCmd;
static Tcl_ObjCmdProc		ZlibStreamPutCmd;

static void		ConvertError(Tcl_Interp *interp, int code,
			    uLong adler);
static Tcl_Obj *	ConvertErrorToList(int code, uLong adler);
static inline int	Deflate(z_streamp strm, void *bufferPtr,
			    int bufferSize, int flush, int *writtenPtr);
static void		ExtractHeader(gz_header *headerPtr, Tcl_Obj *dictObj);
static int		GenerateHeader(Tcl_Interp *interp, Tcl_Obj *dictObj,
			    GzipHeader *headerPtr, int *extraSizePtr);
static int		ZlibPushSubcmd(Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static inline int	ResultCopy(ZlibChannelData *cd, char *buf,
			    int toRead);
static int		ResultGenerate(ZlibChannelData *cd, int n, int flush,
			    int *errorCodePtr);
static Tcl_Channel	ZlibStackChannelTransform(Tcl_Interp *interp,
			    int mode, int format, int level, int limit,
			    Tcl_Channel channel, Tcl_Obj *gzipHeaderDictPtr,
			    Tcl_Obj *compDictObj);
static void		ZlibStreamCleanup(ZlibStreamHandle *zshPtr);
static int		ZlibStreamSubcmd(Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static inline void	ZlibTransformEventTimerKill(ZlibChannelData *cd);
static void		ZlibTransformTimerRun(ClientData clientData);

/*
 * Type of zlib-based compressing and decompressing channels.
 */

static const Tcl_ChannelType zlibChannelType = {
    "zlib",
    TCL_CHANNEL_VERSION_3,
    ZlibTransformClose,
    ZlibTransformInput,
    ZlibTransformOutput,
    NULL,			/* seekProc */
    ZlibTransformSetOption,
    ZlibTransformGetOption,
    ZlibTransformWatch,
    ZlibTransformGetHandle,
    NULL,			/* close2Proc */
    ZlibTransformBlockMode,
    NULL,			/* flushProc */
    ZlibTransformEventHandler,
    NULL,			/* wideSeekProc */
    NULL,
    NULL
};

/*
 *----------------------------------------------------------------------
 *
 * ConvertError --
 *
 *	Utility function for converting a zlib error into a Tcl error.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the interpreter result and errorcode.
 *
 *----------------------------------------------------------------------
 */

static void
ConvertError(
    Tcl_Interp *interp,		/* Interpreter to store the error in. May be
				 * NULL, in which case nothing happens. */
    int code,			/* The zlib error code. */
    uLong adler)		/* The checksum expected (for Z_NEED_DICT) */
{
    const char *codeStr, *codeStr2 = NULL;
    char codeStrBuf[TCL_INTEGER_SPACE];

    if (interp == NULL) {
	return;
    }

    switch (code) {
	/*
	 * Firstly, the case that is *different* because it's really coming
	 * from the OS and is just being reported via zlib. It should be
	 * really uncommon because Tcl handles all I/O rather than delegating
	 * it to zlib, but proving it can't happen is hard.
	 */

    case Z_ERRNO:
	Tcl_SetObjResult(interp, Tcl_NewStringObj(Tcl_PosixError(interp),-1));
	return;

	/*
	 * Normal errors/conditions, some of which have additional detail and
	 * some which don't. (This is not defined by array lookup because zlib
	 * error codes are sometimes negative.)
	 */

    case Z_STREAM_ERROR:
	codeStr = "STREAM";
	break;
    case Z_DATA_ERROR:
	codeStr = "DATA";
	break;
    case Z_MEM_ERROR:
	codeStr = "MEM";
	break;
    case Z_BUF_ERROR:
	codeStr = "BUF";
	break;
    case Z_VERSION_ERROR:
	codeStr = "VERSION";
	break;
    case Z_NEED_DICT:
	codeStr = "NEED_DICT";
	codeStr2 = codeStrBuf;
	sprintf(codeStrBuf, "%lu", adler);
	break;

	/*
	 * These should _not_ happen! This function is for dealing with error
	 * cases, not non-errors!
	 */

    case Z_OK:
	Tcl_Panic("unexpected zlib result in error handler: Z_OK");
    case Z_STREAM_END:
	Tcl_Panic("unexpected zlib result in error handler: Z_STREAM_END");

	/*
	 * Anything else is bad news; it's unexpected. Convert to generic
	 * error.
	 */

    default:
	codeStr = "UNKNOWN";
	codeStr2 = codeStrBuf;
	sprintf(codeStrBuf, "%d", code);
	break;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(zError(code), -1));

    /*
     * Tricky point! We might pass NULL twice here (and will when the error
     * type is known).
     */

    Tcl_SetErrorCode(interp, "TCL", "ZLIB", codeStr, codeStr2, NULL);
}

static Tcl_Obj *
ConvertErrorToList(
    int code,			/* The zlib error code. */
    uLong adler)		/* The checksum expected (for Z_NEED_DICT) */
{
    Tcl_Obj *objv[4];

    TclNewLiteralStringObj(objv[0], "TCL");
    TclNewLiteralStringObj(objv[1], "ZLIB");
    switch (code) {
    case Z_STREAM_ERROR:
	TclNewLiteralStringObj(objv[2], "STREAM");
	return Tcl_NewListObj(3, objv);
    case Z_DATA_ERROR:
	TclNewLiteralStringObj(objv[2], "DATA");
	return Tcl_NewListObj(3, objv);
    case Z_MEM_ERROR:
	TclNewLiteralStringObj(objv[2], "MEM");
	return Tcl_NewListObj(3, objv);
    case Z_BUF_ERROR:
	TclNewLiteralStringObj(objv[2], "BUF");
	return Tcl_NewListObj(3, objv);
    case Z_VERSION_ERROR:
	TclNewLiteralStringObj(objv[2], "VERSION");
	return Tcl_NewListObj(3, objv);
    case Z_ERRNO:
	TclNewLiteralStringObj(objv[2], "POSIX");
	objv[3] = Tcl_NewStringObj(Tcl_ErrnoId(), -1);
	return Tcl_NewListObj(4, objv);
    case Z_NEED_DICT:
	TclNewLiteralStringObj(objv[2], "NEED_DICT");
	objv[3] = Tcl_NewWideIntObj((Tcl_WideInt) adler);
	return Tcl_NewListObj(4, objv);

	/*
	 * These should _not_ happen! This function is for dealing with error
	 * cases, not non-errors!
	 */

    case Z_OK:
	Tcl_Panic("unexpected zlib result in error handler: Z_OK");
    case Z_STREAM_END:
	Tcl_Panic("unexpected zlib result in error handler: Z_STREAM_END");

	/*
	 * Catch-all. Should be unreachable because all cases are already
	 * listed above.
	 */

    default:
	TclNewLiteralStringObj(objv[2], "UNKNOWN");
	TclNewIntObj(objv[3], code);
	return Tcl_NewListObj(4, objv);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateHeader --
 *
 *	Function for creating a gzip header from the contents of a dictionary
 *	(as described in the documentation). GetValue is a helper function.
 *
 * Results:
 *	A Tcl result code.
 *
 * Side effects:
 *	Updates the fields of the given gz_header structure. Adds amount of
 *	extra space required for the header to the variable referenced by the
 *	extraSizePtr argument.
 *
 *----------------------------------------------------------------------
 */

static inline int
GetValue(
    Tcl_Interp *interp,
    Tcl_Obj *dictObj,
    const char *nameStr,
    Tcl_Obj **valuePtrPtr)
{
    Tcl_Obj *name = Tcl_NewStringObj(nameStr, -1);
    int result = Tcl_DictObjGet(interp, dictObj, name, valuePtrPtr);

    TclDecrRefCount(name);
    return result;
}

static int
GenerateHeader(
    Tcl_Interp *interp,		/* Where to put error messages. */
    Tcl_Obj *dictObj,		/* The dictionary whose contents are to be
				 * parsed. */
    GzipHeader *headerPtr,	/* Where to store the parsed-out values. */
    int *extraSizePtr)		/* Variable to add the length of header
				 * strings (filename, comment) to. */
{
    Tcl_Obj *value;
    int len, result = TCL_ERROR;
    const char *valueStr;
    Tcl_Encoding latin1enc;
    static const char *const types[] = {
	"binary", "text"
    };

    /*
     * RFC 1952 says that header strings are in ISO 8859-1 (LATIN-1).
     */

    latin1enc = Tcl_GetEncoding(NULL, "iso8859-1");
    if (latin1enc == NULL) {
	Tcl_Panic("no latin-1 encoding");
    }

    if (GetValue(interp, dictObj, "comment", &value) != TCL_OK) {
	goto error;
    } else if (value != NULL) {
	valueStr = Tcl_GetStringFromObj(value, &len);
	Tcl_UtfToExternal(NULL, latin1enc, valueStr, len, 0, NULL,
		headerPtr->nativeCommentBuf, MAX_COMMENT_LEN-1, NULL, &len,
		NULL);
	headerPtr->nativeCommentBuf[len] = '\0';
	headerPtr->header.comment = (Bytef *) headerPtr->nativeCommentBuf;
	if (extraSizePtr != NULL) {
	    *extraSizePtr += len;
	}
    }

    if (GetValue(interp, dictObj, "crc", &value) != TCL_OK) {
	goto error;
    } else if (value != NULL &&
	    Tcl_GetBooleanFromObj(interp, value, &headerPtr->header.hcrc)) {
	goto error;
    }

    if (GetValue(interp, dictObj, "filename", &value) != TCL_OK) {
	goto error;
    } else if (value != NULL) {
	valueStr = Tcl_GetStringFromObj(value, &len);
	Tcl_UtfToExternal(NULL, latin1enc, valueStr, len, 0, NULL,
		headerPtr->nativeFilenameBuf, MAXPATHLEN-1, NULL, &len, NULL);
	headerPtr->nativeFilenameBuf[len] = '\0';
	headerPtr->header.name = (Bytef *) headerPtr->nativeFilenameBuf;
	if (extraSizePtr != NULL) {
	    *extraSizePtr += len;
	}
    }

    if (GetValue(interp, dictObj, "os", &value) != TCL_OK) {
	goto error;
    } else if (value != NULL && Tcl_GetIntFromObj(interp, value,
	    &headerPtr->header.os) != TCL_OK) {
	goto error;
    }

    /*
     * Ignore the 'size' field, since that is controlled by the size of the
     * input data.
     */

    if (GetValue(interp, dictObj, "time", &value) != TCL_OK) {
	goto error;
    } else if (value != NULL && Tcl_GetLongFromObj(interp, value,
	    (long *) &headerPtr->header.time) != TCL_OK) {
	goto error;
    }

    if (GetValue(interp, dictObj, "type", &value) != TCL_OK) {
	goto error;
    } else if (value != NULL && Tcl_GetIndexFromObj(interp, value, types,
	    "type", TCL_EXACT, &headerPtr->header.text) != TCL_OK) {
	goto error;
    }

    result = TCL_OK;
  error:
    Tcl_FreeEncoding(latin1enc);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ExtractHeader --
 *
 *	Take the values out of a gzip header and store them in a dictionary.
 *	SetValue is a helper macro.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the dictionary, which must be writable (i.e. refCount < 2).
 *
 *----------------------------------------------------------------------
 */

#define SetValue(dictObj, key, value) \
	Tcl_DictObjPut(NULL, (dictObj), Tcl_NewStringObj((key), -1), (value))

static void
ExtractHeader(
    gz_header *headerPtr,	/* The gzip header to extract from. */
    Tcl_Obj *dictObj)		/* The dictionary to store in. */
{
    Tcl_Encoding latin1enc = NULL;
    Tcl_DString tmp;

    if (headerPtr->comment != Z_NULL) {
	if (latin1enc == NULL) {
	    /*
	     * RFC 1952 says that header strings are in ISO 8859-1 (LATIN-1).
	     */

	    latin1enc = Tcl_GetEncoding(NULL, "iso8859-1");
	    if (latin1enc == NULL) {
		Tcl_Panic("no latin-1 encoding");
	    }
	}

	Tcl_ExternalToUtfDString(latin1enc, (char *) headerPtr->comment, -1,
		&tmp);
	SetValue(dictObj, "comment", TclDStringToObj(&tmp));
    }
    SetValue(dictObj, "crc", Tcl_NewBooleanObj(headerPtr->hcrc));
    if (headerPtr->name != Z_NULL) {
	if (latin1enc == NULL) {
	    /*
	     * RFC 1952 says that header strings are in ISO 8859-1 (LATIN-1).
	     */

	    latin1enc = Tcl_GetEncoding(NULL, "iso8859-1");
	    if (latin1enc == NULL) {
		Tcl_Panic("no latin-1 encoding");
	    }
	}

	Tcl_ExternalToUtfDString(latin1enc, (char *) headerPtr->name, -1,
		&tmp);
	SetValue(dictObj, "filename", TclDStringToObj(&tmp));
    }
    if (headerPtr->os != 255) {
	SetValue(dictObj, "os", Tcl_NewIntObj(headerPtr->os));
    }
    if (headerPtr->time != 0 /* magic - no time */) {
	SetValue(dictObj, "time", Tcl_NewLongObj((long) headerPtr->time));
    }
    if (headerPtr->text != Z_UNKNOWN) {
	SetValue(dictObj, "type",
		Tcl_NewStringObj(headerPtr->text ? "text" : "binary", -1));
    }

    if (latin1enc != NULL) {
	Tcl_FreeEncoding(latin1enc);
    }
}

/*
 * Disentangle the worst of how the zlib API is used.
 */

static int
SetInflateDictionary(
    z_streamp strm,
    Tcl_Obj *compDictObj)
{
    if (compDictObj != NULL) {
	int length;
	unsigned char *bytes = Tcl_GetByteArrayFromObj(compDictObj, &length);

	return inflateSetDictionary(strm, bytes, (unsigned) length);
    }
    return Z_OK;
}

static int
SetDeflateDictionary(
    z_streamp strm,
    Tcl_Obj *compDictObj)
{
    if (compDictObj != NULL) {
	int length;
	unsigned char *bytes = Tcl_GetByteArrayFromObj(compDictObj, &length);

	return deflateSetDictionary(strm, bytes, (unsigned) length);
    }
    return Z_OK;
}

static inline int
Deflate(
    z_streamp strm,
    void *bufferPtr,
    int bufferSize,
    int flush,
    int *writtenPtr)
{
    int e;

    strm->next_out = (Bytef *) bufferPtr;
    strm->avail_out = (unsigned) bufferSize;
    e = deflate(strm, flush);
    if (writtenPtr != NULL) {
	*writtenPtr = bufferSize - strm->avail_out;
    }
    return e;
}

static inline void
AppendByteArray(
    Tcl_Obj *listObj,
    void *buffer,
    int size)
{
    if (size > 0) {
	Tcl_Obj *baObj = Tcl_NewByteArrayObj((unsigned char *) buffer, size);

	Tcl_ListObjAppendElement(NULL, listObj, baObj);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ZlibStreamInit --
 *
 *	This command initializes a (de)compression context/handle for
 *	(de)compressing data in chunks.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	The variable pointed to by zshandlePtr is initialised and memory
 *	allocated for internal state. Additionally, if interp is not null, a
 *	Tcl command is created and its name placed in the interp result obj.
 *
 * Note:
 *	At least one of interp and zshandlePtr should be non-NULL or the
 *	reference to the stream will be completely lost.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ZlibStreamInit(
    Tcl_Interp *interp,
    int mode,			/* Either TCL_ZLIB_STREAM_INFLATE or
				 * TCL_ZLIB_STREAM_DEFLATE. */
    int format,			/* Flags from the TCL_ZLIB_FORMAT_* set. */
    int level,			/* 0-9 or TCL_ZLIB_COMPRESS_DEFAULT. */
    Tcl_Obj *dictObj,		/* Dictionary containing headers for gzip. */
    Tcl_ZlibStream *zshandlePtr)
{
    int wbits = 0;
    int e;
    ZlibStreamHandle *zshPtr = NULL;
    Tcl_DString cmdname;
    GzipHeader *gzHeaderPtr = NULL;

    switch (mode) {
    case TCL_ZLIB_STREAM_DEFLATE:
	/*
	 * Compressed format is specified by the wbits parameter. See zlib.h
	 * for details.
	 */

	switch (format) {
	case TCL_ZLIB_FORMAT_RAW:
	    wbits = WBITS_RAW;
	    break;
	case TCL_ZLIB_FORMAT_GZIP:
	    wbits = WBITS_GZIP;
	    if (dictObj) {
		gzHeaderPtr = ckalloc(sizeof(GzipHeader));
		memset(gzHeaderPtr, 0, sizeof(GzipHeader));
		if (GenerateHeader(interp, dictObj, gzHeaderPtr,
			NULL) != TCL_OK) {
		    ckfree(gzHeaderPtr);
		    return TCL_ERROR;
		}
	    }
	    break;
	case TCL_ZLIB_FORMAT_ZLIB:
	    wbits = WBITS_ZLIB;
	    break;
	default:
	    Tcl_Panic("incorrect zlib data format, must be "
		    "TCL_ZLIB_FORMAT_ZLIB, TCL_ZLIB_FORMAT_GZIP or "
		    "TCL_ZLIB_FORMAT_RAW");
	}
	if (level < -1 || level > 9) {
	    Tcl_Panic("compression level should be between 0 (no compression)"
		    " and 9 (best compression) or -1 for default compression "
		    "level");
	}
	break;
    case TCL_ZLIB_STREAM_INFLATE:
	/*
	 * wbits are the same as DEFLATE, but FORMAT_AUTO is valid too.
	 */

	switch (format) {
	case TCL_ZLIB_FORMAT_RAW:
	    wbits = WBITS_RAW;
	    break;
	case TCL_ZLIB_FORMAT_GZIP:
	    wbits = WBITS_GZIP;
	    gzHeaderPtr = ckalloc(sizeof(GzipHeader));
	    memset(gzHeaderPtr, 0, sizeof(GzipHeader));
	    gzHeaderPtr->header.name = (Bytef *)
		    gzHeaderPtr->nativeFilenameBuf;
	    gzHeaderPtr->header.name_max = MAXPATHLEN - 1;
	    gzHeaderPtr->header.comment = (Bytef *)
		    gzHeaderPtr->nativeCommentBuf;
	    gzHeaderPtr->header.name_max = MAX_COMMENT_LEN - 1;
	    break;
	case TCL_ZLIB_FORMAT_ZLIB:
	    wbits = WBITS_ZLIB;
	    break;
	case TCL_ZLIB_FORMAT_AUTO:
	    wbits = WBITS_AUTODETECT;
	    break;
	default:
	    Tcl_Panic("incorrect zlib data format, must be "
		    "TCL_ZLIB_FORMAT_ZLIB, TCL_ZLIB_FORMAT_GZIP, "
		    "TCL_ZLIB_FORMAT_RAW or TCL_ZLIB_FORMAT_AUTO");
	}
	break;
    default:
	Tcl_Panic("bad mode, must be TCL_ZLIB_STREAM_DEFLATE or"
		" TCL_ZLIB_STREAM_INFLATE");
    }

    zshPtr = ckalloc(sizeof(ZlibStreamHandle));
    zshPtr->interp = interp;
    zshPtr->mode = mode;
    zshPtr->format = format;
    zshPtr->level = level;
    zshPtr->wbits = wbits;
    zshPtr->currentInput = NULL;
    zshPtr->streamEnd = 0;
    zshPtr->compDictObj = NULL;
    zshPtr->flags = 0;
    zshPtr->gzHeaderPtr = gzHeaderPtr;
    memset(&zshPtr->stream, 0, sizeof(z_stream));
    zshPtr->stream.adler = 1;

    /*
     * No output buffer available yet
     */

    if (mode == TCL_ZLIB_STREAM_DEFLATE) {
	e = deflateInit2(&zshPtr->stream, level, Z_DEFLATED, wbits,
		MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	if (e == Z_OK && zshPtr->gzHeaderPtr) {
	    e = deflateSetHeader(&zshPtr->stream,
		    &zshPtr->gzHeaderPtr->header);
	}
    } else {
	e = inflateInit2(&zshPtr->stream, wbits);
	if (e == Z_OK && zshPtr->gzHeaderPtr) {
	    e = inflateGetHeader(&zshPtr->stream,
		    &zshPtr->gzHeaderPtr->header);
	}
    }

    if (e != Z_OK) {
	ConvertError(interp, e, zshPtr->stream.adler);
	goto error;
    }

    /*
     * I could do all this in C, but this is easier.
     */

    if (interp != NULL) {
	if (Tcl_EvalEx(interp, "::incr ::tcl::zlib::cmdcounter", -1, 0) != TCL_OK) {
	    goto error;
	}
	Tcl_DStringInit(&cmdname);
	TclDStringAppendLiteral(&cmdname, "::tcl::zlib::streamcmd_");
	TclDStringAppendObj(&cmdname, Tcl_GetObjResult(interp));
	if (Tcl_FindCommand(interp, Tcl_DStringValue(&cmdname),
		NULL, 0) != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "BUG: Stream command name already exists", -1));
	    Tcl_SetErrorCode(interp, "TCL", "BUG", "EXISTING_CMD", NULL);
	    Tcl_DStringFree(&cmdname);
	    goto error;
	}
	Tcl_ResetResult(interp);

	/*
	 * Create the command.
	 */

	zshPtr->cmd = Tcl_CreateObjCommand(interp, Tcl_DStringValue(&cmdname),
		ZlibStreamCmd, zshPtr, ZlibStreamCmdDelete);
	Tcl_DStringFree(&cmdname);
	if (zshPtr->cmd == NULL) {
	    goto error;
	}
    } else {
	zshPtr->cmd = NULL;
    }

    /*
     * Prepare the buffers for use.
     */

    zshPtr->inData = Tcl_NewListObj(0, NULL);
    Tcl_IncrRefCount(zshPtr->inData);
    zshPtr->outData = Tcl_NewListObj(0, NULL);
    Tcl_IncrRefCount(zshPtr->outData);

    zshPtr->outPos = 0;

    /*
     * Now set the variable pointed to by *zshandlePtr to the pointer to the
     * zsh struct.
     */

    if (zshandlePtr) {
	*zshandlePtr = (Tcl_ZlibStream) zshPtr;
    }

    return TCL_OK;

  error:
    if (zshPtr->compDictObj) {
	Tcl_DecrRefCount(zshPtr->compDictObj);
    }
    if (zshPtr->gzHeaderPtr) {
	ckfree(zshPtr->gzHeaderPtr);
    }
    ckfree(zshPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibStreamCmdDelete --
 *
 *	This is the delete command which Tcl invokes when a zlibstream command
 *	is deleted from the interpreter (on stream close, usually).
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Invalidates the zlib stream handle as obtained from Tcl_ZlibStreamInit
 *
 *----------------------------------------------------------------------
 */

static void
ZlibStreamCmdDelete(
    ClientData cd)
{
    ZlibStreamHandle *zshPtr = cd;

    zshPtr->cmd = NULL;
    ZlibStreamCleanup(zshPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ZlibStreamClose --
 *
 *	This procedure must be called after (de)compression is done to ensure
 *	memory is freed and the command is deleted from the interpreter (if
 *	any).
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Invalidates the zlib stream handle as obtained from Tcl_ZlibStreamInit
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ZlibStreamClose(
    Tcl_ZlibStream zshandle)	/* As obtained from Tcl_ZlibStreamInit. */
{
    ZlibStreamHandle *zshPtr = (ZlibStreamHandle *) zshandle;

    /*
     * If the interp is set, deleting the command will trigger
     * ZlibStreamCleanup in ZlibStreamCmdDelete. If no interp is set, call
     * ZlibStreamCleanup directly.
     */

    if (zshPtr->interp && zshPtr->cmd) {
	Tcl_DeleteCommandFromToken(zshPtr->interp, zshPtr->cmd);
    } else {
	ZlibStreamCleanup(zshPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibStreamCleanup --
 *
 *	This procedure is called by either Tcl_ZlibStreamClose or
 *	ZlibStreamCmdDelete to cleanup the stream context.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Invalidates the zlib stream handle.
 *
 *----------------------------------------------------------------------
 */

void
ZlibStreamCleanup(
    ZlibStreamHandle *zshPtr)
{
    if (!zshPtr->streamEnd) {
	if (zshPtr->mode == TCL_ZLIB_STREAM_DEFLATE) {
	    deflateEnd(&zshPtr->stream);
	} else {
	    inflateEnd(&zshPtr->stream);
	}
    }

    if (zshPtr->inData) {
	Tcl_DecrRefCount(zshPtr->inData);
    }
    if (zshPtr->outData) {
	Tcl_DecrRefCount(zshPtr->outData);
    }
    if (zshPtr->currentInput) {
	Tcl_DecrRefCount(zshPtr->currentInput);
    }
    if (zshPtr->compDictObj) {
	Tcl_DecrRefCount(zshPtr->compDictObj);
    }
    if (zshPtr->gzHeaderPtr) {
	ckfree(zshPtr->gzHeaderPtr);
    }

    ckfree(zshPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ZlibStreamReset --
 *
 *	This procedure will reinitialize an existing stream handle.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Any data left in the (de)compression buffer is lost.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ZlibStreamReset(
    Tcl_ZlibStream zshandle)	/* As obtained from Tcl_ZlibStreamInit */
{
    ZlibStreamHandle *zshPtr = (ZlibStreamHandle *) zshandle;
    int e;

    if (!zshPtr->streamEnd) {
	if (zshPtr->mode == TCL_ZLIB_STREAM_DEFLATE) {
	    deflateEnd(&zshPtr->stream);
	} else {
	    inflateEnd(&zshPtr->stream);
	}
    }
    Tcl_SetByteArrayLength(zshPtr->inData, 0);
    Tcl_SetByteArrayLength(zshPtr->outData, 0);
    if (zshPtr->currentInput) {
	Tcl_DecrRefCount(zshPtr->currentInput);
	zshPtr->currentInput = NULL;
    }

    zshPtr->outPos = 0;
    zshPtr->streamEnd = 0;
    memset(&zshPtr->stream, 0, sizeof(z_stream));

    /*
     * No output buffer available yet.
     */

    if (zshPtr->mode == TCL_ZLIB_STREAM_DEFLATE) {
	e = deflateInit2(&zshPtr->stream, zshPtr->level, Z_DEFLATED,
		zshPtr->wbits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	if (e == Z_OK && HaveDictToSet(zshPtr)) {
	    e = SetDeflateDictionary(&zshPtr->stream, zshPtr->compDictObj);
	    if (e == Z_OK) {
		DictWasSet(zshPtr);
	    }
	}
    } else {
	e = inflateInit2(&zshPtr->stream, zshPtr->wbits);
	if (IsRawStream(zshPtr) && HaveDictToSet(zshPtr) && e == Z_OK) {
	    e = SetInflateDictionary(&zshPtr->stream, zshPtr->compDictObj);
	    if (e == Z_OK) {
		DictWasSet(zshPtr);
	    }
	}
    }

    if (e != Z_OK) {
	ConvertError(zshPtr->interp, e, zshPtr->stream.adler);
	/* TODO:cleanup */
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ZlibStreamGetCommandName --
 *
 *	This procedure will return the command name associated with the
 *	stream.
 *
 * Results:
 *	A Tcl_Obj with the name of the Tcl command or NULL if no command is
 *	associated with the stream.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_ZlibStreamGetCommandName(
    Tcl_ZlibStream zshandle)	/* As obtained from Tcl_ZlibStreamInit */
{
    ZlibStreamHandle *zshPtr = (ZlibStreamHandle *) zshandle;
    Tcl_Obj *objPtr;

    if (!zshPtr->interp) {
	return NULL;
    }

    TclNewObj(objPtr);
    Tcl_GetCommandFullName(zshPtr->interp, zshPtr->cmd, objPtr);
    return objPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ZlibStreamEof --
 *
 *	This procedure This function returns 0 or 1 depending on the state of
 *	the (de)compressor. For decompression, eof is reached when the entire
 *	compressed stream has been decompressed. For compression, eof is
 *	reached when the stream has been flushed with TCL_ZLIB_FINALIZE.
 *
 * Results:
 *	Integer.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ZlibStreamEof(
    Tcl_ZlibStream zshandle)	/* As obtained from Tcl_ZlibStreamInit */
{
    ZlibStreamHandle *zshPtr = (ZlibStreamHandle *) zshandle;

    return zshPtr->streamEnd;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ZlibStreamChecksum --
 *
 *	Return the checksum of the uncompressed data seen so far by the
 *	stream.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ZlibStreamChecksum(
    Tcl_ZlibStream zshandle)	/* As obtained from Tcl_ZlibStreamInit */
{
    ZlibStreamHandle *zshPtr = (ZlibStreamHandle *) zshandle;

    return zshPtr->stream.adler;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ZlibStreamSetCompressionDictionary --
 *
 *	Sets the compression dictionary for a stream. This will be used as
 *	appropriate for the next compression or decompression action performed
 *	on the stream.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_ZlibStreamSetCompressionDictionary(
    Tcl_ZlibStream zshandle,
    Tcl_Obj *compressionDictionaryObj)
{
    ZlibStreamHandle *zshPtr = (ZlibStreamHandle *) zshandle;

    if (compressionDictionaryObj != NULL) {
	if (Tcl_IsShared(compressionDictionaryObj)) {
	    compressionDictionaryObj =
		    Tcl_DuplicateObj(compressionDictionaryObj);
	}
	Tcl_IncrRefCount(compressionDictionaryObj);
	zshPtr->flags |= DICT_TO_SET;
    } else {
	zshPtr->flags &= ~DICT_TO_SET;
    }
    if (zshPtr->compDictObj != NULL) {
	Tcl_DecrRefCount(zshPtr->compDictObj);
    }
    zshPtr->compDictObj = compressionDictionaryObj;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ZlibStreamPut --
 *
 *	Add data to the stream for compression or decompression from a
 *	bytearray Tcl_Obj.
 *
 *----------------------------------------------------------------------
 */

#define BUFFER_SIZE_LIMIT	0xFFFF

int
Tcl_ZlibStreamPut(
    Tcl_ZlibStream zshandle,	/* As obtained from Tcl_ZlibStreamInit */
    Tcl_Obj *data,		/* Data to compress/decompress */
    int flush)			/* TCL_ZLIB_NO_FLUSH, TCL_ZLIB_FLUSH,
				 * TCL_ZLIB_FULLFLUSH, or TCL_ZLIB_FINALIZE */
{
    ZlibStreamHandle *zshPtr = (ZlibStreamHandle *) zshandle;
    char *dataTmp = NULL;
    int e, size, outSize, toStore;

    if (zshPtr->streamEnd) {
	if (zshPtr->interp) {
	    Tcl_SetObjResult(zshPtr->interp, Tcl_NewStringObj(
		    "already past compressed stream end", -1));
	    Tcl_SetErrorCode(zshPtr->interp, "TCL", "ZIP", "CLOSED", NULL);
	}
	return TCL_ERROR;
    }

    if (zshPtr->mode == TCL_ZLIB_STREAM_DEFLATE) {
	zshPtr->stream.next_in = Tcl_GetByteArrayFromObj(data, &size);
	zshPtr->stream.avail_in = size;

	/*
	 * Must not do a zero-length compress unless finalizing. [Bug 25842c161]
	 */

	if (size == 0 && flush != Z_FINISH) {
	    return TCL_OK;
	}

	if (HaveDictToSet(zshPtr)) {
	    e = SetDeflateDictionary(&zshPtr->stream, zshPtr->compDictObj);
	    if (e != Z_OK) {
		ConvertError(zshPtr->interp, e, zshPtr->stream.adler);
		return TCL_ERROR;
	    }
	    DictWasSet(zshPtr);
	}

	/*
	 * deflateBound() doesn't seem to take various header sizes into
	 * account, so we add 100 extra bytes. However, we can also loop
	 * around again so we also set an upper bound on the output buffer
	 * size.
	 */

	outSize = deflateBound(&zshPtr->stream, size) + 100;
	if (outSize > BUFFER_SIZE_LIMIT) {
	    outSize = BUFFER_SIZE_LIMIT;
	}
	dataTmp = ckalloc(outSize);

	while (1) {
	    e = Deflate(&zshPtr->stream, dataTmp, outSize, flush, &toStore);

	    /*
	     * Test if we've filled the buffer up and have to ask deflate() to
	     * give us some more. Note that the condition for needing to
	     * repeat a buffer transfer when the result is Z_OK is whether
	     * there is no more space in the buffer we provided; the zlib
	     * library does not necessarily return a different code in that
	     * case. [Bug b26e38a3e4] [Tk Bug 10f2e7872b]
	     */

	    if ((e != Z_BUF_ERROR) && (e != Z_OK || toStore < outSize)) {
		if ((e == Z_OK) || (flush == Z_FINISH && e == Z_STREAM_END)) {
		    break;
		}
		ConvertError(zshPtr->interp, e, zshPtr->stream.adler);
		return TCL_ERROR;
	    }

	    /*
	     * Output buffer too small to hold the data being generated or we
	     * are doing the end-of-stream flush (which can spit out masses of
	     * data). This means we need to put a new buffer into place after
	     * saving the old generated data to the outData list.
	     */

	    AppendByteArray(zshPtr->outData, dataTmp, outSize);

	    if (outSize < BUFFER_SIZE_LIMIT) {
		outSize = BUFFER_SIZE_LIMIT;
		/* There may be *lots* of data left to output... */
		dataTmp = ckrealloc(dataTmp, outSize);
	    }
	}

	/*
	 * And append the final data block to the outData list.
	 */

	AppendByteArray(zshPtr->outData, dataTmp, toStore);
	ckfree(dataTmp);
    } else {
	/*
	 * This is easy. Just append to the inData list.
	 */

	Tcl_ListObjAppendElement(NULL, zshPtr->inData, data);

	/*
	 * and we'll need the flush parameter for the Inflate call.
	 */

	zshPtr->flush = flush;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ZlibStreamGet --
 *
 *	Retrieve data (now compressed or decompressed) from the stream into a
 *	bytearray Tcl_Obj.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ZlibStreamGet(
    Tcl_ZlibStream zshandle,	/* As obtained from Tcl_ZlibStreamInit */
    Tcl_Obj *data,		/* A place to append the data. */
    int count)			/* Number of bytes to grab as a maximum, you
				 * may get less! */
{
    ZlibStreamHandle *zshPtr = (ZlibStreamHandle *) zshandle;
    int e, i, listLen, itemLen, dataPos = 0;
    Tcl_Obj *itemObj;
    unsigned char *dataPtr, *itemPtr;
    int existing;

    /*
     * Getting beyond the of stream, just return empty string.
     */

    if (zshPtr->streamEnd) {
	return TCL_OK;
    }

    (void) Tcl_GetByteArrayFromObj(data, &existing);

    if (zshPtr->mode == TCL_ZLIB_STREAM_INFLATE) {
	if (count == -1) {
	    /*
	     * The only safe thing to do is restict to 65k. We might cause a
	     * panic for out of memory if we just kept growing the buffer.
	     */

	    count = MAX_BUFFER_SIZE;
	}

	/*
	 * Prepare the place to store the data.
	 */

	dataPtr = Tcl_SetByteArrayLength(data, existing+count);
	dataPtr += existing;

	zshPtr->stream.next_out = dataPtr;
	zshPtr->stream.avail_out = count;
	if (zshPtr->stream.avail_in == 0) {
	    /*
	     * zlib will probably need more data to decompress.
	     */

	    if (zshPtr->currentInput) {
		Tcl_DecrRefCount(zshPtr->currentInput);
		zshPtr->currentInput = NULL;
	    }
	    Tcl_ListObjLength(NULL, zshPtr->inData, &listLen);
	    if (listLen > 0) {
		/*
		 * There is more input available, get it from the list and
		 * give it to zlib. At this point, the data must not be shared
		 * since we require the bytearray representation to not vanish
		 * under our feet. [Bug 3081008]
		 */

		Tcl_ListObjIndex(NULL, zshPtr->inData, 0, &itemObj);
		if (Tcl_IsShared(itemObj)) {
		    itemObj = Tcl_DuplicateObj(itemObj);
		}
		itemPtr = Tcl_GetByteArrayFromObj(itemObj, &itemLen);
		Tcl_IncrRefCount(itemObj);
		zshPtr->currentInput = itemObj;
		zshPtr->stream.next_in = itemPtr;
		zshPtr->stream.avail_in = itemLen;

		/*
		 * And remove it from the list
		 */

		Tcl_ListObjReplace(NULL, zshPtr->inData, 0, 1, 0, NULL);
	    }
	}

	/*
	 * When dealing with a raw stream, we set the dictionary here, once.
	 * (You can't do it in response to getting Z_NEED_DATA as raw streams
	 * don't ever issue that.)
	 */

	if (IsRawStream(zshPtr) && HaveDictToSet(zshPtr)) {
	    e = SetInflateDictionary(&zshPtr->stream, zshPtr->compDictObj);
	    if (e != Z_OK) {
		ConvertError(zshPtr->interp, e, zshPtr->stream.adler);
		return TCL_ERROR;
	    }
	    DictWasSet(zshPtr);
	}
	e = inflate(&zshPtr->stream, zshPtr->flush);
	if (e == Z_NEED_DICT && HaveDictToSet(zshPtr)) {
	    e = SetInflateDictionary(&zshPtr->stream, zshPtr->compDictObj);
	    if (e == Z_OK) {
		DictWasSet(zshPtr);
		e = inflate(&zshPtr->stream, zshPtr->flush);
	    }
	};
	Tcl_ListObjLength(NULL, zshPtr->inData, &listLen);

	while ((zshPtr->stream.avail_out > 0)
		&& (e == Z_OK || e == Z_BUF_ERROR) && (listLen > 0)) {
	    /*
	     * State: We have not satisfied the request yet and there may be
	     * more to inflate.
	     */

	    if (zshPtr->stream.avail_in > 0) {
		if (zshPtr->interp) {
		    Tcl_SetObjResult(zshPtr->interp, Tcl_NewStringObj(
			    "unexpected zlib internal state during"
			    " decompression", -1));
		    Tcl_SetErrorCode(zshPtr->interp, "TCL", "ZIP", "STATE",
			    NULL);
		}
		Tcl_SetByteArrayLength(data, existing);
		return TCL_ERROR;
	    }

	    if (zshPtr->currentInput) {
		Tcl_DecrRefCount(zshPtr->currentInput);
		zshPtr->currentInput = 0;
	    }

	    /*
	     * Get the next block of data to go to inflate. At this point, the
	     * data must not be shared since we require the bytearray
	     * representation to not vanish under our feet. [Bug 3081008]
	     */

	    Tcl_ListObjIndex(zshPtr->interp, zshPtr->inData, 0, &itemObj);
	    if (Tcl_IsShared(itemObj)) {
		itemObj = Tcl_DuplicateObj(itemObj);
	    }
	    itemPtr = Tcl_GetByteArrayFromObj(itemObj, &itemLen);
	    Tcl_IncrRefCount(itemObj);
	    zshPtr->currentInput = itemObj;
	    zshPtr->stream.next_in = itemPtr;
	    zshPtr->stream.avail_in = itemLen;

	    /*
	     * Remove it from the list.
	     */

	    Tcl_ListObjReplace(NULL, zshPtr->inData, 0, 1, 0, NULL);
	    listLen--;

	    /*
	     * And call inflate again.
	     */

	    do {
		e = inflate(&zshPtr->stream, zshPtr->flush);
		if (e != Z_NEED_DICT || !HaveDictToSet(zshPtr)) {
		    break;
		}
		e = SetInflateDictionary(&zshPtr->stream,zshPtr->compDictObj);
		DictWasSet(zshPtr);
	    } while (e == Z_OK);
	}
	if (zshPtr->stream.avail_out > 0) {
	    Tcl_SetByteArrayLength(data,
		    existing + count - zshPtr->stream.avail_out);
	}
	if (!(e==Z_OK || e==Z_STREAM_END || e==Z_BUF_ERROR)) {
	    Tcl_SetByteArrayLength(data, existing);
	    ConvertError(zshPtr->interp, e, zshPtr->stream.adler);
	    return TCL_ERROR;
	}
	if (e == Z_STREAM_END) {
	    zshPtr->streamEnd = 1;
	    if (zshPtr->currentInput) {
		Tcl_DecrRefCount(zshPtr->currentInput);
		zshPtr->currentInput = 0;
	    }
	    inflateEnd(&zshPtr->stream);
	}
    } else {
	Tcl_ListObjLength(NULL, zshPtr->outData, &listLen);
	if (count == -1) {
	    count = 0;
	    for (i=0; i<listLen; i++) {
		Tcl_ListObjIndex(NULL, zshPtr->outData, i, &itemObj);
		(void) Tcl_GetByteArrayFromObj(itemObj, &itemLen);
		if (i == 0) {
		    count += itemLen - zshPtr->outPos;
		} else {
		    count += itemLen;
		}
	    }
	}

	/*
	 * Prepare the place to store the data.
	 */

	dataPtr = Tcl_SetByteArrayLength(data, existing + count);
	dataPtr += existing;

	while ((count > dataPos) &&
		(Tcl_ListObjLength(NULL, zshPtr->outData, &listLen) == TCL_OK)
		&& (listLen > 0)) {
	    /*
	     * Get the next chunk off our list of chunks and grab the data out
	     * of it.
	     */

	    Tcl_ListObjIndex(NULL, zshPtr->outData, 0, &itemObj);
	    itemPtr = Tcl_GetByteArrayFromObj(itemObj, &itemLen);
	    if (itemLen-zshPtr->outPos >= count-dataPos) {
		unsigned len = count - dataPos;

		memcpy(dataPtr + dataPos, itemPtr + zshPtr->outPos, len);
		zshPtr->outPos += len;
		dataPos += len;
		if (zshPtr->outPos == itemLen) {
		    zshPtr->outPos = 0;
		}
	    } else {
		unsigned len = itemLen - zshPtr->outPos;

		memcpy(dataPtr + dataPos, itemPtr + zshPtr->outPos, len);
		dataPos += len;
		zshPtr->outPos = 0;
	    }
	    if (zshPtr->outPos == 0) {
		Tcl_ListObjReplace(NULL, zshPtr->outData, 0, 1, 0, NULL);
		listLen--;
	    }
	}
	Tcl_SetByteArrayLength(data, existing + dataPos);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ZlibDeflate --
 *
 *	Compress the contents of Tcl_Obj *data with compression level in
 *	output format, producing the compressed data in the interpreter
 *	result.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ZlibDeflate(
    Tcl_Interp *interp,
    int format,
    Tcl_Obj *data,
    int level,
    Tcl_Obj *gzipHeaderDictObj)
{
    int wbits = 0, inLen = 0, e = 0, extraSize = 0;
    Byte *inData = NULL;
    z_stream stream;
    GzipHeader header;
    gz_header *headerPtr = NULL;
    Tcl_Obj *obj;

    if (!interp) {
	return TCL_ERROR;
    }

    /*
     * Compressed format is specified by the wbits parameter. See zlib.h for
     * details.
     */

    if (format == TCL_ZLIB_FORMAT_RAW) {
	wbits = WBITS_RAW;
    } else if (format == TCL_ZLIB_FORMAT_GZIP) {
	wbits = WBITS_GZIP;

	/*
	 * Need to allocate extra space for the gzip header and footer. The
	 * amount of space is (a bit less than) 32 bytes, plus a byte for each
	 * byte of string that we add. Note that over-allocation is not a
	 * problem. [Bug 2419061]
	 */

	extraSize = 32;
	if (gzipHeaderDictObj) {
	    headerPtr = &header.header;
	    memset(headerPtr, 0, sizeof(gz_header));
	    if (GenerateHeader(interp, gzipHeaderDictObj, &header,
		    &extraSize) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    } else if (format == TCL_ZLIB_FORMAT_ZLIB) {
	wbits = WBITS_ZLIB;
    } else {
	Tcl_Panic("incorrect zlib data format, must be TCL_ZLIB_FORMAT_ZLIB, "
		"TCL_ZLIB_FORMAT_GZIP or TCL_ZLIB_FORMAT_ZLIB");
    }

    if (level < -1 || level > 9) {
	Tcl_Panic("compression level should be between 0 (uncompressed) and "
		"9 (best compression) or -1 for default compression level");
    }

    /*
     * Allocate some space to store the output.
     */

    TclNewObj(obj);

    /*
     * Obtain the pointer to the byte array, we'll pass this pointer straight
     * to the deflate command.
     */

    inData = Tcl_GetByteArrayFromObj(data, &inLen);
    memset(&stream, 0, sizeof(z_stream));
    stream.avail_in = (uInt) inLen;
    stream.next_in = inData;

    /*
     * No output buffer available yet, will alloc after deflateInit2.
     */

    e = deflateInit2(&stream, level, Z_DEFLATED, wbits, MAX_MEM_LEVEL,
	    Z_DEFAULT_STRATEGY);
    if (e != Z_OK) {
	goto error;
    }

    if (headerPtr != NULL) {
	e = deflateSetHeader(&stream, headerPtr);
	if (e != Z_OK) {
	    goto error;
	}
    }

    /*
     * Allocate the output buffer from the value of deflateBound(). This is
     * probably too much space. Before returning to the caller, we will reduce
     * it back to the actual compressed size.
     */

    stream.avail_out = deflateBound(&stream, inLen) + extraSize;
    stream.next_out = Tcl_SetByteArrayLength(obj, stream.avail_out);

    /*
     * Perform the compression, Z_FINISH means do it in one go.
     */

    e = deflate(&stream, Z_FINISH);

    if (e != Z_STREAM_END) {
	e = deflateEnd(&stream);

	/*
	 * deflateEnd() returns Z_OK when there are bytes left to compress, at
	 * this point we consider that an error, although we could continue by
	 * allocating more memory and calling deflate() again.
	 */

	if (e == Z_OK) {
	    e = Z_BUF_ERROR;
	}
    } else {
	e = deflateEnd(&stream);
    }

    if (e != Z_OK) {
	goto error;
    }

    /*
     * Reduce the bytearray length to the actual data length produced by
     * deflate.
     */

    Tcl_SetByteArrayLength(obj, stream.total_out);
    Tcl_SetObjResult(interp, obj);
    return TCL_OK;

  error:
    ConvertError(interp, e, stream.adler);
    TclDecrRefCount(obj);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ZlibInflate --
 *
 *	Decompress data in an object into the interpreter result.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ZlibInflate(
    Tcl_Interp *interp,
    int format,
    Tcl_Obj *data,
    int bufferSize,
    Tcl_Obj *gzipHeaderDictObj)
{
    int wbits = 0, inLen = 0, e = 0, newBufferSize;
    Byte *inData = NULL, *outData = NULL, *newOutData = NULL;
    z_stream stream;
    gz_header header, *headerPtr = NULL;
    Tcl_Obj *obj;
    char *nameBuf = NULL, *commentBuf = NULL;

    if (!interp) {
	return TCL_ERROR;
    }

    /*
     * Compressed format is specified by the wbits parameter. See zlib.h for
     * details.
     */

    switch (format) {
    case TCL_ZLIB_FORMAT_RAW:
	wbits = WBITS_RAW;
	gzipHeaderDictObj = NULL;
	break;
    case TCL_ZLIB_FORMAT_ZLIB:
	wbits = WBITS_ZLIB;
	gzipHeaderDictObj = NULL;
	break;
    case TCL_ZLIB_FORMAT_GZIP:
	wbits = WBITS_GZIP;
	break;
    case TCL_ZLIB_FORMAT_AUTO:
	wbits = WBITS_AUTODETECT;
	break;
    default:
	Tcl_Panic("incorrect zlib data format, must be TCL_ZLIB_FORMAT_ZLIB, "
		"TCL_ZLIB_FORMAT_GZIP, TCL_ZLIB_FORMAT_RAW or "
		"TCL_ZLIB_FORMAT_AUTO");
    }

    if (gzipHeaderDictObj) {
	headerPtr = &header;
	memset(headerPtr, 0, sizeof(gz_header));
	nameBuf = ckalloc(MAXPATHLEN);
	header.name = (Bytef *) nameBuf;
	header.name_max = MAXPATHLEN - 1;
	commentBuf = ckalloc(MAX_COMMENT_LEN);
	header.comment = (Bytef *) commentBuf;
	header.comm_max = MAX_COMMENT_LEN - 1;
    }

    inData = Tcl_GetByteArrayFromObj(data, &inLen);
    if (bufferSize < 1) {
	/*
	 * Start with a buffer (up to) 3 times the size of the input data.
	 */

	if (inLen < 32*1024*1024) {
	    bufferSize = 3*inLen;
	} else if (inLen < 256*1024*1024) {
	    bufferSize = 2*inLen;
	} else {
	    bufferSize = inLen;
	}
    }

    TclNewObj(obj);
    outData = Tcl_SetByteArrayLength(obj, bufferSize);
    memset(&stream, 0, sizeof(z_stream));
    stream.avail_in = (uInt) inLen+1;	/* +1 because zlib can "over-request"
					 * input (but ignore it!) */
    stream.next_in = inData;
    stream.avail_out = bufferSize;
    stream.next_out = outData;

    /*
     * Initialize zlib for decompression.
     */

    e = inflateInit2(&stream, wbits);
    if (e != Z_OK) {
	goto error;
    }
    if (headerPtr) {
	e = inflateGetHeader(&stream, headerPtr);
	if (e != Z_OK) {
	    inflateEnd(&stream);
	    goto error;
	}
    }

    /*
     * Start the decompression cycle.
     */

    while (1) {
	e = inflate(&stream, Z_FINISH);
	if (e != Z_BUF_ERROR) {
	    break;
	}

	/*
	 * Not enough room in the output buffer. Increase it by five times the
	 * bytes still in the input buffer. (Because 3 times didn't do the
	 * trick before, 5 times is what we do next.) Further optimization
	 * should be done by the user, specify the decompressed size!
	 */

	if ((stream.avail_in == 0) && (stream.avail_out > 0)) {
	    e = Z_STREAM_ERROR;
	    break;
	}
	newBufferSize = bufferSize + 5 * stream.avail_in;
	if (newBufferSize == bufferSize) {
	    newBufferSize = bufferSize+1000;
	}
	newOutData = Tcl_SetByteArrayLength(obj, newBufferSize);

	/*
	 * Set next out to the same offset in the new location.
	 */

	stream.next_out = newOutData + stream.total_out;

	/*
	 * And increase avail_out with the number of new bytes allocated.
	 */

	stream.avail_out += newBufferSize - bufferSize;
	outData = newOutData;
	bufferSize = newBufferSize;
    }

    if (e != Z_STREAM_END) {
	inflateEnd(&stream);
	goto error;
    }

    e = inflateEnd(&stream);
    if (e != Z_OK) {
	goto error;
    }

    /*
     * Reduce the BA length to the actual data length produced by deflate.
     */

    Tcl_SetByteArrayLength(obj, stream.total_out);
    if (headerPtr != NULL) {
	ExtractHeader(&header, gzipHeaderDictObj);
	SetValue(gzipHeaderDictObj, "size",
		Tcl_NewLongObj((long) stream.total_out));
	ckfree(nameBuf);
	ckfree(commentBuf);
    }
    Tcl_SetObjResult(interp, obj);
    return TCL_OK;

  error:
    TclDecrRefCount(obj);
    ConvertError(interp, e, stream.adler);
    if (nameBuf) {
	ckfree(nameBuf);
    }
    if (commentBuf) {
	ckfree(commentBuf);
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ZlibCRC32, Tcl_ZlibAdler32 --
 *
 *	Access to the checksumming engines.
 *
 *----------------------------------------------------------------------
 */

unsigned int
Tcl_ZlibCRC32(
    unsigned int crc,
    const unsigned char *buf,
    int len)
{
    /* Nothing much to do, just wrap the crc32(). */
    return crc32(crc, (Bytef *) buf, (unsigned) len);
}

unsigned int
Tcl_ZlibAdler32(
    unsigned int adler,
    const unsigned char *buf,
    int len)
{
    return adler32(adler, (Bytef *) buf, (unsigned) len);
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibCmd --
 *
 *	Implementation of the [zlib] command.
 *
 *----------------------------------------------------------------------
 */

static int
ZlibCmd(
    ClientData notUsed,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    int command, dlen, i, option, level = -1;
    unsigned start, buffersize = 0;
    Byte *data;
    Tcl_Obj *headerDictObj;
    const char *extraInfoStr = NULL;
    static const char *const commands[] = {
	"adler32", "compress", "crc32", "decompress", "deflate", "gunzip",
	"gzip", "inflate", "push", "stream",
	NULL
    };
    enum zlibCommands {
	CMD_ADLER, CMD_COMPRESS, CMD_CRC, CMD_DECOMPRESS, CMD_DEFLATE,
	CMD_GUNZIP, CMD_GZIP, CMD_INFLATE, CMD_PUSH, CMD_STREAM
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "command arg ?...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], commands, "command", 0,
	    &command) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum zlibCommands) command) {
    case CMD_ADLER:			/* adler32 str ?startvalue?
					 * -> checksum */
	if (objc < 3 || objc > 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "data ?startValue?");
	    return TCL_ERROR;
	}
	if (objc>3 && Tcl_GetIntFromObj(interp, objv[3],
		(int *) &start) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (objc < 4) {
	    start = Tcl_ZlibAdler32(0, NULL, 0);
	}
	data = Tcl_GetByteArrayFromObj(objv[2], &dlen);
	Tcl_SetObjResult(interp, Tcl_NewWideIntObj((Tcl_WideInt)
		(uLong) Tcl_ZlibAdler32(start, data, dlen)));
	return TCL_OK;
    case CMD_CRC:			/* crc32 str ?startvalue?
					 * -> checksum */
	if (objc < 3 || objc > 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "data ?startValue?");
	    return TCL_ERROR;
	}
	if (objc>3 && Tcl_GetIntFromObj(interp, objv[3],
		(int *) &start) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (objc < 4) {
	    start = Tcl_ZlibCRC32(0, NULL, 0);
	}
	data = Tcl_GetByteArrayFromObj(objv[2], &dlen);
	Tcl_SetObjResult(interp, Tcl_NewWideIntObj((Tcl_WideInt)
		(uLong) Tcl_ZlibCRC32(start, data, dlen)));
	return TCL_OK;
    case CMD_DEFLATE:			/* deflate data ?level?
					 * -> rawCompressedData */
	if (objc < 3 || objc > 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "data ?level?");
	    return TCL_ERROR;
	}
	if (objc > 3) {
	    if (Tcl_GetIntFromObj(interp, objv[3], &level) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (level < 0 || level > 9) {
		goto badLevel;
	    }
	}
	return Tcl_ZlibDeflate(interp, TCL_ZLIB_FORMAT_RAW, objv[2], level,
		NULL);
    case CMD_COMPRESS:			/* compress data ?level?
					 * -> zlibCompressedData */
	if (objc < 3 || objc > 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "data ?level?");
	    return TCL_ERROR;
	}
	if (objc > 3) {
	    if (Tcl_GetIntFromObj(interp, objv[3], &level) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (level < 0 || level > 9) {
		goto badLevel;
	    }
	}
	return Tcl_ZlibDeflate(interp, TCL_ZLIB_FORMAT_ZLIB, objv[2], level,
		NULL);
    case CMD_GZIP:			/* gzip data ?level?
					 * -> gzippedCompressedData */
	headerDictObj = NULL;

	/*
	 * Legacy argument format support.
	 */

	if (objc == 4
		&& Tcl_GetIntFromObj(interp, objv[3], &level) == TCL_OK) {
	    if (level < 0 || level > 9) {
		extraInfoStr = "\n    (in -level option)";
		goto badLevel;
	    }
	    return Tcl_ZlibDeflate(interp, TCL_ZLIB_FORMAT_GZIP, objv[2],
		    level, NULL);
	}

	if (objc < 3 || objc > 7 || ((objc & 1) == 0)) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "data ?-level level? ?-header header?");
	    return TCL_ERROR;
	}
	for (i=3 ; i<objc ; i+=2) {
	    static const char *const gzipopts[] = {
		"-header", "-level", NULL
	    };

	    if (Tcl_GetIndexFromObj(interp, objv[i], gzipopts, "option", 0,
		    &option) != TCL_OK) {
		return TCL_ERROR;
	    }
	    switch (option) {
	    case 0:
		headerDictObj = objv[i+1];
		break;
	    case 1:
		if (Tcl_GetIntFromObj(interp, objv[i+1],
			&level) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (level < 0 || level > 9) {
		    extraInfoStr = "\n    (in -level option)";
		    goto badLevel;
		}
		break;
	    }
	}
	return Tcl_ZlibDeflate(interp, TCL_ZLIB_FORMAT_GZIP, objv[2], level,
		headerDictObj);
    case CMD_INFLATE:			/* inflate rawcomprdata ?bufferSize?
					 *	-> decompressedData */
	if (objc < 3 || objc > 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "data ?bufferSize?");
	    return TCL_ERROR;
	}
	if (objc > 3) {
	    if (Tcl_GetIntFromObj(interp, objv[3],
		    (int *) &buffersize) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (buffersize < MIN_NONSTREAM_BUFFER_SIZE
		    || buffersize > MAX_BUFFER_SIZE) {
		goto badBuffer;
	    }
	}
	return Tcl_ZlibInflate(interp, TCL_ZLIB_FORMAT_RAW, objv[2],
		buffersize, NULL);
    case CMD_DECOMPRESS:		/* decompress zlibcomprdata \
					 *    ?bufferSize?
					 *	-> decompressedData */
	if (objc < 3 || objc > 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "data ?bufferSize?");
	    return TCL_ERROR;
	}
	if (objc > 3) {
	    if (Tcl_GetIntFromObj(interp, objv[3],
		    (int *) &buffersize) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (buffersize < MIN_NONSTREAM_BUFFER_SIZE
		    || buffersize > MAX_BUFFER_SIZE) {
		goto badBuffer;
	    }
	}
	return Tcl_ZlibInflate(interp, TCL_ZLIB_FORMAT_ZLIB, objv[2],
		buffersize, NULL);
    case CMD_GUNZIP: {			/* gunzip gzippeddata ?bufferSize?
					 *	-> decompressedData */
	Tcl_Obj *headerVarObj;

	if (objc < 3 || objc > 5 || ((objc & 1) == 0)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "data ?-headerVar varName?");
	    return TCL_ERROR;
	}
	headerDictObj = headerVarObj = NULL;
	for (i=3 ; i<objc ; i+=2) {
	    static const char *const gunzipopts[] = {
		"-buffersize", "-headerVar", NULL
	    };

	    if (Tcl_GetIndexFromObj(interp, objv[i], gunzipopts, "option", 0,
		    &option) != TCL_OK) {
		return TCL_ERROR;
	    }
	    switch (option) {
	    case 0:
		if (Tcl_GetIntFromObj(interp, objv[i+1],
			(int *) &buffersize) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (buffersize < MIN_NONSTREAM_BUFFER_SIZE
			|| buffersize > MAX_BUFFER_SIZE) {
		    goto badBuffer;
		}
		break;
	    case 1:
		headerVarObj = objv[i+1];
		headerDictObj = Tcl_NewObj();
		break;
	    }
	}
	if (Tcl_ZlibInflate(interp, TCL_ZLIB_FORMAT_GZIP, objv[2],
		buffersize, headerDictObj) != TCL_OK) {
	    if (headerDictObj) {
		TclDecrRefCount(headerDictObj);
	    }
	    return TCL_ERROR;
	}
	if (headerVarObj != NULL && Tcl_ObjSetVar2(interp, headerVarObj, NULL,
		headerDictObj, TCL_LEAVE_ERR_MSG) == NULL) {
	    return TCL_ERROR;
	}
	return TCL_OK;
    }
    case CMD_STREAM:			/* stream deflate/inflate/...gunzip \
					 *    ?options...?
					 *	-> handleCmd */
	return ZlibStreamSubcmd(interp, objc, objv);
    case CMD_PUSH:			/* push mode channel options...
					 *	-> channel */
	return ZlibPushSubcmd(interp, objc, objv);
    };

    return TCL_ERROR;

  badLevel:
    Tcl_SetObjResult(interp, Tcl_NewStringObj("level must be 0 to 9", -1));
    Tcl_SetErrorCode(interp, "TCL", "VALUE", "COMPRESSIONLEVEL", NULL);
    if (extraInfoStr) {
	Tcl_AddErrorInfo(interp, extraInfoStr);
    }
    return TCL_ERROR;
  badBuffer:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "buffer size must be %d to %d",
	    MIN_NONSTREAM_BUFFER_SIZE, MAX_BUFFER_SIZE));
    Tcl_SetErrorCode(interp, "TCL", "VALUE", "BUFFERSIZE", NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibStreamSubcmd --
 *
 *	Implementation of the [zlib stream] subcommand.
 *
 *----------------------------------------------------------------------
 */

static int
ZlibStreamSubcmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    static const char *const stream_formats[] = {
	"compress", "decompress", "deflate", "gunzip", "gzip", "inflate",
	NULL
    };
    enum zlibFormats {
	FMT_COMPRESS, FMT_DECOMPRESS, FMT_DEFLATE, FMT_GUNZIP, FMT_GZIP,
	FMT_INFLATE
    };
    int i, format, mode = 0, option, level;
    enum objIndices {
	OPT_COMPRESSION_DICTIONARY = 0,
	OPT_GZIP_HEADER = 1,
	OPT_COMPRESSION_LEVEL = 2,
	OPT_END = -1
    };
    Tcl_Obj *obj[3] = { NULL, NULL, NULL };
#define compDictObj	obj[OPT_COMPRESSION_DICTIONARY]
#define gzipHeaderObj	obj[OPT_GZIP_HEADER]
#define levelObj	obj[OPT_COMPRESSION_LEVEL]
    typedef struct {
	const char *name;
	enum objIndices offset;
    } OptDescriptor;
    static const OptDescriptor compressionOpts[] = {
	{ "-dictionary", OPT_COMPRESSION_DICTIONARY },
	{ "-level",	 OPT_COMPRESSION_LEVEL },
	{ NULL, OPT_END }
    };
    static const OptDescriptor gzipOpts[] = {
	{ "-header",	 OPT_GZIP_HEADER },
	{ "-level",	 OPT_COMPRESSION_LEVEL },
	{ NULL, OPT_END }
    };
    static const OptDescriptor expansionOpts[] = {
	{ "-dictionary", OPT_COMPRESSION_DICTIONARY },
	{ NULL, OPT_END }
    };
    static const OptDescriptor gunzipOpts[] = {
	{ NULL, OPT_END }
    };
    const OptDescriptor *desc = NULL;
    Tcl_ZlibStream zh;

    if (objc < 3 || !(objc & 1)) {
	Tcl_WrongNumArgs(interp, 2, objv, "mode ?-option value...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[2], stream_formats, "mode", 0,
	    &format) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * The format determines the compression mode and the options that may be
     * specified.
     */

    switch ((enum zlibFormats) format) {
    case FMT_DEFLATE:
	desc = compressionOpts;
	mode = TCL_ZLIB_STREAM_DEFLATE;
	format = TCL_ZLIB_FORMAT_RAW;
	break;
    case FMT_INFLATE:
	desc = expansionOpts;
	mode = TCL_ZLIB_STREAM_INFLATE;
	format = TCL_ZLIB_FORMAT_RAW;
	break;
    case FMT_COMPRESS:
	desc = compressionOpts;
	mode = TCL_ZLIB_STREAM_DEFLATE;
	format = TCL_ZLIB_FORMAT_ZLIB;
	break;
    case FMT_DECOMPRESS:
	desc = expansionOpts;
	mode = TCL_ZLIB_STREAM_INFLATE;
	format = TCL_ZLIB_FORMAT_ZLIB;
	break;
    case FMT_GZIP:
	desc = gzipOpts;
	mode = TCL_ZLIB_STREAM_DEFLATE;
	format = TCL_ZLIB_FORMAT_GZIP;
	break;
    case FMT_GUNZIP:
	desc = gunzipOpts;
	mode = TCL_ZLIB_STREAM_INFLATE;
	format = TCL_ZLIB_FORMAT_GZIP;
	break;
    default:
	Tcl_Panic("should be unreachable");
    }

    /*
     * Parse the options.
     */

    for (i=3 ; i<objc ; i+=2) {
	if (Tcl_GetIndexFromObjStruct(interp, objv[i], desc,
		sizeof(OptDescriptor), "option", 0, &option) != TCL_OK) {
	    return TCL_ERROR;
	}
	obj[desc[option].offset] = objv[i+1];
    }

    /*
     * If a compression level was given, parse it (integral: 0..9). Otherwise
     * use the default.
     */

    if (levelObj == NULL) {
	level = Z_DEFAULT_COMPRESSION;
    } else if (Tcl_GetIntFromObj(interp, levelObj, &level) != TCL_OK) {
	return TCL_ERROR;
    } else if (level < 0 || level > 9) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("level must be 0 to 9",-1));
	Tcl_SetErrorCode(interp, "TCL", "VALUE", "COMPRESSIONLEVEL", NULL);
	Tcl_AddErrorInfo(interp, "\n    (in -level option)");
	return TCL_ERROR;
    }

    /*
     * Construct the stream now we know its configuration.
     */

    if (Tcl_ZlibStreamInit(interp, mode, format, level, gzipHeaderObj,
	    &zh) != TCL_OK) {
	return TCL_ERROR;
    }
    if (compDictObj != NULL) {
	Tcl_ZlibStreamSetCompressionDictionary(zh, compDictObj);
    }
    Tcl_SetObjResult(interp, Tcl_ZlibStreamGetCommandName(zh));
    return TCL_OK;
#undef compDictObj
#undef gzipHeaderObj
#undef levelObj
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibPushSubcmd --
 *
 *	Implementation of the [zlib push] subcommand.
 *
 *----------------------------------------------------------------------
 */

static int
ZlibPushSubcmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    static const char *const stream_formats[] = {
	"compress", "decompress", "deflate", "gunzip", "gzip", "inflate",
	NULL
    };
    enum zlibFormats {
	FMT_COMPRESS, FMT_DECOMPRESS, FMT_DEFLATE, FMT_GUNZIP, FMT_GZIP,
	FMT_INFLATE
    };
    Tcl_Channel chan;
    int chanMode, format, mode = 0, level, i, option;
    static const char *const pushCompressOptions[] = {
	"-dictionary", "-header", "-level", NULL
    };
    static const char *const pushDecompressOptions[] = {
	"-dictionary", "-header", "-level", "-limit", NULL
    };
    const char *const *pushOptions = pushDecompressOptions;
    enum pushOptions {poDictionary, poHeader, poLevel, poLimit};
    Tcl_Obj *headerObj = NULL, *compDictObj = NULL;
    int limit = 1, dummy;

    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "mode channel ?options...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], stream_formats, "mode", 0,
	    &format) != TCL_OK) {
	return TCL_ERROR;
    }
    switch ((enum zlibFormats) format) {
    case FMT_DEFLATE:
	mode = TCL_ZLIB_STREAM_DEFLATE;
	format = TCL_ZLIB_FORMAT_RAW;
	pushOptions = pushCompressOptions;
	break;
    case FMT_INFLATE:
	mode = TCL_ZLIB_STREAM_INFLATE;
	format = TCL_ZLIB_FORMAT_RAW;
	break;
    case FMT_COMPRESS:
	mode = TCL_ZLIB_STREAM_DEFLATE;
	format = TCL_ZLIB_FORMAT_ZLIB;
	pushOptions = pushCompressOptions;
	break;
    case FMT_DECOMPRESS:
	mode = TCL_ZLIB_STREAM_INFLATE;
	format = TCL_ZLIB_FORMAT_ZLIB;
	break;
    case FMT_GZIP:
	mode = TCL_ZLIB_STREAM_DEFLATE;
	format = TCL_ZLIB_FORMAT_GZIP;
	pushOptions = pushCompressOptions;
	break;
    case FMT_GUNZIP:
	mode = TCL_ZLIB_STREAM_INFLATE;
	format = TCL_ZLIB_FORMAT_GZIP;
	break;
    default:
	Tcl_Panic("should be unreachable");
    }

    if (TclGetChannelFromObj(interp, objv[3], &chan, &chanMode, 0) != TCL_OK){
	return TCL_ERROR;
    }

    /*
     * Sanity checks.
     */

    if (mode == TCL_ZLIB_STREAM_DEFLATE && !(chanMode & TCL_WRITABLE)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"compression may only be applied to writable channels", -1));
	Tcl_SetErrorCode(interp, "TCL", "ZIP", "UNWRITABLE", NULL);
	return TCL_ERROR;
    }
    if (mode == TCL_ZLIB_STREAM_INFLATE && !(chanMode & TCL_READABLE)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"decompression may only be applied to readable channels",-1));
	Tcl_SetErrorCode(interp, "TCL", "ZIP", "UNREADABLE", NULL);
	return TCL_ERROR;
    }

    /*
     * Parse options.
     */

    level = Z_DEFAULT_COMPRESSION;
    for (i=4 ; i<objc ; i++) {
	if (Tcl_GetIndexFromObj(interp, objv[i], pushOptions, "option", 0,
		&option) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (++i > objc-1) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "value missing for %s option", pushOptions[option]));
	    Tcl_SetErrorCode(interp, "TCL", "ZIP", "NOVAL", NULL);
	    return TCL_ERROR;
	}
	switch ((enum pushOptions) option) {
	case poHeader:
	    headerObj = objv[i];
	    if (Tcl_DictObjSize(interp, headerObj, &dummy) != TCL_OK) {
		goto genericOptionError;
	    }
	    break;
	case poLevel:
	    if (Tcl_GetIntFromObj(interp, objv[i], (int*) &level) != TCL_OK) {
		goto genericOptionError;
	    }
	    if (level < 0 || level > 9) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"level must be 0 to 9", -1));
		Tcl_SetErrorCode(interp, "TCL", "VALUE", "COMPRESSIONLEVEL",
			NULL);
		goto genericOptionError;
	    }
	    break;
	case poLimit:
	    if (Tcl_GetIntFromObj(interp, objv[i], (int*) &limit) != TCL_OK) {
		goto genericOptionError;
	    }
	    if (limit < 1 || limit > MAX_BUFFER_SIZE) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"read ahead limit must be 1 to %d",
			MAX_BUFFER_SIZE));
		Tcl_SetErrorCode(interp, "TCL", "VALUE", "BUFFERSIZE", NULL);
		goto genericOptionError;
	    }
	    break;
	case poDictionary:
	    if (format == TCL_ZLIB_FORMAT_GZIP) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"a compression dictionary may not be set in the "
			"gzip format", -1));
		Tcl_SetErrorCode(interp, "TCL", "ZIP", "BADOPT", NULL);
		goto genericOptionError;
	    }
	    compDictObj = objv[i];
	    break;
	}
    }

    if (ZlibStackChannelTransform(interp, mode, format, level, limit, chan,
	    headerObj, compDictObj) == NULL) {
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;

  genericOptionError:
    Tcl_AddErrorInfo(interp, "\n    (in ");
    Tcl_AddErrorInfo(interp, pushOptions[option]);
    Tcl_AddErrorInfo(interp, " option)");
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibStreamCmd --
 *
 *	Implementation of the commands returned by [zlib stream].
 *
 *----------------------------------------------------------------------
 */

static int
ZlibStreamCmd(
    ClientData cd,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Tcl_ZlibStream zstream = cd;
    int command, count, code;
    Tcl_Obj *obj;
    static const char *const cmds[] = {
	"add", "checksum", "close", "eof", "finalize", "flush",
	"fullflush", "get", "header", "put", "reset",
	NULL
    };
    enum zlibStreamCommands {
	zs_add, zs_checksum, zs_close, zs_eof, zs_finalize, zs_flush,
	zs_fullflush, zs_get, zs_header, zs_put, zs_reset
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option data ?...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], cmds, "option", 0,
	    &command) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum zlibStreamCommands) command) {
    case zs_add:		/* $strm add ?$flushopt? $data */
	return ZlibStreamAddCmd(zstream, interp, objc, objv);
    case zs_header:		/* $strm header */
	return ZlibStreamHeaderCmd(zstream, interp, objc, objv);
    case zs_put:		/* $strm put ?$flushopt? $data */
	return ZlibStreamPutCmd(zstream, interp, objc, objv);

    case zs_get:		/* $strm get ?count? */
	if (objc > 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?count?");
	    return TCL_ERROR;
	}

	count = -1;
	if (objc >= 3) {
	    if (Tcl_GetIntFromObj(interp, objv[2], &count) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	TclNewObj(obj);
	code = Tcl_ZlibStreamGet(zstream, obj, count);
	if (code == TCL_OK) {
	    Tcl_SetObjResult(interp, obj);
	} else {
	    TclDecrRefCount(obj);
	}
	return code;
    case zs_flush:		/* $strm flush */
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	TclNewObj(obj);
	Tcl_IncrRefCount(obj);
	code = Tcl_ZlibStreamPut(zstream, obj, Z_SYNC_FLUSH);
	TclDecrRefCount(obj);
	return code;
    case zs_fullflush:		/* $strm fullflush */
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	TclNewObj(obj);
	Tcl_IncrRefCount(obj);
	code = Tcl_ZlibStreamPut(zstream, obj, Z_FULL_FLUSH);
	TclDecrRefCount(obj);
	return code;
    case zs_finalize:		/* $strm finalize */
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}

	/*
	 * The flush commands slightly abuse the empty result obj as input
	 * data.
	 */

	TclNewObj(obj);
	Tcl_IncrRefCount(obj);
	code = Tcl_ZlibStreamPut(zstream, obj, Z_FINISH);
	TclDecrRefCount(obj);
	return code;
    case zs_close:		/* $strm close */
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	return Tcl_ZlibStreamClose(zstream);
    case zs_eof:		/* $strm eof */
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(Tcl_ZlibStreamEof(zstream)));
	return TCL_OK;
    case zs_checksum:		/* $strm checksum */
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewWideIntObj((Tcl_WideInt)
		(uLong) Tcl_ZlibStreamChecksum(zstream)));
	return TCL_OK;
    case zs_reset:		/* $strm reset */
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	return Tcl_ZlibStreamReset(zstream);
    }

    return TCL_OK;
}

static int
ZlibStreamAddCmd(
    ClientData cd,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Tcl_ZlibStream zstream = cd;
    int index, code, buffersize = -1, flush = -1, i;
    Tcl_Obj *obj, *compDictObj = NULL;
    static const char *const add_options[] = {
	"-buffer", "-dictionary", "-finalize", "-flush", "-fullflush", NULL
    };
    enum addOptions {
	ao_buffer, ao_dictionary, ao_finalize, ao_flush, ao_fullflush
    };

    for (i=2; i<objc-1; i++) {
	if (Tcl_GetIndexFromObj(interp, objv[i], add_options, "option", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}

	switch ((enum addOptions) index) {
	case ao_flush: /* -flush */
	    if (flush > -1) {
		flush = -2;
	    } else {
		flush = Z_SYNC_FLUSH;
	    }
	    break;
	case ao_fullflush: /* -fullflush */
	    if (flush > -1) {
		flush = -2;
	    } else {
		flush = Z_FULL_FLUSH;
	    }
	    break;
	case ao_finalize: /* -finalize */
	    if (flush > -1) {
		flush = -2;
	    } else {
		flush = Z_FINISH;
	    }
	    break;
	case ao_buffer: /* -buffer */
	    if (i == objc-2) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"\"-buffer\" option must be followed by integer "
			"decompression buffersize", -1));
		Tcl_SetErrorCode(interp, "TCL", "ZIP", "NOVAL", NULL);
		return TCL_ERROR;
	    }
	    if (Tcl_GetIntFromObj(interp, objv[++i], &buffersize) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (buffersize < 1 || buffersize > MAX_BUFFER_SIZE) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"buffer size must be 1 to %d",
			MAX_BUFFER_SIZE));
		Tcl_SetErrorCode(interp, "TCL", "VALUE", "BUFFERSIZE", NULL);
		return TCL_ERROR;
	    }
	    break;
	case ao_dictionary:
	    if (i == objc-2) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"\"-dictionary\" option must be followed by"
			" compression dictionary bytes", -1));
		Tcl_SetErrorCode(interp, "TCL", "ZIP", "NOVAL", NULL);
		return TCL_ERROR;
	    }
	    compDictObj = objv[++i];
	    break;
	}

	if (flush == -2) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "\"-flush\", \"-fullflush\" and \"-finalize\" options"
		    " are mutually exclusive", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ZIP", "EXCLUSIVE", NULL);
	    return TCL_ERROR;
	}
    }
    if (flush == -1) {
	flush = 0;
    }

    /*
     * Set the compression dictionary if requested.
     */

    if (compDictObj != NULL) {
	int len;

	(void) Tcl_GetByteArrayFromObj(compDictObj, &len);
	if (len == 0) {
	    compDictObj = NULL;
	}
	Tcl_ZlibStreamSetCompressionDictionary(zstream, compDictObj);
    }

    /*
     * Send the data to the stream core, along with any flushing directive.
     */

    if (Tcl_ZlibStreamPut(zstream, objv[objc-1], flush) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Get such data out as we can (up to the requested length).
     */

    TclNewObj(obj);
    code = Tcl_ZlibStreamGet(zstream, obj, buffersize);
    if (code == TCL_OK) {
	Tcl_SetObjResult(interp, obj);
    } else {
	TclDecrRefCount(obj);
    }
    return code;
}

static int
ZlibStreamPutCmd(
    ClientData cd,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Tcl_ZlibStream zstream = cd;
    int index, flush = -1, i;
    Tcl_Obj *compDictObj = NULL;
    static const char *const put_options[] = {
	"-dictionary", "-finalize", "-flush", "-fullflush", NULL
    };
    enum putOptions {
	po_dictionary, po_finalize, po_flush, po_fullflush
    };

    for (i=2; i<objc-1; i++) {
	if (Tcl_GetIndexFromObj(interp, objv[i], put_options, "option", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}

	switch ((enum putOptions) index) {
	case po_flush: /* -flush */
	    if (flush > -1) {
		flush = -2;
	    } else {
		flush = Z_SYNC_FLUSH;
	    }
	    break;
	case po_fullflush: /* -fullflush */
	    if (flush > -1) {
		flush = -2;
	    } else {
		flush = Z_FULL_FLUSH;
	    }
	    break;
	case po_finalize: /* -finalize */
	    if (flush > -1) {
		flush = -2;
	    } else {
		flush = Z_FINISH;
	    }
	    break;
	case po_dictionary:
	    if (i == objc-2) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"\"-dictionary\" option must be followed by"
			" compression dictionary bytes", -1));
		Tcl_SetErrorCode(interp, "TCL", "ZIP", "NOVAL", NULL);
		return TCL_ERROR;
	    }
	    compDictObj = objv[++i];
	    break;
	}
	if (flush == -2) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "\"-flush\", \"-fullflush\" and \"-finalize\" options"
		    " are mutually exclusive", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ZIP", "EXCLUSIVE", NULL);
	    return TCL_ERROR;
	}
    }
    if (flush == -1) {
	flush = 0;
    }

    /*
     * Set the compression dictionary if requested.
     */

    if (compDictObj != NULL) {
	int len;

	(void) Tcl_GetByteArrayFromObj(compDictObj, &len);
	if (len == 0) {
	    compDictObj = NULL;
	}
	Tcl_ZlibStreamSetCompressionDictionary(zstream, compDictObj);
    }

    /*
     * Send the data to the stream core, along with any flushing directive.
     */

    return Tcl_ZlibStreamPut(zstream, objv[objc-1], flush);
}

static int
ZlibStreamHeaderCmd(
    ClientData cd,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    ZlibStreamHandle *zshPtr = cd;
    Tcl_Obj *resultObj;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 2, objv, NULL);
	return TCL_ERROR;
    } else if (zshPtr->mode != TCL_ZLIB_STREAM_INFLATE
	    || zshPtr->format != TCL_ZLIB_FORMAT_GZIP) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"only gunzip streams can produce header information", -1));
	Tcl_SetErrorCode(interp, "TCL", "ZIP", "BADOP", NULL);
	return TCL_ERROR;
    }

    TclNewObj(resultObj);
    ExtractHeader(&zshPtr->gzHeaderPtr->header, resultObj);
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *	Set of functions to support channel stacking.
 *----------------------------------------------------------------------
 *
 * ZlibTransformClose --
 *
 *	How to shut down a stacked compressing/decompressing transform.
 *
 *----------------------------------------------------------------------
 */

static int
ZlibTransformClose(
    ClientData instanceData,
    Tcl_Interp *interp)
{
    ZlibChannelData *cd = instanceData;
    int e, written, result = TCL_OK;

    /*
     * Delete the support timer.
     */

    ZlibTransformEventTimerKill(cd);

    /*
     * Flush any data waiting to be compressed.
     */

    if (cd->mode == TCL_ZLIB_STREAM_DEFLATE) {
	cd->outStream.avail_in = 0;
	do {
	    e = Deflate(&cd->outStream, cd->outBuffer, cd->outAllocated,
		    Z_FINISH, &written);

	    /*
	     * Can't be sure that deflate() won't declare the buffer to be
	     * full (with Z_BUF_ERROR) so handle that case.
	     */

	    if (e == Z_BUF_ERROR) {
		e = Z_OK;
		written = cd->outAllocated;
	    }
	    if (e != Z_OK && e != Z_STREAM_END) {
		/* TODO: is this the right way to do errors on close? */
		if (!TclInThreadExit()) {
		    ConvertError(interp, e, cd->outStream.adler);
		}
		result = TCL_ERROR;
		break;
	    }
	    if (written && Tcl_WriteRaw(cd->parent, cd->outBuffer, written) < 0) {
		/* TODO: is this the right way to do errors on close?
		 * Note: when close is called from FinalizeIOSubsystem then
		 * interp may be NULL */
		if (!TclInThreadExit() && interp) {
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "error while finalizing file: %s",
			    Tcl_PosixError(interp)));
		}
		result = TCL_ERROR;
		break;
	    }
	} while (e != Z_STREAM_END);
	(void) deflateEnd(&cd->outStream);
    } else {
	(void) inflateEnd(&cd->inStream);
    }

    /*
     * Release all memory.
     */

    if (cd->compDictObj) {
	Tcl_DecrRefCount(cd->compDictObj);
	cd->compDictObj = NULL;
    }
    Tcl_DStringFree(&cd->decompressed);

    if (cd->inBuffer) {
	ckfree(cd->inBuffer);
	cd->inBuffer = NULL;
    }
    if (cd->outBuffer) {
	ckfree(cd->outBuffer);
	cd->outBuffer = NULL;
    }
    ckfree(cd);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibTransformInput --
 *
 *	Reader filter that does decompression.
 *
 *----------------------------------------------------------------------
 */

static int
ZlibTransformInput(
    ClientData instanceData,
    char *buf,
    int toRead,
    int *errorCodePtr)
{
    ZlibChannelData *cd = instanceData;
    Tcl_DriverInputProc *inProc =
	    Tcl_ChannelInputProc(Tcl_GetChannelType(cd->parent));
    int readBytes, gotBytes, copied;

    if (cd->mode == TCL_ZLIB_STREAM_DEFLATE) {
	return inProc(Tcl_GetChannelInstanceData(cd->parent), buf, toRead,
		errorCodePtr);
    }

    gotBytes = 0;
    while (toRead > 0) {
	/*
	 * Loop until the request is satisfied (or no data available from
	 * below, possibly EOF).
	 */

	copied = ResultCopy(cd, buf, toRead);
	toRead -= copied;
	buf += copied;
	gotBytes += copied;

	if (toRead == 0) {
	    return gotBytes;
	}

	/*
	 * The buffer is exhausted, but the caller wants even more. We now
	 * have to go to the underlying channel, get more bytes and then
	 * transform them for delivery. We may not get what we want (full EOF
	 * or temporarily out of data).
	 *
	 * Length (cd->decompressed) == 0, toRead > 0 here.
	 *
	 * The zlib transform allows us to read at most one character from the
	 * underlying channel to properly identify Z_STREAM_END without
	 * reading over the border.
	 */

	readBytes = Tcl_ReadRaw(cd->parent, cd->inBuffer, cd->readAheadLimit);

	/*
	 * Three cases here:
	 *  1.	Got some data from the underlying channel (readBytes > 0) so
	 *	it should be fed through the decompression engine.
	 *  2.	Got an error (readBytes < 0) which we should report up except
	 *	for the case where we can convert it to a short read.
	 *  3.	Got an end-of-data from EOF or blocking (readBytes == 0). If
	 *	it is EOF, try flushing the data out of the decompressor.
	 */

	if (readBytes < 0) {

	    /* See ReflectInput() in tclIORTrans.c */
	    if (Tcl_InputBlocked(cd->parent) && (gotBytes > 0)) {
		return gotBytes;
	    }

	    *errorCodePtr = Tcl_GetErrno();
	    return -1;
	}
	if (readBytes == 0) {
	    /*
	     * Eof in parent.
	     *
	     * Now this is a bit different. The partial data waiting is
	     * converted and returned.
	     */

	    if (ResultGenerate(cd, 0, Z_SYNC_FLUSH, errorCodePtr) != TCL_OK) {
		return -1;
	    }

	    if (Tcl_DStringLength(&cd->decompressed) == 0) {
		/*
		 * The drain delivered nothing. Time to deliver what we've
		 * got.
		 */

		return gotBytes;
	    }
	} else /* readBytes > 0 */ {
	    /*
	     * Transform the read chunk, which was not empty. Anything we get
	     * back is a transformation result to be put into our buffers, and
	     * the next iteration will put it into the result.
	     */

	    if (ResultGenerate(cd, readBytes, Z_NO_FLUSH,
		    errorCodePtr) != TCL_OK) {
		return -1;
	    }
	}
    }
    return gotBytes;
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibTransformOutput --
 *
 *	Writer filter that does compression.
 *
 *----------------------------------------------------------------------
 */

static int
ZlibTransformOutput(
    ClientData instanceData,
    const char *buf,
    int toWrite,
    int *errorCodePtr)
{
    ZlibChannelData *cd = instanceData;
    Tcl_DriverOutputProc *outProc =
	    Tcl_ChannelOutputProc(Tcl_GetChannelType(cd->parent));
    int e, produced;
    Tcl_Obj *errObj;

    if (cd->mode == TCL_ZLIB_STREAM_INFLATE) {
	return outProc(Tcl_GetChannelInstanceData(cd->parent), buf, toWrite,
		errorCodePtr);
    }

    /*
     * No zero-length writes. Flushes must be explicit.
     */

    if (toWrite == 0) {
	return 0;
    }

    cd->outStream.next_in = (Bytef *) buf;
    cd->outStream.avail_in = toWrite;
    while (cd->outStream.avail_in > 0) {
	e = Deflate(&cd->outStream, cd->outBuffer, cd->outAllocated,
		Z_NO_FLUSH, &produced);
	if (e != Z_OK || produced == 0) {
	    break;
	}

	if (Tcl_WriteRaw(cd->parent, cd->outBuffer, produced) < 0) {
	    *errorCodePtr = Tcl_GetErrno();
	    return -1;
	}
    }

    if (e == Z_OK) {
	return toWrite - cd->outStream.avail_in;
    }

    errObj = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(NULL, errObj, Tcl_NewStringObj("-errorcode",-1));
    Tcl_ListObjAppendElement(NULL, errObj,
	    ConvertErrorToList(e, cd->outStream.adler));
    Tcl_ListObjAppendElement(NULL, errObj,
	    Tcl_NewStringObj(cd->outStream.msg, -1));
    Tcl_SetChannelError(cd->parent, errObj);
    *errorCodePtr = EINVAL;
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibTransformFlush --
 *
 *	How to perform a flush of a compressing transform.
 *
 *----------------------------------------------------------------------
 */

static int
ZlibTransformFlush(
    Tcl_Interp *interp,
    ZlibChannelData *cd,
    int flushType)
{
    int e, len;

    cd->outStream.avail_in = 0;
    do {
	/*
	 * Get the bytes to go out of the compression engine.
	 */

	e = Deflate(&cd->outStream, cd->outBuffer, cd->outAllocated,
		flushType, &len);
	if (e != Z_OK && e != Z_BUF_ERROR) {
	    ConvertError(interp, e, cd->outStream.adler);
	    return TCL_ERROR;
	}

	/*
	 * Write the bytes we've received to the next layer.
	 */

	if (len > 0 && Tcl_WriteRaw(cd->parent, cd->outBuffer, len) < 0) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "problem flushing channel: %s",
		    Tcl_PosixError(interp)));
	    return TCL_ERROR;
	}

	/*
	 * If we get to this point, either we're in the Z_OK or the
	 * Z_BUF_ERROR state. In the former case, we're done. In the latter
	 * case, it's because there's more bytes to go than would fit in the
	 * buffer we provided, and we need to go round again to get some more.
	 *
	 * We also stop the loop if we would have done a zero-length write.
	 * Those can cause problems at the OS level.
	 */
    } while (len > 0 && e == Z_BUF_ERROR);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibTransformSetOption --
 *
 *	Writing side of [fconfigure] on our channel.
 *
 *----------------------------------------------------------------------
 */

static int
ZlibTransformSetOption(			/* not used */
    ClientData instanceData,
    Tcl_Interp *interp,
    const char *optionName,
    const char *value)
{
    ZlibChannelData *cd = instanceData;
    Tcl_DriverSetOptionProc *setOptionProc =
	    Tcl_ChannelSetOptionProc(Tcl_GetChannelType(cd->parent));
    static const char *compressChanOptions = "dictionary flush";
    static const char *gzipChanOptions = "flush";
    static const char *decompressChanOptions = "dictionary limit";
    static const char *gunzipChanOptions = "flush limit";
    int haveFlushOpt = (cd->mode == TCL_ZLIB_STREAM_DEFLATE);

    if (optionName && (strcmp(optionName, "-dictionary") == 0)
	    && (cd->format != TCL_ZLIB_FORMAT_GZIP)) {
	Tcl_Obj *compDictObj;
	int code;

	TclNewStringObj(compDictObj, value, strlen(value));
	Tcl_IncrRefCount(compDictObj);
	(void) Tcl_GetByteArrayFromObj(compDictObj, NULL);
	if (cd->compDictObj) {
	    TclDecrRefCount(cd->compDictObj);
	}
	cd->compDictObj = compDictObj;
	code = Z_OK;
	if (cd->mode == TCL_ZLIB_STREAM_DEFLATE) {
	    code = SetDeflateDictionary(&cd->outStream, compDictObj);
	    if (code != Z_OK) {
		ConvertError(interp, code, cd->outStream.adler);
		return TCL_ERROR;
	    }
	} else if (cd->format == TCL_ZLIB_FORMAT_RAW) {
	    code = SetInflateDictionary(&cd->inStream, compDictObj);
	    if (code != Z_OK) {
		ConvertError(interp, code, cd->inStream.adler);
		return TCL_ERROR;
	    }
	}
	return TCL_OK;
    }

    if (haveFlushOpt) {
	if (optionName && strcmp(optionName, "-flush") == 0) {
	    int flushType;

	    if (value[0] == 'f' && strcmp(value, "full") == 0) {
		flushType = Z_FULL_FLUSH;
	    } else if (value[0] == 's' && strcmp(value, "sync") == 0) {
		flushType = Z_SYNC_FLUSH;
	    } else {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"unknown -flush type \"%s\": must be full or sync",
			value));
		Tcl_SetErrorCode(interp, "TCL", "VALUE", "FLUSH", NULL);
		return TCL_ERROR;
	    }

	    /*
	     * Try to actually do the flush now.
	     */

	    return ZlibTransformFlush(interp, cd, flushType);
	}
    } else {
	if (optionName && strcmp(optionName, "-limit") == 0) {
	    int newLimit;

	    if (Tcl_GetInt(interp, value, &newLimit) != TCL_OK) {
		return TCL_ERROR;
	    } else if (newLimit < 1 || newLimit > MAX_BUFFER_SIZE) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"-limit must be between 1 and 65536", -1));
		Tcl_SetErrorCode(interp, "TCL", "VALUE", "READLIMIT", NULL);
		return TCL_ERROR;
	    }
	}
    }

    if (setOptionProc == NULL) {
	if (cd->format == TCL_ZLIB_FORMAT_GZIP) {
	    return Tcl_BadChannelOption(interp, optionName,
		    (cd->mode == TCL_ZLIB_STREAM_DEFLATE)
		    ? gzipChanOptions : gunzipChanOptions);
	} else {
	    return Tcl_BadChannelOption(interp, optionName,
		    (cd->mode == TCL_ZLIB_STREAM_DEFLATE)
		    ? compressChanOptions : decompressChanOptions);
	}
    }

    /*
     * Pass all unknown options down, to deeper transforms and/or the base
     * channel.
     */

    return setOptionProc(Tcl_GetChannelInstanceData(cd->parent), interp,
	    optionName, value);
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibTransformGetOption --
 *
 *	Reading side of [fconfigure] on our channel.
 *
 *----------------------------------------------------------------------
 */

static int
ZlibTransformGetOption(
    ClientData instanceData,
    Tcl_Interp *interp,
    const char *optionName,
    Tcl_DString *dsPtr)
{
    ZlibChannelData *cd = instanceData;
    Tcl_DriverGetOptionProc *getOptionProc =
	    Tcl_ChannelGetOptionProc(Tcl_GetChannelType(cd->parent));
    static const char *compressChanOptions = "checksum dictionary";
    static const char *gzipChanOptions = "checksum";
    static const char *decompressChanOptions = "checksum dictionary limit";
    static const char *gunzipChanOptions = "checksum header limit";

    /*
     * The "crc" option reports the current CRC (calculated with the Adler32
     * or CRC32 algorithm according to the format) given the data that has
     * been processed so far.
     */

    if (optionName == NULL || strcmp(optionName, "-checksum") == 0) {
	uLong crc;
	char buf[12];

	if (cd->mode == TCL_ZLIB_STREAM_DEFLATE) {
	    crc = cd->outStream.adler;
	} else {
	    crc = cd->inStream.adler;
	}

	sprintf(buf, "%lu", crc);
	if (optionName == NULL) {
	    Tcl_DStringAppendElement(dsPtr, "-checksum");
	    Tcl_DStringAppendElement(dsPtr, buf);
	} else {
	    Tcl_DStringAppend(dsPtr, buf, -1);
	    return TCL_OK;
	}
    }

    if ((cd->format != TCL_ZLIB_FORMAT_GZIP) &&
	    (optionName == NULL || strcmp(optionName, "-dictionary") == 0)) {
	/*
	 * Embedded NUL bytes are ok; they'll be C080-encoded.
	 */

	if (optionName == NULL) {
	    Tcl_DStringAppendElement(dsPtr, "-dictionary");
	    if (cd->compDictObj) {
		Tcl_DStringAppendElement(dsPtr,
			Tcl_GetString(cd->compDictObj));
	    } else {
		Tcl_DStringAppendElement(dsPtr, "");
	    }
	} else {
	    if (cd->compDictObj) {
		int len;
		const char *str = Tcl_GetStringFromObj(cd->compDictObj, &len);

		Tcl_DStringAppend(dsPtr, str, len);
	    }
	    return TCL_OK;
	}
    }

    /*
     * The "header" option, which is only valid on inflating gzip channels,
     * reports the header that has been read from the start of the stream.
     */

    if ((cd->flags & IN_HEADER) && ((optionName == NULL) ||
	    (strcmp(optionName, "-header") == 0))) {
	Tcl_Obj *tmpObj = Tcl_NewObj();

	ExtractHeader(&cd->inHeader.header, tmpObj);
	if (optionName == NULL) {
	    Tcl_DStringAppendElement(dsPtr, "-header");
	    Tcl_DStringAppendElement(dsPtr, Tcl_GetString(tmpObj));
	    Tcl_DecrRefCount(tmpObj);
	} else {
	    TclDStringAppendObj(dsPtr, tmpObj);
	    Tcl_DecrRefCount(tmpObj);
	    return TCL_OK;
	}
    }

    /*
     * Now we do the standard processing of the stream we wrapped.
     */

    if (getOptionProc) {
	return getOptionProc(Tcl_GetChannelInstanceData(cd->parent),
		interp, optionName, dsPtr);
    }
    if (optionName == NULL) {
	return TCL_OK;
    }
    if (cd->format == TCL_ZLIB_FORMAT_GZIP) {
	return Tcl_BadChannelOption(interp, optionName,
		(cd->mode == TCL_ZLIB_STREAM_DEFLATE)
		? gzipChanOptions : gunzipChanOptions);
    } else {
	return Tcl_BadChannelOption(interp, optionName,
		(cd->mode == TCL_ZLIB_STREAM_DEFLATE)
		? compressChanOptions : decompressChanOptions);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibTransformWatch, ZlibTransformEventHandler --
 *
 *	If we have data pending, trigger a readable event after a short time
 *	(in order to allow a real event to catch up).
 *
 *----------------------------------------------------------------------
 */

static void
ZlibTransformWatch(
    ClientData instanceData,
    int mask)
{
    ZlibChannelData *cd = instanceData;
    Tcl_DriverWatchProc *watchProc;

    /*
     * This code is based on the code in tclIORTrans.c
     */

    watchProc = Tcl_ChannelWatchProc(Tcl_GetChannelType(cd->parent));
    watchProc(Tcl_GetChannelInstanceData(cd->parent), mask);

    if (!(mask & TCL_READABLE) || Tcl_DStringLength(&cd->decompressed) == 0) {
	ZlibTransformEventTimerKill(cd);
    } else if (cd->timer == NULL) {
	cd->timer = Tcl_CreateTimerHandler(SYNTHETIC_EVENT_TIME,
		ZlibTransformTimerRun, cd);
    }
}

static int
ZlibTransformEventHandler(
    ClientData instanceData,
    int interestMask)
{
    ZlibChannelData *cd = instanceData;

    ZlibTransformEventTimerKill(cd);
    return interestMask;
}

static inline void
ZlibTransformEventTimerKill(
    ZlibChannelData *cd)
{
    if (cd->timer != NULL) {
	Tcl_DeleteTimerHandler(cd->timer);
	cd->timer = NULL;
    }
}

static void
ZlibTransformTimerRun(
    ClientData clientData)
{
    ZlibChannelData *cd = clientData;

    cd->timer = NULL;
    Tcl_NotifyChannel(cd->chan, TCL_READABLE);
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibTransformGetHandle --
 *
 *	Anything that needs the OS handle is told to get it from what we are
 *	stacked on top of.
 *
 *----------------------------------------------------------------------
 */

static int
ZlibTransformGetHandle(
    ClientData instanceData,
    int direction,
    ClientData *handlePtr)
{
    ZlibChannelData *cd = instanceData;

    return Tcl_GetChannelHandle(cd->parent, direction, handlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibTransformBlockMode --
 *
 *	We need to keep track of the blocking mode; it changes our behavior.
 *
 *----------------------------------------------------------------------
 */

static int
ZlibTransformBlockMode(
    ClientData instanceData,
    int mode)
{
    ZlibChannelData *cd = instanceData;

    if (mode == TCL_MODE_NONBLOCKING) {
	cd->flags |= ASYNC;
    } else {
	cd->flags &= ~ASYNC;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ZlibStackChannelTransform --
 *
 *	Stacks either compression or decompression onto a channel.
 *
 * Results:
 *	The stacked channel, or NULL if there was an error.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Channel
ZlibStackChannelTransform(
    Tcl_Interp *interp,		/* Where to write error messages. */
    int mode,			/* Whether this is a compressing transform
				 * (TCL_ZLIB_STREAM_DEFLATE) or a
				 * decompressing transform
				 * (TCL_ZLIB_STREAM_INFLATE). Note that
				 * compressing transforms require that the
				 * channel is writable, and decompressing
				 * transforms require that the channel is
				 * readable. */
    int format,			/* One of the TCL_ZLIB_FORMAT_* values that
				 * indicates what compressed format to allow.
				 * TCL_ZLIB_FORMAT_AUTO is only supported for
				 * decompressing transforms. */
    int level,			/* What compression level to use. Ignored for
				 * decompressing transforms. */
    int limit,			/* The limit on the number of bytes to read
				 * ahead; always at least 1. */
    Tcl_Channel channel,	/* The channel to attach to. */
    Tcl_Obj *gzipHeaderDictPtr,	/* A description of header to use, or NULL to
				 * use a default. Ignored if not compressing
				 * to produce gzip-format data. */
    Tcl_Obj *compDictObj)	/* Byte-array object containing compression
				 * dictionary (not dictObj!) to use if
				 * necessary. */
{
    ZlibChannelData *cd = ckalloc(sizeof(ZlibChannelData));
    Tcl_Channel chan;
    int wbits = 0;

    if (mode != TCL_ZLIB_STREAM_DEFLATE && mode != TCL_ZLIB_STREAM_INFLATE) {
	Tcl_Panic("unknown mode: %d", mode);
    }

    memset(cd, 0, sizeof(ZlibChannelData));
    cd->mode = mode;
    cd->format = format;
    cd->readAheadLimit = limit;

    if (format == TCL_ZLIB_FORMAT_GZIP || format == TCL_ZLIB_FORMAT_AUTO) {
	if (mode == TCL_ZLIB_STREAM_DEFLATE) {
	    if (gzipHeaderDictPtr) {
		cd->flags |= OUT_HEADER;
		if (GenerateHeader(interp, gzipHeaderDictPtr, &cd->outHeader,
			NULL) != TCL_OK) {
		    goto error;
		}
	    }
	} else {
	    cd->flags |= IN_HEADER;
	    cd->inHeader.header.name = (Bytef *)
		    &cd->inHeader.nativeFilenameBuf;
	    cd->inHeader.header.name_max = MAXPATHLEN - 1;
	    cd->inHeader.header.comment = (Bytef *)
		    &cd->inHeader.nativeCommentBuf;
	    cd->inHeader.header.comm_max = MAX_COMMENT_LEN - 1;
	}
    }

    if (compDictObj != NULL) {
	cd->compDictObj = Tcl_DuplicateObj(compDictObj);
	Tcl_IncrRefCount(cd->compDictObj);
	Tcl_GetByteArrayFromObj(cd->compDictObj, NULL);
    }

    if (format == TCL_ZLIB_FORMAT_RAW) {
	wbits = WBITS_RAW;
    } else if (format == TCL_ZLIB_FORMAT_ZLIB) {
	wbits = WBITS_ZLIB;
    } else if (format == TCL_ZLIB_FORMAT_GZIP) {
	wbits = WBITS_GZIP;
    } else if (format == TCL_ZLIB_FORMAT_AUTO) {
	wbits = WBITS_AUTODETECT;
    } else {
	Tcl_Panic("bad format: %d", format);
    }

    /*
     * Initialize input inflater or the output deflater.
     */

    if (mode == TCL_ZLIB_STREAM_INFLATE) {
	if (inflateInit2(&cd->inStream, wbits) != Z_OK) {
	    goto error;
	}
	cd->inAllocated = DEFAULT_BUFFER_SIZE;
	cd->inBuffer = ckalloc(cd->inAllocated);
	if (cd->flags & IN_HEADER) {
	    if (inflateGetHeader(&cd->inStream, &cd->inHeader.header) != Z_OK) {
		goto error;
	    }
	}
	if (cd->format == TCL_ZLIB_FORMAT_RAW && cd->compDictObj) {
	    if (SetInflateDictionary(&cd->inStream, cd->compDictObj) != Z_OK) {
		goto error;
	    }
	}
    } else {
	if (deflateInit2(&cd->outStream, level, Z_DEFLATED, wbits,
		MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK) {
	    goto error;
	}
	cd->outAllocated = DEFAULT_BUFFER_SIZE;
	cd->outBuffer = ckalloc(cd->outAllocated);
	if (cd->flags & OUT_HEADER) {
	    if (deflateSetHeader(&cd->outStream, &cd->outHeader.header) != Z_OK) {
		goto error;
	    }
	}
	if (cd->compDictObj) {
	    if (SetDeflateDictionary(&cd->outStream, cd->compDictObj) != Z_OK) {
		goto error;
	    }
	}
    }

    Tcl_DStringInit(&cd->decompressed);

    chan = Tcl_StackChannel(interp, &zlibChannelType, cd,
	    Tcl_GetChannelMode(channel), channel);
    if (chan == NULL) {
	goto error;
    }
    cd->chan = chan;
    cd->parent = Tcl_GetStackedChannel(chan);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(Tcl_GetChannelName(chan), -1));
    return chan;

  error:
    if (cd->inBuffer) {
	ckfree(cd->inBuffer);
	inflateEnd(&cd->inStream);
    }
    if (cd->outBuffer) {
	ckfree(cd->outBuffer);
	deflateEnd(&cd->outStream);
    }
    if (cd->compDictObj) {
	Tcl_DecrRefCount(cd->compDictObj);
    }
    ckfree(cd);
    return NULL;
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
    ZlibChannelData *cd,	/* The location of the buffer to read from. */
    char *buf,			/* The buffer to copy into */
    int toRead)			/* Number of requested bytes */
{
    int have = Tcl_DStringLength(&cd->decompressed);

    if (have == 0) {
	/*
	 * Nothing to copy in the case of an empty buffer.
	 */

	return 0;
    } else if (have > toRead) {
	/*
	 * The internal buffer contains more than requested. Copy the
	 * requested subset to the caller, shift the remaining bytes down, and
	 * truncate.
	 */

	char *src = Tcl_DStringValue(&cd->decompressed);

	memcpy(buf, src, toRead);
	memmove(src, src + toRead, have - toRead);

	Tcl_DStringSetLength(&cd->decompressed, have - toRead);
	return toRead;
    } else /* have <= toRead */ {
	/*
	 * There is just or not enough in the buffer to fully satisfy the
	 * caller, so take everything as best effort.
	 */

	memcpy(buf, Tcl_DStringValue(&cd->decompressed), have);
	TclDStringClear(&cd->decompressed);
	return have;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ResultGenerate --
 *
 *	Extract uncompressed bytes from the compression engine and store them
 *	in our working buffer.
 *
 * Result:
 *	TCL_OK/TCL_ERROR (with *errorCodePtr updated with reason).
 *
 * Side effects:
 *	See above.
 *
 *----------------------------------------------------------------------
 */

static int
ResultGenerate(
    ZlibChannelData *cd,
    int n,
    int flush,
    int *errorCodePtr)
{
#define MAXBUF	1024
    unsigned char buf[MAXBUF];
    int e, written;
    Tcl_Obj *errObj;

    cd->inStream.next_in = (Bytef *) cd->inBuffer;
    cd->inStream.avail_in = n;

    while (1) {
	cd->inStream.next_out = (Bytef *) buf;
	cd->inStream.avail_out = MAXBUF;

	e = inflate(&cd->inStream, flush);
	if (e == Z_NEED_DICT && cd->compDictObj) {
	    e = SetInflateDictionary(&cd->inStream, cd->compDictObj);
	    if (e == Z_OK) {
		/*
		 * A repetition of Z_NEED_DICT is just an error.
		 */

		cd->inStream.next_out = (Bytef *) buf;
		cd->inStream.avail_out = MAXBUF;
		e = inflate(&cd->inStream, flush);
	    }
	}

	/*
	 * avail_out is now the left over space in the output.  Therefore
	 * "MAXBUF - avail_out" is the amount of bytes generated.
	 */

	written = MAXBUF - cd->inStream.avail_out;
	if (written) {
	    Tcl_DStringAppend(&cd->decompressed, (char *) buf, written);
	}

	/*
	 * The cases where we're definitely done.
	 */

	if (((flush == Z_SYNC_FLUSH) && (e == Z_BUF_ERROR))
		|| (e == Z_STREAM_END)
		|| (e == Z_OK && cd->inStream.avail_out == 0)) {
	    return TCL_OK;
	}

	/*
	 * Z_BUF_ERROR can be ignored as per http://www.zlib.net/zlib_how.html
	 *
	 * Just indicates that the zlib couldn't consume input/produce output,
	 * and is fixed by supplying more input.
	 *
	 * Otherwise, we've got errors and need to report to higher-up.
	 */

	if ((e != Z_OK) && (e != Z_BUF_ERROR)) {
	    goto handleError;
	}

	/*
	 * Check if the inflate stopped early.
	 */

	if (cd->inStream.avail_in <= 0 && flush != Z_SYNC_FLUSH) {
	    return TCL_OK;
	}
    }

  handleError:
    errObj = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(NULL, errObj, Tcl_NewStringObj("-errorcode",-1));
    Tcl_ListObjAppendElement(NULL, errObj,
	    ConvertErrorToList(e, cd->inStream.adler));
    Tcl_ListObjAppendElement(NULL, errObj,
	    Tcl_NewStringObj(cd->inStream.msg, -1));
    Tcl_SetChannelError(cd->parent, errObj);
    *errorCodePtr = EINVAL;
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *	Finally, the TclZlibInit function. Used to install the zlib API.
 *----------------------------------------------------------------------
 */

int
TclZlibInit(
    Tcl_Interp *interp)
{
    Tcl_Config cfg[2];

    /*
     * This does two things. It creates a counter used in the creation of
     * stream commands, and it creates the namespace that will contain those
     * commands.
     */

    Tcl_EvalEx(interp, "namespace eval ::tcl::zlib {variable cmdcounter 0}", -1, 0);

    /*
     * Create the public scripted interface to this file's functionality.
     */

    Tcl_CreateObjCommand(interp, "zlib", ZlibCmd, 0, 0);

    /*
     * Store the underlying configuration information.
     *
     * TODO: Describe whether we're using the system version of the library or
     * a compatibility version built into Tcl?
     */

    cfg[0].key = "zlibVersion";
    cfg[0].value = zlibVersion();
    cfg[1].key = NULL;
    Tcl_RegisterConfig(interp, "zlib", cfg, "iso8859-1");

    /*
     * Formally provide the package as a Tcl built-in.
     */

    return Tcl_PkgProvide(interp, "zlib", TCL_ZLIB_VERSION);
}

/*
 *----------------------------------------------------------------------
 *	Stubs used when a suitable zlib installation was not found during
 *	configure.
 *----------------------------------------------------------------------
 */

#else /* !HAVE_ZLIB */
int
Tcl_ZlibStreamInit(
    Tcl_Interp *interp,
    int mode,
    int format,
    int level,
    Tcl_Obj *dictObj,
    Tcl_ZlibStream *zshandle)
{
    if (interp) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("unimplemented", -1));
	Tcl_SetErrorCode(interp, "TCL", "UNIMPLEMENTED", NULL);
    }
    return TCL_ERROR;
}

int
Tcl_ZlibStreamClose(
    Tcl_ZlibStream zshandle)
{
    return TCL_OK;
}

int
Tcl_ZlibStreamReset(
    Tcl_ZlibStream zshandle)
{
    return TCL_OK;
}

Tcl_Obj *
Tcl_ZlibStreamGetCommandName(
    Tcl_ZlibStream zshandle)
{
    return NULL;
}

int
Tcl_ZlibStreamEof(
    Tcl_ZlibStream zshandle)
{
    return 1;
}

int
Tcl_ZlibStreamChecksum(
    Tcl_ZlibStream zshandle)
{
    return 0;
}

int
Tcl_ZlibStreamPut(
    Tcl_ZlibStream zshandle,
    Tcl_Obj *data,
    int flush)
{
    return TCL_OK;
}

int
Tcl_ZlibStreamGet(
    Tcl_ZlibStream zshandle,
    Tcl_Obj *data,
    int count)
{
    return TCL_OK;
}

int
Tcl_ZlibDeflate(
    Tcl_Interp *interp,
    int format,
    Tcl_Obj *data,
    int level,
    Tcl_Obj *gzipHeaderDictObj)
{
    if (interp) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("unimplemented", -1));
	Tcl_SetErrorCode(interp, "TCL", "UNIMPLEMENTED", NULL);
    }
    return TCL_ERROR;
}

int
Tcl_ZlibInflate(
    Tcl_Interp *interp,
    int format,
    Tcl_Obj *data,
    int bufferSize,
    Tcl_Obj *gzipHeaderDictObj)
{
    if (interp) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("unimplemented", -1));
	Tcl_SetErrorCode(interp, "TCL", "UNIMPLEMENTED", NULL);
    }
    return TCL_ERROR;
}

unsigned int
Tcl_ZlibCRC32(
    unsigned int crc,
    const char *buf,
    int len)
{
    return 0;
}

unsigned int
Tcl_ZlibAdler32(
    unsigned int adler,
    const char *buf,
    int len)
{
    return 0;
}

void
Tcl_ZlibStreamSetCompressionDictionary(
    Tcl_ZlibStream zshandle,
    Tcl_Obj *compressionDictionaryObj)
{
    /* Do nothing. */
}
#endif /* HAVE_ZLIB */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
