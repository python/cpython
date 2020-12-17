#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

typedef struct _RMU_SESSION *PRMU_SESSION;

HRESULT DAPI RmuJoinSession(
    __out PRMU_SESSION *ppSession,
    __in_z LPCWSTR wzSessionKey
    );

HRESULT DAPI RmuAddFile(
    __in PRMU_SESSION pSession,
    __in_z LPCWSTR wzPath
    );

HRESULT DAPI RmuAddProcessById(
    __in PRMU_SESSION pSession,
    __in DWORD dwProcessId
    );

HRESULT DAPI RmuAddProcessesByName(
    __in PRMU_SESSION pSession,
    __in_z LPCWSTR wzProcessName
    );

HRESULT DAPI RmuAddService(
    __in PRMU_SESSION pSession,
    __in_z LPCWSTR wzServiceName
    );

HRESULT DAPI RmuRegisterResources(
    __in PRMU_SESSION pSession
    );

HRESULT DAPI RmuEndSession(
    __in PRMU_SESSION pSession
    );

#ifdef __cplusplus
}
#endif
