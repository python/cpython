/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(audioop_getsample__doc__,
"getsample($module, fragment, width, index, /)\n"
"--\n"
"\n"
"Return the value of sample index from the fragment.");

#define AUDIOOP_GETSAMPLE_METHODDEF    \
    {"getsample", (PyCFunction)(void(*)(void))audioop_getsample, METH_FASTCALL, audioop_getsample__doc__},

static PyObject *
audioop_getsample_impl(PyObject *module, Py_buffer *fragment, int width,
                       Py_ssize_t index);

static PyObject *
audioop_getsample(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    Py_ssize_t index;

    if (!_PyArg_CheckPositional("getsample", nargs, 3, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("getsample", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
    return_value = audioop_getsample_impl(module, &fragment, width, index);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_max__doc__,
"max($module, fragment, width, /)\n"
"--\n"
"\n"
"Return the maximum of the absolute value of all samples in a fragment.");

#define AUDIOOP_MAX_METHODDEF    \
    {"max", (PyCFunction)(void(*)(void))audioop_max, METH_FASTCALL, audioop_max__doc__},

static PyObject *
audioop_max_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_max(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!_PyArg_CheckPositional("max", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("max", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_max_impl(module, &fragment, width);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_minmax__doc__,
"minmax($module, fragment, width, /)\n"
"--\n"
"\n"
"Return the minimum and maximum values of all samples in the sound fragment.");

#define AUDIOOP_MINMAX_METHODDEF    \
    {"minmax", (PyCFunction)(void(*)(void))audioop_minmax, METH_FASTCALL, audioop_minmax__doc__},

static PyObject *
audioop_minmax_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_minmax(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!_PyArg_CheckPositional("minmax", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("minmax", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_minmax_impl(module, &fragment, width);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_avg__doc__,
"avg($module, fragment, width, /)\n"
"--\n"
"\n"
"Return the average over all samples in the fragment.");

#define AUDIOOP_AVG_METHODDEF    \
    {"avg", (PyCFunction)(void(*)(void))audioop_avg, METH_FASTCALL, audioop_avg__doc__},

static PyObject *
audioop_avg_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_avg(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!_PyArg_CheckPositional("avg", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("avg", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_avg_impl(module, &fragment, width);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_rms__doc__,
"rms($module, fragment, width, /)\n"
"--\n"
"\n"
"Return the root-mean-square of the fragment, i.e. sqrt(sum(S_i^2)/n).");

#define AUDIOOP_RMS_METHODDEF    \
    {"rms", (PyCFunction)(void(*)(void))audioop_rms, METH_FASTCALL, audioop_rms__doc__},

static PyObject *
audioop_rms_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_rms(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!_PyArg_CheckPositional("rms", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("rms", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_rms_impl(module, &fragment, width);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_findfit__doc__,
"findfit($module, fragment, reference, /)\n"
"--\n"
"\n"
"Try to match reference as well as possible to a portion of fragment.");

#define AUDIOOP_FINDFIT_METHODDEF    \
    {"findfit", (PyCFunction)(void(*)(void))audioop_findfit, METH_FASTCALL, audioop_findfit__doc__},

static PyObject *
audioop_findfit_impl(PyObject *module, Py_buffer *fragment,
                     Py_buffer *reference);

static PyObject *
audioop_findfit(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    Py_buffer reference = {NULL, NULL};

    if (!_PyArg_CheckPositional("findfit", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("findfit", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &reference, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&reference, 'C')) {
        _PyArg_BadArgument("findfit", "argument 2", "contiguous buffer", args[1]);
        goto exit;
    }
    return_value = audioop_findfit_impl(module, &fragment, &reference);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }
    /* Cleanup for reference */
    if (reference.obj) {
       PyBuffer_Release(&reference);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_findfactor__doc__,
"findfactor($module, fragment, reference, /)\n"
"--\n"
"\n"
"Return a factor F such that rms(add(fragment, mul(reference, -F))) is minimal.");

#define AUDIOOP_FINDFACTOR_METHODDEF    \
    {"findfactor", (PyCFunction)(void(*)(void))audioop_findfactor, METH_FASTCALL, audioop_findfactor__doc__},

static PyObject *
audioop_findfactor_impl(PyObject *module, Py_buffer *fragment,
                        Py_buffer *reference);

static PyObject *
audioop_findfactor(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    Py_buffer reference = {NULL, NULL};

    if (!_PyArg_CheckPositional("findfactor", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("findfactor", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &reference, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&reference, 'C')) {
        _PyArg_BadArgument("findfactor", "argument 2", "contiguous buffer", args[1]);
        goto exit;
    }
    return_value = audioop_findfactor_impl(module, &fragment, &reference);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }
    /* Cleanup for reference */
    if (reference.obj) {
       PyBuffer_Release(&reference);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_findmax__doc__,
"findmax($module, fragment, length, /)\n"
"--\n"
"\n"
"Search fragment for a slice of specified number of samples with maximum energy.");

#define AUDIOOP_FINDMAX_METHODDEF    \
    {"findmax", (PyCFunction)(void(*)(void))audioop_findmax, METH_FASTCALL, audioop_findmax__doc__},

static PyObject *
audioop_findmax_impl(PyObject *module, Py_buffer *fragment,
                     Py_ssize_t length);

static PyObject *
audioop_findmax(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    Py_ssize_t length;

    if (!_PyArg_CheckPositional("findmax", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("findmax", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        length = ival;
    }
    return_value = audioop_findmax_impl(module, &fragment, length);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_avgpp__doc__,
"avgpp($module, fragment, width, /)\n"
"--\n"
"\n"
"Return the average peak-peak value over all samples in the fragment.");

#define AUDIOOP_AVGPP_METHODDEF    \
    {"avgpp", (PyCFunction)(void(*)(void))audioop_avgpp, METH_FASTCALL, audioop_avgpp__doc__},

static PyObject *
audioop_avgpp_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_avgpp(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!_PyArg_CheckPositional("avgpp", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("avgpp", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_avgpp_impl(module, &fragment, width);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_maxpp__doc__,
"maxpp($module, fragment, width, /)\n"
"--\n"
"\n"
"Return the maximum peak-peak value in the sound fragment.");

#define AUDIOOP_MAXPP_METHODDEF    \
    {"maxpp", (PyCFunction)(void(*)(void))audioop_maxpp, METH_FASTCALL, audioop_maxpp__doc__},

static PyObject *
audioop_maxpp_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_maxpp(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!_PyArg_CheckPositional("maxpp", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("maxpp", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_maxpp_impl(module, &fragment, width);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_cross__doc__,
"cross($module, fragment, width, /)\n"
"--\n"
"\n"
"Return the number of zero crossings in the fragment passed as an argument.");

#define AUDIOOP_CROSS_METHODDEF    \
    {"cross", (PyCFunction)(void(*)(void))audioop_cross, METH_FASTCALL, audioop_cross__doc__},

static PyObject *
audioop_cross_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_cross(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!_PyArg_CheckPositional("cross", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("cross", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_cross_impl(module, &fragment, width);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_mul__doc__,
"mul($module, fragment, width, factor, /)\n"
"--\n"
"\n"
"Return a fragment that has all samples in the original fragment multiplied by the floating-point value factor.");

#define AUDIOOP_MUL_METHODDEF    \
    {"mul", (PyCFunction)(void(*)(void))audioop_mul, METH_FASTCALL, audioop_mul__doc__},

static PyObject *
audioop_mul_impl(PyObject *module, Py_buffer *fragment, int width,
                 double factor);

static PyObject *
audioop_mul(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    double factor;

    if (!_PyArg_CheckPositional("mul", nargs, 3, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("mul", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_CheckExact(args[2])) {
        factor = PyFloat_AS_DOUBLE(args[2]);
    }
    else
    {
        factor = PyFloat_AsDouble(args[2]);
        if (factor == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = audioop_mul_impl(module, &fragment, width, factor);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_tomono__doc__,
"tomono($module, fragment, width, lfactor, rfactor, /)\n"
"--\n"
"\n"
"Convert a stereo fragment to a mono fragment.");

#define AUDIOOP_TOMONO_METHODDEF    \
    {"tomono", (PyCFunction)(void(*)(void))audioop_tomono, METH_FASTCALL, audioop_tomono__doc__},

static PyObject *
audioop_tomono_impl(PyObject *module, Py_buffer *fragment, int width,
                    double lfactor, double rfactor);

static PyObject *
audioop_tomono(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    double lfactor;
    double rfactor;

    if (!_PyArg_CheckPositional("tomono", nargs, 4, 4)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("tomono", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_CheckExact(args[2])) {
        lfactor = PyFloat_AS_DOUBLE(args[2]);
    }
    else
    {
        lfactor = PyFloat_AsDouble(args[2]);
        if (lfactor == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (PyFloat_CheckExact(args[3])) {
        rfactor = PyFloat_AS_DOUBLE(args[3]);
    }
    else
    {
        rfactor = PyFloat_AsDouble(args[3]);
        if (rfactor == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = audioop_tomono_impl(module, &fragment, width, lfactor, rfactor);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_tostereo__doc__,
"tostereo($module, fragment, width, lfactor, rfactor, /)\n"
"--\n"
"\n"
"Generate a stereo fragment from a mono fragment.");

#define AUDIOOP_TOSTEREO_METHODDEF    \
    {"tostereo", (PyCFunction)(void(*)(void))audioop_tostereo, METH_FASTCALL, audioop_tostereo__doc__},

static PyObject *
audioop_tostereo_impl(PyObject *module, Py_buffer *fragment, int width,
                      double lfactor, double rfactor);

static PyObject *
audioop_tostereo(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    double lfactor;
    double rfactor;

    if (!_PyArg_CheckPositional("tostereo", nargs, 4, 4)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("tostereo", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_CheckExact(args[2])) {
        lfactor = PyFloat_AS_DOUBLE(args[2]);
    }
    else
    {
        lfactor = PyFloat_AsDouble(args[2]);
        if (lfactor == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (PyFloat_CheckExact(args[3])) {
        rfactor = PyFloat_AS_DOUBLE(args[3]);
    }
    else
    {
        rfactor = PyFloat_AsDouble(args[3]);
        if (rfactor == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = audioop_tostereo_impl(module, &fragment, width, lfactor, rfactor);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_add__doc__,
"add($module, fragment1, fragment2, width, /)\n"
"--\n"
"\n"
"Return a fragment which is the addition of the two samples passed as parameters.");

#define AUDIOOP_ADD_METHODDEF    \
    {"add", (PyCFunction)(void(*)(void))audioop_add, METH_FASTCALL, audioop_add__doc__},

static PyObject *
audioop_add_impl(PyObject *module, Py_buffer *fragment1,
                 Py_buffer *fragment2, int width);

static PyObject *
audioop_add(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment1 = {NULL, NULL};
    Py_buffer fragment2 = {NULL, NULL};
    int width;

    if (!_PyArg_CheckPositional("add", nargs, 3, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment1, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment1, 'C')) {
        _PyArg_BadArgument("add", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &fragment2, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment2, 'C')) {
        _PyArg_BadArgument("add", "argument 2", "contiguous buffer", args[1]);
        goto exit;
    }
    width = _PyLong_AsInt(args[2]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_add_impl(module, &fragment1, &fragment2, width);

exit:
    /* Cleanup for fragment1 */
    if (fragment1.obj) {
       PyBuffer_Release(&fragment1);
    }
    /* Cleanup for fragment2 */
    if (fragment2.obj) {
       PyBuffer_Release(&fragment2);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_bias__doc__,
"bias($module, fragment, width, bias, /)\n"
"--\n"
"\n"
"Return a fragment that is the original fragment with a bias added to each sample.");

#define AUDIOOP_BIAS_METHODDEF    \
    {"bias", (PyCFunction)(void(*)(void))audioop_bias, METH_FASTCALL, audioop_bias__doc__},

static PyObject *
audioop_bias_impl(PyObject *module, Py_buffer *fragment, int width, int bias);

static PyObject *
audioop_bias(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    int bias;

    if (!_PyArg_CheckPositional("bias", nargs, 3, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("bias", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    bias = _PyLong_AsInt(args[2]);
    if (bias == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_bias_impl(module, &fragment, width, bias);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_reverse__doc__,
"reverse($module, fragment, width, /)\n"
"--\n"
"\n"
"Reverse the samples in a fragment and returns the modified fragment.");

#define AUDIOOP_REVERSE_METHODDEF    \
    {"reverse", (PyCFunction)(void(*)(void))audioop_reverse, METH_FASTCALL, audioop_reverse__doc__},

static PyObject *
audioop_reverse_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_reverse(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!_PyArg_CheckPositional("reverse", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("reverse", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_reverse_impl(module, &fragment, width);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_byteswap__doc__,
"byteswap($module, fragment, width, /)\n"
"--\n"
"\n"
"Convert big-endian samples to little-endian and vice versa.");

#define AUDIOOP_BYTESWAP_METHODDEF    \
    {"byteswap", (PyCFunction)(void(*)(void))audioop_byteswap, METH_FASTCALL, audioop_byteswap__doc__},

static PyObject *
audioop_byteswap_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_byteswap(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!_PyArg_CheckPositional("byteswap", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("byteswap", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_byteswap_impl(module, &fragment, width);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_lin2lin__doc__,
"lin2lin($module, fragment, width, newwidth, /)\n"
"--\n"
"\n"
"Convert samples between 1-, 2-, 3- and 4-byte formats.");

#define AUDIOOP_LIN2LIN_METHODDEF    \
    {"lin2lin", (PyCFunction)(void(*)(void))audioop_lin2lin, METH_FASTCALL, audioop_lin2lin__doc__},

static PyObject *
audioop_lin2lin_impl(PyObject *module, Py_buffer *fragment, int width,
                     int newwidth);

static PyObject *
audioop_lin2lin(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    int newwidth;

    if (!_PyArg_CheckPositional("lin2lin", nargs, 3, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("lin2lin", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    newwidth = _PyLong_AsInt(args[2]);
    if (newwidth == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_lin2lin_impl(module, &fragment, width, newwidth);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_ratecv__doc__,
"ratecv($module, fragment, width, nchannels, inrate, outrate, state,\n"
"       weightA=1, weightB=0, /)\n"
"--\n"
"\n"
"Convert the frame rate of the input fragment.");

#define AUDIOOP_RATECV_METHODDEF    \
    {"ratecv", (PyCFunction)(void(*)(void))audioop_ratecv, METH_FASTCALL, audioop_ratecv__doc__},

static PyObject *
audioop_ratecv_impl(PyObject *module, Py_buffer *fragment, int width,
                    int nchannels, int inrate, int outrate, PyObject *state,
                    int weightA, int weightB);

static PyObject *
audioop_ratecv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    int nchannels;
    int inrate;
    int outrate;
    PyObject *state;
    int weightA = 1;
    int weightB = 0;

    if (!_PyArg_CheckPositional("ratecv", nargs, 6, 8)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("ratecv", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    nchannels = _PyLong_AsInt(args[2]);
    if (nchannels == -1 && PyErr_Occurred()) {
        goto exit;
    }
    inrate = _PyLong_AsInt(args[3]);
    if (inrate == -1 && PyErr_Occurred()) {
        goto exit;
    }
    outrate = _PyLong_AsInt(args[4]);
    if (outrate == -1 && PyErr_Occurred()) {
        goto exit;
    }
    state = args[5];
    if (nargs < 7) {
        goto skip_optional;
    }
    weightA = _PyLong_AsInt(args[6]);
    if (weightA == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 8) {
        goto skip_optional;
    }
    weightB = _PyLong_AsInt(args[7]);
    if (weightB == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = audioop_ratecv_impl(module, &fragment, width, nchannels, inrate, outrate, state, weightA, weightB);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_lin2ulaw__doc__,
"lin2ulaw($module, fragment, width, /)\n"
"--\n"
"\n"
"Convert samples in the audio fragment to u-LAW encoding.");

#define AUDIOOP_LIN2ULAW_METHODDEF    \
    {"lin2ulaw", (PyCFunction)(void(*)(void))audioop_lin2ulaw, METH_FASTCALL, audioop_lin2ulaw__doc__},

static PyObject *
audioop_lin2ulaw_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_lin2ulaw(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!_PyArg_CheckPositional("lin2ulaw", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("lin2ulaw", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_lin2ulaw_impl(module, &fragment, width);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_ulaw2lin__doc__,
"ulaw2lin($module, fragment, width, /)\n"
"--\n"
"\n"
"Convert sound fragments in u-LAW encoding to linearly encoded sound fragments.");

#define AUDIOOP_ULAW2LIN_METHODDEF    \
    {"ulaw2lin", (PyCFunction)(void(*)(void))audioop_ulaw2lin, METH_FASTCALL, audioop_ulaw2lin__doc__},

static PyObject *
audioop_ulaw2lin_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_ulaw2lin(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!_PyArg_CheckPositional("ulaw2lin", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("ulaw2lin", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_ulaw2lin_impl(module, &fragment, width);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_lin2alaw__doc__,
"lin2alaw($module, fragment, width, /)\n"
"--\n"
"\n"
"Convert samples in the audio fragment to a-LAW encoding.");

#define AUDIOOP_LIN2ALAW_METHODDEF    \
    {"lin2alaw", (PyCFunction)(void(*)(void))audioop_lin2alaw, METH_FASTCALL, audioop_lin2alaw__doc__},

static PyObject *
audioop_lin2alaw_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_lin2alaw(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!_PyArg_CheckPositional("lin2alaw", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("lin2alaw", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_lin2alaw_impl(module, &fragment, width);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_alaw2lin__doc__,
"alaw2lin($module, fragment, width, /)\n"
"--\n"
"\n"
"Convert sound fragments in a-LAW encoding to linearly encoded sound fragments.");

#define AUDIOOP_ALAW2LIN_METHODDEF    \
    {"alaw2lin", (PyCFunction)(void(*)(void))audioop_alaw2lin, METH_FASTCALL, audioop_alaw2lin__doc__},

static PyObject *
audioop_alaw2lin_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_alaw2lin(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!_PyArg_CheckPositional("alaw2lin", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("alaw2lin", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = audioop_alaw2lin_impl(module, &fragment, width);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_lin2adpcm__doc__,
"lin2adpcm($module, fragment, width, state, /)\n"
"--\n"
"\n"
"Convert samples to 4 bit Intel/DVI ADPCM encoding.");

#define AUDIOOP_LIN2ADPCM_METHODDEF    \
    {"lin2adpcm", (PyCFunction)(void(*)(void))audioop_lin2adpcm, METH_FASTCALL, audioop_lin2adpcm__doc__},

static PyObject *
audioop_lin2adpcm_impl(PyObject *module, Py_buffer *fragment, int width,
                       PyObject *state);

static PyObject *
audioop_lin2adpcm(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    PyObject *state;

    if (!_PyArg_CheckPositional("lin2adpcm", nargs, 3, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("lin2adpcm", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    state = args[2];
    return_value = audioop_lin2adpcm_impl(module, &fragment, width, state);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}

PyDoc_STRVAR(audioop_adpcm2lin__doc__,
"adpcm2lin($module, fragment, width, state, /)\n"
"--\n"
"\n"
"Decode an Intel/DVI ADPCM coded fragment to a linear fragment.");

#define AUDIOOP_ADPCM2LIN_METHODDEF    \
    {"adpcm2lin", (PyCFunction)(void(*)(void))audioop_adpcm2lin, METH_FASTCALL, audioop_adpcm2lin__doc__},

static PyObject *
audioop_adpcm2lin_impl(PyObject *module, Py_buffer *fragment, int width,
                       PyObject *state);

static PyObject *
audioop_adpcm2lin(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    PyObject *state;

    if (!_PyArg_CheckPositional("adpcm2lin", nargs, 3, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &fragment, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&fragment, 'C')) {
        _PyArg_BadArgument("adpcm2lin", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    width = _PyLong_AsInt(args[1]);
    if (width == -1 && PyErr_Occurred()) {
        goto exit;
    }
    state = args[2];
    return_value = audioop_adpcm2lin_impl(module, &fragment, width, state);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}
/*[clinic end generated code: output=840f8c315ebd4946 input=a9049054013a1b77]*/
