/* Authors: Gregory P. Smith & Jeffrey Yasskin */
#include "Python.h"
#ifdef HAVE_PIPE2
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include <fcntl.h>


#define POSIX_CALL(call)   if ((call) == -1) goto error


/* Maximum file descriptor, initialized on module load. */
static long max_fd;


/* Given the gc module call gc.enable() and return 0 on success. */
static int _enable_gc(PyObject *gc_module)
{
    PyObject *result;
    result = PyObject_CallMethod(gc_module, "enable", NULL);
    if (result == NULL)
        return 1;
    Py_DECREF(result);
    return 0;
}


/*
 * This function is code executed in the child process immediately after fork
 * to set things up and call exec().
 *
 * All of the code in this function must only use async-signal-safe functions,
 * listed at `man 7 signal` or
 * http://www.opengroup.org/onlinepubs/009695399/functions/xsh_chap02_04.html.
 *
 * This restriction is documented at
 * http://www.opengroup.org/onlinepubs/009695399/functions/fork.html.
 */
static void child_exec(char *const exec_array[],
                       char *const argv[],
                       char *const envp[],
                       const char *cwd,
                       int p2cread, int p2cwrite,
                       int c2pread, int c2pwrite,
                       int errread, int errwrite,
                       int errpipe_read, int errpipe_write,
                       int close_fds, int restore_signals,
                       int call_setsid, Py_ssize_t num_fds_to_keep,
                       PyObject *py_fds_to_keep,
                       PyObject *preexec_fn,
                       PyObject *preexec_fn_args_tuple)
{
    int i, saved_errno, fd_num;
    PyObject *result;
    const char* err_msg = "";
    /* Buffer large enough to hold a hex integer.  We can't malloc. */
    char hex_errno[sizeof(saved_errno)*2+1];

    /* Close parent's pipe ends. */
    if (p2cwrite != -1) {
        POSIX_CALL(close(p2cwrite));
    }
    if (c2pread != -1) {
        POSIX_CALL(close(c2pread));
    }
    if (errread != -1) {
        POSIX_CALL(close(errread));
    }
    POSIX_CALL(close(errpipe_read));

    /* Dup fds for child. */
    if (p2cread != -1) {
        POSIX_CALL(dup2(p2cread, 0));  /* stdin */
    }
    if (c2pwrite != -1) {
        POSIX_CALL(dup2(c2pwrite, 1));  /* stdout */
    }
    if (errwrite != -1) {
        POSIX_CALL(dup2(errwrite, 2));  /* stderr */
    }

    /* Close pipe fds.  Make sure we don't close the same fd more than */
    /* once, or standard fds. */
    if (p2cread != -1 && p2cread != 0) {
        POSIX_CALL(close(p2cread));
    }
    if (c2pwrite != -1 && c2pwrite != p2cread && c2pwrite != 1) {
        POSIX_CALL(close(c2pwrite));
    }
    if (errwrite != -1 && errwrite != p2cread &&
        errwrite != c2pwrite && errwrite != 2) {
        POSIX_CALL(close(errwrite));
    }

    /* close() is intentionally not checked for errors here as we are closing */
    /* a large range of fds, some of which may be invalid. */
    if (close_fds) {
        Py_ssize_t keep_seq_idx;
        int start_fd = 3;
        for (keep_seq_idx = 0; keep_seq_idx < num_fds_to_keep; ++keep_seq_idx) {
            PyObject* py_keep_fd = PySequence_Fast_GET_ITEM(py_fds_to_keep,
                                                            keep_seq_idx);
            int keep_fd = PyLong_AsLong(py_keep_fd);
            if (keep_fd < 0) {  /* Negative number, overflow or not a Long. */
                err_msg = "bad value in fds_to_keep.";
                errno = 0;  /* We don't want to report an OSError. */
                goto error;
            }
            if (keep_fd < start_fd)
                continue;
            for (fd_num = start_fd; fd_num < keep_fd; ++fd_num) {
                close(fd_num);
            }
            start_fd = keep_fd + 1;
        }
        if (start_fd <= max_fd) {
            for (fd_num = start_fd; fd_num < max_fd; ++fd_num) {
                close(fd_num);
            }
        }
    }

    if (cwd)
        POSIX_CALL(chdir(cwd));

    if (restore_signals)
        _Py_RestoreSignals();

#ifdef HAVE_SETSID
    if (call_setsid)
        POSIX_CALL(setsid());
#endif

    if (preexec_fn != Py_None && preexec_fn_args_tuple) {
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

    /* This loop matches the Lib/os.py _execvpe()'s PATH search when */
    /* given the executable_list generated by Lib/subprocess.py.     */
    saved_errno = 0;
    for (i = 0; exec_array[i] != NULL; ++i) {
        const char *executable = exec_array[i];
        if (envp) {
            execve(executable, argv, envp);
        } else {
            execv(executable, argv);
        }
        if (errno != ENOENT && errno != ENOTDIR && saved_errno == 0) {
            saved_errno = errno;
        }
    }
    /* Report the first exec error, not the last. */
    if (saved_errno)
        errno = saved_errno;

error:
    saved_errno = errno;
    /* Report the posix error to our parent process. */
    if (saved_errno) {
        char *cur;
        write(errpipe_write, "OSError:", 8);
        cur = hex_errno + sizeof(hex_errno);
        while (saved_errno != 0 && cur > hex_errno) {
            *--cur = "0123456789ABCDEF"[saved_errno % 16];
            saved_errno /= 16;
        }
        write(errpipe_write, cur, hex_errno + sizeof(hex_errno) - cur);
        write(errpipe_write, ":", 1);
        /* We can't call strerror(saved_errno).  It is not async signal safe.
         * The parent process will look the error message up. */
    } else {
        write(errpipe_write, "RuntimeError:0:", 15);
        write(errpipe_write, err_msg, strlen(err_msg));
    }
}


static PyObject *
subprocess_fork_exec(PyObject* self, PyObject *args)
{
    PyObject *gc_module = NULL;
    PyObject *executable_list, *py_close_fds, *py_fds_to_keep;
    PyObject *env_list, *preexec_fn;
    PyObject *process_args, *converted_args = NULL, *fast_args = NULL;
    PyObject *preexec_fn_args_tuple = NULL;
    int p2cread, p2cwrite, c2pread, c2pwrite, errread, errwrite;
    int errpipe_read, errpipe_write, close_fds, restore_signals;
    int call_setsid;
    PyObject *cwd_obj, *cwd_obj2;
    const char *cwd;
    pid_t pid;
    int need_to_reenable_gc = 0;
    char *const *exec_array, *const *argv = NULL, *const *envp = NULL;
    Py_ssize_t arg_num, num_fds_to_keep;

    if (!PyArg_ParseTuple(
            args, "OOOOOOiiiiiiiiiiO:fork_exec",
            &process_args, &executable_list, &py_close_fds, &py_fds_to_keep,
            &cwd_obj, &env_list,
            &p2cread, &p2cwrite, &c2pread, &c2pwrite,
            &errread, &errwrite, &errpipe_read, &errpipe_write,
            &restore_signals, &call_setsid, &preexec_fn))
        return NULL;

    close_fds = PyObject_IsTrue(py_close_fds);
    if (close_fds && errpipe_write < 3) {  /* precondition */
        PyErr_SetString(PyExc_ValueError, "errpipe_write must be >= 3");
        return NULL;
    }
    num_fds_to_keep = PySequence_Length(py_fds_to_keep);
    if (num_fds_to_keep < 0) {
        PyErr_SetString(PyExc_ValueError, "bad fds_to_keep");
        return NULL;
    }

    /* We need to call gc.disable() when we'll be calling preexec_fn */
    if (preexec_fn != Py_None) {
        PyObject *result;
        gc_module = PyImport_ImportModule("gc");
        if (gc_module == NULL)
            return NULL;
        result = PyObject_CallMethod(gc_module, "isenabled", NULL);
        if (result == NULL) {
            Py_DECREF(gc_module);
            return NULL;
        }
        need_to_reenable_gc = PyObject_IsTrue(result);
        Py_DECREF(result);
        if (need_to_reenable_gc == -1) {
            Py_DECREF(gc_module);
            return NULL;
        }
        result = PyObject_CallMethod(gc_module, "disable", NULL);
        if (result == NULL) {
            Py_DECREF(gc_module);
            return NULL;
        }
        Py_DECREF(result);
    }

    exec_array = _PySequence_BytesToCharpArray(executable_list);
    if (!exec_array)
        return NULL;

    /* Convert args and env into appropriate arguments for exec() */
    /* These conversions are done in the parent process to avoid allocating
       or freeing memory in the child process. */
    if (process_args != Py_None) {
        Py_ssize_t num_args;
        /* Equivalent to:  */
        /*  tuple(PyUnicode_FSConverter(arg) for arg in process_args)  */
        fast_args = PySequence_Fast(process_args, "argv must be a tuple");
        num_args = PySequence_Fast_GET_SIZE(fast_args);
        converted_args = PyTuple_New(num_args);
        if (converted_args == NULL)
            goto cleanup;
        for (arg_num = 0; arg_num < num_args; ++arg_num) {
            PyObject *borrowed_arg, *converted_arg;
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
        if (!envp)
            goto cleanup;
    }

    if (preexec_fn != Py_None) {
        preexec_fn_args_tuple = PyTuple_New(0);
        if (!preexec_fn_args_tuple)
            goto cleanup;
        _PyImport_AcquireLock();
    }

    if (cwd_obj != Py_None) {
        if (PyUnicode_FSConverter(cwd_obj, &cwd_obj2) == 0)
            goto cleanup;
        cwd = PyBytes_AsString(cwd_obj2);
    } else {
        cwd = NULL;
        cwd_obj2 = NULL;
    }

    pid = fork();
    if (pid == 0) {
        /* Child process */
        /*
         * Code from here to _exit() must only use async-signal-safe functions,
         * listed at `man 7 signal` or
         * http://www.opengroup.org/onlinepubs/009695399/functions/xsh_chap02_04.html.
         */

        if (preexec_fn != Py_None) {
            /* We'll be calling back into Python later so we need to do this.
             * This call may not be async-signal-safe but neither is calling
             * back into Python.  The user asked us to use hope as a strategy
             * to avoid deadlock... */
            PyOS_AfterFork();
        }

        child_exec(exec_array, argv, envp, cwd,
                   p2cread, p2cwrite, c2pread, c2pwrite,
                   errread, errwrite, errpipe_read, errpipe_write,
                   close_fds, restore_signals, call_setsid,
                   num_fds_to_keep, py_fds_to_keep,
                   preexec_fn, preexec_fn_args_tuple);
        _exit(255);
        return NULL;  /* Dead code to avoid a potential compiler warning. */
    }
    Py_XDECREF(cwd_obj2);

    if (pid == -1) {
        /* Capture the errno exception before errno can be clobbered. */
        PyErr_SetFromErrno(PyExc_OSError);
    }
    if (preexec_fn != Py_None &&
        _PyImport_ReleaseLock() < 0 && !PyErr_Occurred()) {
        PyErr_SetString(PyExc_RuntimeError,
                        "not holding the import lock");
    }

    /* Parent process */
    if (envp)
        _Py_FreeCharPArray(envp);
    if (argv)
        _Py_FreeCharPArray(argv);
    _Py_FreeCharPArray(exec_array);

    /* Reenable gc in the parent process (or if fork failed). */
    if (need_to_reenable_gc && _enable_gc(gc_module)) {
        Py_XDECREF(gc_module);
        return NULL;
    }
    Py_XDECREF(preexec_fn_args_tuple);
    Py_XDECREF(gc_module);

    if (pid == -1)
        return NULL;  /* fork() failed.  Exception set earlier. */

    return PyLong_FromPid(pid);

cleanup:
    if (envp)
        _Py_FreeCharPArray(envp);
    if (argv)
        _Py_FreeCharPArray(argv);
    _Py_FreeCharPArray(exec_array);
    Py_XDECREF(converted_args);
    Py_XDECREF(fast_args);
    Py_XDECREF(preexec_fn_args_tuple);

    /* Reenable gc if it was disabled. */
    if (need_to_reenable_gc)
        _enable_gc(gc_module);
    Py_XDECREF(gc_module);
    return NULL;
}


PyDoc_STRVAR(subprocess_fork_exec_doc,
"fork_exec(args, executable_list, close_fds, cwd, env,\n\
          p2cread, p2cwrite, c2pread, c2pwrite,\n\
          errread, errwrite, errpipe_read, errpipe_write,\n\
          restore_signals, call_setsid, preexec_fn)\n\
\n\
Forks a child process, closes parent file descriptors as appropriate in the\n\
child and dups the few that are needed before calling exec() in the child\n\
process.\n\
\n\
The preexec_fn, if supplied, will be called immediately before exec.\n\
WARNING: preexec_fn is NOT SAFE if your application uses threads.\n\
         It may trigger infrequent, difficult to debug deadlocks.\n\
\n\
If an error occurs in the child process before the exec, it is\n\
serialized and written to the errpipe_write fd per subprocess.py.\n\
\n\
Returns: the child process's PID.\n\
\n\
Raises: Only on an error in the parent process.\n\
");

PyDoc_STRVAR(subprocess_cloexec_pipe_doc,
"cloexec_pipe() -> (read_end, write_end)\n\n\
Create a pipe whose ends have the cloexec flag set.");

static PyObject *
subprocess_cloexec_pipe(PyObject *self, PyObject *noargs)
{
    int fds[2];
    int res;
#ifdef HAVE_PIPE2
    Py_BEGIN_ALLOW_THREADS
    res = pipe2(fds, O_CLOEXEC);
    Py_END_ALLOW_THREADS
#else
    /* We hold the GIL which offers some protection from other code calling
     * fork() before the CLOEXEC flags have been set but we can't guarantee
     * anything without pipe2(). */
    long oldflags;

    res = pipe(fds);

    if (res == 0) {
        oldflags = fcntl(fds[0], F_GETFD, 0);
        if (oldflags < 0) res = oldflags;
    }
    if (res == 0)
        res = fcntl(fds[0], F_SETFD, oldflags | FD_CLOEXEC);

    if (res == 0) {
        oldflags = fcntl(fds[1], F_GETFD, 0);
        if (oldflags < 0) res = oldflags;
    }
    if (res == 0)
        res = fcntl(fds[1], F_SETFD, oldflags | FD_CLOEXEC);
#endif
    if (res != 0)
        return PyErr_SetFromErrno(PyExc_OSError);
    return Py_BuildValue("(ii)", fds[0], fds[1]);
}

/* module level code ********************************************************/

PyDoc_STRVAR(module_doc,
"A POSIX helper for the subprocess module.");


static PyMethodDef module_methods[] = {
    {"fork_exec", subprocess_fork_exec, METH_VARARGS, subprocess_fork_exec_doc},
    {"cloexec_pipe", subprocess_cloexec_pipe, METH_NOARGS, subprocess_cloexec_pipe_doc},
    {NULL, NULL}  /* sentinel */
};


static struct PyModuleDef _posixsubprocessmodule = {
	PyModuleDef_HEAD_INIT,
	"_posixsubprocess",
	module_doc,
	-1,  /* No memory is needed. */
	module_methods,
};

PyMODINIT_FUNC
PyInit__posixsubprocess(void)
{
#ifdef _SC_OPEN_MAX
    max_fd = sysconf(_SC_OPEN_MAX);
    if (max_fd == -1)
#endif
        max_fd = 256;  /* Matches Lib/subprocess.py */

    return PyModule_Create(&_posixsubprocessmodule);
}
