#include "Python.h"
#include "pycore_atomic.h"          
#include "pycore_fileutils.h"       
#include "pycore_gc.h"              
#include "pycore_hashtable.h"       
#include "pycore_object.h"          
#include "pycore_pymem.h"           
#include "pycore_runtime.h"         
#include "pycore_traceback.h"       
#include "pycore_frame.h"
#include "frameobject.h"            
#include <stdlib.h>                 

#define tracemalloc_config _PyRuntime.tracemalloc.config
_Py_DECLARE_STR(anon_unknown, "<unknown>");

static void* raw_malloc(size_t size);
static void raw_free(void *ptr);

/* Enhanced thread-safety implementation with thread-local storage */
#if defined(TRACE_RAW_MALLOC)
    #define REENTRANT_THREADLOCAL
    #define tracemalloc_reentrant_key _PyRuntime.tracemalloc.reentrant_key
    #define REENTRANT Py_True

    static int get_reentrant(void) {
        void *ptr;
        assert(PyThread_tss_is_created(&tracemalloc_reentrant_key));
        ptr = PyThread_tss_get(&tracemalloc_reentrant_key);
        return ptr != NULL;
    }

    static void set_reentrant(int reentrant) {
        assert(PyThread_tss_is_created(&tracemalloc_reentrant_key));
        PyThread_tss_set(&tracemalloc_reentrant_key, reentrant ? REENTRANT : NULL);
    }
#else
    static int tracemalloc_reentrant = 0;
    static int get_reentrant(void) { return tracemalloc_reentrant; }
    static void set_reentrant(int reentrant) { tracemalloc_reentrant = reentrant; }
#endif

/* Optimized data structures for memory usage tracking */
typedef struct tracemalloc_frame {
    PyObject *filename;    
    unsigned int lineno;   
} frame_t;

typedef struct tracemalloc_traceback {
    Py_uhash_t hash;          
    unsigned int nframe;       
    unsigned int total_nframe; 
    frame_t frames[1];        
} traceback_t;

/* Dynamic memory allocation for tracebacks based on frame count */
#define TRACEBACK_SIZE(NFRAME) \
    (sizeof(traceback_t) + sizeof(frame_t) * ((NFRAME) - 1))

/* Improved maximum frame calculation to prevent overflow */
static const unsigned long MAX_NFRAME = Py_MIN(UINT16_MAX, 
    ((SIZE_MAX - sizeof(traceback_t)) / sizeof(frame_t) + 1));

typedef struct {
    size_t size;              
    traceback_t *traceback;   
} trace_t;

/* Optimized hash functions for better performance */
static Py_uhash_t hashtable_hash_traceback(const void *key) {
    const traceback_t *traceback = (const traceback_t *)key;
    return traceback->hash;
}

/* Enhanced frame capture with better error handling */
static void tracemalloc_get_frame(_PyInterpreterFrame *pyframe, frame_t *frame) {
    assert(pyframe != NULL);
    
    int lineno = PyUnstable_InterpreterFrame_GetLine(pyframe);
    frame->lineno = (lineno >= 0) ? (unsigned int)lineno : 0;

    PyObject *filename = _PyFrame_GetCode(pyframe)->co_filename;
    if (filename && PyUnicode_Check(filename)) {
        _Py_hashtable_entry_t *entry = 
            _Py_hashtable_get_entry(tracemalloc_filenames, filename);
        
        if (entry) {
            frame->filename = (PyObject *)entry->key;
        } else {
            if (_Py_hashtable_set(tracemalloc_filenames, 
                                 Py_NewRef(filename), NULL) == 0) {
                frame->filename = filename;
            } else {
                frame->filename = &_Py_STR(anon_unknown);
            }
        }
    } else {
        frame->filename = &_Py_STR(anon_unknown);
    }
}

/* Improved trace management with memory optimization */
static traceback_t* traceback_new(void) {
    int max_nframe = tracemalloc_config.max_nframe;
    size_t traceback_size = TRACEBACK_SIZE(max_nframe);

    traceback_t *traceback = raw_malloc(traceback_size);
    if (!traceback) return NULL;

    traceback->nframe = 0;
    traceback->total_nframe = 0;

    PyThreadState *tstate = PyThreadState_GET();
    if (!tstate) {
        raw_free(traceback);
        return NULL;
    }

    /* Enhanced frame collection with proper cleanup */
    _PyInterpreterFrame *pyframe = tstate->frame;
    while (pyframe && traceback->nframe < max_nframe) {
        tracemalloc_get_frame(pyframe, &traceback->frames[traceback->nframe]);
        traceback->nframe++;
        if (traceback->total_nframe < UINT16_MAX) {
            traceback->total_nframe++;
        }
        pyframe = pyframe->previous;
    }

    if (traceback->nframe == 0) {
        raw_free(traceback);
        return &tracemalloc_empty_traceback;
    }

    traceback->hash = traceback_hash(traceback);
    return traceback;
}

/* Optimized trace addition with better memory management */
static int tracemalloc_add_trace(unsigned int domain, uintptr_t ptr, size_t size) {
    assert(tracemalloc_config.tracing);

    traceback_t *traceback = traceback_new();
    if (!traceback) return -1;

    _Py_hashtable_t *traces = tracemalloc_get_traces_table(domain);
    if (!traces) {
        traces = tracemalloc_create_traces_table();
        if (!traces || _Py_hashtable_set(tracemalloc_domains, 
                                        (void *)(uintptr_t)domain, traces) < 0) {
            _Py_hashtable_destroy(traces);
            return -1;
        }
    }

    trace_t *trace = _Py_hashtable_get(traces, (const void *)ptr);
    if (trace) {
        tracemalloc_traced_memory -= trace->size;
        trace->size = size;
        trace->traceback = traceback;
    } else {
        trace = raw_malloc(sizeof(trace_t));
        if (!trace) return -1;
        
        trace->size = size;
        trace->traceback = traceback;
        if (_Py_hashtable_set(traces, (void *)ptr, trace) < 0) {
            raw_free(trace);
            return -1;
        }
    }

    tracemalloc_traced_memory += size;
    tracemalloc_peak_traced_memory = Py_MAX(tracemalloc_peak_traced_memory, 
                                           tracemalloc_traced_memory);
    return 0;
}

/* Enhanced initialization with better error handling */
int _PyTraceMalloc_Init(void) {
    if (tracemalloc_config.initialized == TRACEMALLOC_FINALIZED) {
        PyErr_SetString(PyExc_RuntimeError, 
                       "tracemalloc module has been unloaded");
        return -1;
    }

    if (tracemalloc_config.initialized == TRACEMALLOC_INITIALIZED)
        return 0;

    PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &allocators.raw);

    #ifdef REENTRANT_THREADLOCAL
    if (PyThread_tss_create(&tracemalloc_reentrant_key) != 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    #endif

    /* Initialize core data structures with proper error handling */
    if (!tracemalloc_filenames || !tracemalloc_tracebacks ||
        !tracemalloc_traces || !tracemalloc_domains) {
        PyErr_NoMemory();
        return -1;
    }

    tracemalloc_config.initialized = TRACEMALLOC_INITIALIZED;
    return 0;
}
