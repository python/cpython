/* Return the initial module search path. */

#include "Python.h"
#include "marshal.h"              // PyMarshal_ReadObjectFromString
#include "osdefs.h"               // DELIM
#include "pycore_initconfig.h"
#include "pycore_fileutils.h"
#include "pycore_pathconfig.h"
#include "pycore_pymem.h"         // _PyMem_SetDefaultAllocator()
#include <wchar.h>

#ifdef MS_WINDOWS
#  include <windows.h>            // GetFullPathNameW(), MAX_PATH
#  include <pathcch.h>
#endif

#ifdef __APPLE__
#  include <mach-o/dyld.h>
#endif

/* Reference the precompiled getpath.py */
#include "../Python/frozen_modules/getpath.h"

#if (!defined(PREFIX) || !defined(EXEC_PREFIX) \
        || !defined(VERSION) || !defined(VPATH) \
        || !defined(PLATLIBDIR))
#error "PREFIX, EXEC_PREFIX, VERSION, VPATH and PLATLIBDIR macros must be defined"
#endif

#if !defined(PYTHONPATH)
#define PYTHONPATH NULL
#endif

#if !defined(PYDEBUGEXT)
#define PYDEBUGEXT NULL
#endif

#if !defined(PYWINVER)
#ifdef MS_DLL_ID
#define PYWINVER MS_DLL_ID
#else
#define PYWINVER NULL
#endif
#endif

#if !defined(EXE_SUFFIX)
#if defined(MS_WINDOWS) || defined(__CYGWIN__) || defined(__MINGW32__)
#define EXE_SUFFIX L".exe"
#else
#define EXE_SUFFIX NULL
#endif
#endif


/* HELPER FUNCTIONS for getpath.py */

static PyObject *
getpath_abspath(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *r = NULL;
    PyObject *pathobj;
    wchar_t *path;
    if (!PyArg_ParseTuple(args, "U", &pathobj)) {
        return NULL;
    }
    Py_ssize_t len;
    path = PyUnicode_AsWideCharString(pathobj, &len);
    if (path) {
        wchar_t *abs;
        if (_Py_abspath((const wchar_t *)_Py_normpath(path, -1), &abs) == 0 && abs) {
            r = PyUnicode_FromWideChar(abs, -1);
            PyMem_RawFree((void *)abs);
        } else {
            PyErr_SetString(PyExc_OSError, "failed to make path absolute");
        }
        PyMem_Free((void *)path);
    }
    return r;
}


static PyObject *
getpath_basename(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *path;
    if (!PyArg_ParseTuple(args, "U", &path)) {
        return NULL;
    }
    Py_ssize_t end = PyUnicode_GET_LENGTH(path);
    Py_ssize_t pos = PyUnicode_FindChar(path, SEP, 0, end, -1);
    if (pos < 0) {
        return Py_NewRef(path);
    }
    return PyUnicode_Substring(path, pos + 1, end);
}


static PyObject *
getpath_dirname(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *path;
    if (!PyArg_ParseTuple(args, "U", &path)) {
        return NULL;
    }
    Py_ssize_t end = PyUnicode_GET_LENGTH(path);
    Py_ssize_t pos = PyUnicode_FindChar(path, SEP, 0, end, -1);
    if (pos < 0) {
        return PyUnicode_FromStringAndSize(NULL, 0);
    }
    return PyUnicode_Substring(path, 0, pos);
}


static PyObject *
getpath_isabs(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *r = NULL;
    PyObject *pathobj;
    const wchar_t *path;
    if (!PyArg_ParseTuple(args, "U", &pathobj)) {
        return NULL;
    }
    path = PyUnicode_AsWideCharString(pathobj, NULL);
    if (path) {
        r = _Py_isabs(path) ? Py_True : Py_False;
        PyMem_Free((void *)path);
    }
    Py_XINCREF(r);
    return r;
}


static PyObject *
getpath_hassuffix(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *r = NULL;
    PyObject *pathobj;
    PyObject *suffixobj;
    const wchar_t *path;
    const wchar_t *suffix;
    if (!PyArg_ParseTuple(args, "UU", &pathobj, &suffixobj)) {
        return NULL;
    }
    Py_ssize_t len, suffixLen;
    path = PyUnicode_AsWideCharString(pathobj, &len);
    if (path) {
        suffix = PyUnicode_AsWideCharString(suffixobj, &suffixLen);
        if (suffix) {
            if (suffixLen > len ||
#ifdef MS_WINDOWS
                wcsicmp(&path[len - suffixLen], suffix) != 0
#else
                wcscmp(&path[len - suffixLen], suffix) != 0
#endif
            ) {
                r = Py_False;
            } else {
                r = Py_True;
            }
            Py_INCREF(r);
            PyMem_Free((void *)suffix);
        }
        PyMem_Free((void *)path);
    }
    return r;
}


