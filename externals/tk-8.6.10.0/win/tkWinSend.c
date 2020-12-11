/*
 * tkWinSend.c --
 *
 *	This file provides functions that implement the "send" command,
 *	allowing commands to be passed from interpreter to interpreter.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 * Copyright (c) 2003 Pat Thoyts <patthoyts@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkWinSendCom.h"

/*
 * Should be defined in WTypes.h but mingw 1.0 is missing them.
 */

#ifndef _ROTFLAGS_DEFINED
#define _ROTFLAGS_DEFINED
#define ROTFLAGS_REGISTRATIONKEEPSALIVE 0x01
#define ROTFLAGS_ALLOWANYCLIENT		0x02
#endif /* ! _ROTFLAGS_DEFINED */

#define TKWINSEND_CLASS_NAME		"TclEval"
#define TKWINSEND_REGISTRATION_BASE	L"TclEval"

#define MK_E_MONIKERALREADYREGISTERED \
	MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x02A1)

/*
 * Package information structure. This is used to keep interpreter specific
 * details for use when releasing the package resources upon interpreter
 * deletion or package removal.
 */

typedef struct {
    char *name;			/* The registered application name */
    DWORD cookie;		/* ROT cookie returned on registration */
    LPUNKNOWN obj;		/* Interface for the registration object */
    Tcl_Interp *interp;
    Tcl_Command token;		/* Winsend command token */
} RegisteredInterp;

typedef struct SendEvent {
    Tcl_Event header;
    Tcl_Interp *interp;
    Tcl_Obj *cmdPtr;
} SendEvent;

#ifdef TK_SEND_ENABLED_ON_WINDOWS
typedef struct {
    int initialized;
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;
#endif /* TK_SEND_ENABLED_ON_WINDOWS */

/*
 * Functions internal to this file.
 */

#ifdef TK_SEND_ENABLED_ON_WINDOWS
static void		CmdDeleteProc(ClientData clientData);
static void		InterpDeleteProc(ClientData clientData,
			    Tcl_Interp *interp);
static void		RevokeObjectRegistration(RegisteredInterp *riPtr);
#endif /* TK_SEND_ENABLED_ON_WINDOWS */
static HRESULT		BuildMoniker(const char *name, LPMONIKER *pmk);
#ifdef TK_SEND_ENABLED_ON_WINDOWS
static HRESULT		RegisterInterp(const char *name,
			    RegisteredInterp *riPtr);
#endif /* TK_SEND_ENABLED_ON_WINDOWS */
static int		FindInterpreterObject(Tcl_Interp *interp,
			    const char *name, LPDISPATCH *ppdisp);
static int		Send(LPDISPATCH pdispInterp, Tcl_Interp *interp,
			    int async, ClientData clientData, int objc,
			    Tcl_Obj *const objv[]);
static void		SendTrace(const char *format, ...);
static Tcl_EventProc	SendEventProc;

#if defined(DEBUG) || defined(_DEBUG)
#define TRACE SendTrace
#else
#define TRACE 1 ? ((void)0) : SendTrace
#endif /* DEBUG || _DEBUG */

/*
 *--------------------------------------------------------------
 *
 * Tk_SetAppName --
 *
 *	This function is called to associate an ASCII name with a Tk
 *	application. If the application has already been named, the name
 *	replaces the old one.
 *
 * Results:
 *	The return value is the name actually given to the application. This
 *	will normally be the same as name, but if name was already in use for
 *	an application then a name of the form "name #2" will be chosen, with
 *	a high enough number to make the name unique.
 *
 * Side effects:
 *	Registration info is saved, thereby allowing the "send" command to be
 *	used later to invoke commands in the application. In addition, the
 *	"send" command is created in the application's interpreter. The
 *	registration will be removed automatically if the interpreter is
 *	deleted or the "send" command is removed.
 *
 *--------------------------------------------------------------
 */

const char *
Tk_SetAppName(
    Tk_Window tkwin,		/* Token for any window in the application to
				 * be named: it is just used to identify the
				 * application and the display.  */
    const char *name)		/* The name that will be used to refer to the
				 * interpreter in later "send" commands. Must
				 * be globally unique. */
{
#ifndef TK_SEND_ENABLED_ON_WINDOWS
    /*
     * Temporarily disabled for bug #858822
     */

    return name;
#else /* TK_SEND_ENABLED_ON_WINDOWS */

    ThreadSpecificData *tsdPtr = NULL;
    TkWindow *winPtr = (TkWindow *) tkwin;
    RegisteredInterp *riPtr = NULL;
    Tcl_Interp *interp;
    HRESULT hr = S_OK;

    interp = winPtr->mainPtr->interp;
    tsdPtr = Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * Initialise the COM library for this interpreter just once.
     */

    if (tsdPtr->initialized == 0) {
	hr = CoInitialize(0);
	if (FAILED(hr)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "failed to initialize the COM library", -1));
	    Tcl_SetErrorCode(interp, "TK", "SEND", "COM", NULL);
	    return "";
	}
	tsdPtr->initialized = 1;
	TRACE("Initialized COM library for interp 0x%" TCL_Z_MODIFIER "x\n", (size_t)interp);
    }

    /*
     * If the interp hasn't been registered before then we need to create the
     * registration structure and the COM object. If it has been registered
     * already then we can reuse all and just register the new name.
     */

    riPtr = Tcl_GetAssocData(interp, "tkWinSend::ri", NULL);
    if (riPtr == NULL) {
	LPUNKNOWN *objPtr;

	riPtr = ckalloc(sizeof(RegisteredInterp));
	memset(riPtr, 0, sizeof(RegisteredInterp));
	riPtr->interp = interp;

	objPtr = &riPtr->obj;
	hr = TkWinSendCom_CreateInstance(interp, &IID_IUnknown,
		(void **) objPtr);

	Tcl_CreateObjCommand(interp, "send", Tk_SendObjCmd, riPtr,
		CmdDeleteProc);
	if (Tcl_IsSafe(interp)) {
	    Tcl_HideCommand(interp, "send", "send");
	}
	Tcl_SetAssocData(interp, "tkWinSend::ri", NULL, riPtr);
    } else {
	RevokeObjectRegistration(riPtr);
    }

    RegisterInterp(name, riPtr);
    return (const char *) riPtr->name;
