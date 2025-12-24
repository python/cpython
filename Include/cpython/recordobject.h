#ifndef Py_CPYTHON_RECORDOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct {
    PyObject_VAR_HEAD
    PyObject *names;
    /* ob_item contains space for 'ob_size' elements.
       Items must normally not be NULL, except during construction when
       the record is not yet visible outside the function that builds it. */
    PyObject *ob_item[1];
} PyRecordObject;
