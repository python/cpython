#include "Python.h"
#include "hashtable.h"
#include "frameobject.h"
#include "pythread.h"
#include "osdefs.h"

/* Trace memory blocks allocated by PyMem_RawMalloc() */
#define TRACE_RAW_MALLOC

/* Forward declaration */
static void tracemalloc_stop(void);
static void* raw_malloc(size_t size);
static void raw_free(void *ptr);

#ifdef Py_DEBUG
#  define TRACE_DEBUG
#endif

/* Protected by the GIL */
static struct {
    PyMemAllocatorEx mem;
    PyMemAllocatorEx raw;
    PyMemAllocatorEx obj;
} allocators;

static struct {
    /* Module initialized?
       Variable protected by the GIL */
    enum {
        TRACEMALLOC_NOT_INITIALIZED,
        TRACEMALLOC_INITIALIZED,
        TRACEMALLOC_FINALIZED
    } initialized;

    /* Is tracemalloc tracing memory allocations?
       Variable protected by the GIL */
    int tracing;

    /* limit of the number of frames in a traceback, 1 by default.
       Variable protected by the GIL. */
    int max_nframe;

    /* use domain in trace key?
       Variable protected by the GIL. */
    int use_domain;
} tracemalloc_config = {TRACEMALLOC_NOT_INITIALIZED, 0, 1, 0};

#if defined(TRACE_RAW_MALLOC) && defined(WITH_THREAD)
/* This lock is needed because tracemalloc_free() is called without
   the GIL held from PyMem_RawFree(). It cannot acquire the lock because it
   would introduce a deadlock in PyThreadState_DeleteCurrent(). */
static PyThread_type_lock tables_lock;
#  define TABLES_LOCK() PyThread_acquire_lock(tables_lock, 1)
#  define TABLES_UNLOCK() PyThread_release_lock(tables_lock)
#else
   /* variables are protected by the GIL */
#  define TABLES_LOCK()
#  define TABLES_UNLOCK()
#endif


#define DEFAULT_DOMAIN 0

/* Pack the frame_t structure to reduce the memory footprint. */
typedef struct
#ifdef __GNUC__
__attribute__((packed))
#endif
{
    uintptr_t ptr;
    _PyTraceMalloc_domain_t domain;
} pointer_t;

/* Pack the frame_t structure to reduce the memory footprint on 64-bit
   architectures: 12 bytes instead of 16. */
typedef struct
#ifdef __GNUC__
__attribute__((packed))
#elif defined(_MSC_VER)
#pragma pack(push, 4)
#endif
{
    /* filename cannot be NULL: "<unknown>" is used if the Python frame
       filename is NULL */
    PyObject *filename;
    unsigned int lineno;
} frame_t;
#ifdef _MSC_VER
#pragma pack(pop)
#endif


typedef struct {
    Py_uhash_t hash;
    int nframe;
    frame_t frames[1];
} traceback_t;

#define TRACEBACK_SIZE(NFRAME) \
        (sizeof(traceback_t) + sizeof(frame_t) * (NFRAME - 1))

#define MAX_NFRAME \
        ((INT_MAX - (int)sizeof(traceback_t)) / (int)sizeof(frame_t) + 1)


static PyObject *unknown_filename = NULL;
static traceback_t tracemalloc_empty_traceback;

/* Trace of a memory block */
typedef struct {
    /* Size of the memory block in bytes */
    size_t size;

    /* Traceback where the memory block was allocated */
    traceback_t *traceback;
} trace_t;


/* Size in bytes of currently traced memory.
   Protected by TABLES_LOCK(). */
static size_t tracemalloc_traced_memory = 0;

/* Peak size in bytes of traced memory.
   Protected by TABLES_LOCK(). */
static size_t tracemalloc_peak_traced_memory = 0;

/* Hash table used as a set to intern filenames:
   PyObject* => PyObject*.
   Protected by the GIL */
static _Py_hashtable_t *tracemalloc_filenames = NULL;

/* Buffer to store a new traceback in traceback_new().
   Protected by the GIL. */
static traceback_t *tracemalloc_traceback = NULL;

/* Hash table used as a set to intern tracebacks:
   traceback_t* => traceback_t*
   Protected by the GIL */
static _Py_hashtable_t *tracemalloc_tracebacks = NULL;

/* pointer (void*) => trace (trace_t).
   Protected by TABLES_LOCK(). */
static _Py_hashtable_t *tracemalloc_traces = NULL;


#ifdef TRACE_DEBUG
static void
tracemalloc_error(const char *format, ...)
{
    va_list ap;
    fprintf(stderr, "tracemalloc: ");
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    fflush(stderr);
}
#endif


#if defined(WITH_THREAD) && defined(TRACE_RAW_MALLOC)
#define REENTRANT_THREADLOCAL

/* If your OS does not provide native thread local storage, you can implement
   it manually using a lock. Functions of thread.c cannot be used because
   they use PyMem_RawMalloc() which leads to a reentrant call. */
#if !(defined(_POSIX_THREADS) || defined(NT_THREADS))
#  error "need native thread local storage (TLS)"
#endif

static int tracemalloc_reentrant_key = -1;

/* Any non-NULL pointer can be used */
#define REENTRANT Py_True

static int
get_reentrant(void)
{
    void *ptr;

    assert(tracemalloc_reentrant_key != -1);
    ptr = PyThread_get_key_value(tracemalloc_reentrant_key);
    if (ptr != NULL) {
        assert(ptr == REENTRANT);
        return 1;
    }
    else
        return 0;
}

static void
set_reentrant(int reentrant)
{
    assert(reentrant == 0 || reentrant == 1);
    assert(tracemalloc_reentrant_key != -1);

    if (reentrant) {
        assert(!get_reentrant());
        PyThread_set_key_value(tracemalloc_reentrant_key, REENTRANT);
    }
    else {
        assert(get_reentrant());
        PyThread_set_key_value(tracemalloc_reentrant_key, NULL);
    }
}

#else

/* WITH_THREAD not defined: Python compiled without threads,
   or TRACE_RAW_MALLOC not defined: variable protected by the GIL */
static int tracemalloc_reentrant = 0;

static int
get_reentrant(void)
{
    return tracemalloc_reentrant;
}

static void
set_reentrant(int reentrant)
{
    assert(reentrant != tracemalloc_reentrant);
    tracemalloc_reentrant = reentrant;
}
#endif


static Py_uhash_t
hashtable_hash_pyobject(_Py_hashtable_t *ht, const void *pkey)
{
    PyObject *obj;

    _Py_HASHTABLE_READ_KEY(ht, pkey, obj);
    return PyObject_Hash(obj);
}


