# Free Threading Building Blocks

This document describes the low-level primitives used to implement
free-threaded (nogil) CPython. These are internal implementation details
and may change between versions.

For a higher-level overview of free threading aimed at Python users, see
the [free-threading HOWTO](../Doc/howto/free-threading-python.rst). For
C extension authors, see the
[free-threading extensions HOWTO](../Doc/howto/free-threading-extensions.rst).


## Object Header Layout

In free-threaded builds (`Py_GIL_DISABLED`), every `PyObject` has an
extended header (defined in `Include/object.h`):

```c
struct _object {
    // ob_tid stores the thread id (or zero). It is also used by the GC and the
    // trashcan mechanism as a linked list pointer and by the GC to store the
    // computed "gc_refs" refcount.
    uintptr_t ob_tid;          // (declared with _Py_ALIGNED_DEF for alignment)
    uint16_t ob_flags;
    PyMutex ob_mutex;           // per-object lock
    uint8_t ob_gc_bits;         // gc-related state
    uint32_t ob_ref_local;      // local reference count
    Py_ssize_t ob_ref_shared;   // shared (atomic) reference count
    PyTypeObject *ob_type;
};
```

Key fields:

- **`ob_tid`**: The thread id of the owning thread. Set to
  `_Py_UNOWNED_TID` (0) for immortal objects or objects whose reference
  count fields have been merged. Also reused by the GC and the trashcan
  mechanism as a linked list pointer.

