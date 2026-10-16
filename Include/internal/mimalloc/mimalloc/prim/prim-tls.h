/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_PRIM_TLS_H
#define MI_INTERNAL_PRIM_TLS_H

#include "../types.h"
#include "../heap/heap.h"
#include "../heap/theap.h"
#include "../util/threadlocal.h"

// --------------------------------------------------------------------------
// We need fast access to both a unique thread id (in `alloc/free.c:mi_free`) and
// to a thread-local theap pointer (in `alloc/alloc.c:mi_malloc`).
//
// For performance, we tend to use specialized code for various platforms.
// This leads to quite a few ifdefs but it is just for performance and there
// is always a portable fallback (based on regular thread local variables).
//
// Windows          : use NtCurrentTeB and TlsAlloc (MI_TLS_MODEL_WIN32)
// Linux,FreeBSD    : use thread locals with the initial-exec model  (MI_TLS_MODEL_LOCAL)
// macOS            : use pthread locals with assembly for the thread-id  (MI_TLS_MODEL_PTHREADS)
// Android,OpenBSD  : use pthread locals (MI_TLS_MODEL_PTHREADS). todo: maybe on Android MI_TLS_MODEL_LOCAL is better?
// --------------------------------------------------------------------------

static inline mi_threadid_t _mi_prim_thread_id(void);                        // get a unique id for a thread
static inline mi_theap_t*   _mi_theap_default(void);                         // the default thread local theap
static inline mi_theap_t*   _mi_theap_cached(void);                          // last used thread local theap using the _heap_ api
static inline bool          _mi_thread_is_initialized(void);                 // a thread is initialized if it has a default theap
static inline mi_theap_t*   _mi_heap_theap(mi_heap_t* heap);                 // get the thread local theap belonging to a heap
static inline mi_theap_t*   _mi_heap_theap_peek(const mi_heap_t* heap);      // get the theap but don't update _mi_theap_cached
static inline mi_theap_t*   _mi_page_associated_theap_peek(const mi_page_t* page); // get the theap associated with a page (used in `mi_free_collect_mt`)

// Regular exports
void          _mi_tls_slots_init(void);
void          _mi_tls_slots_done(void);
mi_threadid_t _mi_thread_id(void) mi_attr_noexcept;
void          _mi_theap_default_set(mi_theap_t* theap);
void          _mi_theap_cached_set(mi_theap_t* theap);


static inline bool mi_theap_matches_thread(mi_theap_t* theap) {
  const mi_threadid_t tid = _mi_thread_id();
  return (theap==NULL || theap->tld==NULL || theap->tld->thread_id == tid || mi_theap_is_detached(theap));
}


// Default TLS model
#if !defined(MI_TLS_MODEL_LOCAL) && !defined(MI_TLS_MODEL_PTHREADS) && !defined(MI_TLS_MODEL_FIXED) && !defined(MI_TLS_MODEL_WIN32)
#if defined(_WIN32)
#define MI_TLS_MODEL_WIN32        1
#elif defined(__APPLE__) || defined(__OpenBSD__) || defined(__ANDROID__)  // and FreeBSD?
#define MI_TLS_MODEL_PTHREADS     1
#else
#define MI_TLS_MODEL_LOCAL        1
#endif
#endif


//-------------------------------------------------------------------
// Access to the thread pointer
//-------------------------------------------------------------------

// Do we have __builtin_thread_pointer? This would be the preferred way to get a unique thread id
// but unfortunately, it seems we cannot test for this reliably at this time (see issue #883)
// Nevertheless, it seems needed on older graviton platforms (see issue #851).
// For now, we only enable this for specific platforms.
#if !defined(MI_USE_BUILTIN_THREAD_POINTER)   /* allow user override */
  #if !defined(__APPLE__)  /* on apple (M1) the wrong register is read (tpidr_el0 instead of tpidrro_el0) so fall back to TLS slot assembly (<https://github.com/microsoft/mimalloc/issues/343#issuecomment-763272369>)*/ \
      && !defined(__CYGWIN__) \
      && !defined(MI_LIBC_MUSL) \
      && (!defined(__clang_major__) || __clang_major__ >= 14)  /* older clang versions emit bad code; fall back to using the TLS slot (<https://lore.kernel.org/linux-arm-kernel/202110280952.352F66D8@keescook/T/>) */
    #if    (defined(__GNUC__) && (__GNUC__ >= 7)  && defined(__aarch64__)) /* aarch64 for older gcc versions (issue #851) */ \
        || (defined(__GNUC__) && (__GNUC__ >= 7)  && defined(__riscv)) \
        || (defined(__GNUC__) && (__GNUC__ >= 11) && defined(__x86_64__)) \
        || (defined(__clang_major__) && (__clang_major__ >= 14) && (defined(__aarch64__) || defined(__x86_64__) || defined(__riscv)))
      #define MI_USE_BUILTIN_THREAD_POINTER  1
    #endif
  #endif