static int
hashtable_compare_unicode(_Py_hashtable_t *ht, const void *pkey,
                          const _Py_hashtable_entry_t *entry)
{
    PyObject *key1, *key2;

    _Py_HASHTABLE_READ_KEY(ht, pkey, key1);
    _Py_HASHTABLE_ENTRY_READ_KEY(ht, entry, key2);

    if (key1 != NULL && key2 != NULL)
        return (PyUnicode_Compare(key1, key2) == 0);
    else
        return key1 == key2;
}


static Py_uhash_t
hashtable_hash_pointer_t(_Py_hashtable_t *ht, const void *pkey)
{
    pointer_t ptr;
    Py_uhash_t hash;

    _Py_HASHTABLE_READ_KEY(ht, pkey, ptr);

    hash = (Py_uhash_t)_Py_HashPointer((void*)ptr.ptr);
    hash ^= ptr.domain;
    return hash;
}


static int
hashtable_compare_pointer_t(_Py_hashtable_t *ht, const void *pkey,
                            const _Py_hashtable_entry_t *entry)
{
    pointer_t ptr1, ptr2;

    _Py_HASHTABLE_READ_KEY(ht, pkey, ptr1);
    _Py_HASHTABLE_ENTRY_READ_KEY(ht, entry, ptr2);

    /* compare pointer before domain, because pointer is more likely to be
       different */
    return (ptr1.ptr == ptr2.ptr && ptr1.domain == ptr2.domain);

}


static _Py_hashtable_t *
hashtable_new(size_t key_size, size_t data_size,
              _Py_hashtable_hash_func hash_func,
              _Py_hashtable_compare_func compare_func)
{
    _Py_hashtable_allocator_t hashtable_alloc = {malloc, free};
    return _Py_hashtable_new_full(key_size, data_size, 0,
                                  hash_func, compare_func,
                                  &hashtable_alloc);
}


static void*
raw_malloc(size_t size)
{
    return allocators.raw.malloc(allocators.raw.ctx, size);
}

static void
raw_free(void *ptr)
{
    allocators.raw.free(allocators.raw.ctx, ptr);
}


static Py_uhash_t
hashtable_hash_traceback(_Py_hashtable_t *ht, const void *pkey)
{
    traceback_t *traceback;

    _Py_HASHTABLE_READ_KEY(ht, pkey, traceback);
    return traceback->hash;
}


static int
hashtable_compare_traceback(_Py_hashtable_t *ht, const void *pkey,
                            const _Py_hashtable_entry_t *entry)
{
    traceback_t *traceback1, *traceback2;
    const frame_t *frame1, *frame2;
    int i;

    _Py_HASHTABLE_READ_KEY(ht, pkey, traceback1);
    _Py_HASHTABLE_ENTRY_READ_KEY(ht, entry, traceback2);

    if (traceback1->nframe != traceback2->nframe)
        return 0;

    for (i=0; i < traceback1->nframe; i++) {
        frame1 = &traceback1->frames[i];
        frame2 = &traceback2->frames[i];

        if (frame1->lineno != frame2->lineno)
            return 0;

        if (frame1->filename != frame2->filename) {
            assert(PyUnicode_Compare(frame1->filename, frame2->filename) != 0);
            return 0;
        }
    }
    return 1;
}


static void
tracemalloc_get_frame(PyFrameObject *pyframe, frame_t *frame)
{
    PyCodeObject *code;
    PyObject *filename;
    _Py_hashtable_entry_t *entry;
    int lineno;

    frame->filename = unknown_filename;
    lineno = PyFrame_GetLineNumber(pyframe);
    if (lineno < 0)
        lineno = 0;
    frame->lineno = (unsigned int)lineno;

    code = pyframe->f_code;
    if (code == NULL) {
#ifdef TRACE_DEBUG
        tracemalloc_error("failed to get the code object of the frame");
#endif
        return;
    }

    if (code->co_filename == NULL) {
#ifdef TRACE_DEBUG
        tracemalloc_error("failed to get the filename of the code object");
#endif
        return;
    }

    filename = code->co_filename;
    assert(filename != NULL);
    if (filename == NULL)
        return;

    if (!PyUnicode_Check(filename)) {
#ifdef TRACE_DEBUG
        tracemalloc_error("filename is not a unicode string");
#endif
        return;
    }
    if (!PyUnicode_IS_READY(filename)) {
        /* Don't make a Unicode string ready to avoid reentrant calls
           to tracemalloc_malloc() or tracemalloc_realloc() */
#ifdef TRACE_DEBUG
        tracemalloc_error("filename is not a ready unicode string");
#endif
        return;
    }

    /* intern the filename */
    entry = _Py_HASHTABLE_GET_ENTRY(tracemalloc_filenames, filename);
    if (entry != NULL) {
        _Py_HASHTABLE_ENTRY_READ_KEY(tracemalloc_filenames, entry, filename);
    }
    else {
        /* tracemalloc_filenames is responsible to keep a reference
           to the filename */
        Py_INCREF(filename);
        if (_Py_HASHTABLE_SET_NODATA(tracemalloc_filenames, filename) < 0) {
            Py_DECREF(filename);
#ifdef TRACE_DEBUG
            tracemalloc_error("failed to intern the filename");
#endif
            return;
        }
    }

    /* the tracemalloc_filenames table keeps a reference to the filename */
    frame->filename = filename;
}


static Py_uhash_t
traceback_hash(traceback_t *traceback)
{
    /* code based on tuplehash() of Objects/tupleobject.c */
    Py_uhash_t x, y;  /* Unsigned for defined overflow behavior. */
    int len = traceback->nframe;
    Py_uhash_t mult = _PyHASH_MULTIPLIER;
    frame_t *frame;

    x = 0x345678UL;
    frame = traceback->frames;
    while (--len >= 0) {
        y = (Py_uhash_t)PyObject_Hash(frame->filename);
        y ^= (Py_uhash_t)frame->lineno;
        frame++;

        x = (x ^ y) * mult;
        /* the cast might truncate len; that doesn't change hash stability */
        mult += (Py_uhash_t)(82520UL + len + len);
    }
    x += 97531UL;
    return x;
}


static void
traceback_get_frames(traceback_t *traceback)
{
    PyThreadState *tstate;
    PyFrameObject *pyframe;

#ifdef WITH_THREAD
    tstate = PyGILState_GetThisThreadState();
#else
    tstate = PyThreadState_Get();
#endif
    if (tstate == NULL) {
#ifdef TRACE_DEBUG
        tracemalloc_error("failed to get the current thread state");
#endif
        return;
    }

    for (pyframe = tstate->frame; pyframe != NULL; pyframe = pyframe->f_back) {
        tracemalloc_get_frame(pyframe, &traceback->frames[traceback->nframe]);
        assert(traceback->frames[traceback->nframe].filename != NULL);
        traceback->nframe++;
        if (traceback->nframe == tracemalloc_config.max_nframe)
            break;
    }
}