#endif /* TK_SEND_ENABLED_ON_WINDOWS */
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetInterpNames --
 *
 *	This function is invoked to fetch a list of all the interpreter names
 *	currently registered for the display of a particular window.
 *
 * Results:
 *	A standard Tcl return value. Interp->result will be set to hold a list
 *	of all the interpreter names defined for tkwin's display. If an error
 *	occurs, then TCL_ERROR is returned and interp->result will hold an
 *	error message.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkGetInterpNames(
    Tcl_Interp *interp,		/* Interpreter for returning a result. */
    Tk_Window tkwin)		/* Window whose display is to be used for the
				 * lookup. */
{
#ifndef TK_SEND_ENABLED_ON_WINDOWS
    /*
     * Temporarily disabled for bug #858822
     */

    return TCL_OK;
#else /* TK_SEND_ENABLED_ON_WINDOWS */

    LPRUNNINGOBJECTTABLE pROT = NULL;
    LPCOLESTR oleszStub = TKWINSEND_REGISTRATION_BASE;
    HRESULT hr = S_OK;
    Tcl_Obj *objList = NULL;
    int result = TCL_OK;

    hr = GetRunningObjectTable(0, &pROT);
    if (SUCCEEDED(hr)) {
	IBindCtx* pBindCtx = NULL;
	objList = Tcl_NewListObj(0, NULL);
	hr = CreateBindCtx(0, &pBindCtx);

	if (SUCCEEDED(hr)) {
	    IEnumMoniker* pEnum;

	    hr = pROT->lpVtbl->EnumRunning(pROT, &pEnum);
	    if (SUCCEEDED(hr)) {
		IMoniker* pmk = NULL;

		while (pEnum->lpVtbl->Next(pEnum, 1, &pmk, NULL) == S_OK) {
		    LPOLESTR olestr;

		    hr = pmk->lpVtbl->GetDisplayName(pmk, pBindCtx, NULL,
			    &olestr);
		    if (SUCCEEDED(hr)) {
			IMalloc *pMalloc = NULL;

			if (wcsncmp(olestr, oleszStub,
				wcslen(oleszStub)) == 0) {
			    LPOLESTR p = olestr + wcslen(oleszStub);

			    if (*p) {
				Tcl_DString ds;

				Tcl_WinTCharToUtf((LPCTSTR)(p + 1), -1, &ds);
				result = Tcl_ListObjAppendElement(interp,
					objList,
					Tcl_NewStringObj(Tcl_DStringValue(&ds),
						Tcl_DStringLength(&ds)));
				Tcl_DStringFree(&ds);
			    }
			}

			hr = CoGetMalloc(1, &pMalloc);
			if (SUCCEEDED(hr)) {
			    pMalloc->lpVtbl->Free(pMalloc, (void*)olestr);
			    pMalloc->lpVtbl->Release(pMalloc);
			}
		    }
		    pmk->lpVtbl->Release(pmk);
		}
		pEnum->lpVtbl->Release(pEnum);
	    }
	    pBindCtx->lpVtbl->Release(pBindCtx);
	}
	pROT->lpVtbl->Release(pROT);
    }

    if (FAILED(hr)) {
	/*
	 * Expire the list if set.
	 */

	if (objList != NULL) {
	    Tcl_DecrRefCount(objList);
	}
	Tcl_SetObjResult(interp, TkWin32ErrorObj(hr));
	result = TCL_ERROR;
    }

    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, objList);
    }

    return result;
