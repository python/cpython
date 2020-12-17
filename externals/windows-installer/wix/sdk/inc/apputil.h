#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

// functions

/********************************************************************
AppFreeCommandLineArgs - frees argv from AppParseCommandLine.

********************************************************************/
void DAPI AppFreeCommandLineArgs(
    __in LPWSTR* argv
    );

void DAPI AppInitialize(
    __in_ecount(cSafelyLoadSystemDlls) LPCWSTR rgsczSafelyLoadSystemDlls[],
    __in DWORD cSafelyLoadSystemDlls
    );

/********************************************************************
AppInitializeUnsafe - initializes without the full standard safety
                      precautions for an application.

********************************************************************/
void DAPI AppInitializeUnsafe();

/********************************************************************
AppParseCommandLine - parses the command line using CommandLineToArgvW.
                      The caller must free the value of pArgv on success
                      by calling AppFreeCommandLineArgs.

********************************************************************/
DAPI_(HRESULT) AppParseCommandLine(
    __in LPCWSTR wzCommandLine,
    __in int* argc,
    __in LPWSTR** pArgv
    );

#ifdef __cplusplus
}
#endif
