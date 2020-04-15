
/* Return the initial module search path. */
/* Used by DOS, Windows 3.1, Windows 95/98, Windows NT. */

/* ----------------------------------------------------------------
   PATH RULES FOR WINDOWS:
   This describes how sys.path is formed on Windows.  It describes the
   functionality, not the implementation (ie, the order in which these
   are actually fetched is different). The presence of a python._pth or
   pythonXY._pth file alongside the program overrides these rules - see
   below.

   * Python always adds an empty entry at the start, which corresponds
     to the current directory.

   * If the PYTHONPATH env. var. exists, its entries are added next.

   * We look in the registry for "application paths" - that is, sub-keys
     under the main PythonPath registry key.  These are added next (the
     order of sub-key processing is undefined).
     HKEY_CURRENT_USER is searched and added first.
     HKEY_LOCAL_MACHINE is searched and added next.
     (Note that all known installers only use HKLM, so HKCU is typically
     empty)

   * We attempt to locate the "Python Home" - if the PYTHONHOME env var
     is set, we believe it.  Otherwise, we use the path of our host .EXE's
     to try and locate one of our "landmarks" and deduce our home.
     - If we DO have a Python Home: The relevant sub-directories (Lib,
       DLLs, etc) are based on the Python Home
     - If we DO NOT have a Python Home, the core Python Path is
       loaded from the registry.  This is the main PythonPath key,
       and both HKLM and HKCU are combined to form the path)

   * Iff - we can not locate the Python Home, have not had a PYTHONPATH
     specified, and can't locate any Registry entries (ie, we have _nothing_
     we can assume is a good path), a default path with relative entries is
     used (eg. .\Lib;.\DLLs, etc)


   If a '._pth' file exists adjacent to the executable with the same base name
   (e.g. python._pth adjacent to python.exe) or adjacent to the shared library
   (e.g. python36._pth adjacent to python36.dll), it is used in preference to
   the above process. The shared library file takes precedence over the
   executable. The path file must contain a list of paths to add to sys.path,
   one per line. Each path is relative to the directory containing the file.
   Blank lines and comments beginning with '#' are permitted.

   In the presence of this ._pth file, no other paths are added to the search
   path, the registry finder is not enabled, site.py is not imported and
   isolated mode is enabled. The site package can be enabled by including a
   line reading "import site"; no other imports are recognized. Any invalid
   entry (other than directories that do not exist) will result in immediate
   termination of the program.


  The end result of all this is:
  * When running python.exe, or any other .exe in the main Python directory
    (either an installed version, or directly from the PCbuild directory),
    the core path is deduced, and the core paths in the registry are
    ignored.  Other "application paths" in the registry are always read.

  * When Python is hosted in another exe (different directory, embedded via
    COM, etc), the Python Home will not be deduced, so the core path from
    the registry is used.  Other "application paths" in the registry are
    always read.

  * If Python can't find its home and there is no registry (eg, frozen
    exe, some very strange installation setup) you get a path with
    some default, but relative, paths.

  * An embedding application can use Py_SetPath() to override all of
    these automatic path computations.

  * An install of Python can fully specify the contents of sys.path using
    either a 'EXENAME._pth' or 'DLLNAME._pth' file, optionally including
    "import site" to enable the site module.

   ---------------------------------------------------------------- */


#include "Python.h"
#include "pycore_initconfig.h"    // PyStatus
#include "pycore_pathconfig.h"    // _PyPathConfig
#include "osdefs.h"               // SEP, ALTSEP
#include <wchar.h>

#ifndef MS_WINDOWS
#error getpathp.c should only be built on Windows
#endif

#include <windows.h>
#include <pathcch.h>
#include <shlwapi.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#include <string.h>

/* Search in some common locations for the associated Python libraries.
 *
 * Py_GetPath() tries to return a sensible Python module search path.
 *
 * The approach is an adaptation for Windows of the strategy used in
 * ../Modules/getpath.c; it uses the Windows Registry as one of its
 * information sources.
 *
 * Py_SetPath() can be used to override this mechanism.  Call Py_SetPath
 * with a semicolon separated path prior to calling Py_Initialize.
 */

#ifndef LANDMARK
#  define LANDMARK L"lib\\os.py"
#endif

#define INIT_ERR_BUFFER_OVERFLOW() _PyStatus_ERR("buffer overflow")


