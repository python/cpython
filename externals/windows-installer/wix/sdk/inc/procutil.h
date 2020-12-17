#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

// structs
typedef struct _PROC_FILESYSTEMREDIRECTION
{
    BOOL fDisabled;
    LPVOID pvRevertState;
} PROC_FILESYSTEMREDIRECTION;

HRESULT DAPI ProcElevated(
    __in HANDLE hProcess,
    __out BOOL* pfElevated
    );

HRESULT DAPI ProcWow64(
    __in HANDLE hProcess,
    __out BOOL* pfWow64
    );
HRESULT DAPI ProcDisableWowFileSystemRedirection(
    __in PROC_FILESYSTEMREDIRECTION* pfsr
    );
HRESULT DAPI ProcRevertWowFileSystemRedirection(
    __in PROC_FILESYSTEMREDIRECTION* pfsr
    );

HRESULT DAPI ProcExec(
    __in_z LPCWSTR wzExecutablePath,
    __in_z_opt LPCWSTR wzCommandLine,
    __in int nCmdShow,
    __out HANDLE *phProcess
    );
HRESULT DAPI ProcExecute(
    __in_z LPWSTR wzCommand,
    __out HANDLE *phProcess,
    __out_opt HANDLE *phChildStdIn,
    __out_opt HANDLE *phChildStdOutErr
    );
HRESULT DAPI ProcWaitForCompletion(
    __in HANDLE hProcess,
    __in DWORD dwTimeout,
    __out DWORD *pReturnCode
    );
HRESULT DAPI ProcWaitForIds(
    __in_ecount(cProcessIds) const DWORD* pdwProcessIds,
    __in DWORD cProcessIds,
    __in DWORD dwMilliseconds
    );
HRESULT DAPI ProcCloseIds(
    __in_ecount(cProcessIds) const DWORD* pdwProcessIds,
    __in DWORD cProcessIds
    );

// following code in proc2utl.cpp due to dependency on PSAPI.DLL.
HRESULT DAPI ProcFindAllIdsFromExeName(
    __in_z LPCWSTR wzExeName,
    __out DWORD** ppdwProcessIds,
    __out DWORD* pcProcessIds
    );

// following code in proc3utl.cpp due to dependency on Wtsapi32.DLL.
HRESULT DAPI ProcExecuteAsInteractiveUser(
    __in_z LPCWSTR wzExecutablePath,
    __in_z LPCWSTR wzCommand,
    __out HANDLE *phProcess
    );

#ifdef __cplusplus
}
#endif
