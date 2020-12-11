/*
 * tclMacOSXFCmd.c
 *
 *	This file implements the MacOSX specific portion of file manipulation
 *	subcommands of the "file" command.
 *
 * Copyright (c) 2003-2007 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

#ifdef HAVE_GETATTRLIST
#include <sys/attr.h>
#include <sys/paths.h>
#include <libkern/OSByteOrder.h>
#endif

/* Darwin 8 copyfile API. */
#ifdef HAVE_COPYFILE
#ifdef HAVE_COPYFILE_H
#include <copyfile.h>
#if defined(HAVE_WEAK_IMPORT) && (MAC_OS_X_VERSION_MIN_REQUIRED < 1040)
/* Support for weakly importing copyfile. */
#define WEAK_IMPORT_COPYFILE
extern int		copyfile(const char *from, const char *to,
			    copyfile_state_t state, copyfile_flags_t flags)
			    WEAK_IMPORT_ATTRIBUTE;
#endif /* HAVE_WEAK_IMPORT */
#else /* HAVE_COPYFILE_H */
int			copyfile(const char *from, const char *to,
			    void *state, uint32_t flags);
#define COPYFILE_ACL		(1<<0)
#define COPYFILE_XATTR		(1<<2)
#define COPYFILE_NOFOLLOW_SRC	(1<<18)
#if defined(HAVE_WEAK_IMPORT) && (MAC_OS_X_VERSION_MIN_REQUIRED < 1040)
/* Support for weakly importing copyfile. */
#define WEAK_IMPORT_COPYFILE
extern int		copyfile(const char *from, const char *to,
			    void *state, uint32_t flags)
			    WEAK_IMPORT_ATTRIBUTE;
#endif /* HAVE_WEAK_IMPORT */
#endif /* HAVE_COPYFILE_H */
#endif /* HAVE_COPYFILE */

#ifdef WEAK_IMPORT_COPYFILE
#define MayUseCopyFile()	(copyfile != NULL)
#elif defined(HAVE_COPYFILE)
#define MayUseCopyFile()	(1)
#else
#define MayUseCopyFile()	(0)
#endif

#include <libkern/OSByteOrder.h>

/*
 * Constants for file attributes subcommand. Need to be kept in sync with
 * tclUnixFCmd.c !
 */

enum {
    UNIX_GROUP_ATTRIBUTE,
    UNIX_OWNER_ATTRIBUTE,
    UNIX_PERMISSIONS_ATTRIBUTE,
#ifdef HAVE_CHFLAGS
    UNIX_READONLY_ATTRIBUTE,
#endif
#ifdef MAC_OSX_TCL
    MACOSX_CREATOR_ATTRIBUTE,
    MACOSX_TYPE_ATTRIBUTE,
    MACOSX_HIDDEN_ATTRIBUTE,
    MACOSX_RSRCLENGTH_ATTRIBUTE,
#endif
};

typedef u_int32_t OSType;

static int		GetOSTypeFromObj(Tcl_Interp *interp,
			    Tcl_Obj *objPtr, OSType *osTypePtr);
static Tcl_Obj *	NewOSTypeObj(const OSType newOSType);
static int		SetOSTypeFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr);
static void		UpdateStringOfOSType(Tcl_Obj *objPtr);

static const Tcl_ObjType tclOSTypeType = {
    "osType",				/* name */
    NULL,				/* freeIntRepProc */
    NULL,				/* dupIntRepProc */
    UpdateStringOfOSType,		/* updateStringProc */
    SetOSTypeFromAny			/* setFromAnyProc */
};

enum {
    kIsInvisible = 0x4000,
};

#define kFinfoIsInvisible	(OSSwapHostToBigConstInt16(kIsInvisible))

typedef	struct finderinfo {
    u_int32_t type;
    u_int32_t creator;
    u_int16_t fdFlags;
    u_int32_t location;
    u_int16_t reserved;
    u_int32_t extendedFileInfo[4];
} __attribute__ ((__packed__)) finderinfo;

typedef struct fileinfobuf {
    u_int32_t info_length;
    u_int32_t data[8];
} fileinfobuf;

/*
 *----------------------------------------------------------------------
 *
 * TclMacOSXGetFileAttribute
 *
 *	Gets a MacOSX attribute of a file. Which attribute is controlled by
 *	objIndex. The object will have ref count 0.
 *
 * Results:
 *	Standard TCL result. Returns a new Tcl_Obj in attributePtrPtr if there
 *	is no error.
 *
 * Side effects:
 *	A new object is allocated.
 *
 *----------------------------------------------------------------------
 */

