#include "parts.h"
#include "util.h"


static PyObject *
test_file_from_fd(PyObject *self, PyObject *args)
{

}

static PyMethodDef test_methods[] = {
    {"", test_file_from_fd, METH_VARARGS},
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
