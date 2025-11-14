#include "Python.h"
#include "pycore_fileutils.h"     // _Py_HasFileSystemDefaultEncodeErrors
#include "pycore_getopt.h"        // _PyOS_GetOpt()
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_interp.h"        // _PyInterpreterState.runtime
#include "pycore_long.h"          // _PY_LONG_MAX_STR_DIGITS_THRESHOLD
#include "pycore_pathconfig.h"    // _Py_path_config
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pylifecycle.h"   // _Py_PreInitializeFromConfig()
#include "pycore_pymem.h"         // _PyMem_DefaultRawMalloc()
#include "pycore_pyhash.h"        // _Py_HashSecret
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_pystats.h"       // _Py_StatsOn()
#include "pycore_sysmodule.h"     // _PySys_SetIntMaxStrDigits()

#include "osdefs.h"               // DELIM

#include <locale.h>               // setlocale()
#include <stdlib.h>               // getenv()
#if defined(MS_WINDOWS) || defined(__CYGWIN__)
#  ifdef HAVE_IO_H
#    include <io.h>
#  endif
#  ifdef HAVE_FCNTL_H
#    include <fcntl.h>            // O_BINARY
#  endif
#endif

#ifdef __APPLE__
/* Enable system log by default on non-macOS Apple platforms */
#  if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define USE_SYSTEM_LOGGER_DEFAULT 1;
#  else
#define USE_SYSTEM_LOGGER_DEFAULT 0;
#  endif
#endif

#include "config_common.h"

/* --- PyConfig setters ------------------------------------------- */

typedef PyObject* (*config_sys_flag_setter) (int value);

static PyObject*
config_sys_flag_long(int value)
{
    return PyLong_FromLong(value);
}

static PyObject*
config_sys_flag_not(int value)
{
    value = (!value);
    return config_sys_flag_long(value);
}

/* --- PyConfig spec ---------------------------------------------- */

typedef enum {
    PyConfig_MEMBER_INT = 0,
    PyConfig_MEMBER_UINT = 1,
    PyConfig_MEMBER_ULONG = 2,
    PyConfig_MEMBER_BOOL = 3,

    PyConfig_MEMBER_WSTR = 10,
    PyConfig_MEMBER_WSTR_OPT = 11,
    PyConfig_MEMBER_WSTR_LIST = 12,
} PyConfigMemberType;

typedef enum {
    // Option which cannot be get or set by PyConfig_Get() and PyConfig_Set()
    PyConfig_MEMBER_INIT_ONLY = 0,

    // Option which cannot be set by PyConfig_Set()
    PyConfig_MEMBER_READ_ONLY = 1,

    // Public option: can be get and set by PyConfig_Get() and PyConfig_Set()
    PyConfig_MEMBER_PUBLIC = 2,
} PyConfigMemberVisibility;

typedef struct {
    const char *attr;
    int flag_index;
    config_sys_flag_setter flag_setter;
} PyConfigSysSpec;

typedef struct {
    const char *name;
    size_t offset;
    PyConfigMemberType type;
    PyConfigMemberVisibility visibility;
    PyConfigSysSpec sys;
} PyConfigSpec;