static traceback_t *
traceback_new(void)
{
    traceback_t *traceback;
    _Py_hashtable_entry_t *entry;

#ifdef WITH_THREAD
    assert(PyGILState_Check());
#endif

    /* get frames */
    traceback = tracemalloc_traceback;
    traceback->nframe = 0;
    traceback_get_frames(traceback);
    if (traceback->nframe == 0)
        return &tracemalloc_empty_traceback;
    traceback->hash = traceback_hash(traceback);

    /* intern the traceback */
    entry = _Py_HASHTABLE_GET_ENTRY(tracemalloc_tracebacks, traceback);
    if (entry != NULL) {
        _Py_HASHTABLE_ENTRY_READ_KEY(tracemalloc_tracebacks, entry, traceback);
    }
    else {
        traceback_t *copy;
        size_t traceback_size;

        traceback_size = TRACEBACK_SIZE(traceback->nframe);

        copy = raw_malloc(traceback_size);
        if (copy == NULL) {
#ifdef TRACE_DEBUG
            tracemalloc_error("failed to intern the traceback: malloc failed");
#endif
            return NULL;
        }
        memcpy(copy, traceback, traceback_size);

        if (_Py_HASHTABLE_SET_NODATA(tracemalloc_tracebacks, copy) < 0) {
            raw_free(copy);
#ifdef TRACE_DEBUG
            tracemalloc_error("failed to intern the traceback: putdata failed");
#endif
            return NULL;
        }
        traceback = copy;
    }
    return traceback;
}


static int
tracemalloc_use_domain_cb(_Py_hashtable_t *old_traces,
                           _Py_hashtable_entry_t *entry, void *user_data)
{
    uintptr_t ptr;
    pointer_t key;
    _Py_hashtable_t *new_traces = (_Py_hashtable_t *)user_data;
    const void *pdata = _Py_HASHTABLE_ENTRY_PDATA(old_traces, entry);

    _Py_HASHTABLE_ENTRY_READ_KEY(old_traces, entry, ptr);
    key.ptr = ptr;
    key.domain = DEFAULT_DOMAIN;

    return _Py_hashtable_set(new_traces,
                             sizeof(key), &key,
                             old_traces->data_size, pdata);
}


/* Convert tracemalloc_traces from compact key (uintptr_t) to pointer_t key.
 * Return 0 on success, -1 on error. */
static int
tracemalloc_use_domain(void)
{
    _Py_hashtable_t *new_traces = NULL;

    assert(!tracemalloc_config.use_domain);

    new_traces = hashtable_new(sizeof(pointer_t),
                               sizeof(trace_t),
                               hashtable_hash_pointer_t,
                               hashtable_compare_pointer_t);
    if (new_traces == NULL) {
        return -1;
    }

    if (_Py_hashtable_foreach(tracemalloc_traces, tracemalloc_use_domain_cb,
                              new_traces) < 0)
    {
        _Py_hashtable_destroy(new_traces);
        return -1;
    }

    _Py_hashtable_destroy(tracemalloc_traces);
    tracemalloc_traces = new_traces;

    tracemalloc_config.use_domain = 1;

    return 0;
}


static void
tracemalloc_remove_trace(_PyTraceMalloc_domain_t domain, uintptr_t ptr)
{
    trace_t trace;
    int removed;

    assert(tracemalloc_config.tracing);

    if (tracemalloc_config.use_domain) {
        pointer_t key = {ptr, domain};
        removed = _Py_HASHTABLE_POP(tracemalloc_traces, key, trace);
    }
    else {
        removed = _Py_HASHTABLE_POP(tracemalloc_traces, ptr, trace);
    }
    if (!removed) {
        return;
    }

    assert(tracemalloc_traced_memory >= trace.size);
    tracemalloc_traced_memory -= trace.size;
}

#define REMOVE_TRACE(ptr) \
            tracemalloc_remove_trace(DEFAULT_DOMAIN, (uintptr_t)(ptr))


static int
tracemalloc_add_trace(_PyTraceMalloc_domain_t domain, uintptr_t ptr,
                      size_t size)
{
    pointer_t key = {ptr, domain};
    traceback_t *traceback;
    trace_t trace;
    _Py_hashtable_entry_t* entry;
    int res;

    assert(tracemalloc_config.tracing);

    traceback = traceback_new();
    if (traceback == NULL) {
        return -1;
    }

    if (!tracemalloc_config.use_domain && domain != DEFAULT_DOMAIN) {
        /* first trace using a non-zero domain whereas traces use compact
           (uintptr_t) keys: switch to pointer_t keys. */
        if (tracemalloc_use_domain() < 0) {
            return -1;
        }
    }

    if (tracemalloc_config.use_domain) {
        entry = _Py_HASHTABLE_GET_ENTRY(tracemalloc_traces, key);
    }
    else {
        entry = _Py_HASHTABLE_GET_ENTRY(tracemalloc_traces, ptr);
    }

    if (entry != NULL) {
        /* the memory block is already tracked */
        _Py_HASHTABLE_ENTRY_READ_DATA(tracemalloc_traces, entry, trace);
        assert(tracemalloc_traced_memory >= trace.size);
        tracemalloc_traced_memory -= trace.size;

        trace.size = size;
        trace.traceback = traceback;
        _Py_HASHTABLE_ENTRY_WRITE_DATA(tracemalloc_traces, entry, trace);
    }
    else {
        trace.size = size;
        trace.traceback = traceback;

        if (tracemalloc_config.use_domain) {
            res = _Py_HASHTABLE_SET(tracemalloc_traces, key, trace);
        }
        else {
            res = _Py_HASHTABLE_SET(tracemalloc_traces, ptr, trace);
        }
        if (res != 0) {
            return res;
        }
    }

    assert(tracemalloc_traced_memory <= SIZE_MAX - size);
    tracemalloc_traced_memory += size;
    if (tracemalloc_traced_memory > tracemalloc_peak_traced_memory)
        tracemalloc_peak_traced_memory = tracemalloc_traced_memory;
    return 0;
}

#define ADD_TRACE(ptr, size) \
            tracemalloc_add_trace(DEFAULT_DOMAIN, (uintptr_t)(ptr), size)


