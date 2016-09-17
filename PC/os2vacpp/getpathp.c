
/* Return the initial module search path. */
/* Used by DOS, OS/2, Windows 3.1.  Works on NT too. */

#include "Python.h"
#include "osdefs.h"

#ifdef MS_WIN32
#include <windows.h>
extern BOOL PyWin_IsWin32s(void);
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

/* Search in some common locations for the associated Python libraries.
 *
 * Two directories must be found, the platform independent directory
 * (prefix), containing the common .py and .pyc files, and the platform
 * dependent directory (exec_prefix), containing the shared library
 * modules.  Note that prefix and exec_prefix can be the same directory,
 * but for some installations, they are different.
 *
 * Py_GetPath() tries to return a sensible Python module search path.
 *
 * First, we look to see if the executable is in a subdirectory of
 * the Python build directory.  We calculate the full path of the
 * directory containing the executable as progpath.  We work backwards
 * along progpath and look for $dir/Modules/Setup.in, a distinctive
 * landmark.  If found, we use $dir/Lib as $root.  The returned
 * Python path is the compiled #define PYTHONPATH with all the initial
 * "./lib" replaced by $root.
 *
 * Otherwise, if there is a PYTHONPATH environment variable, we return that.
 *
 * Otherwise we try to find $progpath/lib/os.py, and if found, then
 * root is $progpath/lib, and we return Python path as compiled PYTHONPATH
 * with all "./lib" replaced by $root (as above).
 *
 */

#ifndef LANDMARK
#define LANDMARK "lib\\os.py"
#endif

static char prefix[MAXPATHLEN+1];
static char exec_prefix[MAXPATHLEN+1];
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


