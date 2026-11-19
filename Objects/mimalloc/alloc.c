/* ----------------------------------------------------------------------------
Copyright (c) 2018-2024, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE   // for realpath() on Linux
#endif

#include "mimalloc.h"
#include "mimalloc/internal.h"
#include "mimalloc/atomic.h"
#include "mimalloc/prim.h"   // _mi_prim_thread_id()

#include <string.h>      // memset, strlen (for mi_strdup)
#include <stdlib.h>      // malloc, abort

// CPython: don't export the weak C++ `std::get_new_handler` symbol from libpython
#define _ZSt15get_new_handlerv _Py__ZSt15get_new_handlerv

#if (MI_DEBUG>0)
// CPython: fill freed or uninitialized memory but preserve the first `page->debug_offset`
// bytes (e.g. `ob_tid` and the reference count fields that lock-free readers may still access)
static inline void mi_debug_fill(mi_page_t* page, mi_block_t* block, int c, size_t size) {
  size_t offset = (size_t)page->debug_offset;
  if (offset < size) {
    memset((char*)block + offset, c, size - offset);
  }
}
#endif

#define MI_IN_ALLOC_C
#include "alloc-override.c"
#include "free.c"
#undef MI_IN_ALLOC_C

// ------------------------------------------------------
// Allocation
// ------------------------------------------------------

// Fast allocation in a page: just pop from the free list.
// Fall back to generic allocation only if the list is empty.
// Note: in release mode the (inlined) routine is about 7 instructions with a single test.
extern inline void* _mi_page_malloc_zero(mi_heap_t* heap, mi_page_t* page, size_t size, bool zero, mi_page_t** ppage) mi_attr_noexcept
{
  mi_assert_internal(size >= MI_PADDING_SIZE);
  mi_assert_internal(page->block_size == 0 /* empty heap */ || mi_page_block_size(page) >= size);

  // check the free list
  mi_block_t* const block = page->free;
  if mi_unlikely(block == NULL) {
    return _mi_malloc_generic(heap, size, zero, 0, ppage);
  }
  mi_assert_internal(block != NULL && _mi_ptr_page(block) == page);
  if (ppage != NULL) { *ppage = page; };
  // pop from the free list
  page->free = mi_block_next(page, block);
  page->used++;
  mi_assert_internal(page->free == NULL || _mi_ptr_page(page->free) == page);
  mi_assert_internal(page->block_size < MI_MAX_ALIGN_SIZE || _mi_is_aligned(block, MI_MAX_ALIGN_SIZE));

  #if MI_DEBUG>3
  if (page->free_is_zero && size > sizeof(*block)) {
    mi_assert_expensive(mi_mem_is_zero(block+1,size - sizeof(*block)));
  }
  #endif

  // allow use of the block internally
  // note: when tracking we need to avoid ever touching the MI_PADDING since
  // that is tracked by valgrind etc. as non-accessible (through the red-zone, see `mimalloc/track.h`)
  mi_track_mem_undefined(block, mi_page_usable_block_size(page));

  // zero the block? note: we need to zero the full block size (issue #63)
  if mi_unlikely(zero) {
    mi_assert_internal(page->block_size != 0); // do not call with zero'ing for huge blocks (see _mi_malloc_generic)
    #if MI_PADDING
    mi_assert_internal(page->block_size >= MI_PADDING_SIZE);
    #endif
    if (page->free_is_zero) {
      block->next = 0;
      mi_track_mem_defined(block, page->block_size - MI_PADDING_SIZE);
    }
    else {
      _mi_memzero_aligned(block, page->block_size - MI_PADDING_SIZE);
    }
  }

  #if (MI_DEBUG>0) && !MI_TRACK_ENABLED && !MI_TSAN
  if (!zero && !mi_page_is_huge(page)) {
    mi_debug_fill(page, block, MI_DEBUG_UNINIT, mi_page_usable_block_size(page));
  }
  #elif (MI_SECURE!=0)
  if (!zero) { block->next = 0; } // don't leak internal data
  #endif

  #if (MI_STAT>0)
  const size_t bsize = mi_page_usable_block_size(page);
  if (bsize <= MI_MEDIUM_OBJ_SIZE_MAX) {
    mi_heap_stat_increase(heap, malloc_normal, bsize);
    mi_heap_stat_counter_increase(heap, malloc_normal_count, 1);
    #if (MI_STAT>1)
    const size_t bin = _mi_bin(bsize);
    mi_heap_stat_increase(heap, malloc_bins[bin], 1);
    mi_heap_stat_increase(heap, malloc_requested, size - MI_PADDING_SIZE);
    #endif
  }
  #endif

  #if MI_PADDING // && !MI_TRACK_ENABLED
    mi_padding_t* const padding = (mi_padding_t*)((uint8_t*)block + mi_page_usable_block_size(page));
    ptrdiff_t delta = ((uint8_t*)padding - (uint8_t*)block - (size - MI_PADDING_SIZE));
    #if (MI_DEBUG>=2)
    mi_assert_internal(delta >= 0 && mi_page_usable_block_size(page) >= (size - MI_PADDING_SIZE + delta));
    #endif
    mi_track_mem_defined(padding,sizeof(mi_padding_t));  // note: re-enable since mi_page_usable_block_size may set noaccess
    padding->canary = mi_ptr_encode_canary(page,block,page->keys);
    padding->delta  = (uint32_t)(delta);
    #if MI_PADDING_CHECK
    if (!mi_page_is_huge(page)) {
      uint8_t* fill = (uint8_t*)padding - delta;
      const size_t maxpad = (delta > MI_MAX_ALIGN_SIZE ? MI_MAX_ALIGN_SIZE : delta); // set at most N initial padding bytes
      for (size_t i = 0; i < maxpad; i++) { fill[i] = MI_DEBUG_PADDING; }
    }
    #endif
  #endif

  return block;
}

