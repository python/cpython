// Statistics on Python performance (public API).
//
// Define _Py_INCREF_STAT_INC() and _Py_DECREF_STAT_INC() used by Py_INCREF()
// and Py_DECREF().
//
// See Include/cpython/pystats.h for the full API.

#ifndef Py_PYSTATS_H
#define Py_PYSTATS_H
#ifdef __cplusplus
extern "C" {
#endif

#if defined(Py_STATS) && !defined(Py_LIMITED_API)
#  define Py_CPYTHON_PYSTATS_H
#  include "cpython/pystats.h"
#  undef Py_CPYTHON_PYSTATS_H
#else
#  define _Py_INCREF_STAT_INC() ((void)0)
#  define _Py_DECREF_STAT_INC() ((void)0)
#endif  // !Py_STATS

#ifdef __cplusplus
}
#endif
#endif   // !Py_PYSTATS_H
