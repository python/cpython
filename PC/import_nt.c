/********************************************************************

 import_nt.c

  Win32 specific import code.

*/

#include "Python.h"
#include "osdefs.h"
#include <windows.h>
#include "importdl.h"
#include "malloc.h" /* for alloca */

/* a string loaded from the DLL at startup */
extern const char *PyWin_DLLVersionString;

/* Find a module on Windows.

   Read the registry Software\Python\PythonCore\<version>\Modules\<name> (or
   Software\Python\PythonCore\<version>\Modules\<name>\Debug in debug mode)
   from HKEY_CURRENT_USER, or HKEY_LOCAL_MACHINE. Find the file descriptor using
   the file extension. Open the file.

   On success, write the file descriptor into *ppFileDesc, the module path
   (Unicode object) into *pPath, and return the opened file object. If the
   module cannot be found (e.g. no registry key or the file doesn't exist),
   return NULL. On error, raise a Python exception and return NULL.
 */
FILE *
_PyWin_FindRegisteredModule(PyObject *moduleName,
                            struct filedescr **ppFileDesc,
                            PyObject **pPath)
{
    wchar_t pathBuf[MAXPATHLEN+1];
    int pathLen = MAXPATHLEN+1;
    PyObject *path, *moduleKey, *suffix;
    struct filedescr *fdp;
    HKEY keyBase;
    int modNameSize;
    long regStat;
    Py_ssize_t extLen;
    FILE *fp;

    moduleKey = PyUnicode_FromFormat(
#ifdef _DEBUG
        /* In debugging builds, we _must_ have the debug version registered */
        "Software\\Python\\PythonCore\\%s\\Modules\\%U\\Debug",
#else
        "Software\\Python\\PythonCore\\%s\\Modules\\%U",
#endif
        PyWin_DLLVersionString, moduleName);
    if (moduleKey == NULL)
        return NULL;

    keyBase = HKEY_CURRENT_USER;
    modNameSize = pathLen;
    regStat = RegQueryValueW(keyBase, PyUnicode_AS_UNICODE(moduleKey),
                             pathBuf, &modNameSize);
    if (regStat != ERROR_SUCCESS) {
        /* No user setting - lookup in machine settings */
        keyBase = HKEY_LOCAL_MACHINE;
        /* be anal - failure may have reset size param */
        modNameSize = pathLen;
        regStat = RegQueryValueW(keyBase, PyUnicode_AS_UNICODE(moduleKey),
                                 pathBuf, &modNameSize);
        if (regStat != ERROR_SUCCESS) {
            Py_DECREF(moduleKey);
            return NULL;
        }
    }
    Py_DECREF(moduleKey);
    if (modNameSize < 3) {
        /* path shorter than "a.o" or negative length (cast to
           size_t is wrong) */
        return NULL;
    }
    /* use the file extension to locate the type entry. */
    for (fdp = _PyImport_Filetab; fdp->suffix != NULL; fdp++) {
        suffix = PyUnicode_FromString(fdp->suffix);
        if (suffix == NULL)
            return NULL;
        extLen = PyUnicode_GET_SIZE(suffix);
        if ((Py_ssize_t)modNameSize > extLen &&
            _wcsnicmp(pathBuf + ((Py_ssize_t)modNameSize-extLen-1),
                      PyUnicode_AS_UNICODE(suffix),
                      extLen) == 0)
        {
            Py_DECREF(suffix);
            break;
        }
        Py_DECREF(suffix);
    }
    if (fdp->suffix == NULL)
        return NULL;
    path = PyUnicode_FromUnicode(pathBuf, wcslen(pathBuf));
    if (path == NULL)
        return NULL;
    fp = _Py_fopen(path, fdp->mode);
    if (fp == NULL) {
        Py_DECREF(path);
        return NULL;
    }
    *pPath = path;
    *ppFileDesc = fdp;
    return fp;
}
