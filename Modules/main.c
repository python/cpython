/* Python interpreter main program */

#include "Python.h"
#include "pycore_coreconfig.h"
#include "pycore_pylifecycle.h"
#include "pycore_pymem.h"
#include "pycore_pystate.h"

#ifdef __FreeBSD__
#  include <fenv.h>     /* fedisableexcept() */
#endif

/* Includes for exit_sigint() */
#include <stdio.h>      /* perror() */
#ifdef HAVE_SIGNAL_H
#  include <signal.h>   /* SIGINT */
#endif
#if defined(HAVE_GETPID) && defined(HAVE_UNISTD_H)
#  include <unistd.h>   /* getpid() */
#endif
#ifdef _MSC_VER
#  include <crtdbg.h>   /* STATUS_CONTROL_C_EXIT */
#endif
/* End of includes for exit_sigint() */

#define COPYRIGHT \
    "Type \"help\", \"copyright\", \"credits\" or \"license\" " \
    "for more information."

#ifdef __cplusplus
extern "C" {
#endif

/* --- pymain_init() ---------------------------------------------- */

static _PyInitError
pymain_init(const _PyArgv *args)
{
    _PyInitError err;

    err = _PyRuntime_Initialize();
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    /* 754 requires that FP exceptions run in "no stop" mode by default,
     * and until C vendors implement C99's ways to control FP exceptions,
     * Python requires non-stop mode.  Alas, some platforms enable FP
     * exceptions by default.  Here we disable them.
     */
#ifdef __FreeBSD__
    fedisableexcept(FE_OVERFLOW);
#endif

    _PyPreConfig preconfig = _PyPreConfig_INIT;
    /* Set to -1 to enable them depending on the LC_CTYPE locale and the
       environment variables (PYTHONUTF8 and PYTHONCOERCECLOCALE)  */
    preconfig.coerce_c_locale = -1;
    preconfig.utf8_mode = -1;
    err = _Py_PreInitializeFromPyArgv(&preconfig, args);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    /* pass NULL as the config: config is read from command line arguments,
       environment variables, configuration files */
    if (args->use_bytes_argv) {
        return _Py_InitializeFromArgs(NULL, args->argc, args->bytes_argv);
    }
    else {
        return _Py_InitializeFromWideArgs(NULL, args->argc, args->wchar_argv);
    }
}


/* --- pymain_run_python() ---------------------------------------- */

/* Non-zero if filename, command (-c) or module (-m) is set
   on the command line */
#define RUN_CODE(config) \
    (config->run_command != NULL || config->run_filename != NULL \
     || config->run_module != NULL)

/* Return non-zero is stdin is a TTY or if -i command line option is used */
static int
stdin_is_interactive(const _PyCoreConfig *config)
{
    return (isatty(fileno(stdin)) || config->interactive);
}


static PyObject *
pymain_get_importer(const wchar_t *filename)
{
    PyObject *sys_path0 = NULL, *importer;

    sys_path0 = PyUnicode_FromWideChar(filename, wcslen(filename));
    if (sys_path0 == NULL) {
        goto error;
    }

    importer = PyImport_GetImporter(sys_path0);
    if (importer == NULL) {
        goto error;
    }

    if (importer == Py_None) {
        Py_DECREF(sys_path0);
        Py_DECREF(importer);
        return NULL;
    }

    Py_DECREF(importer);
    return sys_path0;

error:
    Py_XDECREF(sys_path0);
    PySys_WriteStderr("Failed checking if argv[0] is an import path entry\n");
    PyErr_Print();
    return NULL;
}


static int
pymain_sys_path_add_path0(PyInterpreterState *interp, PyObject *path0)
{
    _Py_IDENTIFIER(path);
    PyObject *sys_path;
    PyObject *sysdict = interp->sysdict;
    if (sysdict != NULL) {
        sys_path = _PyDict_GetItemIdWithError(sysdict, &PyId_path);
        if (sys_path == NULL && PyErr_Occurred()) {
            goto error;
        }
    }
    else {
        sys_path = NULL;
    }
    if (sys_path == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "unable to get sys.path");
        goto error;
    }

    if (PyList_Insert(sys_path, 0, path0)) {
        goto error;
    }
    return 0;

error:
    PyErr_Print();
    return -1;
}


static void
pymain_header(const _PyCoreConfig *config)
{
    if (config->quiet) {
        return;
    }

    if (!config->verbose && (RUN_CODE(config) || !stdin_is_interactive(config))) {
        return;
    }

    fprintf(stderr, "Python %s on %s\n", Py_GetVersion(), Py_GetPlatform());
    if (config->site_import) {
        fprintf(stderr, "%s\n", COPYRIGHT);
    }
}


