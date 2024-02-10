#ifndef Py_INTERNAL_RUNTIME_INIT_H
#define Py_INTERNAL_RUNTIME_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_ceval_state.h"   // _PyEval_RUNTIME_PERF_INIT
#include "pycore_faulthandler.h"  // _faulthandler_runtime_state_INIT
#include "pycore_floatobject.h"   // _py_float_format_unknown
#include "pycore_object.h"        // _PyObject_HEAD_INIT
#include "pycore_obmalloc_init.h" // _obmalloc_global_state_INIT
#include "pycore_parser.h"        // _parser_runtime_state_INIT
#include "pycore_pyhash.h"        // pyhash_state_INIT
#include "pycore_pymem_init.h"    // _pymem_allocators_standard_INIT
#include "pycore_runtime_init_generated.h"  // _Py_bytes_characters_INIT
#include "pycore_signal.h"        // _signals_RUNTIME_INIT
#include "pycore_tracemalloc.h"   // _tracemalloc_runtime_state_INIT


extern PyTypeObject _PyExc_MemoryError;


/* The static initializers defined here should only be used
   in the runtime init code (in pystate.c and pylifecycle.c). */

#define _PyRuntimeState_INIT(runtime) \
    { \
        .debug_offsets = { \
            .cookie = "xdebugpy", \
            .version = PY_VERSION_HEX, \
            .runtime_state = { \
                .finalizing = offsetof(_PyRuntimeState, _finalizing), \
                .interpreters_head = offsetof(_PyRuntimeState, interpreters.head), \
            }, \
            .interpreter_state = { \
                .next = offsetof(PyInterpreterState, next), \
                .threads_head = offsetof(PyInterpreterState, threads.head), \
                .gc = offsetof(PyInterpreterState, gc), \
                .imports_modules = offsetof(PyInterpreterState, imports.modules), \
                .sysdict = offsetof(PyInterpreterState, sysdict), \
                .builtins = offsetof(PyInterpreterState, builtins), \
                .ceval_gil = offsetof(PyInterpreterState, ceval.gil), \
                .gil_runtime_state_locked = offsetof(PyInterpreterState, _gil.locked), \
                .gil_runtime_state_holder = offsetof(PyInterpreterState, _gil.last_holder), \
            }, \
            .thread_state = { \
                .prev = offsetof(PyThreadState, prev), \
                .next = offsetof(PyThreadState, next), \
                .interp = offsetof(PyThreadState, interp), \
                .current_frame = offsetof(PyThreadState, current_frame), \
                .thread_id = offsetof(PyThreadState, thread_id), \
                .native_thread_id = offsetof(PyThreadState, native_thread_id), \
            }, \
            .interpreter_frame = { \
                .previous = offsetof(_PyInterpreterFrame, previous), \
                .executable = offsetof(_PyInterpreterFrame, f_executable), \
                .instr_ptr = offsetof(_PyInterpreterFrame, instr_ptr), \
                .localsplus = offsetof(_PyInterpreterFrame, localsplus), \
                .owner = offsetof(_PyInterpreterFrame, owner), \
            }, \
            .code_object = { \
                .filename = offsetof(PyCodeObject, co_filename), \
                .name = offsetof(PyCodeObject, co_name), \
                .linetable = offsetof(PyCodeObject, co_linetable), \
                .firstlineno = offsetof(PyCodeObject, co_firstlineno), \
                .argcount = offsetof(PyCodeObject, co_argcount), \
                .localsplusnames = offsetof(PyCodeObject, co_localsplusnames), \
                .localspluskinds = offsetof(PyCodeObject, co_localspluskinds), \
                .co_code_adaptive = offsetof(PyCodeObject, co_code_adaptive), \
            }, \
            .pyobject = { \
                .ob_type = offsetof(PyObject, ob_type), \
            }, \
            .type_object = { \
                .tp_name = offsetof(PyTypeObject, tp_name), \
            }, \
            .tuple_object = { \
                .ob_item = offsetof(PyTupleObject, ob_item), \
            }, \
        }, \
        .allocators = { \
            .standard = _pymem_allocators_standard_INIT(runtime), \
            .debug = _pymem_allocators_debug_INIT, \
            .obj_arena = _pymem_allocators_obj_arena_INIT, \
        }, \
        .obmalloc = _obmalloc_global_state_INIT, \
        .pyhash_state = pyhash_state_INIT, \
        .signals = _signals_RUNTIME_INIT, \
        .interpreters = { \
            /* This prevents interpreters from getting created \
              until _PyInterpreterState_Enable() is called. */ \
            .next_id = -1, \
        }, \
        .xi = { \
            .registry = { \
                .global = 1, \
            }, \
        }, \
        /* A TSS key must be initialized with Py_tss_NEEDS_INIT \
           in accordance with the specification. */ \
        .autoTSSkey = Py_tss_NEEDS_INIT, \
        .parser = _parser_runtime_state_INIT, \
        .ceval = { \
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
        .float_state = { \
            .float_format = _py_float_format_unknown, \
            .double_format = _py_float_format_unknown, \
        }, \
        .types = { \
            .next_version_tag = 1, \
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
        .imports = IMPORTS_INIT, \
        .obmalloc = _obmalloc_state_INIT(INTERP.obmalloc), \
        .ceval = { \
            .recursion_limit = Py_DEFAULT_RECURSION_LIMIT, \
        }, \
        .gc = { \
            .enabled = 1, \
            .generations = { \
                /* .head is set in _PyGC_InitState(). */ \
                { .threshold = 700, }, \
                { .threshold = 10, }, \
                { .threshold = 10, }, \
            }, \
        }, \
        .object_state = _py_object_state_INIT(INTERP), \
        .dtoa = _dtoa_state_INIT(&(INTERP)), \
        .dict_state = _dict_state_INIT, \
        .func_state = { \
            .next_version = 1, \
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
    }

#define _PyThreadState_INIT \
    { \
        ._whence = _PyThreadState_WHENCE_NOTSET, \
        .py_recursion_limit = Py_DEFAULT_RECURSION_LIMIT, \
        .context_ver = 1, \
    }

#ifdef Py_TRACE_REFS
# define _py_object_state_INIT(INTERP) \
    { \
        .refchain = NULL, \
    }
#else
# define _py_object_state_INIT(INTERP) \
    { 0 }
#endif


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

#include "pycore_runtime_init_generated.h"

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_RUNTIME_INIT_H */