static void*
tracemalloc_alloc(int use_calloc, void *ctx, size_t nelem, size_t elsize)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
    void *ptr;

    assert(elsize == 0 || nelem <= SIZE_MAX / elsize);

    if (use_calloc)
        ptr = alloc->calloc(alloc->ctx, nelem, elsize);
    else
        ptr = alloc->malloc(alloc->ctx, nelem * elsize);
    if (ptr == NULL)
        return NULL;

    TABLES_LOCK();
    if (ADD_TRACE(ptr, nelem * elsize) < 0) {
        /* Failed to allocate a trace for the new memory block */
        TABLES_UNLOCK();
        alloc->free(alloc->ctx, ptr);
        return NULL;
    }
    TABLES_UNLOCK();
    return ptr;
}


static void*
tracemalloc_realloc(void *ctx, void *ptr, size_t new_size)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
    void *ptr2;

    ptr2 = alloc->realloc(alloc->ctx, ptr, new_size);
    if (ptr2 == NULL)
        return NULL;

    if (ptr != NULL) {
        /* an existing memory block has been resized */

        TABLES_LOCK();

        /* tracemalloc_add_trace() updates the trace if there is already
           a trace at address (domain, ptr2) */
        if (ptr2 != ptr) {
            REMOVE_TRACE(ptr);
        }

        if (ADD_TRACE(ptr2, new_size) < 0) {
            /* Memory allocation failed. The error cannot be reported to
               the caller, because realloc() may already have shrunk the
               memory block and so removed bytes.

               This case is very unlikely: a hash entry has just been
               released, so the hash table should have at least one free entry.

               The GIL and the table lock ensures that only one thread is
               allocating memory. */
            assert(0 && "should never happen");
        }
        TABLES_UNLOCK();
    }
    else {
        /* new allocation */

        TABLES_LOCK();
        if (ADD_TRACE(ptr2, new_size) < 0) {
            /* Failed to allocate a trace for the new memory block */
            TABLES_UNLOCK();
            alloc->free(alloc->ctx, ptr2);
            return NULL;
        }
        TABLES_UNLOCK();
    }
    return ptr2;
}


static void
tracemalloc_free(void *ctx, void *ptr)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;

    if (ptr == NULL)
        return;

     /* GIL cannot be locked in PyMem_RawFree() because it would introduce
        a deadlock in PyThreadState_DeleteCurrent(). */

    alloc->free(alloc->ctx, ptr);

    TABLES_LOCK();
    REMOVE_TRACE(ptr);
    TABLES_UNLOCK();
}


static void*
tracemalloc_alloc_gil(int use_calloc, void *ctx, size_t nelem, size_t elsize)
{
    void *ptr;

    if (get_reentrant()) {
        PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
        if (use_calloc)
            return alloc->calloc(alloc->ctx, nelem, elsize);
        else
            return alloc->malloc(alloc->ctx, nelem * elsize);
    }

    /* Ignore reentrant call. PyObjet_Malloc() calls PyMem_Malloc() for
       allocations larger than 512 bytes, don't trace the same memory
       allocation twice. */
    set_reentrant(1);

    ptr = tracemalloc_alloc(use_calloc, ctx, nelem, elsize);

    set_reentrant(0);
    return ptr;
}


static void*
tracemalloc_malloc_gil(void *ctx, size_t size)
{
    return tracemalloc_alloc_gil(0, ctx, 1, size);
}


static void*
tracemalloc_calloc_gil(void *ctx, size_t nelem, size_t elsize)
{
    return tracemalloc_alloc_gil(1, ctx, nelem, elsize);
}


static void*
tracemalloc_realloc_gil(void *ctx, void *ptr, size_t new_size)
{
    void *ptr2;

    if (get_reentrant()) {
        /* Reentrant call to PyMem_Realloc() and PyMem_RawRealloc().
           Example: PyMem_RawRealloc() is called internally by pymalloc
           (_PyObject_Malloc() and  _PyObject_Realloc()) to allocate a new
           arena (new_arena()). */
        PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;

        ptr2 = alloc->realloc(alloc->ctx, ptr, new_size);
        if (ptr2 != NULL && ptr != NULL) {
            TABLES_LOCK();
            REMOVE_TRACE(ptr);
            TABLES_UNLOCK();
        }
        return ptr2;
    }

    /* Ignore reentrant call. PyObjet_Realloc() calls PyMem_Realloc() for
       allocations larger than 512 bytes. Don't trace the same memory
       allocation twice. */
    set_reentrant(1);

    ptr2 = tracemalloc_realloc(ctx, ptr, new_size);

    set_reentrant(0);
    return ptr2;
}


#ifdef TRACE_RAW_MALLOC
static void*
tracemalloc_raw_alloc(int use_calloc, void *ctx, size_t nelem, size_t elsize)
{
#ifdef WITH_THREAD
    PyGILState_STATE gil_state;
#endif
    void *ptr;

    if (get_reentrant()) {
        PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
        if (use_calloc)
            return alloc->calloc(alloc->ctx, nelem, elsize);
        else
            return alloc->malloc(alloc->ctx, nelem * elsize);
    }

    /* Ignore reentrant call. PyGILState_Ensure() may call PyMem_RawMalloc()
       indirectly which would call PyGILState_Ensure() if reentrant are not
       disabled. */
    set_reentrant(1);

#ifdef WITH_THREAD
    gil_state = PyGILState_Ensure();
    ptr = tracemalloc_alloc(use_calloc, ctx, nelem, elsize);
    PyGILState_Release(gil_state);
#else
    ptr = tracemalloc_alloc(use_calloc, ctx, nelem, elsize);
#endif

    set_reentrant(0);
    return ptr;
}


static void*
tracemalloc_raw_malloc(void *ctx, size_t size)
{
    return tracemalloc_raw_alloc(0, ctx, 1, size);
}


static void*
tracemalloc_raw_calloc(void *ctx, size_t nelem, size_t elsize)
{
    return tracemalloc_raw_alloc(1, ctx, nelem, elsize);
}


static void*
tracemalloc_raw_realloc(void *ctx, void *ptr, size_t new_size)
{
#ifdef WITH_THREAD
    PyGILState_STATE gil_state;
#endif
    void *ptr2;

    if (get_reentrant()) {
        /* Reentrant call to PyMem_RawRealloc(). */
        PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;

        ptr2 = alloc->realloc(alloc->ctx, ptr, new_size);

        if (ptr2 != NULL && ptr != NULL) {
            TABLES_LOCK();
            REMOVE_TRACE(ptr);
            TABLES_UNLOCK();
        }
        return ptr2;
    }

    /* Ignore reentrant call. PyGILState_Ensure() may call PyMem_RawMalloc()
       indirectly which would call PyGILState_Ensure() if reentrant calls are
       not disabled. */
    set_reentrant(1);

#ifdef WITH_THREAD
    gil_state = PyGILState_Ensure();
    ptr2 = tracemalloc_realloc(ctx, ptr, new_size);
    PyGILState_Release(gil_state);
#else
    ptr2 = tracemalloc_realloc(ctx, ptr, new_size);
#endif

    set_reentrant(0);
    return ptr2;
}
#endif   /* TRACE_RAW_MALLOC */


