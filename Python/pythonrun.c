
/* Top level execution of Python code (including in __main__) */

/* To help control the interfaces between the startup, execution and
 * shutdown code, the phases are split across separate modules (bootstrap,
 * pythonrun, shutdown)
 */

/* TODO: Cull includes following phase split */

#include "Python.h"

#include "pycore_ast.h"           // PyAST_mod2obj()
#include "pycore_audit.h"         // _PySys_Audit()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCall()
#include "pycore_compile.h"       // _PyAST_Compile()
#include "pycore_fileutils.h"     // _PyFile_Flush
#include "pycore_import.h"        // _PyImport_GetImportlibExternalLoader()
#include "pycore_interp.h"        // PyInterpreterState.importlib
#include "pycore_object.h"        // _PyDebug_PrintTotalRefs()
#include "pycore_parser.h"        // _PyParser_ASTFromString()
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pylifecycle.h"   // _Py_FdIsInteractive()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_pythonrun.h"     // export _PyRun_InteractiveLoopObject()
#include "pycore_sysmodule.h"     // _PySys_SetAttr()
#include "pycore_traceback.h"     // _PyTraceBack_Print()
#include "pycore_unicodeobject.h" // _PyUnicode_Equal()

#include "errcode.h"              // E_EOF
#include "marshal.h"              // PyMarshal_ReadLongFromFile()

#include <stdbool.h>

#ifdef MS_WINDOWS
#  include "malloc.h"             // alloca()
#endif

#ifdef MS_WINDOWS
#  undef BYTE
#  include "windows.h"
#endif

/* Forward */
static void flush_io(void);
static PyObject *run_mod(mod_ty, PyObject *, PyObject *, PyObject *,
                          PyCompilerFlags *, PyArena *, PyObject*, int);
static PyObject *run_pyc_file(FILE *, PyObject *, PyObject *,
                              PyCompilerFlags *);
static int PyRun_InteractiveOneObjectEx(FILE *, PyObject *, PyCompilerFlags *);
static PyObject* pyrun_file(FILE *fp, PyObject *filename, int start,
                            PyObject *globals, PyObject *locals, int closeit,
                            PyCompilerFlags *flags);
static PyObject *
_PyRun_StringFlagsWithName(const char *str, PyObject* name, int start,
                           PyObject *globals, PyObject *locals, PyCompilerFlags *flags,
                           int generate_new_source);

int
_PyRun_AnyFileObject(FILE *fp, PyObject *filename, int closeit,
                     PyCompilerFlags *flags)
{
    int decref_filename = 0;
    if (filename == NULL) {
        filename = PyUnicode_FromString("???");
        if (filename == NULL) {
            PyErr_Print();
            return -1;
        }
        decref_filename = 1;
    }

    int res;
    if (_Py_FdIsInteractive(fp, filename)) {
        res = _PyRun_InteractiveLoopObject(fp, filename, flags);
        if (closeit) {
            fclose(fp);
        }
    }
    else {
        res = _PyRun_SimpleFileObject(fp, filename, closeit, flags);
    }

    if (decref_filename) {
        Py_DECREF(filename);
    }
    return res;
}

int
PyRun_AnyFileExFlags(FILE *fp, const char *filename, int closeit,
                     PyCompilerFlags *flags)
{
    PyObject *filename_obj = NULL;
    if (filename != NULL) {
        filename_obj = PyUnicode_DecodeFSDefault(filename);
        if (filename_obj == NULL) {
            PyErr_Print();
            return -1;
        }
    }
    int res = _PyRun_AnyFileObject(fp, filename_obj, closeit, flags);
    Py_XDECREF(filename_obj);
    return res;
}


int
_PyRun_InteractiveLoopObject(FILE *fp, PyObject *filename, PyCompilerFlags *flags)
{
    PyCompilerFlags local_flags = _PyCompilerFlags_INIT;
    if (flags == NULL) {
        flags = &local_flags;
    }

    PyObject *v;
    if (_PySys_GetOptionalAttr(&_Py_ID(ps1), &v) < 0) {
        PyErr_Print();
        return -1;
    }
    if (v == NULL) {
        v = PyUnicode_FromString(">>> ");
        if (v == NULL) {
            PyErr_Clear();
        }
        if (_PySys_SetAttr(&_Py_ID(ps1), v) < 0) {
            PyErr_Clear();
        }
    }
    Py_XDECREF(v);
    if (_PySys_GetOptionalAttr(&_Py_ID(ps2), &v) < 0) {
        PyErr_Print();
        return -1;
    }
    if (v == NULL) {
        v = PyUnicode_FromString("... ");
        if (v == NULL) {
            PyErr_Clear();
        }
        if (_PySys_SetAttr(&_Py_ID(ps2), v) < 0) {
            PyErr_Clear();
        }
    }
    Py_XDECREF(v);

#ifdef Py_REF_DEBUG
    int show_ref_count = _Py_GetConfig()->show_ref_count;
#endif
    int err = 0;
    int ret;
    int nomem_count = 0;
    do {
        ret = PyRun_InteractiveOneObjectEx(fp, filename, flags);
        if (ret == -1 && PyErr_Occurred()) {
            /* Prevent an endless loop after multiple consecutive MemoryErrors
             * while still allowing an interactive command to fail with a
             * MemoryError. */
            if (PyErr_ExceptionMatches(PyExc_MemoryError)) {
                if (++nomem_count > 16) {
                    PyErr_Clear();
                    err = -1;
                    break;
                }
            } else {
                nomem_count = 0;
            }
            PyErr_Print();
            flush_io();
        } else {
            nomem_count = 0;
        }
#ifdef Py_REF_DEBUG
        if (show_ref_count) {
            _PyDebug_PrintTotalRefs();
        }
#endif
    } while (ret != E_EOF);
    return err;
}


int
PyRun_InteractiveLoopFlags(FILE *fp, const char *filename, PyCompilerFlags *flags)
{
    PyObject *filename_obj = PyUnicode_DecodeFSDefault(filename);
    if (filename_obj == NULL) {
        PyErr_Print();
        return -1;
    }

    int err = _PyRun_InteractiveLoopObject(fp, filename_obj, flags);
    Py_DECREF(filename_obj);
    return err;

}


