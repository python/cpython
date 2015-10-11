//-------------------------------------------------------------------------------------------------
// <copyright file="precomp.h" company="Outercurve Foundation">
//   Copyright (c) 2004, Outercurve Foundation.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the root directory of the distribution.
// </copyright>
// 
// <summary>
//    Precompiled header for standard bootstrapper application.
// </summary>
//-------------------------------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <Uxtheme.h>
#include <msiquery.h>
#include <objbase.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <strsafe.h>
#include <stddef.h>
#include <versionhelpers.h>

#include "dutil.h"
#include "memutil.h"
#include "dictutil.h"
#include "dirutil.h"
#include "fileutil.h"
#include "locutil.h"
#include "logutil.h"
#include "pathutil.h"
#include "resrutil.h"
#include "shelutil.h"
#include "strutil.h"
#include "thmutil.h"
#include "uriutil.h"
#include "xmlutil.h"

#include "IBootstrapperEngine.h"
#include "IBootstrapperApplication.h"

#include "BalBaseBootstrapperApplication.h"
#include "balinfo.h"
#include "balcondition.h"

HRESULT CreateBootstrapperApplication(
    __in HMODULE hModule,
    __in BOOL fPrereq,
    __in HRESULT hrHostInitialization,
    __in IBootstrapperEngine* pEngine,
    __in const BOOTSTRAPPER_COMMAND* pCommand,
    __out IBootstrapperApplication** ppApplication
);

#include "IBootstrapperBAFunction.h"

