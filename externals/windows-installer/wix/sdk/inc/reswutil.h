#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

HRESULT DAPI ResWriteString(
    __in_z LPCWSTR wzResourceFile,
    __in DWORD dwDataId,
    __in_z LPCWSTR wzData,
    __in WORD wLangId
    );

HRESULT DAPI ResWriteData(
    __in_z LPCWSTR wzResourceFile,
    __in_z LPCSTR szDataName,
    __in PVOID pData,
    __in DWORD cbData
    );

HRESULT DAPI ResImportDataFromFile(
    __in_z LPCWSTR wzTargetFile,
    __in_z LPCWSTR wzSourceFile,
    __in_z LPCSTR szDataName
    );

#ifdef __cplusplus
}
#endif