// Call _PyParser_ASTFromFile() with sys.stdin.encoding, sys.ps1 and sys.ps2
static int
pyrun_one_parse_ast(FILE *fp, PyObject *filename,
                    PyCompilerFlags *flags, PyArena *arena,
                    mod_ty *pmod, PyObject** interactive_src)
{
    // Get sys.stdin.encoding (as UTF-8)
    PyObject *attr;
    PyObject *encoding_obj = NULL;
    const char *encoding = NULL;
    if (fp == stdin) {
        if (_PySys_GetOptionalAttr(&_Py_ID(stdin), &attr) < 0) {
            PyErr_Clear();
        }
        else if (attr != NULL && attr != Py_None) {
            if (PyObject_GetOptionalAttr(attr, &_Py_ID(encoding), &encoding_obj) < 0) {
                PyErr_Clear();
            }
            else if (encoding_obj && PyUnicode_Check(encoding_obj)) {
                encoding = PyUnicode_AsUTF8(encoding_obj);
                if (!encoding) {
                    PyErr_Clear();
                }
            }
        }
        Py_XDECREF(attr);
    }

    // Get sys.ps1 (as UTF-8)
    PyObject *ps1_obj = NULL;
    const char *ps1 = "";
    if (_PySys_GetOptionalAttr(&_Py_ID(ps1), &attr) < 0) {
        PyErr_Clear();
    }
    else if (attr != NULL) {
        ps1_obj = PyObject_Str(attr);
        Py_DECREF(attr);
        if (ps1_obj == NULL) {
            PyErr_Clear();
        }
        else if (PyUnicode_Check(ps1_obj)) {
            ps1 = PyUnicode_AsUTF8(ps1_obj);
            if (ps1 == NULL) {
                PyErr_Clear();
                ps1 = "";
            }
        }
    }

    // Get sys.ps2 (as UTF-8)
    PyObject *ps2_obj = NULL;
    const char *ps2 = "";
    if (_PySys_GetOptionalAttr(&_Py_ID(ps2), &attr) < 0) {
        PyErr_Clear();
    }
    else if (attr != NULL) {
        ps2_obj = PyObject_Str(attr);
        Py_DECREF(attr);
        if (ps2_obj == NULL) {
            PyErr_Clear();
        }
        else if (PyUnicode_Check(ps2_obj)) {
            ps2 = PyUnicode_AsUTF8(ps2_obj);
            if (ps2 == NULL) {
                PyErr_Clear();
                ps2 = "";
            }
        }
    }

    int errcode = 0;
    *pmod = _PyParser_InteractiveASTFromFile(fp, filename, encoding,
                                             Py_single_input, ps1, ps2,
                                             flags, &errcode, interactive_src, arena);
    Py_XDECREF(ps1_obj);
    Py_XDECREF(ps2_obj);
    Py_XDECREF(encoding_obj);

    if (*pmod == NULL) {
        if (errcode == E_EOF) {
            PyErr_Clear();
            return E_EOF;
        }
        return -1;
    }
    return 0;
}


/* A PyRun_InteractiveOneObject() auxiliary function that does not print the
 * error on failure. */
static int
PyRun_InteractiveOneObjectEx(FILE *fp, PyObject *filename,
                             PyCompilerFlags *flags)
{
    PyArena *arena = _PyArena_New();
    if (arena == NULL) {
        return -1;
    }

    mod_ty mod;
    PyObject *interactive_src;
    int parse_res = pyrun_one_parse_ast(fp, filename, flags, arena, &mod, &interactive_src);
    if (parse_res != 0) {
        _PyArena_Free(arena);
        return parse_res;
    }

    PyObject *main_module = PyImport_AddModuleRef("__main__");
    if (main_module == NULL) {
        _PyArena_Free(arena);
        return -1;
    }
    PyObject *main_dict = PyModule_GetDict(main_module);  // borrowed ref

    PyObject *res = run_mod(mod, filename, main_dict, main_dict, flags, arena, interactive_src, 1);
    Py_INCREF(interactive_src);
    _PyArena_Free(arena);
    Py_DECREF(main_module);
    if (res == NULL) {
        PyThreadState *tstate = _PyThreadState_GET();
        PyObject *exc = _PyErr_GetRaisedException(tstate);
        if (PyType_IsSubtype(Py_TYPE(exc),
                             (PyTypeObject *) PyExc_SyntaxError))
        {
            /* fix "text" attribute */
            assert(interactive_src != NULL);
            PyObject *xs = PyUnicode_Splitlines(interactive_src, 1);
            if (xs == NULL) {
                goto error;
            }
            PyObject *exc_lineno = PyObject_GetAttr(exc, &_Py_ID(lineno));
            if (exc_lineno == NULL) {
                Py_DECREF(xs);
                goto error;
            }
            int n = PyLong_AsInt(exc_lineno);
            Py_DECREF(exc_lineno);
            if (n <= 0 || n > PyList_GET_SIZE(xs)) {
                Py_DECREF(xs);
                goto error;
            }
            PyObject *line = PyList_GET_ITEM(xs, n - 1);
            PyObject_SetAttr(exc, &_Py_ID(text), line);
            Py_DECREF(xs);
        }
error:
        Py_DECREF(interactive_src);
        _PyErr_SetRaisedException(tstate, exc);
        return -1;
    }
    Py_DECREF(interactive_src);
    Py_DECREF(res);

    flush_io();
    return 0;
}

int
PyRun_InteractiveOneObject(FILE *fp, PyObject *filename, PyCompilerFlags *flags)
{
    int res;

    res = PyRun_InteractiveOneObjectEx(fp, filename, flags);
    if (res == -1) {
        PyErr_Print();
        flush_io();
    }
    return res;
}

int
PyRun_InteractiveOneFlags(FILE *fp, const char *filename_str, PyCompilerFlags *flags)
{
    PyObject *filename;
    int res;

    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL) {
        PyErr_Print();
        return -1;
    }
    res = PyRun_InteractiveOneObject(fp, filename, flags);
    Py_DECREF(filename);
    return res;
}


/* Check whether a file maybe a pyc file: Look at the extension,
   the file type, and, if we may close it, at the first few bytes. */

