#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

HRESULT DAPI TimeFromString(
    __in_z LPCWSTR wzTime,
    __out FILETIME* pFileTime
    );
HRESULT DAPI TimeFromString3339(
    __in_z LPCWSTR wzTime,
    __out FILETIME* pFileTime
    );
HRESULT DAPI TimeCurrentTime(
    __deref_out_z LPWSTR* ppwz,
    __in BOOL fGMT
    );
HRESULT DAPI TimeCurrentDateTime(
    __deref_out_z LPWSTR* ppwz,
    __in BOOL fGMT
    );
HRESULT DAPI TimeSystemDateTime(
    __deref_out_z LPWSTR* ppwz,
    __in const SYSTEMTIME *pst,
    __in BOOL fGMT
    );

#ifdef __cplusplus
}
#endif

