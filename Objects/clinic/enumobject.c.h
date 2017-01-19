/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(enum_new__doc__,
"enumerate(iterable, start=0)\n"
"--\n"
"\n"
"Return an enumerate object.\n"
"\n"
"  iterable\n"
"    an object supporting iteration\n"
"\n"
"The enumerate object yields pairs containing a count (from start, which\n"
"defaults to zero) and a value yielded by the iterable argument.\n"
"\n"
"enumerate is useful for obtaining an indexed list:\n"
"    (0, seq[0]), (1, seq[1]), (2, seq[2]), ...");

static PyObject *
enum_new_impl(PyTypeObject *type, PyObject *iterable, PyObject *start);

static PyObject *
enum_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"iterable", "start", NULL};
    static _PyArg_Parser _parser = {"O|O:enumerate", _keywords, 0};
    PyObject *iterable;
    PyObject *start = 0;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &iterable, &start)) {
        goto exit;
    }
    return_value = enum_new_impl(type, iterable, start);

exit:
    return return_value;
}

PyDoc_STRVAR(reversed_new__doc__,
"reversed(sequence, /)\n"
"--\n"
"\n"
"Return a reverse iterator over the values of the given sequence.");

static PyObject *
reversed_new_impl(PyTypeObject *type, PyObject *seq);

static PyObject *
reversed_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *seq;

    if ((type == &PyReversed_Type) &&
        !_PyArg_NoKeywords("reversed", kwargs)) {
        goto exit;
    }
    if (!PyArg_UnpackTuple(args, "reversed",
        1, 1,
        &seq)) {
        goto exit;
    }
    return_value = reversed_new_impl(type, seq);

exit:
    return return_value;
}
/*[clinic end generated code: output=9008c36999c57218 input=a9049054013a1b77]*/
