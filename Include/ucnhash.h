/* Unicode name database interface */

#ifndef Py_UCNHASH_H
#define Py_UCNHASH_H
#ifdef __cplusplus
extern "C" {
#endif

/* revised ucnhash CAPI interface (exported through a PyCObject) */

typedef struct {

    /* Size of this struct */
    int size;

    /* Get name for a given character code.  Returns non-zero if
       success, zero if not.  Does not set Python exceptions. */
    int (*getname)(Py_UCS4 code, char* buffer, int buflen);

    /* Get character code for a given name.  Same error handling
       as for getname. */
    int (*getcode)(const char* name, int namelen, Py_UCS4* code);

} _PyUnicode_Name_CAPI;

#ifdef __cplusplus
}
#endif
#endif /* !Py_UCNHASH_H */