// extra entries for improved efficiency in `alloc-aligned.c`.
extern void* _mi_page_malloc(mi_heap_t* heap, mi_page_t* page, size_t size) mi_attr_noexcept {
  return _mi_page_malloc_zero(heap,page,size,false,NULL);
}
extern void* _mi_page_malloc_zeroed(mi_heap_t* heap, mi_page_t* page, size_t size) mi_attr_noexcept {
  return _mi_page_malloc_zero(heap,page,size,true,NULL);
}

static inline mi_decl_restrict void* mi_heap_malloc_small_zero(mi_heap_t* heap, size_t size, bool zero, mi_page_t** ppage) mi_attr_noexcept {
  mi_assert(heap != NULL);
  mi_assert(size <= MI_SMALL_SIZE_MAX);
  #if MI_DEBUG
  const uintptr_t tid = _mi_thread_id();
  mi_assert(heap->thread_id == 0 || heap->thread_id == tid); // heaps are thread local
  #endif
  #if (MI_PADDING || MI_GUARDED)
  if (size == 0) { size = sizeof(void*); }
  #endif
  #if MI_GUARDED
  if (mi_heap_malloc_use_guarded(heap,size)) {
    return _mi_heap_malloc_guarded(heap, size, zero, ppage);
  }
  #endif

  // get page in constant time, and allocate from it
  mi_page_t* page = _mi_heap_get_free_small_page(heap, size + MI_PADDING_SIZE);
  void* const p = _mi_page_malloc_zero(heap, page, size + MI_PADDING_SIZE, zero, ppage);
  mi_track_malloc(p,size,zero);

  #if MI_DEBUG>3
  if (p != NULL && zero) {
    mi_assert_expensive(mi_mem_is_zero(p, size));
  }
  #endif
  return p;
}

// allocate a small block
mi_decl_nodiscard extern inline mi_decl_restrict void* mi_heap_malloc_small(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  return mi_heap_malloc_small_zero(heap, size, false, NULL);
}

mi_decl_nodiscard extern inline mi_decl_restrict void* mi_heap_zalloc_small(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  return mi_heap_malloc_small_zero(heap, size, true, NULL);
}

mi_decl_nodiscard extern inline mi_decl_restrict void* mi_malloc_small(size_t size) mi_attr_noexcept {
  return mi_heap_malloc_small(mi_prim_get_default_heap(), size);
}

// The main allocation function
extern inline void* _mi_heap_malloc_zero_ex(mi_heap_t* heap, size_t size, bool zero, size_t huge_alignment, mi_page_t** ppage) mi_attr_noexcept {
  // fast path for small objects
  if mi_likely(size <= MI_SMALL_SIZE_MAX) {
    mi_assert_internal(huge_alignment == 0);
    return mi_heap_malloc_small_zero(heap, size, zero, ppage);
  }
  #if MI_GUARDED
  else if (huge_alignment==0 && mi_heap_malloc_use_guarded(heap,size)) {
    return _mi_heap_malloc_guarded(heap, size, zero, ppage);
  }
  #endif
  else {
    // regular allocation
    mi_assert(heap!=NULL);
    mi_assert(heap->thread_id == 0 || heap->thread_id == _mi_thread_id());   // heaps are thread local
    void* const p = _mi_malloc_generic(heap, size + MI_PADDING_SIZE, zero, huge_alignment, ppage);  // note: size can overflow but it is detected in malloc_generic
    mi_track_malloc(p,size,zero);

    #if MI_DEBUG>3
    if (p != NULL && zero) {
      mi_assert_expensive(mi_mem_is_zero(p, size));
    }
    #endif
    return p;
  }
}

