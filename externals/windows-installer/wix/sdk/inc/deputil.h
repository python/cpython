#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

#define ReleaseDependencyArray(rg, c) if (rg) { DepDependencyArrayFree(rg, c); }
#define ReleaseNullDependencyArray(rg, c) if (rg) { DepDependencyArrayFree(rg, c); rg = NULL; }

typedef struct _DEPENDENCY
{
    LPWSTR sczKey;
    LPWSTR sczName;
} DEPENDENCY;


/***************************************************************************
 DepGetProviderInformation - gets the various pieces of data registered
  with a dependency.

 Note: Returns E_NOTFOUND if the dependency was not found.
***************************************************************************/
DAPI_(HRESULT) DepGetProviderInformation(
    __in HKEY hkHive,
    __in_z LPCWSTR wzProviderKey,
    __deref_out_z_opt LPWSTR* psczId,
    __deref_out_z_opt LPWSTR* psczName,
    __out_opt DWORD64* pqwVersion
    );

/***************************************************************************
 DepCheckDependency - Checks that the dependency is registered and within
                      the proper version range.

 Note: Returns E_NOTFOUND if the dependency was not found.
***************************************************************************/
DAPI_(HRESULT) DepCheckDependency(
    __in HKEY hkHive,
    __in_z LPCWSTR wzProviderKey,
    __in_z_opt LPCWSTR wzMinVersion,
    __in_z_opt LPCWSTR wzMaxVersion,
    __in int iAttributes,
    __in STRINGDICT_HANDLE sdDependencies,
    __deref_inout_ecount_opt(*pcDependencies) DEPENDENCY** prgDependencies,
    __inout LPUINT pcDependencies
    );

/***************************************************************************
 DepCheckDependents - Checks if any dependents are still installed for the
                      given provider key.

***************************************************************************/
DAPI_(HRESULT) DepCheckDependents(
    __in HKEY hkHive,
    __in_z LPCWSTR wzProviderKey,
    __in int iAttributes,
    __in C_STRINGDICT_HANDLE sdIgnoredDependents,
    __deref_inout_ecount_opt(*pcDependents) DEPENDENCY** prgDependents,
    __inout LPUINT pcDependents
    );

/***************************************************************************
 DepRegisterDependency - Registers the dependency provider.

***************************************************************************/
DAPI_(HRESULT) DepRegisterDependency(
    __in HKEY hkHive,
    __in_z LPCWSTR wzProviderKey,
    __in_z LPCWSTR wzVersion,
    __in_z LPCWSTR wzDisplayName,
    __in_z_opt LPCWSTR wzId,
    __in int iAttributes
    );

/***************************************************************************
 DepDependentExists - Determines if a dependent is registered.

 Note: Returns S_OK if dependent is registered.
       Returns E_FILENOTFOUND if dependent is not registered
***************************************************************************/
DAPI_(HRESULT) DepDependentExists(
    __in HKEY hkHive,
    __in_z LPCWSTR wzDependencyProviderKey,
    __in_z LPCWSTR wzProviderKey
    );

/***************************************************************************
 DepRegisterDependent - Registers a dependent under the dependency provider.

***************************************************************************/
DAPI_(HRESULT) DepRegisterDependent(
    __in HKEY hkHive,
    __in_z LPCWSTR wzDependencyProviderKey,
    __in_z LPCWSTR wzProviderKey,
    __in_z_opt LPCWSTR wzMinVersion,
    __in_z_opt LPCWSTR wzMaxVersion,
    __in int iAttributes
    );

/***************************************************************************
 DepUnregisterDependency - Removes the dependency provider.

 Note: Caller should call CheckDependents prior to remove a dependency.
       Returns E_FILENOTFOUND if the dependency is not registered.
***************************************************************************/
DAPI_(HRESULT) DepUnregisterDependency(
    __in HKEY hkHive,
    __in_z LPCWSTR wzProviderKey
    );

/***************************************************************************
 DepUnregisterDependent - Removes a dependent under the dependency provider.

 Note: Returns E_FILENOTFOUND if neither the dependency or dependent are
       registered.
 ***************************************************************************/
DAPI_(HRESULT) DepUnregisterDependent(
    __in HKEY hkHive,
    __in_z LPCWSTR wzDependencyProviderKey,
    __in_z LPCWSTR wzProviderKey
    );

/***************************************************************************
 DependencyArrayAlloc - Allocates or expands an array of DEPENDENCY structs.

***************************************************************************/
DAPI_(HRESULT) DepDependencyArrayAlloc(
    __deref_inout_ecount_opt(*pcDependencies) DEPENDENCY** prgDependencies,
    __inout LPUINT pcDependencies,
    __in_z LPCWSTR wzKey,
    __in_z_opt LPCWSTR wzName
    );

/***************************************************************************
 DepDependencyArrayFree - Frees an array of DEPENDENCY structs.

***************************************************************************/
DAPI_(void) DepDependencyArrayFree(
    __in_ecount(cDependencies) DEPENDENCY* rgDependencies,
    __in UINT cDependencies
    );

#ifdef __cplusplus
}
#endif