static int
tracemalloc_clear_filename(_Py_hashtable_t *ht, _Py_hashtable_entry_t *entry,
                           void *user_data)
{
    PyObject *filename;

    _Py_HASHTABLE_ENTRY_READ_KEY(ht, entry, filename);
    Py_DECREF(filename);
    return 0;
}


static int
traceback_free_traceback(_Py_hashtable_t *ht, _Py_hashtable_entry_t *entry,
                         void *user_data)
{
    traceback_t *traceback;

    _Py_HASHTABLE_ENTRY_READ_KEY(ht, entry, traceback);
    raw_free(traceback);
    return 0;
}


/* reentrant flag must be set to call this function and GIL must be held */
static void
tracemalloc_clear_traces(void)
{
#ifdef WITH_THREAD
    /* The GIL protects variables againt concurrent access */
    assert(PyGILState_Check());
#endif

    TABLES_LOCK();
    _Py_hashtable_clear(tracemalloc_traces);
    tracemalloc_traced_memory = 0;
    tracemalloc_peak_traced_memory = 0;
    TABLES_UNLOCK();

    _Py_hashtable_foreach(tracemalloc_tracebacks, traceback_free_traceback, NULL);
    _Py_hashtable_clear(tracemalloc_tracebacks);

    _Py_hashtable_foreach(tracemalloc_filenames, tracemalloc_clear_filename, NULL);
    _Py_hashtable_clear(tracemalloc_filenames);
}


static int
tracemalloc_init(void)
{
    if (tracemalloc_config.initialized == TRACEMALLOC_FINALIZED) {
        PyErr_SetString(PyExc_RuntimeError,
                        "the tracemalloc module has been unloaded");
        return -1;
    }

    if (tracemalloc_config.initialized == TRACEMALLOC_INITIALIZED)
        return 0;

    PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &allocators.raw);

#ifdef REENTRANT_THREADLOCAL
    tracemalloc_reentrant_key = PyThread_create_key();
    if (tracemalloc_reentrant_key == -1) {
#ifdef MS_WINDOWS
        PyErr_SetFromWindowsErr(0);
#else
        PyErr_SetFromErrno(PyExc_OSError);
#endif
        return -1;
    }
#endif

#if defined(WITH_THREAD) && defined(TRACE_RAW_MALLOC)
    if (tables_lock == NULL) {
        tables_lock = PyThread_allocate_lock();
        if (tables_lock == NULL) {
            PyErr_SetString(PyExc_RuntimeError, "cannot allocate lock");
            return -1;
        }
    }
#endif

    tracemalloc_filenames = hashtable_new(sizeof(PyObject *), 0,
                                          hashtable_hash_pyobject,
                                          hashtable_compare_unicode);

    tracemalloc_tracebacks = hashtable_new(sizeof(traceback_t *), 0,
                                           hashtable_hash_traceback,
                                           hashtable_compare_traceback);

    if (tracemalloc_config.use_domain) {
        tracemalloc_traces = hashtable_new(sizeof(pointer_t),
                                           sizeof(trace_t),
                                           hashtable_hash_pointer_t,
                                           hashtable_compare_pointer_t);
    }
    else {
        tracemalloc_traces = hashtable_new(sizeof(uintptr_t),
                                           sizeof(trace_t),
                                           _Py_hashtable_hash_ptr,
                                           _Py_hashtable_compare_direct);
    }

    if (tracemalloc_filenames == NULL || tracemalloc_tracebacks == NULL
       || tracemalloc_traces == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    unknown_filename = PyUnicode_FromString("<unknown>");
    if (unknown_filename == NULL)
        return -1;
    PyUnicode_InternInPlace(&unknown_filename);

    tracemalloc_empty_traceback.nframe = 1;
    /* borrowed reference */
    tracemalloc_empty_traceback.frames[0].filename = unknown_filename;
    tracemalloc_empty_traceback.frames[0].lineno = 0;
    tracemalloc_empty_traceback.hash = traceback_hash(&tracemalloc_empty_traceback);

    tracemalloc_config.initialized = TRACEMALLOC_INITIALIZED;
    return 0;
}


static void
tracemalloc_deinit(void)
{
    if (tracemalloc_config.initialized != TRACEMALLOC_INITIALIZED)
        return;
    tracemalloc_config.initialized = TRACEMALLOC_FINALIZED;

    tracemalloc_stop();

    /* destroy hash tables */
    _Py_hashtable_destroy(tracemalloc_tracebacks);
    _Py_hashtable_destroy(tracemalloc_filenames);
    _Py_hashtable_destroy(tracemalloc_traces);

#if defined(WITH_THREAD) && defined(TRACE_RAW_MALLOC)
    if (tables_lock != NULL) {
        PyThread_free_lock(tables_lock);
        tables_lock = NULL;
    }
#endif

#ifdef REENTRANT_THREADLOCAL
    PyThread_delete_key(tracemalloc_reentrant_key);
    tracemalloc_reentrant_key = -1;
#endif

    Py_XDECREF(unknown_filename);
}


static int
tracemalloc_start(int max_nframe)
{
    PyMemAllocatorEx alloc;
    size_t size;

    if (tracemalloc_init() < 0)
        return -1;

    if (tracemalloc_config.tracing) {
        /* hook already installed: do nothing */
        return 0;
    }

    assert(1 <= max_nframe && max_nframe <= MAX_NFRAME);
    tracemalloc_config.max_nframe = max_nframe;

    /* allocate a buffer to store a new traceback */
    size = TRACEBACK_SIZE(max_nframe);
    assert(tracemalloc_traceback == NULL);
    tracemalloc_traceback = raw_malloc(size);
    if (tracemalloc_traceback == NULL) {
        PyErr_NoMemory();
        return -1;
    }

#ifdef TRACE_RAW_MALLOC
    alloc.malloc = tracemalloc_raw_malloc;
    alloc.calloc = tracemalloc_raw_calloc;
    alloc.realloc = tracemalloc_raw_realloc;
    alloc.free = tracemalloc_free;

    alloc.ctx = &allocators.raw;
    PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &allocators.raw);
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &alloc);
#endif

    alloc.malloc = tracemalloc_malloc_gil;
    alloc.calloc = tracemalloc_calloc_gil;
    alloc.realloc = tracemalloc_realloc_gil;
    alloc.free = tracemalloc_free;

    alloc.ctx = &allocators.mem;
    PyMem_GetAllocator(PYMEM_DOMAIN_MEM, &allocators.mem);
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &alloc);

    alloc.ctx = &allocators.obj;
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &allocators.obj);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &alloc);

    /* everything is ready: start tracing Python memory allocations */
    tracemalloc_config.tracing = 1;

    return 0;
}


