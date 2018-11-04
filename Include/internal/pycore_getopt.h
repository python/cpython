#ifndef Py_INTERNAL_PYGETOPT_H
#define Py_INTERNAL_PYGETOPT_H

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
