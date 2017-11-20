#ifndef Py_ZIPIMPORT_H
#define Py_ZIPIMPORT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) PyZipImporter_Type;

#define ZipImporter_Check(op) PyObject_TypeCheck(op, &PyZipImporter_Type)

#ifdef __cplusplus
}
#endif
#endif /* !Py_ZIPIMPORT_H */
