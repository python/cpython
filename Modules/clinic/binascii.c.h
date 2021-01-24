/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(binascii_a2b_uu__doc__,
"a2b_uu($module, data, /)\n"
"--\n"
"\n"
"Decode a line of uuencoded data.");

#define BINASCII_A2B_UU_METHODDEF    \
    {"a2b_uu", (PyCFunction)binascii_a2b_uu, METH_O, binascii_a2b_uu__doc__},

static PyObject *
binascii_a2b_uu_impl(PyObject *module, Py_buffer *data);

static PyObject *
binascii_a2b_uu(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};

    if (!ascii_buffer_converter(arg, &data)) {
        goto exit;
    }
    return_value = binascii_a2b_uu_impl(module, &data);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);

    return return_value;
}

PyDoc_STRVAR(binascii_b2a_uu__doc__,
"b2a_uu($module, data, /, *, backtick=False)\n"
"--\n"
"\n"
"Uuencode line of data.");

#define BINASCII_B2A_UU_METHODDEF    \
    {"b2a_uu", (PyCFunction)(void(*)(void))binascii_b2a_uu, METH_FASTCALL|METH_KEYWORDS, binascii_b2a_uu__doc__},

static PyObject *
binascii_b2a_uu_impl(PyObject *module, Py_buffer *data, int backtick);

static PyObject *
binascii_b2a_uu(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "backtick", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "b2a_uu", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    int backtick = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("b2a_uu", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    backtick = _PyLong_AsInt(args[1]);
    if (backtick == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = binascii_b2a_uu_impl(module, &data, backtick);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_a2b_base64__doc__,
"a2b_base64($module, data, /)\n"
"--\n"
"\n"
"Decode a line of base64 data.");

#define BINASCII_A2B_BASE64_METHODDEF    \
    {"a2b_base64", (PyCFunction)binascii_a2b_base64, METH_O, binascii_a2b_base64__doc__},

static PyObject *
binascii_a2b_base64_impl(PyObject *module, Py_buffer *data);

static PyObject *
binascii_a2b_base64(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};

    if (!ascii_buffer_converter(arg, &data)) {
        goto exit;
    }
    return_value = binascii_a2b_base64_impl(module, &data);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);

    return return_value;
}

PyDoc_STRVAR(binascii_b2a_base64__doc__,
"b2a_base64($module, data, /, *, newline=True)\n"
"--\n"
"\n"
"Base64-code line of data.");

#define BINASCII_B2A_BASE64_METHODDEF    \
    {"b2a_base64", (PyCFunction)(void(*)(void))binascii_b2a_base64, METH_FASTCALL|METH_KEYWORDS, binascii_b2a_base64__doc__},

static PyObject *
binascii_b2a_base64_impl(PyObject *module, Py_buffer *data, int newline);

static PyObject *
binascii_b2a_base64(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "newline", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "b2a_base64", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    int newline = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("b2a_base64", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    newline = _PyLong_AsInt(args[1]);
    if (newline == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = binascii_b2a_base64_impl(module, &data, newline);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_a2b_hqx__doc__,
"a2b_hqx($module, data, /)\n"
"--\n"
"\n"
"Decode .hqx coding.");

#define BINASCII_A2B_HQX_METHODDEF    \
    {"a2b_hqx", (PyCFunction)binascii_a2b_hqx, METH_O, binascii_a2b_hqx__doc__},

static PyObject *
binascii_a2b_hqx_impl(PyObject *module, Py_buffer *data);

static PyObject *
binascii_a2b_hqx(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};

    if (!ascii_buffer_converter(arg, &data)) {
        goto exit;
    }
    return_value = binascii_a2b_hqx_impl(module, &data);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);

    return return_value;
}

PyDoc_STRVAR(binascii_rlecode_hqx__doc__,
"rlecode_hqx($module, data, /)\n"
"--\n"
"\n"
"Binhex RLE-code binary data.");

#define BINASCII_RLECODE_HQX_METHODDEF    \
    {"rlecode_hqx", (PyCFunction)binascii_rlecode_hqx, METH_O, binascii_rlecode_hqx__doc__},

static PyObject *
binascii_rlecode_hqx_impl(PyObject *module, Py_buffer *data);

