/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_THREADLOCAL_H
#define MI_INTERNAL_THREADLOCAL_H

#include "../types.h"

#define mi_thread_local_key_fast  ((mi_thread_local_t)1)

mi_thread_local_t _mi_thread_local_create(void);
void  _mi_thread_local_free( mi_thread_local_t key );
bool  _mi_thread_local_set(  mi_thread_local_t key, void* val );
void* _mi_thread_local_get(  mi_thread_local_t key );
void  _mi_thread_locals_init(void);
void  _mi_thread_locals_done(void);
void  _mi_thread_locals_thread_done(void);

#endif
