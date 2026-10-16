/* ----------------------------------------------------------------------------
Copyright (c) 2019-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
  Implement dynamic thread local variables (used by heap's for their theap's).
  Unlike most OS native implementations there is no limit on the number
  that can be allocated.
-----------------------------------------------------------------------------*/

#include "mimalloc.h"
#include "mimalloc/prim/prim-tls.h"
#include "mimalloc/util/libc.h"
#include "mimalloc/util/options.h"
#include "mimalloc/util/threadlocal.h"

/* -----------------------------------------------------------
  Each thread can have (a dynamically expanding) array of
  thread-local values. Each slot has a value and a version.
  The version is used to safely reuse slots.
----------------------------------------------------------- */
typedef struct mi_tls_slot_s {
  size_t  version;
  void*   value;
} mi_tls_slot_t;

typedef struct mi_thread_locals_s {
  size_t        count;
  mi_memid_t    memid;
  mi_tls_slot_t slots[1];
} mi_thread_locals_t;

static mi_thread_locals_t mi_thread_locals_empty = mi_init_struct_zero;


/* -----------------------------------------------------------
  We have 2 thread local variables which we implement with either
  a C thread local declaration or using pthread keys.
  - mi_thread_locals: points to an array of thread locals for most keys
  - mi_slot_fast: a single dedicated thread local for slightly faster access. (used for the main heap's theap)
----------------------------------------------------------- */

#if MI_TLS_MODEL_PTHREADS || defined(__APPLE__)   // macOS has fast pthreads
// Use pthreads
#define mi_define_thread_local(tp,name,initval) \
  static pthread_key_t __##name##_key = MI_PTHREAD_KEY_INVALID; \
  static inline tp   name##_peek(void)    { return (tp)mi_pthread_key_get(__##name##_key); } \
  static inline tp   name##_get(void)     { tp result = name##_peek(); return (result!=NULL ? result : initval); } \
  static inline bool name##_set(tp val)   { return mi_pthread_key_set(&__##name##_key,val); } \
  static inline void name##_delete(void)  { mi_pthread_key_delete(&__##name##_key); }

#else
// Direct thread locals
#define mi_define_thread_local(tp,name,initval) \
  static mi_decl_thread tp __##name = initval; \
  static inline tp   name##_peek(void)    { return __##name; } \
  static inline tp   name##_get(void)     { tp result = __##name; return (result!=NULL ? result : initval); } \
  static inline bool name##_set(tp val)   { __##name = val; return true; } \
  static inline void name##_delete(void)  {  }
#endif

mi_define_thread_local(mi_thread_locals_t*, mi_thread_locals, &mi_thread_locals_empty)
mi_define_thread_local(void*, mi_slot_fast, NULL)


/* -----------------------------------------------------------
  Each key consists of the slot index in the lower bits,
  and its version it the top bits. When we get a value
  the version must match or we return NULL. When we set
  a value, we also set the version of the key.
----------------------------------------------------------- */

#if MI_SIZE_BITS >= 64
#define MI_TLS_IDX_BITS     (MI_SIZE_BITS/4)      /* 16 bits for the index, 48 bits for the version */
#elif MI_SIZE_BITS >= 32
#define MI_TLS_IDX_BITS     (12)                  /* 12 bits for index, 20 for the version? */
#else
#error not enough bits for the version for thread locals
#endif
#define MI_TLS_IDX_MASK     ((MI_ZU(1)<<MI_TLS_IDX_BITS)-1)
#define MI_TLS_IDX_MAX      MI_TLS_IDX_MASK
#define MI_TLS_VERSION_MAX  ((MI_ZU(1)<<(MI_SIZE_BITS - MI_TLS_IDX_BITS))-1)


static size_t mi_key_index( size_t key ) {
  return (key & MI_TLS_IDX_MASK);
}

static size_t mi_key_version( size_t key ) {
  return (key >> MI_TLS_IDX_BITS);
}

static mi_thread_local_t mi_key_create( size_t index, size_t version ) {
  mi_assert_internal(version != 0 && version <= MI_TLS_VERSION_MAX);
  mi_assert_internal(index <= MI_TLS_IDX_MAX);
  const mi_thread_local_t key = ((version << MI_TLS_IDX_BITS) | index);
  mi_assert_internal(key != 0);
  return key;
}