#endif

#if defined(MI_PRIM_THREAD_POINTER)  // potential user override
static inline void** mi_prim_thread_pointer(void) {
  return MI_PRIM_THREAD_POINTER();
}
#elif defined(_WIN32)
static inline void** mi_prim_thread_pointer(void) {
  return (void**)NtCurrentTeb();
}
#elif MI_USE_BUILTIN_THREAD_POINTER
static inline void** mi_prim_thread_pointer(void) {
  return (void**)__builtin_thread_pointer();
}
#elif defined(__GNUC__) && !defined(__CYGWIN__)
  #if defined(__aarch64__)
  static inline void** mi_prim_thread_pointer(void) {
    void** tcb;
    #if defined(__APPLE__) // M1, issue rgb(62, 76, 62)
    __asm__ ("mrs %0, tpidrro_el0\n\tbic %0, %0, #7" : "=r" (tcb));
    #else
    __asm__ ("mrs %0, tpidr_el0" : "=r" (tcb));
    #endif
    return tcb;
  }
  #elif defined(__riscv)
  static inline void** mi_prim_thread_pointer(void) {
    void** tcb;
    __asm__ ("mv %0, tp" : "=r" (tcb));
    return tcb;
  }
  #elif defined(__arm__)
  static inline void** mi_prim_thread_pointer(void) {
    void** tcb;
    __asm__ volatile ("mrc p15, 0, %0, c13, c0, 3\n\tbic %0, %0, #3" : "=r" (tcb));
    return tcb;
  }
  #elif defined(__i386__)
  static inline void** mi_prim_thread_pointer(void) {
    void** tcb;
    __asm__ ("movl %%gs:0, %0" : "=r" (tcb) : : );  // x86 32-bit always uses GS
    return tcb;
  }
  #elif defined(__x86_64__)
  static inline void** mi_prim_thread_pointer(void) {
    void** tcb;
    #if defined(__APPLE__)
    __asm__ ("movq %%gs:0, %0" : "=r" (tcb) : : );  // x86_64 macOSX uses GS
    #elif (MI_SIZE_SIZE==4)
    __asm__ ("movl %%fs:0, %0" : "=r" (tcb) : : );  // x32 ABI
    #else
    __asm__ ("movq %%fs:0, %0" : "=r" (tcb) : : );  // x86_64 Linux, BSD uses FS
    #endif
    return tcb;
  }
  #else
  #define MI_NO_THREAD_POINTER (1)
  #endif
#elif MI_USE_PTHREADS && defined(__APPLE__)
static inline void** mi_prim_thread_pointer(void) {
  return (void**)pthread_self();
}
#else
#define MI_NO_THREAD_POINTER (1)
#endif

#if !MI_NO_THREAD_POINTER
#define MI_HAS_TLS_SLOT  (1)
static inline void* mi_prim_tls_slot(size_t slot) {
  #if defined(_WIN32)
    #if (_M_X64 || _M_AMD64) && !defined(_M_ARM64EC)
      return (void*)__readgsqword((unsigned long)(slot*sizeof(void*)));   // direct load at offset from gs
    #elif _M_IX86 && !defined(_M_ARM64EC)
      return (void*)__readfsdword((unsigned long)(slot*sizeof(void*)));   // direct load at offset from fs
    #else
      return mi_prim_thread_pointer()[slot];
    #endif
  #else
    return mi_prim_thread_pointer()[slot];
  #endif
}

static inline void mi_prim_tls_slot_set(size_t slot, void* value) {
  mi_prim_thread_pointer()[slot] = value;
}
#endif

/* ----------------------------------------------------------------------------------------
  Get a unique thread-id for a thread
---------------------------------------------------------------------------------------- */

