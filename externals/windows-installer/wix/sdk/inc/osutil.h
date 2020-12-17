#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

typedef enum OS_VERSION
{
    OS_VERSION_UNKNOWN,
    OS_VERSION_WINNT,
    OS_VERSION_WIN2000,
    OS_VERSION_WINXP,
    OS_VERSION_WIN2003,
    OS_VERSION_VISTA,
    OS_VERSION_WIN2008,
    OS_VERSION_WIN7,
    OS_VERSION_WIN2008_R2,
    OS_VERSION_FUTURE
} OS_VERSION;

void DAPI OsGetVersion(
    __out OS_VERSION* pVersion,
    __out DWORD* pdwServicePack
    );
HRESULT DAPI OsCouldRunPrivileged(
    __out BOOL* pfPrivileged
    );
HRESULT DAPI OsIsRunningPrivileged(
    __out BOOL* pfPrivileged
    );
HRESULT DAPI OsIsUacEnabled(
    __out BOOL* pfUacEnabled
    );

#ifdef __cplusplus
}
#endif
