
/* Interface for marshal.c */

#ifndef Py_MARSHAL_H
#define Py_MARSHAL_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(void) PyMarshal_WriteLongToFile(long, FILE *);
PyAPI_FUNC(void) PyMarshal_WriteObjectToFile(PyObject *, FILE *);
PyAPI_FUNC(PyObject *) PyMarshal_WriteObjectToString(PyObject *);

PyAPI_FUNC(long) PyMarshal_ReadLongFromFile(FILE *);
PyAPI_FUNC(int) PyMarshal_ReadShortFromFile(FILE *);
PyAPI_FUNC(PyObject *) PyMarshal_ReadObjectFromFile(FILE *);
PyAPI_FUNC(PyObject *) PyMarshal_ReadLastObjectFromFile(FILE *);
PyAPI_FUNC(PyObject *) PyMarshal_ReadObjectFromString(char *, int);

#ifdef __cplusplus
}
#endif
#endif /* !Py_MARSHAL_H */