static PyObject *
getpath_isdir(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *r = NULL;
    PyObject *pathobj;
    const wchar_t *path;
    if (!PyArg_ParseTuple(args, "U", &pathobj)) {
        return NULL;
    }
    path = PyUnicode_AsWideCharString(pathobj, NULL);
    if (path) {
#ifdef MS_WINDOWS
        DWORD attr = GetFileAttributesW(path);
        r = (attr != INVALID_FILE_ATTRIBUTES) &&
            (attr & FILE_ATTRIBUTE_DIRECTORY) ? Py_True : Py_False;
#else
        struct stat st;
        r = (_Py_wstat(path, &st) == 0) && S_ISDIR(st.st_mode) ? Py_True : Py_False;
#endif
        PyMem_Free((void *)path);
    }
    Py_XINCREF(r);
    return r;
}


static PyObject *
getpath_isfile(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *r = NULL;
    PyObject *pathobj;
    const wchar_t *path;
    if (!PyArg_ParseTuple(args, "U", &pathobj)) {
        return NULL;
    }
    path = PyUnicode_AsWideCharString(pathobj, NULL);
    if (path) {
#ifdef MS_WINDOWS
        DWORD attr = GetFileAttributesW(path);
        r = (attr != INVALID_FILE_ATTRIBUTES) &&
            !(attr & FILE_ATTRIBUTE_DIRECTORY) ? Py_True : Py_False;
#else
        struct stat st;
        r = (_Py_wstat(path, &st) == 0) && S_ISREG(st.st_mode) ? Py_True : Py_False;
#endif
        PyMem_Free((void *)path);
    }
    Py_XINCREF(r);
    return r;
}


static PyObject *
getpath_isxfile(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *r = NULL;
    PyObject *pathobj;
    const wchar_t *path;
    Py_ssize_t cchPath;
    if (!PyArg_ParseTuple(args, "U", &pathobj)) {
        return NULL;
    }
    path = PyUnicode_AsWideCharString(pathobj, &cchPath);
    if (path) {
#ifdef MS_WINDOWS
        const wchar_t *ext;
        DWORD attr = GetFileAttributesW(path);
        r = (attr != INVALID_FILE_ATTRIBUTES) &&
            !(attr & FILE_ATTRIBUTE_DIRECTORY) &&
            SUCCEEDED(PathCchFindExtension(path, cchPath + 1, &ext)) &&
            (CompareStringOrdinal(ext, -1, L".exe", -1, 1 /* ignore case */) == CSTR_EQUAL)
            ? Py_True : Py_False;
#else
        struct stat st;
        r = (_Py_wstat(path, &st) == 0) &&
            S_ISREG(st.st_mode) &&
            (st.st_mode & 0111)
            ? Py_True : Py_False;
#endif
        PyMem_Free((void *)path);
    }
    Py_XINCREF(r);
    return r;
}


