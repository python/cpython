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
"    Path to be examined; can be string, bytes, path-like object or\n"
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
    {"stat", (PyCFunction)os_stat, METH_FASTCALL|METH_KEYWORDS, os_stat__doc__},

static PyObject *
os_stat_impl(PyObject *module, path_t *path, int dir_fd, int follow_symlinks);

static PyObject *
os_stat(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "dir_fd", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"O&|$O&p:stat", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("stat", "path", 0, 1);
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, FSTATAT_DIR_FD_CONVERTER, &dir_fd, &follow_symlinks)) {
        goto exit;
    }
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
    {"lstat", (PyCFunction)os_lstat, METH_FASTCALL|METH_KEYWORDS, os_lstat__doc__},

static PyObject *
os_lstat_impl(PyObject *module, path_t *path, int dir_fd);

static PyObject *
os_lstat(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "dir_fd", NULL};
    static _PyArg_Parser _parser = {"O&|$O&:lstat", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("lstat", "path", 0, 0);
    int dir_fd = DEFAULT_DIR_FD;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, FSTATAT_DIR_FD_CONVERTER, &dir_fd)) {
        goto exit;
    }
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
"    Path to be tested; can be string or bytes\n"
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
    {"access", (PyCFunction)os_access, METH_FASTCALL|METH_KEYWORDS, os_access__doc__},

static int
os_access_impl(PyObject *module, path_t *path, int mode, int dir_fd,
               int effective_ids, int follow_symlinks);

static PyObject *
os_access(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "mode", "dir_fd", "effective_ids", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"O&i|$O&pp:access", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("access", "path", 0, 0);
    int mode;
    int dir_fd = DEFAULT_DIR_FD;
    int effective_ids = 0;
    int follow_symlinks = 1;
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, &mode, FACCESSAT_DIR_FD_CONVERTER, &dir_fd, &effective_ids, &follow_symlinks)) {
        goto exit;
    }
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

static char *
os_ttyname_impl(PyObject *module, int fd);

static PyObject *
os_ttyname(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;
    char *_return_value;

    if (!PyArg_Parse(arg, "i:ttyname", &fd)) {
        goto exit;
    }
    _return_value = os_ttyname_impl(module, fd);
    if (_return_value == NULL) {
        goto exit;
    }
    return_value = PyUnicode_DecodeFSDefault(_return_value);

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
    {"chdir", (PyCFunction)os_chdir, METH_FASTCALL|METH_KEYWORDS, os_chdir__doc__},

static PyObject *
os_chdir_impl(PyObject *module, path_t *path);

static PyObject *
os_chdir(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {"O&:chdir", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("chdir", "path", 0, PATH_HAVE_FCHDIR);

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path)) {
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
    {"fchdir", (PyCFunction)os_fchdir, METH_FASTCALL|METH_KEYWORDS, os_fchdir__doc__},

static PyObject *
os_fchdir_impl(PyObject *module, int fd);

static PyObject *
os_fchdir(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", NULL};
    static _PyArg_Parser _parser = {"O&:fchdir", _keywords, 0};
    int fd;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        fildes_converter, &fd)) {
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
"    Path to be modified.  May always be specified as a str or bytes.\n"
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
    {"chmod", (PyCFunction)os_chmod, METH_FASTCALL|METH_KEYWORDS, os_chmod__doc__},

static PyObject *
os_chmod_impl(PyObject *module, path_t *path, int mode, int dir_fd,
              int follow_symlinks);

static PyObject *
os_chmod(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "mode", "dir_fd", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"O&i|$O&p:chmod", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("chmod", "path", 0, PATH_HAVE_FCHMOD);
    int mode;
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, &mode, FCHMODAT_DIR_FD_CONVERTER, &dir_fd, &follow_symlinks)) {
        goto exit;
    }
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
    {"fchmod", (PyCFunction)os_fchmod, METH_FASTCALL|METH_KEYWORDS, os_fchmod__doc__},

static PyObject *
os_fchmod_impl(PyObject *module, int fd, int mode);

static PyObject *
os_fchmod(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", "mode", NULL};
    static _PyArg_Parser _parser = {"ii:fchmod", _keywords, 0};
    int fd;
    int mode;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &fd, &mode)) {
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
    {"lchmod", (PyCFunction)os_lchmod, METH_FASTCALL|METH_KEYWORDS, os_lchmod__doc__},

static PyObject *
os_lchmod_impl(PyObject *module, path_t *path, int mode);

static PyObject *
os_lchmod(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "mode", NULL};
    static _PyArg_Parser _parser = {"O&i:lchmod", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("lchmod", "path", 0, 0);
    int mode;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, &mode)) {
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
    {"chflags", (PyCFunction)os_chflags, METH_FASTCALL|METH_KEYWORDS, os_chflags__doc__},

static PyObject *
os_chflags_impl(PyObject *module, path_t *path, unsigned long flags,
                int follow_symlinks);

static PyObject *
os_chflags(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "flags", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"O&k|p:chflags", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("chflags", "path", 0, 0);
    unsigned long flags;
    int follow_symlinks = 1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, &flags, &follow_symlinks)) {
        goto exit;
    }
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
    {"lchflags", (PyCFunction)os_lchflags, METH_FASTCALL|METH_KEYWORDS, os_lchflags__doc__},

static PyObject *
os_lchflags_impl(PyObject *module, path_t *path, unsigned long flags);

static PyObject *
os_lchflags(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "flags", NULL};
    static _PyArg_Parser _parser = {"O&k:lchflags", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("lchflags", "path", 0, 0);
    unsigned long flags;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, &flags)) {
        goto exit;
    }
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
    {"chroot", (PyCFunction)os_chroot, METH_FASTCALL|METH_KEYWORDS, os_chroot__doc__},

static PyObject *
os_chroot_impl(PyObject *module, path_t *path);

static PyObject *
os_chroot(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {"O&:chroot", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("chroot", "path", 0, 0);

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path)) {
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
    {"fsync", (PyCFunction)os_fsync, METH_FASTCALL|METH_KEYWORDS, os_fsync__doc__},

static PyObject *
os_fsync_impl(PyObject *module, int fd);

static PyObject *
os_fsync(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", NULL};
    static _PyArg_Parser _parser = {"O&:fsync", _keywords, 0};
    int fd;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        fildes_converter, &fd)) {
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
    {"fdatasync", (PyCFunction)os_fdatasync, METH_FASTCALL|METH_KEYWORDS, os_fdatasync__doc__},

static PyObject *
os_fdatasync_impl(PyObject *module, int fd);

static PyObject *
os_fdatasync(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", NULL};
    static _PyArg_Parser _parser = {"O&:fdatasync", _keywords, 0};
    int fd;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        fildes_converter, &fd)) {
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
"    Path to be examined; can be string, bytes, or open-file-descriptor int.\n"
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
    {"chown", (PyCFunction)os_chown, METH_FASTCALL|METH_KEYWORDS, os_chown__doc__},

static PyObject *
os_chown_impl(PyObject *module, path_t *path, uid_t uid, gid_t gid,
              int dir_fd, int follow_symlinks);

static PyObject *
os_chown(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "uid", "gid", "dir_fd", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"O&O&O&|$O&p:chown", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("chown", "path", 0, PATH_HAVE_FCHOWN);
    uid_t uid;
    gid_t gid;
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, _Py_Uid_Converter, &uid, _Py_Gid_Converter, &gid, FCHOWNAT_DIR_FD_CONVERTER, &dir_fd, &follow_symlinks)) {
        goto exit;
    }
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
    {"fchown", (PyCFunction)os_fchown, METH_FASTCALL|METH_KEYWORDS, os_fchown__doc__},

