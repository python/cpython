/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(HAVE_CLOCK_GETTIME)

PyDoc_STRVAR(time_clock_gettime__doc__,
"clock_gettime($module, clk_id, /)\n"
"--\n"
"\n"
"Return the time of the specified clock clk_id as a float.");

#define TIME_CLOCK_GETTIME_METHODDEF    \
    {"clock_gettime", (PyCFunction)time_clock_gettime, METH_O, time_clock_gettime__doc__},

static PyObject *
time_clock_gettime_impl(PyObject *module, clockid_t clk_id);

static PyObject *
time_clock_gettime(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    clockid_t clk_id;

    if (!time_clockid_converter(arg, &clk_id)) {
        goto exit;
    }
    return_value = time_clock_gettime_impl(module, clk_id);

exit:
    return return_value;
}

#endif /* defined(HAVE_CLOCK_GETTIME) */

#if defined(HAVE_CLOCK_GETTIME)

PyDoc_STRVAR(time_clock_gettime_ns__doc__,
"clock_gettime_ns($module, clk_id, /)\n"
"--\n"
"\n"
"Return the time of the specified clock clk_id as nanoseconds (int).");

#define TIME_CLOCK_GETTIME_NS_METHODDEF    \
    {"clock_gettime_ns", (PyCFunction)time_clock_gettime_ns, METH_O, time_clock_gettime_ns__doc__},

static PyObject *
time_clock_gettime_ns_impl(PyObject *module, clockid_t clk_id);

static PyObject *
time_clock_gettime_ns(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    clockid_t clk_id;

    if (!time_clockid_converter(arg, &clk_id)) {
        goto exit;
    }
    return_value = time_clock_gettime_ns_impl(module, clk_id);

exit:
    return return_value;
}

#endif /* defined(HAVE_CLOCK_GETTIME) */

#ifndef TIME_CLOCK_GETTIME_METHODDEF
    #define TIME_CLOCK_GETTIME_METHODDEF
#endif /* !defined(TIME_CLOCK_GETTIME_METHODDEF) */

#ifndef TIME_CLOCK_GETTIME_NS_METHODDEF
    #define TIME_CLOCK_GETTIME_NS_METHODDEF
#endif /* !defined(TIME_CLOCK_GETTIME_NS_METHODDEF) */
/*[clinic end generated code: output=b589a2132aa9df47 input=a9049054013a1b77]*/
