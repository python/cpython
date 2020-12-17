/*
 * tkImgPNG.c --
 *
 *	A Tk photo image file handler for PNG files.
 *
 * Copyright (c) 2006-2008 Muonics, Inc.
 * Copyright (c) 2008 Donal K. Fellows
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

#define	PNG_INT32(a,b,c,d)	\
	(((long)(a) << 24) | ((long)(b) << 16) | ((long)(c) << 8) | (long)(d))
#define	PNG_BLOCK_SZ	1024		/* Process up to 1k at a time. */
#define PNG_MIN(a, b) (((a) < (b)) ? (a) : (b))

/*
 * Every PNG image starts with the following 8-byte signature.
 */

#define PNG_SIG_SZ	8
static const unsigned char pngSignature[] = {
    137, 80, 78, 71, 13, 10, 26, 10
};

static const int startLine[8] = {
    0, 0, 0, 4, 0, 2, 0, 1
};

/*
 * Chunk type flags.
 */

#define PNG_CF_ANCILLARY 0x20000000L	/* Non-critical chunk (can ignore). */
#define PNG_CF_PRIVATE   0x00100000L	/* Application-specific chunk. */
#define PNG_CF_RESERVED  0x00001000L	/* Not used. */
#define PNG_CF_COPYSAFE  0x00000010L	/* Opaque data safe for copying. */

/*
 * Chunk types, not all of which have support implemented. Note that there are
 * others in the official extension set which we will never support (as they
 * are officially deprecated).
 */

#define CHUNK_IDAT	PNG_INT32('I','D','A','T')	/* Pixel data. */
#define CHUNK_IEND	PNG_INT32('I','E','N','D')	/* End of Image. */
#define CHUNK_IHDR	PNG_INT32('I','H','D','R')	/* Header. */
#define CHUNK_PLTE	PNG_INT32('P','L','T','E')	/* Palette. */

#define CHUNK_bKGD	PNG_INT32('b','K','G','D')	/* Background Color */
#define CHUNK_cHRM	PNG_INT32('c','H','R','M')	/* Chroma values. */
#define CHUNK_gAMA	PNG_INT32('g','A','M','A')	/* Gamma. */
#define CHUNK_hIST	PNG_INT32('h','I','S','T')	/* Histogram. */
#define CHUNK_iCCP	PNG_INT32('i','C','C','P')	/* Color profile. */
#define CHUNK_iTXt	PNG_INT32('i','T','X','t')	/* Internationalized
							 * text (comments,
							 * etc.) */
#define CHUNK_oFFs	PNG_INT32('o','F','F','s')	/* Image offset. */
#define CHUNK_pCAL	PNG_INT32('p','C','A','L')	/* Pixel calibration
							 * data. */
#define CHUNK_pHYs	PNG_INT32('p','H','Y','s')	/* Physical pixel
							 * dimensions. */
#define CHUNK_sBIT	PNG_INT32('s','B','I','T')	/* Significant bits */
#define CHUNK_sCAL	PNG_INT32('s','C','A','L')	/* Physical scale. */
#define CHUNK_sPLT	PNG_INT32('s','P','L','T')	/* Suggested
							 * palette. */
#define CHUNK_sRGB	PNG_INT32('s','R','G','B')	/* Standard RGB space
							 * declaration. */
#define CHUNK_tEXt	PNG_INT32('t','E','X','t')	/* Plain Latin-1
							 * text. */
#define CHUNK_tIME	PNG_INT32('t','I','M','E')	/* Time stamp. */
#define CHUNK_tRNS	PNG_INT32('t','R','N','S')	/* Transparency. */
#define CHUNK_zTXt	PNG_INT32('z','T','X','t')	/* Compressed Latin-1
							 * text. */

/*
 * Color flags.
 */

#define PNG_COLOR_INDEXED	1
#define PNG_COLOR_USED		2
#define PNG_COLOR_ALPHA		4

/*
 * Actual color types.
 */

#define PNG_COLOR_GRAY		0
#define PNG_COLOR_RGB		(PNG_COLOR_USED)
#define PNG_COLOR_PLTE		(PNG_COLOR_USED | PNG_COLOR_INDEXED)
#define PNG_COLOR_GRAYALPHA	(PNG_COLOR_GRAY | PNG_COLOR_ALPHA)
#define PNG_COLOR_RGBA		(PNG_COLOR_USED | PNG_COLOR_ALPHA)

/*
 * Compression Methods.
 */

#define PNG_COMPRESS_DEFLATE	0

/*
 * Filter Methods.
 */

#define PNG_FILTMETH_STANDARD	0

/*
 * Interlacing Methods.
 */

#define	PNG_INTERLACE_NONE	0
#define PNG_INTERLACE_ADAM7	1

/*
 * State information, used to store everything about the PNG image being
 * currently parsed or created.
 */

typedef struct {
    /*
     * PNG data source/destination channel/object/byte array.
     */

    Tcl_Channel channel;	/* Channel for from-file reads. */
    Tcl_Obj *objDataPtr;
    unsigned char *strDataBuf;	/* Raw source data for from-string reads. */
    int strDataLen;		/* Length of source data. */
    unsigned char *base64Data;	/* base64 encoded string data. */
    unsigned char base64Bits;	/* Remaining bits from last base64 read. */
    unsigned char base64State;	/* Current state of base64 decoder. */
    double alpha;		/* Alpha from -format option. */

    /*
     * Image header information.
     */

    unsigned char bitDepth;	/* Number of bits per pixel. */
    unsigned char colorType;	/* Grayscale, TrueColor, etc. */
    unsigned char compression;	/* Compression Mode (always zlib). */
    unsigned char filter;	/* Filter mode (0 - 3). */
    unsigned char interlace;	/* Type of interlacing (if any). */
    unsigned char numChannels;	/* Number of channels per pixel. */
    unsigned char bytesPerPixel;/* Bytes per pixel in scan line. */
    int bitScale;		/* Scale factor for RGB/Gray depths < 8. */
    int currentLine;		/* Current line being unfiltered. */
    unsigned char phase;	/* Interlacing phase (0..6). */
    Tk_PhotoImageBlock block;
    int blockLen;		/* Number of bytes in Tk image pixels. */

    /*
     * For containing data read from PLTE (palette) and tRNS (transparency)
     * chunks.
     */

    int paletteLen;		/* Number of PLTE entries (1..256). */
    int useTRNS;		/* Flag to indicate whether there was a
				 * palette given. */
    struct {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;
    } palette[256];		/* Palette RGB/Transparency table. */
    unsigned char transVal[6];	/* Fully-transparent RGB/Gray Value. */

    /*
     * For compressing and decompressing IDAT chunks.
     */

    Tcl_ZlibStream stream;	/* Inflating or deflating stream; this one is
				 * not bound to a Tcl command. */
    Tcl_Obj *lastLineObj;	/* Last line of pixels, for unfiltering. */
    Tcl_Obj *thisLineObj;	/* Current line of pixels to process. */
    int lineSize;		/* Number of bytes in a PNG line. */
    int phaseSize;		/* Number of bytes/line in current phase. */
} PNGImage;

/*
 * Maximum size of various chunks.
 */

#define	PNG_PLTE_MAXSZ 768	/* 3 bytes/RGB entry, 256 entries max */
#define	PNG_TRNS_MAXSZ 256	/* 1-byte alpha, 256 entries max */

/*
 * Forward declarations of non-global functions defined in this file:
 */

static void		ApplyAlpha(PNGImage *pngPtr);
static int		CheckColor(Tcl_Interp *interp, PNGImage *pngPtr);
static inline int	CheckCRC(Tcl_Interp *interp, PNGImage *pngPtr,
			    unsigned long calculated);
static void		CleanupPNGImage(PNGImage *pngPtr);
static int		DecodeLine(Tcl_Interp *interp, PNGImage *pngPtr);
static int		DecodePNG(Tcl_Interp *interp, PNGImage *pngPtr,
			    Tcl_Obj *fmtObj, Tk_PhotoHandle imageHandle,
			    int destX, int destY);
static int		EncodePNG(Tcl_Interp *interp,
			    Tk_PhotoImageBlock *blockPtr, PNGImage *pngPtr);
static int		FileMatchPNG(Tcl_Channel chan, const char *fileName,
			    Tcl_Obj *fmtObj, int *widthPtr, int *heightPtr,
			    Tcl_Interp *interp);
static int		FileReadPNG(Tcl_Interp *interp, Tcl_Channel chan,
			    const char *fileName, Tcl_Obj *fmtObj,
			    Tk_PhotoHandle imageHandle, int destX, int destY,
			    int width, int height, int srcX, int srcY);
static int		FileWritePNG(Tcl_Interp *interp, const char *filename,
			    Tcl_Obj *fmtObj, Tk_PhotoImageBlock *blockPtr);
static int		InitPNGImage(Tcl_Interp *interp, PNGImage *pngPtr,
			    Tcl_Channel chan, Tcl_Obj *objPtr, int dir);
static inline unsigned char Paeth(int a, int b, int c);
static int		ParseFormat(Tcl_Interp *interp, Tcl_Obj *fmtObj,
			    PNGImage *pngPtr);
static int		ReadBase64(Tcl_Interp *interp, PNGImage *pngPtr,
			    unsigned char *destPtr, int destSz,
			    unsigned long *crcPtr);
static int		ReadByteArray(Tcl_Interp *interp, PNGImage *pngPtr,
			    unsigned char *destPtr, int destSz,
			    unsigned long *crcPtr);
static int		ReadData(Tcl_Interp *interp, PNGImage *pngPtr,
			    unsigned char *destPtr, int destSz,
			    unsigned long *crcPtr);
static int		ReadChunkHeader(Tcl_Interp *interp, PNGImage *pngPtr,
			    int *sizePtr, unsigned long *typePtr,
			    unsigned long *crcPtr);
static int		ReadIDAT(Tcl_Interp *interp, PNGImage *pngPtr,
			    int chunkSz, unsigned long crc);
static int		ReadIHDR(Tcl_Interp *interp, PNGImage *pngPtr);
static inline int	ReadInt32(Tcl_Interp *interp, PNGImage *pngPtr,
			    unsigned long *resultPtr, unsigned long *crcPtr);
static int		ReadPLTE(Tcl_Interp *interp, PNGImage *pngPtr,
			    int chunkSz, unsigned long crc);
static int		ReadTRNS(Tcl_Interp *interp, PNGImage *pngPtr,
			    int chunkSz, unsigned long crc);
static int		SkipChunk(Tcl_Interp *interp, PNGImage *pngPtr,
			    int chunkSz, unsigned long crc);
static int		StringMatchPNG(Tcl_Obj *dataObj, Tcl_Obj *fmtObj,
			    int *widthPtr, int *heightPtr,
			    Tcl_Interp *interp);
static int		StringReadPNG(Tcl_Interp *interp, Tcl_Obj *dataObj,
			    Tcl_Obj *fmtObj, Tk_PhotoHandle imageHandle,
			    int destX, int destY, int width, int height,
			    int srcX, int srcY);
static int		StringWritePNG(Tcl_Interp *interp, Tcl_Obj *fmtObj,
			    Tk_PhotoImageBlock *blockPtr);
static int		UnfilterLine(Tcl_Interp *interp, PNGImage *pngPtr);
static inline int	WriteByte(Tcl_Interp *interp, PNGImage *pngPtr,
			    unsigned char c, unsigned long *crcPtr);
static inline int	WriteChunk(Tcl_Interp *interp, PNGImage *pngPtr,
			    unsigned long chunkType,
			    const unsigned char *dataPtr, int dataSize);
static int		WriteData(Tcl_Interp *interp, PNGImage *pngPtr,
			    const unsigned char *srcPtr, int srcSz,
			    unsigned long *crcPtr);
static int		WriteExtraChunks(Tcl_Interp *interp,
			    PNGImage *pngPtr);
static int		WriteIHDR(Tcl_Interp *interp, PNGImage *pngPtr,
			    Tk_PhotoImageBlock *blockPtr);