extern inline void* _mi_heap_malloc_zero(mi_heap_t* heap, size_t size, bool zero) mi_attr_noexcept {
  return _mi_heap_malloc_zero_ex(heap, size, zero, 0, NULL);
}

mi_decl_nodiscard extern inline mi_decl_restrict void* mi_heap_malloc(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  return _mi_heap_malloc_zero(heap, size, false);
}

mi_decl_nodiscard extern inline mi_decl_restrict void* mi_malloc(size_t size) mi_attr_noexcept {
  return mi_heap_malloc(mi_prim_get_default_heap(), size);
}

// zero initialized small block
mi_decl_nodiscard mi_decl_restrict void* mi_zalloc_small(size_t size) mi_attr_noexcept {
  return mi_heap_zalloc_small(mi_prim_get_default_heap(), size);
}

mi_decl_nodiscard extern inline mi_decl_restrict void* mi_heap_zalloc(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  return _mi_heap_malloc_zero(heap, size, true);
}

mi_decl_nodiscard mi_decl_restrict void* mi_zalloc(size_t size) mi_attr_noexcept {
  return mi_heap_zalloc(mi_prim_get_default_heap(),size);
}


mi_decl_nodiscard extern inline mi_decl_restrict void* mi_heap_calloc(mi_heap_t* heap, size_t count, size_t size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count,size,&total)) return NULL;
  return mi_heap_zalloc(heap,total);
}

mi_decl_nodiscard mi_decl_restrict void* mi_calloc(size_t count, size_t size) mi_attr_noexcept {
  return mi_heap_calloc(mi_prim_get_default_heap(),count,size);
}

static void* mi_ublock_size( void* p, mi_page_t* page, size_t* pblock_size ) {
  mi_assert_internal(page == _mi_ptr_page(p));
  if (pblock_size!=NULL) {
    if (p!=NULL) { *pblock_size = mi_page_block_size(page); }
  }
  return p;
}

// Return the block size
mi_decl_nodiscard mi_decl_restrict void* mi_umalloc_small(size_t size, size_t* pblock_size) mi_attr_noexcept {
  mi_page_t* page;
  void* p = mi_heap_malloc_small_zero(mi_prim_get_default_heap(), size, false, &page);
  return mi_ublock_size(p,page,pblock_size);
}

mi_decl_nodiscard mi_decl_restrict void* mi_uzalloc_small(size_t size, size_t* pblock_size) mi_attr_noexcept {
  mi_page_t* page;
  void* p = mi_heap_malloc_small_zero(mi_prim_get_default_heap(), size, true, &page);
  return mi_ublock_size(p,page,pblock_size);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_umalloc(mi_heap_t* heap, size_t size, size_t* pblock_size) mi_attr_noexcept {
  mi_page_t* page;
  void* p = _mi_heap_malloc_zero_ex(heap, size, false, 0, &page);
  return mi_ublock_size(p,page,pblock_size);
}

mi_decl_nodiscard mi_decl_restrict void* mi_umalloc(size_t size, size_t* pblock_size) mi_attr_noexcept {
  return mi_heap_umalloc(mi_prim_get_default_heap(), size, pblock_size);
}

mi_decl_nodiscard mi_decl_restrict void* mi_uzalloc(size_t size, size_t* pblock_size) mi_attr_noexcept {
  mi_page_t* page;
  void* p = _mi_heap_malloc_zero_ex(mi_prim_get_default_heap(), size, true, 0, &page);
  return mi_ublock_size(p,page,pblock_size);
}

mi_decl_nodiscard mi_decl_restrict void* mi_ucalloc(size_t count, size_t size, size_t* pblock_size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count,size,&total)) return NULL;
  return mi_uzalloc(total, pblock_size);
}

// Uninitialized `calloc`
mi_decl_nodiscard extern mi_decl_restrict void* mi_heap_mallocn(mi_heap_t* heap, size_t count, size_t size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_heap_malloc(heap, total);
}

mi_decl_nodiscard mi_decl_restrict void* mi_mallocn(size_t count, size_t size) mi_attr_noexcept {
  return mi_heap_mallocn(mi_prim_get_default_heap(),count,size);
}