static void
tracemalloc_stop(void)
{
    if (!tracemalloc_config.tracing)
        return;

    /* stop tracing Python memory allocations */
    tracemalloc_config.tracing = 0;

    /* unregister the hook on memory allocators */
#ifdef TRACE_RAW_MALLOC
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &allocators.raw);
#endif
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &allocators.mem);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocators.obj);

    tracemalloc_clear_traces();

    /* release memory */
    raw_free(tracemalloc_traceback);
    tracemalloc_traceback = NULL;
}

PyDoc_STRVAR(tracemalloc_is_tracing_doc,
    "is_tracing()->bool\n"
    "\n"
    "True if the tracemalloc module is tracing Python memory allocations,\n"
    "False otherwise.");


static PyObject*
py_tracemalloc_is_tracing(PyObject *self)
{
    return PyBool_FromLong(tracemalloc_config.tracing);
}

PyDoc_STRVAR(tracemalloc_clear_traces_doc,
    "clear_traces()\n"
    "\n"
    "Clear traces of memory blocks allocated by Python.");


static PyObject*
py_tracemalloc_clear_traces(PyObject *self)
{
    if (!tracemalloc_config.tracing)
        Py_RETURN_NONE;

    set_reentrant(1);
    tracemalloc_clear_traces();
    set_reentrant(0);

    Py_RETURN_NONE;
}


static PyObject*
frame_to_pyobject(frame_t *frame)
{
    PyObject *frame_obj, *lineno_obj;

    frame_obj = PyTuple_New(2);
    if (frame_obj == NULL)
        return NULL;

    Py_INCREF(frame->filename);
    PyTuple_SET_ITEM(frame_obj, 0, frame->filename);

    lineno_obj = PyLong_FromUnsignedLong(frame->lineno);
    if (lineno_obj == NULL) {
        Py_DECREF(frame_obj);
        return NULL;
    }
    PyTuple_SET_ITEM(frame_obj, 1, lineno_obj);

    return frame_obj;
}


static PyObject*
traceback_to_pyobject(traceback_t *traceback, _Py_hashtable_t *intern_table)
{
    int i;
    PyObject *frames, *frame;

    if (intern_table != NULL) {
        if (_Py_HASHTABLE_GET(intern_table, traceback, frames)) {
            Py_INCREF(frames);
            return frames;
        }
    }

    frames = PyTuple_New(traceback->nframe);
    if (frames == NULL)
        return NULL;

    for (i=0; i < traceback->nframe; i++) {
        frame = frame_to_pyobject(&traceback->frames[i]);
        if (frame == NULL) {
            Py_DECREF(frames);
            return NULL;
        }
        PyTuple_SET_ITEM(frames, i, frame);
    }

    if (intern_table != NULL) {
        if (_Py_HASHTABLE_SET(intern_table, traceback, frames) < 0) {
            Py_DECREF(frames);
            PyErr_NoMemory();
            return NULL;
        }
        /* intern_table keeps a new reference to frames */
        Py_INCREF(frames);
    }
    return frames;
}


static PyObject*
trace_to_pyobject(_PyTraceMalloc_domain_t domain, trace_t *trace,
                  _Py_hashtable_t *intern_tracebacks)
{
    PyObject *trace_obj = NULL;
    PyObject *obj;

    trace_obj = PyTuple_New(3);
    if (trace_obj == NULL)
        return NULL;

    obj = PyLong_FromSize_t(domain);
    if (obj == NULL) {
        Py_DECREF(trace_obj);
        return NULL;
    }
    PyTuple_SET_ITEM(trace_obj, 0, obj);

    obj = PyLong_FromSize_t(trace->size);
    if (obj == NULL) {
        Py_DECREF(trace_obj);
        return NULL;
    }
    PyTuple_SET_ITEM(trace_obj, 1, obj);

    obj = traceback_to_pyobject(trace->traceback, intern_tracebacks);
    if (obj == NULL) {
        Py_DECREF(trace_obj);
        return NULL;
    }
    PyTuple_SET_ITEM(trace_obj, 2, obj);

    return trace_obj;
}


typedef struct {
    _Py_hashtable_t *traces;
    _Py_hashtable_t *tracebacks;
    PyObject *list;
} get_traces_t;

static int
tracemalloc_get_traces_fill(_Py_hashtable_t *traces, _Py_hashtable_entry_t *entry,
                            void *user_data)
{
    get_traces_t *get_traces = user_data;
    _PyTraceMalloc_domain_t domain;
    trace_t trace;
    PyObject *tracemalloc_obj;
    int res;

    if (tracemalloc_config.use_domain) {
        pointer_t key;
        _Py_HASHTABLE_ENTRY_READ_KEY(traces, entry, key);
        domain = key.domain;
    }
    else {
        domain = DEFAULT_DOMAIN;
    }
    _Py_HASHTABLE_ENTRY_READ_DATA(traces, entry, trace);

    tracemalloc_obj = trace_to_pyobject(domain, &trace, get_traces->tracebacks);
    if (tracemalloc_obj == NULL)
        return 1;

    res = PyList_Append(get_traces->list, tracemalloc_obj);
    Py_DECREF(tracemalloc_obj);
    if (res < 0)
        return 1;

    return 0;
}


static int
tracemalloc_pyobject_decref_cb(_Py_hashtable_t *tracebacks,
                               _Py_hashtable_entry_t *entry,
                               void *user_data)
{
    PyObject *obj;
    _Py_HASHTABLE_ENTRY_READ_DATA(tracebacks, entry, obj);
    Py_DECREF(obj);
    return 0;
}


PyDoc_STRVAR(tracemalloc_get_traces_doc,
    "_get_traces() -> list\n"
    "\n"
    "Get traces of all memory blocks allocated by Python.\n"
    "Return a list of (size: int, traceback: tuple) tuples.\n"
    "traceback is a tuple of (filename: str, lineno: int) tuples.\n"
    "\n"
    "Return an empty list if the tracemalloc module is disabled.");

