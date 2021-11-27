
/* Top level execution of Python code (including in __main__) */

/* To help control the interfaces between the startup, execution and
 * shutdown code, the phases are split across separate modules (bootstrap,
 * pythonrun, shutdown)
 */

/* TODO: Cull includes following phase split */

#include "Python.h"

#include "pycore_ast.h"           // PyAST_mod2obj
#include "pycore_compile.h"       // _PyAST_Compile()
#include "pycore_interp.h"        // PyInterpreterState.importlib
#include "pycore_object.h"        // _PyDebug_PrintTotalRefs(),
                                  // _PyType_GetQualName()
#include "pycore_parser.h"        // _PyParser_ASTFromString()
#include "pycore_pyerrors.h"      // _PyErr_Fetch, _Py_Offer_Suggestions
#include "pycore_pylifecycle.h"   // _Py_UnhandledKeyboardInterrupt
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_sysmodule.h"     // _PySys_Audit()

#include "token.h"                // INDENT
#include "errcode.h"              // E_EOF
#include "code.h"                 // PyCodeObject
#include "marshal.h"              // PyMarshal_ReadLongFromFile()

#ifdef MS_WINDOWS
#  include "malloc.h"             // alloca()
#endif

#ifdef MS_WINDOWS
#  undef BYTE
#  include "windows.h"
#endif


_Py_IDENTIFIER(builtins);
_Py_IDENTIFIER(excepthook);
_Py_IDENTIFIER(flush);
_Py_IDENTIFIER(last_traceback);
_Py_IDENTIFIER(last_type);
_Py_IDENTIFIER(last_value);
_Py_IDENTIFIER(ps1);
_Py_IDENTIFIER(ps2);
_Py_IDENTIFIER(stdin);
_Py_IDENTIFIER(stdout);
_Py_IDENTIFIER(stderr);
_Py_static_string(PyId_string, "<string>");

