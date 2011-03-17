/* Return the initial module search path. */

#include "Python.h"
#include "osdefs.h"

#include <sys/types.h>
#include <string.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
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


#ifndef VERSION
#define VERSION "2.1"
#endif

#ifndef VPATH
#define VPATH "."
#endif

#ifndef PREFIX
#  ifdef __VMS
#    define PREFIX ""
#  else
#    define PREFIX "/usr/local"
#  endif
#endif

#ifndef EXEC_PREFIX
#define EXEC_PREFIX PREFIX
#endif

#ifndef PYTHONPATH
#define PYTHONPATH PREFIX "/lib/python" VERSION ":" \
              EXEC_PREFIX "/lib/python" VERSION "/lib-dynload"
#endif

#ifndef LANDMARK
#define LANDMARK L"os.py"
#endif

static wchar_t prefix[MAXPATHLEN+1];
static wchar_t exec_prefix[MAXPATHLEN+1];
static wchar_t progpath[MAXPATHLEN+1];
static wchar_t *module_search_path = NULL;
static int module_search_path_malloced = 0;
static wchar_t *lib_python = L"lib/python" VERSION;

static void
reduce(wchar_t *dir)
{
    size_t i = wcslen(dir);
    while (i > 0 && dir[i] != SEP)
        --i;
    dir[i] = '\0';
}

static int
isfile(wchar_t *filename)          /* Is file, not directory */
{
    struct stat buf;
    if (_Py_wstat(filename, &buf) != 0)
        return 0;
    if (!S_ISREG(buf.st_mode))
        return 0;
    return 1;
}


static int
ismodule(wchar_t *filename)        /* Is module -- check for .pyc/.pyo too */
{
    if (isfile(filename))
        return 1;

    /* Check for the compiled version of prefix. */
    if (wcslen(filename) < MAXPATHLEN) {
        wcscat(filename, Py_OptimizeFlag ? L"o" : L"c");
        if (isfile(filename))
            return 1;
    }
    return 0;
}


static int
isxfile(wchar_t *filename)         /* Is executable file */
{
    struct stat buf;
    if (_Py_wstat(filename, &buf) != 0)
        return 0;
    if (!S_ISREG(buf.st_mode))
        return 0;
    if ((buf.st_mode & 0111) == 0)
        return 0;
    return 1;
}


static int
isdir(wchar_t *filename)                   /* Is directory */
{
    struct stat buf;
    if (_Py_wstat(filename, &buf) != 0)
        return 0;
    if (!S_ISDIR(buf.st_mode))
        return 0;
    return 1;
}


/* Add a path component, by appending stuff to buffer.
   buffer must have at least MAXPATHLEN + 1 bytes allocated, and contain a
   NUL-terminated string with no more than MAXPATHLEN characters (not counting
   the trailing NUL).  It's a fatal error if it contains a string longer than
   that (callers must be careful!).  If these requirements are met, it's
   guaranteed that buffer will still be a NUL-terminated string with no more
   than MAXPATHLEN characters at exit.  If stuff is too long, only as much of
   stuff as fits will be appended.
*/
static void
joinpath(wchar_t *buffer, wchar_t *stuff)
{
    size_t n, k;
    if (stuff[0] == SEP)
        n = 0;
    else {
        n = wcslen(buffer);
        if (n > 0 && buffer[n-1] != SEP && n < MAXPATHLEN)
            buffer[n++] = SEP;
    }
    if (n > MAXPATHLEN)
        Py_FatalError("buffer overflow in getpath.c's joinpath()");
    k = wcslen(stuff);
    if (n + k > MAXPATHLEN)
        k = MAXPATHLEN - n;
    wcsncpy(buffer+n, stuff, k);
    buffer[n+k] = '\0';
}

/* copy_absolute requires that path be allocated at least
   MAXPATHLEN + 1 bytes and that p be no more than MAXPATHLEN bytes. */
static void
copy_absolute(wchar_t *path, wchar_t *p, size_t pathlen)
{
    if (p[0] == SEP)
        wcscpy(path, p);
    else {
        if (!_Py_wgetcwd(path, pathlen)) {
            /* unable to get the current directory */
            wcscpy(path, p);
            return;
        }
        if (p[0] == '.' && p[1] == SEP)
            p += 2;
        joinpath(path, p);
    }
}

