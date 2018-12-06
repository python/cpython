/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(tb_new__doc__,
"TracebackType(tb_next, tb_frame, tb_lasti, tb_lineno)\n"
"--\n"
"\n"
"Create a new traceback object.");

static PyObject *
tb_new_impl(PyTypeObject *type, PyObject *tb_next, PyFrameObject *tb_frame,
            int tb_lasti, int tb_lineno);

static PyObject *
tb_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"tb_next", "tb_frame", "tb_lasti", "tb_lineno", NULL};
    static _PyArg_Parser _parser = {"OO!ii:TracebackType", _keywords, 0};
    PyObject *tb_next;
    PyFrameObject *tb_frame;
    int tb_lasti;
    int tb_lineno;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &tb_next, &PyFrame_Type, &tb_frame, &tb_lasti, &tb_lineno)) {
        goto exit;
    }
    return_value = tb_new_impl(type, tb_next, tb_frame, tb_lasti, tb_lineno);

exit:
    return return_value;
}
/*[clinic end generated code: output=0133130d7d19556f input=a9049054013a1b77]*/
