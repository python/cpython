#include "Python.h"
#include "pycore_gc.h"            // PyGC_Head
#include "pycore_pymem.h"         // _Py_tracemalloc_config
#include "pycore_traceback.h"
#include "pycore_hashtable.h"
#include "frameobject.h"          // PyFrame_GetBack()

#include "clinic/_tracemalloc.c.h"
/*[clinic input]
module _tracemalloc
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=708a98302fc46e5f]*/

/* Trace memory blocks allocated by PyMem_RawMalloc() */
#define TRACE_RAW_MALLOC

/* Forward declaration */
static void tracemalloc_stop(void);
static void* raw_malloc(size_t size);
static void raw_free(void *ptr);

#ifdef Py_DEBUG
#  define TRACE_DEBUG
#endif

#define TO_PTR(key) ((const void *)(uintptr_t)(key))
#define FROM_PTR(key) ((uintptr_t)(key))

/* Protected by the GIL */
static struct {
    PyMemAllocatorEx mem;
    PyMemAllocatorEx raw;
    PyMemAllocatorEx obj;
} allocators;


#if defined(TRACE_RAW_MALLOC)
/* This lock is needed because tracemalloc_free() is called without
   the GIL held from PyMem_RawFree(). It cannot acquire the lock because it
   would introduce a deadlock in _PyThreadState_DeleteCurrent(). */
static PyThread_type_lock tables_lock;
#  define TABLES_LOCK() PyThread_acquire_lock(tables_lock, 1)
#  define TABLES_UNLOCK() PyThread_release_lock(tables_lock)
#else
   /* variables are protected by the GIL */
#  define TABLES_LOCK()
#  define TABLES_UNLOCK()
#endif


#define DEFAULT_DOMAIN 0

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
    /* Number of frames stored */
    uint16_t nframe;
    /* Total number of frames the traceback had */
    uint16_t total_nframe;
    frame_t frames[1];
} traceback_t;

#define TRACEBACK_SIZE(NFRAME) \
        (sizeof(traceback_t) + sizeof(frame_t) * (NFRAME - 1))

/* The maximum number of frames is either:
 - The maximum number of frames we can store in `traceback_t.nframe`
 - The maximum memory size_t we can allocate */
static const unsigned long MAX_NFRAME = Py_MIN(UINT16_MAX, ((SIZE_MAX - sizeof(traceback_t)) / sizeof(frame_t) + 1));


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

/* pointer (void*) => trace (trace_t*).
   Protected by TABLES_LOCK(). */
static _Py_hashtable_t *tracemalloc_traces = NULL;

/* domain (unsigned int) => traces (_Py_hashtable_t).
   Protected by TABLES_LOCK(). */
static _Py_hashtable_t *tracemalloc_domains = NULL;


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


#if defined(TRACE_RAW_MALLOC)
#define REENTRANT_THREADLOCAL

static Py_tss_t tracemalloc_reentrant_key = Py_tss_NEEDS_INIT;

/* Any non-NULL pointer can be used */
#define REENTRANT Py_True

static int
get_reentrant(void)
{
    void *ptr;

    assert(PyThread_tss_is_created(&tracemalloc_reentrant_key));
    ptr = PyThread_tss_get(&tracemalloc_reentrant_key);
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
    assert(PyThread_tss_is_created(&tracemalloc_reentrant_key));

    if (reentrant) {
        assert(!get_reentrant());
        PyThread_tss_set(&tracemalloc_reentrant_key, REENTRANT);
    }
    else {
        assert(get_reentrant());
        PyThread_tss_set(&tracemalloc_reentrant_key, NULL);
    }
}

#else

/* TRACE_RAW_MALLOC not defined: variable protected by the GIL */
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
hashtable_hash_pyobject(const void *key)
{
    PyObject *obj = (PyObject *)key;
    return PyObject_Hash(obj);
}


static int
hashtable_compare_unicode(const void *key1, const void *key2)
{
    PyObject *obj1 = (PyObject *)key1;
    PyObject *obj2 = (PyObject *)key2;
    if (obj1 != NULL && obj2 != NULL) {
        return (PyUnicode_Compare(obj1, obj2) == 0);
    }
    else {
        return obj1 == obj2;
    }
}


static Py_uhash_t
hashtable_hash_uint(const void *key_raw)
{
    unsigned int key = (unsigned int)FROM_PTR(key_raw);
    return (Py_uhash_t)key;
}


