/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(__VXWORKS__)

PyDoc_STRVAR(_vxwapi_isAbs__doc__,
"isAbs($module, path, /)\n"
"--\n"
"\n"
"Check if path is an absolute path on VxWorks (since not all VxWorks absolute paths start with /)");

#define _VXWAPI_ISABS_METHODDEF    \
    {"isAbs", (PyCFunction)_vxwapi_isAbs, METH_O, _vxwapi_isAbs__doc__},

static PyObject *
_vxwapi_isAbs_impl(PyObject *module, const char *path);

static PyObject *
_vxwapi_isAbs(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *path;

    if (!PyArg_Parse(arg, "s:isAbs", &path)) {
        goto exit;
    }
    return_value = _vxwapi_isAbs_impl(module, path);

exit:
    return return_value;
}

#endif /* defined(__VXWORKS__) */

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

#ifndef _VXWAPI_ISABS_METHODDEF
    #define _VXWAPI_ISABS_METHODDEF
#endif /* !defined(_VXWAPI_ISABS_METHODDEF) */

#ifndef _VXWAPI_RTPSPAWN_METHODDEF
    #define _VXWAPI_RTPSPAWN_METHODDEF
#endif /* !defined(_VXWAPI_RTPSPAWN_METHODDEF) */
/*[clinic end generated code: output=2a2f3f7db33c7f47 input=a9049054013a1b77]*/
