#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#define ReleaseCryptMsg(p) if (p) { ::CryptMsgClose(p); p = NULL; }

#ifdef __cplusplus
extern "C" {
#endif


// Use CRYPTPROTECTMEMORY_BLOCK_SIZE, because it's larger and thus more restrictive than RTL_ENCRYPT_MEMORY_SIZE.
#define CRYP_ENCRYPT_MEMORY_SIZE CRYPTPROTECTMEMORY_BLOCK_SIZE
#define SHA1_HASH_LEN 20

typedef NTSTATUS (APIENTRY *PFN_RTLENCRYPTMEMORY)(
    __inout PVOID Memory,
    __in ULONG MemoryLength,
    __in ULONG OptionFlags
    );

typedef NTSTATUS (APIENTRY *PFN_RTLDECRYPTMEMORY)(
    __inout PVOID Memory,
    __in ULONG MemoryLength,
    __in ULONG OptionFlags
    );

typedef BOOL (APIENTRY *PFN_CRYPTPROTECTMEMORY)(
    __inout LPVOID pData,
    __in DWORD cbData,
    __in DWORD dwFlags
    );

typedef BOOL (APIENTRY *PFN_CRYPTUNPROTECTMEMORY)(
    __inout LPVOID pData,
    __in DWORD cbData,
    __in DWORD dwFlags
    );

// function declarations

HRESULT DAPI CrypInitialize();
void DAPI CrypUninitialize();

HRESULT DAPI CrypDecodeObject(
    __in_z LPCSTR szStructType,
    __in_ecount(cbData) const BYTE* pbData,
    __in DWORD cbData,
    __in DWORD dwFlags,
    __out LPVOID* ppvObject,
    __out_opt DWORD* pcbObject
    );

HRESULT DAPI CrypMsgGetParam(
    __in HCRYPTMSG hCryptMsg,
    __in DWORD dwType,
    __in DWORD dwIndex,
    __out LPVOID* ppvData,
    __out_opt DWORD* pcbData
    );

HRESULT DAPI CrypHashFile(
    __in_z LPCWSTR wzFilePath,
    __in DWORD dwProvType,
    __in ALG_ID algid,
    __out_bcount(cbHash) BYTE* pbHash,
    __in DWORD cbHash,
    __out_opt DWORD64* pqwBytesHashed
    );

HRESULT DAPI CrypHashFileHandle(
    __in HANDLE hFile,
    __in DWORD dwProvType,
    __in ALG_ID algid,
    __out_bcount(cbHash) BYTE* pbHash,
    __in DWORD cbHash,
    __out_opt DWORD64* pqwBytesHashed
    );

HRESULT DAPI CrypHashBuffer(
    __in_bcount(cbBuffer) const BYTE* pbBuffer,
    __in SIZE_T cbBuffer,
    __in DWORD dwProvType,
    __in ALG_ID algid,
    __out_bcount(cbHash) BYTE* pbHash,
    __in DWORD cbHash
    );

HRESULT DAPI CrypEncryptMemory(
    __inout LPVOID pData,
    __in DWORD cbData,
    __in DWORD dwFlags
    );

HRESULT DAPI CrypDecryptMemory(
    __inout LPVOID pData,
    __in DWORD cbData,
    __in DWORD dwFlags
    );

#ifdef __cplusplus
}
#endif