/* absolutize() requires that path be allocated at least MAXPATHLEN+1 bytes. */
static void
absolutize(wchar_t *path)
{
    wchar_t buffer[MAXPATHLEN+1];

    if (path[0] == SEP)
        return;
    copy_absolute(buffer, path, MAXPATHLEN+1);
    wcscpy(path, buffer);
}

/* search_for_prefix requires that argv0_path be no more than MAXPATHLEN
   bytes long.
*/
static int
search_for_prefix(wchar_t *argv0_path, wchar_t *home, wchar_t *_prefix)
{
    size_t n;
    wchar_t *vpath;

    /* If PYTHONHOME is set, we believe it unconditionally */
    if (home) {
        wchar_t *delim;
        wcsncpy(prefix, home, MAXPATHLEN);
        delim = wcschr(prefix, DELIM);
        if (delim)
            *delim = L'\0';
        joinpath(prefix, lib_python);
        joinpath(prefix, LANDMARK);
        return 1;
    }

    /* Check to see if argv[0] is in the build directory */
    wcscpy(prefix, argv0_path);
    joinpath(prefix, L"Modules/Setup");
    if (isfile(prefix)) {
        /* Check VPATH to see if argv0_path is in the build directory. */
        vpath = _Py_char2wchar(VPATH, NULL);
        if (vpath != NULL) {
            wcscpy(prefix, argv0_path);
            joinpath(prefix, vpath);
            PyMem_Free(vpath);
            joinpath(prefix, L"Lib");
            joinpath(prefix, LANDMARK);
            if (ismodule(prefix))
                return -1;
        }
    }

    /* Search from argv0_path, until root is found */
    copy_absolute(prefix, argv0_path, MAXPATHLEN+1);
    do {
        n = wcslen(prefix);
        joinpath(prefix, lib_python);
        joinpath(prefix, LANDMARK);
        if (ismodule(prefix))
            return 1;
        prefix[n] = L'\0';
        reduce(prefix);
    } while (prefix[0]);

    /* Look at configure's PREFIX */
    wcsncpy(prefix, _prefix, MAXPATHLEN);
    joinpath(prefix, lib_python);
    joinpath(prefix, LANDMARK);
    if (ismodule(prefix))
        return 1;

    /* Fail */
    return 0;
}


/* search_for_exec_prefix requires that argv0_path be no more than
   MAXPATHLEN bytes long.
*/
static int
search_for_exec_prefix(wchar_t *argv0_path, wchar_t *home, wchar_t *_exec_prefix)
{
    size_t n;

    /* If PYTHONHOME is set, we believe it unconditionally */
    if (home) {
        wchar_t *delim;
        delim = wcschr(home, DELIM);
        if (delim)
            wcsncpy(exec_prefix, delim+1, MAXPATHLEN);
        else
            wcsncpy(exec_prefix, home, MAXPATHLEN);
        joinpath(exec_prefix, lib_python);
        joinpath(exec_prefix, L"lib-dynload");
        return 1;
    }

    /* Check to see if argv[0] is in the build directory. "pybuilddir.txt"
       is written by setup.py and contains the relative path to the location
       of shared library modules. */
    wcscpy(exec_prefix, argv0_path);
    joinpath(exec_prefix, L"pybuilddir.txt");
    if (isfile(exec_prefix)) {
        FILE *f = _Py_wfopen(exec_prefix, L"rb");
        if (f == NULL)
            errno = 0;
        else {
            char buf[MAXPATHLEN+1];
            PyObject *decoded;
            wchar_t rel_builddir_path[MAXPATHLEN+1];
            n = fread(buf, 1, MAXPATHLEN, f);
            buf[n] = '\0';
            fclose(f);
            decoded = PyUnicode_DecodeUTF8(buf, n, "surrogateescape");
            if (decoded != NULL) {
                Py_ssize_t k;
                k = PyUnicode_AsWideChar(decoded,
                                         rel_builddir_path, MAXPATHLEN);
                Py_DECREF(decoded);
                if (k >= 0) {
                    rel_builddir_path[k] = L'\0';
                    wcscpy(exec_prefix, argv0_path);
                    joinpath(exec_prefix, rel_builddir_path);
                    return -1;
                }
            }
        }
    }

    /* Search from argv0_path, until root is found */
    copy_absolute(exec_prefix, argv0_path, MAXPATHLEN+1);
    do {
        n = wcslen(exec_prefix);
        joinpath(exec_prefix, lib_python);
        joinpath(exec_prefix, L"lib-dynload");
        if (isdir(exec_prefix))
            return 1;
        exec_prefix[n] = L'\0';
        reduce(exec_prefix);
    } while (exec_prefix[0]);

    /* Look at configure's EXEC_PREFIX */
    wcsncpy(exec_prefix, _exec_prefix, MAXPATHLEN);
    joinpath(exec_prefix, lib_python);
    joinpath(exec_prefix, L"lib-dynload");
    if (isdir(exec_prefix))
        return 1;

    /* Fail */
    return 0;
}

