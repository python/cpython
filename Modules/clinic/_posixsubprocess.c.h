/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(subprocess_fork_exec__doc__,
"fork_exec($module, args, executable_list, close_fds, pass_fds, cwd,\n"
"          env, p2cread, p2cwrite, c2pread, c2pwrite, errread, errwrite,\n"
"          errpipe_read, errpipe_write, restore_signals, call_setsid,\n"
"          pgid_to_set, gid, groups_list, uid, child_umask, preexec_fn,\n"
"          allow_vfork, /)\n"
"--\n"
"\n"
"Spawn a fresh new child process.\n"
"\n"
"Fork a child process, close parent file descriptors as appropriate in the\n"
"child and duplicate the few that are needed before calling exec() in the\n"
"child process.\n"
"\n"
"If close_fds is True, close file descriptors 3 and higher, except those listed\n"
"in the sorted tuple pass_fds.\n"
"\n"
"The preexec_fn, if supplied, will be called immediately before closing file\n"
"descriptors and exec.\n"
"\n"
"WARNING: preexec_fn is NOT SAFE if your application uses threads.\n"
"         It may trigger infrequent, difficult to debug deadlocks.\n"
"\n"
"If an error occurs in the child process before the exec, it is\n"
"serialized and written to the errpipe_write fd per subprocess.py.\n"
"\n"
"Returns: the child process\'s PID.\n"
"\n"
"Raises: Only on an error in the parent process.");

#define SUBPROCESS_FORK_EXEC_METHODDEF    \
    {"fork_exec", (PyCFunction)(void(*)(void))subprocess_fork_exec, METH_FASTCALL, subprocess_fork_exec__doc__},

static PyObject *
subprocess_fork_exec_impl(PyObject *module, PyObject *process_args,
                          PyObject *executable_list, int close_fds,
                          PyObject *py_fds_to_keep, PyObject *cwd_obj,
                          PyObject *env_list, int p2cread, int p2cwrite,
                          int c2pread, int c2pwrite, int errread,
                          int errwrite, int errpipe_read, int errpipe_write,
                          int restore_signals, int call_setsid,
                          pid_t pgid_to_set, PyObject *gid_object,
                          PyObject *groups_list, PyObject *uid_object,
                          int child_umask, PyObject *preexec_fn,
                          int allow_vfork);

static PyObject *
subprocess_fork_exec(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
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
    pid_t pgid_to_set;
    PyObject *gid_object;
    PyObject *groups_list;
    PyObject *uid_object;
    int child_umask;
    PyObject *preexec_fn;
    int allow_vfork;

    if (!_PyArg_ParseStack(args, nargs, "OOpO!OOiiiiiiiiii" _Py_PARSE_PID "OOOiOp:fork_exec",
        &process_args, &executable_list, &close_fds, &PyTuple_Type, &py_fds_to_keep, &cwd_obj, &env_list, &p2cread, &p2cwrite, &c2pread, &c2pwrite, &errread, &errwrite, &errpipe_read, &errpipe_write, &restore_signals, &call_setsid, &pgid_to_set, &gid_object, &groups_list, &uid_object, &child_umask, &preexec_fn, &allow_vfork)) {
        goto exit;
    }
    return_value = subprocess_fork_exec_impl(module, process_args, executable_list, close_fds, py_fds_to_keep, cwd_obj, env_list, p2cread, p2cwrite, c2pread, c2pwrite, errread, errwrite, errpipe_read, errpipe_write, restore_signals, call_setsid, pgid_to_set, gid_object, groups_list, uid_object, child_umask, preexec_fn, allow_vfork);

exit:
    return return_value;
}
/*[clinic end generated code: output=d0909dc97cac8f01 input=a9049054013a1b77]*/
