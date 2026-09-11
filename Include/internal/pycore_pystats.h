#ifndef Py_INTERNAL_PYSTATS_H
#define Py_INTERNAL_PYSTATS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef Py_STATS
extern int _Py_StatsOn(void);
extern void _Py_StatsOff(void);
extern void _Py_StatsClear(void);
extern int _Py_PrintSpecializationStats(int to_file);
#endif

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_PYSTATS_H
