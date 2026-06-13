// clinic/file.c.h uses internal pycore_modsupport.h API
#define PYTESTCAPI_NEED_INTERNAL_API

#include "parts.h"
#include "util.h"
#include "clinic/file.c.h"


/*[clinic input]
module _testcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6361033e795369fc]*/


/*[clinic input]
_testcapi.pyfile_newstdprinter

    fd: int
    /

[clinic start generated code]*/

static PyObject *
_testcapi_pyfile_newstdprinter_impl(PyObject *module, int fd)
/*[clinic end generated code: output=8a2d1c57b6892db3 input=442f1824142262ea]*/
{
    return PyFile_NewStdPrinter(fd);
}


/*[clinic input]
_testcapi.py_fopen

    path: object
    mode: str(zeroes=True, accept={robuffer, str, NoneType})
    /

Call Py_fopen(), fread(256) and Py_fclose(). Return read bytes.
[clinic start generated code]*/

static PyObject *
_testcapi_py_fopen_impl(PyObject *module, PyObject *path, const char *mode,
                        Py_ssize_t mode_length)
/*[clinic end generated code: output=69840d0cfd8b7fbb input=f3a579dd7eb60926]*/
{
    NULLABLE(path);
    FILE *fp = Py_fopen(path, mode);
    if (fp == NULL) {
        return NULL;
    }

    char buffer[256];
    size_t size = fread(buffer, 1, Py_ARRAY_LENGTH(buffer), fp);
    Py_fclose(fp);

    return PyBytes_FromStringAndSize(buffer, size);
}


/*[clinic input]
_testcapi.py_universalnewlinefgets

    file: object
    size: int
    /

Read a line from a file using Py_UniversalNewlineFgets.
[clinic start generated code]*/

static PyObject *
_testcapi_py_universalnewlinefgets_impl(PyObject *module, PyObject *file,
                                        int size)
/*[clinic end generated code: output=2ce1bc76c9dc871c input=02c236049d18569a]*/
{
    FILE *fp = Py_fopen(file, "rb");
    if (fp == NULL) {
        return NULL;
    }

    char *buf = (char *)PyMem_Malloc(size);
    if (buf == NULL) {
        Py_fclose(fp);
        return PyErr_NoMemory();
    }

    char *result = Py_UniversalNewlineFgets(buf, size, fp, NULL);
    if (result == NULL) {
        PyMem_Free(buf);
        Py_fclose(fp);
        Py_RETURN_NONE;
    }

    PyObject *line = PyBytes_FromString(result);
    PyMem_Free(buf);
    Py_fclose(fp);

    return line;
}

static PyMethodDef test_methods[] = {
    _TESTCAPI_PYFILE_NEWSTDPRINTER_METHODDEF
    _TESTCAPI_PY_FOPEN_METHODDEF
    _TESTCAPI_PY_UNIVERSALNEWLINEFGETS_METHODDEF
    {NULL},
};

int
_PyTestCapi_Init_File(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
