#ifndef Py_INTERNAL_DTOA_H
#define Py_INTERNAL_DTOA_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_pymath.h"        // _PY_SHORT_FLOAT_REPR


typedef uint32_t ULong;

struct
Bigint {
    struct Bigint *next;
    int k, maxwds, sign, wds;
    ULong x[1];
};

#if defined(Py_USING_MEMORY_DEBUGGER) || _PY_SHORT_FLOAT_REPR == 0

struct _dtoa_state {
    int _not_used;
};
#define _dtoa_state_INIT(INTERP) \
    {0}

#else  // !Py_USING_MEMORY_DEBUGGER && _PY_SHORT_FLOAT_REPR != 0

/* The size of the Bigint freelist */
#define Bigint_Kmax 7

#ifndef PRIVATE_MEM
#define PRIVATE_MEM 2304
#endif
#define Bigint_PREALLOC_SIZE \
    ((PRIVATE_MEM+sizeof(double)-1)/sizeof(double))

struct _dtoa_state {
    /* p5s is a linked list of powers of 5 of the form 5**(2**i), i >= 2 */
    // XXX This should be freed during runtime fini.
    struct Bigint *p5s;
    struct Bigint *freelist[Bigint_Kmax+1];
    double preallocated[Bigint_PREALLOC_SIZE];
    double *preallocated_next;
};
#define _dtoa_state_INIT(INTERP) \
    { \
        .preallocated_next = (INTERP)->dtoa.preallocated, \
    }

#endif  // !Py_USING_MEMORY_DEBUGGER


/* These functions are used by modules compiled as C extension like math:
   they must be exported. */

PyAPI_FUNC(double) _Py_dg_strtod(const char *str, char **ptr);
PyAPI_FUNC(char *) _Py_dg_dtoa(double d, int mode, int ndigits,
                        int *decpt, int *sign, char **rve);
PyAPI_FUNC(void) _Py_dg_freedtoa(char *s);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_DTOA_H */
