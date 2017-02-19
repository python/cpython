/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(bisect_bisect_right__doc__,
"bisect_right($module, /, a, x, lo=0, hi=None)\n"
"--\n"
"\n"
"Return the index where to insert item x in list a, assuming a is sorted.\n"
"\n"
"The return value i is such that all e in a[:i] have e <= x, and all e in\n"
"a[i:] have e > x.  So if x already appears in the list, i points just\n"
"beyond the rightmost x already there.\n"
"\n"
"Optional args lo (default 0) and hi (default len(a)) bound the\n"
"slice of a to be searched.");

#define BISECT_BISECT_RIGHT_METHODDEF    \
    {"bisect_right", (PyCFunction)bisect_bisect_right, METH_FASTCALL, bisect_bisect_right__doc__},

static PyObject *
bisect_bisect_right_impl(PyObject *module, PyObject *a, PyObject *x,
                         Py_ssize_t lo, Py_ssize_t hi);

static PyObject *
bisect_bisect_right(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"a", "x", "lo", "hi", NULL};
    static _PyArg_Parser _parser = {"OO|nO&:bisect_right", _keywords, 0};
    PyObject *a;
    PyObject *x;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &a, &x, &lo, optional_ssize_t_converter, &hi)) {
        goto exit;
    }
    return_value = bisect_bisect_right_impl(module, a, x, lo, hi);

exit:
    return return_value;
}

PyDoc_STRVAR(bisect_insort_right__doc__,
"insort_right($module, /, a, x, lo=0, hi=None)\n"
"--\n"
"\n"
"Insert item x in list a, and keep it sorted assuming a is sorted.\n"
"\n"
"If x is already in a, insert it to the right of the rightmost x.\n"
"\n"
"Optional args lo (default 0) and hi (default len(a)) bound the\n"
"slice of a to be searched.");

#define BISECT_INSORT_RIGHT_METHODDEF    \
    {"insort_right", (PyCFunction)bisect_insort_right, METH_FASTCALL, bisect_insort_right__doc__},

static PyObject *
bisect_insort_right_impl(PyObject *module, PyObject *a, PyObject *x,
                         Py_ssize_t lo, Py_ssize_t hi);

static PyObject *
bisect_insort_right(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"a", "x", "lo", "hi", NULL};
    static _PyArg_Parser _parser = {"OO|nO&:insort_right", _keywords, 0};
    PyObject *a;
    PyObject *x;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &a, &x, &lo, optional_ssize_t_converter, &hi)) {
        goto exit;
    }
    return_value = bisect_insort_right_impl(module, a, x, lo, hi);

exit:
    return return_value;
}

PyDoc_STRVAR(bisect_bisect_left__doc__,
"bisect_left($module, /, a, x, lo=0, hi=None)\n"
"--\n"
"\n"
"Return the index where to insert item x in list a, assuming a is sorted.\n"
"\n"
"The return value i is such that all e in a[:i] have e < x, and all e in\n"
"a[i:] have e >= x.  So if x already appears in the list, i points just\n"
"before the leftmost x already there.\n"
"\n"
"Optional args lo (default 0) and hi (default len(a)) bound the\n"
"slice of a to be searched.");

#define BISECT_BISECT_LEFT_METHODDEF    \
    {"bisect_left", (PyCFunction)bisect_bisect_left, METH_FASTCALL, bisect_bisect_left__doc__},

static PyObject *
bisect_bisect_left_impl(PyObject *module, PyObject *a, PyObject *x,
                        Py_ssize_t lo, Py_ssize_t hi);

static PyObject *
bisect_bisect_left(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"a", "x", "lo", "hi", NULL};
    static _PyArg_Parser _parser = {"OO|nO&:bisect_left", _keywords, 0};
    PyObject *a;
    PyObject *x;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &a, &x, &lo, optional_ssize_t_converter, &hi)) {
        goto exit;
    }
    return_value = bisect_bisect_left_impl(module, a, x, lo, hi);

exit:
    return return_value;
}

PyDoc_STRVAR(bisect_insort_left__doc__,
"insort_left($module, /, a, x, lo=0, hi=None)\n"
"--\n"
"\n"
"Insert item x in list a, and keep it sorted assuming a is sorted.\n"
"\n"
"If x is already in a, insert it to the left of the leftmost x.\n"
"\n"
"Optional args lo (default 0) and hi (default len(a)) bound the\n"
"slice of a to be searched.");

#define BISECT_INSORT_LEFT_METHODDEF    \
    {"insort_left", (PyCFunction)bisect_insort_left, METH_FASTCALL, bisect_insort_left__doc__},

static PyObject *
bisect_insort_left_impl(PyObject *module, PyObject *a, PyObject *x,
                        Py_ssize_t lo, Py_ssize_t hi);

static PyObject *
bisect_insort_left(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"a", "x", "lo", "hi", NULL};
    static _PyArg_Parser _parser = {"OO|nO&:insort_left", _keywords, 0};
    PyObject *a;
    PyObject *x;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &a, &x, &lo, optional_ssize_t_converter, &hi)) {
        goto exit;
    }
    return_value = bisect_insort_left_impl(module, a, x, lo, hi);

exit:
    return return_value;
}
/*[clinic end generated code: output=f21a122da59705d0 input=a9049054013a1b77]*/