static PyObject*
py_tracemalloc_get_traces(PyObject *self, PyObject *obj)
{
    get_traces_t get_traces;
    int err;

    get_traces.traces = NULL;
    get_traces.tracebacks = NULL;
    get_traces.list = PyList_New(0);
    if (get_traces.list == NULL)
        goto error;

    if (!tracemalloc_config.tracing)
        return get_traces.list;

    /* the traceback hash table is used temporarily to intern traceback tuple
       of (filename, lineno) tuples */
    get_traces.tracebacks = hashtable_new(sizeof(traceback_t *),
                                          sizeof(PyObject *),
                                          _Py_hashtable_hash_ptr,
                                          _Py_hashtable_compare_direct);
    if (get_traces.tracebacks == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    TABLES_LOCK();
    get_traces.traces = _Py_hashtable_copy(tracemalloc_traces);
    TABLES_UNLOCK();

    if (get_traces.traces == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    set_reentrant(1);
    err = _Py_hashtable_foreach(get_traces.traces,
                                tracemalloc_get_traces_fill, &get_traces);
    set_reentrant(0);
    if (err)
        goto error;

    goto finally;

error:
    Py_CLEAR(get_traces.list);

finally:
    if (get_traces.tracebacks != NULL) {
        _Py_hashtable_foreach(get_traces.tracebacks,
                              tracemalloc_pyobject_decref_cb, NULL);
        _Py_hashtable_destroy(get_traces.tracebacks);
    }
    if (get_traces.traces != NULL) {
        _Py_hashtable_destroy(get_traces.traces);
    }

    return get_traces.list;
}


static traceback_t*
tracemalloc_get_traceback(_PyTraceMalloc_domain_t domain, uintptr_t ptr)
{
    trace_t trace;
    int found;

    if (!tracemalloc_config.tracing)
        return NULL;

    TABLES_LOCK();
    if (tracemalloc_config.use_domain) {
        pointer_t key = {ptr, domain};
        found = _Py_HASHTABLE_GET(tracemalloc_traces, key, trace);
    }
    else {
        found = _Py_HASHTABLE_GET(tracemalloc_traces, ptr, trace);
    }
    TABLES_UNLOCK();

    if (!found)
        return NULL;

    return trace.traceback;
}


PyDoc_STRVAR(tracemalloc_get_object_traceback_doc,
    "_get_object_traceback(obj)\n"
    "\n"
    "Get the traceback where the Python object obj was allocated.\n"
    "Return a tuple of (filename: str, lineno: int) tuples.\n"
    "\n"
    "Return None if the tracemalloc module is disabled or did not\n"
    "trace the allocation of the object.");

static PyObject*
py_tracemalloc_get_object_traceback(PyObject *self, PyObject *obj)
{
    PyTypeObject *type;
    void *ptr;
    traceback_t *traceback;

    type = Py_TYPE(obj);
    if (PyType_IS_GC(type))
        ptr = (void *)((char *)obj - sizeof(PyGC_Head));
    else
        ptr = (void *)obj;

    traceback = tracemalloc_get_traceback(DEFAULT_DOMAIN, (uintptr_t)ptr);
    if (traceback == NULL)
        Py_RETURN_NONE;

    return traceback_to_pyobject(traceback, NULL);
}


#define PUTS(fd, str) _Py_write_noraise(fd, str, (int)strlen(str))

static void
_PyMem_DumpFrame(int fd, frame_t * frame)
{
    PUTS(fd, "  File \"");
    _Py_DumpASCII(fd, frame->filename);
    PUTS(fd, "\", line ");
    _Py_DumpDecimal(fd, frame->lineno);
    PUTS(fd, "\n");
}

/* Dump the traceback where a memory block was allocated into file descriptor
   fd. The function may block on TABLES_LOCK() but it is unlikely. */
void
_PyMem_DumpTraceback(int fd, const void *ptr)
{
    traceback_t *traceback;
    int i;

    traceback = tracemalloc_get_traceback(DEFAULT_DOMAIN, (uintptr_t)ptr);
    if (traceback == NULL)
        return;

    PUTS(fd, "Memory block allocated at (most recent call first):\n");
    for (i=0; i < traceback->nframe; i++) {
        _PyMem_DumpFrame(fd, &traceback->frames[i]);
    }
    PUTS(fd, "\n");
}

#undef PUTS


PyDoc_STRVAR(tracemalloc_start_doc,
    "start(nframe: int=1)\n"
    "\n"
    "Start tracing Python memory allocations. Set also the maximum number \n"
    "of frames stored in the traceback of a trace to nframe.");

static PyObject*
py_tracemalloc_start(PyObject *self, PyObject *args)
{
    Py_ssize_t nframe = 1;
    int nframe_int;

    if (!PyArg_ParseTuple(args, "|n:start", &nframe))
        return NULL;

    if (nframe < 1 || nframe > MAX_NFRAME) {
        PyErr_Format(PyExc_ValueError,
                     "the number of frames must be in range [1; %i]",
                     MAX_NFRAME);
        return NULL;
    }
    nframe_int = Py_SAFE_DOWNCAST(nframe, Py_ssize_t, int);

    if (tracemalloc_start(nframe_int) < 0)
        return NULL;

    Py_RETURN_NONE;
}

PyDoc_STRVAR(tracemalloc_stop_doc,
    "stop()\n"
    "\n"
    "Stop tracing Python memory allocations and clear traces\n"
    "of memory blocks allocated by Python.");


static PyObject*
py_tracemalloc_stop(PyObject *self)
{
    tracemalloc_stop();
    Py_RETURN_NONE;
}


PyDoc_STRVAR(tracemalloc_get_traceback_limit_doc,
    "get_traceback_limit() -> int\n"
    "\n"
    "Get the maximum number of frames stored in the traceback\n"
    "of a trace.\n"
    "\n"
    "By default, a trace of an allocated memory block only stores\n"
    "the most recent frame: the limit is 1.");

static PyObject*
py_tracemalloc_get_traceback_limit(PyObject *self)
{
    return PyLong_FromLong(tracemalloc_config.max_nframe);
}


PyDoc_STRVAR(tracemalloc_get_tracemalloc_memory_doc,
    "get_tracemalloc_memory() -> int\n"
    "\n"
    "Get the memory usage in bytes of the tracemalloc module\n"
    "used internally to trace memory allocations.");

static PyObject*
tracemalloc_get_tracemalloc_memory(PyObject *self)
{
    size_t size;
    PyObject *size_obj;

    size = _Py_hashtable_size(tracemalloc_tracebacks);
    size += _Py_hashtable_size(tracemalloc_filenames);

    TABLES_LOCK();
    size += _Py_hashtable_size(tracemalloc_traces);
    TABLES_UNLOCK();

    size_obj = PyLong_FromSize_t(size);
    return Py_BuildValue("N", size_obj);
}


PyDoc_STRVAR(tracemalloc_get_traced_memory_doc,
    "get_traced_memory() -> (int, int)\n"
    "\n"
    "Get the current size and peak size of memory blocks traced\n"
    "by the tracemalloc module as a tuple: (current: int, peak: int).");

static PyObject*
tracemalloc_get_traced_memory(PyObject *self)
{
    Py_ssize_t size, peak_size;
    PyObject *size_obj, *peak_size_obj;

    if (!tracemalloc_config.tracing)
        return Py_BuildValue("ii", 0, 0);

    TABLES_LOCK();
    size = tracemalloc_traced_memory;
    peak_size = tracemalloc_peak_traced_memory;
    TABLES_UNLOCK();

    size_obj = PyLong_FromSize_t(size);
    peak_size_obj = PyLong_FromSize_t(peak_size);
    return Py_BuildValue("NN", size_obj, peak_size_obj);
}


static PyMethodDef module_methods[] = {
    {"is_tracing", (PyCFunction)py_tracemalloc_is_tracing,
     METH_NOARGS, tracemalloc_is_tracing_doc},
    {"clear_traces", (PyCFunction)py_tracemalloc_clear_traces,
     METH_NOARGS, tracemalloc_clear_traces_doc},
    {"_get_traces", (PyCFunction)py_tracemalloc_get_traces,
     METH_NOARGS, tracemalloc_get_traces_doc},
    {"_get_object_traceback", (PyCFunction)py_tracemalloc_get_object_traceback,
     METH_O, tracemalloc_get_object_traceback_doc},
    {"start", (PyCFunction)py_tracemalloc_start,
      METH_VARARGS, tracemalloc_start_doc},
    {"stop", (PyCFunction)py_tracemalloc_stop,
      METH_NOARGS, tracemalloc_stop_doc},
    {"get_traceback_limit", (PyCFunction)py_tracemalloc_get_traceback_limit,
     METH_NOARGS, tracemalloc_get_traceback_limit_doc},
    {"get_tracemalloc_memory", (PyCFunction)tracemalloc_get_tracemalloc_memory,
     METH_NOARGS, tracemalloc_get_tracemalloc_memory_doc},
    {"get_traced_memory", (PyCFunction)tracemalloc_get_traced_memory,
     METH_NOARGS, tracemalloc_get_traced_memory_doc},

    /* sentinel */
    {NULL, NULL}
};

PyDoc_STRVAR(module_doc,
"Debug module to trace memory blocks allocated by Python.");

static struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "_tracemalloc",
    module_doc,
    0, /* non-negative size to be able to unload the module */
    module_methods,
    NULL,
};

PyMODINIT_FUNC
PyInit__tracemalloc(void)
{
    PyObject *m;
    m = PyModule_Create(&module_def);
    if (m == NULL)
        return NULL;

    if (tracemalloc_init() < 0)
        return NULL;

    return m;
}


static int
parse_sys_xoptions(PyObject *value)
{
    PyObject *valuelong;
    long nframe;

    if (value == Py_True)
        return 1;

    assert(PyUnicode_Check(value));
    if (PyUnicode_GetLength(value) == 0)
        return -1;

    valuelong = PyLong_FromUnicodeObject(value, 10);
    if (valuelong == NULL)
        return -1;

    nframe = PyLong_AsLong(valuelong);
    Py_DECREF(valuelong);
    if (nframe == -1 && PyErr_Occurred())
        return -1;

    if (nframe < 1 || nframe > MAX_NFRAME)
        return -1;

    return Py_SAFE_DOWNCAST(nframe, long, int);
}


int
_PyTraceMalloc_Init(void)
{
    char *p;
    int nframe;

#ifdef WITH_THREAD
    assert(PyGILState_Check());
#endif

    if ((p = Py_GETENV("PYTHONTRACEMALLOC")) && *p != '\0') {
        char *endptr = p;
        long value;

        errno = 0;
        value = strtol(p, &endptr, 10);
        if (*endptr != '\0'
            || value < 1
            || value > MAX_NFRAME
            || errno == ERANGE)
        {
            Py_FatalError("PYTHONTRACEMALLOC: invalid number of frames");
            return -1;
        }

        nframe = (int)value;
    }
    else {
        PyObject *xoptions, *key, *value;

        xoptions = PySys_GetXOptions();
        if (xoptions == NULL)
            return -1;

        key = PyUnicode_FromString("tracemalloc");
        if (key == NULL)
            return -1;

        value = PyDict_GetItemWithError(xoptions, key); /* borrowed */
        Py_DECREF(key);
        if (value == NULL) {
            if (PyErr_Occurred())
                return -1;

            /* -X tracemalloc is not used */
            return 0;
        }

        nframe = parse_sys_xoptions(value);
        if (nframe < 0) {
            Py_FatalError("-X tracemalloc=NFRAME: invalid number of frames");
        }
    }

    return tracemalloc_start(nframe);
}


void
_PyTraceMalloc_Fini(void)
{
#ifdef WITH_THREAD
    assert(PyGILState_Check());
#endif
    tracemalloc_deinit();
}

int
_PyTraceMalloc_Track(_PyTraceMalloc_domain_t domain, uintptr_t ptr,
                     size_t size)
{
    int res;
#ifdef WITH_THREAD
    PyGILState_STATE gil_state;
#endif

    if (!tracemalloc_config.tracing) {
        /* tracemalloc is not tracing: do nothing */
        return -2;
    }

#ifdef WITH_THREAD
    gil_state = PyGILState_Ensure();
#endif

    TABLES_LOCK();
    res = tracemalloc_add_trace(domain, ptr, size);
    TABLES_UNLOCK();

#ifdef WITH_THREAD
    PyGILState_Release(gil_state);
#endif
    return res;
}


int
_PyTraceMalloc_Untrack(_PyTraceMalloc_domain_t domain, uintptr_t ptr)
{
    if (!tracemalloc_config.tracing) {
        /* tracemalloc is not tracing: do nothing */
        return -2;
    }

    TABLES_LOCK();
    tracemalloc_remove_trace(domain, ptr);
    TABLES_UNLOCK();

    return 0;
}


PyObject*
_PyTraceMalloc_GetTraceback(_PyTraceMalloc_domain_t domain, uintptr_t ptr)
{
    traceback_t *traceback;

    traceback = tracemalloc_get_traceback(domain, ptr);
    if (traceback == NULL)
        Py_RETURN_NONE;

    return traceback_to_pyobject(traceback, NULL);
}