static PyObject *
binascii_rlecode_hqx(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("rlecode_hqx", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = binascii_rlecode_hqx_impl(module, &data);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_b2a_hqx__doc__,
"b2a_hqx($module, data, /)\n"
"--\n"
"\n"
"Encode .hqx data.");

#define BINASCII_B2A_HQX_METHODDEF    \
    {"b2a_hqx", (PyCFunction)binascii_b2a_hqx, METH_O, binascii_b2a_hqx__doc__},

static PyObject *
binascii_b2a_hqx_impl(PyObject *module, Py_buffer *data);

static PyObject *
binascii_b2a_hqx(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("b2a_hqx", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = binascii_b2a_hqx_impl(module, &data);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_rledecode_hqx__doc__,
"rledecode_hqx($module, data, /)\n"
"--\n"
"\n"
"Decode hexbin RLE-coded string.");

#define BINASCII_RLEDECODE_HQX_METHODDEF    \
    {"rledecode_hqx", (PyCFunction)binascii_rledecode_hqx, METH_O, binascii_rledecode_hqx__doc__},

static PyObject *
binascii_rledecode_hqx_impl(PyObject *module, Py_buffer *data);

static PyObject *
binascii_rledecode_hqx(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("rledecode_hqx", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = binascii_rledecode_hqx_impl(module, &data);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_crc_hqx__doc__,
"crc_hqx($module, data, crc, /)\n"
"--\n"
"\n"
"Compute CRC-CCITT incrementally.");

#define BINASCII_CRC_HQX_METHODDEF    \
    {"crc_hqx", (PyCFunction)(void(*)(void))binascii_crc_hqx, METH_FASTCALL, binascii_crc_hqx__doc__},

static PyObject *
binascii_crc_hqx_impl(PyObject *module, Py_buffer *data, unsigned int crc);

static PyObject *
binascii_crc_hqx(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int crc;

    if (!_PyArg_CheckPositional("crc_hqx", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("crc_hqx", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    crc = (unsigned int)PyLong_AsUnsignedLongMask(args[1]);
    if (crc == (unsigned int)-1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = binascii_crc_hqx_impl(module, &data, crc);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_crc32__doc__,
"crc32($module, data, crc=0, /)\n"
"--\n"
"\n"
"Compute CRC-32 incrementally.");

#define BINASCII_CRC32_METHODDEF    \
    {"crc32", (PyCFunction)(void(*)(void))binascii_crc32, METH_FASTCALL, binascii_crc32__doc__},

static unsigned int
binascii_crc32_impl(PyObject *module, Py_buffer *data, unsigned int crc);

static PyObject *
binascii_crc32(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int crc = 0;
    unsigned int _return_value;

    if (!_PyArg_CheckPositional("crc32", nargs, 1, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("crc32", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    crc = (unsigned int)PyLong_AsUnsignedLongMask(args[1]);
    if (crc == (unsigned int)-1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = binascii_crc32_impl(module, &data, crc);
    if ((_return_value == (unsigned int)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromUnsignedLong((unsigned long)_return_value);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_b2a_hex__doc__,
"b2a_hex($module, /, data, sep=<unrepresentable>, bytes_per_sep=1)\n"
"--\n"
"\n"
"Hexadecimal representation of binary data.\n"
"\n"
"  sep\n"
"    An optional single character or byte to separate hex bytes.\n"
"  bytes_per_sep\n"
"    How many bytes between separators.  Positive values count from the\n"
"    right, negative values count from the left.\n"
"\n"
"The return value is a bytes object.  This function is also\n"
"available as \"hexlify()\".\n"
"\n"
"Example:\n"
">>> binascii.b2a_hex(b\'\\xb9\\x01\\xef\')\n"
"b\'b901ef\'\n"
">>> binascii.hexlify(b\'\\xb9\\x01\\xef\', \':\')\n"
"b\'b9:01:ef\'\n"
">>> binascii.b2a_hex(b\'\\xb9\\x01\\xef\', b\'_\', 2)\n"
"b\'b9_01ef\'");

#define BINASCII_B2A_HEX_METHODDEF    \
    {"b2a_hex", (PyCFunction)(void(*)(void))binascii_b2a_hex, METH_FASTCALL|METH_KEYWORDS, binascii_b2a_hex__doc__},

static PyObject *
binascii_b2a_hex_impl(PyObject *module, Py_buffer *data, PyObject *sep,
                      int bytes_per_sep);

static PyObject *
binascii_b2a_hex(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"data", "sep", "bytes_per_sep", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "b2a_hex", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    PyObject *sep = NULL;
    int bytes_per_sep = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("b2a_hex", "argument 'data'", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        sep = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    bytes_per_sep = _PyLong_AsInt(args[2]);
    if (bytes_per_sep == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = binascii_b2a_hex_impl(module, &data, sep, bytes_per_sep);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_hexlify__doc__,
"hexlify($module, /, data, sep=<unrepresentable>, bytes_per_sep=1)\n"
"--\n"
"\n"
"Hexadecimal representation of binary data.\n"
"\n"
"  sep\n"
"    An optional single character or byte to separate hex bytes.\n"
"  bytes_per_sep\n"
"    How many bytes between separators.  Positive values count from the\n"
"    right, negative values count from the left.\n"
"\n"
"The return value is a bytes object.  This function is also\n"
"available as \"b2a_hex()\".");

#define BINASCII_HEXLIFY_METHODDEF    \
    {"hexlify", (PyCFunction)(void(*)(void))binascii_hexlify, METH_FASTCALL|METH_KEYWORDS, binascii_hexlify__doc__},

static PyObject *
binascii_hexlify_impl(PyObject *module, Py_buffer *data, PyObject *sep,
                      int bytes_per_sep);

static PyObject *
binascii_hexlify(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"data", "sep", "bytes_per_sep", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "hexlify", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    PyObject *sep = NULL;
    int bytes_per_sep = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("hexlify", "argument 'data'", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        sep = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    bytes_per_sep = _PyLong_AsInt(args[2]);
    if (bytes_per_sep == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = binascii_hexlify_impl(module, &data, sep, bytes_per_sep);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_a2b_hex__doc__,
"a2b_hex($module, hexstr, /)\n"
"--\n"
"\n"
"Binary data of hexadecimal representation.\n"
"\n"
"hexstr must contain an even number of hex digits (upper or lower case).\n"
"This function is also available as \"unhexlify()\".");

#define BINASCII_A2B_HEX_METHODDEF    \
    {"a2b_hex", (PyCFunction)binascii_a2b_hex, METH_O, binascii_a2b_hex__doc__},

static PyObject *
binascii_a2b_hex_impl(PyObject *module, Py_buffer *hexstr);

static PyObject *
binascii_a2b_hex(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer hexstr = {NULL, NULL};

    if (!ascii_buffer_converter(arg, &hexstr)) {
        goto exit;
    }
    return_value = binascii_a2b_hex_impl(module, &hexstr);

exit:
    /* Cleanup for hexstr */
    if (hexstr.obj)
       PyBuffer_Release(&hexstr);

    return return_value;
}

PyDoc_STRVAR(binascii_unhexlify__doc__,
"unhexlify($module, hexstr, /)\n"
"--\n"
"\n"
"Binary data of hexadecimal representation.\n"
"\n"
"hexstr must contain an even number of hex digits (upper or lower case).");

#define BINASCII_UNHEXLIFY_METHODDEF    \
    {"unhexlify", (PyCFunction)binascii_unhexlify, METH_O, binascii_unhexlify__doc__},

static PyObject *
binascii_unhexlify_impl(PyObject *module, Py_buffer *hexstr);

static PyObject *
binascii_unhexlify(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer hexstr = {NULL, NULL};

    if (!ascii_buffer_converter(arg, &hexstr)) {
        goto exit;
    }
    return_value = binascii_unhexlify_impl(module, &hexstr);

exit:
    /* Cleanup for hexstr */
    if (hexstr.obj)
       PyBuffer_Release(&hexstr);

    return return_value;
}

PyDoc_STRVAR(binascii_a2b_qp__doc__,
"a2b_qp($module, /, data, header=False)\n"
"--\n"
"\n"
"Decode a string of qp-encoded data.");

#define BINASCII_A2B_QP_METHODDEF    \
    {"a2b_qp", (PyCFunction)(void(*)(void))binascii_a2b_qp, METH_FASTCALL|METH_KEYWORDS, binascii_a2b_qp__doc__},

static PyObject *
binascii_a2b_qp_impl(PyObject *module, Py_buffer *data, int header);

static PyObject *
binascii_a2b_qp(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"data", "header", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "a2b_qp", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    int header = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!ascii_buffer_converter(args[0], &data)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    header = _PyLong_AsInt(args[1]);
    if (header == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = binascii_a2b_qp_impl(module, &data, header);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);

    return return_value;
}

PyDoc_STRVAR(binascii_b2a_qp__doc__,
"b2a_qp($module, /, data, quotetabs=False, istext=True, header=False)\n"
"--\n"
"\n"
"Encode a string using quoted-printable encoding.\n"
"\n"
"On encoding, when istext is set, newlines are not encoded, and white\n"
"space at end of lines is.  When istext is not set, \\r and \\n (CR/LF)\n"
"are both encoded.  When quotetabs is set, space and tabs are encoded.");

#define BINASCII_B2A_QP_METHODDEF    \
    {"b2a_qp", (PyCFunction)(void(*)(void))binascii_b2a_qp, METH_FASTCALL|METH_KEYWORDS, binascii_b2a_qp__doc__},

static PyObject *
binascii_b2a_qp_impl(PyObject *module, Py_buffer *data, int quotetabs,
                     int istext, int header);

static PyObject *
binascii_b2a_qp(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"data", "quotetabs", "istext", "header", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "b2a_qp", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    int quotetabs = 0;
    int istext = 1;
    int header = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("b2a_qp", "argument 'data'", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        quotetabs = _PyLong_AsInt(args[1]);
        if (quotetabs == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        istext = _PyLong_AsInt(args[2]);
        if (istext == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    header = _PyLong_AsInt(args[3]);
    if (header == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = binascii_b2a_qp_impl(module, &data, quotetabs, istext, header);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}
/*[clinic end generated code: output=95a0178f30801b89 input=a9049054013a1b77]*/