static int		WriteIDAT(Tcl_Interp *interp, PNGImage *pngPtr,
			    Tk_PhotoImageBlock *blockPtr);
static inline int	WriteInt32(Tcl_Interp *interp, PNGImage *pngPtr,
			    unsigned long l, unsigned long *crcPtr);

/*
 * The format record for the PNG file format:
 */

Tk_PhotoImageFormat tkImgFmtPNG = {
    "png",			/* name */
    FileMatchPNG,		/* fileMatchProc */
    StringMatchPNG,		/* stringMatchProc */
    FileReadPNG,		/* fileReadProc */
    StringReadPNG,		/* stringReadProc */
    FileWritePNG,		/* fileWriteProc */
    StringWritePNG,		/* stringWriteProc */
    NULL
};

/*
 *----------------------------------------------------------------------
 *
 * InitPNGImage --
 *
 *	This function is invoked by each of the Tk image handler procs
 *	(MatchStringProc, etc.) to initialize state information used during
 *	the course of encoding or decoding a PNG image.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if initialization failed.
 *
 * Side effects:
 *	The reference count of the -data Tcl_Obj*, if any, is incremented.
 *
 *----------------------------------------------------------------------
 */

static int
InitPNGImage(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    Tcl_Channel chan,
    Tcl_Obj *objPtr,
    int dir)
{
    memset(pngPtr, 0, sizeof(PNGImage));

    pngPtr->channel = chan;
    pngPtr->alpha = 1.0;

    /*
     * If decoding from a -data string object, increment its reference count
     * for the duration of the decode and get its length and byte array for
     * reading with ReadData().
     */

    if (objPtr) {
	Tcl_IncrRefCount(objPtr);
	pngPtr->objDataPtr = objPtr;
	pngPtr->strDataBuf =
		Tcl_GetByteArrayFromObj(objPtr, &pngPtr->strDataLen);
    }

    /*
     * Initialize the palette transparency table to fully opaque.
     */

    memset(pngPtr->palette, 255, sizeof(pngPtr->palette));

    /*
     * Initialize Zlib inflate/deflate stream.
     */

    if (Tcl_ZlibStreamInit(NULL, dir, TCL_ZLIB_FORMAT_ZLIB,
	    TCL_ZLIB_COMPRESS_DEFAULT, NULL, &pngPtr->stream) != TCL_OK) {
	if (interp) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "zlib initialization failed", -1));
	    Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "ZLIB_INIT", NULL);
	}
	if (objPtr) {
	    Tcl_DecrRefCount(objPtr);
	}
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CleanupPNGImage --
 *
 *	This function is invoked by each of the Tk image handler procs
 *	(MatchStringProc, etc.) prior to returning to Tcl in order to clean up
 *	any allocated memory and call other cleanup handlers such as zlib's
 *	inflateEnd/deflateEnd.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count of the -data Tcl_Obj*, if any, is decremented.
 *	Buffers are freed, streams are closed. The PNGImage should not be used
 *	for any purpose without being reinitialized post-cleanup.
 *
 *----------------------------------------------------------------------
 */

static void
CleanupPNGImage(
    PNGImage *pngPtr)
{
    /*
     * Don't need the object containing the -data value anymore.
     */

    if (pngPtr->objDataPtr) {
	Tcl_DecrRefCount(pngPtr->objDataPtr);
    }

    /*
     * Discard pixel buffer.
     */

    if (pngPtr->stream) {
	Tcl_ZlibStreamClose(pngPtr->stream);
    }

    if (pngPtr->block.pixelPtr) {
	ckfree(pngPtr->block.pixelPtr);
    }
    if (pngPtr->thisLineObj) {
	Tcl_DecrRefCount(pngPtr->thisLineObj);
    }
    if (pngPtr->lastLineObj) {
	Tcl_DecrRefCount(pngPtr->lastLineObj);
    }

    memset(pngPtr, 0, sizeof(PNGImage));
}

/*
 *----------------------------------------------------------------------
 *
 * ReadBase64 --
 *
 *	This function is invoked to read the specified number of bytes from
 *	base-64 encoded image data.
 *
 *	Note: It would be better if the Tk_PhotoImage stuff handled this by
 *	creating a channel from the -data value, which would take care of
 *	base64 decoding and made the data readable as if it were coming from a
 *	file.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if an I/O error occurs.
 *
 * Side effects:
 *	The file position will change. The running CRC is updated if a pointer
 *	to it is provided.
 *
 *----------------------------------------------------------------------
 */

static int
ReadBase64(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    unsigned char *destPtr,
    int destSz,
    unsigned long *crcPtr)
{
    static const unsigned char from64[] = {
	0x82, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x80, 0x80,
	0x83, 0x80, 0x80, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
	0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x80,
	0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x3e,
	0x83, 0x83, 0x83, 0x3f, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a,
	0x3b, 0x3c, 0x3d, 0x83, 0x83, 0x83, 0x81, 0x83, 0x83, 0x83, 0x00,
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
	0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
	0x17, 0x18, 0x19, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x1a, 0x1b,
	0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
	0x32, 0x33, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
	0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
	0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
	0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
	0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
	0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
	0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
	0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
	0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
	0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
	0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
	0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
	0x83, 0x83
    };

    /*
     * Definitions for the base-64 decoder.
     */

#define PNG64_SPECIAL	0x80	/* Flag bit */
#define PNG64_SPACE	0x80	/* Whitespace */
#define PNG64_PAD	0x81	/* Padding */
#define PNG64_DONE	0x82	/* End of data */
#define PNG64_BAD	0x83	/* Ooooh, naughty! */

    while (destSz && pngPtr->strDataLen) {
	unsigned char c = 0;
	unsigned char c64 = from64[*pngPtr->strDataBuf++];

	pngPtr->strDataLen--;

	if (PNG64_SPACE == c64) {
	    continue;
	}

	if (c64 & PNG64_SPECIAL) {
	    c = (unsigned char) pngPtr->base64Bits;
	} else {
	    switch (pngPtr->base64State++) {
	    case 0:
		pngPtr->base64Bits = c64 << 2;
		continue;
	    case 1:
		c = (unsigned char) (pngPtr->base64Bits | (c64 >> 4));
		pngPtr->base64Bits = (c64 & 0xF) << 4;
		break;
	    case 2:
		c = (unsigned char) (pngPtr->base64Bits | (c64 >> 2));
		pngPtr->base64Bits = (c64 & 0x3) << 6;
		break;
	    case 3:
		c = (unsigned char) (pngPtr->base64Bits | c64);
		pngPtr->base64State = 0;
		pngPtr->base64Bits = 0;
		break;
	    }
	}

	if (crcPtr) {
	    *crcPtr = Tcl_ZlibCRC32(*crcPtr, &c, 1);
	}

	if (destPtr) {
	    *destPtr++ = c;
	}

	destSz--;

	if (c64 & PNG64_SPECIAL) {
	    break;
	}
    }

    if (destSz) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"unexpected end of image data", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "EARLY_END", NULL);
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ReadByteArray --
 *
 *	This function is invoked to read the specified number of bytes from a
 *	non-base64-encoded byte array provided via the -data option.
 *
 *	Note: It would be better if the Tk_PhotoImage stuff handled this by
 *	creating a channel from the -data value and made the data readable as
 *	if it were coming from a file.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if an I/O error occurs.
 *
 * Side effects:
 *	The file position will change. The running CRC is updated if a pointer
 *	to it is provided.
 *
 *----------------------------------------------------------------------
 */

static int
ReadByteArray(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    unsigned char *destPtr,
    int destSz,
    unsigned long *crcPtr)
{
    /*
     * Check to make sure the number of requested bytes are available.
     */

    if (pngPtr->strDataLen < destSz) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"unexpected end of image data", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "EARLY_END", NULL);
	return TCL_ERROR;
    }

    while (destSz) {
	int blockSz = PNG_MIN(destSz, PNG_BLOCK_SZ);

	memcpy(destPtr, pngPtr->strDataBuf, blockSz);

	pngPtr->strDataBuf += blockSz;
	pngPtr->strDataLen -= blockSz;

	if (crcPtr) {
	    *crcPtr = Tcl_ZlibCRC32(*crcPtr, destPtr, blockSz);
	}

	destPtr += blockSz;
	destSz -= blockSz;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ReadData --
 *
 *	This function is invoked to read the specified number of bytes from
 *	the image file or data. It is a wrapper around the choice of byte
 *	array Tcl_Obj or Tcl_Channel which depends on whether the image data
 *	is coming from a file or -data.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if an I/O error occurs.
 *
 * Side effects:
 *	The file position will change. The running CRC is updated if a pointer
 *	to it is provided.
 *
 *----------------------------------------------------------------------
 */

static int
ReadData(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    unsigned char *destPtr,
    int destSz,
    unsigned long *crcPtr)
{
    if (pngPtr->base64Data) {
	return ReadBase64(interp, pngPtr, destPtr, destSz, crcPtr);
    } else if (pngPtr->strDataBuf) {
	return ReadByteArray(interp, pngPtr, destPtr, destSz, crcPtr);
    }

    while (destSz) {
	int blockSz = PNG_MIN(destSz, PNG_BLOCK_SZ);

	blockSz = Tcl_Read(pngPtr->channel, (char *)destPtr, blockSz);
	if (blockSz < 0) {
	    /* TODO: failure info... */
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "channel read failed: %s", Tcl_PosixError(interp)));
	    return TCL_ERROR;
	}

	/*
	 * Update CRC, pointer, and remaining count if anything was read.
	 */

	if (blockSz) {
	    if (crcPtr) {
		*crcPtr = Tcl_ZlibCRC32(*crcPtr, destPtr, blockSz);
	    }

	    destPtr += blockSz;
	    destSz -= blockSz;
	}

	/*
	 * Check for EOF before all desired data was read.
	 */

	if (destSz && Tcl_Eof(pngPtr->channel)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "unexpected end of file", -1));
	    Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "EOF", NULL);
	    return TCL_ERROR;
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ReadInt32 --
 *
 *	This function is invoked to read a 32-bit integer in network byte
 *	order from the image data and return the value in host byte order.
 *	This is used, for example, to read the 32-bit CRC value for a chunk
 *	stored in the image file for comparison with the calculated CRC value.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if an I/O error occurs.
 *
 * Side effects:
 *	The file position will change. The running CRC is updated if a pointer
 *	to it is provided.
 *
 *----------------------------------------------------------------------
 */

