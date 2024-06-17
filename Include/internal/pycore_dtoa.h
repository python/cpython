#ifndef Py_INTERNAL_DTOA_H
#define Py_INTERNAL_DTOA_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_pymath.h"        // _PY_SHORT_FLOAT_REPR


#if _PY_SHORT_FLOAT_REPR == 1

typedef uint32_t ULong;

struct
Bigint {
    struct Bigint *next;
    int k, maxwds, sign, wds;
    ULong x[1];
};

#ifdef Py_USING_MEMORY_DEBUGGER

struct _dtoa_state {
    int _not_used;
};
#define _dtoa_interp_state_INIT(INTERP) \
    {0}

#else  // !Py_USING_MEMORY_DEBUGGER

/* The size of the Bigint freelist */
#define Bigint_Kmax 7

/* The size of the cached powers of 5 array */
#define Bigint_Pow5size 8

#ifndef PRIVATE_MEM
#define PRIVATE_MEM 2304
#endif
#define Bigint_PREALLOC_SIZE \
    ((PRIVATE_MEM+sizeof(double)-1)/sizeof(double))

struct _dtoa_state {
    // p5s is an array of powers of 5 of the form:
    // 5**(2**(i+2)) for 0 <= i < Bigint_Pow5size
    struct Bigint *p5s[Bigint_Pow5size];
    // XXX This should be freed during runtime fini.
    struct Bigint *freelist[Bigint_Kmax+1];
    double preallocated[Bigint_PREALLOC_SIZE];
    double *preallocated_next;
};
#define _dtoa_state_INIT(INTERP) \
    { \
        .preallocated_next = (INTERP)->dtoa.preallocated, \
    }

#endif  // !Py_USING_MEMORY_DEBUGGER


extern double _Py_dg_strtod(const char *str, char **ptr);
extern char* _Py_dg_dtoa(double d, int mode, int ndigits,
                         int *decpt, int *sign, char **rve);
extern void _Py_dg_freedtoa(char *s);

#endif // _PY_SHORT_FLOAT_REPR == 1


extern PyStatus _PyDtoa_Init(PyInterpreterState *interp);
extern void _PyDtoa_Fini(PyInterpreterState *interp);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_DTOA_H */
