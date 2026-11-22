#ifndef Py_INTERNAL_PYGETOPT_H
#define Py_INTERNAL_PYGETOPT_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

struct _PyOS_GetOpt {
    int error;                    // generate error messages
    Py_ssize_t index;             // index into argv array
    const wchar_t *arg;           // optional argument
    const wchar_t *ptr;
    Py_ssize_t argc;
    wchar_t * const *argv;
};

extern void _PyOS_GetOpt_Init(
    struct _PyOS_GetOpt *getopt,
    Py_ssize_t argc,
    wchar_t * const *argv);

extern int _PyOS_GetOpt(
    struct _PyOS_GetOpt *getopt);

#endif /* !Py_INTERNAL_PYGETOPT_H */
