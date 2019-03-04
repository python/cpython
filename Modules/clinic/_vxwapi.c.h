/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_vxwapi_rtp_spawn__doc__,
"rtp_spawn($module, process_args, executable_list, close_fds,\n"
"          py_fds_to_keep, cwd_obj, env_list, p2cread, p2cwrite,\n"
"          c2pread, c2pwrite, errread, errwrite, errpipe_read,\n"
"          errpipe_write, restore_signals, call_setsid, preexec_fn, /)\n"
"--\n"
"\n"
"Spawn a real time process in the vxWorks OS");

#define _VXWAPI_RTP_SPAWN_METHODDEF    \
    {"rtp_spawn", (PyCFunction)(void(*)(void))_vxwapi_rtp_spawn, METH_FASTCALL, _vxwapi_rtp_spawn__doc__},

static PyObject *
_vxwapi_rtp_spawn_impl(PyObject *module, PyObject *process_args,
                       PyObject *executable_list, int close_fds,
                       PyObject *py_fds_to_keep, PyObject *cwd_obj,
                       PyObject *env_list, int p2cread, int p2cwrite,
                       int c2pread, int c2pwrite, int errread, int errwrite,
                       int errpipe_read, int errpipe_write,
                       int restore_signals, int call_setsid,
                       PyObject *preexec_fn);

static PyObject *
_vxwapi_rtp_spawn(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *process_args;
    PyObject *executable_list;
    int close_fds;
    PyObject *py_fds_to_keep;
    PyObject *cwd_obj;
    PyObject *env_list;
    int p2cread;
    int p2cwrite;
    int c2pread;
    int c2pwrite;
    int errread;
    int errwrite;
    int errpipe_read;
    int errpipe_write;
    int restore_signals;
    int call_setsid;
    PyObject *preexec_fn;

    if (!_PyArg_CheckPositional("rtp_spawn", nargs, 17, 17)) {
        goto exit;
    }
    process_args = args[0];
    executable_list = args[1];
    if (PyFloat_Check(args[2])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    close_fds = _PyLong_AsInt(args[2]);
    if (close_fds == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyTuple_Check(args[3])) {
        _PyArg_BadArgument("rtp_spawn", 4, "tuple", args[3]);
        goto exit;
    }
    py_fds_to_keep = args[3];
    cwd_obj = args[4];
    env_list = args[5];
    if (PyFloat_Check(args[6])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    p2cread = _PyLong_AsInt(args[6]);
    if (p2cread == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_Check(args[7])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    p2cwrite = _PyLong_AsInt(args[7]);
    if (p2cwrite == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_Check(args[8])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    c2pread = _PyLong_AsInt(args[8]);
    if (c2pread == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_Check(args[9])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    c2pwrite = _PyLong_AsInt(args[9]);
    if (c2pwrite == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_Check(args[10])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    errread = _PyLong_AsInt(args[10]);
    if (errread == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_Check(args[11])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    errwrite = _PyLong_AsInt(args[11]);
    if (errwrite == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_Check(args[12])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    errpipe_read = _PyLong_AsInt(args[12]);
    if (errpipe_read == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_Check(args[13])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    errpipe_write = _PyLong_AsInt(args[13]);
    if (errpipe_write == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_Check(args[14])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    restore_signals = _PyLong_AsInt(args[14]);
    if (restore_signals == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_Check(args[15])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    call_setsid = _PyLong_AsInt(args[15]);
    if (call_setsid == -1 && PyErr_Occurred()) {
        goto exit;
    }
    preexec_fn = args[16];
    return_value = _vxwapi_rtp_spawn_impl(module, process_args, executable_list, close_fds, py_fds_to_keep, cwd_obj, env_list, p2cread, p2cwrite, c2pread, c2pwrite, errread, errwrite, errpipe_read, errpipe_write, restore_signals, call_setsid, preexec_fn);

exit:
    return return_value;
}
/*[clinic end generated code: output=216bc865460f7764 input=a9049054013a1b77]*/