static PyObject *
getpath_joinpath(PyObject *Py_UNUSED(self), PyObject *args)
{
    if (!PyTuple_Check(args)) {
        PyErr_SetString(PyExc_TypeError, "requires tuple of arguments");
        return NULL;
    }
    Py_ssize_t n = PyTuple_GET_SIZE(args);
    if (n == 0) {
        return PyUnicode_FromStringAndSize(NULL, 0);
    }
    /* Convert all parts to wchar and accumulate max final length */
    wchar_t **parts = (wchar_t **)PyMem_Malloc(n * sizeof(wchar_t *));
    memset(parts, 0, n * sizeof(wchar_t *));
    Py_ssize_t cchFinal = 0;
    Py_ssize_t first = 0;

    for (Py_ssize_t i = 0; i < n; ++i) {
        PyObject *s = PyTuple_GET_ITEM(args, i);
        Py_ssize_t cch;
        if (s == Py_None) {
            cch = 0;
        } else if (PyUnicode_Check(s)) {
            parts[i] = PyUnicode_AsWideCharString(s, &cch);
            if (!parts[i]) {
                cchFinal = -1;
                break;
            }
            if (_Py_isabs(parts[i])) {
                first = i;
            }
        } else {
            PyErr_SetString(PyExc_TypeError, "all arguments to joinpath() must be str or None");
            cchFinal = -1;
            break;
        }
        cchFinal += cch + 1;
    }

    wchar_t *final = cchFinal > 0 ? (wchar_t *)PyMem_Malloc(cchFinal * sizeof(wchar_t)) : NULL;
    if (!final) {
        for (Py_ssize_t i = 0; i < n; ++i) {
            PyMem_Free(parts[i]);
        }
        PyMem_Free(parts);
        if (cchFinal) {
            PyErr_NoMemory();
            return NULL;
        }
        return PyUnicode_FromStringAndSize(NULL, 0);
    }

    final[0] = '\0';
    /* Now join all the paths. The final result should be shorter than the buffer */
    for (Py_ssize_t i = 0; i < n; ++i) {
        if (!parts[i]) {
            continue;
        }
        if (i >= first && final) {
            if (!final[0]) {
                /* final is definitely long enough to fit any individual part */
                wcscpy(final, parts[i]);
            } else if (_Py_add_relfile(final, parts[i], cchFinal) < 0) {
                /* if we fail, keep iterating to free memory, but stop adding parts */
                PyMem_Free(final);
                final = NULL;
            }
        }
        PyMem_Free(parts[i]);
    }
    PyMem_Free(parts);
    if (!final) {
        PyErr_SetString(PyExc_SystemError, "failed to join paths");
        return NULL;
    }
    PyObject *r = PyUnicode_FromWideChar(_Py_normpath(final, -1), -1);
    PyMem_Free(final);
    return r;
}


static PyObject *
getpath_readlines(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *r = NULL;
    PyObject *pathobj;
    const wchar_t *path;
    if (!PyArg_ParseTuple(args, "U", &pathobj)) {
        return NULL;
    }
    path = PyUnicode_AsWideCharString(pathobj, NULL);
    if (!path) {
        return NULL;
    }
    FILE *fp = _Py_wfopen(path, L"rb");
    PyMem_Free((void *)path);
    if (!fp) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    r = PyList_New(0);
    if (!r) {
        fclose(fp);
        return NULL;
    }
    const size_t MAX_FILE = 32 * 1024;
    char *buffer = (char *)PyMem_Malloc(MAX_FILE);
    if (!buffer) {
        Py_DECREF(r);
        fclose(fp);
        return NULL;
    }

    size_t cb = fread(buffer, 1, MAX_FILE, fp);
    fclose(fp);
    if (!cb) {
        return r;
    }
    if (cb >= MAX_FILE) {
        Py_DECREF(r);
        PyErr_SetString(PyExc_MemoryError,
            "cannot read file larger than 32KB during initialization");
        return NULL;
    }
    buffer[cb] = '\0';

    size_t len;
    wchar_t *wbuffer = _Py_DecodeUTF8_surrogateescape(buffer, cb, &len);
    PyMem_Free((void *)buffer);
    if (!wbuffer) {
        Py_DECREF(r);
        PyErr_NoMemory();
        return NULL;
    }

    wchar_t *p1 = wbuffer;
    wchar_t *p2 = p1;
    while ((p2 = wcschr(p1, L'\n')) != NULL) {
        Py_ssize_t cb = p2 - p1;
        while (cb >= 0 && (p1[cb] == L'\n' || p1[cb] == L'\r')) {
            --cb;
        }
        PyObject *u = PyUnicode_FromWideChar(p1, cb >= 0 ? cb + 1 : 0);
        if (!u || PyList_Append(r, u) < 0) {
            Py_XDECREF(u);
            Py_CLEAR(r);
            break;
        }
        Py_DECREF(u);
        p1 = p2 + 1;
    }
    if (r && p1 && *p1) {
        PyObject *u = PyUnicode_FromWideChar(p1, -1);
        if (!u || PyList_Append(r, u) < 0) {
            Py_CLEAR(r);
        }
        Py_XDECREF(u);
    }
    PyMem_RawFree(wbuffer);
    return r;
}


