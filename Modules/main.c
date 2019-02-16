/* Python interpreter main program */

#include "Python.h"
#include "osdefs.h"
#include "pycore_getopt.h"
#include "pycore_pathconfig.h"
#include "pycore_pylifecycle.h"
#include "pycore_pymem.h"
#include "pycore_pystate.h"

#include <locale.h>
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#include <stdio.h>
#if defined(HAVE_GETPID) && defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(MS_WINDOWS) || defined(__CYGWIN__)
#  include <windows.h>
#  ifdef HAVE_IO_H
#    include <io.h>
#  endif
#  ifdef HAVE_FCNTL_H
#    include <fcntl.h>
#  endif
#endif

#ifdef _MSC_VER
#  include <crtdbg.h>
#endif

#ifdef __FreeBSD__
#  include <fenv.h>
#endif

#if defined(MS_WINDOWS)
#  define PYTHONHOMEHELP "<prefix>\\python{major}{minor}"
#else
#  define PYTHONHOMEHELP "<prefix>/lib/pythonX.X"
#endif

#define COPYRIGHT \
    "Type \"help\", \"copyright\", \"credits\" or \"license\" " \
    "for more information."

#ifdef __cplusplus
extern "C" {
#endif

#define DECODE_LOCALE_ERR(NAME, LEN) \
    (((LEN) == -2) \
     ? _Py_INIT_USER_ERR("cannot decode " NAME) \
     : _Py_INIT_NO_MEMORY())


#ifdef MS_WINDOWS
#define WCSTOK wcstok_s
#else
#define WCSTOK wcstok
#endif

/* For Py_GetArgcArgv(); set by main() */
static wchar_t **orig_argv = NULL;
static int orig_argc = 0;

/* command line options */
#define BASE_OPTS L"bBc:dEhiIJm:OqRsStuvVW:xX:?"

#define PROGRAM_OPTS BASE_OPTS

static const _PyOS_LongOption longoptions[] = {
    {L"check-hash-based-pycs", 1, 0},
    {NULL, 0, 0},
};

/* Short usage message (with %s for argv0) */
static const char usage_line[] =
"usage: %ls [option] ... [-c cmd | -m mod | file | -] [arg] ...\n";

/* Long usage message, split into parts < 512 bytes */
static const char usage_1[] = "\
Options and arguments (and corresponding environment variables):\n\
-b     : issue warnings about str(bytes_instance), str(bytearray_instance)\n\
         and comparing bytes/bytearray with str. (-bb: issue errors)\n\
-B     : don't write .pyc files on import; also PYTHONDONTWRITEBYTECODE=x\n\
-c cmd : program passed in as string (terminates option list)\n\
-d     : debug output from parser; also PYTHONDEBUG=x\n\
-E     : ignore PYTHON* environment variables (such as PYTHONPATH)\n\
-h     : print this help message and exit (also --help)\n\
";
static const char usage_2[] = "\
-i     : inspect interactively after running script; forces a prompt even\n\
         if stdin does not appear to be a terminal; also PYTHONINSPECT=x\n\
-I     : isolate Python from the user's environment (implies -E and -s)\n\
-m mod : run library module as a script (terminates option list)\n\
-O     : remove assert and __debug__-dependent statements; add .opt-1 before\n\
         .pyc extension; also PYTHONOPTIMIZE=x\n\
-OO    : do -O changes and also discard docstrings; add .opt-2 before\n\
         .pyc extension\n\
-q     : don't print version and copyright messages on interactive startup\n\
-s     : don't add user site directory to sys.path; also PYTHONNOUSERSITE\n\
-S     : don't imply 'import site' on initialization\n\
";
static const char usage_3[] = "\
-u     : force the stdout and stderr streams to be unbuffered;\n\
         this option has no effect on stdin; also PYTHONUNBUFFERED=x\n\
-v     : verbose (trace import statements); also PYTHONVERBOSE=x\n\
         can be supplied multiple times to increase verbosity\n\
-V     : print the Python version number and exit (also --version)\n\
         when given twice, print more information about the build\n\
-W arg : warning control; arg is action:message:category:module:lineno\n\
         also PYTHONWARNINGS=arg\n\
-x     : skip first line of source, allowing use of non-Unix forms of #!cmd\n\
-X opt : set implementation-specific option\n\
--check-hash-based-pycs always|default|never:\n\
    control how Python invalidates hash-based .pyc files\n\
";
static const char usage_4[] = "\
file   : program read from script file\n\
-      : program read from stdin (default; interactive mode if a tty)\n\
arg ...: arguments passed to program in sys.argv[1:]\n\n\
Other environment variables:\n\
PYTHONSTARTUP: file executed on interactive startup (no default)\n\
PYTHONPATH   : '%lc'-separated list of directories prefixed to the\n\
               default module search path.  The result is sys.path.\n\
";
static const char usage_5[] =
"PYTHONHOME   : alternate <prefix> directory (or <prefix>%lc<exec_prefix>).\n"
"               The default module search path uses %s.\n"
"PYTHONCASEOK : ignore case in 'import' statements (Windows).\n"
"PYTHONIOENCODING: Encoding[:errors] used for stdin/stdout/stderr.\n"
"PYTHONFAULTHANDLER: dump the Python traceback on fatal errors.\n";
static const char usage_6[] =
"PYTHONHASHSEED: if this variable is set to 'random', a random value is used\n"
"   to seed the hashes of str, bytes and datetime objects.  It can also be\n"
"   set to an integer in the range [0,4294967295] to get hash values with a\n"
"   predictable seed.\n"
"PYTHONMALLOC: set the Python memory allocators and/or install debug hooks\n"
"   on Python memory allocators. Use PYTHONMALLOC=debug to install debug\n"
"   hooks.\n"
"PYTHONCOERCECLOCALE: if this variable is set to 0, it disables the locale\n"
"   coercion behavior. Use PYTHONCOERCECLOCALE=warn to request display of\n"
"   locale coercion and locale compatibility warnings on stderr.\n"
"PYTHONBREAKPOINT: if this variable is set to 0, it disables the default\n"
"   debugger. It can be set to the callable of your debugger of choice.\n"
"PYTHONDEVMODE: enable the development mode.\n"
"PYTHONPYCACHEPREFIX: root directory for bytecode cache (pyc) files.\n";

static void
pymain_usage(int error, const wchar_t* program)
{
    FILE *f = error ? stderr : stdout;

    fprintf(f, usage_line, program);
    if (error)
        fprintf(f, "Try `python -h' for more information.\n");
    else {
        fputs(usage_1, f);
        fputs(usage_2, f);
        fputs(usage_3, f);
        fprintf(f, usage_4, (wint_t)DELIM);
        fprintf(f, usage_5, (wint_t)DELIM, PYTHONHOMEHELP);
        fputs(usage_6, f);
    }
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


/* Main program */

typedef struct {
    wchar_t **argv;
    int nwarnoption;             /* Number of -W command line options */
    wchar_t **warnoptions;       /* Command line -W options */
    int nenv_warnoption;         /* Number of PYTHONWARNINGS environment variables */
    wchar_t **env_warnoptions;   /* PYTHONWARNINGS environment variables */
    int print_help;              /* -h, -? options */
    int print_version;           /* -V option */
} _PyCmdline;

/* Structure used by Py_Main() to pass data to subfunctions */
typedef struct {
    /* Input arguments */
    int argc;
    int use_bytes_argv;
    char **bytes_argv;
    wchar_t **wchar_argv;

    /* Exit status or "exit code": result of pymain_main() */
    int status;
    /* Error message if a function failed */
    _PyInitError err;

    /* non-zero is stdin is a TTY or if -i option is used */
    int stdin_is_interactive;
    int skip_first_line;         /* -x option */
    wchar_t *filename;           /* Trailing arg without -c or -m */
    wchar_t *command;            /* -c argument */
    wchar_t *module;             /* -m argument */
} _PyMain;

#define _PyMain_INIT {.err = _Py_INIT_OK()}
/* Note: _PyMain_INIT sets other fields to 0/NULL */


/* Non-zero if filename, command (-c) or module (-m) is set
   on the command line */
#define RUN_CODE(pymain) \
    (pymain->command != NULL || pymain->filename != NULL \
     || pymain->module != NULL)


static wchar_t*
pymain_wstrdup(_PyMain *pymain, const wchar_t *str)
{
    wchar_t *str2 = _PyMem_RawWcsdup(str);
    if (str2 == NULL) {
        pymain->err = _Py_INIT_NO_MEMORY();
        return NULL;
    }
    return str2;
}


static int
pymain_init_cmdline_argv(_PyMain *pymain, _PyCoreConfig *config,
                         _PyCmdline *cmdline)
{
    assert(cmdline->argv == NULL);

    if (pymain->use_bytes_argv) {
        /* +1 for a the NULL terminator */
        size_t size = sizeof(wchar_t*) * (pymain->argc + 1);
        wchar_t** argv = (wchar_t **)PyMem_RawMalloc(size);
        if (argv == NULL) {
            pymain->err = _Py_INIT_NO_MEMORY();
            return -1;
        }

        for (int i = 0; i < pymain->argc; i++) {
            size_t len;
            wchar_t *arg = Py_DecodeLocale(pymain->bytes_argv[i], &len);
            if (arg == NULL) {
                _Py_wstrlist_clear(i, argv);
                pymain->err = DECODE_LOCALE_ERR("command line arguments",
                                                (Py_ssize_t)len);
                return -1;
            }
            argv[i] = arg;
        }
        argv[pymain->argc] = NULL;

        cmdline->argv = argv;
    }
    else {
        cmdline->argv = pymain->wchar_argv;
    }

    wchar_t *program;
    if (pymain->argc >= 1 && cmdline->argv != NULL) {
        program = cmdline->argv[0];
    }
    else {
        program = L"";
    }
    config->program = pymain_wstrdup(pymain, program);
    if (config->program == NULL) {
        return -1;
    }

    return 0;
}


static void
pymain_clear_cmdline(_PyMain *pymain, _PyCmdline *cmdline)
{
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    _Py_wstrlist_clear(cmdline->nwarnoption, cmdline->warnoptions);
    cmdline->nwarnoption = 0;
    cmdline->warnoptions = NULL;

    _Py_wstrlist_clear(cmdline->nenv_warnoption, cmdline->env_warnoptions);
    cmdline->nenv_warnoption = 0;
    cmdline->env_warnoptions = NULL;

    if (pymain->use_bytes_argv && cmdline->argv != NULL) {
        _Py_wstrlist_clear(pymain->argc, cmdline->argv);
    }
    cmdline->argv = NULL;

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
}


static void
pymain_clear_pymain(_PyMain *pymain)
{
#define CLEAR(ATTR) \
    do { \
        PyMem_RawFree(ATTR); \
        ATTR = NULL; \
    } while (0)

    CLEAR(pymain->filename);
    CLEAR(pymain->command);
    CLEAR(pymain->module);
#undef CLEAR
}

static void
pymain_clear_config(_PyCoreConfig *config)
{
    /* Clear core config with the memory allocator
       used by pymain_read_conf() */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    _PyCoreConfig_Clear(config);

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
}


static void
pymain_free(_PyMain *pymain)
{
    _PyImport_Fini2();

    /* Free global variables which cannot be freed in Py_Finalize():
       configuration options set before Py_Initialize() which should
       remain valid after Py_Finalize(), since
       Py_Initialize()-Py_Finalize() can be called multiple times. */
    _PyPathConfig_ClearGlobal();
    _Py_ClearStandardStreamEncoding();

    /* Force the allocator used by pymain_read_conf() */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    pymain_clear_pymain(pymain);

    _Py_wstrlist_clear(orig_argc, orig_argv);
    orig_argc = 0;
    orig_argv = NULL;

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

#ifdef __INSURE__
    /* Insure++ is a memory analysis tool that aids in discovering
     * memory leaks and other memory problems.  On Python exit, the
     * interned string dictionaries are flagged as being in use at exit
     * (which it is).  Under normal circumstances, this is fine because
     * the memory will be automatically reclaimed by the system.  Under
     * memory debugging, it's a huge source of useless noise, so we
     * trade off slower shutdown for less distraction in the memory
     * reports.  -baw
     */
    _Py_ReleaseInternedUnicodeStrings();
#endif /* __INSURE__ */
}


static int
pymain_sys_path_add_path0(PyInterpreterState *interp, PyObject *path0)
{
    PyObject *sys_path;
    PyObject *sysdict = interp->sysdict;
    if (sysdict != NULL) {
        sys_path = PyDict_GetItemString(sysdict, "path");
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


_PyInitError
_Py_wstrlist_append(int *len, wchar_t ***list, const wchar_t *str)
{
    if (*len == INT_MAX) {
        /* len+1 would overflow */
        return _Py_INIT_NO_MEMORY();
    }
    wchar_t *str2 = _PyMem_RawWcsdup(str);
    if (str2 == NULL) {
        return _Py_INIT_NO_MEMORY();
    }

    size_t size = (*len + 1) * sizeof(list[0]);
    wchar_t **list2 = (wchar_t **)PyMem_RawRealloc(*list, size);
    if (list2 == NULL) {
        PyMem_RawFree(str2);
        return _Py_INIT_NO_MEMORY();
    }
    list2[*len] = str2;
    *list = list2;
    (*len)++;
    return _Py_INIT_OK();
}


static int
pymain_wstrlist_append(_PyMain *pymain, int *len, wchar_t ***list, const wchar_t *str)
{
    _PyInitError err = _Py_wstrlist_append(len, list, str);
    if (_Py_INIT_FAILED(err)) {
        pymain->err = err;
        return -1;
    }
    return 0;
}


/* Parse the command line arguments
   Return 0 on success.
   Return 1 if parsing failed.
   Set pymain->err and return -1 on other errors. */
static int
pymain_parse_cmdline_impl(_PyMain *pymain, _PyCoreConfig *config,
                          _PyCmdline *cmdline)
{
    _PyOS_ResetGetOpt();
    do {
        int longindex = -1;
        int c = _PyOS_GetOpt(pymain->argc, cmdline->argv, PROGRAM_OPTS,
                             longoptions, &longindex);
        if (c == EOF) {
            break;
        }

        if (c == 'c') {
            /* -c is the last option; following arguments
               that look like options are left for the
               command to interpret. */
            size_t len = wcslen(_PyOS_optarg) + 1 + 1;
            wchar_t *command = PyMem_RawMalloc(sizeof(wchar_t) * len);
            if (command == NULL) {
                pymain->err = _Py_INIT_NO_MEMORY();
                return -1;
            }
            memcpy(command, _PyOS_optarg, (len - 2) * sizeof(wchar_t));
            command[len - 2] = '\n';
            command[len - 1] = 0;
            pymain->command = command;
            break;
        }

        if (c == 'm') {
            /* -m is the last option; following arguments
               that look like options are left for the
               module to interpret. */
            pymain->module = pymain_wstrdup(pymain, _PyOS_optarg);
            if (pymain->module == NULL) {
                return -1;
            }
            break;
        }

        switch (c) {
        case 0:
            // Handle long option.
            assert(longindex == 0); // Only one long option now.
            if (!wcscmp(_PyOS_optarg, L"always")) {
                config->_check_hash_pycs_mode = "always";
            } else if (!wcscmp(_PyOS_optarg, L"never")) {
                config->_check_hash_pycs_mode = "never";
            } else if (!wcscmp(_PyOS_optarg, L"default")) {
                config->_check_hash_pycs_mode = "default";
            } else {
                fprintf(stderr, "--check-hash-based-pycs must be one of "
                        "'default', 'always', or 'never'\n");
                return 1;
            }
            break;

        case 'b':
            config->bytes_warning++;
            break;

        case 'd':
            config->parser_debug++;
            break;

        case 'i':
            config->inspect++;
            config->interactive++;
            break;

        case 'I':
            config->isolated++;
            break;

        /* case 'J': reserved for Jython */

        case 'O':
            config->optimization_level++;
            break;

        case 'B':
            config->write_bytecode = 0;
            break;

        case 's':
            config->user_site_directory = 0;
            break;

        case 'S':
            config->site_import = 0;
            break;

        case 'E':
            config->use_environment = 0;
            break;

        case 't':
            /* ignored for backwards compatibility */
            break;

        case 'u':
            config->buffered_stdio = 0;
            break;

        case 'v':
            config->verbose++;
            break;

        case 'x':
            pymain->skip_first_line = 1;
            break;

        case 'h':
        case '?':
            cmdline->print_help++;
            break;

        case 'V':
            cmdline->print_version++;
            break;

        case 'W':
            if (pymain_wstrlist_append(pymain,
                                       &cmdline->nwarnoption,
                                       &cmdline->warnoptions,
                                       _PyOS_optarg) < 0) {
                return -1;
            }
            break;

        case 'X':
            if (pymain_wstrlist_append(pymain,
                                       &config->nxoption,
                                       &config->xoptions,
                                       _PyOS_optarg) < 0) {
                return -1;
            }
            break;

        case 'q':
            config->quiet++;
            break;

        case 'R':
            config->use_hash_seed = 0;
            break;

        /* This space reserved for other options */

        default:
            /* unknown argument: parsing failed */
            return 1;
        }
    } while (1);

    if (pymain->command == NULL && pymain->module == NULL
        && _PyOS_optind < pymain->argc
        && wcscmp(cmdline->argv[_PyOS_optind], L"-") != 0)
    {
        pymain->filename = pymain_wstrdup(pymain, cmdline->argv[_PyOS_optind]);
        if (pymain->filename == NULL) {
            return -1;
        }
    }

    /* -c and -m options are exclusive */
    assert(!(pymain->command != NULL && pymain->module != NULL));

    return 0;
}


static int
add_xoption(PyObject *opts, const wchar_t *s)
{
    PyObject *name, *value;

    const wchar_t *name_end = wcschr(s, L'=');
    if (!name_end) {
        name = PyUnicode_FromWideChar(s, -1);
        value = Py_True;
        Py_INCREF(value);
    }
    else {
        name = PyUnicode_FromWideChar(s, name_end - s);
        value = PyUnicode_FromWideChar(name_end + 1, -1);
    }
    if (name == NULL || value == NULL) {
        goto error;
    }
    if (PyDict_SetItem(opts, name, value) < 0) {
        goto error;
    }
    Py_DECREF(name);
    Py_DECREF(value);
    return 0;

error:
    Py_XDECREF(name);
    Py_XDECREF(value);
    return -1;
}


static PyObject*
config_create_xoptions_dict(const _PyCoreConfig *config)
{
    int nxoption = config->nxoption;
    wchar_t **xoptions = config->xoptions;
    PyObject *dict = PyDict_New();
    if (dict == NULL) {
        return NULL;
    }

    for (int i=0; i < nxoption; i++) {
        wchar_t *option = xoptions[i];
        if (add_xoption(dict, option) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
    }

    return dict;
}


static _PyInitError
config_add_warnings_optlist(_PyCoreConfig *config, int len, wchar_t **options)
{
    for (int i = 0; i < len; i++) {
        _PyInitError err = _Py_wstrlist_append(&config->nwarnoption,
                                               &config->warnoptions,
                                               options[i]);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }
    return _Py_INIT_OK();
}


static _PyInitError
config_init_warnoptions(_PyCoreConfig *config, _PyCmdline *cmdline)
{
    _PyInitError err;

    assert(config->nwarnoption == 0);

    /* The priority order for warnings configuration is (highest precedence
     * first):
     *
     * - the BytesWarning filter, if needed ('-b', '-bb')
     * - any '-W' command line options; then
     * - the 'PYTHONWARNINGS' environment variable; then
     * - the dev mode filter ('-X dev', 'PYTHONDEVMODE'); then
     * - any implicit filters added by _warnings.c/warnings.py
     *
     * All settings except the last are passed to the warnings module via
     * the `sys.warnoptions` list. Since the warnings module works on the basis
     * of "the most recently added filter will be checked first", we add
     * the lowest precedence entries first so that later entries override them.
     */

    if (config->dev_mode) {
        err = _Py_wstrlist_append(&config->nwarnoption,
                                  &config->warnoptions,
                                  L"default");
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    err = config_add_warnings_optlist(config,
                                      cmdline->nenv_warnoption,
                                      cmdline->env_warnoptions);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    err = config_add_warnings_optlist(config,
                                      cmdline->nwarnoption,
                                      cmdline->warnoptions);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    /* If the bytes_warning_flag isn't set, bytesobject.c and bytearrayobject.c
     * don't even try to emit a warning, so we skip setting the filter in that
     * case.
     */
    if (config->bytes_warning) {
        wchar_t *filter;
        if (config->bytes_warning> 1) {
            filter = L"error::BytesWarning";
        }
        else {
            filter = L"default::BytesWarning";
        }
        err = _Py_wstrlist_append(&config->nwarnoption,
                                  &config->warnoptions,
                                  filter);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }
    return _Py_INIT_OK();
}


/* Get warning options from PYTHONWARNINGS environment variable.
   Return 0 on success.
   Set pymain->err and return -1 on error. */
static _PyInitError
cmdline_init_env_warnoptions(_PyMain *pymain, const _PyCoreConfig *config,
                             _PyCmdline *cmdline)
{
    wchar_t *env;
    int res = _PyCoreConfig_GetEnvDup(config, &env,
                                      L"PYTHONWARNINGS", "PYTHONWARNINGS");
    if (res < 0) {
        return DECODE_LOCALE_ERR("PYTHONWARNINGS", res);
    }

    if (env == NULL) {
        return _Py_INIT_OK();
    }


    wchar_t *warning, *context = NULL;
    for (warning = WCSTOK(env, L",", &context);
         warning != NULL;
         warning = WCSTOK(NULL, L",", &context))
    {
        _PyInitError err = _Py_wstrlist_append(&cmdline->nenv_warnoption,
                                               &cmdline->env_warnoptions,
                                               warning);
        if (_Py_INIT_FAILED(err)) {
            PyMem_RawFree(env);
            return err;
        }
    }
    PyMem_RawFree(env);
    return _Py_INIT_OK();
}


static void
pymain_init_stdio(_PyMain *pymain, _PyCoreConfig *config)
{
    pymain->stdin_is_interactive = (isatty(fileno(stdin))
                                    || config->interactive);

#if defined(MS_WINDOWS) || defined(__CYGWIN__)
    /* don't translate newlines (\r\n <=> \n) */
    _setmode(fileno(stdin), O_BINARY);
    _setmode(fileno(stdout), O_BINARY);
    _setmode(fileno(stderr), O_BINARY);
#endif

    if (!config->buffered_stdio) {
#ifdef HAVE_SETVBUF
        setvbuf(stdin,  (char *)NULL, _IONBF, BUFSIZ);
        setvbuf(stdout, (char *)NULL, _IONBF, BUFSIZ);
        setvbuf(stderr, (char *)NULL, _IONBF, BUFSIZ);
#else /* !HAVE_SETVBUF */
        setbuf(stdin,  (char *)NULL);
        setbuf(stdout, (char *)NULL);
        setbuf(stderr, (char *)NULL);
#endif /* !HAVE_SETVBUF */
    }
    else if (config->interactive) {
#ifdef MS_WINDOWS
        /* Doesn't have to have line-buffered -- use unbuffered */
        /* Any set[v]buf(stdin, ...) screws up Tkinter :-( */
        setvbuf(stdout, (char *)NULL, _IONBF, BUFSIZ);
#else /* !MS_WINDOWS */
#ifdef HAVE_SETVBUF
        setvbuf(stdin,  (char *)NULL, _IOLBF, BUFSIZ);
        setvbuf(stdout, (char *)NULL, _IOLBF, BUFSIZ);
#endif /* HAVE_SETVBUF */
#endif /* !MS_WINDOWS */
        /* Leave stderr alone - it should be unbuffered anyway. */
    }
}


static void
pymain_header(_PyMain *pymain, const _PyCoreConfig *config)
{
    if (config->quiet) {
        return;
    }

    if (!config->verbose && (RUN_CODE(pymain) || !pymain->stdin_is_interactive)) {
        return;
    }

    fprintf(stderr, "Python %s on %s\n", Py_GetVersion(), Py_GetPlatform());
    if (config->site_import) {
        fprintf(stderr, "%s\n", COPYRIGHT);
    }
}


static int
pymain_init_core_argv(_PyMain *pymain, _PyCoreConfig *config, _PyCmdline *cmdline)
{
    /* Copy argv to be able to modify it (to force -c/-m) */
    int argc = pymain->argc - _PyOS_optind;
    wchar_t **argv;

    if (argc <= 0 || cmdline->argv == NULL) {
        /* Ensure at least one (empty) argument is seen */
        static wchar_t *empty_argv[1] = {L""};
        argc = 1;
        argv = _Py_wstrlist_copy(1, empty_argv);
    }
    else {
        argv = _Py_wstrlist_copy(argc, &cmdline->argv[_PyOS_optind]);
    }

    if (argv == NULL) {
        pymain->err = _Py_INIT_NO_MEMORY();
        return -1;
    }

    wchar_t *arg0 = NULL;
    if (pymain->command != NULL) {
        /* Force sys.argv[0] = '-c' */
        arg0 = L"-c";
    }
    else if (pymain->module != NULL) {
        /* Force sys.argv[0] = '-m'*/
        arg0 = L"-m";
    }
    if (arg0 != NULL) {
        arg0 = _PyMem_RawWcsdup(arg0);
        if (arg0 == NULL) {
            _Py_wstrlist_clear(argc, argv);
            pymain->err = _Py_INIT_NO_MEMORY();
            return -1;
        }

        assert(argc >= 1);
        PyMem_RawFree(argv[0]);
        argv[0] = arg0;
    }

    config->argc = argc;
    config->argv = argv;
    return 0;
}


PyObject*
_Py_wstrlist_as_pylist(int len, wchar_t **list)
{
    assert(list != NULL || len < 1);

    PyObject *pylist = PyList_New(len);
    if (pylist == NULL) {
        return NULL;
    }

    for (int i = 0; i < len; i++) {
        PyObject *v = PyUnicode_FromWideChar(list[i], -1);
        if (v == NULL) {
            Py_DECREF(pylist);
            return NULL;
        }
        PyList_SET_ITEM(pylist, i, v);
    }
    return pylist;
}


static void
pymain_import_readline(_PyMain *pymain, const _PyCoreConfig *config)
{
    if (config->isolated) {
        return;
    }
    if (!config->inspect && RUN_CODE(pymain)) {
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


static void
pymain_run_startup(_PyMain *pymain, _PyCoreConfig *config, PyCompilerFlags *cf)
{
    const char *startup = _PyCoreConfig_GetEnv(config, "PYTHONSTARTUP");
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
pymain_run_file(_PyMain *pymain, _PyCoreConfig *config, PyCompilerFlags *cf)
{
    const wchar_t *filename = pymain->filename;
    FILE *fp = _Py_wfopen(filename, L"r");
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
        pymain->status = 2;
        return;
    }

    if (pymain->skip_first_line) {
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
        pymain->status = 1;
        fclose(fp);
        return;
    }

    /* call pending calls like signal handlers (SIGINT) */
    if (Py_MakePendingCalls() == -1) {
        PyErr_Print();
        pymain->status = 1;
        fclose(fp);
        return;
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
    pymain->status = (run != 0);
}


static void
pymain_run_stdin(_PyMain *pymain, _PyCoreConfig *config, PyCompilerFlags *cf)
{
    if (pymain->stdin_is_interactive) {
        Py_InspectFlag = 0; /* do exit on SystemExit */
        config->inspect = 0;
        pymain_run_startup(pymain, config, cf);
        pymain_run_interactive_hook();
    }

    /* call pending calls like signal handlers (SIGINT) */
    if (Py_MakePendingCalls() == -1) {
        PyErr_Print();
        pymain->status = 1;
        return;
    }

    int run = PyRun_AnyFileExFlags(stdin, "<stdin>", 0, cf);
    pymain->status = (run != 0);
}


static void
pymain_repl(_PyMain *pymain, _PyCoreConfig *config, PyCompilerFlags *cf)
{
    /* Check this environment variable at the end, to give programs the
       opportunity to set it from Python. */
    if (!Py_InspectFlag && _PyCoreConfig_GetEnv(config, "PYTHONINSPECT")) {
        Py_InspectFlag = 1;
        config->inspect = 1;
    }

    if (!(Py_InspectFlag && pymain->stdin_is_interactive && RUN_CODE(pymain))) {
        return;
    }

    Py_InspectFlag = 0;
    config->inspect = 0;
    pymain_run_interactive_hook();

    int res = PyRun_AnyFileFlags(stdin, "<stdin>", cf);
    pymain->status = (res != 0);
}


/* Parse the command line.
   Handle --version and --help options directly.

   Return 1 if Python must exit.
   Return 0 on success.
   Set pymain->err and return -1 on failure. */
static int
pymain_parse_cmdline(_PyMain *pymain, _PyCoreConfig *config,
                     _PyCmdline *cmdline)
{
    int res = pymain_parse_cmdline_impl(pymain, config, cmdline);
    if (res < 0) {
        return -1;
    }
    if (res) {
        pymain_usage(1, config->program);
        pymain->status = 2;
        return 1;
    }

    if (pymain->command != NULL || pymain->module != NULL) {
        /* Backup _PyOS_optind */
        _PyOS_optind--;
    }

    return 0;
}


/* Parse command line options and environment variables.
   This code must not use Python runtime apart PyMem_Raw memory allocator.

   Return 0 on success.
   Return 1 if Python is done and must exit.
   Set pymain->err and return -1 on error. */
static int
pymain_read_conf_impl(_PyMain *pymain, _PyCoreConfig *config,
                      _PyCmdline *cmdline)
{
    _PyInitError err;

    int res = pymain_parse_cmdline(pymain, config, cmdline);
    if (res != 0) {
        return res;
    }

    if (pymain_init_core_argv(pymain, config, cmdline) < 0) {
        return -1;
    }

    err = _PyCoreConfig_Read(config);
    if (_Py_INIT_FAILED(err)) {
        pymain->err = err;
        return -1;
    }

    if (config->use_environment) {
        err = cmdline_init_env_warnoptions(pymain, config, cmdline);
        if (_Py_INIT_FAILED(err)) {
            pymain->err = err;
            return -1;
        }
    }

    err = config_init_warnoptions(config, cmdline);
    if (_Py_INIT_FAILED(err)) {
        pymain->err = err;
        return -1;
    }
    return 0;
}


/* Read the configuration and initialize the LC_CTYPE locale:
   enable UTF-8 mode (PEP 540) and/or coerce the C locale (PEP 538). */
static int
pymain_read_conf(_PyMain *pymain, _PyCoreConfig *config,
                 _PyCmdline *cmdline)
{
    int init_utf8_mode = Py_UTF8Mode;
#ifdef MS_WINDOWS
    int init_legacy_encoding = Py_LegacyWindowsFSEncodingFlag;
#endif
    _PyCoreConfig save_config = _PyCoreConfig_INIT;
    int res = -1;
    int locale_coerced = 0;
    int loops = 0;

    if (_PyCoreConfig_Copy(&save_config, config) < 0) {
        pymain->err = _Py_INIT_NO_MEMORY();
        goto done;
    }

    /* Set LC_CTYPE to the user preferred locale */
    _Py_SetLocaleFromEnv(LC_CTYPE);

    while (1) {
        int utf8_mode = config->utf8_mode;
        int encoding_changed = 0;

        /* Watchdog to prevent an infinite loop */
        loops++;
        if (loops == 3) {
            pymain->err = _Py_INIT_ERR("Encoding changed twice while "
                                       "reading the configuration");
            goto done;
        }

        /* bpo-34207: Py_DecodeLocale() and Py_EncodeLocale() depend
           on Py_UTF8Mode and Py_LegacyWindowsFSEncodingFlag. */
        Py_UTF8Mode = config->utf8_mode;
#ifdef MS_WINDOWS
        Py_LegacyWindowsFSEncodingFlag = config->legacy_windows_fs_encoding;
#endif

        if (pymain_init_cmdline_argv(pymain, config, cmdline) < 0) {
            goto done;
        }

        int conf_res = pymain_read_conf_impl(pymain, config, cmdline);
        if (conf_res != 0) {
            res = conf_res;
            goto done;
        }

        /* The legacy C locale assumes ASCII as the default text encoding, which
         * causes problems not only for the CPython runtime, but also other
         * components like GNU readline.
         *
         * Accordingly, when the CLI detects it, it attempts to coerce it to a
         * more capable UTF-8 based alternative.
         *
         * See the documentation of the PYTHONCOERCECLOCALE setting for more
         * details.
         */
        if (config->coerce_c_locale && !locale_coerced) {
            locale_coerced = 1;
            _Py_CoerceLegacyLocale(config->coerce_c_locale_warn);
            encoding_changed = 1;
        }

        if (utf8_mode == -1) {
            if (config->utf8_mode == 1) {
                /* UTF-8 Mode enabled */
                encoding_changed = 1;
            }
        }
        else {
            if (config->utf8_mode != utf8_mode) {
                encoding_changed = 1;
            }
        }

        if (!encoding_changed) {
            break;
        }

        /* Reset the configuration before reading again the configuration,
           just keep UTF-8 Mode value. */
        int new_utf8_mode = config->utf8_mode;
        int new_coerce_c_locale = config->coerce_c_locale;
        if (_PyCoreConfig_Copy(config, &save_config) < 0) {
            pymain->err = _Py_INIT_NO_MEMORY();
            goto done;
        }
        pymain_clear_cmdline(pymain, cmdline);
        pymain_clear_pymain(pymain);
        memset(cmdline, 0, sizeof(*cmdline));
        config->utf8_mode = new_utf8_mode;
        config->coerce_c_locale = new_coerce_c_locale;

        /* The encoding changed: read again the configuration
           with the new encoding */
    }
    res = 0;

done:
    _PyCoreConfig_Clear(&save_config);
    Py_UTF8Mode = init_utf8_mode ;
#ifdef MS_WINDOWS
    Py_LegacyWindowsFSEncodingFlag = init_legacy_encoding;
#endif
    return res;
}


void
_PyMainInterpreterConfig_Clear(_PyMainInterpreterConfig *config)
{
    Py_CLEAR(config->argv);
    Py_CLEAR(config->executable);
    Py_CLEAR(config->prefix);
    Py_CLEAR(config->base_prefix);
    Py_CLEAR(config->exec_prefix);
    Py_CLEAR(config->base_exec_prefix);
    Py_CLEAR(config->warnoptions);
    Py_CLEAR(config->xoptions);
    Py_CLEAR(config->module_search_path);
    Py_CLEAR(config->pycache_prefix);
}


static PyObject*
config_copy_attr(PyObject *obj)
{
    if (PyUnicode_Check(obj)) {
        Py_INCREF(obj);
        return obj;
    }
    else if (PyList_Check(obj)) {
        return PyList_GetSlice(obj, 0, Py_SIZE(obj));
    }
    else if (PyDict_Check(obj)) {
        /* The dict type is used for xoptions. Make the assumption that keys
           and values are immutables */
        return PyDict_Copy(obj);
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "cannot copy config attribute of type %.200s",
                     Py_TYPE(obj)->tp_name);
        return NULL;
    }
}


int
_PyMainInterpreterConfig_Copy(_PyMainInterpreterConfig *config,
                              const _PyMainInterpreterConfig *config2)
{
    _PyMainInterpreterConfig_Clear(config);

#define COPY_ATTR(ATTR) config->ATTR = config2->ATTR
#define COPY_OBJ_ATTR(ATTR) \
    do { \
        if (config2->ATTR != NULL) { \
            config->ATTR = config_copy_attr(config2->ATTR); \
            if (config->ATTR == NULL) { \
                return -1; \
            } \
        } \
    } while (0)

    COPY_ATTR(install_signal_handlers);
    COPY_OBJ_ATTR(argv);
    COPY_OBJ_ATTR(executable);
    COPY_OBJ_ATTR(prefix);
    COPY_OBJ_ATTR(base_prefix);
    COPY_OBJ_ATTR(exec_prefix);
    COPY_OBJ_ATTR(base_exec_prefix);
    COPY_OBJ_ATTR(warnoptions);
    COPY_OBJ_ATTR(xoptions);
    COPY_OBJ_ATTR(module_search_path);
    COPY_OBJ_ATTR(pycache_prefix);
#undef COPY_ATTR
#undef COPY_OBJ_ATTR
    return 0;
}


PyObject*
_PyMainInterpreterConfig_AsDict(const _PyMainInterpreterConfig *config)
{
    PyObject *dict, *obj;
    int res;

    dict = PyDict_New();
    if (dict == NULL) {
        return NULL;
    }

#define SET_ITEM_INT(ATTR) \
    do { \
        obj = PyLong_FromLong(config->ATTR); \
        if (obj == NULL) { \
            goto fail; \
        } \
        res = PyDict_SetItemString(dict, #ATTR, obj); \
        Py_DECREF(obj); \
        if (res < 0) { \
            goto fail; \
        } \
    } while (0)

#define SET_ITEM_OBJ(ATTR) \
    do { \
        obj = config->ATTR; \
        if (obj == NULL) { \
            obj = Py_None; \
        } \
        res = PyDict_SetItemString(dict, #ATTR, obj); \
        if (res < 0) { \
            goto fail; \
        } \
    } while (0)

    SET_ITEM_INT(install_signal_handlers);
    SET_ITEM_OBJ(argv);
    SET_ITEM_OBJ(executable);
    SET_ITEM_OBJ(prefix);
    SET_ITEM_OBJ(base_prefix);
    SET_ITEM_OBJ(exec_prefix);
    SET_ITEM_OBJ(base_exec_prefix);
    SET_ITEM_OBJ(warnoptions);
    SET_ITEM_OBJ(xoptions);
    SET_ITEM_OBJ(module_search_path);
    SET_ITEM_OBJ(pycache_prefix);

    return dict;

fail:
    Py_DECREF(dict);
    return NULL;

#undef SET_ITEM_OBJ
}


_PyInitError
_PyMainInterpreterConfig_Read(_PyMainInterpreterConfig *main_config,
                              const _PyCoreConfig *config)
{
    if (main_config->install_signal_handlers < 0) {
        main_config->install_signal_handlers = config->install_signal_handlers;
    }

    if (main_config->xoptions == NULL) {
        main_config->xoptions = config_create_xoptions_dict(config);
        if (main_config->xoptions == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
    }

#define COPY_WSTR(ATTR) \
    do { \
        if (main_config->ATTR == NULL && config->ATTR != NULL) { \
            main_config->ATTR = PyUnicode_FromWideChar(config->ATTR, -1); \
            if (main_config->ATTR == NULL) { \
                return _Py_INIT_NO_MEMORY(); \
            } \
        } \
    } while (0)
#define COPY_WSTRLIST(ATTR, LEN, LIST) \
    do { \
        if (ATTR == NULL) { \
            ATTR = _Py_wstrlist_as_pylist(LEN, LIST); \
            if (ATTR == NULL) { \
                return _Py_INIT_NO_MEMORY(); \
            } \
        } \
    } while (0)

    COPY_WSTRLIST(main_config->warnoptions,
                  config->nwarnoption, config->warnoptions);
    if (config->argc >= 0) {
        COPY_WSTRLIST(main_config->argv,
                      config->argc, config->argv);
    }

    if (config->_install_importlib) {
        COPY_WSTR(executable);
        COPY_WSTR(prefix);
        COPY_WSTR(base_prefix);
        COPY_WSTR(exec_prefix);
        COPY_WSTR(base_exec_prefix);

        COPY_WSTRLIST(main_config->module_search_path,
                      config->nmodule_search_path, config->module_search_paths);

        if (config->pycache_prefix != NULL) {
            COPY_WSTR(pycache_prefix);
        } else {
            main_config->pycache_prefix = NULL;
        }

    }

    return _Py_INIT_OK();
#undef COPY_WSTR
#undef COPY_WSTRLIST
}


static int
pymain_init_python_main(_PyMain *pymain, _PyCoreConfig *config,
                        PyInterpreterState *interp)
{
    _PyInitError err;

    _PyMainInterpreterConfig main_config = _PyMainInterpreterConfig_INIT;
    err = _PyMainInterpreterConfig_Read(&main_config, config);
    if (!_Py_INIT_FAILED(err)) {
        err = _Py_InitializeMainInterpreter(interp, &main_config);
    }
    _PyMainInterpreterConfig_Clear(&main_config);

    if (_Py_INIT_FAILED(err)) {
        pymain->err = err;
        return -1;
    }
    return 0;
}


static int
pymain_run_python(_PyMain *pymain, PyInterpreterState *interp)
{
    int res = 0;
    _PyCoreConfig *config = &interp->core_config;

    PyObject *main_importer_path = NULL;
    if (pymain->filename != NULL) {
        /* If filename is a package (ex: directory or ZIP file) which contains
           __main__.py, main_importer_path is set to filename and will be
           prepended to sys.path.

           Otherwise, main_importer_path is set to NULL. */
        main_importer_path = pymain_get_importer(pymain->filename);
    }

    if (main_importer_path != NULL) {
        if (pymain_sys_path_add_path0(interp, main_importer_path) < 0) {
            pymain->status = 1;
            goto done;
        }
    }
    else if (!config->isolated) {
        PyObject *path0 = _PyPathConfig_ComputeArgv0(config->argc,
                                                     config->argv);
        if (path0 == NULL) {
            pymain->err = _Py_INIT_NO_MEMORY();
            res = -1;
            goto done;
        }

        if (pymain_sys_path_add_path0(interp, path0) < 0) {
            Py_DECREF(path0);
            pymain->status = 1;
            goto done;
        }
        Py_DECREF(path0);
    }

    PyCompilerFlags cf = {.cf_flags = 0};

    pymain_header(pymain, config);
    pymain_import_readline(pymain, config);

    if (pymain->command) {
        pymain->status = pymain_run_command(pymain->command, &cf);
    }
    else if (pymain->module) {
        pymain->status = (pymain_run_module(pymain->module, 1) != 0);
    }
    else if (main_importer_path != NULL) {
        int sts = pymain_run_module(L"__main__", 0);
        pymain->status = (sts != 0);
    }
    else if (pymain->filename != NULL) {
        pymain_run_file(pymain, config, &cf);
    }
    else {
        pymain_run_stdin(pymain, config, &cf);
    }

    pymain_repl(pymain, config, &cf);

done:
    Py_XDECREF(main_importer_path);
    return res;
}


static int
pymain_cmdline_impl(_PyMain *pymain, _PyCoreConfig *config,
                    _PyCmdline *cmdline)
{
    pymain->err = _PyRuntime_Initialize();
    if (_Py_INIT_FAILED(pymain->err)) {
        return -1;
    }

    int res = pymain_read_conf(pymain, config, cmdline);
    if (res < 0) {
        return -1;
    }
    if (res > 0) {
        /* --help or --version command: we are done */
        return 1;
    }

    if (cmdline->print_help) {
        pymain_usage(0, config->program);
        return 1;
    }

    if (cmdline->print_version) {
        printf("Python %s\n",
               (cmdline->print_version >= 2) ? Py_GetVersion() : PY_VERSION);
        return 1;
    }

    /* For Py_GetArgcArgv(). Cleared by pymain_free(). */
    orig_argv = _Py_wstrlist_copy(pymain->argc, cmdline->argv);
    if (orig_argv == NULL) {
        pymain->err = _Py_INIT_NO_MEMORY();
        return -1;
    }
    orig_argc = pymain->argc;
    return 0;
}


/* Read the configuration into _PyCoreConfig and _PyMain, initialize the
   LC_CTYPE locale and Py_DecodeLocale().

   Configuration:

   * Command line arguments
   * Environment variables
   * Py_xxx global configuration variables

   _PyCmdline is a temporary structure used to prioritize these
   variables. */
static int
pymain_cmdline(_PyMain *pymain, _PyCoreConfig *config)
{
    /* Force default allocator, since pymain_free() and pymain_clear_config()
       must use the same allocator than this function. */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
#ifdef Py_DEBUG
    PyMemAllocatorEx default_alloc;
    PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &default_alloc);
#endif

    _PyCmdline cmdline;
    memset(&cmdline, 0, sizeof(cmdline));

    int res = pymain_cmdline_impl(pymain, config, &cmdline);

    pymain_clear_cmdline(pymain, &cmdline);

#ifdef Py_DEBUG
    /* Make sure that PYMEM_DOMAIN_RAW has not been modified */
    PyMemAllocatorEx cur_alloc;
    PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &cur_alloc);
    assert(memcmp(&cur_alloc, &default_alloc, sizeof(cur_alloc)) == 0);
#endif
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    return res;
}


static int
pymain_init(_PyMain *pymain, PyInterpreterState **interp_p)
{
    /* 754 requires that FP exceptions run in "no stop" mode by default,
     * and until C vendors implement C99's ways to control FP exceptions,
     * Python requires non-stop mode.  Alas, some platforms enable FP
     * exceptions by default.  Here we disable them.
     */
#ifdef __FreeBSD__
    fedisableexcept(FE_OVERFLOW);
#endif

    _PyCoreConfig local_config = _PyCoreConfig_INIT;
    _PyCoreConfig *config = &local_config;

    _PyCoreConfig_GetGlobalConfig(config);

    int cmd_res = pymain_cmdline(pymain, config);
    if (cmd_res < 0) {
        _Py_FatalInitError(pymain->err);
    }
    if (cmd_res == 1) {
        pymain_clear_config(config);
        return 1;
    }

    _PyCoreConfig_SetGlobalConfig(config);

    pymain_init_stdio(pymain, config);

    PyInterpreterState *interp;
    pymain->err = _Py_InitializeCore(&interp, config);
    if (_Py_INIT_FAILED(pymain->err)) {
        _Py_FatalInitError(pymain->err);
    }
    *interp_p = interp;

    pymain_clear_config(config);
    config = &interp->core_config;

    if (pymain_init_python_main(pymain, config, interp) < 0) {
        _Py_FatalInitError(pymain->err);
    }
    return 0;
}


static int
pymain_main(_PyMain *pymain)
{
    PyInterpreterState *interp;
    int res = pymain_init(pymain, &interp);
    if (res != 1) {
        if (pymain_run_python(pymain, interp) < 0) {
            _Py_FatalInitError(pymain->err);
        }

        if (Py_FinalizeEx() < 0) {
            /* Value unlikely to be confused with a non-error exit status or
               other special meaning */
            pymain->status = 120;
        }
    }

    pymain_free(pymain);

    if (_Py_UnhandledKeyboardInterrupt) {
        /* https://bugs.python.org/issue1054041 - We need to exit via the
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
        pymain->status = STATUS_CONTROL_C_EXIT;
#else
        pymain->status = SIGINT + 128;
#endif  /* !MS_WINDOWS */
    }

    return pymain->status;
}


int
Py_Main(int argc, wchar_t **argv)
{
    _PyMain pymain = _PyMain_INIT;
    pymain.use_bytes_argv = 0;
    pymain.argc = argc;
    pymain.wchar_argv = argv;

    return pymain_main(&pymain);
}


int
_Py_UnixMain(int argc, char **argv)
{
    _PyMain pymain = _PyMain_INIT;
    pymain.use_bytes_argv = 1;
    pymain.argc = argc;
    pymain.bytes_argv = argv;

    return pymain_main(&pymain);
}


/* this is gonna seem *real weird*, but if you put some other code between
   Py_Main() and Py_GetArgcArgv() you will need to adjust the test in the
   while statement in Misc/gdbinit:ppystack */

/* Make the *original* argc/argv available to other modules.
   This is rare, but it is needed by the secureware extension. */

void
Py_GetArgcArgv(int *argc, wchar_t ***argv)
{
    *argc = orig_argc;
    *argv = orig_argv;
}

#ifdef __cplusplus
}
#endif
