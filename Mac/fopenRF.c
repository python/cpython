
/*
 *  fopenRF.c -- Clone of fopen.c to open Mac resource forks.
 *
 *  Copyright (c) 1989 Symantec Corporation.  All rights reserved.
 *
 */

#include "stdio.h"
#include "errno.h"
#include "string.h"
#include "ansi_private.h"

FILE *fopenRF(char *, char *);
FILE *freopenRF(char *, char *, FILE *);
FILE *__openRF(char *, int, int, FILE *);

#include <Files.h>

#define fcbVPtr(fcb)		(* (VCB **) (fcb + 20))
#define fcbDirID(fcb)		(* (long *) (fcb + 58))
#define fcbCName(fcb)		(fcb + 62)

static void setfiletype(StringPtr, int);
static void stdio_exit(void);
static int fileio(FILE *, int);
static int close(FILE *);
static void replace(unsigned char *, size_t, int, int);


FILE *
fopenRF(filename, mode)
char *filename, *mode;
{
	return(freopenRF(filename, mode, __getfile()));
}


FILE *
freopenRF(filename, mode, fp)
char *filename;
register char *mode;
register FILE *fp;
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
__openRF(filename, omode, oflag, fp)
char *filename;
int omode, oflag;
register FILE *fp;
{
	ioParam pb;
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
		asm {
			lea		pb,a0
			_PBCreate
		}
		if (pb.ioResult == noErr)
			oflag &= ~F_TRUNC;
		else if (pb.ioResult == dupFNErr && !(oflag & F_EXCL))
			oflag &= ~F_CREAT;
		else {
			errno = pb.ioResult;
			return(NULL);
		}
	}
	
		/*  open resource file  */
		
	asm {
		lea		pb,a0
		_PBOpenRF
	}
	if (pb.ioResult) {
		errno = pb.ioResult;
		if (oflag & F_CREAT) asm {
			lea		pb,a0
			_PBDelete
		}
		return(NULL);
	}
	fp->refnum = pb.ioRefNum;
	
		/*  get/set file length  */
		
	if (oflag & F_TRUNC) asm {
		lea		pb,a0
		_PBSetEOF
	}
	else if (!(oflag & F_CREAT)) asm {
		lea		pb,a0
		_PBGetEOF
	}
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
setfiletype(name, oflag)
StringPtr name;
int oflag;
{
	fileParam pb;
	
	pb.ioNamePtr = name;
	pb.ioVRefNum = 0;
	pb.ioFVersNum = 0;
	pb.ioFDirIndex = 0;
	asm {
		lea		pb,a0
		_PBGetFInfo
		bmi.s	@1
	}
	pb.ioFlFndrInfo.fdType = pb.ioFlFndrInfo.fdCreator = '????';
	if (!(oflag & F_BINARY))
		pb.ioFlFndrInfo.fdType = 'TEXT';
	asm {
		lea		pb,a0
		_PBSetFInfo
@1	}
}


/*
 *  stdio_exit - stdio shutdown routine
 *
 */

static void
stdio_exit()
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
fileio(fp, i)
register FILE *fp;
int i;
{
	ioParam pb;
	
	pb.ioRefNum = fp->refnum;
	switch (i) {
	
				/*  read  */
			
		case 0:
			pb.ioBuffer = (Ptr) fp->ptr;
			pb.ioReqCount = fp->cnt;
			pb.ioPosMode = fp->refnum > 0 ? fsFromStart : fsAtMark;
			pb.ioPosOffset = fp->pos - fp->cnt;
			asm {
				lea		pb,a0
				_PBRead
			}
			if (pb.ioResult == eofErr) {
				fp->pos = pb.ioPosOffset;
				if (fp->cnt = pb.ioActCount)
					pb.ioResult = 0;
				else {
					fp->eof = 1;
					return(EOF);
				}
			}
			if (pb.ioResult) {
				fp->pos -= fp->cnt;
				fp->cnt = 0;
			}
			else if (!fp->binary)
				replace(fp->ptr, fp->cnt, '\r', '\n');
			break;
			
				/*  write  */

		case 1:
			pb.ioBuffer = (Ptr) fp->ptr;
			pb.ioReqCount = fp->cnt;
			pb.ioPosMode = fp->refnum > 0 ? fsFromStart : fsAtMark;
			if ((pb.ioPosOffset = fp->pos - fp->cnt) > fp->len) {
				pb.ioMisc = (Ptr) pb.ioPosOffset;
				asm {
					lea		pb,a0
					_PBSetEOF
					bmi.s	@1
				}
			}
			if (!fp->binary)
				replace(fp->ptr, fp->cnt, '\n', '\r');
			asm {
				lea		pb,a0
				_PBWrite
@1			}
			if (pb.ioResult) {
				fp->pos -= fp->cnt;
				fp->cnt = 0;
			}
			else if (pb.ioPosOffset > fp->len)
				fp->len = pb.ioPosOffset;
			break;
			
				/*  close  */

		case 2:
			pb.ioResult = close(fp);
			break;
	}
	
		/*  done  */
		
	if (pb.ioResult) {
		fp->err = 1;
		errno = pb.ioResult;
		return(EOF);
	}
	return(0);
}


static int
close(fp)
register FILE *fp;
{
	HFileParam pb;
	Str255 buf;
	register char *fcb = FCBSPtr + fp->refnum;
	VCB *vcb = fcbVPtr(fcb);
	register char *s;
	
	pb.ioNamePtr = buf;
	pb.ioFRefNum = fp->refnum;
	pb.ioVRefNum = vcb->vcbVRefNum;
	pb.ioFVersNum = 0;
	
		/*  close temporary file - HFS  */
		
	if (fp->delete && vcb->vcbSigWord == 0x4244) {
		pb.ioDirID = fcbDirID(fcb);
		s = fcbCName(fcb);
		asm {
			lea		buf,a0
			moveq	#0,d0
			move.b	(s),d0
@1			move.b	(s)+,(a0)+
			dbra	d0,@1
			lea		pb,a0
			_PBClose
			bmi.s	@9
			_PBHDelete
		}
	}
	
		/*  close temporary file - MFS  */
		
	else if (fp->delete && vcb->vcbSigWord == 0xD2D7) {
		pb.ioFDirIndex = 1;
		do asm {
			lea		pb,a0
			_PBGetFInfo
			bmi.s	@2
			addq.w	#1,pb.ioFDirIndex
		} while (pb.ioFRefNum != fp->refnum);
		asm {
			lea		pb,a0
			_PBClose
			bmi.s	@9
			_PBDelete
		}
	}
	
		/*  normal case - just close file  */
		
	else {
		asm {
@2			lea		pb,a0
			_PBClose
			bmi.s	@9
		}
	}
	
		/*  flush volume buffer  */
		
	pb.ioNamePtr = 0;
	asm {
		lea		pb,a0
		_PBFlshVol
@9	}
	return(pb.ioResult);
}


/*
 *  replace - routine for doing CR/LF conversion
 *
 */

static void
replace(s, n, c1, c2)
register unsigned char *s;
register size_t n;
register int c1, c2;
{
	register unsigned char *t;
	
	for (; n && (t = memchr(s, c1, n)); s = t) {
		*t++ = c2;
		n -= t - s;
	}
}
