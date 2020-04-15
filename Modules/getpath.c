/* Return the initial module search path. */

#include "Python.h"
#include "pycore_fileutils.h"
#include "pycore_initconfig.h"
#include "pycore_pathconfig.h"
#include "osdefs.h"               // DELIM

#include <sys/types.h>
#include <string.h>

#ifdef __APPLE__
#  include <mach-o/dyld.h>
#endif

/* Search in some common locations for the associated Python libraries.
 *
 * Two directories must be found, the platform independent directory
 * (prefix), containing the common .py and .pyc files, and the platform
 * dependent directory (exec_prefix), containing the shared library
 * modules.  Note that prefix and exec_prefix can be the same directory,
 * but for some installations, they are different.
 *
 * Py_GetPath() carries out separate searches for prefix and exec_prefix.
 * Each search tries a number of different locations until a ``landmark''
 * file or directory is found.  If no prefix or exec_prefix is found, a
 * warning message is issued and the preprocessor defined PREFIX and
 * EXEC_PREFIX are used (even though they will not work); python carries on
 * as best as is possible, but most imports will fail.
 *
 * Before any searches are done, the location of the executable is
 * determined.  If argv[0] has one or more slashes in it, it is used
 * unchanged.  Otherwise, it must have been invoked from the shell's path,
 * so we search $PATH for the named executable and use that.  If the
 * executable was not found on $PATH (or there was no $PATH environment
 * variable), the original argv[0] string is used.
 *
 * Next, the executable location is examined to see if it is a symbolic
 * link.  If so, the link is chased (correctly interpreting a relative
 * pathname if one is found) and the directory of the link target is used.
 *
 * Finally, argv0_path is set to the directory containing the executable
 * (i.e. the last component is stripped).
 *
 * With argv0_path in hand, we perform a number of steps.  The same steps
 * are performed for prefix and for exec_prefix, but with a different
 * landmark.
 *
 * Step 1. Are we running python out of the build directory?  This is
 * checked by looking for a different kind of landmark relative to
 * argv0_path.  For prefix, the landmark's path is derived from the VPATH
 * preprocessor variable (taking into account that its value is almost, but
 * not quite, what we need).  For exec_prefix, the landmark is
 * pybuilddir.txt.  If the landmark is found, we're done.
 *
 * For the remaining steps, the prefix landmark will always be
 * lib/python$VERSION/os.py and the exec_prefix will always be
 * lib/python$VERSION/lib-dynload, where $VERSION is Python's version
 * number as supplied by the Makefile.  Note that this means that no more
 * build directory checking is performed; if the first step did not find
 * the landmarks, the assumption is that python is running from an
 * installed setup.
 *
 * Step 2. See if the $PYTHONHOME environment variable points to the
 * installed location of the Python libraries.  If $PYTHONHOME is set, then
 * it points to prefix and exec_prefix.  $PYTHONHOME can be a single
 * directory, which is used for both, or the prefix and exec_prefix
 * directories separated by a colon.
 *
 * Step 3. Try to find prefix and exec_prefix relative to argv0_path,
 * backtracking up the path until it is exhausted.  This is the most common
 * step to succeed.  Note that if prefix and exec_prefix are different,
 * exec_prefix is more likely to be found; however if exec_prefix is a
 * subdirectory of prefix, both will be found.
 *
 * Step 4. Search the directories pointed to by the preprocessor variables
 * PREFIX and EXEC_PREFIX.  These are supplied by the Makefile but can be
 * passed in as options to the configure script.
 *
 * That's it!
 *
 * Well, almost.  Once we have determined prefix and exec_prefix, the
 * preprocessor variable PYTHONPATH is used to construct a path.  Each
 * relative path on PYTHONPATH is prefixed with prefix.  Then the directory
 * containing the shared library modules is appended.  The environment
 * variable $PYTHONPATH is inserted in front of it all.  Finally, the
 * prefix and exec_prefix globals are tweaked so they reflect the values
 * expected by other code, by stripping the "lib/python$VERSION/..." stuff
 * off.  If either points to the build directory, the globals are reset to
 * the corresponding preprocessor variables (so sys.prefix will reflect the
 * installation location, even though sys.path points into the build
 * directory).  This seems to make more sense given that currently the only
 * known use of sys.prefix and sys.exec_prefix is for the ILU installation
 * process to find the installed Python tree.
 *
 * An embedding application can use Py_SetPath() to override all of
 * these automatic path computations.
 *
 * NOTE: Windows MSVC builds use PC/getpathp.c instead!
 */

#ifdef __cplusplus
extern "C" {
#endif


#if (!defined(PREFIX) || !defined(EXEC_PREFIX) \
        || !defined(VERSION) || !defined(VPATH) || !defined(PLATLIBDIR))
#error "PREFIX, EXEC_PREFIX, VERSION, VPATH and PLATLIBDIR macros must be defined"
#endif

#ifndef LANDMARK
#define LANDMARK L"os.py"
#endif

#define BUILD_LANDMARK L"Modules/Setup.local"

#define DECODE_LOCALE_ERR(NAME, LEN) \
    ((LEN) == (size_t)-2) \
     ? _PyStatus_ERR("cannot decode " NAME) \
     : _PyStatus_NO_MEMORY()

#define PATHLEN_ERR() _PyStatus_ERR("path configuration: path too long")

typedef struct {
    wchar_t *path_env;                 /* PATH environment variable */

    wchar_t *pythonpath_macro;         /* PYTHONPATH macro */
    wchar_t *prefix_macro;             /* PREFIX macro */
    wchar_t *exec_prefix_macro;        /* EXEC_PREFIX macro */
    wchar_t *platlibdir_macro;         /* PLATLIBDIR macro */
    wchar_t *vpath_macro;              /* VPATH macro */

    wchar_t *lib_python;               /* "lib/pythonX.Y" */

    int prefix_found;         /* found platform independent libraries? */
    int exec_prefix_found;    /* found the platform dependent libraries? */

    int warnings;
    const wchar_t *pythonpath_env;

    wchar_t *argv0_path;
    wchar_t *zip_path;
    wchar_t *prefix;
    wchar_t *exec_prefix;
} PyCalculatePath;

