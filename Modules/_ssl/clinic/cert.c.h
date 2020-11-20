/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_ssl_Certificate_from_file__doc__,
"from_file($type, /, path, *, password=None, format=FILETYPE_PEM)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_FROM_FILE_METHODDEF    \
    {"from_file", (PyCFunction)(void(*)(void))_ssl_Certificate_from_file, METH_FASTCALL|METH_KEYWORDS|METH_CLASS, _ssl_Certificate_from_file__doc__},

static PyObject *
_ssl_Certificate_from_file_impl(PyTypeObject *type, PyObject *path,
                                PyObject *password, int format);

static PyObject *
_ssl_Certificate_from_file(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "password", "format", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "from_file", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *password = Py_None;
    int format = PY_SSL_FILETYPE_PEM;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_FSConverter(args[0], &path)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        password = args[1];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    format = _PyLong_AsInt(args[2]);
    if (format == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _ssl_Certificate_from_file_impl(type, path, password, format);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_Certificate_from_buffer__doc__,
"from_buffer($type, /, buffer, *, format=FILETYPE_PEM)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_FROM_BUFFER_METHODDEF    \
    {"from_buffer", (PyCFunction)(void(*)(void))_ssl_Certificate_from_buffer, METH_FASTCALL|METH_KEYWORDS|METH_CLASS, _ssl_Certificate_from_buffer__doc__},

static PyObject *
_ssl_Certificate_from_buffer_impl(PyTypeObject *type, Py_buffer *buffer,
                                  int format);

static PyObject *
_ssl_Certificate_from_buffer(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"buffer", "format", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "from_buffer", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer buffer = {NULL, NULL};
    int format = PY_SSL_FILETYPE_PEM;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("from_buffer", "argument 'buffer'", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    format = _PyLong_AsInt(args[1]);
    if (format == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _ssl_Certificate_from_buffer_impl(type, &buffer, format);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_ssl_Certificate_chain_from_file__doc__,
"chain_from_file($type, /, path, *, password=None)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_CHAIN_FROM_FILE_METHODDEF    \
    {"chain_from_file", (PyCFunction)(void(*)(void))_ssl_Certificate_chain_from_file, METH_FASTCALL|METH_KEYWORDS|METH_CLASS, _ssl_Certificate_chain_from_file__doc__},

static PyObject *
_ssl_Certificate_chain_from_file_impl(PyTypeObject *type, PyObject *path,
                                      PyObject *password);

static PyObject *
_ssl_Certificate_chain_from_file(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "password", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "chain_from_file", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *password = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_FSConverter(args[0], &path)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    password = args[1];
skip_optional_kwonly:
    return_value = _ssl_Certificate_chain_from_file_impl(type, path, password);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_Certificate_chain_from_buffer__doc__,
"chain_from_buffer($type, /, buffer)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_CHAIN_FROM_BUFFER_METHODDEF    \
    {"chain_from_buffer", (PyCFunction)(void(*)(void))_ssl_Certificate_chain_from_buffer, METH_FASTCALL|METH_KEYWORDS|METH_CLASS, _ssl_Certificate_chain_from_buffer__doc__},

static PyObject *
_ssl_Certificate_chain_from_buffer_impl(PyTypeObject *type,
                                        Py_buffer *buffer);

static PyObject *
_ssl_Certificate_chain_from_buffer(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"buffer", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "chain_from_buffer", 0};
    PyObject *argsbuf[1];
    Py_buffer buffer = {NULL, NULL};

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("chain_from_buffer", "argument 'buffer'", "contiguous buffer", args[0]);
        goto exit;
    }
    return_value = _ssl_Certificate_chain_from_buffer_impl(type, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_ssl_Certificate_bundle_from_file__doc__,
"bundle_from_file($type, /, path, *, format=FILETYPE_PEM)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_BUNDLE_FROM_FILE_METHODDEF    \
    {"bundle_from_file", (PyCFunction)(void(*)(void))_ssl_Certificate_bundle_from_file, METH_FASTCALL|METH_KEYWORDS|METH_CLASS, _ssl_Certificate_bundle_from_file__doc__},

static PyObject *
_ssl_Certificate_bundle_from_file_impl(PyTypeObject *type, PyObject *path,
                                       int format);

static PyObject *
_ssl_Certificate_bundle_from_file(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "format", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "bundle_from_file", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    int format = PY_SSL_FILETYPE_PEM;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_FSConverter(args[0], &path)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    format = _PyLong_AsInt(args[1]);
    if (format == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _ssl_Certificate_bundle_from_file_impl(type, path, format);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_Certificate_bundle_from_buffer__doc__,
"bundle_from_buffer($type, /, buffer, *, format=FILETYPE_PEM)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_BUNDLE_FROM_BUFFER_METHODDEF    \
    {"bundle_from_buffer", (PyCFunction)(void(*)(void))_ssl_Certificate_bundle_from_buffer, METH_FASTCALL|METH_KEYWORDS|METH_CLASS, _ssl_Certificate_bundle_from_buffer__doc__},

static PyObject *
_ssl_Certificate_bundle_from_buffer_impl(PyTypeObject *type,
                                         Py_buffer *buffer, int format);

static PyObject *
_ssl_Certificate_bundle_from_buffer(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"buffer", "format", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "bundle_from_buffer", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer buffer = {NULL, NULL};
    int format = PY_SSL_FILETYPE_PEM;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("bundle_from_buffer", "argument 'buffer'", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    format = _PyLong_AsInt(args[1]);
    if (format == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _ssl_Certificate_bundle_from_buffer_impl(type, &buffer, format);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_ssl_Certificate_check_hostname__doc__,
"check_hostname($self, /, hostname, *, flags=0)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_CHECK_HOSTNAME_METHODDEF    \
    {"check_hostname", (PyCFunction)(void(*)(void))_ssl_Certificate_check_hostname, METH_FASTCALL|METH_KEYWORDS, _ssl_Certificate_check_hostname__doc__},

static PyObject *
_ssl_Certificate_check_hostname_impl(PySSLCertificate *self, char *hostname,
                                     unsigned int flags);

static PyObject *
_ssl_Certificate_check_hostname(PySSLCertificate *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"hostname", "flags", NULL};
    static _PyArg_Parser _parser = {"et|$I:check_hostname", _keywords, 0};
    char *hostname = NULL;
    unsigned int flags = 0;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        "idna", &hostname, &flags)) {
        goto exit;
    }
    return_value = _ssl_Certificate_check_hostname_impl(self, hostname, flags);

exit:
    /* Cleanup for hostname */
    if (hostname) {
       PyMem_FREE(hostname);
    }

    return return_value;
}

PyDoc_STRVAR(_ssl_Certificate_check_ipaddress__doc__,
"check_ipaddress($self, /, address, *, flags=0)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_CHECK_IPADDRESS_METHODDEF    \
    {"check_ipaddress", (PyCFunction)(void(*)(void))_ssl_Certificate_check_ipaddress, METH_FASTCALL|METH_KEYWORDS, _ssl_Certificate_check_ipaddress__doc__},

static PyObject *
_ssl_Certificate_check_ipaddress_impl(PySSLCertificate *self,
                                      const char *address,
                                      unsigned int flags);

static PyObject *
_ssl_Certificate_check_ipaddress(PySSLCertificate *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"address", "flags", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "check_ipaddress", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    const char *address;
    unsigned int flags = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("check_ipaddress", "argument 'address'", "str", args[0]);
        goto exit;
    }
    Py_ssize_t address_length;
    address = PyUnicode_AsUTF8AndSize(args[0], &address_length);
    if (address == NULL) {
        goto exit;
    }
    if (strlen(address) != (size_t)address_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    flags = (unsigned int)PyLong_AsUnsignedLongMask(args[1]);
    if (flags == (unsigned int)-1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _ssl_Certificate_check_ipaddress_impl(self, address, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_Certificate_dumps__doc__,
"dumps($self, /, format=FILETYPE_PEM)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_DUMPS_METHODDEF    \
    {"dumps", (PyCFunction)(void(*)(void))_ssl_Certificate_dumps, METH_FASTCALL|METH_KEYWORDS, _ssl_Certificate_dumps__doc__},

static PyObject *
_ssl_Certificate_dumps_impl(PySSLCertificate *self, int format);

static PyObject *
_ssl_Certificate_dumps(PySSLCertificate *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"format", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "dumps", 0};
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int format = PY_SSL_FILETYPE_PEM;

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
    return_value = _ssl_Certificate_dumps_impl(self, format);

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

PyDoc_STRVAR(_ssl_Certificate_get_issuer__doc__,
"get_issuer($self, /, *, flags=None)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_GET_ISSUER_METHODDEF    \
    {"get_issuer", (PyCFunction)(void(*)(void))_ssl_Certificate_get_issuer, METH_FASTCALL|METH_KEYWORDS, _ssl_Certificate_get_issuer__doc__},

static PyObject *
_ssl_Certificate_get_issuer_impl(PySSLCertificate *self, PyObject *flags);

static PyObject *
_ssl_Certificate_get_issuer(PySSLCertificate *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"flags", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "get_issuer", 0};
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *flags = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 0, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    flags = args[0];
skip_optional_kwonly:
    return_value = _ssl_Certificate_get_issuer_impl(self, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_Certificate_get_subject__doc__,
"get_subject($self, /, *, flags=None)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_GET_SUBJECT_METHODDEF    \
    {"get_subject", (PyCFunction)(void(*)(void))_ssl_Certificate_get_subject, METH_FASTCALL|METH_KEYWORDS, _ssl_Certificate_get_subject__doc__},

static PyObject *
_ssl_Certificate_get_subject_impl(PySSLCertificate *self, PyObject *flags);

static PyObject *
_ssl_Certificate_get_subject(PySSLCertificate *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"flags", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "get_subject", 0};
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *flags = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 0, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    flags = args[0];
skip_optional_kwonly:
    return_value = _ssl_Certificate_get_subject_impl(self, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_Certificate_get_spki__doc__,
"get_spki($self, /)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_GET_SPKI_METHODDEF    \
    {"get_spki", (PyCFunction)_ssl_Certificate_get_spki, METH_NOARGS, _ssl_Certificate_get_spki__doc__},

static PyObject *
_ssl_Certificate_get_spki_impl(PySSLCertificate *self);

static PyObject *
_ssl_Certificate_get_spki(PySSLCertificate *self, PyObject *Py_UNUSED(ignored))
{
    return _ssl_Certificate_get_spki_impl(self);
}
/*[clinic end generated code: output=54803b371d3e7889 input=a9049054013a1b77]*/