static PyObject *
os_fchown_impl(PyObject *module, int fd, uid_t uid, gid_t gid);

static PyObject *
os_fchown(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", "uid", "gid", NULL};
    static _PyArg_Parser _parser = {"iO&O&:fchown", _keywords, 0};
    int fd;
    uid_t uid;
    gid_t gid;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &fd, _Py_Uid_Converter, &uid, _Py_Gid_Converter, &gid)) {
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
    {"lchown", (PyCFunction)os_lchown, METH_FASTCALL|METH_KEYWORDS, os_lchown__doc__},

static PyObject *
os_lchown_impl(PyObject *module, path_t *path, uid_t uid, gid_t gid);

static PyObject *
os_lchown(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "uid", "gid", NULL};
    static _PyArg_Parser _parser = {"O&O&O&:lchown", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("lchown", "path", 0, 0);
    uid_t uid;
    gid_t gid;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, _Py_Uid_Converter, &uid, _Py_Gid_Converter, &gid)) {
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
    {"link", (PyCFunction)os_link, METH_FASTCALL|METH_KEYWORDS, os_link__doc__},

static PyObject *
os_link_impl(PyObject *module, path_t *src, path_t *dst, int src_dir_fd,
             int dst_dir_fd, int follow_symlinks);

static PyObject *
os_link(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"src", "dst", "src_dir_fd", "dst_dir_fd", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"O&O&|$O&O&p:link", _keywords, 0};
    path_t src = PATH_T_INITIALIZE("link", "src", 0, 0);
    path_t dst = PATH_T_INITIALIZE("link", "dst", 0, 0);
    int src_dir_fd = DEFAULT_DIR_FD;
    int dst_dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &src, path_converter, &dst, dir_fd_converter, &src_dir_fd, dir_fd_converter, &dst_dir_fd, &follow_symlinks)) {
        goto exit;
    }
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
"path can be specified as either str or bytes.  If path is bytes,\n"
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
    {"listdir", (PyCFunction)os_listdir, METH_FASTCALL|METH_KEYWORDS, os_listdir__doc__},

static PyObject *
os_listdir_impl(PyObject *module, path_t *path);

static PyObject *
os_listdir(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {"|O&:listdir", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("listdir", "path", 1, PATH_HAVE_FDOPENDIR);

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path)) {
        goto exit;
    }
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

    if (!PyArg_Parse(arg, "O&:_getfullpathname", path_converter, &path)) {
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
os__getfinalpathname_impl(PyObject *module, PyObject *path);

static PyObject *
os__getfinalpathname(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *path;

    if (!PyArg_Parse(arg, "U:_getfinalpathname", &path)) {
        goto exit;
    }
    return_value = os__getfinalpathname_impl(module, path);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(os__isdir__doc__,
"_isdir($module, path, /)\n"
"--\n"
"\n"
"Return true if the pathname refers to an existing directory.");

#define OS__ISDIR_METHODDEF    \
    {"_isdir", (PyCFunction)os__isdir, METH_O, os__isdir__doc__},

static PyObject *
os__isdir_impl(PyObject *module, path_t *path);

static PyObject *
os__isdir(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    path_t path = PATH_T_INITIALIZE("_isdir", "path", 0, 0);

    if (!PyArg_Parse(arg, "O&:_isdir", path_converter, &path)) {
        goto exit;
    }
    return_value = os__isdir_impl(module, &path);

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
    {"_getvolumepathname", (PyCFunction)os__getvolumepathname, METH_FASTCALL|METH_KEYWORDS, os__getvolumepathname__doc__},

static PyObject *
os__getvolumepathname_impl(PyObject *module, PyObject *path);

static PyObject *
os__getvolumepathname(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {"U:_getvolumepathname", _keywords, 0};
    PyObject *path;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &path)) {
        goto exit;
    }
    return_value = os__getvolumepathname_impl(module, path);

exit:
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
    {"mkdir", (PyCFunction)os_mkdir, METH_FASTCALL|METH_KEYWORDS, os_mkdir__doc__},

static PyObject *
os_mkdir_impl(PyObject *module, path_t *path, int mode, int dir_fd);

static PyObject *
os_mkdir(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "mode", "dir_fd", NULL};
    static _PyArg_Parser _parser = {"O&|i$O&:mkdir", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("mkdir", "path", 0, 0);
    int mode = 511;
    int dir_fd = DEFAULT_DIR_FD;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, &mode, MKDIRAT_DIR_FD_CONVERTER, &dir_fd)) {
        goto exit;
    }
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

    if (!PyArg_Parse(arg, "i:nice", &increment)) {
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
    {"getpriority", (PyCFunction)os_getpriority, METH_FASTCALL|METH_KEYWORDS, os_getpriority__doc__},

static PyObject *
os_getpriority_impl(PyObject *module, int which, int who);

static PyObject *
os_getpriority(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"which", "who", NULL};
    static _PyArg_Parser _parser = {"ii:getpriority", _keywords, 0};
    int which;
    int who;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &which, &who)) {
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
    {"setpriority", (PyCFunction)os_setpriority, METH_FASTCALL|METH_KEYWORDS, os_setpriority__doc__},

static PyObject *
os_setpriority_impl(PyObject *module, int which, int who, int priority);

static PyObject *
os_setpriority(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"which", "who", "priority", NULL};
    static _PyArg_Parser _parser = {"iii:setpriority", _keywords, 0};
    int which;
    int who;
    int priority;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &which, &who, &priority)) {
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
    {"rename", (PyCFunction)os_rename, METH_FASTCALL|METH_KEYWORDS, os_rename__doc__},

static PyObject *
os_rename_impl(PyObject *module, path_t *src, path_t *dst, int src_dir_fd,
               int dst_dir_fd);

static PyObject *
os_rename(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"src", "dst", "src_dir_fd", "dst_dir_fd", NULL};
    static _PyArg_Parser _parser = {"O&O&|$O&O&:rename", _keywords, 0};
    path_t src = PATH_T_INITIALIZE("rename", "src", 0, 0);
    path_t dst = PATH_T_INITIALIZE("rename", "dst", 0, 0);
    int src_dir_fd = DEFAULT_DIR_FD;
    int dst_dir_fd = DEFAULT_DIR_FD;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &src, path_converter, &dst, dir_fd_converter, &src_dir_fd, dir_fd_converter, &dst_dir_fd)) {
        goto exit;
    }
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
"  If they are unavailable, using them will raise a NotImplementedError.\"");

#define OS_REPLACE_METHODDEF    \
    {"replace", (PyCFunction)os_replace, METH_FASTCALL|METH_KEYWORDS, os_replace__doc__},

static PyObject *
os_replace_impl(PyObject *module, path_t *src, path_t *dst, int src_dir_fd,
                int dst_dir_fd);

