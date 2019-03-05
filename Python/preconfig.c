#include "Python.h"
#include "pycore_coreconfig.h"
#include "pycore_getopt.h"


#define DECODE_LOCALE_ERR(NAME, LEN) \
    (((LEN) == -2) \
     ? _Py_INIT_USER_ERR("cannot decode " NAME) \
     : _Py_INIT_NO_MEMORY())


/* --- File system encoding/errors -------------------------------- */

/* The filesystem encoding is chosen by config_init_fs_encoding(),
   see also initfsencoding(). */
const char *Py_FileSystemDefaultEncoding = NULL;
int Py_HasFileSystemDefaultEncoding = 0;
const char *Py_FileSystemDefaultEncodeErrors = NULL;
int _Py_HasFileSystemDefaultEncodeErrors = 0;

void
_Py_ClearFileSystemEncoding(void)
{
    if (!Py_HasFileSystemDefaultEncoding && Py_FileSystemDefaultEncoding) {
        PyMem_RawFree((char*)Py_FileSystemDefaultEncoding);
        Py_FileSystemDefaultEncoding = NULL;
    }
    if (!_Py_HasFileSystemDefaultEncodeErrors && Py_FileSystemDefaultEncodeErrors) {
        PyMem_RawFree((char*)Py_FileSystemDefaultEncodeErrors);
        Py_FileSystemDefaultEncodeErrors = NULL;
    }
}


/* Set Py_FileSystemDefaultEncoding and Py_FileSystemDefaultEncodeErrors
   global configuration variables. */
int
_Py_SetFileSystemEncoding(const char *encoding, const char *errors)
{
    char *encoding2 = _PyMem_RawStrdup(encoding);
    if (encoding2 == NULL) {
        return -1;
    }

    char *errors2 = _PyMem_RawStrdup(errors);
    if (errors2 == NULL) {
        PyMem_RawFree(encoding2);
        return -1;
    }

    _Py_ClearFileSystemEncoding();

    Py_FileSystemDefaultEncoding = encoding2;
    Py_HasFileSystemDefaultEncoding = 0;

    Py_FileSystemDefaultEncodeErrors = errors2;
    _Py_HasFileSystemDefaultEncodeErrors = 0;
    return 0;
}


/* --- _PyArgv ---------------------------------------------------- */

_PyInitError
_PyArgv_Decode(const _PyArgv *args, wchar_t*** argv_p)
{
    wchar_t** argv;
    if (args->use_bytes_argv) {
        /* +1 for a the NULL terminator */
        size_t size = sizeof(wchar_t*) * (args->argc + 1);
        argv = (wchar_t **)PyMem_RawMalloc(size);
        if (argv == NULL) {
            return _Py_INIT_NO_MEMORY();
        }

        for (int i = 0; i < args->argc; i++) {
            size_t len;
            wchar_t *arg = Py_DecodeLocale(args->bytes_argv[i], &len);
            if (arg == NULL) {
                _Py_wstrlist_clear(i, argv);
                return DECODE_LOCALE_ERR("command line arguments",
                                         (Py_ssize_t)len);
            }
            argv[i] = arg;
        }
        argv[args->argc] = NULL;
    }
    else {
        argv = args->wchar_argv;
    }
    *argv_p = argv;
    return _Py_INIT_OK();
}


/* --- _PyPreCmdline ------------------------------------------------- */

typedef struct {
    const _PyArgv *args;
    int argc;
    wchar_t **argv;
} _PyPreCmdline;


static void
precmdline_clear(_PyPreCmdline *cmdline)
{
    if (cmdline->args->use_bytes_argv && cmdline->argv != NULL) {
        _Py_wstrlist_clear(cmdline->args->argc, cmdline->argv);
    }
    cmdline->argv = NULL;
}


/* --- _PyPreConfig ----------------------------------------------- */

void
_PyPreConfig_Clear(_PyPreConfig *config)
{
}


int
_PyPreConfig_Copy(_PyPreConfig *config, const _PyPreConfig *config2)
{
    _PyPreConfig_Clear(config);

#define COPY_ATTR(ATTR) config->ATTR = config2->ATTR

    COPY_ATTR(isolated);
    COPY_ATTR(use_environment);

#undef COPY_ATTR
    return 0;
}


void
_PyPreConfig_GetGlobalConfig(_PyPreConfig *config)
{
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

#undef COPY_FLAG
#undef COPY_NOT_FLAG
}


void
_PyPreConfig_SetGlobalConfig(const _PyPreConfig *config)
{
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

#undef COPY_FLAG
#undef COPY_NOT_FLAG
}


_PyInitError
_PyPreConfig_Read(_PyPreConfig *config)
{
    _PyPreConfig_GetGlobalConfig(config);

    if (config->isolated > 0) {
        config->use_environment = 0;
    }

    /* Default values */
    if (config->use_environment < 0) {
        config->use_environment = 0;
    }

    assert(config->isolated >= 0);
    assert(config->use_environment >= 0);

    return _Py_INIT_OK();
}


int
_PyPreConfig_AsDict(const _PyPreConfig *config, PyObject *dict)
{
#define SET_ITEM(KEY, EXPR) \
        do { \
            PyObject *obj = (EXPR); \
            if (obj == NULL) { \
                goto fail; \
            } \
            int res = PyDict_SetItemString(dict, (KEY), obj); \
            Py_DECREF(obj); \
            if (res < 0) { \
                goto fail; \
            } \
        } while (0)
#define SET_ITEM_INT(ATTR) \
    SET_ITEM(#ATTR, PyLong_FromLong(config->ATTR))

    SET_ITEM_INT(isolated);
    SET_ITEM_INT(use_environment);
    return 0;

fail:
    return -1;

#undef SET_ITEM
#undef SET_ITEM_INT
}


/* Parse the command line arguments */
static _PyInitError
preconfig_parse_cmdline(_PyPreConfig *config, _PyPreCmdline *cmdline)
{
    _PyOS_ResetGetOpt();
    /* Don't log parsing errors into stderr here: _PyCoreConfig_ReadFromArgv()
       is responsible for that */
    _PyOS_opterr = 0;
    do {
        int longindex = -1;
        int c = _PyOS_GetOpt(cmdline->args->argc, cmdline->argv, &longindex);

        if (c == EOF || c == 'c' || c == 'm') {
            break;
        }

        switch (c) {
        case 'E':
            config->use_environment = 0;
            break;

        case 'I':
            config->isolated++;
            break;

        default:
            /* ignore other argument:
               handled by _PyCoreConfig_ReadFromArgv() */
            break;
        }
    } while (1);

    return _Py_INIT_OK();
}


_PyInitError
_PyPreConfig_ReadFromArgv(_PyPreConfig *config, const _PyArgv *args)
{
    _PyInitError err;

    _PyPreCmdline cmdline;
    memset(&cmdline, 0, sizeof(cmdline));
    cmdline.args = args;

    err = _PyArgv_Decode(cmdline.args, &cmdline.argv);
    if (_Py_INIT_FAILED(err)) {
        goto done;
    }

    err = preconfig_parse_cmdline(config, &cmdline);
    if (_Py_INIT_FAILED(err)) {
        goto done;
    }

    err = _PyPreConfig_Read(config);
    if (_Py_INIT_FAILED(err)) {
        goto done;
    }
    err = _Py_INIT_OK();

done:
    precmdline_clear(&cmdline);
    return err;
}


void
_PyPreConfig_Write(const _PyPreConfig *config)
{
}