int
TclMacOSXGetFileAttribute(
    Tcl_Interp *interp,		 /* The interp we are using for errors. */
    int objIndex,		 /* The index of the attribute. */
    Tcl_Obj *fileName,		 /* The name of the file (UTF-8). */
    Tcl_Obj **attributePtrPtr)	 /* A pointer to return the object with. */
{
#ifdef HAVE_GETATTRLIST
    int result;
    Tcl_StatBuf statBuf;
    struct attrlist alist;
    fileinfobuf finfo;
    finderinfo *finder = (finderinfo *) &finfo.data;
    off_t *rsrcForkSize = (off_t *) &finfo.data;
    const char *native;

    result = TclpObjStat(fileName, &statBuf);

    if (result != 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"could not read \"%s\": %s",
		TclGetString(fileName), Tcl_PosixError(interp)));
	return TCL_ERROR;
    }

    if (S_ISDIR(statBuf.st_mode) && objIndex != MACOSX_HIDDEN_ATTRIBUTE) {
	/*
	 * Directories only support attribute "-hidden".
	 */

	errno = EISDIR;
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"invalid attribute: %s", Tcl_PosixError(interp)));
	return TCL_ERROR;
    }

    bzero(&alist, sizeof(struct attrlist));
    alist.bitmapcount = ATTR_BIT_MAP_COUNT;
    if (objIndex == MACOSX_RSRCLENGTH_ATTRIBUTE) {
	alist.fileattr = ATTR_FILE_RSRCLENGTH;
    } else {
	alist.commonattr = ATTR_CMN_FNDRINFO;
    }
    native = Tcl_FSGetNativePath(fileName);
    result = getattrlist(native, &alist, &finfo, sizeof(fileinfobuf), 0);

    if (result != 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"could not read attributes of \"%s\": %s",
		TclGetString(fileName), Tcl_PosixError(interp)));
	return TCL_ERROR;
    }

    switch (objIndex) {
    case MACOSX_CREATOR_ATTRIBUTE:
	*attributePtrPtr = NewOSTypeObj(
		OSSwapBigToHostInt32(finder->creator));
	break;
    case MACOSX_TYPE_ATTRIBUTE:
	*attributePtrPtr = NewOSTypeObj(
		OSSwapBigToHostInt32(finder->type));
	break;
    case MACOSX_HIDDEN_ATTRIBUTE:
	*attributePtrPtr = Tcl_NewBooleanObj(
		(finder->fdFlags & kFinfoIsInvisible) != 0);
	break;
    case MACOSX_RSRCLENGTH_ATTRIBUTE:
	*attributePtrPtr = Tcl_NewWideIntObj(*rsrcForkSize);
	break;
    }
    return TCL_OK;
#else
    Tcl_SetObjResult(interp, Tcl_NewStringObj(
	    "Mac OS X file attributes not supported", -1));
    Tcl_SetErrorCode(interp, "TCL", "UNSUPPORTED", NULL);
    return TCL_ERROR;
#endif /* HAVE_GETATTRLIST */
}

/*
 *---------------------------------------------------------------------------
 *
 * TclMacOSXSetFileAttribute --
 *
 *	Sets a MacOSX attribute of a file. Which attribute is controlled by
 *	objIndex.
 *
 * Results:
 *	Standard TCL result.
 *
 * Side effects:
 *	As above.
 *
 *---------------------------------------------------------------------------
 */