static PyObject *
os_replace(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"src", "dst", "src_dir_fd", "dst_dir_fd", NULL};
    static _PyArg_Parser _parser = {"O&O&|$O&O&:replace", _keywords, 0};
    path_t src = PATH_T_INITIALIZE("replace", "src", 0, 0);
    path_t dst = PATH_T_INITIALIZE("replace", "dst", 0, 0);
    int src_dir_fd = DEFAULT_DIR_FD;
    int dst_dir_fd = DEFAULT_DIR_FD;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &src, path_converter, &dst, dir_fd_converter, &src_dir_fd, dir_fd_converter, &dst_dir_fd)) {
        goto exit;
    }
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
    {"rmdir", (PyCFunction)os_rmdir, METH_FASTCALL|METH_KEYWORDS, os_rmdir__doc__},

static PyObject *
os_rmdir_impl(PyObject *module, path_t *path, int dir_fd);

static PyObject *
os_rmdir(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "dir_fd", NULL};
    static _PyArg_Parser _parser = {"O&|$O&:rmdir", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("rmdir", "path", 0, 0);
    int dir_fd = DEFAULT_DIR_FD;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, UNLINKAT_DIR_FD_CONVERTER, &dir_fd)) {
        goto exit;
    }
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
    {"system", (PyCFunction)os_system, METH_FASTCALL|METH_KEYWORDS, os_system__doc__},

static long
os_system_impl(PyObject *module, Py_UNICODE *command);

static PyObject *
os_system(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"command", NULL};
    static _PyArg_Parser _parser = {"u:system", _keywords, 0};
    Py_UNICODE *command;
    long _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &command)) {
        goto exit;
    }
    _return_value = os_system_impl(module, command);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
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
    {"system", (PyCFunction)os_system, METH_FASTCALL|METH_KEYWORDS, os_system__doc__},

static long
os_system_impl(PyObject *module, PyObject *command);

static PyObject *
os_system(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"command", NULL};
    static _PyArg_Parser _parser = {"O&:system", _keywords, 0};
    PyObject *command = NULL;
    long _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        PyUnicode_FSConverter, &command)) {
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

    if (!PyArg_Parse(arg, "i:umask", &mask)) {
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
    {"unlink", (PyCFunction)os_unlink, METH_FASTCALL|METH_KEYWORDS, os_unlink__doc__},

static PyObject *
os_unlink_impl(PyObject *module, path_t *path, int dir_fd);

static PyObject *
os_unlink(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "dir_fd", NULL};
    static _PyArg_Parser _parser = {"O&|$O&:unlink", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("unlink", "path", 0, 0);
    int dir_fd = DEFAULT_DIR_FD;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, UNLINKAT_DIR_FD_CONVERTER, &dir_fd)) {
        goto exit;
    }
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
    {"remove", (PyCFunction)os_remove, METH_FASTCALL|METH_KEYWORDS, os_remove__doc__},

static PyObject *
os_remove_impl(PyObject *module, path_t *path, int dir_fd);

static PyObject *
os_remove(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "dir_fd", NULL};
    static _PyArg_Parser _parser = {"O&|$O&:remove", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("remove", "path", 0, 0);
    int dir_fd = DEFAULT_DIR_FD;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, UNLINKAT_DIR_FD_CONVERTER, &dir_fd)) {
        goto exit;
    }
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
"utime($module, /, path, times=None, *, ns=None, dir_fd=None,\n"
"      follow_symlinks=True)\n"
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
    {"utime", (PyCFunction)os_utime, METH_FASTCALL|METH_KEYWORDS, os_utime__doc__},

static PyObject *
os_utime_impl(PyObject *module, path_t *path, PyObject *times, PyObject *ns,
              int dir_fd, int follow_symlinks);

static PyObject *
os_utime(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "times", "ns", "dir_fd", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"O&|O$OO&p:utime", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("utime", "path", 0, PATH_UTIME_HAVE_FD);
    PyObject *times = NULL;
    PyObject *ns = NULL;
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, &times, &ns, FUTIMENSAT_DIR_FD_CONVERTER, &dir_fd, &follow_symlinks)) {
        goto exit;
    }
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
    {"_exit", (PyCFunction)os__exit, METH_FASTCALL|METH_KEYWORDS, os__exit__doc__},

static PyObject *
os__exit_impl(PyObject *module, int status);

static PyObject *
os__exit(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {"i:_exit", _keywords, 0};
    int status;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &status)) {
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
    {"execv", (PyCFunction)os_execv, METH_FASTCALL, os_execv__doc__},

static PyObject *
os_execv_impl(PyObject *module, path_t *path, PyObject *argv);

static PyObject *
os_execv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    path_t path = PATH_T_INITIALIZE("execv", "path", 0, 0);
    PyObject *argv;

    if (!_PyArg_ParseStack(args, nargs, "O&O:execv",
        path_converter, &path, &argv)) {
        goto exit;
    }
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
    {"execve", (PyCFunction)os_execve, METH_FASTCALL|METH_KEYWORDS, os_execve__doc__},

static PyObject *
os_execve_impl(PyObject *module, path_t *path, PyObject *argv, PyObject *env);

static PyObject *
os_execve(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "argv", "env", NULL};
    static _PyArg_Parser _parser = {"O&OO:execve", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("execve", "path", 0, PATH_HAVE_FEXECVE);
    PyObject *argv;
    PyObject *env;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, &argv, &env)) {
        goto exit;
    }
    return_value = os_execve_impl(module, &path, argv, env);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* defined(HAVE_EXECV) */

#if (defined(HAVE_SPAWNV) || defined(HAVE_WSPAWNV))

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
    {"spawnv", (PyCFunction)os_spawnv, METH_FASTCALL, os_spawnv__doc__},

static PyObject *
os_spawnv_impl(PyObject *module, int mode, path_t *path, PyObject *argv);

static PyObject *
os_spawnv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int mode;
    path_t path = PATH_T_INITIALIZE("spawnv", "path", 0, 0);
    PyObject *argv;

    if (!_PyArg_ParseStack(args, nargs, "iO&O:spawnv",
        &mode, path_converter, &path, &argv)) {
        goto exit;
    }
    return_value = os_spawnv_impl(module, mode, &path, argv);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* (defined(HAVE_SPAWNV) || defined(HAVE_WSPAWNV)) */

#if (defined(HAVE_SPAWNV) || defined(HAVE_WSPAWNV))

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
    {"spawnve", (PyCFunction)os_spawnve, METH_FASTCALL, os_spawnve__doc__},

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

    if (!_PyArg_ParseStack(args, nargs, "iO&OO:spawnve",
        &mode, path_converter, &path, &argv, &env)) {
        goto exit;
    }
    return_value = os_spawnve_impl(module, mode, &path, argv, env);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

#endif /* (defined(HAVE_SPAWNV) || defined(HAVE_WSPAWNV)) */

#if defined(HAVE_FORK)

PyDoc_STRVAR(os_register_at_fork__doc__,
"register_at_fork($module, /, *, before=None, after_in_child=None,\n"
"                 after_in_parent=None)\n"
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
    {"register_at_fork", (PyCFunction)os_register_at_fork, METH_FASTCALL|METH_KEYWORDS, os_register_at_fork__doc__},

static PyObject *
os_register_at_fork_impl(PyObject *module, PyObject *before,
                         PyObject *after_in_child, PyObject *after_in_parent);

static PyObject *
os_register_at_fork(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"before", "after_in_child", "after_in_parent", NULL};
    static _PyArg_Parser _parser = {"|$OOO:register_at_fork", _keywords, 0};
    PyObject *before = NULL;
    PyObject *after_in_child = NULL;
    PyObject *after_in_parent = NULL;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &before, &after_in_child, &after_in_parent)) {
        goto exit;
    }
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
    {"sched_get_priority_max", (PyCFunction)os_sched_get_priority_max, METH_FASTCALL|METH_KEYWORDS, os_sched_get_priority_max__doc__},

