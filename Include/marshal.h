
/* Interface for marshal.c */

#ifndef Py_MARSHAL_H
#define Py_MARSHAL_H
#ifdef __cplusplus
extern "C" {
#endif

DL_IMPORT(void) PyMarshal_WriteLongToFile(long, FILE *);
DL_IMPORT(void) PyMarshal_WriteShortToFile(int, FILE *);
DL_IMPORT(void) PyMarshal_WriteObjectToFile(PyObject *, FILE *);
DL_IMPORT(PyObject *) PyMarshal_WriteObjectToString(PyObject *);

DL_IMPORT(long) PyMarshal_ReadLongFromFile(FILE *);
DL_IMPORT(int) PyMarshal_ReadShortFromFile(FILE *);
DL_IMPORT(PyObject *) PyMarshal_ReadObjectFromFile(FILE *);
DL_IMPORT(PyObject *) PyMarshal_ReadObjectFromString(char *, int);

#ifdef __cplusplus
}
#endif
#endif /* !Py_MARSHAL_H */
