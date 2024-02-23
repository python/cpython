#include "parts.h"
#include "util.h"


static PyMethodDef test_methods[] = {
    {NULL},
};

int
_PyTestCapi_Init_Codec(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0){
        return -1;
    }

    return 0;
}
