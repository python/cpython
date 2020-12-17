#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

#define LogExitOnFailure(x, i, f) if (FAILED(x)) { LogErrorId(x, i, NULL, NULL, NULL); ExitTrace(x, f); goto LExit; }
#define LogExitOnFailure1(x, i, f, s) if (FAILED(x)) { LogErrorId(x, i, s, NULL, NULL); ExitTrace1(x, f, s); goto LExit; }
#define LogExitOnFailure2(x, i, f, s, t) if (FAILED(x)) { LogErrorId(x, i, s, t, NULL); ExitTrace2(x, f, s, t); goto LExit; }
#define LogExitOnFailure3(x, i, f, s, t, u) if (FAILED(x)) { LogErrorId(x, i, s, t, u); ExitTrace3(x, f, s, t, u); goto LExit; }

#define LogExitOnRootFailure(x, i, f) if (FAILED(x)) { LogErrorId(x, i, NULL, NULL, NULL); Dutil_RootFailure(__FILE__, __LINE__, x); ExitTrace(x, f); goto LExit; }
#define LogExitOnRootFailure1(x, i, f, s) if (FAILED(x)) { LogErrorId(x, i, s, NULL, NULL); Dutil_RootFailure(__FILE__, __LINE__, x); ExitTrace1(x, f, s); goto LExit; }
#define LogExitOnRootFailure2(x, i, f, s, t) if (FAILED(x)) { LogErrorId(x, i, s, t, NULL); Dutil_RootFailure(__FILE__, __LINE__, x); ExitTrace2(x, f, s, t); goto LExit; }
#define LogExitOnRootFailure3(x, i, f, s, t, u) if (FAILED(x)) { LogErrorId(x, i, s, t, u); Dutil_RootFailure(__FILE__, __LINE__, x); ExitTrace3(x, f, s, t, u); goto LExit; }

typedef HRESULT (DAPI *PFN_LOGSTRINGWORKRAW)(
    __in_z LPCSTR szString,
    __in_opt LPVOID pvContext
    );

// enums

// structs

// functions
BOOL DAPI IsLogInitialized();

BOOL DAPI IsLogOpen();

void DAPI LogInitialize(
    __in HMODULE hModule
    );

HRESULT DAPI LogOpen(
    __in_z_opt LPCWSTR wzDirectory,
    __in_z LPCWSTR wzLog,
    __in_z_opt LPCWSTR wzPostfix,
    __in_z_opt LPCWSTR wzExt,
    __in BOOL fAppend,
    __in BOOL fHeader,
    __out_z_opt LPWSTR* psczLogPath
    );

void DAPI LogDisable();

void DAPI LogRedirect(
    __in_opt PFN_LOGSTRINGWORKRAW vpfLogStringWorkRaw,
    __in_opt LPVOID pvContext
    );

HRESULT DAPI LogRename(
    __in_z LPCWSTR wzNewPath
    );

void DAPI LogClose(
    __in BOOL fFooter
    );

void DAPI LogUninitialize(
    __in BOOL fFooter
    );

BOOL DAPI LogIsOpen();

HRESULT DAPI LogSetSpecialParams(
    __in_z_opt LPCWSTR wzSpecialBeginLine,
    __in_z_opt LPCWSTR wzSpecialAfterTimeStamp,
    __in_z_opt LPCWSTR wzSpecialEndLine
    );

REPORT_LEVEL DAPI LogSetLevel(
    __in REPORT_LEVEL rl,
    __in BOOL fLogChange
    );

REPORT_LEVEL DAPI LogGetLevel();

HRESULT DAPI LogGetPath(
    __out_ecount_z(cchLogPath) LPWSTR pwzLogPath, 
    __in DWORD cchLogPath
    );

HANDLE DAPI LogGetHandle();

HRESULT DAPIV LogString(
    __in REPORT_LEVEL rl,
    __in_z __format_string LPCSTR szFormat,
    ...
    );

HRESULT DAPI LogStringArgs(
    __in REPORT_LEVEL rl,
    __in_z __format_string LPCSTR szFormat,
    __in va_list args
    );

HRESULT DAPIV LogStringLine(
    __in REPORT_LEVEL rl,
    __in_z __format_string LPCSTR szFormat,
    ...
    );

HRESULT DAPI LogStringLineArgs(
    __in REPORT_LEVEL rl,
    __in_z __format_string LPCSTR szFormat,
    __in va_list args
    );

HRESULT DAPI LogIdModuleArgs(
    __in REPORT_LEVEL rl,
    __in DWORD dwLogId,
    __in_opt HMODULE hModule,
    __in va_list args
    );

/* 
 * Wraps LogIdModuleArgs, so inline to save the function call
 */

inline HRESULT LogId(
    __in REPORT_LEVEL rl,
    __in DWORD dwLogId,
    ...
    )
{
    HRESULT hr = S_OK;
    va_list args;

    va_start(args, dwLogId);
    hr = LogIdModuleArgs(rl, dwLogId, NULL, args);
    va_end(args);

    return hr;
}


/* 
 * Wraps LogIdModuleArgs, so inline to save the function call
 */
 
inline HRESULT LogIdArgs(
    __in REPORT_LEVEL rl,
    __in DWORD dwLogId,
    __in va_list args
    )
{
    return LogIdModuleArgs(rl, dwLogId, NULL, args);
}

HRESULT DAPIV LogErrorString(
    __in HRESULT hrError,
    __in_z __format_string LPCSTR szFormat,
    ...
    );

HRESULT DAPI LogErrorStringArgs(
    __in HRESULT hrError,
    __in_z __format_string LPCSTR szFormat,
    __in va_list args
    );

HRESULT DAPI LogErrorIdModule(
    __in HRESULT hrError,
    __in DWORD dwLogId,
    __in_opt HMODULE hModule,
    __in_z_opt LPCWSTR wzString1,
    __in_z_opt LPCWSTR wzString2,
    __in_z_opt LPCWSTR wzString3
    );

inline HRESULT LogErrorId(
    __in HRESULT hrError,
    __in DWORD dwLogId,
    __in_z_opt LPCWSTR wzString1,
    __in_z_opt LPCWSTR wzString2,
    __in_z_opt LPCWSTR wzString3
    )
{
    return LogErrorIdModule(hrError, dwLogId, NULL, wzString1, wzString2, wzString3);
}

HRESULT DAPI LogHeader();

HRESULT DAPI LogFooter();

HRESULT LogStringWorkRaw(
    __in_z LPCSTR szLogData
    );

// begin the switch of LogXXX to LogStringXXX
#define Log LogString
#define LogLine LogStringLine

#ifdef __cplusplus
}
#endif

