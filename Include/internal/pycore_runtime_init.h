#ifndef Py_INTERNAL_RUNTIME_INIT_H
#define Py_INTERNAL_RUNTIME_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_structs.h"
#include "pycore_ceval_state.h"   // _PyEval_RUNTIME_PERF_INIT
#include "pycore_debug_offsets.h"  // _Py_DebugOffsets_INIT()
#include "pycore_dtoa.h"          // _dtoa_state_INIT()
#include "pycore_faulthandler.h"  // _faulthandler_runtime_state_INIT
#include "pycore_floatobject.h"   // _py_float_format_unknown
#include "pycore_function.h"
#include "pycore_hamt.h"          // _PyHamt_BitmapNode_Type
#include "pycore_import.h"        // IMPORTS_INIT
#include "pycore_object.h"        // _PyObject_HEAD_INIT
#include "pycore_obmalloc_init.h" // _obmalloc_global_state_INIT
#include "pycore_parser.h"        // _parser_runtime_state_INIT
#include "pycore_pyhash.h"        // pyhash_state_INIT
#include "pycore_pymem_init.h"    // _pymem_allocators_standard_INIT
#include "pycore_pythread.h"      // _pythread_RUNTIME_INIT
#include "pycore_qsbr.h"          // QSBR_INITIAL
#include "pycore_runtime_init_generated.h"  // _Py_bytes_characters_INIT
#include "pycore_signal.h"        // _signals_RUNTIME_INIT
#include "pycore_tracemalloc.h"   // _tracemalloc_runtime_state_INIT
#include "pycore_tuple.h"         // _PyTuple_HASH_EMPTY


extern PyTypeObject _PyExc_MemoryError;


/* The static initializers defined here should only be used
   in the runtime init code (in pystate.c and pylifecycle.c). */

#define _PyRuntimeState_INIT(runtime, debug_cookie) \
    { \
        .debug_offsets = _Py_DebugOffsets_INIT(debug_cookie), \
        .allocators = { \
            .standard = _pymem_allocators_standard_INIT(runtime), \
            .debug = _pymem_allocators_debug_INIT, \
            .obj_arena = _pymem_allocators_obj_arena_INIT, \
            .is_debug_enabled = _pymem_is_debug_enabled_INIT, \
        }, \
        .obmalloc = _obmalloc_global_state_INIT, \
        .pyhash_state = pyhash_state_INIT, \
        .threads = _pythread_RUNTIME_INIT(runtime.threads), \
        .signals = _signals_RUNTIME_INIT, \
        .interpreters = { \
            /* This prevents interpreters from getting created \
              until _PyInterpreterState_Enable() is called. */ \
            .next_id = -1, \
        }, \
        .xi = { \
            .data_lookup = { \
                .registry = { \
                    .global = 1, \
                }, \
            }, \
        }, \
        .parser = _parser_runtime_state_INIT, \
        .ceval = { \
            .pending_mainthread = { \
                .max = MAXPENDINGCALLS_MAIN, \
                .maxloop = MAXPENDINGCALLSLOOP_MAIN, \
            }, \
            .perf = _PyEval_RUNTIME_PERF_INIT, \
        }, \
        .gilstate = { \
            .check_enabled = 1, \
        }, \
        .fileutils = { \
            .force_ascii = -1, \
        }, \
        .faulthandler = _faulthandler_runtime_state_INIT, \
        .tracemalloc = _tracemalloc_runtime_state_INIT, \
        .ref_tracer = { \
            .tracer_func = NULL, \
            .tracer_data = NULL, \
        }, \
        .stoptheworld = { \
            .is_global = 1, \
        }, \
        .float_state = { \
            .float_format = _py_float_format_unknown, \
            .double_format = _py_float_format_unknown, \
        }, \
        .types = { \
            .next_version_tag = _Py_TYPE_VERSION_NEXT, \
        }, \
        .static_objects = { \
            .singletons = { \
                .small_ints = _Py_small_ints_INIT, \
                .bytes_empty = _PyBytes_SIMPLE_INIT(0, 0), \
                .bytes_characters = _Py_bytes_characters_INIT, \
                .strings = { \
                    .literals = _Py_str_literals_INIT, \
                    .identifiers = _Py_str_identifiers_INIT, \
                    .ascii = _Py_str_ascii_INIT, \
                    .latin1 = _Py_str_latin1_INIT, \
                }, \
                .tuple_empty = { \
                    .ob_base = _PyVarObject_HEAD_INIT(&PyTuple_Type, 0), \
                    .ob_hash = _PyTuple_HASH_EMPTY, \
                }, \
                .hamt_bitmap_node_empty = { \
                    .ob_base = _PyVarObject_HEAD_INIT(&_PyHamt_BitmapNode_Type, 0), \
                }, \
                .context_token_missing = { \
                    .ob_base = _PyObject_HEAD_INIT(&_PyContextTokenMissing_Type), \
                }, \
            }, \
        }, \
        ._main_interpreter = _PyInterpreterState_INIT(runtime._main_interpreter), \
    }

