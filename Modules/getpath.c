/* Return the initial module search path. */

#include "Python.h"
#include "pycore_initconfig.h"
#include "osdefs.h"
#include "pycore_fileutils.h"
#include "pycore_pathconfig.h"
#include "pycore_pystate.h"

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
 * these authomatic path computations.
 *
 * NOTE: Windows MSVC builds use PC/getpathp.c instead!
 */

#ifdef __cplusplus
extern "C" {
#endif


#if !defined(PREFIX) || !defined(EXEC_PREFIX) || !defined(VERSION) || !defined(VPATH)
#error "PREFIX, EXEC_PREFIX, VERSION, and VPATH must be constant defined"
#endif

#ifndef LANDMARK
#define LANDMARK L"os.py"
#endif

#define DECODE_LOCALE_ERR(NAME, LEN) \
    ((LEN) == (size_t)-2) \
     ? _PyStatus_ERR("cannot decode " NAME) \
     : _PyStatus_NO_MEMORY()

#define PATHLEN_ERR() _PyStatus_ERR("path configuration: path too long")

typedef struct {
    wchar_t *path_env;                 /* PATH environment variable */

    wchar_t *pythonpath;               /* PYTHONPATH define */
    wchar_t *prefix;                   /* PREFIX define */
    wchar_t *exec_prefix;              /* EXEC_PREFIX define */

    wchar_t *lib_python;               /* "lib/pythonX.Y" */
    wchar_t argv0_path[MAXPATHLEN+1];
    wchar_t zip_path[MAXPATHLEN+1];    /* ".../lib/pythonXY.zip" */

    int prefix_found;         /* found platform independent libraries? */
    int exec_prefix_found;    /* found the platform dependent libraries? */
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


/* Is module -- check for .pyc too */
static int
ismodule(wchar_t *filename, size_t filename_len)
{
    if (isfile(filename)) {
        return 1;
    }

    /* Check for the compiled version of prefix. */
    if (wcslen(filename) + 2 <= filename_len) {
        wcscat(filename, L"c");
        if (isfile(filename)) {
            return 1;
        }
    }
    return 0;
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
isdir(wchar_t *filename)
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
   buflen: 'buffer' length in characters including trailing NUL. */
static PyStatus
joinpath(wchar_t *buffer, const wchar_t *stuff, size_t buflen)
{
    size_t n, k;
    if (stuff[0] != SEP) {
        n = wcslen(buffer);
        if (n >= buflen) {
            return PATHLEN_ERR();
        }

        if (n > 0 && buffer[n-1] != SEP) {
            buffer[n++] = SEP;
        }
    }
    else {
        n = 0;
    }

    k = wcslen(stuff);
    if (n + k >= buflen) {
        return PATHLEN_ERR();
    }
    wcsncpy(buffer+n, stuff, k);
    buffer[n+k] = '\0';

    return _PyStatus_OK();
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
   'pathlen' characters (including trailing NUL). */
static PyStatus
copy_absolute(wchar_t *path, const wchar_t *p, size_t pathlen)
{
    if (p[0] == SEP) {
        if (safe_wcscpy(path, p, pathlen) < 0) {
            return PATHLEN_ERR();
        }
    }
    else {
        if (!_Py_wgetcwd(path, pathlen)) {
            /* unable to get the current directory */
            if (safe_wcscpy(path, p, pathlen) < 0) {
                return PATHLEN_ERR();
            }
            return _PyStatus_OK();
        }
        if (p[0] == '.' && p[1] == SEP) {
            p += 2;
        }
        PyStatus status = joinpath(path, p, pathlen);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
    return _PyStatus_OK();
}


/* path_len: path length in characters including trailing NUL */
static PyStatus
absolutize(wchar_t *path, size_t path_len)
{
    if (path[0] == SEP) {
        return _PyStatus_OK();
    }

    wchar_t abs_path[MAXPATHLEN+1];
    PyStatus status = copy_absolute(abs_path, path, Py_ARRAY_LENGTH(abs_path));
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if (safe_wcscpy(path, abs_path, path_len) < 0) {
        return PATHLEN_ERR();
    }
    return _PyStatus_OK();
}


#if defined(__CYGWIN__) || defined(__MINGW32__)
#ifndef EXE_SUFFIX
#define EXE_SUFFIX L".exe"
#endif

/* pathlen: 'path' length in characters including trailing NUL */
static PyStatus
add_exe_suffix(wchar_t *progpath, size_t progpathlen)
{
    /* Check for already have an executable suffix */
    size_t n = wcslen(progpath);
    size_t s = wcslen(EXE_SUFFIX);
    if (wcsncasecmp(EXE_SUFFIX, progpath + n - s, s) == 0) {
        return _PyStatus_OK();
    }

    if (n + s >= progpathlen) {
        return PATHLEN_ERR();
    }
    wcsncpy(progpath + n, EXE_SUFFIX, s);
    progpath[n+s] = '\0';

    if (!isxfile(progpath)) {
        /* Path that added suffix is invalid: truncate (remove suffix) */
        progpath[n] = '\0';
    }

    return _PyStatus_OK();
}
#endif


/* search_for_prefix requires that argv0_path be no more than MAXPATHLEN
   bytes long.
*/
static PyStatus
search_for_prefix(const PyConfig *config, PyCalculatePath *calculate,
                  wchar_t *prefix, size_t prefix_len,
                  int *found)
{
    PyStatus status;
    size_t n;
    wchar_t *vpath;

    /* If PYTHONHOME is set, we believe it unconditionally */
    if (config->home) {
        if (safe_wcscpy(prefix, config->home, prefix_len) < 0) {
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
        status = joinpath(prefix, LANDMARK, prefix_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
        *found = 1;
        return _PyStatus_OK();
    }

    /* Check to see if argv[0] is in the build directory */
    if (safe_wcscpy(prefix, calculate->argv0_path, prefix_len) < 0) {
        return PATHLEN_ERR();
    }
    status = joinpath(prefix, L"Modules/Setup.local", prefix_len);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if (isfile(prefix)) {
        /* Check VPATH to see if argv0_path is in the build directory. */
        vpath = Py_DecodeLocale(VPATH, NULL);
        if (vpath != NULL) {
            if (safe_wcscpy(prefix, calculate->argv0_path, prefix_len) < 0) {
                return PATHLEN_ERR();
            }
            status = joinpath(prefix, vpath, prefix_len);
            PyMem_RawFree(vpath);
            if (_PyStatus_EXCEPTION(status)) {
                return status;
            }

            status = joinpath(prefix, L"Lib", prefix_len);
            if (_PyStatus_EXCEPTION(status)) {
                return status;
            }
            status = joinpath(prefix, LANDMARK, prefix_len);
            if (_PyStatus_EXCEPTION(status)) {
                return status;
            }

            if (ismodule(prefix, prefix_len)) {
                *found = -1;
                return _PyStatus_OK();
            }
        }
    }

    /* Search from argv0_path, until root is found */
    status = copy_absolute(prefix, calculate->argv0_path, prefix_len);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    do {
        n = wcslen(prefix);
        status = joinpath(prefix, calculate->lib_python, prefix_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
        status = joinpath(prefix, LANDMARK, prefix_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }

        if (ismodule(prefix, prefix_len)) {
            *found = 1;
            return _PyStatus_OK();
        }
        prefix[n] = L'\0';
        reduce(prefix);
    } while (prefix[0]);

    /* Look at configure's PREFIX */
    if (safe_wcscpy(prefix, calculate->prefix, prefix_len) < 0) {
        return PATHLEN_ERR();
    }
    status = joinpath(prefix, calculate->lib_python, prefix_len);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }
    status = joinpath(prefix, LANDMARK, prefix_len);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if (ismodule(prefix, prefix_len)) {
        *found = 1;
        return _PyStatus_OK();
    }

    /* Fail */
    *found = 0;
    return _PyStatus_OK();
}


static PyStatus
calculate_prefix(const PyConfig *config,
                 PyCalculatePath *calculate, wchar_t *prefix, size_t prefix_len)
{
    PyStatus status;

    status = search_for_prefix(config, calculate, prefix, prefix_len,
                            &calculate->prefix_found);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if (!calculate->prefix_found) {
        if (config->pathconfig_warnings) {
            fprintf(stderr,
                "Could not find platform independent libraries <prefix>\n");
        }
        if (safe_wcscpy(prefix, calculate->prefix, prefix_len) < 0) {
            return PATHLEN_ERR();
        }
        status = joinpath(prefix, calculate->lib_python, prefix_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
    else {
        reduce(prefix);
    }
    return _PyStatus_OK();
}


static PyStatus
calculate_reduce_prefix(PyCalculatePath *calculate,
                        wchar_t *prefix, size_t prefix_len)
{
    /* Reduce prefix and exec_prefix to their essence,
     * e.g. /usr/local/lib/python1.5 is reduced to /usr/local.
     * If we're loading relative to the build directory,
     * return the compiled-in defaults instead.
     */
    if (calculate->prefix_found > 0) {
        reduce(prefix);
        reduce(prefix);
        /* The prefix is the root directory, but reduce() chopped
         * off the "/". */
        if (!prefix[0]) {
            wcscpy(prefix, separator);
        }
    }
    else {
        if (safe_wcscpy(prefix, calculate->prefix, prefix_len) < 0) {
            return PATHLEN_ERR();
        }
    }
    return _PyStatus_OK();
}


/* search_for_exec_prefix requires that argv0_path be no more than
   MAXPATHLEN bytes long.
*/
static PyStatus
search_for_exec_prefix(const PyConfig *config,
                       PyCalculatePath *calculate,
                       wchar_t *exec_prefix, size_t exec_prefix_len,
                       int *found)
{
    PyStatus status;
    size_t n;

    /* If PYTHONHOME is set, we believe it unconditionally */
    if (config->home) {
        wchar_t *delim = wcschr(config->home, DELIM);
        if (delim) {
            if (safe_wcscpy(exec_prefix, delim+1, exec_prefix_len) < 0) {
                return PATHLEN_ERR();
            }
        }
        else {
            if (safe_wcscpy(exec_prefix, config->home, exec_prefix_len) < 0) {
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

    /* Check to see if argv[0] is in the build directory. "pybuilddir.txt"
       is written by setup.py and contains the relative path to the location
       of shared library modules. */
    if (safe_wcscpy(exec_prefix, calculate->argv0_path, exec_prefix_len) < 0) {
        return PATHLEN_ERR();
    }
    status = joinpath(exec_prefix, L"pybuilddir.txt", exec_prefix_len);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if (isfile(exec_prefix)) {
        FILE *f = _Py_wfopen(exec_prefix, L"rb");
        if (f == NULL) {
            errno = 0;
        }
        else {
            char buf[MAXPATHLEN + 1];
            n = fread(buf, 1, Py_ARRAY_LENGTH(buf) - 1, f);
            buf[n] = '\0';
            fclose(f);

            wchar_t *pybuilddir;
            size_t dec_len;
            pybuilddir = _Py_DecodeUTF8_surrogateescape(buf, n, &dec_len);
            if (!pybuilddir) {
                return DECODE_LOCALE_ERR("pybuilddir.txt", dec_len);
            }

            if (safe_wcscpy(exec_prefix, calculate->argv0_path, exec_prefix_len) < 0) {
                return PATHLEN_ERR();
            }
            status = joinpath(exec_prefix, pybuilddir, exec_prefix_len);
            PyMem_RawFree(pybuilddir );
            if (_PyStatus_EXCEPTION(status)) {
                return status;
            }

            *found = -1;
            return _PyStatus_OK();
        }
    }

    /* Search from argv0_path, until root is found */
    status = copy_absolute(exec_prefix, calculate->argv0_path, exec_prefix_len);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    do {
        n = wcslen(exec_prefix);
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

    /* Look at configure's EXEC_PREFIX */
    if (safe_wcscpy(exec_prefix, calculate->exec_prefix, exec_prefix_len) < 0) {
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
calculate_exec_prefix(const PyConfig *config,
                      PyCalculatePath *calculate,
                      wchar_t *exec_prefix, size_t exec_prefix_len)
{
    PyStatus status;

    status = search_for_exec_prefix(config, calculate,
                                 exec_prefix, exec_prefix_len,
                                 &calculate->exec_prefix_found);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if (!calculate->exec_prefix_found) {
        if (config->pathconfig_warnings) {
            fprintf(stderr,
                "Could not find platform dependent libraries <exec_prefix>\n");
        }
        if (safe_wcscpy(exec_prefix, calculate->exec_prefix, exec_prefix_len) < 0) {
            return PATHLEN_ERR();
        }
        status = joinpath(exec_prefix, L"lib/lib-dynload", exec_prefix_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
    /* If we found EXEC_PREFIX do *not* reduce it!  (Yet.) */
    return _PyStatus_OK();
}


static PyStatus
calculate_reduce_exec_prefix(PyCalculatePath *calculate,
                             wchar_t *exec_prefix, size_t exec_prefix_len)
{
    if (calculate->exec_prefix_found > 0) {
        reduce(exec_prefix);
        reduce(exec_prefix);
        reduce(exec_prefix);
        if (!exec_prefix[0]) {
            wcscpy(exec_prefix, separator);
        }
    }
    else {
        if (safe_wcscpy(exec_prefix, calculate->exec_prefix, exec_prefix_len) < 0) {
            return PATHLEN_ERR();
        }
    }
    return _PyStatus_OK();
}


static PyStatus
calculate_program_full_path(const PyConfig *config,
                            PyCalculatePath *calculate, _PyPathConfig *pathconfig)
{
    PyStatus status;
    wchar_t program_full_path[MAXPATHLEN + 1];
    const size_t program_full_path_len = Py_ARRAY_LENGTH(program_full_path);
    memset(program_full_path, 0, sizeof(program_full_path));

#ifdef __APPLE__
    char execpath[MAXPATHLEN + 1];
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
    uint32_t nsexeclength = Py_ARRAY_LENGTH(execpath) - 1;
#else
    unsigned long nsexeclength = Py_ARRAY_LENGTH(execpath) - 1;
#endif
#endif

    /* If there is no slash in the argv0 path, then we have to
     * assume python is on the user's $PATH, since there's no
     * other way to find a directory to start the search from.  If
     * $PATH isn't exported, you lose.
     */
    if (wcschr(config->program_name, SEP)) {
        if (safe_wcscpy(program_full_path, config->program_name,
                        program_full_path_len) < 0) {
            return PATHLEN_ERR();
        }
    }
#ifdef __APPLE__
     /* On Mac OS X, if a script uses an interpreter of the form
      * "#!/opt/python2.3/bin/python", the kernel only passes "python"
      * as argv[0], which falls through to the $PATH search below.
      * If /opt/python2.3/bin isn't in your path, or is near the end,
      * this algorithm may incorrectly find /usr/bin/python. To work
      * around this, we can use _NSGetExecutablePath to get a better
      * hint of what the intended interpreter was, although this
      * will fail if a relative path was used. but in that case,
      * absolutize() should help us out below
      */
    else if(0 == _NSGetExecutablePath(execpath, &nsexeclength) &&
            execpath[0] == SEP)
    {
        size_t len;
        wchar_t *path = Py_DecodeLocale(execpath, &len);
        if (path == NULL) {
            return DECODE_LOCALE_ERR("executable path", len);
        }
        if (safe_wcscpy(program_full_path, path, program_full_path_len) < 0) {
            PyMem_RawFree(path);
            return PATHLEN_ERR();
        }
        PyMem_RawFree(path);
    }
#endif /* __APPLE__ */
    else if (calculate->path_env) {
        wchar_t *path = calculate->path_env;
        while (1) {
            wchar_t *delim = wcschr(path, DELIM);

            if (delim) {
                size_t len = delim - path;
                if (len >= program_full_path_len) {
                    return PATHLEN_ERR();
                }
                wcsncpy(program_full_path, path, len);
                program_full_path[len] = '\0';
            }
            else {
                if (safe_wcscpy(program_full_path, path,
                                program_full_path_len) < 0) {
                    return PATHLEN_ERR();
                }
            }

            status = joinpath(program_full_path, config->program_name,
                            program_full_path_len);
            if (_PyStatus_EXCEPTION(status)) {
                return status;
            }

            if (isxfile(program_full_path)) {
                break;
            }

            if (!delim) {
                program_full_path[0] = L'\0';
                break;
            }
            path = delim + 1;
        }
    }
    else {
        program_full_path[0] = '\0';
    }
    if (program_full_path[0] != SEP && program_full_path[0] != '\0') {
        status = absolutize(program_full_path, program_full_path_len);
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
    if (program_full_path[0] != '\0') {
        status = add_exe_suffix(program_full_path, program_full_path_len);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
#endif

    pathconfig->program_full_path = _PyMem_RawWcsdup(program_full_path);
    if (pathconfig->program_full_path == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    return _PyStatus_OK();
}


static PyStatus
calculate_argv0_path(PyCalculatePath *calculate, const wchar_t *program_full_path)
{
    const size_t argv0_path_len = Py_ARRAY_LENGTH(calculate->argv0_path);
    if (safe_wcscpy(calculate->argv0_path, program_full_path, argv0_path_len) < 0) {
        return PATHLEN_ERR();
    }

#ifdef WITH_NEXT_FRAMEWORK
    NSModule pythonModule;

    /* On Mac OS X we have a special case if we're running from a framework.
    ** This is because the python home should be set relative to the library,
    ** which is in the framework, not relative to the executable, which may
    ** be outside of the framework. Except when we're in the build directory...
    */
    pythonModule = NSModuleForSymbol(NSLookupAndBindSymbol("_Py_Initialize"));
    /* Use dylib functions to find out where the framework was loaded from */
    const char* modPath = NSLibraryNameForModule(pythonModule);
    if (modPath != NULL) {
        /* We're in a framework. */
        /* See if we might be in the build directory. The framework in the
        ** build directory is incomplete, it only has the .dylib and a few
        ** needed symlinks, it doesn't have the Lib directories and such.
        ** If we're running with the framework from the build directory we must
        ** be running the interpreter in the build directory, so we use the
        ** build-directory-specific logic to find Lib and such.
        */
        PyStatus status;
        size_t len;
        wchar_t* wbuf = Py_DecodeLocale(modPath, &len);
        if (wbuf == NULL) {
            return DECODE_LOCALE_ERR("framework location", len);
        }

        if (safe_wcscpy(calculate->argv0_path, wbuf, argv0_path_len) < 0) {
            return PATHLEN_ERR();
        }
        reduce(calculate->argv0_path);
        status = joinpath(calculate->argv0_path, calculate->lib_python, argv0_path_len);
        if (_PyStatus_EXCEPTION(status)) {
            PyMem_RawFree(wbuf);
            return status;
        }
        status = joinpath(calculate->argv0_path, LANDMARK, argv0_path_len);
        if (_PyStatus_EXCEPTION(status)) {
            PyMem_RawFree(wbuf);
            return status;
        }
        if (!ismodule(calculate->argv0_path,
                      Py_ARRAY_LENGTH(calculate->argv0_path))) {
            /* We are in the build directory so use the name of the
               executable - we know that the absolute path is passed */
            if (safe_wcscpy(calculate->argv0_path, program_full_path,
                            argv0_path_len) < 0) {
                return PATHLEN_ERR();
            }
        }
        else {
            /* Use the location of the library as the program_full_path */
            if (safe_wcscpy(calculate->argv0_path, wbuf, argv0_path_len) < 0) {
                return PATHLEN_ERR();
            }
        }
        PyMem_RawFree(wbuf);
    }
#endif

#if HAVE_READLINK
    wchar_t tmpbuffer[MAXPATHLEN + 1];
    const size_t buflen = Py_ARRAY_LENGTH(tmpbuffer);
    int linklen = _Py_wreadlink(program_full_path, tmpbuffer, buflen);
    while (linklen != -1) {
        if (tmpbuffer[0] == SEP) {
            /* tmpbuffer should never be longer than MAXPATHLEN,
               but extra check does not hurt */
            if (safe_wcscpy(calculate->argv0_path, tmpbuffer, argv0_path_len) < 0) {
                return PATHLEN_ERR();
            }
        }
        else {
            /* Interpret relative to program_full_path */
            PyStatus status;
            reduce(calculate->argv0_path);
            status = joinpath(calculate->argv0_path, tmpbuffer, argv0_path_len);
            if (_PyStatus_EXCEPTION(status)) {
                return status;
            }
        }
        linklen = _Py_wreadlink(calculate->argv0_path, tmpbuffer, buflen);
    }
#endif /* HAVE_READLINK */

    reduce(calculate->argv0_path);
    /* At this point, argv0_path is guaranteed to be less than
       MAXPATHLEN bytes long. */
    return _PyStatus_OK();
}


/* Search for an "pyvenv.cfg" environment configuration file, first in the
   executable's directory and then in the parent directory.
   If found, open it for use when searching for prefixes.
*/
static PyStatus
calculate_read_pyenv(PyCalculatePath *calculate)
{
    PyStatus status;
    wchar_t tmpbuffer[MAXPATHLEN+1];
    const size_t buflen = Py_ARRAY_LENGTH(tmpbuffer);
    wchar_t *env_cfg = L"pyvenv.cfg";
    FILE *env_file;

    if (safe_wcscpy(tmpbuffer, calculate->argv0_path, buflen) < 0) {
        return PATHLEN_ERR();
    }

    status = joinpath(tmpbuffer, env_cfg, buflen);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }
    env_file = _Py_wfopen(tmpbuffer, L"r");
    if (env_file == NULL) {
        errno = 0;

        reduce(tmpbuffer);
        reduce(tmpbuffer);
        status = joinpath(tmpbuffer, env_cfg, buflen);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }

        env_file = _Py_wfopen(tmpbuffer, L"r");
        if (env_file == NULL) {
            errno = 0;
        }
    }

    if (env_file == NULL) {
        return _PyStatus_OK();
    }

    /* Look for a 'home' variable and set argv0_path to it, if found */
    if (_Py_FindEnvConfigValue(env_file, L"home", tmpbuffer, buflen)) {
        if (safe_wcscpy(calculate->argv0_path, tmpbuffer,
                        Py_ARRAY_LENGTH(calculate->argv0_path)) < 0) {
            return PATHLEN_ERR();
        }
    }
    fclose(env_file);
    return _PyStatus_OK();
}


static PyStatus
calculate_zip_path(PyCalculatePath *calculate, const wchar_t *prefix)
{
    PyStatus status;
    const size_t zip_path_len = Py_ARRAY_LENGTH(calculate->zip_path);
    if (safe_wcscpy(calculate->zip_path, prefix, zip_path_len) < 0) {
        return PATHLEN_ERR();
    }

    if (calculate->prefix_found > 0) {
        /* Use the reduced prefix returned by Py_GetPrefix() */
        reduce(calculate->zip_path);
        reduce(calculate->zip_path);
    }
    else {
        if (safe_wcscpy(calculate->zip_path, calculate->prefix, zip_path_len) < 0) {
            return PATHLEN_ERR();
        }
    }
    status = joinpath(calculate->zip_path, L"lib/python00.zip", zip_path_len);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    /* Replace "00" with version */
    size_t bufsz = wcslen(calculate->zip_path);
    calculate->zip_path[bufsz - 6] = VERSION[0];
    calculate->zip_path[bufsz - 5] = VERSION[2];
    return _PyStatus_OK();
}


static PyStatus
calculate_module_search_path(const PyConfig *config,
                             PyCalculatePath *calculate,
                             const wchar_t *prefix, const wchar_t *exec_prefix,
                             _PyPathConfig *pathconfig)
{
    /* Calculate size of return buffer */
    size_t bufsz = 0;
    if (config->pythonpath_env != NULL) {
        bufsz += wcslen(config->pythonpath_env) + 1;
    }

    wchar_t *defpath = calculate->pythonpath;
    size_t prefixsz = wcslen(prefix) + 1;
    while (1) {
        wchar_t *delim = wcschr(defpath, DELIM);

        if (defpath[0] != SEP) {
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
    bufsz += wcslen(exec_prefix) + 1;

    /* Allocate the buffer */
    wchar_t *buf = PyMem_RawMalloc(bufsz * sizeof(wchar_t));
    if (buf == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    buf[0] = '\0';

    /* Run-time value of $PYTHONPATH goes first */
    if (config->pythonpath_env) {
        wcscpy(buf, config->pythonpath_env);
        wcscat(buf, delimiter);
    }

    /* Next is the default zip path */
    wcscat(buf, calculate->zip_path);
    wcscat(buf, delimiter);

    /* Next goes merge of compile-time $PYTHONPATH with
     * dynamically located prefix.
     */
    defpath = calculate->pythonpath;
    while (1) {
        wchar_t *delim = wcschr(defpath, DELIM);

        if (defpath[0] != SEP) {
            wcscat(buf, prefix);
            if (prefixsz >= 2 && prefix[prefixsz - 2] != SEP &&
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
    wcscat(buf, exec_prefix);

    pathconfig->module_search_path = buf;
    return _PyStatus_OK();
}


static PyStatus
calculate_init(PyCalculatePath *calculate,
               const PyConfig *config)
{
    size_t len;
    const char *path = getenv("PATH");
    if (path) {
        calculate->path_env = Py_DecodeLocale(path, &len);
        if (!calculate->path_env) {
            return DECODE_LOCALE_ERR("PATH environment variable", len);
        }
    }

    calculate->pythonpath = Py_DecodeLocale(PYTHONPATH, &len);
    if (!calculate->pythonpath) {
        return DECODE_LOCALE_ERR("PYTHONPATH define", len);
    }
    calculate->prefix = Py_DecodeLocale(PREFIX, &len);
    if (!calculate->prefix) {
        return DECODE_LOCALE_ERR("PREFIX define", len);
    }
    calculate->exec_prefix = Py_DecodeLocale(EXEC_PREFIX, &len);
    if (!calculate->prefix) {
        return DECODE_LOCALE_ERR("EXEC_PREFIX define", len);
    }
    calculate->lib_python = Py_DecodeLocale("lib/python" VERSION, &len);
    if (!calculate->lib_python) {
        return DECODE_LOCALE_ERR("EXEC_PREFIX define", len);
    }
    return _PyStatus_OK();
}


static void
calculate_free(PyCalculatePath *calculate)
{
    PyMem_RawFree(calculate->pythonpath);
    PyMem_RawFree(calculate->prefix);
    PyMem_RawFree(calculate->exec_prefix);
    PyMem_RawFree(calculate->lib_python);
    PyMem_RawFree(calculate->path_env);
}


static PyStatus
calculate_path_impl(const PyConfig *config,
                    PyCalculatePath *calculate, _PyPathConfig *pathconfig)
{
    PyStatus status;

    status = calculate_program_full_path(config, calculate, pathconfig);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = calculate_argv0_path(calculate, pathconfig->program_full_path);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = calculate_read_pyenv(calculate);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    wchar_t prefix[MAXPATHLEN+1];
    memset(prefix, 0, sizeof(prefix));
    status = calculate_prefix(config, calculate,
                           prefix, Py_ARRAY_LENGTH(prefix));
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = calculate_zip_path(calculate, prefix);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    wchar_t exec_prefix[MAXPATHLEN+1];
    memset(exec_prefix, 0, sizeof(exec_prefix));
    status = calculate_exec_prefix(config, calculate,
                                exec_prefix, Py_ARRAY_LENGTH(exec_prefix));
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if ((!calculate->prefix_found || !calculate->exec_prefix_found) &&
        config->pathconfig_warnings)
    {
        fprintf(stderr,
                "Consider setting $PYTHONHOME to <prefix>[:<exec_prefix>]\n");
    }

    status = calculate_module_search_path(config, calculate,
                                       prefix, exec_prefix, pathconfig);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = calculate_reduce_prefix(calculate, prefix, Py_ARRAY_LENGTH(prefix));
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    pathconfig->prefix = _PyMem_RawWcsdup(prefix);
    if (pathconfig->prefix == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    status = calculate_reduce_exec_prefix(calculate,
                                       exec_prefix, Py_ARRAY_LENGTH(exec_prefix));
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    pathconfig->exec_prefix = _PyMem_RawWcsdup(exec_prefix);
    if (pathconfig->exec_prefix == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    return _PyStatus_OK();
}


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

    status = calculate_path_impl(config, &calculate, pathconfig);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    status = _PyStatus_OK();

done:
    calculate_free(&calculate);
    return status;
}

#ifdef __cplusplus
}
#endif
