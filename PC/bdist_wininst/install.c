/*
  IMPORTANT NOTE: IF THIS FILE IS CHANGED, WININST-6.EXE MUST BE RECOMPILED
  WITH THE MSVC6 WININST.DSW WORKSPACE FILE MANUALLY, AND WININST-7.1.EXE MUST
  BE RECOMPILED WITH THE MSVC 2003.NET WININST-7.1.VCPROJ FILE MANUALLY.

  IF CHANGES TO THIS FILE ARE CHECKED INTO PYTHON CVS, THE RECOMPILED BINARIES
  MUST BE CHECKED IN AS WELL!
*/

/*
 * Written by Thomas Heller, May 2000
 *
 * $Id$
 */

/*
 * Windows Installer program for distutils.
 *
 * (a kind of self-extracting zip-file)
 *
 * At runtime, the exefile has appended:
 * - compressed setup-data in ini-format, containing the following sections:
 *      [metadata]
 *      author=Greg Ward
 *      author_email=gward@python.net
 *      description=Python Distribution Utilities
 *      licence=Python
 *      name=Distutils
 *      url=http://www.python.org/sigs/distutils-sig/
 *      version=0.9pre
 *
 *      [Setup]
 *      info= text to be displayed in the edit-box
 *      title= to be displayed by this program
 *      target_version = if present, python version required
 *      pyc_compile = if 0, do not compile py to pyc
 *      pyo_compile = if 0, do not compile py to pyo
 *
 * - a struct meta_data_hdr, describing the above
 * - a zip-file, containing the modules to be installed.
 *   for the format see http://www.pkware.com/appnote.html
 *
 * What does this program do?
 * - the setup-data is uncompressed and written to a temporary file.
 * - setup-data is queried with GetPrivateProfile... calls
 * - [metadata] - info is displayed in the dialog box
 * - The registry is searched for installations of python
 * - The user can select the python version to use.
 * - The python-installation directory (sys.prefix) is displayed
 * - When the start-button is pressed, files from the zip-archive
 *   are extracted to the file system. All .py filenames are stored
 *   in a list.
 */
/*
 * Includes now an uninstaller.
 */

/*
 * To Do:
 *
 * display some explanation when no python version is found
 * instead showing the user an empty listbox to select something from.
 *
 * Finish the code so that we can use other python installations
 * additionally to those found in the registry,
 * and then #define USE_OTHER_PYTHON_VERSIONS
 *
 *  - install a help-button, which will display something meaningful
 *    to the poor user.
 *    text to the user
 *  - should there be a possibility to display a README file
 *    before starting the installation (if one is present in the archive)
 *  - more comments about what the code does(?)
 *
 *  - evolve this into a full blown installer (???)
 */

#include <windows.h>
#include <commctrl.h>
#include <imagehlp.h>
#include <objbase.h>
#include <shlobj.h>
#include <objidl.h>
#include "resource.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#include <io.h>
#include <fcntl.h>

#include "archive.h"

/* Only for debugging!
   static int dprintf(char *fmt, ...)
   {
   char Buffer[4096];
   va_list marker;
   int result;

   va_start(marker, fmt);
   result = wvsprintf(Buffer, fmt, marker);
   OutputDebugString(Buffer);
   return result;
   }
*/

/* Bah: global variables */
FILE *logfile;

char modulename[MAX_PATH];
wchar_t wmodulename[MAX_PATH];

HWND hwndMain;
HWND hDialog;

char *ini_file;                 /* Full pathname of ini-file */
/* From ini-file */
char info[4096];                /* [Setup] info= */
char title[80];                 /* [Setup] title=, contains package name
                                   including version: "Distutils-1.0.1" */
char target_version[10];        /* [Setup] target_version=, required python
                                   version or empty string */
char build_info[80];            /* [Setup] build_info=, distutils version
                                   and build date */

char meta_name[80];             /* package name without version like
                                   'Distutils' */
char install_script[MAX_PATH];
char *pre_install_script; /* run before we install a single file */

char user_access_control[10]; // one of 'auto', 'force', otherwise none.

int py_major, py_minor;         /* Python version selected for installation */

char *arc_data;                 /* memory mapped archive */
DWORD arc_size;                 /* number of bytes in archive */
int exe_size;                   /* number of bytes for exe-file portion */
char python_dir[MAX_PATH];
char pythondll[MAX_PATH];
BOOL pyc_compile, pyo_compile;
/* Either HKLM or HKCU, depending on where Python itself is registered, and
   the permissions of the current user. */
HKEY hkey_root = (HKEY)-1;

BOOL success;                   /* Installation successful? */
char *failure_reason = NULL;

HANDLE hBitmap;
char *bitmap_bytes;


#define WM_NUMFILES WM_USER+1
/* wParam: 0, lParam: total number of files */
#define WM_NEXTFILE WM_USER+2
/* wParam: number of this file */
/* lParam: points to pathname */

static BOOL notify(int code, char *fmt, ...);

/* Note: If scheme.prefix is nonempty, it must end with a '\'! */
/* Note: purelib must be the FIRST entry! */
SCHEME old_scheme[] = {
    { "PURELIB", "" },
    { "PLATLIB", "" },
    { "HEADERS", "" }, /* 'Include/dist_name' part already in archive */
    { "SCRIPTS", "Scripts\\" },
    { "DATA", "" },
    { NULL, NULL },
};

SCHEME new_scheme[] = {
    { "PURELIB", "Lib\\site-packages\\" },
    { "PLATLIB", "Lib\\site-packages\\" },
    { "HEADERS", "" }, /* 'Include/dist_name' part already in archive */
    { "SCRIPTS", "Scripts\\" },
    { "DATA", "" },
    { NULL, NULL },
};

static void unescape(char *dst, char *src, unsigned size)
{
    char *eon;
    char ch;

    while (src && *src && (size > 2)) {
        if (*src == '\\') {
            switch (*++src) {
            case 'n':
                ++src;
                *dst++ = '\r';
                *dst++ = '\n';
                size -= 2;
                break;
            case 'r':
                ++src;
                *dst++ = '\r';
                --size;
                break;
            case '0': case '1': case '2': case '3':
                ch = (char)strtol(src, &eon, 8);
                if (ch == '\n') {
                    *dst++ = '\r';
                    --size;
                }
                *dst++ = ch;
                --size;
                src = eon;
            }
        } else {
            *dst++ = *src++;
            --size;
        }
    }
    *dst = '\0';
}

static struct tagFile {
    char *path;
    struct tagFile *next;
} *file_list = NULL;

static void set_failure_reason(char *reason)
{
    if (failure_reason)
    free(failure_reason);
    failure_reason = strdup(reason);
    success = FALSE;
}
static char *get_failure_reason()
{
    if (!failure_reason)
    return "Installation failed.";
    return failure_reason;
}

static void add_to_filelist(char *path)
{
    struct tagFile *p;
    p = (struct tagFile *)malloc(sizeof(struct tagFile));
    p->path = strdup(path);
    p->next = file_list;
    file_list = p;
}

static int do_compile_files(int (__cdecl * PyRun_SimpleString)(char *),
                             int optimize)
{
    struct tagFile *p;
    int total, n;
    char Buffer[MAX_PATH + 64];
    int errors = 0;

    total = 0;
    p = file_list;
    while (p) {
        ++total;
        p = p->next;
    }
    SendDlgItemMessage(hDialog, IDC_PROGRESS, PBM_SETRANGE, 0,
                        MAKELPARAM(0, total));
    SendDlgItemMessage(hDialog, IDC_PROGRESS, PBM_SETPOS, 0, 0);

    n = 0;
    p = file_list;
    while (p) {
        ++n;
        wsprintf(Buffer,
                  "import py_compile; py_compile.compile (r'%s')",
                  p->path);
        if (PyRun_SimpleString(Buffer)) {
            ++errors;
        }
        /* We send the notification even if the files could not
         * be created so that the uninstaller will remove them
         * in case they are created later.
         */
        wsprintf(Buffer, "%s%c", p->path, optimize ? 'o' : 'c');
        notify(FILE_CREATED, Buffer);

        SendDlgItemMessage(hDialog, IDC_PROGRESS, PBM_SETPOS, n, 0);
        SetDlgItemText(hDialog, IDC_INFO, p->path);
        p = p->next;
    }
    return errors;
}

