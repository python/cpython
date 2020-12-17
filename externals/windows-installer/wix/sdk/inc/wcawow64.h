#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#include "wcautil.h"

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WIXAPI WcaInitializeWow64();
BOOL WIXAPI WcaIsWow64Process();
BOOL WIXAPI WcaIsWow64Initialized();
HRESULT WIXAPI WcaDisableWow64FSRedirection();
HRESULT WIXAPI WcaRevertWow64FSRedirection();
HRESULT WIXAPI WcaFinalizeWow64();

#ifdef __cplusplus
}
#endif