static inline int
ReadInt32(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    unsigned long *resultPtr,
    unsigned long *crcPtr)
{
    unsigned char p[4];

    if (ReadData(interp, pngPtr, p, 4, crcPtr) == TCL_ERROR) {
	return TCL_ERROR;
    }

    *resultPtr = PNG_INT32(p[0], p[1], p[2], p[3]);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CheckCRC --
 *
 *	This function is reads the final 4-byte integer CRC from a chunk and
 *	compares it to the running CRC calculated over the chunk type and data
 *	fields.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if an I/O error or CRC mismatch occurs.
 *
 * Side effects:
 *	The file position will change.
 *
 *----------------------------------------------------------------------
 */

static inline int
CheckCRC(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    unsigned long calculated)
{
    unsigned long chunked;

    /*
     * Read the CRC field at the end of the chunk.
     */

    if (ReadInt32(interp, pngPtr, &chunked, NULL) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Compare the read CRC to what we calculate to make sure they match.
     */

    if (calculated != chunked) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("CRC check failed", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "CRC", NULL);
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SkipChunk --
 *
 *	This function is used to skip a PNG chunk that is not used by this
 *	implementation. Given the input stream has had the chunk length and
 *	chunk type fields already read, this function will read the number of
 *	bytes indicated by the chunk length, plus four for the CRC, and will
 *	verify that CRC is correct for the skipped data.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if an I/O error or CRC mismatch occurs.
 *
 * Side effects:
 *	The file position will change.
 *
 *----------------------------------------------------------------------
 */

static int
SkipChunk(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    int chunkSz,
    unsigned long crc)
{
    unsigned char buffer[PNG_BLOCK_SZ];

    /*
     * Skip data in blocks until none is left. Read up to PNG_BLOCK_SZ bytes
     * at a time, rather than trusting the claimed chunk size, which may not
     * be trustworthy.
     */

    while (chunkSz) {
	int blockSz = PNG_MIN(chunkSz, PNG_BLOCK_SZ);

	if (ReadData(interp, pngPtr, buffer, blockSz, &crc) == TCL_ERROR) {
	    return TCL_ERROR;
	}

	chunkSz -= blockSz;
    }

    if (CheckCRC(interp, pngPtr, crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 * 4.3. Summary of standard chunks
 *
 * This table summarizes some properties of the standard chunk types.
 *
 *	Critical chunks (must appear in this order, except PLTE is optional):
 *
 *		Name   Multiple	 Ordering constraints OK?
 *
 *		IHDR	No	 Must be first
 *		PLTE	No	 Before IDAT
 *		IDAT	Yes	 Multiple IDATs must be consecutive
 *		IEND	No	 Must be last
 *
 *	Ancillary chunks (need not appear in this order):
 *
 *		Name   Multiple	 Ordering constraints OK?
 *
 *		cHRM	No	 Before PLTE and IDAT
 *		gAMA	No	 Before PLTE and IDAT
 *		iCCP	No	 Before PLTE and IDAT
 *		sBIT	No	 Before PLTE and IDAT
 *		sRGB	No	 Before PLTE and IDAT
 *		bKGD	No	 After PLTE; before IDAT
 *		hIST	No	 After PLTE; before IDAT
 *		tRNS	No	 After PLTE; before IDAT
 *		pHYs	No	 Before IDAT
 *		sPLT	Yes	 Before IDAT
 *		tIME	No	 None
 *		iTXt	Yes	 None
 *		tEXt	Yes	 None
 *		zTXt	Yes	 None
 *
 *	[From the PNG specification.]
 */

/*
 *----------------------------------------------------------------------
 *
 * ReadChunkHeader --
 *
 *	This function is used at the start of each chunk to extract the
 *	four-byte chunk length and four-byte chunk type fields. It will
 *	continue reading until it finds a chunk type that is handled by this
 *	implementation, checking the CRC of any chunks it skips.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if an I/O error occurs or an unknown critical
 *	chunk type is encountered.
 *
 * Side effects:
 *	The file position will change. The running CRC is updated.
 *
 *----------------------------------------------------------------------
 */

static int
ReadChunkHeader(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    int *sizePtr,
    unsigned long *typePtr,
    unsigned long *crcPtr)
{
    unsigned long chunkType = 0;
    int chunkSz = 0;
    unsigned long crc = 0;

    /*
     * Continue until finding a chunk type that is handled.
     */

    while (!chunkType) {
	unsigned long temp;
	unsigned char pc[4];
	int i;

	/*
	 * Read the 4-byte length field for the chunk. The length field is not
	 * included in the CRC calculation, so the running CRC must be reset
	 * afterward. Limit chunk lengths to INT_MAX, to align with the
	 * maximum size for Tcl_Read, Tcl_GetByteArrayFromObj, etc.
	 */

	if (ReadData(interp, pngPtr, pc, 4, NULL) == TCL_ERROR) {
	    return TCL_ERROR;
	}

	temp = PNG_INT32(pc[0], pc[1], pc[2], pc[3]);

	if (temp > INT_MAX) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "chunk size is out of supported range on this architecture",
		    -1));
	    Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "OUTSIZE", NULL);
	    return TCL_ERROR;
	}

	chunkSz = (int) temp;
	crc = Tcl_ZlibCRC32(0, NULL, 0);

	/*
	 * Read the 4-byte chunk type.
	 */

	if (ReadData(interp, pngPtr, pc, 4, &crc) == TCL_ERROR) {
	    return TCL_ERROR;
	}

	/*
	 * Convert it to a host-order integer for simple comparison.
	 */

	chunkType = PNG_INT32(pc[0], pc[1], pc[2], pc[3]);

	/*
	 * Check to see if this is a known/supported chunk type. Note that the
	 * PNG specs require non-critical (i.e., ancillary) chunk types that
	 * are not recognized to be ignored, rather than be treated as an
	 * error. It does, however, recommend that an unknown critical chunk
	 * type be treated as a failure.
	 *
	 * This switch/loop acts as a filter of sorts for undesired chunk
	 * types. The chunk type should still be checked elsewhere for
	 * determining it is in the correct order.
	 */

	switch (chunkType) {
	    /*
	     * These chunk types are required and/or supported.
	     */

	case CHUNK_IDAT:
	case CHUNK_IEND:
	case CHUNK_IHDR:
	case CHUNK_PLTE:
	case CHUNK_tRNS:
	    break;

	    /*
	     * These chunk types are part of the standard, but are not used by
	     * this implementation (at least not yet). Note that these are all
	     * ancillary chunks (lowercase first letter).
	     */

	case CHUNK_bKGD:
	case CHUNK_cHRM:
	case CHUNK_gAMA:
	case CHUNK_hIST:
	case CHUNK_iCCP:
	case CHUNK_iTXt:
	case CHUNK_oFFs:
	case CHUNK_pCAL:
	case CHUNK_pHYs:
	case CHUNK_sBIT:
	case CHUNK_sCAL:
	case CHUNK_sPLT:
	case CHUNK_sRGB:
	case CHUNK_tEXt:
	case CHUNK_tIME:
	case CHUNK_zTXt:
	    /*
	     * TODO: might want to check order here.
	     */

	    if (SkipChunk(interp, pngPtr, chunkSz, crc) == TCL_ERROR) {
		return TCL_ERROR;
	    }

	    chunkType = 0;
	    break;

	default:
	    /*
	     * Unknown chunk type. If it's critical, we can't continue.
	     */

	    if (!(chunkType & PNG_CF_ANCILLARY)) {
		if (chunkType & PNG_INT32(128,128,128,128)) {
		    /*
		     * No nice ASCII conversion; shouldn't happen either, but
		     * we'll be doubly careful.
		     */

		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "encountered an unsupported critical chunk type",
			    -1));
		} else {
		    char typeString[5];

		    typeString[0] = (char) ((chunkType >> 24) & 255);
		    typeString[1] = (char) ((chunkType >> 16) & 255);
		    typeString[2] = (char) ((chunkType >> 8) & 255);
		    typeString[3] = (char) (chunkType & 255);
		    typeString[4] = '\0';
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "encountered an unsupported critical chunk type"
			    " \"%s\"", typeString));
		}
		Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG",
			"UNSUPPORTED_CRITICAL", NULL);
		return TCL_ERROR;
	    }

	    /*
	     * Check to see if the chunk type has legal bytes.
	     */

	    for (i=0 ; i<4 ; i++) {
		if ((pc[i] < 65) || (pc[i] > 122) ||
			((pc[i] > 90) && (pc[i] < 97))) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "invalid chunk type", -1));
		    Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG",
			    "INVALID_CHUNK", NULL);
		    return TCL_ERROR;
		}
	    }

	    /*
	     * It seems to be an otherwise legally labelled ancillary chunk
	     * that we don't want, so skip it after at least checking its CRC.
	     */

	    if (SkipChunk(interp, pngPtr, chunkSz, crc) == TCL_ERROR) {
		return TCL_ERROR;
	    }

	    chunkType = 0;
	}
    }

    /*
     * Found a known chunk type that's handled, albiet possibly not in the
     * right order. Send back the chunk type (for further checking or
     * handling), the chunk size and the current CRC for the rest of the
     * calculation.
     */

    *typePtr = chunkType;
    *sizePtr = chunkSz;
    *crcPtr = crc;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CheckColor --
 *
 *	Do validation on color type, depth, and related information, and
 *	calculates storage requirements and offsets based on image dimensions
 *	and color.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if color information is invalid or some other
 *	failure occurs.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static int