#define DECLPROC(dll, result, name, args)\
    typedef result (*__PROC__##name) args;\
    result (*name)args = (__PROC__##name)GetProcAddress(dll, #name)


#define DECLVAR(dll, type, name)\
    type *name = (type*)GetProcAddress(dll, #name)

typedef void PyObject;

// Convert a "char *" string to "whcar_t *", or NULL on error.
// Result string must be free'd
wchar_t *widen_string(char *src)
{
    wchar_t *result;
    DWORD dest_cch;
    int src_len = strlen(src) + 1; // include NULL term in all ops
    /* use MultiByteToWideChar() to see how much we need. */
    /* NOTE: this will include the null-term in the length */
    dest_cch = MultiByteToWideChar(CP_ACP, 0, src, src_len, NULL, 0);
    // alloc the buffer
    result = (wchar_t *)malloc(dest_cch * sizeof(wchar_t));
    if (result==NULL)
        return NULL;
    /* do the conversion */
    if (0==MultiByteToWideChar(CP_ACP, 0, src, src_len, result, dest_cch)) {
        free(result);
        return NULL;
    }
    return result;
}

/*
 * Returns number of files which failed to compile,
 * -1 if python could not be loaded at all
 */
static int compile_filelist(HINSTANCE hPython, BOOL optimize_flag)
{
    DECLPROC(hPython, void, Py_Initialize, (void));
    DECLPROC(hPython, void, Py_SetProgramName, (wchar_t *));
    DECLPROC(hPython, void, Py_Finalize, (void));
    DECLPROC(hPython, int, PyRun_SimpleString, (char *));
    DECLPROC(hPython, PyObject *, PySys_GetObject, (char *));
    DECLVAR(hPython, int, Py_OptimizeFlag);

    int errors = 0;
    struct tagFile *p = file_list;

    if (!p)
        return 0;

    if (!Py_Initialize || !Py_SetProgramName || !Py_Finalize)
        return -1;

    if (!PyRun_SimpleString || !PySys_GetObject || !Py_OptimizeFlag)
        return -1;

    *Py_OptimizeFlag = optimize_flag ? 1 : 0;
    Py_SetProgramName(wmodulename);
    Py_Initialize();

    errors += do_compile_files(PyRun_SimpleString, optimize_flag);
    Py_Finalize();

    return errors;
}

typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);

struct PyMethodDef {
    char        *ml_name;
    PyCFunction  ml_meth;
    int                  ml_flags;
    char        *ml_doc;
};
typedef struct PyMethodDef PyMethodDef;

// XXX - all of these are potentially fragile!  We load and unload
// the Python DLL multiple times - so storing functions pointers
// is dangerous (although things *look* OK at present)
// Better might be to roll prepare_script_environment() into
// LoadPythonDll(), and create a new UnloadPythonDLL() which also
// clears the global pointers.
void *(*g_Py_BuildValue)(char *, ...);
int (*g_PyArg_ParseTuple)(PyObject *, char *, ...);
PyObject * (*g_PyLong_FromVoidPtr)(void *);

PyObject *g_PyExc_ValueError;
PyObject *g_PyExc_OSError;

PyObject *(*g_PyErr_Format)(PyObject *, char *, ...);

#define DEF_CSIDL(name) { name, #name }

struct {
    int nFolder;
    char *name;
} csidl_names[] = {
    /* Startup menu for all users.
       NT only */
    DEF_CSIDL(CSIDL_COMMON_STARTMENU),
    /* Startup menu. */
    DEF_CSIDL(CSIDL_STARTMENU),

/*    DEF_CSIDL(CSIDL_COMMON_APPDATA), */
/*    DEF_CSIDL(CSIDL_LOCAL_APPDATA), */
    /* Repository for application-specific data.
       Needs Internet Explorer 4.0 */
    DEF_CSIDL(CSIDL_APPDATA),

    /* The desktop for all users.
       NT only */
    DEF_CSIDL(CSIDL_COMMON_DESKTOPDIRECTORY),
    /* The desktop. */
    DEF_CSIDL(CSIDL_DESKTOPDIRECTORY),

    /* Startup folder for all users.
       NT only */
    DEF_CSIDL(CSIDL_COMMON_STARTUP),
    /* Startup folder. */
    DEF_CSIDL(CSIDL_STARTUP),

    /* Programs item in the start menu for all users.
       NT only */
    DEF_CSIDL(CSIDL_COMMON_PROGRAMS),
    /* Program item in the user's start menu. */
    DEF_CSIDL(CSIDL_PROGRAMS),

/*    DEF_CSIDL(CSIDL_PROGRAM_FILES_COMMON), */
/*    DEF_CSIDL(CSIDL_PROGRAM_FILES), */

    /* Virtual folder containing fonts. */
    DEF_CSIDL(CSIDL_FONTS),
};

#define DIM(a) (sizeof(a) / sizeof((a)[0]))

static PyObject *FileCreated(PyObject *self, PyObject *args)
{
    char *path;
    if (!g_PyArg_ParseTuple(args, "s", &path))
        return NULL;
    notify(FILE_CREATED, path);
    return g_Py_BuildValue("");
}

static PyObject *DirectoryCreated(PyObject *self, PyObject *args)
{
    char *path;
    if (!g_PyArg_ParseTuple(args, "s", &path))
        return NULL;
    notify(DIR_CREATED, path);
    return g_Py_BuildValue("");
}

static PyObject *GetSpecialFolderPath(PyObject *self, PyObject *args)
{
    char *name;
    char lpszPath[MAX_PATH];
    int i;
    static HRESULT (WINAPI *My_SHGetSpecialFolderPath)(HWND hwnd,
                                                       LPTSTR lpszPath,
                                                       int nFolder,
                                                       BOOL fCreate);

    if (!My_SHGetSpecialFolderPath) {
        HINSTANCE hLib = LoadLibrary("shell32.dll");
        if (!hLib) {
            g_PyErr_Format(g_PyExc_OSError,
                           "function not available");
            return NULL;
        }
        My_SHGetSpecialFolderPath = (BOOL (WINAPI *)(HWND, LPTSTR,
                                                     int, BOOL))
            GetProcAddress(hLib,
                           "SHGetSpecialFolderPathA");
    }

    if (!g_PyArg_ParseTuple(args, "s", &name))
        return NULL;

    if (!My_SHGetSpecialFolderPath) {
        g_PyErr_Format(g_PyExc_OSError, "function not available");
        return NULL;
    }

    for (i = 0; i < DIM(csidl_names); ++i) {
        if (0 == strcmpi(csidl_names[i].name, name)) {
            int nFolder;
            nFolder = csidl_names[i].nFolder;
            if (My_SHGetSpecialFolderPath(NULL, lpszPath,
                                          nFolder, 0))
                return g_Py_BuildValue("s", lpszPath);
            else {
                g_PyErr_Format(g_PyExc_OSError,
                               "no such folder (%s)", name);
                return NULL;
            }

        }
    };
    g_PyErr_Format(g_PyExc_ValueError, "unknown CSIDL (%s)", name);
    return NULL;
}

static PyObject *CreateShortcut(PyObject *self, PyObject *args)
{
    char *path; /* path and filename */
    char *description;
    char *filename;

    char *arguments = NULL;
    char *iconpath = NULL;
    int iconindex = 0;
    char *workdir = NULL;

    WCHAR wszFilename[MAX_PATH];

    IShellLink *ps1 = NULL;
    IPersistFile *pPf = NULL;

    HRESULT hr;

    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        g_PyErr_Format(g_PyExc_OSError,
                       "CoInitialize failed, error 0x%x", hr);
        goto error;
    }

    if (!g_PyArg_ParseTuple(args, "sss|sssi",
                            &path, &description, &filename,
                            &arguments, &workdir, &iconpath, &iconindex))
        return NULL;

    hr = CoCreateInstance(&CLSID_ShellLink,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_IShellLink,
                          &ps1);
    if (FAILED(hr)) {
        g_PyErr_Format(g_PyExc_OSError,
                       "CoCreateInstance failed, error 0x%x", hr);
        goto error;
    }

    hr = ps1->lpVtbl->QueryInterface(ps1, &IID_IPersistFile,
                                     (void **)&pPf);
    if (FAILED(hr)) {
        g_PyErr_Format(g_PyExc_OSError,
                       "QueryInterface(IPersistFile) error 0x%x", hr);
        goto error;
    }


    hr = ps1->lpVtbl->SetPath(ps1, path);
    if (FAILED(hr)) {
        g_PyErr_Format(g_PyExc_OSError,
                       "SetPath() failed, error 0x%x", hr);
        goto error;
    }

    hr = ps1->lpVtbl->SetDescription(ps1, description);
    if (FAILED(hr)) {
        g_PyErr_Format(g_PyExc_OSError,
                       "SetDescription() failed, error 0x%x", hr);
        goto error;
    }

    if (arguments) {
        hr = ps1->lpVtbl->SetArguments(ps1, arguments);
        if (FAILED(hr)) {
            g_PyErr_Format(g_PyExc_OSError,
                           "SetArguments() error 0x%x", hr);
            goto error;
        }
    }

    if (iconpath) {
        hr = ps1->lpVtbl->SetIconLocation(ps1, iconpath, iconindex);
        if (FAILED(hr)) {
            g_PyErr_Format(g_PyExc_OSError,
                           "SetIconLocation() error 0x%x", hr);
            goto error;
        }
    }

    if (workdir) {
        hr = ps1->lpVtbl->SetWorkingDirectory(ps1, workdir);
        if (FAILED(hr)) {
            g_PyErr_Format(g_PyExc_OSError,
                           "SetWorkingDirectory() error 0x%x", hr);
            goto error;
        }
    }

    MultiByteToWideChar(CP_ACP, 0,
                        filename, -1,
                        wszFilename, MAX_PATH);

    hr = pPf->lpVtbl->Save(pPf, wszFilename, TRUE);
    if (FAILED(hr)) {
        g_PyErr_Format(g_PyExc_OSError,
                       "Failed to create shortcut '%s' - error 0x%x", filename, hr);
        goto error;
    }

    pPf->lpVtbl->Release(pPf);
    ps1->lpVtbl->Release(ps1);
    CoUninitialize();
    return g_Py_BuildValue("");

  error:
    if (pPf)
        pPf->lpVtbl->Release(pPf);

    if (ps1)
        ps1->lpVtbl->Release(ps1);

    CoUninitialize();

    return NULL;
}

static PyObject *PyMessageBox(PyObject *self, PyObject *args)
{
    int rc;
    char *text, *caption;
    int flags;
    if (!g_PyArg_ParseTuple(args, "ssi", &text, &caption, &flags))
        return NULL;
    rc = MessageBox(GetFocus(), text, caption, flags);
    return g_Py_BuildValue("i", rc);
}

static PyObject *GetRootHKey(PyObject *self)
{
    return g_PyLong_FromVoidPtr(hkey_root);
}

#define METH_VARARGS 0x0001
#define METH_NOARGS   0x0004
typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);

PyMethodDef meth[] = {
    {"create_shortcut", CreateShortcut, METH_VARARGS, NULL},
    {"get_special_folder_path", GetSpecialFolderPath, METH_VARARGS, NULL},
    {"get_root_hkey", (PyCFunction)GetRootHKey, METH_NOARGS, NULL},
    {"file_created", FileCreated, METH_VARARGS, NULL},
    {"directory_created", DirectoryCreated, METH_VARARGS, NULL},
    {"message_box", PyMessageBox, METH_VARARGS, NULL},
};

static HINSTANCE LoadPythonDll(char *fname)
{
    char fullpath[_MAX_PATH];
    LONG size = sizeof(fullpath);
    char subkey_name[80];
    char buffer[260 + 12];
    HINSTANCE h;

    /* make sure PYTHONHOME is set, to that sys.path is initialized correctly */
    wsprintf(buffer, "PYTHONHOME=%s", python_dir);
    _putenv(buffer);
    h = LoadLibrary(fname);
    if (h)
        return h;
    wsprintf(subkey_name,
             "SOFTWARE\\Python\\PythonCore\\%d.%d\\InstallPath",
             py_major, py_minor);
    if (ERROR_SUCCESS != RegQueryValue(HKEY_CURRENT_USER, subkey_name,
                                       fullpath, &size) &&
        ERROR_SUCCESS != RegQueryValue(HKEY_LOCAL_MACHINE, subkey_name,
                                       fullpath, &size))
        return NULL;
    strcat(fullpath, "\\");
    strcat(fullpath, fname);
    return LoadLibrary(fullpath);
}

