/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_H
#define MI_INTERNAL_H

// --------------------------------------------------------------------------
// This file includes all the internal API's of mimalloc for convenience.
// (but is not used by mimalloc itself)
// --------------------------------------------------------------------------

#include "types.h"
#include "track.h"

// --------------------------------------------------------------------------
// Internal functions
// --------------------------------------------------------------------------

#include "prim/os.h"
#include "prim/prim-tls.h"

#include "util/libc.h"
#include "util/options.h"
#include "util/random.h"
#include "util/stats.h"
#include "util/threadlocal.h"

#include "arena/arena.h"
#include "arena/arena-alloc.h"
#include "arena/arena-page.h"

#include "heap/page-map.h"
#include "heap/subproc.h"
#include "heap/init.h"
#include "heap/theap.h"
#include "heap/heap.h"

#include "alloc/page.h"
#include "alloc/alloc.h"
#include "alloc/free.h"

#include "profile/sample-profile.h"
#include "profile/sample-guarded.h"
#include "profile/pprof.h"

#endif  // MI_INTERNAL_H
