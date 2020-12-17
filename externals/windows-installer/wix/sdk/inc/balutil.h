#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#include "dutil.h"


#ifdef __cplusplus
extern "C" {
#endif

#define BalExitOnFailure(x, f) if (FAILED(x)) { BalLogError(x, f); ExitTrace(x, f); goto LExit; }
#define BalExitOnFailure1(x, f, s) if (FAILED(x)) { BalLogError(x, f, s); ExitTrace1(x, f, s); goto LExit; }
#define BalExitOnFailure2(x, f, s, t) if (FAILED(x)) { BalLogError(x, f, s, t); ExitTrace2(x, f, s, t); goto LExit; }
#define BalExitOnFailure3(x, f, s, t, u) if (FAILED(x)) { BalLogError(x, f, s, t, u); ExitTrace3(x, f, s, t, u); goto LExit; }

#define BalExitOnRootFailure(x, f) if (FAILED(x)) { BalLogError(x, f); Dutil_RootFailure(__FILE__, __LINE__, x); ExitTrace(x, f); goto LExit; }
#define BalExitOnRootFailure1(x, f, s) if (FAILED(x)) { BalLogError(x, f, s); Dutil_RootFailure(__FILE__, __LINE__, x); ExitTrace1(x, f, s); goto LExit; }
#define BalExitOnRootFailure2(x, f, s, t) if (FAILED(x)) { BalLogError(x, f, s, t); Dutil_RootFailure(__FILE__, __LINE__, x); ExitTrace2(x, f, s, t); goto LExit; }
#define BalExitOnRootFailure3(x, f, s, t, u) if (FAILED(x)) { BalLogError(x, f, s, t, u); Dutil_RootFailure(__FILE__, __LINE__, x); ExitTrace3(x, f, s, t, u); goto LExit; }

#define BalExitOnNullWithLastError(p, x, f) if (NULL == p) { DWORD Dutil_er = ::GetLastError(); x = HRESULT_FROM_WIN32(Dutil_er); if (!FAILED(x)) { x = E_FAIL; } BalLogError(x, f); ExitTrace(x, f); goto LExit; }
#define BalExitOnNullWithLastError1(p, x, f, s) if (NULL == p) { DWORD Dutil_er = ::GetLastError(); x = HRESULT_FROM_WIN32(Dutil_er); if (!FAILED(x)) { x = E_FAIL; } BalLogError(x, f, s); ExitTrace1(x, f, s); goto LExit; }

#define FACILITY_WIX 500

static const HRESULT E_WIXSTDBA_CONDITION_FAILED = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIX, 1);

static const HRESULT E_MBAHOST_NET452_ON_WIN7RTM = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIX, 1000);


/*******************************************************************
 BalInitialize - remembers the engine interface to enable logging and
                 other functions.

********************************************************************/
DAPI_(void) BalInitialize(
    __in IBootstrapperEngine* pEngine
    );

/*******************************************************************
 BalUninitialize - cleans up utility layer internals.

********************************************************************/
DAPI_(void) BalUninitialize();

/*******************************************************************
 BalManifestLoad - loads the Application manifest into an XML document.

********************************************************************/
DAPI_(HRESULT) BalManifestLoad(
    __in HMODULE hUXModule,
    __out IXMLDOMDocument** ppixdManifest
    );

/*******************************************************************
BalFormatString - formats a string using variables in the engine.

 Note: Use StrFree() to release psczOut.
********************************************************************/
DAPI_(HRESULT) BalFormatString(
    __in_z LPCWSTR wzFormat,
    __inout LPWSTR* psczOut
    );

/*******************************************************************
BalGetNumericVariable - gets a number from a variable in the engine.

 Note: Returns E_NOTFOUND if variable does not exist.
********************************************************************/
DAPI_(HRESULT) BalGetNumericVariable(
    __in_z LPCWSTR wzVariable,
    __out LONGLONG* pllValue
    );

/*******************************************************************
BalStringVariableExists - checks if a string variable exists in the engine.

********************************************************************/
DAPI_(BOOL) BalStringVariableExists(
    __in_z LPCWSTR wzVariable
    );

/*******************************************************************
BalGetStringVariable - gets a string from a variable in the engine.

 Note: Use StrFree() to release psczValue.
********************************************************************/
DAPI_(HRESULT) BalGetStringVariable(
    __in_z LPCWSTR wzVariable,
    __inout LPWSTR* psczValue
    );

/*******************************************************************
 BalLog - logs a message with the engine.

********************************************************************/
DAPIV_(HRESULT) BalLog(
    __in BOOTSTRAPPER_LOG_LEVEL level,
    __in_z __format_string LPCSTR szFormat,
    ...
    );

/*******************************************************************
 BalLogError - logs an error message with the engine.

********************************************************************/
DAPIV_(HRESULT) BalLogError(
    __in HRESULT hr,
    __in_z __format_string LPCSTR szFormat,
    ...
    );

#ifdef __cplusplus
}
#endif
