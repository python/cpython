
#ifndef Py_PYGETOPT_H
#define Py_PYGETOPT_H
#ifdef __cplusplus
extern "C" {
#endif

extern DL_IMPORT(int) _PyOS_opterr;
extern DL_IMPORT(int) _PyOS_optind;
extern DL_IMPORT(char *) _PyOS_optarg;

DL_IMPORT(int) _PyOS_GetOpt(int argc, char **argv, char *optstring);

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYGETOPT_H */
