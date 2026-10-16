/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_OS_H
#define MI_INTERNAL_OS_H

#include "../types.h"

void          _mi_os_init(void);                                            // called from process init
void*         _mi_os_alloc(mi_subproc_t* subproc, size_t size, mi_memid_t* memid);
void*         _mi_os_zalloc(mi_subproc_t* subproc, size_t size, mi_memid_t* memid);
void          _mi_os_free(mi_subproc_t* subproc, void* p, size_t size, mi_memid_t memid);
void          _mi_os_free_ex(mi_subproc_t* subproc, void* p, size_t size, bool still_committed, mi_memid_t memid );

size_t        _mi_os_page_size(void);
size_t        _mi_os_guard_page_size(void);
size_t        _mi_os_good_alloc_size(size_t size);
bool          _mi_os_has_overcommit(void);
bool          _mi_os_has_virtual_reserve(void);
bool          _mi_os_canuse_thp(void);
size_t        _mi_os_virtual_address_bits(void);
size_t        _mi_os_minimal_purge_size(void);

bool          _mi_os_reset(mi_subproc_t* subproc, void* addr, size_t size);
bool          _mi_os_decommit(mi_subproc_t* subproc, void* addr, size_t size);
void          _mi_os_reuse(mi_subproc_t* subproc, void* p, size_t size);
mi_decl_nodiscard bool _mi_os_commit(mi_subproc_t* subproc, void* p, size_t size, bool* is_zero);
mi_decl_nodiscard bool _mi_os_commit_ex(mi_subproc_t* subproc, void* addr, size_t size, bool* is_zero, size_t stat_already_committed);
mi_decl_nodiscard bool _mi_os_protect(void* addr, size_t size);
bool          _mi_os_unprotect(void* addr, size_t size);
bool          _mi_os_purge(mi_subproc_t* subproc, void* p, size_t size);
bool          _mi_os_purge_ex(mi_subproc_t* subproc, void* p, size_t size, bool allow_reset, size_t stats_size, mi_commit_fun_t* commit_fun, void* commit_fun_arg);

size_t        _mi_os_secure_guard_page_size(void);
bool          _mi_os_secure_guard_page_set_at(mi_subproc_t* subproc, void* addr, mi_memid_t memid);
bool          _mi_os_secure_guard_page_set_before(mi_subproc_t* subproc, void* addr, mi_memid_t memid);
bool          _mi_os_secure_guard_page_reset_at(mi_subproc_t* subproc, void* addr, mi_memid_t memid);
bool          _mi_os_secure_guard_page_reset_before(mi_subproc_t* subproc, void* addr, mi_memid_t memid);

int           _mi_os_numa_node(void);
int           _mi_os_numa_node_count(void);

void*         _mi_os_alloc_aligned(mi_subproc_t* subproc, size_t size, size_t alignment, bool commit, bool allow_large, mi_memid_t* memid);
void*         _mi_os_alloc_aligned_at_offset(mi_subproc_t* subproc, size_t size, size_t alignment, size_t align_offset, bool commit, bool allow_large, mi_memid_t* memid);

void*         _mi_os_get_aligned_hint(size_t try_alignment, size_t size);
bool          _mi_os_canuse_large_page(size_t size, size_t alignment);
size_t        _mi_os_large_page_size(void);
void*         _mi_os_alloc_huge_os_pages(mi_subproc_t* subproc, size_t pages, int numa_node, mi_msecs_t max_secs, size_t* pages_reserved, size_t* psize, mi_memid_t* memid);

#endif
