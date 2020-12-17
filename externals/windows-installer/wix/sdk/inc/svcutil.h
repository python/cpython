#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif


#define ReleaseServiceHandle(h) if (h) { ::CloseServiceHandle(h); h = NULL; }


HRESULT DAPI SvcQueryConfig(
    __in SC_HANDLE sch,
    __out QUERY_SERVICE_CONFIGW** ppConfig
    );


#ifdef __cplusplus
}
#endif