static void
pymain_import_readline(const _PyCoreConfig *config)
{
    if (config->isolated) {
        return;
    }
    if (!config->inspect && RUN_CODE(config)) {
        return;
    }
    if (!isatty(fileno(stdin))) {
        return;
    }

    PyObject *mod = PyImport_ImportModule("readline");
    if (mod == NULL) {
        PyErr_Clear();
    }
    else {
        Py_DECREF(mod);
    }
}


static int
pymain_run_command(wchar_t *command, PyCompilerFlags *cf)
{
    PyObject *unicode, *bytes;
    int ret;

    unicode = PyUnicode_FromWideChar(command, -1);
    if (unicode == NULL) {
        goto error;
    }

    bytes = PyUnicode_AsUTF8String(unicode);
    Py_DECREF(unicode);
    if (bytes == NULL) {
        goto error;
    }

    ret = PyRun_SimpleStringFlags(PyBytes_AsString(bytes), cf);
    Py_DECREF(bytes);
    return (ret != 0);

error:
    PySys_WriteStderr("Unable to decode the command from the command line:\n");
    PyErr_Print();
    return 1;
}


static int
pymain_run_module(const wchar_t *modname, int set_argv0)
{
    PyObject *module, *runpy, *runmodule, *runargs, *result;
    runpy = PyImport_ImportModule("runpy");
    if (runpy == NULL) {
        fprintf(stderr, "Could not import runpy module\n");
        PyErr_Print();
        return -1;
    }
    runmodule = PyObject_GetAttrString(runpy, "_run_module_as_main");
    if (runmodule == NULL) {
        fprintf(stderr, "Could not access runpy._run_module_as_main\n");
        PyErr_Print();
        Py_DECREF(runpy);
        return -1;
    }
    module = PyUnicode_FromWideChar(modname, wcslen(modname));
    if (module == NULL) {
        fprintf(stderr, "Could not convert module name to unicode\n");
        PyErr_Print();
        Py_DECREF(runpy);
        Py_DECREF(runmodule);
        return -1;
    }
    runargs = Py_BuildValue("(Oi)", module, set_argv0);
    if (runargs == NULL) {
        fprintf(stderr,
            "Could not create arguments for runpy._run_module_as_main\n");
        PyErr_Print();
        Py_DECREF(runpy);
        Py_DECREF(runmodule);
        Py_DECREF(module);
        return -1;
    }
    result = PyObject_Call(runmodule, runargs, NULL);
    if (result == NULL) {
        PyErr_Print();
    }
    Py_DECREF(runpy);
    Py_DECREF(runmodule);
    Py_DECREF(module);
    Py_DECREF(runargs);
    if (result == NULL) {
        return -1;
    }
    Py_DECREF(result);
    return 0;
}


static int
pymain_run_file(_PyCoreConfig *config, PyCompilerFlags *cf)
{
    const wchar_t *filename = config->run_filename;
    FILE *fp = _Py_wfopen(filename, L"rb");
    if (fp == NULL) {
        char *cfilename_buffer;
        const char *cfilename;
        int err = errno;
        cfilename_buffer = _Py_EncodeLocaleRaw(filename, NULL);
        if (cfilename_buffer != NULL)
            cfilename = cfilename_buffer;
        else
            cfilename = "<unprintable file name>";
        fprintf(stderr, "%ls: can't open file '%s': [Errno %d] %s\n",
                config->program, cfilename, err, strerror(err));
        PyMem_RawFree(cfilename_buffer);
        return 2;
    }

    if (config->skip_source_first_line) {
        int ch;
        /* Push back first newline so line numbers remain the same */
        while ((ch = getc(fp)) != EOF) {
            if (ch == '\n') {
                (void)ungetc(ch, fp);
                break;
            }
        }
    }

    struct _Py_stat_struct sb;
    if (_Py_fstat_noraise(fileno(fp), &sb) == 0 && S_ISDIR(sb.st_mode)) {
        fprintf(stderr,
                "%ls: '%ls' is a directory, cannot continue\n",
                config->program, filename);
        fclose(fp);
        return 1;
    }

    /* call pending calls like signal handlers (SIGINT) */
    if (Py_MakePendingCalls() == -1) {
        PyErr_Print();
        fclose(fp);
        return 1;
    }

    PyObject *unicode, *bytes = NULL;
    const char *filename_str;

    unicode = PyUnicode_FromWideChar(filename, wcslen(filename));
    if (unicode != NULL) {
        bytes = PyUnicode_EncodeFSDefault(unicode);
        Py_DECREF(unicode);
    }
    if (bytes != NULL) {
        filename_str = PyBytes_AsString(bytes);
    }
    else {
        PyErr_Clear();
        filename_str = "<filename encoding error>";
    }

    /* PyRun_AnyFileExFlags(closeit=1) calls fclose(fp) before running code */
    int run = PyRun_AnyFileExFlags(fp, filename_str, 1, cf);
    Py_XDECREF(bytes);
    return (run != 0);
}