static int prepare_script_environment(HINSTANCE hPython)
{
    PyObject *mod;
    DECLPROC(hPython, PyObject *, PyImport_ImportModule, (char *));
    DECLPROC(hPython, int, PyObject_SetAttrString, (PyObject *, char *, PyObject *));
    DECLPROC(hPython, PyObject *, PyObject_GetAttrString, (PyObject *, char *));
    DECLPROC(hPython, PyObject *, PyCFunction_New, (PyMethodDef *, PyObject *));
    DECLPROC(hPython, PyObject *, Py_BuildValue, (char *, ...));
    DECLPROC(hPython, int, PyArg_ParseTuple, (PyObject *, char *, ...));
    DECLPROC(hPython, PyObject *, PyErr_Format, (PyObject *, char *));
    DECLPROC(hPython, PyObject *, PyLong_FromVoidPtr, (void *));
    if (!PyImport_ImportModule || !PyObject_GetAttrString ||
        !PyObject_SetAttrString || !PyCFunction_New)
        return 1;
    if (!Py_BuildValue || !PyArg_ParseTuple || !PyErr_Format)
        return 1;

    mod = PyImport_ImportModule("builtins");
    if (mod) {
        int i;
        g_PyExc_ValueError = PyObject_GetAttrString(mod, "ValueError");
        g_PyExc_OSError = PyObject_GetAttrString(mod, "OSError");
        for (i = 0; i < DIM(meth); ++i) {
            PyObject_SetAttrString(mod, meth[i].ml_name,
                                   PyCFunction_New(&meth[i], NULL));
        }
    }
    g_Py_BuildValue = Py_BuildValue;
    g_PyArg_ParseTuple = PyArg_ParseTuple;
    g_PyErr_Format = PyErr_Format;
    g_PyLong_FromVoidPtr = PyLong_FromVoidPtr;

    return 0;
}

/*
 * This function returns one of the following error codes:
 * 1 if the Python-dll does not export the functions we need
 * 2 if no install-script is specified in pathname
 * 3 if the install-script file could not be opened
 * the return value of PyRun_SimpleString() otherwise,
 * which is 0 if everything is ok, -1 if an exception had occurred
 * in the install-script.
 */

static int
do_run_installscript(HINSTANCE hPython, char *pathname, int argc, char **argv)
{
    int fh, result, i;
    static wchar_t *wargv[256];
    DECLPROC(hPython, void, Py_Initialize, (void));
    DECLPROC(hPython, int, PySys_SetArgv, (int, wchar_t **));
    DECLPROC(hPython, int, PyRun_SimpleString, (char *));
    DECLPROC(hPython, void, Py_Finalize, (void));
    DECLPROC(hPython, PyObject *, Py_BuildValue, (char *, ...));
    DECLPROC(hPython, PyObject *, PyCFunction_New,
             (PyMethodDef *, PyObject *));
    DECLPROC(hPython, int, PyArg_ParseTuple, (PyObject *, char *, ...));
    DECLPROC(hPython, PyObject *, PyErr_Format, (PyObject *, char *));

    if (!Py_Initialize || !PySys_SetArgv
        || !PyRun_SimpleString || !Py_Finalize)
        return 1;

    if (!Py_BuildValue || !PyArg_ParseTuple || !PyErr_Format)
        return 1;

    if (!PyCFunction_New || !PyArg_ParseTuple || !PyErr_Format)
        return 1;

    if (pathname == NULL || pathname[0] == '\0')
        return 2;

    fh = open(pathname, _O_RDONLY);
    if (-1 == fh) {
        fprintf(stderr, "Could not open postinstall-script %s\n",
            pathname);
        return 3;
    }

    SetDlgItemText(hDialog, IDC_INFO, "Running Script...");

    Py_Initialize();

    prepare_script_environment(hPython);
    // widen the argv array for py3k.
    memset(wargv, 0, sizeof(wargv));
    for (i=0;i<argc;i++)
        wargv[i] = argv[i] ? widen_string(argv[i]) : NULL;
    PySys_SetArgv(argc, wargv);
    // free the strings we just widened.
    for (i=0;i<argc;i++)
        if (wargv[i])
            free(wargv[i]);

    result = 3;
    {
        struct _stat statbuf;
        if(0 == _fstat(fh, &statbuf)) {
            char *script = alloca(statbuf.st_size + 5);
            int n = read(fh, script, statbuf.st_size);
            if (n > 0) {
                script[n] = '\n';
                script[n+1] = 0;
                result = PyRun_SimpleString(script);
            }
        }
    }
    Py_Finalize();

    close(fh);
    return result;
}

static int
run_installscript(char *pathname, int argc, char **argv, char **pOutput)
{
    HINSTANCE hPython;
    int result = 1;
    int out_buf_size;
    HANDLE redirected, old_stderr, old_stdout;
    char *tempname;

    *pOutput = NULL;

    tempname = tempnam(NULL, NULL);
    // We use a static CRT while the Python version we load uses
    // the CRT from one of various possible DLLs.  As a result we
    // need to redirect the standard handles using the API rather
    // than the CRT.
    redirected = CreateFile(
                                    tempname,
                                    GENERIC_WRITE | GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                                    NULL);
    old_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    old_stderr = GetStdHandle(STD_ERROR_HANDLE);
    SetStdHandle(STD_OUTPUT_HANDLE, redirected);
    SetStdHandle(STD_ERROR_HANDLE, redirected);

    hPython = LoadPythonDll(pythondll);
    if (hPython) {
        result = do_run_installscript(hPython, pathname, argc, argv);
        FreeLibrary(hPython);
    } else {
        fprintf(stderr, "*** Could not load Python ***");
    }
    SetStdHandle(STD_OUTPUT_HANDLE, old_stdout);
    SetStdHandle(STD_ERROR_HANDLE, old_stderr);
    out_buf_size = min(GetFileSize(redirected, NULL), 4096);
    *pOutput = malloc(out_buf_size+1);
    if (*pOutput) {
        DWORD nread = 0;
        SetFilePointer(redirected, 0, 0, FILE_BEGIN);
        ReadFile(redirected, *pOutput, out_buf_size, &nread, NULL);
        (*pOutput)[nread] = '\0';
    }
    CloseHandle(redirected);
    DeleteFile(tempname);
    return result;
}

static int do_run_simple_script(HINSTANCE hPython, char *script)
{
    int rc;
    DECLPROC(hPython, void, Py_Initialize, (void));
    DECLPROC(hPython, void, Py_SetProgramName, (wchar_t *));
    DECLPROC(hPython, void, Py_Finalize, (void));
    DECLPROC(hPython, int, PyRun_SimpleString, (char *));
    DECLPROC(hPython, void, PyErr_Print, (void));

    if (!Py_Initialize || !Py_SetProgramName || !Py_Finalize ||
        !PyRun_SimpleString || !PyErr_Print)
        return -1;

    Py_SetProgramName(wmodulename);
    Py_Initialize();
    prepare_script_environment(hPython);
    rc = PyRun_SimpleString(script);
    if (rc)
        PyErr_Print();
    Py_Finalize();
    return rc;
}

static int run_simple_script(char *script)
{
    int rc;
    HINSTANCE hPython;
    char *tempname = tempnam(NULL, NULL);
    // Redirect output using win32 API - see comments above...
    HANDLE redirected = CreateFile(
                                    tempname,
                                    GENERIC_WRITE | GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                                    NULL);
    HANDLE old_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE old_stderr = GetStdHandle(STD_ERROR_HANDLE);
    SetStdHandle(STD_OUTPUT_HANDLE, redirected);
    SetStdHandle(STD_ERROR_HANDLE, redirected);

    hPython = LoadPythonDll(pythondll);
    if (!hPython) {
        char reason[128];
        wsprintf(reason, "Can't load Python for pre-install script (%d)", GetLastError());
        set_failure_reason(reason);
        return -1;
    }
    rc = do_run_simple_script(hPython, script);
    FreeLibrary(hPython);
    SetStdHandle(STD_OUTPUT_HANDLE, old_stdout);
    SetStdHandle(STD_ERROR_HANDLE, old_stderr);
    /* We only care about the output when we fail.  If the script works
       OK, then we discard it
    */
    if (rc) {
        int err_buf_size;
        char *err_buf;
        const char *prefix = "Running the pre-installation script failed\r\n";
        int prefix_len = strlen(prefix);
        err_buf_size = GetFileSize(redirected, NULL);
        if (err_buf_size==INVALID_FILE_SIZE) // an error - let's try anyway...
            err_buf_size = 4096;
        err_buf = malloc(prefix_len + err_buf_size + 1);
        if (err_buf) {
            DWORD n = 0;
            strcpy(err_buf, prefix);
            SetFilePointer(redirected, 0, 0, FILE_BEGIN);
            ReadFile(redirected, err_buf+prefix_len, err_buf_size, &n, NULL);
            err_buf[prefix_len+n] = '\0';
            set_failure_reason(err_buf);
            free(err_buf);
        } else {
            set_failure_reason("Out of memory!");
        }
    }
    CloseHandle(redirected);
    DeleteFile(tempname);
    return rc;
}


static BOOL SystemError(int error, char *msg)
{
    char Buffer[1024];
    int n;

    if (error) {
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&lpMsgBuf,
            0,
            NULL
            );
        strncpy(Buffer, lpMsgBuf, sizeof(Buffer));
        LocalFree(lpMsgBuf);
    } else
        Buffer[0] = '\0';
    n = lstrlen(Buffer);
    _snprintf(Buffer+n, sizeof(Buffer)-n, msg);
    MessageBox(hwndMain, Buffer, "Runtime Error", MB_OK | MB_ICONSTOP);
    return FALSE;
}

static BOOL notify (int code, char *fmt, ...)
{
    char Buffer[1024];
    va_list marker;
    BOOL result = TRUE;
    int a, b;
    char *cp;

    va_start(marker, fmt);
    _vsnprintf(Buffer, sizeof(Buffer), fmt, marker);

    switch (code) {
/* Questions */
    case CAN_OVERWRITE:
        break;

/* Information notification */
    case DIR_CREATED:
        if (logfile)
            fprintf(logfile, "100 Made Dir: %s\n", fmt);
        break;

    case FILE_CREATED:
        if (logfile)
            fprintf(logfile, "200 File Copy: %s\n", fmt);
        goto add_to_filelist_label;
        break;

    case FILE_OVERWRITTEN:
        if (logfile)
            fprintf(logfile, "200 File Overwrite: %s\n", fmt);
      add_to_filelist_label:
        if ((cp = strrchr(fmt, '.')) && (0 == strcmp (cp, ".py")))
            add_to_filelist(fmt);
        break;

/* Error Messages */
    case ZLIB_ERROR:
        MessageBox(GetFocus(), Buffer, "Error",
                    MB_OK | MB_ICONWARNING);
        break;

    case SYSTEM_ERROR:
        SystemError(GetLastError(), Buffer);
        break;

    case NUM_FILES:
        a = va_arg(marker, int);
        b = va_arg(marker, int);
        SendMessage(hDialog, WM_NUMFILES, 0, MAKELPARAM(0, a));
        SendMessage(hDialog, WM_NEXTFILE, b,(LPARAM)fmt);
    }
    va_end(marker);

    return result;
}

