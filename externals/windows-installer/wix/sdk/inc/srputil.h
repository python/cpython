#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif


typedef enum SRP_ACTION
{
    SRP_ACTION_UNKNOWN,
    SRP_ACTION_UNINSTALL,
    SRP_ACTION_INSTALL,
    SRP_ACTION_MODIFY,
} SRP_ACTION;


/********************************************************************
 SrpInitialize - initializes system restore point functionality.

*******************************************************************/
DAPI_(HRESULT) SrpInitialize(
    __in BOOL fInitializeComSecurity
    );

/********************************************************************
 SrpUninitialize - uninitializes system restore point functionality.

*******************************************************************/
DAPI_(void) SrpUninitialize();

/********************************************************************
 SrpCreateRestorePoint - creates a system restore point.

*******************************************************************/
DAPI_(HRESULT) SrpCreateRestorePoint(
    __in_z LPCWSTR wzApplicationName,
    __in SRP_ACTION action
    );

#ifdef __cplusplus
}
#endif

