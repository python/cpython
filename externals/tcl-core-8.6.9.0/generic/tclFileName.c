/*
 * tclFileName.c --
 *
 *	This file contains routines for converting file names betwen native
 *	and network form.
 *
 * Copyright (c) 1995-1998 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclRegexp.h"
#include "tclFileSystem.h" /* For TclGetPathType() */

/*
 * The following variable is set in the TclPlatformInit call to one of:
 * TCL_PLATFORM_UNIX or TCL_PLATFORM_WINDOWS.
 */

TclPlatformType tclPlatform = TCL_PLATFORM_UNIX;

/*
 * Prototypes for local procedures defined in this file:
 */

static const char *	DoTildeSubst(Tcl_Interp *interp,
			    const char *user, Tcl_DString *resultPtr);
static const char *	ExtractWinRoot(const char *path,
			    Tcl_DString *resultPtr, int offset,
			    Tcl_PathType *typePtr);
static int		SkipToChar(char **stringPtr, int match);
static Tcl_Obj *	SplitWinPath(const char *path);
static Tcl_Obj *	SplitUnixPath(const char *path);
static int		DoGlob(Tcl_Interp *interp, Tcl_Obj *resultPtr,
			    const char *separators, Tcl_Obj *pathPtr, int flags,
			    char *pattern, Tcl_GlobTypeData *types);

/*
 * When there is no support for getting the block size of a file in a stat()
 * call, use this as a guess. Allow it to be overridden in the platform-
 * specific files.
 */

#if (!defined(HAVE_STRUCT_STAT_ST_BLKSIZE) && !defined(GUESSED_BLOCK_SIZE))
#define GUESSED_BLOCK_SIZE	1024
#endif

/*
 *----------------------------------------------------------------------
 *
 * SetResultLength --
 *
 *	Resets the result DString for ExtractWinRoot to accommodate
 *	any NT extended path prefixes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May modify the Tcl_DString.
 *----------------------------------------------------------------------
 */