// Get a unique id for the current thread.
#if defined(MI_PRIM_THREAD_ID)
static inline mi_threadid_t __mi_prim_thread_id(void) {
  return MI_PRIM_THREAD_ID();  // used for example by CPython for a free threaded build (see python/cpython#115488)
}
#elif !MI_NO_THREAD_POINTER
static inline mi_threadid_t __mi_prim_thread_id(void) {
  #if defined(__BIONIC__)
  return (mi_threadid_t)mi_prim_tls_slot(1);
  #else
  return (mi_threadid_t)mi_prim_thread_pointer();
  #endif
}
#else
// otherwise use portable C, taking the address of a thread local variable (this is still very fast on most platforms).
extern mi_decl_hidden mi_decl_thread void* __mi_thread_id_helper;
static inline mi_threadid_t __mi_prim_thread_id(void) {
  return (uintptr_t)&__mi_thread_id_helper;
}
#endif

static inline mi_threadid_t _mi_prim_thread_id(void) {
  const mi_threadid_t tid = __mi_prim_thread_id();
  mi_assert_internal(tid > MI_THREADID_DETACHED);
  mi_assert_internal((tid & MI_PAGE_FLAG_MASK) == 0);  // bottom 2 bits are clear?
  return tid;
}


/* ----------------------------------------------------------------------------------------
Get the thread local default theap: `_mi_theap_default()` (and the cached heap `_mi_theap_cached`).

This is inlined here as it is on the fast path for allocation functions.
We have 4 models:

- MI_TLS_MODEL_LOCAL: use regular thread local (default on Linux, FreeBSD, etc)
    On most platforms (Linux, FreeBSD, NetBSD, etc), this just returns a
    thread local variable (`__mi_theap_default`). With the initial-exec TLS model this ensures
    that the storage will always be available and properly initialized (with an empty theap).

    On some platforms the underlying TLS implementation (or the loader) will call itself `malloc`
    on a first access to a thread local and recurse in the MI_TLS_MODEL_LOCAL.
    A way around this is to define MI_TLS_RECURSE_GUARD which adds an extra check if the process
    is initialized before accessing the thread-local. This is a check in the fast path though
    so this should be avoided.

- MI_TLS_MODEL_PTHREADS: use `pthread_getspecific`. (default on macOS and OpenBSD, maybe good for Android as well?)
    Use pthread local storage. Can be as fast as thread locals on many platforms (like recent macOS).

- MI_TLS_MODEL_FIXED: use a fixed slot in the TLS block.
    This reserves an unused and fixed TLS slot. This is fast and avoids the problem
    where the underlying TLS implementation (or the loader) will call itself `malloc`
    on a first access to a thread local (and recurse in the MI_TLS_MODEL_LOCAL).
    This goes wrong though if the OS or a library uses the same fixed slot, and also
    prevents multiple instances of mimalloc in the same process.

- MI_TLS_MODEL_WIN32: use a dynamically allocated slot with TlsAlloc. (default on Windows)
    We use TlsAlloc'd slot. First tries to use one of the "direct" first 64 slots which
    are the fastest, but falls back to using "expansion" slots when needed (up to 1088 slots).
    (If the allocated slot happens to always be under 64 for a particular program,
    one might use cmake with `-DMI_WIN_DIRECT_TLS=ON` to skip the expansion slot test in the fast path.)

Each model should define `MI_THEAP_INITASNULL` to signify that the initial value
returned from `_mi_theap_default()` can be `NULL` (instead of the address of the empty heap).
This incurs an extra check in the fast path (but can often be combined in an existing check).
------------------------------------------------------------------------------------------- */

#if !defined(MI_TLS_RECURSE_GUARD) && MI_TLS_MODEL_LOCAL && defined(__APPLE__)
#define MI_TLS_RECURSE_GUARD 1     // macOS can allocate on thread-local initialization
#endif

// Declared this way to optimize register spills and branches
mi_decl_cold mi_decl_noinline mi_theap_t* _mi_theap_empty_get(void);

static inline mi_theap_t* __mi_theap_empty(void) {
  #if __GNUC__
  __asm("");  // prevent conditional load
  return (mi_theap_t*)&_mi_theap_empty;
  #else
  return _mi_theap_empty_get();
  #endif
}

#if MI_TLS_MODEL_LOCAL
// Thread local with an initial value (default on Linux). Very efficient.
extern mi_decl_hidden mi_decl_thread mi_theap_t* __mi_theap_default;  // default theap to allocate from
extern mi_decl_hidden mi_decl_thread mi_theap_t* __mi_theap_cached;   // theap from the last used heap

// defined in `heap/init.c`; do not use these directly
extern mi_decl_hidden bool _mi_process_is_initialized;  // has mi_process_init been called?

static inline mi_theap_t* _mi_theap_default(void) {
  #if defined(MI_TLS_RECURSE_GUARD)
  if mi_unlikely(!_mi_process_is_initialized) return _mi_theap_empty_get();
  #endif
  return __mi_theap_default;
}