static void
pymain_run_startup(_PyCoreConfig *config, PyCompilerFlags *cf)
{
    const char *startup = _Py_GetEnv(config->use_environment, "PYTHONSTARTUP");
    if (startup == NULL) {
        return;
    }

    FILE *fp = _Py_fopen(startup, "r");
    if (fp == NULL) {
        int save_errno = errno;
        PySys_WriteStderr("Could not open PYTHONSTARTUP\n");
        errno = save_errno;

        PyErr_SetFromErrnoWithFilename(PyExc_OSError,
                        startup);
        PyErr_Print();
        return;
    }

    (void) PyRun_SimpleFileExFlags(fp, startup, 0, cf);
    PyErr_Clear();
    fclose(fp);
}


static void
pymain_run_interactive_hook(void)
{
    PyObject *sys, *hook, *result;
    sys = PyImport_ImportModule("sys");
    if (sys == NULL) {
        goto error;
    }

    hook = PyObject_GetAttrString(sys, "__interactivehook__");
    Py_DECREF(sys);
    if (hook == NULL) {
        PyErr_Clear();
        return;
    }

    result = _PyObject_CallNoArg(hook);
    Py_DECREF(hook);
    if (result == NULL) {
        goto error;
    }
    Py_DECREF(result);

    return;

error:
    PySys_WriteStderr("Failed calling sys.__interactivehook__\n");
    PyErr_Print();
}


static int
pymain_run_stdin(_PyCoreConfig *config, PyCompilerFlags *cf)
{
    if (stdin_is_interactive(config)) {
        Py_InspectFlag = 0; /* do exit on SystemExit */
        config->inspect = 0;
        pymain_run_startup(config, cf);
        pymain_run_interactive_hook();
    }

    /* call pending calls like signal handlers (SIGINT) */
    if (Py_MakePendingCalls() == -1) {
        PyErr_Print();
        return 1;
    }

    int run = PyRun_AnyFileExFlags(stdin, "<stdin>", 0, cf);
    return (run != 0);
}


static void
pymain_repl(_PyCoreConfig *config, PyCompilerFlags *cf, int *exitcode)
{
    /* Check this environment variable at the end, to give programs the
       opportunity to set it from Python. */
    if (!Py_InspectFlag && _Py_GetEnv(config->use_environment, "PYTHONINSPECT")) {
        Py_InspectFlag = 1;
        config->inspect = 1;
    }

    if (!(Py_InspectFlag && stdin_is_interactive(config) && RUN_CODE(config))) {
        return;
    }

    Py_InspectFlag = 0;
    config->inspect = 0;
    pymain_run_interactive_hook();

    int res = PyRun_AnyFileFlags(stdin, "<stdin>", cf);
    *exitcode = (res != 0);
}


