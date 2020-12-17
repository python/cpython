#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#include "wininet.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum URI_PROTOCOL
{
    URI_PROTOCOL_UNKNOWN,
    URI_PROTOCOL_FILE,
    URI_PROTOCOL_FTP,
    URI_PROTOCOL_HTTP,
    URI_PROTOCOL_HTTPS,
    URI_PROTOCOL_LOCAL,
    URI_PROTOCOL_UNC
} URI_PROTOCOL;

typedef struct _URI_INFO
{
    INTERNET_SCHEME scheme;
    LPWSTR sczHostName;
    INTERNET_PORT port;
    LPWSTR sczUser;
    LPWSTR sczPassword;
    LPWSTR sczPath;
    LPWSTR sczQueryString;
} URI_INFO;


HRESULT DAPI UriCanonicalize(
    __inout_z LPWSTR* psczUri
    );

HRESULT DAPI UriCrack(
    __in_z LPCWSTR wzUri,
    __out_opt INTERNET_SCHEME* pScheme,
    __deref_opt_out_z LPWSTR* psczHostName,
    __out_opt INTERNET_PORT* pPort,
    __deref_opt_out_z LPWSTR* psczUser,
    __deref_opt_out_z LPWSTR* psczPassword,
    __deref_opt_out_z LPWSTR* psczPath,
    __deref_opt_out_z LPWSTR* psczQueryString
    );

HRESULT DAPI UriCrackEx(
    __in_z LPCWSTR wzUri,
    __in URI_INFO* pUriInfo
    );

void DAPI UriInfoUninitialize(
    __in URI_INFO* pUriInfo
    );

HRESULT DAPI UriCreate(
    __inout_z LPWSTR* psczUri,
    __in INTERNET_SCHEME scheme,
    __in_z_opt LPWSTR wzHostName,
    __in INTERNET_PORT port,
    __in_z_opt LPWSTR wzUser,
    __in_z_opt LPWSTR wzPassword,
    __in_z_opt LPWSTR wzPath,
    __in_z_opt LPWSTR wzQueryString
    );

HRESULT DAPI UriCanonicalize(
    __inout_z LPWSTR* psczUri
    );

HRESULT DAPI UriFile(
    __deref_out_z LPWSTR* psczFile,
    __in_z LPCWSTR wzUri
    );

HRESULT DAPI UriProtocol(
    __in_z LPCWSTR wzUri,
    __out URI_PROTOCOL* pProtocol
    );

HRESULT DAPI UriRoot(
    __in_z LPCWSTR wzUri,
    __out LPWSTR* ppwzRoot,
    __out_opt URI_PROTOCOL* pProtocol
    );

HRESULT DAPI UriResolve(
    __in_z LPCWSTR wzUri,
    __in_opt LPCWSTR wzBaseUri,
    __out LPWSTR* ppwzResolvedUri,
    __out_opt const URI_PROTOCOL* pResolvedProtocol
    );

#ifdef __cplusplus
}
#endif