CheckColor(
    Tcl_Interp *interp,
    PNGImage *pngPtr)
{
    int offset;

    /*
     * Verify the color type is valid and the bit depth is allowed.
     */

    switch (pngPtr->colorType) {
    case PNG_COLOR_GRAY:
	pngPtr->numChannels = 1;
	if ((1 != pngPtr->bitDepth) && (2 != pngPtr->bitDepth) &&
		(4 != pngPtr->bitDepth) && (8 != pngPtr->bitDepth) &&
		(16 != pngPtr->bitDepth)) {
	    goto unsupportedDepth;
	}
	break;

    case PNG_COLOR_RGB:
	pngPtr->numChannels = 3;
	if ((8 != pngPtr->bitDepth) && (16 != pngPtr->bitDepth)) {
	    goto unsupportedDepth;
	}
	break;

    case PNG_COLOR_PLTE:
	pngPtr->numChannels = 1;
	if ((1 != pngPtr->bitDepth) && (2 != pngPtr->bitDepth) &&
		(4 != pngPtr->bitDepth) && (8 != pngPtr->bitDepth)) {
	    goto unsupportedDepth;
	}
	break;

    case PNG_COLOR_GRAYALPHA:
	pngPtr->numChannels = 2;
	if ((8 != pngPtr->bitDepth) && (16 != pngPtr->bitDepth)) {
	    goto unsupportedDepth;
	}
	break;

    case PNG_COLOR_RGBA:
	pngPtr->numChannels = 4;
	if ((8 != pngPtr->bitDepth) && (16 != pngPtr->bitDepth)) {
	unsupportedDepth:
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "bit depth is not allowed for given color type", -1));
	    Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "BAD_DEPTH", NULL);
	    return TCL_ERROR;
	}
	break;

    default:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown color type field %d", pngPtr->colorType));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "UNKNOWN_COLOR", NULL);
	return TCL_ERROR;
    }

    /*
     * Set up the Tk photo block's pixel size and channel offsets. offset
     * array elements should already be 0 from the memset during InitPNGImage.
     */

    offset = (pngPtr->bitDepth > 8) ? 2 : 1;

    if (pngPtr->colorType & PNG_COLOR_USED) {
	pngPtr->block.pixelSize = offset * 4;
	pngPtr->block.offset[1] = offset;
	pngPtr->block.offset[2] = offset * 2;
	pngPtr->block.offset[3] = offset * 3;
    } else {
	pngPtr->block.pixelSize = offset * 2;
	pngPtr->block.offset[3] = offset;
    }

    /*
     * Calculate the block pitch, which is the number of bytes per line in the
     * image, given image width and depth of color. Make sure that it it isn't
     * larger than Tk can handle.
     */

    if (pngPtr->block.width > INT_MAX / pngPtr->block.pixelSize) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"image pitch is out of supported range on this architecture",
		-1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "PITCH", NULL);
	return TCL_ERROR;
    }

    pngPtr->block.pitch = pngPtr->block.pixelSize * pngPtr->block.width;

    /*
     * Calculate the total size of the image as represented to Tk given pitch
     * and image height. Make sure that it isn't larger than Tk can handle.
     */

    if (pngPtr->block.height > INT_MAX / pngPtr->block.pitch) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"image total size is out of supported range on this architecture",
		-1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "SIZE", NULL);
	return TCL_ERROR;
    }

    pngPtr->blockLen = pngPtr->block.height * pngPtr->block.pitch;

    /*
     * Determine number of bytes per pixel in the source for later use.
     */

    switch (pngPtr->colorType) {
    case PNG_COLOR_GRAY:
	pngPtr->bytesPerPixel = (pngPtr->bitDepth > 8) ? 2 : 1;
	break;
    case PNG_COLOR_RGB:
	pngPtr->bytesPerPixel = (pngPtr->bitDepth > 8) ? 6 : 3;
	break;
    case PNG_COLOR_PLTE:
	pngPtr->bytesPerPixel = 1;
	break;
    case PNG_COLOR_GRAYALPHA:
	pngPtr->bytesPerPixel = (pngPtr->bitDepth > 8) ? 4 : 2;
	break;
    case PNG_COLOR_RGBA:
	pngPtr->bytesPerPixel = (pngPtr->bitDepth > 8) ? 8 : 4;
	break;
    default:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown color type %d", pngPtr->colorType));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "UNKNOWN_COLOR", NULL);
	return TCL_ERROR;
    }

    /*
     * Calculate scale factor for bit depths less than 8, in order to adjust
     * them to a minimum of 8 bits per pixel in the Tk image.
     */

    if (pngPtr->bitDepth < 8) {
	pngPtr->bitScale = 255 / (int)(pow(2, pngPtr->bitDepth) - 1);
    } else {
	pngPtr->bitScale = 1;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ReadIHDR --
 *
 *	This function reads the PNG header from the beginning of a PNG file
 *	and returns the dimensions of the image.
 *
 * Results:
 *	The return value is 1 if file "f" appears to start with a valid PNG
 *	header, 0 otherwise. If the header is valid, then *widthPtr and
 *	*heightPtr are modified to hold the dimensions of the image.
 *
 * Side effects:
 *	The access position in f advances.
 *
 *----------------------------------------------------------------------
 */

static int
ReadIHDR(
    Tcl_Interp *interp,
    PNGImage *pngPtr)
{
    unsigned char sigBuf[PNG_SIG_SZ];
    unsigned long chunkType;
    int chunkSz;
    unsigned long crc;
    unsigned long width, height;
    int mismatch;

    /*
     * Read the appropriate number of bytes for the PNG signature.
     */

    if (ReadData(interp, pngPtr, sigBuf, PNG_SIG_SZ, NULL) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Compare the read bytes to the expected signature.
     */

    mismatch = memcmp(sigBuf, pngSignature, PNG_SIG_SZ);

    /*
     * If reading from string, reset position and try base64 decode.
     */

    if (mismatch && pngPtr->strDataBuf) {
	pngPtr->strDataBuf = Tcl_GetByteArrayFromObj(pngPtr->objDataPtr,
		&pngPtr->strDataLen);
	pngPtr->base64Data = pngPtr->strDataBuf;

	if (ReadData(interp, pngPtr, sigBuf, PNG_SIG_SZ, NULL) == TCL_ERROR) {
	    return TCL_ERROR;
	}

	mismatch = memcmp(sigBuf, pngSignature, PNG_SIG_SZ);
    }

    if (mismatch) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"data stream does not have a PNG signature", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "NO_SIG", NULL);
	return TCL_ERROR;
    }

    if (ReadChunkHeader(interp, pngPtr, &chunkSz, &chunkType,
	    &crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Read in the IHDR (header) chunk for width, height, etc.
     *
     * The first chunk in the file must be the IHDR (headr) chunk.
     */

    if (chunkType != CHUNK_IHDR) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"expected IHDR chunk type", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "NO_IHDR", NULL);
	return TCL_ERROR;
    }

    if (chunkSz != 13) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"invalid IHDR chunk size", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "BAD_IHDR", NULL);
	return TCL_ERROR;
    }

    /*
     * Read and verify the image width and height to be sure Tk can handle its
     * dimensions. The PNG specification does not permit zero-width or
     * zero-height images.
     */

    if (ReadInt32(interp, pngPtr, &width, &crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    if (ReadInt32(interp, pngPtr, &height, &crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    if (!width || !height || (width > INT_MAX) || (height > INT_MAX)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"image dimensions are invalid or beyond architecture limits",
		-1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "DIMENSIONS", NULL);
	return TCL_ERROR;
    }

    /*
     * Set height and width for the Tk photo block.
     */

    pngPtr->block.width = (int) width;
    pngPtr->block.height = (int) height;

    /*
     * Read and the Bit Depth and Color Type.
     */

    if (ReadData(interp, pngPtr, &pngPtr->bitDepth, 1, &crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    if (ReadData(interp, pngPtr, &pngPtr->colorType, 1, &crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Verify that the color type is valid, the bit depth is allowed for the
     * color type, and calculate the number of channels and pixel depth (bits
     * per pixel * channels). Also set up offsets and sizes in the Tk photo
     * block for the pixel data.
     */

    if (CheckColor(interp, pngPtr) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Only one compression method is currently defined by the standard.
     */

    if (ReadData(interp, pngPtr, &pngPtr->compression, 1, &crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    if (pngPtr->compression != PNG_COMPRESS_DEFLATE) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown compression method %d", pngPtr->compression));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "BAD_COMPRESS", NULL);
	return TCL_ERROR;
    }

    /*
     * Only one filter method is currently defined by the standard; the method
     * has five actual filter types associated with it.
     */

    if (ReadData(interp, pngPtr, &pngPtr->filter, 1, &crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    if (pngPtr->filter != PNG_FILTMETH_STANDARD) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown filter method %d", pngPtr->filter));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "BAD_FILTER", NULL);
	return TCL_ERROR;
    }

    if (ReadData(interp, pngPtr, &pngPtr->interlace, 1, &crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    switch (pngPtr->interlace) {
    case PNG_INTERLACE_NONE:
    case PNG_INTERLACE_ADAM7:
	break;

    default:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown interlace method %d", pngPtr->interlace));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "BAD_INTERLACE", NULL);
	return TCL_ERROR;
    }

    return CheckCRC(interp, pngPtr, crc);
}

/*
 *----------------------------------------------------------------------
 *
 * ReadPLTE --
 *
 *	This function reads the PLTE (indexed color palette) chunk data from
 *	the PNG file and populates the palette table in the PNGImage
 *	structure.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if an I/O error occurs or the PLTE chunk is
 *	invalid.
 *
 * Side effects:
 *	The access position in f advances.
 *
 *----------------------------------------------------------------------
 */

static int
ReadPLTE(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    int chunkSz,
    unsigned long crc)
{
    unsigned char buffer[PNG_PLTE_MAXSZ];
    int i, c;

    /*
     * This chunk is mandatory for color type 3 and forbidden for 2 and 6.
     */

    switch (pngPtr->colorType) {
    case PNG_COLOR_GRAY:
    case PNG_COLOR_GRAYALPHA:
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"PLTE chunk type forbidden for grayscale", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "PLTE_UNEXPECTED",
		NULL);
	return TCL_ERROR;

    default:
	break;
    }

    /*
     * The palette chunk contains from 1 to 256 palette entries. Each entry
     * consists of a 3-byte RGB value. It must therefore contain a non-zero
     * multiple of 3 bytes, up to 768.
     */

    if (!chunkSz || (chunkSz > PNG_PLTE_MAXSZ) || (chunkSz % 3)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"invalid palette chunk size", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "BAD_PLTE", NULL);
	return TCL_ERROR;
    }

    /*
     * Read the palette contents and stash them for later, possibly.
     */

    if (ReadData(interp, pngPtr, buffer, chunkSz, &crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    if (CheckCRC(interp, pngPtr, crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Stash away the palette entries and entry count for later mapping each
     * pixel's palette index to its color.
     */

    for (i=0, c=0 ; c<chunkSz ; i++) {
	pngPtr->palette[i].red = buffer[c++];
	pngPtr->palette[i].green = buffer[c++];
	pngPtr->palette[i].blue = buffer[c++];
    }

    pngPtr->paletteLen = i;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ReadTRNS --
 *
 *	This function reads the tRNS (transparency) chunk data from the PNG
 *	file and populates the alpha field of the palette table in the
 *	PNGImage structure or the single color transparency, as appropriate
 *	for the color type.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if an I/O error occurs or the tRNS chunk is
 *	invalid.
 *
 * Side effects:
 *	The access position in f advances.
 *
 *----------------------------------------------------------------------
 */

static int
ReadTRNS(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    int chunkSz,
    unsigned long crc)
{
    unsigned char buffer[PNG_TRNS_MAXSZ];
    int i;

    if (pngPtr->colorType & PNG_COLOR_ALPHA) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"tRNS chunk not allowed color types with a full alpha channel",
		-1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "INVALID_TRNS", NULL);
	return TCL_ERROR;
    }

    /*
     * For indexed color, there is up to one single-byte transparency value
     * per palette entry (thus a max of 256).
     */

    if (chunkSz > PNG_TRNS_MAXSZ) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"invalid tRNS chunk size", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "BAD_TRNS", NULL);
	return TCL_ERROR;
    }

    /*
     * Read in the raw transparency information.
     */

    if (ReadData(interp, pngPtr, buffer, chunkSz, &crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    if (CheckCRC(interp, pngPtr, crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    switch (pngPtr->colorType) {
    case PNG_COLOR_GRAYALPHA:
    case PNG_COLOR_RGBA:
	break;

    case PNG_COLOR_PLTE:
	/*
	 * The number of tRNS entries must be less than or equal to the number
	 * of PLTE entries, and consists of a single-byte alpha level for the
	 * corresponding PLTE entry.
	 */

	if (chunkSz > pngPtr->paletteLen) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "size of tRNS chunk is too large for the palette", -1));
	    Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "TRNS_SIZE", NULL);
	    return TCL_ERROR;
	}

	for (i=0 ; i<chunkSz ; i++) {
	    pngPtr->palette[i].alpha = buffer[i];
	}
	break;

    case PNG_COLOR_GRAY:
	/*
	 * Grayscale uses a single 2-byte gray level, which we'll store in
	 * palette index 0, since we're not using the palette.
	 */

	if (chunkSz != 2) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "invalid tRNS chunk size - must 2 bytes for grayscale",
		    -1));
	    Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "BAD_TRNS", NULL);
	    return TCL_ERROR;
	}

	/*
	 * According to the PNG specs, if the bit depth is less than 16, then
	 * only the lower byte is used.
	 */

	if (16 == pngPtr->bitDepth) {
	    pngPtr->transVal[0] = buffer[0];
	    pngPtr->transVal[1] = buffer[1];
	} else {
	    pngPtr->transVal[0] = buffer[1];
	}
	pngPtr->useTRNS = 1;
	break;

    case PNG_COLOR_RGB:
	/*
	 * TrueColor uses a single RRGGBB triplet.
	 */

	if (chunkSz != 6) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "invalid tRNS chunk size - must 6 bytes for RGB", -1));
	    Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "BAD_TRNS", NULL);
	    return TCL_ERROR;
	}

	/*
	 * According to the PNG specs, if the bit depth is less than 16, then
	 * only the lower byte is used. But the tRNS chunk still contains two
	 * bytes per channel.
	 */

	if (16 == pngPtr->bitDepth) {
	    memcpy(pngPtr->transVal, buffer, 6);
	} else {
	    pngPtr->transVal[0] = buffer[1];
	    pngPtr->transVal[1] = buffer[3];
	    pngPtr->transVal[2] = buffer[5];
	}
	pngPtr->useTRNS = 1;
	break;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Paeth --
 *
 *	Utility function for applying the Paeth filter to a pixel. The Paeth
 *	filter is a linear function of the pixel to be filtered and the pixels
 *	to the left, above, and above-left of the pixel to be unfiltered.
 *
 * Results:
 *	Result of the Paeth function for the left, above, and above-left
 *	pixels.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static inline unsigned char
Paeth(
    int a,
    int b,
    int c)
{
    int pa = abs(b - c);
    int pb = abs(a - c);
    int pc = abs(a + b - c - c);

    if ((pa <= pb) && (pa <= pc)) {
	return (unsigned char) a;
    }

    if (pb <= pc) {
	return (unsigned char) b;
    }

    return (unsigned char) c;
}

/*
 *----------------------------------------------------------------------
 *
 * UnfilterLine --
 *
 *	Applies the filter algorithm specified in first byte of a line to the
 *	line of pixels being read from a PNG image.
 *
 *	PNG specifies four filter algorithms (Sub, Up, Average, and Paeth)
 *	that combine a pixel's value with those of other pixels in the same
 *	and/or previous lines. Filtering is intended to make an image more
 *	compressible.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if the filter type is not recognized.
 *
 * Side effects:
 *	Pixel data in thisLineObj are modified.
 *
 *----------------------------------------------------------------------
 */

static int
UnfilterLine(
    Tcl_Interp *interp,
    PNGImage *pngPtr)
{
    unsigned char *thisLine =
	    Tcl_GetByteArrayFromObj(pngPtr->thisLineObj, NULL);
    unsigned char *lastLine =
	    Tcl_GetByteArrayFromObj(pngPtr->lastLineObj, NULL);

#define	PNG_FILTER_NONE		0
#define	PNG_FILTER_SUB		1
#define	PNG_FILTER_UP		2
#define	PNG_FILTER_AVG		3
#define	PNG_FILTER_PAETH	4

    switch (*thisLine) {
    case PNG_FILTER_NONE:	/* Nothing to do */
	break;
    case PNG_FILTER_SUB: {	/* Sub(x) = Raw(x) - Raw(x-bpp) */
	unsigned char *rawBpp = thisLine + 1;
	unsigned char *raw = rawBpp + pngPtr->bytesPerPixel;
	unsigned char *end = thisLine + pngPtr->phaseSize;

	while (raw < end) {
	    *raw++ += *rawBpp++;
	}
	break;
    }
    case PNG_FILTER_UP:		/* Up(x) = Raw(x) - Prior(x) */
	if (pngPtr->currentLine > startLine[pngPtr->phase]) {
	    unsigned char *prior = lastLine + 1;
	    unsigned char *raw = thisLine + 1;
	    unsigned char *end = thisLine + pngPtr->phaseSize;

	    while (raw < end) {
		*raw++ += *prior++;
	    }
	}
	break;
    case PNG_FILTER_AVG:
	/* Avg(x) = Raw(x) - floor((Raw(x-bpp)+Prior(x))/2) */
	if (pngPtr->currentLine > startLine[pngPtr->phase]) {
	    unsigned char *prior = lastLine + 1;
	    unsigned char *rawBpp = thisLine + 1;
	    unsigned char *raw = rawBpp;
	    unsigned char *end = thisLine + pngPtr->phaseSize;
	    unsigned char *end2 = raw + pngPtr->bytesPerPixel;

	    while ((raw < end2) && (raw < end)) {
		*raw++ += *prior++ / 2;
	    }

	    while (raw < end) {
		*raw++ += (unsigned char)
			(((int) *rawBpp++ + (int) *prior++) / 2);
	    }
	} else {
	    unsigned char *rawBpp = thisLine + 1;
	    unsigned char *raw = rawBpp + pngPtr->bytesPerPixel;
	    unsigned char *end = thisLine + pngPtr->phaseSize;

	    while (raw < end) {
		*raw++ += *rawBpp++ / 2;
	    }
	}
	break;
    case PNG_FILTER_PAETH:
	/* Paeth(x) = Raw(x) - PaethPredictor(Raw(x-bpp), Prior(x), Prior(x-bpp)) */
	if (pngPtr->currentLine > startLine[pngPtr->phase]) {
	    unsigned char *priorBpp = lastLine + 1;
	    unsigned char *prior = priorBpp;
	    unsigned char *rawBpp = thisLine + 1;
	    unsigned char *raw = rawBpp;
	    unsigned char *end = thisLine + pngPtr->phaseSize;
	    unsigned char *end2 = rawBpp + pngPtr->bytesPerPixel;

	    while ((raw < end) && (raw < end2)) {
		*raw++ += *prior++;
	    }

	    while (raw < end) {
		*raw++ += Paeth(*rawBpp++, *prior++, *priorBpp++);
	    }
	} else {
	    unsigned char *rawBpp = thisLine + 1;
	    unsigned char *raw = rawBpp + pngPtr->bytesPerPixel;
	    unsigned char *end = thisLine + pngPtr->phaseSize;

	    while (raw < end) {
		*raw++ += *rawBpp++;
	    }
	}
	break;
    default:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"invalid filter type %d", *thisLine));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "BAD_FILTER", NULL);
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DecodeLine --
 *
 *	Unfilters a line of pixels from the PNG source data and decodes the
 *	data into the Tk_PhotoImageBlock for later copying into the Tk image.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if the filter type is not recognized.
 *
 * Side effects:
 *	Pixel data in thisLine and block are modified and state information
 *	updated.
 *
 *----------------------------------------------------------------------
 */

static int
DecodeLine(
    Tcl_Interp *interp,
    PNGImage *pngPtr)
{
    unsigned char *pixelPtr = pngPtr->block.pixelPtr;
    int colNum = 0;		/* Current pixel column */
    unsigned char chan = 0;	/* Current channel (0..3) = (R, G, B, A) */
    unsigned char readByte = 0;	/* Current scan line byte */
    int haveBits = 0;		/* Number of bits remaining in current byte */
    unsigned char pixBits = 0;	/* Extracted bits for current channel */
    int shifts = 0;		/* Number of channels extracted from byte */
    int offset = 0;		/* Current offset into pixelPtr */
    int colStep = 1;		/* Column increment each pass */
    int pixStep = 0;		/* extra pixelPtr increment each pass */
    unsigned char lastPixel[6];
    unsigned char *p = Tcl_GetByteArrayFromObj(pngPtr->thisLineObj, NULL);

    p++;
    if (UnfilterLine(interp, pngPtr) == TCL_ERROR) {
	return TCL_ERROR;
    }
    if (pngPtr->currentLine >= pngPtr->block.height) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"PNG image data overflow"));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "DATA_OVERFLOW", NULL);
	return TCL_ERROR;
    }


    if (pngPtr->interlace) {
	switch (pngPtr->phase) {
	case 1:			/* Phase 1: */
	    colStep = 8;	/* 1 pixel per block of 8 per line */
	    break;		/* Start at column 0 */
	case 2:			/* Phase 2: */
	    colStep = 8;	/* 1 pixels per block of 8 per line */
	    colNum = 4;		/* Start at column 4 */
	    break;
	case 3:			/* Phase 3: */
	    colStep = 4;	/* 2 pixels per block of 8 per line */
	    break;		/* Start at column 0 */
	case 4:			/* Phase 4: */
	    colStep = 4;	/* 2 pixels per block of 8 per line */
	    colNum = 2;		/* Start at column 2 */
	    break;
	case 5:			/* Phase 5: */
	    colStep = 2;	/* 4 pixels per block of 8 per line */
	    break;		/* Start at column 0 */
	case 6:			/* Phase 6: */
	    colStep = 2;	/* 4 pixels per block of 8 per line */
	    colNum = 1;		/* Start at column 1 */
	    break;
				/* Phase 7: */
				/* 8 pixels per block of 8 per line */
				/* Start at column 0 */
	}
    }

    /*
     * Calculate offset into pixelPtr for the first pixel of the line.
     */

    offset = pngPtr->currentLine * pngPtr->block.pitch;

    /*
     * Adjust up for the starting pixel of the line.
     */

    offset += colNum * pngPtr->block.pixelSize;

    /*
     * Calculate the extra number of bytes to skip between columns.
     */

    pixStep = (colStep - 1) * pngPtr->block.pixelSize;

    for ( ; colNum < pngPtr->block.width ; colNum += colStep) {
	if (haveBits < (pngPtr->bitDepth * pngPtr->numChannels)) {
	    haveBits = 0;
	}

	for (chan = 0 ; chan < pngPtr->numChannels ; chan++) {
	    if (!haveBits) {
		shifts = 0;
		readByte = *p++;
		haveBits += 8;
	    }

	    if (16 == pngPtr->bitDepth) {
		pngPtr->block.pixelPtr[offset++] = readByte;

		if (pngPtr->useTRNS) {
		    lastPixel[chan * 2] = readByte;
		}

		readByte = *p++;

		if (pngPtr->useTRNS) {
		    lastPixel[(chan * 2) + 1] = readByte;
		}

		pngPtr->block.pixelPtr[offset++] = readByte;

		haveBits = 0;
		continue;
	    }

	    switch (pngPtr->bitDepth) {
	    case 1:
		pixBits = (unsigned char)((readByte >> (7-shifts)) & 0x01);
		break;
	    case 2:
		pixBits = (unsigned char)((readByte >> (6-shifts*2)) & 0x03);
		break;
	    case 4:
		pixBits = (unsigned char)((readByte >> (4-shifts*4)) & 0x0f);
		break;
	    case 8:
		pixBits = readByte;
		break;
	    }

	    if (PNG_COLOR_PLTE == pngPtr->colorType) {
		pixelPtr[offset++] = pngPtr->palette[pixBits].red;
		pixelPtr[offset++] = pngPtr->palette[pixBits].green;
		pixelPtr[offset++] = pngPtr->palette[pixBits].blue;
		pixelPtr[offset++] = pngPtr->palette[pixBits].alpha;
		chan += 2;
	    } else {
		pixelPtr[offset++] = (unsigned char)
			(pixBits * pngPtr->bitScale);

		if (pngPtr->useTRNS) {
		    lastPixel[chan] = pixBits;
		}
	    }

	    haveBits -= pngPtr->bitDepth;
	    shifts++;
	}

	/*
	 * Apply boolean transparency via tRNS data if necessary (where
	 * necessary means a tRNS chunk was provided and we're not using an
	 * alpha channel or indexed alpha).
	 */

	if ((PNG_COLOR_PLTE != pngPtr->colorType) &&
		!(pngPtr->colorType & PNG_COLOR_ALPHA)) {
	    unsigned char alpha;

	    if (pngPtr->useTRNS) {
		if (memcmp(lastPixel, pngPtr->transVal,
			pngPtr->bytesPerPixel) == 0) {
		    alpha = 0x00;
		} else {
		    alpha = 0xff;
		}
	    } else {
		alpha = 0xff;
	    }

	    pixelPtr[offset++] = alpha;

	    if (16 == pngPtr->bitDepth) {
		pixelPtr[offset++] = alpha;
	    }
	}

	offset += pixStep;
    }

    if (pngPtr->interlace) {
	/* Skip lines */

	switch (pngPtr->phase) {
	case 1: case 2: case 3:
	    pngPtr->currentLine += 8;
	    break;
	case 4: case 5:
	    pngPtr->currentLine += 4;
	    break;
	case 6: case 7:
	    pngPtr->currentLine += 2;
	    break;
	}

	/*
	 * Start the next phase if there are no more lines to do.
	 */

	if (pngPtr->currentLine >= pngPtr->block.height) {
	    unsigned long pixels = 0;

	    while ((!pixels || (pngPtr->currentLine >= pngPtr->block.height))
		    && (pngPtr->phase < 7)) {
		pngPtr->phase++;

		switch (pngPtr->phase) {
		case 2:
		    pixels = (pngPtr->block.width + 3) >> 3;
		    pngPtr->currentLine = 0;
		    break;
		case 3:
		    pixels = (pngPtr->block.width + 3) >> 2;
		    pngPtr->currentLine = 4;
		    break;
		case 4:
		    pixels = (pngPtr->block.width + 1) >> 2;
		    pngPtr->currentLine = 0;
		    break;
		case 5:
		    pixels = (pngPtr->block.width + 1) >> 1;
		    pngPtr->currentLine = 2;
		    break;
		case 6:
		    pixels = pngPtr->block.width >> 1;
		    pngPtr->currentLine = 0;
		    break;
		case 7:
		    pngPtr->currentLine = 1;
		    pixels = pngPtr->block.width;
		    break;
		}
	    }

	    if (16 == pngPtr->bitDepth) {
		pngPtr->phaseSize = 1 + (pngPtr->numChannels * pixels * 2);
	    } else {
		pngPtr->phaseSize = 1 + ((pngPtr->numChannels * pixels *
			pngPtr->bitDepth + 7) >> 3);
	    }
	}
    } else {
	pngPtr->currentLine++;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ReadIDAT --
 *
 *	This function reads the IDAT (pixel data) chunk from the PNG file to
 *	build the image. It will continue reading until all IDAT chunks have
 *	been processed or an error occurs.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if an I/O error occurs or an IDAT chunk is
 *	invalid.
 *
 * Side effects:
 *	The access position in f advances. Memory may be allocated by zlib
 *	through PNGZAlloc.
 *
 *----------------------------------------------------------------------
 */

static int
ReadIDAT(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    int chunkSz,
    unsigned long crc)
{
    /*
     * Process IDAT contents until there is no more in this chunk.
     */

    while (chunkSz && !Tcl_ZlibStreamEof(pngPtr->stream)) {
	int len1, len2;

	/*
	 * Read another block of input into the zlib stream if data remains.
	 */

	if (chunkSz) {
	    Tcl_Obj *inputObj = NULL;
	    int blockSz = PNG_MIN(chunkSz, PNG_BLOCK_SZ);
	    unsigned char *inputPtr = NULL;

	    /*
	     * Check for end of zlib stream.
	     */

	    if (Tcl_ZlibStreamEof(pngPtr->stream)) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"extra data after end of zlib stream", -1));
		Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "EXTRA_DATA",
			NULL);
		return TCL_ERROR;
	    }

	    inputObj = Tcl_NewObj();
	    Tcl_IncrRefCount(inputObj);
	    inputPtr = Tcl_SetByteArrayLength(inputObj, blockSz);

	    /*
	     * Read the next bit of IDAT chunk data, up to read buffer size.
	     */

	    if (ReadData(interp, pngPtr, inputPtr, blockSz,
		    &crc) == TCL_ERROR) {
		Tcl_DecrRefCount(inputObj);
		return TCL_ERROR;
	    }

	    chunkSz -= blockSz;

	    Tcl_ZlibStreamPut(pngPtr->stream, inputObj, TCL_ZLIB_NO_FLUSH);
	    Tcl_DecrRefCount(inputObj);
	}

	/*
	 * Inflate, processing each output buffer's worth as a line of pixels,
	 * until we cannot fill the buffer any more.
	 */

    getNextLine:
	Tcl_GetByteArrayFromObj(pngPtr->thisLineObj, &len1);
	if (Tcl_ZlibStreamGet(pngPtr->stream, pngPtr->thisLineObj,
		pngPtr->phaseSize - len1) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	Tcl_GetByteArrayFromObj(pngPtr->thisLineObj, &len2);

	if (len2 == pngPtr->phaseSize) {
	    if (pngPtr->phase > 7) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"extra data after final scan line of final phase",
			-1));
		Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "EXTRA_DATA",
			NULL);
		return TCL_ERROR;
	    }

	    if (DecodeLine(interp, pngPtr) == TCL_ERROR) {
		return TCL_ERROR;
	    }

	    /*
	     * Swap the current/last lines so that we always have the last
	     * line processed available, which is necessary for filtering.
	     */

	    {
		Tcl_Obj *temp = pngPtr->lastLineObj;

		pngPtr->lastLineObj = pngPtr->thisLineObj;
		pngPtr->thisLineObj = temp;
	    }
	    Tcl_SetByteArrayLength(pngPtr->thisLineObj, 0);

	    /*
	     * Try to read another line of pixels out of the buffer
	     * immediately, but don't allow write past end of block.
	     */

	    if (pngPtr->currentLine < pngPtr->block.height) {
		goto getNextLine;
	    }

	}

	/*
	 * Got less than a whole buffer-load of pixels. Either we're going to
	 * be getting more data from the next IDAT, or we've done what we can
	 * here.
	 */
    }

    /*
     * Ensure that if we've got to the end of the compressed data, we've
     * also got to the end of the compressed stream. This sanity check is
     * enforced by most PNG readers.
     */

    if (chunkSz != 0) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"compressed data after stream finalize in PNG data", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "EXTRA_DATA", NULL);
	return TCL_ERROR;
    }

    return CheckCRC(interp, pngPtr, crc);
}

