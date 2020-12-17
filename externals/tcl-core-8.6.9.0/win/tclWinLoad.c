/*
 * tclWinLoad.c --
 *
 *	This function provides a version of the TclLoadFile that works with
 *	the Windows "LoadLibrary" and "GetProcAddress" API for dynamic
 *	loading.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclWinInt.h"

/*
 * Native name of the directory in the native filesystem where DLLs used in
 * this process are copied prior to loading, and mutex used to protect its
 * allocation.
 */

static WCHAR *dllDirectoryName = NULL;
static Tcl_Mutex dllDirectoryNameMutex;

/*
 * Static functions defined within this file.
 */

static void *		FindSymbol(Tcl_Interp *interp,
			    Tcl_LoadHandle loadHandle, const char *symbol);
static int		InitDLLDirectoryName(void);
static void		UnloadFile(Tcl_LoadHandle loadHandle);

/*
 *----------------------------------------------------------------------
 *
 * TclpDlopen --
 *
 *	Dynamically loads a binary code file into memory and returns a handle
 *	to the new code.
 *
 * Results:
 *	A standard Tcl completion code. If an error occurs, an error message
 *	is left in the interp's result.
 *
 * Side effects:
 *	New code suddenly appears in memory.
 *
 *----------------------------------------------------------------------
 */