typedef struct {
    const wchar_t *path_env;           /* PATH environment variable */
    const wchar_t *home;               /* PYTHONHOME environment variable */

    /* Registry key "Software\Python\PythonCore\X.Y\PythonPath"
       where X.Y is the Python version (major.minor) */
    wchar_t *machine_path;   /* from HKEY_LOCAL_MACHINE */
    wchar_t *user_path;      /* from HKEY_CURRENT_USER */

    wchar_t *dll_path;

    const wchar_t *pythonpath_env;
} PyCalculatePath;


/* determine if "ch" is a separator character */
static int
is_sep(wchar_t ch)
{
#ifdef ALTSEP
    return ch == SEP || ch == ALTSEP;
#else
    return ch == SEP;
#endif
}


/* assumes 'dir' null terminated in bounds.  Never writes
   beyond existing terminator. */
static void
reduce(wchar_t *dir)
{
    size_t i = wcsnlen_s(dir, MAXPATHLEN+1);
    if (i >= MAXPATHLEN+1) {
        Py_FatalError("buffer overflow in getpathp.c's reduce()");
    }

    while (i > 0 && !is_sep(dir[i]))
        --i;
    dir[i] = '\0';
}


static int
change_ext(wchar_t *dest, const wchar_t *src, const wchar_t *ext)
{
    size_t src_len = wcsnlen_s(src, MAXPATHLEN+1);
    size_t i = src_len;
    if (i >= MAXPATHLEN+1) {
        Py_FatalError("buffer overflow in getpathp.c's reduce()");
    }

    while (i > 0 && src[i] != '.' && !is_sep(src[i]))
        --i;

    if (i == 0) {
        dest[0] = '\0';
        return -1;
    }

    if (is_sep(src[i])) {
        i = src_len;
    }

    if (wcsncpy_s(dest, MAXPATHLEN+1, src, i) ||
        wcscat_s(dest, MAXPATHLEN+1, ext))
    {
        dest[0] = '\0';
        return -1;
    }

    return 0;
}


static int
exists(const wchar_t *filename)
{
    return GetFileAttributesW(filename) != 0xFFFFFFFF;
}


/* Is module -- check for .pyc too.
   Assumes 'filename' MAXPATHLEN+1 bytes long -
   may extend 'filename' by one character. */