static PyObject *
os_sched_get_priority_max_impl(PyObject *module, int policy);

static PyObject *
os_sched_get_priority_max(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"policy", NULL};
    static _PyArg_Parser _parser = {"i:sched_get_priority_max", _keywords, 0};
    int policy;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &policy)) {
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
    {"sched_get_priority_min", (PyCFunction)os_sched_get_priority_min, METH_FASTCALL|METH_KEYWORDS, os_sched_get_priority_min__doc__},

static PyObject *
os_sched_get_priority_min_impl(PyObject *module, int policy);

static PyObject *
os_sched_get_priority_min(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"policy", NULL};
    static _PyArg_Parser _parser = {"i:sched_get_priority_min", _keywords, 0};
    int policy;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &policy)) {
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
"Get the scheduling policy for the process identifiedy by pid.\n"
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

#if defined(HAVE_SCHED_H) && (defined(HAVE_SCHED_SETSCHEDULER) || defined(HAVE_SCHED_SETPARAM))

PyDoc_STRVAR(os_sched_param__doc__,
"sched_param(sched_priority)\n"
"--\n"
"\n"
"Current has only one field: sched_priority\");\n"
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
    static _PyArg_Parser _parser = {"O:sched_param", _keywords, 0};
    PyObject *sched_priority;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &sched_priority)) {
        goto exit;
    }
    return_value = os_sched_param_impl(type, sched_priority);

exit:
    return return_value;
}

#endif /* defined(HAVE_SCHED_H) && (defined(HAVE_SCHED_SETSCHEDULER) || defined(HAVE_SCHED_SETPARAM)) */

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
    {"sched_setscheduler", (PyCFunction)os_sched_setscheduler, METH_FASTCALL, os_sched_setscheduler__doc__},

static PyObject *
os_sched_setscheduler_impl(PyObject *module, pid_t pid, int policy,
                           struct sched_param *param);

