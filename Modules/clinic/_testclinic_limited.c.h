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

PyDoc_STRVAR(my_float_sum__doc__,
"my_float_sum($module, x, y, /)\n"
"--\n"
"\n");

#define MY_FLOAT_SUM_METHODDEF    \
    {"my_float_sum", (PyCFunction)(void(*)(void))my_float_sum, METH_FASTCALL, my_float_sum__doc__},

static float
my_float_sum_impl(PyObject *module, float x, float y);

static PyObject *
my_float_sum(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    float x;
    float y;
    float _return_value;

    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "my_float_sum expected 2 arguments, got %zd", nargs);
        goto exit;
    }
    x = (float) PyFloat_AsDouble(args[0]);
    if (x == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    y = (float) PyFloat_AsDouble(args[1]);
    if (y == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = my_float_sum_impl(module, x, y);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble((double)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(my_double_sum__doc__,
"my_double_sum($module, x, y, /)\n"
"--\n"
"\n");

#define MY_DOUBLE_SUM_METHODDEF    \
    {"my_double_sum", (PyCFunction)(void(*)(void))my_double_sum, METH_FASTCALL, my_double_sum__doc__},

static double
my_double_sum_impl(PyObject *module, double x, double y);

static PyObject *
my_double_sum(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    double x;
    double y;
    double _return_value;

    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "my_double_sum expected 2 arguments, got %zd", nargs);
        goto exit;
    }
    x = PyFloat_AsDouble(args[0]);
    if (x == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    y = PyFloat_AsDouble(args[1]);
    if (y == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = my_double_sum_impl(module, x, y);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(get_file_descriptor__doc__,
"get_file_descriptor($module, file, /)\n"
"--\n"
"\n"
"Get a file descriptor.");

#define GET_FILE_DESCRIPTOR_METHODDEF    \
    {"get_file_descriptor", (PyCFunction)get_file_descriptor, METH_O, get_file_descriptor__doc__},

static int
get_file_descriptor_impl(PyObject *module, int fd);

static PyObject *
get_file_descriptor(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;
    int _return_value;

    fd = PyObject_AsFileDescriptor(arg);
    if (fd < 0) {
        goto exit;
    }
    _return_value = get_file_descriptor_impl(module, fd);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=03fd7811c056dc74 input=a9049054013a1b77]*/
