/*
 * VxWorks Compatibility Wrapper
 *
 * Python interface to VxWorks methods
 *
 * Author: wenyan.xin@windriver.com
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
           int close_fds, int restore_signals,
           PyObject *preexec_fn,
           PyObject *preexec_fn_args_tuple)
{
    int priority = 0;
    unsigned int uStackSize = 0;
    int options = 0;
    int taskOptions = 0;
    char  pwdbuf[PATH_MAX]={0};

    int pid = RTP_ID_ERROR;
    int stdin_bk = -1, stdout_bk = -1, stderr_bk = -1;
    int saved_errno, reached_preexec = 0;
    const char* err_msg = "";
    char hex_errno[sizeof(saved_errno)*2+1];

    if (-1 != p2cwrite &&
        _Py_set_inheritable_async_safe(p2cwrite, 0, NULL) < 0)
        goto error;

    if (-1 != c2pread &&
        _Py_set_inheritable_async_safe(c2pread, 0, NULL) < 0)
        goto error;

    if (-1 != errread &&
        _Py_set_inheritable_async_safe(errread, 0, NULL) < 0)
        goto error;

    if (-1 != errpipe_read &&
        _Py_set_inheritable_async_safe(errpipe_read, 0, NULL) < 0)
        goto error;

    if (-1 != errpipe_write &&
        _Py_set_inheritable_async_safe(errpipe_write, 0, NULL) < 0)
        goto error;

    if (p2cread == 0) {
        if (_Py_set_inheritable_async_safe(p2cread, 1, NULL) < 0)
            goto error;
    }
    else if (p2cread != -1) {
        stdin_bk = dup(0);
        if (dup2(p2cread, 0) == -1)  /* stdin */
            goto error;
    }

    if (c2pwrite == 1) {
        if (_Py_set_inheritable_async_safe(c2pwrite, 1, NULL) < 0)
            goto error;
    }
    else if (c2pwrite != -1) {
        stdout_bk = dup(1);
        if (dup2(c2pwrite, 1) == -1)  /* stdout */
            goto error;
    }

    if (errwrite == 2) {
        if (_Py_set_inheritable_async_safe(errwrite, 1, NULL) < 0)
            goto error;
    }
    else if (errwrite != -1) {
        stderr_bk = dup(2);
        if (dup2(errwrite, 2) == -1) /* stderr */
            goto error;
    }

    char *cwd_bk = getcwd(pwdbuf, sizeof(pwdbuf));
    if (cwd)
        chdir(cwd);

    reached_preexec = 1;
    if (preexec_fn != Py_None && preexec_fn_args_tuple) {
        PyObject *result;
        /* This is where the user has asked us to deadlock their program. */
        result = PyObject_Call(preexec_fn, preexec_fn_args_tuple, NULL);
        if (result == NULL) {
            /* Stringifying the exception or traceback would involve
             * memory allocation and thus potential for deadlock.
             * We've already faced potential deadlock by calling back
             * into Python in the first place, so it probably doesn't
             * matter but we avoid it to minimize the possibility. */
            err_msg = "Exception occurred in preexec_fn.";
            errno = 0;  /* We don't want to report an OSError. */
            goto error;
        }
        /* Py_DECREF(result); - We're about to exec so why bother? */
    }

    (void)taskPriorityGet(taskIdSelf(), &priority);
    (void)taskStackSizeGet(taskIdSelf(), &uStackSize);

    saved_errno = 0;
    for (int i = 0; exec_array[i] != NULL; ++i) {
        const char *executable = exec_array[i];
        pid = rtpSpawn(executable, (const char **)argvp,
             (const char**)envpp, priority, uStackSize, options, taskOptions);
        if (errno != ENOENT && errno != ENOTDIR && saved_errno == 0) {
            saved_errno = errno;
        }
    }
    /* Report the first exec error, not the last. */
    if (saved_errno)
        errno = saved_errno;

    if (stdin_bk >= 0) {
        if (dup2(stdin_bk, 0) == -1)
            goto error;
        close(stdin_bk);
    }
    if (stdout_bk >= 0) {
        if (dup2(stdout_bk, 1) == -1)
            goto error;
        close(stdout_bk);
    }
    if (stderr_bk >= 0) {
        if (dup2(stderr_bk,2) == -1)
            goto error;
        close(stderr_bk);
    }

    if (cwd && cwd_bk)
        chdir(cwd_bk);

    if (RTP_ID_ERROR != pid) {
        return Py_BuildValue("i", pid);
    }

error:
    saved_errno = errno;
    /* Report the posix error to our parent process. */
    /* We ignore all write() return values as the total size of our writes is
       less than PIPEBUF and we cannot do anything about an error anyways.
       Use _Py_write_noraise() to retry write() if it is interrupted by a
       signal (fails with EINTR). */
    if (saved_errno) {
        char *cur;
        _Py_write_noraise(errpipe_write, "OSError:", 8);
        cur = hex_errno + sizeof(hex_errno);
        while (saved_errno != 0 && cur != hex_errno) {
            *--cur = Py_hexdigits[saved_errno % 16];
            saved_errno /= 16;
        }
        _Py_write_noraise(errpipe_write, cur, hex_errno + sizeof(hex_errno) - cur);
        _Py_write_noraise(errpipe_write, ":", 1);
        if (!reached_preexec) {
            /* Indicate to the parent that the error happened before rtpSpawn(). */
            _Py_write_noraise(errpipe_write, "noexec", 6);
        }
        /* We can't call strerror(saved_errno).  It is not async signal safe.
         * The parent process will look the error message up. */
    } else {
        _Py_write_noraise(errpipe_write, "SubprocessError:0:", 18);
        _Py_write_noraise(errpipe_write, err_msg, strlen(err_msg));
    }
    return PyErr_SetFromErrno(PyExc_OSError);;
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
/*[clinic end generated code: output=5f98889b783df975 input=30419f3fea045213]*/
{
    PyObject *converted_args = NULL, *fast_args = NULL;
    PyObject *cwd_obj2;
    const char *cwd;
    char *const *exec_array, *const *argv = NULL, *const *envp = NULL;
    PyObject *preexec_fn_args_tuple = NULL;

    PyObject *return_value = NULL;

    exec_array = _PySequence_BytesToCharpArray(executable_list);
    if (!exec_array)
        goto cleanup;

    /* Convert args and env into appropriate arguments */
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

    if (preexec_fn != Py_None) {
        preexec_fn_args_tuple = PyTuple_New(0);
        if (!preexec_fn_args_tuple)
            goto cleanup;
    }

    return_value = rtp_spawn_impl(
                        exec_array, argv, envp, cwd,
                        p2cread, p2cwrite, c2pread, c2pwrite,
                        errread, errwrite, errpipe_read, errpipe_write,
                        close_fds, restore_signals, preexec_fn,
                        preexec_fn_args_tuple);

cleanup:
    if (envp)
        _Py_FreeCharPArray(envp);
    if (argv)
        _Py_FreeCharPArray(argv);
    if (exec_array)
        _Py_FreeCharPArray(exec_array);
    Py_XDECREF(preexec_fn_args_tuple);
    Py_XDECREF(converted_args);
    Py_XDECREF(fast_args);
    return return_value;
}



static PyMethodDef _vxwapiMethods[] = {
    _VXWAPI_RTP_SPAWN_METHODDEF
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