static _Py_hashtable_t *
hashtable_new(_Py_hashtable_hash_func hash_func,
              _Py_hashtable_compare_func compare_func,
              _Py_hashtable_destroy_func key_destroy_func,
              _Py_hashtable_destroy_func value_destroy_func)
{
    _Py_hashtable_allocator_t hashtable_alloc = {malloc, free};
    return _Py_hashtable_new_full(hash_func, compare_func,
                                  key_destroy_func, value_destroy_func,
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
hashtable_hash_traceback(const void *key)
{
    const traceback_t *traceback = (const traceback_t *)key;
    return traceback->hash;
}


static int
hashtable_compare_traceback(const void *key1, const void *key2)
{
    const traceback_t *traceback1 = (const traceback_t *)key1;
    const traceback_t *traceback2 = (const traceback_t *)key2;

    if (traceback1->nframe != traceback2->nframe) {
        return 0;
    }
    if (traceback1->total_nframe != traceback2->total_nframe) {
        return 0;
    }

    for (int i=0; i < traceback1->nframe; i++) {
        const frame_t *frame1 = &traceback1->frames[i];
        const frame_t *frame2 = &traceback2->frames[i];

        if (frame1->lineno != frame2->lineno) {
            return 0;
        }
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
    frame->filename = unknown_filename;
    int lineno = PyFrame_GetLineNumber(pyframe);
    if (lineno < 0) {
        lineno = 0;
    }
    frame->lineno = (unsigned int)lineno;

    PyCodeObject *code = PyFrame_GetCode(pyframe);
    PyObject *filename = code->co_filename;
    Py_DECREF(code);

    if (filename == NULL) {
#ifdef TRACE_DEBUG
        tracemalloc_error("failed to get the filename of the code object");
#endif
        return;
    }

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
    _Py_hashtable_entry_t *entry;
    entry = _Py_hashtable_get_entry(tracemalloc_filenames, filename);
    if (entry != NULL) {
        filename = (PyObject *)entry->key;
    }
    else {
        /* tracemalloc_filenames is responsible to keep a reference
           to the filename */
        Py_INCREF(filename);
        if (_Py_hashtable_set(tracemalloc_filenames, filename, NULL) < 0) {
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
    x ^= traceback->total_nframe;
    x += 97531UL;
    return x;
}


static void
traceback_get_frames(traceback_t *traceback)
{
    PyThreadState *tstate = PyGILState_GetThisThreadState();
    if (tstate == NULL) {
#ifdef TRACE_DEBUG
        tracemalloc_error("failed to get the current thread state");
#endif
        return;
    }

    PyFrameObject *pyframe = PyThreadState_GetFrame(tstate);
    for (; pyframe != NULL;) {
        if (traceback->nframe < _Py_tracemalloc_config.max_nframe) {
            tracemalloc_get_frame(pyframe, &traceback->frames[traceback->nframe]);
            assert(traceback->frames[traceback->nframe].filename != NULL);
            traceback->nframe++;
        }
        if (traceback->total_nframe < UINT16_MAX) {
            traceback->total_nframe++;
        }

        PyFrameObject *back = PyFrame_GetBack(pyframe);
        Py_DECREF(pyframe);
        pyframe = back;
    }
}


static traceback_t *
traceback_new(void)
{
    traceback_t *traceback;
    _Py_hashtable_entry_t *entry;

    assert(PyGILState_Check());

    /* get frames */
    traceback = tracemalloc_traceback;
    traceback->nframe = 0;
    traceback->total_nframe = 0;
    traceback_get_frames(traceback);
    if (traceback->nframe == 0)
        return &tracemalloc_empty_traceback;
    traceback->hash = traceback_hash(traceback);

    /* intern the traceback */
    entry = _Py_hashtable_get_entry(tracemalloc_tracebacks, traceback);
    if (entry != NULL) {
        traceback = (traceback_t *)entry->key;
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

        if (_Py_hashtable_set(tracemalloc_tracebacks, copy, NULL) < 0) {
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


static _Py_hashtable_t*
tracemalloc_create_traces_table(void)
{
    return hashtable_new(_Py_hashtable_hash_ptr,
                         _Py_hashtable_compare_direct,
                         NULL, raw_free);
}


static _Py_hashtable_t*
tracemalloc_create_domains_table(void)
{
    return hashtable_new(hashtable_hash_uint,
                         _Py_hashtable_compare_direct,
                         NULL,
                         (_Py_hashtable_destroy_func)_Py_hashtable_destroy);
}


static _Py_hashtable_t*
tracemalloc_get_traces_table(unsigned int domain)
{
    if (domain == DEFAULT_DOMAIN) {
        return tracemalloc_traces;
    }
    else {
        return _Py_hashtable_get(tracemalloc_domains, TO_PTR(domain));
    }
}


static void
tracemalloc_remove_trace(unsigned int domain, uintptr_t ptr)
{
    assert(_Py_tracemalloc_config.tracing);

    _Py_hashtable_t *traces = tracemalloc_get_traces_table(domain);
    if (!traces) {
        return;
    }

    trace_t *trace = _Py_hashtable_steal(traces, TO_PTR(ptr));
    if (!trace) {
        return;
    }
    assert(tracemalloc_traced_memory >= trace->size);
    tracemalloc_traced_memory -= trace->size;
    raw_free(trace);
}

#define REMOVE_TRACE(ptr) \
            tracemalloc_remove_trace(DEFAULT_DOMAIN, (uintptr_t)(ptr))


static int
tracemalloc_add_trace(unsigned int domain, uintptr_t ptr,
                      size_t size)
{
    assert(_Py_tracemalloc_config.tracing);

    traceback_t *traceback = traceback_new();
    if (traceback == NULL) {
        return -1;
    }

    _Py_hashtable_t *traces = tracemalloc_get_traces_table(domain);
    if (traces == NULL) {
        traces = tracemalloc_create_traces_table();
        if (traces == NULL) {
            return -1;
        }

        if (_Py_hashtable_set(tracemalloc_domains, TO_PTR(domain), traces) < 0) {
            _Py_hashtable_destroy(traces);
            return -1;
        }
    }

    trace_t *trace = _Py_hashtable_get(traces, TO_PTR(ptr));
    if (trace != NULL) {
        /* the memory block is already tracked */
        assert(tracemalloc_traced_memory >= trace->size);
        tracemalloc_traced_memory -= trace->size;

        trace->size = size;
        trace->traceback = traceback;
    }
    else {
        trace = raw_malloc(sizeof(trace_t));
        if (trace == NULL) {
            return -1;
        }
        trace->size = size;
        trace->traceback = traceback;

        int res = _Py_hashtable_set(traces, TO_PTR(ptr), trace);
        if (res != 0) {
            raw_free(trace);
            return res;
        }
    }

    assert(tracemalloc_traced_memory <= SIZE_MAX - size);
    tracemalloc_traced_memory += size;
    if (tracemalloc_traced_memory > tracemalloc_peak_traced_memory) {
        tracemalloc_peak_traced_memory = tracemalloc_traced_memory;
    }
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
           a trace at address ptr2 */
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
            Py_FatalError("tracemalloc_realloc() failed to allocate a trace");
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
        a deadlock in _PyThreadState_DeleteCurrent(). */

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
    PyGILState_STATE gil_state;
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

    gil_state = PyGILState_Ensure();
    ptr = tracemalloc_alloc(use_calloc, ctx, nelem, elsize);
    PyGILState_Release(gil_state);

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
    PyGILState_STATE gil_state;
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

    gil_state = PyGILState_Ensure();
    ptr2 = tracemalloc_realloc(ctx, ptr, new_size);
    PyGILState_Release(gil_state);

    set_reentrant(0);
    return ptr2;
}
#endif   /* TRACE_RAW_MALLOC */


static void
tracemalloc_clear_filename(void *value)
{
    PyObject *filename = (PyObject *)value;
    Py_DECREF(filename);
}


/* reentrant flag must be set to call this function and GIL must be held */
static void
tracemalloc_clear_traces(void)
{
    /* The GIL protects variables againt concurrent access */
    assert(PyGILState_Check());

    TABLES_LOCK();
    _Py_hashtable_clear(tracemalloc_traces);
    _Py_hashtable_clear(tracemalloc_domains);
    tracemalloc_traced_memory = 0;
    tracemalloc_peak_traced_memory = 0;
    TABLES_UNLOCK();

    _Py_hashtable_clear(tracemalloc_tracebacks);

    _Py_hashtable_clear(tracemalloc_filenames);
}


static int
tracemalloc_init(void)
{
    if (_Py_tracemalloc_config.initialized == TRACEMALLOC_FINALIZED) {
        PyErr_SetString(PyExc_RuntimeError,
                        "the tracemalloc module has been unloaded");
        return -1;
    }

    if (_Py_tracemalloc_config.initialized == TRACEMALLOC_INITIALIZED)
        return 0;

    PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &allocators.raw);

#ifdef REENTRANT_THREADLOCAL
    if (PyThread_tss_create(&tracemalloc_reentrant_key) != 0) {
#ifdef MS_WINDOWS
        PyErr_SetFromWindowsErr(0);
#else
        PyErr_SetFromErrno(PyExc_OSError);
#endif
        return -1;
    }
#endif

#if defined(TRACE_RAW_MALLOC)
    if (tables_lock == NULL) {
        tables_lock = PyThread_allocate_lock();
        if (tables_lock == NULL) {
            PyErr_SetString(PyExc_RuntimeError, "cannot allocate lock");
            return -1;
        }
    }
#endif

    tracemalloc_filenames = hashtable_new(hashtable_hash_pyobject,
                                          hashtable_compare_unicode,
                                          tracemalloc_clear_filename, NULL);

    tracemalloc_tracebacks = hashtable_new(hashtable_hash_traceback,
                                           hashtable_compare_traceback,
                                           NULL, raw_free);

    tracemalloc_traces = tracemalloc_create_traces_table();
    tracemalloc_domains = tracemalloc_create_domains_table();

    if (tracemalloc_filenames == NULL || tracemalloc_tracebacks == NULL
       || tracemalloc_traces == NULL || tracemalloc_domains == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    unknown_filename = PyUnicode_FromString("<unknown>");
    if (unknown_filename == NULL)
        return -1;
    PyUnicode_InternInPlace(&unknown_filename);

    tracemalloc_empty_traceback.nframe = 1;
    tracemalloc_empty_traceback.total_nframe = 1;
    /* borrowed reference */
    tracemalloc_empty_traceback.frames[0].filename = unknown_filename;
    tracemalloc_empty_traceback.frames[0].lineno = 0;
    tracemalloc_empty_traceback.hash = traceback_hash(&tracemalloc_empty_traceback);

    _Py_tracemalloc_config.initialized = TRACEMALLOC_INITIALIZED;
    return 0;
}


static void
tracemalloc_deinit(void)
{
    if (_Py_tracemalloc_config.initialized != TRACEMALLOC_INITIALIZED)
        return;
    _Py_tracemalloc_config.initialized = TRACEMALLOC_FINALIZED;

    tracemalloc_stop();

    /* destroy hash tables */
    _Py_hashtable_destroy(tracemalloc_domains);
    _Py_hashtable_destroy(tracemalloc_traces);
    _Py_hashtable_destroy(tracemalloc_tracebacks);
    _Py_hashtable_destroy(tracemalloc_filenames);

#if defined(TRACE_RAW_MALLOC)
    if (tables_lock != NULL) {
        PyThread_free_lock(tables_lock);
        tables_lock = NULL;
    }
#endif

#ifdef REENTRANT_THREADLOCAL
    PyThread_tss_delete(&tracemalloc_reentrant_key);
#endif

    Py_XDECREF(unknown_filename);
}


static int
tracemalloc_start(int max_nframe)
{
    PyMemAllocatorEx alloc;
    size_t size;

    if (max_nframe < 1 || (unsigned long) max_nframe > MAX_NFRAME) {
        PyErr_Format(PyExc_ValueError,
                     "the number of frames must be in range [1; %lu]",
                     MAX_NFRAME);
        return -1;
    }

    if (tracemalloc_init() < 0) {
        return -1;
    }

    if (_Py_tracemalloc_config.tracing) {
        /* hook already installed: do nothing */
        return 0;
    }

    _Py_tracemalloc_config.max_nframe = max_nframe;

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
    _Py_tracemalloc_config.tracing = 1;

    return 0;
}


static void
tracemalloc_stop(void)
{
    if (!_Py_tracemalloc_config.tracing)
        return;

    /* stop tracing Python memory allocations */
    _Py_tracemalloc_config.tracing = 0;

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



/*[clinic input]
_tracemalloc.is_tracing

Return True if the tracemalloc module is tracing Python memory allocations.
[clinic start generated code]*/

static PyObject *
_tracemalloc_is_tracing_impl(PyObject *module)
/*[clinic end generated code: output=2d763b42601cd3ef input=af104b0a00192f63]*/
{
    return PyBool_FromLong(_Py_tracemalloc_config.tracing);
}


/*[clinic input]
_tracemalloc.clear_traces

Clear traces of memory blocks allocated by Python.
[clinic start generated code]*/

static PyObject *
_tracemalloc_clear_traces_impl(PyObject *module)
/*[clinic end generated code: output=a86080ee41b84197 input=0dab5b6c785183a5]*/
{
    if (!_Py_tracemalloc_config.tracing)
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
    PyObject *frames;

    if (intern_table != NULL) {
        frames = _Py_hashtable_get(intern_table, (const void *)traceback);
        if (frames) {
            Py_INCREF(frames);
            return frames;
        }
    }

    frames = PyTuple_New(traceback->nframe);
    if (frames == NULL)
        return NULL;

    for (int i=0; i < traceback->nframe; i++) {
        PyObject *frame = frame_to_pyobject(&traceback->frames[i]);
        if (frame == NULL) {
            Py_DECREF(frames);
            return NULL;
        }
        PyTuple_SET_ITEM(frames, i, frame);
    }

    if (intern_table != NULL) {
        if (_Py_hashtable_set(intern_table, traceback, frames) < 0) {
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
trace_to_pyobject(unsigned int domain, const trace_t *trace,
                  _Py_hashtable_t *intern_tracebacks)
{
    PyObject *trace_obj = NULL;
    PyObject *obj;

    trace_obj = PyTuple_New(4);
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

    obj = PyLong_FromUnsignedLong(trace->traceback->total_nframe);
    if (obj == NULL) {
        Py_DECREF(trace_obj);
        return NULL;
    }
    PyTuple_SET_ITEM(trace_obj, 3, obj);

    return trace_obj;
}


typedef struct {
    _Py_hashtable_t *traces;
    _Py_hashtable_t *domains;
    _Py_hashtable_t *tracebacks;
    PyObject *list;
    unsigned int domain;
} get_traces_t;


static int
tracemalloc_copy_trace(_Py_hashtable_t *traces,
                       const void *key, const void *value,
                       void *user_data)
{
    _Py_hashtable_t *traces2 = (_Py_hashtable_t *)user_data;

    trace_t *trace = (trace_t *)value;

    trace_t *trace2 = raw_malloc(sizeof(trace_t));
    if (traces2 == NULL) {
        return -1;
    }
    *trace2 = *trace;
    if (_Py_hashtable_set(traces2, key, trace2) < 0) {
        raw_free(trace2);
        return -1;
    }
    return 0;
}


static _Py_hashtable_t*
tracemalloc_copy_traces(_Py_hashtable_t *traces)
{
    _Py_hashtable_t *traces2 = tracemalloc_create_traces_table();
    if (traces2 == NULL) {
        return NULL;
    }

    int err = _Py_hashtable_foreach(traces,
                                    tracemalloc_copy_trace,
                                    traces2);
    if (err) {
        _Py_hashtable_destroy(traces2);
        return NULL;
    }
    return traces2;
}


static int
tracemalloc_copy_domain(_Py_hashtable_t *domains,
                        const void *key, const void *value,
                        void *user_data)
{
    _Py_hashtable_t *domains2 = (_Py_hashtable_t *)user_data;

    unsigned int domain = (unsigned int)FROM_PTR(key);
    _Py_hashtable_t *traces = (_Py_hashtable_t *)value;

    _Py_hashtable_t *traces2 = tracemalloc_copy_traces(traces);
    if (_Py_hashtable_set(domains2, TO_PTR(domain), traces2) < 0) {
        _Py_hashtable_destroy(traces2);
        return -1;
    }
    return 0;
}


static _Py_hashtable_t*
tracemalloc_copy_domains(_Py_hashtable_t *domains)
{
    _Py_hashtable_t *domains2 = tracemalloc_create_domains_table();
    if (domains2 == NULL) {
        return NULL;
    }

    int err = _Py_hashtable_foreach(domains,
                                    tracemalloc_copy_domain,
                                    domains2);
    if (err) {
        _Py_hashtable_destroy(domains2);
        return NULL;
    }
    return domains2;
}


static int
tracemalloc_get_traces_fill(_Py_hashtable_t *traces,
                            const void *key, const void *value,
                            void *user_data)
{
    get_traces_t *get_traces = user_data;

    const trace_t *trace = (const trace_t *)value;

    PyObject *tuple = trace_to_pyobject(get_traces->domain, trace,
                                        get_traces->tracebacks);
    if (tuple == NULL) {
        return 1;
    }

    int res = PyList_Append(get_traces->list, tuple);
    Py_DECREF(tuple);
    if (res < 0) {
        return 1;
    }

    return 0;
}


static int
tracemalloc_get_traces_domain(_Py_hashtable_t *domains,
                              const void *key, const void *value,
                              void *user_data)
{
    get_traces_t *get_traces = user_data;

    unsigned int domain = (unsigned int)FROM_PTR(key);
    _Py_hashtable_t *traces = (_Py_hashtable_t *)value;

    get_traces->domain = domain;
    return _Py_hashtable_foreach(traces,
                                 tracemalloc_get_traces_fill,
                                 get_traces);
}


static void
tracemalloc_pyobject_decref(void *value)
{
    PyObject *obj = (PyObject *)value;
    Py_DECREF(obj);
}



/*[clinic input]
_tracemalloc._get_traces

Get traces of all memory blocks allocated by Python.

Return a list of (size: int, traceback: tuple) tuples.
traceback is a tuple of (filename: str, lineno: int) tuples.

Return an empty list if the tracemalloc module is disabled.
[clinic start generated code]*/

static PyObject *
_tracemalloc__get_traces_impl(PyObject *module)
/*[clinic end generated code: output=e9929876ced4b5cc input=6c7d2230b24255aa]*/
{
    get_traces_t get_traces;
    get_traces.domain = DEFAULT_DOMAIN;
    get_traces.traces = NULL;
    get_traces.domains = NULL;
    get_traces.tracebacks = NULL;
    get_traces.list = PyList_New(0);
    if (get_traces.list == NULL)
        goto error;

    if (!_Py_tracemalloc_config.tracing)
        return get_traces.list;

    /* the traceback hash table is used temporarily to intern traceback tuple
       of (filename, lineno) tuples */
    get_traces.tracebacks = hashtable_new(_Py_hashtable_hash_ptr,
                                          _Py_hashtable_compare_direct,
                                          NULL, tracemalloc_pyobject_decref);
    if (get_traces.tracebacks == NULL) {
        goto no_memory;
    }

    // Copy all traces so tracemalloc_get_traces_fill() doesn't have to disable
    // temporarily tracemalloc which would impact other threads and so would
    // miss allocations while get_traces() is called.
    TABLES_LOCK();
    get_traces.traces = tracemalloc_copy_traces(tracemalloc_traces);
    TABLES_UNLOCK();

    if (get_traces.traces == NULL) {
        goto no_memory;
    }

    TABLES_LOCK();
    get_traces.domains = tracemalloc_copy_domains(tracemalloc_domains);
    TABLES_UNLOCK();

    if (get_traces.domains == NULL) {
        goto no_memory;
    }

    // Convert traces to a list of tuples
    set_reentrant(1);
    int err = _Py_hashtable_foreach(get_traces.traces,
                                    tracemalloc_get_traces_fill,
                                    &get_traces);
    if (!err) {
        err = _Py_hashtable_foreach(get_traces.domains,
                                    tracemalloc_get_traces_domain,
                                    &get_traces);
    }
    set_reentrant(0);
    if (err) {
        goto error;
    }

    goto finally;

no_memory:
    PyErr_NoMemory();

error:
    Py_CLEAR(get_traces.list);

finally:
    if (get_traces.tracebacks != NULL) {
        _Py_hashtable_destroy(get_traces.tracebacks);
    }
    if (get_traces.traces != NULL) {
        _Py_hashtable_destroy(get_traces.traces);
    }
    if (get_traces.domains != NULL) {
        _Py_hashtable_destroy(get_traces.domains);
    }

    return get_traces.list;
}


static traceback_t*
tracemalloc_get_traceback(unsigned int domain, uintptr_t ptr)
{

    if (!_Py_tracemalloc_config.tracing)
        return NULL;

    trace_t *trace;
    TABLES_LOCK();
    _Py_hashtable_t *traces = tracemalloc_get_traces_table(domain);
    if (traces) {
        trace = _Py_hashtable_get(traces, TO_PTR(ptr));
    }
    else {
        trace = NULL;
    }
    TABLES_UNLOCK();

    if (!trace) {
        return NULL;
    }

    return trace->traceback;
}



/*[clinic input]
_tracemalloc._get_object_traceback

    obj: object
    /

Get the traceback where the Python object obj was allocated.

Return a tuple of (filename: str, lineno: int) tuples.
Return None if the tracemalloc module is disabled or did not
trace the allocation of the object.
[clinic start generated code]*/

static PyObject *
_tracemalloc__get_object_traceback(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=41ee0553a658b0aa input=29495f1b21c53212]*/
{
    PyTypeObject *type;
    void *ptr;
    traceback_t *traceback;

    type = Py_TYPE(obj);
    if (PyType_IS_GC(type)) {
        ptr = (void *)((char *)obj - sizeof(PyGC_Head));
    }
    else {
        ptr = (void *)obj;
    }

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

    if (!_Py_tracemalloc_config.tracing) {
        PUTS(fd, "Enable tracemalloc to get the memory block "
                 "allocation traceback\n\n");
        return;
    }

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



/*[clinic input]
_tracemalloc.start

    nframe: int = 1
    /

Start tracing Python memory allocations.

Also set the maximum number of frames stored in the traceback of a
trace to nframe.
[clinic start generated code]*/

static PyObject *
_tracemalloc_start_impl(PyObject *module, int nframe)
/*[clinic end generated code: output=caae05c23c159d3c input=40d849b5b29d1933]*/
{
    if (tracemalloc_start(nframe) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/*[clinic input]
_tracemalloc.stop

Stop tracing Python memory allocations.

Also clear traces of memory blocks allocated by Python.
[clinic start generated code]*/

static PyObject *
_tracemalloc_stop_impl(PyObject *module)
/*[clinic end generated code: output=c3c42ae03e3955cd input=7478f075e51dae18]*/
{
    tracemalloc_stop();
    Py_RETURN_NONE;
}


/*[clinic input]
_tracemalloc.get_traceback_limit

Get the maximum number of frames stored in the traceback of a trace.

By default, a trace of an allocated memory block only stores
the most recent frame: the limit is 1.
[clinic start generated code]*/

static PyObject *
_tracemalloc_get_traceback_limit_impl(PyObject *module)
/*[clinic end generated code: output=d556d9306ba95567 input=da3cd977fc68ae3b]*/
{
    return PyLong_FromLong(_Py_tracemalloc_config.max_nframe);
}


static int
tracemalloc_get_tracemalloc_memory_cb(_Py_hashtable_t *domains,
                                      const void *key, const void *value,
                                      void *user_data)
{
    const _Py_hashtable_t *traces = value;
    size_t *size = (size_t*)user_data;
    *size += _Py_hashtable_size(traces);
    return 0;
}


/*[clinic input]
_tracemalloc.get_tracemalloc_memory

Get the memory usage in bytes of the tracemalloc module.

This memory is used internally to trace memory allocations.
[clinic start generated code]*/

static PyObject *
_tracemalloc_get_tracemalloc_memory_impl(PyObject *module)
/*[clinic end generated code: output=e3f14e280a55f5aa input=5d919c0f4d5132ad]*/
{
    size_t size;

    size = _Py_hashtable_size(tracemalloc_tracebacks);
    size += _Py_hashtable_size(tracemalloc_filenames);

    TABLES_LOCK();
    size += _Py_hashtable_size(tracemalloc_traces);
    _Py_hashtable_foreach(tracemalloc_domains,
                          tracemalloc_get_tracemalloc_memory_cb, &size);
    TABLES_UNLOCK();

    return PyLong_FromSize_t(size);
}



/*[clinic input]
_tracemalloc.get_traced_memory

Get the current size and peak size of memory blocks traced by tracemalloc.

Returns a tuple: (current: int, peak: int).
[clinic start generated code]*/

static PyObject *
_tracemalloc_get_traced_memory_impl(PyObject *module)
/*[clinic end generated code: output=5b167189adb9e782 input=61ddb5478400ff66]*/
{
    Py_ssize_t size, peak_size;

    if (!_Py_tracemalloc_config.tracing)
        return Py_BuildValue("ii", 0, 0);

    TABLES_LOCK();
    size = tracemalloc_traced_memory;
    peak_size = tracemalloc_peak_traced_memory;
    TABLES_UNLOCK();

    return Py_BuildValue("nn", size, peak_size);
}

/*[clinic input]
_tracemalloc.reset_peak

Set the peak size of memory blocks traced by tracemalloc to the current size.

Do nothing if the tracemalloc module is not tracing memory allocations.

[clinic start generated code]*/

static PyObject *
_tracemalloc_reset_peak_impl(PyObject *module)
/*[clinic end generated code: output=140c2870f691dbb2 input=18afd0635066e9ce]*/
{
    if (!_Py_tracemalloc_config.tracing) {
        Py_RETURN_NONE;
    }

    TABLES_LOCK();
    tracemalloc_peak_traced_memory = tracemalloc_traced_memory;
    TABLES_UNLOCK();

    Py_RETURN_NONE;
}


static PyMethodDef module_methods[] = {
    _TRACEMALLOC_IS_TRACING_METHODDEF
    _TRACEMALLOC_CLEAR_TRACES_METHODDEF
    _TRACEMALLOC__GET_TRACES_METHODDEF
    _TRACEMALLOC__GET_OBJECT_TRACEBACK_METHODDEF
    _TRACEMALLOC_START_METHODDEF
    _TRACEMALLOC_STOP_METHODDEF
    _TRACEMALLOC_GET_TRACEBACK_LIMIT_METHODDEF
    _TRACEMALLOC_GET_TRACEMALLOC_MEMORY_METHODDEF
    _TRACEMALLOC_GET_TRACED_MEMORY_METHODDEF
    _TRACEMALLOC_RESET_PEAK_METHODDEF
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

    if (tracemalloc_init() < 0) {
        Py_DECREF(m);
        return NULL;
    }

    return m;
}


int
_PyTraceMalloc_Init(int nframe)
{
    assert(PyGILState_Check());
    if (nframe == 0) {
        return 0;
    }
    return tracemalloc_start(nframe);
}


void
_PyTraceMalloc_Fini(void)
{
    assert(PyGILState_Check());
    tracemalloc_deinit();
}

int
PyTraceMalloc_Track(unsigned int domain, uintptr_t ptr,
                    size_t size)
{
    int res;
    PyGILState_STATE gil_state;

    if (!_Py_tracemalloc_config.tracing) {
        /* tracemalloc is not tracing: do nothing */
        return -2;
    }

    gil_state = PyGILState_Ensure();

    TABLES_LOCK();
    res = tracemalloc_add_trace(domain, ptr, size);
    TABLES_UNLOCK();

    PyGILState_Release(gil_state);
    return res;
}


int
PyTraceMalloc_Untrack(unsigned int domain, uintptr_t ptr)
{
    if (!_Py_tracemalloc_config.tracing) {
        /* tracemalloc is not tracing: do nothing */
        return -2;
    }

    TABLES_LOCK();
    tracemalloc_remove_trace(domain, ptr);
    TABLES_UNLOCK();

    return 0;
}


/* If the object memory block is already traced, update its trace
   with the current Python traceback.

   Do nothing if tracemalloc is not tracing memory allocations
   or if the object memory block is not already traced. */
int
_PyTraceMalloc_NewReference(PyObject *op)
{
    assert(PyGILState_Check());

    if (!_Py_tracemalloc_config.tracing) {
        /* tracemalloc is not tracing: do nothing */
        return -1;
    }

    uintptr_t ptr;
    PyTypeObject *type = Py_TYPE(op);
    if (PyType_IS_GC(type)) {
        ptr = (uintptr_t)((char *)op - sizeof(PyGC_Head));
    }
    else {
        ptr = (uintptr_t)op;
    }

    int res = -1;

    TABLES_LOCK();
    trace_t *trace = _Py_hashtable_get(tracemalloc_traces, TO_PTR(ptr));
    if (trace != NULL) {
        /* update the traceback of the memory block */
        traceback_t *traceback = traceback_new();
        if (traceback != NULL) {
            trace->traceback = traceback;
            res = 0;
        }
    }
    /* else: cannot track the object, its memory block size is unknown */
    TABLES_UNLOCK();

    return res;
}


PyObject*
_PyTraceMalloc_GetTraceback(unsigned int domain, uintptr_t ptr)
{
    traceback_t *traceback;

    traceback = tracemalloc_get_traceback(domain, ptr);
    if (traceback == NULL)
        Py_RETURN_NONE;

    return traceback_to_pyobject(traceback, NULL);
}
