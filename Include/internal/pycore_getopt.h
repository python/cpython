#ifndef Py_INTERNAL_PYGETOPT_H
#define Py_INTERNAL_PYGETOPT_H

#if !defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_BUILTIN)
#  error "this header requires Py_BUILD_CORE or Py_BUILD_CORE_BUILTIN define"
#endif

extern int _PyOS_opterr;
extern int _PyOS_optind;
extern wchar_t *_PyOS_optarg;

extern void _PyOS_ResetGetOpt(void);

typedef struct {
    const wchar_t *name;
    int has_arg;
    int val;
} _PyOS_LongOption;

extern int _PyOS_GetOpt(int argc, wchar_t **argv, wchar_t *optstring,
                        const _PyOS_LongOption *longopts, int *longindex);

#endif /* !Py_INTERNAL_PYGETOPT_H */
