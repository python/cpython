#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

enum BUNDLE_INSTALL_CONTEXT
{
    BUNDLE_INSTALL_CONTEXT_MACHINE,
    BUNDLE_INSTALL_CONTEXT_USER,
};

HRESULT DAPI BundleGetBundleInfo(
  __in LPCWSTR   szBundleId,                             // Bundle code
  __in LPCWSTR   szAttribute,                            // attribute name
  __out_ecount_opt(*pcchValueBuf) LPWSTR lpValueBuf,     // returned value, NULL if not desired
  __inout_opt                     LPDWORD pcchValueBuf   // in/out buffer character count
    );

HRESULT DAPI BundleEnumRelatedBundle(
  __in     LPCWSTR lpUpgradeCode,
  __in     BUNDLE_INSTALL_CONTEXT context,
  __inout  PDWORD pdwStartIndex,
  __out_ecount(MAX_GUID_CHARS+1)  LPWSTR lpBundleIdBuf
    );

#ifdef __cplusplus
}
#endif