int
TclMacOSXSetFileAttribute(
    Tcl_Interp *interp,		/* The interp for error reporting. */
    int objIndex,		/* The index of the attribute. */
    Tcl_Obj *fileName,		/* The name of the file (UTF-8). */
    Tcl_Obj *attributePtr)	/* New owner for file. */
{
#ifdef HAVE_GETATTRLIST
    int result;
    Tcl_StatBuf statBuf;
    struct attrlist alist;
    fileinfobuf finfo;
    finderinfo *finder = (finderinfo *) &finfo.data;
    off_t *rsrcForkSize = (off_t *) &finfo.data;
    const char *native;

    result = TclpObjStat(fileName, &statBuf);

    if (result != 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"could not read \"%s\": %s",
		TclGetString(fileName), Tcl_PosixError(interp)));
	return TCL_ERROR;
    }

    if (S_ISDIR(statBuf.st_mode) && objIndex != MACOSX_HIDDEN_ATTRIBUTE) {
	/*
	 * Directories only support attribute "-hidden".
	 */

	errno = EISDIR;
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"invalid attribute: %s", Tcl_PosixError(interp)));
	return TCL_ERROR;
    }

    bzero(&alist, sizeof(struct attrlist));
    alist.bitmapcount = ATTR_BIT_MAP_COUNT;
    if (objIndex == MACOSX_RSRCLENGTH_ATTRIBUTE) {
	alist.fileattr = ATTR_FILE_RSRCLENGTH;
    } else {
	alist.commonattr = ATTR_CMN_FNDRINFO;
    }
    native = Tcl_FSGetNativePath(fileName);
    result = getattrlist(native, &alist, &finfo, sizeof(fileinfobuf), 0);

    if (result != 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"could not read attributes of \"%s\": %s",
		TclGetString(fileName), Tcl_PosixError(interp)));
	return TCL_ERROR;
    }

    if (objIndex != MACOSX_RSRCLENGTH_ATTRIBUTE) {
	OSType t;
	int h;

	switch (objIndex) {
	case MACOSX_CREATOR_ATTRIBUTE:
	    if (GetOSTypeFromObj(interp, attributePtr, &t) != TCL_OK) {
		return TCL_ERROR;
	    }
	    finder->creator = OSSwapHostToBigInt32(t);
	    break;
	case MACOSX_TYPE_ATTRIBUTE:
	    if (GetOSTypeFromObj(interp, attributePtr, &t) != TCL_OK) {
		return TCL_ERROR;
	    }
	    finder->type = OSSwapHostToBigInt32(t);
	    break;
	case MACOSX_HIDDEN_ATTRIBUTE:
	    if (Tcl_GetBooleanFromObj(interp, attributePtr, &h) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (h) {
		finder->fdFlags |= kFinfoIsInvisible;
	    } else {
		finder->fdFlags &= ~kFinfoIsInvisible;
	    }
	    break;
	}

	result = setattrlist(native, &alist,
		&finfo.data, sizeof(finfo.data), 0);

	if (result != 0) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "could not set attributes of \"%s\": %s",
		    TclGetString(fileName), Tcl_PosixError(interp)));
	    return TCL_ERROR;
	}
    } else {
	Tcl_WideInt newRsrcForkSize;

	if (TclGetWideIntFromObj(interp, attributePtr,
		&newRsrcForkSize) != TCL_OK) {
	    return TCL_ERROR;
	}

	if (newRsrcForkSize != *rsrcForkSize) {
	    Tcl_DString ds;

	    /*
	     * Only setting rsrclength to 0 to strip a file's resource fork is
	     * supported.
	     */

	    if (newRsrcForkSize != 0) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"setting nonzero rsrclength not supported", -1));
		Tcl_SetErrorCode(interp, "TCL", "UNSUPPORTED", NULL);
		return TCL_ERROR;
	    }

	    /*
	     * Construct path to resource fork.
	     */

	    Tcl_DStringInit(&ds);
	    Tcl_DStringAppend(&ds, native, -1);
	    Tcl_DStringAppend(&ds, _PATH_RSRCFORKSPEC, -1);

	    result = truncate(Tcl_DStringValue(&ds), (off_t)0);
	    if (result != 0) {
		/*
		 * truncate() on a valid resource fork path may fail with a
		 * permission error in some OS releases, try truncating with
		 * open() instead:
		 */

		int fd = open(Tcl_DStringValue(&ds), O_WRONLY | O_TRUNC);

		if (fd > 0) {
		    result = close(fd);
		}
	    }

	    Tcl_DStringFree(&ds);

	    if (result != 0) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"could not truncate resource fork of \"%s\": %s",
			TclGetString(fileName), Tcl_PosixError(interp)));
		return TCL_ERROR;
	    }
	}
    }
    return TCL_OK;
#else
    Tcl_SetObjResult(interp, Tcl_NewStringObj(
	    "Mac OS X file attributes not supported", -1));
    Tcl_SetErrorCode(interp, "TCL", "UNSUPPORTED", NULL);
    return TCL_ERROR;
#endif
}

/*
 *---------------------------------------------------------------------------
 *
 * TclMacOSXCopyFileAttributes --
 *
 *	Copy the MacOSX attributes and resource fork (if present) from one
 *	file to another.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	MacOSX attributes and resource fork are updated in the new file to
 *	reflect the old file.
 *
 *---------------------------------------------------------------------------
 */

