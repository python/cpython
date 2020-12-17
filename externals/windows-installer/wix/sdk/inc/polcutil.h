#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

const LPCWSTR POLICY_BURN_REGISTRY_PATH = L"WiX\\Burn";

/********************************************************************
PolcReadNumber - reads a number from policy.

NOTE: S_FALSE returned if policy not set.
NOTE: out is set to default on S_FALSE or any error.
********************************************************************/
HRESULT DAPI PolcReadNumber(
    __in_z LPCWSTR wzPolicyPath,
    __in_z LPCWSTR wzPolicyName,
    __in DWORD dwDefault,
    __out DWORD* pdw
    );

/********************************************************************
PolcReadString - reads a string from policy.

NOTE: S_FALSE returned if policy not set.
NOTE: out is set to default on S_FALSE or any error.
********************************************************************/
HRESULT DAPI PolcReadString(
    __in_z LPCWSTR wzPolicyPath,
    __in_z LPCWSTR wzPolicyName,
    __in_z_opt LPCWSTR wzDefault,
    __deref_out_z LPWSTR* pscz
    );

#ifdef __cplusplus
}
#endif
