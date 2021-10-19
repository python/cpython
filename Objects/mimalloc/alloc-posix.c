/* ----------------------------------------------------------------------------
Copyright (c) 2018-2021, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

// ------------------------------------------------------------------------
// mi prefixed publi definitions of various Posix, Unix, and C++ functions
// for convenience and used when overriding these functions.
// ------------------------------------------------------------------------
#include "mimalloc.h"
#include "mimalloc-internal.h"

// ------------------------------------------------------
// Posix & Unix functions definitions
// ------------------------------------------------------

#include <errno.h>
#include <string.h>  // memset
#include <stdlib.h>  // getenv

#ifdef _MSC_VER
#pragma warning(disable:4996)  // getenv _wgetenv
#endif

#ifndef EINVAL
#define EINVAL 22
#endif
#ifndef ENOMEM
#define ENOMEM 12
#endif


size_t mi_malloc_size(const void* p) mi_attr_noexcept {
  return mi_usable_size(p);
}

size_t mi_malloc_usable_size(const void *p) mi_attr_noexcept {
  return mi_usable_size(p);
}

void mi_cfree(void* p) mi_attr_noexcept {
  if (mi_is_in_heap_region(p)) {
    mi_free(p);
  }
}

int mi_posix_memalign(void** p, size_t alignment, size_t size) mi_attr_noexcept {
  // Note: The spec dictates we should not modify `*p` on an error. (issue#27)
  // <http://man7.org/linux/man-pages/man3/posix_memalign.3.html>
  if (p == NULL) return EINVAL;
  if (alignment % sizeof(void*) != 0) return EINVAL;   // natural alignment
  if (!_mi_is_power_of_two(alignment)) return EINVAL;  // not a power of 2
  void* q = (mi_malloc_satisfies_alignment(alignment, size) ? mi_malloc(size) : mi_malloc_aligned(size, alignment));
  if (q==NULL && size != 0) return ENOMEM;
  mi_assert_internal(((uintptr_t)q % alignment) == 0);
  *p = q;
  return 0;
}

mi_decl_restrict void* mi_memalign(size_t alignment, size_t size) mi_attr_noexcept {
  void* p = (mi_malloc_satisfies_alignment(alignment,size) ? mi_malloc(size) : mi_malloc_aligned(size, alignment));
  mi_assert_internal(((uintptr_t)p % alignment) == 0);
  return p;
}

mi_decl_restrict void* mi_valloc(size_t size) mi_attr_noexcept {
  return mi_memalign( _mi_os_page_size(), size );
}

mi_decl_restrict void* mi_pvalloc(size_t size) mi_attr_noexcept {
  size_t psize = _mi_os_page_size();
  if (size >= SIZE_MAX - psize) return NULL; // overflow
  size_t asize = _mi_align_up(size, psize);
  return mi_malloc_aligned(asize, psize);
}

mi_decl_restrict void* mi_aligned_alloc(size_t alignment, size_t size) mi_attr_noexcept {
  if (alignment==0 || !_mi_is_power_of_two(alignment)) return NULL; 
  if ((size&(alignment-1)) != 0) return NULL; // C11 requires integral multiple, see <https://en.cppreference.com/w/c/memory/aligned_alloc>
  void* p = (mi_malloc_satisfies_alignment(alignment, size) ? mi_malloc(size) : mi_malloc_aligned(size, alignment));
  mi_assert_internal(((uintptr_t)p % alignment) == 0);
  return p;
}

void* mi_reallocarray( void* p, size_t count, size_t size ) mi_attr_noexcept {  // BSD
  void* newp = mi_reallocn(p,count,size);
  if (newp==NULL) errno = ENOMEM;
  return newp;
}

void* mi__expand(void* p, size_t newsize) mi_attr_noexcept {  // Microsoft
  void* res = mi_expand(p, newsize);
  if (res == NULL) errno = ENOMEM;
  return res;
}

mi_decl_restrict unsigned short* mi_wcsdup(const unsigned short* s) mi_attr_noexcept {
  if (s==NULL) return NULL;
  size_t len;
  for(len = 0; s[len] != 0; len++) { }
  size_t size = (len+1)*sizeof(unsigned short);
  unsigned short* p = (unsigned short*)mi_malloc(size);
  if (p != NULL) {
    _mi_memcpy(p,s,size);
  }
  return p;
}

mi_decl_restrict unsigned char* mi_mbsdup(const unsigned char* s)  mi_attr_noexcept {
  return (unsigned char*)mi_strdup((const char*)s);
}

int mi_dupenv_s(char** buf, size_t* size, const char* name) mi_attr_noexcept {
  if (buf==NULL || name==NULL) return EINVAL;
  if (size != NULL) *size = 0;
  char* p = getenv(name);        // mscver warning 4996
  if (p==NULL) {
    *buf = NULL;
  }
  else {
    *buf = mi_strdup(p);
    if (*buf==NULL) return ENOMEM;
    if (size != NULL) *size = strlen(p);
  }
  return 0;
}

int mi_wdupenv_s(unsigned short** buf, size_t* size, const unsigned short* name) mi_attr_noexcept {
  if (buf==NULL || name==NULL) return EINVAL;
  if (size != NULL) *size = 0;
#if !defined(_WIN32) || (defined(WINAPI_FAMILY) && (WINAPI_FAMILY != WINAPI_FAMILY_DESKTOP_APP))
  // not supported
  *buf = NULL;
  return EINVAL;
#else
  unsigned short* p = (unsigned short*)_wgetenv((const wchar_t*)name);  // msvc warning 4996
  if (p==NULL) {
    *buf = NULL;
  }
  else {
    *buf = mi_wcsdup(p);
    if (*buf==NULL) return ENOMEM;
    if (size != NULL) *size = wcslen((const wchar_t*)p);
  }
  return 0;
#endif
}

void* mi_aligned_offset_recalloc(void* p, size_t newcount, size_t size, size_t alignment, size_t offset) mi_attr_noexcept { // Microsoft
  return mi_recalloc_aligned_at(p, newcount, size, alignment, offset);
}

void* mi_aligned_recalloc(void* p, size_t newcount, size_t size, size_t alignment) mi_attr_noexcept { // Microsoft
  return mi_recalloc_aligned(p, newcount, size, alignment);
}