int
TclMacOSXCopyFileAttributes(
    const char *src,		/* Path name of source file (native). */
    const char *dst,		/* Path name of target file (native). */
    const Tcl_StatBuf *statBufPtr)
				/* Stat info for source file */
{
    if (MayUseCopyFile()) {
#ifdef HAVE_COPYFILE
	if (0 == copyfile(src, dst, NULL, (S_ISLNK(statBufPtr->st_mode)
		? COPYFILE_XATTR | COPYFILE_NOFOLLOW_SRC
		: COPYFILE_XATTR | COPYFILE_ACL))) {
	    return TCL_OK;
	}
#endif /* HAVE_COPYFILE */
    } else {
#if (!defined(HAVE_COPYFILE) || defined(WEAK_IMPORT_COPYFILE)) && defined(HAVE_GETATTRLIST)
	struct attrlist alist;
	fileinfobuf finfo;
	off_t *rsrcForkSize = (off_t *) &finfo.data;
	Tcl_DString srcBuf, dstBuf;
	int result;

	bzero(&alist, sizeof(struct attrlist));
	alist.bitmapcount = ATTR_BIT_MAP_COUNT;
	alist.commonattr = ATTR_CMN_FNDRINFO;

	if (getattrlist(src, &alist, &finfo, sizeof(fileinfobuf), 0)) {
	    return TCL_ERROR;
	}
	if (setattrlist(dst, &alist, &finfo.data, sizeof(finfo.data), 0)) {
	    return TCL_ERROR;
	}

	/*
	 * If we're a directory, we're done as they never have resource forks.
	 */

	if (S_ISDIR(statBufPtr->st_mode)) {
	    return TCL_OK;
	}

	/*
	 * We only copy a non-empty resource fork, so determine if that's the
	 * case first.
	 */

	alist.commonattr = 0;
	alist.fileattr = ATTR_FILE_RSRCLENGTH;
	if (getattrlist(src, &alist, &finfo, sizeof(fileinfobuf), 0)) {
	    return TCL_ERROR;
	} else if (*rsrcForkSize == 0) {
	    return TCL_OK;
	}

	/*
	 * Construct paths to resource forks.
	 */

	Tcl_DStringInit(&srcBuf);
	Tcl_DStringAppend(&srcBuf, src, -1);
	Tcl_DStringAppend(&srcBuf, _PATH_RSRCFORKSPEC, -1);
	Tcl_DStringInit(&dstBuf);
	Tcl_DStringAppend(&dstBuf, dst, -1);
	Tcl_DStringAppend(&dstBuf, _PATH_RSRCFORKSPEC, -1);

	/*
	 * Do the copy.
	 */

	result = TclUnixCopyFile(Tcl_DStringValue(&srcBuf),
		Tcl_DStringValue(&dstBuf), statBufPtr, 1);
	Tcl_DStringFree(&srcBuf);
	Tcl_DStringFree(&dstBuf);
	if (result == 0) {
	    return TCL_OK;
	}
#endif /* (!HAVE_COPYFILE || WEAK_IMPORT_COPYFILE) && HAVE_GETATTRLIST */
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TclMacOSXMatchType --
 *
 *	This routine is used by the globbing code to check if a file matches a
 *	given mac type and/or creator code.
 *
 * Results:
 *	The return value is 1, 0 or -1 indicating whether the file matches the
 *	given criteria, does not match them, or an error occurred (in wich
 *	case an error is left in interp).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclMacOSXMatchType(
    Tcl_Interp *interp,		/* Interpreter to receive errors. */
    const char *pathName,	/* Native path to check. */
    const char *fileName,	/* Native filename to check. */
    Tcl_StatBuf *statBufPtr,	/* Stat info for file to check */
    Tcl_GlobTypeData *types)	/* Type description to match against. */
{
#ifdef HAVE_GETATTRLIST
    struct attrlist alist;
    fileinfobuf finfo;
    finderinfo *finder = (finderinfo *) &finfo.data;
    OSType osType;

    bzero(&alist, sizeof(struct attrlist));
    alist.bitmapcount = ATTR_BIT_MAP_COUNT;
    alist.commonattr = ATTR_CMN_FNDRINFO;
    if (getattrlist(pathName, &alist, &finfo, sizeof(fileinfobuf), 0) != 0) {
	return 0;
    }
    if ((types->perm & TCL_GLOB_PERM_HIDDEN) &&
	    !((finder->fdFlags & kFinfoIsInvisible) || (*fileName == '.'))) {
	return 0;
    }
    if (S_ISDIR(statBufPtr->st_mode)
	    && (types->macType || types->macCreator)) {
	/*
	 * Directories don't support types or creators.
	 */

	return 0;
    }
    if (types->macType) {
	if (GetOSTypeFromObj(interp, types->macType, &osType) != TCL_OK) {
	    return -1;
	}
	if (osType != OSSwapBigToHostInt32(finder->type)) {
	    return 0;
	}
    }
    if (types->macCreator) {
	if (GetOSTypeFromObj(interp, types->macCreator, &osType) != TCL_OK) {
	    return -1;
	}
	if (osType != OSSwapBigToHostInt32(finder->creator)) {
	    return 0;
	}
    }
#endif
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * GetOSTypeFromObj --
 *
 *	Attempt to return an OSType from the Tcl object "objPtr".
 *
 * Results:
 *	Standard TCL result. If an error occurs during conversion, an error
 *	message is left in interp->objResult.
 *
 * Side effects:
 *	The string representation of objPtr will be updated if necessary.
 *
 *----------------------------------------------------------------------
 */

static int
GetOSTypeFromObj(
    Tcl_Interp *interp,		/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr,		/* The object from which to get an OSType. */
    OSType *osTypePtr)		/* Place to store resulting OSType. */
{
    int result = TCL_OK;

    if (objPtr->typePtr != &tclOSTypeType) {
	result = SetOSTypeFromAny(interp, objPtr);
    }
    *osTypePtr = (OSType) objPtr->internalRep.longValue;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * NewOSTypeObj --
 *
 *	Create a new OSType object.
 *
 * Results:
 *	The newly created OSType object is returned, it has ref count 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
NewOSTypeObj(
    const OSType osType)	/* OSType used to initialize the new
				 * object. */
{
    Tcl_Obj *objPtr;

    TclNewObj(objPtr);
    TclInvalidateStringRep(objPtr);
    objPtr->internalRep.longValue = (long) osType;
    objPtr->typePtr = &tclOSTypeType;
    return objPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * SetOSTypeFromAny --
 *
 *	Attempts to force the internal representation for a Tcl object to
 *	tclOSTypeType, specifically.
 *
 * Results:
 *	The return value is a standard object Tcl result. If an error occurs
 *	during conversion, an error message is left in the interpreter's
 *	result unless "interp" is NULL.
 *
 *----------------------------------------------------------------------
 */

static int
SetOSTypeFromAny(
    Tcl_Interp *interp,		/* Tcl interpreter */
    Tcl_Obj *objPtr)		/* Pointer to the object to convert */
{
    const char *string;
    int length, result = TCL_OK;
    Tcl_DString ds;
    Tcl_Encoding encoding = Tcl_GetEncoding(NULL, "macRoman");

    string = Tcl_GetStringFromObj(objPtr, &length);
    Tcl_UtfToExternalDString(encoding, string, length, &ds);

    if (Tcl_DStringLength(&ds) > 4) {
	if (interp) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "expected Macintosh OS type but got \"%s\": ", string));
	    Tcl_SetErrorCode(interp, "TCL", "VALUE", "MAC_OSTYPE", NULL);
	}
	result = TCL_ERROR;
    } else {
	OSType osType;
	char bytes[4] = {'\0','\0','\0','\0'};

	memcpy(bytes, Tcl_DStringValue(&ds), (size_t)Tcl_DStringLength(&ds));
	osType = (OSType) bytes[0] << 24 |
		 (OSType) bytes[1] << 16 |
		 (OSType) bytes[2] <<  8 |
		 (OSType) bytes[3];
	TclFreeIntRep(objPtr);
	objPtr->internalRep.longValue = (long) osType;
	objPtr->typePtr = &tclOSTypeType;
    }
    Tcl_DStringFree(&ds);
    Tcl_FreeEncoding(encoding);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateStringOfOSType --
 *
 *	Update the string representation for an OSType object. Note: This
 *	function does not free an existing old string rep so storage will be
 *	lost if this has not already been done.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The object's string is set to a valid string that results from the
 *	OSType-to-string conversion.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateStringOfOSType(
    register Tcl_Obj *objPtr)	/* OSType object whose string rep to
				 * update. */
{
    char string[5];
    OSType osType = (OSType) objPtr->internalRep.longValue;
    Tcl_DString ds;
    Tcl_Encoding encoding = Tcl_GetEncoding(NULL, "macRoman");
    unsigned len;

    string[0] = (char) (osType >> 24);
    string[1] = (char) (osType >> 16);
    string[2] = (char) (osType >>  8);
    string[3] = (char) (osType);
    string[4] = '\0';
    Tcl_ExternalToUtfDString(encoding, string, -1, &ds);
    len = (unsigned) Tcl_DStringLength(&ds) + 1;
    objPtr->bytes = ckalloc(len);
    memcpy(objPtr->bytes, Tcl_DStringValue(&ds), len);
    objPtr->length = Tcl_DStringLength(&ds);
    Tcl_DStringFree(&ds);
    Tcl_FreeEncoding(encoding);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