#endif /* TK_SEND_ENABLED_ON_WINDOWS */
}

/*
 *--------------------------------------------------------------
 *
 * Tk_SendCmd --
 *
 *	This function is invoked to process the "send" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
Tk_SendObjCmd(
    ClientData clientData,	/* Information about sender (only dispPtr
				 * field is used). */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument strings. */
{
    enum {
	SEND_ASYNC, SEND_DISPLAYOF, SEND_LAST
    };
    static const char *const sendOptions[] = {
	"-async",   "-displayof",   "--",  NULL
    };
    int result = TCL_OK;
    int i, optind, async = 0;
    Tcl_Obj *displayPtr = NULL;

    /*
     * Process the command options.
     */

    for (i = 1; i < objc; i++) {
	if (Tcl_GetIndexFromObjStruct(interp, objv[i], sendOptions,
		sizeof(char *), "option", 0, &optind) != TCL_OK) {
	    break;
	}
	if (optind == SEND_ASYNC) {
	    ++async;
	} else if (optind == SEND_DISPLAYOF) {
	    displayPtr = objv[++i];
	} else if (optind == SEND_LAST) {
	    i++;
	    break;
	}
    }

    /*
     * Ensure we still have a valid command.
     */

    if ((objc - i) < 2) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-async? ?-displayof? ?--? interpName arg ?arg ...?");
	result = TCL_ERROR;
    }

    /*
     * We don't support displayPtr. See TIP #150.
     */

    if (displayPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"option not implemented: \"displayof\" is not available"
		" for this platform.", -1));
	Tcl_SetErrorCode(interp, "TK", "SEND", "DISPLAYOF_WIN", NULL);
	result = TCL_ERROR;
    }

    /*
     * Send the arguments to the foreign interp.
     */
    /* FIX ME: we need to check for local interp */
    if (result == TCL_OK) {
	LPDISPATCH pdisp;

	result = FindInterpreterObject(interp, Tcl_GetString(objv[i]), &pdisp);
	if (result == TCL_OK) {
	    i++;
	    result = Send(pdisp, interp, async, clientData, objc-i, objv+i);
	    pdisp->lpVtbl->Release(pdisp);
	}
    }

    return result;
}

