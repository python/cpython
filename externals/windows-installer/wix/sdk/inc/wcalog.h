#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

BOOL WIXAPI IsVerboseLogging();
HRESULT WIXAPI SetVerboseLoggingAtom(BOOL bValue);

#ifdef __cplusplus
}
#endif
