#ifndef Py_PYTIME_H
#define Py_PYTIME_H

#include "pyconfig.h" /* include for defines */

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

/* Dummy to force linking. */
PyAPI_FUNC(void) _PyTime_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* Py_PYTIME_H */
#ifndef Py_PYTIME_H
#define Py_PYTIME_H

#include "pyconfig.h" /* include for defines */

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

/* Dummy to force linking. */
PyAPI_FUNC(void) _PyTime_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* Py_PYTIME_H */
