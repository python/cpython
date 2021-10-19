/* ----------------------------------------------------------------------------
Copyright (c) 2018-2021, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

#if !defined(MI_IN_ALLOC_C)
#error "this file should be included from 'alloc.c' (so aliases can work)"
#endif

#if defined(MI_MALLOC_OVERRIDE) && defined(_WIN32) && !(defined(MI_SHARED_LIB) && defined(_DLL))
#error "It is only possible to override "malloc" on Windows when building as a DLL (and linking the C runtime as a DLL)"
#endif

#if defined(MI_MALLOC_OVERRIDE) && !(defined(_WIN32)) // || (defined(__APPLE__) && !defined(MI_INTERPOSE)))

// ------------------------------------------------------
// Override system malloc
// ------------------------------------------------------

#if (defined(__GNUC__) || defined(__clang__)) && !defined(__APPLE__)
  // use aliasing to alias the exported function to one of our `mi_` functions
  #if (defined(__GNUC__) && __GNUC__ >= 9)
    #define MI_FORWARD(fun)      __attribute__((alias(#fun), used, visibility("default"), copy(fun)));
  #else
    #define MI_FORWARD(fun)      __attribute__((alias(#fun), used, visibility("default")));
  #endif
  #define MI_FORWARD1(fun,x)      MI_FORWARD(fun)
  #define MI_FORWARD2(fun,x,y)    MI_FORWARD(fun)
  #define MI_FORWARD3(fun,x,y,z)  MI_FORWARD(fun)
  #define MI_FORWARD0(fun,x)      MI_FORWARD(fun)
  #define MI_FORWARD02(fun,x,y)   MI_FORWARD(fun)
#else
  // use forwarding by calling our `mi_` function
  #define MI_FORWARD1(fun,x)      { return fun(x); }
  #define MI_FORWARD2(fun,x,y)    { return fun(x,y); }
  #define MI_FORWARD3(fun,x,y,z)  { return fun(x,y,z); }
  #define MI_FORWARD0(fun,x)      { fun(x); }
  #define MI_FORWARD02(fun,x,y)   { fun(x,y); }
#endif

#if defined(__APPLE__) && defined(MI_SHARED_LIB_EXPORT) && defined(MI_INTERPOSE)
  // use interposing so `DYLD_INSERT_LIBRARIES` works without `DYLD_FORCE_FLAT_NAMESPACE=1`
  // See: <https://books.google.com/books?id=K8vUkpOXhN4C&pg=PA73>
  struct mi_interpose_s {
    const void* replacement;
    const void* target;
  };
  #define MI_INTERPOSE_FUN(oldfun,newfun) { (const void*)&newfun, (const void*)&oldfun }
  #define MI_INTERPOSE_MI(fun)            MI_INTERPOSE_FUN(fun,mi_##fun)
  __attribute__((used)) static struct mi_interpose_s _mi_interposes[]  __attribute__((section("__DATA, __interpose"))) =
  {
    MI_INTERPOSE_MI(malloc),
    MI_INTERPOSE_MI(calloc),
    MI_INTERPOSE_MI(realloc),
    MI_INTERPOSE_MI(strdup),
    MI_INTERPOSE_MI(strndup),
    MI_INTERPOSE_MI(realpath),
    MI_INTERPOSE_MI(posix_memalign),
    MI_INTERPOSE_MI(reallocf),
    MI_INTERPOSE_MI(valloc),
    #ifndef MI_OSX_ZONE
    // some code allocates from default zone but deallocates using plain free :-( (like NxHashResizeToCapacity <https://github.com/nneonneo/osx-10.9-opensource/blob/master/objc4-551.1/runtime/hashtable2.mm>)
    MI_INTERPOSE_FUN(free,mi_cfree), // use safe free that checks if pointers are from us
    #else
    // We interpose malloc_default_zone in alloc-override-osx.c
    MI_INTERPOSE_MI(free),
    #endif
    // some code allocates from a zone but deallocates using plain free :-( (like NxHashResizeToCapacity <https://github.com/nneonneo/osx-10.9-opensource/blob/master/objc4-551.1/runtime/hashtable2.mm>)
    MI_INTERPOSE_FUN(free,mi_cfree), // use safe free that checks if pointers are from us
  };
#elif defined(_MSC_VER)
  // cannot override malloc unless using a dll.
  // we just override new/delete which does work in a static library.
#else
  // On all other systems forward to our API
  void* malloc(size_t size)              MI_FORWARD1(mi_malloc, size)
  void* calloc(size_t size, size_t n)    MI_FORWARD2(mi_calloc, size, n)
  void* realloc(void* p, size_t newsize) MI_FORWARD2(mi_realloc, p, newsize)
  void  free(void* p)                    MI_FORWARD0(mi_free, p)
#endif

#if (defined(__GNUC__) || defined(__clang__)) && !defined(__APPLE__)
#pragma GCC visibility push(default)
#endif

// ------------------------------------------------------
// Override new/delete
// This is not really necessary as they usually call
// malloc/free anyway, but it improves performance.
// ------------------------------------------------------
#ifdef __cplusplus
  // ------------------------------------------------------
  // With a C++ compiler we override the new/delete operators.
  // see <https://en.cppreference.com/w/cpp/memory/new/operator_new>
  // ------------------------------------------------------
  #include <new>
  void operator delete(void* p) noexcept              MI_FORWARD0(mi_free,p)
  void operator delete[](void* p) noexcept            MI_FORWARD0(mi_free,p)

  void* operator new(std::size_t n) noexcept(false)   MI_FORWARD1(mi_new,n)
  void* operator new[](std::size_t n) noexcept(false) MI_FORWARD1(mi_new,n)

  void* operator new  (std::size_t n, const std::nothrow_t& tag) noexcept { UNUSED(tag); return mi_new_nothrow(n); }
  void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept { UNUSED(tag); return mi_new_nothrow(n); }

  #if (__cplusplus >= 201402L || _MSC_VER >= 1916)
  void operator delete  (void* p, std::size_t n) noexcept MI_FORWARD02(mi_free_size,p,n)
  void operator delete[](void* p, std::size_t n) noexcept MI_FORWARD02(mi_free_size,p,n)
  #endif

  #if (__cplusplus > 201402L && defined(__cpp_aligned_new)) && (!defined(__GNUC__) || (__GNUC__ > 5))
  void operator delete  (void* p, std::align_val_t al) noexcept { mi_free_aligned(p, static_cast<size_t>(al)); }
  void operator delete[](void* p, std::align_val_t al) noexcept { mi_free_aligned(p, static_cast<size_t>(al)); }
  void operator delete  (void* p, std::size_t n, std::align_val_t al) noexcept { mi_free_size_aligned(p, n, static_cast<size_t>(al)); };
  void operator delete[](void* p, std::size_t n, std::align_val_t al) noexcept { mi_free_size_aligned(p, n, static_cast<size_t>(al)); };

  void* operator new( std::size_t n, std::align_val_t al)   noexcept(false) { return mi_new_aligned(n, static_cast<size_t>(al)); }
  void* operator new[]( std::size_t n, std::align_val_t al) noexcept(false) { return mi_new_aligned(n, static_cast<size_t>(al)); }
  void* operator new  (std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept { return mi_new_aligned_nothrow(n, static_cast<size_t>(al)); }
  void* operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept { return mi_new_aligned_nothrow(n, static_cast<size_t>(al)); }
  #endif

#elif (defined(__GNUC__) || defined(__clang__))
  // ------------------------------------------------------
  // Override by defining the mangled C++ names of the operators (as
  // used by GCC and CLang).
  // See <https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling>
  // ------------------------------------------------------
  void _ZdlPv(void* p)            MI_FORWARD0(mi_free,p) // delete
  void _ZdaPv(void* p)            MI_FORWARD0(mi_free,p) // delete[]
  void _ZdlPvm(void* p, size_t n) MI_FORWARD02(mi_free_size,p,n)
  void _ZdaPvm(void* p, size_t n) MI_FORWARD02(mi_free_size,p,n)
  void _ZdlPvSt11align_val_t(void* p, size_t al)            { mi_free_aligned(p,al); }
  void _ZdaPvSt11align_val_t(void* p, size_t al)            { mi_free_aligned(p,al); }
  void _ZdlPvmSt11align_val_t(void* p, size_t n, size_t al) { mi_free_size_aligned(p,n,al); }
  void _ZdaPvmSt11align_val_t(void* p, size_t n, size_t al) { mi_free_size_aligned(p,n,al); }

  typedef struct mi_nothrow_s { int _tag; } mi_nothrow_t;
  #if (MI_INTPTR_SIZE==8)
    void* _Znwm(size_t n)                             MI_FORWARD1(mi_new,n)  // new 64-bit
    void* _Znam(size_t n)                             MI_FORWARD1(mi_new,n)  // new[] 64-bit
    void* _ZnwmSt11align_val_t(size_t n, size_t al)   MI_FORWARD2(mi_new_aligned, n, al)
    void* _ZnamSt11align_val_t(size_t n, size_t al)   MI_FORWARD2(mi_new_aligned, n, al)
    void* _ZnwmRKSt9nothrow_t(size_t n, mi_nothrow_t tag) { UNUSED(tag); return mi_new_nothrow(n); }
    void* _ZnamRKSt9nothrow_t(size_t n, mi_nothrow_t tag) { UNUSED(tag); return mi_new_nothrow(n); }
    void* _ZnwmSt11align_val_tRKSt9nothrow_t(size_t n, size_t al, mi_nothrow_t tag) { UNUSED(tag); return mi_new_aligned_nothrow(n,al); }
    void* _ZnamSt11align_val_tRKSt9nothrow_t(size_t n, size_t al, mi_nothrow_t tag) { UNUSED(tag); return mi_new_aligned_nothrow(n,al); }
  #elif (MI_INTPTR_SIZE==4)
    void* _Znwj(size_t n)                             MI_FORWARD1(mi_new,n)  // new 64-bit
    void* _Znaj(size_t n)                             MI_FORWARD1(mi_new,n)  // new[] 64-bit
    void* _ZnwjSt11align_val_t(size_t n, size_t al)   MI_FORWARD2(mi_new_aligned, n, al)
    void* _ZnajSt11align_val_t(size_t n, size_t al)   MI_FORWARD2(mi_new_aligned, n, al)
    void* _ZnwjRKSt9nothrow_t(size_t n, mi_nothrow_t tag) { UNUSED(tag); return mi_new_nothrow(n); }
    void* _ZnajRKSt9nothrow_t(size_t n, mi_nothrow_t tag) { UNUSED(tag); return mi_new_nothrow(n); }
    void* _ZnwjSt11align_val_tRKSt9nothrow_t(size_t n, size_t al, mi_nothrow_t tag) { UNUSED(tag); return mi_new_aligned_nothrow(n,al); }
    void* _ZnajSt11align_val_tRKSt9nothrow_t(size_t n, size_t al, mi_nothrow_t tag) { UNUSED(tag); return mi_new_aligned_nothrow(n,al); }
  #else
  #error "define overloads for new/delete for this platform (just for performance, can be skipped)"
  #endif
#endif // __cplusplus


#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------------------------------
// Posix & Unix functions definitions
// ------------------------------------------------------

void   cfree(void* p)                    MI_FORWARD0(mi_free, p)
void*  reallocf(void* p, size_t newsize) MI_FORWARD2(mi_reallocf,p,newsize)
size_t malloc_size(const void* p)        MI_FORWARD1(mi_usable_size,p)
#if !defined(__ANDROID__)
size_t malloc_usable_size(void *p)       MI_FORWARD1(mi_usable_size,p)
#else
size_t malloc_usable_size(const void *p) MI_FORWARD1(mi_usable_size,p)
#endif

// no forwarding here due to aliasing/name mangling issues
void* valloc(size_t size)                                     { return mi_valloc(size); }
void* pvalloc(size_t size)                                    { return mi_pvalloc(size); }
void* reallocarray(void* p, size_t count, size_t size)        { return mi_reallocarray(p, count, size); }
void* memalign(size_t alignment, size_t size)                 { return mi_memalign(alignment, size); }
int   posix_memalign(void** p, size_t alignment, size_t size) { return mi_posix_memalign(p, alignment, size); }
void* _aligned_malloc(size_t alignment, size_t size)          { return mi_aligned_alloc(alignment, size); }

// `aligned_alloc` is only available when __USE_ISOC11 is defined.
// Note: Conda has a custom glibc where `aligned_alloc` is declared `static inline` and we cannot
// override it, but both _ISOC11_SOURCE and __USE_ISOC11 are undefined in Conda GCC7 or GCC9.
// Fortunately, in the case where `aligned_alloc` is declared as `static inline` it
// uses internally `memalign`, `posix_memalign`, or `_aligned_malloc` so we  can avoid overriding it ourselves.
#if __USE_ISOC11 
void* aligned_alloc(size_t alignment, size_t size)   { return mi_aligned_alloc(alignment, size); }
#endif


#if defined(__GLIBC__) && defined(__linux__)
  // forward __libc interface (needed for glibc-based Linux distributions)
  void* __libc_malloc(size_t size)                  MI_FORWARD1(mi_malloc,size)
  void* __libc_calloc(size_t count, size_t size)    MI_FORWARD2(mi_calloc,count,size)
  void* __libc_realloc(void* p, size_t size)        MI_FORWARD2(mi_realloc,p,size)
  void  __libc_free(void* p)                        MI_FORWARD0(mi_free,p)
  void  __libc_cfree(void* p)                       MI_FORWARD0(mi_free,p)

  void* __libc_valloc(size_t size)                                { return mi_valloc(size); }
  void* __libc_pvalloc(size_t size)                               { return mi_pvalloc(size); }
  void* __libc_memalign(size_t alignment, size_t size)            { return mi_memalign(alignment,size); }
  int   __posix_memalign(void** p, size_t alignment, size_t size) { return mi_posix_memalign(p,alignment,size); }
#endif

#ifdef __cplusplus
}
#endif

#if (defined(__GNUC__) || defined(__clang__)) && !defined(__APPLE__)
#pragma GCC visibility pop
#endif

#endif // MI_MALLOC_OVERRIDE && !_WIN32