static int
maybe_pyc_file(FILE *fp, PyObject *filename, int closeit)
{
    PyObject *ext = PyUnicode_FromString(".pyc");
    if (ext == NULL) {
        return -1;
    }
    Py_ssize_t endswith = PyUnicode_Tailmatch(filename, ext, 0, PY_SSIZE_T_MAX, +1);
    Py_DECREF(ext);
    if (endswith) {
        return 1;
    }

    /* Only look into the file if we are allowed to close it, since
       it then should also be seekable. */
    if (!closeit) {
        return 0;
    }

    /* Read only two bytes of the magic. If the file was opened in
       text mode, the bytes 3 and 4 of the magic (\r\n) might not
       be read as they are on disk. */
    unsigned int halfmagic = PyImport_GetMagicNumber() & 0xFFFF;
    unsigned char buf[2];
    /* Mess:  In case of -x, the stream is NOT at its start now,
       and ungetc() was used to push back the first newline,
       which makes the current stream position formally undefined,
       and a x-platform nightmare.
       Unfortunately, we have no direct way to know whether -x
       was specified.  So we use a terrible hack:  if the current
       stream position is not 0, we assume -x was specified, and
       give up.  Bug 132850 on SourceForge spells out the
       hopelessness of trying anything else (fseek and ftell
       don't work predictably x-platform for text-mode files).
    */
    int ispyc = 0;
    if (ftell(fp) == 0) {
        if (fread(buf, 1, 2, fp) == 2 &&
            ((unsigned int)buf[1]<<8 | buf[0]) == halfmagic)
            ispyc = 1;
        rewind(fp);
    }
    return ispyc;
}


static int
set_main_loader(PyObject *d, PyObject *filename, const char *loader_name)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyObject *loader_type = _PyImport_GetImportlibExternalLoader(interp,
                                                                 loader_name);
    if (loader_type == NULL) {
        return -1;
    }

    PyObject *loader = PyObject_CallFunction(loader_type,
                                             "sO", "__main__", filename);
    Py_DECREF(loader_type);
    if (loader == NULL) {
        return -1;
    }

    if (PyDict_SetItemString(d, "__loader__", loader) < 0) {
        Py_DECREF(loader);
        return -1;
    }
    Py_DECREF(loader);
    return 0;
}


int
_PyRun_SimpleFileObject(FILE *fp, PyObject *filename, int closeit,
                        PyCompilerFlags *flags)
{
    int ret = -1;

    PyObject *main_module = PyImport_AddModuleRef("__main__");
    if (main_module == NULL)
        return -1;
    PyObject *dict = PyModule_GetDict(main_module);  // borrowed ref

    int set_file_name = 0;
    int has_file = PyDict_ContainsString(dict, "__file__");
    if (has_file < 0) {
        goto done;
    }
    if (!has_file) {
        if (PyDict_SetItemString(dict, "__file__", filename) < 0) {
            goto done;
        }
        if (PyDict_SetItemString(dict, "__cached__", Py_None) < 0) {
            goto done;
        }
        set_file_name = 1;
    }

    int pyc = maybe_pyc_file(fp, filename, closeit);
    if (pyc < 0) {
        goto done;
    }

    PyObject *v;
    if (pyc) {
        FILE *pyc_fp;
        /* Try to run a pyc file. First, re-open in binary */
        if (closeit) {
            fclose(fp);
        }

        pyc_fp = Py_fopen(filename, "rb");
        if (pyc_fp == NULL) {
            fprintf(stderr, "python: Can't reopen .pyc file\n");
            goto done;
        }

        if (set_main_loader(dict, filename, "SourcelessFileLoader") < 0) {
            fprintf(stderr, "python: failed to set __main__.__loader__\n");
            ret = -1;
            fclose(pyc_fp);
            goto done;
        }
        v = run_pyc_file(pyc_fp, dict, dict, flags);
    } else {
        /* When running from stdin, leave __main__.__loader__ alone */
        if ((!PyUnicode_Check(filename) || !PyUnicode_EqualToUTF8(filename, "<stdin>")) &&
            set_main_loader(dict, filename, "SourceFileLoader") < 0) {
            fprintf(stderr, "python: failed to set __main__.__loader__\n");
            ret = -1;
            goto done;
        }
        v = pyrun_file(fp, filename, Py_file_input, dict, dict,
                       closeit, flags);
    }
    flush_io();
    if (v == NULL) {
        Py_CLEAR(main_module);
        PyErr_Print();
        goto done;
    }
    Py_DECREF(v);
    ret = 0;

  done:
    if (set_file_name) {
        if (PyDict_PopString(dict, "__file__", NULL) < 0) {
            PyErr_Print();
        }
        if (PyDict_PopString(dict, "__cached__", NULL) < 0) {
            PyErr_Print();
        }
    }
    Py_XDECREF(main_module);
    return ret;
}


int
PyRun_SimpleFileExFlags(FILE *fp, const char *filename, int closeit,
                        PyCompilerFlags *flags)
{
    PyObject *filename_obj = PyUnicode_DecodeFSDefault(filename);
    if (filename_obj == NULL) {
        return -1;
    }
    int res = _PyRun_SimpleFileObject(fp, filename_obj, closeit, flags);
    Py_DECREF(filename_obj);
    return res;
}


int
_PyRun_SimpleStringFlagsWithName(const char *command, const char* name, PyCompilerFlags *flags) {
    PyObject *main_module = PyImport_AddModuleRef("__main__");
    if (main_module == NULL) {
        return -1;
    }
    PyObject *dict = PyModule_GetDict(main_module);  // borrowed ref

    PyObject *res = NULL;
    if (name == NULL) {
        res = PyRun_StringFlags(command, Py_file_input, dict, dict, flags);
    } else {
        PyObject* the_name = PyUnicode_FromString(name);
        if (!the_name) {
            PyErr_Print();
            return -1;
        }
        res = _PyRun_StringFlagsWithName(command, the_name, Py_file_input, dict, dict, flags, 0);
        Py_DECREF(the_name);
    }
    Py_DECREF(main_module);
    if (res == NULL) {
        PyErr_Print();
        return -1;
    }

    Py_DECREF(res);
    return 0;
}

int
PyRun_SimpleStringFlags(const char *command, PyCompilerFlags *flags)
{
    return _PyRun_SimpleStringFlagsWithName(command, NULL, flags);
}

