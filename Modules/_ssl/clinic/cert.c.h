/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


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
    #define NUM_KEYWORDS 1
    #if NUM_KEYWORDS == 0

    #  if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #    define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #  else
    #    define KWTUPLE NULL
    #  endif

    #else  // NUM_KEYWORDS != 0
    #  if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(format), },
    };
    #  define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #  else  // !Py_BUILD_CORE
    #    define KWTUPLE NULL
    #  endif  // !Py_BUILD_CORE
    #endif  // NUM_KEYWORDS != 0
    #undef NUM_KEYWORDS

    static const char * const _keywords[] = {"format", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "public_bytes",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
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
/*[clinic end generated code: output=39d0c03e76b5f361 input=a9049054013a1b77]*/