#define _PyInterpreterState_INIT(INTERP) \
    { \
        .id_refcount = -1, \
        ._whence = _PyInterpreterState_WHENCE_NOTSET, \
        .threads = { \
            .preallocated = &(INTERP)._initial_thread, \
        }, \
        .imports = IMPORTS_INIT, \
        .ceval = { \
            .recursion_limit = Py_DEFAULT_RECURSION_LIMIT, \
            .pending = { \
                .max = MAXPENDINGCALLS, \
                .maxloop = MAXPENDINGCALLSLOOP, \
            }, \
        }, \
        .gc = { \
            .enabled = 1, \
            .young = { .threshold = 2000, }, \
            .old = { \
                { .threshold = 10, }, \
                { .threshold = 0, }, \
            }, \
            .work_to_do = -5000, \
            .phase = GC_PHASE_MARK, \
        }, \
        .qsbr = { \
            .wr_seq = QSBR_INITIAL, \
            .rd_seq = QSBR_INITIAL, \
        }, \
        .dtoa = _dtoa_state_INIT(&(INTERP)), \
        .dict_state = _dict_state_INIT, \
        .mem_free_queue = _Py_mem_free_queue_INIT(INTERP.mem_free_queue), \
        .func_state = { \
            .next_version = FUNC_VERSION_FIRST_VALID, \
        }, \
        .types = { \
            .next_version_tag = _Py_TYPE_BASE_VERSION_TAG, \
        }, \
        .static_objects = { \
            .singletons = { \
                ._not_used = 1, \
                .hamt_empty = { \
                    .ob_base = _PyObject_HEAD_INIT(&_PyHamt_Type), \
                    .h_root = (PyHamtNode*)&_Py_SINGLETON(hamt_bitmap_node_empty), \
                }, \
                .last_resort_memory_error = { \
                    _PyObject_HEAD_INIT(&_PyExc_MemoryError), \
                    .args = (PyObject*)&_Py_SINGLETON(tuple_empty) \
                }, \
            }, \
        }, \
        ._initial_thread = _PyThreadStateImpl_INIT, \
    }

#define _PyThreadStateImpl_INIT \
    { \
        .base = _PyThreadState_INIT, \
        /* The thread and the interpreter's linked list hold a reference */ \
        .refcount = 2, \
    }

#define _PyThreadState_INIT \
    { \
        ._whence = _PyThreadState_WHENCE_NOTSET, \
        .py_recursion_limit = Py_DEFAULT_RECURSION_LIMIT, \
        .context_ver = 1, \
    }


// global objects

#define _PyBytes_SIMPLE_INIT(CH, LEN) \
    { \
        _PyVarObject_HEAD_INIT(&PyBytes_Type, (LEN)), \
        .ob_shash = -1, \
        .ob_sval = { (CH) }, \
    }
#define _PyBytes_CHAR_INIT(CH) \
    { \
        _PyBytes_SIMPLE_INIT((CH), 1) \
    }

#define _PyUnicode_ASCII_BASE_INIT(LITERAL, ASCII) \
    { \
        .ob_base = _PyObject_HEAD_INIT(&PyUnicode_Type), \
        .length = sizeof(LITERAL) - 1, \
        .hash = -1, \
        .state = { \
            .kind = 1, \
            .compact = 1, \
            .ascii = (ASCII), \
            .statically_allocated = 1, \
        }, \
    }
#define _PyASCIIObject_INIT(LITERAL) \
    { \
        ._ascii = _PyUnicode_ASCII_BASE_INIT((LITERAL), 1), \
        ._data = (LITERAL) \
    }
#define INIT_STR(NAME, LITERAL) \
    ._py_ ## NAME = _PyASCIIObject_INIT(LITERAL)
#define INIT_ID(NAME) \
    ._py_ ## NAME = _PyASCIIObject_INIT(#NAME)
#define _PyUnicode_LATIN1_INIT(LITERAL, UTF8) \
    { \
        ._latin1 = { \
            ._base = _PyUnicode_ASCII_BASE_INIT((LITERAL), 0), \
            .utf8 = (UTF8), \
            .utf8_length = sizeof(UTF8) - 1, \
        }, \
        ._data = (LITERAL), \
    }

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_RUNTIME_INIT_H */