static int
parse_exit_code(PyObject *code, int *exitcode_p)
{
    if (PyLong_Check(code)) {
        // gh-125842: Use a long long to avoid an overflow error when `long`
        // is 32-bit. We still truncate the result to an int.
        int exitcode = (int)PyLong_AsLongLong(code);
        if (exitcode == -1 && PyErr_Occurred()) {
            // On overflow or other error, clear the exception and use -1
            // as the exit code to match historical Python behavior.
            PyErr_Clear();
            *exitcode_p = -1;
            return 1;
        }
        *exitcode_p = exitcode;
        return 1;
    }
    else if (code == Py_None) {
        *exitcode_p = 0;
        return 1;
    }
    return 0;
}

int
_Py_HandleSystemExitAndKeyboardInterrupt(int *exitcode_p)
{
    if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
        _Py_atomic_store_int(&_PyRuntime.signals.unhandled_keyboard_interrupt, 1);
        return 0;
    }

    int inspect = _Py_GetConfig()->inspect;
    if (inspect) {
        /* Don't exit if -i flag was given. This flag is set to 0
         * when entering interactive mode for inspecting. */
        return 0;
    }

    if (!PyErr_ExceptionMatches(PyExc_SystemExit)) {
        return 0;
    }

    fflush(stdout);

    PyObject *exc = PyErr_GetRaisedException();
    assert(exc != NULL && PyExceptionInstance_Check(exc));

    PyObject *code = PyObject_GetAttr(exc, &_Py_ID(code));
    if (code == NULL) {
        // If the exception has no 'code' attribute, print the exception below
        PyErr_Clear();
    }
    else if (parse_exit_code(code, exitcode_p)) {
        Py_DECREF(code);
        Py_CLEAR(exc);
        return 1;
    }
    else {
        // If code is not an int or None, print it below
        Py_SETREF(exc, code);
    }

    PyObject *sys_stderr;
    if (_PySys_GetOptionalAttr(&_Py_ID(stderr), &sys_stderr) < 0) {
        PyErr_Clear();
    }
    else if (sys_stderr != NULL && sys_stderr != Py_None) {
        if (PyFile_WriteObject(exc, sys_stderr, Py_PRINT_RAW) < 0) {
            PyErr_Clear();
        }
    }
    else {
        if (PyObject_Print(exc, stderr, Py_PRINT_RAW) < 0) {
            PyErr_Clear();
        }
        fflush(stderr);
    }
    PySys_WriteStderr("\n");
    Py_XDECREF(sys_stderr);
    Py_CLEAR(exc);
    *exitcode_p = 1;
    return 1;
}


static void
handle_system_exit(void)
{
    int exitcode;
    if (_Py_HandleSystemExitAndKeyboardInterrupt(&exitcode)) {
        Py_Exit(exitcode);
    }
}


static void
_PyErr_PrintEx(PyThreadState *tstate, int set_sys_last_vars)
{
    PyObject *typ = NULL, *tb = NULL, *hook = NULL;
    handle_system_exit();

    PyObject *exc = _PyErr_GetRaisedException(tstate);
    if (exc == NULL) {
        goto done;
    }
    assert(PyExceptionInstance_Check(exc));
    typ = Py_NewRef(Py_TYPE(exc));
    tb = PyException_GetTraceback(exc);
    if (tb == NULL) {
        tb = Py_NewRef(Py_None);
    }

    if (set_sys_last_vars) {
        if (_PySys_SetAttr(&_Py_ID(last_exc), exc) < 0) {
            _PyErr_Clear(tstate);
        }
        /* Legacy version: */
        if (_PySys_SetAttr(&_Py_ID(last_type), typ) < 0) {
            _PyErr_Clear(tstate);
        }
        if (_PySys_SetAttr(&_Py_ID(last_value), exc) < 0) {
            _PyErr_Clear(tstate);
        }
        if (_PySys_SetAttr(&_Py_ID(last_traceback), tb) < 0) {
            _PyErr_Clear(tstate);
        }
    }
    if (_PySys_GetOptionalAttr(&_Py_ID(excepthook), &hook) < 0) {
        PyErr_Clear();
    }
    if (_PySys_Audit(tstate, "sys.excepthook", "OOOO", hook ? hook : Py_None,
                     typ, exc, tb) < 0) {
        if (PyErr_ExceptionMatches(PyExc_RuntimeError)) {
            PyErr_Clear();
            goto done;
        }
        PyErr_FormatUnraisable("Exception ignored in audit hook");
    }
    if (hook) {
        PyObject* args[3] = {typ, exc, tb};
        PyObject *result = PyObject_Vectorcall(hook, args, 3, NULL);
        if (result == NULL) {
            handle_system_exit();

            PyObject *exc2 = _PyErr_GetRaisedException(tstate);
            assert(exc2 && PyExceptionInstance_Check(exc2));
            fflush(stdout);
            PySys_WriteStderr("Error in sys.excepthook:\n");
            PyErr_DisplayException(exc2);
            PySys_WriteStderr("\nOriginal exception was:\n");
            PyErr_DisplayException(exc);
            Py_DECREF(exc2);
        }
        else {
            Py_DECREF(result);
        }
    }
    else {
        PySys_WriteStderr("sys.excepthook is missing\n");
        PyErr_DisplayException(exc);
    }

done:
    Py_XDECREF(hook);
    Py_XDECREF(typ);
    Py_XDECREF(exc);
    Py_XDECREF(tb);
}

void
_PyErr_Print(PyThreadState *tstate)
{
    _PyErr_PrintEx(tstate, 1);
}

void
PyErr_PrintEx(int set_sys_last_vars)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _PyErr_PrintEx(tstate, set_sys_last_vars);
}

void
PyErr_Print(void)
{
    PyErr_PrintEx(1);
}

struct exception_print_context
{
    PyObject *file;
    PyObject *seen;            // Prevent cycles in recursion
};

static int
print_exception_invalid_type(struct exception_print_context *ctx,
                             PyObject *value)
{
    PyObject *f = ctx->file;
    const char *const msg = "TypeError: print_exception(): Exception expected "
                            "for value, ";
    if (PyFile_WriteString(msg, f) < 0) {
        return -1;
    }
    if (PyFile_WriteString(Py_TYPE(value)->tp_name, f) < 0) {
        return -1;
    }
    if (PyFile_WriteString(" found\n", f) < 0) {
        return -1;
    }
    return 0;
}

