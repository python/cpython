
/* Return the initial module search path. */
/* This version used by OS/2+EMX */

/* ----------------------------------------------------------------
   PATH RULES FOR OS/2+EMX:
   This describes how sys.path is formed on OS/2+EMX.  It describes the
   functionality, not the implementation (ie, the order in which these
   are actually fetched is different)

   * Python always adds an empty entry at the start, which corresponds
     to the current directory.

   * If the PYTHONPATH env. var. exists, its entries are added next.

   * We attempt to locate the "Python Home" - if the PYTHONHOME env var
     is set, we believe it.  Otherwise, we use the path of our host .EXE's
     to try and locate our "landmark" (lib\\os.py) and deduce our home.
     - If we DO have a Python Home: The relevant sub-directories (Lib,
       plat-win, lib-tk, etc) are based on the Python Home
     - If we DO NOT have a Python Home, the core Python Path is
       loaded from the registry.  This is the main PythonPath key,
       and both HKLM and HKCU are combined to form the path)

   * Iff - we can not locate the Python Home, and have not had a PYTHONPATH
     specified (ie, we have _nothing_ we can assume is a good path), a
     default path with relative entries is used (eg. .\Lib;.\plat-win, etc)


  The end result of all this is:
  * When running python.exe, or any other .exe in the main Python directory
    (either an installed version, or directly from the PCbuild directory),
    the core path is deduced.

  * When Python is hosted in another exe (different directory, embedded via
    COM, etc), the Python Home will not be deduced, so the core path from
    the registry is used.  Other "application paths "in the registry are
    always read.

  * If Python can't find its home and there is no registry (eg, frozen
    exe, some very strange installation setup) you get a path with
    some default, but relative, paths.

   ---------------------------------------------------------------- */


#include "Python.h"
#include "osdefs.h"

#ifndef PYOS_OS2
#error This file only compilable on OS/2
#endif

#define INCL_DOS
#include <os2.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

/* Search in some common locations for the associated Python libraries.
 *
 * Py_GetPath() tries to return a sensible Python module search path.
 *
 * The approach is an adaptation for Windows of the strategy used in
 * ../Modules/getpath.c; it uses the Windows Registry as one of its
 * information sources.
 */

#ifndef LANDMARK
#if defined(PYCC_GCC)
#define LANDMARK "lib/os.py"
#else
#define LANDMARK "lib\\os.py"
#endif
#endif

static char prefix[MAXPATHLEN+1];
static char progpath[MAXPATHLEN+1];
static char *module_search_path = NULL;


static int
is_sep(char ch) /* determine if "ch" is a separator character */
{
#ifdef ALTSEP
    return ch == SEP || ch == ALTSEP;
#else
    return ch == SEP;
#endif
}

/* assumes 'dir' null terminated in bounds.
 * Never writes beyond existing terminator.
 */
static void
reduce(char *dir)
{
    size_t i = strlen(dir);
    while (i > 0 && !is_sep(dir[i]))
        --i;
    dir[i] = '\0';
}

static int
exists(char *filename)
{
    struct stat buf;
    return stat(filename, &buf) == 0;
}

/* Is module  (check for .pyc/.pyo too)
 * Assumes 'filename' MAXPATHLEN+1 bytes long -
 * may extend 'filename' by one character.
 */
