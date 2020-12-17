#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


typedef enum DIR_DELETE
{
    DIR_DELETE_FILES = 1,
    DIR_DELETE_RECURSE = 2,
    DIR_DELETE_SCHEDULE = 4,
} DIR_DELETE;

#ifdef __cplusplus
extern "C" {
#endif

BOOL DAPI DirExists(
    __in_z LPCWSTR wzPath, 
    __out_opt DWORD *pdwAttributes
    );

HRESULT DAPI DirCreateTempPath(
    __in_z LPCWSTR wzPrefix,
    __out_ecount_z(cchPath) LPWSTR wzPath,
    __in DWORD cchPath
    );

HRESULT DAPI DirEnsureExists(
    __in_z LPCWSTR wzPath, 
    __in_opt LPSECURITY_ATTRIBUTES psa
    );

HRESULT DAPI DirEnsureDelete(
    __in_z LPCWSTR wzPath,
    __in BOOL fDeleteFiles,
    __in BOOL fRecurse
    );

HRESULT DAPI DirEnsureDeleteEx(
    __in_z LPCWSTR wzPath,
    __in DWORD dwFlags
    );

DWORD DAPI DirDeleteEmptyDirectoriesToRoot(
    __in_z LPCWSTR wzPath,
    __in DWORD dwFlags
    );

HRESULT DAPI DirGetCurrent(
    __deref_out_z LPWSTR* psczCurrentDirectory
    );

HRESULT DAPI DirSetCurrent(
    __in_z LPCWSTR wzDirectory
    );

#ifdef __cplusplus
}
#endif