#ifdef __cplusplus
extern "C" {
#endif

/* Forward */
static void flush_io(void);
static PyObject *run_mod(mod_ty, PyObject *, PyObject *, PyObject *,
                          PyCompilerFlags *, PyArena *);
static PyObject *run_pyc_file(FILE *, PyObject *, PyObject *,
                              PyCompilerFlags *);
static int PyRun_InteractiveOneObjectEx(FILE *, PyObject *, PyCompilerFlags *);
static PyObject* pyrun_file(FILE *fp, PyObject *filename, int start,
                            PyObject *globals, PyObject *locals, int closeit,
                            PyCompilerFlags *flags);


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


/* Parse input from a file and execute it */
int
PyRun_AnyFileExFlags(FILE *fp, const char *filename, int closeit,
                     PyCompilerFlags *flags)
{
    PyObject *filename_obj;
    if (filename != NULL) {
        filename_obj = PyUnicode_DecodeFSDefault(filename);
        if (filename_obj == NULL) {
            PyErr_Print();
            return -1;
        }
    }
    else {
        filename_obj = NULL;
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

    PyObject *v = _PySys_GetObjectId(&PyId_ps1);
    if (v == NULL) {
        _PySys_SetObjectId(&PyId_ps1, v = PyUnicode_FromString(">>> "));
        Py_XDECREF(v);
    }
    v = _PySys_GetObjectId(&PyId_ps2);
    if (v == NULL) {
        _PySys_SetObjectId(&PyId_ps2, v = PyUnicode_FromString("... "));
        Py_XDECREF(v);
    }

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


/* A PyRun_InteractiveOneObject() auxiliary function that does not print the
 * error on failure. */
static int
PyRun_InteractiveOneObjectEx(FILE *fp, PyObject *filename,
                             PyCompilerFlags *flags)
{
    PyObject *m, *d, *v, *w, *oenc = NULL, *mod_name;
    mod_ty mod;
    PyArena *arena;
    const char *ps1 = "", *ps2 = "", *enc = NULL;
    int errcode = 0;
    _Py_IDENTIFIER(encoding);
    _Py_IDENTIFIER(__main__);

    mod_name = _PyUnicode_FromId(&PyId___main__); /* borrowed */
    if (mod_name == NULL) {
        return -1;
    }

    if (fp == stdin) {
        /* Fetch encoding from sys.stdin if possible. */
        v = _PySys_GetObjectId(&PyId_stdin);
        if (v && v != Py_None) {
            oenc = _PyObject_GetAttrId(v, &PyId_encoding);
            if (oenc)
                enc = PyUnicode_AsUTF8(oenc);
            if (!enc)
                PyErr_Clear();
        }
    }
    v = _PySys_GetObjectId(&PyId_ps1);
    if (v != NULL) {
        v = PyObject_Str(v);
        if (v == NULL)
            PyErr_Clear();
        else if (PyUnicode_Check(v)) {
            ps1 = PyUnicode_AsUTF8(v);
            if (ps1 == NULL) {
                PyErr_Clear();
                ps1 = "";
            }
        }
    }
    w = _PySys_GetObjectId(&PyId_ps2);
    if (w != NULL) {
        w = PyObject_Str(w);
        if (w == NULL)
            PyErr_Clear();
        else if (PyUnicode_Check(w)) {
            ps2 = PyUnicode_AsUTF8(w);
            if (ps2 == NULL) {
                PyErr_Clear();
                ps2 = "";
            }
        }
    }
    arena = _PyArena_New();
    if (arena == NULL) {
        Py_XDECREF(v);
        Py_XDECREF(w);
        Py_XDECREF(oenc);
        return -1;
    }

    mod = _PyParser_ASTFromFile(fp, filename, enc, Py_single_input,
                                ps1, ps2, flags, &errcode, arena);

    Py_XDECREF(v);
    Py_XDECREF(w);
    Py_XDECREF(oenc);
    if (mod == NULL) {
        _PyArena_Free(arena);
        if (errcode == E_EOF) {
            PyErr_Clear();
            return E_EOF;
        }
        return -1;
    }
    m = PyImport_AddModuleObject(mod_name);
    if (m == NULL) {
        _PyArena_Free(arena);
        return -1;
    }
    d = PyModule_GetDict(m);
    v = run_mod(mod, filename, d, d, flags, arena);
    _PyArena_Free(arena);
    if (v == NULL) {
        return -1;
    }
    Py_DECREF(v);
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
    PyObject *bootstrap = PyObject_GetAttrString(interp->importlib,
                                                 "_bootstrap_external");
    if (bootstrap == NULL) {
        return -1;
    }

    PyObject *loader_type = PyObject_GetAttrString(bootstrap, loader_name);
    Py_DECREF(bootstrap);
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
    PyObject *m, *d, *v;
    int set_file_name = 0, ret = -1;

    m = PyImport_AddModule("__main__");
    if (m == NULL)
        return -1;
    Py_INCREF(m);
    d = PyModule_GetDict(m);
    if (_PyDict_GetItemStringWithError(d, "__file__") == NULL) {
        if (PyErr_Occurred()) {
            goto done;
        }
        if (PyDict_SetItemString(d, "__file__", filename) < 0) {
            goto done;
        }
        if (PyDict_SetItemString(d, "__cached__", Py_None) < 0) {
            goto done;
        }
        set_file_name = 1;
    }

    int pyc = maybe_pyc_file(fp, filename, closeit);
    if (pyc < 0) {
        goto done;
    }

    if (pyc) {
        FILE *pyc_fp;
        /* Try to run a pyc file. First, re-open in binary */
        if (closeit) {
            fclose(fp);
        }

        pyc_fp = _Py_fopen_obj(filename, "rb");
        if (pyc_fp == NULL) {
            fprintf(stderr, "python: Can't reopen .pyc file\n");
            goto done;
        }

        if (set_main_loader(d, filename, "SourcelessFileLoader") < 0) {
            fprintf(stderr, "python: failed to set __main__.__loader__\n");
            ret = -1;
            fclose(pyc_fp);
            goto done;
        }
        v = run_pyc_file(pyc_fp, d, d, flags);
    } else {
        /* When running from stdin, leave __main__.__loader__ alone */
        if (PyUnicode_CompareWithASCIIString(filename, "<stdin>") != 0 &&
            set_main_loader(d, filename, "SourceFileLoader") < 0) {
            fprintf(stderr, "python: failed to set __main__.__loader__\n");
            ret = -1;
            goto done;
        }
        v = pyrun_file(fp, filename, Py_file_input, d, d,
                       closeit, flags);
    }
    flush_io();
    if (v == NULL) {
        Py_CLEAR(m);
        PyErr_Print();
        goto done;
    }
    Py_DECREF(v);
    ret = 0;
  done:
    if (set_file_name) {
        if (PyDict_DelItemString(d, "__file__")) {
            PyErr_Clear();
        }
        if (PyDict_DelItemString(d, "__cached__")) {
            PyErr_Clear();
        }
    }
    Py_XDECREF(m);
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
PyRun_SimpleStringFlags(const char *command, PyCompilerFlags *flags)
{
    PyObject *m, *d, *v;
    m = PyImport_AddModule("__main__");
    if (m == NULL)
        return -1;
    d = PyModule_GetDict(m);
    v = PyRun_StringFlags(command, Py_file_input, d, d, flags);
    if (v == NULL) {
        PyErr_Print();
        return -1;
    }
    Py_DECREF(v);
    return 0;
}

static int
parse_syntax_error(PyObject *err, PyObject **message, PyObject **filename,
                   Py_ssize_t *lineno, Py_ssize_t *offset,
                   Py_ssize_t* end_lineno, Py_ssize_t* end_offset,
                   PyObject **text)
{
    Py_ssize_t hold;
    PyObject *v;
    _Py_IDENTIFIER(msg);
    _Py_IDENTIFIER(filename);
    _Py_IDENTIFIER(lineno);
    _Py_IDENTIFIER(offset);
    _Py_IDENTIFIER(end_lineno);
    _Py_IDENTIFIER(end_offset);
    _Py_IDENTIFIER(text);

    *message = NULL;
    *filename = NULL;

    /* new style errors.  `err' is an instance */
    *message = _PyObject_GetAttrId(err, &PyId_msg);
    if (!*message)
        goto finally;

    v = _PyObject_GetAttrId(err, &PyId_filename);
    if (!v)
        goto finally;
    if (v == Py_None) {
        Py_DECREF(v);
        *filename = _PyUnicode_FromId(&PyId_string);
        if (*filename == NULL)
            goto finally;
        Py_INCREF(*filename);
    }
    else {
        *filename = v;
    }

    v = _PyObject_GetAttrId(err, &PyId_lineno);
    if (!v)
        goto finally;
    hold = PyLong_AsSsize_t(v);
    Py_DECREF(v);
    if (hold < 0 && PyErr_Occurred())
        goto finally;
    *lineno = hold;

    v = _PyObject_GetAttrId(err, &PyId_offset);
    if (!v)
        goto finally;
    if (v == Py_None) {
        *offset = -1;
        Py_DECREF(v);
    } else {
        hold = PyLong_AsSsize_t(v);
        Py_DECREF(v);
        if (hold < 0 && PyErr_Occurred())
            goto finally;
        *offset = hold;
    }

    if (Py_TYPE(err) == (PyTypeObject*)PyExc_SyntaxError) {
        v = _PyObject_GetAttrId(err, &PyId_end_lineno);
        if (!v) {
            PyErr_Clear();
            *end_lineno = *lineno;
        }
        else if (v == Py_None) {
            *end_lineno = *lineno;
            Py_DECREF(v);
        } else {
            hold = PyLong_AsSsize_t(v);
            Py_DECREF(v);
            if (hold < 0 && PyErr_Occurred())
                goto finally;
            *end_lineno = hold;
        }

        v = _PyObject_GetAttrId(err, &PyId_end_offset);
        if (!v) {
            PyErr_Clear();
            *end_offset = -1;
        }
        else if (v == Py_None) {
            *end_offset = -1;
            Py_DECREF(v);
        } else {
            hold = PyLong_AsSsize_t(v);
            Py_DECREF(v);
            if (hold < 0 && PyErr_Occurred())
                goto finally;
            *end_offset = hold;
        }
    } else {
        // SyntaxError subclasses
        *end_lineno = *lineno;
        *end_offset = -1;
    }

    v = _PyObject_GetAttrId(err, &PyId_text);
    if (!v)
        goto finally;
    if (v == Py_None) {
        Py_DECREF(v);
        *text = NULL;
    }
    else {
        *text = v;
    }
    return 1;

finally:
    Py_XDECREF(*message);
    Py_XDECREF(*filename);
    return 0;
}

static void
print_error_text(PyObject *f, Py_ssize_t offset, Py_ssize_t end_offset, PyObject *text_obj)
{
    size_t caret_repetitions = (end_offset > 0 && end_offset > offset) ? end_offset - offset : 1;
    /* Convert text to a char pointer; return if error */
    const char *text = PyUnicode_AsUTF8(text_obj);
    if (text == NULL)
        return;

    /* Convert offset from 1-based to 0-based */
    offset--;

    /* Strip leading whitespace from text, adjusting offset as we go */
    while (*text == ' ' || *text == '\t' || *text == '\f') {
        text++;
        offset--;
    }

    /* Calculate text length excluding trailing newline */
    Py_ssize_t len = strlen(text);
    if (len > 0 && text[len-1] == '\n') {
        len--;
    }

    /* Clip offset to at most len */
    if (offset > len) {
        offset = len;
    }

    /* Skip past newlines embedded in text */
    for (;;) {
        const char *nl = strchr(text, '\n');
        if (nl == NULL) {
            break;
        }
        Py_ssize_t inl = nl - text;
        if (inl >= offset) {
            break;
        }
        inl += 1;
        text += inl;
        len -= inl;
        offset -= (int)inl;
    }

    /* Print text */
    PyFile_WriteString("    ", f);
    PyFile_WriteString(text, f);

    /* Make sure there's a newline at the end */
    if (text[len] != '\n') {
        PyFile_WriteString("\n", f);
    }

    /* Don't print caret if it points to the left of the text */
    if (offset < 0)
        return;

    /* Write caret line */
    PyFile_WriteString("    ", f);
    while (--offset >= 0) {
        PyFile_WriteString(" ", f);
    }
    for (size_t caret_iter=0; caret_iter < caret_repetitions ; caret_iter++) {
        PyFile_WriteString("^", f);
    }
    PyFile_WriteString("\n", f);
}


int
_Py_HandleSystemExit(int *exitcode_p)
{
    int inspect = _Py_GetConfig()->inspect;
    if (inspect) {
        /* Don't exit if -i flag was given. This flag is set to 0
         * when entering interactive mode for inspecting. */
        return 0;
    }

    if (!PyErr_ExceptionMatches(PyExc_SystemExit)) {
        return 0;
    }

    PyObject *exception, *value, *tb;
    PyErr_Fetch(&exception, &value, &tb);

    fflush(stdout);

    int exitcode = 0;
    if (value == NULL || value == Py_None) {
        goto done;
    }

    if (PyExceptionInstance_Check(value)) {
        /* The error code should be in the `code' attribute. */
        _Py_IDENTIFIER(code);
        PyObject *code = _PyObject_GetAttrId(value, &PyId_code);
        if (code) {
            Py_DECREF(value);
            value = code;
            if (value == Py_None)
                goto done;
        }
        /* If we failed to dig out the 'code' attribute,
           just let the else clause below print the error. */
    }

    if (PyLong_Check(value)) {
        exitcode = (int)PyLong_AsLong(value);
    }
    else {
        PyObject *sys_stderr = _PySys_GetObjectId(&PyId_stderr);
        /* We clear the exception here to avoid triggering the assertion
         * in PyObject_Str that ensures it won't silently lose exception
         * details.
         */
        PyErr_Clear();
        if (sys_stderr != NULL && sys_stderr != Py_None) {
            PyFile_WriteObject(value, sys_stderr, Py_PRINT_RAW);
        } else {
            PyObject_Print(value, stderr, Py_PRINT_RAW);
            fflush(stderr);
        }
        PySys_WriteStderr("\n");
        exitcode = 1;
    }

 done:
    /* Restore and clear the exception info, in order to properly decref
     * the exception, value, and traceback.      If we just exit instead,
     * these leak, which confuses PYTHONDUMPREFS output, and may prevent
     * some finalizers from running.
     */
    PyErr_Restore(exception, value, tb);
    PyErr_Clear();
    *exitcode_p = exitcode;
    return 1;
}


static void
handle_system_exit(void)
{
    int exitcode;
    if (_Py_HandleSystemExit(&exitcode)) {
        Py_Exit(exitcode);
    }
}


static void
_PyErr_PrintEx(PyThreadState *tstate, int set_sys_last_vars)
{
    PyObject *exception, *v, *tb, *hook;

    handle_system_exit();

    _PyErr_Fetch(tstate, &exception, &v, &tb);
    if (exception == NULL) {
        goto done;
    }

    _PyErr_NormalizeException(tstate, &exception, &v, &tb);
    if (tb == NULL) {
        tb = Py_None;
        Py_INCREF(tb);
    }
    PyException_SetTraceback(v, tb);
    if (exception == NULL) {
        goto done;
    }

    /* Now we know v != NULL too */
    if (set_sys_last_vars) {
        if (_PySys_SetObjectId(&PyId_last_type, exception) < 0) {
            _PyErr_Clear(tstate);
        }
        if (_PySys_SetObjectId(&PyId_last_value, v) < 0) {
            _PyErr_Clear(tstate);
        }
        if (_PySys_SetObjectId(&PyId_last_traceback, tb) < 0) {
            _PyErr_Clear(tstate);
        }
    }
    hook = _PySys_GetObjectId(&PyId_excepthook);
    if (_PySys_Audit(tstate, "sys.excepthook", "OOOO", hook ? hook : Py_None,
                     exception, v, tb) < 0) {
        if (PyErr_ExceptionMatches(PyExc_RuntimeError)) {
            PyErr_Clear();
            goto done;
        }
        _PyErr_WriteUnraisableMsg("in audit hook", NULL);
    }
    if (hook) {
        PyObject* stack[3];
        PyObject *result;

        stack[0] = exception;
        stack[1] = v;
        stack[2] = tb;
        result = _PyObject_FastCall(hook, stack, 3);
        if (result == NULL) {
            handle_system_exit();

            PyObject *exception2, *v2, *tb2;
            _PyErr_Fetch(tstate, &exception2, &v2, &tb2);
            _PyErr_NormalizeException(tstate, &exception2, &v2, &tb2);
            /* It should not be possible for exception2 or v2
               to be NULL. However PyErr_Display() can't
               tolerate NULLs, so just be safe. */
            if (exception2 == NULL) {
                exception2 = Py_None;
                Py_INCREF(exception2);
            }
            if (v2 == NULL) {
                v2 = Py_None;
                Py_INCREF(v2);
            }
            fflush(stdout);
            PySys_WriteStderr("Error in sys.excepthook:\n");
            PyErr_Display(exception2, v2, tb2);
            PySys_WriteStderr("\nOriginal exception was:\n");
            PyErr_Display(exception, v, tb);
            Py_DECREF(exception2);
            Py_DECREF(v2);
            Py_XDECREF(tb2);
        }
        Py_XDECREF(result);
    }
    else {
        PySys_WriteStderr("sys.excepthook is missing\n");
        PyErr_Display(exception, v, tb);
    }

done:
    Py_XDECREF(exception);
    Py_XDECREF(v);
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

static void
print_exception(PyObject *f, PyObject *value)
{
    int err = 0;
    PyObject *type, *tb, *tmp;
    _Py_IDENTIFIER(print_file_and_line);

    if (!PyExceptionInstance_Check(value)) {
        err = PyFile_WriteString("TypeError: print_exception(): Exception expected for value, ", f);
        err += PyFile_WriteString(Py_TYPE(value)->tp_name, f);
        err += PyFile_WriteString(" found\n", f);
        if (err)
            PyErr_Clear();
        return;
    }

    Py_INCREF(value);
    fflush(stdout);
    type = (PyObject *) Py_TYPE(value);
    tb = PyException_GetTraceback(value);
    if (tb && tb != Py_None)
        err = PyTraceBack_Print(tb, f);
    if (err == 0 &&
        (err = _PyObject_LookupAttrId(value, &PyId_print_file_and_line, &tmp)) > 0)
    {
        PyObject *message, *filename, *text;
        Py_ssize_t lineno, offset, end_lineno, end_offset;
        err = 0;
        Py_DECREF(tmp);
        if (!parse_syntax_error(value, &message, &filename,
                                &lineno, &offset,
                                &end_lineno, &end_offset, &text))
            PyErr_Clear();
        else {
            PyObject *line;

            Py_DECREF(value);
            value = message;

            line = PyUnicode_FromFormat("  File \"%S\", line %zd\n",
                                          filename, lineno);
            Py_DECREF(filename);
            if (line != NULL) {
                PyFile_WriteObject(line, f, Py_PRINT_RAW);
                Py_DECREF(line);
            }

            if (text != NULL) {
                Py_ssize_t line_size;
                const char* error_line = PyUnicode_AsUTF8AndSize(text, &line_size);
                // If the location of the error spawn multiple lines, we want
                // to just print the first one and highlight everything until
                // the end of that one since we don't support multi-line error
                // messages.
                if (end_lineno > lineno) {
                    end_offset = (error_line != NULL) ? line_size : -1;
                }
                // Limit the amount of '^' that we can display to
                // the size of the text in the source line.
                if (error_line != NULL && end_offset > line_size + 1) {
                    end_offset = line_size + 1;
                }
                print_error_text(f, offset, end_offset, text);
                Py_DECREF(text);
            }

            /* Can't be bothered to check all those
               PyFile_WriteString() calls */
            if (PyErr_Occurred())
                err = -1;
        }
    }
    if (err) {
        /* Don't do anything else */
    }
    else {
        PyObject* modulename;

        _Py_IDENTIFIER(__module__);
        assert(PyExceptionClass_Check(type));

        modulename = _PyObject_GetAttrId(type, &PyId___module__);
        if (modulename == NULL || !PyUnicode_Check(modulename))
        {
            Py_XDECREF(modulename);
            PyErr_Clear();
            err = PyFile_WriteString("<unknown>.", f);
        }
        else {
            if (!_PyUnicode_EqualToASCIIId(modulename, &PyId_builtins))
            {
                err = PyFile_WriteObject(modulename, f, Py_PRINT_RAW);
                err += PyFile_WriteString(".", f);
            }
            Py_DECREF(modulename);
        }
        if (err == 0) {
            PyObject* qualname = _PyType_GetQualName((PyTypeObject *)type);
            if (qualname == NULL || !PyUnicode_Check(qualname)) {
                Py_XDECREF(qualname);
                PyErr_Clear();
                err = PyFile_WriteString("<unknown>", f);
            }
            else {
                err = PyFile_WriteObject(qualname, f, Py_PRINT_RAW);
                Py_DECREF(qualname);
            }
        }
    }
    if (err == 0 && (value != Py_None)) {
        PyObject *s = PyObject_Str(value);
        /* only print colon if the str() of the
           object is not the empty string
        */
        if (s == NULL) {
            PyErr_Clear();
            err = -1;
            PyFile_WriteString(": <exception str() failed>", f);
        }
        else if (!PyUnicode_Check(s) ||
            PyUnicode_GetLength(s) != 0)
            err = PyFile_WriteString(": ", f);
        if (err == 0)
          err = PyFile_WriteObject(s, f, Py_PRINT_RAW);
        Py_XDECREF(s);
    }
    /* try to write a newline in any case */
    if (err < 0) {
        PyErr_Clear();
    }
    PyObject* suggestions = _Py_Offer_Suggestions(value);
    if (suggestions) {
        // Add a trailer ". Did you mean: (...)?"
        err = PyFile_WriteString(". Did you mean: '", f);
        if (err == 0) {
            err = PyFile_WriteObject(suggestions, f, Py_PRINT_RAW);
            err += PyFile_WriteString("'?", f);
        }
        Py_DECREF(suggestions);
    } else if (PyErr_Occurred()) {
        PyErr_Clear();
    }
    err += PyFile_WriteString("\n", f);
    Py_XDECREF(tb);
    Py_DECREF(value);
    /* If an error happened here, don't show it.
       XXX This is wrong, but too many callers rely on this behavior. */
    if (err != 0)
        PyErr_Clear();
}

static const char cause_message[] =
    "\nThe above exception was the direct cause "
    "of the following exception:\n\n";

static const char context_message[] =
    "\nDuring handling of the above exception, "
    "another exception occurred:\n\n";

static void
print_exception_recursive(PyObject *f, PyObject *value, PyObject *seen)
{
    int err = 0, res;
    PyObject *cause, *context;

    if (seen != NULL) {
        /* Exception chaining */
        PyObject *value_id = PyLong_FromVoidPtr(value);
        if (value_id == NULL || PySet_Add(seen, value_id) == -1)
            PyErr_Clear();
        else if (PyExceptionInstance_Check(value)) {
            PyObject *check_id = NULL;
            cause = PyException_GetCause(value);
            context = PyException_GetContext(value);
            if (cause) {
                check_id = PyLong_FromVoidPtr(cause);
                if (check_id == NULL) {
                    res = -1;
                } else {
                    res = PySet_Contains(seen, check_id);
                    Py_DECREF(check_id);
                }
                if (res == -1)
                    PyErr_Clear();
                if (res == 0) {
                    print_exception_recursive(
                        f, cause, seen);
                    err |= PyFile_WriteString(
                        cause_message, f);
                }
            }
            else if (context &&
                !((PyBaseExceptionObject *)value)->suppress_context) {
                check_id = PyLong_FromVoidPtr(context);
                if (check_id == NULL) {
                    res = -1;
                } else {
                    res = PySet_Contains(seen, check_id);
                    Py_DECREF(check_id);
                }
                if (res == -1)
                    PyErr_Clear();
                if (res == 0) {
                    print_exception_recursive(
                        f, context, seen);
                    err |= PyFile_WriteString(
                        context_message, f);
                }
            }
            Py_XDECREF(context);
            Py_XDECREF(cause);
        }
        Py_XDECREF(value_id);
    }
    print_exception(f, value);
    if (err != 0)
        PyErr_Clear();
}

void
_PyErr_Display(PyObject *file, PyObject *exception, PyObject *value, PyObject *tb)
{
    assert(file != NULL && file != Py_None);

    PyObject *seen;
    if (PyExceptionInstance_Check(value)
        && tb != NULL && PyTraceBack_Check(tb)) {
        /* Put the traceback on the exception, otherwise it won't get
           displayed.  See issue #18776. */
        PyObject *cur_tb = PyException_GetTraceback(value);
        if (cur_tb == NULL)
            PyException_SetTraceback(value, tb);
        else
            Py_DECREF(cur_tb);
    }

    /* We choose to ignore seen being possibly NULL, and report
       at least the main exception (it could be a MemoryError).
    */
    seen = PySet_New(NULL);
    if (seen == NULL) {
        PyErr_Clear();
    }
    print_exception_recursive(file, value, seen);
    Py_XDECREF(seen);

    /* Call file.flush() */
    PyObject *res = _PyObject_CallMethodIdNoArgs(file, &PyId_flush);
    if (!res) {
        /* Silently ignore file.flush() error */
        PyErr_Clear();
    }
    else {
        Py_DECREF(res);
    }
}

void
PyErr_Display(PyObject *exception, PyObject *value, PyObject *tb)
{
    PyObject *file = _PySys_GetObjectId(&PyId_stderr);
    if (file == NULL) {
        _PyObject_Dump(value);
        fprintf(stderr, "lost sys.stderr\n");
        return;
    }
    if (file == Py_None) {
        return;
    }
    Py_INCREF(file);
    _PyErr_Display(file, exception, value, tb);
    Py_DECREF(file);
}

PyObject *
PyRun_StringFlags(const char *str, int start, PyObject *globals,
                  PyObject *locals, PyCompilerFlags *flags)
{
    PyObject *ret = NULL;
    mod_ty mod;
    PyArena *arena;
    PyObject *filename;

    filename = _PyUnicode_FromId(&PyId_string); /* borrowed */
    if (filename == NULL)
        return NULL;

    arena = _PyArena_New();
    if (arena == NULL)
        return NULL;

    mod = _PyParser_ASTFromString(str, filename, start, flags, arena);

    if (mod != NULL)
        ret = run_mod(mod, filename, globals, locals, flags, arena);
    _PyArena_Free(arena);
    return ret;
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
        ret = run_mod(mod, filename, globals, locals, flags, arena);
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
flush_io(void)
{
    PyObject *f, *r;
    PyObject *type, *value, *traceback;

    /* Save the current exception */
    PyErr_Fetch(&type, &value, &traceback);

    f = _PySys_GetObjectId(&PyId_stderr);
    if (f != NULL) {
        r = _PyObject_CallMethodIdNoArgs(f, &PyId_flush);
        if (r)
            Py_DECREF(r);
        else
            PyErr_Clear();
    }
    f = _PySys_GetObjectId(&PyId_stdout);
    if (f != NULL) {
        r = _PyObject_CallMethodIdNoArgs(f, &PyId_flush);
        if (r)
            Py_DECREF(r);
        else
            PyErr_Clear();
    }

    PyErr_Restore(type, value, traceback);
}

static PyObject *
run_eval_code_obj(PyThreadState *tstate, PyCodeObject *co, PyObject *globals, PyObject *locals)
{
    PyObject *v;
    /*
     * We explicitly re-initialize _Py_UnhandledKeyboardInterrupt every eval
     * _just in case_ someone is calling into an embedded Python where they
     * don't care about an uncaught KeyboardInterrupt exception (why didn't they
     * leave config.install_signal_handlers set to 0?!?) but then later call
     * Py_Main() itself (which _checks_ this flag and dies with a signal after
     * its interpreter exits).  We don't want a previous embedded interpreter's
     * uncaught exception to trigger an unexplained signal exit from a future
     * Py_Main() based one.
     */
    _Py_UnhandledKeyboardInterrupt = 0;

    /* Set globals['__builtins__'] if it doesn't exist */
    if (globals != NULL && _PyDict_GetItemStringWithError(globals, "__builtins__") == NULL) {
        if (PyErr_Occurred() ||
            PyDict_SetItemString(globals, "__builtins__",
                                 tstate->interp->builtins) < 0)
        {
            return NULL;
        }
    }

    v = PyEval_EvalCode((PyObject*)co, globals, locals);
    if (!v && _PyErr_Occurred(tstate) == PyExc_KeyboardInterrupt) {
        _Py_UnhandledKeyboardInterrupt = 1;
    }
    return v;
}

static PyObject *
run_mod(mod_ty mod, PyObject *filename, PyObject *globals, PyObject *locals,
            PyCompilerFlags *flags, PyArena *arena)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyCodeObject *co = _PyAST_Compile(mod, filename, flags, -1, arena);
    if (co == NULL)
        return NULL;

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
        PyErr_SetString(PyExc_ValueError,
            "source code string cannot contain null bytes");
        Py_CLEAR(*cmd_copy);
        return NULL;
    }
    return str;
}

#if defined(USE_STACKCHECK)
#if defined(WIN32) && defined(_MSC_VER)

/* Stack checking for Microsoft C */

#include <malloc.h>
#include <excpt.h>

/*
 * Return non-zero when we run out of memory on the stack; zero otherwise.
 */
int
PyOS_CheckStack(void)
{
    __try {
        /* alloca throws a stack overflow exception if there's
           not enough space left on the stack */
        alloca(PYOS_STACK_MARGIN * sizeof(void*));
        return 0;
    } __except (GetExceptionCode() == STATUS_STACK_OVERFLOW ?
                    EXCEPTION_EXECUTE_HANDLER :
            EXCEPTION_CONTINUE_SEARCH) {
        int errcode = _resetstkoflw();
        if (errcode == 0)
        {
            Py_FatalError("Could not reset the stack!");
        }
    }
    return 1;
}

#endif /* WIN32 && _MSC_VER */

/* Alternate implementations can be added here... */

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

#ifdef __cplusplus
}
#endif