#define SPEC(MEMBER, TYPE, VISIBILITY, sys) \
    {#MEMBER, offsetof(PyConfig, MEMBER), \
     PyConfig_MEMBER_##TYPE, PyConfig_MEMBER_##VISIBILITY, sys}

#define SYS_ATTR(name) {name, -1, NULL}
#define SYS_FLAG_SETTER(index, setter) {NULL, index, setter}
#define SYS_FLAG(index) SYS_FLAG_SETTER(index, NULL)
#define NO_SYS SYS_ATTR(NULL)

// Update _test_embed_set_config when adding new members
static const PyConfigSpec PYCONFIG_SPEC[] = {
    // --- Public options -----------

    SPEC(argv, WSTR_LIST, PUBLIC, SYS_ATTR("argv")),
    SPEC(base_exec_prefix, WSTR_OPT, PUBLIC, SYS_ATTR("base_exec_prefix")),
    SPEC(base_executable, WSTR_OPT, PUBLIC, SYS_ATTR("_base_executable")),
    SPEC(base_prefix, WSTR_OPT, PUBLIC, SYS_ATTR("base_prefix")),
    SPEC(bytes_warning, UINT, PUBLIC, SYS_FLAG(9)),
    SPEC(cpu_count, INT, PUBLIC, NO_SYS),
    SPEC(exec_prefix, WSTR_OPT, PUBLIC, SYS_ATTR("exec_prefix")),
    SPEC(executable, WSTR_OPT, PUBLIC, SYS_ATTR("executable")),
    SPEC(inspect, BOOL, PUBLIC, SYS_FLAG(1)),
    SPEC(int_max_str_digits, UINT, PUBLIC, NO_SYS),
    SPEC(interactive, BOOL, PUBLIC, SYS_FLAG(2)),
    SPEC(module_search_paths, WSTR_LIST, PUBLIC, SYS_ATTR("path")),
    SPEC(optimization_level, UINT, PUBLIC, SYS_FLAG(3)),
    SPEC(parser_debug, BOOL, PUBLIC, SYS_FLAG(0)),
    SPEC(platlibdir, WSTR, PUBLIC, SYS_ATTR("platlibdir")),
    SPEC(prefix, WSTR_OPT, PUBLIC, SYS_ATTR("prefix")),
    SPEC(pycache_prefix, WSTR_OPT, PUBLIC, SYS_ATTR("pycache_prefix")),
    SPEC(quiet, BOOL, PUBLIC, SYS_FLAG(10)),
    SPEC(stdlib_dir, WSTR_OPT, PUBLIC, SYS_ATTR("_stdlib_dir")),
    SPEC(use_environment, BOOL, PUBLIC, SYS_FLAG_SETTER(7, config_sys_flag_not)),
    SPEC(verbose, UINT, PUBLIC, SYS_FLAG(8)),
    SPEC(warnoptions, WSTR_LIST, PUBLIC, SYS_ATTR("warnoptions")),
    SPEC(write_bytecode, BOOL, PUBLIC, SYS_FLAG_SETTER(4, config_sys_flag_not)),
    SPEC(xoptions, WSTR_LIST, PUBLIC, SYS_ATTR("_xoptions")),

    // --- Read-only options -----------

#ifdef Py_STATS
    SPEC(_pystats, BOOL, READ_ONLY, NO_SYS),
#endif
    SPEC(buffered_stdio, BOOL, READ_ONLY, NO_SYS),
    SPEC(check_hash_pycs_mode, WSTR, READ_ONLY, NO_SYS),
    SPEC(code_debug_ranges, BOOL, READ_ONLY, NO_SYS),
    SPEC(configure_c_stdio, BOOL, READ_ONLY, NO_SYS),
    SPEC(dev_mode, BOOL, READ_ONLY, NO_SYS),  // sys.flags.dev_mode
    SPEC(dump_refs, BOOL, READ_ONLY, NO_SYS),
    SPEC(dump_refs_file, WSTR_OPT, READ_ONLY, NO_SYS),
#ifdef Py_GIL_DISABLED
    SPEC(enable_gil, INT, READ_ONLY, NO_SYS),
    SPEC(tlbc_enabled, INT, READ_ONLY, NO_SYS),
#endif
    SPEC(faulthandler, BOOL, READ_ONLY, NO_SYS),
    SPEC(filesystem_encoding, WSTR, READ_ONLY, NO_SYS),
    SPEC(filesystem_errors, WSTR, READ_ONLY, NO_SYS),
    SPEC(hash_seed, ULONG, READ_ONLY, NO_SYS),
    SPEC(home, WSTR_OPT, READ_ONLY, NO_SYS),
    SPEC(thread_inherit_context, INT, READ_ONLY, NO_SYS),
    SPEC(context_aware_warnings, INT, READ_ONLY, NO_SYS),
    SPEC(import_time, UINT, READ_ONLY, NO_SYS),
    SPEC(install_signal_handlers, BOOL, READ_ONLY, NO_SYS),
    SPEC(isolated, BOOL, READ_ONLY, NO_SYS),  // sys.flags.isolated
#ifdef MS_WINDOWS
    SPEC(legacy_windows_stdio, BOOL, READ_ONLY, NO_SYS),
#endif
    SPEC(malloc_stats, BOOL, READ_ONLY, NO_SYS),
    SPEC(orig_argv, WSTR_LIST, READ_ONLY, SYS_ATTR("orig_argv")),
    SPEC(parse_argv, BOOL, READ_ONLY, NO_SYS),
    SPEC(pathconfig_warnings, BOOL, READ_ONLY, NO_SYS),
    SPEC(perf_profiling, UINT, READ_ONLY, NO_SYS),
    SPEC(remote_debug, BOOL, READ_ONLY, NO_SYS),
    SPEC(program_name, WSTR, READ_ONLY, NO_SYS),
    SPEC(run_command, WSTR_OPT, READ_ONLY, NO_SYS),
    SPEC(run_filename, WSTR_OPT, READ_ONLY, NO_SYS),
    SPEC(run_module, WSTR_OPT, READ_ONLY, NO_SYS),
#ifdef Py_DEBUG
    SPEC(run_presite, WSTR_OPT, READ_ONLY, NO_SYS),
#endif
    SPEC(safe_path, BOOL, READ_ONLY, NO_SYS),
    SPEC(show_ref_count, BOOL, READ_ONLY, NO_SYS),
    SPEC(site_import, BOOL, READ_ONLY, NO_SYS),  // sys.flags.no_site
    SPEC(skip_source_first_line, BOOL, READ_ONLY, NO_SYS),
    SPEC(stdio_encoding, WSTR, READ_ONLY, NO_SYS),
    SPEC(stdio_errors, WSTR, READ_ONLY, NO_SYS),
    SPEC(tracemalloc, UINT, READ_ONLY, NO_SYS),
    SPEC(use_frozen_modules, BOOL, READ_ONLY, NO_SYS),
    SPEC(use_hash_seed, BOOL, READ_ONLY, NO_SYS),
#ifdef __APPLE__
    SPEC(use_system_logger, BOOL, READ_ONLY, NO_SYS),
#endif
    SPEC(user_site_directory, BOOL, READ_ONLY, NO_SYS),  // sys.flags.no_user_site
    SPEC(warn_default_encoding, BOOL, READ_ONLY, NO_SYS),

    // --- Init-only options -----------

    SPEC(_config_init, UINT, INIT_ONLY, NO_SYS),
    SPEC(_init_main, BOOL, INIT_ONLY, NO_SYS),
    SPEC(_install_importlib, BOOL, INIT_ONLY, NO_SYS),
    SPEC(_is_python_build, BOOL, INIT_ONLY, NO_SYS),
    SPEC(module_search_paths_set, BOOL, INIT_ONLY, NO_SYS),
    SPEC(pythonpath_env, WSTR_OPT, INIT_ONLY, NO_SYS),
    SPEC(sys_path_0, WSTR_OPT, INIT_ONLY, NO_SYS),

    // Array terminator
    {NULL, 0, 0, 0, NO_SYS},
};

#undef SPEC
#define SPEC(MEMBER, TYPE, VISIBILITY) \
    {#MEMBER, offsetof(PyPreConfig, MEMBER), PyConfig_MEMBER_##TYPE, \
     PyConfig_MEMBER_##VISIBILITY, NO_SYS}

static const PyConfigSpec PYPRECONFIG_SPEC[] = {
    // --- Read-only options -----------

    SPEC(allocator, INT, READ_ONLY),
    SPEC(coerce_c_locale, BOOL, READ_ONLY),
    SPEC(coerce_c_locale_warn, BOOL, READ_ONLY),
    SPEC(configure_locale, BOOL, READ_ONLY),
#ifdef MS_WINDOWS
    SPEC(legacy_windows_fs_encoding, BOOL, READ_ONLY),
#endif
    SPEC(utf8_mode, BOOL, READ_ONLY),

    // --- Init-only options -----------
    // Members already present in PYCONFIG_SPEC

    SPEC(_config_init, INT, INIT_ONLY),
    SPEC(dev_mode, BOOL, INIT_ONLY),
    SPEC(isolated, BOOL, INIT_ONLY),
    SPEC(parse_argv, BOOL, INIT_ONLY),
    SPEC(use_environment, BOOL, INIT_ONLY),

    // Array terminator
    {NULL, 0, 0, 0, NO_SYS},
};

#undef SPEC
#undef SYS_ATTR
#undef SYS_FLAG_SETTER
#undef SYS_FLAG
#undef NO_SYS


// Forward declarations
static PyObject* config_get(const PyConfig *config, const PyConfigSpec *spec,
                            int use_sys);
static void initconfig_free_wstr(wchar_t *member);
static void initconfig_free_wstr_list(PyWideStringList *list);
static void initconfig_free_config(const PyConfig *config);


/* --- Command line options --------------------------------------- */

/* Short usage message (with %s for argv0) */
static const char usage_line[] =
"usage: %ls [option] ... [-c cmd | -m mod | file | -] [arg] ...\n";

/* Long help message */
/* Lines sorted by option name; keep in sync with usage_envvars* below */
static const char usage_help[] = "\
Options (and corresponding environment variables):\n\
-b     : issue warnings about converting bytes/bytearray to str and comparing\n\
         bytes/bytearray with str or bytes with int. (-bb: issue errors)\n\
         deprecated since 3.15 and will become no-op in 3.17.\n\
-B     : don't write .pyc files on import; also PYTHONDONTWRITEBYTECODE=x\n\
-c cmd : program passed in as string (terminates option list)\n\
-d     : turn on parser debugging output (for experts only, only works on\n\
         debug builds); also PYTHONDEBUG=x\n\
-E     : ignore PYTHON* environment variables (such as PYTHONPATH)\n\
-h     : print this help message and exit (also -? or --help)\n\
-i     : inspect interactively after running script; forces a prompt even\n\
         if stdin does not appear to be a terminal; also PYTHONINSPECT=x\n\
-I     : isolate Python from the user's environment (implies -E, -P and -s)\n\
-m mod : run library module as a script (terminates option list)\n\
-O     : remove assert and __debug__-dependent statements; add .opt-1 before\n\
         .pyc extension; also PYTHONOPTIMIZE=x\n\
-OO    : do -O changes and also discard docstrings; add .opt-2 before\n\
         .pyc extension\n\
-P     : don't prepend a potentially unsafe path to sys.path; also\n\
         PYTHONSAFEPATH\n\
-q     : don't print version and copyright messages on interactive startup\n\
-s     : don't add user site directory to sys.path; also PYTHONNOUSERSITE=x\n\
-S     : don't imply 'import site' on initialization\n\
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
--help-env: print help about Python environment variables and exit\n\
--help-xoptions: print help about implementation-specific -X options and exit\n\
--help-all: print complete help information and exit\n\
\n\
Arguments:\n\
file   : program read from script file\n\
-      : program read from stdin (default; interactive mode if a tty)\n\
arg ...: arguments passed to program in sys.argv[1:]\n\
";

static const char usage_xoptions[] = "\
The following implementation-specific options are available:\n\
-X cpu_count=N: override the return value of os.cpu_count();\n\
         -X cpu_count=default cancels overriding; also PYTHON_CPU_COUNT\n\
-X dev : enable Python Development Mode; also PYTHONDEVMODE\n\
-X faulthandler: dump the Python traceback on fatal errors;\n\
         also PYTHONFAULTHANDLER\n\
-X frozen_modules=[on|off]: whether to use frozen modules; the default is \"on\"\n\
         for installed Python and \"off\" for a local build;\n\
         also PYTHON_FROZEN_MODULES\n\
"
#ifdef Py_GIL_DISABLED
"-X gil=[0|1]: enable (1) or disable (0) the GIL; also PYTHON_GIL\n"
#endif
"\
-X importtime[=2]: show how long each import takes; use -X importtime=2 to\n\
         log imports of already-loaded modules; also PYTHONPROFILEIMPORTTIME\n\
-X int_max_str_digits=N: limit the size of int<->str conversions;\n\
         0 disables the limit; also PYTHONINTMAXSTRDIGITS\n\
-X no_debug_ranges: don't include extra location information in code objects;\n\
         also PYTHONNODEBUGRANGES\n\
-X perf: support the Linux \"perf\" profiler; also PYTHONPERFSUPPORT=1\n\
-X perf_jit: support the Linux \"perf\" profiler with DWARF support;\n\
         also PYTHON_PERF_JIT_SUPPORT=1\n\
-X disable-remote-debug: disable remote debugging; also PYTHON_DISABLE_REMOTE_DEBUG\n\
"
#ifdef Py_DEBUG
"-X presite=MOD: import this module before site; also PYTHON_PRESITE\n"
#endif
"\
-X pycache_prefix=PATH: write .pyc files to a parallel tree instead of to the\n\
         code tree; also PYTHONPYCACHEPREFIX\n\
"
#ifdef Py_STATS
"-X pystats: enable pystats collection at startup; also PYTHONSTATS\n"
#endif
"\
-X showrefcount: output the total reference count and number of used\n\
         memory blocks when the program finishes or after each statement in\n\
         the interactive interpreter; only works on debug builds\n"
#ifdef Py_GIL_DISABLED
"-X tlbc=[0|1]: enable (1) or disable (0) thread-local bytecode. Also\n\
         PYTHON_TLBC\n"
#endif
"\
-X thread_inherit_context=[0|1]: enable (1) or disable (0) threads inheriting\n\
         context vars by default; enabled by default in the free-threaded\n\
         build and disabled otherwise; also PYTHON_THREAD_INHERIT_CONTEXT\n\
-X context_aware_warnings=[0|1]: if true (1) then the warnings module will\n\
         use a context variables; if false (0) then the warnings module will\n\
         use module globals, which is not concurrent-safe; set to true for\n\
         free-threaded builds and false otherwise; also\n\
         PYTHON_CONTEXT_AWARE_WARNINGS\n\
-X tracemalloc[=N]: trace Python memory allocations; N sets a traceback limit\n \
         of N frames (default: 1); also PYTHONTRACEMALLOC=N\n\
-X utf8[=0|1]: enable (1) or disable (0) UTF-8 mode; also PYTHONUTF8\n\
-X warn_default_encoding: enable opt-in EncodingWarning for 'encoding=None';\n\
         also PYTHONWARNDEFAULTENCODING\
";

/* Envvars that don't have equivalent command-line options are listed first */
static const char usage_envvars[] =
"Environment variables that change behavior:\n"
"PYTHONSTARTUP   : file executed on interactive startup (no default)\n"
"PYTHONPATH      : '%lc'-separated list of directories prefixed to the\n"
"                  default module search path.  The result is sys.path.\n"
"PYTHONHOME      : alternate <prefix> directory (or <prefix>%lc<exec_prefix>).\n"
"                  The default module search path uses %s.\n"
"PYTHONPLATLIBDIR: override sys.platlibdir\n"
"PYTHONCASEOK    : ignore case in 'import' statements (Windows)\n"
"PYTHONIOENCODING: encoding[:errors] used for stdin/stdout/stderr\n"
"PYTHONHASHSEED  : if this variable is set to 'random', a random value is used\n"
"                  to seed the hashes of str and bytes objects.  It can also be\n"
"                  set to an integer in the range [0,4294967295] to get hash\n"
"                  values with a predictable seed.\n"
"PYTHONMALLOC    : set the Python memory allocators and/or install debug hooks\n"
"                  on Python memory allocators.  Use PYTHONMALLOC=debug to\n"
"                  install debug hooks.\n"
"PYTHONMALLOCSTATS: print memory allocator statistics\n"
"PYTHONCOERCECLOCALE: if this variable is set to 0, it disables the locale\n"
"                  coercion behavior.  Use PYTHONCOERCECLOCALE=warn to request\n"
"                  display of locale coercion and locale compatibility warnings\n"
"                  on stderr.\n"
"PYTHONBREAKPOINT: if this variable is set to 0, it disables the default\n"
"                  debugger.  It can be set to the callable of your debugger of\n"
"                  choice.\n"
"PYTHON_COLORS   : if this variable is set to 1, the interpreter will colorize\n"
"                  various kinds of output.  Setting it to 0 deactivates\n"
"                  this behavior.\n"
"PYTHON_HISTORY  : the location of a .python_history file.\n"
"PYTHONASYNCIODEBUG: enable asyncio debug mode\n"
#ifdef Py_TRACE_REFS
"PYTHONDUMPREFS  : dump objects and reference counts still alive after shutdown\n"
"PYTHONDUMPREFSFILE: dump objects and reference counts to the specified file\n"
#endif
#ifdef __APPLE__
"PYTHONEXECUTABLE: set sys.argv[0] to this value (macOS only)\n"
#endif
#ifdef MS_WINDOWS
"PYTHONLEGACYWINDOWSFSENCODING: use legacy \"mbcs\" encoding for file system\n"
"PYTHONLEGACYWINDOWSSTDIO: use legacy Windows stdio\n"
#endif
"PYTHONUSERBASE  : defines the user base directory (site.USER_BASE)\n"
"PYTHON_BASIC_REPL: use the traditional parser-based REPL\n"
"\n"
"These variables have equivalent command-line options (see --help for details):\n"
"PYTHON_CPU_COUNT: override the return value of os.cpu_count() (-X cpu_count)\n"
"PYTHONDEBUG     : enable parser debug mode (-d)\n"
"PYTHONDEVMODE   : enable Python Development Mode (-X dev)\n"
"PYTHONDONTWRITEBYTECODE: don't write .pyc files (-B)\n"
"PYTHONFAULTHANDLER: dump the Python traceback on fatal errors (-X faulthandler)\n"
"PYTHON_FROZEN_MODULES: whether to use frozen modules; the default is \"on\"\n"
"                  for installed Python and \"off\" for a local build\n"
"                  (-X frozen_modules)\n"
#ifdef Py_GIL_DISABLED
"PYTHON_GIL      : when set to 0, disables the GIL (-X gil)\n"
#endif
"PYTHONINSPECT   : inspect interactively after running script (-i)\n"
"PYTHONINTMAXSTRDIGITS: limit the size of int<->str conversions;\n"
"                  0 disables the limit (-X int_max_str_digits=N)\n"
"PYTHONNODEBUGRANGES: don't include extra location information in code objects\n"
"                  (-X no_debug_ranges)\n"
"PYTHONNOUSERSITE: disable user site directory (-s)\n"
"PYTHONOPTIMIZE  : enable level 1 optimizations (-O)\n"
"PYTHONPERFSUPPORT: support the Linux \"perf\" profiler (-X perf)\n"
"PYTHON_PERF_JIT_SUPPORT: enable Linux \"perf\" profiler support with JIT\n"
"                  (-X perf_jit)\n"
#ifdef Py_DEBUG
"PYTHON_PRESITE: import this module before site (-X presite)\n"
#endif
"PYTHONPROFILEIMPORTTIME: show how long each import takes (-X importtime)\n"
"PYTHONPYCACHEPREFIX: root directory for bytecode cache (pyc) files\n"
"                  (-X pycache_prefix)\n"
"PYTHONSAFEPATH  : don't prepend a potentially unsafe path to sys.path.\n"
#ifdef Py_STATS
"PYTHONSTATS     : turns on statistics gathering (-X pystats)\n"
#endif
#ifdef Py_GIL_DISABLED
"PYTHON_TLBC     : when set to 0, disables thread-local bytecode (-X tlbc)\n"
#endif
"PYTHON_THREAD_INHERIT_CONTEXT: if true (1), threads inherit context vars\n"
"                   (-X thread_inherit_context)\n"
"PYTHON_CONTEXT_AWARE_WARNINGS: if true (1), enable thread-safe warnings module\n"
"                   behaviour (-X context_aware_warnings)\n"
"PYTHONTRACEMALLOC: trace Python memory allocations (-X tracemalloc)\n"
"PYTHONUNBUFFERED: disable stdout/stderr buffering (-u)\n"
"PYTHONUTF8      : control the UTF-8 mode (-X utf8)\n"
"PYTHONVERBOSE   : trace import statements (-v)\n"
"PYTHONWARNDEFAULTENCODING: enable opt-in EncodingWarning for 'encoding=None'\n"
"                  (-X warn_default_encoding)\n"
"PYTHONWARNINGS  : warning control (-W)\n"
;

#if defined(MS_WINDOWS)
#  define PYTHONHOMEHELP "<prefix>\\python{major}{minor}"
#else
#  define PYTHONHOMEHELP "<prefix>/lib/pythonX.X"
#endif


/* --- Global configuration variables ----------------------------- */

/* UTF-8 mode (PEP 540): if equal to 1, use the UTF-8 encoding, and change
   stdin and stdout error handler to "surrogateescape". */
int Py_UTF8Mode = 0;
int Py_DebugFlag = 0; /* Needed by parser.c */
int Py_VerboseFlag = 0; /* Needed by import.c */
int Py_QuietFlag = 0; /* Needed by sysmodule.c */
int Py_InteractiveFlag = 0; /* Previously, was used by Py_FdIsInteractive() */
int Py_InspectFlag = 0; /* Needed to determine whether to exit at SystemExit */
int Py_OptimizeFlag = 0; /* Needed by compile.c */
int Py_NoSiteFlag = 0; /* Suppress 'import site' */
int Py_BytesWarningFlag = 0; /* Warn on str(bytes) and str(buffer) */
int Py_FrozenFlag = 0; /* Needed by getpath.c */
int Py_IgnoreEnvironmentFlag = 0; /* e.g. PYTHONPATH, PYTHONHOME */
int Py_DontWriteBytecodeFlag = 0; /* Suppress writing bytecode files (*.pyc) */
int Py_NoUserSiteDirectory = 0; /* for -s and site.py */
int Py_UnbufferedStdioFlag = 0; /* Unbuffered binary std{in,out,err} */
int Py_HashRandomizationFlag = 0; /* for -R and PYTHONHASHSEED */
int Py_IsolatedFlag = 0; /* for -I, isolate from user's env */
#ifdef MS_WINDOWS
int Py_LegacyWindowsFSEncodingFlag = 0; /* Uses mbcs instead of utf-8 */
int Py_LegacyWindowsStdioFlag = 0; /* Uses FileIO instead of WindowsConsoleIO */
#endif


static PyObject *
_Py_GetGlobalVariablesAsDict(void)
{
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
    PyObject *dict, *obj;

    dict = PyDict_New();
    if (dict == NULL) {
        return NULL;
    }

#define SET_ITEM(KEY, EXPR) \
        do { \
            obj = (EXPR); \
            if (obj == NULL) { \
                return NULL; \
            } \
            int res = PyDict_SetItemString(dict, (KEY), obj); \
            Py_DECREF(obj); \
            if (res < 0) { \
                goto fail; \
            } \
        } while (0)
#define SET_ITEM_INT(VAR) \
    SET_ITEM(#VAR, PyLong_FromLong(VAR))
#define FROM_STRING(STR) \
    ((STR != NULL) ? \
        PyUnicode_FromString(STR) \
        : Py_NewRef(Py_None))
#define SET_ITEM_STR(VAR) \
    SET_ITEM(#VAR, FROM_STRING(VAR))

    SET_ITEM_STR(Py_FileSystemDefaultEncoding);
    SET_ITEM_INT(Py_HasFileSystemDefaultEncoding);
    SET_ITEM_STR(Py_FileSystemDefaultEncodeErrors);
    SET_ITEM_INT(_Py_HasFileSystemDefaultEncodeErrors);

    SET_ITEM_INT(Py_UTF8Mode);
    SET_ITEM_INT(Py_DebugFlag);
    SET_ITEM_INT(Py_VerboseFlag);
    SET_ITEM_INT(Py_QuietFlag);
    SET_ITEM_INT(Py_InteractiveFlag);
    SET_ITEM_INT(Py_InspectFlag);

    SET_ITEM_INT(Py_OptimizeFlag);
    SET_ITEM_INT(Py_NoSiteFlag);
    SET_ITEM_INT(Py_BytesWarningFlag);
    SET_ITEM_INT(Py_FrozenFlag);
    SET_ITEM_INT(Py_IgnoreEnvironmentFlag);
    SET_ITEM_INT(Py_DontWriteBytecodeFlag);
    SET_ITEM_INT(Py_NoUserSiteDirectory);
    SET_ITEM_INT(Py_UnbufferedStdioFlag);
    SET_ITEM_INT(Py_HashRandomizationFlag);
    SET_ITEM_INT(Py_IsolatedFlag);

#ifdef MS_WINDOWS
    SET_ITEM_INT(Py_LegacyWindowsFSEncodingFlag);
    SET_ITEM_INT(Py_LegacyWindowsStdioFlag);
#endif

    return dict;

fail:
    Py_DECREF(dict);
    return NULL;

#undef FROM_STRING
#undef SET_ITEM
#undef SET_ITEM_INT
#undef SET_ITEM_STR
_Py_COMP_DIAG_POP
}

char*
Py_GETENV(const char *name)
{
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
    if (Py_IgnoreEnvironmentFlag) {
        return NULL;
    }
    return getenv(name);
_Py_COMP_DIAG_POP
}

/* --- PyStatus ----------------------------------------------- */

PyStatus PyStatus_Ok(void)
{ return _PyStatus_OK(); }

PyStatus PyStatus_Error(const char *err_msg)
{
    assert(err_msg != NULL);
    return (PyStatus){._type = _PyStatus_TYPE_ERROR,
                      .err_msg = err_msg};
}

PyStatus PyStatus_NoMemory(void)
{ return PyStatus_Error("memory allocation failed"); }

PyStatus PyStatus_Exit(int exitcode)
{ return _PyStatus_EXIT(exitcode); }


int PyStatus_IsError(PyStatus status)
{ return _PyStatus_IS_ERROR(status); }

int PyStatus_IsExit(PyStatus status)
{ return _PyStatus_IS_EXIT(status); }

int PyStatus_Exception(PyStatus status)
{ return _PyStatus_EXCEPTION(status); }

void
_PyErr_SetFromPyStatus(PyStatus status)
{
    if (!_PyStatus_IS_ERROR(status)) {
        PyErr_Format(PyExc_SystemError,
                     "_PyErr_SetFromPyStatus() status is not an error");
        return;
    }

    const char *err_msg = status.err_msg;
    if (err_msg == NULL || strlen(err_msg) == 0) {
        PyErr_Format(PyExc_SystemError,
                     "_PyErr_SetFromPyStatus() status has no error message");
        return;
    }

    if (strcmp(err_msg, _PyStatus_NO_MEMORY_ERRMSG) == 0) {
        PyErr_NoMemory();
        return;
    }

    const char *func = status.func;
    if (func) {
        PyErr_Format(PyExc_RuntimeError, "%s: %s", func, err_msg);
    }
    else {
        PyErr_Format(PyExc_RuntimeError, "%s", err_msg);
    }
}


/* --- PyWideStringList ------------------------------------------------ */

#ifndef NDEBUG
int
_PyWideStringList_CheckConsistency(const PyWideStringList *list)
{
    assert(list->length >= 0);
    if (list->length != 0) {
        assert(list->items != NULL);
    }
    for (Py_ssize_t i = 0; i < list->length; i++) {
        assert(list->items[i] != NULL);
    }
    return 1;
}
#endif   /* Py_DEBUG */


static void
_PyWideStringList_ClearEx(PyWideStringList *list,
                          bool use_default_allocator)
{
    assert(_PyWideStringList_CheckConsistency(list));
    for (Py_ssize_t i=0; i < list->length; i++) {
        if (use_default_allocator) {
            _PyMem_DefaultRawFree(list->items[i]);
        }
        else {
            PyMem_RawFree(list->items[i]);
        }
    }
    if (use_default_allocator) {
        _PyMem_DefaultRawFree(list->items);
    }
    else {
        PyMem_RawFree(list->items);
    }
    list->length = 0;
    list->items = NULL;
}

void
_PyWideStringList_Clear(PyWideStringList *list)
{
    _PyWideStringList_ClearEx(list, false);
}

static int
_PyWideStringList_CopyEx(PyWideStringList *list,
                         const PyWideStringList *list2,
                         bool use_default_allocator)
{
    assert(_PyWideStringList_CheckConsistency(list));
    assert(_PyWideStringList_CheckConsistency(list2));

    if (list2->length == 0) {
        _PyWideStringList_ClearEx(list, use_default_allocator);
        return 0;
    }

    PyWideStringList copy = _PyWideStringList_INIT;

    size_t size = list2->length * sizeof(list2->items[0]);
    if (use_default_allocator) {
        copy.items = _PyMem_DefaultRawMalloc(size);
    }
    else {
        copy.items = PyMem_RawMalloc(size);
    }
    if (copy.items == NULL) {
        return -1;
    }

    for (Py_ssize_t i=0; i < list2->length; i++) {
        wchar_t *item;
        if (use_default_allocator) {
            item = _PyMem_DefaultRawWcsdup(list2->items[i]);
        }
        else {
            item = _PyMem_RawWcsdup(list2->items[i]);
        }
        if (item == NULL) {
            _PyWideStringList_ClearEx(&copy, use_default_allocator);
            return -1;
        }
        copy.items[i] = item;
        copy.length = i + 1;
    }

    _PyWideStringList_ClearEx(list, use_default_allocator);
    *list = copy;
    return 0;
}

int
_PyWideStringList_Copy(PyWideStringList *list, const PyWideStringList *list2)
{
    return _PyWideStringList_CopyEx(list, list2, false);
}

PyStatus
PyWideStringList_Insert(PyWideStringList *list,
                        Py_ssize_t index, const wchar_t *item)
{
    Py_ssize_t len = list->length;
    if (len == PY_SSIZE_T_MAX) {
        /* length+1 would overflow */
        return _PyStatus_NO_MEMORY();
    }
    if (index < 0) {
        return _PyStatus_ERR("PyWideStringList_Insert index must be >= 0");
    }
    if (index > len) {
        index = len;
    }

    wchar_t *item2 = _PyMem_RawWcsdup(item);
    if (item2 == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    size_t size = (len + 1) * sizeof(list->items[0]);
    wchar_t **items2 = (wchar_t **)PyMem_RawRealloc(list->items, size);
    if (items2 == NULL) {
        PyMem_RawFree(item2);
        return _PyStatus_NO_MEMORY();
    }

    if (index < len) {
        memmove(&items2[index + 1],
                &items2[index],
                (len - index) * sizeof(items2[0]));
    }

    items2[index] = item2;
    list->items = items2;
    list->length++;
    return _PyStatus_OK();
}


PyStatus
PyWideStringList_Append(PyWideStringList *list, const wchar_t *item)
{
    return PyWideStringList_Insert(list, list->length, item);
}


PyStatus
_PyWideStringList_Extend(PyWideStringList *list, const PyWideStringList *list2)
{
    for (Py_ssize_t i = 0; i < list2->length; i++) {
        PyStatus status = PyWideStringList_Append(list, list2->items[i]);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
    return _PyStatus_OK();
}


static int
_PyWideStringList_Find(PyWideStringList *list, const wchar_t *item)
{
    for (Py_ssize_t i = 0; i < list->length; i++) {
        if (wcscmp(list->items[i], item) == 0) {
            return 1;
        }
    }
    return 0;
}


PyObject*
_PyWideStringList_AsList(const PyWideStringList *list)
{
    assert(_PyWideStringList_CheckConsistency(list));

    PyObject *pylist = PyList_New(list->length);
    if (pylist == NULL) {
        return NULL;
    }

    for (Py_ssize_t i = 0; i < list->length; i++) {
        PyObject *item = PyUnicode_FromWideChar(list->items[i], -1);
        if (item == NULL) {
            Py_DECREF(pylist);
            return NULL;
        }
        PyList_SET_ITEM(pylist, i, item);
    }
    return pylist;
}


static PyObject*
_PyWideStringList_AsTuple(const PyWideStringList *list)
{
    assert(_PyWideStringList_CheckConsistency(list));

    PyObject *tuple = PyTuple_New(list->length);
    if (tuple == NULL) {
        return NULL;
    }

    for (Py_ssize_t i = 0; i < list->length; i++) {
        PyObject *item = PyUnicode_FromWideChar(list->items[i], -1);
        if (item == NULL) {
            Py_DECREF(tuple);
            return NULL;
        }
        PyTuple_SET_ITEM(tuple, i, item);
    }
    return tuple;
}


/* --- Py_GetArgcArgv() ------------------------------------------- */

void
_Py_ClearArgcArgv(void)
{
    _PyWideStringList_ClearEx(&_PyRuntime.orig_argv, true);
}


static int
_Py_SetArgcArgv(Py_ssize_t argc, wchar_t * const *argv)
{
    const PyWideStringList argv_list = {.length = argc, .items = (wchar_t **)argv};

    // XXX _PyRuntime.orig_argv only gets cleared by Py_Main(),
    // so it currently leaks for embedders.
    return _PyWideStringList_CopyEx(&_PyRuntime.orig_argv, &argv_list, true);
}


// _PyConfig_Write() calls _Py_SetArgcArgv() with PyConfig.orig_argv.
void
Py_GetArgcArgv(int *argc, wchar_t ***argv)
{
    *argc = (int)_PyRuntime.orig_argv.length;
    *argv = _PyRuntime.orig_argv.items;
}


/* --- PyConfig ---------------------------------------------- */

#define MAX_HASH_SEED 4294967295UL


#ifndef NDEBUG
static int
config_check_consistency(const PyConfig *config)
{
    /* Check config consistency */
    assert(config->isolated >= 0);
    assert(config->use_environment >= 0);
    assert(config->dev_mode >= 0);
    assert(config->install_signal_handlers >= 0);
    assert(config->use_hash_seed >= 0);
    assert(config->hash_seed <= MAX_HASH_SEED);
    assert(config->faulthandler >= 0);
    assert(config->tracemalloc >= 0);
    assert(config->import_time >= 0);
    assert(config->code_debug_ranges >= 0);
    assert(config->show_ref_count >= 0);
    assert(config->dump_refs >= 0);
    assert(config->malloc_stats >= 0);
    assert(config->site_import >= 0);
    assert(config->bytes_warning >= 0);
    assert(config->warn_default_encoding >= 0);
    assert(config->inspect >= 0);
    assert(config->interactive >= 0);
    assert(config->optimization_level >= 0);
    assert(config->parser_debug >= 0);
    assert(config->write_bytecode >= 0);
    assert(config->verbose >= 0);
    assert(config->quiet >= 0);
    assert(config->user_site_directory >= 0);
    assert(config->parse_argv >= 0);
    assert(config->configure_c_stdio >= 0);
    assert(config->buffered_stdio >= 0);
    assert(_PyWideStringList_CheckConsistency(&config->orig_argv));
    assert(_PyWideStringList_CheckConsistency(&config->argv));
    /* sys.argv must be non-empty: empty argv is replaced with [''] */
    assert(config->argv.length >= 1);
    assert(_PyWideStringList_CheckConsistency(&config->xoptions));
    assert(_PyWideStringList_CheckConsistency(&config->warnoptions));
    assert(_PyWideStringList_CheckConsistency(&config->module_search_paths));
    assert(config->module_search_paths_set >= 0);
    assert(config->filesystem_encoding != NULL);
    assert(config->filesystem_errors != NULL);
    assert(config->stdio_encoding != NULL);
    assert(config->stdio_errors != NULL);
#ifdef MS_WINDOWS
    assert(config->legacy_windows_stdio >= 0);
#endif
    /* -c and -m options are exclusive */
    assert(!(config->run_command != NULL && config->run_module != NULL));
    assert(config->check_hash_pycs_mode != NULL);
    assert(config->_install_importlib >= 0);
    assert(config->pathconfig_warnings >= 0);
    assert(config->_is_python_build >= 0);
    assert(config->safe_path >= 0);
    assert(config->int_max_str_digits >= 0);
    // cpu_count can be -1 if the user doesn't override it.
    assert(config->cpu_count != 0);
    // config->use_frozen_modules is initialized later
    // by _PyConfig_InitImportConfig().
    assert(config->thread_inherit_context >= 0);
    assert(config->context_aware_warnings >= 0);
#ifdef __APPLE__
    assert(config->use_system_logger >= 0);
#endif
#ifdef Py_STATS
    assert(config->_pystats >= 0);
#endif
    return 1;
}
#endif


/* Free memory allocated in config, but don't clear all attributes */
void
PyConfig_Clear(PyConfig *config)
{
#define CLEAR(ATTR) \
    do { \
        PyMem_RawFree(ATTR); \
        ATTR = NULL; \
    } while (0)

    CLEAR(config->pycache_prefix);
    CLEAR(config->pythonpath_env);
    CLEAR(config->home);
    CLEAR(config->program_name);

    _PyWideStringList_Clear(&config->argv);
    _PyWideStringList_Clear(&config->warnoptions);
    _PyWideStringList_Clear(&config->xoptions);
    _PyWideStringList_Clear(&config->module_search_paths);
    config->module_search_paths_set = 0;
    CLEAR(config->stdlib_dir);

    CLEAR(config->executable);
    CLEAR(config->base_executable);
    CLEAR(config->prefix);
    CLEAR(config->base_prefix);
    CLEAR(config->exec_prefix);
    CLEAR(config->base_exec_prefix);
    CLEAR(config->platlibdir);
    CLEAR(config->sys_path_0);

    CLEAR(config->filesystem_encoding);
    CLEAR(config->filesystem_errors);
    CLEAR(config->stdio_encoding);
    CLEAR(config->stdio_errors);
    CLEAR(config->run_command);
    CLEAR(config->run_module);
    CLEAR(config->run_filename);
    CLEAR(config->check_hash_pycs_mode);
#ifdef Py_DEBUG
    CLEAR(config->run_presite);
#endif

    _PyWideStringList_Clear(&config->orig_argv);
#undef CLEAR
}


void
_PyConfig_InitCompatConfig(PyConfig *config)
{
    memset(config, 0, sizeof(*config));

    config->_config_init = (int)_PyConfig_INIT_COMPAT;
    config->import_time = -1;
    config->isolated = -1;
    config->use_environment = -1;
    config->dev_mode = -1;
    config->install_signal_handlers = 1;
    config->use_hash_seed = -1;
    config->faulthandler = -1;
    config->tracemalloc = -1;
    config->perf_profiling = -1;
    config->remote_debug = -1;
    config->module_search_paths_set = 0;
    config->parse_argv = 0;
    config->site_import = -1;
    config->bytes_warning = -1;
    config->warn_default_encoding = 0;
    config->inspect = -1;
    config->interactive = -1;
    config->optimization_level = -1;
    config->parser_debug= -1;
    config->write_bytecode = -1;
    config->verbose = -1;
    config->quiet = -1;
    config->user_site_directory = -1;
    config->configure_c_stdio = 0;
    config->buffered_stdio = -1;
    config->_install_importlib = 1;
    config->check_hash_pycs_mode = NULL;
    config->pathconfig_warnings = -1;
    config->_init_main = 1;
#ifdef MS_WINDOWS
    config->legacy_windows_stdio = -1;
#endif
#ifdef Py_DEBUG
    config->use_frozen_modules = 0;
#else
    config->use_frozen_modules = 1;
#endif
    config->safe_path = 0;
    config->int_max_str_digits = -1;
    config->_is_python_build = 0;
    config->code_debug_ranges = 1;
    config->cpu_count = -1;
#ifdef Py_GIL_DISABLED
    config->thread_inherit_context = 1;
    config->context_aware_warnings = 1;
#else
    config->thread_inherit_context = 0;
    config->context_aware_warnings = 0;
#endif
#ifdef __APPLE__
    config->use_system_logger = USE_SYSTEM_LOGGER_DEFAULT;
#endif
#ifdef Py_GIL_DISABLED
    config->enable_gil = _PyConfig_GIL_DEFAULT;
    config->tlbc_enabled = 1;
#endif
}


static void
config_init_defaults(PyConfig *config)
{
    _PyConfig_InitCompatConfig(config);

    config->isolated = 0;
    config->use_environment = 1;
    config->site_import = 1;
    config->bytes_warning = 0;
    config->inspect = 0;
    config->interactive = 0;
    config->optimization_level = 0;
    config->parser_debug= 0;
    config->write_bytecode = 1;
    config->verbose = 0;
    config->quiet = 0;
    config->user_site_directory = 1;
    config->buffered_stdio = 1;
    config->pathconfig_warnings = 1;
#ifdef MS_WINDOWS
    config->legacy_windows_stdio = 0;
#endif
#ifdef Py_GIL_DISABLED
    config->thread_inherit_context = 1;
    config->context_aware_warnings = 1;
#else
    config->thread_inherit_context = 0;
    config->context_aware_warnings = 0;
#endif
#ifdef __APPLE__
    config->use_system_logger = USE_SYSTEM_LOGGER_DEFAULT;
#endif
}


void
PyConfig_InitPythonConfig(PyConfig *config)
{
    config_init_defaults(config);

    config->_config_init = (int)_PyConfig_INIT_PYTHON;
    config->configure_c_stdio = 1;
    config->parse_argv = 1;
}


void
PyConfig_InitIsolatedConfig(PyConfig *config)
{
    config_init_defaults(config);

    config->_config_init = (int)_PyConfig_INIT_ISOLATED;
    config->isolated = 1;
    config->use_environment = 0;
    config->user_site_directory = 0;
    config->dev_mode = 0;
    config->install_signal_handlers = 0;
    config->use_hash_seed = 0;
    config->tracemalloc = 0;
    config->perf_profiling = 0;
    config->int_max_str_digits = _PY_LONG_DEFAULT_MAX_STR_DIGITS;
    config->safe_path = 1;
    config->pathconfig_warnings = 0;
#ifdef Py_GIL_DISABLED
    config->thread_inherit_context = 1;
#else
    config->thread_inherit_context = 0;
#endif
#ifdef MS_WINDOWS
    config->legacy_windows_stdio = 0;
#endif
#ifdef __APPLE__
    config->use_system_logger = USE_SYSTEM_LOGGER_DEFAULT;
#endif
}


/* Copy str into *config_str (duplicate the string) */
PyStatus
PyConfig_SetString(PyConfig *config, wchar_t **config_str, const wchar_t *str)
{
    PyStatus status = _Py_PreInitializeFromConfig(config, NULL);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    wchar_t *str2;
    if (str != NULL) {
        str2 = _PyMem_RawWcsdup(str);
        if (str2 == NULL) {
            return _PyStatus_NO_MEMORY();
        }
    }
    else {
        str2 = NULL;
    }
    PyMem_RawFree(*config_str);
    *config_str = str2;
    return _PyStatus_OK();
}


static PyStatus
config_set_bytes_string(PyConfig *config, wchar_t **config_str,
                        const char *str, const char *decode_err_msg)
{
    PyStatus status = _Py_PreInitializeFromConfig(config, NULL);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    wchar_t *str2;
    if (str != NULL) {
        size_t len;
        str2 = Py_DecodeLocale(str, &len);
        if (str2 == NULL) {
            if (len == (size_t)-2) {
                return _PyStatus_ERR(decode_err_msg);
            }
            else {
                return  _PyStatus_NO_MEMORY();
            }
        }
    }
    else {
        str2 = NULL;
    }
    PyMem_RawFree(*config_str);
    *config_str = str2;
    return _PyStatus_OK();
}


#define CONFIG_SET_BYTES_STR(config, config_str, str, NAME) \
    config_set_bytes_string(config, config_str, str, "cannot decode " NAME)


/* Decode str using Py_DecodeLocale() and set the result into *config_str.
   Pre-initialize Python if needed to ensure that encodings are properly
   configured. */
PyStatus
PyConfig_SetBytesString(PyConfig *config, wchar_t **config_str,
                        const char *str)
{
    return CONFIG_SET_BYTES_STR(config, config_str, str, "string");
}


static inline void*
config_get_spec_member(const PyConfig *config, const PyConfigSpec *spec)
{
    return (char *)config + spec->offset;
}


static inline void*
preconfig_get_spec_member(const PyPreConfig *preconfig, const PyConfigSpec *spec)
{
    return (char *)preconfig + spec->offset;
}


PyStatus
_PyConfig_Copy(PyConfig *config, const PyConfig *config2)
{
    PyConfig_Clear(config);

    PyStatus status;
    const PyConfigSpec *spec = PYCONFIG_SPEC;
    for (; spec->name != NULL; spec++) {
        void *member = config_get_spec_member(config, spec);
        const void *member2 = config_get_spec_member((PyConfig*)config2, spec);
        switch (spec->type) {
        case PyConfig_MEMBER_INT:
        case PyConfig_MEMBER_UINT:
        case PyConfig_MEMBER_BOOL:
        {
            *(int*)member = *(int*)member2;
            break;
        }
        case PyConfig_MEMBER_ULONG:
        {
            *(unsigned long*)member = *(unsigned long*)member2;
            break;
        }
        case PyConfig_MEMBER_WSTR:
        case PyConfig_MEMBER_WSTR_OPT:
        {
            const wchar_t *str = *(const wchar_t**)member2;
            status = PyConfig_SetString(config, (wchar_t**)member, str);
            if (_PyStatus_EXCEPTION(status)) {
                return status;
            }
            break;
        }
        case PyConfig_MEMBER_WSTR_LIST:
        {
            if (_PyWideStringList_Copy((PyWideStringList*)member,
                                       (const PyWideStringList*)member2) < 0) {
                return _PyStatus_NO_MEMORY();
            }
            break;
        }
        default:
            Py_UNREACHABLE();
        }
    }
    return _PyStatus_OK();
}


PyObject *
_PyConfig_AsDict(const PyConfig *config)
{
    PyObject *dict = PyDict_New();
    if (dict == NULL) {
        return NULL;
    }

    const PyConfigSpec *spec = PYCONFIG_SPEC;
    for (; spec->name != NULL; spec++) {
        PyObject *obj = config_get(config, spec, 0);
        if (obj == NULL) {
            Py_DECREF(dict);
            return NULL;
        }

        int res = PyDict_SetItemString(dict, spec->name, obj);
        Py_DECREF(obj);
        if (res < 0) {
            Py_DECREF(dict);
            return NULL;
        }
    }
    return dict;
}


static void
config_dict_invalid_value(const char *name)
{
    PyErr_Format(PyExc_ValueError, "invalid config value: %s", name);
}


static int
config_dict_get_int(PyObject *dict, const char *name, int *result)
{
    PyObject *item = config_dict_get(dict, name);
    if (item == NULL) {
        return -1;
    }
    int value = PyLong_AsInt(item);
    Py_DECREF(item);
    if (value == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            config_dict_invalid_type(name);
        }
        else if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            config_dict_invalid_value(name);
        }
        return -1;
    }
    *result = value;
    return 0;
}


static int
config_dict_get_ulong(PyObject *dict, const char *name, unsigned long *result)
{
    PyObject *item = config_dict_get(dict, name);
    if (item == NULL) {
        return -1;
    }
    unsigned long value = PyLong_AsUnsignedLong(item);
    Py_DECREF(item);
    if (value == (unsigned long)-1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            config_dict_invalid_type(name);
        }
        else if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            config_dict_invalid_value(name);
        }
        return -1;
    }
    *result = value;
    return 0;
}


static int
config_dict_get_wstr(PyObject *dict, const char *name, PyConfig *config,
                     wchar_t **result)
{
    PyObject *item = config_dict_get(dict, name);
    if (item == NULL) {
        return -1;
    }

    PyStatus status;
    if (item == Py_None) {
        status = PyConfig_SetString(config, result, NULL);
    }
    else if (!PyUnicode_Check(item)) {
        config_dict_invalid_type(name);
        goto error;
    }
    else {
        wchar_t *wstr = PyUnicode_AsWideCharString(item, NULL);
        if (wstr == NULL) {
            goto error;
        }
        status = PyConfig_SetString(config, result, wstr);
        PyMem_Free(wstr);
    }
    if (_PyStatus_EXCEPTION(status)) {
        PyErr_NoMemory();
        goto error;
    }
    Py_DECREF(item);
    return 0;

error:
    Py_DECREF(item);
    return -1;
}


static int
config_dict_get_wstrlist(PyObject *dict, const char *name, PyConfig *config,
                         PyWideStringList *result)
{
    PyObject *list = config_dict_get(dict, name);
    if (list == NULL) {
        return -1;
    }

    int is_list = PyList_CheckExact(list);
    if (!is_list && !PyTuple_CheckExact(list)) {
        Py_DECREF(list);
        config_dict_invalid_type(name);
        return -1;
    }

    PyWideStringList wstrlist = _PyWideStringList_INIT;
    Py_ssize_t len = is_list ? PyList_GET_SIZE(list) : PyTuple_GET_SIZE(list);
    for (Py_ssize_t i=0; i < len; i++) {
        PyObject *item = is_list ? PyList_GET_ITEM(list, i) : PyTuple_GET_ITEM(list, i);

        if (item == Py_None) {
            config_dict_invalid_value(name);
            goto error;
        }
        else if (!PyUnicode_Check(item)) {
            config_dict_invalid_type(name);
            goto error;
        }
        wchar_t *wstr = PyUnicode_AsWideCharString(item, NULL);
        if (wstr == NULL) {
            goto error;
        }
        PyStatus status = PyWideStringList_Append(&wstrlist, wstr);
        PyMem_Free(wstr);
        if (_PyStatus_EXCEPTION(status)) {
            PyErr_NoMemory();
            goto error;
        }
    }

    if (_PyWideStringList_Copy(result, &wstrlist) < 0) {
        PyErr_NoMemory();
        goto error;
    }
    _PyWideStringList_Clear(&wstrlist);
    Py_DECREF(list);
    return 0;

error:
    _PyWideStringList_Clear(&wstrlist);
    Py_DECREF(list);
    return -1;
}


static int
config_dict_get_xoptions(PyObject *dict, const char *name, PyConfig *config,
                         PyWideStringList *result)
{
    PyObject *xoptions = config_dict_get(dict, name);
    if (xoptions == NULL) {
        return -1;
    }

    if (!PyDict_CheckExact(xoptions)) {
        Py_DECREF(xoptions);
        config_dict_invalid_type(name);
        return -1;
    }

    Py_ssize_t pos = 0;
    PyObject *key, *value;
    PyWideStringList wstrlist = _PyWideStringList_INIT;
    while (PyDict_Next(xoptions, &pos, &key, &value)) {
        PyObject *item;

        if (value != Py_True) {
            item = PyUnicode_FromFormat("%S=%S", key, value);
            if (item == NULL) {
                goto error;
            }
        }
        else {
            item = Py_NewRef(key);
        }

        wchar_t *wstr = PyUnicode_AsWideCharString(item, NULL);
        Py_DECREF(item);
        if (wstr == NULL) {
            goto error;
        }

        PyStatus status = PyWideStringList_Append(&wstrlist, wstr);
        PyMem_Free(wstr);
        if (_PyStatus_EXCEPTION(status)) {
            PyErr_NoMemory();
            goto error;
        }
    }

    if (_PyWideStringList_Copy(result, &wstrlist) < 0) {
        PyErr_NoMemory();
        goto error;
    }
    _PyWideStringList_Clear(&wstrlist);
    Py_DECREF(xoptions);
    return 0;

error:
    _PyWideStringList_Clear(&wstrlist);
    Py_DECREF(xoptions);
    return -1;
}


int
_PyConfig_FromDict(PyConfig *config, PyObject *dict)
{
    if (!PyDict_Check(dict)) {
        PyErr_SetString(PyExc_TypeError, "dict expected");
        return -1;
    }

    const PyConfigSpec *spec = PYCONFIG_SPEC;
    for (; spec->name != NULL; spec++) {
        char *member = (char *)config + spec->offset;
        switch (spec->type) {
        case PyConfig_MEMBER_INT:
        case PyConfig_MEMBER_UINT:
        case PyConfig_MEMBER_BOOL:
        {
            int value;
            if (config_dict_get_int(dict, spec->name, &value) < 0) {
                return -1;
            }
            if (spec->type == PyConfig_MEMBER_BOOL
                || spec->type == PyConfig_MEMBER_UINT)
            {
                if (value < 0) {
                    config_dict_invalid_value(spec->name);
                    return -1;
                }
            }
            *(int*)member = value;
            break;
        }
        case PyConfig_MEMBER_ULONG:
        {
            if (config_dict_get_ulong(dict, spec->name,
                                      (unsigned long*)member) < 0) {
                return -1;
            }
            break;
        }
        case PyConfig_MEMBER_WSTR:
        {
            wchar_t **wstr = (wchar_t**)member;
            if (config_dict_get_wstr(dict, spec->name, config, wstr) < 0) {
                return -1;
            }
            if (*wstr == NULL) {
                config_dict_invalid_value(spec->name);
                return -1;
            }
            break;
        }
        case PyConfig_MEMBER_WSTR_OPT:
        {
            wchar_t **wstr = (wchar_t**)member;
            if (config_dict_get_wstr(dict, spec->name, config, wstr) < 0) {
                return -1;
            }
            break;
        }
        case PyConfig_MEMBER_WSTR_LIST:
        {
            if (strcmp(spec->name, "xoptions") == 0) {
                if (config_dict_get_xoptions(dict, spec->name, config,
                                             (PyWideStringList*)member) < 0) {
                    return -1;
                }
            }
            else {
                if (config_dict_get_wstrlist(dict, spec->name, config,
                                             (PyWideStringList*)member) < 0) {
                    return -1;
                }
            }
            break;
        }
        default:
            Py_UNREACHABLE();
        }
    }

    if (!(config->_config_init == _PyConfig_INIT_COMPAT
          || config->_config_init == _PyConfig_INIT_PYTHON
          || config->_config_init == _PyConfig_INIT_ISOLATED))
    {
        config_dict_invalid_value("_config_init");
        return -1;
    }

    if (config->hash_seed > MAX_HASH_SEED) {
        config_dict_invalid_value("hash_seed");
        return -1;
    }
    return 0;
}


static const char*
config_get_env(const PyConfig *config, const char *name)
{
    return _Py_GetEnv(config->use_environment, name);
}


/* Get a copy of the environment variable as wchar_t*.
   Return 0 on success, but *dest can be NULL.
   Return -1 on memory allocation failure. Return -2 on decoding error. */
static PyStatus
config_get_env_dup(PyConfig *config,
                   wchar_t **dest,
                   wchar_t *wname, char *name,
                   const char *decode_err_msg)
{
    assert(*dest == NULL);
    assert(config->use_environment >= 0);

    if (!config->use_environment) {
        *dest = NULL;
        return _PyStatus_OK();
    }

#ifdef MS_WINDOWS
    const wchar_t *var = _wgetenv(wname);
    if (!var || var[0] == '\0') {
        *dest = NULL;
        return _PyStatus_OK();
    }

    return PyConfig_SetString(config, dest, var);
#else
    const char *var = getenv(name);
    if (!var || var[0] == '\0') {
        *dest = NULL;
        return _PyStatus_OK();
    }

    return config_set_bytes_string(config, dest, var, decode_err_msg);
#endif
}


#define CONFIG_GET_ENV_DUP(CONFIG, DEST, WNAME, NAME) \
    config_get_env_dup(CONFIG, DEST, WNAME, NAME, "cannot decode " NAME)


static void
config_get_global_vars(PyConfig *config)
{
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
    if (config->_config_init != _PyConfig_INIT_COMPAT) {
        /* Python and Isolated configuration ignore global variables */
        return;
    }

#define COPY_FLAG(ATTR, VALUE) \
        if (config->ATTR == -1) { \
            config->ATTR = VALUE; \
        }
#define COPY_NOT_FLAG(ATTR, VALUE) \
        if (config->ATTR == -1) { \
            config->ATTR = !(VALUE); \
        }

    COPY_FLAG(isolated, Py_IsolatedFlag);
    COPY_NOT_FLAG(use_environment, Py_IgnoreEnvironmentFlag);
    COPY_FLAG(bytes_warning, Py_BytesWarningFlag);
    COPY_FLAG(inspect, Py_InspectFlag);
    COPY_FLAG(interactive, Py_InteractiveFlag);
    COPY_FLAG(optimization_level, Py_OptimizeFlag);
    COPY_FLAG(parser_debug, Py_DebugFlag);
    COPY_FLAG(verbose, Py_VerboseFlag);
    COPY_FLAG(quiet, Py_QuietFlag);
#ifdef MS_WINDOWS
    COPY_FLAG(legacy_windows_stdio, Py_LegacyWindowsStdioFlag);
#endif
    COPY_NOT_FLAG(pathconfig_warnings, Py_FrozenFlag);

    COPY_NOT_FLAG(buffered_stdio, Py_UnbufferedStdioFlag);
    COPY_NOT_FLAG(site_import, Py_NoSiteFlag);
    COPY_NOT_FLAG(write_bytecode, Py_DontWriteBytecodeFlag);
    COPY_NOT_FLAG(user_site_directory, Py_NoUserSiteDirectory);

#undef COPY_FLAG
#undef COPY_NOT_FLAG
_Py_COMP_DIAG_POP
}


/* Set Py_xxx global configuration variables from 'config' configuration. */
static void
config_set_global_vars(const PyConfig *config)
{
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
#define COPY_FLAG(ATTR, VAR) \
        if (config->ATTR != -1) { \
            VAR = config->ATTR; \
        }
#define COPY_NOT_FLAG(ATTR, VAR) \
        if (config->ATTR != -1) { \
            VAR = !config->ATTR; \
        }

    COPY_FLAG(isolated, Py_IsolatedFlag);
    COPY_NOT_FLAG(use_environment, Py_IgnoreEnvironmentFlag);
    COPY_FLAG(bytes_warning, Py_BytesWarningFlag);
    COPY_FLAG(inspect, Py_InspectFlag);
    COPY_FLAG(interactive, Py_InteractiveFlag);
    COPY_FLAG(optimization_level, Py_OptimizeFlag);
    COPY_FLAG(parser_debug, Py_DebugFlag);
    COPY_FLAG(verbose, Py_VerboseFlag);
    COPY_FLAG(quiet, Py_QuietFlag);
#ifdef MS_WINDOWS
    COPY_FLAG(legacy_windows_stdio, Py_LegacyWindowsStdioFlag);
#endif
    COPY_NOT_FLAG(pathconfig_warnings, Py_FrozenFlag);

    COPY_NOT_FLAG(buffered_stdio, Py_UnbufferedStdioFlag);
    COPY_NOT_FLAG(site_import, Py_NoSiteFlag);
    COPY_NOT_FLAG(write_bytecode, Py_DontWriteBytecodeFlag);
    COPY_NOT_FLAG(user_site_directory, Py_NoUserSiteDirectory);

    /* Random or non-zero hash seed */
    Py_HashRandomizationFlag = (config->use_hash_seed == 0 ||
                                config->hash_seed != 0);

#undef COPY_FLAG
#undef COPY_NOT_FLAG
_Py_COMP_DIAG_POP
}


static const wchar_t*
config_get_xoption(const PyConfig *config, wchar_t *name)
{
    return _Py_get_xoption(&config->xoptions, name);
}

static const wchar_t*
config_get_xoption_value(const PyConfig *config, wchar_t *name)
{
    const wchar_t *xoption = config_get_xoption(config, name);
    if (xoption == NULL) {
        return NULL;
    }
    const wchar_t *sep = wcschr(xoption, L'=');
    return sep ? sep + 1 : L"";
}


static PyStatus
config_init_hash_seed(PyConfig *config)
{
    static_assert(sizeof(_Py_HashSecret_t) == sizeof(_Py_HashSecret.uc),
                  "_Py_HashSecret_t has wrong size");

    const char *seed_text = config_get_env(config, "PYTHONHASHSEED");

    /* Convert a text seed to a numeric one */
    if (seed_text && strcmp(seed_text, "random") != 0) {
        const char *endptr = seed_text;
        unsigned long seed;
        errno = 0;
        seed = strtoul(seed_text, (char **)&endptr, 10);
        if (*endptr != '\0'
            || seed > MAX_HASH_SEED
            || (errno == ERANGE && seed == ULONG_MAX))
        {
            return _PyStatus_ERR("PYTHONHASHSEED must be \"random\" "
                                "or an integer in range [0; 4294967295]");
        }
        /* Use a specific hash */
        config->use_hash_seed = 1;
        config->hash_seed = seed;
    }
    else {
        /* Use a random hash */
        config->use_hash_seed = 0;
        config->hash_seed = 0;
    }
    return _PyStatus_OK();
}


static int
config_wstr_to_int(const wchar_t *wstr, int *result)
{
    const wchar_t *endptr = wstr;
    errno = 0;
    long value = wcstol(wstr, (wchar_t **)&endptr, 10);
    if (*endptr != '\0' || errno == ERANGE) {
        return -1;
    }
    if (value < INT_MIN || value > INT_MAX) {
        return -1;
    }

    *result = (int)value;
    return 0;
}

static PyStatus
config_read_gil(PyConfig *config, size_t len, wchar_t first_char)
{
    if (len == 1 && first_char == L'0') {
#ifdef Py_GIL_DISABLED
        config->enable_gil = _PyConfig_GIL_DISABLE;
#else
        return _PyStatus_ERR("Disabling the GIL is not supported by this build");
#endif
    }
    else if (len == 1 && first_char == L'1') {
#ifdef Py_GIL_DISABLED
        config->enable_gil = _PyConfig_GIL_ENABLE;
#else
        return _PyStatus_OK();
#endif
    }
    else {
        return _PyStatus_ERR("PYTHON_GIL / -X gil must be \"0\" or \"1\"");
    }
    return _PyStatus_OK();
}

static PyStatus
config_read_env_vars(PyConfig *config)
{
    PyStatus status;
    int use_env = config->use_environment;

    /* Get environment variables */
    _Py_get_env_flag(use_env, &config->parser_debug, "PYTHONDEBUG");
    _Py_get_env_flag(use_env, &config->verbose, "PYTHONVERBOSE");
    _Py_get_env_flag(use_env, &config->optimization_level, "PYTHONOPTIMIZE");
    _Py_get_env_flag(use_env, &config->inspect, "PYTHONINSPECT");

    int dont_write_bytecode = 0;
    _Py_get_env_flag(use_env, &dont_write_bytecode, "PYTHONDONTWRITEBYTECODE");
    if (dont_write_bytecode) {
        config->write_bytecode = 0;
    }

    int no_user_site_directory = 0;
    _Py_get_env_flag(use_env, &no_user_site_directory, "PYTHONNOUSERSITE");
    if (no_user_site_directory) {
        config->user_site_directory = 0;
    }

    int unbuffered_stdio = 0;
    _Py_get_env_flag(use_env, &unbuffered_stdio, "PYTHONUNBUFFERED");
    if (unbuffered_stdio) {
        config->buffered_stdio = 0;
    }

#ifdef MS_WINDOWS
    _Py_get_env_flag(use_env, &config->legacy_windows_stdio,
                     "PYTHONLEGACYWINDOWSSTDIO");
#endif

    if (config_get_env(config, "PYTHONDUMPREFS")) {
        config->dump_refs = 1;
    }
    if (config_get_env(config, "PYTHONMALLOCSTATS")) {
        config->malloc_stats = 1;
    }

    if (config->dump_refs_file == NULL) {
        status = CONFIG_GET_ENV_DUP(config, &config->dump_refs_file,
                                    L"PYTHONDUMPREFSFILE", "PYTHONDUMPREFSFILE");
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (config->pythonpath_env == NULL) {
        status = CONFIG_GET_ENV_DUP(config, &config->pythonpath_env,
                                    L"PYTHONPATH", "PYTHONPATH");
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if(config->platlibdir == NULL) {
        status = CONFIG_GET_ENV_DUP(config, &config->platlibdir,
                                    L"PYTHONPLATLIBDIR", "PYTHONPLATLIBDIR");
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (config->use_hash_seed < 0) {
        status = config_init_hash_seed(config);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (config_get_env(config, "PYTHONSAFEPATH")) {
        config->safe_path = 1;
    }

    const char *gil = config_get_env(config, "PYTHON_GIL");
    if (gil != NULL) {
        size_t len = strlen(gil);
        status = config_read_gil(config, len, gil[0]);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    return _PyStatus_OK();
}

static PyStatus
config_init_cpu_count(PyConfig *config)
{
    const char *env = config_get_env(config, "PYTHON_CPU_COUNT");
    if (env) {
        int cpu_count = -1;
        if (strcmp(env, "default") == 0) {
            cpu_count = -1;
        }
        else if (_Py_str_to_int(env, &cpu_count) < 0 || cpu_count < 1) {
            goto error;
        }
        config->cpu_count = cpu_count;
    }

    const wchar_t *xoption = config_get_xoption(config, L"cpu_count");
    if (xoption) {
        int cpu_count = -1;
        const wchar_t *sep = wcschr(xoption, L'=');
        if (sep) {
            if (wcscmp(sep + 1, L"default") == 0) {
                cpu_count = -1;
            }
            else if (config_wstr_to_int(sep + 1, &cpu_count) < 0 || cpu_count < 1) {
                goto error;
            }
        }
        else {
            goto error;
        }
        config->cpu_count = cpu_count;
    }
    return _PyStatus_OK();

error:
    return _PyStatus_ERR("-X cpu_count=n option: n is missing or an invalid number, "
                         "n must be greater than 0");
}

static PyStatus
config_init_thread_inherit_context(PyConfig *config)
{
    const char *env = config_get_env(config, "PYTHON_THREAD_INHERIT_CONTEXT");
    if (env) {
        int enabled;
        if (_Py_str_to_int(env, &enabled) < 0 || (enabled < 0) || (enabled > 1)) {
            return _PyStatus_ERR(
                "PYTHON_THREAD_INHERIT_CONTEXT=N: N is missing or invalid");
        }
        config->thread_inherit_context = enabled;
    }

    const wchar_t *xoption = config_get_xoption(config, L"thread_inherit_context");
    if (xoption) {
        int enabled;
        const wchar_t *sep = wcschr(xoption, L'=');
        if (!sep || (config_wstr_to_int(sep + 1, &enabled) < 0) || (enabled < 0) || (enabled > 1)) {
            return _PyStatus_ERR(
                "-X thread_inherit_context=n: n is missing or invalid");
        }
        config->thread_inherit_context = enabled;
    }
    return _PyStatus_OK();
}

static PyStatus
config_init_context_aware_warnings(PyConfig *config)
{
    const char *env = config_get_env(config, "PYTHON_CONTEXT_AWARE_WARNINGS");
    if (env) {
        int enabled;
        if (_Py_str_to_int(env, &enabled) < 0 || (enabled < 0) || (enabled > 1)) {
            return _PyStatus_ERR(
                "PYTHON_CONTEXT_AWARE_WARNINGS=N: N is missing or invalid");
        }
        config->context_aware_warnings = enabled;
    }

    const wchar_t *xoption = config_get_xoption(config, L"context_aware_warnings");
    if (xoption) {
        int enabled;
        const wchar_t *sep = wcschr(xoption, L'=');
        if (!sep || (config_wstr_to_int(sep + 1, &enabled) < 0) || (enabled < 0) || (enabled > 1)) {
            return _PyStatus_ERR(
                "-X context_aware_warnings=n: n is missing or invalid");
        }
        config->context_aware_warnings = enabled;
    }
    return _PyStatus_OK();
}

static PyStatus
config_init_tlbc(PyConfig *config)
{
#ifdef Py_GIL_DISABLED
    const char *env = config_get_env(config, "PYTHON_TLBC");
    if (env) {
        int enabled;
        if (_Py_str_to_int(env, &enabled) < 0 || (enabled < 0) || (enabled > 1)) {
            return _PyStatus_ERR(
                "PYTHON_TLBC=N: N is missing or invalid");
        }
        config->tlbc_enabled = enabled;
    }

    const wchar_t *xoption = config_get_xoption(config, L"tlbc");
    if (xoption) {
        int enabled;
        const wchar_t *sep = wcschr(xoption, L'=');
        if (!sep || (config_wstr_to_int(sep + 1, &enabled) < 0) || (enabled < 0) || (enabled > 1)) {
            return _PyStatus_ERR(
                "-X tlbc=n: n is missing or invalid");
        }
        config->tlbc_enabled = enabled;
    }
    return _PyStatus_OK();
#else
    return _PyStatus_OK();
#endif
}

static PyStatus
config_init_perf_profiling(PyConfig *config)
{
    int active = 0;
    const char *env = config_get_env(config, "PYTHONPERFSUPPORT");
    if (env) {
        if (_Py_str_to_int(env, &active) != 0) {
            active = 0;
        }
        if (active) {
            config->perf_profiling = 1;
        }
    }
    const wchar_t *xoption = config_get_xoption(config, L"perf");
    if (xoption) {
        config->perf_profiling = 1;
    }
    env = config_get_env(config, "PYTHON_PERF_JIT_SUPPORT");
    if (env) {
        if (_Py_str_to_int(env, &active) != 0) {
            active = 0;
        }
        if (active) {
            config->perf_profiling = 2;
        }
    }
    xoption = config_get_xoption(config, L"perf_jit");
    if (xoption) {
        config->perf_profiling = 2;
    }

    return _PyStatus_OK();

}

static PyStatus
config_init_remote_debug(PyConfig *config)
{
#ifndef Py_REMOTE_DEBUG
    config->remote_debug = 0;
#else
    int active = 1;
    const char *env = Py_GETENV("PYTHON_DISABLE_REMOTE_DEBUG");
    if (env) {
        active = 0;
    }
    const wchar_t *xoption = config_get_xoption(config, L"disable-remote-debug");
    if (xoption) {
        active = 0;
    }

    config->remote_debug = active;
#endif
    return _PyStatus_OK();

}

static PyStatus
config_init_tracemalloc(PyConfig *config)
{
    int nframe;
    int valid;

    const char *env = config_get_env(config, "PYTHONTRACEMALLOC");
    if (env) {
        if (!_Py_str_to_int(env, &nframe)) {
            valid = (nframe >= 0);
        }
        else {
            valid = 0;
        }
        if (!valid) {
            return _PyStatus_ERR("PYTHONTRACEMALLOC: invalid number of frames");
        }
        config->tracemalloc = nframe;
    }

    const wchar_t *xoption = config_get_xoption(config, L"tracemalloc");
    if (xoption) {
        const wchar_t *sep = wcschr(xoption, L'=');
        if (sep) {
            if (!config_wstr_to_int(sep + 1, &nframe)) {
                valid = (nframe >= 0);
            }
            else {
                valid = 0;
            }
            if (!valid) {
                return _PyStatus_ERR("-X tracemalloc=NFRAME: "
                                     "invalid number of frames");
            }
        }
        else {
            /* -X tracemalloc behaves as -X tracemalloc=1 */
            nframe = 1;
        }
        config->tracemalloc = nframe;
    }
    return _PyStatus_OK();
}

static PyStatus
config_init_int_max_str_digits(PyConfig *config)
{
    int maxdigits;

    const char *env = config_get_env(config, "PYTHONINTMAXSTRDIGITS");
    if (env) {
        bool valid = 0;
        if (!_Py_str_to_int(env, &maxdigits)) {
            valid = ((maxdigits == 0) || (maxdigits >= _PY_LONG_MAX_STR_DIGITS_THRESHOLD));
        }
        if (!valid) {
#define STRINGIFY(VAL) _STRINGIFY(VAL)
#define _STRINGIFY(VAL) #VAL
            return _PyStatus_ERR(
                    "PYTHONINTMAXSTRDIGITS: invalid limit; must be >= "
                    STRINGIFY(_PY_LONG_MAX_STR_DIGITS_THRESHOLD)
                    " or 0 for unlimited.");
        }
        config->int_max_str_digits = maxdigits;
    }

    const wchar_t *xoption = config_get_xoption(config, L"int_max_str_digits");
    if (xoption) {
        const wchar_t *sep = wcschr(xoption, L'=');
        bool valid = 0;
        if (sep) {
            if (!config_wstr_to_int(sep + 1, &maxdigits)) {
                valid = ((maxdigits == 0) || (maxdigits >= _PY_LONG_MAX_STR_DIGITS_THRESHOLD));
            }
        }
        if (!valid) {
            return _PyStatus_ERR(
                    "-X int_max_str_digits: invalid limit; must be >= "
                    STRINGIFY(_PY_LONG_MAX_STR_DIGITS_THRESHOLD)
                    " or 0 for unlimited.");
#undef _STRINGIFY
#undef STRINGIFY
        }
        config->int_max_str_digits = maxdigits;
    }
    if (config->int_max_str_digits < 0) {
        config->int_max_str_digits = _PY_LONG_DEFAULT_MAX_STR_DIGITS;
    }
    return _PyStatus_OK();
}

static PyStatus
config_init_pycache_prefix(PyConfig *config)
{
    assert(config->pycache_prefix == NULL);

    const wchar_t *xoption = config_get_xoption(config, L"pycache_prefix");
    if (xoption) {
        const wchar_t *sep = wcschr(xoption, L'=');
        if (sep && wcslen(sep) > 1) {
            config->pycache_prefix = _PyMem_RawWcsdup(sep + 1);
            if (config->pycache_prefix == NULL) {
                return _PyStatus_NO_MEMORY();
            }
        }
        else {
            // PYTHONPYCACHEPREFIX env var ignored
            // if "-X pycache_prefix=" option is used
            config->pycache_prefix = NULL;
        }
        return _PyStatus_OK();
    }

    return CONFIG_GET_ENV_DUP(config, &config->pycache_prefix,
                              L"PYTHONPYCACHEPREFIX",
                              "PYTHONPYCACHEPREFIX");
}


#ifdef Py_DEBUG
static PyStatus
config_init_run_presite(PyConfig *config)
{
    assert(config->run_presite == NULL);

    const wchar_t *xoption = config_get_xoption(config, L"presite");
    if (xoption) {
        const wchar_t *sep = wcschr(xoption, L'=');
        if (sep && wcslen(sep) > 1) {
            config->run_presite = _PyMem_RawWcsdup(sep + 1);
            if (config->run_presite == NULL) {
                return _PyStatus_NO_MEMORY();
            }
        }
        else {
            // PYTHON_PRESITE env var ignored
            // if "-X presite=" option is used
            config->run_presite = NULL;
        }
        return _PyStatus_OK();
    }

    return CONFIG_GET_ENV_DUP(config, &config->run_presite,
                              L"PYTHON_PRESITE",
                              "PYTHON_PRESITE");
}
#endif

static PyStatus
config_init_import_time(PyConfig *config)
{
    int importtime = 0;

    const char *env = config_get_env(config, "PYTHONPROFILEIMPORTTIME");
    if (env) {
        if (_Py_str_to_int(env, &importtime) != 0) {
            importtime = 1;
        }
        if (importtime < 0 || importtime > 2) {
            return _PyStatus_ERR(
                "PYTHONPROFILEIMPORTTIME: numeric values other than 1 and 2 "
                "are reserved for future use.");
        }
    }

    const wchar_t *x_value = config_get_xoption_value(config, L"importtime");
    if (x_value) {
        if (*x_value == 0 || config_wstr_to_int(x_value, &importtime) != 0) {
            importtime = 1;
        }
        if (importtime < 0 || importtime > 2) {
            return _PyStatus_ERR(
                "-X importtime: values other than 1 and 2 "
                "are reserved for future use.");
        }
    }

    config->import_time = importtime;
    return _PyStatus_OK();
}

static PyStatus
config_read_complex_options(PyConfig *config)
{
    /* More complex options configured by env var and -X option */
    if (config->faulthandler < 0) {
        if (config_get_env(config, "PYTHONFAULTHANDLER")
           || config_get_xoption(config, L"faulthandler")) {
            config->faulthandler = 1;
        }
    }
    if (config_get_env(config, "PYTHONNODEBUGRANGES")
       || config_get_xoption(config, L"no_debug_ranges")) {
        config->code_debug_ranges = 0;
    }

    PyStatus status;
    if (config->import_time < 0) {
        status = config_init_import_time(config);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (config->tracemalloc < 0) {
        status = config_init_tracemalloc(config);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (config->perf_profiling < 0) {
        status = config_init_perf_profiling(config);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (config->remote_debug < 0) {
        status = config_init_remote_debug(config);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (config->int_max_str_digits < 0) {
        status = config_init_int_max_str_digits(config);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (config->cpu_count < 0) {
        status = config_init_cpu_count(config);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (config->pycache_prefix == NULL) {
        status = config_init_pycache_prefix(config);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

#ifdef Py_DEBUG
    if (config->run_presite == NULL) {
        status = config_init_run_presite(config);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
#endif

    status = config_init_thread_inherit_context(config);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = config_init_context_aware_warnings(config);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = config_init_tlbc(config);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    return _PyStatus_OK();
}


static const wchar_t *
config_get_stdio_errors(const PyPreConfig *preconfig)
{
    if (preconfig->utf8_mode) {
        /* UTF-8 Mode uses UTF-8/surrogateescape */
        return L"surrogateescape";
    }

#ifndef MS_WINDOWS
    const char *loc = setlocale(LC_CTYPE, NULL);
    if (loc != NULL) {
        /* surrogateescape is the default in the legacy C and POSIX locales */
        if (strcmp(loc, "C") == 0 || strcmp(loc, "POSIX") == 0) {
            return L"surrogateescape";
        }

#ifdef PY_COERCE_C_LOCALE
        /* surrogateescape is the default in locale coercion target locales */
        if (_Py_IsLocaleCoercionTarget(loc)) {
            return L"surrogateescape";
        }
#endif
    }

    return L"strict";
#else
    /* On Windows, always use surrogateescape by default */
    return L"surrogateescape";
#endif
}


// See also config_get_fs_encoding()
static PyStatus
config_get_locale_encoding(PyConfig *config, const PyPreConfig *preconfig,
                           wchar_t **locale_encoding)
{
    wchar_t *encoding;
    if (preconfig->utf8_mode) {
        encoding = _PyMem_RawWcsdup(L"utf-8");
    }
    else {
        encoding = _Py_GetLocaleEncoding();
    }
    if (encoding == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    PyStatus status = PyConfig_SetString(config, locale_encoding, encoding);
    PyMem_RawFree(encoding);
    return status;
}


static PyStatus
config_init_stdio_encoding(PyConfig *config,
                           const PyPreConfig *preconfig)
{
    PyStatus status;

    // Exit if encoding and errors are defined
    if (config->stdio_encoding != NULL && config->stdio_errors != NULL) {
        return _PyStatus_OK();
    }

    /* PYTHONIOENCODING environment variable */
    const char *opt = config_get_env(config, "PYTHONIOENCODING");
    if (opt) {
        char *pythonioencoding = _PyMem_RawStrdup(opt);
        if (pythonioencoding == NULL) {
            return _PyStatus_NO_MEMORY();
        }

        char *errors = strchr(pythonioencoding, ':');
        if (errors) {
            *errors = '\0';
            errors++;
            if (!errors[0]) {
                errors = NULL;
            }
        }

        /* Does PYTHONIOENCODING contain an encoding? */
        if (pythonioencoding[0]) {
            if (config->stdio_encoding == NULL) {
                status = CONFIG_SET_BYTES_STR(config, &config->stdio_encoding,
                                              pythonioencoding,
                                              "PYTHONIOENCODING environment variable");
                if (_PyStatus_EXCEPTION(status)) {
                    PyMem_RawFree(pythonioencoding);
                    return status;
                }
            }

            /* If the encoding is set but not the error handler,
               use "strict" error handler by default.
               PYTHONIOENCODING=latin1 behaves as
               PYTHONIOENCODING=latin1:strict. */
            if (!errors) {
                errors = "strict";
            }
        }

        if (config->stdio_errors == NULL && errors != NULL) {
            status = CONFIG_SET_BYTES_STR(config, &config->stdio_errors,
                                          errors,
                                          "PYTHONIOENCODING environment variable");
            if (_PyStatus_EXCEPTION(status)) {
                PyMem_RawFree(pythonioencoding);
                return status;
            }
        }

        PyMem_RawFree(pythonioencoding);
    }

    /* Choose the default error handler based on the current locale. */
    if (config->stdio_encoding == NULL) {
        status = config_get_locale_encoding(config, preconfig,
                                            &config->stdio_encoding);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
    if (config->stdio_errors == NULL) {
        const wchar_t *errors = config_get_stdio_errors(preconfig);
        assert(errors != NULL);

        status = PyConfig_SetString(config, &config->stdio_errors, errors);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    return _PyStatus_OK();
}


// See also config_get_locale_encoding()
static PyStatus
config_get_fs_encoding(PyConfig *config, const PyPreConfig *preconfig,
                       wchar_t **fs_encoding)
{
#ifdef _Py_FORCE_UTF8_FS_ENCODING
    return PyConfig_SetString(config, fs_encoding, L"utf-8");
#elif defined(MS_WINDOWS)
    const wchar_t *encoding;
    if (preconfig->legacy_windows_fs_encoding) {
        // Legacy Windows filesystem encoding: mbcs/replace
        encoding = L"mbcs";
    }
    else {
        // Windows defaults to utf-8/surrogatepass (PEP 529)
        encoding = L"utf-8";
    }
     return PyConfig_SetString(config, fs_encoding, encoding);
#else  // !MS_WINDOWS
    if (preconfig->utf8_mode) {
        return PyConfig_SetString(config, fs_encoding, L"utf-8");
    }

    if (_Py_GetForceASCII()) {
        return PyConfig_SetString(config, fs_encoding, L"ascii");
    }

    return config_get_locale_encoding(config, preconfig, fs_encoding);
#endif  // !MS_WINDOWS
}


static PyStatus
config_init_fs_encoding(PyConfig *config, const PyPreConfig *preconfig)
{
    PyStatus status;

    if (config->filesystem_encoding == NULL) {
        status = config_get_fs_encoding(config, preconfig,
                                        &config->filesystem_encoding);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (config->filesystem_errors == NULL) {
        const wchar_t *errors;
#ifdef MS_WINDOWS
        if (preconfig->legacy_windows_fs_encoding) {
            errors = L"replace";
        }
        else {
            errors = L"surrogatepass";
        }
#else
        errors = L"surrogateescape";
#endif
        status = PyConfig_SetString(config, &config->filesystem_errors, errors);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
    return _PyStatus_OK();
}


static PyStatus
config_init_import(PyConfig *config, int compute_path_config)
{
    PyStatus status;

    status = _PyConfig_InitPathConfig(config, compute_path_config);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    const char *env = config_get_env(config, "PYTHON_FROZEN_MODULES");
    if (env == NULL) {
    }
    else if (strcmp(env, "on") == 0) {
        config->use_frozen_modules = 1;
    }
    else if (strcmp(env, "off") == 0) {
        config->use_frozen_modules = 0;
    } else {
        return PyStatus_Error("bad value for PYTHON_FROZEN_MODULES "
                              "(expected \"on\" or \"off\")");
    }

    /* -X frozen_modules=[on|off] */
    const wchar_t *value = config_get_xoption_value(config, L"frozen_modules");
    if (value == NULL) {
    }
    else if (wcscmp(value, L"on") == 0) {
        config->use_frozen_modules = 1;
    }
    else if (wcscmp(value, L"off") == 0) {
        config->use_frozen_modules = 0;
    }
    else if (wcslen(value) == 0) {
        // "-X frozen_modules" and "-X frozen_modules=" both imply "on".
        config->use_frozen_modules = 1;
    }
    else {
        return PyStatus_Error("bad value for option -X frozen_modules "
                              "(expected \"on\" or \"off\")");
    }

    assert(config->use_frozen_modules >= 0);
    return _PyStatus_OK();
}

PyStatus
_PyConfig_InitImportConfig(PyConfig *config)
{
    return config_init_import(config, 1);
}


static PyStatus
config_read(PyConfig *config, int compute_path_config)
{
    PyStatus status;
    const PyPreConfig *preconfig = &_PyRuntime.preconfig;

    if (config->use_environment) {
        status = config_read_env_vars(config);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    /* -X options */
    if (config_get_xoption(config, L"showrefcount")) {
        config->show_ref_count = 1;
    }

    const wchar_t *x_gil = config_get_xoption_value(config, L"gil");
    if (x_gil != NULL) {
        size_t len = wcslen(x_gil);
        status = config_read_gil(config, len, x_gil[0]);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

#ifdef Py_STATS
    if (config_get_xoption(config, L"pystats")) {
        config->_pystats = 1;
    }
    else if (config_get_env(config, "PYTHONSTATS")) {
        config->_pystats = 1;
    }
    if (config->_pystats < 0) {
        config->_pystats = 0;
    }
#endif

    status = config_read_complex_options(config);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if (config->_install_importlib) {
        status = config_init_import(config, compute_path_config);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    /* default values */
    if (config->dev_mode) {
        if (config->faulthandler < 0) {
            config->faulthandler = 1;
        }
    }
    if (config->faulthandler < 0) {
        config->faulthandler = 0;
    }
    if (config->tracemalloc < 0) {
        config->tracemalloc = 0;
    }
    if (config->perf_profiling < 0) {
        config->perf_profiling = 0;
    }
    if (config->remote_debug < 0) {
        config->remote_debug = -1;
    }
    if (config->use_hash_seed < 0) {
        config->use_hash_seed = 0;
        config->hash_seed = 0;
    }

    if (config->filesystem_encoding == NULL || config->filesystem_errors == NULL) {
        status = config_init_fs_encoding(config, preconfig);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    status = config_init_stdio_encoding(config, preconfig);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if (config->argv.length < 1) {
        /* Ensure at least one (empty) argument is seen */
        status = PyWideStringList_Append(&config->argv, L"");
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (config->check_hash_pycs_mode == NULL) {
        status = PyConfig_SetString(config, &config->check_hash_pycs_mode,
                                    L"default");
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (config->configure_c_stdio < 0) {
        config->configure_c_stdio = 1;
    }

    // Only parse arguments once.
    if (config->parse_argv == 1) {
        config->parse_argv = 2;
    }

    return _PyStatus_OK();
}


static void
config_init_stdio(const PyConfig *config)
{
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


/* Write the configuration:

   - set Py_xxx global configuration variables
   - initialize C standard streams (stdin, stdout, stderr) */
PyStatus
_PyConfig_Write(const PyConfig *config, _PyRuntimeState *runtime)
{
    config_set_global_vars(config);

    if (config->configure_c_stdio) {
        config_init_stdio(config);
    }

    /* Write the new pre-configuration into _PyRuntime */
    PyPreConfig *preconfig = &runtime->preconfig;
    preconfig->isolated = config->isolated;
    preconfig->use_environment = config->use_environment;
    preconfig->dev_mode = config->dev_mode;

    if (_Py_SetArgcArgv(config->orig_argv.length,
                        config->orig_argv.items) < 0)
    {
        return _PyStatus_NO_MEMORY();
    }

    return _PyStatus_OK();
}


/* --- PyConfig command line parser -------------------------- */

static void
config_usage(int error, const wchar_t* program)
{
    FILE *f = error ? stderr : stdout;

    fprintf(f, usage_line, program);
    if (error)
        fprintf(f, "Try `python -h' for more information.\n");
    else {
        fputs(usage_help, f);
    }
}

static void
config_envvars_usage(void)
{
    printf(usage_envvars, (wint_t)DELIM, (wint_t)DELIM, PYTHONHOMEHELP);
}

static void
config_xoptions_usage(void)
{
    puts(usage_xoptions);
}

static void
config_complete_usage(const wchar_t* program)
{
   config_usage(0, program);
   putchar('\n');
   config_envvars_usage();
   putchar('\n');
   config_xoptions_usage();
}


/* Parse the command line arguments */
static PyStatus
config_parse_cmdline(PyConfig *config, PyWideStringList *warnoptions,
                     Py_ssize_t *opt_index)
{
    PyStatus status;
    const PyWideStringList *argv = &config->argv;
    int print_version = 0;
    const wchar_t* program = config->program_name;
    if (!program && argv->length >= 1) {
        program = argv->items[0];
    }

    _PyOS_ResetGetOpt();
    do {
        int longindex = -1;
        int c = _PyOS_GetOpt(argv->length, argv->items, &longindex);
        if (c == EOF) {
            break;
        }

        if (c == 'c') {
            if (config->run_command == NULL) {
                /* -c is the last option; following arguments
                   that look like options are left for the
                   command to interpret. */
                size_t len = wcslen(_PyOS_optarg) + 1 + 1;
                wchar_t *command = PyMem_RawMalloc(sizeof(wchar_t) * len);
                if (command == NULL) {
                    return _PyStatus_NO_MEMORY();
                }
                memcpy(command, _PyOS_optarg, (len - 2) * sizeof(wchar_t));
                command[len - 2] = '\n';
                command[len - 1] = 0;
                config->run_command = command;
            }
            break;
        }

        if (c == 'm') {
            /* -m is the last option; following arguments
               that look like options are left for the
               module to interpret. */
            if (config->run_module == NULL) {
                config->run_module = _PyMem_RawWcsdup(_PyOS_optarg);
                if (config->run_module == NULL) {
                    return _PyStatus_NO_MEMORY();
                }
            }
            break;
        }

        switch (c) {
        // Integers represent long options, see Python/getopt.c
        case 0:
            // check-hash-based-pycs
            if (wcscmp(_PyOS_optarg, L"always") == 0
                || wcscmp(_PyOS_optarg, L"never") == 0
                || wcscmp(_PyOS_optarg, L"default") == 0)
            {
                status = PyConfig_SetString(config, &config->check_hash_pycs_mode,
                                            _PyOS_optarg);
                if (_PyStatus_EXCEPTION(status)) {
                    return status;
                }
            } else {
                fprintf(stderr, "--check-hash-based-pycs must be one of "
                        "'default', 'always', or 'never'\n");
                config_usage(1, program);
                return _PyStatus_EXIT(2);
            }
            break;

        case 1:
            // help-all
            config_complete_usage(program);
            return _PyStatus_EXIT(0);

        case 2:
            // help-env
            config_envvars_usage();
            return _PyStatus_EXIT(0);

        case 3:
            // help-xoptions
            config_xoptions_usage();
            return _PyStatus_EXIT(0);

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

        case 'E':
        case 'I':
        case 'X':
            /* option handled by _PyPreCmdline_Read() */
            break;

        case 'O':
            config->optimization_level++;
            break;

        case 'P':
            config->safe_path = 1;
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
            config->skip_source_first_line = 1;
            break;

        case 'h':
        case '?':
            config_usage(0, program);
            return _PyStatus_EXIT(0);

        case 'V':
            print_version++;
            break;

        case 'W':
            status = PyWideStringList_Append(warnoptions, _PyOS_optarg);
            if (_PyStatus_EXCEPTION(status)) {
                return status;
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
            config_usage(1, program);
            return _PyStatus_EXIT(2);
        }
    } while (1);

    if (print_version) {
        printf("Python %s\n",
                (print_version >= 2) ? Py_GetVersion() : PY_VERSION);
        return _PyStatus_EXIT(0);
    }

    if (config->run_command == NULL && config->run_module == NULL
        && _PyOS_optind < argv->length
        && wcscmp(argv->items[_PyOS_optind], L"-") != 0
        && config->run_filename == NULL)
    {
        config->run_filename = _PyMem_RawWcsdup(argv->items[_PyOS_optind]);
        if (config->run_filename == NULL) {
            return _PyStatus_NO_MEMORY();
        }
    }

    if (config->run_command != NULL || config->run_module != NULL) {
        /* Backup _PyOS_optind */
        _PyOS_optind--;
    }

    *opt_index = _PyOS_optind;

    return _PyStatus_OK();
}


#ifdef MS_WINDOWS
#  define WCSTOK wcstok_s
#else
#  define WCSTOK wcstok
#endif

/* Get warning options from PYTHONWARNINGS environment variable. */
static PyStatus
config_init_env_warnoptions(PyConfig *config, PyWideStringList *warnoptions)
{
    PyStatus status;
    /* CONFIG_GET_ENV_DUP requires dest to be initialized to NULL */
    wchar_t *env = NULL;
    status = CONFIG_GET_ENV_DUP(config, &env,
                             L"PYTHONWARNINGS", "PYTHONWARNINGS");
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    /* env var is not set or is empty */
    if (env == NULL) {
        return _PyStatus_OK();
    }


    wchar_t *warning, *context = NULL;
    for (warning = WCSTOK(env, L",", &context);
         warning != NULL;
         warning = WCSTOK(NULL, L",", &context))
    {
        status = PyWideStringList_Append(warnoptions, warning);
        if (_PyStatus_EXCEPTION(status)) {
            PyMem_RawFree(env);
            return status;
        }
    }
    PyMem_RawFree(env);
    return _PyStatus_OK();
}


static PyStatus
warnoptions_append(PyConfig *config, PyWideStringList *options,
                   const wchar_t *option)
{
    /* config_init_warnoptions() add existing config warnoptions at the end:
       ensure that the new option is not already present in this list to
       prevent change the options order when config_init_warnoptions() is
       called twice. */
    if (_PyWideStringList_Find(&config->warnoptions, option)) {
        /* Already present: do nothing */
        return _PyStatus_OK();
    }
    if (_PyWideStringList_Find(options, option)) {
        /* Already present: do nothing */
        return _PyStatus_OK();
    }
    return PyWideStringList_Append(options, option);
}


static PyStatus
warnoptions_extend(PyConfig *config, PyWideStringList *options,
                   const PyWideStringList *options2)
{
    const Py_ssize_t len = options2->length;
    wchar_t *const *items = options2->items;

    for (Py_ssize_t i = 0; i < len; i++) {
        PyStatus status = warnoptions_append(config, options, items[i]);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
    return _PyStatus_OK();
}


static PyStatus
config_init_warnoptions(PyConfig *config,
                        const PyWideStringList *cmdline_warnoptions,
                        const PyWideStringList *env_warnoptions,
                        const PyWideStringList *sys_warnoptions)
{
    PyStatus status;
    PyWideStringList options = _PyWideStringList_INIT;

    /* Priority of warnings options, lowest to highest:
     *
     * - any implicit filters added by _warnings.c/warnings.py
     * - PyConfig.dev_mode: "default" filter
     * - PYTHONWARNINGS environment variable
     * - '-W' command line options
     * - PyConfig.bytes_warning ('-b' and '-bb' command line options):
     *   "default::BytesWarning" or "error::BytesWarning" filter
     * - early PySys_AddWarnOption() calls
     * - PyConfig.warnoptions
     *
     * PyConfig.warnoptions is copied to sys.warnoptions. Since the warnings
     * module works on the basis of "the most recently added filter will be
     * checked first", we add the lowest precedence entries first so that later
     * entries override them.
     */

    if (config->dev_mode) {
        status = warnoptions_append(config, &options, L"default");
        if (_PyStatus_EXCEPTION(status)) {
            goto error;
        }
    }

    status = warnoptions_extend(config, &options, env_warnoptions);
    if (_PyStatus_EXCEPTION(status)) {
        goto error;
    }

    status = warnoptions_extend(config, &options, cmdline_warnoptions);
    if (_PyStatus_EXCEPTION(status)) {
        goto error;
    }

    /* If the bytes_warning_flag isn't set, bytesobject.c and bytearrayobject.c
     * don't even try to emit a warning, so we skip setting the filter in that
     * case.
     */
    if (config->bytes_warning) {
        const wchar_t *filter;
        if (config->bytes_warning> 1) {
            filter = L"error::BytesWarning";
        }
        else {
            filter = L"default::BytesWarning";
        }
        status = warnoptions_append(config, &options, filter);
        if (_PyStatus_EXCEPTION(status)) {
            goto error;
        }
    }

    status = warnoptions_extend(config, &options, sys_warnoptions);
    if (_PyStatus_EXCEPTION(status)) {
        goto error;
    }

    /* Always add all PyConfig.warnoptions options */
    status = _PyWideStringList_Extend(&options, &config->warnoptions);
    if (_PyStatus_EXCEPTION(status)) {
        goto error;
    }

    _PyWideStringList_Clear(&config->warnoptions);
    config->warnoptions = options;
    return _PyStatus_OK();

error:
    _PyWideStringList_Clear(&options);
    return status;
}


static PyStatus
config_update_argv(PyConfig *config, Py_ssize_t opt_index)
{
    const PyWideStringList *cmdline_argv = &config->argv;
    PyWideStringList config_argv = _PyWideStringList_INIT;

    /* Copy argv to be able to modify it (to force -c/-m) */
    if (cmdline_argv->length <= opt_index) {
        /* Ensure at least one (empty) argument is seen */
        PyStatus status = PyWideStringList_Append(&config_argv, L"");
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
    else {
        PyWideStringList slice;
        slice.length = cmdline_argv->length - opt_index;
        slice.items = &cmdline_argv->items[opt_index];
        if (_PyWideStringList_Copy(&config_argv, &slice) < 0) {
            return _PyStatus_NO_MEMORY();
        }
    }
    assert(config_argv.length >= 1);

    wchar_t *arg0 = NULL;
    if (config->run_command != NULL) {
        /* Force sys.argv[0] = '-c' */
        arg0 = L"-c";
    }
    else if (config->run_module != NULL) {
        /* Force sys.argv[0] = '-m'*/
        arg0 = L"-m";
    }

    if (arg0 != NULL) {
        arg0 = _PyMem_RawWcsdup(arg0);
        if (arg0 == NULL) {
            _PyWideStringList_Clear(&config_argv);
            return _PyStatus_NO_MEMORY();
        }

        PyMem_RawFree(config_argv.items[0]);
        config_argv.items[0] = arg0;
    }

    _PyWideStringList_Clear(&config->argv);
    config->argv = config_argv;
    return _PyStatus_OK();
}


static PyStatus
core_read_precmdline(PyConfig *config, _PyPreCmdline *precmdline)
{
    PyStatus status;

    if (config->parse_argv == 1) {
        if (_PyWideStringList_Copy(&precmdline->argv, &config->argv) < 0) {
            return _PyStatus_NO_MEMORY();
        }
    }

    PyPreConfig preconfig;

    status = _PyPreConfig_InitFromPreConfig(&preconfig, &_PyRuntime.preconfig);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    _PyPreConfig_GetConfig(&preconfig, config);

    status = _PyPreCmdline_Read(precmdline, &preconfig);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PyPreCmdline_SetConfig(precmdline, config);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }
    return _PyStatus_OK();
}


/* Get run_filename absolute path */
static PyStatus
config_run_filename_abspath(PyConfig *config)
{
    if (!config->run_filename) {
        return _PyStatus_OK();
    }

#ifndef MS_WINDOWS
    if (_Py_isabs(config->run_filename)) {
        /* path is already absolute */
        return _PyStatus_OK();
    }
#endif

    wchar_t *abs_filename;
    if (_Py_abspath(config->run_filename, &abs_filename) < 0) {
        /* failed to get the absolute path of the command line filename:
           ignore the error, keep the relative path */
        return _PyStatus_OK();
    }
    if (abs_filename == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    PyMem_RawFree(config->run_filename);
    config->run_filename = abs_filename;
    return _PyStatus_OK();
}


static PyStatus
config_read_cmdline(PyConfig *config)
{
    PyStatus status;
    PyWideStringList cmdline_warnoptions = _PyWideStringList_INIT;
    PyWideStringList env_warnoptions = _PyWideStringList_INIT;
    PyWideStringList sys_warnoptions = _PyWideStringList_INIT;

    if (config->parse_argv < 0) {
        config->parse_argv = 1;
    }

    if (config->parse_argv == 1) {
        Py_ssize_t opt_index;
        status = config_parse_cmdline(config, &cmdline_warnoptions, &opt_index);
        if (_PyStatus_EXCEPTION(status)) {
            goto done;
        }

        status = config_run_filename_abspath(config);
        if (_PyStatus_EXCEPTION(status)) {
            goto done;
        }

        status = config_update_argv(config, opt_index);
        if (_PyStatus_EXCEPTION(status)) {
            goto done;
        }
    }
    else {
        status = config_run_filename_abspath(config);
        if (_PyStatus_EXCEPTION(status)) {
            goto done;
        }
    }

    if (config->use_environment) {
        status = config_init_env_warnoptions(config, &env_warnoptions);
        if (_PyStatus_EXCEPTION(status)) {
            goto done;
        }
    }

    /* Handle early PySys_AddWarnOption() calls */
    status = _PySys_ReadPreinitWarnOptions(&sys_warnoptions);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    status = config_init_warnoptions(config,
                                     &cmdline_warnoptions,
                                     &env_warnoptions,
                                     &sys_warnoptions);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    status = _PyStatus_OK();

done:
    _PyWideStringList_Clear(&cmdline_warnoptions);
    _PyWideStringList_Clear(&env_warnoptions);
    _PyWideStringList_Clear(&sys_warnoptions);
    return status;
}


PyStatus
_PyConfig_SetPyArgv(PyConfig *config, const _PyArgv *args)
{
    PyStatus status = _Py_PreInitializeFromConfig(config, args);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    return _PyArgv_AsWstrList(args, &config->argv);
}


/* Set config.argv: decode argv using Py_DecodeLocale(). Pre-initialize Python
   if needed to ensure that encodings are properly configured. */
PyStatus
PyConfig_SetBytesArgv(PyConfig *config, Py_ssize_t argc, char * const *argv)
{
    _PyArgv args = {
        .argc = argc,
        .use_bytes_argv = 1,
        .bytes_argv = argv,
        .wchar_argv = NULL};
    return _PyConfig_SetPyArgv(config, &args);
}


PyStatus
PyConfig_SetArgv(PyConfig *config, Py_ssize_t argc, wchar_t * const *argv)
{
    _PyArgv args = {
        .argc = argc,
        .use_bytes_argv = 0,
        .bytes_argv = NULL,
        .wchar_argv = argv};
    return _PyConfig_SetPyArgv(config, &args);
}


PyStatus
PyConfig_SetWideStringList(PyConfig *config, PyWideStringList *list,
                           Py_ssize_t length, wchar_t **items)
{
    PyStatus status = _Py_PreInitializeFromConfig(config, NULL);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    PyWideStringList list2 = {.length = length, .items = items};
    if (_PyWideStringList_Copy(list, &list2) < 0) {
        return _PyStatus_NO_MEMORY();
    }
    return _PyStatus_OK();
}


/* Read the configuration into PyConfig from:

   * Command line arguments
   * Environment variables
   * Py_xxx global configuration variables

   The only side effects are to modify config and to call _Py_SetArgcArgv(). */
PyStatus
_PyConfig_Read(PyConfig *config, int compute_path_config)
{
    PyStatus status;

    status = _Py_PreInitializeFromConfig(config, NULL);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    config_get_global_vars(config);

    if (config->orig_argv.length == 0
        && !(config->argv.length == 1
             && wcscmp(config->argv.items[0], L"") == 0))
    {
        if (_PyWideStringList_Copy(&config->orig_argv, &config->argv) < 0) {
            return _PyStatus_NO_MEMORY();
        }
    }

    _PyPreCmdline precmdline = _PyPreCmdline_INIT;
    status = core_read_precmdline(config, &precmdline);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    assert(config->isolated >= 0);
    if (config->isolated) {
        config->safe_path = 1;
        config->use_environment = 0;
        config->user_site_directory = 0;
    }

    status = config_read_cmdline(config);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    /* Handle early PySys_AddXOption() calls */
    status = _PySys_ReadPreinitXOptions(config);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    status = config_read(config, compute_path_config);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    assert(config_check_consistency(config));

    status = _PyStatus_OK();

done:
    _PyPreCmdline_Clear(&precmdline);
    return status;
}


PyStatus
PyConfig_Read(PyConfig *config)
{
    return _PyConfig_Read(config, 0);
}


PyObject*
_Py_GetConfigsAsDict(void)
{
    PyObject *result = NULL;
    PyObject *dict = NULL;

    result = PyDict_New();
    if (result == NULL) {
        goto error;
    }

    /* global result */
    dict = _Py_GetGlobalVariablesAsDict();
    if (dict == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(result, "global_config", dict) < 0) {
        goto error;
    }
    Py_CLEAR(dict);

    /* pre config */
    PyInterpreterState *interp = _PyInterpreterState_GET();
    const PyPreConfig *pre_config = &interp->runtime->preconfig;
    dict = _PyPreConfig_AsDict(pre_config);
    if (dict == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(result, "pre_config", dict) < 0) {
        goto error;
    }
    Py_CLEAR(dict);

    /* core config */
    const PyConfig *config = _PyInterpreterState_GetConfig(interp);
    dict = _PyConfig_AsDict(config);
    if (dict == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(result, "config", dict) < 0) {
        goto error;
    }
    Py_CLEAR(dict);

    return result;

error:
    Py_XDECREF(result);
    Py_XDECREF(dict);
    return NULL;
}


static void
init_dump_ascii_wstr(const wchar_t *str)
{
    if (str == NULL) {
        PySys_WriteStderr("(not set)");
        return;
    }

    PySys_WriteStderr("'");
    for (; *str != L'\0'; str++) {
        unsigned int ch = (unsigned int)*str;
        if (ch == L'\'') {
            PySys_WriteStderr("\\'");
        } else if (0x20 <= ch && ch < 0x7f) {
            PySys_WriteStderr("%c", ch);
        }
        else if (ch <= 0xff) {
            PySys_WriteStderr("\\x%02x", ch);
        }
#if SIZEOF_WCHAR_T > 2
        else if (ch > 0xffff) {
            PySys_WriteStderr("\\U%08x", ch);
        }
#endif
        else {
            PySys_WriteStderr("\\u%04x", ch);
        }
    }
    PySys_WriteStderr("'");
}


/* Dump the Python path configuration into sys.stderr */
void
_Py_DumpPathConfig(PyThreadState *tstate)
{
    PyObject *exc = _PyErr_GetRaisedException(tstate);

    PySys_WriteStderr("Python path configuration:\n");

#define DUMP_CONFIG(NAME, FIELD) \
        do { \
            PySys_WriteStderr("  " NAME " = "); \
            init_dump_ascii_wstr(config->FIELD); \
            PySys_WriteStderr("\n"); \
        } while (0)

    const PyConfig *config = _PyInterpreterState_GetConfig(tstate->interp);
    DUMP_CONFIG("PYTHONHOME", home);
    DUMP_CONFIG("PYTHONPATH", pythonpath_env);
    DUMP_CONFIG("program name", program_name);
    PySys_WriteStderr("  isolated = %i\n", config->isolated);
    PySys_WriteStderr("  environment = %i\n", config->use_environment);
    PySys_WriteStderr("  user site = %i\n", config->user_site_directory);
    PySys_WriteStderr("  safe_path = %i\n", config->safe_path);
    PySys_WriteStderr("  import site = %i\n", config->site_import);
    PySys_WriteStderr("  is in build tree = %i\n", config->_is_python_build);
    DUMP_CONFIG("stdlib dir", stdlib_dir);
    DUMP_CONFIG("sys.path[0]", sys_path_0);
#undef DUMP_CONFIG

#define DUMP_SYS(NAME) \
        do { \
            PySys_FormatStderr("  sys.%s = ", #NAME); \
            if (PySys_GetOptionalAttrString(#NAME, &obj) < 0) { \
                PyErr_Clear(); \
            } \
            if (obj != NULL) { \
                PySys_FormatStderr("%A", obj); \
                Py_DECREF(obj); \
            } \
            else { \
                PySys_WriteStderr("(not set)"); \
            } \
            PySys_FormatStderr("\n"); \
        } while (0)

    PyObject *obj;
    DUMP_SYS(_base_executable);
    DUMP_SYS(base_prefix);
    DUMP_SYS(base_exec_prefix);
    DUMP_SYS(platlibdir);
    DUMP_SYS(executable);
    DUMP_SYS(prefix);
    DUMP_SYS(exec_prefix);
#undef DUMP_SYS

    PyObject *sys_path;
    (void) PySys_GetOptionalAttrString("path", &sys_path);
    if (sys_path != NULL && PyList_Check(sys_path)) {
        PySys_WriteStderr("  sys.path = [\n");
        Py_ssize_t len = PyList_GET_SIZE(sys_path);
        for (Py_ssize_t i=0; i < len; i++) {
            PyObject *path = PyList_GET_ITEM(sys_path, i);
            PySys_FormatStderr("    %A,\n", path);
        }
        PySys_WriteStderr("  ]\n");
    }
    Py_XDECREF(sys_path);

    _PyErr_SetRaisedException(tstate, exc);
}


// --- PyInitConfig API ---------------------------------------------------

struct PyInitConfig {
    PyPreConfig preconfig;
    PyConfig config;
    struct _inittab *inittab;
    Py_ssize_t inittab_size;
    PyStatus status;
    char *err_msg;
};

static PyInitConfig*
initconfig_alloc(void)
{
    return calloc(1, sizeof(PyInitConfig));
}


PyInitConfig*
PyInitConfig_Create(void)
{
    PyInitConfig *config = initconfig_alloc();
    if (config == NULL) {
        return NULL;
    }
    PyPreConfig_InitIsolatedConfig(&config->preconfig);
    PyConfig_InitIsolatedConfig(&config->config);
    config->status = _PyStatus_OK();
    return config;
}


void
PyInitConfig_Free(PyInitConfig *config)
{
    if (config == NULL) {
        return;
    }

    initconfig_free_config(&config->config);
    PyMem_RawFree(config->inittab);
    free(config->err_msg);
    free(config);
}


int
PyInitConfig_GetError(PyInitConfig* config, const char **perr_msg)
{
    if (_PyStatus_IS_EXIT(config->status)) {
        char buffer[22];  // len("exit code -2147483648\0")
        PyOS_snprintf(buffer, sizeof(buffer),
                      "exit code %i",
                      config->status.exitcode);

        if (config->err_msg != NULL) {
            free(config->err_msg);
        }
        config->err_msg = strdup(buffer);
        if (config->err_msg != NULL) {
            *perr_msg = config->err_msg;
            return 1;
        }
        config->status = _PyStatus_NO_MEMORY();
    }

    if (_PyStatus_IS_ERROR(config->status) && config->status.err_msg != NULL) {
        *perr_msg = config->status.err_msg;
        return 1;
    }
    else {
        *perr_msg = NULL;
        return 0;
    }
}


int
PyInitConfig_GetExitCode(PyInitConfig* config, int *exitcode)
{
    if (_PyStatus_IS_EXIT(config->status)) {
        *exitcode = config->status.exitcode;
        return 1;
    }
    else {
        return 0;
    }
}


static void
initconfig_set_error(PyInitConfig *config, const char *err_msg)
{
    config->status = _PyStatus_ERR(err_msg);
}


static const PyConfigSpec*
initconfig_find_spec(const PyConfigSpec *spec, const char *name)
{
    for (; spec->name != NULL; spec++) {
        if (strcmp(name, spec->name) == 0) {
            return spec;
        }
    }
    return NULL;
}


int
PyInitConfig_HasOption(PyInitConfig *config, const char *name)
{
    const PyConfigSpec *spec = initconfig_find_spec(PYCONFIG_SPEC, name);
    if (spec == NULL) {
        spec = initconfig_find_spec(PYPRECONFIG_SPEC, name);
    }
    return (spec != NULL);
}


static const PyConfigSpec*
initconfig_prepare(PyInitConfig *config, const char *name, void **raw_member)
{
    const PyConfigSpec *spec = initconfig_find_spec(PYCONFIG_SPEC, name);
    if (spec != NULL) {
        *raw_member = config_get_spec_member(&config->config, spec);
        return spec;
    }

    spec = initconfig_find_spec(PYPRECONFIG_SPEC, name);
    if (spec != NULL) {
        *raw_member = preconfig_get_spec_member(&config->preconfig, spec);
        return spec;
    }

    initconfig_set_error(config, "unknown config option name");
    return NULL;
}


int
PyInitConfig_GetInt(PyInitConfig *config, const char *name, int64_t *value)
{
    void *raw_member;
    const PyConfigSpec *spec = initconfig_prepare(config, name, &raw_member);
    if (spec == NULL) {
        return -1;
    }

    switch (spec->type) {
    case PyConfig_MEMBER_INT:
    case PyConfig_MEMBER_UINT:
    case PyConfig_MEMBER_BOOL:
    {
        int *member = raw_member;
        *value = *member;
        break;
    }

    case PyConfig_MEMBER_ULONG:
    {
        unsigned long *member = raw_member;
#if SIZEOF_LONG >= 8
        if ((unsigned long)INT64_MAX < *member) {
            initconfig_set_error(config,
                "config option value doesn't fit into int64_t");
            return -1;
        }
#endif
        *value = *member;
        break;
    }

    default:
        initconfig_set_error(config, "config option type is not int");
        return -1;
    }
    return 0;
}


static char*
wstr_to_utf8(PyInitConfig *config, wchar_t *wstr)
{
    char *utf8;
    int res = _Py_EncodeUTF8Ex(wstr, &utf8, NULL, NULL, 1, _Py_ERROR_STRICT);
    if (res == -2) {
        initconfig_set_error(config, "encoding error");
        return NULL;
    }
    if (res < 0) {
        config->status = _PyStatus_NO_MEMORY();
        return NULL;
    }

    // Copy to use the malloc() memory allocator
    size_t size = strlen(utf8) + 1;
    char *str = malloc(size);
    if (str == NULL) {
        PyMem_RawFree(utf8);
        config->status = _PyStatus_NO_MEMORY();
        return NULL;
    }

    memcpy(str, utf8, size);
    PyMem_RawFree(utf8);
    return str;
}


int
PyInitConfig_GetStr(PyInitConfig *config, const char *name, char **value)
{
    void *raw_member;
    const PyConfigSpec *spec = initconfig_prepare(config, name, &raw_member);
    if (spec == NULL) {
        return -1;
    }

    if (spec->type != PyConfig_MEMBER_WSTR
        && spec->type != PyConfig_MEMBER_WSTR_OPT)
    {
        initconfig_set_error(config, "config option type is not string");
        return -1;
    }

    wchar_t **member = raw_member;
    if (*member == NULL) {
        *value = NULL;
        return 0;
    }

    *value = wstr_to_utf8(config, *member);
    if (*value == NULL) {
        return -1;
    }
    return 0;
}


int
PyInitConfig_GetStrList(PyInitConfig *config, const char *name, size_t *length, char ***items)
{
    void *raw_member;
    const PyConfigSpec *spec = initconfig_prepare(config, name, &raw_member);
    if (spec == NULL) {
        return -1;
    }

    if (spec->type != PyConfig_MEMBER_WSTR_LIST) {
        initconfig_set_error(config, "config option type is not string list");
        return -1;
    }

    PyWideStringList *list = raw_member;
    *length = list->length;

    *items = malloc(list->length * sizeof(char*));
    if (*items == NULL) {
        config->status = _PyStatus_NO_MEMORY();
        return -1;
    }

    for (Py_ssize_t i=0; i < list->length; i++) {
        (*items)[i] = wstr_to_utf8(config, list->items[i]);
        if ((*items)[i] == NULL) {
            PyInitConfig_FreeStrList(i, *items);
            return -1;
        }
    }
    return 0;
}


void
PyInitConfig_FreeStrList(size_t length, char **items)
{
    for (size_t i=0; i < length; i++) {
        free(items[i]);
    }
    free(items);
}


int
PyInitConfig_SetInt(PyInitConfig *config, const char *name, int64_t value)
{
    void *raw_member;
    const PyConfigSpec *spec = initconfig_prepare(config, name, &raw_member);
    if (spec == NULL) {
        return -1;
    }

    switch (spec->type) {
    case PyConfig_MEMBER_INT:
    {
        if (value < (int64_t)INT_MIN || (int64_t)INT_MAX < value) {
            initconfig_set_error(config,
                "config option value is out of int range");
            return -1;
        }
        int int_value = (int)value;

        int *member = raw_member;
        *member = int_value;
        break;
    }

    case PyConfig_MEMBER_UINT:
    case PyConfig_MEMBER_BOOL:
    {
        if (value < 0 || (uint64_t)UINT_MAX < (uint64_t)value) {
            initconfig_set_error(config,
                "config option value is out of unsigned int range");
            return -1;
        }
        int int_value = (int)value;

        int *member = raw_member;
        *member = int_value;
        break;
    }

    case PyConfig_MEMBER_ULONG:
    {
        if (value < 0 || (uint64_t)ULONG_MAX < (uint64_t)value) {
            initconfig_set_error(config,
                "config option value is out of unsigned long range");
            return -1;
        }
        unsigned long ulong_value = (unsigned long)value;

        unsigned long *member = raw_member;
        *member = ulong_value;
        break;
    }

    default:
        initconfig_set_error(config, "config option type is not int");
        return -1;
    }

    if (strcmp(name, "hash_seed") == 0) {
        config->config.use_hash_seed = 1;
    }

    return 0;
}


static wchar_t*
utf8_to_wstr(PyInitConfig *config, const char *str)
{
    wchar_t *wstr;
    size_t wlen;
    int res = _Py_DecodeUTF8Ex(str, strlen(str), &wstr, &wlen, NULL, _Py_ERROR_STRICT);
    if (res == -2) {
        initconfig_set_error(config, "decoding error");
        return NULL;
    }
    if (res < 0) {
        config->status = _PyStatus_NO_MEMORY();
        return NULL;
    }

    // Copy to use the malloc() memory allocator
    size_t size = (wlen + 1) * sizeof(wchar_t);
    wchar_t *wstr2 = malloc(size);
    if (wstr2 == NULL) {
        PyMem_RawFree(wstr);
        config->status = _PyStatus_NO_MEMORY();
        return NULL;
    }

    memcpy(wstr2, wstr, size);
    PyMem_RawFree(wstr);
    return wstr2;
}


int
PyInitConfig_SetStr(PyInitConfig *config, const char *name, const char* value)
{
    void *raw_member;
    const PyConfigSpec *spec = initconfig_prepare(config, name, &raw_member);
    if (spec == NULL) {
        return -1;
    }

    if (spec->type != PyConfig_MEMBER_WSTR
            && spec->type != PyConfig_MEMBER_WSTR_OPT) {
        initconfig_set_error(config, "config option type is not string");
        return -1;
    }

    if (value == NULL && spec->type != PyConfig_MEMBER_WSTR_OPT) {
        initconfig_set_error(config, "config option string cannot be NULL");
    }

    wchar_t **member = raw_member;

    *member = utf8_to_wstr(config, value);
    if (*member == NULL) {
        return -1;
    }
    return 0;
}


static void
initconfig_free_wstr(wchar_t *member)
{
    if (member) {
        free(member);
    }
}


static void
initconfig_free_wstr_list(PyWideStringList *list)
{
    for (Py_ssize_t i = 0; i < list->length; i++) {
        free(list->items[i]);
    }
    free(list->items);
}


static void
initconfig_free_config(const PyConfig *config)
{
    const PyConfigSpec *spec = PYCONFIG_SPEC;
    for (; spec->name != NULL; spec++) {
        void *member = config_get_spec_member(config, spec);
        if (spec->type == PyConfig_MEMBER_WSTR
            || spec->type == PyConfig_MEMBER_WSTR_OPT)
        {
            wchar_t *wstr = *(wchar_t **)member;
            initconfig_free_wstr(wstr);
        }
        else if (spec->type == PyConfig_MEMBER_WSTR_LIST) {
            initconfig_free_wstr_list(member);
        }
    }
}


static int
initconfig_set_str_list(PyInitConfig *config, PyWideStringList *list,
                        Py_ssize_t length, char * const *items)
{
    PyWideStringList wlist = _PyWideStringList_INIT;
    size_t size = sizeof(wchar_t*) * length;
    wlist.items = (wchar_t **)malloc(size);
    if (wlist.items == NULL) {
        config->status = _PyStatus_NO_MEMORY();
        return -1;
    }

    for (Py_ssize_t i = 0; i < length; i++) {
        wchar_t *arg = utf8_to_wstr(config, items[i]);
        if (arg == NULL) {
            initconfig_free_wstr_list(&wlist);
            return -1;
        }
        wlist.items[i] = arg;
        wlist.length++;
    }

    initconfig_free_wstr_list(list);
    *list = wlist;
    return 0;
}


int
PyInitConfig_SetStrList(PyInitConfig *config, const char *name,
                        size_t length, char * const *items)
{
    void *raw_member;
    const PyConfigSpec *spec = initconfig_prepare(config, name, &raw_member);
    if (spec == NULL) {
        return -1;
    }

    if (spec->type != PyConfig_MEMBER_WSTR_LIST) {
        initconfig_set_error(config, "config option type is not strings list");
        return -1;
    }
    PyWideStringList *list = raw_member;
    if (initconfig_set_str_list(config, list, length, items) < 0) {
        return -1;
    }

    if (strcmp(name, "module_search_paths") == 0) {
        config->config.module_search_paths_set = 1;
    }
    return 0;
}


int
PyInitConfig_AddModule(PyInitConfig *config, const char *name,
                       PyObject* (*initfunc)(void))
{
    size_t size = sizeof(struct _inittab) * (config->inittab_size + 2);
    struct _inittab *new_inittab = PyMem_RawRealloc(config->inittab, size);
    if (new_inittab == NULL) {
        config->status = _PyStatus_NO_MEMORY();
        return -1;
    }
    config->inittab = new_inittab;

    struct _inittab *entry = &config->inittab[config->inittab_size];
    entry->name = name;
    entry->initfunc = initfunc;

    // Terminator entry
    entry = &config->inittab[config->inittab_size + 1];
    entry->name = NULL;
    entry->initfunc = NULL;

    config->inittab_size++;
    return 0;
}


int
Py_InitializeFromInitConfig(PyInitConfig *config)
{
    if (config->inittab_size >= 1) {
        if (PyImport_ExtendInittab(config->inittab) < 0) {
            config->status = _PyStatus_NO_MEMORY();
            return -1;
        }
    }

    _PyPreConfig_GetConfig(&config->preconfig, &config->config);

    config->status = Py_PreInitializeFromArgs(
        &config->preconfig,
        config->config.argv.length,
        config->config.argv.items);
    if (_PyStatus_EXCEPTION(config->status)) {
        return -1;
    }

    config->status = Py_InitializeFromConfig(&config->config);
    if (_PyStatus_EXCEPTION(config->status)) {
        return -1;
    }

    return 0;
}


// --- PyConfig_Get() -------------------------------------------------------

static const PyConfigSpec*
config_generic_find_spec(const PyConfigSpec *spec, const char *name)
{
    for (; spec->name != NULL; spec++) {
        if (spec->visibility == PyConfig_MEMBER_INIT_ONLY) {
            continue;
        }
        if (strcmp(name, spec->name) == 0) {
            return spec;
        }
    }
    return NULL;
}


static const PyConfigSpec*
config_find_spec(const char *name)
{
    return config_generic_find_spec(PYCONFIG_SPEC, name);
}


static const PyConfigSpec*
preconfig_find_spec(const char *name)
{
    return config_generic_find_spec(PYPRECONFIG_SPEC, name);
}


static int
config_add_xoption(PyObject *dict, const wchar_t *str)
{
    PyObject *name = NULL, *value = NULL;

    const wchar_t *name_end = wcschr(str, L'=');
    if (!name_end) {
        name = PyUnicode_FromWideChar(str, -1);
        if (name == NULL) {
            goto error;
        }
        value = Py_NewRef(Py_True);
    }
    else {
        name = PyUnicode_FromWideChar(str, name_end - str);
        if (name == NULL) {
            goto error;
        }
        value = PyUnicode_FromWideChar(name_end + 1, -1);
        if (value == NULL) {
            goto error;
        }
    }
    if (PyDict_SetItem(dict, name, value) < 0) {
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


PyObject*
_PyConfig_CreateXOptionsDict(const PyConfig *config)
{
    PyObject *dict = PyDict_New();
    if (dict == NULL) {
        return NULL;
    }

    Py_ssize_t nxoption = config->xoptions.length;
    wchar_t **xoptions = config->xoptions.items;
    for (Py_ssize_t i=0; i < nxoption; i++) {
        const wchar_t *option = xoptions[i];
        if (config_add_xoption(dict, option) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
    }
    return dict;
}


static int
config_get_sys_write_bytecode(const PyConfig *config, int *value)
{
    PyObject *attr = PySys_GetAttrString("dont_write_bytecode");
    if (attr == NULL) {
        return -1;
    }

    int is_true = PyObject_IsTrue(attr);
    Py_DECREF(attr);
    if (is_true < 0) {
        return -1;
    }
    *value = (!is_true);
    return 0;
}


static PyObject*
config_get(const PyConfig *config, const PyConfigSpec *spec,
           int use_sys)
{
    if (use_sys) {
        if (spec->sys.attr != NULL) {
            return PySys_GetAttrString(spec->sys.attr);
        }

        if (strcmp(spec->name, "write_bytecode") == 0) {
            int value;
            if (config_get_sys_write_bytecode(config, &value) < 0) {
                return NULL;
            }
            return PyBool_FromLong(value);
        }

        if (strcmp(spec->name, "int_max_str_digits") == 0) {
            PyInterpreterState *interp = _PyInterpreterState_GET();
            return PyLong_FromLong(interp->long_state.max_str_digits);
        }
    }

    void *member = config_get_spec_member(config, spec);
    switch (spec->type) {
    case PyConfig_MEMBER_INT:
    case PyConfig_MEMBER_UINT:
    {
        int value = *(int *)member;
        return PyLong_FromLong(value);
    }

    case PyConfig_MEMBER_BOOL:
    {
        int value = *(int *)member;
        return PyBool_FromLong(value != 0);
    }

    case PyConfig_MEMBER_ULONG:
    {
        unsigned long value = *(unsigned long *)member;
        return PyLong_FromUnsignedLong(value);
    }

    case PyConfig_MEMBER_WSTR:
    case PyConfig_MEMBER_WSTR_OPT:
    {
        wchar_t *wstr = *(wchar_t **)member;
        if (wstr != NULL) {
            return PyUnicode_FromWideChar(wstr, -1);
        }
        else {
            return Py_NewRef(Py_None);
        }
    }

    case PyConfig_MEMBER_WSTR_LIST:
    {
        if (strcmp(spec->name, "xoptions") == 0) {
            return _PyConfig_CreateXOptionsDict(config);
        }
        else {
            const PyWideStringList *list = (const PyWideStringList *)member;
            return _PyWideStringList_AsTuple(list);
        }
    }

    default:
        Py_UNREACHABLE();
    }
}


static PyObject*
preconfig_get(const PyPreConfig *preconfig, const PyConfigSpec *spec)
{
    // The type of all PYPRECONFIG_SPEC members is INT or BOOL.
    assert(spec->type == PyConfig_MEMBER_INT
           || spec->type == PyConfig_MEMBER_BOOL);

    char *member = (char *)preconfig + spec->offset;
    int value = *(int *)member;

    if (spec->type == PyConfig_MEMBER_BOOL) {
        return PyBool_FromLong(value != 0);
    }
    else {
        return PyLong_FromLong(value);
    }
}


static void
config_unknown_name_error(const char *name)
{
    PyErr_Format(PyExc_ValueError, "unknown config option name: %s", name);
}


PyObject*
PyConfig_Get(const char *name)
{
    const PyConfigSpec *spec = config_find_spec(name);
    if (spec != NULL) {
        const PyConfig *config = _Py_GetConfig();
        return config_get(config, spec, 1);
    }

    spec = preconfig_find_spec(name);
    if (spec != NULL) {
        const PyPreConfig *preconfig = &_PyRuntime.preconfig;
        return preconfig_get(preconfig, spec);
    }

    config_unknown_name_error(name);
    return NULL;
}


int
PyConfig_GetInt(const char *name, int *value)
{
    assert(!PyErr_Occurred());

    PyObject *obj = PyConfig_Get(name);
    if (obj == NULL) {
        return -1;
    }

    if (!PyLong_Check(obj)) {
        Py_DECREF(obj);
        PyErr_Format(PyExc_TypeError, "config option %s is not an int", name);
        return -1;
    }

    int as_int = PyLong_AsInt(obj);
    Py_DECREF(obj);
    if (as_int == -1 && PyErr_Occurred()) {
        PyErr_Format(PyExc_OverflowError,
                     "config option %s value does not fit into a C int", name);
        return -1;
    }

    *value = as_int;
    return 0;
}


static int
config_names_add(PyObject *names, const PyConfigSpec *spec)
{
    for (; spec->name != NULL; spec++) {
        if (spec->visibility == PyConfig_MEMBER_INIT_ONLY) {
            continue;
        }
        PyObject *name = PyUnicode_FromString(spec->name);
        if (name == NULL) {
            return -1;
        }
        int res = PyList_Append(names, name);
        Py_DECREF(name);
        if (res < 0) {
            return -1;
        }
    }
    return 0;
}


PyObject*
PyConfig_Names(void)
{
    PyObject *names = PyList_New(0);
    if (names == NULL) {
        goto error;
    }

    if (config_names_add(names, PYCONFIG_SPEC) < 0) {
        goto error;
    }
    if (config_names_add(names, PYPRECONFIG_SPEC) < 0) {
        goto error;
    }

    PyObject *frozen = PyFrozenSet_New(names);
    Py_DECREF(names);
    return frozen;

error:
    Py_XDECREF(names);
    return NULL;
}


// --- PyConfig_Set() -------------------------------------------------------

static int
config_set_sys_flag(const PyConfigSpec *spec, int int_value)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyConfig *config = &interp->config;

    if (spec->type == PyConfig_MEMBER_BOOL) {
        if (int_value != 0) {
            // convert values < 0 and values > 1 to 1
            int_value = 1;
        }
    }

    PyObject *value;
    if (spec->sys.flag_setter) {
        value = spec->sys.flag_setter(int_value);
    }
    else {
        value = config_sys_flag_long(int_value);
    }
    if (value == NULL) {
        return -1;
    }

    // Set sys.flags.FLAG
    Py_ssize_t pos = spec->sys.flag_index;
    if (_PySys_SetFlagObj(pos, value) < 0) {
        goto error;
    }

    // Set PyConfig.ATTR
    assert(spec->type == PyConfig_MEMBER_INT
           || spec->type == PyConfig_MEMBER_UINT
           || spec->type == PyConfig_MEMBER_BOOL);
    int *member = config_get_spec_member(config, spec);
    *member = int_value;

    // Set sys.dont_write_bytecode attribute
    if (strcmp(spec->name, "write_bytecode") == 0) {
        if (PySys_SetObject("dont_write_bytecode", value) < 0) {
            goto error;
        }
    }

    Py_DECREF(value);
    return 0;

error:
    Py_DECREF(value);
    return -1;
}


// Set PyConfig.ATTR integer member
static int
config_set_int_attr(const PyConfigSpec *spec, int value)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyConfig *config = &interp->config;
    int *member = config_get_spec_member(config, spec);
    *member = value;
    return 0;
}


int
PyConfig_Set(const char *name, PyObject *value)
{
    if (PySys_Audit("cpython.PyConfig_Set", "sO", name, value) < 0) {
        return -1;
    }

    const PyConfigSpec *spec = config_find_spec(name);
    if (spec == NULL) {
        spec = preconfig_find_spec(name);
        if (spec == NULL) {
            config_unknown_name_error(name);
            return -1;
        }
        assert(spec->visibility != PyConfig_MEMBER_PUBLIC);
    }

    if (spec->visibility != PyConfig_MEMBER_PUBLIC) {
        PyErr_Format(PyExc_ValueError, "cannot set read-only option %s",
                     name);
        return -1;
    }

    int int_value = 0;
    int has_int_value = 0;

    switch (spec->type) {
    case PyConfig_MEMBER_INT:
    case PyConfig_MEMBER_UINT:
    case PyConfig_MEMBER_BOOL:
        if (!PyLong_Check(value)) {
            PyErr_Format(PyExc_TypeError, "expected int or bool, got %T", value);
            return -1;
        }
        int_value = PyLong_AsInt(value);
        if (int_value == -1 && PyErr_Occurred()) {
            return -1;
        }
        if (int_value < 0 && spec->type != PyConfig_MEMBER_INT) {
            PyErr_Format(PyExc_ValueError, "value must be >= 0");
            return -1;
        }
        has_int_value = 1;
        break;

    case PyConfig_MEMBER_ULONG:
        // not implemented: only hash_seed uses this type, and it's read-only
        goto cannot_set;

    case PyConfig_MEMBER_WSTR:
        if (!PyUnicode_CheckExact(value)) {
            PyErr_Format(PyExc_TypeError, "expected str, got %T", value);
            return -1;
        }
        break;

    case PyConfig_MEMBER_WSTR_OPT:
        if (value != Py_None && !PyUnicode_CheckExact(value)) {
            PyErr_Format(PyExc_TypeError, "expected str or None, got %T", value);
            return -1;
        }
        break;

    case PyConfig_MEMBER_WSTR_LIST:
        if (strcmp(spec->name, "xoptions") != 0) {
            if (!PyList_Check(value)) {
                PyErr_Format(PyExc_TypeError, "expected list[str], got %T",
                             value);
                return -1;
            }
            for (Py_ssize_t i=0; i < PyList_GET_SIZE(value); i++) {
                PyObject *item = PyList_GET_ITEM(value, i);
                if (!PyUnicode_Check(item)) {
                    PyErr_Format(PyExc_TypeError,
                                 "expected str, list item %zd has type %T",
                                 i, item);
                    return -1;
                }
            }
        }
        else {
            // xoptions type is dict[str, str]
            if (!PyDict_Check(value)) {
                PyErr_Format(PyExc_TypeError,
                             "expected dict[str, str | bool], got %T",
                             value);
                return -1;
            }

            Py_ssize_t pos = 0;
            PyObject *key, *item;
            while (PyDict_Next(value, &pos, &key, &item)) {
                if (!PyUnicode_Check(key)) {
                    PyErr_Format(PyExc_TypeError,
                                 "expected str, "
                                 "got dict key type %T", key);
                    return -1;
                }
                if (!PyUnicode_Check(item) && !PyBool_Check(item)) {
                    PyErr_Format(PyExc_TypeError,
                                 "expected str or bool, "
                                 "got dict value type %T", key);
                    return -1;
                }
            }
        }
        break;

    default:
        Py_UNREACHABLE();
    }

    if (spec->sys.attr != NULL) {
        // Set the sys attribute, but don't set PyInterpreterState.config
        // to keep the code simple.
        return PySys_SetObject(spec->sys.attr, value);
    }
    else if (has_int_value) {
        if (spec->sys.flag_index >= 0) {
            return config_set_sys_flag(spec, int_value);
        }
        else if (strcmp(spec->name, "int_max_str_digits") == 0) {
            return _PySys_SetIntMaxStrDigits(int_value);
        }
        else {
            return config_set_int_attr(spec, int_value);
        }
    }

cannot_set:
    PyErr_Format(PyExc_ValueError, "cannot set option %s", name);
    return -1;
}