// Expand (or shrink) in place (or fail)
void* mi_expand(void* p, size_t newsize) mi_attr_noexcept {
  #if MI_PADDING
  // we do not shrink/expand with padding enabled
  MI_UNUSED(p); MI_UNUSED(newsize);
  return NULL;
  #else
  if (p == NULL) return NULL;
  const mi_page_t* const page = mi_validate_ptr_page(p,"mi_expand");
  const size_t size = _mi_page_usable_size(page,p);
  if (newsize > size) return NULL;
  return p; // it fits
  #endif
}

void* _mi_heap_realloc_zero(mi_heap_t* heap, void* p, size_t newsize, bool zero, size_t* pblock_size_pre, size_t* pblock_size_post) mi_attr_noexcept {
  // if p == NULL then behave as malloc.
  // else if size == 0 then reallocate to a zero-sized block (and don't return NULL, just as mi_malloc(0)).
  // (this means that returning NULL always indicates an error, and `p` will not have been freed in that case.)
  const mi_page_t* page;
  size_t size;
  if (p==NULL) {
    page = NULL;
    size = 0;
    if (pblock_size_pre!=NULL) { *pblock_size_pre = 0; }
  }
  else {
    page = mi_validate_ptr_page(p,"mi_realloc");
    if mi_unlikely(page==NULL) {  // invalid pointer
      if (pblock_size_pre!=NULL) { *pblock_size_pre = 0; }
      if (pblock_size_post!=NULL) { *pblock_size_post = 0; }
      return NULL;
    }
    size = _mi_page_usable_size(page,p);
    if (pblock_size_pre!=NULL) { *pblock_size_pre = mi_page_block_size(page); }
  }
  if mi_unlikely(newsize <= size && newsize >= (size / 2) && newsize > 0) {  // note: newsize must be > 0 or otherwise we return NULL for realloc(NULL,0)
    mi_assert_internal(p!=NULL);
    // todo: do not track as the usable size is still the same in the free; adjust potential padding?
    // mi_track_resize(p,size,newsize)
    // if (newsize < size) { mi_track_mem_noaccess((uint8_t*)p + newsize, size - newsize); }
    if (pblock_size_post!=NULL) { *pblock_size_post = mi_page_block_size(page); }
    return p;  // reallocation still fits and not more than 50% waste
  }
  // note: we don't zero allocate upfront so we only zero initialize the expanded part
  mi_page_t* newpage; // use block_size for zero-ing, issue #763
  void* const newp = _mi_heap_malloc_zero_ex(heap,newsize,false /* no zero */,0,&newpage);
  if mi_likely(newp != NULL) {
    if (pblock_size_post!=NULL) { *pblock_size_post = mi_page_block_size(newpage); }
    const size_t copy_size  = (newsize > size ? size : newsize);
    const size_t zero_start = _mi_align_down( (copy_size >= sizeof(intptr_t) ? copy_size - sizeof(intptr_t) : 0), sizeof(intptr_t)); // also set last word in the previous allocation to zero to ensure any padding is zero-initialized
    const size_t usable = _mi_page_usable_size(newpage,newp);
    mi_assert_internal(usable >= newsize);
    if (zero && usable > zero_start) {
      _mi_memzero_aligned((uint8_t*)newp + zero_start, usable - zero_start);
    }
    else if (newsize == 0) {
      ((uint8_t*)newp)[0] = 0; // work around for applications that expect zero-reallocation to be zero initialized (issue #725)
    }
    if mi_likely(p != NULL) {
      mi_track_mem_defined(p,copy_size);  // _mi_useable_size may be too large for byte precise memory tracking..
      _mi_memcpy_aligned(newp, p, copy_size);
      mi_free(p); // only free the original pointer if successful
    }
  }
  return newp;
}

mi_decl_nodiscard void* mi_heap_realloc(mi_heap_t* heap, void* p, size_t newsize) mi_attr_noexcept {
  return _mi_heap_realloc_zero(heap, p, newsize, false, NULL, NULL);
}

mi_decl_nodiscard void* mi_heap_reallocn(mi_heap_t* heap, void* p, size_t count, size_t size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_heap_realloc(heap, p, total);
}


// Reallocate but free `p` on errors
mi_decl_nodiscard void* mi_heap_reallocf(mi_heap_t* heap, void* p, size_t newsize) mi_attr_noexcept {
  void* newp = mi_heap_realloc(heap, p, newsize);
  if (newp==NULL && p!=NULL) mi_free(p);
  return newp;
}

mi_decl_nodiscard void* mi_heap_rezalloc(mi_heap_t* heap, void* p, size_t newsize) mi_attr_noexcept {
  return _mi_heap_realloc_zero(heap, p, newsize, true, NULL, NULL);
}

