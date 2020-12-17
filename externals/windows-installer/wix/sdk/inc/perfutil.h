#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

// structs


// functions
void DAPI PerfInitialize(
    );
void DAPI PerfClickTime(
    __out_opt LARGE_INTEGER* pliElapsed
    );
double DAPI PerfConvertToSeconds(
    __in const LARGE_INTEGER* pli
    );

#ifdef __cplusplus
}
#endif