static void
SetResultLength(
    Tcl_DString *resultPtr,
    int offset,
    int extended)
{
    Tcl_DStringSetLength(resultPtr, offset);
    if (extended == 2) {
	TclDStringAppendLiteral(resultPtr, "//?/UNC/");
    } else if (extended == 1) {
	TclDStringAppendLiteral(resultPtr, "//?/");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ExtractWinRoot --
 *
 *	Matches the root portion of a Windows path and appends it to the
 *	specified Tcl_DString.
 *
 * Results:
 *	Returns the position in the path immediately after the root including
 *	any trailing slashes. Appends a cleaned up version of the root to the
 *	Tcl_DString at the specified offest.
 *
 * Side effects:
 *	Modifies the specified Tcl_DString.
 *
 *----------------------------------------------------------------------
 */

static const char *
ExtractWinRoot(
    const char *path,		/* Path to parse. */
    Tcl_DString *resultPtr,	/* Buffer to hold result. */
    int offset,			/* Offset in buffer where result should be
				 * stored. */
    Tcl_PathType *typePtr)	/* Where to store pathType result */
{
    int extended = 0;

    if (   (path[0] == '/' || path[0] == '\\')
	&& (path[1] == '/' || path[1] == '\\')
	&& (path[2] == '?')
	&& (path[3] == '/' || path[3] == '\\')) {
	extended = 1;
	path = path + 4;
	if (path[0] == 'U' && path[1] == 'N' && path[2] == 'C'
	    && (path[3] == '/' || path[3] == '\\')) {
	    extended = 2;
	    path = path + 4;
	}
    }

    if (path[0] == '/' || path[0] == '\\') {
	/*
	 * Might be a UNC or Vol-Relative path.
	 */

	const char *host, *share, *tail;
	int hlen, slen;

	if (path[1] != '/' && path[1] != '\\') {
	    SetResultLength(resultPtr, offset, extended);
	    *typePtr = TCL_PATH_VOLUME_RELATIVE;
	    TclDStringAppendLiteral(resultPtr, "/");
	    return &path[1];
	}
	host = &path[2];

	/*
	 * Skip separators.
	 */

	while (host[0] == '/' || host[0] == '\\') {
	    host++;
	}

	for (hlen = 0; host[hlen];hlen++) {
	    if (host[hlen] == '/' || host[hlen] == '\\') {
		break;
	    }
	}
	if (host[hlen] == 0 || host[hlen+1] == 0) {
	    /*
	     * The path given is simply of the form '/foo', '//foo',
	     * '/////foo' or the same with backslashes. If there is exactly
	     * one leading '/' the path is volume relative (see filename man
	     * page). If there are more than one, we are simply assuming they
	     * are superfluous and we trim them away. (An alternative
	     * interpretation would be that it is a host name, but we have
	     * been documented that that is not the case).
	     */

	    *typePtr = TCL_PATH_VOLUME_RELATIVE;
	    TclDStringAppendLiteral(resultPtr, "/");
	    return &path[2];
	}
	SetResultLength(resultPtr, offset, extended);
	share = &host[hlen];

	/*
	 * Skip separators.
	 */

	while (share[0] == '/' || share[0] == '\\') {
	    share++;
	}

	for (slen=0; share[slen]; slen++) {
	    if (share[slen] == '/' || share[slen] == '\\') {
		break;
	    }
	}
	TclDStringAppendLiteral(resultPtr, "//");
	Tcl_DStringAppend(resultPtr, host, hlen);
	TclDStringAppendLiteral(resultPtr, "/");
	Tcl_DStringAppend(resultPtr, share, slen);

	tail = &share[slen];

	/*
	 * Skip separators.
	 */

	while (tail[0] == '/' || tail[0] == '\\') {
	    tail++;
	}

	*typePtr = TCL_PATH_ABSOLUTE;
	return tail;
    } else if (*path && path[1] == ':') {
	/*
	 * Might be a drive separator.
	 */

	SetResultLength(resultPtr, offset, extended);

	if (path[2] != '/' && path[2] != '\\') {
	    *typePtr = TCL_PATH_VOLUME_RELATIVE;
	    Tcl_DStringAppend(resultPtr, path, 2);
	    return &path[2];
	} else {
	    const char *tail = &path[3];

	    /*
	     * Skip separators.
	     */

	    while (*tail && (tail[0] == '/' || tail[0] == '\\')) {
		tail++;
	    }

	    *typePtr = TCL_PATH_ABSOLUTE;
	    Tcl_DStringAppend(resultPtr, path, 2);
	    TclDStringAppendLiteral(resultPtr, "/");

	    return tail;
	}
    } else {
	int abs = 0;

	/*
	 * Check for Windows devices.
	 */

	if ((path[0] == 'c' || path[0] == 'C')
		&& (path[1] == 'o' || path[1] == 'O')) {
	    if ((path[2] == 'm' || path[2] == 'M')
		    && path[3] >= '1' && path[3] <= '9') {
		/*
		 * May have match for 'com[1-9]:?', which is a serial port.
		 */

		if (path[4] == '\0') {
		    abs = 4;
		} else if (path [4] == ':' && path[5] == '\0') {
		    abs = 5;
		}

	    } else if ((path[2] == 'n' || path[2] == 'N') && path[3] == '\0') {
		/*
		 * Have match for 'con'.
		 */

		abs = 3;
	    }

	} else if ((path[0] == 'l' || path[0] == 'L')
		&& (path[1] == 'p' || path[1] == 'P')
		&& (path[2] == 't' || path[2] == 'T')) {
	    if (path[3] >= '1' && path[3] <= '9') {
		/*
		 * May have match for 'lpt[1-9]:?'
		 */

		if (path[4] == '\0') {
		    abs = 4;
		} else if (path [4] == ':' && path[5] == '\0') {
		    abs = 5;
		}
	    }

	} else if ((path[0] == 'p' || path[0] == 'P')
		&& (path[1] == 'r' || path[1] == 'R')
		&& (path[2] == 'n' || path[2] == 'N')
		&& path[3] == '\0') {
	    /*
	     * Have match for 'prn'.
	     */
	    abs = 3;

	} else if ((path[0] == 'n' || path[0] == 'N')
		&& (path[1] == 'u' || path[1] == 'U')
		&& (path[2] == 'l' || path[2] == 'L')
		&& path[3] == '\0') {
	    /*
	     * Have match for 'nul'.
	     */

	    abs = 3;

	} else if ((path[0] == 'a' || path[0] == 'A')
		&& (path[1] == 'u' || path[1] == 'U')
		&& (path[2] == 'x' || path[2] == 'X')
		&& path[3] == '\0') {
	    /*
	     * Have match for 'aux'.
	     */

	    abs = 3;
	}

	if (abs != 0) {
	    *typePtr = TCL_PATH_ABSOLUTE;
	    SetResultLength(resultPtr, offset, extended);
	    Tcl_DStringAppend(resultPtr, path, abs);
	    return path + abs;
	}
    }

    /*
     * Anything else is treated as relative.
     */

    *typePtr = TCL_PATH_RELATIVE;
    return path;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetPathType --
 *
 *	Determines whether a given path is relative to the current directory,
 *	relative to the current volume, or absolute.
 *
 *	The objectified Tcl_FSGetPathType should be used in preference to this
 *	function (as you can see below, this is just a wrapper around that
 *	other function).
 *
 * Results:
 *	Returns one of TCL_PATH_ABSOLUTE, TCL_PATH_RELATIVE, or
 *	TCL_PATH_VOLUME_RELATIVE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_PathType
Tcl_GetPathType(
    const char *path)
{
    Tcl_PathType type;
    Tcl_Obj *tempObj = Tcl_NewStringObj(path,-1);

    Tcl_IncrRefCount(tempObj);
    type = Tcl_FSGetPathType(tempObj);
    Tcl_DecrRefCount(tempObj);
    return type;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetNativePathType --
 *
 *	Determines whether a given path is relative to the current directory,
 *	relative to the current volume, or absolute, but ONLY FOR THE NATIVE
 *	FILESYSTEM. This function is called from tclIOUtil.c (but needs to be
 *	here due to its dependence on static variables/functions in this
 *	file). The exported function Tcl_FSGetPathType should be used by
 *	extensions.
 *
 *	Note that '~' paths are always considered TCL_PATH_ABSOLUTE, even
 *	though expanding the '~' could lead to any possible path type. This
 *	function should therefore be considered a low-level, string
 *	manipulation function only -- it doesn't actually do any expansion in
 *	making its determination.
 *
 * Results:
 *	Returns one of TCL_PATH_ABSOLUTE, TCL_PATH_RELATIVE, or
 *	TCL_PATH_VOLUME_RELATIVE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_PathType
TclpGetNativePathType(
    Tcl_Obj *pathPtr,		/* Native path of interest */
    int *driveNameLengthPtr,	/* Returns length of drive, if non-NULL and
				 * path was absolute */
    Tcl_Obj **driveNameRef)
{
    Tcl_PathType type = TCL_PATH_ABSOLUTE;
    int pathLen;
    const char *path = Tcl_GetStringFromObj(pathPtr, &pathLen);

    if (path[0] == '~') {
	/*
	 * This case is common to all platforms. Paths that begin with ~ are
	 * absolute.
	 */

	if (driveNameLengthPtr != NULL) {
	    const char *end = path + 1;
	    while ((*end != '\0') && (*end != '/')) {
		end++;
	    }
	    *driveNameLengthPtr = end - path;
	}
    } else {
	switch (tclPlatform) {
	case TCL_PLATFORM_UNIX: {
	    const char *origPath = path;

	    /*
	     * Paths that begin with / are absolute.
	     */

	    if (path[0] == '/') {
		++path;
#if defined(__CYGWIN__) || defined(__QNX__)
		/*
		 * Check for "//" network path prefix
		 */
		if ((*path == '/') && path[1] && (path[1] != '/')) {
		    path += 2;
		    while (*path && *path != '/') {
			++path;
		    }
#if defined(__CYGWIN__)
		    /* UNC paths need to be followed by a share name */
		    if (*path++ && (*path && *path != '/')) {
			++path;
			while (*path && *path != '/') {
			    ++path;
			}
		    } else {
			path = origPath + 1;
		    }
#endif
		}
#endif
		if (driveNameLengthPtr != NULL) {
		    /*
		     * We need this addition in case the QNX or Cygwin code was used.
		     */

		    *driveNameLengthPtr = (path - origPath);
		}
	    } else {
		type = TCL_PATH_RELATIVE;
	    }
	    break;
	}
	case TCL_PLATFORM_WINDOWS: {
	    Tcl_DString ds;
	    const char *rootEnd;

	    Tcl_DStringInit(&ds);
	    rootEnd = ExtractWinRoot(path, &ds, 0, &type);
	    if ((rootEnd != path) && (driveNameLengthPtr != NULL)) {
		*driveNameLengthPtr = rootEnd - path;
		if (driveNameRef != NULL) {
		    *driveNameRef = TclDStringToObj(&ds);
		    Tcl_IncrRefCount(*driveNameRef);
		}
	    }
	    Tcl_DStringFree(&ds);
	    break;
	}
	}
    }
    return type;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpNativeSplitPath --
 *
 *	This function takes the given Tcl_Obj, which should be a valid path,
 *	and returns a Tcl List object containing each segment of that path as
 *	an element.
 *
 *	Note this function currently calls the older Split(Plat)Path
 *	functions, which require more memory allocation than is desirable.
 *
 * Results:
 *	Returns list object with refCount of zero. If the passed in lenPtr is
 *	non-NULL, we use it to return the number of elements in the returned
 *	list.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Obj *
TclpNativeSplitPath(
    Tcl_Obj *pathPtr,		/* Path to split. */
    int *lenPtr)		/* int to store number of path elements. */
{
    Tcl_Obj *resultPtr = NULL;	/* Needed only to prevent gcc warnings. */

    /*
     * Perform platform specific splitting.
     */

    switch (tclPlatform) {
    case TCL_PLATFORM_UNIX:
	resultPtr = SplitUnixPath(Tcl_GetString(pathPtr));
	break;

    case TCL_PLATFORM_WINDOWS:
	resultPtr = SplitWinPath(Tcl_GetString(pathPtr));
	break;
    }

    /*
     * Compute the number of elements in the result.
     */

    if (lenPtr != NULL) {
	Tcl_ListObjLength(NULL, resultPtr, lenPtr);
    }
    return resultPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SplitPath --
 *
 *	Split a path into a list of path components. The first element of the
 *	list will have the same path type as the original path.
 *
 * Results:
 *	Returns a standard Tcl result. The interpreter result contains a list
 *	of path components. *argvPtr will be filled in with the address of an
 *	array whose elements point to the elements of path, in order.
 *	*argcPtr will get filled in with the number of valid elements in the
 *	array. A single block of memory is dynamically allocated to hold both
 *	the argv array and a copy of the path elements. The caller must
 *	eventually free this memory by calling ckfree() on *argvPtr. Note:
 *	*argvPtr and *argcPtr are only modified if the procedure returns
 *	normally.
 *
 * Side effects:
 *	Allocates memory.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SplitPath(
    const char *path,		/* Pointer to string containing a path. */
    int *argcPtr,		/* Pointer to location to fill in with the
				 * number of elements in the path. */
    const char ***argvPtr)	/* Pointer to place to store pointer to array
				 * of pointers to path elements. */
{
    Tcl_Obj *resultPtr = NULL;	/* Needed only to prevent gcc warnings. */
    Tcl_Obj *tmpPtr, *eltPtr;
    int i, size, len;
    char *p;
    const char *str;

    /*
     * Perform the splitting, using objectified, vfs-aware code.
     */

    tmpPtr = Tcl_NewStringObj(path, -1);
    Tcl_IncrRefCount(tmpPtr);
    resultPtr = Tcl_FSSplitPath(tmpPtr, argcPtr);
    Tcl_IncrRefCount(resultPtr);
    Tcl_DecrRefCount(tmpPtr);

    /*
     * Calculate space required for the result.
     */

    size = 1;
    for (i = 0; i < *argcPtr; i++) {
	Tcl_ListObjIndex(NULL, resultPtr, i, &eltPtr);
	Tcl_GetStringFromObj(eltPtr, &len);
	size += len + 1;
    }

    /*
     * Allocate a buffer large enough to hold the contents of all of the list
     * plus the argv pointers and the terminating NULL pointer.
     */

    *argvPtr = ckalloc((((*argcPtr) + 1) * sizeof(char *)) + size);

    /*
     * Position p after the last argv pointer and copy the contents of the
     * list in, piece by piece.
     */

    p = (char *) &(*argvPtr)[(*argcPtr) + 1];
    for (i = 0; i < *argcPtr; i++) {
	Tcl_ListObjIndex(NULL, resultPtr, i, &eltPtr);
	str = Tcl_GetStringFromObj(eltPtr, &len);
	memcpy(p, str, (size_t) len+1);
	p += len+1;
    }

    /*
     * Now set up the argv pointers.
     */

    p = (char *) &(*argvPtr)[(*argcPtr) + 1];

    for (i = 0; i < *argcPtr; i++) {
	(*argvPtr)[i] = p;
	for (; *(p++)!='\0'; );
    }
    (*argvPtr)[i] = NULL;

    /*
     * Free the result ptr given to us by Tcl_FSSplitPath
     */

    Tcl_DecrRefCount(resultPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * SplitUnixPath --
 *
 *	This routine is used by Tcl_(FS)SplitPath to handle splitting Unix
 *	paths.
 *
 * Results:
 *	Returns a newly allocated Tcl list object.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
SplitUnixPath(
    const char *path)		/* Pointer to string containing a path. */
{
    int length;
    const char *origPath = path, *elementStart;
    Tcl_Obj *result = Tcl_NewObj();

    /*
     * Deal with the root directory as a special case.
     */

    if (*path == '/') {
	Tcl_Obj *rootElt;
	++path;
#if defined(__CYGWIN__) || defined(__QNX__)
	/*
	 * Check for "//" network path prefix
	 */
	if ((*path == '/') && path[1] && (path[1] != '/')) {
	    path += 2;
	    while (*path && *path != '/') {
		++path;
	    }
#if defined(__CYGWIN__)
	    /* UNC paths need to be followed by a share name */
	    if (*path++ && (*path && *path != '/')) {
		++path;
		while (*path && *path != '/') {
		    ++path;
		}
	    } else {
		path = origPath + 1;
	    }
#endif
	}
#endif
	rootElt = Tcl_NewStringObj(origPath, path - origPath);
	Tcl_ListObjAppendElement(NULL, result, rootElt);
	while (*path == '/') {
	    ++path;
	}
    }

    /*
     * Split on slashes. Embedded elements that start with tilde will be
     * prefixed with "./" so they are not affected by tilde substitution.
     */

    for (;;) {
	elementStart = path;
	while ((*path != '\0') && (*path != '/')) {
	    path++;
	}
	length = path - elementStart;
	if (length > 0) {
	    Tcl_Obj *nextElt;
	    if ((elementStart[0] == '~') && (elementStart != origPath)) {
		TclNewLiteralStringObj(nextElt, "./");
		Tcl_AppendToObj(nextElt, elementStart, length);
	    } else {
		nextElt = Tcl_NewStringObj(elementStart, length);
	    }
	    Tcl_ListObjAppendElement(NULL, result, nextElt);
	}
	if (*path++ == '\0') {
	    break;
	}
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * SplitWinPath --
 *
 *	This routine is used by Tcl_(FS)SplitPath to handle splitting Windows
 *	paths.
 *
 * Results:
 *	Returns a newly allocated Tcl list object.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
SplitWinPath(
    const char *path)		/* Pointer to string containing a path. */
{
    int length;
    const char *p, *elementStart;
    Tcl_PathType type = TCL_PATH_ABSOLUTE;
    Tcl_DString buf;
    Tcl_Obj *result = Tcl_NewObj();
    Tcl_DStringInit(&buf);

    p = ExtractWinRoot(path, &buf, 0, &type);

    /*
     * Terminate the root portion, if we matched something.
     */

    if (p != path) {
	Tcl_ListObjAppendElement(NULL, result, TclDStringToObj(&buf));
    }
    Tcl_DStringFree(&buf);

    /*
     * Split on slashes. Embedded elements that start with tilde or a drive
     * letter will be prefixed with "./" so they are not affected by tilde
     * substitution.
     */

    do {
	elementStart = p;
	while ((*p != '\0') && (*p != '/') && (*p != '\\')) {
	    p++;
	}
	length = p - elementStart;
	if (length > 0) {
	    Tcl_Obj *nextElt;
	    if ((elementStart != path) && ((elementStart[0] == '~')
		    || (isalpha(UCHAR(elementStart[0]))
			&& elementStart[1] == ':'))) {
		TclNewLiteralStringObj(nextElt, "./");
		Tcl_AppendToObj(nextElt, elementStart, length);
	    } else {
		nextElt = Tcl_NewStringObj(elementStart, length);
	    }
	    Tcl_ListObjAppendElement(NULL, result, nextElt);
	}
    } while (*p++ != '\0');

    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_FSJoinToPath --
 *
 *	This function takes the given object, which should usually be a valid
 *	path or NULL, and joins onto it the array of paths segments given.
 *
 *	The objects in the array given will temporarily have their refCount
 *	increased by one, and then decreased by one when this function exits
 *	(which means if they had zero refCount when we were called, they will
 *	be freed).
 *
 * Results:
 *	Returns object owned by the caller (which should increment its
 *	refCount) - typically an object with refCount of zero.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_FSJoinToPath(
    Tcl_Obj *pathPtr,		/* Valid path or NULL. */
    int objc,			/* Number of array elements to join */
    Tcl_Obj *const objv[])	/* Path elements to join. */
{
    if (pathPtr == NULL) {
	return TclJoinPath(objc, objv);
    }
    if (objc == 0) {
	return TclJoinPath(1, &pathPtr);
    }
    if (objc == 1) {
	Tcl_Obj *pair[2];

	pair[0] = pathPtr;
	pair[1] = objv[0];
	return TclJoinPath(2, pair);
    } else {
	int elemc = objc + 1;
	Tcl_Obj *ret, **elemv = ckalloc(elemc*sizeof(Tcl_Obj *));

	elemv[0] = pathPtr;
	memcpy(elemv+1, objv, objc*sizeof(Tcl_Obj *));
	ret = TclJoinPath(elemc, elemv);
	ckfree(elemv);
	return ret;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpNativeJoinPath --
 *
 *	'prefix' is absolute, 'joining' is relative to prefix.
 *
 * Results:
 *	modifies prefix
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
TclpNativeJoinPath(
    Tcl_Obj *prefix,
    const char *joining)
{
    int length, needsSep;
    char *dest;
    const char *p;
    const char *start;

    start = Tcl_GetStringFromObj(prefix, &length);

    /*
     * Remove the ./ from tilde prefixed elements, and drive-letter prefixed
     * elements on Windows, unless it is the first component.
     */

    p = joining;

    if (length != 0) {
	if ((p[0] == '.') && (p[1] == '/') && ((p[2] == '~')
		|| (tclPlatform==TCL_PLATFORM_WINDOWS && isalpha(UCHAR(p[2]))
		&& (p[3] == ':')))) {
	    p += 2;
	}
    }
    if (*p == '\0') {
	return;
    }

    switch (tclPlatform) {
    case TCL_PLATFORM_UNIX:
	/*
	 * Append a separator if needed.
	 */

	if (length > 0 && (start[length-1] != '/')) {
	    Tcl_AppendToObj(prefix, "/", 1);
	    Tcl_GetStringFromObj(prefix, &length);
	}
	needsSep = 0;

	/*
	 * Append the element, eliminating duplicate and trailing slashes.
	 */

	Tcl_SetObjLength(prefix, length + (int) strlen(p));

	dest = Tcl_GetString(prefix) + length;
	for (; *p != '\0'; p++) {
	    if (*p == '/') {
		while (p[1] == '/') {
		    p++;
		}
		if (p[1] != '\0' && needsSep) {
		    *dest++ = '/';
		}
	    } else {
		*dest++ = *p;
		needsSep = 1;
	    }
	}
	length = dest - Tcl_GetString(prefix);
	Tcl_SetObjLength(prefix, length);
	break;

    case TCL_PLATFORM_WINDOWS:
	/*
	 * Check to see if we need to append a separator.
	 */

	if ((length > 0) &&
		(start[length-1] != '/') && (start[length-1] != ':')) {
	    Tcl_AppendToObj(prefix, "/", 1);
	    Tcl_GetStringFromObj(prefix, &length);
	}
	needsSep = 0;

	/*
	 * Append the element, eliminating duplicate and trailing slashes.
	 */

	Tcl_SetObjLength(prefix, length + (int) strlen(p));
	dest = Tcl_GetString(prefix) + length;
	for (; *p != '\0'; p++) {
	    if ((*p == '/') || (*p == '\\')) {
		while ((p[1] == '/') || (p[1] == '\\')) {
		    p++;
		}
		if ((p[1] != '\0') && needsSep) {
		    *dest++ = '/';
		}
	    } else {
		*dest++ = *p;
		needsSep = 1;
	    }
	}
	length = dest - Tcl_GetString(prefix);
	Tcl_SetObjLength(prefix, length);
	break;
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_JoinPath --
 *
 *	Combine a list of paths in a platform specific manner. The function
 *	'Tcl_FSJoinPath' should be used in preference where possible.
 *
 * Results:
 *	Appends the joined path to the end of the specified Tcl_DString
 *	returning a pointer to the resulting string. Note that the
 *	Tcl_DString must already be initialized.
 *
 * Side effects:
 *	Modifies the Tcl_DString.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_JoinPath(
    int argc,
    const char *const *argv,
    Tcl_DString *resultPtr)	/* Pointer to previously initialized DString */
{
    int i, len;
    Tcl_Obj *listObj = Tcl_NewObj();
    Tcl_Obj *resultObj;
    const char *resultStr;

    /*
     * Build the list of paths.
     */

    for (i = 0; i < argc; i++) {
	Tcl_ListObjAppendElement(NULL, listObj,
		Tcl_NewStringObj(argv[i], -1));
    }

    /*
     * Ask the objectified code to join the paths.
     */

    Tcl_IncrRefCount(listObj);
    resultObj = Tcl_FSJoinPath(listObj, argc);
    Tcl_IncrRefCount(resultObj);
    Tcl_DecrRefCount(listObj);

    /*
     * Store the result.
     */

    resultStr = Tcl_GetStringFromObj(resultObj, &len);
    Tcl_DStringAppend(resultPtr, resultStr, len);
    Tcl_DecrRefCount(resultObj);

    /*
     * Return a pointer to the result.
     */

    return Tcl_DStringValue(resultPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_TranslateFileName --
 *
 *	Converts a file name into a form usable by the native system
 *	interfaces. If the name starts with a tilde, it will produce a name
 *	where the tilde and following characters have been replaced by the
 *	home directory location for the named user.
 *
 * Results:
 *	The return value is a pointer to a string containing the name after
 *	tilde substitution. If there was no tilde substitution, the return
 *	value is a pointer to a copy of the original string. If there was an
 *	error in processing the name, then an error message is left in the
 *	interp's result (if interp was not NULL) and the return value is NULL.
 *	Space for the return value is allocated in bufferPtr; the caller must
 *	call Tcl_DStringFree() to free the space if the return value was not
 *	NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_TranslateFileName(
    Tcl_Interp *interp,		/* Interpreter in which to store error message
				 * (if necessary). */
    const char *name,		/* File name, which may begin with "~" (to
				 * indicate current user's home directory) or
				 * "~<user>" (to indicate any user's home
				 * directory). */
    Tcl_DString *bufferPtr)	/* Uninitialized or free DString filled with
				 * name after tilde substitution. */
{
    Tcl_Obj *path = Tcl_NewStringObj(name, -1);
    Tcl_Obj *transPtr;

    Tcl_IncrRefCount(path);
    transPtr = Tcl_FSGetTranslatedPath(interp, path);
    if (transPtr == NULL) {
	Tcl_DecrRefCount(path);
	return NULL;
    }

    Tcl_DStringInit(bufferPtr);
    TclDStringAppendObj(bufferPtr, transPtr);
    Tcl_DecrRefCount(path);
    Tcl_DecrRefCount(transPtr);

    /*
     * Convert forward slashes to backslashes in Windows paths because some
     * system interfaces don't accept forward slashes.
     */

    if (tclPlatform == TCL_PLATFORM_WINDOWS) {
	register char *p;
	for (p = Tcl_DStringValue(bufferPtr); *p != '\0'; p++) {
	    if (*p == '/') {
		*p = '\\';
	    }
	}
    }

    return Tcl_DStringValue(bufferPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetExtension --
 *
 *	This function returns a pointer to the beginning of the extension part
 *	of a file name.
 *
 * Results:
 *	Returns a pointer into name which indicates where the extension
 *	starts. If there is no extension, returns NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
TclGetExtension(
    const char *name)		/* File name to parse. */
{
    const char *p, *lastSep;

    /*
     * First find the last directory separator.
     */

    lastSep = NULL;		/* Needed only to prevent gcc warnings. */
    switch (tclPlatform) {
    case TCL_PLATFORM_UNIX:
	lastSep = strrchr(name, '/');
	break;

    case TCL_PLATFORM_WINDOWS:
	lastSep = NULL;
	for (p = name; *p != '\0'; p++) {
	    if (strchr("/\\:", *p) != NULL) {
		lastSep = p;
	    }
	}
	break;
    }
    p = strrchr(name, '.');
    if ((p != NULL) && (lastSep != NULL) && (lastSep > p)) {
	p = NULL;
    }

    /*
     * In earlier versions, we used to back up to the first period in a series
     * so that "foo..o" would be split into "foo" and "..o". This is a
     * confusing and usually incorrect behavior, so now we split at the last
     * period in the name.
     */

    return p;
}

/*
 *----------------------------------------------------------------------
 *
 * DoTildeSubst --
 *
 *	Given a string following a tilde, this routine returns the
 *	corresponding home directory.
 *
 * Results:
 *	The result is a pointer to a static string containing the home
 *	directory in native format. If there was an error in processing the
 *	substitution, then an error message is left in the interp's result and
 *	the return value is NULL. On success, the results are appended to
 *	resultPtr, and the contents of resultPtr are returned.
 *
 * Side effects:
 *	Information may be left in resultPtr.
 *
 *----------------------------------------------------------------------
 */

static const char *
DoTildeSubst(
    Tcl_Interp *interp,		/* Interpreter in which to store error message
				 * (if necessary). */
    const char *user,		/* Name of user whose home directory should be
				 * substituted, or "" for current user. */
    Tcl_DString *resultPtr)	/* Initialized DString filled with name after
				 * tilde substitution. */
{
    const char *dir;

    if (*user == '\0') {
	Tcl_DString dirString;

	dir = TclGetEnv("HOME", &dirString);
	if (dir == NULL) {
	    if (interp) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"couldn't find HOME environment "
			"variable to expand path", -1));
		Tcl_SetErrorCode(interp, "TCL", "FILENAME", "NO_HOME", NULL);
	    }
	    return NULL;
	}
	Tcl_JoinPath(1, &dir, resultPtr);
	Tcl_DStringFree(&dirString);
    } else if (TclpGetUserHome(user, resultPtr) == NULL) {
	if (interp) {
	    Tcl_ResetResult(interp);
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "user \"%s\" doesn't exist", user));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "USER", user, NULL);
	}
	return NULL;
    }
    return Tcl_DStringValue(resultPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GlobObjCmd --
 *
 *	This procedure is invoked to process the "glob" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_GlobObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int index, i, globFlags, length, join, dir, result;
    char *string;
    const char *separators;
    Tcl_Obj *typePtr, *look;
    Tcl_Obj *pathOrDir = NULL;
    Tcl_DString prefix;
    static const char *const options[] = {
	"-directory", "-join", "-nocomplain", "-path", "-tails",
	"-types", "--", NULL
    };
    enum options {
	GLOB_DIR, GLOB_JOIN, GLOB_NOCOMPLAIN, GLOB_PATH, GLOB_TAILS,
	GLOB_TYPE, GLOB_LAST
    };
    enum pathDirOptions {PATH_NONE = -1 , PATH_GENERAL = 0, PATH_DIR = 1};
    Tcl_GlobTypeData *globTypes = NULL;

    globFlags = 0;
    join = 0;
    dir = PATH_NONE;
    typePtr = NULL;
    for (i = 1; i < objc; i++) {
	if (Tcl_GetIndexFromObj(interp, objv[i], options, "option", 0,
		&index) != TCL_OK) {
	    string = Tcl_GetStringFromObj(objv[i], &length);
	    if (string[0] == '-') {
		/*
		 * It looks like the command contains an option so signal an
		 * error.
		 */

		return TCL_ERROR;
	    } else {
		/*
		 * This clearly isn't an option; assume it's the first glob
		 * pattern. We must clear the error.
		 */

		Tcl_ResetResult(interp);
		break;
	    }
	}

	switch (index) {
	case GLOB_NOCOMPLAIN:			/* -nocomplain */
	    globFlags |= TCL_GLOBMODE_NO_COMPLAIN;
	    break;
	case GLOB_DIR:				/* -dir */
	    if (i == (objc-1)) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"missing argument to \"-directory\"", -1));
		Tcl_SetErrorCode(interp, "TCL", "ARGUMENT", "MISSING", NULL);
		return TCL_ERROR;
	    }
	    if (dir != PATH_NONE) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"\"-directory\" cannot be used with \"-path\"", -1));
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "GLOB",
			"BADOPTIONCOMBINATION", NULL);
		return TCL_ERROR;
	    }
	    dir = PATH_DIR;
	    globFlags |= TCL_GLOBMODE_DIR;
	    pathOrDir = objv[i+1];
	    i++;
	    break;
	case GLOB_JOIN:				/* -join */
	    join = 1;
	    break;
	case GLOB_TAILS:				/* -tails */
	    globFlags |= TCL_GLOBMODE_TAILS;
	    break;
	case GLOB_PATH:				/* -path */
	    if (i == (objc-1)) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"missing argument to \"-path\"", -1));
		Tcl_SetErrorCode(interp, "TCL", "ARGUMENT", "MISSING", NULL);
		return TCL_ERROR;
	    }
	    if (dir != PATH_NONE) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"\"-path\" cannot be used with \"-directory\"", -1));
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "GLOB",
			"BADOPTIONCOMBINATION", NULL);
		return TCL_ERROR;
	    }
	    dir = PATH_GENERAL;
	    pathOrDir = objv[i+1];
	    i++;
	    break;
	case GLOB_TYPE:				/* -types */
	    if (i == (objc-1)) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"missing argument to \"-types\"", -1));
		Tcl_SetErrorCode(interp, "TCL", "ARGUMENT", "MISSING", NULL);
		return TCL_ERROR;
	    }
	    typePtr = objv[i+1];
	    if (Tcl_ListObjLength(interp, typePtr, &length) != TCL_OK) {
		return TCL_ERROR;
	    }
	    i++;
	    break;
	case GLOB_LAST:				/* -- */
	    i++;
	    goto endOfForLoop;
	}
    }

  endOfForLoop:
    if ((globFlags & TCL_GLOBMODE_TAILS) && (pathOrDir == NULL)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"\"-tails\" must be used with either "
		"\"-directory\" or \"-path\"", -1));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "GLOB",
		"BADOPTIONCOMBINATION", NULL);
	return TCL_ERROR;
    }

    separators = NULL;		/* lint. */
    switch (tclPlatform) {
    case TCL_PLATFORM_UNIX:
	separators = "/";
	break;
    case TCL_PLATFORM_WINDOWS:
	separators = "/\\:";
	break;
    }

    if (dir == PATH_GENERAL) {
	int pathlength;
	const char *last;
	const char *first = Tcl_GetStringFromObj(pathOrDir,&pathlength);

	/*
	 * Find the last path separator in the path
	 */

	last = first + pathlength;
	for (; last != first; last--) {
	    if (strchr(separators, *(last-1)) != NULL) {
		break;
	    }
	}

	if (last == first + pathlength) {
	    /*
	     * It's really a directory.
	     */

	    dir = PATH_DIR;

	} else {
	    Tcl_DString pref;
	    char *search, *find;
	    Tcl_DStringInit(&pref);
	    if (last == first) {
		/*
		 * The whole thing is a prefix. This means we must remove any
		 * 'tails' flag too, since it is irrelevant now (the same
		 * effect will happen without it), but in particular its use
		 * in TclGlob requires a non-NULL pathOrDir.
		 */

		Tcl_DStringAppend(&pref, first, -1);
		globFlags &= ~TCL_GLOBMODE_TAILS;
		pathOrDir = NULL;
	    } else {
		/*
		 * Have to split off the end.
		 */

		Tcl_DStringAppend(&pref, last, first+pathlength-last);
		pathOrDir = Tcl_NewStringObj(first, last-first-1);

		/*
		 * We must ensure that we haven't cut off too much, and turned
		 * a valid path like '/' or 'C:/' into an incorrect path like
		 * '' or 'C:'. The way we do this is to add a separator if
		 * there are none presently in the prefix.
		 */

		if (strpbrk(Tcl_GetString(pathOrDir), "\\/") == NULL) {
		    Tcl_AppendToObj(pathOrDir, last-1, 1);
		}
	    }

	    /*
	     * Need to quote 'prefix'.
	     */

	    Tcl_DStringInit(&prefix);
	    search = Tcl_DStringValue(&pref);
	    while ((find = (strpbrk(search, "\\[]*?{}"))) != NULL) {
		Tcl_DStringAppend(&prefix, search, find-search);
		TclDStringAppendLiteral(&prefix, "\\");
		Tcl_DStringAppend(&prefix, find, 1);
		search = find+1;
		if (*search == '\0') {
		    break;
		}
	    }
	    if (*search != '\0') {
		Tcl_DStringAppend(&prefix, search, -1);
	    }
	    Tcl_DStringFree(&pref);
	}
    }

    if (pathOrDir != NULL) {
	Tcl_IncrRefCount(pathOrDir);
    }

    if (typePtr != NULL) {
	/*
	 * The rest of the possible type arguments (except 'd') are platform
	 * specific. We don't complain when they are used on an incompatible
	 * platform.
	 */

	Tcl_ListObjLength(interp, typePtr, &length);
	if (length <= 0) {
	    goto skipTypes;
	}
	globTypes = TclStackAlloc(interp, sizeof(Tcl_GlobTypeData));
	globTypes->type = 0;
	globTypes->perm = 0;
	globTypes->macType = NULL;
	globTypes->macCreator = NULL;

	while (--length >= 0) {
	    int len;
	    const char *str;

	    Tcl_ListObjIndex(interp, typePtr, length, &look);
	    str = Tcl_GetStringFromObj(look, &len);
	    if (strcmp("readonly", str) == 0) {
		globTypes->perm |= TCL_GLOB_PERM_RONLY;
	    } else if (strcmp("hidden", str) == 0) {
		globTypes->perm |= TCL_GLOB_PERM_HIDDEN;
	    } else if (len == 1) {
		switch (str[0]) {
		case 'r':
		    globTypes->perm |= TCL_GLOB_PERM_R;
		    break;
		case 'w':
		    globTypes->perm |= TCL_GLOB_PERM_W;
		    break;
		case 'x':
		    globTypes->perm |= TCL_GLOB_PERM_X;
		    break;
		case 'b':
		    globTypes->type |= TCL_GLOB_TYPE_BLOCK;
		    break;
		case 'c':
		    globTypes->type |= TCL_GLOB_TYPE_CHAR;
		    break;
		case 'd':
		    globTypes->type |= TCL_GLOB_TYPE_DIR;
		    break;
		case 'p':
		    globTypes->type |= TCL_GLOB_TYPE_PIPE;
		    break;
		case 'f':
		    globTypes->type |= TCL_GLOB_TYPE_FILE;
		    break;
		case 'l':
		    globTypes->type |= TCL_GLOB_TYPE_LINK;
		    break;
		case 's':
		    globTypes->type |= TCL_GLOB_TYPE_SOCK;
		    break;
		default:
		    goto badTypesArg;
		}

	    } else if (len == 4) {
		/*
		 * This is assumed to be a MacOS file type.
		 */

		if (globTypes->macType != NULL) {
		    goto badMacTypesArg;
		}
		globTypes->macType = look;
		Tcl_IncrRefCount(look);

	    } else {
		Tcl_Obj *item;

		if ((Tcl_ListObjLength(NULL, look, &len) == TCL_OK)
			&& (len == 3)) {
		    Tcl_ListObjIndex(interp, look, 0, &item);
		    if (!strcmp("macintosh", Tcl_GetString(item))) {
			Tcl_ListObjIndex(interp, look, 1, &item);
			if (!strcmp("type", Tcl_GetString(item))) {
			    Tcl_ListObjIndex(interp, look, 2, &item);
			    if (globTypes->macType != NULL) {
				goto badMacTypesArg;
			    }
			    globTypes->macType = item;
			    Tcl_IncrRefCount(item);
			    continue;
			} else if (!strcmp("creator", Tcl_GetString(item))) {
			    Tcl_ListObjIndex(interp, look, 2, &item);
			    if (globTypes->macCreator != NULL) {
				goto badMacTypesArg;
			    }
			    globTypes->macCreator = item;
			    Tcl_IncrRefCount(item);
			    continue;
			}
		    }
		}

		/*
		 * Error cases. We reset the 'join' flag to zero, since we
		 * haven't yet made use of it.
		 */

	    badTypesArg:
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"bad argument to \"-types\": %s",
			Tcl_GetString(look)));
		Tcl_SetErrorCode(interp, "TCL", "ARGUMENT", "BAD", NULL);
		result = TCL_ERROR;
		join = 0;
		goto endOfGlob;

	    badMacTypesArg:
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"only one MacOS type or creator argument"
			" to \"-types\" allowed", -1));
		result = TCL_ERROR;
		Tcl_SetErrorCode(interp, "TCL", "ARGUMENT", "BAD", NULL);
		join = 0;
		goto endOfGlob;
	    }
	}
    }

  skipTypes:
    /*
     * Now we perform the actual glob below. This may involve joining together
     * the pattern arguments, dealing with particular file types etc. We use a
     * 'goto' to ensure we free any memory allocated along the way.
     */

    objc -= i;
    objv += i;
    result = TCL_OK;

    if (join) {
	if (dir != PATH_GENERAL) {
	    Tcl_DStringInit(&prefix);
	}
	for (i = 0; i < objc; i++) {
	    TclDStringAppendObj(&prefix, objv[i]);
	    if (i != objc -1) {
		Tcl_DStringAppend(&prefix, separators, 1);
	    }
	}
	if (TclGlob(interp, Tcl_DStringValue(&prefix), pathOrDir, globFlags,
		globTypes) != TCL_OK) {
	    result = TCL_ERROR;
	    goto endOfGlob;
	}
    } else if (dir == PATH_GENERAL) {
	Tcl_DString str;

	Tcl_DStringInit(&str);
	for (i = 0; i < objc; i++) {
	    Tcl_DStringSetLength(&str, 0);
	    if (dir == PATH_GENERAL) {
		TclDStringAppendDString(&str, &prefix);
	    }
	    TclDStringAppendObj(&str, objv[i]);
	    if (TclGlob(interp, Tcl_DStringValue(&str), pathOrDir, globFlags,
		    globTypes) != TCL_OK) {
		result = TCL_ERROR;
		Tcl_DStringFree(&str);
		goto endOfGlob;
	    }
	}
	Tcl_DStringFree(&str);
    } else {
	for (i = 0; i < objc; i++) {
	    string = Tcl_GetString(objv[i]);
	    if (TclGlob(interp, string, pathOrDir, globFlags,
		    globTypes) != TCL_OK) {
		result = TCL_ERROR;
		goto endOfGlob;
	    }
	}
    }

    if ((globFlags & TCL_GLOBMODE_NO_COMPLAIN) == 0) {
	if (Tcl_ListObjLength(interp, Tcl_GetObjResult(interp),
		&length) != TCL_OK) {
	    /*
	     * This should never happen. Maybe we should be more dramatic.
	     */

	    result = TCL_ERROR;
	    goto endOfGlob;
	}

	if (length == 0) {
	    Tcl_Obj *errorMsg =
		    Tcl_ObjPrintf("no files matched glob pattern%s \"",
			    (join || (objc == 1)) ? "" : "s");

	    if (join) {
		Tcl_AppendToObj(errorMsg, Tcl_DStringValue(&prefix), -1);
	    } else {
		const char *sep = "";

		for (i = 0; i < objc; i++) {
		    Tcl_AppendPrintfToObj(errorMsg, "%s%s",
			    sep, Tcl_GetString(objv[i]));
		    sep = " ";
		}
	    }
	    Tcl_AppendToObj(errorMsg, "\"", -1);
	    Tcl_SetObjResult(interp, errorMsg);
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "GLOB", "NOMATCH",
		    NULL);
	    result = TCL_ERROR;
	}
    }

  endOfGlob:
    if (join || (dir == PATH_GENERAL)) {
	Tcl_DStringFree(&prefix);
    }
    if (pathOrDir != NULL) {
	Tcl_DecrRefCount(pathOrDir);
    }
    if (globTypes != NULL) {
	if (globTypes->macType != NULL) {
	    Tcl_DecrRefCount(globTypes->macType);
	}
	if (globTypes->macCreator != NULL) {
	    Tcl_DecrRefCount(globTypes->macCreator);
	}
	TclStackFree(interp, globTypes);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGlob --
 *
 *	This procedure prepares arguments for the DoGlob call. It sets the
 *	separator string based on the platform, performs * tilde substitution,
 *	and calls DoGlob.
 *
 *	The interpreter's result, on entry to this function, must be a valid
 *	Tcl list (e.g. it could be empty), since we will lappend any new
 *	results to that list. If it is not a valid list, this function will
 *	fail to do anything very meaningful.
 *
 *	Note that if globFlags contains 'TCL_GLOBMODE_TAILS' then pathPrefix
 *	cannot be NULL (it is only allowed with -dir or -path).
 *
 * Results:
 *	The return value is a standard Tcl result indicating whether an error
 *	occurred in globbing. After a normal return the result in interp (set
 *	by DoGlob) holds all of the file names given by the pattern and
 *	pathPrefix arguments. After an error the result in interp will hold
 *	an error message.
 *
 * Side effects:
 *	The 'pattern' is written to.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
TclGlob(
    Tcl_Interp *interp,		/* Interpreter for returning error message or
				 * appending list of matching file names. */
    char *pattern,		/* Glob pattern to match. Must not refer to a
				 * static string. */
    Tcl_Obj *pathPrefix,	/* Path prefix to glob pattern, if non-null,
				 * which is considered literally. */
    int globFlags,		/* Stores or'ed combination of flags */
    Tcl_GlobTypeData *types)	/* Struct containing acceptable types. May be
				 * NULL. */
{
    const char *separators;
    const char *head;
    char *tail, *start;
    int result;
    Tcl_Obj *filenamesObj, *savedResultObj;

    separators = NULL;		/* lint. */
    switch (tclPlatform) {
    case TCL_PLATFORM_UNIX:
	separators = "/";
	break;
    case TCL_PLATFORM_WINDOWS:
	separators = "/\\:";
	break;
    }

    if (pathPrefix == NULL) {
	char c;
	Tcl_DString buffer;
	Tcl_DStringInit(&buffer);

	start = pattern;

	/*
	 * Perform tilde substitution, if needed.
	 */

	if (start[0] == '~') {
	    /*
	     * Find the first path separator after the tilde.
	     */

	    for (tail = start; *tail != '\0'; tail++) {
		if (*tail == '\\') {
		    if (strchr(separators, tail[1]) != NULL) {
			break;
		    }
		} else if (strchr(separators, *tail) != NULL) {
		    break;
		}
	    }

	    /*
	     * Determine the home directory for the specified user.
	     */

	    c = *tail;
	    *tail = '\0';
	    head = DoTildeSubst(interp, start+1, &buffer);
	    *tail = c;
	    if (head == NULL) {
		return TCL_ERROR;
	    }
	    if (head != Tcl_DStringValue(&buffer)) {
		Tcl_DStringAppend(&buffer, head, -1);
	    }
	    pathPrefix = TclDStringToObj(&buffer);
	    Tcl_IncrRefCount(pathPrefix);
	    globFlags |= TCL_GLOBMODE_DIR;
	    if (c != '\0') {
		tail++;
	    }
	    Tcl_DStringFree(&buffer);
	} else {
	    tail = pattern;
	}
    } else {
	Tcl_IncrRefCount(pathPrefix);
	tail = pattern;
    }

    /*
     * Handling empty path prefixes with glob patterns like 'C:' or
     * 'c:////////' is a pain on Windows if we leave it too late, since these
     * aren't really patterns at all! We therefore check the head of the
     * pattern now for such cases, if we don't have an unquoted prefix yet.
     *
     * Similarly on Unix with '/' at the head of the pattern -- it just
     * indicates the root volume, so we treat it as such.
     */

    if (tclPlatform == TCL_PLATFORM_WINDOWS) {
	if (pathPrefix == NULL && tail[0] != '\0' && tail[1] == ':') {
	    char *p = tail + 1;
	    pathPrefix = Tcl_NewStringObj(tail, 1);
	    while (*p != '\0') {
		char c = p[1];
		if (*p == '\\') {
		    if (strchr(separators, c) != NULL) {
			if (c == '\\') {
			    c = '/';
			}
			Tcl_AppendToObj(pathPrefix, &c, 1);
			p++;
		    } else {
			break;
		    }
		} else if (strchr(separators, *p) != NULL) {
		    Tcl_AppendToObj(pathPrefix, p, 1);
		} else {
		    break;
		}
		p++;
	    }
	    tail = p;
	    Tcl_IncrRefCount(pathPrefix);
	} else if (pathPrefix == NULL && (tail[0] == '/'
		|| (tail[0] == '\\' && tail[1] == '\\'))) {
	    int driveNameLen;
	    Tcl_Obj *driveName;
	    Tcl_Obj *temp = Tcl_NewStringObj(tail, -1);
	    Tcl_IncrRefCount(temp);

	    switch (TclGetPathType(temp, NULL, &driveNameLen, &driveName)) {
	    case TCL_PATH_VOLUME_RELATIVE: {
		/*
		 * Volume relative path which is equivalent to a path in the
		 * root of the cwd's volume. We will actually return
		 * non-volume-relative paths here. i.e. 'glob /foo*' will
		 * return 'C:/foobar'. This is much the same as globbing for a
		 * path with '\\' will return one with '/' on Windows.
		 */

		Tcl_Obj *cwd = Tcl_FSGetCwd(interp);

		if (cwd == NULL) {
		    Tcl_DecrRefCount(temp);
		    return TCL_ERROR;
		}
		pathPrefix = Tcl_NewStringObj(Tcl_GetString(cwd), 3);
		Tcl_DecrRefCount(cwd);
		if (tail[0] == '/') {
		    tail++;
		} else {
		    tail += 2;
		}
		Tcl_IncrRefCount(pathPrefix);
		break;
	    }
	    case TCL_PATH_ABSOLUTE:
		/*
		 * Absolute, possibly network path //Machine/Share. Use that
		 * as the path prefix (it already has a refCount).
		 */

		pathPrefix = driveName;
		tail += driveNameLen;
		break;
	    case TCL_PATH_RELATIVE:
		/* Do nothing */
		break;
	    }
	    Tcl_DecrRefCount(temp);
	}

	/*
	 * ':' no longer needed as a separator. It is only relevant to the
	 * beginning of the path.
	 */

	separators = "/\\";

    } else if (tclPlatform == TCL_PLATFORM_UNIX) {
	if (pathPrefix == NULL && tail[0] == '/') {
	    pathPrefix = Tcl_NewStringObj(tail, 1);
	    tail++;
	    Tcl_IncrRefCount(pathPrefix);
	}
    }

    /*
     * Finally if we still haven't managed to generate a path prefix, check if
     * the path starts with a current volume.
     */

    if (pathPrefix == NULL) {
	int driveNameLen;
	Tcl_Obj *driveName;
	if (TclFSNonnativePathType(tail, (int) strlen(tail), NULL,
		&driveNameLen, &driveName) == TCL_PATH_ABSOLUTE) {
	    pathPrefix = driveName;
	    tail += driveNameLen;
	}
    }

    /*
     * To process a [glob] invocation, this function may be called multiple
     * times. Each time, the previously discovered filenames are in the
     * interpreter result. We stash that away here so the result is free for
     * error messsages.
     */

    savedResultObj = Tcl_GetObjResult(interp);
    Tcl_IncrRefCount(savedResultObj);
    Tcl_ResetResult(interp);
    TclNewObj(filenamesObj);
    Tcl_IncrRefCount(filenamesObj);

    /*
     * Now we do the actual globbing, adding filenames as we go to buffer in
     * filenamesObj
     */

    if (*tail == '\0' && pathPrefix != NULL) {
	/*
	 * An empty pattern. This means 'pathPrefix' is actually a full path
	 * of a file/directory we want to simply check for existence and type.
	 */

	if (types == NULL) {
	    /*
	     * We just want to check for existence. In this case we make it
	     * easy on Tcl_FSMatchInDirectory and its sub-implementations by
	     * not bothering them (even though they should support this
	     * situation) and we just use the simple existence check with
	     * Tcl_FSAccess.
	     */

	    if (Tcl_FSAccess(pathPrefix, F_OK) == 0) {
		Tcl_ListObjAppendElement(interp, filenamesObj, pathPrefix);
	    }
	    result = TCL_OK;
	} else {
	    /*
	     * We want to check for the correct type. Tcl_FSMatchInDirectory
	     * is documented to do this for us, if we give it a NULL pattern.
	     */

	    result = Tcl_FSMatchInDirectory(interp, filenamesObj, pathPrefix,
		    NULL, types);
	}
    } else {
	result = DoGlob(interp, filenamesObj, separators, pathPrefix,
		globFlags & TCL_GLOBMODE_DIR, tail, types);
    }

    /*
     * Check for errors...
     */

    if (result != TCL_OK) {
	TclDecrRefCount(filenamesObj);
	TclDecrRefCount(savedResultObj);
	if (pathPrefix != NULL) {
	    Tcl_DecrRefCount(pathPrefix);
	}
	return result;
    }

    /*
     * If we only want the tails, we must strip off the prefix now. It may
     * seem more efficient to pass the tails flag down into DoGlob,
     * Tcl_FSMatchInDirectory, but those functions are continually adjusting
     * the prefix as the various pieces of the pattern are assimilated, so
     * that would add a lot of complexity to the code. This way is a little
     * slower (when the -tails flag is given), but much simpler to code.
     *
     * We do it by rewriting the result list in-place.
     */

    if (globFlags & TCL_GLOBMODE_TAILS) {
	int objc, i;
	Tcl_Obj **objv;
	int prefixLen;
	const char *pre;

	/*
	 * If this length has never been set, set it here.
	 */

	if (pathPrefix == NULL) {
	    Tcl_Panic("Called TclGlob with TCL_GLOBMODE_TAILS and pathPrefix==NULL");
	}

	pre = Tcl_GetStringFromObj(pathPrefix, &prefixLen);
	if (prefixLen > 0
		&& (strchr(separators, pre[prefixLen-1]) == NULL)) {
	    /*
	     * If we're on Windows and the prefix is a volume relative one
	     * like 'C:', then there won't be a path separator in between, so
	     * no need to skip it here.
	     */

	    if ((tclPlatform != TCL_PLATFORM_WINDOWS) || (prefixLen != 2)
		    || (pre[1] != ':')) {
		prefixLen++;
	    }
	}

	Tcl_ListObjGetElements(NULL, filenamesObj, &objc, &objv);
	for (i = 0; i< objc; i++) {
	    int len;
	    const char *oldStr = Tcl_GetStringFromObj(objv[i], &len);
	    Tcl_Obj *elem;

	    if (len == prefixLen) {
		if ((pattern[0] == '\0')
			|| (strchr(separators, pattern[0]) == NULL)) {
		    TclNewLiteralStringObj(elem, ".");
		} else {
		    TclNewLiteralStringObj(elem, "/");
		}
	    } else {
		elem = Tcl_NewStringObj(oldStr+prefixLen, len-prefixLen);
	    }
	    Tcl_ListObjReplace(interp, filenamesObj, i, 1, 1, &elem);
	}
    }

    /*
     * Now we have a list of discovered filenames in filenamesObj and a list
     * of previously discovered (saved earlier from the interpreter result) in
     * savedResultObj. Merge them and put them back in the interpreter result.
     */

    if (Tcl_IsShared(savedResultObj)) {
	TclDecrRefCount(savedResultObj);
	savedResultObj = Tcl_DuplicateObj(savedResultObj);
	Tcl_IncrRefCount(savedResultObj);
    }
    if (Tcl_ListObjAppendList(interp, savedResultObj, filenamesObj) != TCL_OK){
	result = TCL_ERROR;
    } else {
	Tcl_SetObjResult(interp, savedResultObj);
    }
    TclDecrRefCount(savedResultObj);
    TclDecrRefCount(filenamesObj);
    if (pathPrefix != NULL) {
	Tcl_DecrRefCount(pathPrefix);
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * SkipToChar --
 *
 *	This function traverses a glob pattern looking for the next unquoted
 *	occurance of the specified character at the same braces nesting level.
 *
 * Results:
 *	Updates stringPtr to point to the matching character, or to the end of
 *	the string if nothing matched. The return value is 1 if a match was
 *	found at the top level, otherwise it is 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
SkipToChar(
    char **stringPtr,		/* Pointer string to check. */
    int match)			/* Character to find. */
{
    int quoted, level;
    register char *p;

    quoted = 0;
    level = 0;

    for (p = *stringPtr; *p != '\0'; p++) {
	if (quoted) {
	    quoted = 0;
	    continue;
	}
	if ((level == 0) && (*p == match)) {
	    *stringPtr = p;
	    return 1;
	}
	if (*p == '{') {
	    level++;
	} else if (*p == '}') {
	    level--;
	} else if (*p == '\\') {
	    quoted = 1;
	}
    }
    *stringPtr = p;
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * DoGlob --
 *
 *	This recursive procedure forms the heart of the globbing code. It
 *	performs a depth-first traversal of the tree given by the path name to
 *	be globbed and the pattern. The directory and remainder are assumed to
 *	be native format paths. The prefix contained in 'pathPtr' is either a
 *	directory or path from which to start the search (or NULL). If pathPtr
 *	is NULL, then the pattern must not start with an absolute path
 *	specification (that case should be handled by moving the absolute path
 *	prefix into pathPtr before calling DoGlob).
 *
 * Results:
 *	The return value is a standard Tcl result indicating whether an error
 *	occurred in globbing. After a normal return the result in interp will
 *	be set to hold all of the file names given by the dir and remaining
 *	arguments. After an error the result in interp will hold an error
 *	message.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
DoGlob(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting
				 * (e.g. unmatched brace). */
    Tcl_Obj *matchesObj,	/* Unshared list object in which to place all
				 * resulting filenames. Caller allocates and
				 * deallocates; DoGlob must not touch the
				 * refCount of this object. */
    const char *separators,	/* String containing separator characters that
				 * should be used to identify globbing
				 * boundaries. */
    Tcl_Obj *pathPtr,		/* Completely expanded prefix. */
    int flags,			/* If non-zero then pathPtr is a directory */
    char *pattern,		/* The pattern to match against. Must not be a
				 * pointer to a static string. */
    Tcl_GlobTypeData *types)	/* List object containing list of acceptable
				 * types. May be NULL. */
{
    int baseLength, quoted, count;
    int result = TCL_OK;
    char *name, *p, *openBrace, *closeBrace, *firstSpecialChar;
    Tcl_Obj *joinedPtr;

    /*
     * Consume any leading directory separators, leaving pattern pointing just
     * past the last initial separator.
     */

    count = 0;
    name = pattern;
    for (; *pattern != '\0'; pattern++) {
	if (*pattern == '\\') {
	    /*
	     * If the first character is escaped, either we have a directory
	     * separator, or we have any other character. In the latter case
	     * the rest is a pattern, and we must break from the loop. This
	     * is particularly important on Windows where '\' is both the
	     * escaping character and a directory separator.
	     */

	    if (strchr(separators, pattern[1]) != NULL) {
		pattern++;
	    } else {
		break;
	    }
	} else if (strchr(separators, *pattern) == NULL) {
	    break;
	}
	count++;
    }

    /*
     * Look for the first matching pair of braces or the first directory
     * separator that is not inside a pair of braces.
     */

    openBrace = closeBrace = NULL;
    quoted = 0;
    for (p = pattern; *p != '\0'; p++) {
	if (quoted) {
	    quoted = 0;

	} else if (*p == '\\') {
	    quoted = 1;
	    if (strchr(separators, p[1]) != NULL) {
		/*
		 * Quoted directory separator.
		 */
		break;
	    }

	} else if (strchr(separators, *p) != NULL) {
	    /*
	     * Unquoted directory separator.
	     */
	    break;

	} else if (*p == '{') {
	    openBrace = p;
	    p++;
	    if (SkipToChar(&p, '}')) {
		/*
		 * Balanced braces.
		 */

		closeBrace = p;
		break;
	    }
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "unmatched open-brace in file name", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "GLOB", "BALANCE",
		    NULL);
	    return TCL_ERROR;

	} else if (*p == '}') {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "unmatched close-brace in file name", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "GLOB", "BALANCE",
		    NULL);
	    return TCL_ERROR;
	}
    }

    /*
     * Substitute the alternate patterns from the braces and recurse.
     */

    if (openBrace != NULL) {
	char *element;
	Tcl_DString newName;

	Tcl_DStringInit(&newName);

	/*
	 * For each element within in the outermost pair of braces, append the
	 * element and the remainder to the fixed portion before the first
	 * brace and recursively call DoGlob.
	 */

	Tcl_DStringAppend(&newName, pattern, openBrace-pattern);
	baseLength = Tcl_DStringLength(&newName);
	*closeBrace = '\0';
	for (p = openBrace; p != closeBrace; ) {
	    p++;
	    element = p;
	    SkipToChar(&p, ',');
	    Tcl_DStringSetLength(&newName, baseLength);
	    Tcl_DStringAppend(&newName, element, p-element);
	    Tcl_DStringAppend(&newName, closeBrace+1, -1);
	    result = DoGlob(interp, matchesObj, separators, pathPtr, flags,
		    Tcl_DStringValue(&newName), types);
	    if (result != TCL_OK) {
		break;
	    }
	}
	*closeBrace = '}';
	Tcl_DStringFree(&newName);
	return result;
    }

    /*
     * At this point, there are no more brace substitutions to perform on this
     * path component. The variable p is pointing at a quoted or unquoted
     * directory separator or the end of the string. So we need to check for
     * special globbing characters in the current pattern. We avoid modifying
     * pattern if p is pointing at the end of the string.
     *
     * If we find any globbing characters, then we must call
     * Tcl_FSMatchInDirectory. If we're at the end of the string, then that's
     * all we need to do. If we're not at the end of the string, then we must
     * recurse, so we do that below.
     *
     * Alternatively, if there are no globbing characters then again there are
     * two cases. If we're at the end of the string, we just need to check for
     * the given path's existence and type. If we're not at the end of the
     * string, we recurse.
     */

    if (*p != '\0') {
	char savedChar = *p;

	/*
	 * Note that we are modifying the string in place. This won't work if
	 * the string is a static.
	 */

	*p = '\0';
	firstSpecialChar = strpbrk(pattern, "*[]?\\");
	*p = savedChar;
    } else {
	firstSpecialChar = strpbrk(pattern, "*[]?\\");
    }

    if (firstSpecialChar != NULL) {
	/*
	 * Look for matching files in the given directory. The implementation
	 * of this function is filesystem specific. For each file that
	 * matches, it will add the match onto the resultPtr given.
	 */

	static Tcl_GlobTypeData dirOnly = {
	    TCL_GLOB_TYPE_DIR, 0, NULL, NULL
	};
	char save = *p;
	Tcl_Obj *subdirsPtr;

	if (*p == '\0') {
	    return Tcl_FSMatchInDirectory(interp, matchesObj, pathPtr,
		    pattern, types);
	}

	/*
	 * We do the recursion ourselves. This makes implementing
	 * Tcl_FSMatchInDirectory for each filesystem much easier.
	 */

	*p = '\0';
	TclNewObj(subdirsPtr);
	Tcl_IncrRefCount(subdirsPtr);
	result = Tcl_FSMatchInDirectory(interp, subdirsPtr, pathPtr,
		pattern, &dirOnly);
	*p = save;
	if (result == TCL_OK) {
	    int subdirc, i, repair = -1;
	    Tcl_Obj **subdirv;

	    result = Tcl_ListObjGetElements(interp, subdirsPtr,
		    &subdirc, &subdirv);
	    for (i=0; result==TCL_OK && i<subdirc; i++) {
		Tcl_Obj *copy = NULL;

		if (pathPtr == NULL && Tcl_GetString(subdirv[i])[0] == '~') {
		    Tcl_ListObjLength(NULL, matchesObj, &repair);
		    copy = subdirv[i];
		    subdirv[i] = Tcl_NewStringObj("./", 2);
		    Tcl_AppendObjToObj(subdirv[i], copy);
		    Tcl_IncrRefCount(subdirv[i]);
		}
		result = DoGlob(interp, matchesObj, separators, subdirv[i],
			1, p+1, types);
		if (copy) {
		    int end;

		    Tcl_DecrRefCount(subdirv[i]);
		    subdirv[i] = copy;
		    Tcl_ListObjLength(NULL, matchesObj, &end);
		    while (repair < end) {
			const char *bytes;
			int numBytes;
			Tcl_Obj *fixme, *newObj;

			Tcl_ListObjIndex(NULL, matchesObj, repair, &fixme);
			bytes = Tcl_GetStringFromObj(fixme, &numBytes);
			newObj = Tcl_NewStringObj(bytes+2, numBytes-2);
			Tcl_ListObjReplace(NULL, matchesObj, repair, 1,
				1, &newObj);
			repair++;
		    }
		    repair = -1;
		}
	    }
	}
	TclDecrRefCount(subdirsPtr);
	return result;
    }

    /*
     * We reach here with no pattern char in current section
     */

    if (*p == '\0') {
	int length;
	Tcl_DString append;

	/*
	 * This is the code path reached by a command like 'glob foo'.
	 *
	 * There are no more wildcards in the pattern and no more unprocessed
	 * characters in the pattern, so now we can construct the path, and
	 * pass it to Tcl_FSMatchInDirectory with an empty pattern to verify
	 * the existence of the file and check it is of the correct type (if a
	 * 'types' flag it given -- if no such flag was given, we could just
	 * use 'Tcl_FSLStat', but for simplicity we keep to a common
	 * approach).
	 */

	Tcl_DStringInit(&append);
	Tcl_DStringAppend(&append, pattern, p-pattern);

	if (pathPtr != NULL) {
	    (void) Tcl_GetStringFromObj(pathPtr, &length);
	} else {
	    length = 0;
	}

	switch (tclPlatform) {
	case TCL_PLATFORM_WINDOWS:
	    if (length == 0 && (Tcl_DStringLength(&append) == 0)) {
		if (((*name == '\\') && (name[1] == '/' ||
			name[1] == '\\')) || (*name == '/')) {
		    TclDStringAppendLiteral(&append, "/");
		} else {
		    TclDStringAppendLiteral(&append, ".");
		}
	    }

	    break;

	case TCL_PLATFORM_UNIX:
	    if (length == 0 && (Tcl_DStringLength(&append) == 0)) {
		if ((*name == '\\' && name[1] == '/') || (*name == '/')) {
		    TclDStringAppendLiteral(&append, "/");
		} else {
		    TclDStringAppendLiteral(&append, ".");
		}
	    }
	    break;
	}

	/*
	 * Common for all platforms.
	 */

	if (pathPtr == NULL) {
	    joinedPtr = TclDStringToObj(&append);
	} else if (flags) {
	    joinedPtr = TclNewFSPathObj(pathPtr, Tcl_DStringValue(&append),
		    Tcl_DStringLength(&append));
	} else {
	    joinedPtr = Tcl_DuplicateObj(pathPtr);
	    if (strchr(separators, Tcl_DStringValue(&append)[0]) == NULL) {
		/*
		 * The current prefix must end in a separator.
		 */

		int len;
		const char *joined = Tcl_GetStringFromObj(joinedPtr,&len);

		if (strchr(separators, joined[len-1]) == NULL) {
		    Tcl_AppendToObj(joinedPtr, "/", 1);
		}
	    }
	    Tcl_AppendToObj(joinedPtr, Tcl_DStringValue(&append),
		    Tcl_DStringLength(&append));
	}
	Tcl_IncrRefCount(joinedPtr);
	Tcl_DStringFree(&append);
	result = Tcl_FSMatchInDirectory(interp, matchesObj, joinedPtr, NULL,
		types);
	Tcl_DecrRefCount(joinedPtr);
	return result;
    }

    /*
     * If it's not the end of the string, we must recurse
     */

    if (pathPtr == NULL) {
	joinedPtr = Tcl_NewStringObj(pattern, p-pattern);
    } else if (flags) {
	joinedPtr = TclNewFSPathObj(pathPtr, pattern, p-pattern);
    } else {
	joinedPtr = Tcl_DuplicateObj(pathPtr);
	if (strchr(separators, pattern[0]) == NULL) {
	    /*
	     * The current prefix must end in a separator, unless this is a
	     * volume-relative path. In particular globbing in Windows shares,
	     * when not using -dir or -path, e.g. 'glob [file join
	     * //machine/share/subdir *]' requires adding a separator here.
	     * This behaviour is not currently tested for in the test suite.
	     */

	    int len;
	    const char *joined = Tcl_GetStringFromObj(joinedPtr,&len);

	    if (strchr(separators, joined[len-1]) == NULL) {
		if (Tcl_FSGetPathType(pathPtr) != TCL_PATH_VOLUME_RELATIVE) {
		    Tcl_AppendToObj(joinedPtr, "/", 1);
		}
	    }
	}
	Tcl_AppendToObj(joinedPtr, pattern, p-pattern);
    }

    Tcl_IncrRefCount(joinedPtr);
    result = DoGlob(interp, matchesObj, separators, joinedPtr, 1, p, types);
    Tcl_DecrRefCount(joinedPtr);

    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_AllocStatBuf --
 *
 *	This procedure allocates a Tcl_StatBuf on the heap. It exists so that
 *	extensions may be used unchanged on systems where largefile support is
 *	optional.
 *
 * Results:
 *	A pointer to a Tcl_StatBuf which may be deallocated by being passed to
 *	ckfree().
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tcl_StatBuf *
Tcl_AllocStatBuf(void)
{
    return ckalloc(sizeof(Tcl_StatBuf));
}

/*
 *---------------------------------------------------------------------------
 *
 * Access functions for Tcl_StatBuf --
 *
 *	These functions provide portable read-only access to the portable
 *	fields of the Tcl_StatBuf structure (really a 'struct stat', 'struct
 *	stat64' or something else related). [TIP #316]
 *
 * Results:
 *	The value from the field being retrieved.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

unsigned
Tcl_GetFSDeviceFromStat(
    const Tcl_StatBuf *statPtr)
{
    return (unsigned) statPtr->st_dev;
}

unsigned
Tcl_GetFSInodeFromStat(
    const Tcl_StatBuf *statPtr)
{
    return (unsigned) statPtr->st_ino;
}

unsigned
Tcl_GetModeFromStat(
    const Tcl_StatBuf *statPtr)
{
    return (unsigned) statPtr->st_mode;
}

int
Tcl_GetLinkCountFromStat(
    const Tcl_StatBuf *statPtr)
{
    return (int)statPtr->st_nlink;
}

int
Tcl_GetUserIdFromStat(
    const Tcl_StatBuf *statPtr)
{
    return (int) statPtr->st_uid;
}

int
Tcl_GetGroupIdFromStat(
    const Tcl_StatBuf *statPtr)
{
    return (int) statPtr->st_gid;
}

int
Tcl_GetDeviceTypeFromStat(
    const Tcl_StatBuf *statPtr)
{
    return (int) statPtr->st_rdev;
}

Tcl_WideInt
Tcl_GetAccessTimeFromStat(
    const Tcl_StatBuf *statPtr)
{
    return (Tcl_WideInt) statPtr->st_atime;
}

Tcl_WideInt
Tcl_GetModificationTimeFromStat(
    const Tcl_StatBuf *statPtr)
{
    return (Tcl_WideInt) statPtr->st_mtime;
}

Tcl_WideInt
Tcl_GetChangeTimeFromStat(
    const Tcl_StatBuf *statPtr)
{
    return (Tcl_WideInt) statPtr->st_ctime;
}

Tcl_WideUInt
Tcl_GetSizeFromStat(
    const Tcl_StatBuf *statPtr)
{
    return (Tcl_WideUInt) statPtr->st_size;
}

Tcl_WideUInt
Tcl_GetBlocksFromStat(
    const Tcl_StatBuf *statPtr)
{
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    return (Tcl_WideUInt) statPtr->st_blocks;
#else
    register unsigned blksize = Tcl_GetBlockSizeFromStat(statPtr);

    return ((Tcl_WideUInt) statPtr->st_size + blksize - 1) / blksize;
#endif
}

unsigned
Tcl_GetBlockSizeFromStat(
    const Tcl_StatBuf *statPtr)
{
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
    return (unsigned) statPtr->st_blksize;
#else
    /*
     * Not a great guess, but will do...
     */

    return GUESSED_BLOCK_SIZE;
#endif
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