mi_decl_nodiscard void* mi_heap_recalloc(mi_heap_t* heap, void* p, size_t count, size_t size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_heap_rezalloc(heap, p, total);
}


mi_decl_nodiscard void* mi_realloc(void* p, size_t newsize) mi_attr_noexcept {
  return mi_heap_realloc(mi_prim_get_default_heap(),p,newsize);
}

mi_decl_nodiscard void* mi_reallocn(void* p, size_t count, size_t size) mi_attr_noexcept {
  return mi_heap_reallocn(mi_prim_get_default_heap(),p,count,size);
}

mi_decl_nodiscard void* mi_urealloc(void* p, size_t newsize, size_t* pblock_size_pre, size_t* pblock_size_post) mi_attr_noexcept {
  return _mi_heap_realloc_zero(mi_prim_get_default_heap(),p,newsize, false, pblock_size_pre, pblock_size_post);
}

// Reallocate but free `p` on errors
mi_decl_nodiscard void* mi_reallocf(void* p, size_t newsize) mi_attr_noexcept {
  return mi_heap_reallocf(mi_prim_get_default_heap(),p,newsize);
}

mi_decl_nodiscard void* mi_rezalloc(void* p, size_t newsize) mi_attr_noexcept {
  return mi_heap_rezalloc(mi_prim_get_default_heap(), p, newsize);
}

mi_decl_nodiscard void* mi_recalloc(void* p, size_t count, size_t size) mi_attr_noexcept {
  return mi_heap_recalloc(mi_prim_get_default_heap(), p, count, size);
}



// ------------------------------------------------------
// strdup, strndup, and realpath
// ------------------------------------------------------

// `strdup` using mi_malloc
mi_decl_nodiscard mi_decl_restrict char* mi_heap_strdup(mi_heap_t* heap, const char* s) mi_attr_noexcept {
  if (s == NULL) return NULL;
  size_t len = _mi_strlen(s);
  if (len > MI_MAX_ALLOC_SIZE - 1) return NULL;  // prevent overflow on len+1
  char* t = (char*)mi_heap_malloc(heap,len+1);
  if (t == NULL) return NULL;
  _mi_memcpy(t, s, len);
  t[len] = 0;
  return t;
}

mi_decl_nodiscard mi_decl_restrict char* mi_strdup(const char* s) mi_attr_noexcept {
  return mi_heap_strdup(mi_prim_get_default_heap(), s);
}

// `strndup` using mi_malloc
mi_decl_nodiscard mi_decl_restrict char* mi_heap_strndup(mi_heap_t* heap, const char* s, size_t n) mi_attr_noexcept {
  if (s == NULL) return NULL;
  const size_t len = _mi_strnlen(s,n);  // len <= n
  if (len > MI_MAX_ALLOC_SIZE - 1) return NULL;  // prevent overflow on len+1
  char* t = (char*)mi_heap_malloc(heap, len+1);
  if (t == NULL) return NULL;
  _mi_memcpy(t, s, len);
  t[len] = 0;
  return t;
}

mi_decl_nodiscard mi_decl_restrict char* mi_strndup(const char* s, size_t n) mi_attr_noexcept {
  return mi_heap_strndup(mi_prim_get_default_heap(),s,n);
}

#ifndef __wasi__
// `realpath` using mi_malloc
#ifdef _WIN32
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

mi_decl_nodiscard mi_decl_restrict char* mi_heap_realpath(mi_heap_t* heap, const char* fname, char* resolved_name) mi_attr_noexcept {
  // todo: use GetFullPathNameW to allow longer file names
  if (fname==NULL || *fname==0) {
    errno = EINVAL;
    return NULL;
  }
  char buf[PATH_MAX];
  DWORD res = GetFullPathNameA(fname, PATH_MAX, (resolved_name == NULL ? buf : resolved_name), NULL);
  if (res == 0) {
    DWORD err = GetLastError();
    switch (err) {
      case ERROR_LOCK_VIOLATION:
      case ERROR_SHARING_VIOLATION:
      case ERROR_INVALID_ACCESS:    errno = EACCES; break;
      case ERROR_INVALID_HANDLE:
      case ERROR_INVALID_FUNCTION:  errno = EINVAL; break;
      case ERROR_PATH_NOT_FOUND:    errno = ENOTDIR; break;
      case ERROR_FILE_NOT_FOUND:    errno = ENOENT; break;
      case ERROR_NOT_ENOUGH_MEMORY: errno = ENOMEM; break;
      default:                      errno = EIO;
    }
    return NULL;
  }
  else if (res > PATH_MAX) {
    errno = ENAMETOOLONG;
    return NULL;
  }
  else if (resolved_name != NULL) {
    return resolved_name;
  }
  else {
    return mi_heap_strndup(heap, buf, PATH_MAX);
  }
}

