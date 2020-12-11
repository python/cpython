/*
 * tclFCmd.c
 *
 *	This file implements the generic portion of file manipulation
 *	subcommands of the "file" command.
 *
 * Copyright (c) 1996-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclFileSystem.h"

/*
 * Declarations for local functions defined in this file:
 */

static int		CopyRenameOneFile(Tcl_Interp *interp,
			    Tcl_Obj *srcPathPtr, Tcl_Obj *destPathPtr,
			    int copyFlag, int force);
static Tcl_Obj *	FileBasename(Tcl_Interp *interp, Tcl_Obj *pathPtr);
static int		FileCopyRename(Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[], int copyFlag);
static int		FileForceOption(Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[], int *forcePtr);

/*
 *---------------------------------------------------------------------------
 *
 * TclFileRenameCmd
 *
 *	This function implements the "rename" subcommand of the "file"
 *	command. Filename arguments need to be translated to native format
 *	before being passed to platform-specific code that implements rename
 *	functionality.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

int
TclFileRenameCmd(
    ClientData clientData,	/* Unused */
    Tcl_Interp *interp,		/* Interp for error reporting or recursive
				 * calls in the case of a tricky rename. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument strings passed to Tcl_FileCmd. */
{
    return FileCopyRename(interp, objc, objv, 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclFileCopyCmd
 *
 *	This function implements the "copy" subcommand of the "file" command.
 *	Filename arguments need to be translated to native format before being
 *	passed to platform-specific code that implements copy functionality.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

int
TclFileCopyCmd(
    ClientData clientData,	/* Unused */
    Tcl_Interp *interp,		/* Used for error reporting or recursive calls
				 * in the case of a tricky copy. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument strings passed to Tcl_FileCmd. */
{
    return FileCopyRename(interp, objc, objv, 1);
}

/*
 *---------------------------------------------------------------------------
 *
 * FileCopyRename --
 *
 *	Performs the work of TclFileRenameCmd and TclFileCopyCmd. See
 *	comments for those functions.
 *
 * Results:
 *	See above.
 *
 * Side effects:
 *	See above.
 *
 *---------------------------------------------------------------------------
 */

static int
FileCopyRename(
    Tcl_Interp *interp,		/* Used for error reporting. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[],	/* Argument strings passed to Tcl_FileCmd. */
    int copyFlag)		/* If non-zero, copy source(s). Otherwise,
				 * rename them. */
{
    int i, result, force;
    Tcl_StatBuf statBuf;
    Tcl_Obj *target;

    i = FileForceOption(interp, objc - 1, objv + 1, &force);
    if (i < 0) {
	return TCL_ERROR;
    }
    i++;
    if ((objc - i) < 2) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-option value ...? source ?source ...? target");
	return TCL_ERROR;
    }

    /*
     * If target doesn't exist or isn't a directory, try the copy/rename. More
     * than 2 arguments is only valid if the target is an existing directory.
     */

    target = objv[objc - 1];
    if (Tcl_FSConvertToPathType(interp, target) != TCL_OK) {
	return TCL_ERROR;
    }

    result = TCL_OK;

    /*
     * Call Tcl_FSStat() so that if target is a symlink that points to a
     * directory we will put the sources in that directory instead of
     * overwriting the symlink.
     */

    if ((Tcl_FSStat(target, &statBuf) != 0) || !S_ISDIR(statBuf.st_mode)) {
	if ((objc - i) > 2) {
	    errno = ENOTDIR;
	    Tcl_PosixError(interp);
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "error %s: target \"%s\" is not a directory",
		    (copyFlag?"copying":"renaming"), TclGetString(target)));
	    result = TCL_ERROR;
	} else {
	    /*
	     * Even though already have target == translated(objv[i+1]), pass
	     * the original argument down, so if there's an error, the error
	     * message will reflect the original arguments.
	     */

	    result = CopyRenameOneFile(interp, objv[i], objv[i + 1], copyFlag,
		    force);
	}
	return result;
    }

    /*
     * Move each source file into target directory. Extract the basename from
     * each source, and append it to the end of the target path.
     */

    for ( ; i<objc-1 ; i++) {
	Tcl_Obj *jargv[2];
	Tcl_Obj *source, *newFileName;

	source = FileBasename(interp, objv[i]);
	if (source == NULL) {
	    result = TCL_ERROR;
	    break;
	}
	jargv[0] = objv[objc - 1];
	jargv[1] = source;
	newFileName = TclJoinPath(2, jargv, 1);
	Tcl_IncrRefCount(newFileName);
	result = CopyRenameOneFile(interp, objv[i], newFileName, copyFlag,
		force);
	Tcl_DecrRefCount(newFileName);
	Tcl_DecrRefCount(source);

	if (result == TCL_ERROR) {
	    break;
	}
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclFileMakeDirsCmd
 *
 *	This function implements the "mkdir" subcommand of the "file" command.
 *	Filename arguments need to be translated to native format before being
 *	passed to platform-specific code that implements mkdir functionality.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
TclFileMakeDirsCmd(
    ClientData clientData,	/* Unused */
    Tcl_Interp *interp,		/* Used for error reporting. */
    int objc,			/* Number of arguments */
    Tcl_Obj *const objv[])	/* Argument strings passed to Tcl_FileCmd. */
{
    Tcl_Obj *errfile = NULL;
    int result, i, j, pobjc;
    Tcl_Obj *split = NULL;
    Tcl_Obj *target = NULL;
    Tcl_StatBuf statBuf;

    result = TCL_OK;
    for (i = 1; i < objc; i++) {
	if (Tcl_FSConvertToPathType(interp, objv[i]) != TCL_OK) {
	    result = TCL_ERROR;
	    break;
	}

	split = Tcl_FSSplitPath(objv[i], &pobjc);
	Tcl_IncrRefCount(split);
	if (pobjc == 0) {
	    errno = ENOENT;
	    errfile = objv[i];
	    break;
	}
	for (j = 0; j < pobjc; j++) {
	    int errCount = 2;

	    target = Tcl_FSJoinPath(split, j + 1);
	    Tcl_IncrRefCount(target);

	createDir:

	    /*
	     * Call Tcl_FSStat() so that if target is a symlink that points to
	     * a directory we will create subdirectories in that directory.
	     */

	    if (Tcl_FSStat(target, &statBuf) == 0) {
		if (!S_ISDIR(statBuf.st_mode)) {
		    errno = EEXIST;
		    errfile = target;
		    goto done;
		}
	    } else if (errno != ENOENT) {
		/*
		 * If Tcl_FSStat() failed and the error is anything other than
		 * non-existence of the target, throw the error.
		 */

		errfile = target;
		goto done;
	    } else if (Tcl_FSCreateDirectory(target) != TCL_OK) {
		/*
		 * Create might have failed because of being in a race
		 * condition with another process trying to create the same
		 * subdirectory.
		 */

		if (errno == EEXIST) {
		    /* Be aware other workers could delete it immediately after
		     * creation, so give this worker still one chance (repeat once),
		     * see [270f78ca95] for description of the race-condition.
		     * Don't repeat the create always (to avoid endless loop). */
		    if (--errCount > 0) {
			goto createDir;
		    }
		    /* Already tried, with delete in-between directly after
		     * creation, so just continue (assume created successful). */
		    goto nextPart;
		}

		/* return with error */
		errfile = target;
		goto done;
	    }

	nextPart:
	    /*
	     * Forget about this sub-path.
	     */

	    Tcl_DecrRefCount(target);
	    target = NULL;
	}
	Tcl_DecrRefCount(split);
	split = NULL;
    }

  done:
    if (errfile != NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't create directory \"%s\": %s",
		TclGetString(errfile), Tcl_PosixError(interp)));
	result = TCL_ERROR;
    }
    if (split != NULL) {
	Tcl_DecrRefCount(split);
    }
    if (target != NULL) {
	Tcl_DecrRefCount(target);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFileDeleteCmd
 *
 *	This function implements the "delete" subcommand of the "file"
 *	command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
TclFileDeleteCmd(
    ClientData clientData,	/* Unused */
    Tcl_Interp *interp,		/* Used for error reporting */
    int objc,			/* Number of arguments */
    Tcl_Obj *const objv[])	/* Argument strings passed to Tcl_FileCmd. */
{
    int i, force, result;
    Tcl_Obj *errfile;
    Tcl_Obj *errorBuffer = NULL;

    i = FileForceOption(interp, objc - 1, objv + 1, &force);
    if (i < 0) {
	return TCL_ERROR;
    }

    errfile = NULL;
    result = TCL_OK;

    for (i++ ; i < objc; i++) {
	Tcl_StatBuf statBuf;

	errfile = objv[i];
	if (Tcl_FSConvertToPathType(interp, objv[i]) != TCL_OK) {
	    result = TCL_ERROR;
	    goto done;
	}

	/*
	 * Call lstat() to get info so can delete symbolic link itself.
	 */

	if (Tcl_FSLstat(objv[i], &statBuf) != 0) {
	    result = TCL_ERROR;
	} else if (S_ISDIR(statBuf.st_mode)) {
	    /*
	     * We own a reference count on errorBuffer, if it was set as a
	     * result of this call.
	     */

	    result = Tcl_FSRemoveDirectory(objv[i], force, &errorBuffer);
	    if (result != TCL_OK) {
		if ((force == 0) && (errno == EEXIST)) {
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "error deleting \"%s\": directory not empty",
			    TclGetString(objv[i])));
		    Tcl_PosixError(interp);
		    goto done;
		}

		/*
		 * If possible, use the untranslated name for the file.
		 */

		errfile = errorBuffer;

		/*
		 * FS supposed to check between translated objv and errfile.
		 */

		if (Tcl_FSEqualPaths(objv[i], errfile)) {
		    errfile = objv[i];
		}
	    }
	} else {
	    result = Tcl_FSDeleteFile(objv[i]);
	}

	if (result != TCL_OK) {

	    /*
	     * Avoid possible race condition (file/directory deleted after call
	     * of lstat), so bypass ENOENT because not an error, just a no-op
	     */
	    if (errno == ENOENT) {
		result = TCL_OK;
		continue;
	    }
	    /*
	     * It is important that we break on error, otherwise we might end
	     * up owning reference counts on numerous errorBuffers.
	     */
	    result = TCL_ERROR;
	    break;
	}
    }
    if (result != TCL_OK) {
	if (errfile == NULL) {
	    /*
	     * We try to accomodate poor error results from our Tcl_FS calls.
	     */

	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "error deleting unknown file: %s",
		    Tcl_PosixError(interp)));
	} else {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "error deleting \"%s\": %s",
		    TclGetString(errfile), Tcl_PosixError(interp)));
	}
    }

  done:
    if (errorBuffer != NULL) {
	Tcl_DecrRefCount(errorBuffer);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * CopyRenameOneFile
 *
 *	Copies or renames specified source file or directory hierarchy to the
 *	specified target.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Target is overwritten if the force flag is set. Attempting to
 *	copy/rename a file onto a directory or a directory onto a file will
 *	always result in an error.
 *
 *----------------------------------------------------------------------
 */

static int
CopyRenameOneFile(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Obj *source,		/* Pathname of file to copy. May need to be
				 * translated. */
    Tcl_Obj *target,		/* Pathname of file to create/overwrite. May
				 * need to be translated. */
    int copyFlag,		/* If non-zero, copy files. Otherwise, rename
				 * them. */
    int force)			/* If non-zero, overwrite target file if it
				 * exists. Otherwise, error if target already
				 * exists. */
{
    int result;
    Tcl_Obj *errfile, *errorBuffer;
    Tcl_Obj *actualSource=NULL;	/* If source is a link, then this is the real
				 * file/directory. */
    Tcl_StatBuf sourceStatBuf, targetStatBuf;

    if (Tcl_FSConvertToPathType(interp, source) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_FSConvertToPathType(interp, target) != TCL_OK) {
	return TCL_ERROR;
    }

    errfile = NULL;
    errorBuffer = NULL;
    result = TCL_ERROR;

    /*
     * We want to copy/rename links and not the files they point to, so we use
     * lstat(). If target is a link, we also want to replace the link and not
     * the file it points to, so we also use lstat() on the target.
     */

    if (Tcl_FSLstat(source, &sourceStatBuf) != 0) {
	errfile = source;
	goto done;
    }
    if (Tcl_FSLstat(target, &targetStatBuf) != 0) {
	if (errno != ENOENT) {
	    errfile = target;
	    goto done;
	}
    } else {
	if (force == 0) {
	    errno = EEXIST;
	    errfile = target;
	    goto done;
	}

	/*
	 * Prevent copying or renaming a file onto itself. On Windows since
	 * 8.5 we do get an inode number, however the unsigned short field is
	 * insufficient to accept the Win32 API file id so it is truncated to
	 * 16 bits and we get collisions. See bug #2015723.
	 */

#if !defined(_WIN32) && !defined(__CYGWIN__)
	if ((sourceStatBuf.st_ino != 0) && (targetStatBuf.st_ino != 0)) {
	    if ((sourceStatBuf.st_ino == targetStatBuf.st_ino) &&
		    (sourceStatBuf.st_dev == targetStatBuf.st_dev)) {
		result = TCL_OK;
		goto done;
	    }
	}
#endif

	/*
	 * Prevent copying/renaming a file onto a directory and vice-versa.
	 * This is a policy decision based on the fact that existing
	 * implementations of copy and rename on all platforms also prevent
	 * this.
	 */

	if (S_ISDIR(sourceStatBuf.st_mode)
		&& !S_ISDIR(targetStatBuf.st_mode)) {
	    errno = EISDIR;
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't overwrite file \"%s\" with directory \"%s\"",
		    TclGetString(target), TclGetString(source)));
	    goto done;
	}
	if (!S_ISDIR(sourceStatBuf.st_mode)
		&& S_ISDIR(targetStatBuf.st_mode)) {
	    errno = EISDIR;
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't overwrite directory \"%s\" with file \"%s\"",
		    TclGetString(target), TclGetString(source)));
	    goto done;
	}

	/*
	 * The destination exists, but appears to be ok to over-write, and
	 * -force is given. We now try to adjust permissions to ensure the
	 * operation succeeds. If we can't adjust permissions, we'll let the
	 * actual copy/rename return an error later.
	 */

	{
	    Tcl_Obj *perm;
	    int index;

	    TclNewLiteralStringObj(perm, "u+w");
	    Tcl_IncrRefCount(perm);
	    if (TclFSFileAttrIndex(target, "-permissions", &index) == TCL_OK) {
		Tcl_FSFileAttrsSet(NULL, index, target, perm);
	    }
	    Tcl_DecrRefCount(perm);
	}
    }

    if (copyFlag == 0) {
	result = Tcl_FSRenameFile(source, target);
	if (result == TCL_OK) {
	    goto done;
	}

	if (errno == EINVAL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "error renaming \"%s\" to \"%s\": trying to rename a"
		    " volume or move a directory into itself",
		    TclGetString(source), TclGetString(target)));
	    goto done;
	} else if (errno != EXDEV) {
	    errfile = target;
	    goto done;
	}

	/*
	 * The rename failed because the move was across file systems. Fall
	 * through to copy file and then remove original. Note that the
	 * low-level Tcl_FSRenameFileProc in the filesystem is allowed to
	 * implement cross-filesystem moves itself, if it desires.
	 */
    }

    actualSource = source;
    Tcl_IncrRefCount(actualSource);

    /*
     * Activate the following block to copy files instead of links. However
     * Tcl's semantics currently say we should copy links, so any such change
     * should be the subject of careful study on the consequences.
     *
     * Perhaps there could be an optional flag to 'file copy' to dictate which
     * approach to use, with the default being _not_ to have this block
     * active.
     */

