
/*
 *  fopen.c
 *
 *  Copyright (c) 1991 Symantec Corporation.  All rights reserved.
 *
 */

#include <MacHeaders>

#include "stdio.h"
#include "errno.h"
#include "string.h"
#include "ansi_private.h"

extern long _ftype, _fcreator;

#define fcbVPtr(fcb)		(* (VCB **) (fcb + 20))
#define fcbDirID(fcb)		(* (long *) (fcb + 58))
#define fcbCName(fcb)		(fcb + 62)

static void setfiletype(StringPtr, int);
static void stdio_exit(void);
static int fileio(FILE *, int);
static int close(FILE *);
static void replace(unsigned char *, size_t, int, int);

FILE *freopenRF();
FILE *__openRF();

FILE *
fopenRF(const char *filename, const char *mode)
{
	return(freopenRF(filename, mode, __getfile()));
}


FILE *
freopenRF(const char *filename, const char *mode, FILE *fp)
{
	int omode, oflag;
	
		/*  interpret "rwa"  */
	
	if (mode[0] == 'r') {
		omode = fsRdPerm;
		oflag = 0;
	}
	else if (mode[0] == 'w') {
		omode = fsWrPerm;
		oflag = F_CREAT+F_TRUNC;
	}
	else if (mode[0] == 'a') {
		omode = fsWrPerm;
		oflag = F_CREAT+F_APPEND;
	}
	else {
		errno = EINVAL;
		return(NULL);
	}
		
		/*  interpret "b+"  */
		
	if (mode[1] == 'b') {
		oflag |= F_BINARY;
		if (mode[2] == '+')
			omode = fsRdWrPerm;
	}
	else if (mode[1] == '+') {
		omode = fsRdWrPerm;
		if (mode[2] == 'b')
			oflag |= F_BINARY;
	}
	
		/*  open the file  */
		
	return(__openRF(filename, omode, oflag, fp));
}


FILE *
__openRF(const char *filename, int omode, int oflag, FILE *fp)
{
	IOParam pb;
	char pname[FILENAME_MAX];

	if (fp == NULL)
		return(NULL);
	fclose(fp);
	
		/*  set up pb  */
	
	pb.ioNamePtr = __c2p(filename, pname);
	pb.ioVRefNum = 0;
	pb.ioVersNum = 0;
	pb.ioPermssn = omode;
	pb.ioMisc = 0;

		/*  create file  */

	if (oflag & F_CREAT) {
		PBCreateSync((ParmBlkPtr)&pb);
		if (pb.ioResult == noErr)
			oflag &= ~F_TRUNC;
		else if (pb.ioResult == dupFNErr && !(oflag & F_EXCL))
			oflag &= ~F_CREAT;
		else {
			errno = pb.ioResult;
			return(NULL);
		}
	}
	
		/*  open file  */
		
	PBOpenRFSync((ParmBlkPtr)&pb);
	if (pb.ioResult) {
		errno = pb.ioResult;
		if (oflag & F_CREAT)
			PBDeleteSync((ParmBlkPtr)&pb);
		return(NULL);
	}
	fp->refnum = pb.ioRefNum;
	
		/*  get/set file length  */
		
	if (oflag & F_TRUNC)
		PBSetEOFSync((ParmBlkPtr)&pb);
	else if (!(oflag & F_CREAT))
		PBGetEOFSync((ParmBlkPtr)&pb);
	fp->len = (fpos_t) pb.ioMisc;
		
		/*  initialize rest of FILE structure  */
		
	if (oflag & F_APPEND) {
		fp->append = 1;
		fp->pos = fp->len;
	}
	if (oflag & F_BINARY)
		fp->binary = 1;
	setvbuf(fp, NULL, _IOFBF, BUFSIZ);
	fp->proc = fileio;

		/*  set file type  */

	if (oflag & (F_CREAT|F_TRUNC))
		setfiletype(pb.ioNamePtr, oflag);
		
		/*  done  */
		
	__atexit_stdio(stdio_exit);
	return(fp);
}


/*
 *  setfiletype - set type/creator of new file
 *
 */

static void
setfiletype(StringPtr name, int oflag)
{
	FileParam pb;
	
	pb.ioNamePtr = name;
	pb.ioVRefNum = 0;
	pb.ioFVersNum = 0;
	pb.ioFDirIndex = 0;
	if (PBGetFInfoSync((ParmBlkPtr)&pb) == noErr) {
		if (oflag & F_BINARY)
			pb.ioFlFndrInfo.fdType = _ftype;
		else
			pb.ioFlFndrInfo.fdType = 'TEXT';
		pb.ioFlFndrInfo.fdCreator = _fcreator;
		PBSetFInfoSync((ParmBlkPtr)&pb);
	}
}


