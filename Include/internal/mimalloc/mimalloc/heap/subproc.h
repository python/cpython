/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_SUBPROC_H
#define MI_INTERNAL_SUBPROC_H

#include "../types.h"

mi_subproc_t* _mi_subproc_main_init(void);
void          _mi_subproc_main_done(void);
mi_subproc_t* _mi_subproc_main(void);
bool          _mi_subproc_is_main(mi_subproc_t* subproc);
mi_subproc_t* _mi_subproc(void);          // current subproc of this thread
mi_subproc_t* _mi_subproc_from_id(mi_subproc_id_t subproc_id);
void          _mi_subprocs_unsafe_destroy_all(void);
void          _mi_fork_init(void);
void          _mi_fork_child(void);

void*         _mi_meta_zalloc( mi_subproc_t* subproc, size_t size, mi_memid_t* memid );
void*         _mi_meta_rezalloc( mi_subproc_t* subproc, void* p, size_t newsize, mi_memid_t* memid );
void*         _mi_meta_zalloc_aligned( mi_subproc_t* subproc, size_t size, size_t alignment, mi_memid_t* memid );
void          _mi_meta_free(mi_subproc_t* subproc, void* p, mi_memid_t memid);
bool          _mi_meta_is_meta_page(const mi_subproc_t* subproc, const mi_page_t* p);

#endif
