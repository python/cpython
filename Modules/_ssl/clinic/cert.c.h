/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_ssl_Certificate_public_bytes__doc__,
"public_bytes($self, /, format=Encoding.PEM)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_PUBLIC_BYTES_METHODDEF    \
    {"public_bytes", _PyCFunction_CAST(_ssl_Certificate_public_bytes), METH_FASTCALL|METH_KEYWORDS, _ssl_Certificate_public_bytes__doc__},

static PyObject *
_ssl_Certificate_public_bytes_impl(PySSLCertificate *self, int format);

static PyObject *
_ssl_Certificate_public_bytes(PySSLCertificate *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"format", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "public_bytes", 0};
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int format = PY_SSL_ENCODING_PEM;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    format = _PyLong_AsInt(args[0]);
    if (format == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = _ssl_Certificate_public_bytes_impl(self, format);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_Certificate_get_info__doc__,
"get_info($self, /)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_GET_INFO_METHODDEF    \
    {"get_info", (PyCFunction)_ssl_Certificate_get_info, METH_NOARGS, _ssl_Certificate_get_info__doc__},

static PyObject *
_ssl_Certificate_get_info_impl(PySSLCertificate *self);

static PyObject *
_ssl_Certificate_get_info(PySSLCertificate *self, PyObject *Py_UNUSED(ignored))
{
    return _ssl_Certificate_get_info_impl(self);
}
/*[clinic end generated code: output=18885c4d167d5244 input=a9049054013a1b77]*/
