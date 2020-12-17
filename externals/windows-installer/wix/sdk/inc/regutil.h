#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif


#define ReleaseRegKey(h) if (h) { ::RegCloseKey(h); h = NULL; }

typedef enum REG_KEY_BITNESS
{
    REG_KEY_DEFAULT = 0,
    REG_KEY_32BIT = 1,
    REG_KEY_64BIT = 2
} REG_KEY_BITNESS;

typedef LSTATUS (APIENTRY *PFN_REGCREATEKEYEXW)(
    __in HKEY hKey,
    __in LPCWSTR lpSubKey,
    __reserved DWORD Reserved,
    __in_opt LPWSTR lpClass,
    __in DWORD dwOptions,
    __in REGSAM samDesired,
    __in_opt CONST LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    __out PHKEY phkResult,
    __out_opt LPDWORD lpdwDisposition
    );
typedef LSTATUS (APIENTRY *PFN_REGOPENKEYEXW)(
    __in HKEY hKey,
    __in_opt LPCWSTR lpSubKey,
    __reserved DWORD ulOptions,
    __in REGSAM samDesired,
    __out PHKEY phkResult
    );
typedef LSTATUS (APIENTRY *PFN_REGDELETEKEYEXW)(
    __in HKEY hKey,
    __in LPCWSTR lpSubKey,
    __in REGSAM samDesired,
    __reserved DWORD Reserved
    );
typedef LSTATUS (APIENTRY *PFN_REGDELETEKEYW)(
    __in HKEY hKey,
    __in LPCWSTR lpSubKey
    );
typedef LSTATUS (APIENTRY *PFN_REGENUMKEYEXW)(
    __in         HKEY hKey,
    __in         DWORD dwIndex,
    __out        LPWSTR lpName,
    __inout      LPDWORD lpcName,
    __reserved   LPDWORD lpReserved,
    __inout      LPWSTR lpClass,
    __inout_opt  LPDWORD lpcClass,
    __out_opt    PFILETIME lpftLastWriteTime
    );
typedef LSTATUS (APIENTRY *PFN_REGENUMVALUEW)(
    __in         HKEY hKey,
    __in         DWORD dwIndex,
    __out        LPWSTR lpValueName,
    __inout      LPDWORD lpcchValueName,
    __reserved   LPDWORD lpReserved,
    __out_opt    LPDWORD lpType,
    __out_opt    LPBYTE lpData,
    __out_opt    LPDWORD lpcbData
    );
typedef LSTATUS (APIENTRY *PFN_REGQUERYINFOKEYW)(
    __in         HKEY hKey,
    __out        LPWSTR lpClass,
    __inout_opt  LPDWORD lpcClass,
    __reserved   LPDWORD lpReserved,
    __out_opt    LPDWORD lpcSubKeys,
    __out_opt    LPDWORD lpcMaxSubKeyLen,
    __out_opt    LPDWORD lpcMaxClassLen,
    __out_opt    LPDWORD lpcValues,
    __out_opt    LPDWORD lpcMaxValueNameLen,
    __out_opt    LPDWORD lpcMaxValueLen,
    __out_opt    LPDWORD lpcbSecurityDescriptor,
    __out_opt    PFILETIME lpftLastWriteTime
    );
typedef LSTATUS (APIENTRY *PFN_REGQUERYVALUEEXW)(
    __in HKEY hKey,
    __in_opt LPCWSTR lpValueName,
    __reserved LPDWORD lpReserved,
    __out_opt LPDWORD lpType,
    __out_bcount_part_opt(*lpcbData, *lpcbData) __out_data_source(REGISTRY) LPBYTE lpData,
    __inout_opt LPDWORD lpcbData
    );
typedef LSTATUS (APIENTRY *PFN_REGSETVALUEEXW)(
    __in HKEY hKey,
    __in_opt LPCWSTR lpValueName,
    __reserved DWORD Reserved,
    __in DWORD dwType,
    __in_bcount_opt(cbData) CONST BYTE* lpData,
    __in DWORD cbData
    );
typedef LSTATUS (APIENTRY *PFN_REGDELETEVALUEW)(
    __in HKEY hKey,
    __in_opt LPCWSTR lpValueName
    );

HRESULT DAPI RegInitialize();
void DAPI RegUninitialize();

