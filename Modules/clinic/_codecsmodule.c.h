/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_codecs__forget_codec__doc__,
"_forget_codec($module, encoding, /)\n"
"--\n"
"\n"
"Purge the named codec from the internal codec lookup cache");

#define _CODECS__FORGET_CODEC_METHODDEF    \
    {"_forget_codec", (PyCFunction)_codecs__forget_codec, METH_O, _codecs__forget_codec__doc__},

static PyObject *
_codecs__forget_codec_impl(PyModuleDef *module, const char *encoding);

static PyObject *
_codecs__forget_codec(PyModuleDef *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *encoding;

    if (!PyArg_Parse(arg, "s:_forget_codec", &encoding))
        goto exit;
    return_value = _codecs__forget_codec_impl(module, encoding);

exit:
    return return_value;
}
/*[clinic end generated code: output=52cc017e06c8ef9a input=a9049054013a1b77]*/