/*
 *  stdio_exit - stdio shutdown routine
 *
 */

static void
stdio_exit(void)
{
	register FILE *fp;
	int n;
	
	for (fp = &__file[0], n = FOPEN_MAX; n--; fp++)
		fclose(fp);
}


/*
 *  fileio - I/O handler proc for files and devices
 *
 */

static int
fileio(FILE *fp, int i)
{
	IOParam pb;
	
	pb.ioRefNum = fp->refnum;
	switch (i) {
	
				/*  read  */
			
		case 0:
			pb.ioBuffer = (Ptr) fp->ptr;
			pb.ioReqCount = fp->cnt;
			pb.ioPosMode = fp->refnum > 0 ? fsFromStart : fsAtMark;
			pb.ioPosOffset = fp->pos - fp->cnt;
			PBReadSync((ParmBlkPtr)&pb);
			if (pb.ioResult == eofErr) {
				fp->pos = pb.ioPosOffset;
				if (fp->cnt = pb.ioActCount)
					pb.ioResult = 0;
				else {
					fp->eof = 1;
					return(EOF);
				}
			}
			if (!pb.ioResult && !fp->binary)
				replace(fp->ptr, fp->cnt, '\r', '\n');
			break;
			
				/*  write  */

		case 1:
			pb.ioBuffer = (Ptr) fp->ptr;
			pb.ioReqCount = fp->cnt;
			pb.ioPosMode = fp->refnum > 0 ? fsFromStart : fsAtMark;
			if ((pb.ioPosOffset = fp->pos - fp->cnt) > fp->len) {
				pb.ioMisc = (Ptr) pb.ioPosOffset;
				if (PBSetEOFSync((ParmBlkPtr)&pb) != noErr)
					break;
			}
			if (!fp->binary)
				replace(fp->ptr, fp->cnt, '\n', '\r');
			PBWriteSync((ParmBlkPtr)&pb);
			if (!pb.ioResult && pb.ioPosOffset > fp->len)
				fp->len = pb.ioPosOffset;
			break;
			
				/*  close  */

		case 2:
			pb.ioResult = close(fp);
			break;
	}
	
		/*  done  */
		
	if (pb.ioResult) {
		if (i < 2) {
			fp->pos -= fp->cnt;
			fp->cnt = 0;
		}
		fp->err = 1;
		errno = pb.ioResult;
		return(EOF);
	}
	return(0);
}


static int
close(FILE *fp)
{
	HFileParam pb;
	Str255 buf;
	register char *fcb = FCBSPtr + fp->refnum;
	VCB *vcb = fcbVPtr(fcb);
	register char *s;
	enum { none, MFS, HFS } del = none;
	
	pb.ioVRefNum = vcb->vcbVRefNum;
	if (fp->remove) {
		pb.ioNamePtr = buf;
		pb.ioFVersNum = 0;
	
			/*  close temporary file - HFS  */
			
		if (vcb->vcbSigWord == 0x4244) {
			pb.ioDirID = fcbDirID(fcb);
			s = fcbCName(fcb);
			memcpy(buf, s, Length(s) + 1);
			del = HFS;
		}
		
			/*  close temporary file - MFS  */
			
		else if (vcb->vcbSigWord == 0xD2D7) {
			for (pb.ioFDirIndex = 1; PBGetFInfoSync((ParmBlkPtr)&pb) == noErr; pb.ioFDirIndex++) {
				if (pb.ioFRefNum == fp->refnum) {
					del = MFS;
					break;
				}
			}
		}
	}
	
		/*  close file and flush volume buffer  */
	
	pb.ioFRefNum = fp->refnum;
	if (PBCloseSync((ParmBlkPtr)&pb) == noErr) {
		if (del == MFS)
			PBDeleteSync((ParmBlkPtr)&pb);
		else if (del == HFS)
			PBHDeleteSync((HParmBlkPtr)&pb);
		pb.ioNamePtr = 0;
		PBFlushVolSync((ParmBlkPtr)&pb);
	}
	return(pb.ioResult);
}


/*
 *  replace - routine for doing CR/LF conversion
 *
 */

static void
replace(register unsigned char *s, register size_t n, register int c1, register int c2)
{
#pragma options(honor_register)
	register unsigned char *t;
	
	for (; n && (t = memchr(s, c1, n)); s = t) {
		*t++ = c2;
		n -= t - s;
	}
}
