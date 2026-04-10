// _testclinic_limited can built with the Py_BUILD_CORE_BUILTIN macro defined
// if one of the Modules/Setup files asks to build it as "static" (gh-109723).
#undef Py_BUILD_CORE
#undef Py_BUILD_CORE_MODULE
#undef Py_BUILD_CORE_BUILTIN

// For now, AC only supports the limited C API version 3.13
#include "pyconfig.h"   // Py_GIL_DISABLED
#ifndef Py_GIL_DISABLED
#  define Py_LIMITED_API 0x030d0000
#endif

/* Always enable assertions */
#undef NDEBUG

#include "Python.h"


#include "clinic/_testclinic_limited.c.h"


/*[clinic input]
module  _testclinic_limited
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=dd408149a4fc0dbb]*/


/*[clinic input]
test_empty_function

[clinic start generated code]*/

static PyObject *
test_empty_function_impl(PyObject *module)
/*[clinic end generated code: output=0f8aeb3ddced55cb input=0dd7048651ad4ae4]*/
{
    Py_RETURN_NONE;
}


/*[clinic input]
my_int_func -> int

    arg: int
    /

[clinic start generated code]*/

static int
my_int_func_impl(PyObject *module, int arg)
/*[clinic end generated code: output=761cd54582f10e4f input=16eb8bba71d82740]*/
{
    return arg;
}


/*[clinic input]
my_int_sum -> int

    x: int
    y: int
    /

[clinic start generated code]*/

static int
my_int_sum_impl(PyObject *module, int x, int y)
/*[clinic end generated code: output=3e52db9ab5f37e2f input=0edb6796813bf2d3]*/
{
    return x + y;
}


/*[clinic input]
my_float_sum -> float

    x: float
    y: float
    /

[clinic start generated code]*/

static float
my_float_sum_impl(PyObject *module, float x, float y)
/*[clinic end generated code: output=634f59a5a419cad7 input=d4b5313bdf4dc377]*/
{
    return x + y;
}


/*[clinic input]
my_double_sum -> double

    x: double
    y: double
    /

[clinic start generated code]*/

static double
my_double_sum_impl(PyObject *module, double x, double y)
/*[clinic end generated code: output=a75576d9e4d8557f input=16b11c8aba172801]*/
{
    return x + y;
}


/*[clinic input]
get_file_descriptor -> int

    file as fd: fildes
    /

Get a file descriptor.
[clinic start generated code]*/

static int
get_file_descriptor_impl(PyObject *module, int fd)
/*[clinic end generated code: output=80051ebad54db8a8 input=82e2a1418848cd5b]*/
{
    return fd;
}


static PyMethodDef tester_methods[] = {
    TEST_EMPTY_FUNCTION_METHODDEF
    MY_INT_FUNC_METHODDEF
    MY_INT_SUM_METHODDEF
    MY_FLOAT_SUM_METHODDEF
    MY_DOUBLE_SUM_METHODDEF
    GET_FILE_DESCRIPTOR_METHODDEF
    {NULL, NULL}
};

static struct PyModuleDef _testclinic_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_testclinic_limited",
    .m_size = 0,
    .m_methods = tester_methods,
};

PyMODINIT_FUNC
PyInit__testclinic_limited(void)
{
    PyObject *m = PyModule_Create(&_testclinic_module);
    if (m == NULL) {
        return NULL;
    }
#ifdef Py_GIL_DISABLED
    PyUnstable_Module_SetGIL(m, Py_MOD_GIL_NOT_USED);
#endif
    return m;
}
