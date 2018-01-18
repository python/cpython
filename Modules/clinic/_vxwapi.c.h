/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(__VXWORKS__)

PyDoc_STRVAR(_vxwapi_rtpSpawn__doc__,
"rtpSpawn($module, rtpFileName, argv, envp, priority, uStackSize,\n"
"         options, taskOptions, /)\n"
"--\n"
"\n"
"Spawn a real time process in the vxWorks OS");

#define _VXWAPI_RTPSPAWN_METHODDEF    \
    {"rtpSpawn", (PyCFunction)_vxwapi_rtpSpawn, METH_FASTCALL, _vxwapi_rtpSpawn__doc__},

static PyObject *
_vxwapi_rtpSpawn_impl(PyObject *module, const char *rtpFileName,
                      PyObject *argv, PyObject *envp, int priority,
                      unsigned int uStackSize, int options, int taskOptions);

static PyObject *
_vxwapi_rtpSpawn(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *rtpFileName;
    PyObject *argv;
    PyObject *envp;
    int priority;
    unsigned int uStackSize;
    int options;
    int taskOptions;

    if (!_PyArg_ParseStack(args, nargs, "sO!O!iIii:rtpSpawn",
        &rtpFileName, &PyList_Type, &argv, &PyList_Type, &envp, &priority, &uStackSize, &options, &taskOptions)) {
        goto exit;
    }
    return_value = _vxwapi_rtpSpawn_impl(module, rtpFileName, argv, envp, priority, uStackSize, options, taskOptions);

exit:
    return return_value;
}

#endif /* defined(__VXWORKS__) */

#ifndef _VXWAPI_RTPSPAWN_METHODDEF
    #define _VXWAPI_RTPSPAWN_METHODDEF
#endif /* !defined(_VXWAPI_RTPSPAWN_METHODDEF) */
/*[clinic end generated code: output=b00bd9a882c5b91c input=a9049054013a1b77]*/
