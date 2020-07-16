/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(os_stat__doc__,
"stat($module, /, path, *, dir_fd=None, follow_symlinks=True)\n"
"--\n"
"\n"
"Perform a stat system call on the given path.\n"
"\n"
"  path\n"
"    Path to be examined; can be string, bytes, a path-like object or\n"
"    open-file-descriptor int.\n"
"  dir_fd\n"
"    If not None, it should be a file descriptor open to a directory,\n"
"    and path should be a relative string; path will then be relative to\n"
"    that directory.\n"
"  follow_symlinks\n"
"    If False, and the last element of the path is a symbolic link,\n"
"    stat will examine the symbolic link itself instead of the file\n"
"    the link points to.\n"
"\n"
"dir_fd and follow_symlinks may not be implemented\n"
"  on your platform.  If they are unavailable, using them will raise a\n"
"  NotImplementedError.\n"
"\n"
"It\'s an error to use dir_fd or follow_symlinks when specifying path as\n"
"  an open file descriptor.");

#define OS_STAT_METHODDEF    \
    {"stat", (PyCFunction)(void(*)(void))os_stat, METH_FASTCALL|METH_KEYWORDS, os_stat__doc__},

static PyObject *
os_stat_impl(PyObject *module, path_t *path, int dir_fd, int follow_symlinks);

