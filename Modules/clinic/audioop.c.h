/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(audioop_getsample__doc__,
"getsample($module, fragment, width, index, /)\n"
"--\n"
"\n"
"Return the value of sample index from the fragment.");

#define AUDIOOP_GETSAMPLE_METHODDEF    \
    {"getsample", (PyCFunction)audioop_getsample, METH_VARARGS, audioop_getsample__doc__},

static PyObject *
audioop_getsample_impl(PyObject *module, Py_buffer *fragment, int width,
                       Py_ssize_t index);

static PyObject *
audioop_getsample(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    Py_ssize_t index;

    if (!PyArg_ParseTuple(args, "y*in:getsample",
        &fragment, &width, &index)) {
        goto exit;
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
    {"max", (PyCFunction)audioop_max, METH_VARARGS, audioop_max__doc__},

static PyObject *
audioop_max_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_max(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!PyArg_ParseTuple(args, "y*i:max",
        &fragment, &width)) {
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
    {"minmax", (PyCFunction)audioop_minmax, METH_VARARGS, audioop_minmax__doc__},

static PyObject *
audioop_minmax_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_minmax(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!PyArg_ParseTuple(args, "y*i:minmax",
        &fragment, &width)) {
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
    {"avg", (PyCFunction)audioop_avg, METH_VARARGS, audioop_avg__doc__},

static PyObject *
audioop_avg_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_avg(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!PyArg_ParseTuple(args, "y*i:avg",
        &fragment, &width)) {
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
    {"rms", (PyCFunction)audioop_rms, METH_VARARGS, audioop_rms__doc__},

static PyObject *
audioop_rms_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_rms(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!PyArg_ParseTuple(args, "y*i:rms",
        &fragment, &width)) {
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
    {"findfit", (PyCFunction)audioop_findfit, METH_VARARGS, audioop_findfit__doc__},

static PyObject *
audioop_findfit_impl(PyObject *module, Py_buffer *fragment,
                     Py_buffer *reference);

static PyObject *
audioop_findfit(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    Py_buffer reference = {NULL, NULL};

    if (!PyArg_ParseTuple(args, "y*y*:findfit",
        &fragment, &reference)) {
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
    {"findfactor", (PyCFunction)audioop_findfactor, METH_VARARGS, audioop_findfactor__doc__},

static PyObject *
audioop_findfactor_impl(PyObject *module, Py_buffer *fragment,
                        Py_buffer *reference);

static PyObject *
audioop_findfactor(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    Py_buffer reference = {NULL, NULL};

    if (!PyArg_ParseTuple(args, "y*y*:findfactor",
        &fragment, &reference)) {
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
    {"findmax", (PyCFunction)audioop_findmax, METH_VARARGS, audioop_findmax__doc__},

static PyObject *
audioop_findmax_impl(PyObject *module, Py_buffer *fragment,
                     Py_ssize_t length);

static PyObject *
audioop_findmax(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    Py_ssize_t length;

    if (!PyArg_ParseTuple(args, "y*n:findmax",
        &fragment, &length)) {
        goto exit;
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
    {"avgpp", (PyCFunction)audioop_avgpp, METH_VARARGS, audioop_avgpp__doc__},

static PyObject *
audioop_avgpp_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_avgpp(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!PyArg_ParseTuple(args, "y*i:avgpp",
        &fragment, &width)) {
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
    {"maxpp", (PyCFunction)audioop_maxpp, METH_VARARGS, audioop_maxpp__doc__},

static PyObject *
audioop_maxpp_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_maxpp(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!PyArg_ParseTuple(args, "y*i:maxpp",
        &fragment, &width)) {
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
    {"cross", (PyCFunction)audioop_cross, METH_VARARGS, audioop_cross__doc__},

static PyObject *
audioop_cross_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_cross(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!PyArg_ParseTuple(args, "y*i:cross",
        &fragment, &width)) {
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
    {"mul", (PyCFunction)audioop_mul, METH_VARARGS, audioop_mul__doc__},

static PyObject *
audioop_mul_impl(PyObject *module, Py_buffer *fragment, int width,
                 double factor);

static PyObject *
audioop_mul(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    double factor;

    if (!PyArg_ParseTuple(args, "y*id:mul",
        &fragment, &width, &factor)) {
        goto exit;
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
    {"tomono", (PyCFunction)audioop_tomono, METH_VARARGS, audioop_tomono__doc__},

static PyObject *
audioop_tomono_impl(PyObject *module, Py_buffer *fragment, int width,
                    double lfactor, double rfactor);

static PyObject *
audioop_tomono(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    double lfactor;
    double rfactor;

    if (!PyArg_ParseTuple(args, "y*idd:tomono",
        &fragment, &width, &lfactor, &rfactor)) {
        goto exit;
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
    {"tostereo", (PyCFunction)audioop_tostereo, METH_VARARGS, audioop_tostereo__doc__},

static PyObject *
audioop_tostereo_impl(PyObject *module, Py_buffer *fragment, int width,
                      double lfactor, double rfactor);

static PyObject *
audioop_tostereo(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    double lfactor;
    double rfactor;

    if (!PyArg_ParseTuple(args, "y*idd:tostereo",
        &fragment, &width, &lfactor, &rfactor)) {
        goto exit;
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
    {"add", (PyCFunction)audioop_add, METH_VARARGS, audioop_add__doc__},

static PyObject *
audioop_add_impl(PyObject *module, Py_buffer *fragment1,
                 Py_buffer *fragment2, int width);

static PyObject *
audioop_add(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment1 = {NULL, NULL};
    Py_buffer fragment2 = {NULL, NULL};
    int width;

    if (!PyArg_ParseTuple(args, "y*y*i:add",
        &fragment1, &fragment2, &width)) {
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
    {"bias", (PyCFunction)audioop_bias, METH_VARARGS, audioop_bias__doc__},

static PyObject *
audioop_bias_impl(PyObject *module, Py_buffer *fragment, int width, int bias);

static PyObject *
audioop_bias(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    int bias;

    if (!PyArg_ParseTuple(args, "y*ii:bias",
        &fragment, &width, &bias)) {
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
    {"reverse", (PyCFunction)audioop_reverse, METH_VARARGS, audioop_reverse__doc__},

static PyObject *
audioop_reverse_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_reverse(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!PyArg_ParseTuple(args, "y*i:reverse",
        &fragment, &width)) {
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
    {"byteswap", (PyCFunction)audioop_byteswap, METH_VARARGS, audioop_byteswap__doc__},

static PyObject *
audioop_byteswap_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_byteswap(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!PyArg_ParseTuple(args, "y*i:byteswap",
        &fragment, &width)) {
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
    {"lin2lin", (PyCFunction)audioop_lin2lin, METH_VARARGS, audioop_lin2lin__doc__},

static PyObject *
audioop_lin2lin_impl(PyObject *module, Py_buffer *fragment, int width,
                     int newwidth);

static PyObject *
audioop_lin2lin(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    int newwidth;

    if (!PyArg_ParseTuple(args, "y*ii:lin2lin",
        &fragment, &width, &newwidth)) {
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
    {"ratecv", (PyCFunction)audioop_ratecv, METH_VARARGS, audioop_ratecv__doc__},

static PyObject *
audioop_ratecv_impl(PyObject *module, Py_buffer *fragment, int width,
                    int nchannels, int inrate, int outrate, PyObject *state,
                    int weightA, int weightB);

static PyObject *
audioop_ratecv(PyObject *module, PyObject *args)
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

    if (!PyArg_ParseTuple(args, "y*iiiiO|ii:ratecv",
        &fragment, &width, &nchannels, &inrate, &outrate, &state, &weightA, &weightB)) {
        goto exit;
    }
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
    {"lin2ulaw", (PyCFunction)audioop_lin2ulaw, METH_VARARGS, audioop_lin2ulaw__doc__},

static PyObject *
audioop_lin2ulaw_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_lin2ulaw(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!PyArg_ParseTuple(args, "y*i:lin2ulaw",
        &fragment, &width)) {
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
    {"ulaw2lin", (PyCFunction)audioop_ulaw2lin, METH_VARARGS, audioop_ulaw2lin__doc__},

static PyObject *
audioop_ulaw2lin_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_ulaw2lin(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!PyArg_ParseTuple(args, "y*i:ulaw2lin",
        &fragment, &width)) {
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
    {"lin2alaw", (PyCFunction)audioop_lin2alaw, METH_VARARGS, audioop_lin2alaw__doc__},

static PyObject *
audioop_lin2alaw_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_lin2alaw(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!PyArg_ParseTuple(args, "y*i:lin2alaw",
        &fragment, &width)) {
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
    {"alaw2lin", (PyCFunction)audioop_alaw2lin, METH_VARARGS, audioop_alaw2lin__doc__},

static PyObject *
audioop_alaw2lin_impl(PyObject *module, Py_buffer *fragment, int width);

static PyObject *
audioop_alaw2lin(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;

    if (!PyArg_ParseTuple(args, "y*i:alaw2lin",
        &fragment, &width)) {
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
    {"lin2adpcm", (PyCFunction)audioop_lin2adpcm, METH_VARARGS, audioop_lin2adpcm__doc__},

static PyObject *
audioop_lin2adpcm_impl(PyObject *module, Py_buffer *fragment, int width,
                       PyObject *state);

static PyObject *
audioop_lin2adpcm(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    PyObject *state;

    if (!PyArg_ParseTuple(args, "y*iO:lin2adpcm",
        &fragment, &width, &state)) {
        goto exit;
    }
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
    {"adpcm2lin", (PyCFunction)audioop_adpcm2lin, METH_VARARGS, audioop_adpcm2lin__doc__},

static PyObject *
audioop_adpcm2lin_impl(PyObject *module, Py_buffer *fragment, int width,
                       PyObject *state);

static PyObject *
audioop_adpcm2lin(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer fragment = {NULL, NULL};
    int width;
    PyObject *state;

    if (!PyArg_ParseTuple(args, "y*iO:adpcm2lin",
        &fragment, &width, &state)) {
        goto exit;
    }
    return_value = audioop_adpcm2lin_impl(module, &fragment, width, state);

exit:
    /* Cleanup for fragment */
    if (fragment.obj) {
       PyBuffer_Release(&fragment);
    }

    return return_value;
}
/*[clinic end generated code: output=e0ab74c3fa57c39c input=a9049054013a1b77]*/
