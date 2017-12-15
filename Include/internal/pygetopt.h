#ifndef Py_INTERNAL_PYGETOPT_H
#define Py_INTERNAL_PYGETOPT_H

int _PyOS_opterr;
int _PyOS_optind;
wchar_t *_PyOS_optarg;

void _PyOS_ResetGetOpt(void);

typedef struct {
    const wchar_t *name;
    int has_arg;
    int val;
} _PyOS_LongOption;

int _PyOS_GetOpt(int argc, wchar_t **argv, wchar_t *optstring,
                 const _PyOS_LongOption *longopts, int *longindex);

#endif /* !Py_INTERNAL_PYGETOPT_H */