static int
print_exception_traceback(struct exception_print_context *ctx, PyObject *value)
{
    PyObject *f = ctx->file;
    int err = 0;

    PyObject *tb = PyException_GetTraceback(value);
    if (tb && tb != Py_None) {
        const char *header = EXCEPTION_TB_HEADER;
        err = _PyTraceBack_Print(tb, header, f);
    }
    Py_XDECREF(tb);
    return err;
}

static int
print_exception_file_and_line(struct exception_print_context *ctx,
                              PyObject **value_p)
{
    PyObject *f = ctx->file;

    PyObject *tmp;
    int res = PyObject_GetOptionalAttr(*value_p, &_Py_ID(print_file_and_line), &tmp);
    if (res <= 0) {
        if (res < 0) {
            PyErr_Clear();
        }
        return 0;
    }
    Py_DECREF(tmp);

    PyObject *filename = NULL;
    Py_ssize_t lineno = 0;
    PyObject* v = PyObject_GetAttr(*value_p, &_Py_ID(filename));
    if (!v) {
        return -1;
    }
    if (v == Py_None) {
        Py_DECREF(v);
        _Py_DECLARE_STR(anon_string, "<string>");
        filename = Py_NewRef(&_Py_STR(anon_string));
    }
    else {
        filename = v;
    }

    PyObject *line = PyUnicode_FromFormat("  File \"%S\", line %zd\n",
                                          filename, lineno);
    Py_DECREF(filename);
    if (line == NULL) {
        goto error;
    }
    if (PyFile_WriteObject(line, f, Py_PRINT_RAW) < 0) {
        goto error;
    }
    Py_CLEAR(line);

    assert(!PyErr_Occurred());
    return 0;

error:
    Py_XDECREF(line);
    return -1;
}

/* Prints the message line: module.qualname[: str(exc)] */
static int
print_exception_message(struct exception_print_context *ctx, PyObject *type,
                        PyObject *value)
{
    PyObject *f = ctx->file;

    if (PyErr_GivenExceptionMatches(value, PyExc_MemoryError)) {
        // The Python APIs in this function require allocating memory
        // for various objects. If we're out of memory, we can't do that,
        return -1;
    }

    assert(PyExceptionClass_Check(type));

    PyObject *modulename = PyObject_GetAttr(type, &_Py_ID(__module__));
    if (modulename == NULL || !PyUnicode_Check(modulename)) {
        Py_XDECREF(modulename);
        PyErr_Clear();
        if (PyFile_WriteString("<unknown>.", f) < 0) {
            return -1;
        }
    }
    else {
        if (!_PyUnicode_Equal(modulename, &_Py_ID(builtins)) &&
            !_PyUnicode_Equal(modulename, &_Py_ID(__main__)))
        {
            int res = PyFile_WriteObject(modulename, f, Py_PRINT_RAW);
            Py_DECREF(modulename);
            if (res < 0) {
                return -1;
            }
            if (PyFile_WriteString(".", f) < 0) {
                return -1;
            }
        }
        else {
            Py_DECREF(modulename);
        }
    }

    PyObject *qualname = PyType_GetQualName((PyTypeObject *)type);
    if (qualname == NULL || !PyUnicode_Check(qualname)) {
        Py_XDECREF(qualname);
        PyErr_Clear();
        if (PyFile_WriteString("<unknown>", f) < 0) {
            return -1;
        }
    }
    else {
        int res = PyFile_WriteObject(qualname, f, Py_PRINT_RAW);
        Py_DECREF(qualname);
        if (res < 0) {
            return -1;
        }
    }

    if (Py_IsNone(value)) {
        return 0;
    }

    PyObject *s = PyObject_Str(value);
    if (s == NULL) {
        PyErr_Clear();
        if (PyFile_WriteString(": <exception str() failed>", f) < 0) {
            return -1;
        }
    }
    else {
        /* only print colon if the str() of the
           object is not the empty string
        */
        if (!PyUnicode_Check(s) || PyUnicode_GetLength(s) != 0) {
            if (PyFile_WriteString(": ", f) < 0) {
                Py_DECREF(s);
                return -1;
            }
        }
        int res = PyFile_WriteObject(s, f, Py_PRINT_RAW);
        Py_DECREF(s);
        if (res < 0) {
            return -1;
        }
    }

    return 0;
}

static int
print_exception(struct exception_print_context *ctx, PyObject *value)
{
    PyObject *f = ctx->file;

    if (!PyExceptionInstance_Check(value)) {
        return print_exception_invalid_type(ctx, value);
    }

    Py_INCREF(value);
    fflush(stdout);

    if (print_exception_traceback(ctx, value) < 0) {
        goto error;
    }

    /* grab the type and notes now because value can change below */
    PyObject *type = (PyObject *) Py_TYPE(value);

    if (print_exception_file_and_line(ctx, &value) < 0) {
        goto error;
    }
    if (print_exception_message(ctx, type, value) < 0) {
        goto error;
    }
    if (PyFile_WriteString("\n", f) < 0) {
        goto error;
    }
    Py_DECREF(value);
    assert(!PyErr_Occurred());
    return 0;
error:
    Py_DECREF(value);
    return -1;
}

static const char cause_message[] =
    "The above exception was the direct cause "
    "of the following exception:\n";

static const char context_message[] =
    "During handling of the above exception, "
    "another exception occurred:\n";

static int
print_exception_recursive(struct exception_print_context*, PyObject*);

static int
print_chained(struct exception_print_context* ctx, PyObject *value,
              const char * message, const char *tag)
{
    PyObject *f = ctx->file;
    if (_Py_EnterRecursiveCall(" in print_chained")) {
        return -1;
    }
    int res = print_exception_recursive(ctx, value);
    _Py_LeaveRecursiveCall();
    if (res < 0) {
        return -1;
    }

    if (PyFile_WriteString("\n", f) < 0) {
        return -1;
    }
    if (PyFile_WriteString(message, f) < 0) {
        return -1;
    }
    if (PyFile_WriteString("\n", f) < 0) {
        return -1;
    }
    return 0;
}

/* Return true if value is in seen or there was a lookup error.
 * Return false if lookup succeeded and the item was not found.
 * We suppress errors because this makes us err on the side of
 * under-printing which is better than over-printing irregular
 * exceptions (e.g., unhashable ones).
 */