#if 0
#ifdef S_ISLNK
    if (copyFlag && S_ISLNK(sourceStatBuf.st_mode)) {
	/*
	 * We want to copy files not links. Therefore we must follow the link.
	 * There are two purposes to this 'stat' call here. First we want to
	 * know if the linked-file/dir actually exists, and second, in the
	 * block of code which follows, some 20 lines down, we want to check
	 * if the thing is a file or directory.
	 */

	if (Tcl_FSStat(source, &sourceStatBuf) != 0) {
	    /*
	     * Actual file doesn't exist.
	     */

	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "error copying \"%s\": the target of this link doesn't"
		    " exist", TclGetString(source)));
	    goto done;
	} else {
	    int counter = 0;

	    while (1) {
		Tcl_Obj *path = Tcl_FSLink(actualSource, NULL, 0);
		if (path == NULL) {
		    break;
		}

		/*
		 * Now we want to check if this is a relative path, and if so,
		 * to make it absolute.
		 */

		if (Tcl_FSGetPathType(path) == TCL_PATH_RELATIVE) {
		    Tcl_Obj *abs = Tcl_FSJoinToPath(actualSource, 1, &path);

		    if (abs == NULL) {
			break;
		    }
		    Tcl_IncrRefCount(abs);
		    Tcl_DecrRefCount(path);
		    path = abs;
		}
		Tcl_DecrRefCount(actualSource);
		actualSource = path;
		counter++;

		/*
		 * Arbitrary limit of 20 links to follow.
		 */

		if (counter > 20) {
		    /*
		     * Too many links.
		     */

		    Tcl_SetErrno(EMLINK);
		    errfile = source;
		    goto done;
		}
	    }
	    /* Now 'actualSource' is the correct file */
	}
    }