static const wchar_t delimiter[2] = {DELIM, '\0'};
static const wchar_t separator[2] = {SEP, '\0'};


/* Get file status. Encode the path to the locale encoding. */
static int
_Py_wstat(const wchar_t* path, struct stat *buf)
{
    int err;
    char *fname;
    fname = _Py_EncodeLocaleRaw(path, NULL);
    if (fname == NULL) {
        errno = EINVAL;
        return -1;
    }
    err = stat(fname, buf);
    PyMem_RawFree(fname);
    return err;
}


static void
reduce(wchar_t *dir)
{
    size_t i = wcslen(dir);
    while (i > 0 && dir[i] != SEP) {
        --i;
    }
    dir[i] = '\0';
}


/* Is file, not directory */
static int
isfile(const wchar_t *filename)
{
    struct stat buf;
    if (_Py_wstat(filename, &buf) != 0) {
        return 0;
    }
    if (!S_ISREG(buf.st_mode)) {
        return 0;
    }
    return 1;
}


/* Is executable file */
static int
isxfile(const wchar_t *filename)
{
    struct stat buf;
    if (_Py_wstat(filename, &buf) != 0) {
        return 0;
    }
    if (!S_ISREG(buf.st_mode)) {
        return 0;
    }
    if ((buf.st_mode & 0111) == 0) {
        return 0;
    }
    return 1;
}


/* Is directory */
static int
isdir(const wchar_t *filename)
{
    struct stat buf;
    if (_Py_wstat(filename, &buf) != 0) {
        return 0;
    }
    if (!S_ISDIR(buf.st_mode)) {
        return 0;
    }
    return 1;
}


/* Add a path component, by appending stuff to buffer.
   buflen: 'buffer' length in characters including trailing NUL.

   If path2 is empty:

   - if path doesn't end with SEP and is not empty, add SEP to path
   - otherwise, do nothing. */
static PyStatus
joinpath(wchar_t *path, const wchar_t *path2, size_t path_len)
{
    size_t n;
    if (!_Py_isabs(path2)) {
        n = wcslen(path);
        if (n >= path_len) {
            return PATHLEN_ERR();
        }

        if (n > 0 && path[n-1] != SEP) {
            path[n++] = SEP;
        }
    }
    else {
        n = 0;
    }

    size_t k = wcslen(path2);
    if (n + k >= path_len) {
        return PATHLEN_ERR();
    }
    wcsncpy(path + n, path2, k);
    path[n + k] = '\0';

    return _PyStatus_OK();
}


static wchar_t*
substring(const wchar_t *str, size_t len)
{
    wchar_t *substr = PyMem_RawMalloc((len + 1) * sizeof(wchar_t));
    if (substr == NULL) {
        return NULL;
    }

    if (len) {
        memcpy(substr, str, len * sizeof(wchar_t));
    }
    substr[len] = L'\0';
    return substr;
}


static wchar_t*
joinpath2(const wchar_t *path, const wchar_t *path2)
{
    if (_Py_isabs(path2)) {
        return _PyMem_RawWcsdup(path2);
    }

    size_t len = wcslen(path);
    int add_sep = (len > 0 && path[len - 1] != SEP);
    len += add_sep;
    len += wcslen(path2);

    wchar_t *new_path = PyMem_RawMalloc((len + 1) * sizeof(wchar_t));
    if (new_path == NULL) {
        return NULL;
    }

    wcscpy(new_path, path);
    if (add_sep) {
        wcscat(new_path, separator);
    }
    wcscat(new_path, path2);
    return new_path;
}


static inline int
safe_wcscpy(wchar_t *dst, const wchar_t *src, size_t n)
{
    size_t srclen = wcslen(src);
    if (n <= srclen) {
        dst[0] = L'\0';
        return -1;
    }
    memcpy(dst, src, (srclen + 1) * sizeof(wchar_t));
    return 0;
}


/* copy_absolute requires that path be allocated at least
   'abs_path_len' characters (including trailing NUL). */
