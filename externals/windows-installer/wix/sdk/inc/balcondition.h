#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BAL_CONDITION
{
    LPWSTR sczCondition;
    LPWSTR sczMessage;
} BAL_CONDITION;


typedef struct _BAL_CONDITIONS
{
    BAL_CONDITION* rgConditions;
    DWORD cConditions;
} BAL_CONDITIONS;


/*******************************************************************
 BalConditionsParseFromXml - loads the conditions from the UX manifest.

********************************************************************/
DAPI_(HRESULT) BalConditionsParseFromXml(
    __in BAL_CONDITIONS* pConditions,
    __in IXMLDOMDocument* pixdManifest,
    __in_opt WIX_LOCALIZATION* pWixLoc
    );


/*******************************************************************
 BalConditionEvaluate - evaluates condition against the provided IBurnCore.

 NOTE: psczMessage is optional.
********************************************************************/
DAPI_(HRESULT) BalConditionEvaluate(
    __in BAL_CONDITION* pCondition,
    __in IBootstrapperEngine* pEngine,
    __out BOOL* pfResult,
    __out_z_opt LPWSTR* psczMessage
    );


/*******************************************************************
 BalConditionsUninitialize - uninitializes any conditions previously loaded.

********************************************************************/
DAPI_(void) BalConditionsUninitialize(
    __in BAL_CONDITIONS* pConditions
    );


#ifdef __cplusplus
}
#endif
