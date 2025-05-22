#ifndef Py_INTERNAL_GIL_H
#define Py_INTERNAL_GIL_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_condvar.h"       // PyCOND_T

#ifndef Py_HAVE_CONDVAR
#  error You need either a POSIX-compatible or a Windows system!
#endif

/* Enable if you want to force the switching of threads at least
   every `interval`. */
#undef FORCE_SWITCHING
#define FORCE_SWITCHING

struct _gil_runtime_state {
#ifdef Py_GIL_DISABLED
    /* If this GIL is disabled, enabled == 0.

       If this GIL is enabled transiently (most likely to initialize a module
       of unknown safety), enabled indicates the number of active transient
       requests.

       If this GIL is enabled permanently, enabled == INT_MAX.

       It must not be modified directly; use _PyEval_EnableGILTransiently(),
       _PyEval_EnableGILPermanently(), and _PyEval_DisableGIL()

       It is always read and written atomically, but a thread can assume its
       value will be stable as long as that thread is attached or knows that no
       other threads are attached (e.g., during a stop-the-world.). */
    int enabled;
#endif
    /* microseconds (the Python API uses seconds, though) */
    unsigned long interval;
    /* Last PyThreadState holding / having held the GIL. This helps us
       know whether anyone else was scheduled after we dropped the GIL. */
    PyThreadState* last_holder;
    /* Whether the GIL is already taken (-1 if uninitialized). This is
       atomic because it can be read without any lock taken in ceval.c. */
    int locked;
    /* Number of GIL switches since the beginning. */
    unsigned long switch_number;
    /* This condition variable allows one or several threads to wait
       until the GIL is released. In addition, the mutex also protects
       the above variables. */
    PyCOND_T cond;
    PyMUTEX_T mutex;
#ifdef FORCE_SWITCHING
    /* This condition variable helps the GIL-releasing thread wait for
       a GIL-awaiting thread to be scheduled and take the GIL. */
    PyCOND_T switch_cond;
    PyMUTEX_T switch_mutex;
#endif
};

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GIL_H */
