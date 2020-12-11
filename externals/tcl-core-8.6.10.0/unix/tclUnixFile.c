/*
 * tclUnixFile.c --
 *
 *	This file contains wrappers around UNIX file handling functions.
 *	These wrappers mask differences between Windows and UNIX.
 *
 * Copyright (c) 1995-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclFileSystem.h"

static int NativeMatchType(Tcl_Interp *interp, const char* nativeEntry,
	const char* nativeName, Tcl_GlobTypeData *types);

/*
 *---------------------------------------------------------------------------
 *
 * TclpFindExecutable --
 *
 *	This function computes the absolute path name of the current
 *	application, given its argv[0] value. For Cygwin, argv[0] is
 *	ignored and the path is determined the same as under win32.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The computed path name is stored as a ProcessGlobalValue.
 *
 *---------------------------------------------------------------------------
 */

void
TclpFindExecutable(
    const char *argv0)		/* The value of the application's argv[0]
				 * (native). */
{
    Tcl_Encoding encoding;
#ifdef __CYGWIN__
    int length;
    char buf[PATH_MAX * 2];
    char name[PATH_MAX * TCL_UTF_MAX + 1];
    GetModuleFileNameW(NULL, buf, PATH_MAX);
    cygwin_conv_path(3, buf, name, PATH_MAX);
    length = strlen(name);
    if ((length > 4) && !strcasecmp(name + length - 4, ".exe")) {
	/* Strip '.exe' part. */
	length -= 4;
    }
    encoding = Tcl_GetEncoding(NULL, NULL);
    TclSetObjNameOfExecutable(
	    Tcl_NewStringObj(name, length), encoding);
#else
    const char *name, *p;
    Tcl_StatBuf statBuf;
    Tcl_DString buffer, nameString, cwd, utfName;

    if (argv0 == NULL) {
	return;
    }
    Tcl_DStringInit(&buffer);

    name = argv0;
    for (p = name; *p != '\0'; p++) {
	if (*p == '/') {
	    /*
	     * The name contains a slash, so use the name directly without
	     * doing a path search.
	     */

	    goto gotName;
	}
    }

    p = getenv("PATH");					/* INTL: Native. */
    if (p == NULL) {
	/*
	 * There's no PATH environment variable; use the default that is used
	 * by sh.
	 */

	p = ":/bin:/usr/bin";
    } else if (*p == '\0') {
	/*
	 * An empty path is equivalent to ".".
	 */

	p = "./";
    }

    /*
     * Search through all the directories named in the PATH variable to see if
     * argv[0] is in one of them. If so, use that file name.
     */

    while (1) {
	while (TclIsSpaceProc(*p)) {
	    p++;
	}
	name = p;
	while ((*p != ':') && (*p != 0)) {
	    p++;
	}
	TclDStringClear(&buffer);
	if (p != name) {
	    Tcl_DStringAppend(&buffer, name, p - name);
	    if (p[-1] != '/') {
		TclDStringAppendLiteral(&buffer, "/");
	    }
	}
	name = Tcl_DStringAppend(&buffer, argv0, -1);

	/*
	 * INTL: The following calls to access() and stat() should not be
	 * converted to Tclp routines because they need to operate on native
	 * strings directly.
	 */

	if ((access(name, X_OK) == 0)			/* INTL: Native. */
		&& (TclOSstat(name, &statBuf) == 0)	/* INTL: Native. */
		&& S_ISREG(statBuf.st_mode)) {
	    goto gotName;
	}
	if (*p == '\0') {
	    break;
	} else if (*(p+1) == 0) {
	    p = "./";
	} else {
	    p++;
	}
    }
    TclSetObjNameOfExecutable(Tcl_NewObj(), NULL);
    goto done;

    /*
     * If the name starts with "/" then just store it
     */

  gotName:
#ifdef DJGPP
    if (name[1] == ':')
#else
    if (name[0] == '/')
#endif
    {
	encoding = Tcl_GetEncoding(NULL, NULL);
	Tcl_ExternalToUtfDString(encoding, name, -1, &utfName);
	TclSetObjNameOfExecutable(
		Tcl_NewStringObj(Tcl_DStringValue(&utfName), -1), encoding);
	Tcl_DStringFree(&utfName);
	goto done;
    }

    if (TclpGetCwd(NULL, &cwd) == NULL) {
	TclSetObjNameOfExecutable(Tcl_NewObj(), NULL);
	goto done;
    }

    /*
     * The name is relative to the current working directory. First strip off
     * a leading "./", if any, then add the full path name of the current
     * working directory.
     */

    if ((name[0] == '.') && (name[1] == '/')) {
	name += 2;
    }

    Tcl_DStringInit(&nameString);
    Tcl_DStringAppend(&nameString, name, -1);

    Tcl_DStringFree(&buffer);
    Tcl_UtfToExternalDString(NULL, Tcl_DStringValue(&cwd),
	    Tcl_DStringLength(&cwd), &buffer);
    if (Tcl_DStringValue(&cwd)[Tcl_DStringLength(&cwd) -1] != '/') {
	TclDStringAppendLiteral(&buffer, "/");
    }
    Tcl_DStringFree(&cwd);
    TclDStringAppendDString(&buffer, &nameString);
    Tcl_DStringFree(&nameString);

    encoding = Tcl_GetEncoding(NULL, NULL);
    Tcl_ExternalToUtfDString(encoding, Tcl_DStringValue(&buffer), -1,
	    &utfName);
    TclSetObjNameOfExecutable(
	    Tcl_NewStringObj(Tcl_DStringValue(&utfName), -1), encoding);
    Tcl_DStringFree(&utfName);

  done:
    Tcl_DStringFree(&buffer);
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TclpMatchInDirectory --
 *
 *	This routine is used by the globbing code to search a directory for
 *	all files which match a given pattern.
 *
 * Results:
 *	The return value is a standard Tcl result indicating whether an error
 *	occurred in globbing. Errors are left in interp, good results are
 *	[lappend]ed to resultPtr (which must be a valid object).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclpMatchInDirectory(
    Tcl_Interp *interp,		/* Interpreter to receive errors. */
    Tcl_Obj *resultPtr,		/* List object to lappend results. */
    Tcl_Obj *pathPtr,		/* Contains path to directory to search. */
    const char *pattern,	/* Pattern to match against. */
    Tcl_GlobTypeData *types)	/* Object containing list of acceptable types.
				 * May be NULL. In particular the directory
				 * flag is very important. */
{
    const char *native;
    Tcl_Obj *fileNamePtr;
    int matchResult = 0;

    if (types != NULL && types->type == TCL_GLOB_TYPE_MOUNT) {
	/*
	 * The native filesystem never adds mounts.
	 */

	return TCL_OK;
    }

    fileNamePtr = Tcl_FSGetTranslatedPath(interp, pathPtr);
    if (fileNamePtr == NULL) {
	return TCL_ERROR;
    }

    if (pattern == NULL || (*pattern == '\0')) {
	/*
	 * Match a file directly.
	 */

	Tcl_Obj *tailPtr;
	const char *nativeTail;

	native = Tcl_FSGetNativePath(pathPtr);
	tailPtr = TclPathPart(interp, pathPtr, TCL_PATH_TAIL);
	nativeTail = Tcl_FSGetNativePath(tailPtr);
	matchResult = NativeMatchType(interp, native, nativeTail, types);
	if (matchResult == 1) {
	    Tcl_ListObjAppendElement(interp, resultPtr, pathPtr);
	}
	Tcl_DecrRefCount(tailPtr);
	Tcl_DecrRefCount(fileNamePtr);
    } else {
	TclDIR *d;
	Tcl_DirEntry *entryPtr;
	const char *dirName;
	int dirLength, nativeDirLen;
	int matchHidden, matchHiddenPat;
	Tcl_StatBuf statBuf;
	Tcl_DString ds;		/* native encoding of dir */
	Tcl_DString dsOrig;	/* utf-8 encoding of dir */

	Tcl_DStringInit(&dsOrig);
	dirName = Tcl_GetStringFromObj(fileNamePtr, &dirLength);
	Tcl_DStringAppend(&dsOrig, dirName, dirLength);

	/*
	 * Make sure that the directory part of the name really is a
	 * directory. If the directory name is "", use the name "." instead,
	 * because some UNIX systems don't treat "" like "." automatically.
	 * Keep the "" for use in generating file names, otherwise "glob
	 * foo.c" would return "./foo.c".
	 */

	if (dirLength == 0) {
	    dirName = ".";
	} else {
	    dirName = Tcl_DStringValue(&dsOrig);

	    /*
	     * Make sure we have a trailing directory delimiter.
	     */

	    if (dirName[dirLength-1] != '/') {
		dirName = TclDStringAppendLiteral(&dsOrig, "/");
		dirLength++;
	    }
	}

	/*
	 * Now open the directory for reading and iterate over the contents.
	 */

	native = Tcl_UtfToExternalDString(NULL, dirName, -1, &ds);

	if ((TclOSstat(native, &statBuf) != 0)		/* INTL: Native. */
		|| !S_ISDIR(statBuf.st_mode)) {
	    Tcl_DStringFree(&dsOrig);
	    Tcl_DStringFree(&ds);
	    Tcl_DecrRefCount(fileNamePtr);
	    return TCL_OK;
	}

	d = TclOSopendir(native);				/* INTL: Native. */
	if (d == NULL) {
	    Tcl_DStringFree(&ds);
	    if (interp != NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"couldn't read directory \"%s\": %s",
			Tcl_DStringValue(&dsOrig), Tcl_PosixError(interp)));
	    }
	    Tcl_DStringFree(&dsOrig);
	    Tcl_DecrRefCount(fileNamePtr);
	    return TCL_ERROR;
	}

	nativeDirLen = Tcl_DStringLength(&ds);

	/*
	 * Check to see if -type or the pattern requests hidden files.
	 */

	matchHiddenPat = (pattern[0] == '.')
		|| ((pattern[0] == '\\') && (pattern[1] == '.'));
	matchHidden = matchHiddenPat
		|| (types && (types->perm & TCL_GLOB_PERM_HIDDEN));
	while ((entryPtr = TclOSreaddir(d)) != NULL) {	/* INTL: Native. */
	    Tcl_DString utfDs;
	    const char *utfname;

	    /*
	     * Skip this file if it doesn't agree with the hidden parameters
	     * requested by the user (via -type or pattern).
	     */

	    if (*entryPtr->d_name == '.') {
		if (!matchHidden) {
		    continue;
		}
	    } else {
#ifdef MAC_OSX_TCL
		if (matchHiddenPat) {
		    continue;
		}
		/* Also need to check HFS hidden flag in TclMacOSXMatchType. */
#else
		if (matchHidden) {
		    continue;
		}
#endif
	    }

	    /*
	     * Now check to see if the file matches, according to both type
	     * and pattern. If so, add the file to the result.
	     */

	    utfname = Tcl_ExternalToUtfDString(NULL, entryPtr->d_name, -1,
		    &utfDs);
	    if (Tcl_StringCaseMatch(utfname, pattern, 0)) {
		int typeOk = 1;

		if (types != NULL) {
		    Tcl_DStringSetLength(&ds, nativeDirLen);
		    native = Tcl_DStringAppend(&ds, entryPtr->d_name, -1);
		    matchResult = NativeMatchType(interp, native,
			    entryPtr->d_name, types);
		    typeOk = (matchResult == 1);
		}
		if (typeOk) {
		    Tcl_ListObjAppendElement(interp, resultPtr,
			    TclNewFSPathObj(pathPtr, utfname,
			    Tcl_DStringLength(&utfDs)));
		}
	    }
	    Tcl_DStringFree(&utfDs);
	    if (matchResult < 0) {
		break;
	    }
	}

	TclOSclosedir(d);
	Tcl_DStringFree(&ds);
	Tcl_DStringFree(&dsOrig);
	Tcl_DecrRefCount(fileNamePtr);
    }
    if (matchResult < 0) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NativeMatchType --
 *
 *	This routine is used by the globbing code to check if a file matches a
 *	given type description.
 *
 * Results:
 *	The return value is 1, 0 or -1 indicating whether the file matches the
 *	given criteria, does not match them, or an error occurred (in which
 *	case an error is left in interp).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
