#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

HRESULT DAPI UserBuildDomainUserName(
    __out_ecount_z(cchDest) LPWSTR wzDest,
    __in int cchDest,
    __in_z LPCWSTR pwzName,
    __in_z LPCWSTR pwzDomain
    );

HRESULT DAPI UserCheckIsMember(
    __in_z LPCWSTR pwzName,
    __in_z LPCWSTR pwzDomain,
    __in_z LPCWSTR pwzGroupName,
    __in_z LPCWSTR pwzGroupDomain,
    __out LPBOOL lpfMember
    );

HRESULT DAPI UserCreateADsPath(
    __in_z LPCWSTR wzObjectDomain, 
    __in_z LPCWSTR wzObjectName,
    __out BSTR *pbstrAdsPath
    );

#ifdef __cplusplus
}
#endif