static int
ismodule(wchar_t *filename, int update_filename)
{
    size_t n;

    if (exists(filename)) {
        return 1;
    }

    /* Check for the compiled version of prefix. */
    n = wcsnlen_s(filename, MAXPATHLEN+1);
    if (n < MAXPATHLEN) {
        int exist = 0;
        filename[n] = L'c';
        filename[n + 1] = L'\0';
        exist = exists(filename);
        if (!update_filename) {
            filename[n] = L'\0';
        }
        return exist;
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
join(wchar_t *buffer, const wchar_t *stuff)
{
    if (FAILED(PathCchCombineEx(buffer, MAXPATHLEN+1, buffer, stuff, 0))) {
        Py_FatalError("buffer overflow in getpathp.c's join()");
    }
}

/* Call PathCchCanonicalizeEx(path): remove navigation elements such as "."
   and ".." to produce a direct, well-formed path. */
static PyStatus
canonicalize(wchar_t *buffer, const wchar_t *path)
{
    if (buffer == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    if (FAILED(PathCchCanonicalizeEx(buffer, MAXPATHLEN + 1, path, 0))) {
        return INIT_ERR_BUFFER_OVERFLOW();
    }
    return _PyStatus_OK();
}


/* gotlandmark only called by search_for_prefix, which ensures
   'prefix' is null terminated in bounds.  join() ensures
   'landmark' can not overflow prefix if too long. */
static int
gotlandmark(const wchar_t *prefix, const wchar_t *landmark)
{
    wchar_t filename[MAXPATHLEN+1];
    memset(filename, 0, sizeof(filename));
    wcscpy_s(filename, Py_ARRAY_LENGTH(filename), prefix);
    join(filename, landmark);
    return ismodule(filename, FALSE);
}


/* assumes argv0_path is MAXPATHLEN+1 bytes long, already \0 term'd.
   assumption provided by only caller, calculate_path() */
static int
search_for_prefix(wchar_t *prefix, const wchar_t *argv0_path, const wchar_t *landmark)
{
    /* Search from argv0_path, until landmark is found */
    wcscpy_s(prefix, MAXPATHLEN + 1, argv0_path);
    do {
        if (gotlandmark(prefix, landmark)) {
            return 1;
        }
        reduce(prefix);
    } while (prefix[0]);
    return 0;
}


#ifdef Py_ENABLE_SHARED

/* a string loaded from the DLL at startup.*/
extern const char *PyWin_DLLVersionString;

/* Load a PYTHONPATH value from the registry.
   Load from either HKEY_LOCAL_MACHINE or HKEY_CURRENT_USER.

   Works in both Unicode and 8bit environments.  Only uses the
   Ex family of functions so it also works with Windows CE.

   Returns NULL, or a pointer that should be freed.

   XXX - this code is pretty strange, as it used to also
   work on Win16, where the buffer sizes werent available
   in advance.  It could be simplied now Win16/Win32s is dead!
*/
static wchar_t *
getpythonregpath(HKEY keyBase, int skipcore)
{
    HKEY newKey = 0;
    DWORD dataSize = 0;
    DWORD numKeys = 0;
    LONG rc;
    wchar_t *retval = NULL;
    WCHAR *dataBuf = NULL;
    static const WCHAR keyPrefix[] = L"Software\\Python\\PythonCore\\";
    static const WCHAR keySuffix[] = L"\\PythonPath";
    size_t versionLen, keyBufLen;
    DWORD index;
    WCHAR *keyBuf = NULL;
    WCHAR *keyBufPtr;
    WCHAR **ppPaths = NULL;

    /* Tried to use sysget("winver") but here is too early :-( */
    versionLen = strlen(PyWin_DLLVersionString);
    /* Space for all the chars, plus one \0 */
    keyBufLen = sizeof(keyPrefix) +
                sizeof(WCHAR)*(versionLen-1) +
                sizeof(keySuffix);
    keyBuf = keyBufPtr = PyMem_RawMalloc(keyBufLen);
    if (keyBuf==NULL) {
        goto done;
    }

    memcpy_s(keyBufPtr, keyBufLen, keyPrefix, sizeof(keyPrefix)-sizeof(WCHAR));
    keyBufPtr += Py_ARRAY_LENGTH(keyPrefix) - 1;
    mbstowcs(keyBufPtr, PyWin_DLLVersionString, versionLen);
    keyBufPtr += versionLen;
    /* NULL comes with this one! */
    memcpy(keyBufPtr, keySuffix, sizeof(keySuffix));
    /* Open the root Python key */
    rc=RegOpenKeyExW(keyBase,
                    keyBuf, /* subkey */
            0, /* reserved */
            KEY_READ,
            &newKey);
    if (rc!=ERROR_SUCCESS) {
        goto done;
    }
    /* Find out how big our core buffer is, and how many subkeys we have */
    rc = RegQueryInfoKey(newKey, NULL, NULL, NULL, &numKeys, NULL, NULL,
                    NULL, NULL, &dataSize, NULL, NULL);
    if (rc!=ERROR_SUCCESS) {
        goto done;
    }
    if (skipcore) {
        dataSize = 0; /* Only count core ones if we want them! */
    }
    /* Allocate a temp array of char buffers, so we only need to loop
       reading the registry once
    */
    ppPaths = PyMem_RawCalloc(numKeys, sizeof(WCHAR *));
    if (ppPaths==NULL) {
        goto done;
    }
    /* Loop over all subkeys, allocating a temp sub-buffer. */
    for(index=0;index<numKeys;index++) {
        WCHAR keyBuf[MAX_PATH+1];
        HKEY subKey = 0;
        DWORD reqdSize = MAX_PATH+1;
        /* Get the sub-key name */
        DWORD rc = RegEnumKeyExW(newKey, index, keyBuf, &reqdSize,
                                 NULL, NULL, NULL, NULL );
        if (rc!=ERROR_SUCCESS) {
            goto done;
        }
        /* Open the sub-key */
        rc=RegOpenKeyExW(newKey,
                                        keyBuf, /* subkey */
                        0, /* reserved */
                        KEY_READ,
                        &subKey);
        if (rc!=ERROR_SUCCESS) {
            goto done;
        }
        /* Find the value of the buffer size, malloc, then read it */
        RegQueryValueExW(subKey, NULL, 0, NULL, NULL, &reqdSize);
        if (reqdSize) {
            ppPaths[index] = PyMem_RawMalloc(reqdSize);
            if (ppPaths[index]) {
                RegQueryValueExW(subKey, NULL, 0, NULL,
                                (LPBYTE)ppPaths[index],
                                &reqdSize);
                dataSize += reqdSize + 1; /* 1 for the ";" */
            }
        }
        RegCloseKey(subKey);
    }

    /* return null if no path to return */
    if (dataSize == 0) {
        goto done;
    }

    /* original datasize from RegQueryInfo doesn't include the \0 */
    dataBuf = PyMem_RawMalloc((dataSize+1) * sizeof(WCHAR));
    if (dataBuf) {
        WCHAR *szCur = dataBuf;
        /* Copy our collected strings */
        for (index=0;index<numKeys;index++) {
            if (index > 0) {
                *(szCur++) = L';';
                dataSize--;
            }
            if (ppPaths[index]) {
                Py_ssize_t len = wcslen(ppPaths[index]);
                wcsncpy(szCur, ppPaths[index], len);
                szCur += len;
                assert(dataSize > (DWORD)len);
                dataSize -= (DWORD)len;
            }
        }
        if (skipcore) {
            *szCur = '\0';
        }
        else {
            /* If we have no values, we don't need a ';' */
            if (numKeys) {
                *(szCur++) = L';';
                dataSize--;
            }
            /* Now append the core path entries -
               this will include the NULL
            */
            rc = RegQueryValueExW(newKey, NULL, 0, NULL,
                                  (LPBYTE)szCur, &dataSize);
            if (rc != ERROR_SUCCESS) {
                PyMem_RawFree(dataBuf);
                goto done;
            }
        }
        /* And set the result - caller must free */
        retval = dataBuf;
    }
done:
    /* Loop freeing my temp buffers */
    if (ppPaths) {
        for(index=0; index<numKeys; index++)
            PyMem_RawFree(ppPaths[index]);
        PyMem_RawFree(ppPaths);
    }
    if (newKey) {
        RegCloseKey(newKey);
    }
    PyMem_RawFree(keyBuf);
    return retval;
}
#endif /* Py_ENABLE_SHARED */


wchar_t*
_Py_GetDLLPath(void)
{
    wchar_t dll_path[MAXPATHLEN+1];
    memset(dll_path, 0, sizeof(dll_path));

#ifdef Py_ENABLE_SHARED
    extern HANDLE PyWin_DLLhModule;
    if (PyWin_DLLhModule) {
        if (!GetModuleFileNameW(PyWin_DLLhModule, dll_path, MAXPATHLEN)) {
            dll_path[0] = 0;
        }
    }
#else
    dll_path[0] = 0;
#endif

    return _PyMem_RawWcsdup(dll_path);
}


static PyStatus
get_program_full_path(_PyPathConfig *pathconfig)
{
    PyStatus status;
    const wchar_t *pyvenv_launcher;
    wchar_t program_full_path[MAXPATHLEN+1];
    memset(program_full_path, 0, sizeof(program_full_path));

    if (!GetModuleFileNameW(NULL, program_full_path, MAXPATHLEN)) {
        /* GetModuleFileName should never fail when passed NULL */
        return _PyStatus_ERR("Cannot determine program path");
    }

    /* The launcher may need to force the executable path to a
     * different environment, so override it here. */
    pyvenv_launcher = _wgetenv(L"__PYVENV_LAUNCHER__");
    if (pyvenv_launcher && pyvenv_launcher[0]) {
        /* If overridden, preserve the original full path */
        if (pathconfig->base_executable == NULL) {
            pathconfig->base_executable = PyMem_RawMalloc(
                sizeof(wchar_t) * (MAXPATHLEN + 1));
            if (pathconfig->base_executable == NULL) {
                return _PyStatus_NO_MEMORY();
            }

            status = canonicalize(pathconfig->base_executable,
                                  program_full_path);
            if (_PyStatus_EXCEPTION(status)) {
                return status;
            }
        }

        wcscpy_s(program_full_path, MAXPATHLEN+1, pyvenv_launcher);
        /* bpo-35873: Clear the environment variable to avoid it being
        * inherited by child processes. */
        _wputenv_s(L"__PYVENV_LAUNCHER__", L"");
    }

    if (pathconfig->program_full_path == NULL) {
        pathconfig->program_full_path = PyMem_RawMalloc(
            sizeof(wchar_t) * (MAXPATHLEN + 1));
        if (pathconfig->program_full_path == NULL) {
            return _PyStatus_NO_MEMORY();
        }

        status = canonicalize(pathconfig->program_full_path,
                              program_full_path);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
    return _PyStatus_OK();
}


static PyStatus
read_pth_file(_PyPathConfig *pathconfig, wchar_t *prefix, const wchar_t *path,
              int *found)
{
    PyStatus status;
    wchar_t *buf = NULL;
    wchar_t *wline = NULL;
    FILE *sp_file;

    sp_file = _Py_wfopen(path, L"r");
    if (sp_file == NULL) {
        return _PyStatus_OK();
    }

    wcscpy_s(prefix, MAXPATHLEN+1, path);
    reduce(prefix);
    pathconfig->isolated = 1;
    pathconfig->site_import = 0;

    size_t bufsiz = MAXPATHLEN;
    size_t prefixlen = wcslen(prefix);

    buf = (wchar_t*)PyMem_RawMalloc(bufsiz * sizeof(wchar_t));
    if (buf == NULL) {
        status = _PyStatus_NO_MEMORY();
        goto done;
    }
    buf[0] = '\0';

    while (!feof(sp_file)) {
        char line[MAXPATHLEN + 1];
        char *p = fgets(line, Py_ARRAY_LENGTH(line), sp_file);
        if (!p) {
            break;
        }
        if (*p == '\0' || *p == '\r' || *p == '\n' || *p == '#') {
            continue;
        }
        while (*++p) {
            if (*p == '\r' || *p == '\n') {
                *p = '\0';
                break;
            }
        }

        if (strcmp(line, "import site") == 0) {
            pathconfig->site_import = 1;
            continue;
        }
        else if (strncmp(line, "import ", 7) == 0) {
            status = _PyStatus_ERR("only 'import site' is supported "
                                   "in ._pth file");
            goto done;
        }

        DWORD wn = MultiByteToWideChar(CP_UTF8, 0, line, -1, NULL, 0);
        wchar_t *wline = (wchar_t*)PyMem_RawMalloc((wn + 1) * sizeof(wchar_t));
        if (wline == NULL) {
            status = _PyStatus_NO_MEMORY();
            goto done;
        }
        wn = MultiByteToWideChar(CP_UTF8, 0, line, -1, wline, wn + 1);
        wline[wn] = '\0';

        size_t usedsiz = wcslen(buf);
        while (usedsiz + wn + prefixlen + 4 > bufsiz) {
            bufsiz += MAXPATHLEN;
            wchar_t *tmp = (wchar_t*)PyMem_RawRealloc(buf, (bufsiz + 1) *
                                                            sizeof(wchar_t));
            if (tmp == NULL) {
                status = _PyStatus_NO_MEMORY();
                goto done;
            }
            buf = tmp;
        }

        if (usedsiz) {
            wcscat_s(buf, bufsiz, L";");
            usedsiz += 1;
        }

        errno_t result;
        _Py_BEGIN_SUPPRESS_IPH
        result = wcscat_s(buf, bufsiz, prefix);
        _Py_END_SUPPRESS_IPH

        if (result == EINVAL) {
            status = _PyStatus_ERR("invalid argument during ._pth processing");
            goto done;
        } else if (result == ERANGE) {
            status = _PyStatus_ERR("buffer overflow during ._pth processing");
            goto done;
        }

        wchar_t *b = &buf[usedsiz];
        join(b, wline);

        PyMem_RawFree(wline);
        wline = NULL;
    }

    if (pathconfig->module_search_path == NULL) {
        pathconfig->module_search_path = _PyMem_RawWcsdup(buf);
        if (pathconfig->module_search_path == NULL) {
            status = _PyStatus_NO_MEMORY();
            goto done;
        }
    }

    *found = 1;
    status = _PyStatus_OK();
    goto done;

done:
    PyMem_RawFree(buf);
    PyMem_RawFree(wline);
    fclose(sp_file);
    return status;
}


static int
get_pth_filename(PyCalculatePath *calculate, wchar_t *filename,
                 const _PyPathConfig *pathconfig)
{
    if (calculate->dll_path[0]) {
        if (!change_ext(filename, calculate->dll_path, L"._pth") &&
            exists(filename))
        {
            return 1;
        }
    }
    if (pathconfig->program_full_path[0]) {
        if (!change_ext(filename, pathconfig->program_full_path, L"._pth") &&
            exists(filename))
        {
            return 1;
        }
    }
    return 0;
}


static PyStatus
calculate_pth_file(PyCalculatePath *calculate, _PyPathConfig *pathconfig,
                   wchar_t *prefix, int *found)
{
    wchar_t filename[MAXPATHLEN+1];

    if (!get_pth_filename(calculate, filename, pathconfig)) {
        return _PyStatus_OK();
    }

    return read_pth_file(pathconfig, prefix, filename, found);
}


/* Search for an environment configuration file, first in the
   executable's directory and then in the parent directory.
   If found, open it for use when searching for prefixes.
*/
static PyStatus
calculate_pyvenv_file(PyCalculatePath *calculate,
                      wchar_t *argv0_path, size_t argv0_path_len)
{
    wchar_t filename[MAXPATHLEN+1];
    const wchar_t *env_cfg = L"pyvenv.cfg";

    /* Filename: <argv0_path_len> / "pyvenv.cfg" */
    wcscpy_s(filename, MAXPATHLEN+1, argv0_path);
    join(filename, env_cfg);

    FILE *env_file = _Py_wfopen(filename, L"r");
    if (env_file == NULL) {
        errno = 0;

        /* Filename: <basename(basename(argv0_path_len))> / "pyvenv.cfg" */
        reduce(filename);
        reduce(filename);
        join(filename, env_cfg);

        env_file = _Py_wfopen(filename, L"r");
        if (env_file == NULL) {
            errno = 0;
            return _PyStatus_OK();
        }
    }

    /* Look for a 'home' variable and set argv0_path to it, if found */
    wchar_t *home = NULL;
    PyStatus status = _Py_FindEnvConfigValue(env_file, L"home", &home);
    if (_PyStatus_EXCEPTION(status)) {
        fclose(env_file);
        return status;
    }
    if (home) {
        wcscpy_s(argv0_path, argv0_path_len, home);
        PyMem_RawFree(home);
    }
    fclose(env_file);
    return _PyStatus_OK();
}


static void
calculate_home_prefix(PyCalculatePath *calculate,
                      const wchar_t *argv0_path,
                      const wchar_t *zip_path,
                      wchar_t *prefix)
{
    if (calculate->home == NULL || *calculate->home == '\0') {
        if (zip_path[0] && exists(zip_path)) {
            wcscpy_s(prefix, MAXPATHLEN+1, zip_path);
            reduce(prefix);
            calculate->home = prefix;
        }
        else if (search_for_prefix(prefix, argv0_path, LANDMARK)) {
            calculate->home = prefix;
        }
        else {
            calculate->home = NULL;
        }
    }
    else {
        wcscpy_s(prefix, MAXPATHLEN+1, calculate->home);
    }
}


static PyStatus
calculate_module_search_path(PyCalculatePath *calculate,
                             _PyPathConfig *pathconfig,
                             const wchar_t *argv0_path,
                             wchar_t *prefix,
                             const wchar_t *zip_path)
{
    int skiphome = calculate->home==NULL ? 0 : 1;
#ifdef Py_ENABLE_SHARED
    if (!Py_IgnoreEnvironmentFlag) {
        calculate->machine_path = getpythonregpath(HKEY_LOCAL_MACHINE,
                                                   skiphome);
        calculate->user_path = getpythonregpath(HKEY_CURRENT_USER, skiphome);
    }
#endif
    /* We only use the default relative PYTHONPATH if we haven't
       anything better to use! */
    int skipdefault = (calculate->pythonpath_env != NULL ||
                       calculate->home != NULL ||
                       calculate->machine_path != NULL ||
                       calculate->user_path != NULL);

    /* We need to construct a path from the following parts.
       (1) the PYTHONPATH environment variable, if set;
       (2) for Win32, the zip archive file path;
       (3) for Win32, the machine_path and user_path, if set;
       (4) the PYTHONPATH config macro, with the leading "."
           of each component replaced with home, if set;
       (5) the directory containing the executable (argv0_path).
       The length calculation calculates #4 first.
       Extra rules:
       - If PYTHONHOME is set (in any way) item (3) is ignored.
       - If registry values are used, (4) and (5) are ignored.
    */

    /* Calculate size of return buffer */
    size_t bufsz = 0;
    if (calculate->home != NULL) {
        const wchar_t *p;
        bufsz = 1;
        for (p = PYTHONPATH; *p; p++) {
            if (*p == DELIM) {
                bufsz++; /* number of DELIM plus one */
            }
        }
        bufsz *= wcslen(calculate->home);
    }
    bufsz += wcslen(PYTHONPATH) + 1;
    bufsz += wcslen(argv0_path) + 1;
    if (calculate->user_path) {
        bufsz += wcslen(calculate->user_path) + 1;
    }
    if (calculate->machine_path) {
        bufsz += wcslen(calculate->machine_path) + 1;
    }
    bufsz += wcslen(zip_path) + 1;
    if (calculate->pythonpath_env != NULL) {
        bufsz += wcslen(calculate->pythonpath_env) + 1;
    }

    wchar_t *buf, *start_buf;
    buf = PyMem_RawMalloc(bufsz * sizeof(wchar_t));
    if (buf == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    start_buf = buf;

    if (calculate->pythonpath_env) {
        if (wcscpy_s(buf, bufsz - (buf - start_buf),
                     calculate->pythonpath_env)) {
            return INIT_ERR_BUFFER_OVERFLOW();
        }
        buf = wcschr(buf, L'\0');
        *buf++ = DELIM;
    }
    if (zip_path[0]) {
        if (wcscpy_s(buf, bufsz - (buf - start_buf), zip_path)) {
            return INIT_ERR_BUFFER_OVERFLOW();
        }
        buf = wcschr(buf, L'\0');
        *buf++ = DELIM;
    }
    if (calculate->user_path) {
        if (wcscpy_s(buf, bufsz - (buf - start_buf), calculate->user_path)) {
            return INIT_ERR_BUFFER_OVERFLOW();
        }
        buf = wcschr(buf, L'\0');
        *buf++ = DELIM;
    }
    if (calculate->machine_path) {
        if (wcscpy_s(buf, bufsz - (buf - start_buf), calculate->machine_path)) {
            return INIT_ERR_BUFFER_OVERFLOW();
        }
        buf = wcschr(buf, L'\0');
        *buf++ = DELIM;
    }
    if (calculate->home == NULL) {
        if (!skipdefault) {
            if (wcscpy_s(buf, bufsz - (buf - start_buf), PYTHONPATH)) {
                return INIT_ERR_BUFFER_OVERFLOW();
            }
            buf = wcschr(buf, L'\0');
            *buf++ = DELIM;
        }
    } else {
        const wchar_t *p = PYTHONPATH;
        const wchar_t *q;
        size_t n;
        for (;;) {
            q = wcschr(p, DELIM);
            if (q == NULL) {
                n = wcslen(p);
            }
            else {
                n = q-p;
            }
            if (p[0] == '.' && is_sep(p[1])) {
                if (wcscpy_s(buf, bufsz - (buf - start_buf), calculate->home)) {
                    return INIT_ERR_BUFFER_OVERFLOW();
                }
                buf = wcschr(buf, L'\0');
                p++;
                n--;
            }
            wcsncpy(buf, p, n);
            buf += n;
            *buf++ = DELIM;
            if (q == NULL) {
                break;
            }
            p = q+1;
        }
    }
    if (argv0_path) {
        wcscpy(buf, argv0_path);
        buf = wcschr(buf, L'\0');
        *buf++ = DELIM;
    }
    *(buf - 1) = L'\0';

    /* Now to pull one last hack/trick.  If sys.prefix is
       empty, then try and find it somewhere on the paths
       we calculated.  We scan backwards, as our general policy
       is that Python core directories are at the *end* of
       sys.path.  We assume that our "lib" directory is
       on the path, and that our 'prefix' directory is
       the parent of that.
    */
    if (prefix[0] == L'\0') {
        wchar_t lookBuf[MAXPATHLEN+1];
        const wchar_t *look = buf - 1; /* 'buf' is at the end of the buffer */
        while (1) {
            Py_ssize_t nchars;
            const wchar_t *lookEnd = look;
            /* 'look' will end up one character before the
               start of the path in question - even if this
               is one character before the start of the buffer
            */
            while (look >= start_buf && *look != DELIM)
                look--;
            nchars = lookEnd-look;
            wcsncpy(lookBuf, look+1, nchars);
            lookBuf[nchars] = L'\0';
            /* Up one level to the parent */
            reduce(lookBuf);
            if (search_for_prefix(prefix, lookBuf, LANDMARK)) {
                break;
            }
            /* If we are out of paths to search - give up */
            if (look < start_buf) {
                break;
            }
            look--;
        }
    }

    pathconfig->module_search_path = start_buf;
    return _PyStatus_OK();
}


static PyStatus
calculate_path(PyCalculatePath *calculate, _PyPathConfig *pathconfig)
{
    PyStatus status;

    status = get_program_full_path(pathconfig);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    /* program_full_path guaranteed \0 terminated in MAXPATH+1 bytes. */
    wchar_t argv0_path[MAXPATHLEN+1];
    memset(argv0_path, 0, sizeof(argv0_path));

    wcscpy_s(argv0_path, MAXPATHLEN+1, pathconfig->program_full_path);
    reduce(argv0_path);

    wchar_t prefix[MAXPATHLEN+1];
    memset(prefix, 0, sizeof(prefix));

    /* Search for a sys.path file */
    int pth_found = 0;
    status = calculate_pth_file(calculate, pathconfig, prefix, &pth_found);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }
    if (pth_found) {
        goto done;
    }

    status = calculate_pyvenv_file(calculate,
                                   argv0_path, Py_ARRAY_LENGTH(argv0_path));
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    /* Calculate zip archive path from DLL or exe path */
    wchar_t zip_path[MAXPATHLEN+1];
    memset(zip_path, 0, sizeof(zip_path));

    change_ext(zip_path,
               calculate->dll_path[0] ? calculate->dll_path : pathconfig->program_full_path,
               L".zip");

    calculate_home_prefix(calculate, argv0_path, zip_path, prefix);

    if (pathconfig->module_search_path == NULL) {
        status = calculate_module_search_path(calculate, pathconfig,
                                              argv0_path, prefix, zip_path);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

done:
    if (pathconfig->prefix == NULL) {
        pathconfig->prefix = _PyMem_RawWcsdup(prefix);
        if (pathconfig->prefix == NULL) {
            return _PyStatus_NO_MEMORY();
        }
    }
    if (pathconfig->exec_prefix == NULL) {
        pathconfig->exec_prefix = _PyMem_RawWcsdup(prefix);
        if (pathconfig->exec_prefix == NULL) {
            return _PyStatus_NO_MEMORY();
        }
    }

    return _PyStatus_OK();
}


static PyStatus
calculate_init(PyCalculatePath *calculate, _PyPathConfig *pathconfig,
               const PyConfig *config)
{
    calculate->home = pathconfig->home;
    calculate->path_env = _wgetenv(L"PATH");

    calculate->dll_path = _Py_GetDLLPath();
    if (calculate->dll_path == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    calculate->pythonpath_env = config->pythonpath_env;

    return _PyStatus_OK();
}


static void
calculate_free(PyCalculatePath *calculate)
{
    PyMem_RawFree(calculate->machine_path);
    PyMem_RawFree(calculate->user_path);
    PyMem_RawFree(calculate->dll_path);
}


/* Calculate the Python path configuration.

   Inputs:

   - PyConfig.pythonpath_env: PYTHONPATH environment variable
   - _PyPathConfig.home: Py_SetPythonHome() or PYTHONHOME environment variable
   - DLL path: _Py_GetDLLPath()
   - PATH environment variable
   - __PYVENV_LAUNCHER__ environment variable
   - GetModuleFileNameW(NULL): fully qualified path of the executable file of
     the current process
   - ._pth configuration file
   - pyvenv.cfg configuration file
   - Registry key "Software\Python\PythonCore\X.Y\PythonPath"
     of HKEY_CURRENT_USER and HKEY_LOCAL_MACHINE where X.Y is the Python
     version.

   Outputs, 'pathconfig' fields:

   - base_executable
   - program_full_path
   - module_search_path
   - prefix
   - exec_prefix
   - isolated
   - site_import

   If a field is already set (non NULL), it is left unchanged. */
PyStatus
_PyPathConfig_Calculate(_PyPathConfig *pathconfig, const PyConfig *config)
{
    PyStatus status;
    PyCalculatePath calculate;
    memset(&calculate, 0, sizeof(calculate));

    status = calculate_init(&calculate, pathconfig, config);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    status = calculate_path(&calculate, pathconfig);

done:
    calculate_free(&calculate);
    return status;
}


/* Load python3.dll before loading any extension module that might refer
   to it. That way, we can be sure that always the python3.dll corresponding
   to this python DLL is loaded, not a python3.dll that might be on the path
   by chance.
   Return whether the DLL was found.
*/
static int python3_checked = 0;
static HANDLE hPython3;
int
_Py_CheckPython3(void)
{
    wchar_t py3path[MAXPATHLEN+1];
    wchar_t *s;
    if (python3_checked) {
        return hPython3 != NULL;
    }
    python3_checked = 1;

    /* If there is a python3.dll next to the python3y.dll,
       assume this is a build tree; use that DLL */
    if (_Py_dll_path != NULL) {
        wcscpy(py3path, _Py_dll_path);
    }
    else {
        wcscpy(py3path, L"");
    }
    s = wcsrchr(py3path, L'\\');
    if (!s) {
        s = py3path;
    }
    wcscpy(s, L"\\python3.dll");
    hPython3 = LoadLibraryExW(py3path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (hPython3 != NULL) {
        return 1;
    }

    /* Check sys.prefix\DLLs\python3.dll */
    wcscpy(py3path, Py_GetPrefix());
    wcscat(py3path, L"\\DLLs\\python3.dll");
    hPython3 = LoadLibraryExW(py3path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    return hPython3 != NULL;
}