static PyObject *
getpath_realpath(PyObject *Py_UNUSED(self) , PyObject *args)
{
    PyObject *pathobj;
    if (!PyArg_ParseTuple(args, "U", &pathobj)) {
        return NULL;
    }
#if defined(HAVE_READLINK)
    /* This readlink calculation only resolves a symlinked file, and
       does not resolve any path segments. This is consistent with
       prior releases, however, the realpath implementation below is
       potentially correct in more cases. */
    PyObject *r = NULL;
    int nlink = 0;
    wchar_t *path = PyUnicode_AsWideCharString(pathobj, NULL);
    if (!path) {
        goto done;
    }
    wchar_t *path2 = _PyMem_RawWcsdup(path);
    PyMem_Free((void *)path);
    path = path2;
    while (path) {
        wchar_t resolved[MAXPATHLEN + 1];
        int linklen = _Py_wreadlink(path, resolved, Py_ARRAY_LENGTH(resolved));
        if (linklen == -1) {
            r = PyUnicode_FromWideChar(path, -1);
            break;
        }
        if (_Py_isabs(resolved)) {
            PyMem_RawFree((void *)path);
            path = _PyMem_RawWcsdup(resolved);
        } else {
            wchar_t *s = wcsrchr(path, SEP);
            if (s) {
                *s = L'\0';
            }
            path2 = _Py_join_relfile(path, resolved);
            if (path2) {
                path2 = _Py_normpath(path2, -1);
            }
            PyMem_RawFree((void *)path);
            path = path2;
        }
        nlink++;
        /* 40 is the Linux kernel 4.2 limit */
        if (nlink >= 40) {
            PyErr_SetString(PyExc_OSError, "maximum number of symbolic links reached");
            break;
        }
    }
    if (!path) {
        PyErr_NoMemory();
    }
done:
    PyMem_RawFree((void *)path);
    return r;

#elif defined(HAVE_REALPATH)
    PyObject *r = NULL;
    struct stat st;
    const char *narrow = NULL;
    wchar_t *path = PyUnicode_AsWideCharString(pathobj, NULL);
    if (!path) {
        goto done;
    }
    narrow = Py_EncodeLocale(path, NULL);
    if (!narrow) {
        PyErr_NoMemory();
        goto done;
    }
    if (lstat(narrow, &st)) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto done;
    }
    if (!S_ISLNK(st.st_mode)) {
        Py_INCREF(pathobj);
        r = pathobj;
        goto done;
    }
    wchar_t resolved[MAXPATHLEN+1];
    if (_Py_wrealpath(path, resolved, MAXPATHLEN) == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
    } else {
        r = PyUnicode_FromWideChar(resolved, -1);
    }
done:
    PyMem_Free((void *)path);
    PyMem_Free((void *)narrow);
    return r;
#endif

    Py_INCREF(pathobj);
    return pathobj;
}