static void
calculate_path(void)
{
    extern wchar_t *Py_GetProgramName(void);

    static wchar_t delimiter[2] = {DELIM, '\0'};
    static wchar_t separator[2] = {SEP, '\0'};
    char *_rtpypath = Py_GETENV("PYTHONPATH"); /* XXX use wide version on Windows */
    wchar_t rtpypath[MAXPATHLEN+1];
    wchar_t *home = Py_GetPythonHome();
    char *_path = getenv("PATH");
    wchar_t *path_buffer = NULL;
    wchar_t *path = NULL;
    wchar_t *prog = Py_GetProgramName();
    wchar_t argv0_path[MAXPATHLEN+1];
    wchar_t zip_path[MAXPATHLEN+1];
    int pfound, efound; /* 1 if found; -1 if found build directory */
    wchar_t *buf;
    size_t bufsz;
    size_t prefixsz;
    wchar_t *defpath;
#ifdef WITH_NEXT_FRAMEWORK
    NSModule pythonModule;
#endif
#ifdef __APPLE__
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
    uint32_t nsexeclength = MAXPATHLEN;
#else
    unsigned long nsexeclength = MAXPATHLEN;
#endif
    char execpath[MAXPATHLEN+1];
#endif
    wchar_t *_pythonpath, *_prefix, *_exec_prefix;

    _pythonpath = _Py_char2wchar(PYTHONPATH, NULL);
    _prefix = _Py_char2wchar(PREFIX, NULL);
    _exec_prefix = _Py_char2wchar(EXEC_PREFIX, NULL);

    if (!_pythonpath || !_prefix || !_exec_prefix) {
        Py_FatalError(
            "Unable to decode path variables in getpath.c: "
            "memory error");
    }

    if (_path) {
        path_buffer = _Py_char2wchar(_path, NULL);
        path = path_buffer;
    }

    /* If there is no slash in the argv0 path, then we have to
     * assume python is on the user's $PATH, since there's no
     * other way to find a directory to start the search from.  If
     * $PATH isn't exported, you lose.
     */
    if (wcschr(prog, SEP))
        wcsncpy(progpath, prog, MAXPATHLEN);
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
    else if(0 == _NSGetExecutablePath(execpath, &nsexeclength) && execpath[0] == SEP) {
        size_t r = mbstowcs(progpath, execpath, MAXPATHLEN+1);
        if (r == (size_t)-1 || r > MAXPATHLEN) {
            /* Could not convert execpath, or it's too long. */
            progpath[0] = '\0';
        }
    }
#endif /* __APPLE__ */
    else if (path) {
        while (1) {
            wchar_t *delim = wcschr(path, DELIM);

            if (delim) {
                size_t len = delim - path;
                if (len > MAXPATHLEN)
                    len = MAXPATHLEN;
                wcsncpy(progpath, path, len);
                *(progpath + len) = '\0';
            }
            else
                wcsncpy(progpath, path, MAXPATHLEN);

            joinpath(progpath, prog);
            if (isxfile(progpath))
                break;

            if (!delim) {
                progpath[0] = L'\0';
                break;
            }
            path = delim + 1;
        }
    }
    else
        progpath[0] = '\0';
    if (path_buffer != NULL)
        PyMem_Free(path_buffer);
    if (progpath[0] != SEP && progpath[0] != '\0')
        absolutize(progpath);
    wcsncpy(argv0_path, progpath, MAXPATHLEN);
    argv0_path[MAXPATHLEN] = '\0';

#ifdef WITH_NEXT_FRAMEWORK
    /* On Mac OS X we have a special case if we're running from a framework.
    ** This is because the python home should be set relative to the library,
    ** which is in the framework, not relative to the executable, which may
    ** be outside of the framework. Except when we're in the build directory...
    */
    pythonModule = NSModuleForSymbol(NSLookupAndBindSymbol("_Py_Initialize"));
    /* Use dylib functions to find out where the framework was loaded from */
    buf = (wchar_t *)NSLibraryNameForModule(pythonModule);
    if (buf != NULL) {
        /* We're in a framework. */
        /* See if we might be in the build directory. The framework in the
        ** build directory is incomplete, it only has the .dylib and a few
        ** needed symlinks, it doesn't have the Lib directories and such.
        ** If we're running with the framework from the build directory we must
        ** be running the interpreter in the build directory, so we use the
        ** build-directory-specific logic to find Lib and such.
        */
        wcsncpy(argv0_path, buf, MAXPATHLEN);
        reduce(argv0_path);
        joinpath(argv0_path, lib_python);
        joinpath(argv0_path, LANDMARK);
        if (!ismodule(argv0_path)) {
            /* We are in the build directory so use the name of the
               executable - we know that the absolute path is passed */
            wcsncpy(argv0_path, progpath, MAXPATHLEN);
        }
        else {
            /* Use the location of the library as the progpath */
            wcsncpy(argv0_path, buf, MAXPATHLEN);
        }
    }
#endif

#if HAVE_READLINK
    {
        wchar_t tmpbuffer[MAXPATHLEN+1];
        int linklen = _Py_wreadlink(progpath, tmpbuffer, MAXPATHLEN);
        while (linklen != -1) {
            if (tmpbuffer[0] == SEP)
                /* tmpbuffer should never be longer than MAXPATHLEN,
                   but extra check does not hurt */
                wcsncpy(argv0_path, tmpbuffer, MAXPATHLEN);
            else {
                /* Interpret relative to progpath */
                reduce(argv0_path);
                joinpath(argv0_path, tmpbuffer);
            }
            linklen = _Py_wreadlink(argv0_path, tmpbuffer, MAXPATHLEN);
        }
    }
#endif /* HAVE_READLINK */

    reduce(argv0_path);
    /* At this point, argv0_path is guaranteed to be less than
       MAXPATHLEN bytes long.
    */

    if (!(pfound = search_for_prefix(argv0_path, home, _prefix))) {
        if (!Py_FrozenFlag)
            fprintf(stderr,
                "Could not find platform independent libraries <prefix>\n");
        wcsncpy(prefix, _prefix, MAXPATHLEN);
        joinpath(prefix, lib_python);
    }
    else
        reduce(prefix);

    wcsncpy(zip_path, prefix, MAXPATHLEN);
    zip_path[MAXPATHLEN] = L'\0';
    if (pfound > 0) { /* Use the reduced prefix returned by Py_GetPrefix() */
        reduce(zip_path);
        reduce(zip_path);
    }
    else
        wcsncpy(zip_path, _prefix, MAXPATHLEN);
    joinpath(zip_path, L"lib/python00.zip");
    bufsz = wcslen(zip_path);   /* Replace "00" with version */
    zip_path[bufsz - 6] = VERSION[0];
    zip_path[bufsz - 5] = VERSION[2];

    if (!(efound = search_for_exec_prefix(argv0_path, home, _exec_prefix))) {
        if (!Py_FrozenFlag)
            fprintf(stderr,
                "Could not find platform dependent libraries <exec_prefix>\n");
        wcsncpy(exec_prefix, _exec_prefix, MAXPATHLEN);
        joinpath(exec_prefix, L"lib/lib-dynload");
    }
    /* If we found EXEC_PREFIX do *not* reduce it!  (Yet.) */

    if ((!pfound || !efound) && !Py_FrozenFlag)
        fprintf(stderr,
                "Consider setting $PYTHONHOME to <prefix>[:<exec_prefix>]\n");

    /* Calculate size of return buffer.
     */
    bufsz = 0;

    if (_rtpypath) {
        size_t s = mbstowcs(rtpypath, _rtpypath, sizeof(rtpypath)/sizeof(wchar_t));
        if (s == (size_t)-1 || s >=sizeof(rtpypath))
            /* XXX deal with errors more gracefully */
            _rtpypath = NULL;
        if (_rtpypath)
            bufsz += wcslen(rtpypath) + 1;
    }

    defpath = _pythonpath;
    prefixsz = wcslen(prefix) + 1;
    while (1) {
        wchar_t *delim = wcschr(defpath, DELIM);

        if (defpath[0] != SEP)
            /* Paths are relative to prefix */
            bufsz += prefixsz;

        if (delim)
            bufsz += delim - defpath + 1;
        else {
            bufsz += wcslen(defpath) + 1;
            break;
        }
        defpath = delim + 1;
    }

    bufsz += wcslen(zip_path) + 1;
    bufsz += wcslen(exec_prefix) + 1;

    buf = (wchar_t *)PyMem_Malloc(bufsz*sizeof(wchar_t));

    if (buf == NULL) {
        /* We can't exit, so print a warning and limp along */
        fprintf(stderr, "Not enough memory for dynamic PYTHONPATH.\n");
        fprintf(stderr, "Using default static PYTHONPATH.\n");
        module_search_path = L"" PYTHONPATH;
    }
    else {
        /* Run-time value of $PYTHONPATH goes first */
        if (_rtpypath) {
            wcscpy(buf, rtpypath);
            wcscat(buf, delimiter);
        }
        else
            buf[0] = '\0';

        /* Next is the default zip path */
        wcscat(buf, zip_path);
        wcscat(buf, delimiter);

        /* Next goes merge of compile-time $PYTHONPATH with
         * dynamically located prefix.
         */
        defpath = _pythonpath;
        while (1) {
            wchar_t *delim = wcschr(defpath, DELIM);

            if (defpath[0] != SEP) {
                wcscat(buf, prefix);
                wcscat(buf, separator);
            }

            if (delim) {
                size_t len = delim - defpath + 1;
                size_t end = wcslen(buf) + len;
                wcsncat(buf, defpath, len);
                *(buf + end) = '\0';
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

        /* And publish the results */
        module_search_path = buf;
        module_search_path_malloced = 1;
    }

    /* Reduce prefix and exec_prefix to their essence,
     * e.g. /usr/local/lib/python1.5 is reduced to /usr/local.
     * If we're loading relative to the build directory,
     * return the compiled-in defaults instead.
     */
    if (pfound > 0) {
        reduce(prefix);
        reduce(prefix);
        /* The prefix is the root directory, but reduce() chopped
         * off the "/". */
        if (!prefix[0])
                wcscpy(prefix, separator);
    }
    else
        wcsncpy(prefix, _prefix, MAXPATHLEN);

    if (efound > 0) {
        reduce(exec_prefix);
        reduce(exec_prefix);
        reduce(exec_prefix);
        if (!exec_prefix[0])
                wcscpy(exec_prefix, separator);
    }
    else
        wcsncpy(exec_prefix, _exec_prefix, MAXPATHLEN);

    PyMem_Free(_pythonpath);
    PyMem_Free(_prefix);
    PyMem_Free(_exec_prefix);
}


/* External interface */
void
Py_SetPath(const wchar_t *path)
{
    if (module_search_path != NULL) {
        if (module_search_path_malloced)
            PyMem_Free(module_search_path);
        module_search_path = NULL;
        module_search_path_malloced = 0;
    }
    if (path != NULL) {
        extern wchar_t *Py_GetProgramName(void);
        wchar_t *prog = Py_GetProgramName();
        wcsncpy(progpath, prog, MAXPATHLEN);
        exec_prefix[0] = prefix[0] = L'\0';
        module_search_path = PyMem_Malloc((wcslen(path) + 1) * sizeof(wchar_t));
        module_search_path_malloced = 1;
        if (module_search_path != NULL)
            wcscpy(module_search_path, path);
    }
}

wchar_t *
Py_GetPath(void)
{
    if (!module_search_path)
        calculate_path();
    return module_search_path;
}

wchar_t *
Py_GetPrefix(void)
{
    if (!module_search_path)
        calculate_path();
    return prefix;
}

wchar_t *
Py_GetExecPrefix(void)
{
    if (!module_search_path)
        calculate_path();
    return exec_prefix;
}

wchar_t *
Py_GetProgramFullPath(void)
{
    if (!module_search_path)
        calculate_path();
    return progpath;
}


#ifdef __cplusplus
}
#endif