- **`ob_mutex`**: A one-byte mutex (`PyMutex`) used by the critical
  section API. Must not be locked directly; use `Py_BEGIN_CRITICAL_SECTION`
  instead (see [Per-Object Locking](#per-object-locking-critical-sections)
  below).

- **`ob_ref_local`**: Reference count for references held by the owning
  thread. Non-atomic — only the owning thread modifies it. A value of
  `UINT32_MAX` (`_Py_IMMORTAL_REFCNT_LOCAL`) indicates an immortal object.

- **`ob_ref_shared`**: Atomic reference count for cross-thread references.
  The two least-significant bits store state flags (see
  [Shared Reference Count Flags](#shared-reference-count-flags) below).
  The actual reference count is stored in the upper bits, shifted by
  `_Py_REF_SHARED_SHIFT` (2).

- **`ob_gc_bits`**: Bit flags for the garbage collector (see
  [GC Bit Flags](#gc-bit-flags) below).


## Thread Ownership

Each object is "owned" by the thread that created it. The owning thread
can use fast, non-atomic operations on `ob_ref_local`. Other threads
must use atomic operations on `ob_ref_shared`.

| Primitive | Header | Description |
|---|---|---|
| `_Py_ThreadId()` | `Include/object.h` | Returns the current thread's id (`uintptr_t`) using platform-specific TLS. |
| `_Py_IsOwnedByCurrentThread(op)` | `Include/object.h` | Returns non-zero if `op->ob_tid` matches the current thread. Uses an atomic load under ThreadSanitizer. |
| `_Py_UNOWNED_TID` | `Include/object.h` | Constant (0) indicating no owning thread — used for immortal objects and objects with merged refcounts. |


## Reference Counting Primitives

Free-threaded CPython splits the reference count into a local part
(`ob_ref_local`) and a shared part (`ob_ref_shared`). The owning thread
increments `ob_ref_local` directly. Other threads atomically update
`ob_ref_shared`.

### Incrementing References

| Primitive | Header | Description |
|---|---|---|
| `_Py_TryIncrefFast(op)` | `pycore_object.h` | Tries to increment the refcount using the fast (non-atomic) path. Succeeds only for immortal objects or objects owned by the current thread. Returns 0 if the object requires an atomic operation. |
| `_Py_TryIncRefShared(op)` | `pycore_object.h` | Atomically increments `ob_ref_shared` using a compare-and-exchange loop. Fails (returns 0) if the raw shared field is 0 or `_Py_REF_MERGED` (0x3). |
| `_Py_TryIncrefCompare(src, op)` | `pycore_object.h` | Increfs `op` (trying fast path, then shared) and verifies that `*src` still points to `op`. If `*src` changed concurrently, decrefs `op` and returns 0. |
| `_Py_TryIncref(op)` | `pycore_object.h` | Convenience wrapper: tries `_Py_TryIncrefFast`, then falls back to `_Py_TryIncRefShared`. In GIL builds, checks `Py_REFCNT(op) > 0` and calls `Py_INCREF`. |

### Safe Pointer Loading

These load a `PyObject *` from a location that may be concurrently
updated by another thread, and return it with an incremented reference
count:

| Primitive | Header | Description |
|---|---|---|
| `_Py_XGetRef(ptr)` | `pycore_object.h` | Loads `*ptr` and increfs it, retrying in a loop until it succeeds or `*ptr` is NULL. The writer must set `_Py_REF_MAYBE_WEAKREF` on the stored object. |
| `_Py_TryXGetRef(ptr)` | `pycore_object.h` | Single-attempt version of `_Py_XGetRef`. Returns NULL on failure, which may be due to a NULL value or a concurrent update. |
| `_Py_NewRefWithLock(op)` | `pycore_object.h` | Like `Py_NewRef` but also optimistically sets `_Py_REF_MAYBE_WEAKREF` on objects owned by a different thread. Uses the fast path for local/immortal objects, falls back to an atomic CAS loop on `ob_ref_shared`. |

### Shared Reference Count Flags

The two least-significant bits of `ob_ref_shared` encode state
(defined in `Include/refcount.h`):

| Flag | Value | Meaning |
|---|---|---|
| `_Py_REF_SHARED_INIT` | `0x0` | Initial state, no flags set. |
| `_Py_REF_MAYBE_WEAKREF` | `0x1` | Object may have weak references. |
| `_Py_REF_QUEUED` | `0x2` | Queued for refcount merging by the owning thread. |
| `_Py_REF_MERGED` | `0x3` | Local and shared refcounts have been merged. |

Helpers:

| Primitive | Header | Description |
|---|---|---|
| `_Py_REF_IS_MERGED(ob_ref_shared)` | `pycore_object.h` | True if the low two bits equal `_Py_REF_MERGED`. |
| `_Py_REF_IS_QUEUED(ob_ref_shared)` | `pycore_object.h` | True if the low two bits equal `_Py_REF_QUEUED`. |
| `_Py_ExplicitMergeRefcount(op, extra)` | `pycore_object.h` | Merges the local and shared reference count fields, adding `extra` to the refcount when merging. |
| `_PyObject_SetMaybeWeakref(op)` | `pycore_object.h` | Atomically sets `_Py_REF_MAYBE_WEAKREF` on `ob_ref_shared` if no flags are currently set. No-op for immortal objects or objects already in WEAKREF/QUEUED/MERGED state. |


## Biased Reference Counting

Each object has two refcount fields: `ob_ref_local` (modified only by
the owning thread) and `ob_ref_shared` (modified atomically by all
other threads). The true refcount is the sum of both fields. The
`ob_ref_shared` field can be negative (although the total refcount must
be at least zero).

When a non-owning thread calls `Py_DECREF`, it calls `_Py_DecRefShared`
which decrements `ob_ref_shared`. If `ob_ref_shared` is already zero
(or only has the `_Py_REF_MAYBE_WEAKREF` flag set, meaning the count
portion is zero), the thread does not subtract from the refcount.
Instead, it sets the `_Py_REF_QUEUED` flag and enqueues the object for
the owning thread to merge. The queue holds a reference to the object.
The owning thread is notified via the eval breaker mechanism.

When the owning thread processes its merge queue (via
`_Py_brc_merge_refcounts`), it calls `_Py_ExplicitMergeRefcount` for
each queued object, which merges `ob_ref_local` and `ob_ref_shared`
into a single value, sets `ob_tid` to 0, and sets the `_Py_REF_MERGED`
flag. If the merged refcount is zero, the object is deallocated.

Similarly, when the owning thread's own `Py_DECREF` drops `ob_ref_local`
to zero, it calls `_Py_MergeZeroLocalRefcount`, which either deallocates
immediately (if `ob_ref_shared` is also zero) or gives up ownership by
setting `ob_tid` to 0 and `_Py_REF_MERGED`, and deallocates if the
combined refcount is zero.

If `ob_ref_shared` is nonzero when a non-owning thread decrements it,
the decrement subtracts normally — the shared count can go negative.

Defined in `Include/internal/pycore_brc.h`:

| Primitive | Description |
|---|---|
| `_Py_brc_queue_object(ob)` | Enqueues `ob` to be merged by its owning thread. Steals a reference to the object. |
| `_Py_brc_merge_refcounts(tstate)` | Merges the refcounts of all objects queued for the current thread. |

The per-interpreter state (`struct _brc_state`) uses a hash table of
`_Py_BRC_NUM_BUCKETS` (257) buckets keyed by thread id. Each bucket is
protected by its own `PyMutex`. Per-thread state
(`struct _brc_thread_state`) stores a stack of objects to merge.


## Per-Object Locking (Critical Sections)

Instead of locking `ob_mutex` directly, code must use the **critical
section API**. Critical sections integrate with the thread state to
support suspension (e.g., when a thread needs to release locks for GC)
and deadlock avoidance.

Defined in `Include/cpython/critical_section.h` (public) and
`Include/internal/pycore_critical_section.h` (internal):

| Macro | Description |
|---|---|
| `Py_BEGIN_CRITICAL_SECTION(op)` | Acquires `op->ob_mutex` via the critical section stack. |
| `Py_END_CRITICAL_SECTION()` | Releases the lock and pops the critical section. |
| `Py_BEGIN_CRITICAL_SECTION2(a, b)` | Acquires locks on two objects. Sorts mutexes by address to prevent deadlocks. If both arguments are the same object, degrades to a single-mutex critical section. |
| `Py_END_CRITICAL_SECTION2()` | Releases both locks. |
| `Py_BEGIN_CRITICAL_SECTION_SEQUENCE_FAST(op)` | Specialized variant for `PySequence_Fast` — only locks if `op` is a list (`PyList_CheckExact`). Tuples are immutable and need no locking. |

All critical section macros are **no-ops** in GIL-enabled builds (they
expand to just `{` and `}`).

### PyMutex

`PyMutex` (defined in `Include/cpython/pylock.h`) is a one-byte mutex:

```
Bit layout (only the two least significant bits are used):
  0b00  unlocked
  0b01  locked
  0b10  unlocked and has parked threads
  0b11  locked and has parked threads
```

The fast path uses a single compare-and-swap. If contended, the calling
thread is "parked" until the mutex is unlocked. If the current thread
holds the GIL, the GIL is released while the thread is parked.

| Function | Description |
|---|---|
| `PyMutex_Lock(m)` | Locks the mutex. Parks the calling thread if contended. |
| `PyMutex_Unlock(m)` | Unlocks the mutex. Wakes a parked thread if any. |
| `PyMutex_IsLocked(m)` | Returns non-zero if currently locked. |


## Immortalization

Immortal objects are never deallocated and their reference counts are
never modified. In free-threaded builds, immortality is indicated by
`ob_ref_local == UINT32_MAX` (`_Py_IMMORTAL_REFCNT_LOCAL`).

Defined in `Include/refcount.h` and `Include/internal/pycore_object.h`:

| Primitive | Description |
|---|---|
| `_Py_IsImmortal(op)` | True if `op` is immortal. |
| `_Py_IsStaticImmortal(op)` | True if `op` was statically allocated as immortal. |
| `_Py_SetImmortal(op)` | Promotes `op` to immortal. |
| `_Py_SetMortal(op, refcnt)` | Demotes `op` back to mortal. Should only be used during runtime finalization. |


## Deferred Reference Counting

Frequently shared objects (e.g., top-level functions, types, modules)
use **deferred reference counting** to avoid contention on their
reference count fields. These objects add `_Py_REF_DEFERRED`
(`PY_SSIZE_T_MAX / 8`) to `ob_ref_shared` so that they are not
immediately deallocated when the non-deferred reference count drops to
zero. They are only freed during cyclic garbage collection.

Defined in `Include/internal/pycore_object_deferred.h`:

| Primitive | Description |
|---|---|
| `_PyObject_SetDeferredRefcount(op)` | Marks `op` as using deferred reference counting. Objects that use deferred reference counting should be tracked by the GC so that they are eventually collected. No-op in GIL builds. |
| `_PyObject_HasDeferredRefcount(op)` | True if `op` uses deferred reference counting (checks `_PyGC_BITS_DEFERRED`). Always returns 0 in GIL builds. |

The GC scans each thread's evaluation stack and local variables to keep
deferred-refcounted objects alive.

See also: [Stack references (_PyStackRef)](stackrefs.md) for how
deferred reference counting interacts with the evaluation stack.


## GC Bit Flags

In free-threaded builds, GC state is stored in `ob_gc_bits` rather than
a separate `PyGC_Head` linked list. Defined in
`Include/internal/pycore_gc.h`:

| Flag | Bit | Meaning |
|---|---|---|
| `_PyGC_BITS_TRACKED` | 0 | Tracked by the GC. |
| `_PyGC_BITS_FINALIZED` | 1 | `tp_finalize` was called. |
| `_PyGC_BITS_UNREACHABLE` | 2 | Object determined unreachable during collection. |
| `_PyGC_BITS_FROZEN` | 3 | Object is frozen (not collected). |
| `_PyGC_BITS_SHARED` | 4 | Object is shared between threads. |
| `_PyGC_BITS_ALIVE` | 5 | Reachable from a known root. |
| `_PyGC_BITS_DEFERRED` | 6 | Uses deferred reference counting. |

Setting the bits requires a relaxed store and the per-object lock must
be held (except when the object is only visible to a single thread,
e.g., during initialization or destruction). Reading the bits requires
a relaxed load but does not require the per-object lock.

Helpers:

| Primitive | Description |
|---|---|
| `_PyObject_SET_GC_BITS(op, bits)` | Sets the specified bits (atomic relaxed load-modify-store). |
| `_PyObject_HAS_GC_BITS(op, bits)` | True if the specified bits are set (atomic relaxed load). |
| `_PyObject_CLEAR_GC_BITS(op, bits)` | Clears the specified bits (atomic relaxed load-modify-store). |


## Atomic Operations

### Unconditional Atomics (`_Py_atomic_*`)

Full atomic operations defined in `Include/cpython/pyatomic.h`. These
are always atomic regardless of build configuration. They support
multiple memory orderings (relaxed, acquire, release, seq_cst).

### Free-Threading Wrappers (`FT_ATOMIC_*`)

Defined in `Include/internal/pycore_pyatomic_ft_wrappers.h`. These
are **atomic in free-threaded builds** and plain reads/writes in
GIL-enabled builds:

```c
// Free-threaded build:
#define FT_ATOMIC_LOAD_PTR(value)  _Py_atomic_load_ptr(&value)
#define FT_MUTEX_LOCK(lock)        PyMutex_Lock(lock)

// GIL-enabled build:
#define FT_ATOMIC_LOAD_PTR(value)  value
#define FT_MUTEX_LOCK(lock)        do {} while (0)
```

Use `FT_ATOMIC_*` when protecting data that is inherently safe under
the GIL but needs atomics in free-threaded builds. Use `_Py_atomic_*`
when the operation must always be atomic.


## QSBR (Quiescent-State Based Reclamation)

QSBR is used to safely reclaim memory that has been logically removed
from a data structure but may still be accessed by concurrent readers
(e.g., resized list backing arrays, dictionary keys).

See the dedicated document: [Quiescent-State Based Reclamation (QSBR)](qsbr.md).


## Key Source Files

| File | Contents |
|---|---|
| `Include/object.h` | Object header, `_Py_ThreadId`, `_Py_IsOwnedByCurrentThread` |
| `Include/refcount.h` | Immortalization constants, `_Py_REF_SHARED_*` flags, `Py_INCREF`/`Py_DECREF` |
| `Include/cpython/pylock.h` | `PyMutex` definition and lock/unlock |
| `Include/cpython/critical_section.h` | Public critical section macros |
| `Include/cpython/pyatomic.h` | Unconditional atomic operations |
| `Include/internal/pycore_object.h` | `_Py_TryIncrefFast`, `_Py_TryXGetRef`, `_Py_XGetRef`, etc. |
| `Include/internal/pycore_critical_section.h` | Internal critical section implementation |
| `Include/internal/pycore_brc.h` | Biased reference counting (per-thread merge queues) |
| `Include/internal/pycore_object_deferred.h` | Deferred reference counting |
| `Include/internal/pycore_gc.h` | GC bit flags and helpers |
| `Include/internal/pycore_pyatomic_ft_wrappers.h` | `FT_ATOMIC_*` wrappers |
| `Include/internal/pycore_qsbr.h` | QSBR implementation |
| `Python/gc_free_threading.c` | Free-threaded garbage collector |
| `Python/critical_section.c` | Critical section slow paths |
