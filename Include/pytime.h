#ifndef Py_LIMITED_API
#ifndef Py_PYTIME_H
#define Py_PYTIME_H

#include "pyconfig.h" /* include for defines */
#include "object.h"

/**************************************************************************
Symbols and macros to supply platform-independent interfaces to time related
functions and constants
**************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_GETTIMEOFDAY
typedef struct timeval _PyTime_timeval;
#else
typedef struct {
    time_t       tv_sec;   /* seconds since Jan. 1, 1970 */
    long         tv_usec;  /* and microseconds */
} _PyTime_timeval;
#endif

/* Similar to POSIX gettimeofday but cannot fail.  If system gettimeofday
 * fails or is not available, fall back to lower resolution clocks.
 */
PyAPI_FUNC(void) _PyTime_gettimeofday(_PyTime_timeval *tp);

#define _PyTime_ADD_SECONDS(tv, interval) \
do { \
    tv.tv_usec += (long) (((long) interval - interval) * 1000000); \
    tv.tv_sec += (time_t) interval + (time_t) (tv.tv_usec / 1000000); \
    tv.tv_usec %= 1000000; \
} while (0)

#define _PyTime_INTERVAL(tv_start, tv_end) \
    ((tv_end.tv_sec - tv_start.tv_sec) + \
     (tv_end.tv_usec - tv_start.tv_usec) * 0.000001)

#ifndef Py_LIMITED_API
/* Convert a number of seconds, int or float, to a timespec structure.
   nsec is always in the range [0; 999999999]. For example, -1.2 is converted
   to (-2, 800000000). */
PyAPI_FUNC(int) _PyTime_ObjectToTimespec(
    PyObject *obj,
    time_t *sec,
    long *nsec);
#endif

/* Dummy to force linking. */
PyAPI_FUNC(void) _PyTime_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* Py_PYTIME_H */
#endif /* Py_LIMITED_API */
