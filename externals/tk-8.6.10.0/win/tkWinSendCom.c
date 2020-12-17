/*
 * tkWinSendCom.c --
 *
 *	This file provides support functions that implement the Windows "send"
 *	command using COM interfaces, allowing commands to be passed from
 *	interpreter to interpreter. See also tkWinSend.c, where most of the
 *	interesting functions are.
 *
 * We implement a COM class for use in registering Tcl interpreters with the
 * system's Running Object Table. This class implements an IDispatch interface
 * with the following method:
 *	Send(String cmd) As String
 * In other words the Send methods takes a string and evaluates this in the
 * Tcl interpreter. The result is returned as another string.
 *
 * Copyright (C) 2002 Pat Thoyts <patthoyts@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkWinSendCom.h"

/*
 * ----------------------------------------------------------------------
 * Non-public prototypes.
 *
 *	These are the interface methods for IUnknown, IDispatch and
 *	ISupportErrorInfo.
 *
 * ----------------------------------------------------------------------
 */

static void		TkWinSendCom_Destroy(LPDISPATCH pdisp);

static STDMETHODIMP	WinSendCom_QueryInterface(IDispatch *This,
			    REFIID riid, void **ppvObject);
static STDMETHODIMP_(ULONG)	WinSendCom_AddRef(IDispatch *This);
static STDMETHODIMP_(ULONG)	WinSendCom_Release(IDispatch *This);
static STDMETHODIMP	WinSendCom_GetTypeInfoCount(IDispatch *This,
			    UINT *pctinfo);
static STDMETHODIMP	WinSendCom_GetTypeInfo(IDispatch *This, UINT iTInfo,
			    LCID lcid, ITypeInfo **ppTI);
static STDMETHODIMP	WinSendCom_GetIDsOfNames(IDispatch *This, REFIID riid,
			    LPOLESTR *rgszNames, UINT cNames, LCID lcid,
			    DISPID *rgDispId);
static STDMETHODIMP	WinSendCom_Invoke(IDispatch *This, DISPID dispidMember,
			    REFIID riid, LCID lcid, WORD wFlags,
			    DISPPARAMS *pDispParams, VARIANT *pvarResult,
			    EXCEPINFO *pExcepInfo, UINT *puArgErr);
static STDMETHODIMP	ISupportErrorInfo_QueryInterface(
			    ISupportErrorInfo *This, REFIID riid,
			    void **ppvObject);
static STDMETHODIMP_(ULONG)	ISupportErrorInfo_AddRef(
				    ISupportErrorInfo *This);
static STDMETHODIMP_(ULONG)	ISupportErrorInfo_Release(
				    ISupportErrorInfo *This);
static STDMETHODIMP	ISupportErrorInfo_InterfaceSupportsErrorInfo(
			    ISupportErrorInfo *This, REFIID riid);
static HRESULT		Send(TkWinSendCom *obj, VARIANT vCmd,
			    VARIANT *pvResult, EXCEPINFO *pExcepInfo,
			    UINT *puArgErr);
static HRESULT		Async(TkWinSendCom *obj, VARIANT Cmd,
			    EXCEPINFO *pExcepInfo, UINT *puArgErr);

/*
 * ----------------------------------------------------------------------
 *
 * CreateInstance --
 *
 *	Create and initialises a new instance of the WinSend COM class and
 *	returns an interface pointer for you to use.
 *
 * ----------------------------------------------------------------------
 */

HRESULT
TkWinSendCom_CreateInstance(
    Tcl_Interp *interp,
    REFIID riid,
    void **ppv)
{
    /*
     * Construct v-tables for each interface.
     */

    static IDispatchVtbl vtbl = {
	WinSendCom_QueryInterface,
	WinSendCom_AddRef,
	WinSendCom_Release,
	WinSendCom_GetTypeInfoCount,
	WinSendCom_GetTypeInfo,
	WinSendCom_GetIDsOfNames,
	WinSendCom_Invoke,
    };
    static ISupportErrorInfoVtbl vtbl2 = {
	ISupportErrorInfo_QueryInterface,
	ISupportErrorInfo_AddRef,
	ISupportErrorInfo_Release,
	ISupportErrorInfo_InterfaceSupportsErrorInfo,
    };
    TkWinSendCom *obj = NULL;

    /*
     * This had probably better always be globally visible memory so we shall
     * use the COM Task allocator.
     */

    obj = (TkWinSendCom *) CoTaskMemAlloc(sizeof(TkWinSendCom));
    if (obj == NULL) {
	*ppv = NULL;
	return E_OUTOFMEMORY;
    }

    obj->lpVtbl = &vtbl;
    obj->lpVtbl2 = &vtbl2;
    obj->refcount = 0;
    obj->interp = interp;

    /*
     * lock the interp? Tcl_AddRef/Retain?
     */

    return obj->lpVtbl->QueryInterface((IDispatch *) obj, riid, ppv);
}