static int
ismodule(char *filename)
{
    if (exists(filename))
        return 1;

    /* Check for the compiled version of prefix. */
    if (strlen(filename) < MAXPATHLEN) {
        strcat(filename, Py_OptimizeFlag ? "o" : "c");
        if (exists(filename))
            return 1;
    }
    return 0;
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
join(char *buffer, char *stuff)
{
    size_t n, k;
    if (is_sep(stuff[0]))
        n = 0;
    else {
        n = strlen(buffer);
        if (n > 0 && !is_sep(buffer[n-1]) && n < MAXPATHLEN)
            buffer[n++] = SEP;
    }
    if (n > MAXPATHLEN)
        Py_FatalError("buffer overflow in getpathp.c's joinpath()");
    k = strlen(stuff);
    if (n + k > MAXPATHLEN)
        k = MAXPATHLEN - n;
    strncpy(buffer+n, stuff, k);
    buffer[n+k] = '\0';
}

/* gotlandmark only called by search_for_prefix, which ensures
 * 'prefix' is null terminated in bounds.  join() ensures
 * 'landmark' can not overflow prefix if too long.
 */
static int
gotlandmark(char *landmark)
{
    int n, ok;

    n = strlen(prefix);
    join(prefix, landmark);
    ok = ismodule(prefix);
    prefix[n] = '\0';
    return ok;
}

/* assumes argv0_path is MAXPATHLEN+1 bytes long, already \0 term'd.
 * assumption provided by only caller, calculate_path()
 */
static int
search_for_prefix(char *argv0_path, char *landmark)
{
    /* Search from argv0_path, until landmark is found */
    strcpy(prefix, argv0_path);
    do {
        if (gotlandmark(landmark))
            return 1;
        reduce(prefix);
    } while (prefix[0]);
    return 0;
}


static void
get_progpath(void)
{
    extern char *Py_GetProgramName(void);
    char *path = getenv("PATH");
    char *prog = Py_GetProgramName();

    PPIB pib;
    if ((DosGetInfoBlocks(NULL, &pib) == 0) &&
        (DosQueryModuleName(pib->pib_hmte, sizeof(progpath), progpath) == 0))
        return;

    if (prog == NULL || *prog == '\0')
        prog = "python";

    /* If there is no slash in the argv0 path, then we have to
     * assume python is on the user's $PATH, since there's no
     * other way to find a directory to start the search from.  If
     * $PATH isn't exported, you lose.
     */
#ifdef ALTSEP
    if (strchr(prog, SEP) || strchr(prog, ALTSEP))
#else
    if (strchr(prog, SEP))
#endif
        strncpy(progpath, prog, MAXPATHLEN);
    else if (path) {
        while (1) {
            char *delim = strchr(path, DELIM);

            if (delim) {
                size_t len = delim - path;
                /* ensure we can't overwrite buffer */
#if !defined(PYCC_GCC)
                len = min(MAXPATHLEN,len);
#else
                len = MAXPATHLEN < len ? MAXPATHLEN : len;
#endif
                strncpy(progpath, path, len);
                *(progpath + len) = '\0';
            }
            else
                strncpy(progpath, path, MAXPATHLEN);

            /* join() is safe for MAXPATHLEN+1 size buffer */
            join(progpath, prog);
            if (exists(progpath))
                break;

            if (!delim) {
                progpath[0] = '\0';
                break;
            }
            path = delim + 1;
        }
    }
    else
        progpath[0] = '\0';
}

static void
calculate_path(void)
{
    char argv0_path[MAXPATHLEN+1];
    char *buf;
    size_t bufsz;
    char *pythonhome = Py_GetPythonHome();
    char *envpath = getenv("PYTHONPATH");
    char zip_path[MAXPATHLEN+1];
    size_t len;

    get_progpath();
    /* progpath guaranteed \0 terminated in MAXPATH+1 bytes. */
    strcpy(argv0_path, progpath);
    reduce(argv0_path);
    if (pythonhome == NULL || *pythonhome == '\0') {
        if (search_for_prefix(argv0_path, LANDMARK))
            pythonhome = prefix;
        else
            pythonhome = NULL;
    }
    else
        strncpy(prefix, pythonhome, MAXPATHLEN);

    if (envpath && *envpath == '\0')
        envpath = NULL;

    /* Calculate zip archive path */
    strncpy(zip_path, progpath, MAXPATHLEN);
    zip_path[MAXPATHLEN] = '\0';
    len = strlen(zip_path);
    if (len > 4) {
        zip_path[len-3] = 'z';  /* change ending to "zip" */
        zip_path[len-2] = 'i';
        zip_path[len-1] = 'p';
    }
    else {
        zip_path[0] = 0;
    }

    /* We need to construct a path from the following parts.
     * (1) the PYTHONPATH environment variable, if set;
     * (2) the zip archive file path;
     * (3) the PYTHONPATH config macro, with the leading "."
     *     of each component replaced with pythonhome, if set;
     * (4) the directory containing the executable (argv0_path).
     * The length calculation calculates #3 first.
     */

    /* Calculate size of return buffer */
    if (pythonhome != NULL) {
        char *p;
        bufsz = 1;
        for (p = PYTHONPATH; *p; p++) {
            if (*p == DELIM)
                bufsz++; /* number of DELIM plus one */
        }
        bufsz *= strlen(pythonhome);
    }
    else
        bufsz = 0;
    bufsz += strlen(PYTHONPATH) + 1;
    bufsz += strlen(argv0_path) + 1;
    bufsz += strlen(zip_path) + 1;
    if (envpath != NULL)
        bufsz += strlen(envpath) + 1;

    module_search_path = buf = malloc(bufsz);
    if (buf == NULL) {
        /* We can't exit, so print a warning and limp along */
        fprintf(stderr, "Can't malloc dynamic PYTHONPATH.\n");
        if (envpath) {
            fprintf(stderr, "Using environment $PYTHONPATH.\n");
            module_search_path = envpath;
        }
        else {
            fprintf(stderr, "Using default static path.\n");
            module_search_path = PYTHONPATH;
        }
        return;
    }

    if (envpath) {
        strcpy(buf, envpath);
        buf = strchr(buf, '\0');
        *buf++ = DELIM;
    }
    if (zip_path[0]) {
        strcpy(buf, zip_path);
        buf = strchr(buf, '\0');
        *buf++ = DELIM;
    }

    if (pythonhome == NULL) {
        strcpy(buf, PYTHONPATH);
        buf = strchr(buf, '\0');
    }
    else {
        char *p = PYTHONPATH;
        char *q;
        size_t n;
        for (;;) {
            q = strchr(p, DELIM);
            if (q == NULL)
                n = strlen(p);
            else
                n = q-p;
            if (p[0] == '.' && is_sep(p[1])) {
                strcpy(buf, pythonhome);
                buf = strchr(buf, '\0');
                p++;
                n--;
            }
            strncpy(buf, p, n);
            buf += n;
            if (q == NULL)
                break;
            *buf++ = DELIM;
            p = q+1;
        }
    }
    if (argv0_path) {
        *buf++ = DELIM;
        strcpy(buf, argv0_path);
        buf = strchr(buf, '\0');
    }
    *buf = '\0';
}


/* External interface */

char *
Py_GetPath(void)
{
    if (!module_search_path)
        calculate_path();
    return module_search_path;
}

char *
Py_GetPrefix(void)
{
    if (!module_search_path)
        calculate_path();
    return prefix;
}

char *
Py_GetExecPrefix(void)
{
    return Py_GetPrefix();
}

char *
Py_GetProgramFullPath(void)
{
    if (!module_search_path)
        calculate_path();
    return progpath;
}
