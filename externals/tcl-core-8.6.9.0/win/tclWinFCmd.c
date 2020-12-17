/*
 * tclWinFCmd.c
 *
 *	This file implements the Windows specific portion of file manipulation
 *	subcommands of the "file" command.
 *
 * Copyright (c) 1996-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclWinInt.h"

/*
 * The following constants specify the type of callback when
 * TraverseWinTree() calls the traverseProc()
 */

#define DOTREE_PRED	1	/* pre-order directory  */
#define DOTREE_POSTD	2	/* post-order directory */
#define DOTREE_F	3	/* regular file */
#define DOTREE_LINK	4	/* symbolic link */

/*
 * Callbacks for file attributes code.
 */

static int		GetWinFileAttributes(Tcl_Interp *interp, int objIndex,
			    Tcl_Obj *fileName, Tcl_Obj **attributePtrPtr);
static int		GetWinFileLongName(Tcl_Interp *interp, int objIndex,
			    Tcl_Obj *fileName, Tcl_Obj **attributePtrPtr);
static int		GetWinFileShortName(Tcl_Interp *interp, int objIndex,
			    Tcl_Obj *fileName, Tcl_Obj **attributePtrPtr);
static int		SetWinFileAttributes(Tcl_Interp *interp, int objIndex,
			    Tcl_Obj *fileName, Tcl_Obj *attributePtr);
static int		CannotSetAttribute(Tcl_Interp *interp, int objIndex,
			    Tcl_Obj *fileName, Tcl_Obj *attributePtr);

/*
 * Constants and variables necessary for file attributes subcommand.
 */

enum {
    WIN_ARCHIVE_ATTRIBUTE,
    WIN_HIDDEN_ATTRIBUTE,
    WIN_LONGNAME_ATTRIBUTE,
    WIN_READONLY_ATTRIBUTE,
    WIN_SHORTNAME_ATTRIBUTE,
    WIN_SYSTEM_ATTRIBUTE
};

static const int attributeArray[] = {FILE_ATTRIBUTE_ARCHIVE, FILE_ATTRIBUTE_HIDDEN,
	0, FILE_ATTRIBUTE_READONLY, 0, FILE_ATTRIBUTE_SYSTEM};


const char *const tclpFileAttrStrings[] = {
	"-archive", "-hidden", "-longname", "-readonly",
	"-shortname", "-system", (char *) NULL
};

const TclFileAttrProcs tclpFileAttrProcs[] = {
	{GetWinFileAttributes, SetWinFileAttributes},
	{GetWinFileAttributes, SetWinFileAttributes},
	{GetWinFileLongName, CannotSetAttribute},
	{GetWinFileAttributes, SetWinFileAttributes},
	{GetWinFileShortName, CannotSetAttribute},
	{GetWinFileAttributes, SetWinFileAttributes}};

/*
 * Prototype for the TraverseWinTree callback function.
 */

typedef int (TraversalProc)(const TCHAR *srcPtr, const TCHAR *dstPtr,
	int type, Tcl_DString *errorPtr);

/*
 * Declarations for local functions defined in this file:
 */

static void		StatError(Tcl_Interp *interp, Tcl_Obj *fileName);
static int		ConvertFileNameFormat(Tcl_Interp *interp,
			    int objIndex, Tcl_Obj *fileName, int longShort,
			    Tcl_Obj **attributePtrPtr);
static int		DoCopyFile(const TCHAR *srcPtr, const TCHAR *dstPtr);
static int		DoCreateDirectory(const TCHAR *pathPtr);
static int		DoRemoveJustDirectory(const TCHAR *nativeSrc,
			    int ignoreError, Tcl_DString *errorPtr);
static int		DoRemoveDirectory(Tcl_DString *pathPtr, int recursive,
			    Tcl_DString *errorPtr);
static int		DoRenameFile(const TCHAR *nativeSrc,
			    const TCHAR *dstPtr);
static int		TraversalCopy(const TCHAR *srcPtr, const TCHAR *dstPtr,
			    int type, Tcl_DString *errorPtr);
static int		TraversalDelete(const TCHAR *srcPtr,
			    const TCHAR *dstPtr, int type,
			    Tcl_DString *errorPtr);
static int		TraverseWinTree(TraversalProc *traverseProc,
			    Tcl_DString *sourcePtr, Tcl_DString *dstPtr,
			    Tcl_DString *errorPtr);

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjRenameFile, DoRenameFile --
 *
 *	Changes the name of an existing file or directory, from src to dst.
 *	If src and dst refer to the same file or directory, does nothing and
 *	returns success. Otherwise if dst already exists, it will be deleted
 *	and replaced by src subject to the following conditions:
 *	    If src is a directory, dst may be an empty directory.
 *	    If src is a file, dst may be a file.
 *	In any other situation where dst already exists, the rename will fail.
 *
 * Results:
 *	If the file or directory was successfully renamed, returns TCL_OK.
 *	Otherwise the return value is TCL_ERROR and errno is set to indicate
 *	the error. Some possible values for errno are:
 *
 *	ENAMETOOLONG: src or dst names are too long.
 *	EACCES:	    src or dst parent directory can't be read and/or written.
 *	EEXIST:	    dst is a non-empty directory.
 *	EINVAL:	    src is a root directory or dst is a subdirectory of src.
 *	EISDIR:	    dst is a directory, but src is not.
 *	ENOENT:	    src doesn't exist.	src or dst is "".
 *	ENOTDIR:    src is a directory, but dst is not.
 *	EXDEV:	    src and dst are on different filesystems.
 *
 *	EACCES:	    exists an open file already referring to src or dst.
 *	EACCES:	    src or dst specify the current working directory (NT).
 *	EACCES:	    src specifies a char device (nul:, com1:, etc.)
 *	EEXIST:	    dst specifies a char device (nul:, com1:, etc.) (NT)
 *	EACCES:	    dst specifies a char device (nul:, com1:, etc.) (95)
 *
 * Side effects:
 *	The implementation supports cross-filesystem renames of files, but the
 *	caller should be prepared to emulate cross-filesystem renames of
 *	directories if errno is EXDEV.
 *
 *---------------------------------------------------------------------------
 */

int
TclpObjRenameFile(
    Tcl_Obj *srcPathPtr,
    Tcl_Obj *destPathPtr)
{
    return DoRenameFile(Tcl_FSGetNativePath(srcPathPtr),
	    Tcl_FSGetNativePath(destPathPtr));
}