/*
 * ----------------------------------------------------------------------
 *
 * TkWinSendCom_Destroy --
 *
 *	This helper function is the destructor for our COM class.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Releases the storage allocated for this object.
 *
 * ----------------------------------------------------------------------
 */
static void
TkWinSendCom_Destroy(
    LPDISPATCH pdisp)
{
    CoTaskMemFree((void *) pdisp);
}

/*
 * ----------------------------------------------------------------------
 *
 * IDispatch --
 *
 *	The IDispatch interface implements the 'late-binding' COM methods
 *	typically used by scripting COM clients. The Invoke method is the most
 *	important one.
 *
 * ----------------------------------------------------------------------
 */

static STDMETHODIMP
WinSendCom_QueryInterface(
    IDispatch *This,
    REFIID riid,
    void **ppvObject)
{
    HRESULT hr = E_NOINTERFACE;
    TkWinSendCom *this = (TkWinSendCom *) This;
    *ppvObject = NULL;

    if (memcmp(riid, &IID_IUnknown, sizeof(IID)) == 0
	    || memcmp(riid, &IID_IDispatch, sizeof(IID)) == 0) {
	*ppvObject = (void **) this;
	this->lpVtbl->AddRef(This);
	hr = S_OK;
    } else if (memcmp(riid, &IID_ISupportErrorInfo, sizeof(IID)) == 0) {
	*ppvObject = (void **) (this + 1);
	this->lpVtbl2->AddRef((ISupportErrorInfo *) (this + 1));
	hr = S_OK;
    }
    return hr;
}

static STDMETHODIMP_(ULONG)
WinSendCom_AddRef(
    IDispatch *This)
{
    TkWinSendCom *this = (TkWinSendCom*)This;

    return InterlockedIncrement(&this->refcount);
}

static STDMETHODIMP_(ULONG)
WinSendCom_Release(
    IDispatch *This)
{
    long r = 0;
    TkWinSendCom *this = (TkWinSendCom*)This;

    if ((r = InterlockedDecrement(&this->refcount)) == 0) {
	TkWinSendCom_Destroy(This);
    }
    return r;
}

static STDMETHODIMP
WinSendCom_GetTypeInfoCount(
    IDispatch *This,
    UINT *pctinfo)
{
    HRESULT hr = E_POINTER;

    if (pctinfo != NULL) {
	*pctinfo = 0;
	hr = S_OK;
    }
    return hr;
}

static STDMETHODIMP
WinSendCom_GetTypeInfo(
    IDispatch *This,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTI)
{
    HRESULT hr = E_POINTER;

    if (ppTI) {
	*ppTI = NULL;
	hr = E_NOTIMPL;
    }
    return hr;
}

static STDMETHODIMP
WinSendCom_GetIDsOfNames(
    IDispatch *This,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId)
{
    HRESULT hr = E_POINTER;

    if (rgDispId) {
	hr = DISP_E_UNKNOWNNAME;
	if (_wcsicmp(*rgszNames, L"Send") == 0) {
	    *rgDispId = TKWINSENDCOM_DISPID_SEND, hr = S_OK;
	} else if (_wcsicmp(*rgszNames, L"Async") == 0) {
	    *rgDispId = TKWINSENDCOM_DISPID_ASYNC, hr = S_OK;
	}
    }
    return hr;
}

static STDMETHODIMP
WinSendCom_Invoke(
    IDispatch *This,
    DISPID dispidMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pvarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    TkWinSendCom *this = (TkWinSendCom*)This;

    switch (dispidMember) {
    case TKWINSENDCOM_DISPID_SEND:
	if (wFlags | DISPATCH_METHOD) {
	    if (pDispParams->cArgs != 1) {
		hr = DISP_E_BADPARAMCOUNT;
	    } else {
		hr = Send(this, pDispParams->rgvarg[0], pvarResult,
			pExcepInfo, puArgErr);
	    }
	}
	break;

    case TKWINSENDCOM_DISPID_ASYNC:
	if (wFlags | DISPATCH_METHOD) {
	    if (pDispParams->cArgs != 1) {
		hr = DISP_E_BADPARAMCOUNT;
	    } else {
		hr = Async(this, pDispParams->rgvarg[0], pExcepInfo, puArgErr);
	    }
	}
	break;
    }
    return hr;
}

/*
 * ----------------------------------------------------------------------
 *
 * ISupportErrorInfo --
 *
 *	This interface provides rich error information to COM clients. Used by
 *	VB and scripting COM clients.
 *
 * ----------------------------------------------------------------------
 */

static STDMETHODIMP
ISupportErrorInfo_QueryInterface(
    ISupportErrorInfo *This,
    REFIID riid,
    void **ppvObject)
{
    TkWinSendCom *this = (TkWinSendCom *)(This - 1);

    return this->lpVtbl->QueryInterface((IDispatch *) this, riid, ppvObject);
}

