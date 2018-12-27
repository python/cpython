/*
 * VxWorks Compatibility Wrapper
 *
 * Python interface to VxWorks methods
 *
 * Author: Oscar Shi (co-op winter2018), wenyan.xin@windriver.com
 *
 ************************************************/

#include <Python.h>
#include <rtpLib.h>
#include <pathLib.h>
#include <taskLibCommon.h>
#include "clinic/_vxwapi.c.h"


/*[clinic input]
module _vxwapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6efcf3b26a262ef1]*/

/*[clinic input]
_vxwapi.isAbs

     path: str
     /

Check if path is an absolute path on VxWorks (since not all VxWorks absolute paths start with /)
[clinic start generated code]*/

static PyObject *
_vxwapi_isAbs_impl(PyObject *module, const char *path)
/*[clinic end generated code: output=c6929732e0e3b56e input=485a1871f58eb505]*/
{                                                                  
    long ret = (long)_pathIsAbsolute(path, NULL);

    return PyLong_FromLong(ret);
}


static PyObject *
rtp_spawn_impl(
           char *const exec_array[],
           char *const argvp[],
           char *const envpp[],
           const char *cwd,
           int p2cread, int p2cwrite,
           int c2pread, int c2pwrite,
           int errread, int errwrite,
           int errpipe_read, int errpipe_write,
           int close_fds, int restore_signals)
{
    int priority = 0;
    unsigned int uStackSize = 0;
    int options = 0;
    int taskOptions = 0;

    int pid = RTP_ID_ERROR;
    (void)taskPriorityGet(taskIdSelf(), &priority);
    (void)taskStackSizeGet(taskIdSelf(), &uStackSize);

    char  pwdbuf[256]={0};
    char *cwd_bk = getcwd(pwdbuf, sizeof(pwdbuf));
    if (cwd)
        chdir(cwd);

    for (int i = 0; exec_array[i] != NULL; ++i) {
        const char *executable = exec_array[i];
        pid = rtpSpawn(executable, (const char **)argvp,
               (const char **) envpp, priority, uStackSize, options, taskOptions);
    }

    if (pid == RTP_ID_ERROR){
        PyErr_SetString(PyExc_RuntimeError, "RTPSpawn failed to spawn task");
    }

    if (cwd && cwd_bk)
        chdir(cwd_bk);

    if (RTP_ID_ERROR != pid)
        return Py_BuildValue("i", pid);

    return NULL;
}


/*[clinic input]
_vxwapi.rtp_spawn

    process_args: object
    executable_list: object
    close_fds: int
    py_fds_to_keep: object(subclass_of='&PyTuple_Type')
    cwd_obj: object
    env_list: object
    p2cread: int
    p2cwrite: int
    c2pread: int
    c2pwrite: int
    errread: int
    errwrite: int
    errpipe_read: int
    errpipe_write: int
    restore_signals: int
    call_setsid: int
    preexec_fn: object
    /

Spawn a real time process in the vxWorks OS
[clinic start generated code]*/

static PyObject *
_vxwapi_rtp_spawn_impl(PyObject *module, PyObject *process_args,
                       PyObject *executable_list, int close_fds,
                       PyObject *py_fds_to_keep, PyObject *cwd_obj,
                       PyObject *env_list, int p2cread, int p2cwrite,
                       int c2pread, int c2pwrite, int errread, int errwrite,
                       int errpipe_read, int errpipe_write,
                       int restore_signals, int call_setsid,
                       PyObject *preexec_fn)
/*[clinic end generated code: output=5f98889b783df975 input=14080493d27112ef]*/
{
    PyObject *converted_args = NULL, *fast_args = NULL;
    PyObject *cwd_obj2;
    const char *cwd;
    char *const *exec_array, *const *argv = NULL, *const *envp = NULL;

    PyObject *return_value = NULL;

    exec_array = _PySequence_BytesToCharpArray(executable_list);
    if (!exec_array)
        goto cleanup;

    /* Convert args and env into appropriate arguments for exec() */
    /* These conversions are done in the parent process to avoid allocating
       or freeing memory in the child process. */
    if (process_args != Py_None) {
        Py_ssize_t num_args;
        Py_ssize_t arg_num;
        /* Equivalent to:  */
        /*  tuple(PyUnicode_FSConverter(arg) for arg in process_args)  */
        fast_args = PySequence_Fast(process_args, "argv must be a tuple");
        if (fast_args == NULL)
            goto cleanup;
        num_args = PySequence_Fast_GET_SIZE(fast_args);
        converted_args = PyTuple_New(num_args);
        if (converted_args == NULL)
            goto cleanup;
        for (arg_num = 0; arg_num < num_args; ++arg_num) {
            PyObject *borrowed_arg, *converted_arg;
            if (PySequence_Fast_GET_SIZE(fast_args) != num_args) {
                   PyErr_SetString(PyExc_RuntimeError, 
		   "args changed during iteration");
                goto cleanup;
            }
            borrowed_arg = PySequence_Fast_GET_ITEM(fast_args, arg_num);
            if (PyUnicode_FSConverter(borrowed_arg, &converted_arg) == 0)
                goto cleanup;
            PyTuple_SET_ITEM(converted_args, arg_num, converted_arg);
        }

        argv = _PySequence_BytesToCharpArray(converted_args);
        Py_CLEAR(converted_args);
        Py_CLEAR(fast_args);
        if (!argv)
            goto cleanup;
    }

    if (env_list != Py_None) {
        envp = _PySequence_BytesToCharpArray(env_list);
    }

    if (cwd_obj != Py_None) {
        if (PyUnicode_FSConverter(cwd_obj, &cwd_obj2) == 0)
            goto cleanup;
        cwd = PyBytes_AsString(cwd_obj2);
    } else {
        cwd = NULL;
        cwd_obj2 = NULL;
    }

    return_value = rtp_spawn_impl(
                        exec_array, argv, envp, cwd,
                        p2cread, p2cwrite, c2pread, c2pwrite,
                        errread, errwrite, errpipe_read, errpipe_write,
                        close_fds, restore_signals );

cleanup:
    if (envp)
        _Py_FreeCharPArray(envp);
    if (argv)
        _Py_FreeCharPArray(argv);
    if (exec_array)
        _Py_FreeCharPArray(exec_array);
    Py_XDECREF(converted_args);
    Py_XDECREF(fast_args);
    return return_value;
}



static PyMethodDef _vxwapiMethods[] = {
    _VXWAPI_RTP_SPAWN_METHODDEF
    _VXWAPI_ISABS_METHODDEF
    { NULL, NULL }
};

static struct PyModuleDef _vxwapimodule = {
    PyModuleDef_HEAD_INIT,
    "_vxwapi",
    NULL,
    -1,
    _vxwapiMethods
};

PyMODINIT_FUNC
PyInit__vxwapi(void)
{
    return PyModule_Create(&_vxwapimodule);
}

