#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

#define ReleaseApupChain(p) if (p) { ApupFreeChain(p); p = NULL; }
#define ReleaseNullApupChain(p) if (p) { ApupFreeChain(p); p = NULL; }


const LPCWSTR APPLICATION_SYNDICATION_NAMESPACE = L"http://appsyndication.org/2006/appsyn";

typedef enum APUP_HASH_ALGORITHM
{
    APUP_HASH_ALGORITHM_UNKNOWN,
    APUP_HASH_ALGORITHM_MD5,
    APUP_HASH_ALGORITHM_SHA1,
    APUP_HASH_ALGORITHM_SHA256,
} APUP_HASH_ALGORITHM;


struct APPLICATION_UPDATE_ENCLOSURE
{
    LPWSTR wzUrl;
    LPWSTR wzLocalName;
    DWORD64 dw64Size;

    BYTE* rgbDigest;
    DWORD cbDigest;
    APUP_HASH_ALGORITHM digestAlgorithm;

    BOOL fInstaller;
};


struct APPLICATION_UPDATE_ENTRY
{
    LPWSTR wzApplicationId;
    LPWSTR wzApplicationType;
    LPWSTR wzTitle;
    LPWSTR wzSummary;
    LPWSTR wzContentType;
    LPWSTR wzContent;

    LPWSTR wzUpgradeId;
    BOOL fUpgradeExclusive;
    DWORD64 dw64Version;
    DWORD64 dw64UpgradeVersion;

    DWORD64 dw64TotalSize;

    DWORD cEnclosures;
    APPLICATION_UPDATE_ENCLOSURE* rgEnclosures;
};


struct APPLICATION_UPDATE_CHAIN
{
    LPWSTR wzDefaultApplicationId;
    LPWSTR wzDefaultApplicationType;

    DWORD cEntries;
    APPLICATION_UPDATE_ENTRY* rgEntries;
};


HRESULT DAPI ApupAllocChainFromAtom(
    __in ATOM_FEED* pFeed,
    __out APPLICATION_UPDATE_CHAIN** ppChain
    );

HRESULT DAPI ApupFilterChain(
    __in APPLICATION_UPDATE_CHAIN* pChain,
    __in DWORD64 dw64Version,
    __out APPLICATION_UPDATE_CHAIN** ppFilteredChain
    );

void DAPI ApupFreeChain(
    __in APPLICATION_UPDATE_CHAIN* pChain
    );

#ifdef __cplusplus
}
#endif