#endif /* S_ISLNK */
#endif

    if (S_ISDIR(sourceStatBuf.st_mode)) {
	result = Tcl_FSCopyDirectory(actualSource, target, &errorBuffer);
	if (result != TCL_OK) {
	    if (errno == EXDEV) {
		/*
		 * The copy failed because we're trying to do a
		 * cross-filesystem copy. We do this through our Tcl library.
		 */

		Tcl_Obj *copyCommand, *cmdObj, *opObj;

		TclNewObj(copyCommand);
		TclNewLiteralStringObj(cmdObj, "::tcl::CopyDirectory");
		Tcl_ListObjAppendElement(interp, copyCommand, cmdObj);
		if (copyFlag) {
		    TclNewLiteralStringObj(opObj, "copying");
		} else {
		    TclNewLiteralStringObj(opObj, "renaming");
		}
		Tcl_ListObjAppendElement(interp, copyCommand, opObj);
		Tcl_ListObjAppendElement(interp, copyCommand, source);
		Tcl_ListObjAppendElement(interp, copyCommand, target);
		Tcl_IncrRefCount(copyCommand);
		result = Tcl_EvalObjEx(interp, copyCommand,
			TCL_EVAL_GLOBAL | TCL_EVAL_DIRECT);
		Tcl_DecrRefCount(copyCommand);
		if (result != TCL_OK) {
		    /*
		     * There was an error in the Tcl-level copy. We will pass
		     * on the Tcl error message and can ensure this by setting
		     * errfile to NULL
		     */

		    errfile = NULL;
		}
	    } else {
		errfile = errorBuffer;
		if (Tcl_FSEqualPaths(errfile, source)) {
		    errfile = source;
		} else if (Tcl_FSEqualPaths(errfile, target)) {
		    errfile = target;
		}
	    }
	}
    } else {
	result = Tcl_FSCopyFile(actualSource, target);
	if ((result != TCL_OK) && (errno == EXDEV)) {
	    result = TclCrossFilesystemCopy(interp, source, target);
	}
	if (result != TCL_OK) {
	    /*
	     * We could examine 'errno' to double-check if the problem was
	     * with the target, but we checked the source above, so it should
	     * be quite clear
	     */

	    errfile = target;
	}
	/*
	 * We now need to reset the result, because the above call,
	 * may have left set it.  (Ideally we would prefer not to pass
	 * an interpreter in above, but the channel IO code used by
	 * TclCrossFilesystemCopy currently requires one)
	 */
	Tcl_ResetResult(interp);
    }
    if ((copyFlag == 0) && (result == TCL_OK)) {
	if (S_ISDIR(sourceStatBuf.st_mode)) {
	    result = Tcl_FSRemoveDirectory(source, 1, &errorBuffer);
	    if (result != TCL_OK) {
		errfile = errorBuffer;
		if (Tcl_FSEqualPaths(errfile, source) == 0) {
		    errfile = source;
		}
	    }
	} else {
	    result = Tcl_FSDeleteFile(source);
	    if (result != TCL_OK) {
		errfile = source;
	    }
	}
	if (result != TCL_OK) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf("can't unlink \"%s\": %s",
		    TclGetString(errfile), Tcl_PosixError(interp)));
	    errfile = NULL;
	}
    }

  done:
    if (errfile != NULL) {
	Tcl_Obj *errorMsg = Tcl_ObjPrintf("error %s \"%s\"",
		(copyFlag ? "copying" : "renaming"), TclGetString(source));

	if (errfile != source) {
	    Tcl_AppendPrintfToObj(errorMsg, " to \"%s\"",
		    TclGetString(target));
	    if (errfile != target) {
		Tcl_AppendPrintfToObj(errorMsg, ": \"%s\"",
			TclGetString(errfile));
	    }
	}
	Tcl_AppendPrintfToObj(errorMsg, ": %s", Tcl_PosixError(interp));
	Tcl_SetObjResult(interp, errorMsg);
    }
    if (errorBuffer != NULL) {
	Tcl_DecrRefCount(errorBuffer);
    }
    if (actualSource != NULL) {
	Tcl_DecrRefCount(actualSource);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * FileForceOption --
 *
 *	Helps parse command line options for file commands that take the
 *	"-force" and "--" options.
 *
 * Results:
 *	The return value is how many arguments from argv were consumed by this
 *	function, or -1 if there was an error parsing the options. If an error
 *	occurred, an error message is left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
FileForceOption(
    Tcl_Interp *interp,		/* Interp, for error return. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[],	/* Argument strings.  First command line
				 * option, if it exists, begins at 0. */
    int *forcePtr)		/* If the "-force" was specified, *forcePtr is
				 * filled with 1, otherwise with 0. */
{
    int force, i, idx;
    static const char *const options[] = {
	"-force", "--", NULL
    };

    force = 0;
    for (i = 0; i < objc; i++) {
	if (TclGetString(objv[i])[0] != '-') {
	    break;
	}
	if (Tcl_GetIndexFromObj(interp, objv[i], options, "option", TCL_EXACT,
		&idx) != TCL_OK) {
	    return -1;
	}
	if (idx == 0 /* -force */) {
	    force = 1;
	} else { /* -- */
	    i++;
	    break;
	}
    }
    *forcePtr = force;
    return i;
}
/*
 *---------------------------------------------------------------------------
 *
 * FileBasename --
 *
 *	Given a path in either tcl format (with / separators), or in the
 *	platform-specific format for the current platform, return all the
 *	characters in the path after the last directory separator. But, if
 *	path is the root directory, returns no characters.
 *
 * Results:
 *	Returns the string object that represents the basename. If there is an
 *	error, an error message is left in interp, and NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static Tcl_Obj *
FileBasename(
    Tcl_Interp *interp,		/* Interp, for error return. */
    Tcl_Obj *pathPtr)		/* Path whose basename to extract. */
{
    int objc;
    Tcl_Obj *splitPtr;
    Tcl_Obj *resultPtr = NULL;

    splitPtr = Tcl_FSSplitPath(pathPtr, &objc);
    Tcl_IncrRefCount(splitPtr);

    if (objc != 0) {
	if ((objc == 1) && (*TclGetString(pathPtr) == '~')) {
	    Tcl_DecrRefCount(splitPtr);
	    if (Tcl_FSConvertToPathType(interp, pathPtr) != TCL_OK) {
		return NULL;
	    }
	    splitPtr = Tcl_FSSplitPath(pathPtr, &objc);
	    Tcl_IncrRefCount(splitPtr);
	}

	/*
	 * Return the last component, unless it is the only component, and it
	 * is the root of an absolute path.
	 */

	if (objc > 0) {
	    Tcl_ListObjIndex(NULL, splitPtr, objc-1, &resultPtr);
	    if ((objc == 1) &&
		    (Tcl_FSGetPathType(resultPtr) != TCL_PATH_RELATIVE)) {
		resultPtr = NULL;
	    }
	}
    }
    if (resultPtr == NULL) {
	resultPtr = Tcl_NewObj();
    }
    Tcl_IncrRefCount(resultPtr);
    Tcl_DecrRefCount(splitPtr);
    return resultPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFileAttrsCmd --
 *
 *	Sets or gets the platform-specific attributes of a file. The objc-objv
 *	points to the file name with the rest of the command line following.
 *	This routine uses platform-specific tables of option strings and
 *	callbacks. The callback to get the attributes take three parameters:
 *	    Tcl_Interp *interp;	    The interp to report errors with. Since
 *				    this is an object-based API, the object
 *				    form of the result should be used.
 *	    const char *fileName;   This is extracted using
 *				    Tcl_TranslateFileName.
 *	    TclObj **attrObjPtrPtr; A new object to hold the attribute is
 *				    allocated and put here.
 *	The first two parameters of the callback used to write out the
 *	attributes are the same. The third parameter is:
 *	    const *attrObjPtr;	    A pointer to the object that has the new
 *				    attribute.
 *	They both return standard TCL errors; if the routine to get an
 *	attribute fails, no object is allocated and *attrObjPtrPtr is
 *	unchanged.
 *
 * Results:
 *	Standard TCL error.
 *
 * Side effects:
 *	May set file attributes for the file name.
 *
 *----------------------------------------------------------------------
 */

int
TclFileAttrsCmd(
    ClientData clientData,	/* Unused */
    Tcl_Interp *interp,		/* The interpreter for error reporting. */
    int objc,			/* Number of command line arguments. */
    Tcl_Obj *const objv[])	/* The command line objects. */
{
    int result;
    const char *const *attributeStrings;
    const char **attributeStringsAllocated = NULL;
    Tcl_Obj *objStrings = NULL;
    int numObjStrings = -1;
    Tcl_Obj *filePtr;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "name ?-option value ...?");
	return TCL_ERROR;
    }

    filePtr = objv[1];
    if (Tcl_FSConvertToPathType(interp, filePtr) != TCL_OK) {
	return TCL_ERROR;
    }

    objc -= 2;
    objv += 2;
    result = TCL_ERROR;
    Tcl_SetErrno(0);

    /*
     * Get the set of attribute names from the filesystem.
     */

    attributeStrings = Tcl_FSFileAttrStrings(filePtr, &objStrings);
    if (attributeStrings == NULL) {
	int index;
	Tcl_Obj *objPtr;

	if (objStrings == NULL) {
	    if (Tcl_GetErrno() != 0) {
		/*
		 * There was an error, probably that the filePtr is not
		 * accepted by any filesystem
		 */

		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"could not read \"%s\": %s",
			TclGetString(filePtr), Tcl_PosixError(interp)));
	    }
	    return TCL_ERROR;
	}

	/*
	 * We own the object now.
	 */

	Tcl_IncrRefCount(objStrings);

	/*
	 * Use objStrings as a list object.
	 */

	if (Tcl_ListObjLength(interp, objStrings, &numObjStrings) != TCL_OK) {
	    goto end;
	}
	attributeStringsAllocated = (const char **)
		TclStackAlloc(interp, (1+numObjStrings) * sizeof(char *));
	for (index = 0; index < numObjStrings; index++) {
	    Tcl_ListObjIndex(interp, objStrings, index, &objPtr);
	    attributeStringsAllocated[index] = TclGetString(objPtr);
	}
	attributeStringsAllocated[index] = NULL;
	attributeStrings = attributeStringsAllocated;
    } else if (objStrings != NULL) {
	Tcl_Panic("must not update objPtrRef's variable and return non-NULL");
    }

    /*
     * Process the attributes to produce a list of all of them, the value of a
     * particular attribute, or to set one or more attributes (depending on
     * the number of arguments).
     */

    if (objc == 0) {
	/*
	 * Get all attributes.
	 */

	int index, res = TCL_OK, nbAtts = 0;
	Tcl_Obj *listPtr;

	listPtr = Tcl_NewListObj(0, NULL);
	for (index = 0; attributeStrings[index] != NULL; index++) {
	    Tcl_Obj *objPtrAttr;

	    if (res != TCL_OK) {
		/*
		 * Clear the error from the last iteration.
		 */

		Tcl_ResetResult(interp);
	    }

	    res = Tcl_FSFileAttrsGet(interp, index, filePtr, &objPtrAttr);
	    if (res == TCL_OK) {
		Tcl_Obj *objPtr =
			Tcl_NewStringObj(attributeStrings[index], -1);

		Tcl_ListObjAppendElement(interp, listPtr, objPtr);
		Tcl_ListObjAppendElement(interp, listPtr, objPtrAttr);
		nbAtts++;
	    }
	}

	if (index > 0 && nbAtts == 0) {
	    /*
	     * Error: no valid attributes found.
	     */

	    Tcl_DecrRefCount(listPtr);
	    goto end;
	}

	Tcl_SetObjResult(interp, listPtr);
    } else if (objc == 1) {
	/*
	 * Get one attribute.
	 */

	int index;
	Tcl_Obj *objPtr = NULL;

	if (numObjStrings == 0) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad option \"%s\", there are no file attributes in this"
		    " filesystem", TclGetString(objv[0])));
	    Tcl_SetErrorCode(interp, "TCL","OPERATION","FATTR","NONE", NULL);
	    goto end;
	}

	if (Tcl_GetIndexFromObj(interp, objv[0], attributeStrings,
		"option", 0, &index) != TCL_OK) {
	    goto end;
	}
	if (attributeStringsAllocated != NULL) {
	    TclFreeIntRep(objv[0]);
	}
	if (Tcl_FSFileAttrsGet(interp, index, filePtr,
		&objPtr) != TCL_OK) {
	    goto end;
	}
	Tcl_SetObjResult(interp, objPtr);
    } else {
	/*
	 * Set option/value pairs.
	 */

	int i, index;

	if (numObjStrings == 0) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad option \"%s\", there are no file attributes in this"
		    " filesystem", TclGetString(objv[0])));
	    Tcl_SetErrorCode(interp, "TCL","OPERATION","FATTR","NONE", NULL);
	    goto end;
	}

	for (i = 0; i < objc ; i += 2) {
	    if (Tcl_GetIndexFromObj(interp, objv[i], attributeStrings,
		    "option", 0, &index) != TCL_OK) {
		goto end;
	    }
	    if (attributeStringsAllocated != NULL) {
		TclFreeIntRep(objv[i]);
	    }
	    if (i + 1 == objc) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"value for \"%s\" missing", TclGetString(objv[i])));
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "FATTR",
			"NOVALUE", NULL);
		goto end;
	    }
	    if (Tcl_FSFileAttrsSet(interp, index, filePtr,
		    objv[i + 1]) != TCL_OK) {
		goto end;
	    }
	}
    }
    result = TCL_OK;

    /*
     * Free up the array we allocated and drop our reference to any list of
     * attribute names issued by the filesystem.
     */

  end:
    if (attributeStringsAllocated != NULL) {
	TclStackFree(interp, (void *) attributeStringsAllocated);
    }
    if (objStrings != NULL) {
	Tcl_DecrRefCount(objStrings);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFileLinkCmd --
 *
 *	This function is invoked to process the "file link" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May create a new link.
 *
 *----------------------------------------------------------------------
 */

int
TclFileLinkCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Tcl_Obj *contents;
    int index;

    if (objc < 2 || objc > 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "?-linktype? linkname ?target?");
	return TCL_ERROR;
    }

    /*
     * Index of the 'source' argument.
     */

    if (objc == 4) {
	index = 2;
    } else {
	index = 1;
    }

    if (objc > 2) {
	int linkAction;

	if (objc == 4) {
	    /*
	     * We have a '-linktype' argument.
	     */

	    static const char *const linkTypes[] = {
		"-symbolic", "-hard", NULL
	    };
	    if (Tcl_GetIndexFromObj(interp, objv[1], linkTypes, "option", 0,
		    &linkAction) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (linkAction == 0) {
		linkAction = TCL_CREATE_SYMBOLIC_LINK;
	    } else {
		linkAction = TCL_CREATE_HARD_LINK;
	    }
	} else {
	    linkAction = TCL_CREATE_SYMBOLIC_LINK | TCL_CREATE_HARD_LINK;
	}
	if (Tcl_FSConvertToPathType(interp, objv[index]) != TCL_OK) {
	    return TCL_ERROR;
	}

	/*
	 * Create link from source to target.
	 */

	contents = Tcl_FSLink(objv[index], objv[index+1], linkAction);
	if (contents == NULL) {
	    /*
	     * We handle three common error cases specially, and for all other
	     * errors, we use the standard posix error message.
	     */

	    if (errno == EEXIST) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"could not create new link \"%s\": that path already"
			" exists", TclGetString(objv[index])));
		Tcl_PosixError(interp);
	    } else if (errno == ENOENT) {
		/*
		 * There are two cases here: either the target doesn't exist,
		 * or the directory of the src doesn't exist.
		 */

		int access;
		Tcl_Obj *dirPtr = TclPathPart(interp, objv[index],
			TCL_PATH_DIRNAME);

		if (dirPtr == NULL) {
		    return TCL_ERROR;
		}
		access = Tcl_FSAccess(dirPtr, F_OK);
		Tcl_DecrRefCount(dirPtr);
		if (access != 0) {
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "could not create new link \"%s\": no such file"
			    " or directory", TclGetString(objv[index])));
		    Tcl_PosixError(interp);
		} else {
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "could not create new link \"%s\": target \"%s\" "
			    "doesn't exist", TclGetString(objv[index]),
			    TclGetString(objv[index+1])));
		    errno = ENOENT;
		    Tcl_PosixError(interp);
		}
	    } else {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"could not create new link \"%s\" pointing to \"%s\": %s",
			TclGetString(objv[index]),
			TclGetString(objv[index+1]), Tcl_PosixError(interp)));
	    }
	    return TCL_ERROR;
	}
    } else {
	if (Tcl_FSConvertToPathType(interp, objv[index]) != TCL_OK) {
	    return TCL_ERROR;
	}

	/*
	 * Read link
	 */

	contents = Tcl_FSLink(objv[index], NULL, 0);
	if (contents == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "could not read link \"%s\": %s",
		    TclGetString(objv[index]), Tcl_PosixError(interp)));
	    return TCL_ERROR;
	}
    }
    Tcl_SetObjResult(interp, contents);
    if (objc == 2) {
	/*
	 * If we are reading a link, we need to free this result refCount. If
	 * we are creating a link, this will just be objv[index+1], and so we
	 * don't own it.
	 */

	Tcl_DecrRefCount(contents);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFileReadLinkCmd --
 *
 *	This function is invoked to process the "file readlink" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclFileReadLinkCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Tcl_Obj *contents;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "name");
	return TCL_ERROR;
    }

    if (Tcl_FSConvertToPathType(interp, objv[1]) != TCL_OK) {
	return TCL_ERROR;
    }

    contents = Tcl_FSLink(objv[1], NULL, 0);

    if (contents == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"could not read link \"%s\": %s",
		TclGetString(objv[1]), Tcl_PosixError(interp)));
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, contents);
    Tcl_DecrRefCount(contents);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclFileTemporaryCmd
 *
 *	This function implements the "tempfile" subcommand of the "file"
 *	command.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	Creates a temporary file. Opens a channel to that file and puts the
 *	name of that channel in the result. *Might* register suitable exit
 *	handlers to ensure that the temporary file gets deleted. Might write
 *	to a variable, so reentrancy is a potential issue.
 *
 *---------------------------------------------------------------------------
 */