/*
 *--------------------------------------------------------------
 *
 * FindInterpreterObject --
 *
 *	Search the set of objects currently registered with the Running Object
 *	Table for one which matches the registered name. Tk objects are named
 *	using BuildMoniker by always prefixing with TclEval.
 *
 * Results:
 *	If a matching object registration is found, then the registered
 *	IDispatch interface pointer is returned. If not, then an error message
 *	is placed in the interpreter and TCL_ERROR is returned.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
FindInterpreterObject(
    Tcl_Interp *interp,
    const char *name,
    LPDISPATCH *ppdisp)
{
    LPRUNNINGOBJECTTABLE pROT = NULL;
    int result = TCL_OK;
    HRESULT hr = GetRunningObjectTable(0, &pROT);

    if (SUCCEEDED(hr)) {
	IBindCtx* pBindCtx = NULL;

	hr = CreateBindCtx(0, &pBindCtx);
	if (SUCCEEDED(hr)) {
	    LPMONIKER pmk = NULL;

	    hr = BuildMoniker(name, &pmk);
	    if (SUCCEEDED(hr)) {
		IUnknown *pUnkInterp = NULL, **ppUnkInterp = &pUnkInterp;

		hr = pROT->lpVtbl->IsRunning(pROT, pmk);
		hr = pmk->lpVtbl->BindToObject(pmk, pBindCtx, NULL,
			&IID_IUnknown, (void **) ppUnkInterp);
		if (SUCCEEDED(hr)) {
		    hr = pUnkInterp->lpVtbl->QueryInterface(pUnkInterp,
			    &IID_IDispatch, (void **) ppdisp);
		    pUnkInterp->lpVtbl->Release(pUnkInterp);

		} else {
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "no application named \"%s\"", name));
		    Tcl_SetErrorCode(interp, "TK", "LOOKUP", "APPLICATION",
			    NULL);
		    result = TCL_ERROR;
		}

		pmk->lpVtbl->Release(pmk);
	    }
	    pBindCtx->lpVtbl->Release(pBindCtx);
	}
	pROT->lpVtbl->Release(pROT);
    }
    if (FAILED(hr) && result == TCL_OK) {
	Tcl_SetObjResult(interp, TkWin32ErrorObj(hr));
	result = TCL_ERROR;
    }
    return result;
}

/*
 *--------------------------------------------------------------
 *
 * CmdDeleteProc --
 *
 *	This function is invoked by Tcl when the "send" command is deleted in
 *	an interpreter. It unregisters the interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The interpreter given by riPtr is unregistered, the registration
 *	structure is free'd and the COM object unregistered and released.
 *
 *--------------------------------------------------------------
 */

#ifdef TK_SEND_ENABLED_ON_WINDOWS
static void
CmdDeleteProc(
    ClientData clientData)
{
    RegisteredInterp *riPtr = (RegisteredInterp *)clientData;

    /*
     * Lock the package structure in memory.
     */

    Tcl_Preserve(clientData);

    /*
     * Revoke the ROT registration.
     */

    RevokeObjectRegistration(riPtr);

    /*
     * Release the registration object.
     */

    riPtr->obj->lpVtbl->Release(riPtr->obj);
    riPtr->obj = NULL;

    Tcl_DeleteAssocData(riPtr->interp, "tkWinSend::ri");

    /*
     * Unlock the package data structure.
     */

    Tcl_Release(clientData);

    ckfree(clientData);
}

/*
 *--------------------------------------------------------------
 *
 * RevokeObjectRegistration --
 *
 *	Releases the interpreters registration object from the Running Object
 *	Table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The stored cookie value is zeroed and the name is free'd and the
 *	pointer set to NULL.
 *
 *--------------------------------------------------------------
 */

static void
RevokeObjectRegistration(
    RegisteredInterp *riPtr)
{
    LPRUNNINGOBJECTTABLE pROT = NULL;
    HRESULT hr = S_OK;

    if (riPtr->cookie != 0) {
	hr = GetRunningObjectTable(0, &pROT);
	if (SUCCEEDED(hr)) {
	    hr = pROT->lpVtbl->Revoke(pROT, riPtr->cookie);
	    pROT->lpVtbl->Release(pROT);
	    riPtr->cookie = 0;
	}
    }

    /*
     * Release the name storage.
     */

    if (riPtr->name != NULL) {
	free(riPtr->name);
	riPtr->name = NULL;
    }
}
#endif /* TK_SEND_ENABLED_ON_WINDOWS */

/*
 * ----------------------------------------------------------------------
 *
 * InterpDeleteProc --
 *
 *	This is called when the interpreter is deleted and used to unregister
 *	the COM libraries.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */

#ifdef TK_SEND_ENABLED_ON_WINDOWS
static void
InterpDeleteProc(
    ClientData clientData,
    Tcl_Interp *interp)
{
    CoUninitialize();
}
#endif /* TK_SEND_ENABLED_ON_WINDOWS */