void DAPI RegFunctionOverride(
    __in_opt PFN_REGCREATEKEYEXW pfnRegCreateKeyExW,
    __in_opt PFN_REGOPENKEYEXW pfnRegOpenKeyExW,
    __in_opt PFN_REGDELETEKEYEXW pfnRegDeleteKeyExW,
    __in_opt PFN_REGENUMKEYEXW pfnRegEnumKeyExW,
    __in_opt PFN_REGENUMVALUEW pfnRegEnumValueW,
    __in_opt PFN_REGQUERYINFOKEYW pfnRegQueryInfoKeyW,
    __in_opt PFN_REGQUERYVALUEEXW pfnRegQueryValueExW,
    __in_opt PFN_REGSETVALUEEXW pfnRegSetValueExW,
    __in_opt PFN_REGDELETEVALUEW pfnRegDeleteValueW
    );
HRESULT DAPI RegCreate(
    __in HKEY hkRoot,
    __in_z LPCWSTR wzSubKey,
    __in DWORD dwAccess,
    __out HKEY* phk
    );
HRESULT DAPI RegCreateEx(
    __in HKEY hkRoot,
    __in_z LPCWSTR wzSubKey,
    __in DWORD dwAccess,
    __in BOOL fVolatile,
    __in_opt SECURITY_ATTRIBUTES* pSecurityAttributes,
    __out HKEY* phk,
    __out_opt BOOL* pfCreated
    );
HRESULT DAPI RegOpen(
    __in HKEY hkRoot,
    __in_z LPCWSTR wzSubKey,
    __in DWORD dwAccess,
    __out HKEY* phk
    );
HRESULT DAPI RegDelete(
    __in HKEY hkRoot,
    __in_z LPCWSTR wzSubKey,
    __in REG_KEY_BITNESS kbKeyBitness,
    __in BOOL fDeleteTree
    );
HRESULT DAPI RegKeyEnum(
    __in HKEY hk,
    __in DWORD dwIndex,
    __deref_out_z LPWSTR* psczKey
    );
HRESULT DAPI RegValueEnum(
    __in HKEY hk,
    __in DWORD dwIndex,
    __deref_out_z LPWSTR* psczName,
    __out_opt DWORD *pdwType
    );
HRESULT DAPI RegGetType(
    __in HKEY hk,
    __in_z_opt LPCWSTR wzName,
    __out DWORD *pdwType
     );
HRESULT DAPI RegReadBinary(
    __in HKEY hk,
    __in_z_opt LPCWSTR wzName,
    __deref_out_bcount_opt(*pcbBuffer) BYTE** ppbBuffer,
    __out SIZE_T *pcbBuffer
     );
HRESULT DAPI RegReadString(
    __in HKEY hk,
    __in_z_opt LPCWSTR wzName,
    __deref_out_z LPWSTR* psczValue
    );
HRESULT DAPI RegReadStringArray(
    __in HKEY hk,
    __in_z_opt LPCWSTR wzName,
    __deref_out_ecount_opt(pcStrings) LPWSTR** prgsczStrings,
    __out DWORD *pcStrings
    );
HRESULT DAPI RegReadVersion(
    __in HKEY hk,
    __in_z_opt LPCWSTR wzName,
    __out DWORD64* pdw64Version
    );
HRESULT DAPI RegReadNumber(
    __in HKEY hk,
    __in_z_opt LPCWSTR wzName,
    __out DWORD* pdwValue
    );
HRESULT DAPI RegReadQword(
    __in HKEY hk,
    __in_z_opt LPCWSTR wzName,
    __out DWORD64* pqwValue
    );
HRESULT DAPI RegWriteBinary(
    __in HKEY hk,
    __in_z_opt LPCWSTR wzName,
    __in_bcount(cbBuffer) const BYTE *pbBuffer,
    __in DWORD cbBuffer
    );
HRESULT DAPI RegWriteString(
    __in HKEY hk,
    __in_z_opt LPCWSTR wzName,
    __in_z_opt LPCWSTR wzValue
    );
HRESULT DAPI RegWriteStringArray(
    __in HKEY hk,
    __in_z_opt LPCWSTR wzName,
    __in_ecount(cValues) LPWSTR *rgwzStrings,
    __in DWORD cStrings
    );
HRESULT DAPI RegWriteStringFormatted(
    __in HKEY hk,
    __in_z_opt LPCWSTR wzName,
    __in __format_string LPCWSTR szFormat,
    ...
    );
HRESULT DAPI RegWriteNumber(
    __in HKEY hk,
    __in_z_opt LPCWSTR wzName,
    __in DWORD dwValue
    );
HRESULT DAPI RegWriteQword(
    __in HKEY hk,
    __in_z_opt LPCWSTR wzName,
    __in DWORD64 qwValue
    );
HRESULT DAPI RegQueryKey(
    __in HKEY hk,
    __out_opt DWORD* pcSubKeys,
    __out_opt DWORD* pcValues
    );

#ifdef __cplusplus
}
#endif

