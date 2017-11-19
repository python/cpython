#ifndef Py_ZIPIMPORT_H
#define Py_ZIPIMPORT_H
#ifdef __cplusplus
extern "C" {
#endif

/* zipimporter object definition and support */

typedef struct _zipimporter ZipImporter;

struct _zipimporter {
    PyObject_HEAD
    PyObject *archive;  /* pathname of the Zip archive,
                           decoded from the filesystem encoding */
    PyObject *prefix;   /* file prefix: "a/sub/directory/",
                           encoded to the filesystem encoding */
    PyObject *files;    /* dict with file info {path: toc_entry} */
};

static PyTypeObject ZipImporter_Type;

#define ZipImporter_Check(op) PyObject_TypeCheck(op, &ZipImporter_Type)

#ifdef __cplusplus
}
#endif
#endif /* !Py_ZIPIMPORT_H */