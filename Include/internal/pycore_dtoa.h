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

#ifndef Py_USING_MEMORY_DEBUGGER
/* The size of the Bigint freelist */
# define Bigint_Kmax 7
#endif


/* These functions are used by modules compiled as C extension like math:
   they must be exported. */

PyAPI_FUNC(double) _Py_dg_strtod(const char *str, char **ptr);
PyAPI_FUNC(char *) _Py_dg_dtoa(double d, int mode, int ndigits,
                        int *decpt, int *sign, char **rve);
PyAPI_FUNC(void) _Py_dg_freedtoa(char *s);
PyAPI_FUNC(double) _Py_dg_stdnan(int sign);
PyAPI_FUNC(double) _Py_dg_infinity(int sign);

#endif // _PY_SHORT_FLOAT_REPR == 1

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_DTOA_H */