int
TclpDlopen(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Obj *pathPtr,		/* Name of the file containing the desired
				 * code (UTF-8). */
    Tcl_LoadHandle *loadHandle,	/* Filled with token for dynamically loaded
				 * file which will be passed back to
				 * (*unloadProcPtr)() to unload the file. */
    Tcl_FSUnloadFileProc **unloadProcPtr,
				/* Filled with address of Tcl_FSUnloadFileProc
				 * function which should be used for this
				 * file. */
    int flags)
{
    HINSTANCE hInstance = NULL;
    const TCHAR *nativeName;
    Tcl_LoadHandle handlePtr;
    DWORD firstError;

    /*
     * First try the full path the user gave us. This is particularly
     * important if the cwd is inside a vfs, and we are trying to load using a
     * relative path.
     */

    nativeName = Tcl_FSGetNativePath(pathPtr);
    if (nativeName != NULL) {
	hInstance = LoadLibraryEx(nativeName, NULL,
		LOAD_WITH_ALTERED_SEARCH_PATH);
    }
    if (hInstance == NULL) {
	/*
	 * Let the OS loader examine the binary search path for whatever
	 * string the user gave us which hopefully refers to a file on the
	 * binary path.
	 */

	Tcl_DString ds;

        /* 
         * Remember the first error on load attempt to be used if the
         * second load attempt below also fails.
        */
        firstError = (nativeName == NULL) ?
		ERROR_MOD_NOT_FOUND : GetLastError();

	nativeName = Tcl_WinUtfToTChar(Tcl_GetString(pathPtr), -1, &ds);
	hInstance = LoadLibraryEx(nativeName, NULL,
		LOAD_WITH_ALTERED_SEARCH_PATH);
	Tcl_DStringFree(&ds);
    }

    if (hInstance == NULL) {
	DWORD lastError;
        Tcl_Obj *errMsg;

        /*
         * We choose to only use the error from the second call if the first
         * call failed due to the file not being found. Else stick to the
         * first error for reporting purposes.
         */
        if (firstError == ERROR_MOD_NOT_FOUND ||
            firstError == ERROR_DLL_NOT_FOUND)
            lastError = GetLastError();
        else
            lastError = firstError;

	errMsg = Tcl_ObjPrintf("couldn't load library \"%s\": ",
		Tcl_GetString(pathPtr));

	/*
	 * Check for possible DLL errors. This doesn't work quite right,
	 * because Windows seems to only return ERROR_MOD_NOT_FOUND for just
	 * about any problem, but it's better than nothing. It'd be even
	 * better if there was a way to get what DLLs
	 */

	if (interp) {
	    switch (lastError) {
	    case ERROR_MOD_NOT_FOUND:
		Tcl_SetErrorCode(interp, "WIN_LOAD", "MOD_NOT_FOUND", NULL);
		goto notFoundMsg;
	    case ERROR_DLL_NOT_FOUND:
		Tcl_SetErrorCode(interp, "WIN_LOAD", "DLL_NOT_FOUND", NULL);
	    notFoundMsg:
		Tcl_AppendToObj(errMsg, "this library or a dependent library"
			" could not be found in library path", -1);
		break;
	    case ERROR_PROC_NOT_FOUND:
		Tcl_SetErrorCode(interp, "WIN_LOAD", "PROC_NOT_FOUND", NULL);
		Tcl_AppendToObj(errMsg, "A function specified in the import"
			" table could not be resolved by the system. Windows"
			" is not telling which one, I'm sorry.", -1);
		break;
	    case ERROR_INVALID_DLL:
		Tcl_SetErrorCode(interp, "WIN_LOAD", "INVALID_DLL", NULL);
		Tcl_AppendToObj(errMsg, "this library or a dependent library"
			" is damaged", -1);
		break;
	    case ERROR_DLL_INIT_FAILED:
		Tcl_SetErrorCode(interp, "WIN_LOAD", "DLL_INIT_FAILED", NULL);
		Tcl_AppendToObj(errMsg, "the library initialization"
			" routine failed", -1);
		break;
            case ERROR_BAD_EXE_FORMAT:
		Tcl_SetErrorCode(interp, "WIN_LOAD", "BAD_EXE_FORMAT", NULL);
		Tcl_AppendToObj(errMsg, "Bad exe format. Possibly a 32/64-bit mismatch.", -1);
                break;
            default:
		TclWinConvertError(lastError);
		Tcl_AppendToObj(errMsg, Tcl_PosixError(interp), -1);
	    }
	    Tcl_SetObjResult(interp, errMsg);
	}
	return TCL_ERROR;
    }

    /*
     * Succeded; package everything up for Tcl.
     */

    handlePtr = ckalloc(sizeof(struct Tcl_LoadHandle_));
    handlePtr->clientData = (ClientData) hInstance;
    handlePtr->findSymbolProcPtr = &FindSymbol;
    handlePtr->unloadFileProcPtr = &UnloadFile;
    *loadHandle = handlePtr;
    *unloadProcPtr = &UnloadFile;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FindSymbol --
 *
 *	Looks up a symbol, by name, through a handle associated with a
 *	previously loaded piece of code (shared library).
 *
 * Results:
 *	Returns a pointer to the function associated with 'symbol' if it is
 *	found. Otherwise returns NULL and may leave an error message in the
 *	interp's result.
 *
 *----------------------------------------------------------------------
 */

static void *
FindSymbol(
    Tcl_Interp *interp,
    Tcl_LoadHandle loadHandle,
    const char *symbol)
{
    HINSTANCE hInstance = (HINSTANCE) loadHandle->clientData;
    Tcl_PackageInitProc *proc = NULL;

    /*
     * For each symbol, check for both Symbol and _Symbol, since Borland
     * generates C symbols with a leading '_' by default.
     */

    proc = (void *) GetProcAddress(hInstance, symbol);
    if (proc == NULL) {
	Tcl_DString ds;
	const char *sym2;

	Tcl_DStringInit(&ds);
	TclDStringAppendLiteral(&ds, "_");
	sym2 = Tcl_DStringAppend(&ds, symbol, -1);
	proc = (Tcl_PackageInitProc *) GetProcAddress(hInstance, sym2);
	Tcl_DStringFree(&ds);
    }
    if (proc == NULL && interp != NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"cannot find symbol \"%s\"", symbol));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "LOAD_SYMBOL", symbol, NULL);
    }
    return proc;
}

/*
 *----------------------------------------------------------------------
 *
 * UnloadFile --
 *
 *	Unloads a dynamically loaded binary code file from memory. Code
 *	pointers in the formerly loaded file are no longer valid after calling
 *	this function.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Code removed from memory.
 *
 *----------------------------------------------------------------------
 */

static void
UnloadFile(
    Tcl_LoadHandle loadHandle)	/* loadHandle returned by a previous call to
				 * TclpDlopen(). The loadHandle is a token
				 * that represents the loaded file. */
{
    HINSTANCE hInstance = (HINSTANCE) loadHandle->clientData;

    FreeLibrary(hInstance);
    ckfree(loadHandle);
}