#else

#include <unistd.h>  // pathconf

static size_t mi_path_max(void) {
  static _Atomic(size_t) path_max = 0;
  size_t pmax = mi_atomic_load_acquire(&path_max);
  if (pmax == 0) {
    long m = 0;
    #ifdef _PC_PATH_MAX
    m = pathconf("/",_PC_PATH_MAX);
    #endif
    if (m <= 0) pmax = 4096;      // guess
    else if (m < 256) pmax = 256; // at least 256
    else if (m > 64*1024) pmax = 64*1024;  // at most 64 KiB
    else pmax = m;
    size_t expected = 0;
    mi_atomic_cas_strong_acq_rel(&path_max, &expected, pmax);
  }
  return pmax;
}

char* mi_heap_realpath(mi_heap_t* heap, const char* fname, char* resolved_name) mi_attr_noexcept {
  if (resolved_name != NULL) {
    return realpath(fname,resolved_name);
  }
  else {
  /*
    char* rname = realpath(fname, NULL);
    if (rname == NULL) return NULL;
    char* result = mi_heap_strdup(heap, rname);
    mi_cfree(rname);  // note: may leak the original pointer if allocated internally with the system allocator
    // note: with ASAN realpath is intercepted and mi_cfree may leak the returned pointer :-(
    return result;
  */
    const size_t n  = mi_path_max();
    char* const buf = (char*)mi_zalloc(n+1);
    if (buf == NULL) {
      errno = ENOMEM;
      return NULL;
    }
    char* rname  = realpath(fname,buf);
    char* result = mi_heap_strndup(heap,rname,n); // ok if `rname==NULL`
    mi_free(buf);
    return result;
  }
}
#endif

mi_decl_nodiscard mi_decl_restrict char* mi_realpath(const char* fname, char* resolved_name) mi_attr_noexcept {
  return mi_heap_realpath(mi_prim_get_default_heap(),fname,resolved_name);
}
#endif

/*-------------------------------------------------------
C++ new and new_aligned
The standard requires calling into `get_new_handler` and
throwing the bad_alloc exception on failure. If we compile
with a C++ compiler we can implement this precisely. If we
use a C compiler we cannot throw a `bad_alloc` exception
but we call `abort` instead (i.e. not returning).
Also, the standard requires calling the new handler until
it returns false, but we limit the total calls.
-------------------------------------------------------*/
#define MI_TRY_NEW_MAX (4)

#ifdef __cplusplus
#include <new>
static bool mi_try_new_handler(bool nothrow) {
  #if defined(_MSC_VER) || (__cplusplus >= 201103L)
    std::new_handler h = std::get_new_handler();
  #else
    std::new_handler h = std::set_new_handler();
    std::set_new_handler(h);
  #endif
  if (h==NULL) {
    _mi_error_message(ENOMEM, "out of memory in 'new'");
    #if defined(_CPPUNWIND) || defined(__cpp_exceptions)  // exceptions are not always enabled
    if (!nothrow) {
      throw std::bad_alloc();
    }
    #else
    MI_UNUSED(nothrow);
    #endif
    return false;
  }
  else if (!nothrow) {
    h();
    return true;
  }
  else {
    try {
      h();
    }
    catch(...) {     // swallow std::bad_alloc
      return false;  // stop trying
    }
    return true;
  }
}
#else
typedef void (*std_new_handler_t)(void);

#if (defined(__GNUC__) || (defined(__clang__) && !defined(_MSC_VER)))  // exclude clang-cl, see issue #631
std_new_handler_t __attribute__((weak)) _ZSt15get_new_handlerv(void) {
  return NULL;
}
static std_new_handler_t mi_get_new_handler(void) {
  return _ZSt15get_new_handlerv();
}
#else
// note: on windows we could dynamically link to `?get_new_handler@std@@YAP6AXXZXZ`.
static std_new_handler_t mi_get_new_handler(void) {
  return NULL;
}
#endif

static bool mi_try_new_handler(bool nothrow) {
  std_new_handler_t h = mi_get_new_handler();
  if (h==NULL) {
    _mi_error_message(ENOMEM, "out of memory in 'new'");
    if (!nothrow) {
      abort();  // cannot throw in plain C, use abort
    }
    return false;
  }
  else {
    h();
    return true;
  }
}
#endif