static char *MapExistingFile(char *pathname, DWORD *psize)
{
    HANDLE hFile, hFileMapping;
    DWORD nSizeLow, nSizeHigh;
    char *data;

    hFile = CreateFile(pathname,
                        GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;
    nSizeLow = GetFileSize(hFile, &nSizeHigh);
    hFileMapping = CreateFileMapping(hFile,
                                      NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(hFile);

    if (hFileMapping == INVALID_HANDLE_VALUE)
        return NULL;

    data = MapViewOfFile(hFileMapping,
                          FILE_MAP_READ, 0, 0, 0);

    CloseHandle(hFileMapping);
    *psize = nSizeLow;
    return data;
}


static void create_bitmap(HWND hwnd)
{
    BITMAPFILEHEADER *bfh;
    BITMAPINFO *bi;
    HDC hdc;

    if (!bitmap_bytes)
        return;

    if (hBitmap)
        return;

    hdc = GetDC(hwnd);

    bfh = (BITMAPFILEHEADER *)bitmap_bytes;
    bi = (BITMAPINFO *)(bitmap_bytes + sizeof(BITMAPFILEHEADER));

    hBitmap = CreateDIBitmap(hdc,
                             &bi->bmiHeader,
                             CBM_INIT,
                             bitmap_bytes + bfh->bfOffBits,
                             bi,
                             DIB_RGB_COLORS);
    ReleaseDC(hwnd, hdc);
}

/* Extract everything we need to begin the installation.  Currently this
   is the INI filename with install data, and the raw pre-install script
*/
static BOOL ExtractInstallData(char *data, DWORD size, int *pexe_size,
                               char **out_ini_file, char **out_preinstall_script)
{
    /* read the end of central directory record */
    struct eof_cdir *pe = (struct eof_cdir *)&data[size - sizeof
                                                   (struct eof_cdir)];

    int arc_start = size - sizeof (struct eof_cdir) - pe->nBytesCDir -
        pe->ofsCDir;

    int ofs = arc_start - sizeof (struct meta_data_hdr);

    /* read meta_data info */
    struct meta_data_hdr *pmd = (struct meta_data_hdr *)&data[ofs];
    char *src, *dst;
    char *ini_file;
    char tempdir[MAX_PATH];

    /* ensure that if we fail, we don't have garbage out pointers */
    *out_ini_file = *out_preinstall_script = NULL;

    if (pe->tag != 0x06054b50) {
        return FALSE;
    }

    if (pmd->tag != 0x1234567B) {
        return SystemError(0,
                   "Invalid cfgdata magic number (see bdist_wininst.py)");
    }
    if (ofs < 0) {
        return FALSE;
    }

    if (pmd->bitmap_size) {
        /* Store pointer to bitmap bytes */
        bitmap_bytes = (char *)pmd - pmd->uncomp_size - pmd->bitmap_size;
    }

    *pexe_size = ofs - pmd->uncomp_size - pmd->bitmap_size;

    src = ((char *)pmd) - pmd->uncomp_size;
    ini_file = malloc(MAX_PATH); /* will be returned, so do not free it */
    if (!ini_file)
        return FALSE;
    if (!GetTempPath(sizeof(tempdir), tempdir)
        || !GetTempFileName(tempdir, "~du", 0, ini_file)) {
        SystemError(GetLastError(),
                     "Could not create temporary file");
        return FALSE;
    }

    dst = map_new_file(CREATE_ALWAYS, ini_file, NULL, pmd->uncomp_size,
                        0, 0, NULL/*notify*/);
    if (!dst)
        return FALSE;
    /* Up to the first \0 is the INI file data. */
    strncpy(dst, src, pmd->uncomp_size);
    src += strlen(dst) + 1;
    /* Up to next \0 is the pre-install script */
    *out_preinstall_script = strdup(src);
    *out_ini_file = ini_file;
    UnmapViewOfFile(dst);
    return TRUE;
}

static void PumpMessages(void)
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    HFONT hFont;
    int h;
    PAINTSTRUCT ps;
    switch (msg) {
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        h = GetSystemMetrics(SM_CYSCREEN) / 10;
        hFont = CreateFont(h, 0, 0, 0, 700, TRUE,
                            0, 0, 0, 0, 0, 0, 0, "Times Roman");
        hFont = SelectObject(hdc, hFont);
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc, 15, 15, title, strlen(title));
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOut(hdc, 10, 10, title, strlen(title));
        DeleteObject(SelectObject(hdc, hFont));
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static HWND CreateBackground(char *title)
{
    WNDCLASS wc;
    HWND hwnd;
    char buffer[4096];

    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbWndExtra = 0;
    wc.cbClsExtra = 0;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 128));
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "SetupWindowClass";

    if (!RegisterClass(&wc))
        MessageBox(hwndMain,
                    "Could not register window class",
                    "Setup.exe", MB_OK);

    wsprintf(buffer, "Setup %s", title);
    hwnd = CreateWindow("SetupWindowClass",
                         buffer,
                         0,
                         0, 0,
                         GetSystemMetrics(SM_CXFULLSCREEN),
                         GetSystemMetrics(SM_CYFULLSCREEN),
                         NULL,
                         NULL,
                         GetModuleHandle(NULL),
                         NULL);
    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    UpdateWindow(hwnd);
    return hwnd;
}

/*
 * Center a window on the screen
 */
static void CenterWindow(HWND hwnd)
{
    RECT rc;
    int w, h;

    GetWindowRect(hwnd, &rc);
    w = GetSystemMetrics(SM_CXSCREEN);
    h = GetSystemMetrics(SM_CYSCREEN);
    MoveWindow(hwnd,
               (w - (rc.right-rc.left))/2,
               (h - (rc.bottom-rc.top))/2,
                rc.right-rc.left, rc.bottom-rc.top, FALSE);
}

#include <prsht.h>

BOOL CALLBACK
IntroDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR lpnm;
    char Buffer[4096];

    switch (msg) {
    case WM_INITDIALOG:
        create_bitmap(hwnd);
        if(hBitmap)
            SendDlgItemMessage(hwnd, IDC_BITMAP, STM_SETIMAGE,
                               IMAGE_BITMAP, (LPARAM)hBitmap);
        CenterWindow(GetParent(hwnd));
        wsprintf(Buffer,
                  "This Wizard will install %s on your computer. "
                  "Click Next to continue "
                  "or Cancel to exit the Setup Wizard.",
                  meta_name);
        SetDlgItemText(hwnd, IDC_TITLE, Buffer);
        SetDlgItemText(hwnd, IDC_INTRO_TEXT, info);
        SetDlgItemText(hwnd, IDC_BUILD_INFO, build_info);
        return FALSE;

    case WM_NOTIFY:
        lpnm = (LPNMHDR) lParam;

        switch (lpnm->code) {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_NEXT);
            break;

        case PSN_WIZNEXT:
            break;

        case PSN_RESET:
            break;

        default:
            break;
        }
    }
    return FALSE;
}

#ifdef USE_OTHER_PYTHON_VERSIONS
/* These are really private variables used to communicate
 * between StatusRoutine and CheckPythonExe
 */
char bound_image_dll[_MAX_PATH];
int bound_image_major;
int bound_image_minor;

static BOOL __stdcall StatusRoutine(IMAGEHLP_STATUS_REASON reason,
                                    PSTR ImageName,
                                    PSTR DllName,
                                    ULONG Va,
                                    ULONG Parameter)
{
    char fname[_MAX_PATH];
    int int_version;

    switch(reason) {
    case BindOutOfMemory:
    case BindRvaToVaFailed:
    case BindNoRoomInImage:
    case BindImportProcedureFailed:
        break;

    case BindImportProcedure:
    case BindForwarder:
    case BindForwarderNOT:
    case BindImageModified:
    case BindExpandFileHeaders:
    case BindImageComplete:
    case BindSymbolsNotUpdated:
    case BindMismatchedSymbols:
    case BindImportModuleFailed:
        break;

    case BindImportModule:
        if (1 == sscanf(DllName, "python%d", &int_version)) {
            SearchPath(NULL, DllName, NULL, sizeof(fname),
                       fname, NULL);
            strcpy(bound_image_dll, fname);
            bound_image_major = int_version / 10;
            bound_image_minor = int_version % 10;
            OutputDebugString("BOUND ");
            OutputDebugString(fname);
            OutputDebugString("\n");
        }
        break;
    }
    return TRUE;
}

/*
 */
static LPSTR get_sys_prefix(LPSTR exe, LPSTR dll)
{
    void (__cdecl * Py_Initialize)(void);
    void (__cdecl * Py_SetProgramName)(char *);
    void (__cdecl * Py_Finalize)(void);
    void* (__cdecl * PySys_GetObject)(char *);
    void (__cdecl * PySys_SetArgv)(int, char **);
    char* (__cdecl * Py_GetPrefix)(void);
    char* (__cdecl * Py_GetPath)(void);
    HINSTANCE hPython;
    LPSTR prefix = NULL;
    int (__cdecl * PyRun_SimpleString)(char *);

    {
        char Buffer[256];
        wsprintf(Buffer, "PYTHONHOME=%s", exe);
        *strrchr(Buffer, '\\') = '\0';
//      MessageBox(GetFocus(), Buffer, "PYTHONHOME", MB_OK);
                _putenv(Buffer);
                _putenv("PYTHONPATH=");
    }

    hPython = LoadLibrary(dll);
    if (!hPython)
        return NULL;
    Py_Initialize = (void (*)(void))GetProcAddress
        (hPython,"Py_Initialize");

    PySys_SetArgv = (void (*)(int, char **))GetProcAddress
        (hPython,"PySys_SetArgv");

    PyRun_SimpleString = (int (*)(char *))GetProcAddress
        (hPython,"PyRun_SimpleString");

    Py_SetProgramName = (void (*)(char *))GetProcAddress
        (hPython,"Py_SetProgramName");

    PySys_GetObject = (void* (*)(char *))GetProcAddress
        (hPython,"PySys_GetObject");

    Py_GetPrefix = (char * (*)(void))GetProcAddress
        (hPython,"Py_GetPrefix");

    Py_GetPath = (char * (*)(void))GetProcAddress
        (hPython,"Py_GetPath");

    Py_Finalize = (void (*)(void))GetProcAddress(hPython,
                                                  "Py_Finalize");
    Py_SetProgramName(exe);
    Py_Initialize();
    PySys_SetArgv(1, &exe);

    MessageBox(GetFocus(), Py_GetPrefix(), "PREFIX", MB_OK);
    MessageBox(GetFocus(), Py_GetPath(), "PATH", MB_OK);

    Py_Finalize();
    FreeLibrary(hPython);

    return prefix;
}