static bool
print_exception_seen_lookup(struct exception_print_context *ctx,
                            PyObject *value)
{
    PyObject *check_id = PyLong_FromVoidPtr(value);
    if (check_id == NULL) {
        PyErr_Clear();
        return true;
    }

    int in_seen = PySet_Contains(ctx->seen, check_id);
    Py_DECREF(check_id);
    if (in_seen == -1) {
        PyErr_Clear();
        return true;
    }

    if (in_seen == 1) {
        /* value is in seen */
        return true;
    }
    return false;
}

static int
print_exception_cause_and_context(struct exception_print_context *ctx,
                                  PyObject *value)
{
    PyObject *value_id = PyLong_FromVoidPtr(value);
    if (value_id == NULL || PySet_Add(ctx->seen, value_id) == -1) {
        PyErr_Clear();
        Py_XDECREF(value_id);
        return 0;
    }
    Py_DECREF(value_id);

    if (!PyExceptionInstance_Check(value)) {
        return 0;
    }

    PyObject *cause = PyException_GetCause(value);
    if (cause) {
        int err = 0;
        if (!print_exception_seen_lookup(ctx, cause)) {
            err = print_chained(ctx, cause, cause_message, "cause");
        }
        Py_DECREF(cause);
        return err;
    }
    if (((PyBaseExceptionObject *)value)->suppress_context) {
        return 0;
    }
    PyObject *context = PyException_GetContext(value);
    if (context) {
        int err = 0;
        if (!print_exception_seen_lookup(ctx, context)) {
            err = print_chained(ctx, context, context_message, "context");
        }
        Py_DECREF(context);
        return err;
    }
    return 0;
}

static int
print_exception_recursive(struct exception_print_context *ctx, PyObject *value)
{
    if (_Py_EnterRecursiveCall(" in print_exception_recursive")) {
        return -1;
    }
    if (ctx->seen != NULL) {
        /* Exception chaining */
        if (print_exception_cause_and_context(ctx, value) < 0) {
            goto error;
        }
    }
    if (print_exception(ctx, value) < 0) {
        goto error;
    }
    assert(!PyErr_Occurred());

    _Py_LeaveRecursiveCall();
    return 0;
error:
    _Py_LeaveRecursiveCall();
    return -1;
}

void
_PyErr_Display(PyObject *file, PyObject *unused, PyObject *value, PyObject *tb)
{
    assert(value != NULL);
    assert(file != NULL && file != Py_None);
    if (PyExceptionInstance_Check(value)
        && tb != NULL && PyTraceBack_Check(tb)) {
        /* Put the traceback on the exception, otherwise it won't get
           displayed.  See issue #18776. */
        PyObject *cur_tb = PyException_GetTraceback(value);
        if (cur_tb == NULL) {
            PyException_SetTraceback(value, tb);
        }
        else {
            Py_DECREF(cur_tb);
        }
    }

    // Try first with the stdlib traceback module
    PyObject *print_exception_fn = PyImport_ImportModuleAttrString(
        "traceback",
        "_print_exception_bltin");
    if (print_exception_fn == NULL || !PyCallable_Check(print_exception_fn)) {
        goto fallback;
    }

    PyObject* result = PyObject_CallOneArg(print_exception_fn, value);

    Py_XDECREF(print_exception_fn);
    if (result) {
        Py_DECREF(result);
        return;
    }
fallback:
#ifdef Py_DEBUG
     if (PyErr_Occurred()) {
         PyErr_FormatUnraisable(
             "Exception ignored in the internal traceback machinery");
     }
#endif
    PyErr_Clear();
    struct exception_print_context ctx;
    ctx.file = file;

    /* We choose to ignore seen being possibly NULL, and report
       at least the main exception (it could be a MemoryError).
    */
    ctx.seen = PySet_New(NULL);
    if (ctx.seen == NULL) {
        PyErr_Clear();
    }
    if (print_exception_recursive(&ctx, value) < 0) {
        PyErr_Clear();
        _PyObject_Dump(value);
        fprintf(stderr, "lost sys.stderr\n");
    }
    Py_XDECREF(ctx.seen);

    /* Call file.flush() */
    if (_PyFile_Flush(file) < 0) {
        /* Silently ignore file.flush() error */
        PyErr_Clear();
    }
}

void
PyErr_Display(PyObject *unused, PyObject *value, PyObject *tb)
{
    PyObject *file;
    if (_PySys_GetOptionalAttr(&_Py_ID(stderr), &file) < 0) {
        PyObject *exc = PyErr_GetRaisedException();
        _PyObject_Dump(value);
        fprintf(stderr, "lost sys.stderr\n");
        _PyObject_Dump(exc);
        Py_DECREF(exc);
        return;
    }
    if (file == NULL) {
        _PyObject_Dump(value);
        fprintf(stderr, "lost sys.stderr\n");
        return;
    }
    if (file == Py_None) {
        Py_DECREF(file);
        return;
    }
    _PyErr_Display(file, NULL, value, tb);
    Py_DECREF(file);
}

void _PyErr_DisplayException(PyObject *file, PyObject *exc)
{
    _PyErr_Display(file, NULL, exc, NULL);
}

void PyErr_DisplayException(PyObject *exc)
{
    PyErr_Display(NULL, exc, NULL);
}

static PyObject *
_PyRun_StringFlagsWithName(const char *str, PyObject* name, int start,
                           PyObject *globals, PyObject *locals, PyCompilerFlags *flags,
                           int generate_new_source)
{
    PyObject *ret = NULL;
    mod_ty mod;
    PyArena *arena;

    arena = _PyArena_New();
    if (arena == NULL)
        return NULL;

    PyObject* source = NULL;
    _Py_DECLARE_STR(anon_string, "<string>");

    if (name) {
        source = PyUnicode_FromString(str);
        if (!source) {
            PyErr_Clear();
        }
    } else {
        name = &_Py_STR(anon_string);
    }

    mod = _PyParser_ASTFromString(str, name, start, flags, arena);

   if (mod != NULL) {
        ret = run_mod(mod, name, globals, locals, flags, arena, source, generate_new_source);
    }
    Py_XDECREF(source);
    _PyArena_Free(arena);
    return ret;
}


PyObject *
PyRun_StringFlags(const char *str, int start, PyObject *globals,
                     PyObject *locals, PyCompilerFlags *flags) {

    return _PyRun_StringFlagsWithName(str, NULL, start, globals, locals, flags, 0);
}