static inline mi_theap_t* _mi_theap_cached(void) {
  return __mi_theap_cached;
}

#elif MI_TLS_MODEL_PTHREADS
// Dynamic pthread slots. This can be fast depending on the platform (default for macOS and OpenBSD)
// On some platforms (like macOS), the loader might allocate on thread local declarations which
// can be avoided with pthreads.
#define MI_THEAP_INITASNULL  1

extern mi_decl_hidden _Atomic(pthread_key_t) _mi_theap_default_key;
extern mi_decl_hidden _Atomic(pthread_key_t) _mi_theap_cached_key;

static inline mi_theap_t* _mi_theap_default(void) {
  pthread_key_t key = mi_atomic_load_relaxed(&_mi_theap_default_key);
  #if defined(__APPLE__) && defined(__aarch64__) && MI_HAS_TLS_SLOT
  // on apple arm64, the pthread specific slots are direct slots; inline it to avoid a stack frame setup in `mi_malloc`
  // todo: this is probably also the case on x64 and power pc?  
  if (key == MI_PTHREAD_KEY_INVALID) return NULL;
  return (mi_theap_t*)mi_prim_tls_slot(key);
  #else
  return (mi_theap_t*)mi_pthread_key_get(key);
  #endif
}

static inline mi_theap_t* _mi_theap_cached(void) {
  pthread_key_t key = mi_atomic_load_relaxed(&_mi_theap_cached_key);
  #if defined(__APPLE__) && defined(__aarch64__) && MI_HAS_TLS_SLOT
  if (key == MI_PTHREAD_KEY_INVALID) return NULL;
  return (mi_theap_t*)mi_prim_tls_slot(key);
  #else
  return (mi_theap_t*)mi_pthread_key_get(key);
  #endif
}

#elif MI_TLS_MODEL_WIN32
// Dynamic TLS slots -- this is the default on Windows.
#define MI_THEAP_INITASNULL  1

// We try to use direct slots (64 available), but can also use the expansion slots (upto 1024 extra available)
// See <https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/pebteb/teb/index.htm> for the offsets.
#if MI_SIZE_SIZE==4
#define MI_TLS_EXPANSION_SLOT    (0x0F94 / MI_SIZE_SIZE)
#else
#define MI_TLS_EXPANSION_SLOT    (0x1780 / MI_SIZE_SIZE)
#endif

extern mi_decl_hidden _Atomic(size_t) _mi_theap_default_slot;
extern mi_decl_hidden _Atomic(size_t) _mi_theap_cached_slot;
extern mi_decl_hidden _Atomic(size_t) _mi_theap_default_expansion_slot;
extern mi_decl_hidden _Atomic(size_t) _mi_theap_cached_expansion_slot;

static inline mi_theap_t* _mi_theap_default(void) {
  const size_t slot = mi_atomic_load_relaxed(&_mi_theap_default_slot);
  mi_theap_t* theap  = (mi_theap_t*)mi_prim_tls_slot(slot);
  #if !MI_WIN_DIRECT_TLS
  if mi_unlikely(slot==MI_TLS_EXPANSION_SLOT) {       // in TlsExpansionSlots ?
    mi_theap_t** const eslots = (mi_theap_t**)theap;  // theap is actually the expansion slot entry
    if mi_likely(eslots!=NULL) {                      // is it initialized? (on this thread)
      theap = eslots[mi_atomic_load_relaxed(&_mi_theap_default_expansion_slot)];
    }
  }
  #endif
  return theap;
}

static inline mi_theap_t* _mi_theap_cached(void) {
  const size_t slot = mi_atomic_load_relaxed(&_mi_theap_cached_slot);
  mi_theap_t* theap = (mi_theap_t*)mi_prim_tls_slot(slot);
  #if !MI_WIN_DIRECT_TLS
  if mi_unlikely(slot==MI_TLS_EXPANSION_SLOT) {       // in TlsExpansionSlots ?
    mi_theap_t** const eslots = (mi_theap_t**)theap;  // theap is the expansion slot entry
    if mi_likely(eslots!=NULL) {                      // is it initialized? (on this thread)
      theap = eslots[mi_atomic_load_relaxed(&_mi_theap_cached_expansion_slot)];
    }
  }
  #endif
  return theap;
}

#elif MI_TLS_MODEL_FIXED
// Fixed TLS slot. Can be the fastest approach, but does not work if there are multiple instances of
// mimalloc in the same process. Most OS's do not have official user reserved fixed slots so this cannot be
// guaranteed to work in general.
#define MI_THEAP_INITASNULL  1