/*
 * ----------------------------------------------------------------------
 *
 * BuildMoniker --
 *
 *	Construct a moniker from the given name. This ensures that all our
 *	monikers have the same prefix.
 *
 * Results:
 *	S_OK. If the name cannot be turned into a moniker then a COM error
 *	code is returned.
 *
 * Side effects:
 *	The moniker created is stored at the address given by ppmk.
 *
 * ----------------------------------------------------------------------
 */

static HRESULT
BuildMoniker(
    const char *name,
    LPMONIKER *ppmk)
{
    LPMONIKER pmkClass = NULL;
    HRESULT hr = CreateFileMoniker(TKWINSEND_REGISTRATION_BASE, &pmkClass);

    if (SUCCEEDED(hr)) {
	LPMONIKER pmkItem = NULL;
	Tcl_DString dString;

	Tcl_WinUtfToTChar(name, -1, &dString);
	hr = CreateFileMoniker((LPOLESTR)Tcl_DStringValue(&dString), &pmkItem);
	Tcl_DStringFree(&dString);
	if (SUCCEEDED(hr)) {
	    hr = pmkClass->lpVtbl->ComposeWith(pmkClass, pmkItem, FALSE, ppmk);
	    pmkItem->lpVtbl->Release(pmkItem);
	}
	pmkClass->lpVtbl->Release(pmkClass);
    }
    return hr;
}

/*
 * ----------------------------------------------------------------------
 *
 * RegisterInterp --
 *
 *	Attempts to register the provided name for this interpreter. If the
 *	given name is already in use, then a numeric suffix is appended as
 *	" #n" until we identify a unique name.
 *
 * Results:
 *	Returns S_OK if successful, else a COM error code.
 *
 * Side effects:
 *	Registration returns a cookie value which is stored. We also store a
 *	copy of the name.
 *
 * ----------------------------------------------------------------------
 */

#ifdef TK_SEND_ENABLED_ON_WINDOWS
static HRESULT
RegisterInterp(
    const char *name,
    RegisteredInterp *riPtr)
{
    HRESULT hr = S_OK;
    LPRUNNINGOBJECTTABLE pROT = NULL;
    LPMONIKER pmk = NULL;
    int i, offset;
    const char *actualName = name;
    Tcl_DString dString;
    Tcl_DStringInit(&dString);

    hr = GetRunningObjectTable(0, &pROT);
    if (SUCCEEDED(hr)) {
	offset = 0;
	for (i = 1; SUCCEEDED(hr); i++) {
	    if (i > 1) {
		if (i == 2) {
		    Tcl_DStringInit(&dString);
		    Tcl_DStringAppend(&dString, name, -1);
		    Tcl_DStringAppend(&dString, " #", 2);
		    offset = Tcl_DStringLength(&dString);
		    Tcl_DStringSetLength(&dString, offset+TCL_INTEGER_SPACE);
		    actualName = Tcl_DStringValue(&dString);
		}
		sprintf(Tcl_DStringValue(&dString) + offset, "%d", i);
	    }

	    hr = BuildMoniker(actualName, &pmk);
	    if (SUCCEEDED(hr)) {

		hr = pROT->lpVtbl->Register(pROT,
		    ROTFLAGS_REGISTRATIONKEEPSALIVE,
		    riPtr->obj, pmk, &riPtr->cookie);

		pmk->lpVtbl->Release(pmk);
	    }

	    if (hr == MK_S_MONIKERALREADYREGISTERED) {
		pROT->lpVtbl->Revoke(pROT, riPtr->cookie);
	    } else if (hr == S_OK) {
		break;
	    }
	}

	pROT->lpVtbl->Release(pROT);
    }

    if (SUCCEEDED(hr)) {
	riPtr->name = strdup(actualName);
    }

    Tcl_DStringFree(&dString);
    return hr;
}
#endif /* TK_SEND_ENABLED_ON_WINDOWS */

/*
 * ----------------------------------------------------------------------
 *
 * Send --
 *
 *	Perform an interface call to the server object. We convert the Tcl
 *	arguments into a BSTR using 'concat'. The result should be a BSTR that
 *	we can set as the interp's result string.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */

static int
Send(
    LPDISPATCH pdispInterp,	/* Pointer to the remote interp's COM
				 * object. */
    Tcl_Interp *interp,		/* The local interpreter. */
    int async,			/* Flag for the calling style. */
    ClientData clientData,	/* The RegisteredInterp structure for this
				 * interp. */
    int objc,			/* Number of arguments to be sent. */
    Tcl_Obj *const objv[])	/* The arguments to be sent. */
{
    VARIANT vCmd, vResult;
    DISPPARAMS dp;
    EXCEPINFO ei;
    UINT uiErr = 0;
    HRESULT hr = S_OK, ehr = S_OK;
    Tcl_Obj *cmd = NULL;
    DISPID dispid;
    Tcl_DString ds;
    const char *src;

    cmd = Tcl_ConcatObj(objc, objv);

    /*
     * Setup the arguments for the COM method call.
     */

    VariantInit(&vCmd);
    VariantInit(&vResult);
    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));

    vCmd.vt = VT_BSTR;
    src = Tcl_GetString(cmd);
    Tcl_WinUtfToTChar(src, cmd->length, &ds);
    vCmd.bstrVal = SysAllocString((WCHAR *) Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);

    dp.cArgs = 1;
    dp.rgvarg = &vCmd;

    /*
     * Select the method to use based upon the async flag and call the method.
     */

    dispid = async ? TKWINSENDCOM_DISPID_ASYNC : TKWINSENDCOM_DISPID_SEND;

    hr = pdispInterp->lpVtbl->Invoke(pdispInterp, dispid,
	    &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD,
	    &dp, &vResult, &ei, &uiErr);

    /*
     * Convert the result into a string and place in the interps result.
     */

    ehr = VariantChangeType(&vResult, &vResult, 0, VT_BSTR);
    if (SUCCEEDED(ehr)) {
	Tcl_WinTCharToUtf((LPCTSTR)vResult.bstrVal, SysStringLen(vResult.bstrVal) *
		sizeof (WCHAR), &ds);
	Tcl_DStringResult(interp, &ds);
    }

    /*
     * Errors are returned as dispatch exceptions. If an error code was
     * returned then we decode the exception and setup the Tcl error
     * variables.
     */

    if (hr == DISP_E_EXCEPTION && ei.bstrSource != NULL) {
	Tcl_Obj *opError, *opErrorCode, *opErrorInfo;
	Tcl_WinTCharToUtf((LPCTSTR)ei.bstrSource, SysStringLen(ei.bstrSource) *
		sizeof (WCHAR), &ds);
	opError = Tcl_NewStringObj(Tcl_DStringValue(&ds),
		Tcl_DStringLength(&ds));
	Tcl_DStringFree(&ds);
	Tcl_ListObjIndex(interp, opError, 0, &opErrorCode);
	Tcl_SetObjErrorCode(interp, opErrorCode);
	Tcl_ListObjIndex(interp, opError, 1, &opErrorInfo);
	Tcl_AppendObjToErrorInfo(interp, opErrorInfo);
    }

    /*
     * Clean up any COM allocated resources.
     */

    SysFreeString(ei.bstrDescription);
    SysFreeString(ei.bstrSource);
    SysFreeString(ei.bstrHelpFile);
    VariantClear(&vCmd);

    return (SUCCEEDED(hr) ? TCL_OK : TCL_ERROR);
}

/*
 * ----------------------------------------------------------------------
 *
 * TkWinSend_SetExcepInfo --
 *
 *	Convert the error information from a Tcl interpreter into a COM
 *	exception structure. This information is then registered with the COM
 *	thread exception object so that it can be used for rich error
 *	reporting by COM clients.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The current COM thread has its error object modified.
 *
 * ----------------------------------------------------------------------
 */