static int
DoRenameFile(
    const TCHAR *nativeSrc,	/* Pathname of file or dir to be renamed
				 * (native). */
    const TCHAR *nativeDst)	/* New pathname for file or directory
				 * (native). */
{
#if defined(HAVE_NO_SEH) && !defined(_WIN64)
    TCLEXCEPTION_REGISTRATION registration;
#endif
    DWORD srcAttr, dstAttr;
    int retval = -1;

    /*
     * The MoveFile API acts differently under Win95/98 and NT WRT NULL and
     * "". Avoid passing these values.
     */

    if (nativeSrc == NULL || nativeSrc[0] == '\0' ||
	    nativeDst == NULL || nativeDst[0] == '\0') {
	Tcl_SetErrno(ENOENT);
	return TCL_ERROR;
    }

    /*
     * The MoveFile API would throw an exception under NT if one of the
     * arguments is a char block device.
     */

#if defined(HAVE_NO_SEH) && !defined(_WIN64)
    /*
     * Don't have SEH available, do things the hard way. Note that this needs
     * to be one block of asm, to avoid stack imbalance; also, it is illegal
     * for one asm block to contain a jump to another.
     */

    __asm__ __volatile__ (
	/*
	 * Pick up params before messing with the stack.
	 */

	"movl	    %[nativeDst],   %%ebx"	    "\n\t"
	"movl	    %[nativeSrc],   %%ecx"	    "\n\t"

	/*
	 * Construct an TCLEXCEPTION_REGISTRATION to protect the call to
	 * MoveFile.
	 */

	"leal	    %[registration], %%edx"	    "\n\t"
	"movl	    %%fs:0,	    %%eax"	    "\n\t"
	"movl	    %%eax,	    0x0(%%edx)"	    "\n\t" /* link */
	"leal	    1f,		    %%eax"	    "\n\t"
	"movl	    %%eax,	    0x4(%%edx)"	    "\n\t" /* handler */
	"movl	    %%ebp,	    0x8(%%edx)"	    "\n\t" /* ebp */
	"movl	    %%esp,	    0xc(%%edx)"	    "\n\t" /* esp */
	"movl	    $0,		    0x10(%%edx)"    "\n\t" /* status */

	/*
	 * Link the TCLEXCEPTION_REGISTRATION on the chain.
	 */

	"movl	    %%edx,	    %%fs:0"	    "\n\t"

	/*
	 * Call MoveFile(nativeSrc, nativeDst)
	 */

	"pushl	    %%ebx"			    "\n\t"
	"pushl	    %%ecx"			    "\n\t"
	"movl	    %[moveFile],    %%eax"	    "\n\t"
	"call	    *%%eax"			    "\n\t"

	/*
	 * Come here on normal exit. Recover the TCLEXCEPTION_REGISTRATION and
	 * put the status return from MoveFile into it.
	 */

	"movl	    %%fs:0,	    %%edx"	    "\n\t"
	"movl	    %%eax,	    0x10(%%edx)"    "\n\t"
	"jmp	    2f"				    "\n"

	/*
	 * Come here on an exception. Recover the TCLEXCEPTION_REGISTRATION
	 */

	"1:"					    "\t"
	"movl	    %%fs:0,	    %%edx"	    "\n\t"
	"movl	    0x8(%%edx),	    %%edx"	    "\n\t"

	/*
	 * Come here however we exited. Restore context from the
	 * TCLEXCEPTION_REGISTRATION in case the stack is unbalanced.
	 */

	"2:"					    "\t"
	"movl	    0xc(%%edx),	    %%esp"	    "\n\t"
	"movl	    0x8(%%edx),	    %%ebp"	    "\n\t"
	"movl	    0x0(%%edx),	    %%eax"	    "\n\t"
	"movl	    %%eax,	    %%fs:0"	    "\n\t"

	:
	/* No outputs */
	:
	[registration]	"m"	(registration),
	[nativeDst]	"m"	(nativeDst),
	[nativeSrc]	"m"	(nativeSrc),
	[moveFile]	"r"	(MoveFile)
	:
	"%eax", "%ebx", "%ecx", "%edx", "memory"
	);
    if (registration.status != FALSE) {
	retval = TCL_OK;
    }
#else
#ifndef HAVE_NO_SEH
    __try {
#endif
	if ((*MoveFile)(nativeSrc, nativeDst) != FALSE) {
	    retval = TCL_OK;
	}
#ifndef HAVE_NO_SEH
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
#endif

    if (retval != -1) {
	return retval;
    }

    TclWinConvertError(GetLastError());

    srcAttr = GetFileAttributes(nativeSrc);
    dstAttr = GetFileAttributes(nativeDst);
    if (srcAttr == 0xffffffff) {
	if (GetFullPathName(nativeSrc, 0, NULL,
		NULL) >= MAX_PATH) {
	    errno = ENAMETOOLONG;
	    return TCL_ERROR;
	}
	srcAttr = 0;
    }
    if (dstAttr == 0xffffffff) {
	if (GetFullPathName(nativeDst, 0, NULL,
		NULL) >= MAX_PATH) {
	    errno = ENAMETOOLONG;
	    return TCL_ERROR;
	}
	dstAttr = 0;
    }

    if (errno == EBADF) {
	errno = EACCES;
	return TCL_ERROR;
    }
    if (errno == EACCES) {
    decode:
	if (srcAttr & FILE_ATTRIBUTE_DIRECTORY) {
	    TCHAR *nativeSrcRest, *nativeDstRest;
	    const char **srcArgv, **dstArgv;
	    int size, srcArgc, dstArgc;
	    TCHAR nativeSrcPath[MAX_PATH];
	    TCHAR nativeDstPath[MAX_PATH];
	    Tcl_DString srcString, dstString;
	    const char *src, *dst;

	    size = GetFullPathName(nativeSrc, MAX_PATH,
		    nativeSrcPath, &nativeSrcRest);
	    if ((size == 0) || (size > MAX_PATH)) {
		return TCL_ERROR;
	    }
	    size = GetFullPathName(nativeDst, MAX_PATH,
		    nativeDstPath, &nativeDstRest);
	    if ((size == 0) || (size > MAX_PATH)) {
		return TCL_ERROR;
	    }
	    CharLower(nativeSrcPath);
	    CharLower(nativeDstPath);

	    src = Tcl_WinTCharToUtf(nativeSrcPath, -1, &srcString);
	    dst = Tcl_WinTCharToUtf(nativeDstPath, -1, &dstString);

	    /*
	     * Check whether the destination path is actually inside the
	     * source path. This is true if the prefix matches, and the next
	     * character is either end-of-string or a directory separator
	     */

	    if ((strncmp(src, dst, (size_t) Tcl_DStringLength(&srcString))==0)
		    && (dst[Tcl_DStringLength(&srcString)] == '\\'
		    || dst[Tcl_DStringLength(&srcString)] == '/'
		    || dst[Tcl_DStringLength(&srcString)] == '\0')) {
		/*
		 * Trying to move a directory into itself.
		 */

		errno = EINVAL;
		Tcl_DStringFree(&srcString);
		Tcl_DStringFree(&dstString);
		return TCL_ERROR;
	    }
	    Tcl_SplitPath(src, &srcArgc, &srcArgv);
	    Tcl_SplitPath(dst, &dstArgc, &dstArgv);
	    Tcl_DStringFree(&srcString);
	    Tcl_DStringFree(&dstString);

	    if (srcArgc == 1) {
		/*
		 * They are trying to move a root directory. Whether or not it
		 * is across filesystems, this cannot be done.
		 */

		Tcl_SetErrno(EINVAL);
	    } else if ((srcArgc > 0) && (dstArgc > 0) &&
		    (strcmp(srcArgv[0], dstArgv[0]) != 0)) {
		/*
		 * If src is a directory and dst filesystem != src filesystem,
		 * errno should be EXDEV. It is very important to get this
		 * behavior, so that the caller can respond to a cross
		 * filesystem rename by simulating it with copy and delete.
		 * The MoveFile system call already handles the case of moving
		 * a file between filesystems.
		 */

		Tcl_SetErrno(EXDEV);
	    }

	    ckfree(srcArgv);
	    ckfree(dstArgv);
	}

	/*
	 * Other types of access failure is that dst is a read-only
	 * filesystem, that an open file referred to src or dest, or that src
	 * or dest specified the current working directory on the current
	 * filesystem. EACCES is returned for those cases.
	 */

    } else if (Tcl_GetErrno() == EEXIST) {
	/*
	 * Reports EEXIST any time the target already exists. If it makes
	 * sense, remove the old file and try renaming again.
	 */

	if (srcAttr & FILE_ATTRIBUTE_DIRECTORY) {
	    if (dstAttr & FILE_ATTRIBUTE_DIRECTORY) {
		/*
		 * Overwrite empty dst directory with src directory. The
		 * following call will remove an empty directory. If it fails,
		 * it's because it wasn't empty.
		 */

		if (DoRemoveJustDirectory(nativeDst, 0, NULL) == TCL_OK) {
		    /*
		     * Now that that empty directory is gone, we can try
		     * renaming again. If that fails, we'll put this empty
		     * directory back, for completeness.
		     */

		    if (MoveFile(nativeSrc,
			    nativeDst) != FALSE) {
			return TCL_OK;
		    }

		    /*
		     * Some new error has occurred. Don't know what it could
		     * be, but report this one.
		     */

		    TclWinConvertError(GetLastError());
		    CreateDirectory(nativeDst, NULL);
		    SetFileAttributes(nativeDst, dstAttr);
		    if (Tcl_GetErrno() == EACCES) {
			/*
			 * Decode the EACCES to a more meaningful error.
			 */

			goto decode;
		    }
		}
	    } else {	/* (dstAttr & FILE_ATTRIBUTE_DIRECTORY) == 0 */
		Tcl_SetErrno(ENOTDIR);
	    }
	} else {    /* (srcAttr & FILE_ATTRIBUTE_DIRECTORY) == 0 */
	    if (dstAttr & FILE_ATTRIBUTE_DIRECTORY) {
		Tcl_SetErrno(EISDIR);
	    } else {
		/*
		 * Overwrite existing file by:
		 *
		 * 1. Rename existing file to temp name.
		 * 2. Rename old file to new name.
		 * 3. If success, delete temp file. If failure, put temp file
		 *    back to old name.
		 */

		TCHAR *nativeRest, *nativeTmp, *nativePrefix;
		int result, size;
		TCHAR tempBuf[MAX_PATH];

		size = GetFullPathName(nativeDst, MAX_PATH,
			tempBuf, &nativeRest);
		if ((size == 0) || (size > MAX_PATH) || (nativeRest == NULL)) {
		    return TCL_ERROR;
		}
		nativeTmp = (TCHAR *) tempBuf;
		nativeRest[0] = L'\0';

		result = TCL_ERROR;
		nativePrefix = (TCHAR *) L"tclr";
		if (GetTempFileName(nativeTmp, nativePrefix,
			0, tempBuf) != 0) {
		    /*
		     * Strictly speaking, need the following DeleteFile and
		     * MoveFile to be joined as an atomic operation so no
		     * other app comes along in the meantime and creates the
		     * same temp file.
		     */

		    nativeTmp = tempBuf;
		    DeleteFile(nativeTmp);
		    if (MoveFile(nativeDst, nativeTmp) != FALSE) {
			if (MoveFile(nativeSrc, nativeDst) != FALSE) {
			    SetFileAttributes(nativeTmp, FILE_ATTRIBUTE_NORMAL);
			    DeleteFile(nativeTmp);
			    return TCL_OK;
			} else {
			    DeleteFile(nativeDst);
			    MoveFile(nativeTmp, nativeDst);
			}
		    }

		    /*
		     * Can't backup dst file or move src file. Return that
		     * error. Could happen if an open file refers to dst.
		     */

		    TclWinConvertError(GetLastError());
		    if (Tcl_GetErrno() == EACCES) {
			/*
			 * Decode the EACCES to a more meaningful error.
			 */

			goto decode;
		    }
		}
		return result;
	    }
	}
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjCopyFile, DoCopyFile --
 *
 *	Copy a single file (not a directory). If dst already exists and is not
 *	a directory, it is removed.
 *
 * Results:
 *	If the file was successfully copied, returns TCL_OK. Otherwise the
 *	return value is TCL_ERROR and errno is set to indicate the error.
 *	Some possible values for errno are:
 *
 *	EACCES:	    src or dst parent directory can't be read and/or written.
 *	EISDIR:	    src or dst is a directory.
 *	ENOENT:	    src doesn't exist.	src or dst is "".
 *
 *	EACCES:	    exists an open file already referring to dst (95).
 *	EACCES:	    src specifies a char device (nul:, com1:, etc.) (NT)
 *	ENOENT:	    src specifies a char device (nul:, com1:, etc.) (95)
 *
 * Side effects:
 *	It is not an error to copy to a char device.
 *
 *---------------------------------------------------------------------------
 */

int
TclpObjCopyFile(
    Tcl_Obj *srcPathPtr,
    Tcl_Obj *destPathPtr)
{
    return DoCopyFile(Tcl_FSGetNativePath(srcPathPtr),
	    Tcl_FSGetNativePath(destPathPtr));
}

static int
DoCopyFile(
    const TCHAR *nativeSrc,	/* Pathname of file to be copied (native). */
    const TCHAR *nativeDst)	/* Pathname of file to copy to (native). */
{
#if defined(HAVE_NO_SEH) && !defined(_WIN64)
    TCLEXCEPTION_REGISTRATION registration;
#endif
    int retval = -1;

    /*
     * The CopyFile API acts differently under Win95/98 and NT WRT NULL and
     * "". Avoid passing these values.
     */

    if (nativeSrc == NULL || nativeSrc[0] == '\0' ||
	    nativeDst == NULL || nativeDst[0] == '\0') {
	Tcl_SetErrno(ENOENT);
	return TCL_ERROR;
    }

    /*
     * The CopyFile API would throw an exception under NT if one of the
     * arguments is a char block device.
     */

#if defined(HAVE_NO_SEH) && !defined(_WIN64)
    /*
     * Don't have SEH available, do things the hard way. Note that this needs
     * to be one block of asm, to avoid stack imbalance; also, it is illegal
     * for one asm block to contain a jump to another.
     */

    __asm__ __volatile__ (

	/*
	 * Pick up parameters before messing with the stack
	 */

	"movl	    %[nativeDst],   %%ebx"	    "\n\t"
	"movl	    %[nativeSrc],   %%ecx"	    "\n\t"

	/*
	 * Construct an TCLEXCEPTION_REGISTRATION to protect the call to
	 * CopyFile.
	 */

	"leal	    %[registration], %%edx"	    "\n\t"
	"movl	    %%fs:0,	    %%eax"	    "\n\t"
	"movl	    %%eax,	    0x0(%%edx)"	    "\n\t" /* link */
	"leal	    1f,		    %%eax"	    "\n\t"
	"movl	    %%eax,	    0x4(%%edx)"	    "\n\t" /* handler */
	"movl	    %%ebp,	    0x8(%%edx)"	    "\n\t" /* ebp */
	"movl	    %%esp,	    0xc(%%edx)"	    "\n\t" /* esp */
	"movl	    $0,		    0x10(%%edx)"    "\n\t" /* status */

	/*
	 * Link the TCLEXCEPTION_REGISTRATION on the chain.
	 */

	"movl	    %%edx,	    %%fs:0"	    "\n\t"

	/*
	 * Call CopyFile(nativeSrc, nativeDst, 0)
	 */

	"movl	    %[copyFile],    %%eax"	    "\n\t"
	"pushl	    $0"				    "\n\t"
	"pushl	    %%ebx"			    "\n\t"
	"pushl	    %%ecx"			    "\n\t"
	"call	    *%%eax"			    "\n\t"

	/*
	 * Come here on normal exit. Recover the TCLEXCEPTION_REGISTRATION and
	 * put the status return from CopyFile into it.
	 */

	"movl	    %%fs:0,	    %%edx"	    "\n\t"
	"movl	    %%eax,	    0x10(%%edx)"    "\n\t"
	"jmp	    2f"				    "\n"

	/*
	 * Come here on an exception. Recover the TCLEXCEPTION_REGISTRATION
	 */

	"1:"					    "\t"
	"movl	    %%fs:0,	    %%edx"	    "\n\t"
	"movl	    0x8(%%edx),	    %%edx"	    "\n\t"

	/*
	 * Come here however we exited. Restore context from the
	 * TCLEXCEPTION_REGISTRATION in case the stack is unbalanced.
	 */

	"2:"					    "\t"
	"movl	    0xc(%%edx),	    %%esp"	    "\n\t"
	"movl	    0x8(%%edx),	    %%ebp"	    "\n\t"
	"movl	    0x0(%%edx),	    %%eax"	    "\n\t"
	"movl	    %%eax,	    %%fs:0"	    "\n\t"

	:
	/* No outputs */
	:
	[registration]	"m"	(registration),
	[nativeDst]	"m"	(nativeDst),
	[nativeSrc]	"m"	(nativeSrc),
	[copyFile]	"r"	(CopyFile)
	:
	"%eax", "%ebx", "%ecx", "%edx", "memory"
	);
    if (registration.status != FALSE) {
	retval = TCL_OK;
    }
#else
#ifndef HAVE_NO_SEH
    __try {
#endif
	if (CopyFile(nativeSrc, nativeDst, 0) != FALSE) {
	    retval = TCL_OK;
	}
#ifndef HAVE_NO_SEH
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
#endif

    if (retval != -1) {
	return retval;
    }

    TclWinConvertError(GetLastError());
    if (Tcl_GetErrno() == EBADF) {
	Tcl_SetErrno(EACCES);
	return TCL_ERROR;
    }
    if (Tcl_GetErrno() == EACCES) {
	DWORD srcAttr, dstAttr;

	srcAttr = GetFileAttributes(nativeSrc);
	dstAttr = GetFileAttributes(nativeDst);
	if (srcAttr != 0xffffffff) {
	    if (dstAttr == 0xffffffff) {
		dstAttr = 0;
	    }
	    if ((srcAttr & FILE_ATTRIBUTE_DIRECTORY) ||
		    (dstAttr & FILE_ATTRIBUTE_DIRECTORY)) {
		if (srcAttr & FILE_ATTRIBUTE_REPARSE_POINT) {
		    /* Source is a symbolic link -- copy it */
		    if (TclWinSymLinkCopyDirectory(nativeSrc, nativeDst)==0) {
			return TCL_OK;
		    }
		}
		Tcl_SetErrno(EISDIR);
	    }
	    if (dstAttr & FILE_ATTRIBUTE_READONLY) {
		SetFileAttributes(nativeDst,
			dstAttr & ~((DWORD)FILE_ATTRIBUTE_READONLY));
		if (CopyFile(nativeSrc, nativeDst,
			0) != FALSE) {
		    return TCL_OK;
		}

		/*
		 * Still can't copy onto dst. Return that error, and restore
		 * attributes of dst.
		 */

		TclWinConvertError(GetLastError());
		SetFileAttributes(nativeDst, dstAttr);
	    }
	}
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjDeleteFile, TclpDeleteFile --
 *
 *	Removes a single file (not a directory).
 *
 * Results:
 *	If the file was successfully deleted, returns TCL_OK. Otherwise the
 *	return value is TCL_ERROR and errno is set to indicate the error.
 *	Some possible values for errno are:
 *
 *	EACCES:	    a parent directory can't be read and/or written.
 *	EISDIR:	    path is a directory.
 *	ENOENT:	    path doesn't exist or is "".
 *
 *	EACCES:	    exists an open file already referring to path.
 *	EACCES:	    path is a char device (nul:, com1:, etc.)
 *
 * Side effects:
 *	The file is deleted, even if it is read-only.
 *
 *---------------------------------------------------------------------------
 */

int
TclpObjDeleteFile(
    Tcl_Obj *pathPtr)
{
    return TclpDeleteFile(Tcl_FSGetNativePath(pathPtr));
}

int
TclpDeleteFile(
    const void *nativePath)	/* Pathname of file to be removed (native). */
{
    DWORD attr;
    const TCHAR *path = nativePath;

    /*
     * The DeleteFile API acts differently under Win95/98 and NT WRT NULL and
     * "". Avoid passing these values.
     */

    if (path == NULL || path[0] == '\0') {
	Tcl_SetErrno(ENOENT);
	return TCL_ERROR;
    }

    if (DeleteFile(path) != FALSE) {
	return TCL_OK;
    }
    TclWinConvertError(GetLastError());

    if (Tcl_GetErrno() == EACCES) {
	attr = GetFileAttributes(path);
	if (attr != 0xffffffff) {
	    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
		if (attr & FILE_ATTRIBUTE_REPARSE_POINT) {
		    /*
		     * It is a symbolic link - remove it.
		     */
		    if (TclWinSymLinkDelete(path, 0) == 0) {
			return TCL_OK;
		    }
		}

		/*
		 * If we fall through here, it is a directory.
		 *
		 * Windows NT reports removing a directory as EACCES instead
		 * of EISDIR.
		 */

		Tcl_SetErrno(EISDIR);
	    } else if (attr & FILE_ATTRIBUTE_READONLY) {
		int res = SetFileAttributes(path,
			attr & ~((DWORD) FILE_ATTRIBUTE_READONLY));

		if ((res != 0) &&
			(DeleteFile(path) != FALSE)) {
		    return TCL_OK;
		}
		TclWinConvertError(GetLastError());
		if (res != 0) {
		    SetFileAttributes(path, attr);
		}
	    }
	}
    } else if (Tcl_GetErrno() == ENOENT) {
	attr = GetFileAttributes(path);
	if (attr != 0xffffffff) {
	    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
		/*
		 * Windows 95 reports removing a directory as ENOENT instead
		 * of EISDIR.
		 */

		Tcl_SetErrno(EISDIR);
	    }
	}
    } else if (Tcl_GetErrno() == EINVAL) {
	/*
	 * Windows NT reports removing a char device as EINVAL instead of
	 * EACCES.
	 */

	Tcl_SetErrno(EACCES);
    }

    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjCreateDirectory --
 *
 *	Creates the specified directory. All parent directories of the
 *	specified directory must already exist. The directory is automatically
 *	created with permissions so that user can access the new directory and
 *	create new files or subdirectories in it.
 *
 * Results:
 *	If the directory was successfully created, returns TCL_OK. Otherwise
 *	the return value is TCL_ERROR and errno is set to indicate the error.
 *	Some possible values for errno are:
 *
 *	EACCES:	    a parent directory can't be read and/or written.
 *	EEXIST:	    path already exists.
 *	ENOENT:	    a parent directory doesn't exist.
 *
 * Side effects:
 *	A directory is created.
 *
 *---------------------------------------------------------------------------
 */

int
TclpObjCreateDirectory(
    Tcl_Obj *pathPtr)
{
    return DoCreateDirectory(Tcl_FSGetNativePath(pathPtr));
}

static int
DoCreateDirectory(
    const TCHAR *nativePath)	/* Pathname of directory to create (native). */
{
    if (CreateDirectory(nativePath, NULL) == 0) {
	DWORD error = GetLastError();

	TclWinConvertError(error);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjCopyDirectory --
 *
 *	Recursively copies a directory. The target directory dst must not
 *	already exist. Note that this function does not merge two directory
 *	hierarchies, even if the target directory is an an empty directory.
 *
 * Results:
 *	If the directory was successfully copied, returns TCL_OK. Otherwise
 *	the return value is TCL_ERROR, errno is set to indicate the error, and
 *	the pathname of the file that caused the error is stored in errorPtr.
 *	See TclpCreateDirectory and TclpCopyFile for a description of possible
 *	values for errno.
 *
 * Side effects:
 *	An exact copy of the directory hierarchy src will be created with the
 *	name dst. If an error occurs, the error will be returned immediately,
 *	and remaining files will not be processed.
 *
 *---------------------------------------------------------------------------
 */

int
TclpObjCopyDirectory(
    Tcl_Obj *srcPathPtr,
    Tcl_Obj *destPathPtr,
    Tcl_Obj **errorPtr)
{
    Tcl_DString ds;
    Tcl_DString srcString, dstString;
    Tcl_Obj *normSrcPtr, *normDestPtr;
    int ret;

    normSrcPtr = Tcl_FSGetNormalizedPath(NULL,srcPathPtr);
    normDestPtr = Tcl_FSGetNormalizedPath(NULL,destPathPtr);
    if ((normSrcPtr == NULL) || (normDestPtr == NULL)) {
	return TCL_ERROR;
    }

    Tcl_WinUtfToTChar(Tcl_GetString(normSrcPtr), -1, &srcString);
    Tcl_WinUtfToTChar(Tcl_GetString(normDestPtr), -1, &dstString);

    ret = TraverseWinTree(TraversalCopy, &srcString, &dstString, &ds);

    Tcl_DStringFree(&srcString);
    Tcl_DStringFree(&dstString);

    if (ret != TCL_OK) {
	if (!strcmp(Tcl_DStringValue(&ds), TclGetString(normSrcPtr))) {
	    *errorPtr = srcPathPtr;
	} else if (!strcmp(Tcl_DStringValue(&ds), TclGetString(normDestPtr))) {
	    *errorPtr = destPathPtr;
	} else {
	    *errorPtr = Tcl_NewStringObj(Tcl_DStringValue(&ds), -1);
	}
	Tcl_DStringFree(&ds);
	Tcl_IncrRefCount(*errorPtr);
    }
    return ret;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpObjRemoveDirectory, DoRemoveDirectory --
 *
 *	Removes directory (and its contents, if the recursive flag is set).
 *
 * Results:
 *	If the directory was successfully removed, returns TCL_OK. Otherwise
 *	the return value is TCL_ERROR, errno is set to indicate the error, and
 *	the pathname of the file that caused the error is stored in errorPtr.
 *	Some possible values for errno are:
 *
 *	EACCES:	    path directory can't be read and/or written.
 *	EEXIST:	    path is a non-empty directory.
 *	EINVAL:	    path is root directory or current directory.
 *	ENOENT:	    path doesn't exist or is "".
 *	ENOTDIR:    path is not a directory.
 *
 *	EACCES:	    path is a char device (nul:, com1:, etc.) (95)
 *	EINVAL:	    path is a char device (nul:, com1:, etc.) (NT)
 *
 * Side effects:
 *	Directory removed. If an error occurs, the error will be returned
 *	immediately, and remaining files will not be deleted.
 *
 *----------------------------------------------------------------------
 */

int
TclpObjRemoveDirectory(
    Tcl_Obj *pathPtr,
    int recursive,
    Tcl_Obj **errorPtr)
{
    Tcl_DString ds;
    Tcl_Obj *normPtr = NULL;
    int ret;

    if (recursive) {
	/*
	 * In the recursive case, the string rep is used to construct a
	 * Tcl_DString which may be used extensively, so we can't optimize
	 * this case easily.
	 */

	Tcl_DString native;
	normPtr = Tcl_FSGetNormalizedPath(NULL, pathPtr);
	if (normPtr == NULL) {
	    return TCL_ERROR;
	}
	Tcl_WinUtfToTChar(Tcl_GetString(normPtr), -1, &native);
	ret = DoRemoveDirectory(&native, recursive, &ds);
	Tcl_DStringFree(&native);
    } else {
	ret = DoRemoveJustDirectory(Tcl_FSGetNativePath(pathPtr), 0, &ds);
    }

    if (ret != TCL_OK) {
	if (Tcl_DStringLength(&ds) > 0) {
	    if (normPtr != NULL &&
		    !strcmp(Tcl_DStringValue(&ds), TclGetString(normPtr))) {
		*errorPtr = pathPtr;
	    } else {
		*errorPtr = TclDStringToObj(&ds);
	    }
	    Tcl_IncrRefCount(*errorPtr);
	}
	Tcl_DStringFree(&ds);
    }

    return ret;
}

static int
DoRemoveJustDirectory(
    const TCHAR *nativePath,	/* Pathname of directory to be removed
				 * (native). */
    int ignoreError,		/* If non-zero, don't initialize the errorPtr
				 * under some circumstances on return. */
    Tcl_DString *errorPtr)	/* If non-NULL, uninitialized or free DString
				 * filled with UTF-8 name of file causing
				 * error. */
{
    DWORD attr;

    /*
     * The RemoveDirectory API acts differently under Win95/98 and NT WRT NULL
     * and "". Avoid passing these values.
     */

    if (nativePath == NULL || nativePath[0] == '\0') {
	Tcl_SetErrno(ENOENT);
	Tcl_DStringInit(errorPtr);
	return TCL_ERROR;
    }

    attr = GetFileAttributes(nativePath);

    if (attr & FILE_ATTRIBUTE_REPARSE_POINT) {
	/*
	 * It is a symbolic link - remove it.
	 */
	if (TclWinSymLinkDelete(nativePath, 0) == 0) {
	    return TCL_OK;
	}
    } else {
	/*
	 * Ordinary directory.
	 */

	if (RemoveDirectory(nativePath) != FALSE) {
	    return TCL_OK;
	}
    }

    TclWinConvertError(GetLastError());

    if (Tcl_GetErrno() == EACCES) {
	attr = GetFileAttributes(nativePath);
	if (attr != 0xffffffff) {
	    if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		/*
		 * Windows 95 reports calling RemoveDirectory on a file as an
		 * EACCES, not an ENOTDIR.
		 */

		Tcl_SetErrno(ENOTDIR);
		goto end;
	    }

	    if (attr & FILE_ATTRIBUTE_REPARSE_POINT) {
		/*
		 * It is a symbolic link - remove it.
		 */

		if (TclWinSymLinkDelete(nativePath, 1) != 0) {
		    goto end;
		}
	    }

	    if (attr & FILE_ATTRIBUTE_READONLY) {
		attr &= ~FILE_ATTRIBUTE_READONLY;
		if (SetFileAttributes(nativePath,
			attr) == FALSE) {
		    goto end;
		}
		if (RemoveDirectory(nativePath) != FALSE) {
		    return TCL_OK;
		}
		TclWinConvertError(GetLastError());
		SetFileAttributes(nativePath,
			attr | FILE_ATTRIBUTE_READONLY);
	    }
	}
    }

    if (Tcl_GetErrno() == ENOTEMPTY) {
	/*
	 * The caller depends on EEXIST to signify that the directory is not
	 * empty, not ENOTEMPTY.
	 */

	Tcl_SetErrno(EEXIST);
    }

    if ((ignoreError != 0) && (Tcl_GetErrno() == EEXIST)) {
	/*
	 * If we're being recursive, this error may actually be ok, so we
	 * don't want to initialise the errorPtr yet.
	 */
	return TCL_ERROR;
    }

  end:
    if (errorPtr != NULL) {
	char *p = Tcl_WinTCharToUtf(nativePath, -1, errorPtr);
	for (; *p; ++p) {
	    if (*p == '\\') *p = '/';
	}
    }
    return TCL_ERROR;

}

static int
DoRemoveDirectory(
    Tcl_DString *pathPtr,	/* Pathname of directory to be removed
				 * (native). */
    int recursive,		/* If non-zero, removes directories that are
				 * nonempty. Otherwise, will only remove empty
				 * directories. */
    Tcl_DString *errorPtr)	/* If non-NULL, uninitialized or free DString
				 * filled with UTF-8 name of file causing
				 * error. */
{
    int res = DoRemoveJustDirectory((const TCHAR *)Tcl_DStringValue(pathPtr), recursive,
	    errorPtr);

    if ((res == TCL_ERROR) && (recursive != 0) && (Tcl_GetErrno() == EEXIST)) {
	/*
	 * The directory is nonempty, but the recursive flag has been
	 * specified, so we recursively remove all the files in the directory.
	 */

	return TraverseWinTree(TraversalDelete, pathPtr, NULL, errorPtr);
    } else {
	return res;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TraverseWinTree --
 *
 *	Traverse directory tree specified by sourcePtr, calling the function
 *	traverseProc for each file and directory encountered. If destPtr is
 *	non-null, each of name in the sourcePtr directory is appended to the
 *	directory specified by destPtr and passed as the second argument to
 *	traverseProc().
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	None caused by TraverseWinTree, however the user specified
 *	traverseProc() may change state. If an error occurs, the error will be
 *	returned immediately, and remaining files will not be processed.
 *
 *---------------------------------------------------------------------------
 */

static int
TraverseWinTree(
    TraversalProc *traverseProc,/* Function to call for every file and
				 * directory in source hierarchy. */
    Tcl_DString *sourcePtr,	/* Pathname of source directory to be
				 * traversed (native). */
    Tcl_DString *targetPtr,	/* Pathname of directory to traverse in
				 * parallel with source directory (native),
				 * may be NULL. */
    Tcl_DString *errorPtr)	/* If non-NULL, uninitialized or free DString
				 * filled with UTF-8 name of file causing
				 * error. */
{
    DWORD sourceAttr;
    TCHAR *nativeSource, *nativeTarget, *nativeErrfile;
    int result, found, sourceLen, targetLen = 0, oldSourceLen, oldTargetLen;
    HANDLE handle;
    WIN32_FIND_DATA data;

    nativeErrfile = NULL;
    result = TCL_OK;
    oldTargetLen = 0;		/* lint. */

    nativeSource = (TCHAR *) Tcl_DStringValue(sourcePtr);
    nativeTarget = (TCHAR *)
	    (targetPtr == NULL ? NULL : Tcl_DStringValue(targetPtr));

    oldSourceLen = Tcl_DStringLength(sourcePtr);
    sourceAttr = GetFileAttributes(nativeSource);
    if (sourceAttr == 0xffffffff) {
	nativeErrfile = nativeSource;
	goto end;
    }

    if (sourceAttr & FILE_ATTRIBUTE_REPARSE_POINT) {
	/*
	 * Process the symbolic link
	 */

	return traverseProc(nativeSource, nativeTarget, DOTREE_LINK,
		errorPtr);
    }

    if ((sourceAttr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
	/*
	 * Process the regular file
	 */

	return traverseProc(nativeSource, nativeTarget, DOTREE_F, errorPtr);
    }

    Tcl_DStringAppend(sourcePtr, (char *) TEXT("\\*.*"), 4 * sizeof(TCHAR) + 1);
    Tcl_DStringSetLength(sourcePtr, Tcl_DStringLength(sourcePtr) - 1);

    nativeSource = (TCHAR *) Tcl_DStringValue(sourcePtr);
    handle = FindFirstFile(nativeSource, &data);
    if (handle == INVALID_HANDLE_VALUE) {
	/*
	 * Can't read directory.
	 */

	TclWinConvertError(GetLastError());
	nativeErrfile = nativeSource;
	goto end;
    }

    Tcl_DStringSetLength(sourcePtr, oldSourceLen + 1);
    Tcl_DStringSetLength(sourcePtr, oldSourceLen);
    result = traverseProc(nativeSource, nativeTarget, DOTREE_PRED,
	    errorPtr);
    if (result != TCL_OK) {
	FindClose(handle);
	return result;
    }

    sourceLen = oldSourceLen + sizeof(TCHAR);
    Tcl_DStringAppend(sourcePtr, (char *) TEXT("\\"), sizeof(TCHAR) + 1);
    Tcl_DStringSetLength(sourcePtr, sourceLen);
    if (targetPtr != NULL) {
	oldTargetLen = Tcl_DStringLength(targetPtr);

	targetLen = oldTargetLen;
	targetLen += sizeof(TCHAR);
	Tcl_DStringAppend(targetPtr, (char *) TEXT("\\"), sizeof(TCHAR) + 1);
	Tcl_DStringSetLength(targetPtr, targetLen);
    }

    found = 1;
    for (; found; found = FindNextFile(handle, &data)) {
	TCHAR *nativeName;
	int len;

	TCHAR *wp = data.cFileName;
	if (*wp == '.') {
	    wp++;
	    if (*wp == '.') {
		wp++;
	    }
	    if (*wp == '\0') {
		continue;
	    }
	}
	nativeName = (TCHAR *) data.cFileName;
	len = _tcslen(data.cFileName) * sizeof(TCHAR);

	/*
	 * Append name after slash, and recurse on the file.
	 */

	Tcl_DStringAppend(sourcePtr, (char *) nativeName, len + 1);
	Tcl_DStringSetLength(sourcePtr, Tcl_DStringLength(sourcePtr) - 1);
	if (targetPtr != NULL) {
	    Tcl_DStringAppend(targetPtr, (char *) nativeName, len + 1);
	    Tcl_DStringSetLength(targetPtr, Tcl_DStringLength(targetPtr) - 1);
	}
	result = TraverseWinTree(traverseProc, sourcePtr, targetPtr,
		errorPtr);
	if (result != TCL_OK) {
	    break;
	}

	/*
	 * Remove name after slash.
	 */

	Tcl_DStringSetLength(sourcePtr, sourceLen);
	if (targetPtr != NULL) {
	    Tcl_DStringSetLength(targetPtr, targetLen);
	}
    }
    FindClose(handle);

    /*
     * Strip off the trailing slash we added.
     */

    Tcl_DStringSetLength(sourcePtr, oldSourceLen + 1);
    Tcl_DStringSetLength(sourcePtr, oldSourceLen);
    if (targetPtr != NULL) {
	Tcl_DStringSetLength(targetPtr, oldTargetLen + 1);
	Tcl_DStringSetLength(targetPtr, oldTargetLen);
    }
    if (result == TCL_OK) {
	/*
	 * Call traverseProc() on a directory after visiting all the
	 * files in that directory.
	 */

	result = traverseProc((const TCHAR *)Tcl_DStringValue(sourcePtr),
		(const TCHAR *)(targetPtr == NULL ? NULL : Tcl_DStringValue(targetPtr)),
		DOTREE_POSTD, errorPtr);
    }

  end:
    if (nativeErrfile != NULL) {
	TclWinConvertError(GetLastError());
	if (errorPtr != NULL) {
	    Tcl_WinTCharToUtf(nativeErrfile, -1, errorPtr);
	}
	result = TCL_ERROR;
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TraversalCopy
 *
 *	Called from TraverseUnixTree in order to execute a recursive copy of a
 *	directory.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	Depending on the value of type, src may be copied to dst.
 *
 *----------------------------------------------------------------------
 */

static int
TraversalCopy(
    const TCHAR *nativeSrc,	/* Source pathname to copy. */
    const TCHAR *nativeDst,	/* Destination pathname of copy. */
    int type,			/* Reason for call - see TraverseWinTree() */
    Tcl_DString *errorPtr)	/* If non-NULL, initialized DString filled
				 * with UTF-8 name of file causing error. */
{
    switch (type) {
    case DOTREE_F:
	if (DoCopyFile(nativeSrc, nativeDst) == TCL_OK) {
	    return TCL_OK;
	}
	break;
    case DOTREE_LINK:
	if (TclWinSymLinkCopyDirectory(nativeSrc, nativeDst) == TCL_OK) {
	    return TCL_OK;
	}
	break;
    case DOTREE_PRED:
	if (DoCreateDirectory(nativeDst) == TCL_OK) {
	    DWORD attr = GetFileAttributes(nativeSrc);

	    if (SetFileAttributes(nativeDst,
		    attr) != FALSE) {
		return TCL_OK;
	    }
	    TclWinConvertError(GetLastError());
	}
	break;
    case DOTREE_POSTD:
	return TCL_OK;
    }

    /*
     * There shouldn't be a problem with src, because we already checked it to
     * get here.
     */

    if (errorPtr != NULL) {
	Tcl_WinTCharToUtf(nativeDst, -1, errorPtr);
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TraversalDelete --
 *
 *	Called by function TraverseWinTree for every file and directory that
 *	it encounters in a directory hierarchy. This function unlinks files,
 *	and removes directories after all the containing files have been
 *	processed.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	Files or directory specified by src will be deleted. If an error
 *	occurs, the windows error is converted to a Posix error and errno is
 *	set accordingly.
 *
 *----------------------------------------------------------------------
 */

static int
TraversalDelete(
    const TCHAR *nativeSrc,	/* Source pathname to delete. */
    const TCHAR *dstPtr,	/* Not used. */
    int type,			/* Reason for call - see TraverseWinTree() */
    Tcl_DString *errorPtr)	/* If non-NULL, initialized DString filled
				 * with UTF-8 name of file causing error. */
{
    switch (type) {
    case DOTREE_F:
	if (TclpDeleteFile(nativeSrc) == TCL_OK) {
	    return TCL_OK;
	}
	break;
    case DOTREE_LINK:
	if (DoRemoveJustDirectory(nativeSrc, 0, NULL) == TCL_OK) {
	    return TCL_OK;
	}
	break;
    case DOTREE_PRED:
	return TCL_OK;
    case DOTREE_POSTD:
	if (DoRemoveJustDirectory(nativeSrc, 0, NULL) == TCL_OK) {
	    return TCL_OK;
	}
	break;
    }

    if (errorPtr != NULL) {
	Tcl_WinTCharToUtf(nativeSrc, -1, errorPtr);
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * StatError --
 *
 *	Sets the object result with the appropriate error.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The interp's object result is set with an error message based on the
 *	objIndex, fileName and errno.
 *
 *----------------------------------------------------------------------
 */

static void
StatError(
    Tcl_Interp *interp,		/* The interp that has the error */
    Tcl_Obj *fileName)		/* The name of the file which caused the
				 * error. */
{
    TclWinConvertError(GetLastError());
    Tcl_SetObjResult(interp, Tcl_ObjPrintf("could not read \"%s\": %s",
	    TclGetString(fileName), Tcl_PosixError(interp)));
}

/*
 *----------------------------------------------------------------------
 *
 * GetWinFileAttributes --
 *
 *	Returns a Tcl_Obj containing the value of a file attribute. This
 *	routine gets the -hidden, -readonly or -system attribute.
 *
 * Results:
 *	Standard Tcl result and a Tcl_Obj in attributePtrPtr. The object will
 *	have ref count 0. If the return value is not TCL_OK, attributePtrPtr
 *	is not touched.
 *
 * Side effects:
 *	A new object is allocated if the file is valid.
 *
 *----------------------------------------------------------------------
 */

static int
GetWinFileAttributes(
    Tcl_Interp *interp,		/* The interp we are using for errors. */
    int objIndex,		/* The index of the attribute. */
    Tcl_Obj *fileName,		/* The name of the file. */
    Tcl_Obj **attributePtrPtr)	/* A pointer to return the object with. */
{
    DWORD result;
    const TCHAR *nativeName;
    int attr;

    nativeName = Tcl_FSGetNativePath(fileName);
    result = GetFileAttributes(nativeName);

    if (result == 0xffffffff) {
	StatError(interp, fileName);
	return TCL_ERROR;
    }

    attr = (int)(result & attributeArray[objIndex]);
    if ((objIndex == WIN_HIDDEN_ATTRIBUTE) && (attr != 0)) {
	/*
	 * It is hidden. However there is a bug on some Windows OSes in which
	 * root volumes (drives) formatted as NTFS are declared hidden when
	 * they are not (and cannot be).
	 *
	 * We test for, and fix that case, here.
	 */

	int len;
	const char *str = Tcl_GetStringFromObj(fileName,&len);

	if (len < 4) {
	    if (len == 0) {
		/*
		 * Not sure if this is possible, but we pass it on anyway.
		 */
	    } else if (len == 1 && (str[0] == '/' || str[0] == '\\')) {
		/*
		 * Path is pointing to the root volume.
		 */

		attr = 0;
	    } else if ((str[1] == ':')
		    && (len == 2 || (str[2] == '/' || str[2] == '\\'))) {
		/*
		 * Path is of the form 'x:' or 'x:/' or 'x:\'
		 */

		attr = 0;
	    }
	}
    }

    *attributePtrPtr = Tcl_NewBooleanObj(attr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ConvertFileNameFormat --
 *
 *	Returns a Tcl_Obj containing either the long or short version of the
 *	file name.
 *
 * Results:
 *	Standard Tcl result and a Tcl_Obj in attributePtrPtr. The object will
 *	have ref count 0. If the return value is not TCL_OK, attributePtrPtr
 *	is not touched.
 *
 *	Warning: if you pass this function a drive name like 'c:' it will
 *	actually return the current working directory on that drive. To avoid
 *	this, make sure the drive name ends in a slash, like this 'c:/'.
 *
 * Side effects:
 *	A new object is allocated if the file is valid.
 *
 *----------------------------------------------------------------------
 */

static int
ConvertFileNameFormat(
    Tcl_Interp *interp,		/* The interp we are using for errors. */
    int objIndex,		/* The index of the attribute. */
    Tcl_Obj *fileName,		/* The name of the file. */
    int longShort,		/* 0 to short name, 1 to long name. */
    Tcl_Obj **attributePtrPtr)	/* A pointer to return the object with. */
{
    int pathc, i;
    Tcl_Obj *splitPath;

    splitPath = Tcl_FSSplitPath(fileName, &pathc);

    if (splitPath == NULL || pathc == 0) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "could not read \"%s\": no such file or directory",
		    Tcl_GetString(fileName)));
	    errno = ENOENT;
	    Tcl_PosixError(interp);
	}
	goto cleanup;
    }

    /*
     * We will decrement this again at the end.	 It is safer to do this in
     * case any of the calls below retain a reference to splitPath.
     */

    Tcl_IncrRefCount(splitPath);

    for (i = 0; i < pathc; i++) {
	Tcl_Obj *elt;
	char *pathv;
	int pathLen;

	Tcl_ListObjIndex(NULL, splitPath, i, &elt);

	pathv = Tcl_GetStringFromObj(elt, &pathLen);
	if ((pathv[0] == '/') || ((pathLen == 3) && (pathv[1] == ':'))
		|| (strcmp(pathv, ".") == 0) || (strcmp(pathv, "..") == 0)) {
	    /*
	     * Handle "/", "//machine/export", "c:/", "." or ".." by just
	     * copying the string literally.  Uppercase the drive letter, just
	     * because it looks better under Windows to do so.
	     */

	simple:
	    /*
	     * Here we are modifying the string representation in place.
	     *
	     * I believe this is legal, since this won't affect any file
	     * representation this thing may have.
	     */

	    pathv[0] = (char) Tcl_UniCharToUpper(UCHAR(pathv[0]));
	} else {
	    Tcl_Obj *tempPath;
	    Tcl_DString ds;
	    Tcl_DString dsTemp;
	    const TCHAR *nativeName;
	    const char *tempString;
	    int tempLen;
	    WIN32_FIND_DATA data;
	    HANDLE handle;
	    DWORD attr;

	    tempPath = Tcl_FSJoinPath(splitPath, i+1);
	    Tcl_IncrRefCount(tempPath);

	    /*
	     * We'd like to call Tcl_FSGetNativePath(tempPath) but that is
	     * likely to lead to infinite loops.
	     */

	    Tcl_DStringInit(&ds);
	    tempString = Tcl_GetStringFromObj(tempPath,&tempLen);
	    nativeName = Tcl_WinUtfToTChar(tempString, tempLen, &ds);
	    Tcl_DecrRefCount(tempPath);
	    handle = FindFirstFile(nativeName, &data);
	    if (handle == INVALID_HANDLE_VALUE) {
		/*
		 * FindFirstFile() doesn't like root directories. We would
		 * only get a root directory here if the caller specified "c:"
		 * or "c:." and the current directory on the drive was the
		 * root directory
		 */

		attr = GetFileAttributes(nativeName);
		if ((attr!=0xFFFFFFFF) && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
		    Tcl_DStringFree(&ds);
		    goto simple;
		}
	    }

	    if (handle == INVALID_HANDLE_VALUE) {
		Tcl_DStringFree(&ds);
		if (interp != NULL) {
		    StatError(interp, fileName);
		}
		goto cleanup;
	    }
	    nativeName = data.cAlternateFileName;
	    if (longShort) {
		if (data.cFileName[0] != '\0') {
		    nativeName = data.cFileName;
		}
	    } else {
		if (data.cAlternateFileName[0] == '\0') {
		    nativeName = (TCHAR *) data.cFileName;
		}
	    }

	    /*
	     * Purify reports a extraneous UMR in Tcl_WinTCharToUtf() trying
	     * to dereference nativeName as a Unicode string. I have proven to
	     * myself that purify is wrong by running the following example
	     * when nativeName == data.w.cAlternateFileName and noting that
	     * purify doesn't complain about the first line, but does complain
	     * about the second.
	     *
	     *	fprintf(stderr, "%d\n", data.w.cAlternateFileName[0]);
	     *	fprintf(stderr, "%d\n", ((WCHAR *) nativeName)[0]);
	     */

	    Tcl_DStringInit(&dsTemp);
	    Tcl_WinTCharToUtf(nativeName, -1, &dsTemp);
	    Tcl_DStringFree(&ds);

	    /*
	     * Deal with issues of tildes being absolute.
	     */

	    if (Tcl_DStringValue(&dsTemp)[0] == '~') {
		TclNewLiteralStringObj(tempPath, "./");
		Tcl_AppendToObj(tempPath, Tcl_DStringValue(&dsTemp),
			Tcl_DStringLength(&dsTemp));
		Tcl_DStringFree(&dsTemp);
	    } else {
		tempPath = TclDStringToObj(&dsTemp);
	    }
	    Tcl_ListObjReplace(NULL, splitPath, i, 1, 1, &tempPath);
	    FindClose(handle);
	}
    }

    *attributePtrPtr = Tcl_FSJoinPath(splitPath, -1);

    if (splitPath != NULL) {
	/*
	 * Unfortunately, the object we will return may have its only refCount
	 * as part of the list splitPath. This means if we free splitPath, the
	 * object will disappear. So, we have to be very careful here.
	 * Unfortunately this means we must manipulate the object's refCount
	 * directly.
	 */

	Tcl_IncrRefCount(*attributePtrPtr);
	Tcl_DecrRefCount(splitPath);
	--(*attributePtrPtr)->refCount;
    }
    return TCL_OK;

  cleanup:
    if (splitPath != NULL) {
	Tcl_DecrRefCount(splitPath);
    }

    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * GetWinFileLongName --
 *
 *	Returns a Tcl_Obj containing the long version of the file name.
 *
 * Results:
 *	Standard Tcl result and a Tcl_Obj in attributePtrPtr. The object will
 *	have ref count 0. If the return value is not TCL_OK, attributePtrPtr
 *	is not touched.
 *
 * Side effects:
 *	A new object is allocated if the file is valid.
 *
 *----------------------------------------------------------------------
 */

static int
GetWinFileLongName(
    Tcl_Interp *interp,		/* The interp we are using for errors. */
    int objIndex,		/* The index of the attribute. */
    Tcl_Obj *fileName,		/* The name of the file. */
    Tcl_Obj **attributePtrPtr)	/* A pointer to return the object with. */
{
    return ConvertFileNameFormat(interp, objIndex, fileName, 1,
	    attributePtrPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * GetWinFileShortName --
 *
 *	Returns a Tcl_Obj containing the short version of the file name.
 *
 * Results:
 *	Standard Tcl result and a Tcl_Obj in attributePtrPtr. The object will
 *	have ref count 0. If the return value is not TCL_OK, attributePtrPtr
 *	is not touched.
 *
 * Side effects:
 *	A new object is allocated if the file is valid.
 *
 *----------------------------------------------------------------------
 */

static int
GetWinFileShortName(
    Tcl_Interp *interp,		/* The interp we are using for errors. */
    int objIndex,		/* The index of the attribute. */
    Tcl_Obj *fileName,		/* The name of the file. */
    Tcl_Obj **attributePtrPtr)	/* A pointer to return the object with. */
{
    return ConvertFileNameFormat(interp, objIndex, fileName, 0,
	    attributePtrPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * SetWinFileAttributes --
 *
 *	Set the file attributes to the value given by attributePtr. This
 *	routine sets the -hidden, -readonly, or -system attributes.
 *
 * Results:
 *	Standard TCL error.
 *
 * Side effects:
 *	The file's attribute is set.
 *
 *----------------------------------------------------------------------
 */

static int
SetWinFileAttributes(
    Tcl_Interp *interp,		/* The interp we are using for errors. */
    int objIndex,		/* The index of the attribute. */
    Tcl_Obj *fileName,		/* The name of the file. */
    Tcl_Obj *attributePtr)	/* The new value of the attribute. */
{
    DWORD fileAttributes, old;
    int yesNo, result;
    const TCHAR *nativeName;

    nativeName = Tcl_FSGetNativePath(fileName);
    fileAttributes = old = GetFileAttributes(nativeName);

    if (fileAttributes == 0xffffffff) {
	StatError(interp, fileName);
	return TCL_ERROR;
    }

    result = Tcl_GetBooleanFromObj(interp, attributePtr, &yesNo);
    if (result != TCL_OK) {
	return result;
    }

    if (yesNo) {
	fileAttributes |= (attributeArray[objIndex]);
    } else {
	fileAttributes &= ~(attributeArray[objIndex]);
    }

    if ((fileAttributes != old)
	    && !SetFileAttributes(nativeName, fileAttributes)) {
	StatError(interp, fileName);
	return TCL_ERROR;
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * SetWinFileLongName --
 *
 *	The attribute in question is a readonly attribute and cannot be set.
 *
 * Results:
 *	TCL_ERROR
 *
 * Side effects:
 *	The object result is set to a pertinent error message.
 *
 *----------------------------------------------------------------------
 */

static int
CannotSetAttribute(
    Tcl_Interp *interp,		/* The interp we are using for errors. */
    int objIndex,		/* The index of the attribute. */
    Tcl_Obj *fileName,		/* The name of the file. */
    Tcl_Obj *attributePtr)	/* The new value of the attribute. */
{
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "cannot set attribute \"%s\" for file \"%s\": attribute is readonly",
	    tclpFileAttrStrings[objIndex], Tcl_GetString(fileName)));
    errno = EINVAL;
    Tcl_PosixError(interp);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjListVolumes --
 *
 *	Lists the currently mounted volumes
 *
 * Results:
 *	The list of volumes.
 *
 * Side effects:
 *	None
 *
 *---------------------------------------------------------------------------
 */

Tcl_Obj *
TclpObjListVolumes(void)
{
    Tcl_Obj *resultPtr, *elemPtr;
    char buf[40 * 4];		/* There couldn't be more than 30 drives??? */
    int i;
    char *p;

    resultPtr = Tcl_NewObj();

    /*
     * On Win32s:
     * GetLogicalDriveStrings() isn't implemented.
     * GetLogicalDrives() returns incorrect information.
     */

    if (GetLogicalDriveStringsA(sizeof(buf), buf) == 0) {
	/*
	 * GetVolumeInformation() will detects all drives, but causes
	 * chattering on empty floppy drives. We only do this if
	 * GetLogicalDriveStrings() didn't work. It has also been reported
	 * that on some laptops it takes a while for GetVolumeInformation() to
	 * return when pinging an empty floppy drive, another reason to try to
	 * avoid calling it.
	 */

	buf[1] = ':';
	buf[2] = '/';
	buf[3] = '\0';

	for (i = 0; i < 26; i++) {
	    buf[0] = (char) ('a' + i);
	    if (GetVolumeInformationA(buf, NULL, 0, NULL, NULL, NULL, NULL, 0)
		    || (GetLastError() == ERROR_NOT_READY)) {
		elemPtr = Tcl_NewStringObj(buf, -1);
		Tcl_ListObjAppendElement(NULL, resultPtr, elemPtr);
	    }
	}
    } else {
	for (p = buf; *p != '\0'; p += 4) {
	    p[2] = '/';
	    elemPtr = Tcl_NewStringObj(p, -1);
	    Tcl_ListObjAppendElement(NULL, resultPtr, elemPtr);
	}
    }

    Tcl_IncrRefCount(resultPtr);
    return resultPtr;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
