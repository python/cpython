/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(test_empty_function__doc__,
"test_empty_function($module, /)\n"
"--\n"
"\n");

#define TEST_EMPTY_FUNCTION_METHODDEF    \
    {"test_empty_function", (PyCFunction)test_empty_function, METH_NOARGS, test_empty_function__doc__},

static PyObject *
test_empty_function_impl(PyObject *module);

static PyObject *
test_empty_function(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return test_empty_function_impl(module);
}

PyDoc_STRVAR(my_int_func__doc__,
"my_int_func($module, arg, /)\n"
"--\n"
"\n");

#define MY_INT_FUNC_METHODDEF    \
    {"my_int_func", (PyCFunction)my_int_func, METH_O, my_int_func__doc__},

static int
my_int_func_impl(PyObject *module, int arg);

static PyObject *
my_int_func(PyObject *module, PyObject *arg_)
{
    PyObject *return_value = NULL;
    int arg;
    int _return_value;

    arg = PyLong_AsInt(arg_);
    if (arg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = my_int_func_impl(module, arg);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(my_int_sum__doc__,
"my_int_sum($module, x, y, /)\n"
"--\n"
"\n");

#define MY_INT_SUM_METHODDEF    \
    {"my_int_sum", (PyCFunction)(void(*)(void))my_int_sum, METH_FASTCALL, my_int_sum__doc__},

static int
my_int_sum_impl(PyObject *module, int x, int y);

static PyObject *
my_int_sum(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int x;
    int y;
    int _return_value;

    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "my_int_sum expected 2 arguments, got %zd", nargs);
        goto exit;
    }
    x = PyLong_AsInt(args[0]);
    if (x == -1 && PyErr_Occurred()) {
        goto exit;
    }
    y = PyLong_AsInt(args[1]);
    if (y == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = my_int_sum_impl(module, x, y);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=5cf64baf978d2288 input=a9049054013a1b77]*/