/*
 *----------------------------------------------------------------------
 *
 * ApplyAlpha --
 *
 *	Applies an overall alpha value to a complete image that has been read.
 *	This alpha value is specified using the -format option to [image
 *	create photo].
 *
 * Results:
 *	N/A
 *
 * Side effects:
 *	The access position in f may change.
 *
 *----------------------------------------------------------------------
 */

static void
ApplyAlpha(
    PNGImage *pngPtr)
{
    if (pngPtr->alpha != 1.0) {
	register unsigned char *p = pngPtr->block.pixelPtr;
	unsigned char *endPtr = p + pngPtr->blockLen;
	int offset = pngPtr->block.offset[3];

	p += offset;

	if (16 == pngPtr->bitDepth) {
	    register unsigned int channel;

	    while (p < endPtr) {
		channel = (unsigned int)
			(((p[0] << 8) | p[1]) * pngPtr->alpha);

		*p++ = (unsigned char) (channel >> 8);
		*p++ = (unsigned char) (channel & 0xff);

		p += offset;
	    }
	} else {
	    while (p < endPtr) {
		p[0] = (unsigned char) (pngPtr->alpha * p[0]);
		p += 1 + offset;
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ParseFormat --
 *
 *	This function parses the -format string that can be specified to the
 *	[image create photo] command to extract options for postprocessing of
 *	loaded images. Currently, this just allows specifying and applying an
 *	overall alpha value to the loaded image (for example, to make it
 *	entirely 50% as transparent as the actual image file).
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if the format specification is invalid.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static int
ParseFormat(
    Tcl_Interp *interp,
    Tcl_Obj *fmtObj,
    PNGImage *pngPtr)
{
    Tcl_Obj **objv = NULL;
    int objc = 0;
    static const char *const fmtOptions[] = {
	"-alpha", NULL
    };
    enum fmtOptions {
	OPT_ALPHA
    };

    /*
     * Extract elements of format specification as a list.
     */

    if (fmtObj &&
	    Tcl_ListObjGetElements(interp, fmtObj, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }

    for (; objc>0 ; objc--, objv++) {
	int optIndex;

	/*
	 * Ignore the "png" part of the format specification.
	 */

	if (!strcasecmp(Tcl_GetString(objv[0]), "png")) {
	    continue;
	}

	if (Tcl_GetIndexFromObjStruct(interp, objv[0], fmtOptions,
		sizeof(char *), "option", 0, &optIndex) == TCL_ERROR) {
	    return TCL_ERROR;
	}

	if (objc < 2) {
	    Tcl_WrongNumArgs(interp, 1, objv, "value");
	    return TCL_ERROR;
	}

	objc--;
	objv++;

	switch ((enum fmtOptions) optIndex) {
	case OPT_ALPHA:
	    if (Tcl_GetDoubleFromObj(interp, objv[0],
		    &pngPtr->alpha) == TCL_ERROR) {
		return TCL_ERROR;
	    }

	    if ((pngPtr->alpha < 0.0) || (pngPtr->alpha > 1.0)) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"-alpha value must be between 0.0 and 1.0", -1));
		Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "BAD_ALPHA",
			NULL);
		return TCL_ERROR;
	    }
	    break;
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DecodePNG --
 *
 *	This function handles the entirety of reading a PNG file (or data)
 *	from the first byte to the last.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if an I/O error occurs or any problems are
 *	detected in the PNG file.
 *
 * Side effects:
 *	The access position in f advances. Memory may be allocated and image
 *	dimensions and contents may change.
 *
 *----------------------------------------------------------------------
 */

static int
DecodePNG(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    Tcl_Obj *fmtObj,
    Tk_PhotoHandle imageHandle,
    int destX,
    int destY)
{
    unsigned long chunkType;
    int chunkSz;
    unsigned long crc;

    /*
     * Parse the PNG signature and IHDR (header) chunk.
     */

    if (ReadIHDR(interp, pngPtr) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Extract alpha value from -format object, if specified.
     */

    if (ParseFormat(interp, fmtObj, pngPtr) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * The next chunk may either be a PLTE (Palette) chunk or the first of at
     * least one IDAT (data) chunks. It could also be one of a number of
     * ancillary chunks, but those are skipped for us by the switch in
     * ReadChunkHeader().
     *
     * PLTE is mandatory for color type 3 and forbidden for 2 and 6
     */

    if (ReadChunkHeader(interp, pngPtr, &chunkSz, &chunkType,
	    &crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    if (CHUNK_PLTE == chunkType) {
	/*
	 * Finish parsing the PLTE chunk.
	 */

	if (ReadPLTE(interp, pngPtr, chunkSz, crc) == TCL_ERROR) {
	    return TCL_ERROR;
	}

	/*
	 * Begin the next chunk.
	 */

	if (ReadChunkHeader(interp, pngPtr, &chunkSz, &chunkType,
		&crc) == TCL_ERROR) {
	    return TCL_ERROR;
	}
    } else if (PNG_COLOR_PLTE == pngPtr->colorType) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"PLTE chunk required for indexed color", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "NEED_PLTE", NULL);
	return TCL_ERROR;
    }

    /*
     * The next chunk may be a tRNS (palette transparency) chunk, depending on
     * the color type. It must come after the PLTE chunk and before the IDAT
     * chunk, but can be present if there is no PLTE chunk because it can be
     * used for Grayscale and TrueColor in lieu of an alpha channel.
     */

    if (CHUNK_tRNS == chunkType) {
	/*
	 * Finish parsing the tRNS chunk.
	 */

	if (ReadTRNS(interp, pngPtr, chunkSz, crc) == TCL_ERROR) {
	    return TCL_ERROR;
	}

	/*
	 * Begin the next chunk.
	 */

	if (ReadChunkHeader(interp, pngPtr, &chunkSz, &chunkType,
		&crc) == TCL_ERROR) {
	    return TCL_ERROR;
	}
    }

    /*
     * Other ancillary chunk types could appear here, but for now we're only
     * interested in IDAT. The others should have been skipped.
     */

    if (chunkType != CHUNK_IDAT) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"at least one IDAT chunk is required", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "NEED_IDAT", NULL);
	return TCL_ERROR;
    }

    /*
     * Expand the photo size (if not set by the user) to provide enough space
     * for the image being parsed. It does not matter if width or height wrap
     * to negative here: Tk will not shrink the image.
     */

    if (Tk_PhotoExpand(interp, imageHandle, destX + pngPtr->block.width,
	    destY + pngPtr->block.height) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * A scan line consists of one byte for a filter type, plus the number of
     * bits per color sample times the number of color samples per pixel.
     */

    if (pngPtr->block.width > ((INT_MAX - 1) / (pngPtr->numChannels * 2))) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"line size is out of supported range on this architecture",
		-1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "LINE_SIZE", NULL);
	return TCL_ERROR;
    }

    if (16 == pngPtr->bitDepth) {
	pngPtr->lineSize = 1 + (pngPtr->numChannels * pngPtr->block.width*2);
    } else {
	pngPtr->lineSize = 1 + ((pngPtr->numChannels * pngPtr->block.width) /
		(8 / pngPtr->bitDepth));
	if (pngPtr->block.width % (8 / pngPtr->bitDepth)) {
	    pngPtr->lineSize++;
	}
    }

    /*
     * Allocate space for decoding the scan lines.
     */

    pngPtr->lastLineObj = Tcl_NewObj();
    Tcl_IncrRefCount(pngPtr->lastLineObj);
    pngPtr->thisLineObj = Tcl_NewObj();
    Tcl_IncrRefCount(pngPtr->thisLineObj);

    pngPtr->block.pixelPtr = attemptckalloc(pngPtr->blockLen);
    if (!pngPtr->block.pixelPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"memory allocation failed", -1));
	Tcl_SetErrorCode(interp, "TK", "MALLOC", NULL);
	return TCL_ERROR;
    }

    /*
     * Determine size of the first phase if interlaced. Phase size should
     * always be <= line size, so probably not necessary to check for
     * arithmetic overflow here: should be covered by line size check.
     */

    if (pngPtr->interlace) {
	/*
	 * Only one pixel per block of 8 per line in the first phase.
	 */

	unsigned int pixels = (pngPtr->block.width + 7) >> 3;

	pngPtr->phase = 1;
	if (16 == pngPtr->bitDepth) {
	    pngPtr->phaseSize = 1 + pngPtr->numChannels*pixels*2;
	} else {
	    pngPtr->phaseSize = 1 +
		    ((pngPtr->numChannels*pixels*pngPtr->bitDepth + 7) >> 3);
	}
    } else {
	pngPtr->phaseSize = pngPtr->lineSize;
    }

    /*
     * All of the IDAT (data) chunks must be consecutive.
     */

    while (CHUNK_IDAT == chunkType) {
	if (ReadIDAT(interp, pngPtr, chunkSz, crc) == TCL_ERROR) {
	    return TCL_ERROR;
	}

	if (ReadChunkHeader(interp, pngPtr, &chunkSz, &chunkType,
		&crc) == TCL_ERROR) {
	    return TCL_ERROR;
	}
    }

    /*
     * Ensure that we've got to the end of the compressed stream now that
     * there are no more IDAT segments. This sanity check is enforced by most
     * PNG readers.
     */

    if (!Tcl_ZlibStreamEof(pngPtr->stream)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"unfinalized data stream in PNG data", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "EXTRA_DATA", NULL);
	return TCL_ERROR;
    }

    /*
     * Now skip the remaining chunks which we're also not interested in.
     */

    while (CHUNK_IEND != chunkType) {
	if (SkipChunk(interp, pngPtr, chunkSz, crc) == TCL_ERROR) {
	    return TCL_ERROR;
	}

	if (ReadChunkHeader(interp, pngPtr, &chunkSz, &chunkType,
		&crc) == TCL_ERROR) {
	    return TCL_ERROR;
	}
    }

    /*
     * Got the IEND (end of image) chunk. Do some final checks...
     */

    if (chunkSz) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"IEND chunk contents must be empty", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "BAD_IEND", NULL);
	return TCL_ERROR;
    }

    /*
     * Check the CRC on the IEND chunk.
     */

    if (CheckCRC(interp, pngPtr, crc) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * TODO: verify that nothing else comes after the IEND chunk, or do we
     * really care?
     */

#if 0
    if (ReadData(interp, pngPtr, &c, 1, NULL) != TCL_ERROR) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"extra data following IEND chunk", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "BAD_IEND", NULL);
	return TCL_ERROR;
    }