static PyMethodDef getpath_methods[] = {
    {"abspath", getpath_abspath, METH_VARARGS, NULL},
    {"basename", getpath_basename, METH_VARARGS, NULL},
    {"dirname", getpath_dirname, METH_VARARGS, NULL},
    {"hassuffix", getpath_hassuffix, METH_VARARGS, NULL},
    {"isabs", getpath_isabs, METH_VARARGS, NULL},
    {"isdir", getpath_isdir, METH_VARARGS, NULL},
    {"isfile", getpath_isfile, METH_VARARGS, NULL},
    {"isxfile", getpath_isxfile, METH_VARARGS, NULL},
    {"joinpath", getpath_joinpath, METH_VARARGS, NULL},
    {"readlines", getpath_readlines, METH_VARARGS, NULL},
    {"realpath", getpath_realpath, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};


/* Two implementations of warn() to use depending on whether warnings
   are enabled or not. */

static PyObject *
getpath_warn(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *msgobj;
    if (!PyArg_ParseTuple(args, "U", &msgobj)) {
        return NULL;
    }
    fprintf(stderr, "%s\n", PyUnicode_AsUTF8(msgobj));
    Py_RETURN_NONE;
}


static PyObject *
getpath_nowarn(PyObject *Py_UNUSED(self), PyObject *args)
{
    Py_RETURN_NONE;
}


static PyMethodDef getpath_warn_method = {"warn", getpath_warn, METH_VARARGS, NULL};
static PyMethodDef getpath_nowarn_method = {"warn", getpath_nowarn, METH_VARARGS, NULL};

/* Add the helper functions to the dict */
static int
funcs_to_dict(PyObject *dict, int warnings)
{
    for (PyMethodDef *m = getpath_methods; m->ml_name; ++m) {
        PyObject *f = PyCFunction_NewEx(m, NULL, NULL);
        if (!f) {
            return 0;
        }
        if (PyDict_SetItemString(dict, m->ml_name, f) < 0) {
            Py_DECREF(f);
            return 0;
        }
        Py_DECREF(f);
    }
    PyMethodDef *m2 = warnings ? &getpath_warn_method : &getpath_nowarn_method;
    PyObject *f = PyCFunction_NewEx(m2, NULL, NULL);
    if (!f) {
        return 0;
    }
    if (PyDict_SetItemString(dict, m2->ml_name, f) < 0) {
        Py_DECREF(f);
        return 0;
    }
    Py_DECREF(f);
    return 1;
}


/* Add a wide-character string constant to the dict */
static int
wchar_to_dict(PyObject *dict, const char *key, const wchar_t *s)
{
    PyObject *u;
    int r;
    if (s && s[0]) {
        u = PyUnicode_FromWideChar(s, -1);
        if (!u) {
            return 0;
        }
    } else {
        u = Py_None;
        Py_INCREF(u);
    }
    r = PyDict_SetItemString(dict, key, u) == 0;
    Py_DECREF(u);
    return r;
}


/* Add a narrow string constant to the dict, using default locale decoding */
static int
decode_to_dict(PyObject *dict, const char *key, const char *s)
{
    PyObject *u = NULL;
    int r;
    if (s && s[0]) {
        size_t len;
        const wchar_t *w = Py_DecodeLocale(s, &len);
        if (w) {
            u = PyUnicode_FromWideChar(w, len);
            PyMem_RawFree((void *)w);
        }
        if (!u) {
            return 0;
        }
    } else {
        u = Py_None;
        Py_INCREF(u);
    }
    r = PyDict_SetItemString(dict, key, u) == 0;
    Py_DECREF(u);
    return r;
}

/* Add an environment variable to the dict, optionally clearing it afterwards */
static int
env_to_dict(PyObject *dict, const char *key, int and_clear)
{
    PyObject *u = NULL;
    int r = 0;
    assert(strncmp(key, "ENV_", 4) == 0);
    assert(strlen(key) < 64);
#ifdef MS_WINDOWS
    wchar_t wkey[64];
    // Quick convert to wchar_t, since we know key is ASCII
    wchar_t *wp = wkey;
    for (const char *p = &key[4]; *p; ++p) {
        assert(*p < 128);
        *wp++ = *p;
    }
    *wp = L'\0';
    const wchar_t *v = _wgetenv(wkey);
    if (v) {
        u = PyUnicode_FromWideChar(v, -1);
        if (!u) {
            PyErr_Clear();
        }
    }
#else
    const char *v = getenv(&key[4]);
    if (v) {
        size_t len;
        const wchar_t *w = Py_DecodeLocale(v, &len);
        if (w) {
            u = PyUnicode_FromWideChar(w, len);
            if (!u) {
                PyErr_Clear();
            }
            PyMem_RawFree((void *)w);
        }
    }
#endif
    if (u) {
        r = PyDict_SetItemString(dict, key, u) == 0;
        Py_DECREF(u);
    } else {
        r = PyDict_SetItemString(dict, key, Py_None) == 0;
    }
    if (r && and_clear) {
#ifdef MS_WINDOWS
        _wputenv_s(wkey, L"");
#else
        unsetenv(&key[4]);
#endif
    }
    return r;
}


/* Add an integer constant to the dict */
static int
int_to_dict(PyObject *dict, const char *key, int v)
{
    PyObject *o;
    int r;
    o = PyLong_FromLong(v);
    if (!o) {
        return 0;
    }
    r = PyDict_SetItemString(dict, key, o) == 0;
    Py_DECREF(o);
    return r;
}


#ifdef MS_WINDOWS
static int
winmodule_to_dict(PyObject *dict, const char *key, HMODULE mod)
{
    wchar_t *buffer = NULL;
    for (DWORD cch = 256; buffer == NULL && cch < (1024 * 1024); cch *= 2) {
        buffer = (wchar_t*)PyMem_RawMalloc(cch * sizeof(wchar_t));
        if (buffer) {
            if (GetModuleFileNameW(mod, buffer, cch) == cch) {
                PyMem_RawFree(buffer);
                buffer = NULL;
            }
        }
    }
    int r = wchar_to_dict(dict, key, buffer);
    PyMem_RawFree(buffer);
    return r;
}
#endif


/* Add the current executable's path to the dict */
static int
progname_to_dict(PyObject *dict, const char *key)
{
#ifdef MS_WINDOWS
    return winmodule_to_dict(dict, key, NULL);
#elif defined(__APPLE__)
    char *path;
    uint32_t pathLen = 256;
    while (pathLen) {
        path = PyMem_RawMalloc((pathLen + 1) * sizeof(char));
        if (!path) {
            return 0;
        }
        if (_NSGetExecutablePath(path, &pathLen) != 0) {
            PyMem_RawFree(path);
            continue;
        }
        // Only keep if the path is absolute
        if (path[0] == SEP) {
            int r = decode_to_dict(dict, key, path);
            PyMem_RawFree(path);
            return r;
        }
        // Fall back and store None
        PyMem_RawFree(path);
        break;
    }
#endif
    return PyDict_SetItemString(dict, key, Py_None) == 0;
}


/* Add the runtime library's path to the dict */
static int
library_to_dict(PyObject *dict, const char *key)
{
#ifdef MS_WINDOWS
    extern HMODULE PyWin_DLLhModule;
    if (PyWin_DLLhModule) {
        return winmodule_to_dict(dict, key, PyWin_DLLhModule);
    }
#elif defined(WITH_NEXT_FRAMEWORK)
    static char modPath[MAXPATHLEN + 1];
    static int modPathInitialized = -1;
    if (modPathInitialized < 0) {
        modPathInitialized = 0;

        /* On Mac OS X we have a special case if we're running from a framework.
           This is because the python home should be set relative to the library,
           which is in the framework, not relative to the executable, which may
           be outside of the framework. Except when we're in the build
           directory... */
        NSSymbol symbol = NSLookupAndBindSymbol("_Py_Initialize");
        if (symbol != NULL) {
            NSModule pythonModule = NSModuleForSymbol(symbol);
            if (pythonModule != NULL) {
                /* Use dylib functions to find out where the framework was loaded from */
                const char *path = NSLibraryNameForModule(pythonModule);
                if (path) {
                    strncpy(modPath, path, MAXPATHLEN);
                    modPathInitialized = 1;
                }
            }
        }
    }
    if (modPathInitialized > 0) {
        return decode_to_dict(dict, key, modPath);
    }
#endif
    return PyDict_SetItemString(dict, key, Py_None) == 0;
}


PyObject *
_Py_Get_Getpath_CodeObject(void)
{
    return PyMarshal_ReadObjectFromString(
        (const char*)_Py_M__getpath, sizeof(_Py_M__getpath));
}


/* Perform the actual path calculation.

   When compute_path_config is 0, this only reads any initialised path
   config values into the PyConfig struct. For example, Py_SetHome() or
   Py_SetPath(). The only error should be due to failed memory allocation.

   When compute_path_config is 1, full path calculation is performed.
   The GIL must be held, and there may be filesystem access, side
   effects, and potential unraisable errors that are reported directly
   to stderr.

   Calling this function multiple times on the same PyConfig is only
   safe because already-configured values are not recalculated. To
   actually recalculate paths, you need a clean PyConfig.
*/
PyStatus
_PyConfig_InitPathConfig(PyConfig *config, int compute_path_config)
{
    PyStatus status = _PyPathConfig_ReadGlobal(config);

    if (_PyStatus_EXCEPTION(status) || !compute_path_config) {
        return status;
    }

    if (!_PyThreadState_UncheckedGet()) {
        return PyStatus_Error("cannot calculate path configuration without GIL");
    }

    PyObject *configDict = _PyConfig_AsDict(config);
    if (!configDict) {
        PyErr_Clear();
        return PyStatus_NoMemory();
    }

    PyObject *dict = PyDict_New();
    if (!dict) {
        PyErr_Clear();
        Py_DECREF(configDict);
        return PyStatus_NoMemory();
    }

    if (PyDict_SetItemString(dict, "config", configDict) < 0) {
        PyErr_Clear();
        Py_DECREF(configDict);
        Py_DECREF(dict);
        return PyStatus_NoMemory();
    }
    /* reference now held by dict */
    Py_DECREF(configDict);

    PyObject *co = _Py_Get_Getpath_CodeObject();
    if (!co || !PyCode_Check(co)) {
        PyErr_Clear();
        Py_XDECREF(co);
        Py_DECREF(dict);
        return PyStatus_Error("error reading frozen getpath.py");
    }

#ifdef MS_WINDOWS
    PyObject *winreg = PyImport_ImportModule("winreg");
    if (!winreg || PyDict_SetItemString(dict, "winreg", winreg) < 0) {
        PyErr_Clear();
        Py_XDECREF(winreg);
        if (PyDict_SetItemString(dict, "winreg", Py_None) < 0) {
            PyErr_Clear();
            Py_DECREF(co);
            Py_DECREF(dict);
            return PyStatus_Error("error importing winreg module");
        }
    } else {
        Py_DECREF(winreg);
    }
#endif

    if (
#ifdef MS_WINDOWS
        !decode_to_dict(dict, "os_name", "nt") ||
#elif defined(__APPLE__)
        !decode_to_dict(dict, "os_name", "darwin") ||
#else
        !decode_to_dict(dict, "os_name", "posix") ||
#endif
#ifdef WITH_NEXT_FRAMEWORK
        !int_to_dict(dict, "WITH_NEXT_FRAMEWORK", 1) ||
#else
        !int_to_dict(dict, "WITH_NEXT_FRAMEWORK", 0) ||
#endif
        !decode_to_dict(dict, "PREFIX", PREFIX) ||
        !decode_to_dict(dict, "EXEC_PREFIX", EXEC_PREFIX) ||
        !decode_to_dict(dict, "PYTHONPATH", PYTHONPATH) ||
        !decode_to_dict(dict, "VPATH", VPATH) ||
        !decode_to_dict(dict, "PLATLIBDIR", PLATLIBDIR) ||
        !decode_to_dict(dict, "PYDEBUGEXT", PYDEBUGEXT) ||
        !int_to_dict(dict, "VERSION_MAJOR", PY_MAJOR_VERSION) ||
        !int_to_dict(dict, "VERSION_MINOR", PY_MINOR_VERSION) ||
        !decode_to_dict(dict, "PYWINVER", PYWINVER) ||
        !wchar_to_dict(dict, "EXE_SUFFIX", EXE_SUFFIX) ||
        !env_to_dict(dict, "ENV_PATH", 0) ||
        !env_to_dict(dict, "ENV_PYTHONHOME", 0) ||
        !env_to_dict(dict, "ENV_PYTHONEXECUTABLE", 0) ||
        !env_to_dict(dict, "ENV___PYVENV_LAUNCHER__", 1) ||
        !progname_to_dict(dict, "real_executable") ||
        !library_to_dict(dict, "library") ||
        !wchar_to_dict(dict, "executable_dir", NULL) ||
        !wchar_to_dict(dict, "py_setpath", _PyPathConfig_GetGlobalModuleSearchPath()) ||
        !funcs_to_dict(dict, config->pathconfig_warnings) ||
#ifndef MS_WINDOWS
        PyDict_SetItemString(dict, "winreg", Py_None) < 0 ||
#endif
        PyDict_SetItemString(dict, "__builtins__", PyEval_GetBuiltins()) < 0
    ) {
        Py_DECREF(co);
        Py_DECREF(dict);
        _PyErr_WriteUnraisableMsg("error evaluating initial values", NULL);
        return PyStatus_Error("error evaluating initial values");
    }

    PyObject *r = PyEval_EvalCode(co, dict, dict);
    Py_DECREF(co);

    if (!r) {
        Py_DECREF(dict);
        _PyErr_WriteUnraisableMsg("error evaluating path", NULL);
        return PyStatus_Error("error evaluating path");
    }
    Py_DECREF(r);

#if 0
    PyObject *it = PyObject_GetIter(configDict);
    for (PyObject *k = PyIter_Next(it); k; k = PyIter_Next(it)) {
        if (!strcmp("__builtins__", PyUnicode_AsUTF8(k))) {
            Py_DECREF(k);
            continue;
        }
        fprintf(stderr, "%s = ", PyUnicode_AsUTF8(k));
        PyObject *o = PyDict_GetItem(configDict, k);
        o = PyObject_Repr(o);
        fprintf(stderr, "%s\n", PyUnicode_AsUTF8(o));
        Py_DECREF(o);
        Py_DECREF(k);
    }
    Py_DECREF(it);
#endif

    if (_PyConfig_FromDict(config, configDict) < 0) {
        _PyErr_WriteUnraisableMsg("reading getpath results", NULL);
        Py_DECREF(dict);
        return PyStatus_Error("error getting getpath results");
    }

    Py_DECREF(dict);

    return _PyStatus_OK();
}

