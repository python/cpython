#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#include <fdi.h>
#include <sys\stat.h>

#ifdef __cplusplus
extern "C" {
#endif

// structs


// callback function prototypes
typedef HRESULT (*CAB_CALLBACK_OPEN_FILE)(LPCWSTR wzFile, INT_PTR* ppFile);
typedef HRESULT (*CAB_CALLBACK_READ_FILE)(INT_PTR pFile, LPVOID pvData, DWORD cbData, DWORD* pcbRead);
typedef HRESULT (*CAB_CALLBACK_WRITE_FILE)(INT_PTR pFile, LPVOID pvData, DWORD cbData, DWORD* pcbRead);
typedef LONG (*CAB_CALLBACK_SEEK_FILE)(INT_PTR pFile, DWORD dwMove, DWORD dwMoveMethod);
typedef HRESULT (*CAB_CALLBACK_CLOSE_FILE)(INT_PTR pFile);

typedef HRESULT (*CAB_CALLBACK_BEGIN_FILE)(LPCWSTR wzFileId, FILETIME* pftFileTime, DWORD cbFileSize, LPVOID pvContext, INT_PTR* ppFile);
typedef HRESULT (*CAB_CALLBACK_END_FILE)(LPCWSTR wzFileId, LPVOID pvContext, INT_PTR pFile);
typedef HRESULT (*CAB_CALLBACK_PROGRESS)(BOOL fBeginFile, LPCWSTR wzFileId, LPVOID pvContext);

// function type with calling convention of __stdcall that .NET 1.1 understands only
// .NET 2.0 will not need this
typedef INT_PTR (FAR __stdcall *STDCALL_PFNFDINOTIFY)(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin);


// functions
HRESULT DAPI CabInitialize(
    __in BOOL fDelayLoad
    );
void DAPI CabUninitialize(
    );

HRESULT DAPI CabExtract(
    __in_z LPCWSTR wzCabinet,
    __in_z LPCWSTR wzExtractFile,
    __in_z LPCWSTR wzExtractDir,
    __in_opt CAB_CALLBACK_PROGRESS pfnProgress,
    __in_opt LPVOID pvContext,
    __in DWORD64 dw64EmbeddedOffset
    );

HRESULT DAPI CabEnumerate(
    __in_z LPCWSTR wzCabinet,
    __in_z LPCWSTR wzEnumerateFile,
    __in STDCALL_PFNFDINOTIFY pfnNotify,
    __in DWORD64 dw64EmbeddedOffset
    );

#ifdef __cplusplus
}
#endif