static PyStatus
copy_absolute(wchar_t *abs_path, const wchar_t *path, size_t abs_path_len)
{
    if (_Py_isabs(path)) {
        if (safe_wcscpy(abs_path, path, abs_path_len) < 0) {
            return PATHLEN_ERR();
        }
    }
    else {
        if (!_Py_wgetcwd(abs_path, abs_path_len)) {
            /* unable to get the current directory */
            if (safe_wcscpy(abs_path, path, abs_path_len) < 0) {
                return PATHLEN_ERR();
            }
            return _PyStatus_OK();
        }
        if (path[0] == '.' && path[1] == SEP) {
            path += 2;
        }
        PyStatus status = joinpath(abs_path, path, abs_path_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
    return _PyStatus_OK();
}


/* path_len: path length in characters including trailing NUL */
static PyStatus
absolutize(wchar_t **path_p)
{
    assert(!_Py_isabs(*path_p));

    wchar_t abs_path[MAXPATHLEN+1];
    wchar_t *path = *path_p;

    PyStatus status = copy_absolute(abs_path, path, Py_ARRAY_LENGTH(abs_path));
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    PyMem_RawFree(*path_p);
    *path_p = _PyMem_RawWcsdup(abs_path);
    if (*path_p == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    return _PyStatus_OK();
}


/* Is module -- check for .pyc too */
static PyStatus
ismodule(const wchar_t *path, int *result)
{
    wchar_t *filename = joinpath2(path, LANDMARK);
    if (filename == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    if (isfile(filename)) {
        PyMem_RawFree(filename);
        *result = 1;
        return _PyStatus_OK();
    }

    /* Check for the compiled version of prefix. */
    size_t len = wcslen(filename);
    wchar_t *pyc = PyMem_RawMalloc((len + 2) * sizeof(wchar_t));
    if (pyc == NULL) {
        PyMem_RawFree(filename);
        return _PyStatus_NO_MEMORY();
    }

    memcpy(pyc, filename, len * sizeof(wchar_t));
    pyc[len] = L'c';
    pyc[len + 1] = L'\0';
    *result = isfile(pyc);

    PyMem_RawFree(filename);
    PyMem_RawFree(pyc);

    return _PyStatus_OK();
}


#if defined(__CYGWIN__) || defined(__MINGW32__)
#ifndef EXE_SUFFIX
#define EXE_SUFFIX L".exe"
#endif

/* pathlen: 'path' length in characters including trailing NUL */
static PyStatus
add_exe_suffix(wchar_t **progpath_p)
{
    wchar_t *progpath = *progpath_p;

    /* Check for already have an executable suffix */
    size_t n = wcslen(progpath);
    size_t s = wcslen(EXE_SUFFIX);
    if (wcsncasecmp(EXE_SUFFIX, progpath + n - s, s) == 0) {
        return _PyStatus_OK();
    }

    wchar_t *progpath2 = PyMem_RawMalloc((n + s + 1) * sizeof(wchar_t));
    if (progpath2 == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    memcpy(progpath2, progpath, n * sizeof(wchar_t));
    memcpy(progpath2 + n, EXE_SUFFIX, s * sizeof(wchar_t));
    progpath2[n+s] = L'\0';

    if (isxfile(progpath2)) {
        PyMem_RawFree(*progpath_p);
        *progpath_p = progpath2;
    }
    else {
        PyMem_RawFree(progpath2);
    }
    return _PyStatus_OK();
}
#endif


/* search_for_prefix requires that argv0_path be no more than MAXPATHLEN
   bytes long.
*/
static PyStatus
search_for_prefix(PyCalculatePath *calculate, _PyPathConfig *pathconfig,
                  wchar_t *prefix, size_t prefix_len, int *found)
{
    PyStatus status;

    /* If PYTHONHOME is set, we believe it unconditionally */
    if (pathconfig->home) {
        /* Path: <home> / <lib_python> */
        if (safe_wcscpy(prefix, pathconfig->home, prefix_len) < 0) {
            return PATHLEN_ERR();
        }
        wchar_t *delim = wcschr(prefix, DELIM);
        if (delim) {
            *delim = L'\0';
        }
        status = joinpath(prefix, calculate->lib_python, prefix_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
        *found = 1;
        return _PyStatus_OK();
    }

    /* Check to see if argv0_path is in the build directory

       Path: <argv0_path> / <BUILD_LANDMARK define> */
    wchar_t *path = joinpath2(calculate->argv0_path, BUILD_LANDMARK);
    if (path == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    int is_build_dir = isfile(path);
    PyMem_RawFree(path);

    if (is_build_dir) {
        /* argv0_path is the build directory (BUILD_LANDMARK exists),
           now also check LANDMARK using ismodule(). */

        /* Path: <argv0_path> / <VPATH macro> / Lib */
        /* or if VPATH is empty: <argv0_path> / Lib */
        if (safe_wcscpy(prefix, calculate->argv0_path, prefix_len) < 0) {
            return PATHLEN_ERR();
        }

        status = joinpath(prefix, calculate->vpath_macro, prefix_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }

        status = joinpath(prefix, L"Lib", prefix_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }

        int module;
        status = ismodule(prefix, &module);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
        if (module) {
            /* BUILD_LANDMARK and LANDMARK found */
            *found = -1;
            return _PyStatus_OK();
        }
    }

    /* Search from argv0_path, until root is found */
    status = copy_absolute(prefix, calculate->argv0_path, prefix_len);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    do {
        /* Path: <argv0_path or substring> / <lib_python> / LANDMARK */
        size_t n = wcslen(prefix);
        status = joinpath(prefix, calculate->lib_python, prefix_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }

        int module;
        status = ismodule(prefix, &module);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
        if (module) {
            *found = 1;
            return _PyStatus_OK();
        }
        prefix[n] = L'\0';
        reduce(prefix);
    } while (prefix[0]);

    /* Look at configure's PREFIX.
       Path: <PREFIX macro> / <lib_python> / LANDMARK */
    if (safe_wcscpy(prefix, calculate->prefix_macro, prefix_len) < 0) {
        return PATHLEN_ERR();
    }
    status = joinpath(prefix, calculate->lib_python, prefix_len);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    int module;
    status = ismodule(prefix, &module);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }
    if (module) {
        *found = 1;
        return _PyStatus_OK();
    }

    /* Fail */
    *found = 0;
    return _PyStatus_OK();
}


static PyStatus
calculate_prefix(PyCalculatePath *calculate, _PyPathConfig *pathconfig)
{
    wchar_t prefix[MAXPATHLEN+1];
    memset(prefix, 0, sizeof(prefix));
    size_t prefix_len = Py_ARRAY_LENGTH(prefix);

    PyStatus status;
    status = search_for_prefix(calculate, pathconfig,
                               prefix, prefix_len,
                               &calculate->prefix_found);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if (!calculate->prefix_found) {
        if (calculate->warnings) {
            fprintf(stderr,
                "Could not find platform independent libraries <prefix>\n");
        }

        calculate->prefix = joinpath2(calculate->prefix_macro,
                                      calculate->lib_python);
    }
    else {
        calculate->prefix = _PyMem_RawWcsdup(prefix);
    }

    if (calculate->prefix == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    return _PyStatus_OK();
}


static PyStatus
calculate_set_prefix(PyCalculatePath *calculate, _PyPathConfig *pathconfig)
{
    /* Reduce prefix and exec_prefix to their essence,
     * e.g. /usr/local/lib/python1.5 is reduced to /usr/local.
     * If we're loading relative to the build directory,
     * return the compiled-in defaults instead.
     */
    if (calculate->prefix_found > 0) {
        wchar_t *prefix = _PyMem_RawWcsdup(calculate->prefix);
        if (prefix == NULL) {
            return _PyStatus_NO_MEMORY();
        }

        reduce(prefix);
        reduce(prefix);
        if (prefix[0]) {
            pathconfig->prefix = prefix;
        }
        else {
            PyMem_RawFree(prefix);

            /* The prefix is the root directory, but reduce() chopped
               off the "/". */
            pathconfig->prefix = _PyMem_RawWcsdup(separator);
            if (pathconfig->prefix == NULL) {
                return _PyStatus_NO_MEMORY();
            }
        }
    }
    else {
        pathconfig->prefix = _PyMem_RawWcsdup(calculate->prefix_macro);
        if (pathconfig->prefix == NULL) {
            return _PyStatus_NO_MEMORY();
        }
    }
    return _PyStatus_OK();
}


static PyStatus
calculate_pybuilddir(const wchar_t *argv0_path,
                     wchar_t *exec_prefix, size_t exec_prefix_len,
                     int *found)
{
    PyStatus status;

    /* Check to see if argv[0] is in the build directory. "pybuilddir.txt"
       is written by setup.py and contains the relative path to the location
       of shared library modules.

       Filename: <argv0_path> / "pybuilddir.txt" */
    wchar_t *filename = joinpath2(argv0_path, L"pybuilddir.txt");
    if (filename == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    FILE *fp = _Py_wfopen(filename, L"rb");
    PyMem_RawFree(filename);
    if (fp == NULL) {
        errno = 0;
        return _PyStatus_OK();
    }

    char buf[MAXPATHLEN + 1];
    size_t n = fread(buf, 1, Py_ARRAY_LENGTH(buf) - 1, fp);
    buf[n] = '\0';
    fclose(fp);

    size_t dec_len;
    wchar_t *pybuilddir = _Py_DecodeUTF8_surrogateescape(buf, n, &dec_len);
    if (!pybuilddir) {
        return DECODE_LOCALE_ERR("pybuilddir.txt", dec_len);
    }

    /* Path: <argv0_path> / <pybuilddir content> */
    if (safe_wcscpy(exec_prefix, argv0_path, exec_prefix_len) < 0) {
        PyMem_RawFree(pybuilddir);
        return PATHLEN_ERR();
    }
    status = joinpath(exec_prefix, pybuilddir, exec_prefix_len);
    PyMem_RawFree(pybuilddir);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    *found = -1;
    return _PyStatus_OK();
}


/* search_for_exec_prefix requires that argv0_path be no more than
   MAXPATHLEN bytes long.
*/
static PyStatus
search_for_exec_prefix(PyCalculatePath *calculate, _PyPathConfig *pathconfig,
                       wchar_t *exec_prefix, size_t exec_prefix_len,
                       int *found)
{
    PyStatus status;

    /* If PYTHONHOME is set, we believe it unconditionally */
    if (pathconfig->home) {
        /* Path: <home> / <lib_python> / "lib-dynload" */
        wchar_t *delim = wcschr(pathconfig->home, DELIM);
        if (delim) {
            if (safe_wcscpy(exec_prefix, delim+1, exec_prefix_len) < 0) {
                return PATHLEN_ERR();
            }
        }
        else {
            if (safe_wcscpy(exec_prefix, pathconfig->home, exec_prefix_len) < 0) {
                return PATHLEN_ERR();
            }
        }
        status = joinpath(exec_prefix, calculate->lib_python, exec_prefix_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
        status = joinpath(exec_prefix, L"lib-dynload", exec_prefix_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
        *found = 1;
        return _PyStatus_OK();
    }

    /* Check for pybuilddir.txt */
    assert(*found == 0);
    status = calculate_pybuilddir(calculate->argv0_path,
                                  exec_prefix, exec_prefix_len, found);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }
    if (*found) {
        return _PyStatus_OK();
    }

    /* Search from argv0_path, until root is found */
    status = copy_absolute(exec_prefix, calculate->argv0_path, exec_prefix_len);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    do {
        /* Path: <argv0_path or substring> / <lib_python> / "lib-dynload" */
        size_t n = wcslen(exec_prefix);
        status = joinpath(exec_prefix, calculate->lib_python, exec_prefix_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
        status = joinpath(exec_prefix, L"lib-dynload", exec_prefix_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
        if (isdir(exec_prefix)) {
            *found = 1;
            return _PyStatus_OK();
        }
        exec_prefix[n] = L'\0';
        reduce(exec_prefix);
    } while (exec_prefix[0]);

    /* Look at configure's EXEC_PREFIX.

       Path: <EXEC_PREFIX macro> / <lib_python> / "lib-dynload" */
    if (safe_wcscpy(exec_prefix, calculate->exec_prefix_macro, exec_prefix_len) < 0) {
        return PATHLEN_ERR();
    }
    status = joinpath(exec_prefix, calculate->lib_python, exec_prefix_len);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }
    status = joinpath(exec_prefix, L"lib-dynload", exec_prefix_len);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }
    if (isdir(exec_prefix)) {
        *found = 1;
        return _PyStatus_OK();
    }

    /* Fail */
    *found = 0;
    return _PyStatus_OK();
}


static PyStatus
calculate_exec_prefix(PyCalculatePath *calculate, _PyPathConfig *pathconfig)
{
    PyStatus status;
    wchar_t exec_prefix[MAXPATHLEN+1];
    memset(exec_prefix, 0, sizeof(exec_prefix));
    size_t exec_prefix_len = Py_ARRAY_LENGTH(exec_prefix);

    status = search_for_exec_prefix(calculate, pathconfig,
                                    exec_prefix, exec_prefix_len,
                                    &calculate->exec_prefix_found);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if (!calculate->exec_prefix_found) {
        if (calculate->warnings) {
            fprintf(stderr,
                "Could not find platform dependent libraries <exec_prefix>\n");
        }

        /* <PLATLIBDIR> / "lib-dynload" */
        wchar_t *lib_dynload = joinpath2(calculate->platlibdir_macro,
                                         L"lib-dynload");
        if (lib_dynload == NULL) {
            return _PyStatus_NO_MEMORY();
        }

        calculate->exec_prefix = joinpath2(calculate->exec_prefix_macro,
                                           lib_dynload);
        PyMem_RawFree(lib_dynload);

        if (calculate->exec_prefix == NULL) {
            return _PyStatus_NO_MEMORY();
        }
    }
    else {
        /* If we found EXEC_PREFIX do *not* reduce it!  (Yet.) */
        calculate->exec_prefix = _PyMem_RawWcsdup(exec_prefix);
        if (calculate->exec_prefix == NULL) {
            return _PyStatus_NO_MEMORY();
        }
    }
    return _PyStatus_OK();
}


static PyStatus
calculate_set_exec_prefix(PyCalculatePath *calculate,
                          _PyPathConfig *pathconfig)
{
    if (calculate->exec_prefix_found > 0) {
        wchar_t *exec_prefix = _PyMem_RawWcsdup(calculate->exec_prefix);
        if (exec_prefix == NULL) {
            return _PyStatus_NO_MEMORY();
        }

        reduce(exec_prefix);
        reduce(exec_prefix);
        reduce(exec_prefix);

        if (exec_prefix[0]) {
            pathconfig->exec_prefix = exec_prefix;
        }
        else {
            /* empty string: use SEP instead */
            PyMem_RawFree(exec_prefix);

            /* The exec_prefix is the root directory, but reduce() chopped
               off the "/". */
            pathconfig->exec_prefix = _PyMem_RawWcsdup(separator);
            if (pathconfig->exec_prefix == NULL) {
                return _PyStatus_NO_MEMORY();
            }
        }
    }
    else {
        pathconfig->exec_prefix = _PyMem_RawWcsdup(calculate->exec_prefix_macro);
        if (pathconfig->exec_prefix == NULL) {
            return _PyStatus_NO_MEMORY();
        }
    }
    return _PyStatus_OK();
}


/* Similar to shutil.which().
   If found, write the path into *abs_path_p. */
static PyStatus
calculate_which(const wchar_t *path_env, wchar_t *program_name,
                wchar_t **abs_path_p)
{
    while (1) {
        wchar_t *delim = wcschr(path_env, DELIM);
        wchar_t *abs_path;

        if (delim) {
            wchar_t *path = substring(path_env, delim - path_env);
            if (path == NULL) {
                return _PyStatus_NO_MEMORY();
            }
            abs_path = joinpath2(path, program_name);
            PyMem_RawFree(path);
        }
        else {
            abs_path = joinpath2(path_env, program_name);
        }

        if (abs_path == NULL) {
            return _PyStatus_NO_MEMORY();
        }

        if (isxfile(abs_path)) {
            *abs_path_p = abs_path;
            return _PyStatus_OK();
        }
        PyMem_RawFree(abs_path);

        if (!delim) {
            break;
        }
        path_env = delim + 1;
    }

    /* not found */
    return _PyStatus_OK();
}


#ifdef __APPLE__
static PyStatus
calculate_program_macos(wchar_t **abs_path_p)
{
    char execpath[MAXPATHLEN + 1];
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
    uint32_t nsexeclength = Py_ARRAY_LENGTH(execpath) - 1;
#else
    unsigned long nsexeclength = Py_ARRAY_LENGTH(execpath) - 1;
#endif

    /* On Mac OS X, if a script uses an interpreter of the form
       "#!/opt/python2.3/bin/python", the kernel only passes "python"
       as argv[0], which falls through to the $PATH search below.
       If /opt/python2.3/bin isn't in your path, or is near the end,
       this algorithm may incorrectly find /usr/bin/python. To work
       around this, we can use _NSGetExecutablePath to get a better
       hint of what the intended interpreter was, although this
       will fail if a relative path was used. but in that case,
       absolutize() should help us out below
     */
    if (_NSGetExecutablePath(execpath, &nsexeclength) != 0
        || (wchar_t)execpath[0] != SEP)
    {
        /* _NSGetExecutablePath() failed or the path is relative */
        return _PyStatus_OK();
    }

    size_t len;
    *abs_path_p = Py_DecodeLocale(execpath, &len);
    if (*abs_path_p == NULL) {
        return DECODE_LOCALE_ERR("executable path", len);
    }
    return _PyStatus_OK();
}
#endif  /* __APPLE__ */


static PyStatus
calculate_program_impl(PyCalculatePath *calculate, _PyPathConfig *pathconfig)
{
    assert(pathconfig->program_full_path == NULL);

    PyStatus status;

    /* If there is no slash in the argv0 path, then we have to
     * assume python is on the user's $PATH, since there's no
     * other way to find a directory to start the search from.  If
     * $PATH isn't exported, you lose.
     */
    if (wcschr(pathconfig->program_name, SEP)) {
        pathconfig->program_full_path = _PyMem_RawWcsdup(pathconfig->program_name);
        if (pathconfig->program_full_path == NULL) {
            return _PyStatus_NO_MEMORY();
        }
        return _PyStatus_OK();
    }

#ifdef __APPLE__
    wchar_t *abs_path = NULL;
    status = calculate_program_macos(&abs_path);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }
    if (abs_path) {
        pathconfig->program_full_path = abs_path;
        return _PyStatus_OK();
    }
#endif /* __APPLE__ */

    if (calculate->path_env) {
        wchar_t *abs_path = NULL;
        status = calculate_which(calculate->path_env, pathconfig->program_name,
                                 &abs_path);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
        if (abs_path) {
            pathconfig->program_full_path = abs_path;
            return _PyStatus_OK();
        }
    }

    /* In the last resort, use an empty string */
    pathconfig->program_full_path = _PyMem_RawWcsdup(L"");
    if (pathconfig->program_full_path == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    return _PyStatus_OK();
}


/* Calculate pathconfig->program_full_path */
static PyStatus
calculate_program(PyCalculatePath *calculate, _PyPathConfig *pathconfig)
{
    PyStatus status;

    status = calculate_program_impl(calculate, pathconfig);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if (pathconfig->program_full_path[0] != '\0') {
        /* program_full_path is not empty */

        /* Make sure that program_full_path is an absolute path */
        if (!_Py_isabs(pathconfig->program_full_path)) {
            status = absolutize(&pathconfig->program_full_path);
            if (_PyStatus_EXCEPTION(status)) {
                return status;
            }
        }

#if defined(__CYGWIN__) || defined(__MINGW32__)
        /* For these platforms it is necessary to ensure that the .exe suffix
         * is appended to the filename, otherwise there is potential for
         * sys.executable to return the name of a directory under the same
         * path (bpo-28441).
         */
        status = add_exe_suffix(&pathconfig->program_full_path);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
#endif
    }
    return _PyStatus_OK();
}


#if HAVE_READLINK
static PyStatus
resolve_symlinks(wchar_t **path_p)
{
    wchar_t new_path[MAXPATHLEN + 1];
    const size_t new_path_len = Py_ARRAY_LENGTH(new_path);
    unsigned int nlink = 0;

    while (1) {
        int linklen = _Py_wreadlink(*path_p, new_path, new_path_len);
        if (linklen == -1) {
            /* not a symbolic link: we are done */
            break;
        }

        if (_Py_isabs(new_path)) {
            PyMem_RawFree(*path_p);
            *path_p = _PyMem_RawWcsdup(new_path);
            if (*path_p == NULL) {
                return _PyStatus_NO_MEMORY();
            }
        }
        else {
            /* new_path is relative to path */
            reduce(*path_p);

            wchar_t *abs_path = joinpath2(*path_p, new_path);
            if (abs_path == NULL) {
                return _PyStatus_NO_MEMORY();
            }

            PyMem_RawFree(*path_p);
            *path_p = abs_path;
        }

        nlink++;
        /* 40 is the Linux kernel 4.2 limit */
        if (nlink >= 40) {
            return _PyStatus_ERR("maximum number of symbolic links reached");
        }
    }
    return _PyStatus_OK();
}
#endif /* HAVE_READLINK */


#ifdef WITH_NEXT_FRAMEWORK
static PyStatus
calculate_argv0_path_framework(PyCalculatePath *calculate, _PyPathConfig *pathconfig)
{
    NSModule pythonModule;

    /* On Mac OS X we have a special case if we're running from a framework.
       This is because the python home should be set relative to the library,
       which is in the framework, not relative to the executable, which may
       be outside of the framework. Except when we're in the build
       directory... */
    pythonModule = NSModuleForSymbol(NSLookupAndBindSymbol("_Py_Initialize"));

    /* Use dylib functions to find out where the framework was loaded from */
    const char* modPath = NSLibraryNameForModule(pythonModule);
    if (modPath == NULL) {
        return _PyStatus_OK();
    }

    /* We're in a framework.
       See if we might be in the build directory. The framework in the
       build directory is incomplete, it only has the .dylib and a few
       needed symlinks, it doesn't have the Lib directories and such.
       If we're running with the framework from the build directory we must
       be running the interpreter in the build directory, so we use the
       build-directory-specific logic to find Lib and such. */
    size_t len;
    wchar_t* wbuf = Py_DecodeLocale(modPath, &len);
    if (wbuf == NULL) {
        return DECODE_LOCALE_ERR("framework location", len);
    }

    /* Path: reduce(modPath) / lib_python / LANDMARK */
    PyStatus status;

    wchar_t *parent = _PyMem_RawWcsdup(wbuf);
    if (parent == NULL) {
        status = _PyStatus_NO_MEMORY();
        goto done;
    }

    reduce(parent);
    wchar_t *lib_python = joinpath2(parent, calculate->lib_python);
    PyMem_RawFree(parent);

    if (lib_python == NULL) {
        status = _PyStatus_NO_MEMORY();
        goto done;
    }

    int module;
    status = ismodule(lib_python, &module);
    PyMem_RawFree(lib_python);

    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }
    if (!module) {
        /* We are in the build directory so use the name of the
           executable - we know that the absolute path is passed */
        PyMem_RawFree(calculate->argv0_path);
        calculate->argv0_path = _PyMem_RawWcsdup(pathconfig->program_full_path);
        if (calculate->argv0_path == NULL) {
            status = _PyStatus_NO_MEMORY();
            goto done;
        }

        status = _PyStatus_OK();
        goto done;
    }

    /* Use the location of the library as argv0_path */
    PyMem_RawFree(calculate->argv0_path);
    calculate->argv0_path = wbuf;
    return _PyStatus_OK();

done:
    PyMem_RawFree(wbuf);
    return status;
}
#endif


static PyStatus
calculate_argv0_path(PyCalculatePath *calculate,
                     _PyPathConfig *pathconfig)
{
    PyStatus status;

    calculate->argv0_path = _PyMem_RawWcsdup(pathconfig->program_full_path);
    if (calculate->argv0_path == NULL) {
        return _PyStatus_NO_MEMORY();
    }

#ifdef WITH_NEXT_FRAMEWORK
    status = calculate_argv0_path_framework(calculate, pathconfig);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }
#endif

    status = resolve_symlinks(&calculate->argv0_path);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    reduce(calculate->argv0_path);

    return _PyStatus_OK();
}


static PyStatus
calculate_open_pyenv(PyCalculatePath *calculate, FILE **env_file_p)
{
    *env_file_p = NULL;

    const wchar_t *env_cfg = L"pyvenv.cfg";

    /* Filename: <argv0_path> / "pyvenv.cfg" */
    wchar_t *filename = joinpath2(calculate->argv0_path, env_cfg);
    if (filename == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    *env_file_p = _Py_wfopen(filename, L"r");
    PyMem_RawFree(filename);

    if (*env_file_p != NULL) {
        return _PyStatus_OK();

    }

    /* fopen() failed: reset errno */
    errno = 0;

    /* Path: <basename(argv0_path)> / "pyvenv.cfg" */
    wchar_t *parent = _PyMem_RawWcsdup(calculate->argv0_path);
    if (parent == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    reduce(parent);

    filename = joinpath2(parent, env_cfg);
    PyMem_RawFree(parent);
    if (filename == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    *env_file_p = _Py_wfopen(filename, L"r");
    PyMem_RawFree(filename);

    if (*env_file_p == NULL) {
        /* fopen() failed: reset errno */
        errno = 0;
    }
    return _PyStatus_OK();
}


/* Search for an "pyvenv.cfg" environment configuration file, first in the
   executable's directory and then in the parent directory.
   If found, open it for use when searching for prefixes.

   Write the 'home' variable of pyvenv.cfg into calculate->argv0_path. */
static PyStatus
calculate_read_pyenv(PyCalculatePath *calculate)
{
    PyStatus status;
    FILE *env_file = NULL;

    status = calculate_open_pyenv(calculate, &env_file);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }
    if (env_file == NULL) {
        /* pyvenv.cfg not found */
        return _PyStatus_OK();
    }

    /* Look for a 'home' variable and set argv0_path to it, if found */
    wchar_t *home = NULL;
    status = _Py_FindEnvConfigValue(env_file, L"home", &home);
    if (_PyStatus_EXCEPTION(status)) {
        fclose(env_file);
        return status;
    }

    if (home) {
        PyMem_RawFree(calculate->argv0_path);
        calculate->argv0_path = home;
    }
    fclose(env_file);
    return _PyStatus_OK();
}


static PyStatus
calculate_zip_path(PyCalculatePath *calculate)
{
    PyStatus res;

    /* Path: <PLATLIBDIR> / "python00.zip" */
    wchar_t *path = joinpath2(calculate->platlibdir_macro, L"python00.zip");
    if (path == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    if (calculate->prefix_found > 0) {
        /* Use the reduced prefix returned by Py_GetPrefix()

           Path: <basename(basename(prefix))> / <PLATLIBDIR> / "python00.zip" */
        wchar_t *parent = _PyMem_RawWcsdup(calculate->prefix);
        if (parent == NULL) {
            res = _PyStatus_NO_MEMORY();
            goto done;
        }
        reduce(parent);
        reduce(parent);
        calculate->zip_path = joinpath2(parent, path);
        PyMem_RawFree(parent);
    }
    else {
        calculate->zip_path = joinpath2(calculate->prefix_macro, path);
    }

    if (calculate->zip_path == NULL) {
        res = _PyStatus_NO_MEMORY();
        goto done;
    }

    /* Replace "00" with version */
    size_t len = wcslen(calculate->zip_path);
    calculate->zip_path[len - 6] = VERSION[0];
    calculate->zip_path[len - 5] = VERSION[2];

    res = _PyStatus_OK();

done:
    PyMem_RawFree(path);
    return res;
}


static PyStatus
calculate_module_search_path(PyCalculatePath *calculate,
                             _PyPathConfig *pathconfig)
{
    /* Calculate size of return buffer */
    size_t bufsz = 0;
    if (calculate->pythonpath_env != NULL) {
        bufsz += wcslen(calculate->pythonpath_env) + 1;
    }

    wchar_t *defpath = calculate->pythonpath_macro;
    size_t prefixsz = wcslen(calculate->prefix) + 1;
    while (1) {
        wchar_t *delim = wcschr(defpath, DELIM);

        if (!_Py_isabs(defpath)) {
            /* Paths are relative to prefix */
            bufsz += prefixsz;
        }

        if (delim) {
            bufsz += delim - defpath + 1;
        }
        else {
            bufsz += wcslen(defpath) + 1;
            break;
        }
        defpath = delim + 1;
    }

    bufsz += wcslen(calculate->zip_path) + 1;
    bufsz += wcslen(calculate->exec_prefix) + 1;

    /* Allocate the buffer */
    wchar_t *buf = PyMem_RawMalloc(bufsz * sizeof(wchar_t));
    if (buf == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    buf[0] = '\0';

    /* Run-time value of $PYTHONPATH goes first */
    if (calculate->pythonpath_env) {
        wcscpy(buf, calculate->pythonpath_env);
        wcscat(buf, delimiter);
    }

    /* Next is the default zip path */
    wcscat(buf, calculate->zip_path);
    wcscat(buf, delimiter);

    /* Next goes merge of compile-time $PYTHONPATH with
     * dynamically located prefix.
     */
    defpath = calculate->pythonpath_macro;
    while (1) {
        wchar_t *delim = wcschr(defpath, DELIM);

        if (!_Py_isabs(defpath)) {
            wcscat(buf, calculate->prefix);
            if (prefixsz >= 2 && calculate->prefix[prefixsz - 2] != SEP &&
                defpath[0] != (delim ? DELIM : L'\0'))
            {
                /* not empty */
                wcscat(buf, separator);
            }
        }

        if (delim) {
            size_t len = delim - defpath + 1;
            size_t end = wcslen(buf) + len;
            wcsncat(buf, defpath, len);
            buf[end] = '\0';
        }
        else {
            wcscat(buf, defpath);
            break;
        }
        defpath = delim + 1;
    }
    wcscat(buf, delimiter);

    /* Finally, on goes the directory for dynamic-load modules */
    wcscat(buf, calculate->exec_prefix);

    pathconfig->module_search_path = buf;
    return _PyStatus_OK();
}


static PyStatus
calculate_init(PyCalculatePath *calculate, const PyConfig *config)
{
    size_t len;
    const char *path = getenv("PATH");
    if (path) {
        calculate->path_env = Py_DecodeLocale(path, &len);
        if (!calculate->path_env) {
            return DECODE_LOCALE_ERR("PATH environment variable", len);
        }
    }

    /* Decode macros */
    calculate->pythonpath_macro = Py_DecodeLocale(PYTHONPATH, &len);
    if (!calculate->pythonpath_macro) {
        return DECODE_LOCALE_ERR("PYTHONPATH macro", len);
    }
    calculate->prefix_macro = Py_DecodeLocale(PREFIX, &len);
    if (!calculate->prefix_macro) {
        return DECODE_LOCALE_ERR("PREFIX macro", len);
    }
    calculate->exec_prefix_macro = Py_DecodeLocale(EXEC_PREFIX, &len);
    if (!calculate->exec_prefix_macro) {
        return DECODE_LOCALE_ERR("EXEC_PREFIX macro", len);
    }
    calculate->vpath_macro = Py_DecodeLocale(VPATH, &len);
    if (!calculate->vpath_macro) {
        return DECODE_LOCALE_ERR("VPATH macro", len);
    }
    calculate->platlibdir_macro = Py_DecodeLocale(PLATLIBDIR, &len);
    if (!calculate->platlibdir_macro) {
        return DECODE_LOCALE_ERR("PLATLIBDIR macro", len);
    }

    calculate->lib_python = Py_DecodeLocale(PLATLIBDIR "/python" VERSION, &len);
    if (!calculate->lib_python) {
        return DECODE_LOCALE_ERR("VERSION macro", len);
    }

    calculate->warnings = config->pathconfig_warnings;
    calculate->pythonpath_env = config->pythonpath_env;

    return _PyStatus_OK();
}


static void
calculate_free(PyCalculatePath *calculate)
{
    PyMem_RawFree(calculate->pythonpath_macro);
    PyMem_RawFree(calculate->prefix_macro);
    PyMem_RawFree(calculate->exec_prefix_macro);
    PyMem_RawFree(calculate->vpath_macro);
    PyMem_RawFree(calculate->platlibdir_macro);
    PyMem_RawFree(calculate->lib_python);
    PyMem_RawFree(calculate->path_env);
    PyMem_RawFree(calculate->zip_path);
    PyMem_RawFree(calculate->argv0_path);
    PyMem_RawFree(calculate->prefix);
    PyMem_RawFree(calculate->exec_prefix);
}


static PyStatus
calculate_path(PyCalculatePath *calculate, _PyPathConfig *pathconfig)
{
    PyStatus status;

    if (pathconfig->program_full_path == NULL) {
        status = calculate_program(calculate, pathconfig);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    status = calculate_argv0_path(calculate, pathconfig);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    /* If a pyvenv.cfg configure file is found,
       argv0_path is overriden with its 'home' variable. */
    status = calculate_read_pyenv(calculate);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = calculate_prefix(calculate, pathconfig);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = calculate_zip_path(calculate);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = calculate_exec_prefix(calculate, pathconfig);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if ((!calculate->prefix_found || !calculate->exec_prefix_found)
        && calculate->warnings)
    {
        fprintf(stderr,
                "Consider setting $PYTHONHOME to <prefix>[:<exec_prefix>]\n");
    }

    if (pathconfig->module_search_path == NULL) {
        status = calculate_module_search_path(calculate, pathconfig);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (pathconfig->prefix == NULL) {
        status = calculate_set_prefix(calculate, pathconfig);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (pathconfig->exec_prefix == NULL) {
        status = calculate_set_exec_prefix(calculate, pathconfig);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
    return _PyStatus_OK();
}


/* Calculate the Python path configuration.

   Inputs:

   - PATH environment variable
   - Macros: PYTHONPATH, PREFIX, EXEC_PREFIX, VERSION (ex: "3.9").
     PREFIX and EXEC_PREFIX are generated by the configure script.
     PYTHONPATH macro is the default search path.
   - pybuilddir.txt file
   - pyvenv.cfg configuration file
   - PyConfig fields ('config' function argument):

     - pathconfig_warnings
     - pythonpath_env (PYTHONPATH environment variable)

   - _PyPathConfig fields ('pathconfig' function argument):

     - program_name: see config_init_program_name()
     - home: Py_SetPythonHome() or PYTHONHOME environment variable

   - current working directory: see copy_absolute()

   Outputs, 'pathconfig' fields:

   - program_full_path
   - module_search_path
   - prefix
   - exec_prefix

   If a field is already set (non NULL), it is left unchanged. */
PyStatus
_PyPathConfig_Calculate(_PyPathConfig *pathconfig, const PyConfig *config)
{
    PyStatus status;
    PyCalculatePath calculate;
    memset(&calculate, 0, sizeof(calculate));

    status = calculate_init(&calculate, config);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    status = calculate_path(&calculate, pathconfig);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    /* program_full_path must an either an empty string or an absolute path */
    assert(wcslen(pathconfig->program_full_path) == 0
           || _Py_isabs(pathconfig->program_full_path));

    status = _PyStatus_OK();

done:
    calculate_free(&calculate);
    return status;
}

#ifdef __cplusplus
}
#endif
