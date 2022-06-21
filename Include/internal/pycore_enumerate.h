#ifndef Py_INTERNAL_ENUMERATE_H
#define Py_INTERNAL_ENUMERATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef struct {
    PyObject_HEAD
    Py_ssize_t en_index;           /* current index of enumeration */
    PyObject* en_sit;              /* secondary iterator of enumeration */
    PyObject* en_result;           /* result tuple  */
    PyObject* en_longindex;        /* index for sequences >= PY_SSIZE_T_MAX */
    PyObject* one;                 /* borrowed reference */
} _PyEnumObject;

int
_PyEnum_GetNext(_PyEnumObject *en, PyObject **stack, PyObject **index_target);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_ENUMERATE_H */