static PyObject *
pyrun_file(FILE *fp, PyObject *filename, int start, PyObject *globals,
           PyObject *locals, int closeit, PyCompilerFlags *flags)
{
    PyArena *arena = _PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    mod_ty mod;
    mod = _PyParser_ASTFromFile(fp, filename, NULL, start, NULL, NULL,
                                flags, NULL, arena);

    if (closeit) {
        fclose(fp);
    }

    PyObject *ret;
    if (mod != NULL) {
        ret = run_mod(mod, filename, globals, locals, flags, arena, NULL, 0);
    }
    else {
        ret = NULL;
    }
    _PyArena_Free(arena);

    return ret;
}


PyObject *
PyRun_FileExFlags(FILE *fp, const char *filename, int start, PyObject *globals,
                  PyObject *locals, int closeit, PyCompilerFlags *flags)
{
    PyObject *filename_obj = PyUnicode_DecodeFSDefault(filename);
    if (filename_obj == NULL) {
        return NULL;
    }

    PyObject *res = pyrun_file(fp, filename_obj, start, globals,
                               locals, closeit, flags);
    Py_DECREF(filename_obj);
    return res;

}

static void
flush_io_stream(PyThreadState *tstate, PyObject *name)
{
    PyObject *f;
    if (_PySys_GetOptionalAttr(name, &f) < 0) {
        PyErr_Clear();
    }
    if (f != NULL) {
        if (_PyFile_Flush(f) < 0) {
            PyErr_Clear();
        }
        Py_DECREF(f);
    }
}

static void
flush_io(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    flush_io_stream(tstate, &_Py_ID(stderr));
    flush_io_stream(tstate, &_Py_ID(stdout));
    _PyErr_SetRaisedException(tstate, exc);
}

static PyObject *
run_eval_code_obj(PyThreadState *tstate, PyCodeObject *co, PyObject *globals, PyObject *locals)
{
    /* Set globals['__builtins__'] if it doesn't exist */
    if (!globals || !PyDict_Check(globals)) {
        PyErr_SetString(PyExc_SystemError, "globals must be a real dict");
        return NULL;
    }
    int has_builtins = PyDict_ContainsString(globals, "__builtins__");
    if (has_builtins < 0) {
        return NULL;
    }
    if (!has_builtins) {
        if (PyDict_SetItemString(globals, "__builtins__",
                                 tstate->interp->builtins) < 0)
        {
            return NULL;
        }
    }

    return PyEval_EvalCode((PyObject*)co, globals, locals);
}

static PyObject *
run_mod(mod_ty mod, PyObject *filename, PyObject *globals, PyObject *locals,
            PyCompilerFlags *flags, PyArena *arena, PyObject* interactive_src,
            int generate_new_source)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject* interactive_filename = filename;
    if (interactive_src) {
        PyInterpreterState *interp = tstate->interp;
        if (generate_new_source) {
            interactive_filename = PyUnicode_FromFormat(
                "%U-%d", filename, interp->_interactive_src_count++);
        } else {
            Py_INCREF(interactive_filename);
        }
        if (interactive_filename == NULL) {
            return NULL;
        }
    }

    PyCodeObject *co = _PyAST_Compile(mod, interactive_filename, flags, -1, arena);
    if (co == NULL) {
        if (interactive_src) {
            Py_DECREF(interactive_filename);
        }
        return NULL;
    }

    if (interactive_src) {
        PyObject *print_tb_func = PyImport_ImportModuleAttrString(
            "linecache",
            "_register_code");
        if (print_tb_func == NULL) {
            Py_DECREF(co);
            Py_DECREF(interactive_filename);
            return NULL;
        }

        if (!PyCallable_Check(print_tb_func)) {
            Py_DECREF(co);
            Py_DECREF(interactive_filename);
            Py_DECREF(print_tb_func);
            PyErr_SetString(PyExc_ValueError, "linecache._register_code is not callable");
            return NULL;
        }

        PyObject* result = PyObject_CallFunction(
            print_tb_func, "OOO",
            co,
            interactive_src,
            filename
        );

        Py_DECREF(interactive_filename);

        Py_XDECREF(print_tb_func);
        Py_XDECREF(result);
        if (!result) {
            Py_DECREF(co);
            return NULL;
        }
    }

    if (_PySys_Audit(tstate, "exec", "O", co) < 0) {
        Py_DECREF(co);
        return NULL;
    }

    PyObject *v = run_eval_code_obj(tstate, co, globals, locals);
    Py_DECREF(co);
    return v;
}

static PyObject *
run_pyc_file(FILE *fp, PyObject *globals, PyObject *locals,
             PyCompilerFlags *flags)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyCodeObject *co;
    PyObject *v;
    long magic;
    long PyImport_GetMagicNumber(void);

    magic = PyMarshal_ReadLongFromFile(fp);
    if (magic != PyImport_GetMagicNumber()) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_RuntimeError,
                       "Bad magic number in .pyc file");
        goto error;
    }
    /* Skip the rest of the header. */
    (void) PyMarshal_ReadLongFromFile(fp);
    (void) PyMarshal_ReadLongFromFile(fp);
    (void) PyMarshal_ReadLongFromFile(fp);
    if (PyErr_Occurred()) {
        goto error;
    }
    v = PyMarshal_ReadLastObjectFromFile(fp);
    if (v == NULL || !PyCode_Check(v)) {
        Py_XDECREF(v);
        PyErr_SetString(PyExc_RuntimeError,
                   "Bad code object in .pyc file");
        goto error;
    }
    fclose(fp);
    co = (PyCodeObject *)v;
    v = run_eval_code_obj(tstate, co, globals, locals);
    if (v && flags)
        flags->cf_flags |= (co->co_flags & PyCF_MASK);
    Py_DECREF(co);
    return v;
error:
    fclose(fp);
    return NULL;
}