/*
 *----------------------------------------------------------------------
 *
 * TclGuessPackageName --
 *
 *	If the "load" command is invoked without providing a package name,
 *	this function is invoked to try to figure it out.
 *
 * Results:
 *	Always returns 0 to indicate that we couldn't figure out a package
 *	name; generic code will then try to guess the package from the file
 *	name. A return value of 1 would have meant that we figured out the
 *	package name and put it in bufPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclGuessPackageName(
    const char *fileName,	/* Name of file containing package (already
				 * translated to local form if needed). */
    Tcl_DString *bufPtr)	/* Initialized empty dstring. Append package
				 * name to this if possible. */
{
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpTempFileNameForLibrary --
 *
 *	Constructs a temporary file name for loading a shared object (DLL).
 *
 * Results:
 *	Returns the constructed file name.
 *
 * On Windows, a DLL is identified by the final component of its path name.
 * Cross linking among DLL's (and hence, preloading) will not work unless this
 * name is preserved when copying a DLL from a VFS to a temp file for
 * preloading. For this reason, all DLLs in a given process are copied to a
 * temp directory, and their names are preserved.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclpTempFileNameForLibrary(
    Tcl_Interp *interp,		/* Tcl interpreter. */
    Tcl_Obj *path)		/* Path name of the DLL in the VFS. */
{
    Tcl_Obj *fileName;		/* Name of the temp file. */
    Tcl_Obj *tail;		/* Tail of the source path. */

    Tcl_MutexLock(&dllDirectoryNameMutex);
    if (dllDirectoryName == NULL) {
	if (InitDLLDirectoryName() == TCL_ERROR) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "couldn't create temporary directory: %s",
		    Tcl_PosixError(interp)));
	    Tcl_MutexUnlock(&dllDirectoryNameMutex);
	    return NULL;
	}
    }
    Tcl_MutexUnlock(&dllDirectoryNameMutex);

    /*
     * Now we know where to put temporary DLLs, construct the name.
     */

    fileName = TclpNativeToNormalized(dllDirectoryName);
    tail = TclPathPart(interp, path, TCL_PATH_TAIL);
    if (tail == NULL) {
	Tcl_DecrRefCount(fileName);
	return NULL;
    }
    Tcl_AppendToObj(fileName, "/", 1);
    Tcl_AppendObjToObj(fileName, tail);
    return fileName;
}

/*
 *----------------------------------------------------------------------
 *
 * InitDLLDirectoryName --
 *
 *	Helper for TclpTempFileNameForLibrary; builds a temporary directory
 *	that is specific to the current process. Should only be called once
 *	per process start. Caller must hold dllDirectoryNameMutex.
 *
 * Results:
 *	Tcl result code.
 *
 * Side-effects:
 *	Creates temp directory.
 *	Allocates memory pointed to by dllDirectoryName.
 *
 *----------------------------------------------------------------------
 * [Candidate for process global?]
 */

static int
InitDLLDirectoryName(void)
{
    size_t nameLen;		/* Length of the temp folder name. */
    WCHAR name[MAX_PATH];	/* Path name of the temp folder. */
    DWORD id;			/* The process id. */
    DWORD lastError;		/* Last error to happen in Win API. */
    int i;

    /*
     * Determine the name of the directory to use, and create it.  (Keep
     * trying with new names until an attempt to create the directory
     * succeeds)
     */

    nameLen = GetTempPathW(MAX_PATH, name);
    if (nameLen >= MAX_PATH-12) {
	Tcl_SetErrno(ENAMETOOLONG);
	return TCL_ERROR;
    }

    wcscpy(name+nameLen, L"TCLXXXXXXXX");
    nameLen += 11;

    id = GetCurrentProcessId();
    lastError = ERROR_ALREADY_EXISTS;

    for (i=0 ; i<256 ; i++) {
	wsprintfW(name+nameLen-8, L"%08x", id);
	if (CreateDirectoryW(name, NULL)) {
	    /*
	     * Issue: we don't schedule this directory for deletion by anyone.
	     * Can we ask the OS to do this for us?  There appears to be
	     * potential for using CreateFile (with the flag
	     * FILE_FLAG_BACKUP_SEMANTICS) and RemoveDirectory to do this...
	     */

	    goto copyToGlobalBuffer;
	}
	lastError = GetLastError();
	if (lastError != ERROR_ALREADY_EXISTS) {
	    break;
	}
	id *= 16777619;
    }

    TclWinConvertError(lastError);
    return TCL_ERROR;

    /*
     * Store our computed value in the global.
     */

  copyToGlobalBuffer:
    dllDirectoryName = ckalloc((nameLen+1) * sizeof(WCHAR));
    wcscpy(dllDirectoryName, name);
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
