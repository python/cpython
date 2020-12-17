#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#if defined(__cplusplus)
extern "C" {
#endif

HRESULT DAPI WuaPauseAutomaticUpdates();

HRESULT DAPI WuaResumeAutomaticUpdates();

HRESULT DAPI WuaRestartRequired(
    __out BOOL* pfRestartRequired
    );

#if defined(__cplusplus)
}
#endif
