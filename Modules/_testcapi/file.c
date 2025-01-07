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
_testcapi.py_fopen

    path: object
    mode: str
    /

Call Py_fopen(), fread(256) and Py_fclose(). Return read bytes.
[clinic start generated code]*/

static PyObject *
_testcapi_py_fopen_impl(PyObject *module, PyObject *path, const char *mode)
/*[clinic end generated code: output=5a900af000f759de input=d7e7b8f0fd151953]*/
{
    FILE *fp = Py_fopen(path, mode);
    if (fp == NULL) {
        return NULL;
    }

    char buffer[256];
    size_t size = fread(buffer, 1, Py_ARRAY_LENGTH(buffer), fp);
    Py_fclose(fp);

    return PyBytes_FromStringAndSize(buffer, size);
}

static PyMethodDef test_methods[] = {
    _TESTCAPI_PY_FOPEN_METHODDEF
    {NULL},
};

int
_PyTestCapi_Init_File(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0){
        return -1;
    }

    return 0;
}