int
TclFileTemporaryCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Tcl_Obj *nameVarObj = NULL;	/* Variable to store the name of the temporary
				 * file in. */
    Tcl_Obj *nameObj = NULL;	/* Object that will contain the filename. */
    Tcl_Channel chan;		/* The channel opened (RDWR) on the temporary
				 * file, or NULL if there's an error. */
    Tcl_Obj *tempDirObj = NULL, *tempBaseObj = NULL, *tempExtObj = NULL;
				/* Pieces of template. Each piece is NULL if
				 * it is omitted. The platform temporary file
				 * engine might ignore some pieces. */

    if (objc < 1 || objc > 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "?nameVar? ?template?");
	return TCL_ERROR;
    }

    if (objc > 1) {
	nameVarObj = objv[1];
	TclNewObj(nameObj);
    }
    if (objc > 2) {
	int length;
	Tcl_Obj *templateObj = objv[2];
	const char *string = TclGetStringFromObj(templateObj, &length);

	/*
	 * Treat an empty string as if it wasn't there.
	 */

	if (length == 0) {
	    goto makeTemporary;
	}

	/*
	 * The template only gives a directory if there is a directory
	 * separator in it.
	 */

	if (strchr(string, '/') != NULL
		|| (tclPlatform == TCL_PLATFORM_WINDOWS
		    && strchr(string, '\\') != NULL)) {
	    tempDirObj = TclPathPart(interp, templateObj, TCL_PATH_DIRNAME);

	    /*
	     * Only allow creation of temporary files in the native filesystem
	     * since they are frequently used for integration with external
	     * tools or system libraries. [Bug 2388866]
	     */

	    if (tempDirObj != NULL && Tcl_FSGetFileSystemForPath(tempDirObj)
		    != &tclNativeFilesystem) {
		TclDecrRefCount(tempDirObj);
		tempDirObj = NULL;
	    }
	}

	/*
	 * The template only gives the filename if the last character isn't a
	 * directory separator.
	 */

	if (string[length-1] != '/' && (tclPlatform != TCL_PLATFORM_WINDOWS
		|| string[length-1] != '\\')) {
	    Tcl_Obj *tailObj = TclPathPart(interp, templateObj,
		    TCL_PATH_TAIL);

	    if (tailObj != NULL) {
		tempBaseObj = TclPathPart(interp, tailObj, TCL_PATH_ROOT);
		tempExtObj = TclPathPart(interp, tailObj, TCL_PATH_EXTENSION);
		TclDecrRefCount(tailObj);
	    }
	}
    }

    /*
     * Convert empty parts of the template into unspecified parts.
     */

    if (tempDirObj && !TclGetString(tempDirObj)[0]) {
	TclDecrRefCount(tempDirObj);
	tempDirObj = NULL;
    }
    if (tempBaseObj && !TclGetString(tempBaseObj)[0]) {
	TclDecrRefCount(tempBaseObj);
	tempBaseObj = NULL;
    }
    if (tempExtObj && !TclGetString(tempExtObj)[0]) {
	TclDecrRefCount(tempExtObj);
	tempExtObj = NULL;
    }

    /*
     * Create and open the temporary file.
     */

  makeTemporary:
    chan = TclpOpenTemporaryFile(tempDirObj,tempBaseObj,tempExtObj, nameObj);

    /*
     * If we created pieces of template, get rid of them now.
     */

    if (tempDirObj) {
	TclDecrRefCount(tempDirObj);
    }
    if (tempBaseObj) {
	TclDecrRefCount(tempBaseObj);
    }
    if (tempExtObj) {
	TclDecrRefCount(tempExtObj);
    }

    /*
     * Deal with results.
     */

    if (chan == NULL) {
	if (nameVarObj) {
	    TclDecrRefCount(nameObj);
	}
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't create temporary file: %s", Tcl_PosixError(interp)));
	return TCL_ERROR;
    }
    Tcl_RegisterChannel(interp, chan);
    if (nameVarObj != NULL) {
	if (Tcl_ObjSetVar2(interp, nameVarObj, NULL, nameObj,
		TCL_LEAVE_ERR_MSG) == NULL) {
	    Tcl_UnregisterChannel(interp, chan);
	    return TCL_ERROR;
	}
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(Tcl_GetChannelName(chan), -1));
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
