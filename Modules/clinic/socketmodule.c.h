/*[clinic input]
preserve
[clinic start generated code]*/

static int
sock_initobj_impl(PySocketSockObject *self, int family, int type, int proto,
                  PyObject *fdobj);

static int
sock_initobj(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"family", "type", "proto", "fileno", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "socket", 0};
    PyObject *argsbuf[4];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    int family = -1;
    int type = -1;
    int proto = -1;
    PyObject *fdobj = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 4, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        family = _PyLong_AsInt(fastargs[0]);
        if (family == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[1]) {
        type = _PyLong_AsInt(fastargs[1]);
        if (type == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[2]) {
        proto = _PyLong_AsInt(fastargs[2]);
        if (proto == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    fdobj = fastargs[3];
skip_optional_pos:
    return_value = sock_initobj_impl((PySocketSockObject *)self, family, type, proto, fdobj);

exit:
    return return_value;
}
/*[clinic end generated code: output=2433d6ac51bc962a input=a9049054013a1b77]*/
