
#ifndef Py_PYGETOPT_H
#define Py_PYGETOPT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_LIMITED_API
PyAPI_DATA(int) _PyOS_opterr;
PyAPI_DATA(int) _PyOS_optind;
PyAPI_DATA(wchar_t *) _PyOS_optarg;

PyAPI_FUNC(void) _PyOS_ResetGetOpt(void);

typedef struct {
    const wchar_t *name;
    int has_arg;
    int val;
} _PyOS_LongOption;

PyAPI_FUNC(int) _PyOS_GetOpt(int argc, wchar_t **argv, wchar_t *optstring,
                             const _PyOS_LongOption *longopts, int *longindex);
#endif /* !Py_LIMITED_API */

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYGETOPT_H */