#endif

    /*
     * Apply overall image alpha if specified.
     */

    ApplyAlpha(pngPtr);

    /*
     * Copy the decoded image block into the Tk photo image.
     */

    if (Tk_PhotoPutBlock(interp, imageHandle, &pngPtr->block, destX, destY,
	    pngPtr->block.width, pngPtr->block.height,
	    TK_PHOTO_COMPOSITE_SET) == TCL_ERROR) {
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FileMatchPNG --
 *
 *	This function is invoked by the photo image type to see if a file
 *	contains image data in PNG format.
 *
 * Results:
 *	The return value is 1 if the first characters in file f look like PNG
 *	data, and 0 otherwise.
 *
 * Side effects:
 *	The access position in f may change.
 *
 *----------------------------------------------------------------------
 */

static int
FileMatchPNG(
    Tcl_Channel chan,
    const char *fileName,
    Tcl_Obj *fmtObj,
    int *widthPtr,
    int *heightPtr,
    Tcl_Interp *interp)
{
    PNGImage png;
    int match = 0;

    InitPNGImage(NULL, &png, chan, NULL, TCL_ZLIB_STREAM_INFLATE);

    if (ReadIHDR(interp, &png) == TCL_OK) {
	*widthPtr = png.block.width;
	*heightPtr = png.block.height;
	match = 1;
    }

    CleanupPNGImage(&png);

    return match;
}

/*
 *----------------------------------------------------------------------
 *
 * FileReadPNG --
 *
 *	This function is called by the photo image type to read PNG format
 *	data from a file and write it into a given photo image.
 *
 * Results:
 *	A standard TCL completion code. If TCL_ERROR is returned then an error
 *	message is left in the interp's result.
 *
 * Side effects:
 *	The access position in file f is changed, and new data is added to the
 *	image given by imageHandle.
 *
 *----------------------------------------------------------------------
 */

static int
FileReadPNG(
    Tcl_Interp *interp,
    Tcl_Channel chan,
    const char *fileName,
    Tcl_Obj *fmtObj,
    Tk_PhotoHandle imageHandle,
    int destX,
    int destY,
    int width,
    int height,
    int srcX,
    int srcY)
{
    PNGImage png;
    int result = TCL_ERROR;

    result = InitPNGImage(interp, &png, chan, NULL, TCL_ZLIB_STREAM_INFLATE);

    if (TCL_OK == result) {
	result = DecodePNG(interp, &png, fmtObj, imageHandle, destX, destY);
    }

    CleanupPNGImage(&png);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * StringMatchPNG --
 *
 *	This function is invoked by the photo image type to see if an object
 *	contains image data in PNG format.
 *
 * Results:
 *	The return value is 1 if the first characters in the data are like PNG
 *	data, and 0 otherwise.
 *
 * Side effects:
 *	The size of the image is placed in widthPre and heightPtr.
 *
 *----------------------------------------------------------------------
 */

static int
StringMatchPNG(
    Tcl_Obj *pObjData,
    Tcl_Obj *fmtObj,
    int *widthPtr,
    int *heightPtr,
    Tcl_Interp *interp)
{
    PNGImage png;
    int match = 0;

    InitPNGImage(NULL, &png, NULL, pObjData, TCL_ZLIB_STREAM_INFLATE);

    png.strDataBuf = Tcl_GetByteArrayFromObj(pObjData, &png.strDataLen);

    if (ReadIHDR(interp, &png) == TCL_OK) {
	*widthPtr = png.block.width;
	*heightPtr = png.block.height;
	match = 1;
    }

    CleanupPNGImage(&png);
    return match;
}

/*
 *----------------------------------------------------------------------
 *
 * StringReadPNG --
 *
 *	This function is called by the photo image type to read PNG format
 *	data from an object and give it to the photo image.
 *
 * Results:
 *	A standard TCL completion code. If TCL_ERROR is returned then an error
 *	message is left in the interp's result.
 *
 * Side effects:
 *	New data is added to the image given by imageHandle.
 *
 *----------------------------------------------------------------------
 */

static int
StringReadPNG(
    Tcl_Interp *interp,
    Tcl_Obj *pObjData,
    Tcl_Obj *fmtObj,
    Tk_PhotoHandle imageHandle,
    int destX,
    int destY,
    int width,
    int height,
    int srcX,
    int srcY)
{
    PNGImage png;
    int result = TCL_ERROR;

    result = InitPNGImage(interp, &png, NULL, pObjData,
	    TCL_ZLIB_STREAM_INFLATE);

    if (TCL_OK == result) {
	result = DecodePNG(interp, &png, fmtObj, imageHandle, destX, destY);
    }

    CleanupPNGImage(&png);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * WriteData --
 *
 *	This function writes a bytes from a buffer out to the PNG image.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if the write fails.
 *
 * Side effects:
 *	File or buffer will be modified.
 *
 *----------------------------------------------------------------------
 */

static int
WriteData(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    const unsigned char *srcPtr,
    int srcSz,
    unsigned long *crcPtr)
{
    if (!srcPtr || !srcSz) {
	return TCL_OK;
    }

    if (crcPtr) {
	*crcPtr = Tcl_ZlibCRC32(*crcPtr, srcPtr, srcSz);
    }

    /*
     * TODO: is Tcl_AppendObjToObj faster here? i.e., does Tcl join the
     * objects immediately or store them in a multi-object rep?
     */

    if (pngPtr->objDataPtr) {
	int objSz;
	unsigned char *destPtr;

	Tcl_GetByteArrayFromObj(pngPtr->objDataPtr, &objSz);

	if (objSz > INT_MAX - srcSz) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "image too large to store completely in byte array", -1));
	    Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "TOO_LARGE", NULL);
	    return TCL_ERROR;
	}

	destPtr = Tcl_SetByteArrayLength(pngPtr->objDataPtr, objSz + srcSz);

	if (!destPtr) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "memory allocation failed", -1));
	    Tcl_SetErrorCode(interp, "TK", "MALLOC", NULL);
	    return TCL_ERROR;
	}

	memcpy(destPtr+objSz, srcPtr, srcSz);
    } else if (Tcl_Write(pngPtr->channel, (const char *) srcPtr, srcSz) < 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"write to channel failed: %s", Tcl_PosixError(interp)));
	return TCL_ERROR;
    }

    return TCL_OK;
}

