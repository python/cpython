/* Static DTrace probes interface */

#ifndef Py_DTRACE_H
#define Py_DTRACE_H

#ifdef WITH_DTRACE

#include "pydtrace_probes.h"

/* pydtrace_probes.h, on systems with DTrace, is auto-generated to include
   `PyDTrace_{PROBE}` and `PyDTrace_{PROBE}_ENABLED()` macros for every probe
   defined in pydtrace_provider.d.

   Calling these functions must be guarded by a `PyDTrace_{PROBE}_ENABLED()`
   check to minimize performance impact when probing is off. For example:

       if (PyDTrace_FUNCTION_ENTRY_ENABLED())
           PyDTrace_FUNCTION_ENTRY(f);
*/

#else

/* Without DTrace, compile to nothing. */

#define PyDTrace_LINE(arg0, arg1, arg2) do ; while (0)
#define PyDTrace_FUNCTION_ENTRY(arg0, arg1, arg2)  do ; while (0)
#define PyDTrace_FUNCTION_RETURN(arg0, arg1, arg2) do ; while (0)
#define PyDTrace_GC_START(arg0)               do ; while (0)
#define PyDTrace_GC_DONE(arg0)                do ; while (0)
#define PyDTrace_INSTANCE_NEW_START(arg0)     do ; while (0)
#define PyDTrace_INSTANCE_NEW_DONE(arg0)      do ; while (0)
#define PyDTrace_INSTANCE_DELETE_START(arg0)  do ; while (0)
#define PyDTrace_INSTANCE_DELETE_DONE(arg0)   do ; while (0)

#define PyDTrace_LINE_ENABLED()                  (0)
#define PyDTrace_FUNCTION_ENTRY_ENABLED()        (0)
#define PyDTrace_FUNCTION_RETURN_ENABLED()       (0)
#define PyDTrace_GC_START_ENABLED()              (0)
#define PyDTrace_GC_DONE_ENABLED()               (0)
#define PyDTrace_INSTANCE_NEW_START_ENABLED()    (0)
#define PyDTrace_INSTANCE_NEW_DONE_ENABLED()     (0)
#define PyDTrace_INSTANCE_DELETE_START_ENABLED() (0)
#define PyDTrace_INSTANCE_DELETE_DONE_ENABLED()  (0)

#endif /* !WITH_DTRACE */

#endif /* !Py_DTRACE_H */
