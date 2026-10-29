/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

// --------------------------------------------------------------------------
// This is a "single file include" of the entire mimalloc library.
// Just include this file in your build to link statically with mimalloc.
// This will also override the standard library allocation functions
// (if cmake -DMI_OVERRIDE=ON (or the macro MI_MALLOC_OVERRIDE=1 is defined)).
//
// This is also used for a static override where we create a single object
// file containing the whole library.
// If such object file is linked first it will override all the standard library
// allocation functions (on Unix's).
// --------------------------------------------------------------------------

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#if defined(__sun)
// same remarks as prim/os.c for the static's context.
#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#endif

#include "mimalloc.h"

#include "prim/os.c"
#include "prim/prim.c"            // includes platform specific prim/<platform>/prim.c
#include "prim/prim-tls.c"
#if MI_OSX_ZONE
#include "prim/osx/alloc-override-zone.c"
#endif

#include "util/libc.c"
#include "util/options.c"
#include "util/random.c"
#include "util/stats.c"
#include "util/threadlocal.c"

#include "arena/arena.c"
#include "arena/arena-alloc.c"
#include "arena/arena-page.c"
#include "arena/bitmap-chunk.c"
#include "arena/bitmap.c"

#include "alloc/alloc.c"           // includes alloc-override.c and free.c (for aliasing to work)
#include "alloc/alloc-generic.c"
#include "alloc/alloc-aligned.c"
#include "alloc/alloc-posix.c"
#include "alloc/page.c"
#include "alloc/page-queue.c"

#include "heap/heap.c"
#include "heap/init.c"
#include "heap/page-map.c"
#include "heap/subproc.c"
#include "heap/theap.c"

#include "profile/sample-guarded.c"
#include "profile/sample-profile.c"
#include "profile/pprof.c"
