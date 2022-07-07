#ifndef Py_INTERNAL_RUNTIME_INIT_H
#define Py_INTERNAL_RUNTIME_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_object.h"


/* The static initializers defined here should only be used
   in the runtime init code (in pystate.c and pylifecycle.c). */


#define _PyRuntimeState_INIT \
    { \
        .gilstate = { \
            .check_enabled = 1, \
            /* A TSS key must be initialized with Py_tss_NEEDS_INIT \
               in accordance with the specification. */ \
            .autoTSSkey = Py_tss_NEEDS_INIT, \
        }, \
        .interpreters = { \
            /* This prevents interpreters from getting created \
              until _PyInterpreterState_Enable() is called. */ \
            .next_id = -1, \
        }, \
        .global_objects = _Py_global_objects_INIT, \
        ._main_interpreter = _PyInterpreterState_INIT, \
    }

#ifdef HAVE_DLOPEN
#  include <dlfcn.h>
#  if HAVE_DECL_RTLD_NOW
#    define _Py_DLOPEN_FLAGS RTLD_NOW
#  else
#    define _Py_DLOPEN_FLAGS RTLD_LAZY
#  endif
#  define DLOPENFLAGS_INIT .dlopenflags = _Py_DLOPEN_FLAGS,
#else
#  define _Py_DLOPEN_FLAGS 0
#  define DLOPENFLAGS_INIT
#endif

#define _PyInterpreterState_INIT \
    { \
        ._static = 1, \
        .id_refcount = -1, \
        DLOPENFLAGS_INIT \
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
        ._initial_thread = _PyThreadState_INIT, \
    }

#define _PyThreadState_INIT \
    { \
        ._static = 1, \
        .recursion_limit = Py_DEFAULT_RECURSION_LIMIT, \
        .context_ver = 1, \
    }


// global objects

#define _PyLong_DIGIT_INIT(val) \
    { \
        _PyVarObject_IMMORTAL_INIT(&PyLong_Type, \
                                   ((val) == 0 ? 0 : ((val) > 0 ? 1 : -1))), \
        .ob_digit = { ((val) >= 0 ? (val) : -(val)) }, \
    }

#define _PyBytes_SIMPLE_INIT(CH, LEN) \
    { \
        _PyVarObject_IMMORTAL_INIT(&PyBytes_Type, LEN), \
        .ob_shash = -1, \
        .ob_sval = { CH }, \
    }
#define _PyBytes_CHAR_INIT(CH) \
    { \
        _PyBytes_SIMPLE_INIT(CH, 1) \
    }

#define _PyUnicode_ASCII_BASE_INIT(LITERAL, ASCII) \
    { \
        .ob_base = _PyObject_IMMORTAL_INIT(&PyUnicode_Type), \
        .length = sizeof(LITERAL) - 1, \
        .hash = -1, \
        .state = { \
            .kind = 1, \
            .compact = 1, \
            .ascii = ASCII, \
        }, \
    }
#define _PyASCIIObject_INIT(LITERAL) \
    { \
        ._ascii = _PyUnicode_ASCII_BASE_INIT(LITERAL, 1), \
        ._data = LITERAL \
    }
#define INIT_STR(NAME, LITERAL) \
    ._ ## NAME = _PyASCIIObject_INIT(LITERAL)
#define INIT_ID(NAME) \
    ._ ## NAME = _PyASCIIObject_INIT(#NAME)
#define _PyUnicode_LATIN1_INIT(LITERAL) \
    { \
        ._latin1 = { \
            ._base = _PyUnicode_ASCII_BASE_INIT(LITERAL, 0), \
        }, \
        ._data = LITERAL, \
    }

#include "pycore_runtime_init_generated.h"

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_RUNTIME_INIT_H */