static PyObject *
os_stat(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "dir_fd", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "stat", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    path_t path = PATH_T_INITIALIZE("stat", "path", 0, 1);
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        if (!FSTATAT_DIR_FD_CONVERTER(args[1], &dir_fd)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    follow_symlinks = PyObject_IsTrue(args[2]);
    if (follow_symlinks < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_stat_impl(module, &path, dir_fd, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

PyDoc_STRVAR(os_lstat__doc__,
"lstat($module, /, path, *, dir_fd=None)\n"
"--\n"
"\n"
"Perform a stat system call on the given path, without following symbolic links.\n"
"\n"
"Like stat(), but do not follow symbolic links.\n"
"Equivalent to stat(path, follow_symlinks=False).");

#define OS_LSTAT_METHODDEF    \
    {"lstat", (PyCFunction)(void(*)(void))os_lstat, METH_FASTCALL|METH_KEYWORDS, os_lstat__doc__},

static PyObject *
os_lstat_impl(PyObject *module, path_t *path, int dir_fd);

static PyObject *
os_lstat(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "dir_fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "lstat", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    path_t path = PATH_T_INITIALIZE("lstat", "path", 0, 0);
    int dir_fd = DEFAULT_DIR_FD;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!FSTATAT_DIR_FD_CONVERTER(args[1], &dir_fd)) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_lstat_impl(module, &path, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

PyDoc_STRVAR(os_access__doc__,
"access($module, /, path, mode, *, dir_fd=None, effective_ids=False,\n"
"       follow_symlinks=True)\n"
"--\n"
"\n"
"Use the real uid/gid to test for access to a path.\n"
"\n"
"  path\n"
"    Path to be tested; can be string, bytes, or a path-like object.\n"
"  mode\n"
"    Operating-system mode bitfield.  Can be F_OK to test existence,\n"
"    or the inclusive-OR of R_OK, W_OK, and X_OK.\n"
"  dir_fd\n"
"    If not None, it should be a file descriptor open to a directory,\n"
"    and path should be relative; path will then be relative to that\n"
"    directory.\n"
"  effective_ids\n"
"    If True, access will use the effective uid/gid instead of\n"
"    the real uid/gid.\n"
"  follow_symlinks\n"
"    If False, and the last element of the path is a symbolic link,\n"
"    access will examine the symbolic link itself instead of the file\n"
"    the link points to.\n"
"\n"
"dir_fd, effective_ids, and follow_symlinks may not be implemented\n"
"  on your platform.  If they are unavailable, using them will raise a\n"
"  NotImplementedError.\n"
"\n"
"Note that most operations will use the effective uid/gid, therefore this\n"
"  routine can be used in a suid/sgid environment to test if the invoking user\n"
"  has the specified access to the path.");

#define OS_ACCESS_METHODDEF    \
    {"access", (PyCFunction)(void(*)(void))os_access, METH_FASTCALL|METH_KEYWORDS, os_access__doc__},

static int
os_access_impl(PyObject *module, path_t *path, int mode, int dir_fd,
               int effective_ids, int follow_symlinks);

static PyObject *
os_access(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "mode", "dir_fd", "effective_ids", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "access", 0};
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    path_t path = PATH_T_INITIALIZE("access", "path", 0, 0);
    int mode;
    int dir_fd = DEFAULT_DIR_FD;
    int effective_ids = 0;
    int follow_symlinks = 1;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    mode = _PyLong_AsInt(args[1]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[2]) {
        if (!FACCESSAT_DIR_FD_CONVERTER(args[2], &dir_fd)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[3]) {
        effective_ids = PyObject_IsTrue(args[3]);
        if (effective_ids < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    follow_symlinks = PyObject_IsTrue(args[4]);
    if (follow_symlinks < 0) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = os_access_impl(module, &path, mode, dir_fd, effective_ids, follow_symlinks);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#if defined(HAVE_TTYNAME)

PyDoc_STRVAR(os_ttyname__doc__,
"ttyname($module, fd, /)\n"
"--\n"
"\n"
"Return the name of the terminal device connected to \'fd\'.\n"
"\n"
"  fd\n"
"    Integer file descriptor handle.");

#define OS_TTYNAME_METHODDEF    \
    {"ttyname", (PyCFunction)os_ttyname, METH_O, os_ttyname__doc__},

static PyObject *
os_ttyname_impl(PyObject *module, int fd);

static PyObject *
os_ttyname(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;

    fd = _PyLong_AsInt(arg);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_ttyname_impl(module, fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_TTYNAME) */

#if defined(HAVE_CTERMID)

PyDoc_STRVAR(os_ctermid__doc__,
"ctermid($module, /)\n"
"--\n"
"\n"
"Return the name of the controlling terminal for this process.");

#define OS_CTERMID_METHODDEF    \
    {"ctermid", (PyCFunction)os_ctermid, METH_NOARGS, os_ctermid__doc__},

static PyObject *
os_ctermid_impl(PyObject *module);

static PyObject *
os_ctermid(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_ctermid_impl(module);
}

#endif /* defined(HAVE_CTERMID) */

PyDoc_STRVAR(os_chdir__doc__,
"chdir($module, /, path)\n"
"--\n"
"\n"
"Change the current working directory to the specified path.\n"
"\n"
"path may always be specified as a string.\n"
"On some platforms, path may also be specified as an open file descriptor.\n"
"  If this functionality is unavailable, using it raises an exception.");

#define OS_CHDIR_METHODDEF    \
    {"chdir", (PyCFunction)(void(*)(void))os_chdir, METH_FASTCALL|METH_KEYWORDS, os_chdir__doc__},

static PyObject *
os_chdir_impl(PyObject *module, path_t *path);

static PyObject *
os_chdir(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "chdir", 0};
    PyObject *argsbuf[1];
    path_t path = PATH_T_INITIALIZE("chdir", "path", 0, PATH_HAVE_FCHDIR);

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    return_value = os_chdir_impl(module, &path);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#if defined(HAVE_FCHDIR)

PyDoc_STRVAR(os_fchdir__doc__,
"fchdir($module, /, fd)\n"
"--\n"
"\n"
"Change to the directory of the given file descriptor.\n"
"\n"
"fd must be opened on a directory, not a file.\n"
"Equivalent to os.chdir(fd).");

#define OS_FCHDIR_METHODDEF    \
    {"fchdir", (PyCFunction)(void(*)(void))os_fchdir, METH_FASTCALL|METH_KEYWORDS, os_fchdir__doc__},

static PyObject *
os_fchdir_impl(PyObject *module, int fd);

static PyObject *
os_fchdir(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "fchdir", 0};
    PyObject *argsbuf[1];
    int fd;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!fildes_converter(args[0], &fd)) {
        goto exit;
    }
    return_value = os_fchdir_impl(module, fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_FCHDIR) */

PyDoc_STRVAR(os_chmod__doc__,
"chmod($module, /, path, mode, *, dir_fd=None, follow_symlinks=True)\n"
"--\n"
"\n"
"Change the access permissions of a file.\n"
"\n"
"  path\n"
"    Path to be modified.  May always be specified as a str, bytes, or a path-like object.\n"
"    On some platforms, path may also be specified as an open file descriptor.\n"
"    If this functionality is unavailable, using it raises an exception.\n"
"  mode\n"
"    Operating-system mode bitfield.\n"
"  dir_fd\n"
"    If not None, it should be a file descriptor open to a directory,\n"
"    and path should be relative; path will then be relative to that\n"
"    directory.\n"
"  follow_symlinks\n"
"    If False, and the last element of the path is a symbolic link,\n"
"    chmod will modify the symbolic link itself instead of the file\n"
"    the link points to.\n"
"\n"
"It is an error to use dir_fd or follow_symlinks when specifying path as\n"
"  an open file descriptor.\n"
"dir_fd and follow_symlinks may not be implemented on your platform.\n"
"  If they are unavailable, using them will raise a NotImplementedError.");

#define OS_CHMOD_METHODDEF    \
    {"chmod", (PyCFunction)(void(*)(void))os_chmod, METH_FASTCALL|METH_KEYWORDS, os_chmod__doc__},

static PyObject *
os_chmod_impl(PyObject *module, path_t *path, int mode, int dir_fd,
              int follow_symlinks);

static PyObject *
os_chmod(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "mode", "dir_fd", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "chmod", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    path_t path = PATH_T_INITIALIZE("chmod", "path", 0, PATH_HAVE_FCHMOD);
    int mode;
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    mode = _PyLong_AsInt(args[1]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[2]) {
        if (!FCHMODAT_DIR_FD_CONVERTER(args[2], &dir_fd)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    follow_symlinks = PyObject_IsTrue(args[3]);
    if (follow_symlinks < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_chmod_impl(module, &path, mode, dir_fd, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#if defined(HAVE_FCHMOD)

PyDoc_STRVAR(os_fchmod__doc__,
"fchmod($module, /, fd, mode)\n"
"--\n"
"\n"
"Change the access permissions of the file given by file descriptor fd.\n"
"\n"
"Equivalent to os.chmod(fd, mode).");

#define OS_FCHMOD_METHODDEF    \
    {"fchmod", (PyCFunction)(void(*)(void))os_fchmod, METH_FASTCALL|METH_KEYWORDS, os_fchmod__doc__},

static PyObject *
os_fchmod_impl(PyObject *module, int fd, int mode);

static PyObject *
os_fchmod(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", "mode", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "fchmod", 0};
    PyObject *argsbuf[2];
    int fd;
    int mode;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    mode = _PyLong_AsInt(args[1]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_fchmod_impl(module, fd, mode);

exit:
    return return_value;
}

#endif /* defined(HAVE_FCHMOD) */

#if defined(HAVE_LCHMOD)

PyDoc_STRVAR(os_lchmod__doc__,
"lchmod($module, /, path, mode)\n"
"--\n"
"\n"
"Change the access permissions of a file, without following symbolic links.\n"
"\n"
"If path is a symlink, this affects the link itself rather than the target.\n"
"Equivalent to chmod(path, mode, follow_symlinks=False).\"");

#define OS_LCHMOD_METHODDEF    \
    {"lchmod", (PyCFunction)(void(*)(void))os_lchmod, METH_FASTCALL|METH_KEYWORDS, os_lchmod__doc__},

static PyObject *
os_lchmod_impl(PyObject *module, path_t *path, int mode);

static PyObject *
os_lchmod(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "mode", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "lchmod", 0};
    PyObject *argsbuf[2];
    path_t path = PATH_T_INITIALIZE("lchmod", "path", 0, 0);
    int mode;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    mode = _PyLong_AsInt(args[1]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_lchmod_impl(module, &path, mode);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(HAVE_LCHMOD) */

#if defined(HAVE_CHFLAGS)

PyDoc_STRVAR(os_chflags__doc__,
"chflags($module, /, path, flags, follow_symlinks=True)\n"
"--\n"
"\n"
"Set file flags.\n"
"\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, chflags will change flags on the symbolic link itself instead of the\n"
"  file the link points to.\n"
"follow_symlinks may not be implemented on your platform.  If it is\n"
"unavailable, using it will raise a NotImplementedError.");

#define OS_CHFLAGS_METHODDEF    \
    {"chflags", (PyCFunction)(void(*)(void))os_chflags, METH_FASTCALL|METH_KEYWORDS, os_chflags__doc__},

static PyObject *
os_chflags_impl(PyObject *module, path_t *path, unsigned long flags,
                int follow_symlinks);

static PyObject *
os_chflags(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "flags", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "chflags", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    path_t path = PATH_T_INITIALIZE("chflags", "path", 0, 0);
    unsigned long flags;
    int follow_symlinks = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!PyLong_Check(args[1])) {
        _PyArg_BadArgument("chflags", "argument 'flags'", "int", args[1]);
        goto exit;
    }
    flags = PyLong_AsUnsignedLongMask(args[1]);
    if (!noptargs) {
        goto skip_optional_pos;
    }
    follow_symlinks = PyObject_IsTrue(args[2]);
    if (follow_symlinks < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = os_chflags_impl(module, &path, flags, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(HAVE_CHFLAGS) */

#if defined(HAVE_LCHFLAGS)

PyDoc_STRVAR(os_lchflags__doc__,
"lchflags($module, /, path, flags)\n"
"--\n"
"\n"
"Set file flags.\n"
"\n"
"This function will not follow symbolic links.\n"
"Equivalent to chflags(path, flags, follow_symlinks=False).");

#define OS_LCHFLAGS_METHODDEF    \
    {"lchflags", (PyCFunction)(void(*)(void))os_lchflags, METH_FASTCALL|METH_KEYWORDS, os_lchflags__doc__},

static PyObject *
os_lchflags_impl(PyObject *module, path_t *path, unsigned long flags);

static PyObject *
os_lchflags(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "flags", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "lchflags", 0};
    PyObject *argsbuf[2];
    path_t path = PATH_T_INITIALIZE("lchflags", "path", 0, 0);
    unsigned long flags;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!PyLong_Check(args[1])) {
        _PyArg_BadArgument("lchflags", "argument 'flags'", "int", args[1]);
        goto exit;
    }
    flags = PyLong_AsUnsignedLongMask(args[1]);
    return_value = os_lchflags_impl(module, &path, flags);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(HAVE_LCHFLAGS) */

#if defined(HAVE_CHROOT)

PyDoc_STRVAR(os_chroot__doc__,
"chroot($module, /, path)\n"
"--\n"
"\n"
"Change root directory to path.");

#define OS_CHROOT_METHODDEF    \
    {"chroot", (PyCFunction)(void(*)(void))os_chroot, METH_FASTCALL|METH_KEYWORDS, os_chroot__doc__},

static PyObject *
os_chroot_impl(PyObject *module, path_t *path);

static PyObject *
os_chroot(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "chroot", 0};
    PyObject *argsbuf[1];
    path_t path = PATH_T_INITIALIZE("chroot", "path", 0, 0);

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    return_value = os_chroot_impl(module, &path);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(HAVE_CHROOT) */

#if defined(HAVE_FSYNC)

PyDoc_STRVAR(os_fsync__doc__,
"fsync($module, /, fd)\n"
"--\n"
"\n"
"Force write of fd to disk.");

#define OS_FSYNC_METHODDEF    \
    {"fsync", (PyCFunction)(void(*)(void))os_fsync, METH_FASTCALL|METH_KEYWORDS, os_fsync__doc__},

static PyObject *
os_fsync_impl(PyObject *module, int fd);

static PyObject *
os_fsync(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "fsync", 0};
    PyObject *argsbuf[1];
    int fd;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!fildes_converter(args[0], &fd)) {
        goto exit;
    }
    return_value = os_fsync_impl(module, fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_FSYNC) */

#if defined(HAVE_SYNC)

PyDoc_STRVAR(os_sync__doc__,
"sync($module, /)\n"
"--\n"
"\n"
"Force write of everything to disk.");

#define OS_SYNC_METHODDEF    \
    {"sync", (PyCFunction)os_sync, METH_NOARGS, os_sync__doc__},

static PyObject *
os_sync_impl(PyObject *module);

static PyObject *
os_sync(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_sync_impl(module);
}

#endif /* defined(HAVE_SYNC) */

#if defined(HAVE_FDATASYNC)

PyDoc_STRVAR(os_fdatasync__doc__,
"fdatasync($module, /, fd)\n"
"--\n"
"\n"
"Force write of fd to disk without forcing update of metadata.");

#define OS_FDATASYNC_METHODDEF    \
    {"fdatasync", (PyCFunction)(void(*)(void))os_fdatasync, METH_FASTCALL|METH_KEYWORDS, os_fdatasync__doc__},

static PyObject *
os_fdatasync_impl(PyObject *module, int fd);

static PyObject *
os_fdatasync(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "fdatasync", 0};
    PyObject *argsbuf[1];
    int fd;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!fildes_converter(args[0], &fd)) {
        goto exit;
    }
    return_value = os_fdatasync_impl(module, fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_FDATASYNC) */

#if defined(HAVE_CHOWN)

PyDoc_STRVAR(os_chown__doc__,
"chown($module, /, path, uid, gid, *, dir_fd=None, follow_symlinks=True)\n"
"--\n"
"\n"
"Change the owner and group id of path to the numeric uid and gid.\\\n"
"\n"
"  path\n"
"    Path to be examined; can be string, bytes, a path-like object, or open-file-descriptor int.\n"
"  dir_fd\n"
"    If not None, it should be a file descriptor open to a directory,\n"
"    and path should be relative; path will then be relative to that\n"
"    directory.\n"
"  follow_symlinks\n"
"    If False, and the last element of the path is a symbolic link,\n"
"    stat will examine the symbolic link itself instead of the file\n"
"    the link points to.\n"
"\n"
"path may always be specified as a string.\n"
"On some platforms, path may also be specified as an open file descriptor.\n"
"  If this functionality is unavailable, using it raises an exception.\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, chown will modify the symbolic link itself instead of the file the\n"
"  link points to.\n"
"It is an error to use dir_fd or follow_symlinks when specifying path as\n"
"  an open file descriptor.\n"
"dir_fd and follow_symlinks may not be implemented on your platform.\n"
"  If they are unavailable, using them will raise a NotImplementedError.");

#define OS_CHOWN_METHODDEF    \
    {"chown", (PyCFunction)(void(*)(void))os_chown, METH_FASTCALL|METH_KEYWORDS, os_chown__doc__},

static PyObject *
os_chown_impl(PyObject *module, path_t *path, uid_t uid, gid_t gid,
              int dir_fd, int follow_symlinks);

static PyObject *
os_chown(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "uid", "gid", "dir_fd", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "chown", 0};
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    path_t path = PATH_T_INITIALIZE("chown", "path", 0, PATH_HAVE_FCHOWN);
    uid_t uid;
    gid_t gid;
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!_Py_Uid_Converter(args[1], &uid)) {
        goto exit;
    }
    if (!_Py_Gid_Converter(args[2], &gid)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[3]) {
        if (!FCHOWNAT_DIR_FD_CONVERTER(args[3], &dir_fd)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    follow_symlinks = PyObject_IsTrue(args[4]);
    if (follow_symlinks < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_chown_impl(module, &path, uid, gid, dir_fd, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(HAVE_CHOWN) */

#if defined(HAVE_FCHOWN)

PyDoc_STRVAR(os_fchown__doc__,
"fchown($module, /, fd, uid, gid)\n"
"--\n"
"\n"
"Change the owner and group id of the file specified by file descriptor.\n"
"\n"
"Equivalent to os.chown(fd, uid, gid).");

#define OS_FCHOWN_METHODDEF    \
    {"fchown", (PyCFunction)(void(*)(void))os_fchown, METH_FASTCALL|METH_KEYWORDS, os_fchown__doc__},

static PyObject *
os_fchown_impl(PyObject *module, int fd, uid_t uid, gid_t gid);

static PyObject *
os_fchown(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", "uid", "gid", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "fchown", 0};
    PyObject *argsbuf[3];
    int fd;
    uid_t uid;
    gid_t gid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!_Py_Uid_Converter(args[1], &uid)) {
        goto exit;
    }
    if (!_Py_Gid_Converter(args[2], &gid)) {
        goto exit;
    }
    return_value = os_fchown_impl(module, fd, uid, gid);

exit:
    return return_value;
}

#endif /* defined(HAVE_FCHOWN) */

#if defined(HAVE_LCHOWN)

PyDoc_STRVAR(os_lchown__doc__,
"lchown($module, /, path, uid, gid)\n"
"--\n"
"\n"
"Change the owner and group id of path to the numeric uid and gid.\n"
"\n"
"This function will not follow symbolic links.\n"
"Equivalent to os.chown(path, uid, gid, follow_symlinks=False).");

#define OS_LCHOWN_METHODDEF    \
    {"lchown", (PyCFunction)(void(*)(void))os_lchown, METH_FASTCALL|METH_KEYWORDS, os_lchown__doc__},

static PyObject *
os_lchown_impl(PyObject *module, path_t *path, uid_t uid, gid_t gid);

static PyObject *
os_lchown(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "uid", "gid", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "lchown", 0};
    PyObject *argsbuf[3];
    path_t path = PATH_T_INITIALIZE("lchown", "path", 0, 0);
    uid_t uid;
    gid_t gid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!_Py_Uid_Converter(args[1], &uid)) {
        goto exit;
    }
    if (!_Py_Gid_Converter(args[2], &gid)) {
        goto exit;
    }
    return_value = os_lchown_impl(module, &path, uid, gid);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(HAVE_LCHOWN) */

PyDoc_STRVAR(os_getcwd__doc__,
"getcwd($module, /)\n"
"--\n"
"\n"
"Return a unicode string representing the current working directory.");

#define OS_GETCWD_METHODDEF    \
    {"getcwd", (PyCFunction)os_getcwd, METH_NOARGS, os_getcwd__doc__},

static PyObject *
os_getcwd_impl(PyObject *module);

static PyObject *
os_getcwd(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_getcwd_impl(module);
}

PyDoc_STRVAR(os_getcwdb__doc__,
"getcwdb($module, /)\n"
"--\n"
"\n"
"Return a bytes string representing the current working directory.");

#define OS_GETCWDB_METHODDEF    \
    {"getcwdb", (PyCFunction)os_getcwdb, METH_NOARGS, os_getcwdb__doc__},

static PyObject *
os_getcwdb_impl(PyObject *module);

static PyObject *
os_getcwdb(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_getcwdb_impl(module);
}

#if defined(HAVE_LINK)

PyDoc_STRVAR(os_link__doc__,
"link($module, /, src, dst, *, src_dir_fd=None, dst_dir_fd=None,\n"
"     follow_symlinks=True)\n"
"--\n"
"\n"
"Create a hard link to a file.\n"
"\n"
"If either src_dir_fd or dst_dir_fd is not None, it should be a file\n"
"  descriptor open to a directory, and the respective path string (src or dst)\n"
"  should be relative; the path will then be relative to that directory.\n"
"If follow_symlinks is False, and the last element of src is a symbolic\n"
"  link, link will create a link to the symbolic link itself instead of the\n"
"  file the link points to.\n"
"src_dir_fd, dst_dir_fd, and follow_symlinks may not be implemented on your\n"
"  platform.  If they are unavailable, using them will raise a\n"
"  NotImplementedError.");

#define OS_LINK_METHODDEF    \
    {"link", (PyCFunction)(void(*)(void))os_link, METH_FASTCALL|METH_KEYWORDS, os_link__doc__},

static PyObject *
os_link_impl(PyObject *module, path_t *src, path_t *dst, int src_dir_fd,
             int dst_dir_fd, int follow_symlinks);

static PyObject *
os_link(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"src", "dst", "src_dir_fd", "dst_dir_fd", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "link", 0};
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    path_t src = PATH_T_INITIALIZE("link", "src", 0, 0);
    path_t dst = PATH_T_INITIALIZE("link", "dst", 0, 0);
    int src_dir_fd = DEFAULT_DIR_FD;
    int dst_dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &src)) {
        goto exit;
    }
    if (!path_converter(args[1], &dst)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[2]) {
        if (!dir_fd_converter(args[2], &src_dir_fd)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[3]) {
        if (!dir_fd_converter(args[3], &dst_dir_fd)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    follow_symlinks = PyObject_IsTrue(args[4]);
    if (follow_symlinks < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_link_impl(module, &src, &dst, src_dir_fd, dst_dir_fd, follow_symlinks);

exit:
    /* Cleanup for src */
    path_cleanup(&src);
    /* Cleanup for dst */
    path_cleanup(&dst);

    return return_value;
}

#endif /* defined(HAVE_LINK) */

PyDoc_STRVAR(os_listdir__doc__,
"listdir($module, /, path=None)\n"
"--\n"
"\n"
"Return a list containing the names of the files in the directory.\n"
"\n"
"path can be specified as either str, bytes, or a path-like object.  If path is bytes,\n"
"  the filenames returned will also be bytes; in all other circumstances\n"
"  the filenames returned will be str.\n"
"If path is None, uses the path=\'.\'.\n"
"On some platforms, path may also be specified as an open file descriptor;\\\n"
"  the file descriptor must refer to a directory.\n"
"  If this functionality is unavailable, using it raises NotImplementedError.\n"
"\n"
"The list is in arbitrary order.  It does not include the special\n"
"entries \'.\' and \'..\' even if they are present in the directory.");

#define OS_LISTDIR_METHODDEF    \
    {"listdir", (PyCFunction)(void(*)(void))os_listdir, METH_FASTCALL|METH_KEYWORDS, os_listdir__doc__},

static PyObject *
os_listdir_impl(PyObject *module, path_t *path);

static PyObject *
os_listdir(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "listdir", 0};
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    path_t path = PATH_T_INITIALIZE("listdir", "path", 1, PATH_HAVE_FDOPENDIR);

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
skip_optional_pos:
    return_value = os_listdir_impl(module, &path);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(os__getfullpathname__doc__,
"_getfullpathname($module, path, /)\n"
"--\n"
"\n");

#define OS__GETFULLPATHNAME_METHODDEF    \
    {"_getfullpathname", (PyCFunction)os__getfullpathname, METH_O, os__getfullpathname__doc__},

static PyObject *
os__getfullpathname_impl(PyObject *module, path_t *path);

static PyObject *
os__getfullpathname(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    path_t path = PATH_T_INITIALIZE("_getfullpathname", "path", 0, 0);

    if (!path_converter(arg, &path)) {
        goto exit;
    }
    return_value = os__getfullpathname_impl(module, &path);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(os__getfinalpathname__doc__,
"_getfinalpathname($module, path, /)\n"
"--\n"
"\n"
"A helper function for samepath on windows.");

#define OS__GETFINALPATHNAME_METHODDEF    \
    {"_getfinalpathname", (PyCFunction)os__getfinalpathname, METH_O, os__getfinalpathname__doc__},

static PyObject *
os__getfinalpathname_impl(PyObject *module, path_t *path);

static PyObject *
os__getfinalpathname(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    path_t path = PATH_T_INITIALIZE("_getfinalpathname", "path", 0, 0);

    if (!path_converter(arg, &path)) {
        goto exit;
    }
    return_value = os__getfinalpathname_impl(module, &path);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(os__getvolumepathname__doc__,
"_getvolumepathname($module, /, path)\n"
"--\n"
"\n"
"A helper function for ismount on Win32.");

#define OS__GETVOLUMEPATHNAME_METHODDEF    \
    {"_getvolumepathname", (PyCFunction)(void(*)(void))os__getvolumepathname, METH_FASTCALL|METH_KEYWORDS, os__getvolumepathname__doc__},

static PyObject *
os__getvolumepathname_impl(PyObject *module, path_t *path);

static PyObject *
os__getvolumepathname(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "_getvolumepathname", 0};
    PyObject *argsbuf[1];
    path_t path = PATH_T_INITIALIZE("_getvolumepathname", "path", 0, 0);

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    return_value = os__getvolumepathname_impl(module, &path);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

PyDoc_STRVAR(os_mkdir__doc__,
"mkdir($module, /, path, mode=511, *, dir_fd=None)\n"
"--\n"
"\n"
"Create a directory.\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.\n"
"\n"
"The mode argument is ignored on Windows.");

#define OS_MKDIR_METHODDEF    \
    {"mkdir", (PyCFunction)(void(*)(void))os_mkdir, METH_FASTCALL|METH_KEYWORDS, os_mkdir__doc__},

static PyObject *
os_mkdir_impl(PyObject *module, path_t *path, int mode, int dir_fd);

static PyObject *
os_mkdir(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "mode", "dir_fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "mkdir", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    path_t path = PATH_T_INITIALIZE("mkdir", "path", 0, 0);
    int mode = 511;
    int dir_fd = DEFAULT_DIR_FD;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        mode = _PyLong_AsInt(args[1]);
        if (mode == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!MKDIRAT_DIR_FD_CONVERTER(args[2], &dir_fd)) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_mkdir_impl(module, &path, mode, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#if defined(HAVE_NICE)

PyDoc_STRVAR(os_nice__doc__,
"nice($module, increment, /)\n"
"--\n"
"\n"
"Add increment to the priority of process and return the new priority.");

#define OS_NICE_METHODDEF    \
    {"nice", (PyCFunction)os_nice, METH_O, os_nice__doc__},

static PyObject *
os_nice_impl(PyObject *module, int increment);

static PyObject *
os_nice(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int increment;

    increment = _PyLong_AsInt(arg);
    if (increment == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_nice_impl(module, increment);

exit:
    return return_value;
}

#endif /* defined(HAVE_NICE) */

#if defined(HAVE_GETPRIORITY)

PyDoc_STRVAR(os_getpriority__doc__,
"getpriority($module, /, which, who)\n"
"--\n"
"\n"
"Return program scheduling priority.");

#define OS_GETPRIORITY_METHODDEF    \
    {"getpriority", (PyCFunction)(void(*)(void))os_getpriority, METH_FASTCALL|METH_KEYWORDS, os_getpriority__doc__},

static PyObject *
os_getpriority_impl(PyObject *module, int which, int who);

static PyObject *
os_getpriority(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"which", "who", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "getpriority", 0};
    PyObject *argsbuf[2];
    int which;
    int who;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    which = _PyLong_AsInt(args[0]);
    if (which == -1 && PyErr_Occurred()) {
        goto exit;
    }
    who = _PyLong_AsInt(args[1]);
    if (who == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_getpriority_impl(module, which, who);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETPRIORITY) */

#if defined(HAVE_SETPRIORITY)

PyDoc_STRVAR(os_setpriority__doc__,
"setpriority($module, /, which, who, priority)\n"
"--\n"
"\n"
"Set program scheduling priority.");

#define OS_SETPRIORITY_METHODDEF    \
    {"setpriority", (PyCFunction)(void(*)(void))os_setpriority, METH_FASTCALL|METH_KEYWORDS, os_setpriority__doc__},

static PyObject *
os_setpriority_impl(PyObject *module, int which, int who, int priority);

static PyObject *
os_setpriority(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"which", "who", "priority", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "setpriority", 0};
    PyObject *argsbuf[3];
    int which;
    int who;
    int priority;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    which = _PyLong_AsInt(args[0]);
    if (which == -1 && PyErr_Occurred()) {
        goto exit;
    }
    who = _PyLong_AsInt(args[1]);
    if (who == -1 && PyErr_Occurred()) {
        goto exit;
    }
    priority = _PyLong_AsInt(args[2]);
    if (priority == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_setpriority_impl(module, which, who, priority);

exit:
    return return_value;
}

#endif /* defined(HAVE_SETPRIORITY) */

PyDoc_STRVAR(os_rename__doc__,
"rename($module, /, src, dst, *, src_dir_fd=None, dst_dir_fd=None)\n"
"--\n"
"\n"
"Rename a file or directory.\n"
"\n"
"If either src_dir_fd or dst_dir_fd is not None, it should be a file\n"
"  descriptor open to a directory, and the respective path string (src or dst)\n"
"  should be relative; the path will then be relative to that directory.\n"
"src_dir_fd and dst_dir_fd, may not be implemented on your platform.\n"
"  If they are unavailable, using them will raise a NotImplementedError.");

#define OS_RENAME_METHODDEF    \
    {"rename", (PyCFunction)(void(*)(void))os_rename, METH_FASTCALL|METH_KEYWORDS, os_rename__doc__},

static PyObject *
os_rename_impl(PyObject *module, path_t *src, path_t *dst, int src_dir_fd,
               int dst_dir_fd);

static PyObject *
os_rename(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"src", "dst", "src_dir_fd", "dst_dir_fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "rename", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    path_t src = PATH_T_INITIALIZE("rename", "src", 0, 0);
    path_t dst = PATH_T_INITIALIZE("rename", "dst", 0, 0);
    int src_dir_fd = DEFAULT_DIR_FD;
    int dst_dir_fd = DEFAULT_DIR_FD;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &src)) {
        goto exit;
    }
    if (!path_converter(args[1], &dst)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[2]) {
        if (!dir_fd_converter(args[2], &src_dir_fd)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!dir_fd_converter(args[3], &dst_dir_fd)) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_rename_impl(module, &src, &dst, src_dir_fd, dst_dir_fd);

exit:
    /* Cleanup for src */
    path_cleanup(&src);
    /* Cleanup for dst */
    path_cleanup(&dst);

    return return_value;
}

PyDoc_STRVAR(os_replace__doc__,
"replace($module, /, src, dst, *, src_dir_fd=None, dst_dir_fd=None)\n"
"--\n"
"\n"
"Rename a file or directory, overwriting the destination.\n"
"\n"
"If either src_dir_fd or dst_dir_fd is not None, it should be a file\n"
"  descriptor open to a directory, and the respective path string (src or dst)\n"
"  should be relative; the path will then be relative to that directory.\n"
"src_dir_fd and dst_dir_fd, may not be implemented on your platform.\n"
"  If they are unavailable, using them will raise a NotImplementedError.");

#define OS_REPLACE_METHODDEF    \
    {"replace", (PyCFunction)(void(*)(void))os_replace, METH_FASTCALL|METH_KEYWORDS, os_replace__doc__},

static PyObject *
os_replace_impl(PyObject *module, path_t *src, path_t *dst, int src_dir_fd,
                int dst_dir_fd);

static PyObject *
os_replace(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"src", "dst", "src_dir_fd", "dst_dir_fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "replace", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    path_t src = PATH_T_INITIALIZE("replace", "src", 0, 0);
    path_t dst = PATH_T_INITIALIZE("replace", "dst", 0, 0);
    int src_dir_fd = DEFAULT_DIR_FD;
    int dst_dir_fd = DEFAULT_DIR_FD;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &src)) {
        goto exit;
    }
    if (!path_converter(args[1], &dst)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[2]) {
        if (!dir_fd_converter(args[2], &src_dir_fd)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!dir_fd_converter(args[3], &dst_dir_fd)) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_replace_impl(module, &src, &dst, src_dir_fd, dst_dir_fd);

exit:
    /* Cleanup for src */
    path_cleanup(&src);
    /* Cleanup for dst */
    path_cleanup(&dst);

    return return_value;
}

PyDoc_STRVAR(os_rmdir__doc__,
"rmdir($module, /, path, *, dir_fd=None)\n"
"--\n"
"\n"
"Remove a directory.\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.");

#define OS_RMDIR_METHODDEF    \
    {"rmdir", (PyCFunction)(void(*)(void))os_rmdir, METH_FASTCALL|METH_KEYWORDS, os_rmdir__doc__},

static PyObject *
os_rmdir_impl(PyObject *module, path_t *path, int dir_fd);

static PyObject *
os_rmdir(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "dir_fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "rmdir", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    path_t path = PATH_T_INITIALIZE("rmdir", "path", 0, 0);
    int dir_fd = DEFAULT_DIR_FD;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!UNLINKAT_DIR_FD_CONVERTER(args[1], &dir_fd)) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_rmdir_impl(module, &path, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#if defined(HAVE_SYSTEM) && defined(MS_WINDOWS)

PyDoc_STRVAR(os_system__doc__,
"system($module, /, command)\n"
"--\n"
"\n"
"Execute the command in a subshell.");

#define OS_SYSTEM_METHODDEF    \
    {"system", (PyCFunction)(void(*)(void))os_system, METH_FASTCALL|METH_KEYWORDS, os_system__doc__},

static long
os_system_impl(PyObject *module, const Py_UNICODE *command);

static PyObject *
os_system(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"command", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "system", 0};
    PyObject *argsbuf[1];
    const Py_UNICODE *command;
    long _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("system", "argument 'command'", "str", args[0]);
        goto exit;
    }
    #if USE_UNICODE_WCHAR_CACHE
    command = _PyUnicode_AsUnicode(args[0]);
    #else /* USE_UNICODE_WCHAR_CACHE */
    command = PyUnicode_AsWideCharString(args[0], NULL);
    #endif /* USE_UNICODE_WCHAR_CACHE */
    if (command == NULL) {
        goto exit;
    }
    _return_value = os_system_impl(module, command);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    /* Cleanup for command */
    #if !USE_UNICODE_WCHAR_CACHE
    PyMem_Free((void *)command);
    #endif /* USE_UNICODE_WCHAR_CACHE */

    return return_value;
}

#endif /* defined(HAVE_SYSTEM) && defined(MS_WINDOWS) */

#if defined(HAVE_SYSTEM) && !defined(MS_WINDOWS)

PyDoc_STRVAR(os_system__doc__,
"system($module, /, command)\n"
"--\n"
"\n"
"Execute the command in a subshell.");

#define OS_SYSTEM_METHODDEF    \
    {"system", (PyCFunction)(void(*)(void))os_system, METH_FASTCALL|METH_KEYWORDS, os_system__doc__},

static long
os_system_impl(PyObject *module, PyObject *command);

static PyObject *
os_system(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"command", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "system", 0};
    PyObject *argsbuf[1];
    PyObject *command = NULL;
    long _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_FSConverter(args[0], &command)) {
        goto exit;
    }
    _return_value = os_system_impl(module, command);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    /* Cleanup for command */
    Py_XDECREF(command);

    return return_value;
}

#endif /* defined(HAVE_SYSTEM) && !defined(MS_WINDOWS) */

PyDoc_STRVAR(os_umask__doc__,
"umask($module, mask, /)\n"
"--\n"
"\n"
"Set the current numeric umask and return the previous umask.");

#define OS_UMASK_METHODDEF    \
    {"umask", (PyCFunction)os_umask, METH_O, os_umask__doc__},

static PyObject *
os_umask_impl(PyObject *module, int mask);

static PyObject *
os_umask(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int mask;

    mask = _PyLong_AsInt(arg);
    if (mask == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_umask_impl(module, mask);

exit:
    return return_value;
}

PyDoc_STRVAR(os_unlink__doc__,
"unlink($module, /, path, *, dir_fd=None)\n"
"--\n"
"\n"
"Remove a file (same as remove()).\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.");

#define OS_UNLINK_METHODDEF    \
    {"unlink", (PyCFunction)(void(*)(void))os_unlink, METH_FASTCALL|METH_KEYWORDS, os_unlink__doc__},

static PyObject *
os_unlink_impl(PyObject *module, path_t *path, int dir_fd);

static PyObject *
os_unlink(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "dir_fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "unlink", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    path_t path = PATH_T_INITIALIZE("unlink", "path", 0, 0);
    int dir_fd = DEFAULT_DIR_FD;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!UNLINKAT_DIR_FD_CONVERTER(args[1], &dir_fd)) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_unlink_impl(module, &path, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

PyDoc_STRVAR(os_remove__doc__,
"remove($module, /, path, *, dir_fd=None)\n"
"--\n"
"\n"
"Remove a file (same as unlink()).\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.");

#define OS_REMOVE_METHODDEF    \
    {"remove", (PyCFunction)(void(*)(void))os_remove, METH_FASTCALL|METH_KEYWORDS, os_remove__doc__},

static PyObject *
os_remove_impl(PyObject *module, path_t *path, int dir_fd);

static PyObject *
os_remove(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "dir_fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "remove", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    path_t path = PATH_T_INITIALIZE("remove", "path", 0, 0);
    int dir_fd = DEFAULT_DIR_FD;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!UNLINKAT_DIR_FD_CONVERTER(args[1], &dir_fd)) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_remove_impl(module, &path, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#if defined(HAVE_UNAME)

PyDoc_STRVAR(os_uname__doc__,
"uname($module, /)\n"
"--\n"
"\n"
"Return an object identifying the current operating system.\n"
"\n"
"The object behaves like a named tuple with the following fields:\n"
"  (sysname, nodename, release, version, machine)");

#define OS_UNAME_METHODDEF    \
    {"uname", (PyCFunction)os_uname, METH_NOARGS, os_uname__doc__},

static PyObject *
os_uname_impl(PyObject *module);

static PyObject *
os_uname(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_uname_impl(module);
}

#endif /* defined(HAVE_UNAME) */

PyDoc_STRVAR(os_utime__doc__,
"utime($module, /, path, times=None, *, ns=<unrepresentable>,\n"
"      dir_fd=None, follow_symlinks=True)\n"
"--\n"
"\n"
"Set the access and modified time of path.\n"
"\n"
"path may always be specified as a string.\n"
"On some platforms, path may also be specified as an open file descriptor.\n"
"  If this functionality is unavailable, using it raises an exception.\n"
"\n"
"If times is not None, it must be a tuple (atime, mtime);\n"
"    atime and mtime should be expressed as float seconds since the epoch.\n"
"If ns is specified, it must be a tuple (atime_ns, mtime_ns);\n"
"    atime_ns and mtime_ns should be expressed as integer nanoseconds\n"
"    since the epoch.\n"
"If times is None and ns is unspecified, utime uses the current time.\n"
"Specifying tuples for both times and ns is an error.\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, utime will modify the symbolic link itself instead of the file the\n"
"  link points to.\n"
"It is an error to use dir_fd or follow_symlinks when specifying path\n"
"  as an open file descriptor.\n"
"dir_fd and follow_symlinks may not be available on your platform.\n"
"  If they are unavailable, using them will raise a NotImplementedError.");

#define OS_UTIME_METHODDEF    \
    {"utime", (PyCFunction)(void(*)(void))os_utime, METH_FASTCALL|METH_KEYWORDS, os_utime__doc__},

static PyObject *
os_utime_impl(PyObject *module, path_t *path, PyObject *times, PyObject *ns,
              int dir_fd, int follow_symlinks);

static PyObject *
os_utime(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "times", "ns", "dir_fd", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "utime", 0};
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    path_t path = PATH_T_INITIALIZE("utime", "path", 0, PATH_UTIME_HAVE_FD);
    PyObject *times = Py_None;
    PyObject *ns = NULL;
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        times = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[2]) {
        ns = args[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[3]) {
        if (!FUTIMENSAT_DIR_FD_CONVERTER(args[3], &dir_fd)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    follow_symlinks = PyObject_IsTrue(args[4]);
    if (follow_symlinks < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_utime_impl(module, &path, times, ns, dir_fd, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

PyDoc_STRVAR(os__exit__doc__,
"_exit($module, /, status)\n"
"--\n"
"\n"
"Exit to the system with specified status, without normal exit processing.");

#define OS__EXIT_METHODDEF    \
    {"_exit", (PyCFunction)(void(*)(void))os__exit, METH_FASTCALL|METH_KEYWORDS, os__exit__doc__},

static PyObject *
os__exit_impl(PyObject *module, int status);

static PyObject *
os__exit(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "_exit", 0};
    PyObject *argsbuf[1];
    int status;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    status = _PyLong_AsInt(args[0]);
    if (status == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os__exit_impl(module, status);

exit:
    return return_value;
}

#if defined(HAVE_EXECV)

PyDoc_STRVAR(os_execv__doc__,
"execv($module, path, argv, /)\n"
"--\n"
"\n"
"Execute an executable path with arguments, replacing current process.\n"
"\n"
"  path\n"
"    Path of executable file.\n"
"  argv\n"
"    Tuple or list of strings.");

#define OS_EXECV_METHODDEF    \
    {"execv", (PyCFunction)(void(*)(void))os_execv, METH_FASTCALL, os_execv__doc__},

static PyObject *
os_execv_impl(PyObject *module, path_t *path, PyObject *argv);

static PyObject *
os_execv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    path_t path = PATH_T_INITIALIZE("execv", "path", 0, 0);
    PyObject *argv;

    if (!_PyArg_CheckPositional("execv", nargs, 2, 2)) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    argv = args[1];
    return_value = os_execv_impl(module, &path, argv);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(HAVE_EXECV) */

#if defined(HAVE_EXECV)

PyDoc_STRVAR(os_execve__doc__,
"execve($module, /, path, argv, env)\n"
"--\n"
"\n"
"Execute an executable path with arguments, replacing current process.\n"
"\n"
"  path\n"
"    Path of executable file.\n"
"  argv\n"
"    Tuple or list of strings.\n"
"  env\n"
"    Dictionary of strings mapping to strings.");

#define OS_EXECVE_METHODDEF    \
    {"execve", (PyCFunction)(void(*)(void))os_execve, METH_FASTCALL|METH_KEYWORDS, os_execve__doc__},

static PyObject *
os_execve_impl(PyObject *module, path_t *path, PyObject *argv, PyObject *env);

static PyObject *
os_execve(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "argv", "env", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "execve", 0};
    PyObject *argsbuf[3];
    path_t path = PATH_T_INITIALIZE("execve", "path", 0, PATH_HAVE_FEXECVE);
    PyObject *argv;
    PyObject *env;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    argv = args[1];
    env = args[2];
    return_value = os_execve_impl(module, &path, argv, env);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(HAVE_EXECV) */

#if defined(HAVE_POSIX_SPAWN)

PyDoc_STRVAR(os_posix_spawn__doc__,
"posix_spawn($module, path, argv, env, /, *, file_actions=(),\n"
"            setpgroup=<unrepresentable>, resetids=False, setsid=False,\n"
"            setsigmask=(), setsigdef=(), scheduler=<unrepresentable>)\n"
"--\n"
"\n"
"Execute the program specified by path in a new process.\n"
"\n"
"  path\n"
"    Path of executable file.\n"
"  argv\n"
"    Tuple or list of strings.\n"
"  env\n"
"    Dictionary of strings mapping to strings.\n"
"  file_actions\n"
"    A sequence of file action tuples.\n"
"  setpgroup\n"
"    The pgroup to use with the POSIX_SPAWN_SETPGROUP flag.\n"
"  resetids\n"
"    If the value is `true` the POSIX_SPAWN_RESETIDS will be activated.\n"
"  setsid\n"
"    If the value is `true` the POSIX_SPAWN_SETSID or POSIX_SPAWN_SETSID_NP will be activated.\n"
"  setsigmask\n"
"    The sigmask to use with the POSIX_SPAWN_SETSIGMASK flag.\n"
"  setsigdef\n"
"    The sigmask to use with the POSIX_SPAWN_SETSIGDEF flag.\n"
"  scheduler\n"
"    A tuple with the scheduler policy (optional) and parameters.");

#define OS_POSIX_SPAWN_METHODDEF    \
    {"posix_spawn", (PyCFunction)(void(*)(void))os_posix_spawn, METH_FASTCALL|METH_KEYWORDS, os_posix_spawn__doc__},

static PyObject *
os_posix_spawn_impl(PyObject *module, path_t *path, PyObject *argv,
                    PyObject *env, PyObject *file_actions,
                    PyObject *setpgroup, int resetids, int setsid,
                    PyObject *setsigmask, PyObject *setsigdef,
                    PyObject *scheduler);

static PyObject *
os_posix_spawn(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "", "", "file_actions", "setpgroup", "resetids", "setsid", "setsigmask", "setsigdef", "scheduler", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "posix_spawn", 0};
    PyObject *argsbuf[10];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    path_t path = PATH_T_INITIALIZE("posix_spawn", "path", 0, 0);
    PyObject *argv;
    PyObject *env;
    PyObject *file_actions = NULL;
    PyObject *setpgroup = NULL;
    int resetids = 0;
    int setsid = 0;
    PyObject *setsigmask = NULL;
    PyObject *setsigdef = NULL;
    PyObject *scheduler = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    argv = args[1];
    env = args[2];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[3]) {
        file_actions = args[3];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[4]) {
        setpgroup = args[4];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[5]) {
        resetids = _PyLong_AsInt(args[5]);
        if (resetids == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[6]) {
        setsid = _PyLong_AsInt(args[6]);
        if (setsid == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[7]) {
        setsigmask = args[7];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[8]) {
        setsigdef = args[8];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    scheduler = args[9];
skip_optional_kwonly:
    return_value = os_posix_spawn_impl(module, &path, argv, env, file_actions, setpgroup, resetids, setsid, setsigmask, setsigdef, scheduler);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(HAVE_POSIX_SPAWN) */

#if defined(HAVE_POSIX_SPAWNP)

PyDoc_STRVAR(os_posix_spawnp__doc__,
"posix_spawnp($module, path, argv, env, /, *, file_actions=(),\n"
"             setpgroup=<unrepresentable>, resetids=False, setsid=False,\n"
"             setsigmask=(), setsigdef=(), scheduler=<unrepresentable>)\n"
"--\n"
"\n"
"Execute the program specified by path in a new process.\n"
"\n"
"  path\n"
"    Path of executable file.\n"
"  argv\n"
"    Tuple or list of strings.\n"
"  env\n"
"    Dictionary of strings mapping to strings.\n"
"  file_actions\n"
"    A sequence of file action tuples.\n"
"  setpgroup\n"
"    The pgroup to use with the POSIX_SPAWN_SETPGROUP flag.\n"
"  resetids\n"
"    If the value is `True` the POSIX_SPAWN_RESETIDS will be activated.\n"
"  setsid\n"
"    If the value is `True` the POSIX_SPAWN_SETSID or POSIX_SPAWN_SETSID_NP will be activated.\n"
"  setsigmask\n"
"    The sigmask to use with the POSIX_SPAWN_SETSIGMASK flag.\n"
"  setsigdef\n"
"    The sigmask to use with the POSIX_SPAWN_SETSIGDEF flag.\n"
"  scheduler\n"
"    A tuple with the scheduler policy (optional) and parameters.");

#define OS_POSIX_SPAWNP_METHODDEF    \
    {"posix_spawnp", (PyCFunction)(void(*)(void))os_posix_spawnp, METH_FASTCALL|METH_KEYWORDS, os_posix_spawnp__doc__},

static PyObject *
os_posix_spawnp_impl(PyObject *module, path_t *path, PyObject *argv,
                     PyObject *env, PyObject *file_actions,
                     PyObject *setpgroup, int resetids, int setsid,
                     PyObject *setsigmask, PyObject *setsigdef,
                     PyObject *scheduler);

static PyObject *
os_posix_spawnp(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "", "", "file_actions", "setpgroup", "resetids", "setsid", "setsigmask", "setsigdef", "scheduler", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "posix_spawnp", 0};
    PyObject *argsbuf[10];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    path_t path = PATH_T_INITIALIZE("posix_spawnp", "path", 0, 0);
    PyObject *argv;
    PyObject *env;
    PyObject *file_actions = NULL;
    PyObject *setpgroup = NULL;
    int resetids = 0;
    int setsid = 0;
    PyObject *setsigmask = NULL;
    PyObject *setsigdef = NULL;
    PyObject *scheduler = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    argv = args[1];
    env = args[2];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[3]) {
        file_actions = args[3];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[4]) {
        setpgroup = args[4];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[5]) {
        resetids = _PyLong_AsInt(args[5]);
        if (resetids == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[6]) {
        setsid = _PyLong_AsInt(args[6]);
        if (setsid == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[7]) {
        setsigmask = args[7];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[8]) {
        setsigdef = args[8];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    scheduler = args[9];
skip_optional_kwonly:
    return_value = os_posix_spawnp_impl(module, &path, argv, env, file_actions, setpgroup, resetids, setsid, setsigmask, setsigdef, scheduler);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(HAVE_POSIX_SPAWNP) */

#if (defined(HAVE_SPAWNV) || defined(HAVE_WSPAWNV) || defined(HAVE_RTPSPAWN))

PyDoc_STRVAR(os_spawnv__doc__,
"spawnv($module, mode, path, argv, /)\n"
"--\n"
"\n"
"Execute the program specified by path in a new process.\n"
"\n"
"  mode\n"
"    Mode of process creation.\n"
"  path\n"
"    Path of executable file.\n"
"  argv\n"
"    Tuple or list of strings.");

#define OS_SPAWNV_METHODDEF    \
    {"spawnv", (PyCFunction)(void(*)(void))os_spawnv, METH_FASTCALL, os_spawnv__doc__},

static PyObject *
os_spawnv_impl(PyObject *module, int mode, path_t *path, PyObject *argv);

static PyObject *
os_spawnv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int mode;
    path_t path = PATH_T_INITIALIZE("spawnv", "path", 0, 0);
    PyObject *argv;

    if (!_PyArg_CheckPositional("spawnv", nargs, 3, 3)) {
        goto exit;
    }
    mode = _PyLong_AsInt(args[0]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!path_converter(args[1], &path)) {
        goto exit;
    }
    argv = args[2];
    return_value = os_spawnv_impl(module, mode, &path, argv);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* (defined(HAVE_SPAWNV) || defined(HAVE_WSPAWNV) || defined(HAVE_RTPSPAWN)) */

#if (defined(HAVE_SPAWNV) || defined(HAVE_WSPAWNV) || defined(HAVE_RTPSPAWN))

PyDoc_STRVAR(os_spawnve__doc__,
"spawnve($module, mode, path, argv, env, /)\n"
"--\n"
"\n"
"Execute the program specified by path in a new process.\n"
"\n"
"  mode\n"
"    Mode of process creation.\n"
"  path\n"
"    Path of executable file.\n"
"  argv\n"
"    Tuple or list of strings.\n"
"  env\n"
"    Dictionary of strings mapping to strings.");

#define OS_SPAWNVE_METHODDEF    \
    {"spawnve", (PyCFunction)(void(*)(void))os_spawnve, METH_FASTCALL, os_spawnve__doc__},

static PyObject *
os_spawnve_impl(PyObject *module, int mode, path_t *path, PyObject *argv,
                PyObject *env);

static PyObject *
os_spawnve(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int mode;
    path_t path = PATH_T_INITIALIZE("spawnve", "path", 0, 0);
    PyObject *argv;
    PyObject *env;

    if (!_PyArg_CheckPositional("spawnve", nargs, 4, 4)) {
        goto exit;
    }
    mode = _PyLong_AsInt(args[0]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!path_converter(args[1], &path)) {
        goto exit;
    }
    argv = args[2];
    env = args[3];
    return_value = os_spawnve_impl(module, mode, &path, argv, env);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* (defined(HAVE_SPAWNV) || defined(HAVE_WSPAWNV) || defined(HAVE_RTPSPAWN)) */

#if defined(HAVE_FORK)

PyDoc_STRVAR(os_register_at_fork__doc__,
"register_at_fork($module, /, *, before=<unrepresentable>,\n"
"                 after_in_child=<unrepresentable>,\n"
"                 after_in_parent=<unrepresentable>)\n"
"--\n"
"\n"
"Register callables to be called when forking a new process.\n"
"\n"
"  before\n"
"    A callable to be called in the parent before the fork() syscall.\n"
"  after_in_child\n"
"    A callable to be called in the child after fork().\n"
"  after_in_parent\n"
"    A callable to be called in the parent after fork().\n"
"\n"
"\'before\' callbacks are called in reverse order.\n"
"\'after_in_child\' and \'after_in_parent\' callbacks are called in order.");

#define OS_REGISTER_AT_FORK_METHODDEF    \
    {"register_at_fork", (PyCFunction)(void(*)(void))os_register_at_fork, METH_FASTCALL|METH_KEYWORDS, os_register_at_fork__doc__},

static PyObject *
os_register_at_fork_impl(PyObject *module, PyObject *before,
                         PyObject *after_in_child, PyObject *after_in_parent);

static PyObject *
os_register_at_fork(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"before", "after_in_child", "after_in_parent", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "register_at_fork", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *before = NULL;
    PyObject *after_in_child = NULL;
    PyObject *after_in_parent = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 0, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[0]) {
        before = args[0];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[1]) {
        after_in_child = args[1];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    after_in_parent = args[2];
skip_optional_kwonly:
    return_value = os_register_at_fork_impl(module, before, after_in_child, after_in_parent);

exit:
    return return_value;
}

#endif /* defined(HAVE_FORK) */

#if defined(HAVE_FORK1)

PyDoc_STRVAR(os_fork1__doc__,
"fork1($module, /)\n"
"--\n"
"\n"
"Fork a child process with a single multiplexed (i.e., not bound) thread.\n"
"\n"
"Return 0 to child process and PID of child to parent process.");

#define OS_FORK1_METHODDEF    \
    {"fork1", (PyCFunction)os_fork1, METH_NOARGS, os_fork1__doc__},

static PyObject *
os_fork1_impl(PyObject *module);

static PyObject *
os_fork1(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_fork1_impl(module);
}

#endif /* defined(HAVE_FORK1) */

#if defined(HAVE_FORK)

PyDoc_STRVAR(os_fork__doc__,
"fork($module, /)\n"
"--\n"
"\n"
"Fork a child process.\n"
"\n"
"Return 0 to child process and PID of child to parent process.");

#define OS_FORK_METHODDEF    \
    {"fork", (PyCFunction)os_fork, METH_NOARGS, os_fork__doc__},

static PyObject *
os_fork_impl(PyObject *module);

static PyObject *
os_fork(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_fork_impl(module);
}

#endif /* defined(HAVE_FORK) */

#if defined(HAVE_SCHED_H) && defined(HAVE_SCHED_GET_PRIORITY_MAX)

PyDoc_STRVAR(os_sched_get_priority_max__doc__,
"sched_get_priority_max($module, /, policy)\n"
"--\n"
"\n"
"Get the maximum scheduling priority for policy.");

#define OS_SCHED_GET_PRIORITY_MAX_METHODDEF    \
    {"sched_get_priority_max", (PyCFunction)(void(*)(void))os_sched_get_priority_max, METH_FASTCALL|METH_KEYWORDS, os_sched_get_priority_max__doc__},

static PyObject *
os_sched_get_priority_max_impl(PyObject *module, int policy);

static PyObject *
os_sched_get_priority_max(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"policy", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "sched_get_priority_max", 0};
    PyObject *argsbuf[1];
    int policy;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    policy = _PyLong_AsInt(args[0]);
    if (policy == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_sched_get_priority_max_impl(module, policy);

exit:
    return return_value;
}

#endif /* defined(HAVE_SCHED_H) && defined(HAVE_SCHED_GET_PRIORITY_MAX) */

#if defined(HAVE_SCHED_H) && defined(HAVE_SCHED_GET_PRIORITY_MAX)

PyDoc_STRVAR(os_sched_get_priority_min__doc__,
"sched_get_priority_min($module, /, policy)\n"
"--\n"
"\n"
"Get the minimum scheduling priority for policy.");

#define OS_SCHED_GET_PRIORITY_MIN_METHODDEF    \
    {"sched_get_priority_min", (PyCFunction)(void(*)(void))os_sched_get_priority_min, METH_FASTCALL|METH_KEYWORDS, os_sched_get_priority_min__doc__},

static PyObject *
os_sched_get_priority_min_impl(PyObject *module, int policy);

static PyObject *
os_sched_get_priority_min(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"policy", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "sched_get_priority_min", 0};
    PyObject *argsbuf[1];
    int policy;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    policy = _PyLong_AsInt(args[0]);
    if (policy == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_sched_get_priority_min_impl(module, policy);

exit:
    return return_value;
}

#endif /* defined(HAVE_SCHED_H) && defined(HAVE_SCHED_GET_PRIORITY_MAX) */

#if defined(HAVE_SCHED_H) && defined(HAVE_SCHED_SETSCHEDULER)

PyDoc_STRVAR(os_sched_getscheduler__doc__,
"sched_getscheduler($module, pid, /)\n"
"--\n"
"\n"
"Get the scheduling policy for the process identified by pid.\n"
"\n"
"Passing 0 for pid returns the scheduling policy for the calling process.");

#define OS_SCHED_GETSCHEDULER_METHODDEF    \
    {"sched_getscheduler", (PyCFunction)os_sched_getscheduler, METH_O, os_sched_getscheduler__doc__},

static PyObject *
os_sched_getscheduler_impl(PyObject *module, pid_t pid);

static PyObject *
os_sched_getscheduler(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    pid_t pid;

    if (!PyArg_Parse(arg, "" _Py_PARSE_PID ":sched_getscheduler", &pid)) {
        goto exit;
    }
    return_value = os_sched_getscheduler_impl(module, pid);

exit:
    return return_value;
}

#endif /* defined(HAVE_SCHED_H) && defined(HAVE_SCHED_SETSCHEDULER) */

#if defined(HAVE_SCHED_H) && (defined(HAVE_SCHED_SETPARAM) || defined(HAVE_SCHED_SETSCHEDULER) || defined(POSIX_SPAWN_SETSCHEDULER) || defined(POSIX_SPAWN_SETSCHEDPARAM))

PyDoc_STRVAR(os_sched_param__doc__,
"sched_param(sched_priority)\n"
"--\n"
"\n"
"Currently has only one field: sched_priority\n"
"\n"
"  sched_priority\n"
"    A scheduling parameter.");

static PyObject *
os_sched_param_impl(PyTypeObject *type, PyObject *sched_priority);

static PyObject *
os_sched_param(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"sched_priority", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "sched_param", 0};
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *sched_priority;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    sched_priority = fastargs[0];
    return_value = os_sched_param_impl(type, sched_priority);

exit:
    return return_value;
}

#endif /* defined(HAVE_SCHED_H) && (defined(HAVE_SCHED_SETPARAM) || defined(HAVE_SCHED_SETSCHEDULER) || defined(POSIX_SPAWN_SETSCHEDULER) || defined(POSIX_SPAWN_SETSCHEDPARAM)) */

#if defined(HAVE_SCHED_H) && defined(HAVE_SCHED_SETSCHEDULER)

PyDoc_STRVAR(os_sched_setscheduler__doc__,
"sched_setscheduler($module, pid, policy, param, /)\n"
"--\n"
"\n"
"Set the scheduling policy for the process identified by pid.\n"
"\n"
"If pid is 0, the calling process is changed.\n"
"param is an instance of sched_param.");

#define OS_SCHED_SETSCHEDULER_METHODDEF    \
    {"sched_setscheduler", (PyCFunction)(void(*)(void))os_sched_setscheduler, METH_FASTCALL, os_sched_setscheduler__doc__},

static PyObject *
os_sched_setscheduler_impl(PyObject *module, pid_t pid, int policy,
                           PyObject *param_obj);

static PyObject *
os_sched_setscheduler(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    pid_t pid;
    int policy;
    PyObject *param_obj;

    if (!_PyArg_ParseStack(args, nargs, "" _Py_PARSE_PID "iO:sched_setscheduler",
        &pid, &policy, &param_obj)) {
        goto exit;
    }
    return_value = os_sched_setscheduler_impl(module, pid, policy, param_obj);

exit:
    return return_value;
}

#endif /* defined(HAVE_SCHED_H) && defined(HAVE_SCHED_SETSCHEDULER) */

#if defined(HAVE_SCHED_H) && defined(HAVE_SCHED_SETPARAM)

PyDoc_STRVAR(os_sched_getparam__doc__,
"sched_getparam($module, pid, /)\n"
"--\n"
"\n"
"Returns scheduling parameters for the process identified by pid.\n"
"\n"
"If pid is 0, returns parameters for the calling process.\n"
"Return value is an instance of sched_param.");

#define OS_SCHED_GETPARAM_METHODDEF    \
    {"sched_getparam", (PyCFunction)os_sched_getparam, METH_O, os_sched_getparam__doc__},

static PyObject *
os_sched_getparam_impl(PyObject *module, pid_t pid);

static PyObject *
os_sched_getparam(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    pid_t pid;

    if (!PyArg_Parse(arg, "" _Py_PARSE_PID ":sched_getparam", &pid)) {
        goto exit;
    }
    return_value = os_sched_getparam_impl(module, pid);

exit:
    return return_value;
}

#endif /* defined(HAVE_SCHED_H) && defined(HAVE_SCHED_SETPARAM) */

#if defined(HAVE_SCHED_H) && defined(HAVE_SCHED_SETPARAM)

PyDoc_STRVAR(os_sched_setparam__doc__,
"sched_setparam($module, pid, param, /)\n"
"--\n"
"\n"
"Set scheduling parameters for the process identified by pid.\n"
"\n"
"If pid is 0, sets parameters for the calling process.\n"
"param should be an instance of sched_param.");

#define OS_SCHED_SETPARAM_METHODDEF    \
    {"sched_setparam", (PyCFunction)(void(*)(void))os_sched_setparam, METH_FASTCALL, os_sched_setparam__doc__},

static PyObject *
os_sched_setparam_impl(PyObject *module, pid_t pid, PyObject *param_obj);

static PyObject *
os_sched_setparam(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    pid_t pid;
    PyObject *param_obj;

    if (!_PyArg_ParseStack(args, nargs, "" _Py_PARSE_PID "O:sched_setparam",
        &pid, &param_obj)) {
        goto exit;
    }
    return_value = os_sched_setparam_impl(module, pid, param_obj);

exit:
    return return_value;
}

#endif /* defined(HAVE_SCHED_H) && defined(HAVE_SCHED_SETPARAM) */

#if defined(HAVE_SCHED_H) && defined(HAVE_SCHED_RR_GET_INTERVAL)

PyDoc_STRVAR(os_sched_rr_get_interval__doc__,
"sched_rr_get_interval($module, pid, /)\n"
"--\n"
"\n"
"Return the round-robin quantum for the process identified by pid, in seconds.\n"
"\n"
"Value returned is a float.");

#define OS_SCHED_RR_GET_INTERVAL_METHODDEF    \
    {"sched_rr_get_interval", (PyCFunction)os_sched_rr_get_interval, METH_O, os_sched_rr_get_interval__doc__},

static double
os_sched_rr_get_interval_impl(PyObject *module, pid_t pid);

static PyObject *
os_sched_rr_get_interval(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    pid_t pid;
    double _return_value;

    if (!PyArg_Parse(arg, "" _Py_PARSE_PID ":sched_rr_get_interval", &pid)) {
        goto exit;
    }
    _return_value = os_sched_rr_get_interval_impl(module, pid);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_SCHED_H) && defined(HAVE_SCHED_RR_GET_INTERVAL) */

#if defined(HAVE_SCHED_H)

PyDoc_STRVAR(os_sched_yield__doc__,
"sched_yield($module, /)\n"
"--\n"
"\n"
"Voluntarily relinquish the CPU.");

#define OS_SCHED_YIELD_METHODDEF    \
    {"sched_yield", (PyCFunction)os_sched_yield, METH_NOARGS, os_sched_yield__doc__},

static PyObject *
os_sched_yield_impl(PyObject *module);

static PyObject *
os_sched_yield(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_sched_yield_impl(module);
}

#endif /* defined(HAVE_SCHED_H) */

#if defined(HAVE_SCHED_H) && defined(HAVE_SCHED_SETAFFINITY)

PyDoc_STRVAR(os_sched_setaffinity__doc__,
"sched_setaffinity($module, pid, mask, /)\n"
"--\n"
"\n"
"Set the CPU affinity of the process identified by pid to mask.\n"
"\n"
"mask should be an iterable of integers identifying CPUs.");

#define OS_SCHED_SETAFFINITY_METHODDEF    \
    {"sched_setaffinity", (PyCFunction)(void(*)(void))os_sched_setaffinity, METH_FASTCALL, os_sched_setaffinity__doc__},

static PyObject *
os_sched_setaffinity_impl(PyObject *module, pid_t pid, PyObject *mask);

static PyObject *
os_sched_setaffinity(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    pid_t pid;
    PyObject *mask;

    if (!_PyArg_ParseStack(args, nargs, "" _Py_PARSE_PID "O:sched_setaffinity",
        &pid, &mask)) {
        goto exit;
    }
    return_value = os_sched_setaffinity_impl(module, pid, mask);

exit:
    return return_value;
}

#endif /* defined(HAVE_SCHED_H) && defined(HAVE_SCHED_SETAFFINITY) */

#if defined(HAVE_SCHED_H) && defined(HAVE_SCHED_SETAFFINITY)

PyDoc_STRVAR(os_sched_getaffinity__doc__,
"sched_getaffinity($module, pid, /)\n"
"--\n"
"\n"
"Return the affinity of the process identified by pid (or the current process if zero).\n"
"\n"
"The affinity is returned as a set of CPU identifiers.");

#define OS_SCHED_GETAFFINITY_METHODDEF    \
    {"sched_getaffinity", (PyCFunction)os_sched_getaffinity, METH_O, os_sched_getaffinity__doc__},

static PyObject *
os_sched_getaffinity_impl(PyObject *module, pid_t pid);

static PyObject *
os_sched_getaffinity(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    pid_t pid;

    if (!PyArg_Parse(arg, "" _Py_PARSE_PID ":sched_getaffinity", &pid)) {
        goto exit;
    }
    return_value = os_sched_getaffinity_impl(module, pid);

exit:
    return return_value;
}

#endif /* defined(HAVE_SCHED_H) && defined(HAVE_SCHED_SETAFFINITY) */

#if (defined(HAVE_OPENPTY) || defined(HAVE__GETPTY) || defined(HAVE_DEV_PTMX))

PyDoc_STRVAR(os_openpty__doc__,
"openpty($module, /)\n"
"--\n"
"\n"
"Open a pseudo-terminal.\n"
"\n"
"Return a tuple of (master_fd, slave_fd) containing open file descriptors\n"
"for both the master and slave ends.");

#define OS_OPENPTY_METHODDEF    \
    {"openpty", (PyCFunction)os_openpty, METH_NOARGS, os_openpty__doc__},

static PyObject *
os_openpty_impl(PyObject *module);

static PyObject *
os_openpty(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_openpty_impl(module);
}

#endif /* (defined(HAVE_OPENPTY) || defined(HAVE__GETPTY) || defined(HAVE_DEV_PTMX)) */

#if defined(HAVE_FORKPTY)

PyDoc_STRVAR(os_forkpty__doc__,
"forkpty($module, /)\n"
"--\n"
"\n"
"Fork a new process with a new pseudo-terminal as controlling tty.\n"
"\n"
"Returns a tuple of (pid, master_fd).\n"
"Like fork(), return pid of 0 to the child process,\n"
"and pid of child to the parent process.\n"
"To both, return fd of newly opened pseudo-terminal.");

#define OS_FORKPTY_METHODDEF    \
    {"forkpty", (PyCFunction)os_forkpty, METH_NOARGS, os_forkpty__doc__},

static PyObject *
os_forkpty_impl(PyObject *module);

static PyObject *
os_forkpty(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_forkpty_impl(module);
}

#endif /* defined(HAVE_FORKPTY) */

#if defined(HAVE_GETEGID)

PyDoc_STRVAR(os_getegid__doc__,
"getegid($module, /)\n"
"--\n"
"\n"
"Return the current process\'s effective group id.");

#define OS_GETEGID_METHODDEF    \
    {"getegid", (PyCFunction)os_getegid, METH_NOARGS, os_getegid__doc__},

static PyObject *
os_getegid_impl(PyObject *module);

static PyObject *
os_getegid(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_getegid_impl(module);
}

#endif /* defined(HAVE_GETEGID) */

#if defined(HAVE_GETEUID)

PyDoc_STRVAR(os_geteuid__doc__,
"geteuid($module, /)\n"
"--\n"
"\n"
"Return the current process\'s effective user id.");

#define OS_GETEUID_METHODDEF    \
    {"geteuid", (PyCFunction)os_geteuid, METH_NOARGS, os_geteuid__doc__},

static PyObject *
os_geteuid_impl(PyObject *module);

static PyObject *
os_geteuid(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_geteuid_impl(module);
}

#endif /* defined(HAVE_GETEUID) */

#if defined(HAVE_GETGID)

PyDoc_STRVAR(os_getgid__doc__,
"getgid($module, /)\n"
"--\n"
"\n"
"Return the current process\'s group id.");

#define OS_GETGID_METHODDEF    \
    {"getgid", (PyCFunction)os_getgid, METH_NOARGS, os_getgid__doc__},

static PyObject *
os_getgid_impl(PyObject *module);

static PyObject *
os_getgid(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_getgid_impl(module);
}

#endif /* defined(HAVE_GETGID) */

#if defined(HAVE_GETPID)

PyDoc_STRVAR(os_getpid__doc__,
"getpid($module, /)\n"
"--\n"
"\n"
"Return the current process id.");

#define OS_GETPID_METHODDEF    \
    {"getpid", (PyCFunction)os_getpid, METH_NOARGS, os_getpid__doc__},

static PyObject *
os_getpid_impl(PyObject *module);

static PyObject *
os_getpid(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_getpid_impl(module);
}

#endif /* defined(HAVE_GETPID) */

#if defined(HAVE_GETGROUPLIST) && defined(__APPLE__)

PyDoc_STRVAR(os_getgrouplist__doc__,
"getgrouplist($module, user, group, /)\n"
"--\n"
"\n"
"Returns a list of groups to which a user belongs.\n"
"\n"
"  user\n"
"    username to lookup\n"
"  group\n"
"    base group id of the user");

#define OS_GETGROUPLIST_METHODDEF    \
    {"getgrouplist", (PyCFunction)(void(*)(void))os_getgrouplist, METH_FASTCALL, os_getgrouplist__doc__},

static PyObject *
os_getgrouplist_impl(PyObject *module, const char *user, int basegid);

static PyObject *
os_getgrouplist(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *user;
    int basegid;

    if (!_PyArg_CheckPositional("getgrouplist", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("getgrouplist", "argument 1", "str", args[0]);
        goto exit;
    }
    Py_ssize_t user_length;
    user = PyUnicode_AsUTF8AndSize(args[0], &user_length);
    if (user == NULL) {
        goto exit;
    }
    if (strlen(user) != (size_t)user_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    basegid = _PyLong_AsInt(args[1]);
    if (basegid == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_getgrouplist_impl(module, user, basegid);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETGROUPLIST) && defined(__APPLE__) */

#if defined(HAVE_GETGROUPLIST) && !defined(__APPLE__)

PyDoc_STRVAR(os_getgrouplist__doc__,
"getgrouplist($module, user, group, /)\n"
"--\n"
"\n"
"Returns a list of groups to which a user belongs.\n"
"\n"
"  user\n"
"    username to lookup\n"
"  group\n"
"    base group id of the user");

#define OS_GETGROUPLIST_METHODDEF    \
    {"getgrouplist", (PyCFunction)(void(*)(void))os_getgrouplist, METH_FASTCALL, os_getgrouplist__doc__},

static PyObject *
os_getgrouplist_impl(PyObject *module, const char *user, gid_t basegid);

static PyObject *
os_getgrouplist(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *user;
    gid_t basegid;

    if (!_PyArg_CheckPositional("getgrouplist", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("getgrouplist", "argument 1", "str", args[0]);
        goto exit;
    }
    Py_ssize_t user_length;
    user = PyUnicode_AsUTF8AndSize(args[0], &user_length);
    if (user == NULL) {
        goto exit;
    }
    if (strlen(user) != (size_t)user_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (!_Py_Gid_Converter(args[1], &basegid)) {
        goto exit;
    }
    return_value = os_getgrouplist_impl(module, user, basegid);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETGROUPLIST) && !defined(__APPLE__) */

#if defined(HAVE_GETGROUPS)

PyDoc_STRVAR(os_getgroups__doc__,
"getgroups($module, /)\n"
"--\n"
"\n"
"Return list of supplemental group IDs for the process.");

#define OS_GETGROUPS_METHODDEF    \
    {"getgroups", (PyCFunction)os_getgroups, METH_NOARGS, os_getgroups__doc__},

static PyObject *
os_getgroups_impl(PyObject *module);

static PyObject *
os_getgroups(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_getgroups_impl(module);
}

#endif /* defined(HAVE_GETGROUPS) */

#if defined(HAVE_INITGROUPS) && defined(__APPLE__)

PyDoc_STRVAR(os_initgroups__doc__,
"initgroups($module, username, gid, /)\n"
"--\n"
"\n"
"Initialize the group access list.\n"
"\n"
"Call the system initgroups() to initialize the group access list with all of\n"
"the groups of which the specified username is a member, plus the specified\n"
"group id.");

#define OS_INITGROUPS_METHODDEF    \
    {"initgroups", (PyCFunction)(void(*)(void))os_initgroups, METH_FASTCALL, os_initgroups__doc__},

static PyObject *
os_initgroups_impl(PyObject *module, PyObject *oname, int gid);

static PyObject *
os_initgroups(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *oname = NULL;
    int gid;

    if (!_PyArg_CheckPositional("initgroups", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_FSConverter(args[0], &oname)) {
        goto exit;
    }
    gid = _PyLong_AsInt(args[1]);
    if (gid == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_initgroups_impl(module, oname, gid);

exit:
    /* Cleanup for oname */
    Py_XDECREF(oname);

    return return_value;
}

#endif /* defined(HAVE_INITGROUPS) && defined(__APPLE__) */

#if defined(HAVE_INITGROUPS) && !defined(__APPLE__)

PyDoc_STRVAR(os_initgroups__doc__,
"initgroups($module, username, gid, /)\n"
"--\n"
"\n"
"Initialize the group access list.\n"
"\n"
"Call the system initgroups() to initialize the group access list with all of\n"
"the groups of which the specified username is a member, plus the specified\n"
"group id.");

#define OS_INITGROUPS_METHODDEF    \
    {"initgroups", (PyCFunction)(void(*)(void))os_initgroups, METH_FASTCALL, os_initgroups__doc__},

static PyObject *
os_initgroups_impl(PyObject *module, PyObject *oname, gid_t gid);

static PyObject *
os_initgroups(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *oname = NULL;
    gid_t gid;

    if (!_PyArg_CheckPositional("initgroups", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_FSConverter(args[0], &oname)) {
        goto exit;
    }
    if (!_Py_Gid_Converter(args[1], &gid)) {
        goto exit;
    }
    return_value = os_initgroups_impl(module, oname, gid);

exit:
    /* Cleanup for oname */
    Py_XDECREF(oname);

    return return_value;
}

#endif /* defined(HAVE_INITGROUPS) && !defined(__APPLE__) */

#if defined(HAVE_GETPGID)

PyDoc_STRVAR(os_getpgid__doc__,
"getpgid($module, /, pid)\n"
"--\n"
"\n"
"Call the system call getpgid(), and return the result.");

#define OS_GETPGID_METHODDEF    \
    {"getpgid", (PyCFunction)(void(*)(void))os_getpgid, METH_FASTCALL|METH_KEYWORDS, os_getpgid__doc__},

static PyObject *
os_getpgid_impl(PyObject *module, pid_t pid);

static PyObject *
os_getpgid(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"pid", NULL};
    static _PyArg_Parser _parser = {"" _Py_PARSE_PID ":getpgid", _keywords, 0};
    pid_t pid;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &pid)) {
        goto exit;
    }
    return_value = os_getpgid_impl(module, pid);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETPGID) */

#if defined(HAVE_GETPGRP)

PyDoc_STRVAR(os_getpgrp__doc__,
"getpgrp($module, /)\n"
"--\n"
"\n"
"Return the current process group id.");

#define OS_GETPGRP_METHODDEF    \
    {"getpgrp", (PyCFunction)os_getpgrp, METH_NOARGS, os_getpgrp__doc__},

static PyObject *
os_getpgrp_impl(PyObject *module);

static PyObject *
os_getpgrp(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_getpgrp_impl(module);
}

#endif /* defined(HAVE_GETPGRP) */

#if defined(HAVE_SETPGRP)

PyDoc_STRVAR(os_setpgrp__doc__,
"setpgrp($module, /)\n"
"--\n"
"\n"
"Make the current process the leader of its process group.");

#define OS_SETPGRP_METHODDEF    \
    {"setpgrp", (PyCFunction)os_setpgrp, METH_NOARGS, os_setpgrp__doc__},

static PyObject *
os_setpgrp_impl(PyObject *module);

static PyObject *
os_setpgrp(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_setpgrp_impl(module);
}

#endif /* defined(HAVE_SETPGRP) */

#if defined(HAVE_GETPPID)

PyDoc_STRVAR(os_getppid__doc__,
"getppid($module, /)\n"
"--\n"
"\n"
"Return the parent\'s process id.\n"
"\n"
"If the parent process has already exited, Windows machines will still\n"
"return its id; others systems will return the id of the \'init\' process (1).");

#define OS_GETPPID_METHODDEF    \
    {"getppid", (PyCFunction)os_getppid, METH_NOARGS, os_getppid__doc__},

static PyObject *
os_getppid_impl(PyObject *module);

static PyObject *
os_getppid(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_getppid_impl(module);
}

#endif /* defined(HAVE_GETPPID) */

#if defined(HAVE_GETLOGIN)

PyDoc_STRVAR(os_getlogin__doc__,
"getlogin($module, /)\n"
"--\n"
"\n"
"Return the actual login name.");

#define OS_GETLOGIN_METHODDEF    \
    {"getlogin", (PyCFunction)os_getlogin, METH_NOARGS, os_getlogin__doc__},

static PyObject *
os_getlogin_impl(PyObject *module);

static PyObject *
os_getlogin(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_getlogin_impl(module);
}

#endif /* defined(HAVE_GETLOGIN) */

#if defined(HAVE_GETUID)

PyDoc_STRVAR(os_getuid__doc__,
"getuid($module, /)\n"
"--\n"
"\n"
"Return the current process\'s user id.");

#define OS_GETUID_METHODDEF    \
    {"getuid", (PyCFunction)os_getuid, METH_NOARGS, os_getuid__doc__},

static PyObject *
os_getuid_impl(PyObject *module);

static PyObject *
os_getuid(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_getuid_impl(module);
}

#endif /* defined(HAVE_GETUID) */

#if defined(HAVE_KILL)

PyDoc_STRVAR(os_kill__doc__,
"kill($module, pid, signal, /)\n"
"--\n"
"\n"
"Kill a process with a signal.");

#define OS_KILL_METHODDEF    \
    {"kill", (PyCFunction)(void(*)(void))os_kill, METH_FASTCALL, os_kill__doc__},

static PyObject *
os_kill_impl(PyObject *module, pid_t pid, Py_ssize_t signal);

static PyObject *
os_kill(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    pid_t pid;
    Py_ssize_t signal;

    if (!_PyArg_ParseStack(args, nargs, "" _Py_PARSE_PID "n:kill",
        &pid, &signal)) {
        goto exit;
    }
    return_value = os_kill_impl(module, pid, signal);

exit:
    return return_value;
}

#endif /* defined(HAVE_KILL) */

#if defined(HAVE_KILLPG)

PyDoc_STRVAR(os_killpg__doc__,
"killpg($module, pgid, signal, /)\n"
"--\n"
"\n"
"Kill a process group with a signal.");

#define OS_KILLPG_METHODDEF    \
    {"killpg", (PyCFunction)(void(*)(void))os_killpg, METH_FASTCALL, os_killpg__doc__},

static PyObject *
os_killpg_impl(PyObject *module, pid_t pgid, int signal);

static PyObject *
os_killpg(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    pid_t pgid;
    int signal;

    if (!_PyArg_ParseStack(args, nargs, "" _Py_PARSE_PID "i:killpg",
        &pgid, &signal)) {
        goto exit;
    }
    return_value = os_killpg_impl(module, pgid, signal);

exit:
    return return_value;
}

#endif /* defined(HAVE_KILLPG) */

#if defined(HAVE_PLOCK)

PyDoc_STRVAR(os_plock__doc__,
"plock($module, op, /)\n"
"--\n"
"\n"
"Lock program segments into memory.\");");

#define OS_PLOCK_METHODDEF    \
    {"plock", (PyCFunction)os_plock, METH_O, os_plock__doc__},

static PyObject *
os_plock_impl(PyObject *module, int op);

static PyObject *
os_plock(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int op;

    op = _PyLong_AsInt(arg);
    if (op == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_plock_impl(module, op);

exit:
    return return_value;
}

#endif /* defined(HAVE_PLOCK) */

#if defined(HAVE_SETUID)

PyDoc_STRVAR(os_setuid__doc__,
"setuid($module, uid, /)\n"
"--\n"
"\n"
"Set the current process\'s user id.");

#define OS_SETUID_METHODDEF    \
    {"setuid", (PyCFunction)os_setuid, METH_O, os_setuid__doc__},

static PyObject *
os_setuid_impl(PyObject *module, uid_t uid);

static PyObject *
os_setuid(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    uid_t uid;

    if (!_Py_Uid_Converter(arg, &uid)) {
        goto exit;
    }
    return_value = os_setuid_impl(module, uid);

exit:
    return return_value;
}

#endif /* defined(HAVE_SETUID) */

#if defined(HAVE_SETEUID)

PyDoc_STRVAR(os_seteuid__doc__,
"seteuid($module, euid, /)\n"
"--\n"
"\n"
"Set the current process\'s effective user id.");

#define OS_SETEUID_METHODDEF    \
    {"seteuid", (PyCFunction)os_seteuid, METH_O, os_seteuid__doc__},

static PyObject *
os_seteuid_impl(PyObject *module, uid_t euid);

static PyObject *
os_seteuid(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    uid_t euid;

    if (!_Py_Uid_Converter(arg, &euid)) {
        goto exit;
    }
    return_value = os_seteuid_impl(module, euid);

exit:
    return return_value;
}

#endif /* defined(HAVE_SETEUID) */

#if defined(HAVE_SETEGID)

PyDoc_STRVAR(os_setegid__doc__,
"setegid($module, egid, /)\n"
"--\n"
"\n"
"Set the current process\'s effective group id.");

#define OS_SETEGID_METHODDEF    \
    {"setegid", (PyCFunction)os_setegid, METH_O, os_setegid__doc__},

static PyObject *
os_setegid_impl(PyObject *module, gid_t egid);

static PyObject *
os_setegid(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    gid_t egid;

    if (!_Py_Gid_Converter(arg, &egid)) {
        goto exit;
    }
    return_value = os_setegid_impl(module, egid);

exit:
    return return_value;
}

#endif /* defined(HAVE_SETEGID) */

#if defined(HAVE_SETREUID)

PyDoc_STRVAR(os_setreuid__doc__,
"setreuid($module, ruid, euid, /)\n"
"--\n"
"\n"
"Set the current process\'s real and effective user ids.");

#define OS_SETREUID_METHODDEF    \
    {"setreuid", (PyCFunction)(void(*)(void))os_setreuid, METH_FASTCALL, os_setreuid__doc__},

static PyObject *
os_setreuid_impl(PyObject *module, uid_t ruid, uid_t euid);

static PyObject *
os_setreuid(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    uid_t ruid;
    uid_t euid;

    if (!_PyArg_CheckPositional("setreuid", nargs, 2, 2)) {
        goto exit;
    }
    if (!_Py_Uid_Converter(args[0], &ruid)) {
        goto exit;
    }
    if (!_Py_Uid_Converter(args[1], &euid)) {
        goto exit;
    }
    return_value = os_setreuid_impl(module, ruid, euid);

exit:
    return return_value;
}

#endif /* defined(HAVE_SETREUID) */

#if defined(HAVE_SETREGID)

PyDoc_STRVAR(os_setregid__doc__,
"setregid($module, rgid, egid, /)\n"
"--\n"
"\n"
"Set the current process\'s real and effective group ids.");

#define OS_SETREGID_METHODDEF    \
    {"setregid", (PyCFunction)(void(*)(void))os_setregid, METH_FASTCALL, os_setregid__doc__},

static PyObject *
os_setregid_impl(PyObject *module, gid_t rgid, gid_t egid);

static PyObject *
os_setregid(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    gid_t rgid;
    gid_t egid;

    if (!_PyArg_CheckPositional("setregid", nargs, 2, 2)) {
        goto exit;
    }
    if (!_Py_Gid_Converter(args[0], &rgid)) {
        goto exit;
    }
    if (!_Py_Gid_Converter(args[1], &egid)) {
        goto exit;
    }
    return_value = os_setregid_impl(module, rgid, egid);

exit:
    return return_value;
}

#endif /* defined(HAVE_SETREGID) */

#if defined(HAVE_SETGID)

PyDoc_STRVAR(os_setgid__doc__,
"setgid($module, gid, /)\n"
"--\n"
"\n"
"Set the current process\'s group id.");

#define OS_SETGID_METHODDEF    \
    {"setgid", (PyCFunction)os_setgid, METH_O, os_setgid__doc__},

static PyObject *
os_setgid_impl(PyObject *module, gid_t gid);

static PyObject *
os_setgid(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    gid_t gid;

    if (!_Py_Gid_Converter(arg, &gid)) {
        goto exit;
    }
    return_value = os_setgid_impl(module, gid);

exit:
    return return_value;
}

#endif /* defined(HAVE_SETGID) */

#if defined(HAVE_SETGROUPS)

PyDoc_STRVAR(os_setgroups__doc__,
"setgroups($module, groups, /)\n"
"--\n"
"\n"
"Set the groups of the current process to list.");

#define OS_SETGROUPS_METHODDEF    \
    {"setgroups", (PyCFunction)os_setgroups, METH_O, os_setgroups__doc__},

#endif /* defined(HAVE_SETGROUPS) */

#if defined(HAVE_WAIT3)

PyDoc_STRVAR(os_wait3__doc__,
"wait3($module, /, options)\n"
"--\n"
"\n"
"Wait for completion of a child process.\n"
"\n"
"Returns a tuple of information about the child process:\n"
"  (pid, status, rusage)");

#define OS_WAIT3_METHODDEF    \
    {"wait3", (PyCFunction)(void(*)(void))os_wait3, METH_FASTCALL|METH_KEYWORDS, os_wait3__doc__},

static PyObject *
os_wait3_impl(PyObject *module, int options);

static PyObject *
os_wait3(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"options", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "wait3", 0};
    PyObject *argsbuf[1];
    int options;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    options = _PyLong_AsInt(args[0]);
    if (options == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_wait3_impl(module, options);

exit:
    return return_value;
}

#endif /* defined(HAVE_WAIT3) */

#if defined(HAVE_WAIT4)

PyDoc_STRVAR(os_wait4__doc__,
"wait4($module, /, pid, options)\n"
"--\n"
"\n"
"Wait for completion of a specific child process.\n"
"\n"
"Returns a tuple of information about the child process:\n"
"  (pid, status, rusage)");

#define OS_WAIT4_METHODDEF    \
    {"wait4", (PyCFunction)(void(*)(void))os_wait4, METH_FASTCALL|METH_KEYWORDS, os_wait4__doc__},

static PyObject *
os_wait4_impl(PyObject *module, pid_t pid, int options);

static PyObject *
os_wait4(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"pid", "options", NULL};
    static _PyArg_Parser _parser = {"" _Py_PARSE_PID "i:wait4", _keywords, 0};
    pid_t pid;
    int options;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &pid, &options)) {
        goto exit;
    }
    return_value = os_wait4_impl(module, pid, options);

exit:
    return return_value;
}

#endif /* defined(HAVE_WAIT4) */

#if (defined(HAVE_WAITID) && !defined(__APPLE__))

PyDoc_STRVAR(os_waitid__doc__,
"waitid($module, idtype, id, options, /)\n"
"--\n"
"\n"
"Returns the result of waiting for a process or processes.\n"
"\n"
"  idtype\n"
"    Must be one of be P_PID, P_PGID or P_ALL.\n"
"  id\n"
"    The id to wait on.\n"
"  options\n"
"    Constructed from the ORing of one or more of WEXITED, WSTOPPED\n"
"    or WCONTINUED and additionally may be ORed with WNOHANG or WNOWAIT.\n"
"\n"
"Returns either waitid_result or None if WNOHANG is specified and there are\n"
"no children in a waitable state.");

#define OS_WAITID_METHODDEF    \
    {"waitid", (PyCFunction)(void(*)(void))os_waitid, METH_FASTCALL, os_waitid__doc__},

static PyObject *
os_waitid_impl(PyObject *module, idtype_t idtype, id_t id, int options);

static PyObject *
os_waitid(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    idtype_t idtype;
    id_t id;
    int options;

    if (!_PyArg_ParseStack(args, nargs, "i" _Py_PARSE_PID "i:waitid",
        &idtype, &id, &options)) {
        goto exit;
    }
    return_value = os_waitid_impl(module, idtype, id, options);

exit:
    return return_value;
}

#endif /* (defined(HAVE_WAITID) && !defined(__APPLE__)) */

#if defined(HAVE_WAITPID)

PyDoc_STRVAR(os_waitpid__doc__,
"waitpid($module, pid, options, /)\n"
"--\n"
"\n"
"Wait for completion of a given child process.\n"
"\n"
"Returns a tuple of information regarding the child process:\n"
"    (pid, status)\n"
"\n"
"The options argument is ignored on Windows.");

#define OS_WAITPID_METHODDEF    \
    {"waitpid", (PyCFunction)(void(*)(void))os_waitpid, METH_FASTCALL, os_waitpid__doc__},

static PyObject *
os_waitpid_impl(PyObject *module, pid_t pid, int options);

static PyObject *
os_waitpid(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    pid_t pid;
    int options;

    if (!_PyArg_ParseStack(args, nargs, "" _Py_PARSE_PID "i:waitpid",
        &pid, &options)) {
        goto exit;
    }
    return_value = os_waitpid_impl(module, pid, options);

exit:
    return return_value;
}

#endif /* defined(HAVE_WAITPID) */

#if !defined(HAVE_WAITPID) && defined(HAVE_CWAIT)

PyDoc_STRVAR(os_waitpid__doc__,
"waitpid($module, pid, options, /)\n"
"--\n"
"\n"
"Wait for completion of a given process.\n"
"\n"
"Returns a tuple of information regarding the process:\n"
"    (pid, status << 8)\n"
"\n"
"The options argument is ignored on Windows.");

#define OS_WAITPID_METHODDEF    \
    {"waitpid", (PyCFunction)(void(*)(void))os_waitpid, METH_FASTCALL, os_waitpid__doc__},

static PyObject *
os_waitpid_impl(PyObject *module, intptr_t pid, int options);

static PyObject *
os_waitpid(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    intptr_t pid;
    int options;

    if (!_PyArg_ParseStack(args, nargs, "" _Py_PARSE_INTPTR "i:waitpid",
        &pid, &options)) {
        goto exit;
    }
    return_value = os_waitpid_impl(module, pid, options);

exit:
    return return_value;
}

#endif /* !defined(HAVE_WAITPID) && defined(HAVE_CWAIT) */

#if defined(HAVE_WAIT)

PyDoc_STRVAR(os_wait__doc__,
"wait($module, /)\n"
"--\n"
"\n"
"Wait for completion of a child process.\n"
"\n"
"Returns a tuple of information about the child process:\n"
"    (pid, status)");

#define OS_WAIT_METHODDEF    \
    {"wait", (PyCFunction)os_wait, METH_NOARGS, os_wait__doc__},

static PyObject *
os_wait_impl(PyObject *module);

static PyObject *
os_wait(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_wait_impl(module);
}

#endif /* defined(HAVE_WAIT) */

#if (defined(__linux__) && defined(__NR_pidfd_open))

PyDoc_STRVAR(os_pidfd_open__doc__,
"pidfd_open($module, /, pid, flags=0)\n"
"--\n"
"\n"
"Return a file descriptor referring to the process *pid*.\n"
"\n"
"The descriptor can be used to perform process management without races and\n"
"signals.");

#define OS_PIDFD_OPEN_METHODDEF    \
    {"pidfd_open", (PyCFunction)(void(*)(void))os_pidfd_open, METH_FASTCALL|METH_KEYWORDS, os_pidfd_open__doc__},

static PyObject *
os_pidfd_open_impl(PyObject *module, pid_t pid, unsigned int flags);

static PyObject *
os_pidfd_open(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"pid", "flags", NULL};
    static _PyArg_Parser _parser = {"" _Py_PARSE_PID "|O&:pidfd_open", _keywords, 0};
    pid_t pid;
    unsigned int flags = 0;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &pid, _PyLong_UnsignedInt_Converter, &flags)) {
        goto exit;
    }
    return_value = os_pidfd_open_impl(module, pid, flags);

exit:
    return return_value;
}

#endif /* (defined(__linux__) && defined(__NR_pidfd_open)) */

#if (defined(HAVE_READLINK) || defined(MS_WINDOWS))

PyDoc_STRVAR(os_readlink__doc__,
"readlink($module, /, path, *, dir_fd=None)\n"
"--\n"
"\n"
"Return a string representing the path to which the symbolic link points.\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"and path should be relative; path will then be relative to that directory.\n"
"\n"
"dir_fd may not be implemented on your platform.  If it is unavailable,\n"
"using it will raise a NotImplementedError.");

#define OS_READLINK_METHODDEF    \
    {"readlink", (PyCFunction)(void(*)(void))os_readlink, METH_FASTCALL|METH_KEYWORDS, os_readlink__doc__},

static PyObject *
os_readlink_impl(PyObject *module, path_t *path, int dir_fd);

static PyObject *
os_readlink(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "dir_fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "readlink", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    path_t path = PATH_T_INITIALIZE("readlink", "path", 0, 0);
    int dir_fd = DEFAULT_DIR_FD;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!READLINKAT_DIR_FD_CONVERTER(args[1], &dir_fd)) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_readlink_impl(module, &path, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* (defined(HAVE_READLINK) || defined(MS_WINDOWS)) */

#if defined(HAVE_SYMLINK)

PyDoc_STRVAR(os_symlink__doc__,
"symlink($module, /, src, dst, target_is_directory=False, *, dir_fd=None)\n"
"--\n"
"\n"
"Create a symbolic link pointing to src named dst.\n"
"\n"
"target_is_directory is required on Windows if the target is to be\n"
"  interpreted as a directory.  (On Windows, symlink requires\n"
"  Windows 6.0 or greater, and raises a NotImplementedError otherwise.)\n"
"  target_is_directory is ignored on non-Windows platforms.\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.");

#define OS_SYMLINK_METHODDEF    \
    {"symlink", (PyCFunction)(void(*)(void))os_symlink, METH_FASTCALL|METH_KEYWORDS, os_symlink__doc__},

static PyObject *
os_symlink_impl(PyObject *module, path_t *src, path_t *dst,
                int target_is_directory, int dir_fd);

static PyObject *
os_symlink(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"src", "dst", "target_is_directory", "dir_fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "symlink", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    path_t src = PATH_T_INITIALIZE("symlink", "src", 0, 0);
    path_t dst = PATH_T_INITIALIZE("symlink", "dst", 0, 0);
    int target_is_directory = 0;
    int dir_fd = DEFAULT_DIR_FD;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &src)) {
        goto exit;
    }
    if (!path_converter(args[1], &dst)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        target_is_directory = PyObject_IsTrue(args[2]);
        if (target_is_directory < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!SYMLINKAT_DIR_FD_CONVERTER(args[3], &dir_fd)) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_symlink_impl(module, &src, &dst, target_is_directory, dir_fd);

exit:
    /* Cleanup for src */
    path_cleanup(&src);
    /* Cleanup for dst */
    path_cleanup(&dst);

    return return_value;
}

#endif /* defined(HAVE_SYMLINK) */

#if defined(HAVE_TIMES)

PyDoc_STRVAR(os_times__doc__,
"times($module, /)\n"
"--\n"
"\n"
"Return a collection containing process timing information.\n"
"\n"
"The object returned behaves like a named tuple with these fields:\n"
"  (utime, stime, cutime, cstime, elapsed_time)\n"
"All fields are floating point numbers.");

#define OS_TIMES_METHODDEF    \
    {"times", (PyCFunction)os_times, METH_NOARGS, os_times__doc__},

static PyObject *
os_times_impl(PyObject *module);

static PyObject *
os_times(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_times_impl(module);
}

#endif /* defined(HAVE_TIMES) */

#if defined(HAVE_GETSID)

PyDoc_STRVAR(os_getsid__doc__,
"getsid($module, pid, /)\n"
"--\n"
"\n"
"Call the system call getsid(pid) and return the result.");

#define OS_GETSID_METHODDEF    \
    {"getsid", (PyCFunction)os_getsid, METH_O, os_getsid__doc__},

static PyObject *
os_getsid_impl(PyObject *module, pid_t pid);

static PyObject *
os_getsid(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    pid_t pid;

    if (!PyArg_Parse(arg, "" _Py_PARSE_PID ":getsid", &pid)) {
        goto exit;
    }
    return_value = os_getsid_impl(module, pid);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETSID) */

#if defined(HAVE_SETSID)

PyDoc_STRVAR(os_setsid__doc__,
"setsid($module, /)\n"
"--\n"
"\n"
"Call the system call setsid().");

#define OS_SETSID_METHODDEF    \
    {"setsid", (PyCFunction)os_setsid, METH_NOARGS, os_setsid__doc__},

static PyObject *
os_setsid_impl(PyObject *module);

static PyObject *
os_setsid(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_setsid_impl(module);
}

#endif /* defined(HAVE_SETSID) */

#if defined(HAVE_SETPGID)

PyDoc_STRVAR(os_setpgid__doc__,
"setpgid($module, pid, pgrp, /)\n"
"--\n"
"\n"
"Call the system call setpgid(pid, pgrp).");

#define OS_SETPGID_METHODDEF    \
    {"setpgid", (PyCFunction)(void(*)(void))os_setpgid, METH_FASTCALL, os_setpgid__doc__},

static PyObject *
os_setpgid_impl(PyObject *module, pid_t pid, pid_t pgrp);

static PyObject *
os_setpgid(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    pid_t pid;
    pid_t pgrp;

    if (!_PyArg_ParseStack(args, nargs, "" _Py_PARSE_PID "" _Py_PARSE_PID ":setpgid",
        &pid, &pgrp)) {
        goto exit;
    }
    return_value = os_setpgid_impl(module, pid, pgrp);

exit:
    return return_value;
}

#endif /* defined(HAVE_SETPGID) */

#if defined(HAVE_TCGETPGRP)

PyDoc_STRVAR(os_tcgetpgrp__doc__,
"tcgetpgrp($module, fd, /)\n"
"--\n"
"\n"
"Return the process group associated with the terminal specified by fd.");

#define OS_TCGETPGRP_METHODDEF    \
    {"tcgetpgrp", (PyCFunction)os_tcgetpgrp, METH_O, os_tcgetpgrp__doc__},

static PyObject *
os_tcgetpgrp_impl(PyObject *module, int fd);

static PyObject *
os_tcgetpgrp(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;

    fd = _PyLong_AsInt(arg);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_tcgetpgrp_impl(module, fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_TCGETPGRP) */

#if defined(HAVE_TCSETPGRP)

PyDoc_STRVAR(os_tcsetpgrp__doc__,
"tcsetpgrp($module, fd, pgid, /)\n"
"--\n"
"\n"
"Set the process group associated with the terminal specified by fd.");

#define OS_TCSETPGRP_METHODDEF    \
    {"tcsetpgrp", (PyCFunction)(void(*)(void))os_tcsetpgrp, METH_FASTCALL, os_tcsetpgrp__doc__},

static PyObject *
os_tcsetpgrp_impl(PyObject *module, int fd, pid_t pgid);

static PyObject *
os_tcsetpgrp(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    pid_t pgid;

    if (!_PyArg_ParseStack(args, nargs, "i" _Py_PARSE_PID ":tcsetpgrp",
        &fd, &pgid)) {
        goto exit;
    }
    return_value = os_tcsetpgrp_impl(module, fd, pgid);

exit:
    return return_value;
}

#endif /* defined(HAVE_TCSETPGRP) */

PyDoc_STRVAR(os_open__doc__,
"open($module, /, path, flags, mode=511, *, dir_fd=None)\n"
"--\n"
"\n"
"Open a file for low level IO.  Returns a file descriptor (integer).\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.");

#define OS_OPEN_METHODDEF    \
    {"open", (PyCFunction)(void(*)(void))os_open, METH_FASTCALL|METH_KEYWORDS, os_open__doc__},

static int
os_open_impl(PyObject *module, path_t *path, int flags, int mode, int dir_fd);

static PyObject *
os_open(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "flags", "mode", "dir_fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "open", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    path_t path = PATH_T_INITIALIZE("open", "path", 0, 0);
    int flags;
    int mode = 511;
    int dir_fd = DEFAULT_DIR_FD;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    flags = _PyLong_AsInt(args[1]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        mode = _PyLong_AsInt(args[2]);
        if (mode == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!OPENAT_DIR_FD_CONVERTER(args[3], &dir_fd)) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = os_open_impl(module, &path, flags, mode, dir_fd);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

PyDoc_STRVAR(os_close__doc__,
"close($module, /, fd)\n"
"--\n"
"\n"
"Close a file descriptor.");

#define OS_CLOSE_METHODDEF    \
    {"close", (PyCFunction)(void(*)(void))os_close, METH_FASTCALL|METH_KEYWORDS, os_close__doc__},

static PyObject *
os_close_impl(PyObject *module, int fd);

static PyObject *
os_close(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "close", 0};
    PyObject *argsbuf[1];
    int fd;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_close_impl(module, fd);

exit:
    return return_value;
}

PyDoc_STRVAR(os_closerange__doc__,
"closerange($module, fd_low, fd_high, /)\n"
"--\n"
"\n"
"Closes all file descriptors in [fd_low, fd_high), ignoring errors.");

#define OS_CLOSERANGE_METHODDEF    \
    {"closerange", (PyCFunction)(void(*)(void))os_closerange, METH_FASTCALL, os_closerange__doc__},

static PyObject *
os_closerange_impl(PyObject *module, int fd_low, int fd_high);

static PyObject *
os_closerange(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd_low;
    int fd_high;

    if (!_PyArg_CheckPositional("closerange", nargs, 2, 2)) {
        goto exit;
    }
    fd_low = _PyLong_AsInt(args[0]);
    if (fd_low == -1 && PyErr_Occurred()) {
        goto exit;
    }
    fd_high = _PyLong_AsInt(args[1]);
    if (fd_high == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_closerange_impl(module, fd_low, fd_high);

exit:
    return return_value;
}

PyDoc_STRVAR(os_dup__doc__,
"dup($module, fd, /)\n"
"--\n"
"\n"
"Return a duplicate of a file descriptor.");

#define OS_DUP_METHODDEF    \
    {"dup", (PyCFunction)os_dup, METH_O, os_dup__doc__},

static int
os_dup_impl(PyObject *module, int fd);

static PyObject *
os_dup(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;
    int _return_value;

    fd = _PyLong_AsInt(arg);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = os_dup_impl(module, fd);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(os_dup2__doc__,
"dup2($module, /, fd, fd2, inheritable=True)\n"
"--\n"
"\n"
"Duplicate file descriptor.");

#define OS_DUP2_METHODDEF    \
    {"dup2", (PyCFunction)(void(*)(void))os_dup2, METH_FASTCALL|METH_KEYWORDS, os_dup2__doc__},

static int
os_dup2_impl(PyObject *module, int fd, int fd2, int inheritable);

static PyObject *
os_dup2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", "fd2", "inheritable", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "dup2", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    int fd;
    int fd2;
    int inheritable = 1;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    fd2 = _PyLong_AsInt(args[1]);
    if (fd2 == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    inheritable = PyObject_IsTrue(args[2]);
    if (inheritable < 0) {
        goto exit;
    }
skip_optional_pos:
    _return_value = os_dup2_impl(module, fd, fd2, inheritable);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

#if defined(HAVE_LOCKF)

PyDoc_STRVAR(os_lockf__doc__,
"lockf($module, fd, command, length, /)\n"
"--\n"
"\n"
"Apply, test or remove a POSIX lock on an open file descriptor.\n"
"\n"
"  fd\n"
"    An open file descriptor.\n"
"  command\n"
"    One of F_LOCK, F_TLOCK, F_ULOCK or F_TEST.\n"
"  length\n"
"    The number of bytes to lock, starting at the current position.");

#define OS_LOCKF_METHODDEF    \
    {"lockf", (PyCFunction)(void(*)(void))os_lockf, METH_FASTCALL, os_lockf__doc__},

static PyObject *
os_lockf_impl(PyObject *module, int fd, int command, Py_off_t length);

static PyObject *
os_lockf(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int command;
    Py_off_t length;

    if (!_PyArg_CheckPositional("lockf", nargs, 3, 3)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    command = _PyLong_AsInt(args[1]);
    if (command == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!Py_off_t_converter(args[2], &length)) {
        goto exit;
    }
    return_value = os_lockf_impl(module, fd, command, length);

exit:
    return return_value;
}

#endif /* defined(HAVE_LOCKF) */

PyDoc_STRVAR(os_lseek__doc__,
"lseek($module, fd, position, how, /)\n"
"--\n"
"\n"
"Set the position of a file descriptor.  Return the new position.\n"
"\n"
"Return the new cursor position in number of bytes\n"
"relative to the beginning of the file.");

#define OS_LSEEK_METHODDEF    \
    {"lseek", (PyCFunction)(void(*)(void))os_lseek, METH_FASTCALL, os_lseek__doc__},

static Py_off_t
os_lseek_impl(PyObject *module, int fd, Py_off_t position, int how);

static PyObject *
os_lseek(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    Py_off_t position;
    int how;
    Py_off_t _return_value;

    if (!_PyArg_CheckPositional("lseek", nargs, 3, 3)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!Py_off_t_converter(args[1], &position)) {
        goto exit;
    }
    how = _PyLong_AsInt(args[2]);
    if (how == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = os_lseek_impl(module, fd, position, how);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromPy_off_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(os_read__doc__,
"read($module, fd, length, /)\n"
"--\n"
"\n"
"Read from a file descriptor.  Returns a bytes object.");

#define OS_READ_METHODDEF    \
    {"read", (PyCFunction)(void(*)(void))os_read, METH_FASTCALL, os_read__doc__},

static PyObject *
os_read_impl(PyObject *module, int fd, Py_ssize_t length);

static PyObject *
os_read(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    Py_ssize_t length;

    if (!_PyArg_CheckPositional("read", nargs, 2, 2)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
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
    return_value = os_read_impl(module, fd, length);

exit:
    return return_value;
}

#if defined(HAVE_READV)

PyDoc_STRVAR(os_readv__doc__,
"readv($module, fd, buffers, /)\n"
"--\n"
"\n"
"Read from a file descriptor fd into an iterable of buffers.\n"
"\n"
"The buffers should be mutable buffers accepting bytes.\n"
"readv will transfer data into each buffer until it is full\n"
"and then move on to the next buffer in the sequence to hold\n"
"the rest of the data.\n"
"\n"
"readv returns the total number of bytes read,\n"
"which may be less than the total capacity of all the buffers.");

#define OS_READV_METHODDEF    \
    {"readv", (PyCFunction)(void(*)(void))os_readv, METH_FASTCALL, os_readv__doc__},

static Py_ssize_t
os_readv_impl(PyObject *module, int fd, PyObject *buffers);

static PyObject *
os_readv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    PyObject *buffers;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckPositional("readv", nargs, 2, 2)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    buffers = args[1];
    _return_value = os_readv_impl(module, fd, buffers);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_READV) */

#if defined(HAVE_PREAD)

PyDoc_STRVAR(os_pread__doc__,
"pread($module, fd, length, offset, /)\n"
"--\n"
"\n"
"Read a number of bytes from a file descriptor starting at a particular offset.\n"
"\n"
"Read length bytes from file descriptor fd, starting at offset bytes from\n"
"the beginning of the file.  The file offset remains unchanged.");

#define OS_PREAD_METHODDEF    \
    {"pread", (PyCFunction)(void(*)(void))os_pread, METH_FASTCALL, os_pread__doc__},

static PyObject *
os_pread_impl(PyObject *module, int fd, Py_ssize_t length, Py_off_t offset);

static PyObject *
os_pread(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    Py_ssize_t length;
    Py_off_t offset;

    if (!_PyArg_CheckPositional("pread", nargs, 3, 3)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
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
    if (!Py_off_t_converter(args[2], &offset)) {
        goto exit;
    }
    return_value = os_pread_impl(module, fd, length, offset);

exit:
    return return_value;
}

#endif /* defined(HAVE_PREAD) */

#if (defined(HAVE_PREADV) || defined (HAVE_PREADV2))

PyDoc_STRVAR(os_preadv__doc__,
"preadv($module, fd, buffers, offset, flags=0, /)\n"
"--\n"
"\n"
"Reads from a file descriptor into a number of mutable bytes-like objects.\n"
"\n"
"Combines the functionality of readv() and pread(). As readv(), it will\n"
"transfer data into each buffer until it is full and then move on to the next\n"
"buffer in the sequence to hold the rest of the data. Its fourth argument,\n"
"specifies the file offset at which the input operation is to be performed. It\n"
"will return the total number of bytes read (which can be less than the total\n"
"capacity of all the objects).\n"
"\n"
"The flags argument contains a bitwise OR of zero or more of the following flags:\n"
"\n"
"- RWF_HIPRI\n"
"- RWF_NOWAIT\n"
"\n"
"Using non-zero flags requires Linux 4.6 or newer.");

#define OS_PREADV_METHODDEF    \
    {"preadv", (PyCFunction)(void(*)(void))os_preadv, METH_FASTCALL, os_preadv__doc__},

static Py_ssize_t
os_preadv_impl(PyObject *module, int fd, PyObject *buffers, Py_off_t offset,
               int flags);

static PyObject *
os_preadv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    PyObject *buffers;
    Py_off_t offset;
    int flags = 0;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckPositional("preadv", nargs, 3, 4)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    buffers = args[1];
    if (!Py_off_t_converter(args[2], &offset)) {
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    flags = _PyLong_AsInt(args[3]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = os_preadv_impl(module, fd, buffers, offset, flags);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

#endif /* (defined(HAVE_PREADV) || defined (HAVE_PREADV2)) */

PyDoc_STRVAR(os_write__doc__,
"write($module, fd, data, /)\n"
"--\n"
"\n"
"Write a bytes object to a file descriptor.");

#define OS_WRITE_METHODDEF    \
    {"write", (PyCFunction)(void(*)(void))os_write, METH_FASTCALL, os_write__doc__},

static Py_ssize_t
os_write_impl(PyObject *module, int fd, Py_buffer *data);

static PyObject *
os_write(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    Py_buffer data = {NULL, NULL};
    Py_ssize_t _return_value;

    if (!_PyArg_CheckPositional("write", nargs, 2, 2)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("write", "argument 2", "contiguous buffer", args[1]);
        goto exit;
    }
    _return_value = os_write_impl(module, fd, &data);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#if defined(HAVE_SENDFILE) && defined(__APPLE__)

PyDoc_STRVAR(os_sendfile__doc__,
"sendfile($module, /, out_fd, in_fd, offset, count, headers=(),\n"
"         trailers=(), flags=0)\n"
"--\n"
"\n"
"Copy count bytes from file descriptor in_fd to file descriptor out_fd.");

#define OS_SENDFILE_METHODDEF    \
    {"sendfile", (PyCFunction)(void(*)(void))os_sendfile, METH_FASTCALL|METH_KEYWORDS, os_sendfile__doc__},

static PyObject *
os_sendfile_impl(PyObject *module, int out_fd, int in_fd, Py_off_t offset,
                 Py_off_t sbytes, PyObject *headers, PyObject *trailers,
                 int flags);

static PyObject *
os_sendfile(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"out_fd", "in_fd", "offset", "count", "headers", "trailers", "flags", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "sendfile", 0};
    PyObject *argsbuf[7];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 4;
    int out_fd;
    int in_fd;
    Py_off_t offset;
    Py_off_t sbytes;
    PyObject *headers = NULL;
    PyObject *trailers = NULL;
    int flags = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 4, 7, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    out_fd = _PyLong_AsInt(args[0]);
    if (out_fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    in_fd = _PyLong_AsInt(args[1]);
    if (in_fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!Py_off_t_converter(args[2], &offset)) {
        goto exit;
    }
    if (!Py_off_t_converter(args[3], &sbytes)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[4]) {
        headers = args[4];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[5]) {
        trailers = args[5];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    flags = _PyLong_AsInt(args[6]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = os_sendfile_impl(module, out_fd, in_fd, offset, sbytes, headers, trailers, flags);

exit:
    return return_value;
}

#endif /* defined(HAVE_SENDFILE) && defined(__APPLE__) */

#if defined(HAVE_SENDFILE) && !defined(__APPLE__) && (defined(__FreeBSD__) || defined(__DragonFly__))

PyDoc_STRVAR(os_sendfile__doc__,
"sendfile($module, /, out_fd, in_fd, offset, count, headers=(),\n"
"         trailers=(), flags=0)\n"
"--\n"
"\n"
"Copy count bytes from file descriptor in_fd to file descriptor out_fd.");

#define OS_SENDFILE_METHODDEF    \
    {"sendfile", (PyCFunction)(void(*)(void))os_sendfile, METH_FASTCALL|METH_KEYWORDS, os_sendfile__doc__},

static PyObject *
os_sendfile_impl(PyObject *module, int out_fd, int in_fd, Py_off_t offset,
                 Py_ssize_t count, PyObject *headers, PyObject *trailers,
                 int flags);

static PyObject *
os_sendfile(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"out_fd", "in_fd", "offset", "count", "headers", "trailers", "flags", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "sendfile", 0};
    PyObject *argsbuf[7];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 4;
    int out_fd;
    int in_fd;
    Py_off_t offset;
    Py_ssize_t count;
    PyObject *headers = NULL;
    PyObject *trailers = NULL;
    int flags = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 4, 7, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    out_fd = _PyLong_AsInt(args[0]);
    if (out_fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    in_fd = _PyLong_AsInt(args[1]);
    if (in_fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!Py_off_t_converter(args[2], &offset)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[3]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        count = ival;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[4]) {
        headers = args[4];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[5]) {
        trailers = args[5];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    flags = _PyLong_AsInt(args[6]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = os_sendfile_impl(module, out_fd, in_fd, offset, count, headers, trailers, flags);

exit:
    return return_value;
}

#endif /* defined(HAVE_SENDFILE) && !defined(__APPLE__) && (defined(__FreeBSD__) || defined(__DragonFly__)) */

#if defined(HAVE_SENDFILE) && !defined(__APPLE__) && !(defined(__FreeBSD__) || defined(__DragonFly__))

PyDoc_STRVAR(os_sendfile__doc__,
"sendfile($module, /, out_fd, in_fd, offset, count)\n"
"--\n"
"\n"
"Copy count bytes from file descriptor in_fd to file descriptor out_fd.");

#define OS_SENDFILE_METHODDEF    \
    {"sendfile", (PyCFunction)(void(*)(void))os_sendfile, METH_FASTCALL|METH_KEYWORDS, os_sendfile__doc__},

static PyObject *
os_sendfile_impl(PyObject *module, int out_fd, int in_fd, PyObject *offobj,
                 Py_ssize_t count);

static PyObject *
os_sendfile(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"out_fd", "in_fd", "offset", "count", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "sendfile", 0};
    PyObject *argsbuf[4];
    int out_fd;
    int in_fd;
    PyObject *offobj;
    Py_ssize_t count;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 4, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    out_fd = _PyLong_AsInt(args[0]);
    if (out_fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    in_fd = _PyLong_AsInt(args[1]);
    if (in_fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    offobj = args[2];
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[3]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        count = ival;
    }
    return_value = os_sendfile_impl(module, out_fd, in_fd, offobj, count);

exit:
    return return_value;
}

#endif /* defined(HAVE_SENDFILE) && !defined(__APPLE__) && !(defined(__FreeBSD__) || defined(__DragonFly__)) */

#if defined(__APPLE__)

PyDoc_STRVAR(os__fcopyfile__doc__,
"_fcopyfile($module, in_fd, out_fd, flags, /)\n"
"--\n"
"\n"
"Efficiently copy content or metadata of 2 regular file descriptors (macOS).");

#define OS__FCOPYFILE_METHODDEF    \
    {"_fcopyfile", (PyCFunction)(void(*)(void))os__fcopyfile, METH_FASTCALL, os__fcopyfile__doc__},

static PyObject *
os__fcopyfile_impl(PyObject *module, int in_fd, int out_fd, int flags);

static PyObject *
os__fcopyfile(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int in_fd;
    int out_fd;
    int flags;

    if (!_PyArg_CheckPositional("_fcopyfile", nargs, 3, 3)) {
        goto exit;
    }
    in_fd = _PyLong_AsInt(args[0]);
    if (in_fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    out_fd = _PyLong_AsInt(args[1]);
    if (out_fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    flags = _PyLong_AsInt(args[2]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os__fcopyfile_impl(module, in_fd, out_fd, flags);

exit:
    return return_value;
}

#endif /* defined(__APPLE__) */

PyDoc_STRVAR(os_fstat__doc__,
"fstat($module, /, fd)\n"
"--\n"
"\n"
"Perform a stat system call on the given file descriptor.\n"
"\n"
"Like stat(), but for an open file descriptor.\n"
"Equivalent to os.stat(fd).");

#define OS_FSTAT_METHODDEF    \
    {"fstat", (PyCFunction)(void(*)(void))os_fstat, METH_FASTCALL|METH_KEYWORDS, os_fstat__doc__},

static PyObject *
os_fstat_impl(PyObject *module, int fd);

static PyObject *
os_fstat(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "fstat", 0};
    PyObject *argsbuf[1];
    int fd;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_fstat_impl(module, fd);

exit:
    return return_value;
}

PyDoc_STRVAR(os_isatty__doc__,
"isatty($module, fd, /)\n"
"--\n"
"\n"
"Return True if the fd is connected to a terminal.\n"
"\n"
"Return True if the file descriptor is an open file descriptor\n"
"connected to the slave end of a terminal.");

#define OS_ISATTY_METHODDEF    \
    {"isatty", (PyCFunction)os_isatty, METH_O, os_isatty__doc__},

static int
os_isatty_impl(PyObject *module, int fd);

static PyObject *
os_isatty(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;
    int _return_value;

    fd = _PyLong_AsInt(arg);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = os_isatty_impl(module, fd);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

#if defined(HAVE_PIPE)

PyDoc_STRVAR(os_pipe__doc__,
"pipe($module, /)\n"
"--\n"
"\n"
"Create a pipe.\n"
"\n"
"Returns a tuple of two file descriptors:\n"
"  (read_fd, write_fd)");

#define OS_PIPE_METHODDEF    \
    {"pipe", (PyCFunction)os_pipe, METH_NOARGS, os_pipe__doc__},

static PyObject *
os_pipe_impl(PyObject *module);

static PyObject *
os_pipe(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_pipe_impl(module);
}

#endif /* defined(HAVE_PIPE) */

#if defined(HAVE_PIPE2)

PyDoc_STRVAR(os_pipe2__doc__,
"pipe2($module, flags, /)\n"
"--\n"
"\n"
"Create a pipe with flags set atomically.\n"
"\n"
"Returns a tuple of two file descriptors:\n"
"  (read_fd, write_fd)\n"
"\n"
"flags can be constructed by ORing together one or more of these values:\n"
"O_NONBLOCK, O_CLOEXEC.");

#define OS_PIPE2_METHODDEF    \
    {"pipe2", (PyCFunction)os_pipe2, METH_O, os_pipe2__doc__},

static PyObject *
os_pipe2_impl(PyObject *module, int flags);

static PyObject *
os_pipe2(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int flags;

    flags = _PyLong_AsInt(arg);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_pipe2_impl(module, flags);

exit:
    return return_value;
}

#endif /* defined(HAVE_PIPE2) */

#if defined(HAVE_WRITEV)

PyDoc_STRVAR(os_writev__doc__,
"writev($module, fd, buffers, /)\n"
"--\n"
"\n"
"Iterate over buffers, and write the contents of each to a file descriptor.\n"
"\n"
"Returns the total number of bytes written.\n"
"buffers must be a sequence of bytes-like objects.");

#define OS_WRITEV_METHODDEF    \
    {"writev", (PyCFunction)(void(*)(void))os_writev, METH_FASTCALL, os_writev__doc__},

static Py_ssize_t
os_writev_impl(PyObject *module, int fd, PyObject *buffers);

static PyObject *
os_writev(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    PyObject *buffers;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckPositional("writev", nargs, 2, 2)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    buffers = args[1];
    _return_value = os_writev_impl(module, fd, buffers);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_WRITEV) */

#if defined(HAVE_PWRITE)

PyDoc_STRVAR(os_pwrite__doc__,
"pwrite($module, fd, buffer, offset, /)\n"
"--\n"
"\n"
"Write bytes to a file descriptor starting at a particular offset.\n"
"\n"
"Write buffer to fd, starting at offset bytes from the beginning of\n"
"the file.  Returns the number of bytes writte.  Does not change the\n"
"current file offset.");

#define OS_PWRITE_METHODDEF    \
    {"pwrite", (PyCFunction)(void(*)(void))os_pwrite, METH_FASTCALL, os_pwrite__doc__},

static Py_ssize_t
os_pwrite_impl(PyObject *module, int fd, Py_buffer *buffer, Py_off_t offset);

static PyObject *
os_pwrite(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    Py_buffer buffer = {NULL, NULL};
    Py_off_t offset;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckPositional("pwrite", nargs, 3, 3)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("pwrite", "argument 2", "contiguous buffer", args[1]);
        goto exit;
    }
    if (!Py_off_t_converter(args[2], &offset)) {
        goto exit;
    }
    _return_value = os_pwrite_impl(module, fd, &buffer, offset);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

#endif /* defined(HAVE_PWRITE) */

#if (defined(HAVE_PWRITEV) || defined (HAVE_PWRITEV2))

PyDoc_STRVAR(os_pwritev__doc__,
"pwritev($module, fd, buffers, offset, flags=0, /)\n"
"--\n"
"\n"
"Writes the contents of bytes-like objects to a file descriptor at a given offset.\n"
"\n"
"Combines the functionality of writev() and pwrite(). All buffers must be a sequence\n"
"of bytes-like objects. Buffers are processed in array order. Entire contents of first\n"
"buffer is written before proceeding to second, and so on. The operating system may\n"
"set a limit (sysconf() value SC_IOV_MAX) on the number of buffers that can be used.\n"
"This function writes the contents of each object to the file descriptor and returns\n"
"the total number of bytes written.\n"
"\n"
"The flags argument contains a bitwise OR of zero or more of the following flags:\n"
"\n"
"- RWF_DSYNC\n"
"- RWF_SYNC\n"
"- RWF_APPEND\n"
"\n"
"Using non-zero flags requires Linux 4.7 or newer.");

#define OS_PWRITEV_METHODDEF    \
    {"pwritev", (PyCFunction)(void(*)(void))os_pwritev, METH_FASTCALL, os_pwritev__doc__},

static Py_ssize_t
os_pwritev_impl(PyObject *module, int fd, PyObject *buffers, Py_off_t offset,
                int flags);

static PyObject *
os_pwritev(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    PyObject *buffers;
    Py_off_t offset;
    int flags = 0;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckPositional("pwritev", nargs, 3, 4)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    buffers = args[1];
    if (!Py_off_t_converter(args[2], &offset)) {
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    flags = _PyLong_AsInt(args[3]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = os_pwritev_impl(module, fd, buffers, offset, flags);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

#endif /* (defined(HAVE_PWRITEV) || defined (HAVE_PWRITEV2)) */

#if defined(HAVE_COPY_FILE_RANGE)

PyDoc_STRVAR(os_copy_file_range__doc__,
"copy_file_range($module, /, src, dst, count, offset_src=None,\n"
"                offset_dst=None)\n"
"--\n"
"\n"
"Copy count bytes from one file descriptor to another.\n"
"\n"
"  src\n"
"    Source file descriptor.\n"
"  dst\n"
"    Destination file descriptor.\n"
"  count\n"
"    Number of bytes to copy.\n"
"  offset_src\n"
"    Starting offset in src.\n"
"  offset_dst\n"
"    Starting offset in dst.\n"
"\n"
"If offset_src is None, then src is read from the current position;\n"
"respectively for offset_dst.");

#define OS_COPY_FILE_RANGE_METHODDEF    \
    {"copy_file_range", (PyCFunction)(void(*)(void))os_copy_file_range, METH_FASTCALL|METH_KEYWORDS, os_copy_file_range__doc__},

static PyObject *
os_copy_file_range_impl(PyObject *module, int src, int dst, Py_ssize_t count,
                        PyObject *offset_src, PyObject *offset_dst);

static PyObject *
os_copy_file_range(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"src", "dst", "count", "offset_src", "offset_dst", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "copy_file_range", 0};
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    int src;
    int dst;
    Py_ssize_t count;
    PyObject *offset_src = Py_None;
    PyObject *offset_dst = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 5, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    src = _PyLong_AsInt(args[0]);
    if (src == -1 && PyErr_Occurred()) {
        goto exit;
    }
    dst = _PyLong_AsInt(args[1]);
    if (dst == -1 && PyErr_Occurred()) {
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
        count = ival;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[3]) {
        offset_src = args[3];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    offset_dst = args[4];
skip_optional_pos:
    return_value = os_copy_file_range_impl(module, src, dst, count, offset_src, offset_dst);

exit:
    return return_value;
}

#endif /* defined(HAVE_COPY_FILE_RANGE) */

#if defined(HAVE_MKFIFO)

PyDoc_STRVAR(os_mkfifo__doc__,
"mkfifo($module, /, path, mode=438, *, dir_fd=None)\n"
"--\n"
"\n"
"Create a \"fifo\" (a POSIX named pipe).\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.");

#define OS_MKFIFO_METHODDEF    \
    {"mkfifo", (PyCFunction)(void(*)(void))os_mkfifo, METH_FASTCALL|METH_KEYWORDS, os_mkfifo__doc__},

static PyObject *
os_mkfifo_impl(PyObject *module, path_t *path, int mode, int dir_fd);

static PyObject *
os_mkfifo(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "mode", "dir_fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "mkfifo", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    path_t path = PATH_T_INITIALIZE("mkfifo", "path", 0, 0);
    int mode = 438;
    int dir_fd = DEFAULT_DIR_FD;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        mode = _PyLong_AsInt(args[1]);
        if (mode == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!MKFIFOAT_DIR_FD_CONVERTER(args[2], &dir_fd)) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_mkfifo_impl(module, &path, mode, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(HAVE_MKFIFO) */

#if (defined(HAVE_MKNOD) && defined(HAVE_MAKEDEV))

PyDoc_STRVAR(os_mknod__doc__,
"mknod($module, /, path, mode=384, device=0, *, dir_fd=None)\n"
"--\n"
"\n"
"Create a node in the file system.\n"
"\n"
"Create a node in the file system (file, device special file or named pipe)\n"
"at path.  mode specifies both the permissions to use and the\n"
"type of node to be created, being combined (bitwise OR) with one of\n"
"S_IFREG, S_IFCHR, S_IFBLK, and S_IFIFO.  If S_IFCHR or S_IFBLK is set on mode,\n"
"device defines the newly created device special file (probably using\n"
"os.makedev()).  Otherwise device is ignored.\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.");

#define OS_MKNOD_METHODDEF    \
    {"mknod", (PyCFunction)(void(*)(void))os_mknod, METH_FASTCALL|METH_KEYWORDS, os_mknod__doc__},

static PyObject *
os_mknod_impl(PyObject *module, path_t *path, int mode, dev_t device,
              int dir_fd);

static PyObject *
os_mknod(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "mode", "device", "dir_fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "mknod", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    path_t path = PATH_T_INITIALIZE("mknod", "path", 0, 0);
    int mode = 384;
    dev_t device = 0;
    int dir_fd = DEFAULT_DIR_FD;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        mode = _PyLong_AsInt(args[1]);
        if (mode == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        if (!_Py_Dev_Converter(args[2], &device)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!MKNODAT_DIR_FD_CONVERTER(args[3], &dir_fd)) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_mknod_impl(module, &path, mode, device, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* (defined(HAVE_MKNOD) && defined(HAVE_MAKEDEV)) */

#if defined(HAVE_DEVICE_MACROS)

PyDoc_STRVAR(os_major__doc__,
"major($module, device, /)\n"
"--\n"
"\n"
"Extracts a device major number from a raw device number.");

#define OS_MAJOR_METHODDEF    \
    {"major", (PyCFunction)os_major, METH_O, os_major__doc__},

static unsigned int
os_major_impl(PyObject *module, dev_t device);

static PyObject *
os_major(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    dev_t device;
    unsigned int _return_value;

    if (!_Py_Dev_Converter(arg, &device)) {
        goto exit;
    }
    _return_value = os_major_impl(module, device);
    if ((_return_value == (unsigned int)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromUnsignedLong((unsigned long)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_DEVICE_MACROS) */

#if defined(HAVE_DEVICE_MACROS)

PyDoc_STRVAR(os_minor__doc__,
"minor($module, device, /)\n"
"--\n"
"\n"
"Extracts a device minor number from a raw device number.");

#define OS_MINOR_METHODDEF    \
    {"minor", (PyCFunction)os_minor, METH_O, os_minor__doc__},

static unsigned int
os_minor_impl(PyObject *module, dev_t device);

static PyObject *
os_minor(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    dev_t device;
    unsigned int _return_value;

    if (!_Py_Dev_Converter(arg, &device)) {
        goto exit;
    }
    _return_value = os_minor_impl(module, device);
    if ((_return_value == (unsigned int)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromUnsignedLong((unsigned long)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_DEVICE_MACROS) */

#if defined(HAVE_DEVICE_MACROS)

PyDoc_STRVAR(os_makedev__doc__,
"makedev($module, major, minor, /)\n"
"--\n"
"\n"
"Composes a raw device number from the major and minor device numbers.");

#define OS_MAKEDEV_METHODDEF    \
    {"makedev", (PyCFunction)(void(*)(void))os_makedev, METH_FASTCALL, os_makedev__doc__},

static dev_t
os_makedev_impl(PyObject *module, int major, int minor);

static PyObject *
os_makedev(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int major;
    int minor;
    dev_t _return_value;

    if (!_PyArg_CheckPositional("makedev", nargs, 2, 2)) {
        goto exit;
    }
    major = _PyLong_AsInt(args[0]);
    if (major == -1 && PyErr_Occurred()) {
        goto exit;
    }
    minor = _PyLong_AsInt(args[1]);
    if (minor == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = os_makedev_impl(module, major, minor);
    if ((_return_value == (dev_t)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _PyLong_FromDev(_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_DEVICE_MACROS) */

#if (defined HAVE_FTRUNCATE || defined MS_WINDOWS)

PyDoc_STRVAR(os_ftruncate__doc__,
"ftruncate($module, fd, length, /)\n"
"--\n"
"\n"
"Truncate a file, specified by file descriptor, to a specific length.");

#define OS_FTRUNCATE_METHODDEF    \
    {"ftruncate", (PyCFunction)(void(*)(void))os_ftruncate, METH_FASTCALL, os_ftruncate__doc__},

static PyObject *
os_ftruncate_impl(PyObject *module, int fd, Py_off_t length);

static PyObject *
os_ftruncate(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    Py_off_t length;

    if (!_PyArg_CheckPositional("ftruncate", nargs, 2, 2)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!Py_off_t_converter(args[1], &length)) {
        goto exit;
    }
    return_value = os_ftruncate_impl(module, fd, length);

exit:
    return return_value;
}

#endif /* (defined HAVE_FTRUNCATE || defined MS_WINDOWS) */

#if (defined HAVE_TRUNCATE || defined MS_WINDOWS)

PyDoc_STRVAR(os_truncate__doc__,
"truncate($module, /, path, length)\n"
"--\n"
"\n"
"Truncate a file, specified by path, to a specific length.\n"
"\n"
"On some platforms, path may also be specified as an open file descriptor.\n"
"  If this functionality is unavailable, using it raises an exception.");

#define OS_TRUNCATE_METHODDEF    \
    {"truncate", (PyCFunction)(void(*)(void))os_truncate, METH_FASTCALL|METH_KEYWORDS, os_truncate__doc__},

static PyObject *
os_truncate_impl(PyObject *module, path_t *path, Py_off_t length);

static PyObject *
os_truncate(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "length", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "truncate", 0};
    PyObject *argsbuf[2];
    path_t path = PATH_T_INITIALIZE("truncate", "path", 0, PATH_HAVE_FTRUNCATE);
    Py_off_t length;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!Py_off_t_converter(args[1], &length)) {
        goto exit;
    }
    return_value = os_truncate_impl(module, &path, length);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* (defined HAVE_TRUNCATE || defined MS_WINDOWS) */

#if (defined(HAVE_POSIX_FALLOCATE) && !defined(POSIX_FADVISE_AIX_BUG))

PyDoc_STRVAR(os_posix_fallocate__doc__,
"posix_fallocate($module, fd, offset, length, /)\n"
"--\n"
"\n"
"Ensure a file has allocated at least a particular number of bytes on disk.\n"
"\n"
"Ensure that the file specified by fd encompasses a range of bytes\n"
"starting at offset bytes from the beginning and continuing for length bytes.");

#define OS_POSIX_FALLOCATE_METHODDEF    \
    {"posix_fallocate", (PyCFunction)(void(*)(void))os_posix_fallocate, METH_FASTCALL, os_posix_fallocate__doc__},

static PyObject *
os_posix_fallocate_impl(PyObject *module, int fd, Py_off_t offset,
                        Py_off_t length);

static PyObject *
os_posix_fallocate(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    Py_off_t offset;
    Py_off_t length;

    if (!_PyArg_CheckPositional("posix_fallocate", nargs, 3, 3)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!Py_off_t_converter(args[1], &offset)) {
        goto exit;
    }
    if (!Py_off_t_converter(args[2], &length)) {
        goto exit;
    }
    return_value = os_posix_fallocate_impl(module, fd, offset, length);

exit:
    return return_value;
}

#endif /* (defined(HAVE_POSIX_FALLOCATE) && !defined(POSIX_FADVISE_AIX_BUG)) */

#if (defined(HAVE_POSIX_FADVISE) && !defined(POSIX_FADVISE_AIX_BUG))

PyDoc_STRVAR(os_posix_fadvise__doc__,
"posix_fadvise($module, fd, offset, length, advice, /)\n"
"--\n"
"\n"
"Announce an intention to access data in a specific pattern.\n"
"\n"
"Announce an intention to access data in a specific pattern, thus allowing\n"
"the kernel to make optimizations.\n"
"The advice applies to the region of the file specified by fd starting at\n"
"offset and continuing for length bytes.\n"
"advice is one of POSIX_FADV_NORMAL, POSIX_FADV_SEQUENTIAL,\n"
"POSIX_FADV_RANDOM, POSIX_FADV_NOREUSE, POSIX_FADV_WILLNEED, or\n"
"POSIX_FADV_DONTNEED.");

#define OS_POSIX_FADVISE_METHODDEF    \
    {"posix_fadvise", (PyCFunction)(void(*)(void))os_posix_fadvise, METH_FASTCALL, os_posix_fadvise__doc__},

static PyObject *
os_posix_fadvise_impl(PyObject *module, int fd, Py_off_t offset,
                      Py_off_t length, int advice);

static PyObject *
os_posix_fadvise(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    Py_off_t offset;
    Py_off_t length;
    int advice;

    if (!_PyArg_CheckPositional("posix_fadvise", nargs, 4, 4)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!Py_off_t_converter(args[1], &offset)) {
        goto exit;
    }
    if (!Py_off_t_converter(args[2], &length)) {
        goto exit;
    }
    advice = _PyLong_AsInt(args[3]);
    if (advice == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_posix_fadvise_impl(module, fd, offset, length, advice);

exit:
    return return_value;
}

#endif /* (defined(HAVE_POSIX_FADVISE) && !defined(POSIX_FADVISE_AIX_BUG)) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(os_putenv__doc__,
"putenv($module, name, value, /)\n"
"--\n"
"\n"
"Change or add an environment variable.");

#define OS_PUTENV_METHODDEF    \
    {"putenv", (PyCFunction)(void(*)(void))os_putenv, METH_FASTCALL, os_putenv__doc__},

static PyObject *
os_putenv_impl(PyObject *module, PyObject *name, PyObject *value);

static PyObject *
os_putenv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *name;
    PyObject *value;

    if (!_PyArg_CheckPositional("putenv", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("putenv", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    name = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("putenv", "argument 2", "str", args[1]);
        goto exit;
    }
    if (PyUnicode_READY(args[1]) == -1) {
        goto exit;
    }
    value = args[1];
    return_value = os_putenv_impl(module, name, value);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if !defined(MS_WINDOWS)

PyDoc_STRVAR(os_putenv__doc__,
"putenv($module, name, value, /)\n"
"--\n"
"\n"
"Change or add an environment variable.");

#define OS_PUTENV_METHODDEF    \
    {"putenv", (PyCFunction)(void(*)(void))os_putenv, METH_FASTCALL, os_putenv__doc__},

static PyObject *
os_putenv_impl(PyObject *module, PyObject *name, PyObject *value);

static PyObject *
os_putenv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *name = NULL;
    PyObject *value = NULL;

    if (!_PyArg_CheckPositional("putenv", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_FSConverter(args[0], &name)) {
        goto exit;
    }
    if (!PyUnicode_FSConverter(args[1], &value)) {
        goto exit;
    }
    return_value = os_putenv_impl(module, name, value);

exit:
    /* Cleanup for name */
    Py_XDECREF(name);
    /* Cleanup for value */
    Py_XDECREF(value);

    return return_value;
}

#endif /* !defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(os_unsetenv__doc__,
"unsetenv($module, name, /)\n"
"--\n"
"\n"
"Delete an environment variable.");

#define OS_UNSETENV_METHODDEF    \
    {"unsetenv", (PyCFunction)os_unsetenv, METH_O, os_unsetenv__doc__},

static PyObject *
os_unsetenv_impl(PyObject *module, PyObject *name);

static PyObject *
os_unsetenv(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *name;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("unsetenv", "argument", "str", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg) == -1) {
        goto exit;
    }
    name = arg;
    return_value = os_unsetenv_impl(module, name);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if !defined(MS_WINDOWS)

PyDoc_STRVAR(os_unsetenv__doc__,
"unsetenv($module, name, /)\n"
"--\n"
"\n"
"Delete an environment variable.");

#define OS_UNSETENV_METHODDEF    \
    {"unsetenv", (PyCFunction)os_unsetenv, METH_O, os_unsetenv__doc__},

static PyObject *
os_unsetenv_impl(PyObject *module, PyObject *name);

static PyObject *
os_unsetenv(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *name = NULL;

    if (!PyUnicode_FSConverter(arg, &name)) {
        goto exit;
    }
    return_value = os_unsetenv_impl(module, name);

exit:
    /* Cleanup for name */
    Py_XDECREF(name);

    return return_value;
}

#endif /* !defined(MS_WINDOWS) */

PyDoc_STRVAR(os_strerror__doc__,
"strerror($module, code, /)\n"
"--\n"
"\n"
"Translate an error code to a message string.");

#define OS_STRERROR_METHODDEF    \
    {"strerror", (PyCFunction)os_strerror, METH_O, os_strerror__doc__},

static PyObject *
os_strerror_impl(PyObject *module, int code);

static PyObject *
os_strerror(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int code;

    code = _PyLong_AsInt(arg);
    if (code == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_strerror_impl(module, code);

exit:
    return return_value;
}

#if defined(HAVE_SYS_WAIT_H) && defined(WCOREDUMP)

PyDoc_STRVAR(os_WCOREDUMP__doc__,
"WCOREDUMP($module, status, /)\n"
"--\n"
"\n"
"Return True if the process returning status was dumped to a core file.");

#define OS_WCOREDUMP_METHODDEF    \
    {"WCOREDUMP", (PyCFunction)os_WCOREDUMP, METH_O, os_WCOREDUMP__doc__},

static int
os_WCOREDUMP_impl(PyObject *module, int status);

static PyObject *
os_WCOREDUMP(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int status;
    int _return_value;

    status = _PyLong_AsInt(arg);
    if (status == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = os_WCOREDUMP_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_SYS_WAIT_H) && defined(WCOREDUMP) */

#if defined(HAVE_SYS_WAIT_H) && defined(WIFCONTINUED)

PyDoc_STRVAR(os_WIFCONTINUED__doc__,
"WIFCONTINUED($module, /, status)\n"
"--\n"
"\n"
"Return True if a particular process was continued from a job control stop.\n"
"\n"
"Return True if the process returning status was continued from a\n"
"job control stop.");

#define OS_WIFCONTINUED_METHODDEF    \
    {"WIFCONTINUED", (PyCFunction)(void(*)(void))os_WIFCONTINUED, METH_FASTCALL|METH_KEYWORDS, os_WIFCONTINUED__doc__},

static int
os_WIFCONTINUED_impl(PyObject *module, int status);

static PyObject *
os_WIFCONTINUED(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "WIFCONTINUED", 0};
    PyObject *argsbuf[1];
    int status;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    status = _PyLong_AsInt(args[0]);
    if (status == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = os_WIFCONTINUED_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_SYS_WAIT_H) && defined(WIFCONTINUED) */

#if defined(HAVE_SYS_WAIT_H) && defined(WIFSTOPPED)

PyDoc_STRVAR(os_WIFSTOPPED__doc__,
"WIFSTOPPED($module, /, status)\n"
"--\n"
"\n"
"Return True if the process returning status was stopped.");

#define OS_WIFSTOPPED_METHODDEF    \
    {"WIFSTOPPED", (PyCFunction)(void(*)(void))os_WIFSTOPPED, METH_FASTCALL|METH_KEYWORDS, os_WIFSTOPPED__doc__},

static int
os_WIFSTOPPED_impl(PyObject *module, int status);

static PyObject *
os_WIFSTOPPED(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "WIFSTOPPED", 0};
    PyObject *argsbuf[1];
    int status;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    status = _PyLong_AsInt(args[0]);
    if (status == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = os_WIFSTOPPED_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_SYS_WAIT_H) && defined(WIFSTOPPED) */

#if defined(HAVE_SYS_WAIT_H) && defined(WIFSIGNALED)

PyDoc_STRVAR(os_WIFSIGNALED__doc__,
"WIFSIGNALED($module, /, status)\n"
"--\n"
"\n"
"Return True if the process returning status was terminated by a signal.");

#define OS_WIFSIGNALED_METHODDEF    \
    {"WIFSIGNALED", (PyCFunction)(void(*)(void))os_WIFSIGNALED, METH_FASTCALL|METH_KEYWORDS, os_WIFSIGNALED__doc__},

static int
os_WIFSIGNALED_impl(PyObject *module, int status);

static PyObject *
os_WIFSIGNALED(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "WIFSIGNALED", 0};
    PyObject *argsbuf[1];
    int status;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    status = _PyLong_AsInt(args[0]);
    if (status == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = os_WIFSIGNALED_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_SYS_WAIT_H) && defined(WIFSIGNALED) */

#if defined(HAVE_SYS_WAIT_H) && defined(WIFEXITED)

PyDoc_STRVAR(os_WIFEXITED__doc__,
"WIFEXITED($module, /, status)\n"
"--\n"
"\n"
"Return True if the process returning status exited via the exit() system call.");

#define OS_WIFEXITED_METHODDEF    \
    {"WIFEXITED", (PyCFunction)(void(*)(void))os_WIFEXITED, METH_FASTCALL|METH_KEYWORDS, os_WIFEXITED__doc__},

static int
os_WIFEXITED_impl(PyObject *module, int status);

static PyObject *
os_WIFEXITED(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "WIFEXITED", 0};
    PyObject *argsbuf[1];
    int status;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    status = _PyLong_AsInt(args[0]);
    if (status == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = os_WIFEXITED_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_SYS_WAIT_H) && defined(WIFEXITED) */

#if defined(HAVE_SYS_WAIT_H) && defined(WEXITSTATUS)

PyDoc_STRVAR(os_WEXITSTATUS__doc__,
"WEXITSTATUS($module, /, status)\n"
"--\n"
"\n"
"Return the process return code from status.");

#define OS_WEXITSTATUS_METHODDEF    \
    {"WEXITSTATUS", (PyCFunction)(void(*)(void))os_WEXITSTATUS, METH_FASTCALL|METH_KEYWORDS, os_WEXITSTATUS__doc__},

static int
os_WEXITSTATUS_impl(PyObject *module, int status);

static PyObject *
os_WEXITSTATUS(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "WEXITSTATUS", 0};
    PyObject *argsbuf[1];
    int status;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    status = _PyLong_AsInt(args[0]);
    if (status == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = os_WEXITSTATUS_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_SYS_WAIT_H) && defined(WEXITSTATUS) */

#if defined(HAVE_SYS_WAIT_H) && defined(WTERMSIG)

PyDoc_STRVAR(os_WTERMSIG__doc__,
"WTERMSIG($module, /, status)\n"
"--\n"
"\n"
"Return the signal that terminated the process that provided the status value.");

#define OS_WTERMSIG_METHODDEF    \
    {"WTERMSIG", (PyCFunction)(void(*)(void))os_WTERMSIG, METH_FASTCALL|METH_KEYWORDS, os_WTERMSIG__doc__},

static int
os_WTERMSIG_impl(PyObject *module, int status);

static PyObject *
os_WTERMSIG(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "WTERMSIG", 0};
    PyObject *argsbuf[1];
    int status;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    status = _PyLong_AsInt(args[0]);
    if (status == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = os_WTERMSIG_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_SYS_WAIT_H) && defined(WTERMSIG) */

#if defined(HAVE_SYS_WAIT_H) && defined(WSTOPSIG)

PyDoc_STRVAR(os_WSTOPSIG__doc__,
"WSTOPSIG($module, /, status)\n"
"--\n"
"\n"
"Return the signal that stopped the process that provided the status value.");

#define OS_WSTOPSIG_METHODDEF    \
    {"WSTOPSIG", (PyCFunction)(void(*)(void))os_WSTOPSIG, METH_FASTCALL|METH_KEYWORDS, os_WSTOPSIG__doc__},

static int
os_WSTOPSIG_impl(PyObject *module, int status);

static PyObject *
os_WSTOPSIG(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "WSTOPSIG", 0};
    PyObject *argsbuf[1];
    int status;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    status = _PyLong_AsInt(args[0]);
    if (status == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = os_WSTOPSIG_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_SYS_WAIT_H) && defined(WSTOPSIG) */

#if (defined(HAVE_FSTATVFS) && defined(HAVE_SYS_STATVFS_H))

PyDoc_STRVAR(os_fstatvfs__doc__,
"fstatvfs($module, fd, /)\n"
"--\n"
"\n"
"Perform an fstatvfs system call on the given fd.\n"
"\n"
"Equivalent to statvfs(fd).");

#define OS_FSTATVFS_METHODDEF    \
    {"fstatvfs", (PyCFunction)os_fstatvfs, METH_O, os_fstatvfs__doc__},

static PyObject *
os_fstatvfs_impl(PyObject *module, int fd);

static PyObject *
os_fstatvfs(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;

    fd = _PyLong_AsInt(arg);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_fstatvfs_impl(module, fd);

exit:
    return return_value;
}

#endif /* (defined(HAVE_FSTATVFS) && defined(HAVE_SYS_STATVFS_H)) */

#if (defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H))

PyDoc_STRVAR(os_statvfs__doc__,
"statvfs($module, /, path)\n"
"--\n"
"\n"
"Perform a statvfs system call on the given path.\n"
"\n"
"path may always be specified as a string.\n"
"On some platforms, path may also be specified as an open file descriptor.\n"
"  If this functionality is unavailable, using it raises an exception.");

#define OS_STATVFS_METHODDEF    \
    {"statvfs", (PyCFunction)(void(*)(void))os_statvfs, METH_FASTCALL|METH_KEYWORDS, os_statvfs__doc__},

static PyObject *
os_statvfs_impl(PyObject *module, path_t *path);

static PyObject *
os_statvfs(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "statvfs", 0};
    PyObject *argsbuf[1];
    path_t path = PATH_T_INITIALIZE("statvfs", "path", 0, PATH_HAVE_FSTATVFS);

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    return_value = os_statvfs_impl(module, &path);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* (defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H)) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(os__getdiskusage__doc__,
"_getdiskusage($module, /, path)\n"
"--\n"
"\n"
"Return disk usage statistics about the given path as a (total, free) tuple.");

#define OS__GETDISKUSAGE_METHODDEF    \
    {"_getdiskusage", (PyCFunction)(void(*)(void))os__getdiskusage, METH_FASTCALL|METH_KEYWORDS, os__getdiskusage__doc__},

static PyObject *
os__getdiskusage_impl(PyObject *module, path_t *path);

static PyObject *
os__getdiskusage(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "_getdiskusage", 0};
    PyObject *argsbuf[1];
    path_t path = PATH_T_INITIALIZE("_getdiskusage", "path", 0, 0);

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    return_value = os__getdiskusage_impl(module, &path);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(HAVE_FPATHCONF)

PyDoc_STRVAR(os_fpathconf__doc__,
"fpathconf($module, fd, name, /)\n"
"--\n"
"\n"
"Return the configuration limit name for the file descriptor fd.\n"
"\n"
"If there is no limit, return -1.");

#define OS_FPATHCONF_METHODDEF    \
    {"fpathconf", (PyCFunction)(void(*)(void))os_fpathconf, METH_FASTCALL, os_fpathconf__doc__},

static long
os_fpathconf_impl(PyObject *module, int fd, int name);

static PyObject *
os_fpathconf(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int name;
    long _return_value;

    if (!_PyArg_CheckPositional("fpathconf", nargs, 2, 2)) {
        goto exit;
    }
    if (!fildes_converter(args[0], &fd)) {
        goto exit;
    }
    if (!conv_path_confname(args[1], &name)) {
        goto exit;
    }
    _return_value = os_fpathconf_impl(module, fd, name);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_FPATHCONF) */

#if defined(HAVE_PATHCONF)

PyDoc_STRVAR(os_pathconf__doc__,
"pathconf($module, /, path, name)\n"
"--\n"
"\n"
"Return the configuration limit name for the file or directory path.\n"
"\n"
"If there is no limit, return -1.\n"
"On some platforms, path may also be specified as an open file descriptor.\n"
"  If this functionality is unavailable, using it raises an exception.");

#define OS_PATHCONF_METHODDEF    \
    {"pathconf", (PyCFunction)(void(*)(void))os_pathconf, METH_FASTCALL|METH_KEYWORDS, os_pathconf__doc__},

static long
os_pathconf_impl(PyObject *module, path_t *path, int name);

static PyObject *
os_pathconf(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "name", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "pathconf", 0};
    PyObject *argsbuf[2];
    path_t path = PATH_T_INITIALIZE("pathconf", "path", 0, PATH_HAVE_FPATHCONF);
    int name;
    long _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!conv_path_confname(args[1], &name)) {
        goto exit;
    }
    _return_value = os_pathconf_impl(module, &path, name);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(HAVE_PATHCONF) */

#if defined(HAVE_CONFSTR)

PyDoc_STRVAR(os_confstr__doc__,
"confstr($module, name, /)\n"
"--\n"
"\n"
"Return a string-valued system configuration variable.");

#define OS_CONFSTR_METHODDEF    \
    {"confstr", (PyCFunction)os_confstr, METH_O, os_confstr__doc__},

static PyObject *
os_confstr_impl(PyObject *module, int name);

static PyObject *
os_confstr(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int name;

    if (!conv_confstr_confname(arg, &name)) {
        goto exit;
    }
    return_value = os_confstr_impl(module, name);

exit:
    return return_value;
}

#endif /* defined(HAVE_CONFSTR) */

#if defined(HAVE_SYSCONF)

PyDoc_STRVAR(os_sysconf__doc__,
"sysconf($module, name, /)\n"
"--\n"
"\n"
"Return an integer-valued system configuration variable.");

#define OS_SYSCONF_METHODDEF    \
    {"sysconf", (PyCFunction)os_sysconf, METH_O, os_sysconf__doc__},

static long
os_sysconf_impl(PyObject *module, int name);

static PyObject *
os_sysconf(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int name;
    long _return_value;

    if (!conv_sysconf_confname(arg, &name)) {
        goto exit;
    }
    _return_value = os_sysconf_impl(module, name);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_SYSCONF) */

PyDoc_STRVAR(os_abort__doc__,
"abort($module, /)\n"
"--\n"
"\n"
"Abort the interpreter immediately.\n"
"\n"
"This function \'dumps core\' or otherwise fails in the hardest way possible\n"
"on the hosting operating system.  This function never returns.");

#define OS_ABORT_METHODDEF    \
    {"abort", (PyCFunction)os_abort, METH_NOARGS, os_abort__doc__},

static PyObject *
os_abort_impl(PyObject *module);

static PyObject *
os_abort(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_abort_impl(module);
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(os_startfile__doc__,
"startfile($module, /, filepath, operation=<unrepresentable>)\n"
"--\n"
"\n"
"Start a file with its associated application.\n"
"\n"
"When \"operation\" is not specified or \"open\", this acts like\n"
"double-clicking the file in Explorer, or giving the file name as an\n"
"argument to the DOS \"start\" command: the file is opened with whatever\n"
"application (if any) its extension is associated.\n"
"When another \"operation\" is given, it specifies what should be done with\n"
"the file.  A typical operation is \"print\".\n"
"\n"
"startfile returns as soon as the associated application is launched.\n"
"There is no option to wait for the application to close, and no way\n"
"to retrieve the application\'s exit status.\n"
"\n"
"The filepath is relative to the current directory.  If you want to use\n"
"an absolute path, make sure the first character is not a slash (\"/\");\n"
"the underlying Win32 ShellExecute function doesn\'t work if it is.");

#define OS_STARTFILE_METHODDEF    \
    {"startfile", (PyCFunction)(void(*)(void))os_startfile, METH_FASTCALL|METH_KEYWORDS, os_startfile__doc__},

static PyObject *
os_startfile_impl(PyObject *module, path_t *filepath,
                  const Py_UNICODE *operation);

static PyObject *
os_startfile(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"filepath", "operation", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "startfile", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    path_t filepath = PATH_T_INITIALIZE("startfile", "filepath", 0, 0);
    const Py_UNICODE *operation = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &filepath)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("startfile", "argument 'operation'", "str", args[1]);
        goto exit;
    }
    #if USE_UNICODE_WCHAR_CACHE
    operation = _PyUnicode_AsUnicode(args[1]);
    #else /* USE_UNICODE_WCHAR_CACHE */
    operation = PyUnicode_AsWideCharString(args[1], NULL);
    #endif /* USE_UNICODE_WCHAR_CACHE */
    if (operation == NULL) {
        goto exit;
    }
skip_optional_pos:
    return_value = os_startfile_impl(module, &filepath, operation);

exit:
    /* Cleanup for filepath */
    path_cleanup(&filepath);
    /* Cleanup for operation */
    #if !USE_UNICODE_WCHAR_CACHE
    PyMem_Free((void *)operation);
    #endif /* USE_UNICODE_WCHAR_CACHE */

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(HAVE_GETLOADAVG)

PyDoc_STRVAR(os_getloadavg__doc__,
"getloadavg($module, /)\n"
"--\n"
"\n"
"Return average recent system load information.\n"
"\n"
"Return the number of processes in the system run queue averaged over\n"
"the last 1, 5, and 15 minutes as a tuple of three floats.\n"
"Raises OSError if the load average was unobtainable.");

#define OS_GETLOADAVG_METHODDEF    \
    {"getloadavg", (PyCFunction)os_getloadavg, METH_NOARGS, os_getloadavg__doc__},

static PyObject *
os_getloadavg_impl(PyObject *module);

static PyObject *
os_getloadavg(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_getloadavg_impl(module);
}

#endif /* defined(HAVE_GETLOADAVG) */

PyDoc_STRVAR(os_device_encoding__doc__,
"device_encoding($module, /, fd)\n"
"--\n"
"\n"
"Return a string describing the encoding of a terminal\'s file descriptor.\n"
"\n"
"The file descriptor must be attached to a terminal.\n"
"If the device is not a terminal, return None.");

#define OS_DEVICE_ENCODING_METHODDEF    \
    {"device_encoding", (PyCFunction)(void(*)(void))os_device_encoding, METH_FASTCALL|METH_KEYWORDS, os_device_encoding__doc__},

static PyObject *
os_device_encoding_impl(PyObject *module, int fd);

static PyObject *
os_device_encoding(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "device_encoding", 0};
    PyObject *argsbuf[1];
    int fd;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_device_encoding_impl(module, fd);

exit:
    return return_value;
}

#if defined(HAVE_SETRESUID)

PyDoc_STRVAR(os_setresuid__doc__,
"setresuid($module, ruid, euid, suid, /)\n"
"--\n"
"\n"
"Set the current process\'s real, effective, and saved user ids.");

#define OS_SETRESUID_METHODDEF    \
    {"setresuid", (PyCFunction)(void(*)(void))os_setresuid, METH_FASTCALL, os_setresuid__doc__},

static PyObject *
os_setresuid_impl(PyObject *module, uid_t ruid, uid_t euid, uid_t suid);

static PyObject *
os_setresuid(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    uid_t ruid;
    uid_t euid;
    uid_t suid;

    if (!_PyArg_CheckPositional("setresuid", nargs, 3, 3)) {
        goto exit;
    }
    if (!_Py_Uid_Converter(args[0], &ruid)) {
        goto exit;
    }
    if (!_Py_Uid_Converter(args[1], &euid)) {
        goto exit;
    }
    if (!_Py_Uid_Converter(args[2], &suid)) {
        goto exit;
    }
    return_value = os_setresuid_impl(module, ruid, euid, suid);

exit:
    return return_value;
}

#endif /* defined(HAVE_SETRESUID) */

#if defined(HAVE_SETRESGID)

PyDoc_STRVAR(os_setresgid__doc__,
"setresgid($module, rgid, egid, sgid, /)\n"
"--\n"
"\n"
"Set the current process\'s real, effective, and saved group ids.");

#define OS_SETRESGID_METHODDEF    \
    {"setresgid", (PyCFunction)(void(*)(void))os_setresgid, METH_FASTCALL, os_setresgid__doc__},

static PyObject *
os_setresgid_impl(PyObject *module, gid_t rgid, gid_t egid, gid_t sgid);

static PyObject *
os_setresgid(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    gid_t rgid;
    gid_t egid;
    gid_t sgid;

    if (!_PyArg_CheckPositional("setresgid", nargs, 3, 3)) {
        goto exit;
    }
    if (!_Py_Gid_Converter(args[0], &rgid)) {
        goto exit;
    }
    if (!_Py_Gid_Converter(args[1], &egid)) {
        goto exit;
    }
    if (!_Py_Gid_Converter(args[2], &sgid)) {
        goto exit;
    }
    return_value = os_setresgid_impl(module, rgid, egid, sgid);

exit:
    return return_value;
}

#endif /* defined(HAVE_SETRESGID) */

#if defined(HAVE_GETRESUID)

PyDoc_STRVAR(os_getresuid__doc__,
"getresuid($module, /)\n"
"--\n"
"\n"
"Return a tuple of the current process\'s real, effective, and saved user ids.");

#define OS_GETRESUID_METHODDEF    \
    {"getresuid", (PyCFunction)os_getresuid, METH_NOARGS, os_getresuid__doc__},

static PyObject *
os_getresuid_impl(PyObject *module);

static PyObject *
os_getresuid(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_getresuid_impl(module);
}

#endif /* defined(HAVE_GETRESUID) */

#if defined(HAVE_GETRESGID)

PyDoc_STRVAR(os_getresgid__doc__,
"getresgid($module, /)\n"
"--\n"
"\n"
"Return a tuple of the current process\'s real, effective, and saved group ids.");

#define OS_GETRESGID_METHODDEF    \
    {"getresgid", (PyCFunction)os_getresgid, METH_NOARGS, os_getresgid__doc__},

static PyObject *
os_getresgid_impl(PyObject *module);

static PyObject *
os_getresgid(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_getresgid_impl(module);
}

#endif /* defined(HAVE_GETRESGID) */

#if defined(USE_XATTRS)

PyDoc_STRVAR(os_getxattr__doc__,
"getxattr($module, /, path, attribute, *, follow_symlinks=True)\n"
"--\n"
"\n"
"Return the value of extended attribute attribute on path.\n"
"\n"
"path may be either a string, a path-like object, or an open file descriptor.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, getxattr will examine the symbolic link itself instead of the file\n"
"  the link points to.");

#define OS_GETXATTR_METHODDEF    \
    {"getxattr", (PyCFunction)(void(*)(void))os_getxattr, METH_FASTCALL|METH_KEYWORDS, os_getxattr__doc__},

static PyObject *
os_getxattr_impl(PyObject *module, path_t *path, path_t *attribute,
                 int follow_symlinks);

static PyObject *
os_getxattr(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "attribute", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "getxattr", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    path_t path = PATH_T_INITIALIZE("getxattr", "path", 0, 1);
    path_t attribute = PATH_T_INITIALIZE("getxattr", "attribute", 0, 0);
    int follow_symlinks = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!path_converter(args[1], &attribute)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    follow_symlinks = PyObject_IsTrue(args[2]);
    if (follow_symlinks < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_getxattr_impl(module, &path, &attribute, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);
    /* Cleanup for attribute */
    path_cleanup(&attribute);

    return return_value;
}

#endif /* defined(USE_XATTRS) */

#if defined(USE_XATTRS)

PyDoc_STRVAR(os_setxattr__doc__,
"setxattr($module, /, path, attribute, value, flags=0, *,\n"
"         follow_symlinks=True)\n"
"--\n"
"\n"
"Set extended attribute attribute on path to value.\n"
"\n"
"path may be either a string, a path-like object,  or an open file descriptor.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, setxattr will modify the symbolic link itself instead of the file\n"
"  the link points to.");

#define OS_SETXATTR_METHODDEF    \
    {"setxattr", (PyCFunction)(void(*)(void))os_setxattr, METH_FASTCALL|METH_KEYWORDS, os_setxattr__doc__},

static PyObject *
os_setxattr_impl(PyObject *module, path_t *path, path_t *attribute,
                 Py_buffer *value, int flags, int follow_symlinks);

static PyObject *
os_setxattr(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "attribute", "value", "flags", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "setxattr", 0};
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    path_t path = PATH_T_INITIALIZE("setxattr", "path", 0, 1);
    path_t attribute = PATH_T_INITIALIZE("setxattr", "attribute", 0, 0);
    Py_buffer value = {NULL, NULL};
    int flags = 0;
    int follow_symlinks = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!path_converter(args[1], &attribute)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[2], &value, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&value, 'C')) {
        _PyArg_BadArgument("setxattr", "argument 'value'", "contiguous buffer", args[2]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[3]) {
        flags = _PyLong_AsInt(args[3]);
        if (flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    follow_symlinks = PyObject_IsTrue(args[4]);
    if (follow_symlinks < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_setxattr_impl(module, &path, &attribute, &value, flags, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);
    /* Cleanup for attribute */
    path_cleanup(&attribute);
    /* Cleanup for value */
    if (value.obj) {
       PyBuffer_Release(&value);
    }

    return return_value;
}

#endif /* defined(USE_XATTRS) */

#if defined(USE_XATTRS)

PyDoc_STRVAR(os_removexattr__doc__,
"removexattr($module, /, path, attribute, *, follow_symlinks=True)\n"
"--\n"
"\n"
"Remove extended attribute attribute on path.\n"
"\n"
"path may be either a string, a path-like object, or an open file descriptor.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, removexattr will modify the symbolic link itself instead of the file\n"
"  the link points to.");

#define OS_REMOVEXATTR_METHODDEF    \
    {"removexattr", (PyCFunction)(void(*)(void))os_removexattr, METH_FASTCALL|METH_KEYWORDS, os_removexattr__doc__},

static PyObject *
os_removexattr_impl(PyObject *module, path_t *path, path_t *attribute,
                    int follow_symlinks);

static PyObject *
os_removexattr(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "attribute", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "removexattr", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    path_t path = PATH_T_INITIALIZE("removexattr", "path", 0, 1);
    path_t attribute = PATH_T_INITIALIZE("removexattr", "attribute", 0, 0);
    int follow_symlinks = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    if (!path_converter(args[1], &attribute)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    follow_symlinks = PyObject_IsTrue(args[2]);
    if (follow_symlinks < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_removexattr_impl(module, &path, &attribute, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);
    /* Cleanup for attribute */
    path_cleanup(&attribute);

    return return_value;
}

#endif /* defined(USE_XATTRS) */

#if defined(USE_XATTRS)

PyDoc_STRVAR(os_listxattr__doc__,
"listxattr($module, /, path=None, *, follow_symlinks=True)\n"
"--\n"
"\n"
"Return a list of extended attributes on path.\n"
"\n"
"path may be either None, a string, a path-like object, or an open file descriptor.\n"
"if path is None, listxattr will examine the current directory.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, listxattr will examine the symbolic link itself instead of the file\n"
"  the link points to.");

#define OS_LISTXATTR_METHODDEF    \
    {"listxattr", (PyCFunction)(void(*)(void))os_listxattr, METH_FASTCALL|METH_KEYWORDS, os_listxattr__doc__},

static PyObject *
os_listxattr_impl(PyObject *module, path_t *path, int follow_symlinks);

static PyObject *
os_listxattr(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "listxattr", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    path_t path = PATH_T_INITIALIZE("listxattr", "path", 1, 1);
    int follow_symlinks = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        if (!path_converter(args[0], &path)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    follow_symlinks = PyObject_IsTrue(args[1]);
    if (follow_symlinks < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = os_listxattr_impl(module, &path, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(USE_XATTRS) */

PyDoc_STRVAR(os_urandom__doc__,
"urandom($module, size, /)\n"
"--\n"
"\n"
"Return a bytes object containing random bytes suitable for cryptographic use.");

#define OS_URANDOM_METHODDEF    \
    {"urandom", (PyCFunction)os_urandom, METH_O, os_urandom__doc__},

static PyObject *
os_urandom_impl(PyObject *module, Py_ssize_t size);

static PyObject *
os_urandom(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_ssize_t size;

    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(arg);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        size = ival;
    }
    return_value = os_urandom_impl(module, size);

exit:
    return return_value;
}

#if defined(HAVE_MEMFD_CREATE)

PyDoc_STRVAR(os_memfd_create__doc__,
"memfd_create($module, /, name, flags=MFD_CLOEXEC)\n"
"--\n"
"\n");

#define OS_MEMFD_CREATE_METHODDEF    \
    {"memfd_create", (PyCFunction)(void(*)(void))os_memfd_create, METH_FASTCALL|METH_KEYWORDS, os_memfd_create__doc__},

static PyObject *
os_memfd_create_impl(PyObject *module, PyObject *name, unsigned int flags);

static PyObject *
os_memfd_create(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"name", "flags", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "memfd_create", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *name = NULL;
    unsigned int flags = MFD_CLOEXEC;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_FSConverter(args[0], &name)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    flags = (unsigned int)PyLong_AsUnsignedLongMask(args[1]);
    if (flags == (unsigned int)-1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = os_memfd_create_impl(module, name, flags);

exit:
    /* Cleanup for name */
    Py_XDECREF(name);

    return return_value;
}

#endif /* defined(HAVE_MEMFD_CREATE) */

#if (defined(TERMSIZE_USE_CONIO) || defined(TERMSIZE_USE_IOCTL))

PyDoc_STRVAR(os_get_terminal_size__doc__,
"get_terminal_size($module, fd=<unrepresentable>, /)\n"
"--\n"
"\n"
"Return the size of the terminal window as (columns, lines).\n"
"\n"
"The optional argument fd (default standard output) specifies\n"
"which file descriptor should be queried.\n"
"\n"
"If the file descriptor is not connected to a terminal, an OSError\n"
"is thrown.\n"
"\n"
"This function will only be defined if an implementation is\n"
"available for this system.\n"
"\n"
"shutil.get_terminal_size is the high-level function which should\n"
"normally be used, os.get_terminal_size is the low-level implementation.");

#define OS_GET_TERMINAL_SIZE_METHODDEF    \
    {"get_terminal_size", (PyCFunction)(void(*)(void))os_get_terminal_size, METH_FASTCALL, os_get_terminal_size__doc__},

static PyObject *
os_get_terminal_size_impl(PyObject *module, int fd);

static PyObject *
os_get_terminal_size(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd = fileno(stdout);

    if (!_PyArg_CheckPositional("get_terminal_size", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = os_get_terminal_size_impl(module, fd);

exit:
    return return_value;
}

#endif /* (defined(TERMSIZE_USE_CONIO) || defined(TERMSIZE_USE_IOCTL)) */

PyDoc_STRVAR(os_cpu_count__doc__,
"cpu_count($module, /)\n"
"--\n"
"\n"
"Return the number of CPUs in the system; return None if indeterminable.\n"
"\n"
"This number is not equivalent to the number of CPUs the current process can\n"
"use.  The number of usable CPUs can be obtained with\n"
"``len(os.sched_getaffinity(0))``");

#define OS_CPU_COUNT_METHODDEF    \
    {"cpu_count", (PyCFunction)os_cpu_count, METH_NOARGS, os_cpu_count__doc__},

static PyObject *
os_cpu_count_impl(PyObject *module);

static PyObject *
os_cpu_count(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return os_cpu_count_impl(module);
}

PyDoc_STRVAR(os_get_inheritable__doc__,
"get_inheritable($module, fd, /)\n"
"--\n"
"\n"
"Get the close-on-exe flag of the specified file descriptor.");

#define OS_GET_INHERITABLE_METHODDEF    \
    {"get_inheritable", (PyCFunction)os_get_inheritable, METH_O, os_get_inheritable__doc__},

static int
os_get_inheritable_impl(PyObject *module, int fd);

static PyObject *
os_get_inheritable(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;
    int _return_value;

    fd = _PyLong_AsInt(arg);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = os_get_inheritable_impl(module, fd);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(os_set_inheritable__doc__,
"set_inheritable($module, fd, inheritable, /)\n"
"--\n"
"\n"
"Set the inheritable flag of the specified file descriptor.");

#define OS_SET_INHERITABLE_METHODDEF    \
    {"set_inheritable", (PyCFunction)(void(*)(void))os_set_inheritable, METH_FASTCALL, os_set_inheritable__doc__},

static PyObject *
os_set_inheritable_impl(PyObject *module, int fd, int inheritable);

static PyObject *
os_set_inheritable(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int inheritable;

    if (!_PyArg_CheckPositional("set_inheritable", nargs, 2, 2)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    inheritable = _PyLong_AsInt(args[1]);
    if (inheritable == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_set_inheritable_impl(module, fd, inheritable);

exit:
    return return_value;
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(os_get_handle_inheritable__doc__,
"get_handle_inheritable($module, handle, /)\n"
"--\n"
"\n"
"Get the close-on-exe flag of the specified file descriptor.");

#define OS_GET_HANDLE_INHERITABLE_METHODDEF    \
    {"get_handle_inheritable", (PyCFunction)os_get_handle_inheritable, METH_O, os_get_handle_inheritable__doc__},

static int
os_get_handle_inheritable_impl(PyObject *module, intptr_t handle);

static PyObject *
os_get_handle_inheritable(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    intptr_t handle;
    int _return_value;

    if (!PyArg_Parse(arg, "" _Py_PARSE_INTPTR ":get_handle_inheritable", &handle)) {
        goto exit;
    }
    _return_value = os_get_handle_inheritable_impl(module, handle);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(os_set_handle_inheritable__doc__,
"set_handle_inheritable($module, handle, inheritable, /)\n"
"--\n"
"\n"
"Set the inheritable flag of the specified handle.");

#define OS_SET_HANDLE_INHERITABLE_METHODDEF    \
    {"set_handle_inheritable", (PyCFunction)(void(*)(void))os_set_handle_inheritable, METH_FASTCALL, os_set_handle_inheritable__doc__},

static PyObject *
os_set_handle_inheritable_impl(PyObject *module, intptr_t handle,
                               int inheritable);

static PyObject *
os_set_handle_inheritable(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    intptr_t handle;
    int inheritable;

    if (!_PyArg_ParseStack(args, nargs, "" _Py_PARSE_INTPTR "p:set_handle_inheritable",
        &handle, &inheritable)) {
        goto exit;
    }
    return_value = os_set_handle_inheritable_impl(module, handle, inheritable);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if !defined(MS_WINDOWS)

PyDoc_STRVAR(os_get_blocking__doc__,
"get_blocking($module, fd, /)\n"
"--\n"
"\n"
"Get the blocking mode of the file descriptor.\n"
"\n"
"Return False if the O_NONBLOCK flag is set, True if the flag is cleared.");

#define OS_GET_BLOCKING_METHODDEF    \
    {"get_blocking", (PyCFunction)os_get_blocking, METH_O, os_get_blocking__doc__},

static int
os_get_blocking_impl(PyObject *module, int fd);

static PyObject *
os_get_blocking(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;
    int _return_value;

    fd = _PyLong_AsInt(arg);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = os_get_blocking_impl(module, fd);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* !defined(MS_WINDOWS) */

#if !defined(MS_WINDOWS)

PyDoc_STRVAR(os_set_blocking__doc__,
"set_blocking($module, fd, blocking, /)\n"
"--\n"
"\n"
"Set the blocking mode of the specified file descriptor.\n"
"\n"
"Set the O_NONBLOCK flag if blocking is False,\n"
"clear the O_NONBLOCK flag otherwise.");

#define OS_SET_BLOCKING_METHODDEF    \
    {"set_blocking", (PyCFunction)(void(*)(void))os_set_blocking, METH_FASTCALL, os_set_blocking__doc__},

static PyObject *
os_set_blocking_impl(PyObject *module, int fd, int blocking);

static PyObject *
os_set_blocking(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int blocking;

    if (!_PyArg_CheckPositional("set_blocking", nargs, 2, 2)) {
        goto exit;
    }
    fd = _PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    blocking = _PyLong_AsInt(args[1]);
    if (blocking == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = os_set_blocking_impl(module, fd, blocking);

exit:
    return return_value;
}

#endif /* !defined(MS_WINDOWS) */

PyDoc_STRVAR(os_DirEntry_is_symlink__doc__,
"is_symlink($self, /)\n"
"--\n"
"\n"
"Return True if the entry is a symbolic link; cached per entry.");

#define OS_DIRENTRY_IS_SYMLINK_METHODDEF    \
    {"is_symlink", (PyCFunction)(void(*)(void))os_DirEntry_is_symlink, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, os_DirEntry_is_symlink__doc__},

static int
os_DirEntry_is_symlink_impl(DirEntry *self, PyTypeObject *defining_class);

static PyObject *
os_DirEntry_is_symlink(DirEntry *self, PyTypeObject *defining_class, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = { NULL};
    static _PyArg_Parser _parser = {":is_symlink", _keywords, 0};
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser
        )) {
        goto exit;
    }
    _return_value = os_DirEntry_is_symlink_impl(self, defining_class);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(os_DirEntry_stat__doc__,
"stat($self, /, *, follow_symlinks=True)\n"
"--\n"
"\n"
"Return stat_result object for the entry; cached per entry.");

#define OS_DIRENTRY_STAT_METHODDEF    \
    {"stat", (PyCFunction)(void(*)(void))os_DirEntry_stat, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, os_DirEntry_stat__doc__},

static PyObject *
os_DirEntry_stat_impl(DirEntry *self, PyTypeObject *defining_class,
                      int follow_symlinks);

static PyObject *
os_DirEntry_stat(DirEntry *self, PyTypeObject *defining_class, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"|$p:stat", _keywords, 0};
    int follow_symlinks = 1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &follow_symlinks)) {
        goto exit;
    }
    return_value = os_DirEntry_stat_impl(self, defining_class, follow_symlinks);

exit:
    return return_value;
}

PyDoc_STRVAR(os_DirEntry_is_dir__doc__,
"is_dir($self, /, *, follow_symlinks=True)\n"
"--\n"
"\n"
"Return True if the entry is a directory; cached per entry.");

#define OS_DIRENTRY_IS_DIR_METHODDEF    \
    {"is_dir", (PyCFunction)(void(*)(void))os_DirEntry_is_dir, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, os_DirEntry_is_dir__doc__},

static int
os_DirEntry_is_dir_impl(DirEntry *self, PyTypeObject *defining_class,
                        int follow_symlinks);

static PyObject *
os_DirEntry_is_dir(DirEntry *self, PyTypeObject *defining_class, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"|$p:is_dir", _keywords, 0};
    int follow_symlinks = 1;
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &follow_symlinks)) {
        goto exit;
    }
    _return_value = os_DirEntry_is_dir_impl(self, defining_class, follow_symlinks);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(os_DirEntry_is_file__doc__,
"is_file($self, /, *, follow_symlinks=True)\n"
"--\n"
"\n"
"Return True if the entry is a file; cached per entry.");

#define OS_DIRENTRY_IS_FILE_METHODDEF    \
    {"is_file", (PyCFunction)(void(*)(void))os_DirEntry_is_file, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, os_DirEntry_is_file__doc__},

static int
os_DirEntry_is_file_impl(DirEntry *self, PyTypeObject *defining_class,
                         int follow_symlinks);

static PyObject *
os_DirEntry_is_file(DirEntry *self, PyTypeObject *defining_class, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"|$p:is_file", _keywords, 0};
    int follow_symlinks = 1;
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &follow_symlinks)) {
        goto exit;
    }
    _return_value = os_DirEntry_is_file_impl(self, defining_class, follow_symlinks);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(os_DirEntry_inode__doc__,
"inode($self, /)\n"
"--\n"
"\n"
"Return inode of the entry; cached per entry.");

#define OS_DIRENTRY_INODE_METHODDEF    \
    {"inode", (PyCFunction)os_DirEntry_inode, METH_NOARGS, os_DirEntry_inode__doc__},

static PyObject *
os_DirEntry_inode_impl(DirEntry *self);

static PyObject *
os_DirEntry_inode(DirEntry *self, PyObject *Py_UNUSED(ignored))
{
    return os_DirEntry_inode_impl(self);
}

PyDoc_STRVAR(os_DirEntry___fspath____doc__,
"__fspath__($self, /)\n"
"--\n"
"\n"
"Returns the path for the entry.");

#define OS_DIRENTRY___FSPATH___METHODDEF    \
    {"__fspath__", (PyCFunction)os_DirEntry___fspath__, METH_NOARGS, os_DirEntry___fspath____doc__},

static PyObject *
os_DirEntry___fspath___impl(DirEntry *self);

static PyObject *
os_DirEntry___fspath__(DirEntry *self, PyObject *Py_UNUSED(ignored))
{
    return os_DirEntry___fspath___impl(self);
}

PyDoc_STRVAR(os_scandir__doc__,
"scandir($module, /, path=None)\n"
"--\n"
"\n"
"Return an iterator of DirEntry objects for given path.\n"
"\n"
"path can be specified as either str, bytes, or a path-like object.  If path\n"
"is bytes, the names of yielded DirEntry objects will also be bytes; in\n"
"all other circumstances they will be str.\n"
"\n"
"If path is None, uses the path=\'.\'.");

#define OS_SCANDIR_METHODDEF    \
    {"scandir", (PyCFunction)(void(*)(void))os_scandir, METH_FASTCALL|METH_KEYWORDS, os_scandir__doc__},

static PyObject *
os_scandir_impl(PyObject *module, path_t *path);

static PyObject *
os_scandir(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "scandir", 0};
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    path_t path = PATH_T_INITIALIZE("scandir", "path", 1, PATH_HAVE_FDOPENDIR);

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
skip_optional_pos:
    return_value = os_scandir_impl(module, &path);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

PyDoc_STRVAR(os_fspath__doc__,
"fspath($module, /, path)\n"
"--\n"
"\n"
"Return the file system path representation of the object.\n"
"\n"
"If the object is str or bytes, then allow it to pass through as-is. If the\n"
"object defines __fspath__(), then return the result of that method. All other\n"
"types raise a TypeError.");

#define OS_FSPATH_METHODDEF    \
    {"fspath", (PyCFunction)(void(*)(void))os_fspath, METH_FASTCALL|METH_KEYWORDS, os_fspath__doc__},

static PyObject *
os_fspath_impl(PyObject *module, PyObject *path);

static PyObject *
os_fspath(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "fspath", 0};
    PyObject *argsbuf[1];
    PyObject *path;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    path = args[0];
    return_value = os_fspath_impl(module, path);

exit:
    return return_value;
}

#if defined(HAVE_GETRANDOM_SYSCALL)

PyDoc_STRVAR(os_getrandom__doc__,
"getrandom($module, /, size, flags=0)\n"
"--\n"
"\n"
"Obtain a series of random bytes.");

#define OS_GETRANDOM_METHODDEF    \
    {"getrandom", (PyCFunction)(void(*)(void))os_getrandom, METH_FASTCALL|METH_KEYWORDS, os_getrandom__doc__},

static PyObject *
os_getrandom_impl(PyObject *module, Py_ssize_t size, int flags);

static PyObject *
os_getrandom(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"size", "flags", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "getrandom", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_ssize_t size;
    int flags = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        size = ival;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    flags = _PyLong_AsInt(args[1]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = os_getrandom_impl(module, size, flags);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETRANDOM_SYSCALL) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(os__add_dll_directory__doc__,
"_add_dll_directory($module, /, path)\n"
"--\n"
"\n"
"Add a path to the DLL search path.\n"
"\n"
"This search path is used when resolving dependencies for imported\n"
"extension modules (the module itself is resolved through sys.path),\n"
"and also by ctypes.\n"
"\n"
"Returns an opaque value that may be passed to os.remove_dll_directory\n"
"to remove this directory from the search path.");

#define OS__ADD_DLL_DIRECTORY_METHODDEF    \
    {"_add_dll_directory", (PyCFunction)(void(*)(void))os__add_dll_directory, METH_FASTCALL|METH_KEYWORDS, os__add_dll_directory__doc__},

static PyObject *
os__add_dll_directory_impl(PyObject *module, path_t *path);

static PyObject *
os__add_dll_directory(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "_add_dll_directory", 0};
    PyObject *argsbuf[1];
    path_t path = PATH_T_INITIALIZE("_add_dll_directory", "path", 0, 0);

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!path_converter(args[0], &path)) {
        goto exit;
    }
    return_value = os__add_dll_directory_impl(module, &path);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(os__remove_dll_directory__doc__,
"_remove_dll_directory($module, /, cookie)\n"
"--\n"
"\n"
"Removes a path from the DLL search path.\n"
"\n"
"The parameter is an opaque value that was returned from\n"
"os.add_dll_directory. You can only remove directories that you added\n"
"yourself.");

#define OS__REMOVE_DLL_DIRECTORY_METHODDEF    \
    {"_remove_dll_directory", (PyCFunction)(void(*)(void))os__remove_dll_directory, METH_FASTCALL|METH_KEYWORDS, os__remove_dll_directory__doc__},

static PyObject *
os__remove_dll_directory_impl(PyObject *module, PyObject *cookie);

static PyObject *
os__remove_dll_directory(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"cookie", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "_remove_dll_directory", 0};
    PyObject *argsbuf[1];
    PyObject *cookie;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    cookie = args[0];
    return_value = os__remove_dll_directory_impl(module, cookie);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if (defined(WIFEXITED) || defined(MS_WINDOWS))

PyDoc_STRVAR(os_waitstatus_to_exitcode__doc__,
"waitstatus_to_exitcode($module, /, status)\n"
"--\n"
"\n"
"Convert a wait status to an exit code.\n"
"\n"
"On Unix:\n"
"\n"
"* If WIFEXITED(status) is true, return WEXITSTATUS(status).\n"
"* If WIFSIGNALED(status) is true, return -WTERMSIG(status).\n"
"* Otherwise, raise a ValueError.\n"
"\n"
"On Windows, return status shifted right by 8 bits.\n"
"\n"
"On Unix, if the process is being traced or if waitpid() was called with\n"
"WUNTRACED option, the caller must first check if WIFSTOPPED(status) is true.\n"
"This function must not be called if WIFSTOPPED(status) is true.");

#define OS_WAITSTATUS_TO_EXITCODE_METHODDEF    \
    {"waitstatus_to_exitcode", (PyCFunction)(void(*)(void))os_waitstatus_to_exitcode, METH_FASTCALL|METH_KEYWORDS, os_waitstatus_to_exitcode__doc__},

static PyObject *
os_waitstatus_to_exitcode_impl(PyObject *module, PyObject *status_obj);

static PyObject *
os_waitstatus_to_exitcode(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "waitstatus_to_exitcode", 0};
    PyObject *argsbuf[1];
    PyObject *status_obj;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    status_obj = args[0];
    return_value = os_waitstatus_to_exitcode_impl(module, status_obj);

exit:
    return return_value;
}

#endif /* (defined(WIFEXITED) || defined(MS_WINDOWS)) */

#ifndef OS_TTYNAME_METHODDEF
    #define OS_TTYNAME_METHODDEF
#endif /* !defined(OS_TTYNAME_METHODDEF) */

#ifndef OS_CTERMID_METHODDEF
    #define OS_CTERMID_METHODDEF
#endif /* !defined(OS_CTERMID_METHODDEF) */

#ifndef OS_FCHDIR_METHODDEF
    #define OS_FCHDIR_METHODDEF
#endif /* !defined(OS_FCHDIR_METHODDEF) */

#ifndef OS_FCHMOD_METHODDEF
    #define OS_FCHMOD_METHODDEF
#endif /* !defined(OS_FCHMOD_METHODDEF) */

#ifndef OS_LCHMOD_METHODDEF
    #define OS_LCHMOD_METHODDEF
#endif /* !defined(OS_LCHMOD_METHODDEF) */

#ifndef OS_CHFLAGS_METHODDEF
    #define OS_CHFLAGS_METHODDEF
#endif /* !defined(OS_CHFLAGS_METHODDEF) */

#ifndef OS_LCHFLAGS_METHODDEF
    #define OS_LCHFLAGS_METHODDEF
#endif /* !defined(OS_LCHFLAGS_METHODDEF) */

#ifndef OS_CHROOT_METHODDEF
    #define OS_CHROOT_METHODDEF
#endif /* !defined(OS_CHROOT_METHODDEF) */

#ifndef OS_FSYNC_METHODDEF
    #define OS_FSYNC_METHODDEF
#endif /* !defined(OS_FSYNC_METHODDEF) */

#ifndef OS_SYNC_METHODDEF
    #define OS_SYNC_METHODDEF
#endif /* !defined(OS_SYNC_METHODDEF) */

#ifndef OS_FDATASYNC_METHODDEF
    #define OS_FDATASYNC_METHODDEF
#endif /* !defined(OS_FDATASYNC_METHODDEF) */

#ifndef OS_CHOWN_METHODDEF
    #define OS_CHOWN_METHODDEF
#endif /* !defined(OS_CHOWN_METHODDEF) */

#ifndef OS_FCHOWN_METHODDEF
    #define OS_FCHOWN_METHODDEF
#endif /* !defined(OS_FCHOWN_METHODDEF) */

#ifndef OS_LCHOWN_METHODDEF
    #define OS_LCHOWN_METHODDEF
#endif /* !defined(OS_LCHOWN_METHODDEF) */

#ifndef OS_LINK_METHODDEF
    #define OS_LINK_METHODDEF
#endif /* !defined(OS_LINK_METHODDEF) */

#ifndef OS__GETFULLPATHNAME_METHODDEF
    #define OS__GETFULLPATHNAME_METHODDEF
#endif /* !defined(OS__GETFULLPATHNAME_METHODDEF) */

#ifndef OS__GETFINALPATHNAME_METHODDEF
    #define OS__GETFINALPATHNAME_METHODDEF
#endif /* !defined(OS__GETFINALPATHNAME_METHODDEF) */

#ifndef OS__GETVOLUMEPATHNAME_METHODDEF
    #define OS__GETVOLUMEPATHNAME_METHODDEF
#endif /* !defined(OS__GETVOLUMEPATHNAME_METHODDEF) */

#ifndef OS_NICE_METHODDEF
    #define OS_NICE_METHODDEF
#endif /* !defined(OS_NICE_METHODDEF) */

#ifndef OS_GETPRIORITY_METHODDEF
    #define OS_GETPRIORITY_METHODDEF
#endif /* !defined(OS_GETPRIORITY_METHODDEF) */

#ifndef OS_SETPRIORITY_METHODDEF
    #define OS_SETPRIORITY_METHODDEF
#endif /* !defined(OS_SETPRIORITY_METHODDEF) */

#ifndef OS_SYSTEM_METHODDEF
    #define OS_SYSTEM_METHODDEF
#endif /* !defined(OS_SYSTEM_METHODDEF) */

#ifndef OS_UNAME_METHODDEF
    #define OS_UNAME_METHODDEF
#endif /* !defined(OS_UNAME_METHODDEF) */

#ifndef OS_EXECV_METHODDEF
    #define OS_EXECV_METHODDEF
#endif /* !defined(OS_EXECV_METHODDEF) */

#ifndef OS_EXECVE_METHODDEF
    #define OS_EXECVE_METHODDEF
#endif /* !defined(OS_EXECVE_METHODDEF) */

#ifndef OS_POSIX_SPAWN_METHODDEF
    #define OS_POSIX_SPAWN_METHODDEF
#endif /* !defined(OS_POSIX_SPAWN_METHODDEF) */

#ifndef OS_POSIX_SPAWNP_METHODDEF
    #define OS_POSIX_SPAWNP_METHODDEF
#endif /* !defined(OS_POSIX_SPAWNP_METHODDEF) */

#ifndef OS_SPAWNV_METHODDEF
    #define OS_SPAWNV_METHODDEF
#endif /* !defined(OS_SPAWNV_METHODDEF) */

#ifndef OS_SPAWNVE_METHODDEF
    #define OS_SPAWNVE_METHODDEF
#endif /* !defined(OS_SPAWNVE_METHODDEF) */

#ifndef OS_REGISTER_AT_FORK_METHODDEF
    #define OS_REGISTER_AT_FORK_METHODDEF
#endif /* !defined(OS_REGISTER_AT_FORK_METHODDEF) */

#ifndef OS_FORK1_METHODDEF
    #define OS_FORK1_METHODDEF
#endif /* !defined(OS_FORK1_METHODDEF) */

#ifndef OS_FORK_METHODDEF
    #define OS_FORK_METHODDEF
#endif /* !defined(OS_FORK_METHODDEF) */

#ifndef OS_SCHED_GET_PRIORITY_MAX_METHODDEF
    #define OS_SCHED_GET_PRIORITY_MAX_METHODDEF
#endif /* !defined(OS_SCHED_GET_PRIORITY_MAX_METHODDEF) */

#ifndef OS_SCHED_GET_PRIORITY_MIN_METHODDEF
    #define OS_SCHED_GET_PRIORITY_MIN_METHODDEF
#endif /* !defined(OS_SCHED_GET_PRIORITY_MIN_METHODDEF) */

#ifndef OS_SCHED_GETSCHEDULER_METHODDEF
    #define OS_SCHED_GETSCHEDULER_METHODDEF
#endif /* !defined(OS_SCHED_GETSCHEDULER_METHODDEF) */

#ifndef OS_SCHED_SETSCHEDULER_METHODDEF
    #define OS_SCHED_SETSCHEDULER_METHODDEF
#endif /* !defined(OS_SCHED_SETSCHEDULER_METHODDEF) */

#ifndef OS_SCHED_GETPARAM_METHODDEF
    #define OS_SCHED_GETPARAM_METHODDEF
#endif /* !defined(OS_SCHED_GETPARAM_METHODDEF) */

#ifndef OS_SCHED_SETPARAM_METHODDEF
    #define OS_SCHED_SETPARAM_METHODDEF
#endif /* !defined(OS_SCHED_SETPARAM_METHODDEF) */

#ifndef OS_SCHED_RR_GET_INTERVAL_METHODDEF
    #define OS_SCHED_RR_GET_INTERVAL_METHODDEF
#endif /* !defined(OS_SCHED_RR_GET_INTERVAL_METHODDEF) */

#ifndef OS_SCHED_YIELD_METHODDEF
    #define OS_SCHED_YIELD_METHODDEF
#endif /* !defined(OS_SCHED_YIELD_METHODDEF) */

#ifndef OS_SCHED_SETAFFINITY_METHODDEF
    #define OS_SCHED_SETAFFINITY_METHODDEF
#endif /* !defined(OS_SCHED_SETAFFINITY_METHODDEF) */

#ifndef OS_SCHED_GETAFFINITY_METHODDEF
    #define OS_SCHED_GETAFFINITY_METHODDEF
#endif /* !defined(OS_SCHED_GETAFFINITY_METHODDEF) */

#ifndef OS_OPENPTY_METHODDEF
    #define OS_OPENPTY_METHODDEF
#endif /* !defined(OS_OPENPTY_METHODDEF) */

#ifndef OS_FORKPTY_METHODDEF
    #define OS_FORKPTY_METHODDEF
#endif /* !defined(OS_FORKPTY_METHODDEF) */

#ifndef OS_GETEGID_METHODDEF
    #define OS_GETEGID_METHODDEF
#endif /* !defined(OS_GETEGID_METHODDEF) */

#ifndef OS_GETEUID_METHODDEF
    #define OS_GETEUID_METHODDEF
#endif /* !defined(OS_GETEUID_METHODDEF) */

#ifndef OS_GETGID_METHODDEF
    #define OS_GETGID_METHODDEF
#endif /* !defined(OS_GETGID_METHODDEF) */

#ifndef OS_GETPID_METHODDEF
    #define OS_GETPID_METHODDEF
#endif /* !defined(OS_GETPID_METHODDEF) */

#ifndef OS_GETGROUPLIST_METHODDEF
    #define OS_GETGROUPLIST_METHODDEF
#endif /* !defined(OS_GETGROUPLIST_METHODDEF) */

#ifndef OS_GETGROUPS_METHODDEF
    #define OS_GETGROUPS_METHODDEF
#endif /* !defined(OS_GETGROUPS_METHODDEF) */

#ifndef OS_INITGROUPS_METHODDEF
    #define OS_INITGROUPS_METHODDEF
#endif /* !defined(OS_INITGROUPS_METHODDEF) */

#ifndef OS_GETPGID_METHODDEF
    #define OS_GETPGID_METHODDEF
#endif /* !defined(OS_GETPGID_METHODDEF) */

#ifndef OS_GETPGRP_METHODDEF
    #define OS_GETPGRP_METHODDEF
#endif /* !defined(OS_GETPGRP_METHODDEF) */

#ifndef OS_SETPGRP_METHODDEF
    #define OS_SETPGRP_METHODDEF
#endif /* !defined(OS_SETPGRP_METHODDEF) */

#ifndef OS_GETPPID_METHODDEF
    #define OS_GETPPID_METHODDEF
#endif /* !defined(OS_GETPPID_METHODDEF) */

#ifndef OS_GETLOGIN_METHODDEF
    #define OS_GETLOGIN_METHODDEF
#endif /* !defined(OS_GETLOGIN_METHODDEF) */

#ifndef OS_GETUID_METHODDEF
    #define OS_GETUID_METHODDEF
#endif /* !defined(OS_GETUID_METHODDEF) */

#ifndef OS_KILL_METHODDEF
    #define OS_KILL_METHODDEF
#endif /* !defined(OS_KILL_METHODDEF) */

#ifndef OS_KILLPG_METHODDEF
    #define OS_KILLPG_METHODDEF
#endif /* !defined(OS_KILLPG_METHODDEF) */

#ifndef OS_PLOCK_METHODDEF
    #define OS_PLOCK_METHODDEF
#endif /* !defined(OS_PLOCK_METHODDEF) */

#ifndef OS_SETUID_METHODDEF
    #define OS_SETUID_METHODDEF
#endif /* !defined(OS_SETUID_METHODDEF) */

#ifndef OS_SETEUID_METHODDEF
    #define OS_SETEUID_METHODDEF
#endif /* !defined(OS_SETEUID_METHODDEF) */

#ifndef OS_SETEGID_METHODDEF
    #define OS_SETEGID_METHODDEF
#endif /* !defined(OS_SETEGID_METHODDEF) */

#ifndef OS_SETREUID_METHODDEF
    #define OS_SETREUID_METHODDEF
#endif /* !defined(OS_SETREUID_METHODDEF) */

#ifndef OS_SETREGID_METHODDEF
    #define OS_SETREGID_METHODDEF
#endif /* !defined(OS_SETREGID_METHODDEF) */

#ifndef OS_SETGID_METHODDEF
    #define OS_SETGID_METHODDEF
#endif /* !defined(OS_SETGID_METHODDEF) */

#ifndef OS_SETGROUPS_METHODDEF
    #define OS_SETGROUPS_METHODDEF
#endif /* !defined(OS_SETGROUPS_METHODDEF) */

#ifndef OS_WAIT3_METHODDEF
    #define OS_WAIT3_METHODDEF
#endif /* !defined(OS_WAIT3_METHODDEF) */

#ifndef OS_WAIT4_METHODDEF
    #define OS_WAIT4_METHODDEF
#endif /* !defined(OS_WAIT4_METHODDEF) */

#ifndef OS_WAITID_METHODDEF
    #define OS_WAITID_METHODDEF
#endif /* !defined(OS_WAITID_METHODDEF) */

#ifndef OS_WAITPID_METHODDEF
    #define OS_WAITPID_METHODDEF
#endif /* !defined(OS_WAITPID_METHODDEF) */

#ifndef OS_WAIT_METHODDEF
    #define OS_WAIT_METHODDEF
#endif /* !defined(OS_WAIT_METHODDEF) */

#ifndef OS_PIDFD_OPEN_METHODDEF
    #define OS_PIDFD_OPEN_METHODDEF
#endif /* !defined(OS_PIDFD_OPEN_METHODDEF) */

#ifndef OS_READLINK_METHODDEF
    #define OS_READLINK_METHODDEF
#endif /* !defined(OS_READLINK_METHODDEF) */

#ifndef OS_SYMLINK_METHODDEF
    #define OS_SYMLINK_METHODDEF
#endif /* !defined(OS_SYMLINK_METHODDEF) */

#ifndef OS_TIMES_METHODDEF
    #define OS_TIMES_METHODDEF
#endif /* !defined(OS_TIMES_METHODDEF) */

#ifndef OS_GETSID_METHODDEF
    #define OS_GETSID_METHODDEF
#endif /* !defined(OS_GETSID_METHODDEF) */

#ifndef OS_SETSID_METHODDEF
    #define OS_SETSID_METHODDEF
#endif /* !defined(OS_SETSID_METHODDEF) */

#ifndef OS_SETPGID_METHODDEF
    #define OS_SETPGID_METHODDEF
#endif /* !defined(OS_SETPGID_METHODDEF) */

#ifndef OS_TCGETPGRP_METHODDEF
    #define OS_TCGETPGRP_METHODDEF
#endif /* !defined(OS_TCGETPGRP_METHODDEF) */

#ifndef OS_TCSETPGRP_METHODDEF
    #define OS_TCSETPGRP_METHODDEF
#endif /* !defined(OS_TCSETPGRP_METHODDEF) */

#ifndef OS_LOCKF_METHODDEF
    #define OS_LOCKF_METHODDEF
#endif /* !defined(OS_LOCKF_METHODDEF) */

#ifndef OS_READV_METHODDEF
    #define OS_READV_METHODDEF
#endif /* !defined(OS_READV_METHODDEF) */

#ifndef OS_PREAD_METHODDEF
    #define OS_PREAD_METHODDEF
#endif /* !defined(OS_PREAD_METHODDEF) */

#ifndef OS_PREADV_METHODDEF
    #define OS_PREADV_METHODDEF
#endif /* !defined(OS_PREADV_METHODDEF) */

#ifndef OS_SENDFILE_METHODDEF
    #define OS_SENDFILE_METHODDEF
#endif /* !defined(OS_SENDFILE_METHODDEF) */

#ifndef OS__FCOPYFILE_METHODDEF
    #define OS__FCOPYFILE_METHODDEF
#endif /* !defined(OS__FCOPYFILE_METHODDEF) */

#ifndef OS_PIPE_METHODDEF
    #define OS_PIPE_METHODDEF
#endif /* !defined(OS_PIPE_METHODDEF) */

#ifndef OS_PIPE2_METHODDEF
    #define OS_PIPE2_METHODDEF
#endif /* !defined(OS_PIPE2_METHODDEF) */

#ifndef OS_WRITEV_METHODDEF
    #define OS_WRITEV_METHODDEF
#endif /* !defined(OS_WRITEV_METHODDEF) */

#ifndef OS_PWRITE_METHODDEF
    #define OS_PWRITE_METHODDEF
#endif /* !defined(OS_PWRITE_METHODDEF) */

#ifndef OS_PWRITEV_METHODDEF
    #define OS_PWRITEV_METHODDEF
#endif /* !defined(OS_PWRITEV_METHODDEF) */

#ifndef OS_COPY_FILE_RANGE_METHODDEF
    #define OS_COPY_FILE_RANGE_METHODDEF
#endif /* !defined(OS_COPY_FILE_RANGE_METHODDEF) */

#ifndef OS_MKFIFO_METHODDEF
    #define OS_MKFIFO_METHODDEF
#endif /* !defined(OS_MKFIFO_METHODDEF) */

#ifndef OS_MKNOD_METHODDEF
    #define OS_MKNOD_METHODDEF
#endif /* !defined(OS_MKNOD_METHODDEF) */

#ifndef OS_MAJOR_METHODDEF
    #define OS_MAJOR_METHODDEF
#endif /* !defined(OS_MAJOR_METHODDEF) */

#ifndef OS_MINOR_METHODDEF
    #define OS_MINOR_METHODDEF
#endif /* !defined(OS_MINOR_METHODDEF) */

#ifndef OS_MAKEDEV_METHODDEF
    #define OS_MAKEDEV_METHODDEF
#endif /* !defined(OS_MAKEDEV_METHODDEF) */

#ifndef OS_FTRUNCATE_METHODDEF
    #define OS_FTRUNCATE_METHODDEF
#endif /* !defined(OS_FTRUNCATE_METHODDEF) */

#ifndef OS_TRUNCATE_METHODDEF
    #define OS_TRUNCATE_METHODDEF
#endif /* !defined(OS_TRUNCATE_METHODDEF) */

#ifndef OS_POSIX_FALLOCATE_METHODDEF
    #define OS_POSIX_FALLOCATE_METHODDEF
#endif /* !defined(OS_POSIX_FALLOCATE_METHODDEF) */

#ifndef OS_POSIX_FADVISE_METHODDEF
    #define OS_POSIX_FADVISE_METHODDEF
#endif /* !defined(OS_POSIX_FADVISE_METHODDEF) */

#ifndef OS_PUTENV_METHODDEF
    #define OS_PUTENV_METHODDEF
#endif /* !defined(OS_PUTENV_METHODDEF) */

#ifndef OS_UNSETENV_METHODDEF
    #define OS_UNSETENV_METHODDEF
#endif /* !defined(OS_UNSETENV_METHODDEF) */

#ifndef OS_WCOREDUMP_METHODDEF
    #define OS_WCOREDUMP_METHODDEF
#endif /* !defined(OS_WCOREDUMP_METHODDEF) */

#ifndef OS_WIFCONTINUED_METHODDEF
    #define OS_WIFCONTINUED_METHODDEF
#endif /* !defined(OS_WIFCONTINUED_METHODDEF) */

#ifndef OS_WIFSTOPPED_METHODDEF
    #define OS_WIFSTOPPED_METHODDEF
#endif /* !defined(OS_WIFSTOPPED_METHODDEF) */

#ifndef OS_WIFSIGNALED_METHODDEF
    #define OS_WIFSIGNALED_METHODDEF
#endif /* !defined(OS_WIFSIGNALED_METHODDEF) */

#ifndef OS_WIFEXITED_METHODDEF
    #define OS_WIFEXITED_METHODDEF
#endif /* !defined(OS_WIFEXITED_METHODDEF) */

#ifndef OS_WEXITSTATUS_METHODDEF
    #define OS_WEXITSTATUS_METHODDEF
#endif /* !defined(OS_WEXITSTATUS_METHODDEF) */

#ifndef OS_WTERMSIG_METHODDEF
    #define OS_WTERMSIG_METHODDEF
#endif /* !defined(OS_WTERMSIG_METHODDEF) */

#ifndef OS_WSTOPSIG_METHODDEF
    #define OS_WSTOPSIG_METHODDEF
#endif /* !defined(OS_WSTOPSIG_METHODDEF) */

#ifndef OS_FSTATVFS_METHODDEF
    #define OS_FSTATVFS_METHODDEF
#endif /* !defined(OS_FSTATVFS_METHODDEF) */

#ifndef OS_STATVFS_METHODDEF
    #define OS_STATVFS_METHODDEF
#endif /* !defined(OS_STATVFS_METHODDEF) */

#ifndef OS__GETDISKUSAGE_METHODDEF
    #define OS__GETDISKUSAGE_METHODDEF
#endif /* !defined(OS__GETDISKUSAGE_METHODDEF) */

#ifndef OS_FPATHCONF_METHODDEF
    #define OS_FPATHCONF_METHODDEF
#endif /* !defined(OS_FPATHCONF_METHODDEF) */

#ifndef OS_PATHCONF_METHODDEF
    #define OS_PATHCONF_METHODDEF
#endif /* !defined(OS_PATHCONF_METHODDEF) */

#ifndef OS_CONFSTR_METHODDEF
    #define OS_CONFSTR_METHODDEF
#endif /* !defined(OS_CONFSTR_METHODDEF) */

#ifndef OS_SYSCONF_METHODDEF
    #define OS_SYSCONF_METHODDEF
#endif /* !defined(OS_SYSCONF_METHODDEF) */

#ifndef OS_STARTFILE_METHODDEF
    #define OS_STARTFILE_METHODDEF
#endif /* !defined(OS_STARTFILE_METHODDEF) */

#ifndef OS_GETLOADAVG_METHODDEF
    #define OS_GETLOADAVG_METHODDEF
#endif /* !defined(OS_GETLOADAVG_METHODDEF) */

#ifndef OS_SETRESUID_METHODDEF
    #define OS_SETRESUID_METHODDEF
#endif /* !defined(OS_SETRESUID_METHODDEF) */

#ifndef OS_SETRESGID_METHODDEF
    #define OS_SETRESGID_METHODDEF
#endif /* !defined(OS_SETRESGID_METHODDEF) */

#ifndef OS_GETRESUID_METHODDEF
    #define OS_GETRESUID_METHODDEF
#endif /* !defined(OS_GETRESUID_METHODDEF) */

#ifndef OS_GETRESGID_METHODDEF
    #define OS_GETRESGID_METHODDEF
#endif /* !defined(OS_GETRESGID_METHODDEF) */

#ifndef OS_GETXATTR_METHODDEF
    #define OS_GETXATTR_METHODDEF
#endif /* !defined(OS_GETXATTR_METHODDEF) */

#ifndef OS_SETXATTR_METHODDEF
    #define OS_SETXATTR_METHODDEF
#endif /* !defined(OS_SETXATTR_METHODDEF) */

#ifndef OS_REMOVEXATTR_METHODDEF
    #define OS_REMOVEXATTR_METHODDEF
#endif /* !defined(OS_REMOVEXATTR_METHODDEF) */

#ifndef OS_LISTXATTR_METHODDEF
    #define OS_LISTXATTR_METHODDEF
#endif /* !defined(OS_LISTXATTR_METHODDEF) */

#ifndef OS_MEMFD_CREATE_METHODDEF
    #define OS_MEMFD_CREATE_METHODDEF
#endif /* !defined(OS_MEMFD_CREATE_METHODDEF) */

#ifndef OS_GET_TERMINAL_SIZE_METHODDEF
    #define OS_GET_TERMINAL_SIZE_METHODDEF
#endif /* !defined(OS_GET_TERMINAL_SIZE_METHODDEF) */

#ifndef OS_GET_HANDLE_INHERITABLE_METHODDEF
    #define OS_GET_HANDLE_INHERITABLE_METHODDEF
#endif /* !defined(OS_GET_HANDLE_INHERITABLE_METHODDEF) */

#ifndef OS_SET_HANDLE_INHERITABLE_METHODDEF
    #define OS_SET_HANDLE_INHERITABLE_METHODDEF
#endif /* !defined(OS_SET_HANDLE_INHERITABLE_METHODDEF) */

#ifndef OS_GET_BLOCKING_METHODDEF
    #define OS_GET_BLOCKING_METHODDEF
#endif /* !defined(OS_GET_BLOCKING_METHODDEF) */

#ifndef OS_SET_BLOCKING_METHODDEF
    #define OS_SET_BLOCKING_METHODDEF
#endif /* !defined(OS_SET_BLOCKING_METHODDEF) */

#ifndef OS_GETRANDOM_METHODDEF
    #define OS_GETRANDOM_METHODDEF
#endif /* !defined(OS_GETRANDOM_METHODDEF) */

#ifndef OS__ADD_DLL_DIRECTORY_METHODDEF
    #define OS__ADD_DLL_DIRECTORY_METHODDEF
#endif /* !defined(OS__ADD_DLL_DIRECTORY_METHODDEF) */

#ifndef OS__REMOVE_DLL_DIRECTORY_METHODDEF
    #define OS__REMOVE_DLL_DIRECTORY_METHODDEF
#endif /* !defined(OS__REMOVE_DLL_DIRECTORY_METHODDEF) */

#ifndef OS_WAITSTATUS_TO_EXITCODE_METHODDEF
    #define OS_WAITSTATUS_TO_EXITCODE_METHODDEF
#endif /* !defined(OS_WAITSTATUS_TO_EXITCODE_METHODDEF) */
/*[clinic end generated code: output=a0fbdea47249ee0c input=a9049054013a1b77]*/
