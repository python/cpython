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

/* Structure used by time.get_clock_info() */
typedef struct {
    const char *implementation;
    int monotonic;
    int adjustable;
    double resolution;
} _Py_clock_info_t;

/* Similar to POSIX gettimeofday but cannot fail.  If system gettimeofday
 * fails or is not available, fall back to lower resolution clocks.
 */
PyAPI_FUNC(void) _PyTime_gettimeofday(_PyTime_timeval *tp);

/* Similar to _PyTime_gettimeofday() but retrieve also information on the
 * clock used to get the current time. */
PyAPI_FUNC(void) _PyTime_gettimeofday_info(
    _PyTime_timeval *tp,
    _Py_clock_info_t *info);

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

typedef enum {
    /* Round towards zero. */
    _PyTime_ROUND_DOWN=0,
    /* Round away from zero. */
    _PyTime_ROUND_UP
} _PyTime_round_t;

/* Convert a number of seconds, int or float, to time_t. */
PyAPI_FUNC(int) _PyTime_ObjectToTime_t(
    PyObject *obj,
    time_t *sec,
    _PyTime_round_t);

/* Convert a time_t to a PyLong. */
PyAPI_FUNC(PyObject *) _PyLong_FromTime_t(
    time_t sec);

/* Convert a PyLong to a time_t. */
PyAPI_FUNC(time_t) _PyLong_AsTime_t(
    PyObject *obj);

/* Convert a number of seconds, int or float, to a timeval structure.
   usec is in the range [0; 999999] and rounded towards zero.
   For example, -1.2 is converted to (-2, 800000). */
PyAPI_FUNC(int) _PyTime_ObjectToTimeval(
    PyObject *obj,
    time_t *sec,
    long *usec,
    _PyTime_round_t);

/* Convert a number of seconds, int or float, to a timespec structure.
   nsec is in the range [0; 999999999] and rounded towards zero.
   For example, -1.2 is converted to (-2, 800000000). */
PyAPI_FUNC(int) _PyTime_ObjectToTimespec(
    PyObject *obj,
    time_t *sec,
    long *nsec,
    _PyTime_round_t);
#endif

/* Dummy to force linking. */
PyAPI_FUNC(void) _PyTime_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* Py_PYTIME_H */
#endif /* Py_LIMITED_API */