PyObject *
Py_CompileStringObject(const char *str, PyObject *filename, int start,
                       PyCompilerFlags *flags, int optimize)
{
    PyCodeObject *co;
    mod_ty mod;
    PyArena *arena = _PyArena_New();
    if (arena == NULL)
        return NULL;

    mod = _PyParser_ASTFromString(str, filename, start, flags, arena);
    if (mod == NULL) {
        _PyArena_Free(arena);
        return NULL;
    }
    if (flags && (flags->cf_flags & PyCF_ONLY_AST)) {
        int syntax_check_only = ((flags->cf_flags & PyCF_OPTIMIZED_AST) == PyCF_ONLY_AST); /* unoptiomized AST */
        if (_PyCompile_AstOptimize(mod, filename, flags, optimize, arena, syntax_check_only) < 0) {
            _PyArena_Free(arena);
            return NULL;
        }
        PyObject *result = PyAST_mod2obj(mod);
        _PyArena_Free(arena);
        return result;
    }
    co = _PyAST_Compile(mod, filename, flags, optimize, arena);
    _PyArena_Free(arena);
    return (PyObject *)co;
}

PyObject *
Py_CompileStringExFlags(const char *str, const char *filename_str, int start,
                        PyCompilerFlags *flags, int optimize)
{
    PyObject *filename, *co;
    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL)
        return NULL;
    co = Py_CompileStringObject(str, filename, start, flags, optimize);
    Py_DECREF(filename);
    return co;
}

const char *
_Py_SourceAsString(PyObject *cmd, const char *funcname, const char *what, PyCompilerFlags *cf, PyObject **cmd_copy)
{
    const char *str;
    Py_ssize_t size;
    Py_buffer view;

    *cmd_copy = NULL;
    if (PyUnicode_Check(cmd)) {
        cf->cf_flags |= PyCF_IGNORE_COOKIE;
        str = PyUnicode_AsUTF8AndSize(cmd, &size);
        if (str == NULL)
            return NULL;
    }
    else if (PyBytes_Check(cmd)) {
        str = PyBytes_AS_STRING(cmd);
        size = PyBytes_GET_SIZE(cmd);
    }
    else if (PyByteArray_Check(cmd)) {
        str = PyByteArray_AS_STRING(cmd);
        size = PyByteArray_GET_SIZE(cmd);
    }
    else if (PyObject_GetBuffer(cmd, &view, PyBUF_SIMPLE) == 0) {
        /* Copy to NUL-terminated buffer. */
        *cmd_copy = PyBytes_FromStringAndSize(
            (const char *)view.buf, view.len);
        PyBuffer_Release(&view);
        if (*cmd_copy == NULL) {
            return NULL;
        }
        str = PyBytes_AS_STRING(*cmd_copy);
        size = PyBytes_GET_SIZE(*cmd_copy);
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "%s() arg 1 must be a %s object",
            funcname, what);
        return NULL;
    }

    if (strlen(str) != (size_t)size) {
        PyErr_SetString(PyExc_SyntaxError,
            "source code string cannot contain null bytes");
        Py_CLEAR(*cmd_copy);
        return NULL;
    }
    return str;
}

#if defined(USE_STACKCHECK)

/* Stack checking */

/*
 * Return non-zero when we run out of memory on the stack; zero otherwise.
 */
int
PyOS_CheckStack(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _Py_ReachedRecursionLimit(tstate);
}

#endif /* USE_STACKCHECK */

/* Deprecated C API functions still provided for binary compatibility */

#undef PyRun_AnyFile
PyAPI_FUNC(int)
PyRun_AnyFile(FILE *fp, const char *name)
{
    return PyRun_AnyFileExFlags(fp, name, 0, NULL);
}

#undef PyRun_AnyFileEx
PyAPI_FUNC(int)
PyRun_AnyFileEx(FILE *fp, const char *name, int closeit)
{
    return PyRun_AnyFileExFlags(fp, name, closeit, NULL);
}

#undef PyRun_AnyFileFlags
PyAPI_FUNC(int)
PyRun_AnyFileFlags(FILE *fp, const char *name, PyCompilerFlags *flags)
{
    return PyRun_AnyFileExFlags(fp, name, 0, flags);
}

#undef PyRun_File
PyAPI_FUNC(PyObject *)
PyRun_File(FILE *fp, const char *p, int s, PyObject *g, PyObject *l)
{
    return PyRun_FileExFlags(fp, p, s, g, l, 0, NULL);
}

#undef PyRun_FileEx
PyAPI_FUNC(PyObject *)
PyRun_FileEx(FILE *fp, const char *p, int s, PyObject *g, PyObject *l, int c)
{
    return PyRun_FileExFlags(fp, p, s, g, l, c, NULL);
}

#undef PyRun_FileFlags
PyAPI_FUNC(PyObject *)
PyRun_FileFlags(FILE *fp, const char *p, int s, PyObject *g, PyObject *l,
                PyCompilerFlags *flags)
{
    return PyRun_FileExFlags(fp, p, s, g, l, 0, flags);
}

#undef PyRun_SimpleFile
PyAPI_FUNC(int)
PyRun_SimpleFile(FILE *f, const char *p)
{
    return PyRun_SimpleFileExFlags(f, p, 0, NULL);
}

#undef PyRun_SimpleFileEx
PyAPI_FUNC(int)
PyRun_SimpleFileEx(FILE *f, const char *p, int c)
{
    return PyRun_SimpleFileExFlags(f, p, c, NULL);
}


#undef PyRun_String
PyAPI_FUNC(PyObject *)
PyRun_String(const char *str, int s, PyObject *g, PyObject *l)
{
    return PyRun_StringFlags(str, s, g, l, NULL);
}

#undef PyRun_SimpleString
PyAPI_FUNC(int)
PyRun_SimpleString(const char *s)
{
    return PyRun_SimpleStringFlags(s, NULL);
}

#undef Py_CompileString
PyAPI_FUNC(PyObject *)
Py_CompileString(const char *str, const char *p, int s)
{
    return Py_CompileStringExFlags(str, p, s, NULL, -1);
}

#undef Py_CompileStringFlags
PyAPI_FUNC(PyObject *)
Py_CompileStringFlags(const char *str, const char *p, int s,
                      PyCompilerFlags *flags)
{
    return Py_CompileStringExFlags(str, p, s, flags, -1);
}

#undef PyRun_InteractiveOne
PyAPI_FUNC(int)
PyRun_InteractiveOne(FILE *f, const char *p)
{
    return PyRun_InteractiveOneFlags(f, p, NULL);
}

#undef PyRun_InteractiveLoop
PyAPI_FUNC(int)
PyRun_InteractiveLoop(FILE *f, const char *p)
{
    return PyRun_InteractiveLoopFlags(f, p, NULL);
}
