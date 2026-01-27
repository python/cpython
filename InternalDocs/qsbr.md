# Quiescent-State Based Reclamation (QSBR)

## Introduction

When implementing lock-free data structures, a key challenge is determining
when it is safe to free memory that has been logically removed from a
structure. Freeing memory too early can lead to use-after-free bugs if another
thread is still accessing it. Freeing it too late results in excessive memory
consumption.

Safe memory reclamation (SMR) schemes address this by delaying the free
operation until all concurrent read accesses are guaranteed to have completed.
Quiescent-State Based Reclamation (QSBR) is a SMR scheme used in Python's
free-threaded build to manage the lifecycle of shared memory.

QSBR requires threads to periodically report that they are in a quiescent
state. A thread is in a quiescent state if it holds no references to shared
objects that might be reclaimed. Think of it as a checkpoint where a thread
signals, "I am not in the middle of any operation that relies on a shared
resource." In Python, the eval_breaker provides a natural and convenient place
for threads to report this state.


## Use in Free-Threaded Python

While CPython's memory management is dominated by reference counting and a
tracing garbage collector, these mechanisms are not suitable for all data
structures. For example, the backing array of a list object is not individually
reference-counted but may have a shorter lifetime than the `PyListObject` that
contains it. We could delay reclamation until the next GC run, but we want
reclamation to be prompt and to run the GC less frequently in the free-threaded
build, as it requires pausing all threads.

Many operations in the free-threaded build are protected by locks. However, for
performance-critical code, we want to allow reads to happen concurrently with
updates. For instance, we want to avoid locking during most list read accesses.
If a list is resized while another thread is reading it, QSBR provides the
mechanism to determine when it is safe to free the list's old backing array.

Specific use cases for QSBR include:

* Dictionary keys (`PyDictKeysObject`) and list arrays (`_PyListArray`): When a
dictionary or list that may be shared between threads is resized, we use QSBR
to delay freeing the old keys or array until it's safe. For dicts and lists
that are not shared, their storage can be freed immediately upon resize.

* Mimalloc `mi_page_t`: Non-locking dictionary and list accesses require
cooperation from the memory allocator. If an object is freed and its memory is
reused, we must ensure the new object's reference count field is at the same
memory location. In practice, this means when a mimalloc page (`mi_page_t`)
becomes empty, we don't immediately allow it to be reused for allocations of a
different size class. QSBR is used to determine when it's safe to repurpose the
page or return its memory to the OS.


## Implementation Details


### Core Implementation

The proposal to add QSBR to Python is contained in
[Github issue 115103](https://github.com/python/cpython/issues/115103).
Many details of that proposal have been copied here, so they can be kept
up-to-date with the actual implementation.

Python's QSBR implementation is based on FreeBSD's "Global Unbounded
Sequences." [^1][^2][^3].  It relies on a few key counters:

* Global Write Sequence (`wr_seq`): A per-interpreter counter, `wr_seq`, is started
at 1 and incremented by 2 each time it is advanced. This ensures its value is
always odd, which can be used to distinguish it from other state values. When
an object needs to be reclaimed, `wr_seq` is advanced, and the object is tagged
with this new sequence number.

* Per-Thread Read Sequence: Each thread has a local read sequence counter. When
a thread reaches a quiescent state (e.g., at the eval_breaker), it copies the
current global `wr_seq` to its local counter.

* Global Read Sequence (`rd_seq`): This per-interpreter value stores the minimum
of all per-thread read sequence counters (excluding detached threads). It is
updated by a "polling" operation.

To free an object, the following steps are taken:

1. Advance the global `wr_seq`.

2. Add the object's pointer to a deferred-free list, tagging it with the new
   `wr_seq` value as its qsbr_goal.

Periodically, a polling mechanism processes this deferred-free list:

1. The minimum read sequence value across all active threads is calculated and
   stored as the global `rd_seq`.

2. For each item on the deferred-free list, if its qsbr_goal is less than or
   equal to the new `rd_seq`, its memory is freed, and it is removed from the:
   list. Otherwise, it remains on the list for a future attempt.


### Deferred Advance Optimization

To reduce memory contention from frequent updates to the global `wr_seq`, its
advancement is sometimes deferred. Instead of incrementing `wr_seq` on every
reclamation request, each thread tracks its number of deferrals locally. Once
the deferral count reaches a limit (QSBR_DEFERRED_LIMIT, currently 10), the
thread advances the global `wr_seq` and resets its local count.

When an object is added to the deferred-free list, its qsbr_goal is set to
`wr_seq` + 2. By setting the goal to the next sequence value, we ensure it's safe
to defer the global counter advancement. This optimization improves runtime
speed but may increase peak memory usage by slightly delaying when memory can
be reclaimed.


## Limitations

Determining the `rd_seq` requires scanning over all thread states. This operation
could become a bottleneck in applications with a very large number of threads
(e.g., >1,000). Future work may address this with more advanced mechanisms,
such as a tree-based structure or incremental scanning. For now, the
implementation prioritizes simplicity, with plans for refinement if
multi-threaded benchmarks reveal performance issues.


## References

[^1]: https://youtu.be/ZXUIFj4nRjk?t=694
[^2]: https://people.kernel.org/joelfernandes/gus-vs-rcu
[^3]: http://bxr.su/FreeBSD/sys/kern/subr_smr.c#44
