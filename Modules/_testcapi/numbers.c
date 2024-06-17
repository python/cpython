#include "parts.h"
#include "util.h"

static PyMethodDef test_methods[] = {
    {NULL},
};

int
_PyTestCapi_Init_Numbers(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