static PyObject *
os_sched_setscheduler(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    pid_t pid;
    int policy;
    struct sched_param param;

    if (!_PyArg_ParseStack(args, nargs, "" _Py_PARSE_PID "iO&:sched_setscheduler",
        &pid, &policy, convert_sched_param, &param)) {
        goto exit;
    }
    return_value = os_sched_setscheduler_impl(module, pid, policy, &param);

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
    {"sched_setparam", (PyCFunction)os_sched_setparam, METH_FASTCALL, os_sched_setparam__doc__},

static PyObject *
os_sched_setparam_impl(PyObject *module, pid_t pid,
                       struct sched_param *param);

static PyObject *
os_sched_setparam(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    pid_t pid;
    struct sched_param param;

    if (!_PyArg_ParseStack(args, nargs, "" _Py_PARSE_PID "O&:sched_setparam",
        &pid, convert_sched_param, &param)) {
        goto exit;
    }
    return_value = os_sched_setparam_impl(module, pid, &param);

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
    {"sched_setaffinity", (PyCFunction)os_sched_setaffinity, METH_FASTCALL, os_sched_setaffinity__doc__},

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

#if defined(HAVE_GETPGID)

PyDoc_STRVAR(os_getpgid__doc__,
"getpgid($module, /, pid)\n"
"--\n"
"\n"
"Call the system call getpgid(), and return the result.");

#define OS_GETPGID_METHODDEF    \
    {"getpgid", (PyCFunction)os_getpgid, METH_FASTCALL|METH_KEYWORDS, os_getpgid__doc__},

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
    {"kill", (PyCFunction)os_kill, METH_FASTCALL, os_kill__doc__},

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
    {"killpg", (PyCFunction)os_killpg, METH_FASTCALL, os_killpg__doc__},

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

    if (!PyArg_Parse(arg, "i:plock", &op)) {
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

    if (!PyArg_Parse(arg, "O&:setuid", _Py_Uid_Converter, &uid)) {
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

    if (!PyArg_Parse(arg, "O&:seteuid", _Py_Uid_Converter, &euid)) {
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

    if (!PyArg_Parse(arg, "O&:setegid", _Py_Gid_Converter, &egid)) {
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
    {"setreuid", (PyCFunction)os_setreuid, METH_FASTCALL, os_setreuid__doc__},

static PyObject *
os_setreuid_impl(PyObject *module, uid_t ruid, uid_t euid);

static PyObject *
os_setreuid(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    uid_t ruid;
    uid_t euid;

    if (!_PyArg_ParseStack(args, nargs, "O&O&:setreuid",
        _Py_Uid_Converter, &ruid, _Py_Uid_Converter, &euid)) {
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
    {"setregid", (PyCFunction)os_setregid, METH_FASTCALL, os_setregid__doc__},

static PyObject *
os_setregid_impl(PyObject *module, gid_t rgid, gid_t egid);

static PyObject *
os_setregid(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    gid_t rgid;
    gid_t egid;

    if (!_PyArg_ParseStack(args, nargs, "O&O&:setregid",
        _Py_Gid_Converter, &rgid, _Py_Gid_Converter, &egid)) {
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

    if (!PyArg_Parse(arg, "O&:setgid", _Py_Gid_Converter, &gid)) {
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
    {"wait3", (PyCFunction)os_wait3, METH_FASTCALL|METH_KEYWORDS, os_wait3__doc__},

static PyObject *
os_wait3_impl(PyObject *module, int options);

static PyObject *
os_wait3(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"options", NULL};
    static _PyArg_Parser _parser = {"i:wait3", _keywords, 0};
    int options;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &options)) {
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
    {"wait4", (PyCFunction)os_wait4, METH_FASTCALL|METH_KEYWORDS, os_wait4__doc__},

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
    {"waitid", (PyCFunction)os_waitid, METH_FASTCALL, os_waitid__doc__},

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
    {"waitpid", (PyCFunction)os_waitpid, METH_FASTCALL, os_waitpid__doc__},

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

#if defined(HAVE_CWAIT)

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
    {"waitpid", (PyCFunction)os_waitpid, METH_FASTCALL, os_waitpid__doc__},

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

#endif /* defined(HAVE_CWAIT) */

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
    {"symlink", (PyCFunction)os_symlink, METH_FASTCALL|METH_KEYWORDS, os_symlink__doc__},

static PyObject *
os_symlink_impl(PyObject *module, path_t *src, path_t *dst,
                int target_is_directory, int dir_fd);

static PyObject *
os_symlink(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"src", "dst", "target_is_directory", "dir_fd", NULL};
    static _PyArg_Parser _parser = {"O&O&|p$O&:symlink", _keywords, 0};
    path_t src = PATH_T_INITIALIZE("symlink", "src", 0, 0);
    path_t dst = PATH_T_INITIALIZE("symlink", "dst", 0, 0);
    int target_is_directory = 0;
    int dir_fd = DEFAULT_DIR_FD;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &src, path_converter, &dst, &target_is_directory, SYMLINKAT_DIR_FD_CONVERTER, &dir_fd)) {
        goto exit;
    }
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
    {"setpgid", (PyCFunction)os_setpgid, METH_FASTCALL, os_setpgid__doc__},

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

    if (!PyArg_Parse(arg, "i:tcgetpgrp", &fd)) {
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
    {"tcsetpgrp", (PyCFunction)os_tcsetpgrp, METH_FASTCALL, os_tcsetpgrp__doc__},

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
    {"open", (PyCFunction)os_open, METH_FASTCALL|METH_KEYWORDS, os_open__doc__},

static int
os_open_impl(PyObject *module, path_t *path, int flags, int mode, int dir_fd);

static PyObject *
os_open(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "flags", "mode", "dir_fd", NULL};
    static _PyArg_Parser _parser = {"O&i|i$O&:open", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("open", "path", 0, 0);
    int flags;
    int mode = 511;
    int dir_fd = DEFAULT_DIR_FD;
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, &flags, &mode, OPENAT_DIR_FD_CONVERTER, &dir_fd)) {
        goto exit;
    }
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
    {"close", (PyCFunction)os_close, METH_FASTCALL|METH_KEYWORDS, os_close__doc__},

static PyObject *
os_close_impl(PyObject *module, int fd);

static PyObject *
os_close(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", NULL};
    static _PyArg_Parser _parser = {"i:close", _keywords, 0};
    int fd;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &fd)) {
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
    {"closerange", (PyCFunction)os_closerange, METH_FASTCALL, os_closerange__doc__},

static PyObject *
os_closerange_impl(PyObject *module, int fd_low, int fd_high);

static PyObject *
os_closerange(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd_low;
    int fd_high;

    if (!_PyArg_ParseStack(args, nargs, "ii:closerange",
        &fd_low, &fd_high)) {
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

    if (!PyArg_Parse(arg, "i:dup", &fd)) {
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
    {"dup2", (PyCFunction)os_dup2, METH_FASTCALL|METH_KEYWORDS, os_dup2__doc__},

static int
os_dup2_impl(PyObject *module, int fd, int fd2, int inheritable);

static PyObject *
os_dup2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", "fd2", "inheritable", NULL};
    static _PyArg_Parser _parser = {"ii|p:dup2", _keywords, 0};
    int fd;
    int fd2;
    int inheritable = 1;
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &fd, &fd2, &inheritable)) {
        goto exit;
    }
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
    {"lockf", (PyCFunction)os_lockf, METH_FASTCALL, os_lockf__doc__},

static PyObject *
os_lockf_impl(PyObject *module, int fd, int command, Py_off_t length);

static PyObject *
os_lockf(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int command;
    Py_off_t length;

    if (!_PyArg_ParseStack(args, nargs, "iiO&:lockf",
        &fd, &command, Py_off_t_converter, &length)) {
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
    {"lseek", (PyCFunction)os_lseek, METH_FASTCALL, os_lseek__doc__},

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

    if (!_PyArg_ParseStack(args, nargs, "iO&i:lseek",
        &fd, Py_off_t_converter, &position, &how)) {
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
    {"read", (PyCFunction)os_read, METH_FASTCALL, os_read__doc__},

static PyObject *
os_read_impl(PyObject *module, int fd, Py_ssize_t length);

static PyObject *
os_read(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    Py_ssize_t length;

    if (!_PyArg_ParseStack(args, nargs, "in:read",
        &fd, &length)) {
        goto exit;
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
    {"readv", (PyCFunction)os_readv, METH_FASTCALL, os_readv__doc__},

static Py_ssize_t
os_readv_impl(PyObject *module, int fd, PyObject *buffers);

static PyObject *
os_readv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    PyObject *buffers;
    Py_ssize_t _return_value;

    if (!_PyArg_ParseStack(args, nargs, "iO:readv",
        &fd, &buffers)) {
        goto exit;
    }
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
    {"pread", (PyCFunction)os_pread, METH_FASTCALL, os_pread__doc__},

static PyObject *
os_pread_impl(PyObject *module, int fd, int length, Py_off_t offset);

static PyObject *
os_pread(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int length;
    Py_off_t offset;

    if (!_PyArg_ParseStack(args, nargs, "iiO&:pread",
        &fd, &length, Py_off_t_converter, &offset)) {
        goto exit;
    }
    return_value = os_pread_impl(module, fd, length, offset);

exit:
    return return_value;
}

#endif /* defined(HAVE_PREAD) */

PyDoc_STRVAR(os_write__doc__,
"write($module, fd, data, /)\n"
"--\n"
"\n"
"Write a bytes object to a file descriptor.");

#define OS_WRITE_METHODDEF    \
    {"write", (PyCFunction)os_write, METH_FASTCALL, os_write__doc__},

static Py_ssize_t
os_write_impl(PyObject *module, int fd, Py_buffer *data);

static PyObject *
os_write(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    Py_buffer data = {NULL, NULL};
    Py_ssize_t _return_value;

    if (!_PyArg_ParseStack(args, nargs, "iy*:write",
        &fd, &data)) {
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

PyDoc_STRVAR(os_fstat__doc__,
"fstat($module, /, fd)\n"
"--\n"
"\n"
"Perform a stat system call on the given file descriptor.\n"
"\n"
"Like stat(), but for an open file descriptor.\n"
"Equivalent to os.stat(fd).");

#define OS_FSTAT_METHODDEF    \
    {"fstat", (PyCFunction)os_fstat, METH_FASTCALL|METH_KEYWORDS, os_fstat__doc__},

static PyObject *
os_fstat_impl(PyObject *module, int fd);

static PyObject *
os_fstat(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", NULL};
    static _PyArg_Parser _parser = {"i:fstat", _keywords, 0};
    int fd;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &fd)) {
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

    if (!PyArg_Parse(arg, "i:isatty", &fd)) {
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

    if (!PyArg_Parse(arg, "i:pipe2", &flags)) {
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
    {"writev", (PyCFunction)os_writev, METH_FASTCALL, os_writev__doc__},

static Py_ssize_t
os_writev_impl(PyObject *module, int fd, PyObject *buffers);

static PyObject *
os_writev(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    PyObject *buffers;
    Py_ssize_t _return_value;

    if (!_PyArg_ParseStack(args, nargs, "iO:writev",
        &fd, &buffers)) {
        goto exit;
    }
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
    {"pwrite", (PyCFunction)os_pwrite, METH_FASTCALL, os_pwrite__doc__},

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

    if (!_PyArg_ParseStack(args, nargs, "iy*O&:pwrite",
        &fd, &buffer, Py_off_t_converter, &offset)) {
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
    {"mkfifo", (PyCFunction)os_mkfifo, METH_FASTCALL|METH_KEYWORDS, os_mkfifo__doc__},

static PyObject *
os_mkfifo_impl(PyObject *module, path_t *path, int mode, int dir_fd);

static PyObject *
os_mkfifo(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "mode", "dir_fd", NULL};
    static _PyArg_Parser _parser = {"O&|i$O&:mkfifo", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("mkfifo", "path", 0, 0);
    int mode = 438;
    int dir_fd = DEFAULT_DIR_FD;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, &mode, MKFIFOAT_DIR_FD_CONVERTER, &dir_fd)) {
        goto exit;
    }
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
    {"mknod", (PyCFunction)os_mknod, METH_FASTCALL|METH_KEYWORDS, os_mknod__doc__},

static PyObject *
os_mknod_impl(PyObject *module, path_t *path, int mode, dev_t device,
              int dir_fd);

static PyObject *
os_mknod(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "mode", "device", "dir_fd", NULL};
    static _PyArg_Parser _parser = {"O&|iO&$O&:mknod", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("mknod", "path", 0, 0);
    int mode = 384;
    dev_t device = 0;
    int dir_fd = DEFAULT_DIR_FD;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, &mode, _Py_Dev_Converter, &device, MKNODAT_DIR_FD_CONVERTER, &dir_fd)) {
        goto exit;
    }
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

    if (!PyArg_Parse(arg, "O&:major", _Py_Dev_Converter, &device)) {
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

    if (!PyArg_Parse(arg, "O&:minor", _Py_Dev_Converter, &device)) {
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
    {"makedev", (PyCFunction)os_makedev, METH_FASTCALL, os_makedev__doc__},

static dev_t
os_makedev_impl(PyObject *module, int major, int minor);

static PyObject *
os_makedev(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int major;
    int minor;
    dev_t _return_value;

    if (!_PyArg_ParseStack(args, nargs, "ii:makedev",
        &major, &minor)) {
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
    {"ftruncate", (PyCFunction)os_ftruncate, METH_FASTCALL, os_ftruncate__doc__},

static PyObject *
os_ftruncate_impl(PyObject *module, int fd, Py_off_t length);

static PyObject *
os_ftruncate(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    Py_off_t length;

    if (!_PyArg_ParseStack(args, nargs, "iO&:ftruncate",
        &fd, Py_off_t_converter, &length)) {
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
    {"truncate", (PyCFunction)os_truncate, METH_FASTCALL|METH_KEYWORDS, os_truncate__doc__},

static PyObject *
os_truncate_impl(PyObject *module, path_t *path, Py_off_t length);

static PyObject *
os_truncate(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "length", NULL};
    static _PyArg_Parser _parser = {"O&O&:truncate", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("truncate", "path", 0, PATH_HAVE_FTRUNCATE);
    Py_off_t length;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, Py_off_t_converter, &length)) {
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
    {"posix_fallocate", (PyCFunction)os_posix_fallocate, METH_FASTCALL, os_posix_fallocate__doc__},

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

    if (!_PyArg_ParseStack(args, nargs, "iO&O&:posix_fallocate",
        &fd, Py_off_t_converter, &offset, Py_off_t_converter, &length)) {
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
    {"posix_fadvise", (PyCFunction)os_posix_fadvise, METH_FASTCALL, os_posix_fadvise__doc__},

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

    if (!_PyArg_ParseStack(args, nargs, "iO&O&i:posix_fadvise",
        &fd, Py_off_t_converter, &offset, Py_off_t_converter, &length, &advice)) {
        goto exit;
    }
    return_value = os_posix_fadvise_impl(module, fd, offset, length, advice);

exit:
    return return_value;
}

#endif /* (defined(HAVE_POSIX_FADVISE) && !defined(POSIX_FADVISE_AIX_BUG)) */

#if defined(HAVE_PUTENV) && defined(MS_WINDOWS)

PyDoc_STRVAR(os_putenv__doc__,
"putenv($module, name, value, /)\n"
"--\n"
"\n"
"Change or add an environment variable.");

#define OS_PUTENV_METHODDEF    \
    {"putenv", (PyCFunction)os_putenv, METH_FASTCALL, os_putenv__doc__},

static PyObject *
os_putenv_impl(PyObject *module, PyObject *name, PyObject *value);

static PyObject *
os_putenv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *name;
    PyObject *value;

    if (!_PyArg_ParseStack(args, nargs, "UU:putenv",
        &name, &value)) {
        goto exit;
    }
    return_value = os_putenv_impl(module, name, value);

exit:
    return return_value;
}

#endif /* defined(HAVE_PUTENV) && defined(MS_WINDOWS) */

#if defined(HAVE_PUTENV) && !defined(MS_WINDOWS)

PyDoc_STRVAR(os_putenv__doc__,
"putenv($module, name, value, /)\n"
"--\n"
"\n"
"Change or add an environment variable.");

#define OS_PUTENV_METHODDEF    \
    {"putenv", (PyCFunction)os_putenv, METH_FASTCALL, os_putenv__doc__},

static PyObject *
os_putenv_impl(PyObject *module, PyObject *name, PyObject *value);

static PyObject *
os_putenv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *name = NULL;
    PyObject *value = NULL;

    if (!_PyArg_ParseStack(args, nargs, "O&O&:putenv",
        PyUnicode_FSConverter, &name, PyUnicode_FSConverter, &value)) {
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

#endif /* defined(HAVE_PUTENV) && !defined(MS_WINDOWS) */

#if defined(HAVE_UNSETENV)

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

    if (!PyArg_Parse(arg, "O&:unsetenv", PyUnicode_FSConverter, &name)) {
        goto exit;
    }
    return_value = os_unsetenv_impl(module, name);

exit:
    /* Cleanup for name */
    Py_XDECREF(name);

    return return_value;
}

#endif /* defined(HAVE_UNSETENV) */

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

    if (!PyArg_Parse(arg, "i:strerror", &code)) {
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

    if (!PyArg_Parse(arg, "i:WCOREDUMP", &status)) {
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
    {"WIFCONTINUED", (PyCFunction)os_WIFCONTINUED, METH_FASTCALL|METH_KEYWORDS, os_WIFCONTINUED__doc__},

static int
os_WIFCONTINUED_impl(PyObject *module, int status);

static PyObject *
os_WIFCONTINUED(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {"i:WIFCONTINUED", _keywords, 0};
    int status;
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &status)) {
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
    {"WIFSTOPPED", (PyCFunction)os_WIFSTOPPED, METH_FASTCALL|METH_KEYWORDS, os_WIFSTOPPED__doc__},

static int
os_WIFSTOPPED_impl(PyObject *module, int status);

static PyObject *
os_WIFSTOPPED(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {"i:WIFSTOPPED", _keywords, 0};
    int status;
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &status)) {
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
    {"WIFSIGNALED", (PyCFunction)os_WIFSIGNALED, METH_FASTCALL|METH_KEYWORDS, os_WIFSIGNALED__doc__},

static int
os_WIFSIGNALED_impl(PyObject *module, int status);

static PyObject *
os_WIFSIGNALED(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {"i:WIFSIGNALED", _keywords, 0};
    int status;
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &status)) {
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
    {"WIFEXITED", (PyCFunction)os_WIFEXITED, METH_FASTCALL|METH_KEYWORDS, os_WIFEXITED__doc__},

static int
os_WIFEXITED_impl(PyObject *module, int status);

static PyObject *
os_WIFEXITED(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {"i:WIFEXITED", _keywords, 0};
    int status;
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &status)) {
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
    {"WEXITSTATUS", (PyCFunction)os_WEXITSTATUS, METH_FASTCALL|METH_KEYWORDS, os_WEXITSTATUS__doc__},

static int
os_WEXITSTATUS_impl(PyObject *module, int status);

static PyObject *
os_WEXITSTATUS(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {"i:WEXITSTATUS", _keywords, 0};
    int status;
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &status)) {
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
    {"WTERMSIG", (PyCFunction)os_WTERMSIG, METH_FASTCALL|METH_KEYWORDS, os_WTERMSIG__doc__},

static int
os_WTERMSIG_impl(PyObject *module, int status);

static PyObject *
os_WTERMSIG(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {"i:WTERMSIG", _keywords, 0};
    int status;
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &status)) {
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
    {"WSTOPSIG", (PyCFunction)os_WSTOPSIG, METH_FASTCALL|METH_KEYWORDS, os_WSTOPSIG__doc__},

static int
os_WSTOPSIG_impl(PyObject *module, int status);

static PyObject *
os_WSTOPSIG(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"status", NULL};
    static _PyArg_Parser _parser = {"i:WSTOPSIG", _keywords, 0};
    int status;
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &status)) {
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

    if (!PyArg_Parse(arg, "i:fstatvfs", &fd)) {
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
    {"statvfs", (PyCFunction)os_statvfs, METH_FASTCALL|METH_KEYWORDS, os_statvfs__doc__},

static PyObject *
os_statvfs_impl(PyObject *module, path_t *path);

static PyObject *
os_statvfs(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {"O&:statvfs", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("statvfs", "path", 0, PATH_HAVE_FSTATVFS);

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path)) {
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
    {"_getdiskusage", (PyCFunction)os__getdiskusage, METH_FASTCALL|METH_KEYWORDS, os__getdiskusage__doc__},

static PyObject *
os__getdiskusage_impl(PyObject *module, Py_UNICODE *path);

static PyObject *
os__getdiskusage(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {"u:_getdiskusage", _keywords, 0};
    Py_UNICODE *path;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &path)) {
        goto exit;
    }
    return_value = os__getdiskusage_impl(module, path);

exit:
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
    {"fpathconf", (PyCFunction)os_fpathconf, METH_FASTCALL, os_fpathconf__doc__},

static long
os_fpathconf_impl(PyObject *module, int fd, int name);

static PyObject *
os_fpathconf(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int name;
    long _return_value;

    if (!_PyArg_ParseStack(args, nargs, "iO&:fpathconf",
        &fd, conv_path_confname, &name)) {
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
    {"pathconf", (PyCFunction)os_pathconf, METH_FASTCALL|METH_KEYWORDS, os_pathconf__doc__},

static long
os_pathconf_impl(PyObject *module, path_t *path, int name);

static PyObject *
os_pathconf(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "name", NULL};
    static _PyArg_Parser _parser = {"O&O&:pathconf", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("pathconf", "path", 0, PATH_HAVE_FPATHCONF);
    int name;
    long _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, conv_path_confname, &name)) {
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

    if (!PyArg_Parse(arg, "O&:confstr", conv_confstr_confname, &name)) {
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

    if (!PyArg_Parse(arg, "O&:sysconf", conv_sysconf_confname, &name)) {
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
"startfile($module, /, filepath, operation=None)\n"
"--\n"
"\n"
"startfile(filepath [, operation])\n"
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
    {"startfile", (PyCFunction)os_startfile, METH_FASTCALL|METH_KEYWORDS, os_startfile__doc__},

static PyObject *
os_startfile_impl(PyObject *module, path_t *filepath, Py_UNICODE *operation);

static PyObject *
os_startfile(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"filepath", "operation", NULL};
    static _PyArg_Parser _parser = {"O&|u:startfile", _keywords, 0};
    path_t filepath = PATH_T_INITIALIZE("startfile", "filepath", 0, 0);
    Py_UNICODE *operation = NULL;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &filepath, &operation)) {
        goto exit;
    }
    return_value = os_startfile_impl(module, &filepath, operation);

exit:
    /* Cleanup for filepath */
    path_cleanup(&filepath);

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
    {"device_encoding", (PyCFunction)os_device_encoding, METH_FASTCALL|METH_KEYWORDS, os_device_encoding__doc__},

static PyObject *
os_device_encoding_impl(PyObject *module, int fd);

static PyObject *
os_device_encoding(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", NULL};
    static _PyArg_Parser _parser = {"i:device_encoding", _keywords, 0};
    int fd;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &fd)) {
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
    {"setresuid", (PyCFunction)os_setresuid, METH_FASTCALL, os_setresuid__doc__},

static PyObject *
os_setresuid_impl(PyObject *module, uid_t ruid, uid_t euid, uid_t suid);

static PyObject *
os_setresuid(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    uid_t ruid;
    uid_t euid;
    uid_t suid;

    if (!_PyArg_ParseStack(args, nargs, "O&O&O&:setresuid",
        _Py_Uid_Converter, &ruid, _Py_Uid_Converter, &euid, _Py_Uid_Converter, &suid)) {
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
    {"setresgid", (PyCFunction)os_setresgid, METH_FASTCALL, os_setresgid__doc__},

static PyObject *
os_setresgid_impl(PyObject *module, gid_t rgid, gid_t egid, gid_t sgid);

static PyObject *
os_setresgid(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    gid_t rgid;
    gid_t egid;
    gid_t sgid;

    if (!_PyArg_ParseStack(args, nargs, "O&O&O&:setresgid",
        _Py_Gid_Converter, &rgid, _Py_Gid_Converter, &egid, _Py_Gid_Converter, &sgid)) {
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
"path may be either a string or an open file descriptor.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, getxattr will examine the symbolic link itself instead of the file\n"
"  the link points to.");

#define OS_GETXATTR_METHODDEF    \
    {"getxattr", (PyCFunction)os_getxattr, METH_FASTCALL|METH_KEYWORDS, os_getxattr__doc__},

static PyObject *
os_getxattr_impl(PyObject *module, path_t *path, path_t *attribute,
                 int follow_symlinks);

static PyObject *
os_getxattr(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "attribute", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"O&O&|$p:getxattr", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("getxattr", "path", 0, 1);
    path_t attribute = PATH_T_INITIALIZE("getxattr", "attribute", 0, 0);
    int follow_symlinks = 1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, path_converter, &attribute, &follow_symlinks)) {
        goto exit;
    }
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
"path may be either a string or an open file descriptor.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, setxattr will modify the symbolic link itself instead of the file\n"
"  the link points to.");

#define OS_SETXATTR_METHODDEF    \
    {"setxattr", (PyCFunction)os_setxattr, METH_FASTCALL|METH_KEYWORDS, os_setxattr__doc__},

static PyObject *
os_setxattr_impl(PyObject *module, path_t *path, path_t *attribute,
                 Py_buffer *value, int flags, int follow_symlinks);

static PyObject *
os_setxattr(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "attribute", "value", "flags", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"O&O&y*|i$p:setxattr", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("setxattr", "path", 0, 1);
    path_t attribute = PATH_T_INITIALIZE("setxattr", "attribute", 0, 0);
    Py_buffer value = {NULL, NULL};
    int flags = 0;
    int follow_symlinks = 1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, path_converter, &attribute, &value, &flags, &follow_symlinks)) {
        goto exit;
    }
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
"path may be either a string or an open file descriptor.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, removexattr will modify the symbolic link itself instead of the file\n"
"  the link points to.");

#define OS_REMOVEXATTR_METHODDEF    \
    {"removexattr", (PyCFunction)os_removexattr, METH_FASTCALL|METH_KEYWORDS, os_removexattr__doc__},

static PyObject *
os_removexattr_impl(PyObject *module, path_t *path, path_t *attribute,
                    int follow_symlinks);

static PyObject *
os_removexattr(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "attribute", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"O&O&|$p:removexattr", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("removexattr", "path", 0, 1);
    path_t attribute = PATH_T_INITIALIZE("removexattr", "attribute", 0, 0);
    int follow_symlinks = 1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, path_converter, &attribute, &follow_symlinks)) {
        goto exit;
    }
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
"path may be either None, a string, or an open file descriptor.\n"
"if path is None, listxattr will examine the current directory.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, listxattr will examine the symbolic link itself instead of the file\n"
"  the link points to.");

#define OS_LISTXATTR_METHODDEF    \
    {"listxattr", (PyCFunction)os_listxattr, METH_FASTCALL|METH_KEYWORDS, os_listxattr__doc__},

static PyObject *
os_listxattr_impl(PyObject *module, path_t *path, int follow_symlinks);

static PyObject *
os_listxattr(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"|O&$p:listxattr", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("listxattr", "path", 1, 1);
    int follow_symlinks = 1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path, &follow_symlinks)) {
        goto exit;
    }
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

    if (!PyArg_Parse(arg, "n:urandom", &size)) {
        goto exit;
    }
    return_value = os_urandom_impl(module, size);

exit:
    return return_value;
}

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

    if (!PyArg_Parse(arg, "i:get_inheritable", &fd)) {
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
    {"set_inheritable", (PyCFunction)os_set_inheritable, METH_FASTCALL, os_set_inheritable__doc__},

static PyObject *
os_set_inheritable_impl(PyObject *module, int fd, int inheritable);

static PyObject *
os_set_inheritable(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int inheritable;

    if (!_PyArg_ParseStack(args, nargs, "ii:set_inheritable",
        &fd, &inheritable)) {
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
    {"set_handle_inheritable", (PyCFunction)os_set_handle_inheritable, METH_FASTCALL, os_set_handle_inheritable__doc__},

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

PyDoc_STRVAR(os_DirEntry_is_symlink__doc__,
"is_symlink($self, /)\n"
"--\n"
"\n"
"Return True if the entry is a symbolic link; cached per entry.");

#define OS_DIRENTRY_IS_SYMLINK_METHODDEF    \
    {"is_symlink", (PyCFunction)os_DirEntry_is_symlink, METH_NOARGS, os_DirEntry_is_symlink__doc__},

static int
os_DirEntry_is_symlink_impl(DirEntry *self);

static PyObject *
os_DirEntry_is_symlink(DirEntry *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = os_DirEntry_is_symlink_impl(self);
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
    {"stat", (PyCFunction)os_DirEntry_stat, METH_FASTCALL|METH_KEYWORDS, os_DirEntry_stat__doc__},

static PyObject *
os_DirEntry_stat_impl(DirEntry *self, int follow_symlinks);

static PyObject *
os_DirEntry_stat(DirEntry *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"follow_symlinks", NULL};
    static _PyArg_Parser _parser = {"|$p:stat", _keywords, 0};
    int follow_symlinks = 1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &follow_symlinks)) {
        goto exit;
    }
    return_value = os_DirEntry_stat_impl(self, follow_symlinks);

exit:
    return return_value;
}

PyDoc_STRVAR(os_DirEntry_is_dir__doc__,
"is_dir($self, /, *, follow_symlinks=True)\n"
"--\n"
"\n"
"Return True if the entry is a directory; cached per entry.");

#define OS_DIRENTRY_IS_DIR_METHODDEF    \
    {"is_dir", (PyCFunction)os_DirEntry_is_dir, METH_FASTCALL|METH_KEYWORDS, os_DirEntry_is_dir__doc__},

static int
os_DirEntry_is_dir_impl(DirEntry *self, int follow_symlinks);

static PyObject *
os_DirEntry_is_dir(DirEntry *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    _return_value = os_DirEntry_is_dir_impl(self, follow_symlinks);
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
    {"is_file", (PyCFunction)os_DirEntry_is_file, METH_FASTCALL|METH_KEYWORDS, os_DirEntry_is_file__doc__},

static int
os_DirEntry_is_file_impl(DirEntry *self, int follow_symlinks);

static PyObject *
os_DirEntry_is_file(DirEntry *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    _return_value = os_DirEntry_is_file_impl(self, follow_symlinks);
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
"path can be specified as either str, bytes or path-like object.  If path\n"
"is bytes, the names of yielded DirEntry objects will also be bytes; in\n"
"all other circumstances they will be str.\n"
"\n"
"If path is None, uses the path=\'.\'.");

#define OS_SCANDIR_METHODDEF    \
    {"scandir", (PyCFunction)os_scandir, METH_FASTCALL|METH_KEYWORDS, os_scandir__doc__},

static PyObject *
os_scandir_impl(PyObject *module, path_t *path);

static PyObject *
os_scandir(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {"|O&:scandir", _keywords, 0};
    path_t path = PATH_T_INITIALIZE("scandir", "path", 1, PATH_HAVE_FDOPENDIR);

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        path_converter, &path)) {
        goto exit;
    }
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
    {"fspath", (PyCFunction)os_fspath, METH_FASTCALL|METH_KEYWORDS, os_fspath__doc__},

static PyObject *
os_fspath_impl(PyObject *module, PyObject *path);

static PyObject *
os_fspath(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {"O:fspath", _keywords, 0};
    PyObject *path;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &path)) {
        goto exit;
    }
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
    {"getrandom", (PyCFunction)os_getrandom, METH_FASTCALL|METH_KEYWORDS, os_getrandom__doc__},

static PyObject *
os_getrandom_impl(PyObject *module, Py_ssize_t size, int flags);

static PyObject *
os_getrandom(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"size", "flags", NULL};
    static _PyArg_Parser _parser = {"n|i:getrandom", _keywords, 0};
    Py_ssize_t size;
    int flags = 0;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &size, &flags)) {
        goto exit;
    }
    return_value = os_getrandom_impl(module, size, flags);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETRANDOM_SYSCALL) */

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

#ifndef OS__ISDIR_METHODDEF
    #define OS__ISDIR_METHODDEF
#endif /* !defined(OS__ISDIR_METHODDEF) */

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

#ifndef OS_GETGROUPS_METHODDEF
    #define OS_GETGROUPS_METHODDEF
#endif /* !defined(OS_GETGROUPS_METHODDEF) */

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

#ifndef OS_GET_HANDLE_INHERITABLE_METHODDEF
    #define OS_GET_HANDLE_INHERITABLE_METHODDEF
#endif /* !defined(OS_GET_HANDLE_INHERITABLE_METHODDEF) */

#ifndef OS_SET_HANDLE_INHERITABLE_METHODDEF
    #define OS_SET_HANDLE_INHERITABLE_METHODDEF
#endif /* !defined(OS_SET_HANDLE_INHERITABLE_METHODDEF) */

#ifndef OS_GETRANDOM_METHODDEF
    #define OS_GETRANDOM_METHODDEF
#endif /* !defined(OS_GETRANDOM_METHODDEF) */
/*[clinic end generated code: output=6345053cd5992caf input=a9049054013a1b77]*/
