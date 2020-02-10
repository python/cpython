/* ----------------------------------------------------------------------------
Copyright (c) 2018-2021, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MIMALLOC_TRACK_H
#define MIMALLOC_TRACK_H

// ------------------------------------------------------
// Track memory ranges with macros for tools like Valgrind
// address sanitizer, or other memory checkers.
// ------------------------------------------------------

#if MI_VALGRIND

#define MI_TRACK_ENABLED 1
#define MI_TRACK_TOOL    "valgrind"

#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>

#define mi_track_malloc(p,size,zero)        VALGRIND_MALLOCLIKE_BLOCK(p,size,MI_PADDING_SIZE /*red zone*/,zero)
#define mi_track_resize(p,oldsize,newsize)  VALGRIND_RESIZEINPLACE_BLOCK(p,oldsize,newsize,MI_PADDING_SIZE /*red zone*/)
#define mi_track_free(p)                    VALGRIND_FREELIKE_BLOCK(p,MI_PADDING_SIZE /*red zone*/)
#define mi_track_free_size(p,_size)         mi_track_free(p)
#define mi_track_mem_defined(p,size)        VALGRIND_MAKE_MEM_DEFINED(p,size)
#define mi_track_mem_undefined(p,size)      VALGRIND_MAKE_MEM_UNDEFINED(p,size)
#define mi_track_mem_noaccess(p,size)       VALGRIND_MAKE_MEM_NOACCESS(p,size)

#elif MI_ASAN

#define MI_TRACK_ENABLED 1
#define MI_TRACK_TOOL    "asan"

#include <sanitizer/asan_interface.h>

#define mi_track_malloc(p,size,zero)        ASAN_UNPOISON_MEMORY_REGION(p,size)
#define mi_track_resize(p,oldsize,newsize)  ASAN_POISON_MEMORY_REGION(p,oldsize); ASAN_UNPOISON_MEMORY_REGION(p,newsize)
#define mi_track_free(p)                    ASAN_POISON_MEMORY_REGION(p,mi_usable_size(p))
#define mi_track_free_size(p,size)          ASAN_POISON_MEMORY_REGION(p,size)
#define mi_track_mem_defined(p,size)        ASAN_UNPOISON_MEMORY_REGION(p,size)
#define mi_track_mem_undefined(p,size)      ASAN_UNPOISON_MEMORY_REGION(p,size)
#define mi_track_mem_noaccess(p,size)       ASAN_POISON_MEMORY_REGION(p,size)

#else

#define MI_TRACK_ENABLED 0
#define MI_TRACK_TOOL    "none"

#define mi_track_malloc(p,size,zero)
#define mi_track_resize(p,oldsize,newsize)
#define mi_track_free(p)
#define mi_track_free_size(p,_size)
#define mi_track_mem_defined(p,size)
#define mi_track_mem_undefined(p,size)
#define mi_track_mem_noaccess(p,size)

#endif

#endif