static inline int
WriteByte(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    unsigned char c,
    unsigned long *crcPtr)
{
    return WriteData(interp, pngPtr, &c, 1, crcPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * WriteInt32 --
 *
 *	This function writes a 32-bit integer value out to the PNG image as
 *	four bytes in network byte order.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if the write fails.
 *
 * Side effects:
 *	File or buffer will be modified.
 *
 *----------------------------------------------------------------------
 */

static inline int
WriteInt32(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    unsigned long l,
    unsigned long *crcPtr)
{
    unsigned char pc[4];

    pc[0] = (unsigned char) ((l & 0xff000000) >> 24);
    pc[1] = (unsigned char) ((l & 0x00ff0000) >> 16);
    pc[2] = (unsigned char) ((l & 0x0000ff00) >> 8);
    pc[3] = (unsigned char) ((l & 0x000000ff) >> 0);

    return WriteData(interp, pngPtr, pc, 4, crcPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * WriteChunk --
 *
 *	Writes a complete chunk to the PNG image, including chunk type,
 *	length, contents, and CRC.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if the write fails.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static inline int
WriteChunk(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    unsigned long chunkType,
    const unsigned char *dataPtr,
    int dataSize)
{
    unsigned long crc = Tcl_ZlibCRC32(0, NULL, 0);
    int result = TCL_OK;

    /*
     * Write the length field for the chunk.
     */

    result = WriteInt32(interp, pngPtr, dataSize, NULL);

    /*
     * Write the Chunk Type.
     */

    if (TCL_OK == result) {
	result = WriteInt32(interp, pngPtr, chunkType, &crc);
    }

    /*
     * Write the contents (if any).
     */

    if (TCL_OK == result) {
	result = WriteData(interp, pngPtr, dataPtr, dataSize, &crc);
    }

    /*
     * Write out the CRC at the end of the chunk.
     */

    if (TCL_OK == result) {
	result = WriteInt32(interp, pngPtr, crc, NULL);
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * WriteIHDR --
 *
 *	This function writes the PNG header at the beginning of a PNG file,
 *	which includes information such as dimensions and color type.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if the write fails.
 *
 * Side effects:
 *	File or buffer will be modified.
 *
 *----------------------------------------------------------------------
 */

static int
WriteIHDR(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    Tk_PhotoImageBlock *blockPtr)
{
    unsigned long crc = Tcl_ZlibCRC32(0, NULL, 0);
    int result = TCL_OK;

    /*
     * The IHDR (header) chunk has a fixed size of 13 bytes.
     */

    result = WriteInt32(interp, pngPtr, 13, NULL);

    /*
     * Write the IHDR Chunk Type.
     */

    if (TCL_OK == result) {
	result = WriteInt32(interp, pngPtr, CHUNK_IHDR, &crc);
    }

    /*
     * Write the image width, height.
     */

    if (TCL_OK == result) {
	result = WriteInt32(interp, pngPtr, (unsigned long) blockPtr->width,
		&crc);
    }

    if (TCL_OK == result) {
	result = WriteInt32(interp, pngPtr, (unsigned long) blockPtr->height,
		&crc);
    }

    /*
     * Write bit depth. Although the PNG format supports 16 bits per channel,
     * Tk supports only 8 in the internal representation, which blockPtr
     * points to.
     */

    if (TCL_OK == result) {
	result = WriteByte(interp, pngPtr, 8, &crc);
    }

    /*
     * Write out the color type, previously determined.
     */

    if (TCL_OK == result) {
	result = WriteByte(interp, pngPtr, pngPtr->colorType, &crc);
    }

    /*
     * Write compression method (only one method is defined).
     */

    if (TCL_OK == result) {
	result = WriteByte(interp, pngPtr, PNG_COMPRESS_DEFLATE, &crc);
    }

    /*
     * Write filter method (only one method is defined).
     */

    if (TCL_OK == result) {
	result = WriteByte(interp, pngPtr, PNG_FILTMETH_STANDARD, &crc);
    }

    /*
     * Write interlace method as not interlaced.
     *
     * TODO: support interlace through -format?
     */

    if (TCL_OK == result) {
	result = WriteByte(interp, pngPtr, PNG_INTERLACE_NONE, &crc);
    }

    /*
     * Write out the CRC at the end of the chunk.
     */

    if (TCL_OK == result) {
	result = WriteInt32(interp, pngPtr, crc, NULL);
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * WriteIDAT --
 *
 *	Writes the IDAT (data) chunk to the PNG image, containing the pixel
 *	channel data. Currently, image lines are not filtered and writing
 *	interlaced pixels is not supported.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if the write fails.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static int
WriteIDAT(
    Tcl_Interp *interp,
    PNGImage *pngPtr,
    Tk_PhotoImageBlock *blockPtr)
{
    int rowNum, flush = TCL_ZLIB_NO_FLUSH, outputSize, result;
    Tcl_Obj *outputObj;
    unsigned char *outputBytes;

    /*
     * Filter and compress each row one at a time.
     */

    for (rowNum=0 ; rowNum < blockPtr->height ; rowNum++) {
	int colNum;
	unsigned char *srcPtr, *destPtr;

	srcPtr = blockPtr->pixelPtr + (rowNum * blockPtr->pitch);
	destPtr = Tcl_SetByteArrayLength(pngPtr->thisLineObj,
		pngPtr->lineSize);

	/*
	 * TODO: use Paeth filtering.
	 */

	*destPtr++ = PNG_FILTER_NONE;

	/*
	 * Copy each pixel into the destination buffer after the filter type
	 * before filtering.
	 */

	for (colNum = 0 ; colNum < blockPtr->width ; colNum++) {
	    /*
	     * Copy red or gray channel.
	     */

	    *destPtr++ = srcPtr[blockPtr->offset[0]];

	    /*
	     * If not grayscale, copy the green and blue channels.
	     */

	    if (pngPtr->colorType & PNG_COLOR_USED) {
		*destPtr++ = srcPtr[blockPtr->offset[1]];
		*destPtr++ = srcPtr[blockPtr->offset[2]];
	    }

	    /*
	     * Copy the alpha channel, if used.
	     */

	    if (pngPtr->colorType & PNG_COLOR_ALPHA) {
		*destPtr++ = srcPtr[blockPtr->offset[3]];
	    }

	    /*
	     * Point to the start of the next pixel.
	     */

	    srcPtr += blockPtr->pixelSize;
	}

	/*
	 * Compress the line of pixels into the destination. If this is the
	 * last line, finalize the compressor at the same time. Note that this
	 * can't be just a flush; that leads to a file that some PNG readers
	 * choke on. [Bug 2984787]
	 */

	if (rowNum + 1 == blockPtr->height) {
	    flush = TCL_ZLIB_FINALIZE;
	}
	if (Tcl_ZlibStreamPut(pngPtr->stream, pngPtr->thisLineObj,
		flush) != TCL_OK) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "deflate() returned error", -1));
	    Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "DEFLATE", NULL);
	    return TCL_ERROR;
	}

	/*
	 * Swap line buffers to keep the last around for filtering next.
	 */

	{
	    Tcl_Obj *temp = pngPtr->lastLineObj;

	    pngPtr->lastLineObj = pngPtr->thisLineObj;
	    pngPtr->thisLineObj = temp;
	}
    }

    /*
     * Now get the compressed data and write it as one big IDAT chunk.
     */

    outputObj = Tcl_NewObj();
    (void) Tcl_ZlibStreamGet(pngPtr->stream, outputObj, -1);
    outputBytes = Tcl_GetByteArrayFromObj(outputObj, &outputSize);
    result = WriteChunk(interp, pngPtr, CHUNK_IDAT, outputBytes, outputSize);
    Tcl_DecrRefCount(outputObj);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * WriteExtraChunks --
 *
 *	Writes an sBIT and a tEXt chunks to the PNG image, describing a bunch
 *	of not very important metadata that many readers seem to need anyway.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if the write fails.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static int
WriteExtraChunks(
    Tcl_Interp *interp,
    PNGImage *pngPtr)
{
    static const unsigned char sBIT_contents[] = {
	8, 8, 8, 8
    };
    int sBIT_length = 4;
    Tcl_DString buf;

    /*
     * Each byte of each channel is always significant; we always write RGBA
     * images with 8 bits per channel as that is what the photo image's basic
     * data model is.
     */

    switch (pngPtr->colorType) {
    case PNG_COLOR_GRAY:
	sBIT_length = 1;
	break;
    case PNG_COLOR_GRAYALPHA:
	sBIT_length = 2;
	break;
    case PNG_COLOR_RGB:
    case PNG_COLOR_PLTE:
	sBIT_length = 3;
	break;
    case PNG_COLOR_RGBA:
	sBIT_length = 4;
	break;
    }
    if (WriteChunk(interp, pngPtr, CHUNK_sBIT, sBIT_contents, sBIT_length)
	    != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Say that it is Tk that made the PNG. Note that we *need* the NUL at the
     * end of "Software" to be transferred; do *not* change the length
     * parameter to -1 there!
     */

    Tcl_DStringInit(&buf);
    Tcl_DStringAppend(&buf, "Software", 9);
    Tcl_DStringAppend(&buf, "Tk Toolkit v", -1);
    Tcl_DStringAppend(&buf, TK_PATCH_LEVEL, -1);
    if (WriteChunk(interp, pngPtr, CHUNK_tEXt,
	    (unsigned char *) Tcl_DStringValue(&buf),
	    Tcl_DStringLength(&buf)) != TCL_OK) {
	Tcl_DStringFree(&buf);
	return TCL_ERROR;
    }
    Tcl_DStringFree(&buf);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * EncodePNG --
 *
 *	This function handles the entirety of writing a PNG file (or data)
 *	from the first byte to the last. No effort is made to optimize the
 *	image data for best compression.
 *
 * Results:
 *	TCL_OK, or TCL_ERROR if an I/O or memory error occurs.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static int
EncodePNG(
    Tcl_Interp *interp,
    Tk_PhotoImageBlock *blockPtr,
    PNGImage *pngPtr)
{
    int greenOffset, blueOffset, alphaOffset;

    /*
     * Determine appropriate color type based on color usage (e.g., only red
     * and maybe alpha channel = grayscale).
     *
     * TODO: Check whether this is doing any good; Tk might just be pushing
     * full RGBA data all the time through here, even though the actual image
     * doesn't need it...
     */

    greenOffset = blockPtr->offset[1] - blockPtr->offset[0];
    blueOffset = blockPtr->offset[2] - blockPtr->offset[0];
    alphaOffset = blockPtr->offset[3];
    if ((alphaOffset >= blockPtr->pixelSize) || (alphaOffset < 0)) {
	alphaOffset = 0;
    } else {
	alphaOffset -= blockPtr->offset[0];
    }

    if ((greenOffset != 0) || (blueOffset != 0)) {
	if (alphaOffset) {
	    pngPtr->colorType = PNG_COLOR_RGBA;
	    pngPtr->bytesPerPixel = 4;
	} else {
	    pngPtr->colorType = PNG_COLOR_RGB;
	    pngPtr->bytesPerPixel = 3;
	}
    } else {
	if (alphaOffset) {
	    pngPtr->colorType = PNG_COLOR_GRAYALPHA;
	    pngPtr->bytesPerPixel = 2;
	} else {
	    pngPtr->colorType = PNG_COLOR_GRAY;
	    pngPtr->bytesPerPixel = 1;
	}
    }

    /*
     * Allocate buffers for lines for filtering and compressed data.
     */

    pngPtr->lineSize = 1 + (pngPtr->bytesPerPixel * blockPtr->width);
    pngPtr->blockLen = pngPtr->lineSize * blockPtr->height;

    if ((blockPtr->width > (INT_MAX - 1) / (pngPtr->bytesPerPixel)) ||
	    (blockPtr->height > INT_MAX / pngPtr->lineSize)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"image is too large to encode pixel data", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "PNG", "TOO_LARGE", NULL);
	return TCL_ERROR;
    }

    pngPtr->lastLineObj = Tcl_NewObj();
    Tcl_IncrRefCount(pngPtr->lastLineObj);
    pngPtr->thisLineObj = Tcl_NewObj();
    Tcl_IncrRefCount(pngPtr->thisLineObj);

    /*
     * Write out the PNG Signature that all PNGs begin with.
     */

    if (WriteData(interp, pngPtr, pngSignature, PNG_SIG_SZ,
	    NULL) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Write out the IHDR (header) chunk containing image dimensions, color
     * type, etc.
     */

    if (WriteIHDR(interp, pngPtr, blockPtr) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Write out the extra chunks containing metadata that is of interest to
     * other programs more than us.
     */

    if (WriteExtraChunks(interp, pngPtr) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Write out the image pixels in the IDAT (data) chunk.
     */

    if (WriteIDAT(interp, pngPtr, blockPtr) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Write out the IEND chunk that all PNGs end with.
     */

    return WriteChunk(interp, pngPtr, CHUNK_IEND, NULL, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * FileWritePNG --
 *
 *	This function is called by the photo image type to write PNG format
 *	data to a file.
 *
 * Results:
 *	A standard TCL completion code. If TCL_ERROR is returned then an error
 *	message is left in the interp's result.
 *
 * Side effects:
 *	The specified file is overwritten.
 *
 *----------------------------------------------------------------------
 */

static int
FileWritePNG(
    Tcl_Interp *interp,
    const char *filename,
    Tcl_Obj *fmtObj,
    Tk_PhotoImageBlock *blockPtr)
{
    Tcl_Channel chan;
    PNGImage png;
    int result = TCL_ERROR;

    /*
     * Open a Tcl file channel where the image data will be stored. Tk ought
     * to take care of this, and just provide a channel, but it doesn't.
     */

    chan = Tcl_OpenFileChannel(interp, filename, "w", 0644);

    if (!chan) {
	return TCL_ERROR;
    }

    /*
     * Initalize PNGImage instance for encoding.
     */

    if (InitPNGImage(interp, &png, chan, NULL,
	    TCL_ZLIB_STREAM_DEFLATE) == TCL_ERROR) {
	goto cleanup;
    }

    /*
     * Set the translation mode to binary so that CR and LF are not to the
     * platform's EOL sequence.
     */

    if (Tcl_SetChannelOption(interp, chan, "-translation",
	    "binary") != TCL_OK) {
	goto cleanup;
    }

    /*
     * Write the raw PNG data out to the file.
     */

    result = EncodePNG(interp, blockPtr, &png);

  cleanup:
    Tcl_Close(interp, chan);
    CleanupPNGImage(&png);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * StringWritePNG --
 *
 *	This function is called by the photo image type to write PNG format
 *	data to a Tcl object and return it in the result.
 *
 * Results:
 *	A standard TCL completion code. If TCL_ERROR is returned then an error
 *	message is left in the interp's result.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static int
StringWritePNG(
    Tcl_Interp *interp,
    Tcl_Obj *fmtObj,
    Tk_PhotoImageBlock *blockPtr)
{
    Tcl_Obj *resultObj = Tcl_NewObj();
    PNGImage png;
    int result = TCL_ERROR;

    /*
     * Initalize PNGImage instance for encoding.
     */

    if (InitPNGImage(interp, &png, NULL, resultObj,
	    TCL_ZLIB_STREAM_DEFLATE) == TCL_ERROR) {
	goto cleanup;
    }

    /*
     * Write the raw PNG data into the prepared Tcl_Obj buffer. Set the result
     * back to the interpreter if successful.
     */

    result = EncodePNG(interp, blockPtr, &png);

    if (TCL_OK == result) {
	Tcl_SetObjResult(interp, png.objDataPtr);
    }

  cleanup:
    CleanupPNGImage(&png);
    return result;
}

/*
 * Local Variables:
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