NativeMatchType(
    Tcl_Interp *interp,       /* Interpreter to receive errors. */
    const char *nativeEntry,  /* Native path to check. */
    const char *nativeName,   /* Native filename to check. */
    Tcl_GlobTypeData *types)  /* Type description to match against. */
{
    Tcl_StatBuf buf;

    if (types == NULL) {
	/*
	 * Simply check for the file's existence, but do it with lstat, in
	 * case it is a link to a file which doesn't exist (since that case
	 * would not show up if we used 'access' or 'stat')
	 */

	if (TclOSlstat(nativeEntry, &buf) != 0) {
	    return 0;
	}
	return 1;
    }

    if (types->perm != 0) {
	if (TclOSstat(nativeEntry, &buf) != 0) {
	    /*
	     * Either the file has disappeared between the 'readdir' call and
	     * the 'stat' call, or the file is a link to a file which doesn't
	     * exist (which we could ascertain with lstat), or there is some
	     * other strange problem. In all these cases, we define this to
	     * mean the file does not match any defined permission, and
	     * therefore it is not added to the list of files to return.
	     */

	    return 0;
	}

	/*
	 * readonly means that there are NO write permissions (even for user),
	 * but execute is OK for anybody OR that the user immutable flag is
	 * set (where supported).
	 */

	if (((types->perm & TCL_GLOB_PERM_RONLY) &&
#if defined(HAVE_CHFLAGS) && defined(UF_IMMUTABLE)
		!(buf.st_flags & UF_IMMUTABLE) &&
#endif
		(buf.st_mode & (S_IWOTH|S_IWGRP|S_IWUSR))) ||
	    ((types->perm & TCL_GLOB_PERM_R) &&
		(access(nativeEntry, R_OK) != 0)) ||
	    ((types->perm & TCL_GLOB_PERM_W) &&
		(access(nativeEntry, W_OK) != 0)) ||
	    ((types->perm & TCL_GLOB_PERM_X) &&
		(access(nativeEntry, X_OK) != 0))
#ifndef MAC_OSX_TCL
	    || ((types->perm & TCL_GLOB_PERM_HIDDEN) &&
		(*nativeName != '.'))
#endif /* MAC_OSX_TCL */
		) {
	    return 0;
	}
    }
    if (types->type != 0) {
	if (types->perm == 0) {
	    /*
	     * We haven't yet done a stat on the file.
	     */

	    if (TclOSstat(nativeEntry, &buf) != 0) {
		/*
		 * Posix error occurred. The only ok case is if this is a link
		 * to a nonexistent file, and the user did 'glob -l'. So we
		 * check that here:
		 */

		if ((types->type & TCL_GLOB_TYPE_LINK)
			&& (TclOSlstat(nativeEntry, &buf) == 0)
			&& S_ISLNK(buf.st_mode)) {
		    return 1;
		}
		return 0;
	    }
	}

	/*
	 * In order bcdpsfl as in 'find -t'
	 */

	if (    ((types->type & TCL_GLOB_TYPE_BLOCK)&& S_ISBLK(buf.st_mode)) ||
		((types->type & TCL_GLOB_TYPE_CHAR) && S_ISCHR(buf.st_mode)) ||
		((types->type & TCL_GLOB_TYPE_DIR)  && S_ISDIR(buf.st_mode)) ||
		((types->type & TCL_GLOB_TYPE_PIPE) && S_ISFIFO(buf.st_mode))||
#ifdef S_ISSOCK
		((types->type & TCL_GLOB_TYPE_SOCK) && S_ISSOCK(buf.st_mode))||
#endif /* S_ISSOCK */
		((types->type & TCL_GLOB_TYPE_FILE) && S_ISREG(buf.st_mode))) {
	    /*
	     * Do nothing - this file is ok.
	     */
	} else {
#ifdef S_ISLNK
	    if ((types->type & TCL_GLOB_TYPE_LINK)
		    && (TclOSlstat(nativeEntry, &buf) == 0)
		    && S_ISLNK(buf.st_mode)) {
		goto filetypeOK;
	    }
#endif /* S_ISLNK */
	    return 0;
	}
    }
  filetypeOK:

    /*
     * If we're on OSX, we also have to worry about matching the file creator
     * code (if specified). Do that now.
     */

#ifdef MAC_OSX_TCL
    if (types->macType != NULL || types->macCreator != NULL ||
	    (types->perm & TCL_GLOB_PERM_HIDDEN)) {
	int matchResult;

	if (types->perm == 0 && types->type == 0) {
	    /*
	     * We haven't yet done a stat on the file.
	     */

	    if (TclOSstat(nativeEntry, &buf) != 0) {
		return 0;
	    }
	}

	matchResult = TclMacOSXMatchType(interp, nativeEntry, nativeName,
		&buf, types);
	if (matchResult != 1) {
	    return matchResult;
	}
    }
#endif /* MAC_OSX_TCL */

    return 1;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpGetUserHome --
 *
 *	This function takes the specified user name and finds their home
 *	directory.
 *
 * Results:
 *	The result is a pointer to a string specifying the user's home
 *	directory, or NULL if the user's home directory could not be
 *	determined. Storage for the result string is allocated in bufferPtr;
 *	the caller must call Tcl_DStringFree() when the result is no longer
 *	needed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
TclpGetUserHome(
    const char *name,		/* User name for desired home directory. */
    Tcl_DString *bufferPtr)	/* Uninitialized or free DString filled with
				 * name of user's home directory. */
{
    struct passwd *pwPtr;
    Tcl_DString ds;
    const char *native = Tcl_UtfToExternalDString(NULL, name, -1, &ds);

    pwPtr = TclpGetPwNam(native);			/* INTL: Native. */
    Tcl_DStringFree(&ds);

    if (pwPtr == NULL) {
	return NULL;
    }
    Tcl_ExternalToUtfDString(NULL, pwPtr->pw_dir, -1, bufferPtr);
    return Tcl_DStringValue(bufferPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjAccess --
 *
 *	This function replaces the library version of access().
 *
 * Results:
 *	See access() documentation.
 *
 * Side effects:
 *	See access() documentation.
 *
 *---------------------------------------------------------------------------
 */

int
TclpObjAccess(
    Tcl_Obj *pathPtr,		/* Path of file to access */
    int mode)			/* Permission setting. */
{
    const char *path = Tcl_FSGetNativePath(pathPtr);

    if (path == NULL) {
	return -1;
    }
    return access(path, mode);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjChdir --
 *
 *	This function replaces the library version of chdir().
 *
 * Results:
 *	See chdir() documentation.
 *
 * Side effects:
 *	See chdir() documentation.
 *
 *---------------------------------------------------------------------------
 */

int
TclpObjChdir(
    Tcl_Obj *pathPtr)		/* Path to new working directory */
{
    const char *path = Tcl_FSGetNativePath(pathPtr);

    if (path == NULL) {
	return -1;
    }
    return chdir(path);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpObjLstat --
 *
 *	This function replaces the library version of lstat().
 *
 * Results:
 *	See lstat() documentation.
 *
 * Side effects:
 *	See lstat() documentation.
 *
 *----------------------------------------------------------------------
 */

int
TclpObjLstat(
    Tcl_Obj *pathPtr,		/* Path of file to stat */
    Tcl_StatBuf *bufPtr)	/* Filled with results of stat call. */
{
    return TclOSlstat(Tcl_FSGetNativePath(pathPtr), bufPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpGetNativeCwd --
 *
 *	This function replaces the library version of getcwd().
 *
 * Results:
 *	The input and output are filesystem paths in native form. The result
 *	is either the given clientData, if the working directory hasn't
 *	changed, or a new clientData (owned by our caller), giving the new
 *	native path, or NULL if the current directory could not be determined.
 *	If NULL is returned, the caller can examine the standard posix error
 *	codes to determine the cause of the problem.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ClientData
TclpGetNativeCwd(
    ClientData clientData)
{
    char buffer[MAXPATHLEN+1];

#ifdef USEGETWD
    if (getwd(buffer) == NULL) {			/* INTL: Native. */
	return NULL;
    }
#else
    if (getcwd(buffer, MAXPATHLEN+1) == NULL) {		/* INTL: Native. */
	return NULL;
    }
#endif /* USEGETWD */

    if ((clientData == NULL) || strcmp(buffer, (const char *) clientData)) {
	char *newCd = ckalloc(strlen(buffer) + 1);

	strcpy(newCd, buffer);
	return newCd;
    }

    /*
     * No change to pwd.
     */

    return clientData;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpGetCwd --
 *
 *	This function replaces the library version of getcwd(). (Obsolete
 *	function, only retained for old extensions which may call it
 *	directly).
 *
 * Results:
 *	The result is a pointer to a string specifying the current directory,
 *	or NULL if the current directory could not be determined. If NULL is
 *	returned, an error message is left in the interp's result. Storage for
 *	the result string is allocated in bufferPtr; the caller must call
 *	Tcl_DStringFree() when the result is no longer needed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
TclpGetCwd(
    Tcl_Interp *interp,		/* If non-NULL, used for error reporting. */
    Tcl_DString *bufferPtr)	/* Uninitialized or free DString filled with
				 * name of current directory. */
{
    char buffer[MAXPATHLEN+1];

#ifdef USEGETWD
    if (getwd(buffer) == NULL)				/* INTL: Native. */
#else
    if (getcwd(buffer, MAXPATHLEN+1) == NULL)		/* INTL: Native. */
#endif /* USEGETWD */
    {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "error getting working directory name: %s",
		    Tcl_PosixError(interp)));
	}
	return NULL;
    }
    return Tcl_ExternalToUtfDString(NULL, buffer, -1, bufferPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpReadlink --
 *
 *	This function replaces the library version of readlink().
 *
 * Results:
 *	The result is a pointer to a string specifying the contents of the
 *	symbolic link given by 'path', or NULL if the symbolic link could not
 *	be read. Storage for the result string is allocated in bufferPtr; the
 *	caller must call Tcl_DStringFree() when the result is no longer
 *	needed.
 *
 * Side effects:
 *	See readlink() documentation.
 *
 *---------------------------------------------------------------------------
 */

char *
TclpReadlink(
    const char *path,		/* Path of file to readlink (UTF-8). */
    Tcl_DString *linkPtr)	/* Uninitialized or free DString filled with
				 * contents of link (UTF-8). */
{
#ifndef DJGPP
    char link[MAXPATHLEN];
    int length;
    const char *native;
    Tcl_DString ds;

    native = Tcl_UtfToExternalDString(NULL, path, -1, &ds);
    length = readlink(native, link, sizeof(link));	/* INTL: Native. */
    Tcl_DStringFree(&ds);

    if (length < 0) {
	return NULL;
    }

    Tcl_ExternalToUtfDString(NULL, link, length, linkPtr);
    return Tcl_DStringValue(linkPtr);
#else
    return NULL;
#endif /* !DJGPP */
}

/*
 *----------------------------------------------------------------------
 *
 * TclpObjStat --
 *
 *	This function replaces the library version of stat().
 *
 * Results:
 *	See stat() documentation.
 *
 * Side effects:
 *	See stat() documentation.
 *
 *----------------------------------------------------------------------
 */

int
TclpObjStat(
    Tcl_Obj *pathPtr,		/* Path of file to stat */
    Tcl_StatBuf *bufPtr)	/* Filled with results of stat call. */
{
    const char *path = Tcl_FSGetNativePath(pathPtr);

    if (path == NULL) {
	return -1;
    }
    return TclOSstat(path, bufPtr);
}

#ifdef S_IFLNK

Tcl_Obj *
TclpObjLink(
    Tcl_Obj *pathPtr,
    Tcl_Obj *toPtr,
    int linkAction)
{
    if (toPtr != NULL) {
	const char *src = Tcl_FSGetNativePath(pathPtr);
	const char *target = NULL;

	if (src == NULL) {
	    return NULL;
	}

	/*
	 * If we're making a symbolic link and the path is relative, then we
	 * must check whether it exists _relative_ to the directory in which
	 * the src is found (not relative to the current cwd which is just not
	 * relevant in this case).
	 *
	 * If we're making a hard link, then a relative path is just converted
	 * to absolute relative to the cwd.
	 */

	if ((linkAction & TCL_CREATE_SYMBOLIC_LINK)
		&& (Tcl_FSGetPathType(toPtr) == TCL_PATH_RELATIVE)) {
	    Tcl_Obj *dirPtr, *absPtr;

	    dirPtr = TclPathPart(NULL, pathPtr, TCL_PATH_DIRNAME);
	    if (dirPtr == NULL) {
		return NULL;
	    }
	    absPtr = Tcl_FSJoinToPath(dirPtr, 1, &toPtr);
	    Tcl_IncrRefCount(absPtr);
	    if (Tcl_FSAccess(absPtr, F_OK) == -1) {
		Tcl_DecrRefCount(absPtr);
		Tcl_DecrRefCount(dirPtr);

		/*
		 * Target doesn't exist.
		 */

		errno = ENOENT;
		return NULL;
	    }

	    /*
	     * Target exists; we'll construct the relative path we want below.
	     */

	    Tcl_DecrRefCount(absPtr);
	    Tcl_DecrRefCount(dirPtr);
	} else {
	    target = Tcl_FSGetNativePath(toPtr);
	    if (target == NULL) {
		return NULL;
	    }
	    if (access(target, F_OK) == -1) {
		/*
		 * Target doesn't exist.
		 */

		errno = ENOENT;
		return NULL;
	    }
	}

	if (access(src, F_OK) != -1) {
	    /*
	     * Src exists.
	     */

	    errno = EEXIST;
	    return NULL;
	}

	/*
	 * Check symbolic link flag first, since we prefer to create these.
	 */

	if (linkAction & TCL_CREATE_SYMBOLIC_LINK) {
	    int targetLen;
	    Tcl_DString ds;
	    Tcl_Obj *transPtr;

	    /*
	     * Now we don't want to link to the absolute, normalized path.
	     * Relative links are quite acceptable (but links to ~user are not
	     * -- these must be expanded first).
	     */

	    transPtr = Tcl_FSGetTranslatedPath(NULL, toPtr);
	    if (transPtr == NULL) {
		return NULL;
	    }
	    target = Tcl_GetStringFromObj(transPtr, &targetLen);
	    target = Tcl_UtfToExternalDString(NULL, target, targetLen, &ds);
	    Tcl_DecrRefCount(transPtr);

	    if (symlink(target, src) != 0) {
		toPtr = NULL;
	    }
	    Tcl_DStringFree(&ds);
	} else if (linkAction & TCL_CREATE_HARD_LINK) {
	    if (link(target, src) != 0) {
		return NULL;
	    }
	} else {
	    errno = ENODEV;
	    return NULL;
	}
	return toPtr;
    } else {
	Tcl_Obj *linkPtr = NULL;

	char link[MAXPATHLEN];
	int length;
	Tcl_DString ds;
	Tcl_Obj *transPtr;

	transPtr = Tcl_FSGetTranslatedPath(NULL, pathPtr);
	if (transPtr == NULL) {
	    return NULL;
	}
	Tcl_DecrRefCount(transPtr);

	length = readlink(Tcl_FSGetNativePath(pathPtr), link, sizeof(link));
	if (length < 0) {
	    return NULL;
	}

	Tcl_ExternalToUtfDString(NULL, link, length, &ds);
	linkPtr = TclDStringToObj(&ds);
	Tcl_IncrRefCount(linkPtr);
	return linkPtr;
    }
}
#endif /* S_IFLNK */

/*
 *---------------------------------------------------------------------------
 *
 * TclpFilesystemPathType --
 *
 *	This function is part of the native filesystem support, and returns
 *	the path type of the given path. Right now it simply returns NULL. In
 *	the future it could return specific path types, like 'nfs', 'samba',
 *	'FAT32', etc.
 *
 * Results:
 *	NULL at present.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Obj *
TclpFilesystemPathType(
    Tcl_Obj *pathPtr)
{
    /*
     * All native paths are of the same type.
     */

    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpNativeToNormalized --
 *
 *	Convert native format to a normalized path object, with refCount of
 *	zero.
 *
 *	Currently assumes all native paths are actually normalized already, so
 *	if the path given is not normalized this will actually just convert to
 *	a valid string path, but not necessarily a normalized one.
 *
 * Results:
 *	A valid normalized path.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Obj *
TclpNativeToNormalized(
    ClientData clientData)
{
    Tcl_DString ds;

    Tcl_ExternalToUtfDString(NULL, (const char *) clientData, -1, &ds);
    return TclDStringToObj(&ds);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclNativeCreateNativeRep --
 *
 *	Create a native representation for the given path.
 *
 * Results:
 *	The nativePath representation.
 *
 * Side effects:
 *	Memory will be allocated. The path may need to be normalized.
 *
 *---------------------------------------------------------------------------
 */

ClientData
TclNativeCreateNativeRep(
    Tcl_Obj *pathPtr)
{
    char *nativePathPtr;
    const char *str;
    Tcl_DString ds;
    Tcl_Obj *validPathPtr;
    int len;

    if (TclFSCwdIsNative()) {
	/*
	 * The cwd is native, which means we can use the translated path
	 * without worrying about normalization (this will also usually be
	 * shorter so the utf-to-external conversion will be somewhat faster).
	 */

	validPathPtr = Tcl_FSGetTranslatedPath(NULL, pathPtr);
	if (validPathPtr == NULL) {
	    return NULL;
	}
    } else {
	/*
	 * Make sure the normalized path is set.
	 */

	validPathPtr = Tcl_FSGetNormalizedPath(NULL, pathPtr);
	if (validPathPtr == NULL) {
	    return NULL;
	}
	Tcl_IncrRefCount(validPathPtr);
    }

    str = Tcl_GetStringFromObj(validPathPtr, &len);
    Tcl_UtfToExternalDString(NULL, str, len, &ds);
    len = Tcl_DStringLength(&ds) + sizeof(char);
    if (strlen(Tcl_DStringValue(&ds)) < len - sizeof(char)) {
	/* See bug [3118489]: NUL in filenames */
	Tcl_DecrRefCount(validPathPtr);
	Tcl_DStringFree(&ds);
	return NULL;
    }
    Tcl_DecrRefCount(validPathPtr);
    nativePathPtr = ckalloc(len);
    memcpy(nativePathPtr, Tcl_DStringValue(&ds), (size_t) len);

    Tcl_DStringFree(&ds);
    return nativePathPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclNativeDupInternalRep --
 *
 *	Duplicate the native representation.
 *
 * Results:
 *	The copied native representation, or NULL if it is not possible to
 *	copy the representation.
 *
 * Side effects:
 *	Memory will be allocated for the copy.
 *
 *---------------------------------------------------------------------------
 */

ClientData
TclNativeDupInternalRep(
    ClientData clientData)
{
    char *copy;
    size_t len;

    if (clientData == NULL) {
	return NULL;
    }

    /*
     * ASCII representation when running on Unix.
     */

    len = (strlen((const char*) clientData) + 1) * sizeof(char);

    copy = ckalloc(len);
    memcpy(copy, clientData, len);
    return copy;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpUtime --
 *
 *	Set the modification date for a file.
 *
 * Results:
 *	0 on success, -1 on error.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TclpUtime(
    Tcl_Obj *pathPtr,		/* File to modify */
    struct utimbuf *tval)	/* New modification date structure */
{
    return utime(Tcl_FSGetNativePath(pathPtr), tval);
}

#ifdef __CYGWIN__

int
TclOSstat(
    const char *name,
    void *cygstat)
{
    struct stat buf;
    Tcl_StatBuf *statBuf = cygstat;
    int result = stat(name, &buf);

    statBuf->st_mode = buf.st_mode;
    statBuf->st_ino = buf.st_ino;
    statBuf->st_dev = buf.st_dev;
    statBuf->st_rdev = buf.st_rdev;
    statBuf->st_nlink = buf.st_nlink;
    statBuf->st_uid = buf.st_uid;
    statBuf->st_gid = buf.st_gid;
    statBuf->st_size = buf.st_size;
    statBuf->st_atime = buf.st_atime;
    statBuf->st_mtime = buf.st_mtime;
    statBuf->st_ctime = buf.st_ctime;
    return result;
}

int
TclOSlstat(
    const char *name,
    void *cygstat)
{
    struct stat buf;
    Tcl_StatBuf *statBuf = cygstat;
    int result = lstat(name, &buf);

    statBuf->st_mode = buf.st_mode;
    statBuf->st_ino = buf.st_ino;
    statBuf->st_dev = buf.st_dev;
    statBuf->st_rdev = buf.st_rdev;
    statBuf->st_nlink = buf.st_nlink;
    statBuf->st_uid = buf.st_uid;
    statBuf->st_gid = buf.st_gid;
    statBuf->st_size = buf.st_size;
    statBuf->st_atime = buf.st_atime;
    statBuf->st_mtime = buf.st_mtime;
    statBuf->st_ctime = buf.st_ctime;
    return result;
}
#endif /* CYGWIN */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
