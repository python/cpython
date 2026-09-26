#ifndef Py_INTERNAL_DTOA_H
#define Py_INTERNAL_DTOA_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_pymath.h"        // _PY_SHORT_FLOAT_REPR


#if defined(Py_USING_MEMORY_DEBUGGER) || _PY_SHORT_FLOAT_REPR == 0

#define _dtoa_state_INIT(INTERP) \
    {0}

#else

#define _dtoa_state_INIT(INTERP) \
    { \
        .preallocated_next = (INTERP)->dtoa.preallocated, \
    }
#endif

extern double _Py_dg_strtod(const char *str, char **ptr);
extern char* _Py_dg_dtoa(double d, int mode, int ndigits,
                         int *decpt, int *sign, char **rve);
extern void _Py_dg_freedtoa(char *s);


extern PyStatus _PyDtoa_Init(PyInterpreterState *interp);
extern void _PyDtoa_Fini(PyInterpreterState *interp);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_DTOA_H */
