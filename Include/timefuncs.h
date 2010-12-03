/*  timefuncs.h
 */

/* Utility function related to timemodule.c. */

#ifndef TIMEFUNCS_H
#define TIMEFUNCS_H
#ifdef __cplusplus
extern "C" {
#endif


/* Cast double x to time_t, but raise ValueError if x is too large
 * to fit in a time_t.  ValueError is set on return iff the return
 * value is (time_t)-1 and PyErr_Occurred().
 */
#ifndef Py_LIMITED_API
PyAPI_FUNC(time_t) _PyTime_DoubleToTimet(double x);
#endif


#ifdef __cplusplus
}
#endif
#endif  /* TIMEFUNCS_H */