static BOOL
CheckPythonExe(LPSTR pathname, LPSTR version, int *pmajor, int *pminor)
{
    bound_image_dll[0] = '\0';
    if (!BindImageEx(BIND_NO_BOUND_IMPORTS | BIND_NO_UPDATE | BIND_ALL_IMAGES,
                     pathname,
                     NULL,
                     NULL,
                     StatusRoutine))
        return SystemError(0, "Could not bind image");
    if (bound_image_dll[0] == '\0')
        return SystemError(0, "Does not seem to be a python executable");
    *pmajor = bound_image_major;
    *pminor = bound_image_minor;
    if (version && *version) {
        char core_version[12];
        wsprintf(core_version, "%d.%d", bound_image_major, bound_image_minor);
        if (strcmp(version, core_version))
            return SystemError(0, "Wrong Python version");
    }
    get_sys_prefix(pathname, bound_image_dll);
    return TRUE;
}

/*
 * Browse for other python versions. Insert it into the listbox specified
 * by hwnd. version, if not NULL or empty, is the version required.
 */
static BOOL GetOtherPythonVersion(HWND hwnd, LPSTR version)
{
    char vers_name[_MAX_PATH + 80];
    DWORD itemindex;
    OPENFILENAME of;
    char pathname[_MAX_PATH];
    DWORD result;

    strcpy(pathname, "python.exe");

    memset(&of, 0, sizeof(of));
    of.lStructSize = sizeof(OPENFILENAME);
    of.hwndOwner = GetParent(hwnd);
    of.hInstance = NULL;
    of.lpstrFilter = "python.exe\0python.exe\0";
    of.lpstrCustomFilter = NULL;
    of.nMaxCustFilter = 0;
    of.nFilterIndex = 1;
    of.lpstrFile = pathname;
    of.nMaxFile = sizeof(pathname);
    of.lpstrFileTitle = NULL;
    of.nMaxFileTitle = 0;
    of.lpstrInitialDir = NULL;
    of.lpstrTitle = "Python executable";
    of.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    of.lpstrDefExt = "exe";

    result = GetOpenFileName(&of);
    if (result) {
        int major, minor;
        if (!CheckPythonExe(pathname, version, &major, &minor)) {
            return FALSE;
        }
        *strrchr(pathname, '\\') = '\0';
        wsprintf(vers_name, "Python Version %d.%d in %s",
                  major, minor, pathname);
        itemindex = SendMessage(hwnd, LB_INSERTSTRING, -1,
                                (LPARAM)(LPSTR)vers_name);
        SendMessage(hwnd, LB_SETCURSEL, itemindex, 0);
        SendMessage(hwnd, LB_SETITEMDATA, itemindex,
                    (LPARAM)(LPSTR)strdup(pathname));
        return TRUE;
    }
    return FALSE;
}
#endif /* USE_OTHER_PYTHON_VERSIONS */

typedef struct _InstalledVersionInfo {
    char prefix[MAX_PATH+1]; // sys.prefix directory.
    HKEY hkey; // Is this Python in HKCU or HKLM?
} InstalledVersionInfo;


/*
 * Fill the listbox specified by hwnd with all python versions found
 * in the registry. version, if not NULL or empty, is the version
 * required.
 */
static BOOL GetPythonVersions(HWND hwnd, HKEY hkRoot, LPSTR version)
{
    DWORD index = 0;
    char core_version[80];
    HKEY hKey;
    BOOL result = TRUE;
    DWORD bufsize;

    if (ERROR_SUCCESS != RegOpenKeyEx(hkRoot,
                                       "Software\\Python\\PythonCore",
                                       0,       KEY_READ, &hKey))
        return FALSE;
    bufsize = sizeof(core_version);
    while (ERROR_SUCCESS == RegEnumKeyEx(hKey, index,
                                          core_version, &bufsize, NULL,
                                          NULL, NULL, NULL)) {
        char subkey_name[80], vers_name[80];
        int itemindex;
        DWORD value_size;
        HKEY hk;

        bufsize = sizeof(core_version);
        ++index;
        if (version && *version && strcmp(version, core_version))
            continue;

        wsprintf(vers_name, "Python Version %s (found in registry)",
                  core_version);
        wsprintf(subkey_name,
                  "Software\\Python\\PythonCore\\%s\\InstallPath",
                  core_version);
        if (ERROR_SUCCESS == RegOpenKeyEx(hkRoot, subkey_name, 0, KEY_READ, &hk)) {
            InstalledVersionInfo *ivi =
                  (InstalledVersionInfo *)malloc(sizeof(InstalledVersionInfo));
            value_size = sizeof(ivi->prefix);
            if (ivi &&
                ERROR_SUCCESS == RegQueryValueEx(hk, NULL, NULL, NULL,
                                                 ivi->prefix, &value_size)) {
                itemindex = SendMessage(hwnd, LB_ADDSTRING, 0,
                                        (LPARAM)(LPSTR)vers_name);
                ivi->hkey = hkRoot;
                SendMessage(hwnd, LB_SETITEMDATA, itemindex,
                            (LPARAM)(LPSTR)ivi);
            }
            RegCloseKey(hk);
        }
    }
    RegCloseKey(hKey);
    return result;
}

/* Determine if the current user can write to HKEY_LOCAL_MACHINE */
BOOL HasLocalMachinePrivs()
{
                HKEY hKey;
                DWORD result;
                static char KeyName[] =
                        "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";

                result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                          KeyName,
                                          0,
                                          KEY_CREATE_SUB_KEY,
                                          &hKey);
                if (result==0)
                        RegCloseKey(hKey);
                return result==0;
}

// Check the root registry key to use - either HKLM or HKCU.
// If Python is installed in HKCU, then our extension also must be installed
// in HKCU - as Python won't be available for other users, we shouldn't either
// (and will fail if we are!)
// If Python is installed in HKLM, then we will also prefer to use HKLM, but
// this may not be possible - so we silently fall back to HKCU.
//
// We assume hkey_root is already set to where Python itself is installed.
void CheckRootKey(HWND hwnd)
{
    if (hkey_root==HKEY_CURRENT_USER) {
        ; // as above, always install ourself in HKCU too.
    } else if (hkey_root==HKEY_LOCAL_MACHINE) {
        // Python in HKLM, but we may or may not have permissions there.
        // Open the uninstall key with 'create' permissions - if this fails,
        // we don't have permission.
        if (!HasLocalMachinePrivs())
            hkey_root = HKEY_CURRENT_USER;
    } else {
        MessageBox(hwnd, "Don't know Python's installation type",
                           "Strange", MB_OK | MB_ICONSTOP);
        /* Default to wherever they can, but preferring HKLM */
        hkey_root = HasLocalMachinePrivs() ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    }
}

/* Return the installation scheme depending on Python version number */
SCHEME *GetScheme(int major, int minor)
{
    if (major > 2)
        return new_scheme;
    else if((major == 2) && (minor >= 2))
        return new_scheme;
    return old_scheme;
}

BOOL CALLBACK
SelectPythonDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR lpnm;

    switch (msg) {
    case WM_INITDIALOG:
        if (hBitmap)
            SendDlgItemMessage(hwnd, IDC_BITMAP, STM_SETIMAGE,
                               IMAGE_BITMAP, (LPARAM)hBitmap);
        GetPythonVersions(GetDlgItem(hwnd, IDC_VERSIONS_LIST),
                           HKEY_LOCAL_MACHINE, target_version);
        GetPythonVersions(GetDlgItem(hwnd, IDC_VERSIONS_LIST),
                           HKEY_CURRENT_USER, target_version);
        {               /* select the last entry which is the highest python
                   version found */
            int count;
            count = SendDlgItemMessage(hwnd, IDC_VERSIONS_LIST,
                                        LB_GETCOUNT, 0, 0);
            if (count && count != LB_ERR)
                SendDlgItemMessage(hwnd, IDC_VERSIONS_LIST, LB_SETCURSEL,
                                    count-1, 0);

            /* If a specific Python version is required,
             * display a prominent notice showing this fact.
             */
            if (target_version && target_version[0]) {
                char buffer[4096];
                wsprintf(buffer,
                         "Python %s is required for this package. "
                         "Select installation to use:",
                         target_version);
                SetDlgItemText(hwnd, IDC_TITLE, buffer);
            }

            if (count == 0) {
                char Buffer[4096];
                char *msg;
                if (target_version && target_version[0]) {
                    wsprintf(Buffer,
                             "Python version %s required, which was not found"
                             " in the registry.", target_version);
                    msg = Buffer;
                } else
                    msg = "No Python installation found in the registry.";
                MessageBox(hwnd, msg, "Cannot install",
                           MB_OK | MB_ICONSTOP);
            }
        }
        goto UpdateInstallDir;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
/*
  case IDC_OTHERPYTHON:
  if (GetOtherPythonVersion(GetDlgItem(hwnd, IDC_VERSIONS_LIST),
  target_version))
  goto UpdateInstallDir;
  break;
*/
        case IDC_VERSIONS_LIST:
            switch (HIWORD(wParam)) {
                int id;
            case LBN_SELCHANGE:
              UpdateInstallDir:
                PropSheet_SetWizButtons(GetParent(hwnd),
                                        PSWIZB_BACK | PSWIZB_NEXT);
                id = SendDlgItemMessage(hwnd, IDC_VERSIONS_LIST,
                                         LB_GETCURSEL, 0, 0);
                if (id == LB_ERR) {
                    PropSheet_SetWizButtons(GetParent(hwnd),
                                            PSWIZB_BACK);
                    SetDlgItemText(hwnd, IDC_PATH, "");
                    SetDlgItemText(hwnd, IDC_INSTALL_PATH, "");
                    strcpy(python_dir, "");
                    strcpy(pythondll, "");
                } else {
                    char *pbuf;
                    int result;
                    InstalledVersionInfo *ivi;
                    PropSheet_SetWizButtons(GetParent(hwnd),
                                            PSWIZB_BACK | PSWIZB_NEXT);
                    /* Get the python directory */
            ivi = (InstalledVersionInfo *)
                        SendDlgItemMessage(hwnd,
                                                                IDC_VERSIONS_LIST,
                                                                LB_GETITEMDATA,
                                                                id,
                                                                0);
            hkey_root = ivi->hkey;
                                strcpy(python_dir, ivi->prefix);
                                SetDlgItemText(hwnd, IDC_PATH, python_dir);
                                /* retrieve the python version and pythondll to use */
                    result = SendDlgItemMessage(hwnd, IDC_VERSIONS_LIST,
                                                 LB_GETTEXTLEN, (WPARAM)id, 0);
                    pbuf = (char *)malloc(result + 1);
                    if (pbuf) {
                        /* guess the name of the python-dll */
                        SendDlgItemMessage(hwnd, IDC_VERSIONS_LIST,
                                            LB_GETTEXT, (WPARAM)id,
                                            (LPARAM)pbuf);
                        result = sscanf(pbuf, "Python Version %d.%d",
                                         &py_major, &py_minor);
                        if (result == 2) {
#ifdef _DEBUG
                            wsprintf(pythondll, "python%d%d_d.dll",
                                     py_major, py_minor);
#else
                            wsprintf(pythondll, "python%d%d.dll",
                                     py_major, py_minor);
#endif
                        }
                        free(pbuf);
                    } else
                        strcpy(pythondll, "");
                    /* retrieve the scheme for this version */
                    {
                        char install_path[_MAX_PATH];
                        SCHEME *scheme = GetScheme(py_major, py_minor);
                        strcpy(install_path, python_dir);
                        if (install_path[strlen(install_path)-1] != '\\')
                            strcat(install_path, "\\");
                        strcat(install_path, scheme[0].prefix);
                        SetDlgItemText(hwnd, IDC_INSTALL_PATH, install_path);
                    }
                }
            }
            break;
        }
        return 0;

    case WM_NOTIFY:
        lpnm = (LPNMHDR) lParam;

        switch (lpnm->code) {
            int id;
        case PSN_SETACTIVE:
            id = SendDlgItemMessage(hwnd, IDC_VERSIONS_LIST,
                                     LB_GETCURSEL, 0, 0);
            if (id == LB_ERR)
                PropSheet_SetWizButtons(GetParent(hwnd),
                                        PSWIZB_BACK);
            else
                PropSheet_SetWizButtons(GetParent(hwnd),
                                        PSWIZB_BACK | PSWIZB_NEXT);
            break;

        case PSN_WIZNEXT:
            break;

        case PSN_WIZFINISH:
            break;

        case PSN_RESET:
            break;

        default:
            break;
        }
    }
    return 0;
}