// dynamically reallocate the thread local slots when needed
static mi_thread_locals_t* mi_thread_locals_expand(size_t least_idx) {
  mi_thread_locals_t* tls_old = mi_thread_locals_get();
  const size_t count_old = tls_old->count;
  size_t count;
  if (count_old==0) {
    tls_old = NULL; // so we allocate fresh from mi_thread_locals_empty
    count = 16;     // start with 16 slots
  }
  else if (count_old >= 1024) {
    count = count_old + 1024;  // at some point increase linearly
  }
  else {
    count = 2*count_old;       // and double initially
  }
  if (count <= least_idx) {
    count = least_idx + 1;
  }
  if (count > MI_TLS_IDX_MAX) { return NULL; }  // too large
  // allocate as meta (for secure mode)
  // we could also allocate on the main heap; this is recursion safe as that uses the fast local key
  mi_memid_t memid = (tls_old==NULL ? _mi_memid_none() : tls_old->memid);
  mi_thread_locals_t* tls = (mi_thread_locals_t*)_mi_meta_rezalloc(_mi_subproc(), tls_old, sizeof(mi_thread_locals_t) + count*sizeof(mi_tls_slot_t), &memid);
  if mi_unlikely(tls==NULL) return NULL;
  tls->memid = memid;
  tls->count = count;
  mi_thread_locals_set(tls);
  return tls;
}

static mi_decl_noinline bool mi_thread_local_set_expand( mi_thread_local_t key, void* val ) {
  if (val==NULL) return true;
  const size_t idx = mi_key_index(key);
  mi_thread_locals_t* tls = mi_thread_locals_expand(idx);
  if (tls==NULL) {
    _mi_error_message(EFAULT,"unable to allocate thread local variables\n");
    return false;
  }
  mi_assert_internal(tls == mi_thread_locals_get());
  mi_assert_internal(idx < tls->count);
  tls->slots[idx].value = val;
  tls->slots[idx].version = mi_key_version(key);
  return true;
}

// set a tls slot; returns `true` if successful.
// Can return `false` if we could not reallocate the slots array.
static mi_decl_noinline bool mi_thread_local_set_regular( mi_thread_local_t key, void* val ) {
  mi_thread_locals_t* tls = mi_thread_locals_get();
  mi_assert_internal(tls!=NULL);
  mi_assert_internal(key!=0);
  const size_t idx = mi_key_index(key);
  if mi_likely(idx < tls->count) {
    tls->slots[idx].value = val;
    tls->slots[idx].version = mi_key_version(key);
    return true;
  }
  else {
    return mi_thread_local_set_expand( key, val );  // tailcall
  }
}

bool _mi_thread_local_set( mi_thread_local_t key, void* val ) {
  mi_assert_internal(key!=0);
  if (key == mi_thread_local_key_fast) {
    return mi_slot_fast_set(val);
  }
  else {
    return mi_thread_local_set_regular(key,val);
  }
}

// get a tls slot value
static mi_decl_noinline void* mi_thread_local_get_regular( mi_thread_local_t key ) {
  const mi_thread_locals_t* const tls = mi_thread_locals_peek();
  if mi_unlikely(tls==NULL) {
    // this can happen if a thread local is accessed after the thread local has been freed
    // from mi_thread_done or mi_process_done.
    // todo: can we remove this check? now we can still call this from process done when stats are printed (which calls mi_heap_theap_peek)
    return NULL;
  }
  const size_t idx = mi_key_index(key);
  if mi_likely(idx < tls->count && mi_key_version(key) == tls->slots[idx].version) {
    return tls->slots[idx].value;
  }
  else {
    return NULL;
  }
}

// get a thread local value
void* _mi_thread_local_get( mi_thread_local_t key ) {
  if mi_likely(key == mi_thread_local_key_fast) {
    return mi_slot_fast_get();
  }
  else {
    void* p = mi_thread_local_get_regular(key);
    if (key==0) { mi_assert_internal(p==NULL); }
    return p;
  }
}

void _mi_thread_locals_thread_done(void) {
  mi_thread_locals_t* const tls = mi_thread_locals_peek();
  if (tls!=NULL && tls->count > 0) {
    _mi_meta_free(_mi_subproc(), tls, tls->memid);
    mi_thread_locals_set(NULL);
  }
  if (mi_slot_fast_peek() != NULL) {
    mi_slot_fast_set(NULL);
  }
}

/* -----------------------------------------------------------
Create and free fresh TLS key's
----------------------------------------------------------- */
#include "../arena/bitmap.h"

