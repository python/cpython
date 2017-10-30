/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(itertools_groupby__doc__,
"groupby(iterable, key=None)\n"
"--\n"
"\n"
"make an iterator that returns consecutive keys and groups from the iterable\n"
"\n"
"  iterable\n"
"    Elements to divide into groups according to the key function.\n"
"  key\n"
"    A function computing a key value for each element.  If None, key\n"
"    defaults to an identity function and returns the element unchanged.");

static PyObject *
itertools_groupby_impl(PyTypeObject *type, PyObject *iterable, PyObject *key);

static PyObject *
itertools_groupby(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"iterable", "key", NULL};
    static _PyArg_Parser _parser = {"O|O:groupby", _keywords, 0};
    PyObject *iterable;
    PyObject *key = Py_None;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &iterable, &key)) {
        goto exit;
    }
    return_value = itertools_groupby_impl(type, iterable, key);

exit:
    return return_value;
}

static PyObject *
itertools__grouper_impl(PyTypeObject *type, PyObject *parent,
                        PyObject *tgtkey);

static PyObject *
itertools__grouper(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *parent;
    PyObject *tgtkey;

    if ((type == &_grouper_type) &&
        !_PyArg_NoKeywords("_grouper", kwargs)) {
        goto exit;
    }
    if (!PyArg_ParseTuple(args, "O!O:_grouper",
        &groupby_type, &parent, &tgtkey)) {
        goto exit;
    }
    return_value = itertools__grouper_impl(type, parent, tgtkey);

exit:
    return return_value;
}
/*[clinic end generated code: output=ecc325ee9ecf0cf6 input=a9049054013a1b77]*/
