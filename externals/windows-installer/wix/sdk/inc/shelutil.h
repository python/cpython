#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifndef REFKNOWNFOLDERID
#define REFKNOWNFOLDERID REFGUID
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef BOOL (STDAPICALLTYPE *PFN_SHELLEXECUTEEXW)(
    __inout LPSHELLEXECUTEINFOW lpExecInfo
    );

void DAPI ShelFunctionOverride(
    __in_opt PFN_SHELLEXECUTEEXW pfnShellExecuteExW
    );
HRESULT DAPI ShelExec(
    __in_z LPCWSTR wzTargetPath,
    __in_opt LPCWSTR wzParameters,
    __in_opt LPCWSTR wzVerb,
    __in_opt LPCWSTR wzWorkingDirectory,
    __in int nShowCmd,
    __in_opt HWND hwndParent,
    __out_opt HANDLE* phProcess
    );
HRESULT DAPI ShelExecUnelevated(
    __in_z LPCWSTR wzTargetPath,
    __in_z_opt LPCWSTR wzParameters,
    __in_z_opt LPCWSTR wzVerb,
    __in_z_opt LPCWSTR wzWorkingDirectory,
    __in int nShowCmd
    );
HRESULT DAPI ShelGetFolder(
    __out_z LPWSTR* psczFolderPath,
    __in int csidlFolder
    );
HRESULT DAPI ShelGetKnownFolder(
    __out_z LPWSTR* psczFolderPath,
    __in REFKNOWNFOLDERID rfidFolder
    );

#ifdef __cplusplus
}
#endif
