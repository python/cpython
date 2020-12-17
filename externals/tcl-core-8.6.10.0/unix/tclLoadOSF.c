/*
 * tclLoadOSF.c --
 *
 *	This function provides a version of the TclLoadFile that works under
 *	OSF/1 1.0/1.1/1.2 and related systems, utilizing the old OSF/1
 *	/sbin/loader and /usr/include/loader.h. OSF/1 versions from 1.3 and on
 *	use ELF, rtld, and dlopen()[/usr/include/ldfcn.h].
 *
 *	This is useful for:
 *		OSF/1 1.0, 1.1, 1.2 (from OSF)
 *			includes: MK4 and AD1 (from OSF RI)
 *		OSF/1 1.3 (from OSF) using ROSE
 *		HP OSF/1 1.0 ("Acorn") using COFF
 *
 *	This is likely to be useful for:
 *		Paragon OSF/1 (from Intel)
 *		HI-OSF/1 (from Hitachi)
 *
 *	This is NOT to be used on:
 *		Digitial Alpha OSF/1 systems
 *		OSF/1 1.3 or later (from OSF) using ELF
 *			includes: MK6, MK7, AD2, AD3 (from OSF RI)
 *
 *	This approach to things was utter @&^#; thankfully, OSF/1 eventually
 *	supported dlopen().
 *
 *	John Robert LoVerso <loverso@freebsd.osf.org>
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include <sys/types.h>
#include <loader.h>

/*
 * Static functions defined within this file.
 */

static void *		FindSymbol(Tcl_Interp *interp,
			    Tcl_LoadHandle loadHandle, const char* symbol);
static void		UnloadFile(Tcl_LoadHandle handle);

/*
 *----------------------------------------------------------------------
 *
 * TclpDlopen --
 *
 *	Dynamically loads a binary code file into memory and returns a handle
 *	to the new code.
 *
 * Results:
 *	A standard Tcl completion code.  If an error occurs, an error message
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
    Tcl_LoadHandle newHandle;
    ldr_module_t lm;
    char *pkg;
    char *fileName = Tcl_GetString(pathPtr);
    const char *native;

    /*
     * First try the full path the user gave us.  This is particularly
     * important if the cwd is inside a vfs, and we are trying to load using a
     * relative path.
     */

    native = Tcl_FSGetNativePath(pathPtr);
    lm = (Tcl_PackageInitProc *) load(native, LDR_NOFLAGS);

    if (lm == LDR_NULL_MODULE) {
	/*
	 * Let the OS loader examine the binary search path for whatever
	 * string the user gave us which hopefully refers to a file on the
	 * binary path
	 */

	Tcl_DString ds;

	native = Tcl_UtfToExternalDString(NULL, fileName, -1, &ds);
	lm = (Tcl_PackageInitProc *) load(native, LDR_NOFLAGS);
	Tcl_DStringFree(&ds);
    }

    if (lm == LDR_NULL_MODULE) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"couldn't load file \"%s\": %s",
		fileName, Tcl_PosixError(interp)));
	return TCL_ERROR;
    }

    *clientDataPtr = NULL;

    /*
     * My convention is to use a [OSF loader] package name the same as shlib,
     * since the idiots never implemented ldr_lookup() and it is otherwise
     * impossible to get a package name given a module.
     *
     * I build loadable modules with a makefile rule like
     *		ld ... -export $@: -o $@ $(OBJS)
     */

    if ((pkg = strrchr(fileName, '/')) == NULL) {
        pkg = fileName;
    } else {
	pkg++;
    }
    newHandle = ckalloc(sizeof(*newHandle));
    newHandle->clientData = pkg;
    newHandle->findSymbolProcPtr = &FindSymbol;
    newHandle->unloadFileProcPtr = &UnloadFile;
    *loadHandle = newHandle;
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
 *	found.  Otherwise returns NULL and may leave an error message in the
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
    void *retval = ldr_lookup_package((char *) loadHandle, symbol);

    if (retval == NULL && interp != NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"cannot find symbol \"%s\"", symbol));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "LOAD_SYMBOL", symbol, NULL);
    }
    return retval;
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
 *	Does nothing.  Can anything be done?
 *
 *----------------------------------------------------------------------
 */

static void
UnloadFile(
    Tcl_LoadHandle loadHandle)	/* loadHandle returned by a previous call to
				 * TclpDlopen(). The loadHandle is a token
				 * that represents the loaded file. */
{
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
 *	name.  A return value of 1 would have meant that we figured out the
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
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
