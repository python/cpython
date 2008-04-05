
#ifndef Py_PYGETOPT_H
#define Py_PYGETOPT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(int) _PyOS_opterr;
PyAPI_DATA(int) _PyOS_optind;
PyAPI_DATA(wchar_t *) _PyOS_optarg;

PyAPI_FUNC(int) _PyOS_GetOpt(int argc, wchar_t **argv, wchar_t *optstring);

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYGETOPT_H */
