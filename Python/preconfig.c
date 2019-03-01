#include "Python.h"
#include "pycore_coreconfig.h"


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