static BOOL OpenLogfile(char *dir)
{
    char buffer[_MAX_PATH+1];
    time_t ltime;
    struct tm *now;
    long result;
    HKEY hKey, hSubkey;
    char subkey_name[256];
    static char KeyName[] =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
    const char *root_name = (hkey_root==HKEY_LOCAL_MACHINE ?
                            "HKEY_LOCAL_MACHINE" : "HKEY_CURRENT_USER");
    DWORD disposition;

    /* Use Create, as the Uninstall subkey may not exist under HKCU.
       Use CreateKeyEx, so we can specify a SAM specifying write access
    */
        result = RegCreateKeyEx(hkey_root,
                      KeyName,
                      0, /* reserved */
                  NULL, /* class */
                  0, /* options */
                  KEY_CREATE_SUB_KEY, /* sam */
                  NULL, /* security */
                  &hKey, /* result key */
                  NULL); /* disposition */
    if (result != ERROR_SUCCESS) {
        if (result == ERROR_ACCESS_DENIED) {
            /* This should no longer be able to happen - we have already
               checked if they have permissions in HKLM, and all users
               should have write access to HKCU.
            */
            MessageBox(GetFocus(),
                       "You do not seem to have sufficient access rights\n"
                       "on this machine to install this software",
                       NULL,
                       MB_OK | MB_ICONSTOP);
            return FALSE;
        } else {
            MessageBox(GetFocus(), KeyName, "Could not open key", MB_OK);
        }
    }

    sprintf(buffer, "%s\\%s-wininst.log", dir, meta_name);
    logfile = fopen(buffer, "a");
    time(&ltime);
    now = localtime(&ltime);
    strftime(buffer, sizeof(buffer),
             "*** Installation started %Y/%m/%d %H:%M ***\n",
             localtime(&ltime));
    fprintf(logfile, buffer);
    fprintf(logfile, "Source: %s\n", modulename);

    /* Root key must be first entry processed by uninstaller. */
    fprintf(logfile, "999 Root Key: %s\n", root_name);

    sprintf(subkey_name, "%s-py%d.%d", meta_name, py_major, py_minor);

    result = RegCreateKeyEx(hKey, subkey_name,
                            0, NULL, 0,
                            KEY_WRITE,
                            NULL,
                            &hSubkey,
                            &disposition);

    if (result != ERROR_SUCCESS)
        MessageBox(GetFocus(), subkey_name, "Could not create key", MB_OK);

    RegCloseKey(hKey);

    if (disposition == REG_CREATED_NEW_KEY)
        fprintf(logfile, "020 Reg DB Key: [%s]%s\n", KeyName, subkey_name);

    sprintf(buffer, "Python %d.%d %s", py_major, py_minor, title);

    result = RegSetValueEx(hSubkey, "DisplayName",
                           0,
                           REG_SZ,
                           buffer,
                           strlen(buffer)+1);

    if (result != ERROR_SUCCESS)
        MessageBox(GetFocus(), buffer, "Could not set key value", MB_OK);

    fprintf(logfile, "040 Reg DB Value: [%s\\%s]%s=%s\n",
        KeyName, subkey_name, "DisplayName", buffer);

    {
        FILE *fp;
        sprintf(buffer, "%s\\Remove%s.exe", dir, meta_name);
        fp = fopen(buffer, "wb");
        fwrite(arc_data, exe_size, 1, fp);
        fclose(fp);

        sprintf(buffer, "\"%s\\Remove%s.exe\" -u \"%s\\%s-wininst.log\"",
            dir, meta_name, dir, meta_name);

        result = RegSetValueEx(hSubkey, "UninstallString",
                               0,
                               REG_SZ,
                               buffer,
                               strlen(buffer)+1);

        if (result != ERROR_SUCCESS)
            MessageBox(GetFocus(), buffer, "Could not set key value", MB_OK);

        fprintf(logfile, "040 Reg DB Value: [%s\\%s]%s=%s\n",
            KeyName, subkey_name, "UninstallString", buffer);
    }
    return TRUE;
}

static void CloseLogfile(void)
{
    char buffer[_MAX_PATH+1];
    time_t ltime;
    struct tm *now;

    time(&ltime);
    now = localtime(&ltime);
    strftime(buffer, sizeof(buffer),
             "*** Installation finished %Y/%m/%d %H:%M ***\n",
             localtime(&ltime));
    fprintf(logfile, buffer);
    if (logfile)
        fclose(logfile);
}

BOOL CALLBACK
InstallFilesDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR lpnm;
    char Buffer[4096];
    SCHEME *scheme;

    switch (msg) {
    case WM_INITDIALOG:
        if (hBitmap)
            SendDlgItemMessage(hwnd, IDC_BITMAP, STM_SETIMAGE,
                               IMAGE_BITMAP, (LPARAM)hBitmap);
        wsprintf(Buffer,
                  "Click Next to begin the installation of %s. "
                  "If you want to review or change any of your "
                  " installation settings, click Back. "
                  "Click Cancel to exit the wizard.",
                  meta_name);
        SetDlgItemText(hwnd, IDC_TITLE, Buffer);
        SetDlgItemText(hwnd, IDC_INFO, "Ready to install");
        break;

    case WM_NUMFILES:
        SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETRANGE, 0, lParam);
        PumpMessages();
        return TRUE;

    case WM_NEXTFILE:
        SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETPOS, wParam,
                            0);
        SetDlgItemText(hwnd, IDC_INFO, (LPSTR)lParam);
        PumpMessages();
        return TRUE;

    case WM_NOTIFY:
        lpnm = (LPNMHDR) lParam;

        switch (lpnm->code) {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hwnd),
                                    PSWIZB_BACK | PSWIZB_NEXT);
            break;

        case PSN_WIZFINISH:
            break;

        case PSN_WIZNEXT:
            /* Handle a Next button click here */
            hDialog = hwnd;
            success = TRUE;

            /* Disable the buttons while we work.  Sending CANCELTOCLOSE has
              the effect of disabling the cancel button, which is a) as we
              do everything synchronously we can't cancel, and b) the next
              step is 'finished', when it is too late to cancel anyway.
              The next step being 'Finished' means we also don't need to
              restore the button state back */
            PropSheet_SetWizButtons(GetParent(hwnd), 0);
            SendMessage(GetParent(hwnd), PSM_CANCELTOCLOSE, 0, 0);
            /* Make sure the installation directory name ends in a */
            /* backslash */
            if (python_dir[strlen(python_dir)-1] != '\\')
                strcat(python_dir, "\\");
            /* Strip the trailing backslash again */
            python_dir[strlen(python_dir)-1] = '\0';

            CheckRootKey(hwnd);

            if (!OpenLogfile(python_dir))
                break;

/*
 * The scheme we have to use depends on the Python version...
 if sys.version < "2.2":
 WINDOWS_SCHEME = {
 'purelib': '$base',
 'platlib': '$base',
 'headers': '$base/Include/$dist_name',
 'scripts': '$base/Scripts',
 'data'   : '$base',
 }
 else:
 WINDOWS_SCHEME = {
 'purelib': '$base/Lib/site-packages',
 'platlib': '$base/Lib/site-packages',
 'headers': '$base/Include/$dist_name',
 'scripts': '$base/Scripts',
 'data'   : '$base',
 }
*/
            scheme = GetScheme(py_major, py_minor);
            /* Run the pre-install script. */
            if (pre_install_script && *pre_install_script) {
                SetDlgItemText (hwnd, IDC_TITLE,
                                "Running pre-installation script");
                run_simple_script(pre_install_script);
            }
            if (!success) {
                break;
            }
            /* Extract all files from the archive */
            SetDlgItemText(hwnd, IDC_TITLE, "Installing files...");
            if (!unzip_archive (scheme,
                                python_dir, arc_data,
                                arc_size, notify))
                set_failure_reason("Failed to unzip installation files");
            /* Compile the py-files */
            if (success && pyc_compile) {
                int errors;
                HINSTANCE hPython;
                SetDlgItemText(hwnd, IDC_TITLE,
                                "Compiling files to .pyc...");

                SetDlgItemText(hDialog, IDC_INFO, "Loading python...");
                hPython = LoadPythonDll(pythondll);
                if (hPython) {
                    errors = compile_filelist(hPython, FALSE);
                    FreeLibrary(hPython);
                }
                /* Compilation errors are intentionally ignored:
                 * Python2.0 contains a bug which will result
                 * in sys.path containing garbage under certain
                 * circumstances, and an error message will only
                 * confuse the user.
                 */
            }
            if (success && pyo_compile) {
                int errors;
                HINSTANCE hPython;
                SetDlgItemText(hwnd, IDC_TITLE,
                                "Compiling files to .pyo...");

                SetDlgItemText(hDialog, IDC_INFO, "Loading python...");
                hPython = LoadPythonDll(pythondll);
                if (hPython) {
                    errors = compile_filelist(hPython, TRUE);
                    FreeLibrary(hPython);
                }
                /* Errors ignored: see above */
            }


            break;

        case PSN_RESET:
            break;

        default:
            break;
        }
    }
    return 0;
}