static mi_lock_t    mi_thread_locals_lock;    // we need a lock in order to re-allocate the slot bits
static mi_bitmap_t* mi_thread_locals_free;    // reuse an arena bitmap to track which slots were assigned (1=free, 0=in-use)
static mi_memid_t   mi_thread_locals_memid;   // provenance of mi_thread_locals_free
static size_t       mi_thread_locals_version; // version to be able to reuse slots safely

void _mi_thread_locals_init(void) {
  mi_lock_init(&mi_thread_locals_lock);
}

void _mi_thread_locals_done(void) {
  mi_lock(&mi_thread_locals_lock) {
    mi_bitmap_t* const slots = mi_thread_locals_free;
    if (slots!=NULL) {
      _mi_meta_free(_mi_subproc_main(), slots, mi_thread_locals_memid);
    }
  }
  mi_lock_done(&mi_thread_locals_lock);
  mi_thread_locals_delete();
  mi_slot_fast_delete();
}

// strange signature but allows us to reuse the arena code for claiming free pages
static bool mi_thread_local_claim_fun(size_t _slice_index, mi_arena_t* _arena, bool* keep_set) {
  MI_UNUSED(_slice_index); MI_UNUSED(_arena);
  *keep_set = false;
  return true;
}

// When we claim a free slot, we increase the global version counter
// (so if we reuse a slot it will be returning NULL initially when a thread tries to get it)
static mi_thread_local_t mi_thread_local_claim(void) {
  size_t idx = 0;
  if (mi_thread_locals_free != NULL && mi_bitmap_try_find_and_claim(mi_thread_locals_free,0,&idx,&mi_thread_local_claim_fun,NULL)) {
    mi_thread_locals_version++;
    if (mi_thread_locals_version >= MI_TLS_VERSION_MAX) { mi_thread_locals_version = 1; }  /* wrap around the version */
    return mi_key_create( idx, mi_thread_locals_version);
  }
  else {
    return 0;
  }
}

static bool mi_thread_local_create_expand(void) {
  mi_bitmap_t* const slots = mi_thread_locals_free;
  // 1024 bits at a time
  const size_t oldcount = (slots==NULL ? 0 : mi_bitmap_max_bits(slots));
  const size_t newcount = 1024 + oldcount;
  if (newcount > MI_TLS_IDX_MAX) { return false; }
  const size_t newsize = mi_bitmap_size( newcount, NULL );
  // mi_bitmap_t* newslots = (mi_bitmap_t*)mi_zalloc_aligned(newsize, MI_BCHUNK_SIZE);
  mi_memid_t memid;
  mi_bitmap_t* newslots = (mi_bitmap_t*)_mi_meta_zalloc_aligned(_mi_subproc_main(), newsize, MI_BCHUNK_SIZE, &memid); // always allocate thread locals in the main subprocess
  mi_assert_internal(mi_is_aligned(newslots,MI_BCHUNK_SIZE));
  if (newslots==NULL) { return false; }
  if (slots!=NULL) {
    // copy over the previous bitmap
    const size_t oldsize = mi_bitmap_size(oldcount,NULL);
    mi_memcpy_aligned(newslots, slots, oldsize);
    _mi_meta_free(_mi_subproc_main(), slots, mi_thread_locals_memid);
  }
  mi_bitmap_init(newslots, newcount, true /* pretend already zero'd so we do not zero out the copied old entries */);
  mi_bitmap_unsafe_setN(newslots, oldcount, newcount - oldcount);  /* set the new expanded slots as available */
  mi_thread_locals_free = newslots;
  mi_thread_locals_memid = memid;
  return true;
}


// create a fresh key
mi_thread_local_t _mi_thread_local_create(void) {
  mi_thread_local_t key = 0;
  mi_lock(&mi_thread_locals_lock) {
    key = mi_thread_local_claim();
    if (key==0) {
      if (mi_thread_local_create_expand()) {
        key = mi_thread_local_claim();
      }
    }
  }
  mi_assert_internal(key!=0);
  mi_assert_internal(key!=mi_thread_local_key_fast);
  return key;
}

// free a key
void _mi_thread_local_free(mi_thread_local_t key) {
  if (key==0) return;
  const size_t idx = mi_key_index(key);
  mi_lock(&mi_thread_locals_lock) {
    mi_bitmap_t* const slots = mi_thread_locals_free;
    if (slots!=NULL && idx < mi_bitmap_max_bits(slots)) {
      mi_bitmap_set(slots,idx);
    }
  }
}