static _PyInitError
pymain_run_python(int *exitcode)
{
    _PyInitError err;

    PyInterpreterState *interp = _PyInterpreterState_GET_UNSAFE();
    /* pymain_run_stdin() modify the config */
    _PyCoreConfig *config = &interp->core_config;

    PyObject *main_importer_path = NULL;
    if (config->run_filename != NULL) {
        /* If filename is a package (ex: directory or ZIP file) which contains
           __main__.py, main_importer_path is set to filename and will be
           prepended to sys.path.

           Otherwise, main_importer_path is set to NULL. */
        main_importer_path = pymain_get_importer(config->run_filename);
    }

    if (main_importer_path != NULL) {
        if (pymain_sys_path_add_path0(interp, main_importer_path) < 0) {
            err = _Py_INIT_EXIT(1);
            goto done;
        }
    }
    else if (!config->isolated) {
        PyObject *path0 = NULL;
        if (_PyPathConfig_ComputeSysPath0(&config->argv, &path0)) {
            if (path0 == NULL) {
                err = _Py_INIT_NO_MEMORY();
                goto done;
            }

            if (pymain_sys_path_add_path0(interp, path0) < 0) {
                Py_DECREF(path0);
                err = _Py_INIT_EXIT(1);
                goto done;
            }
            Py_DECREF(path0);
        }
    }

    PyCompilerFlags cf = {.cf_flags = 0, .cf_feature_version = PY_MINOR_VERSION};

    pymain_header(config);
    pymain_import_readline(config);

    if (config->run_command) {
        *exitcode = pymain_run_command(config->run_command, &cf);
    }
    else if (config->run_module) {
        *exitcode = (pymain_run_module(config->run_module, 1) != 0);
    }
    else if (main_importer_path != NULL) {
        int sts = pymain_run_module(L"__main__", 0);
        *exitcode = (sts != 0);
    }
    else if (config->run_filename != NULL) {
        *exitcode = pymain_run_file(config, &cf);
    }
    else {
        *exitcode = pymain_run_stdin(config, &cf);
    }

    pymain_repl(config, &cf, exitcode);
    err = _Py_INIT_OK();

done:
    Py_XDECREF(main_importer_path);
    return err;
}


/* --- pymain_main() ---------------------------------------------- */

static void
pymain_free(void)
{
    _PyImport_Fini2();

    /* Free global variables which cannot be freed in Py_Finalize():
       configuration options set before Py_Initialize() which should
       remain valid after Py_Finalize(), since
       Py_Initialize()-Py_Finalize() can be called multiple times. */
    _PyPathConfig_ClearGlobal();
    _Py_ClearStandardStreamEncoding();
    _Py_ClearArgcArgv();
    _PyRuntime_Finalize();
}


static int
exit_sigint(void)
{
    /* bpo-1054041: We need to exit via the
     * SIG_DFL handler for SIGINT if KeyboardInterrupt went unhandled.
     * If we don't, a calling process such as a shell may not know
     * about the user's ^C.  https://www.cons.org/cracauer/sigint.html */
#if defined(HAVE_GETPID) && !defined(MS_WINDOWS)
    if (PyOS_setsig(SIGINT, SIG_DFL) == SIG_ERR) {
        perror("signal");  /* Impossible in normal environments. */
    } else {
        kill(getpid(), SIGINT);
    }
    /* If setting SIG_DFL failed, or kill failed to terminate us,
     * there isn't much else we can do aside from an error code. */
#endif  /* HAVE_GETPID && !MS_WINDOWS */
#ifdef MS_WINDOWS
    /* cmd.exe detects this, prints ^C, and offers to terminate. */
    /* https://msdn.microsoft.com/en-us/library/cc704588.aspx */
    return STATUS_CONTROL_C_EXIT;
#else
    return SIGINT + 128;
#endif  /* !MS_WINDOWS */
}


static void _Py_NO_RETURN
pymain_exit_error(_PyInitError err)
{
    if (_Py_INIT_IS_EXIT(err)) {
        /* If it's an error rather than a regular exit, leave Python runtime
           alive: _Py_ExitInitError() uses the current exception and use
           sys.stdout in this case. */
        pymain_free();
    }
    _Py_ExitInitError(err);
}


int
_Py_RunMain(void)
{
    int exitcode = 0;

    _PyInitError err = pymain_run_python(&exitcode);
    if (_Py_INIT_FAILED(err)) {
        pymain_exit_error(err);
    }

    if (Py_FinalizeEx() < 0) {
        /* Value unlikely to be confused with a non-error exit status or
           other special meaning */
        exitcode = 120;
    }

    pymain_free();

    if (_Py_UnhandledKeyboardInterrupt) {
        exitcode = exit_sigint();
    }

    return exitcode;
}


static int
pymain_main(_PyArgv *args)
{
    _PyInitError err = pymain_init(args);
    if (_Py_INIT_FAILED(err)) {
        pymain_exit_error(err);
    }

    return _Py_RunMain();
}


int
Py_Main(int argc, wchar_t **argv)
{
    _PyArgv args = {
        .argc = argc,
        .use_bytes_argv = 0,
        .bytes_argv = NULL,
        .wchar_argv = argv};
    return pymain_main(&args);
}


int
_Py_UnixMain(int argc, char **argv)
{
    _PyArgv args = {
        .argc = argc,
        .use_bytes_argv = 1,
        .bytes_argv = argv,
        .wchar_argv = NULL};
    return pymain_main(&args);
}

#ifdef __cplusplus
}
#endif