void
TkWinSend_SetExcepInfo(
    Tcl_Interp *interp,
    EXCEPINFO *pExcepInfo)
{
    Tcl_Obj *opError, *opErrorInfo, *opErrorCode;
    ICreateErrorInfo *pCEI;
    IErrorInfo *pEI, **ppEI = &pEI;
    HRESULT hr;
    Tcl_DString ds;
    const char *src;

    if (!pExcepInfo) {
	return;
    }

    opError = Tcl_GetObjResult(interp);
    opErrorInfo = Tcl_GetVar2Ex(interp, "errorInfo", NULL, TCL_GLOBAL_ONLY);
    opErrorCode = Tcl_GetVar2Ex(interp, "errorCode", NULL, TCL_GLOBAL_ONLY);

    /*
     * Pack the trace onto the end of the Tcl exception descriptor.
     */

    opErrorCode = Tcl_DuplicateObj(opErrorCode);
    Tcl_IncrRefCount(opErrorCode);
    Tcl_ListObjAppendElement(interp, opErrorCode, opErrorInfo);
    /* TODO: Handle failure to append */

    src = Tcl_GetString(opError);
    Tcl_WinUtfToTChar(src, opError->length, &ds);
    pExcepInfo->bstrDescription =
	    SysAllocString((WCHAR *) Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
    src = Tcl_GetString(opErrorCode);
    Tcl_WinUtfToTChar(src, opErrorCode->length, &ds);
    pExcepInfo->bstrSource =
	    SysAllocString((WCHAR *) Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
    Tcl_DecrRefCount(opErrorCode);
    pExcepInfo->scode = E_FAIL;

    hr = CreateErrorInfo(&pCEI);
    if (!SUCCEEDED(hr)) {
	return;
    }

    hr = pCEI->lpVtbl->SetGUID(pCEI, &IID_IDispatch);
    hr = pCEI->lpVtbl->SetDescription(pCEI, pExcepInfo->bstrDescription);
    hr = pCEI->lpVtbl->SetSource(pCEI, pExcepInfo->bstrSource);
    hr = pCEI->lpVtbl->QueryInterface(pCEI, &IID_IErrorInfo, (void **) ppEI);
    if (SUCCEEDED(hr)) {
	SetErrorInfo(0, pEI);
	pEI->lpVtbl->Release(pEI);
    }
    pCEI->lpVtbl->Release(pCEI);
}

/*
 * ----------------------------------------------------------------------
 *
 * TkWinSend_QueueCommand --
 *
 *	Queue a script for asynchronous evaluation. This is called from the
 *	COM objects Async method.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */

int
TkWinSend_QueueCommand(
    Tcl_Interp *interp,
    Tcl_Obj *cmdPtr)
{
    SendEvent *evPtr;

    TRACE("SendQueueCommand()\n");

    evPtr = ckalloc(sizeof(SendEvent));
    evPtr->header.proc = SendEventProc;
    evPtr->header.nextPtr = NULL;
    evPtr->interp = interp;
    Tcl_Preserve(evPtr->interp);

    if (Tcl_IsShared(cmdPtr)) {
	evPtr->cmdPtr = Tcl_DuplicateObj(cmdPtr);
    } else {
	evPtr->cmdPtr = cmdPtr;
	Tcl_IncrRefCount(evPtr->cmdPtr);
    }

    Tcl_QueueEvent((Tcl_Event *)evPtr, TCL_QUEUE_TAIL);

    return 0;
}

/*
 * ----------------------------------------------------------------------
 *
 * SendEventProc --
 *
 *	Handle a request for an asynchronous send. Nothing is returned to the
 *	caller so the result is discarded.
 *
 * Results:
 *	Returns 1 if the event was handled or 0 to indicate it has been
 *	deferred.
 *
 * Side effects:
 *	The target interpreter's result will be modified.
 *
 * ----------------------------------------------------------------------
 */

static int
SendEventProc(
    Tcl_Event *eventPtr,
    int flags)
{
    SendEvent *evPtr = (SendEvent *)eventPtr;

    TRACE("SendEventProc\n");

    Tcl_EvalObjEx(evPtr->interp, evPtr->cmdPtr,
	    TCL_EVAL_DIRECT | TCL_EVAL_GLOBAL);

    Tcl_DecrRefCount(evPtr->cmdPtr);
    Tcl_Release(evPtr->interp);

    return 1; /* 1 to indicate the event has been handled */
}

/*
 * ----------------------------------------------------------------------
 *
 * SendTrace --
 *
 *	Provide trace information to the Windows debug stream. To use this -
 *	use the TRACE macro, which compiles to nothing when DEBUG is not
 *	defined.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */

static void
SendTrace(
    const char *format, ...)
{
    va_list args;
    static char buffer[1024];

    va_start(args, format);
    _vsnprintf(buffer, 1023, format, args);
    OutputDebugStringA(buffer);
    va_end(args);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