static void
reduce(char *dir)
{
    int i = strlen(dir);
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
    int n, k;
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


static int
search_for_prefix(char *argv0_path, char *landmark)
{
    int n;

    /* Search from argv0_path, until root is found */
    strcpy(prefix, argv0_path);
    do {
        n = strlen(prefix);
        join(prefix, landmark);
        if (exists(prefix)) {
            prefix[n] = '\0';
            return 1;
        }
        prefix[n] = '\0';
        reduce(prefix);
    } while (prefix[0]);
    return 0;
}

#ifdef MS_WIN32
#include "malloc.h" // for alloca - see comments below!
extern const char *PyWin_DLLVersionString; // a string loaded from the DLL at startup.


/* Load a PYTHONPATH value from the registry.
   Load from either HKEY_LOCAL_MACHINE or HKEY_CURRENT_USER.

   Returns NULL, or a pointer that should be freed.
*/

static char *
getpythonregpath(HKEY keyBase, BOOL bWin32s)
{
    HKEY newKey = 0;
    DWORD nameSize = 0;
    DWORD dataSize = 0;
    DWORD numEntries = 0;
    LONG rc;
    char *retval = NULL;
    char *dataBuf;
    const char keyPrefix[] = "Software\\Python\\PythonCore\\";
    const char keySuffix[] = "\\PythonPath";
    int versionLen;
    char *keyBuf;

    // Tried to use sysget("winver") but here is too early :-(
    versionLen = strlen(PyWin_DLLVersionString);
    // alloca == no free required, but memory only local to fn.
    // also no heap fragmentation!  Am I being silly?
    keyBuf = alloca(sizeof(keyPrefix)-1 + versionLen + sizeof(keySuffix)); // chars only, plus 1 NULL.
    // lots of constants here for the compiler to optimize away :-)
    memcpy(keyBuf, keyPrefix, sizeof(keyPrefix)-1);
    memcpy(keyBuf+sizeof(keyPrefix)-1, PyWin_DLLVersionString, versionLen);
    memcpy(keyBuf+sizeof(keyPrefix)-1+versionLen, keySuffix, sizeof(keySuffix)); // NULL comes with this one!

    rc=RegOpenKey(keyBase,
                  keyBuf,
                  &newKey);
    if (rc==ERROR_SUCCESS) {
        RegQueryInfoKey(newKey, NULL, NULL, NULL, NULL, NULL, NULL,
                        &numEntries, &nameSize, &dataSize, NULL, NULL);
    }
    if (bWin32s && numEntries==0 && dataSize==0) {
        /* must hardcode for Win32s */
        numEntries = 1;
        dataSize = 511;
    }
    if (numEntries) {
        /* Loop over all subkeys. */
        /* Win32s doesnt know how many subkeys, so we do
           it twice */
        char keyBuf[MAX_PATH+1];
        int index = 0;
        int off = 0;
        for(index=0;;index++) {
            long reqdSize = 0;
            DWORD rc = RegEnumKey(newKey,
                                  index, keyBuf, MAX_PATH+1);
            if (rc) break;
            rc = RegQueryValue(newKey, keyBuf, NULL, &reqdSize);
            if (rc) break;
            if (bWin32s && reqdSize==0) reqdSize = 512;
            dataSize += reqdSize + 1; /* 1 for the ";" */
        }
        dataBuf = malloc(dataSize+1);
        if (dataBuf==NULL)
            return NULL; /* pretty serious?  Raise error? */
        /* Now loop over, grabbing the paths.
           Subkeys before main library */
        for(index=0;;index++) {
            int adjust;
            long reqdSize = dataSize;
            DWORD rc = RegEnumKey(newKey,
                                  index, keyBuf,MAX_PATH+1);
            if (rc) break;
            rc = RegQueryValue(newKey,
                               keyBuf, dataBuf+off, &reqdSize);
            if (rc) break;
            if (reqdSize>1) {
                /* If Nothing, or only '\0' copied. */
                adjust = strlen(dataBuf+off);
                dataSize -= adjust;
                off += adjust;
                dataBuf[off++] = ';';
                dataBuf[off] = '\0';
                dataSize--;
            }
        }
        /* Additionally, win32s doesnt work as expected, so
           the specific strlen() is required for 3.1. */
        rc = RegQueryValue(newKey, "", dataBuf+off, &dataSize);
        if (rc==ERROR_SUCCESS) {
            if (strlen(dataBuf)==0)
                free(dataBuf);
            else
                retval = dataBuf; /* caller will free */
        }
        else
            free(dataBuf);
    }

    if (newKey)
        RegCloseKey(newKey);
    return retval;
}
#endif /* MS_WIN32 */

static void
get_progpath(void)
{
    extern char *Py_GetProgramName(void);
    char *path = getenv("PATH");
    char *prog = Py_GetProgramName();

#ifdef MS_WIN32
    if (GetModuleFileName(NULL, progpath, MAXPATHLEN))
        return;
#endif
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
        strcpy(progpath, prog);
    else if (path) {
        while (1) {
            char *delim = strchr(path, DELIM);

            if (delim) {
                int len = delim - path;
                strncpy(progpath, path, len);
                *(progpath + len) = '\0';
            }
            else
                strcpy(progpath, path);

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
    int bufsz;
    char *pythonhome = Py_GetPythonHome();
    char *envpath = Py_GETENV("PYTHONPATH");
#ifdef MS_WIN32
    char *machinepath, *userpath;

    /* Are we running under Windows 3.1(1) Win32s? */
    if (PyWin_IsWin32s()) {
        /* Only CLASSES_ROOT is supported */
        machinepath = getpythonregpath(HKEY_CLASSES_ROOT, TRUE);
        userpath = NULL;
    } else {
        machinepath = getpythonregpath(HKEY_LOCAL_MACHINE, FALSE);
        userpath = getpythonregpath(HKEY_CURRENT_USER, FALSE);
    }
#endif

    get_progpath();
    strcpy(argv0_path, progpath);
    reduce(argv0_path);
    if (pythonhome == NULL || *pythonhome == '\0') {
        if (search_for_prefix(argv0_path, LANDMARK))
            pythonhome = prefix;
        else
            pythonhome = NULL;
    }
    else {
        char *delim;

        strcpy(prefix, pythonhome);

        /* Extract Any Optional Trailing EXEC_PREFIX */
        /* e.g. PYTHONHOME=<prefix>:<exec_prefix>   */
        delim = strchr(prefix, DELIM);
        if (delim) {
            *delim = '\0';
            strcpy(exec_prefix, delim+1);
        } else
            strcpy(exec_prefix, EXEC_PREFIX);
    }

    if (envpath && *envpath == '\0')
        envpath = NULL;

    /* We need to construct a path from the following parts:
       (1) the PYTHONPATH environment variable, if set;
       (2) for Win32, the machinepath and userpath, if set;
       (3) the PYTHONPATH config macro, with the leading "."
           of each component replaced with pythonhome, if set;
       (4) the directory containing the executable (argv0_path).
       The length calculation calculates #3 first.
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
    if (envpath != NULL)
        bufsz += strlen(envpath) + 1;
    bufsz += strlen(argv0_path) + 1;
#ifdef MS_WIN32
    if (machinepath)
        bufsz += strlen(machinepath) + 1;
    if (userpath)
        bufsz += strlen(userpath) + 1;
#endif

    module_search_path = buf = malloc(bufsz);
    if (buf == NULL) {
        /* We can't exit, so print a warning and limp along */
        fprintf(stderr, "Can't malloc dynamic PYTHONPATH.\n");
        if (envpath) {
            fprintf(stderr, "Using default static $PYTHONPATH.\n");
            module_search_path = envpath;
        }
        else {
            fprintf(stderr, "Using environment $PYTHONPATH.\n");
            module_search_path = PYTHONPATH;
        }
        return;
    }

    if (envpath) {
        strcpy(buf, envpath);
        buf = strchr(buf, '\0');
        *buf++ = DELIM;
    }
#ifdef MS_WIN32
    if (machinepath) {
        strcpy(buf, machinepath);
        buf = strchr(buf, '\0');
        *buf++ = DELIM;
    }
    if (userpath) {
        strcpy(buf, userpath);
        buf = strchr(buf, '\0');
        *buf++ = DELIM;
    }
#endif
    if (pythonhome == NULL) {
        strcpy(buf, PYTHONPATH);
        buf = strchr(buf, '\0');
    }
    else {
        char *p = PYTHONPATH;
        char *q;
        int n;
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
    if (!module_search_path)
        calculate_path();

    return exec_prefix;
}

char *
Py_GetProgramFullPath(void)
{
    if (!module_search_path)
        calculate_path();

    return progpath;
}
