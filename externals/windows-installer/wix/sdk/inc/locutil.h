#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

struct LOC_STRING
{
    LPWSTR wzId;
    LPWSTR wzText;
    BOOL bOverridable;
};

const int LOC_CONTROL_NOT_SET = INT_MAX;

struct LOC_CONTROL
{
    LPWSTR wzControl;
    int nX;
    int nY;
    int nWidth;
    int nHeight;
    LPWSTR wzText;
};

const int WIX_LOCALIZATION_LANGUAGE_NOT_SET = INT_MAX;

struct WIX_LOCALIZATION
{
    DWORD dwLangId;

    DWORD cLocStrings;
    LOC_STRING* rgLocStrings;

    DWORD cLocControls;
    LOC_CONTROL* rgLocControls;
};

/********************************************************************
 LocProbeForFile - Searches for a localization file on disk.

*******************************************************************/
HRESULT DAPI LocProbeForFile(
    __in_z LPCWSTR wzBasePath,
    __in_z LPCWSTR wzLocFileName,
    __in_z_opt LPCWSTR wzLanguage,
    __inout LPWSTR* psczPath
    );

/********************************************************************
 LocProbeForFileEx - Searches for a localization file on disk.
 useUILanguage should be set to TRUE.
*******************************************************************/
extern "C" HRESULT DAPI LocProbeForFileEx(
    __in_z LPCWSTR wzBasePath,
    __in_z LPCWSTR wzLocFileName,
    __in_z_opt LPCWSTR wzLanguage,
    __inout LPWSTR* psczPath,
    __in BOOL useUILanguage
    );

/********************************************************************
 LocLoadFromFile - Loads a localization file

*******************************************************************/
HRESULT DAPI LocLoadFromFile(
    __in_z LPCWSTR wzWxlFile,
    __out WIX_LOCALIZATION** ppWixLoc
    );

/********************************************************************
 LocLoadFromResource - loads a localization file from a module's data
                       resource.

 NOTE: The resource data must be UTF-8 encoded.
*******************************************************************/
HRESULT DAPI LocLoadFromResource(
    __in HMODULE hModule,
    __in_z LPCSTR szResource,
    __out WIX_LOCALIZATION** ppWixLoc
    );

/********************************************************************
 LocFree - free memory allocated when loading a localization file

*******************************************************************/
void DAPI LocFree(
    __in_opt WIX_LOCALIZATION* pWixLoc
    );

/********************************************************************
 LocLocalizeString - replace any #(loc.id) in a string with the
                    correct sub string
*******************************************************************/
HRESULT DAPI LocLocalizeString(
    __in const WIX_LOCALIZATION* pWixLoc,
    __inout LPWSTR* psczInput
    );

/********************************************************************
 LocGetControl - returns a control's localization information
*******************************************************************/
HRESULT DAPI LocGetControl(
    __in const WIX_LOCALIZATION* pWixLoc,
    __in_z LPCWSTR wzId,
    __out LOC_CONTROL** ppLocControl
    );

/********************************************************************
LocGetString - returns a string's localization information
*******************************************************************/
extern "C" HRESULT DAPI LocGetString(
    __in const WIX_LOCALIZATION* pWixLoc,
    __in_z LPCWSTR wzId,
    __out LOC_STRING** ppLocString
    );

/********************************************************************
LocAddString - adds a localization string
*******************************************************************/
extern "C" HRESULT DAPI LocAddString(
    __in WIX_LOCALIZATION* pWixLoc,
    __in_z LPCWSTR wzId,
    __in_z LPCWSTR wzLocString,
    __in BOOL bOverridable
    );

#ifdef __cplusplus
}
#endif