static STDMETHODIMP_(ULONG)
ISupportErrorInfo_AddRef(
    ISupportErrorInfo *This)
{
    TkWinSendCom *this = (TkWinSendCom *)(This - 1);

    return InterlockedIncrement(&this->refcount);
}

static STDMETHODIMP_(ULONG)
ISupportErrorInfo_Release(
    ISupportErrorInfo *This)
{
    TkWinSendCom *this = (TkWinSendCom *)(This - 1);

    return this->lpVtbl->Release((IDispatch *) this);
}

static STDMETHODIMP
ISupportErrorInfo_InterfaceSupportsErrorInfo(
    ISupportErrorInfo *This,
    REFIID riid)
{
    /*TkWinSendCom *this = (TkWinSendCom*)(This - 1);*/
    return S_OK; /* or S_FALSE */
}

/*
 * ----------------------------------------------------------------------
 *
 * Async --
 *
 *	Queues the command for evaluation in the assigned interpreter.
 *
 * Results:
 *	A standard COM HRESULT is returned. The Tcl result is discarded.
 *
 * Side effects:
 *	The interpreters state and result will be modified.
 *
 * ----------------------------------------------------------------------
 */

static HRESULT
Async(
    TkWinSendCom *obj,
    VARIANT Cmd,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr)
{
    HRESULT hr = S_OK;
    VARIANT vCmd;
    Tcl_DString ds;

    VariantInit(&vCmd);

    hr = VariantChangeType(&vCmd, &Cmd, 0, VT_BSTR);
    if (FAILED(hr)) {
	Tcl_SetObjResult(obj->interp, Tcl_NewStringObj(
		"invalid args: Async(command)", -1));
	TkWinSend_SetExcepInfo(obj->interp, pExcepInfo);
	hr = DISP_E_EXCEPTION;
    }

    if (SUCCEEDED(hr) && obj->interp) {
	Tcl_Obj *scriptPtr;

	Tcl_WinTCharToUtf((LPCTSTR)vCmd.bstrVal, SysStringLen(vCmd.bstrVal) *
		sizeof (WCHAR), &ds);
	scriptPtr =
		Tcl_NewStringObj(Tcl_DStringValue(&ds), Tcl_DStringLength(&ds));
	Tcl_DStringFree(&ds);
	TkWinSend_QueueCommand(obj->interp, scriptPtr);
    }

    VariantClear(&vCmd);
    return hr;
}

/*
 * ----------------------------------------------------------------------
 *
 * Send --
 *
 *	Evaluates the string in the assigned interpreter. If the result is a
 *	valid address then set it to the result returned by the evaluation.
 *	Tcl exceptions are converted into COM exceptions.
 *
 * Results:
 *	A standard COM HRESULT is returned. The Tcl result is set as the
 *	method calls result.
 *
 * Side effects:
 *	The interpreters state and result will be modified.
 *
 * ----------------------------------------------------------------------
 */

static HRESULT
Send(
    TkWinSendCom *obj,
    VARIANT vCmd,
    VARIANT *pvResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr)
{
    HRESULT hr = S_OK;
    int result = TCL_OK;
    VARIANT v;
    register Tcl_Interp *interp = obj->interp;
    Tcl_Obj *scriptPtr;
    Tcl_DString ds;

    if (interp == NULL) {
	return S_OK;
    }
    VariantInit(&v);
    hr = VariantChangeType(&v, &vCmd, 0, VT_BSTR);
    if (!SUCCEEDED(hr)) {
	return hr;
    }

    Tcl_WinTCharToUtf((LPCTSTR)v.bstrVal, SysStringLen(v.bstrVal) *
	    sizeof(WCHAR), &ds);
    scriptPtr = Tcl_NewStringObj(Tcl_DStringValue(&ds), Tcl_DStringLength(&ds));
    Tcl_DStringFree(&ds);
    Tcl_Preserve(interp);
    Tcl_IncrRefCount(scriptPtr);
    result = Tcl_EvalObjEx(interp, scriptPtr,
	    TCL_EVAL_DIRECT | TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(scriptPtr);
    if (pvResult != NULL) {
	Tcl_Obj *obj;
	const char *src;

	VariantInit(pvResult);
	pvResult->vt = VT_BSTR;
	obj = Tcl_GetObjResult(interp);
	src = Tcl_GetString(obj);
	Tcl_WinUtfToTChar(src, obj->length, &ds);
	pvResult->bstrVal = SysAllocString((WCHAR *) Tcl_DStringValue(&ds));
	Tcl_DStringFree(&ds);
    }
    if (result == TCL_ERROR) {
	hr = DISP_E_EXCEPTION;
	TkWinSend_SetExcepInfo(interp, pExcepInfo);
    }
    Tcl_Release(interp);
    VariantClear(&v);
    return hr;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