BOOL CALLBACK
FinishedDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR lpnm;

    switch (msg) {
    case WM_INITDIALOG:
        if (hBitmap)
            SendDlgItemMessage(hwnd, IDC_BITMAP, STM_SETIMAGE,
                               IMAGE_BITMAP, (LPARAM)hBitmap);
        if (!success)
            SetDlgItemText(hwnd, IDC_INFO, get_failure_reason());

        /* async delay: will show the dialog box completely before
           the install_script is started */
        PostMessage(hwnd, WM_USER, 0, 0L);
        return TRUE;

    case WM_USER:

        if (success && install_script && install_script[0]) {
            char fname[MAX_PATH];
            char *buffer;
            HCURSOR hCursor;
            int result;

            char *argv[3] = {NULL, "-install", NULL};

            SetDlgItemText(hwnd, IDC_TITLE,
                            "Please wait while running postinstall script...");
            strcpy(fname, python_dir);
            strcat(fname, "\\Scripts\\");
            strcat(fname, install_script);

            if (logfile)
                fprintf(logfile, "300 Run Script: [%s]%s\n", pythondll, fname);

            hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

            argv[0] = fname;

            result = run_installscript(fname, 2, argv, &buffer);
            if (0 != result) {
                fprintf(stderr, "*** run_installscript: internal error 0x%X ***\n", result);
            }
            if (buffer)
                SetDlgItemText(hwnd, IDC_INFO, buffer);
            SetDlgItemText(hwnd, IDC_TITLE,
                            "Postinstall script finished.\n"
                            "Click the Finish button to exit the Setup wizard.");

            free(buffer);
            SetCursor(hCursor);
            CloseLogfile();
        }

        return TRUE;

    case WM_NOTIFY:
        lpnm = (LPNMHDR) lParam;

        switch (lpnm->code) {
        case PSN_SETACTIVE: /* Enable the Finish button */
            PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_FINISH);
            break;

        case PSN_WIZNEXT:
            break;

        case PSN_WIZFINISH:
            break;

        case PSN_RESET:
            break;

        default:
            break;
        }
    }
    return 0;
}

void RunWizard(HWND hwnd)
{
    PROPSHEETPAGE   psp =       {0};
    HPROPSHEETPAGE  ahpsp[4] =  {0};
    PROPSHEETHEADER psh =       {0};

    /* Display module information */
    psp.dwSize =        sizeof(psp);
    psp.dwFlags =       PSP_DEFAULT|PSP_HIDEHEADER;
    psp.hInstance =     GetModuleHandle (NULL);
    psp.lParam =        0;
    psp.pfnDlgProc =    IntroDlgProc;
    psp.pszTemplate =   MAKEINTRESOURCE(IDD_INTRO);

    ahpsp[0] =          CreatePropertySheetPage(&psp);

    /* Select python version to use */
    psp.dwFlags =       PSP_DEFAULT|PSP_HIDEHEADER;
    psp.pszTemplate =       MAKEINTRESOURCE(IDD_SELECTPYTHON);
    psp.pfnDlgProc =        SelectPythonDlgProc;

    ahpsp[1] =              CreatePropertySheetPage(&psp);

    /* Install the files */
    psp.dwFlags =           PSP_DEFAULT|PSP_HIDEHEADER;
    psp.pszTemplate =       MAKEINTRESOURCE(IDD_INSTALLFILES);
    psp.pfnDlgProc =        InstallFilesDlgProc;

    ahpsp[2] =              CreatePropertySheetPage(&psp);

    /* Show success or failure */
    psp.dwFlags =           PSP_DEFAULT|PSP_HIDEHEADER;
    psp.pszTemplate =       MAKEINTRESOURCE(IDD_FINISHED);
    psp.pfnDlgProc =        FinishedDlgProc;

    ahpsp[3] =              CreatePropertySheetPage(&psp);

    /* Create the property sheet */
    psh.dwSize =            sizeof(psh);
    psh.hInstance =         GetModuleHandle(NULL);
    psh.hwndParent =        hwnd;
    psh.phpage =            ahpsp;
    psh.dwFlags =           PSH_WIZARD/*97*//*|PSH_WATERMARK|PSH_HEADER*/;
        psh.pszbmWatermark =    NULL;
        psh.pszbmHeader =       NULL;
        psh.nStartPage =        0;
        psh.nPages =            4;

        PropertySheet(&psh);
}

// subtly different from HasLocalMachinePrivs(), in that after executing
// an 'elevated' process, we expect this to return TRUE - but there is no
// such implication for HasLocalMachinePrivs
BOOL MyIsUserAnAdmin()
{
    typedef BOOL (WINAPI *PFNIsUserAnAdmin)();
    static PFNIsUserAnAdmin pfnIsUserAnAdmin = NULL;
    HMODULE shell32;
    // This function isn't guaranteed to be available (and it can't hurt
    // to leave the library loaded)
    if (0 == (shell32=LoadLibrary("shell32.dll")))
        return FALSE;
    if (0 == (pfnIsUserAnAdmin=(PFNIsUserAnAdmin)GetProcAddress(shell32, "IsUserAnAdmin")))
        return FALSE;
    return (*pfnIsUserAnAdmin)();
}

// Some magic for Vista's UAC.  If there is a target_version, and
// if that target version is installed in the registry under
// HKLM, and we are not current administrator, then
// re-execute ourselves requesting elevation.
// Split into 2 functions - "should we elevate" and "spawn elevated"

// Returns TRUE if we should spawn an elevated child
BOOL NeedAutoUAC()
{
    HKEY hk;
    char key_name[80];
    // no Python version info == we can't know yet.
    if (target_version[0] == '\0')
        return FALSE;
    // see how python is current installed
    wsprintf(key_name,
                     "Software\\Python\\PythonCore\\%s\\InstallPath",
                     target_version);
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      key_name, 0, KEY_READ, &hk))
        return FALSE;
    RegCloseKey(hk);
    // Python is installed in HKLM - we must elevate.
    return TRUE;
}

// Returns TRUE if the platform supports UAC.
BOOL PlatformSupportsUAC()
{
    // Note that win2k does seem to support ShellExecute with 'runas',
    // but does *not* support IsUserAnAdmin - so we just pretend things
    // only work on XP and later.
    BOOL bIsWindowsXPorLater;
    OSVERSIONINFO winverinfo;
    winverinfo.dwOSVersionInfoSize = sizeof(winverinfo);
    if (!GetVersionEx(&winverinfo))
        return FALSE; // something bad has gone wrong
    bIsWindowsXPorLater =
       ( (winverinfo.dwMajorVersion > 5) ||
       ( (winverinfo.dwMajorVersion == 5) && (winverinfo.dwMinorVersion >= 1) ));
    return bIsWindowsXPorLater;
}

// Spawn ourself as an elevated application.  On failure, a message is
// displayed to the user - but this app will always terminate, even
// on error.
void SpawnUAC()
{
    // interesting failure scenario that has been seen: initial executable
    // runs from a network drive - but once elevated, that network share
    // isn't seen, and ShellExecute fails with SE_ERR_ACCESSDENIED.
    int ret = (int)ShellExecute(0, "runas", modulename, "", NULL,
                                SW_SHOWNORMAL);
    if (ret <= 32) {
        char msg[128];
        wsprintf(msg, "Failed to start elevated process (ShellExecute returned %d)", ret);
        MessageBox(0, msg, "Setup", MB_OK | MB_ICONERROR);
    }
}

int DoInstall(void)
{
    char ini_buffer[4096];

    /* Read installation information */
    GetPrivateProfileString("Setup", "title", "", ini_buffer,
                             sizeof(ini_buffer), ini_file);
    unescape(title, ini_buffer, sizeof(title));

    GetPrivateProfileString("Setup", "info", "", ini_buffer,
                             sizeof(ini_buffer), ini_file);
    unescape(info, ini_buffer, sizeof(info));

    GetPrivateProfileString("Setup", "build_info", "", build_info,
                             sizeof(build_info), ini_file);

    pyc_compile = GetPrivateProfileInt("Setup", "target_compile", 1,
                                        ini_file);
    pyo_compile = GetPrivateProfileInt("Setup", "target_optimize", 1,
                                        ini_file);

    GetPrivateProfileString("Setup", "target_version", "",
                             target_version, sizeof(target_version),
                             ini_file);

    GetPrivateProfileString("metadata", "name", "",
                             meta_name, sizeof(meta_name),
                             ini_file);

    GetPrivateProfileString("Setup", "install_script", "",
                             install_script, sizeof(install_script),
                             ini_file);

    GetPrivateProfileString("Setup", "user_access_control", "",
                             user_access_control, sizeof(user_access_control), ini_file);

    // See if we need to do the Vista UAC magic.
    if (strcmp(user_access_control, "force")==0) {
        if (PlatformSupportsUAC() && !MyIsUserAnAdmin()) {
            SpawnUAC();
            return 0;
        }
        // already admin - keep going
    } else if (strcmp(user_access_control, "auto")==0) {
        // Check if it looks like we need UAC control, based
        // on how Python itself was installed.
        if (PlatformSupportsUAC() && !MyIsUserAnAdmin() && NeedAutoUAC()) {
            SpawnUAC();
            return 0;
        }
    } else {
        // display a warning about unknown values - only the developer
        // of the extension will see it (until they fix it!)
        if (user_access_control[0] && strcmp(user_access_control, "none") != 0) {
            MessageBox(GetFocus(), "Bad user_access_control value", "oops", MB_OK);
        // nothing to do.
        }
    }

    hwndMain = CreateBackground(title);

    RunWizard(hwndMain);

    /* Clean up */
    UnmapViewOfFile(arc_data);
    if (ini_file)
        DeleteFile(ini_file);

    if (hBitmap)
        DeleteObject(hBitmap);

    return 0;
}

/*********************** uninstall section ******************************/

static int compare(const void *p1, const void *p2)
{
    return strcmp(*(char **)p2, *(char **)p1);
}

/*
 * Commit suicide (remove the uninstaller itself).
 *
 * Create a batch file to first remove the uninstaller
 * (will succeed after it has finished), then the batch file itself.
 *
 * This technique has been demonstrated by Jeff Richter,
 * MSJ 1/1996
 */