mi_decl_export mi_decl_noinline void* mi_heap_try_new(mi_heap_t* heap, size_t size, bool nothrow ) {
  void* p = NULL;
  for(int i = 0; i < MI_TRY_NEW_MAX && p == NULL && mi_try_new_handler(nothrow); i++) {
    if (size > MI_MAX_ALLOC_SIZE) return NULL; // call try_new_handler at least once
    p = mi_heap_malloc(heap,size);
  }
  return p;
}

static mi_decl_noinline void* mi_try_new(size_t size, bool nothrow) {
  return mi_heap_try_new(mi_prim_get_default_heap(), size, nothrow);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_alloc_new(mi_heap_t* heap, size_t size) {
  void* p = mi_heap_malloc(heap,size);
  if mi_unlikely(p == NULL) return mi_heap_try_new(heap, size, false);
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_new(size_t size) {
  return mi_heap_alloc_new(mi_prim_get_default_heap(), size);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_alloc_new_n(mi_heap_t* heap, size_t count, size_t size) {
  size_t total;
  if mi_unlikely(mi_count_size_overflow(count, size, &total)) {
    mi_try_new_handler(false);  // on overflow we invoke the try_new_handler once to potentially throw std::bad_alloc
    return NULL;
  }
  else {
    return mi_heap_alloc_new(heap,total);
  }
}

mi_decl_nodiscard mi_decl_restrict void* mi_new_n(size_t count, size_t size) {
  return mi_heap_alloc_new_n(mi_prim_get_default_heap(), count, size);
}

mi_decl_nodiscard mi_decl_restrict void* mi_new_nothrow(size_t size) mi_attr_noexcept {
  void* p = mi_malloc(size);
  if mi_unlikely(p == NULL) return mi_try_new(size, true);
  return p;
}

static mi_decl_noinline void* mi_try_new_aligned(size_t size, size_t alignment, bool nothrow) {
  void* p = NULL;
  for(int i = 0; i < MI_TRY_NEW_MAX && p==NULL && mi_try_new_handler(nothrow); i++) {
    if (!mi_alignment_is_valid(alignment)) return NULL;
    p = mi_malloc_aligned(size,alignment);
  }
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_new_aligned(size_t size, size_t alignment) {
  void* p = mi_malloc_aligned(size, alignment);
  if mi_unlikely(p==NULL) return mi_try_new_aligned(size,alignment,false);
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_new_aligned_nothrow(size_t size, size_t alignment) mi_attr_noexcept {
  void* p = mi_malloc_aligned(size, alignment);
  if mi_unlikely(p==NULL) return mi_try_new_aligned(size,alignment,true);
  return p;
}

static mi_decl_noinline void* mi_try_new_realloc(void* p, size_t newsize) {
  void* q = NULL;
  for(int i = 0; i < MI_TRY_NEW_MAX && q==NULL && mi_try_new_handler(false); i++) {
    if (newsize > MI_MAX_ALLOC_SIZE) return NULL;
    q = mi_realloc(p,newsize);
  }
  return q;
}

mi_decl_nodiscard void* mi_new_realloc(void* p, size_t newsize) {
  void* q = mi_realloc(p, newsize);
  if (q == NULL) return mi_try_new_realloc(p,newsize);
  return q;
}

mi_decl_nodiscard void* mi_new_reallocn(void* p, size_t newcount, size_t size) {
  size_t total;
  if mi_unlikely(mi_count_size_overflow(newcount, size, &total)) {
    mi_try_new_handler(false);
    return NULL;
  }
  else {
    return mi_new_realloc(p, total);
  }
}

#if MI_GUARDED
// We always allocate a guarded allocation at an offset (`mi_page_has_aligned` will be true).
// We then set the first word of the block to `0` for regular offset aligned allocations (in `alloc-aligned.c`)
// and the first word to `~0` for guarded allocations to have a correct `mi_usable_size`

static void* mi_block_ptr_set_guarded(mi_block_t* block, size_t obj_size, size_t* usable_size) {
  // TODO: we can still make padding work by moving it out of the guard page area
  mi_page_t* const page = _mi_ptr_page(block);
  mi_page_set_has_aligned(page, true);
  block->next = MI_BLOCK_TAG_GUARDED;

  // set guard page at the end of the block
  mi_segment_t* const segment = _mi_page_segment(page);
  const size_t block_size = mi_page_block_size(page);  // must use `block_size` to match `mi_free_local`
  const size_t os_page_size = _mi_os_page_size();
  mi_assert_internal(block_size >= obj_size + os_page_size + sizeof(mi_block_t));
  if (block_size < obj_size + os_page_size + sizeof(mi_block_t)) {
    // should never happen
    mi_free(block);
    return NULL;
  }
  uint8_t* guard_page = (uint8_t*)block + block_size - os_page_size;
  mi_assert_internal(_mi_is_aligned(guard_page, os_page_size));
  if mi_likely(segment->allow_decommit && _mi_is_aligned(guard_page, os_page_size)) {
    const bool ok = _mi_os_protect(guard_page, os_page_size);
    if mi_unlikely(!ok) {
      _mi_warning_message("failed to set a guard page behind an object (object %p of size %zu)\n", block, block_size);
    }
  }
  else {
    _mi_warning_message("unable to set a guard page behind an object due to pinned memory (large OS pages?) (object %p of size %zu)\n", block, block_size);
  }

  // align pointer just in front of the guard page
  size_t offset = block_size - os_page_size - obj_size;
  mi_assert_internal(offset > sizeof(mi_block_t));
  if (offset > MI_BLOCK_ALIGNMENT_MAX) {
    // give up to place it right in front of the guard page if the offset is too large for unalignment
    offset = MI_BLOCK_ALIGNMENT_MAX;
  }
  uint8_t* const p = (uint8_t*)block + offset;
  mi_assert_internal(p == guard_page - obj_size || offset == MI_BLOCK_ALIGNMENT_MAX);
  if (usable_size != NULL) { *usable_size = (guard_page - p); mi_assert_internal(mi_usable_size(p)==*usable_size); }
  mi_track_align(block, p, offset, obj_size);
  mi_track_mem_defined(block, sizeof(mi_block_t));
  return p;
}

mi_decl_restrict void* _mi_heap_malloc_guarded(mi_heap_t* heap, size_t size, bool zero, mi_page_t** ppage) mi_attr_noexcept
{
  // allocate multiple of page size ending in a guard page
  // ensure minimal alignment requirement?
  if mi_unlikely(size >= MI_MAX_ALLOC_SIZE - MI_PADDING_SIZE) {  // check up front so the `req_size` won't overflow
    _mi_error_message(EOVERFLOW, "(guarded) allocation request is too large (%zu bytes)\n", size);
    return NULL;
  }
  const size_t os_page_size = _mi_os_page_size();
  const size_t obj_size = (mi_option_is_enabled(mi_option_guarded_precise) ? size : _mi_align_up(size, MI_MAX_ALIGN_SIZE));
  const size_t bsize    = _mi_align_up(_mi_align_up(obj_size, MI_MAX_ALIGN_SIZE) + sizeof(mi_block_t), MI_MAX_ALIGN_SIZE);
  const size_t req_size = _mi_align_up(bsize + os_page_size, os_page_size);
  mi_block_t* const block = (mi_block_t*)_mi_malloc_generic(heap, req_size, false /* don't zero */, 0 /* huge_alignment */, ppage);
  if (block==NULL) return NULL;
  size_t usable_size = 0;
  void* const p = mi_block_ptr_set_guarded(block, obj_size, &usable_size);
  if (p == NULL) return NULL;
  if (zero) {
    _mi_memzero(p,obj_size);  // we have to zero afterwards as padding might have written inside the block (if the `blocksize > reqsize + os_page_size`)
  }

  // stats
  mi_track_malloc(p, usable_size, zero);
  if (!mi_heap_is_initialized(heap)) { heap = mi_prim_get_default_heap(); }
  _mi_stat_counter_increase(&heap->tld->stats.malloc_guarded_count, 1);
  #if MI_STAT>1
  // adjust stats to only count the allocated size of the block (and not the guard page)
  mi_heap_stat_adjust_decrease(heap, malloc_requested, req_size);
  mi_heap_stat_increase(heap, malloc_requested, size);
  #endif
  #if MI_DEBUG>3
  if (zero) {
    mi_assert_expensive(mi_mem_is_zero(p, size));
  }
  #endif
  return p;
}
#endif

// ------------------------------------------------------
// ensure explicit external inline definitions are emitted!
// ------------------------------------------------------

#ifdef __cplusplus
void* _mi_externs[] = {
  (void*)&_mi_page_malloc,
  (void*)&_mi_page_malloc_zero,
  (void*)&_mi_heap_malloc_zero,
  (void*)&_mi_heap_malloc_zero_ex,
  (void*)&mi_malloc,
  (void*)&mi_malloc_small,
  (void*)&mi_zalloc_small,
  (void*)&mi_heap_malloc,
  (void*)&mi_heap_zalloc,
  (void*)&mi_heap_malloc_small,
  // (void*)&mi_heap_alloc_new,
  // (void*)&mi_heap_alloc_new_n
};
#endif