#if !MI_HAS_TLS_SLOT
#error this platform cannot support MI_TLS_MODEL_FIXED without defining mi_prim_tls_slot
#endif

#if !defined(MI_TLS_MODEL_FIXED_DEFAULT)
  #if defined(__APPLE__) && !defined(__POWERPC__)  // macOS on arm64 or x64
    // we use the last two swift framework slots which seem unused.
    // we may want to use slot 6 and 11 instead which are only used by Windows emulation.
    // see <https://github.com/apple/darwin-libpthread/blob/main/private/pthread/tsd_private.h#L99> for assigned slots
    #define MI_TLS_MODEL_FIXED_DEFAULT   108
    #define MI_TLS_MODEL_FIXED_CACHED    109
  #elif defined(_WIN32)
    // we use two seemingly unused fields in the Windows TEB.
    // see <https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/pebteb/teb/index.htm>
    #define MI_TLS_MODEL_FIXED_DEFAULT   5     // arbitrary user pointer
    #define MI_TLS_MODEL_FIXED_CACHED    7     // environment pointer (used by OS2)
  #else
    #error define the TLS model fixed slots (or change the TLS model away from MI_TLS_MODEL_FIXED)
  #endif
#endif

static inline mi_theap_t* _mi_theap_default(void) {
  return (mi_theap_t*)mi_prim_tls_slot(MI_TLS_MODEL_FIXED_DEFAULT);
}

static inline mi_theap_t* _mi_theap_cached(void) {
  return (mi_theap_t*)mi_prim_tls_slot(MI_TLS_MODEL_FIXED_CACHED);
}

#else
#error "no TLS model is defined for this platform?"
#endif


// Check if a thread is initialized (without using a thread-local if using fixed slots)
static inline bool _mi_thread_is_initialized(void) {
  return mi_theap_is_initialized(_mi_theap_default());
}

// Get (and possible create) the theap belonging to a heap
// We cache the last accessed theap in `_mi_theap_cached` for better performance.
static inline mi_theap_t* _mi_heap_theap(mi_heap_t* heap) {
  mi_theap_t* theap = _mi_theap_cached();
  #if MI_THEAP_INITASNULL
  if mi_likely(theap!=NULL && mi_theap_heap_peek(theap)==heap) return theap;
  #else
  if mi_likely(mi_theap_heap_peek(theap)==heap) return theap;
  #endif
  return _mi_heap_theap_get_or_init(heap);
}

static inline mi_theap_t* _mi_heap_theap_cached(mi_heap_t* heap) {
  mi_theap_t* theap = _mi_theap_cached();
  #if MI_THEAP_INITASNULL
  if mi_likely(theap!=NULL && mi_theap_heap_peek(theap)==heap) return theap;
  #else
  if mi_likely(mi_theap_heap_peek(theap)==heap) return theap;
  #endif
  return NULL;
}

// Get the theap belonging to a heap without creating it if it is not yet initialized.
static inline mi_theap_t* _mi_heap_theap_peek(const mi_heap_t* heap) {
  mi_theap_t* theap = _mi_theap_cached();
  #if MI_THEAP_INITASNULL
  if mi_likely(theap!=NULL && mi_theap_heap_peek(theap)==heap) return theap;
  #else
  if mi_likely(mi_theap_heap_peek(theap)==heap) return theap;
  #endif
  theap = (mi_theap_t*)_mi_thread_local_get(heap->theap);  // don't update the cache on a query
  mi_assert_internal(theap==NULL || (!_mi_is_empty_theap(theap) && theap->heap==heap));
  return theap;
}

// Find the associated theap or NULL if it does not exist (during shutdown)
// Should be fast as it is called in `alloc/free.c:mi_free_try_collect`.
static inline mi_theap_t* _mi_page_associated_theap_peek(const mi_page_t* page) {
  mi_heap_t* const heap = page->heap;
  mi_theap_t* const theap = (mi_theap_t*)_mi_thread_local_get(heap->theap);
  if (theap==NULL || theap->tld==NULL /* heap destroy */) return NULL;
  if (theap->heap != heap) return NULL; // should never happen, but can happen for a free across subprocesses, which can happen during pthread tls storage deallocation
  mi_assert_internal(!_mi_is_empty_theap(theap) && mi_theap_matches_thread(theap));
  // note: for pages allocated by a detached theap, the returned theap may not be detached
  return theap;
}

#endif  // MI_PRIM_TLS_H