void remove_exe(void)
{
    char exename[_MAX_PATH];
    char batname[_MAX_PATH];
    FILE *fp;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    GetModuleFileName(NULL, exename, sizeof(exename));
    sprintf(batname, "%s.bat", exename);
    fp = fopen(batname, "w");
    fprintf(fp, ":Repeat\n");
    fprintf(fp, "del \"%s\"\n", exename);
    fprintf(fp, "if exist \"%s\" goto Repeat\n", exename);
    fprintf(fp, "del \"%s\"\n", batname);
    fclose(fp);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    if (CreateProcess(NULL,
                      batname,
                      NULL,
                      NULL,
                      FALSE,
                      CREATE_SUSPENDED | IDLE_PRIORITY_CLASS,
                      NULL,
                      "\\",
                      &si,
                      &pi)) {
        SetThreadPriority(pi.hThread, THREAD_PRIORITY_IDLE);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
        CloseHandle(pi.hProcess);
        ResumeThread(pi.hThread);
        CloseHandle(pi.hThread);
    }
}

void DeleteRegistryKey(char *string)
{
    char *keyname;
    char *subkeyname;
    char *delim;
    HKEY hKey;
    long result;
    char *line;

    line = strdup(string); /* so we can change it */

    keyname = strchr(line, '[');
    if (!keyname)
        return;
    ++keyname;

    subkeyname = strchr(keyname, ']');
    if (!subkeyname)
        return;
    *subkeyname++='\0';
    delim = strchr(subkeyname, '\n');
    if (delim)
        *delim = '\0';

    result = RegOpenKeyEx(hkey_root,
                          keyname,
                          0,
                          KEY_WRITE,
                          &hKey);

    if (result != ERROR_SUCCESS)
        MessageBox(GetFocus(), string, "Could not open key", MB_OK);
    else {
        result = RegDeleteKey(hKey, subkeyname);
        if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND)
            MessageBox(GetFocus(), string, "Could not delete key", MB_OK);
        RegCloseKey(hKey);
    }
    free(line);
}

void DeleteRegistryValue(char *string)
{
    char *keyname;
    char *valuename;
    char *value;
    HKEY hKey;
    long result;
    char *line;

    line = strdup(string); /* so we can change it */

/* Format is 'Reg DB Value: [key]name=value' */
    keyname = strchr(line, '[');
    if (!keyname)
        return;
    ++keyname;
    valuename = strchr(keyname, ']');
    if (!valuename)
        return;
    *valuename++ = '\0';
    value = strchr(valuename, '=');
    if (!value)
        return;

    *value++ = '\0';

    result = RegOpenKeyEx(hkey_root,
                          keyname,
                          0,
                          KEY_WRITE,
                          &hKey);
    if (result != ERROR_SUCCESS)
        MessageBox(GetFocus(), string, "Could not open key", MB_OK);
    else {
        result = RegDeleteValue(hKey, valuename);
        if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND)
            MessageBox(GetFocus(), string, "Could not delete value", MB_OK);
        RegCloseKey(hKey);
    }
    free(line);
}

BOOL MyDeleteFile(char *line)
{
    char *pathname = strchr(line, ':');
    if (!pathname)
        return FALSE;
    ++pathname;
    while (isspace(*pathname))
        ++pathname;
    return DeleteFile(pathname);
}

BOOL MyRemoveDirectory(char *line)
{
    char *pathname = strchr(line, ':');
    if (!pathname)
        return FALSE;
    ++pathname;
    while (isspace(*pathname))
        ++pathname;
    return RemoveDirectory(pathname);
}

BOOL Run_RemoveScript(char *line)
{
    char *dllname;
    char *scriptname;
    static char lastscript[MAX_PATH];

/* Format is 'Run Scripts: [pythondll]scriptname' */
/* XXX Currently, pythondll carries no path!!! */
    dllname = strchr(line, '[');
    if (!dllname)
        return FALSE;
    ++dllname;
    scriptname = strchr(dllname, ']');
    if (!scriptname)
        return FALSE;
    *scriptname++ = '\0';
    /* this function may be called more than one time with the same
       script, only run it one time */
    if (strcmp(lastscript, scriptname)) {
        char *argv[3] = {NULL, "-remove", NULL};
        char *buffer = NULL;

        argv[0] = scriptname;

        if (0 != run_installscript(scriptname, 2, argv, &buffer))
            fprintf(stderr, "*** Could not run installation script ***");

        if (buffer && buffer[0])
            MessageBox(GetFocus(), buffer, "uninstall-script", MB_OK);
        free(buffer);

        strcpy(lastscript, scriptname);
    }
    return TRUE;
}

int DoUninstall(int argc, char **argv)
{
    FILE *logfile;
    char buffer[4096];
    int nLines = 0;
    int i;
    char *cp;
    int nFiles = 0;
    int nDirs = 0;
    int nErrors = 0;
    char **lines;
    int lines_buffer_size = 10;

    if (argc != 3) {
        MessageBox(NULL,
                   "Wrong number of args",
                   NULL,
                   MB_OK);
        return 1; /* Error */
    }
    if (strcmp(argv[1], "-u")) {
        MessageBox(NULL,
                   "2. arg is not -u",
                   NULL,
                   MB_OK);
        return 1; /* Error */
    }

    logfile = fopen(argv[2], "r");
    if (!logfile) {
        MessageBox(NULL,
                   "could not open logfile",
                   NULL,
                   MB_OK);
        return 1; /* Error */
    }

    lines = (char **)malloc(sizeof(char *) * lines_buffer_size);
    if (!lines)
        return SystemError(0, "Out of memory");

    /* Read the whole logfile, realloacting the buffer */
    while (fgets(buffer, sizeof(buffer), logfile)) {
        int len = strlen(buffer);
        /* remove trailing white space */
        while (isspace(buffer[len-1]))
            len -= 1;
        buffer[len] = '\0';
        lines[nLines++] = strdup(buffer);
        if (nLines >= lines_buffer_size) {
            lines_buffer_size += 10;
            lines = (char **)realloc(lines,
                                     sizeof(char *) * lines_buffer_size);
            if (!lines)
                return SystemError(0, "Out of memory");
        }
    }
    fclose(logfile);

    /* Sort all the lines, so that highest 3-digit codes are first */
    qsort(&lines[0], nLines, sizeof(char *),
          compare);

    if (IDYES != MessageBox(NULL,
                            "Are you sure you want to remove\n"
                            "this package from your computer?",
                            "Please confirm",
                            MB_YESNO | MB_ICONQUESTION))
        return 0;

    hkey_root = HKEY_LOCAL_MACHINE;
    cp = "";
    for (i = 0; i < nLines; ++i) {
        /* Ignore duplicate lines */
        if (strcmp(cp, lines[i])) {
            int ign;
            cp = lines[i];
            /* Parse the lines */
            if (2 == sscanf(cp, "%d Root Key: %s", &ign, &buffer)) {
                if (strcmp(buffer, "HKEY_CURRENT_USER")==0)
                    hkey_root = HKEY_CURRENT_USER;
                else {
                    // HKLM - check they have permissions.
                    if (!HasLocalMachinePrivs()) {
                        MessageBox(GetFocus(),
                                   "You do not seem to have sufficient access rights\n"
                                   "on this machine to uninstall this software",
                                   NULL,
                                   MB_OK | MB_ICONSTOP);
                        return 1; /* Error */
                    }
                }
            } else if (2 == sscanf(cp, "%d Made Dir: %s", &ign, &buffer)) {
                if (MyRemoveDirectory(cp))
                    ++nDirs;
                else {
                    int code = GetLastError();
                    if (code != 2 && code != 3) { /* file or path not found */
                        ++nErrors;
                    }
                }
            } else if (2 == sscanf(cp, "%d File Copy: %s", &ign, &buffer)) {
                if (MyDeleteFile(cp))
                    ++nFiles;
                else {
                    int code = GetLastError();
                    if (code != 2 && code != 3) { /* file or path not found */
                        ++nErrors;
                    }
                }
            } else if (2 == sscanf(cp, "%d File Overwrite: %s", &ign, &buffer)) {
                if (MyDeleteFile(cp))
                    ++nFiles;
                else {
                    int code = GetLastError();
                    if (code != 2 && code != 3) { /* file or path not found */
                        ++nErrors;
                    }
                }
            } else if (2 == sscanf(cp, "%d Reg DB Key: %s", &ign, &buffer)) {
                DeleteRegistryKey(cp);
            } else if (2 == sscanf(cp, "%d Reg DB Value: %s", &ign, &buffer)) {
                DeleteRegistryValue(cp);
            } else if (2 == sscanf(cp, "%d Run Script: %s", &ign, &buffer)) {
                Run_RemoveScript(cp);
            }
        }
    }

    if (DeleteFile(argv[2])) {
        ++nFiles;
    } else {
        ++nErrors;
        SystemError(GetLastError(), argv[2]);
    }
    if (nErrors)
        wsprintf(buffer,
                 "%d files and %d directories removed\n"
                 "%d files or directories could not be removed",
                 nFiles, nDirs, nErrors);
    else
        wsprintf(buffer, "%d files and %d directories removed",
                 nFiles, nDirs);
    MessageBox(NULL, buffer, "Uninstall Finished!",
               MB_OK | MB_ICONINFORMATION);
    remove_exe();
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst,
                    LPSTR lpszCmdLine, INT nCmdShow)
{
    extern int __argc;
    extern char **__argv;
    char *basename;

    GetModuleFileName(NULL, modulename, sizeof(modulename));
    GetModuleFileNameW(NULL, wmodulename, sizeof(wmodulename)/sizeof(wmodulename[0]));

    /* Map the executable file to memory */
    arc_data = MapExistingFile(modulename, &arc_size);
    if (!arc_data) {
        SystemError(GetLastError(), "Could not open archive");
        return 1;
    }

    /* OK. So this program can act as installer (self-extracting
     * zip-file, or as uninstaller when started with '-u logfile'
     * command line flags.
     *
     * The installer is usually started without command line flags,
     * and the uninstaller is usually started with the '-u logfile'
     * flag. What to do if some innocent user double-clicks the
     * exe-file?
     * The following implements a defensive strategy...
     */

    /* Try to extract the configuration data into a temporary file */
    if (ExtractInstallData(arc_data, arc_size, &exe_size,
                           &ini_file, &pre_install_script))
        return DoInstall();

    if (!ini_file && __argc > 1) {
        return DoUninstall(__argc, __argv);
    }


    basename = strrchr(modulename, '\\');
    if (basename)
        ++basename;

    /* Last guess about the purpose of this program */
    if (basename && (0 == strncmp(basename, "Remove", 6)))
        SystemError(0, "This program is normally started by windows");
    else
        SystemError(0, "Setup program invalid or damaged");
    return 1;
}
