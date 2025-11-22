// _sysconfig provides data for the Python sysconfig module

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

#include "pycore_importdl.h"   // _PyImport_DynLoadFiletab
#include "pycore_long.h"       // _PyLong_GetZero, _PyLong_GetOne


/*[clinic input]
module _sysconfig
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=0a7c02d3e212ac97]*/

#include "clinic/_sysconfig.c.h"

#ifdef MS_WINDOWS
static int
add_string_value(PyObject *dict, const char *key, const char *str_value)
{
    PyObject *value = PyUnicode_FromString(str_value);
    if (value == NULL) {
        return -1;
    }
    int err = PyDict_SetItemString(dict, key, value);
    Py_DECREF(value);
    return err;
}
#endif

/*[clinic input]
@permit_long_summary
_sysconfig.config_vars

Returns a dictionary containing build variables intended to be exposed by sysconfig.
[clinic start generated code]*/

static PyObject *
_sysconfig_config_vars_impl(PyObject *module)
/*[clinic end generated code: output=9c41cdee63ea9487 input=fdda9cab12ca19fe]*/
{
    PyObject *config = PyDict_New();
    if (config == NULL) {
        return NULL;
    }

#ifdef MS_WINDOWS
    if (add_string_value(config, "EXT_SUFFIX", PYD_TAGGED_SUFFIX) < 0) {
        Py_DECREF(config);
        return NULL;
    }
    if (add_string_value(config, "SOABI", PYD_SOABI) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "HAVE_IO_H",
#   ifdef HAVE_IO_H
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
        ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "HAVE_SYS_UTIME_H",
#   ifdef HAVE_SYS_UTIME_H
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
        ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "HAVE_TEMPNAM",
#   ifdef HAVE_TEMPNAM
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "HAVE_TMPNAM",
#   ifdef HAVE_TMPNAM
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "HAVE_STRERROR",
#   ifdef HAVE_STRERROR
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "DONT_HAVE_SIG_ALARM",
#   ifdef DONT_HAVE_SIG_ALARM
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "DONT_HAVE_SIG_PAUSE",
#   ifdef DONT_HAVE_SIG_PAUSE
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

#   if defined(LONG_BIT)
#   ifndef SIZEOF_LONG
#   define SIZEOF_LONG sizeof(long)
#   endif
    if (PyDict_SetItemString(config, "LONG_BIT", PyLong_FromLong(LONG_BIT)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef WORD_BIT
    if (PyDict_SetItemString(config, "WORD_BIT", PyLong_FromLong(WORD_BIT)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

    if (PyDict_SetItemString(config, "NT_THREADS",
#   ifdef NT_THREADS
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "WITH_THREAD",
#   ifdef WITH_THREAD
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "NETSCAPE_PI",
#   ifdef NETSCAPE_PI
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "USE_SOCKET",
#   ifdef USE_SOCKET
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }


    if (PyDict_SetItemString(config, "HAVE_WINDOWS_CONSOLE_IO",
#   ifdef HAVE_WINDOWS_CONSOLE_IO
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

#   ifdef PYD_PLATFORM_TAG
    if (PyDict_SetItemString(config, "PYD_PLATFORM_TAG", PyUnicode_FromString(PYD_PLATFORM_TAG)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef PY_SSIZE_T_MAX
    if (PyDict_SetItemString(config, "PY_SSIZE_T_MAX", PyLong_FromLongLong(PY_SSIZE_T_MAX)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

    if (PyDict_SetItemString(config, "HAVE_PY_SSIZE_T",
#   ifdef HAVE_PY_SSIZE_T
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

#   ifdef PY_LLONG_MIN
    if (PyDict_SetItemString(config, "PY_LLONG_MIN", PyLong_FromLongLong(PY_LLONG_MIN)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef PY_LLONG_MAX
    if (PyDict_SetItemString(config, "PY_LLONG_MAX", PyLong_FromLongLong(PY_LLONG_MAX)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef PY_ULLONG_MAX
    if (PyDict_SetItemString(config, "PY_ULLONG_MAX", PyLong_FromUnsignedLongLong(PY_ULLONG_MAX)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

    if (PyDict_SetItemString(config, "Py_ENABLE_SHARED",
#   ifdef Py_ENABLE_SHARED
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "HAVE_DECLSPEC_DLL",
#   ifdef HAVE_DECLSPEC_DLL
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

#   ifdef PLATFORM
    if (PyDict_SetItemString(config, "PLATFORM", PyUnicode_FromString(PLATFORM)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef SIZEOF_HKEY
    if (PyDict_SetItemString(config, "SIZEOF_HKEY", PyLong_FromLong(SIZEOF_HKEY)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

    if (PyDict_SetItemString(config, "HAVE_LARGEFILE_SUPPORT",
#   ifdef HAVE_LARGEFILE_SUPPORT
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "HAVE_UINTPTR_T",
#   ifdef HAVE_UINTPTR_T
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "HAVE_INTPTR_T",
#   ifdef HAVE_INTPTR_T
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "HAVE_DIRECT_H",
#   ifdef HAVE_DIRECT_H
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "HAVE_DECL_TZNAME",
#   ifdef HAVE_DECL_TZNAME
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "HAVE_TZNAME",
#   ifdef HAVE_TZNAME
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "SYSDIR",
#   ifdef SYSDIR
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "SYSNDIR",
#   ifdef SYSNDIR
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "TIME_WITH_SYS_TIME",
#   ifdef TIME_WITH_SYS_TIME
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "TM_IN_SYS_TIME",
#   ifdef TM_IN_SYS_TIME
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "VOID_CLOSEDIR",
#   ifdef VOID_CLOSEDIR
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "GETPGRP_HAVE_ARGS",
#   ifdef GETPGRP_HAVE_ARGS
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "HAVE_PUTENV",
#   ifdef HAVE_PUTENV
        _PyLong_GetOne()
#   else
        _PyLong_GetZero()
#   endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

/*The writer "Locked-chess-official" is to lazy.
 *He didn't want to change the format after this line*/

#   ifdef WITH_READLINE
    if (PyDict_SetItemString(config, "WITH_READLINE", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "WITH_READLINE", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_GETTIMEOFDAY
    if (PyDict_SetItemString(config, "HAVE_GETTIMEOFDAY", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_GETTIMEOFDAY", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_PROCESS_H
    if (PyDict_SetItemString(config, "HAVE_PROCESS_H", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_PROCESS_H", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_STDDEF_H
    if (PyDict_SetItemString(config, "HAVE_STDDEF_H", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_STDDEF_H", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_SYS_AUDIOIO_H
    if (PyDict_SetItemString(config, "HAVE_SYS_AUDIOIO_H", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_SYS_AUDIOIO_H", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_LIBMPC
    if (PyDict_SetItemString(config, "HAVE_LIBMPC", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_LIBMPC", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_LIBSEQ
    if (PyDict_SetItemString(config, "HAVE_LIBSEQ", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_LIBSEQ", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_LIBSOCKET
    if (PyDict_SetItemString(config, "HAVE_LIBSOCKET", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_LIBSOCKET", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_LIBSUN
    if (PyDict_SetItemString(config, "HAVE_LIBSUN", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_LIBSUN", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_LIBTERMCAP
    if (PyDict_SetItemString(config, "HAVE_LIBTERMCAP", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_LIBTERMCAP", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_LIBTERMLIB
    if (PyDict_SetItemString(config, "HAVE_LIBTERMLIB", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_LIBTERMLIB", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_LIBTHREAD
    if (PyDict_SetItemString(config, "HAVE_LIBTHREAD", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_LIBTHREAD", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef Py_SOCKET_FD_CAN_BE_GE_FD_SETSIZE
    if (PyDict_SetItemString(config, "Py_SOCKET_FD_CAN_BE_GE_FD_SETSIZE", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "Py_SOCKET_FD_CAN_BE_GE_FD_SETSIZE", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_X509_VERIFY_PARAM_SET1_HOST
    if (PyDict_SetItemString(config, "HAVE_X509_VERIFY_PARAM_SET1_HOST", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_X509_VERIFY_PARAM_SET1_HOST", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif
#else
#   ifdef AC_APPLE_UNIVERSAL_BUILD
    if (PyDict_SetItemString(config, "AC_APPLE_UNIVERSAL_BUILD", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "AC_APPLE_UNIVERSAL_BUILD", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef AIX_BUILDDATE
    if (PyDict_SetItemString(config, "AIX_BUILDDATE", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "AIX_BUILDDATE", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef AIX_GENUINE_CPLUSPLUS
    if (PyDict_SetItemString(config, "AIX_GENUINE_CPLUSPLUS", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "AIX_GENUINE_CPLUSPLUS", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef ALIGNOF_LONG
    if (PyDict_SetItemString(config, "ALIGNOF_LONG", PyLong_FromLong(ALIGNOF_LONG)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef ALT_SOABI
    if (PyDict_SetItemString(config, "ALT_SOABI", PyUnicode_FromString(ALT_SOABI)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef ANDROID_API_LEVEL
    if (PyDict_SetItemString(config, "ANDROID_API_LEVEL", PyLong_FromLong(ANDROID_API_LEVEL)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef ENABLE_IPV6
    if (PyDict_SetItemString(config, "ENABLE_IPV6", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "ENABLE_IPV6", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef GETPGRP_HAVE_ARG
    if (PyDict_SetItemString(config, "GETPGRP_HAVE_ARG", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "GETPGRP_HAVE_ARG", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_ACCEPT4
    if (PyDict_SetItemString(config, "HAVE_ACCEPT4", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_ACCEPT4", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_ACOSH
    if (PyDict_SetItemString(config, "HAVE_ACOSH", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_ACOSH", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_ADDRINFO
    if (PyDict_SetItemString(config, "HAVE_ADDRINFO", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_ADDRINFO", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_ALARM
    if (PyDict_SetItemString(config, "HAVE_ALARM", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_ALARM", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_ALIGNED_REQUIRED
    if (PyDict_SetItemString(config, "PyDict_SetItemString", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "PyDict_SetItemString", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_ALLOCA_H
    if (PyDict_SetItemString(config, "HAVE_ALLOCA_H", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_ALLOCA_H", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_ASM_TYPES_H
    if (PyDict_SetItemString(config, "HAVE_ASM_TYPES_H", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_ASM_TYPES_H", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_ATANH
    if (PyDict_SetItemString(config, "HAVE_ATANH", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_ATANH", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_BACKTRACE
    if (PyDict_SetItemString(config, "HAVE_BACKTRACE", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_BACKTRACE", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_BIND_TEXTDOMAIN_CODESET
    if (PyDict_SetItemString(config, "HAVE_BIND_TEXTDOMAIN_CODESET", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_BIND_TEXTDOMAIN_CODESET", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_BLUETOOTH_BLUETOOTH_H
    if (PyDict_SetItemString(config, "HAVE_BLUETOOTH_BLUETOOTH_H", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_BLUETOOTH_BLUETOOTH_H", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_BLUETOOTH_H
    if (PyDict_SetItemString(config, "HAVE_BLUETOOTH_H", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_BLUETOOTH_H", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_BROKEN_MBSTOWCS
    if (PyDict_SetItemString(config, "HAVE_BROKEN_MBSTOWCS", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_BROKEN_MBSTOWCS", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_BROKEN_NICE
    if (PyDict_SetItemString(config, "HAVE_BROKEN_NICE", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_BROKEN_NICE", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_BROKEN_PIPE_BUF
    if (PyDict_SetItemString(config, "HAVE_BROKEN_PIPE_BUF", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_BROKEN_PIPE_BUF", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_BROKEN_POLL
    if (PyDict_SetItemString(config, "HAVE_BROKEN_POLL", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_BROKEN_POLL", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_BROKEN_POSIX_SEMAPHORES
    if (PyDict_SetItemString(config, "HAVE_BROKEN_POSIX_SEMAPHORES", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_BROKEN_POSIX_SEMAPHORES", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_BROKEN_PTHREAD_SIGMASK
    if (PyDict_SetItemString(config, "HAVE_BROKEN_PTHREAD_SIGMASK", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_BROKEN_PTHREAD_SIGMASK", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_BROKEN_SEM_GETVALUE
    if (PyDict_SetItemString(config, "HAVE_BROKEN_SEM_GETVALUE", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_BROKEN_SEM_GETVALUE", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_BROKEN_UNSETENV
    if (PyDict_SetItemString(config, "HAVE_BROKEN_UNSETENV", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_BROKEN_UNSETENV", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_BUILTIN_ATOMIC
    if (PyDict_SetItemString(config, "HAVE_BUILTIN_ATOMIC", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_BUILTIN_ATOMIC", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_BZLIB_H
    if (PyDict_SetItemString(config, "HAVE_BZLIB_H", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_BZLIB_H", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CHFLAGS
    if (PyDict_SetItemString(config, "HAVE_CHFLAGS", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CHFLAGS", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CHMOD
    if (PyDict_SetItemString(config, "HAVE_CHMOD", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CHMOD", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CHOWN
    if (PyDict_SetItemString(config, "HAVE_CHOWN", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CHOWN", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CHROOT
    if (PyDict_SetItemString(config, "HAVE_CHROOT", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CHROOT", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CLEARENV
    if (PyDict_SetItemString(config, "HAVE_CLEARENV", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CLEARENV", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CLOCK_GETRES
    if (PyDict_SetItemString(config, "HAVE_CLOCK_GETRES", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CLOCK_GETRES", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CLOCK_GETTIME
    if (PyDict_SetItemString(config, "HAVE_CLOCK_GETTIME", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CLOCK_GETTIME", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CLOCK_NANOSLEEP
    if (PyDict_SetItemString(config, "HAVE_CLOCK_NANOSLEEP", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CLOCK_NANOSLEEP", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CLOCK_SETTIME
    if (PyDict_SetItemString(config, "HAVE_CLOCK_SETTIME", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CLOCK_SETTIME", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CLOCK_T
    if (PyDict_SetItemString(config, "HAVE_CLOCK_T", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CLOCK_T", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CLOSEFROM
    if (PyDict_SetItemString(config, "HAVE_CLOSEFROM", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CLOSEFROM", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CLOSE_RANGE
    if (PyDict_SetItemString(config, "HAVE_CLOSE_RANGE", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CLOSE_RANGE", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_COMPUTED_GOTOS
    if (PyDict_SetItemString(config, "HAVE_COMPUTED_GOTOS", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_COMPUTED_GOTOS", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CONFSTR
    if (PyDict_SetItemString(config, "HAVE_CONFSTR", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CONFSTR", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_COPY_FILE_RANGE
    if (PyDict_SetItemString(config, "HAVE_COPY_FILE_RANGE", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_COPY_FILE_RANGE", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CTERMID
    if (PyDict_SetItemString(config, "HAVE_CTERMID", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CTERMID", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CTERMID_R
    if (PyDict_SetItemString(config, "HAVE_CTERMID_R", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CTERMID_R", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CURSES_FILTER
    if (PyDict_SetItemString(config, "HAVE_CURSES_FILTER", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CURSES_FILTER", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CURSES_H
    if (PyDict_SetItemString(config, "HAVE_CURSES_H", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CURSES_H", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CURSES_HAS_KEY
    if (PyDict_SetItemString(config, "HAVE_CURSES_HAS_KEY", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CURSES_HAS_KEY", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CURSES_IMMEDOK
    if (PyDict_SetItemString(config, "HAVE_CURSES_IMMEDOK", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CURSES_IMMEDOK", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CURSES_IS_PAD
    if (PyDict_SetItemString(config, "HAVE_CURSES_IS_PAD", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CURSES_IS_PAD", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CURSES_IS_TERM_RESIZED
    if (PyDict_SetItemString(config, "HAVE_CURSES_IS_TERM_RESIZED", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CURSES_IS_TERM_RESIZED", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CURSES_RESIZETERM
    if (PyDict_SetItemString(config, "HAVE_CURSES_RESIZETERM", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CURSES_RESIZETERM", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CURSES_RESIZE_TERM
    if (PyDict_SetItemString(config, "HAVE_CURSES_RESIZE_TERM", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CURSES_RESIZE_TERM", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CURSES_SYNCOK
    if (PyDict_SetItemString(config, "HAVE_CURSES_SYNCOK", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CURSES_SYNCOK", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CURSES_TYPEAHEAD
    if (PyDict_SetItemString(config, "HAVE_CURSES_TYPEAHEAD", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CURSES_TYPEAHEAD", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CURSES_USE_ENV
    if (PyDict_SetItemString(config, "HAVE_CURSES_USE_ENV", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CURSES_USE_ENV", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_CURSES_WCHGAT
    if (PyDict_SetItemString(config, "HAVE_CURSES_WCHGAT", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_CURSES_WCHGAT", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_DB_H
    if (PyDict_SetItemString(config, "HAVE_DB_H", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_DB_H", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_DECL_RTLD_DEEPBIND
    if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_DEEPBIND", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_DEEPBIND", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_DECL_RTLD_GLOBAL
    if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_GLOBAL", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_GLOBAL", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#   ifdef HAVE_DECL_RTLD_LAZY
    if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_LAZY", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   else
    if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_LAZY", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#   endif

#ifdef HAVE_DECL_RTLD_LOCAL
if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_LOCAL", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_LOCAL", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DECL_RTLD_MEMBER
if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_MEMBER", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_MEMBER", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DECL_RTLD_NODELETE
if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_NODELETE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_NODELETE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DECL_RTLD_NOLOAD
if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_NOLOAD", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_NOLOAD", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DECL_RTLD_NOW
if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_NOW", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DECL_RTLD_NOW", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DECL_UT_NAMESIZE
if (PyDict_SetItemString(config, "HAVE_DECL_UT_NAMESIZE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DECL_UT_NAMESIZE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DEVICE_MACROS
if (PyDict_SetItemString(config, "HAVE_DEVICE_MACROS", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DEVICE_MACROS", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DEV_PTMX
if (PyDict_SetItemString(config, "HAVE_DEV_PTMX", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DEV_PTMX", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DIRENT_D_TYPE
if (PyDict_SetItemString(config, "HAVE_DIRENT_D_TYPE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DIRENT_D_TYPE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DIRENT_H
if (PyDict_SetItemString(config, "HAVE_DIRENT_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DIRENT_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DIRFD
if (PyDict_SetItemString(config, "HAVE_DIRFD", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DIRFD", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DLADDR
if (PyDict_SetItemString(config, "HAVE_DLADDR", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DLADDR", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DLADDR1
if (PyDict_SetItemString(config, "HAVE_DLADDR1", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DLADDR1", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DLOPEN
if (PyDict_SetItemString(config, "HAVE_DLOPEN", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DLOPEN", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DUP2
if (PyDict_SetItemString(config, "HAVE_DUP2", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DUP2", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DUP3
if (PyDict_SetItemString(config, "HAVE_DUP3", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DUP3", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_ENDIAN_H
if (PyDict_SetItemString(config, "HAVE_ENDIAN_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_ENDIAN_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_EPOLL
if (PyDict_SetItemString(config, "HAVE_EPOLL", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_EPOLL", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_EPOLL_CREATE1
if (PyDict_SetItemString(config, "HAVE_EPOLL_CREATE1", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_EPOLL_CREATE1", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_EVENTFD
if (PyDict_SetItemString(config, "HAVE_EVENTFD", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_EVENTFD", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_EXECINFO_H
if (PyDict_SetItemString(config, "HAVE_EXECINFO_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_EXECINFO_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_EXECV
if (PyDict_SetItemString(config, "HAVE_EXECV", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_EXECV", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_EXPLICIT_BZERO
if (PyDict_SetItemString(config, "HAVE_EXPLICIT_BZERO", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_EXPLICIT_BZERO", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_EXPM1
if (PyDict_SetItemString(config, "HAVE_EXPM1", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_EXPM1", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FACCESSAT
if (PyDict_SetItemString(config, "HAVE_FACCESSAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FACCESSAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FCHDIR
if (PyDict_SetItemString(config, "HAVE_FCHDIR", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FCHDIR", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FCHMOD
if (PyDict_SetItemString(config, "HAVE_FCHMOD", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FCHMOD", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FCHMODAT
if (PyDict_SetItemString(config, "HAVE_FCHMODAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FCHMODAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FCHOWN
if (PyDict_SetItemString(config, "HAVE_FCHOWN", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FCHOWN", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FCHOWNAT
if (PyDict_SetItemString(config, "HAVE_FCHOWNAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FCHOWNAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FDATASYNC
if (PyDict_SetItemString(config, "HAVE_FDATASYNC", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FDATASYNC", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FDOPENDIR
if (PyDict_SetItemString(config, "HAVE_FDOPENDIR", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FDOPENDIR", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FEXECVE
if (PyDict_SetItemString(config, "HAVE_FEXECVE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FEXECVE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FFI_CLOSURE_ALLOC
if (PyDict_SetItemString(config, "HAVE_FFI_CLOSURE_ALLOC", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FFI_CLOSURE_ALLOC", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FFI_PREP_CIF_VAR
if (PyDict_SetItemString(config, "HAVE_FFI_PREP_CIF_VAR", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FFI_PREP_CIF_VAR", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FFI_PREP_CLOSURE_LOC
if (PyDict_SetItemString(config, "HAVE_FFI_PREP_CLOSURE_LOC", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FFI_PREP_CLOSURE_LOC", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FLOCK
if (PyDict_SetItemString(config, "HAVE_FLOCK", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FLOCK", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FORK
if (PyDict_SetItemString(config, "HAVE_FORK", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FORK", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FORKPTY
if (PyDict_SetItemString(config, "HAVE_FORKPTY", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FORKPTY", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FPATHCONF
if (PyDict_SetItemString(config, "HAVE_FPATHCONF", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FPATHCONF", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FSEEKO
if (PyDict_SetItemString(config, "HAVE_FSEEKO", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FSEEKO", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FSTATAT
if (PyDict_SetItemString(config, "HAVE_FSTATAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FSTATAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FSTATVFS
if (PyDict_SetItemString(config, "HAVE_FSTATVFS", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FSTATVFS", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FSYNC
if (PyDict_SetItemString(config, "HAVE_FSYNC", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FSYNC", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FTELLO
if (PyDict_SetItemString(config, "HAVE_FTELLO", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FTELLO", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FTRUNCATE
if (PyDict_SetItemString(config, "HAVE_FTRUNCATE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FTRUNCATE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FUTIMENS
if (PyDict_SetItemString(config, "HAVE_FUTIMENS", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FUTIMENS", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FUTIMES
if (PyDict_SetItemString(config, "HAVE_FUTIMES", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FUTIMES", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FUTIMESAT
if (PyDict_SetItemString(config, "HAVE_FUTIMESAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FUTIMESAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GAI_STRERROR
if (PyDict_SetItemString(config, "HAVE_GAI_STRERROR", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GAI_STRERROR", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GCC_ASM_FOR_X64
if (PyDict_SetItemString(config, "HAVE_GCC_ASM_FOR_X64", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GCC_ASM_FOR_X64", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GCC_ASM_FOR_X87
if (PyDict_SetItemString(config, "HAVE_GCC_ASM_FOR_X87", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GCC_ASM_FOR_X87", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GCC_UINT128_T
if (PyDict_SetItemString(config, "HAVE_GCC_UINT128_T", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GCC_UINT128_T", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GDBM_H
if (PyDict_SetItemString(config, "HAVE_GDBM_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GDBM_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETADDRINFO
if (PyDict_SetItemString(config, "HAVE_GETADDRINFO", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETADDRINFO", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETC_UNLOCKED
if (PyDict_SetItemString(config, "HAVE_GETC_UNLOCKED", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETC_UNLOCKED", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETEGID
if (PyDict_SetItemString(config, "HAVE_GETEGID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETEGID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETENTROPY
if (PyDict_SetItemString(config, "HAVE_GETENTROPY", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETENTROPY", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETEUID
if (PyDict_SetItemString(config, "HAVE_GETEUID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETEUID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETGID
if (PyDict_SetItemString(config, "HAVE_GETGID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETGID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETGRENT
if (PyDict_SetItemString(config, "HAVE_GETGRENT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETGRENT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETGRGID
if (PyDict_SetItemString(config, "HAVE_GETGRGID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETGRGID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETGRGID_R
if (PyDict_SetItemString(config, "HAVE_GETGRGID_R", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETGRGID_R", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETGRNAM_R
if (PyDict_SetItemString(config, "HAVE_GETGRNAM_R", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETGRNAM_R", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETGROUPLIST
if (PyDict_SetItemString(config, "HAVE_GETGROUPLIST", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETGROUPLIST", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETGROUPS
if (PyDict_SetItemString(config, "HAVE_GETGROUPS", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETGROUPS", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETHOSTBYNAME_R
if (PyDict_SetItemString(config, "HAVE_GETHOSTBYNAME_R", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETHOSTBYNAME_R", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETHOSTBYNAME_R_6_ARG
if (PyDict_SetItemString(config, "HAVE_GETHOSTBYNAME_R_6_ARG", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETHOSTBYNAME_R_6_ARG", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETITIMER
if (PyDict_SetItemString(config, "HAVE_GETITIMER", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETITIMER", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETLOADAVG
if (PyDict_SetItemString(config, "HAVE_GETLOADAVG", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETLOADAVG", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETLOGIN
if (PyDict_SetItemString(config, "HAVE_GETLOGIN", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETLOGIN", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETLOGIN_R
if (PyDict_SetItemString(config, "HAVE_GETLOGIN_R", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETLOGIN_R", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETNAMEINFO
if (PyDict_SetItemString(config, "HAVE_GETNAMEINFO", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETNAMEINFO", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETPAGESIZE
if (PyDict_SetItemString(config, "HAVE_GETPAGESIZE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETPAGESIZE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETPGID
if (PyDict_SetItemString(config, "HAVE_GETPGID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETPGID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETPPID
if (PyDict_SetItemString(config, "HAVE_GETPPID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETPPID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETPRIORITY
if (PyDict_SetItemString(config, "HAVE_GETPRIORITY", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETPRIORITY", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETPWENT
if (PyDict_SetItemString(config, "HAVE_GETPWENT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETPWENT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETPWNAM_R
if (PyDict_SetItemString(config, "HAVE_GETPWNAM_R", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETPWNAM_R", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETPWUID
if (PyDict_SetItemString(config, "HAVE_GETPWUID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETPWUID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETPWUID_R
if (PyDict_SetItemString(config, "HAVE_GETPWUID_R", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETPWUID_R", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETRANDOM
if (PyDict_SetItemString(config, "HAVE_GETRANDOM", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETRANDOM", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETRANDOM_SYSCALL
if (PyDict_SetItemString(config, "HAVE_GETRANDOM_SYSCALL", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETRANDOM_SYSCALL", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETRESGID
if (PyDict_SetItemString(config, "HAVE_GETRESGID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETRESGID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETRESUID
if (PyDict_SetItemString(config, "HAVE_GETRESUID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETRESUID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETRUSAGE
if (PyDict_SetItemString(config, "HAVE_GETRUSAGE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETRUSAGE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETSID
if (PyDict_SetItemString(config, "HAVE_GETSID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETSID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETSPENT
if (PyDict_SetItemString(config, "HAVE_GETSPENT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETSPENT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETSPNAM
if (PyDict_SetItemString(config, "HAVE_GETSPNAM", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETSPNAM", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETUID
if (PyDict_SetItemString(config, "HAVE_GETUID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETUID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GRANTPT
if (PyDict_SetItemString(config, "HAVE_GRANTPT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GRANTPT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GRP_H
if (PyDict_SetItemString(config, "HAVE_GRP_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GRP_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_HSTRERROR
if (PyDict_SetItemString(config, "HAVE_HSTRERROR", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_HSTRERROR", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_HTOLE64
if (PyDict_SetItemString(config, "HAVE_HTOLE64", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_HTOLE64", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_IF_NAMEINDEX
if (PyDict_SetItemString(config, "HAVE_IF_NAMEINDEX", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_IF_NAMEINDEX", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_INET_ATON
if (PyDict_SetItemString(config, "HAVE_INET_ATON", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_INET_ATON", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_INITGROUPS
if (PyDict_SetItemString(config, "HAVE_INITGROUPS", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_INITGROUPS", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_INTTYPES_H
if (PyDict_SetItemString(config, "HAVE_INTTYPES_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_INTTYPES_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_KILL
if (PyDict_SetItemString(config, "HAVE_KILL", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_KILL", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_KILLPG
if (PyDict_SetItemString(config, "HAVE_KILLPG", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_KILLPG", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LANGINFO_H
if (PyDict_SetItemString(config, "HAVE_LANGINFO_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LANGINFO_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LCHOWN
if (PyDict_SetItemString(config, "HAVE_LCHOWN", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LCHOWN", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LIBDB
if (PyDict_SetItemString(config, "HAVE_LIBDB", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LIBDB", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LIBINTL_H
if (PyDict_SetItemString(config, "HAVE_LIBINTL_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LIBINTL_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LIBSQLITE3
if (PyDict_SetItemString(config, "HAVE_LIBSQLITE3", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LIBSQLITE3", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINK
if (PyDict_SetItemString(config, "HAVE_LINK", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINK", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINKAT
if (PyDict_SetItemString(config, "HAVE_LINKAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINKAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINK_H
if (PyDict_SetItemString(config, "HAVE_LINK_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINK_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_AUXVEC_H
if (PyDict_SetItemString(config, "HAVE_LINUX_AUXVEC_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_AUXVEC_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_CAN_BCM_H
if (PyDict_SetItemString(config, "HAVE_LINUX_CAN_BCM_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_CAN_BCM_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_CAN_H
if (PyDict_SetItemString(config, "HAVE_LINUX_CAN_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_CAN_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_CAN_ISOTP_H
if (PyDict_SetItemString(config, "HAVE_LINUX_CAN_ISOTP_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_CAN_ISOTP_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_CAN_J1939_H
if (PyDict_SetItemString(config, "HAVE_LINUX_CAN_J1939_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_CAN_J1939_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_CAN_RAW_FD_FRAMES
if (PyDict_SetItemString(config, "HAVE_LINUX_CAN_RAW_FD_FRAMES", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_CAN_RAW_FD_FRAMES", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_CAN_RAW_H
if (PyDict_SetItemString(config, "HAVE_LINUX_CAN_RAW_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_CAN_RAW_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_CAN_RAW_JOIN_FILTERS
if (PyDict_SetItemString(config, "HAVE_LINUX_CAN_RAW_JOIN_FILTERS", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_CAN_RAW_JOIN_FILTERS", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_FS_H
if (PyDict_SetItemString(config, "HAVE_LINUX_FS_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_FS_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_LIMITS_H
if (PyDict_SetItemString(config, "HAVE_LINUX_LIMITS_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_LIMITS_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_MEMFD_H
if (PyDict_SetItemString(config, "HAVE_LINUX_MEMFD_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_MEMFD_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_NETFILTER_IPV4_H
if (PyDict_SetItemString(config, "HAVE_LINUX_NETFILTER_IPV4_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_NETFILTER_IPV4_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_NETLINK_H
if (PyDict_SetItemString(config, "HAVE_LINUX_NETLINK_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_NETLINK_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_QRTR_H
if (PyDict_SetItemString(config, "HAVE_LINUX_QRTR_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_QRTR_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_RANDOM_H
if (PyDict_SetItemString(config, "HAVE_LINUX_RANDOM_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_RANDOM_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_SCHED_H
if (PyDict_SetItemString(config, "HAVE_LINUX_SCHED_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_SCHED_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_SOUNDCARD_H
if (PyDict_SetItemString(config, "HAVE_LINUX_SOUNDCARD_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_SOUNDCARD_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_TIPC_H
if (PyDict_SetItemString(config, "HAVE_LINUX_TIPC_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_TIPC_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_VM_SOCKETS_H
if (PyDict_SetItemString(config, "HAVE_LINUX_VM_SOCKETS_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_VM_SOCKETS_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LINUX_WAIT_H
if (PyDict_SetItemString(config, "HAVE_LINUX_WAIT_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LINUX_WAIT_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LOCKF
if (PyDict_SetItemString(config, "HAVE_LOCKF", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LOCKF", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LOG1P
if (PyDict_SetItemString(config, "HAVE_LOG1P", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LOG1P", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LOG2
if (PyDict_SetItemString(config, "HAVE_LOG2", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LOG2", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LOGIN_TTY
if (PyDict_SetItemString(config, "HAVE_LOGIN_TTY", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LOGIN_TTY", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LONG_DOUBLE
if (PyDict_SetItemString(config, "HAVE_LONG_DOUBLE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LONG_DOUBLE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LUTIMES
if (PyDict_SetItemString(config, "HAVE_LUTIMES", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LUTIMES", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_MADVISE
if (PyDict_SetItemString(config, "HAVE_MADVISE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_MADVISE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_MAKEDEV
if (PyDict_SetItemString(config, "HAVE_MAKEDEV", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_MAKEDEV", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_MBRTOWC
if (PyDict_SetItemString(config, "HAVE_MBRTOWC", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_MBRTOWC", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_MEMFD_CREATE
if (PyDict_SetItemString(config, "HAVE_MEMFD_CREATE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_MEMFD_CREATE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_MEMRCHR
if (PyDict_SetItemString(config, "HAVE_MEMRCHR", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_MEMRCHR", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_MKDIRAT
if (PyDict_SetItemString(config, "HAVE_MKDIRAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_MKDIRAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_MKFIFO
if (PyDict_SetItemString(config, "HAVE_MKFIFO", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_MKFIFO", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_MKFIFOAT
if (PyDict_SetItemString(config, "HAVE_MKFIFOAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_MKFIFOAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_MKNOD
if (PyDict_SetItemString(config, "HAVE_MKNOD", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_MKNOD", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_MKNODAT
if (PyDict_SetItemString(config, "HAVE_MKNODAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_MKNODAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_MMAP
if (PyDict_SetItemString(config, "HAVE_MMAP", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_MMAP", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_MREMAP
if (PyDict_SetItemString(config, "HAVE_MREMAP", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_MREMAP", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_NANOSLEEP
if (PyDict_SetItemString(config, "HAVE_NANOSLEEP", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_NANOSLEEP", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_NCURSESW
if (PyDict_SetItemString(config, "HAVE_NCURSESW", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_NCURSESW", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_NCURSESW_CURSES_H
if (PyDict_SetItemString(config, "HAVE_NCURSESW_CURSES_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_NCURSESW_CURSES_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_NCURSESW_NCURSES_H
if (PyDict_SetItemString(config, "HAVE_NCURSESW_NCURSES_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_NCURSESW_NCURSES_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_NCURSESW_PANEL_H
if (PyDict_SetItemString(config, "HAVE_NCURSESW_PANEL_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_NCURSESW_PANEL_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_NCURSES_H
if (PyDict_SetItemString(config, "HAVE_NCURSES_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_NCURSES_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_NETDB_H
if (PyDict_SetItemString(config, "HAVE_NETDB_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_NETDB_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_NETINET_IN_H
if (PyDict_SetItemString(config, "HAVE_NETINET_IN_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_NETINET_IN_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_NETPACKET_PACKET_H
if (PyDict_SetItemString(config, "HAVE_NETPACKET_PACKET_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_NETPACKET_PACKET_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_NET_ETHERNET_H
if (PyDict_SetItemString(config, "HAVE_NET_ETHERNET_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_NET_ETHERNET_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_NET_IF_H
if (PyDict_SetItemString(config, "HAVE_NET_IF_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_NET_IF_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_OPENAT
if (PyDict_SetItemString(config, "HAVE_OPENAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_OPENAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_OPENDIR
if (PyDict_SetItemString(config, "HAVE_OPENDIR", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_OPENDIR", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_OPENPTY
if (PyDict_SetItemString(config, "HAVE_OPENPTY", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_OPENPTY", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PANELW
if (PyDict_SetItemString(config, "HAVE_PANELW", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PANELW", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PANEL_H
if (PyDict_SetItemString(config, "HAVE_PANEL_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PANEL_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PATHCONF
if (PyDict_SetItemString(config, "HAVE_PATHCONF", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PATHCONF", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PAUSE
if (PyDict_SetItemString(config, "HAVE_PAUSE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PAUSE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PIPE
if (PyDict_SetItemString(config, "HAVE_PIPE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PIPE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PIPE2
if (PyDict_SetItemString(config, "HAVE_PIPE2", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PIPE2", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_POLL
if (PyDict_SetItemString(config, "HAVE_POLL", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_POLL", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_POLL_H
if (PyDict_SetItemString(config, "HAVE_POLL_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_POLL_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_POSIX_FADVISE
if (PyDict_SetItemString(config, "HAVE_POSIX_FADVISE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_POSIX_FADVISE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_POSIX_FALLOCATE
if (PyDict_SetItemString(config, "HAVE_POSIX_FALLOCATE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_POSIX_FALLOCATE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_POSIX_OPENPT
if (PyDict_SetItemString(config, "HAVE_POSIX_OPENPT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_POSIX_OPENPT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_POSIX_SPAWN
if (PyDict_SetItemString(config, "HAVE_POSIX_SPAWN", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_POSIX_SPAWN", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_POSIX_SPAWNP
if (PyDict_SetItemString(config, "HAVE_POSIX_SPAWNP", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_POSIX_SPAWNP", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_POSIX_SPAWN_FILE_ACTIONS_ADDCLOSEFROM_NP
if (PyDict_SetItemString(config, "HAVE_POSIX_SPAWN_FILE_ACTIONS_ADDCLOSEFROM_NP", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_POSIX_SPAWN_FILE_ACTIONS_ADDCLOSEFROM_NP", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PREAD
if (PyDict_SetItemString(config, "HAVE_PREAD", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PREAD", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PREADV
if (PyDict_SetItemString(config, "HAVE_PREADV", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PREADV", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PREADV2
if (PyDict_SetItemString(config, "HAVE_PREADV2", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PREADV2", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PRLIMIT
if (PyDict_SetItemString(config, "HAVE_PRLIMIT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PRLIMIT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PROCESS_VM_READV
if (PyDict_SetItemString(config, "HAVE_PROCESS_VM_READV", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PROCESS_VM_READV", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PTHREAD_CONDATTR_SETCLOCK
if (PyDict_SetItemString(config, "HAVE_PTHREAD_CONDATTR_SETCLOCK", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PTHREAD_CONDATTR_SETCLOCK", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PTHREAD_GETATTR_NP
if (PyDict_SetItemString(config, "HAVE_PTHREAD_GETATTR_NP", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PTHREAD_GETATTR_NP", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PTHREAD_GETCPUCLOCKID
if (PyDict_SetItemString(config, "HAVE_PTHREAD_GETCPUCLOCKID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PTHREAD_GETCPUCLOCKID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PTHREAD_GETNAME_NP
if (PyDict_SetItemString(config, "HAVE_PTHREAD_GETNAME_NP", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PTHREAD_GETNAME_NP", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PTHREAD_H
if (PyDict_SetItemString(config, "HAVE_PTHREAD_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PTHREAD_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PTHREAD_KILL
if (PyDict_SetItemString(config, "HAVE_PTHREAD_KILL", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PTHREAD_KILL", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PTHREAD_SETNAME_NP
if (PyDict_SetItemString(config, "HAVE_PTHREAD_SETNAME_NP", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PTHREAD_SETNAME_NP", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PTHREAD_SIGMASK
if (PyDict_SetItemString(config, "HAVE_PTHREAD_SIGMASK", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PTHREAD_SIGMASK", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PTSNAME
if (PyDict_SetItemString(config, "HAVE_PTSNAME", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PTSNAME", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PTSNAME_R
if (PyDict_SetItemString(config, "HAVE_PTSNAME_R", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PTSNAME_R", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PTY_H
if (PyDict_SetItemString(config, "HAVE_PTY_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PTY_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PWRITE
if (PyDict_SetItemString(config, "HAVE_PWRITE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PWRITE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PWRITEV
if (PyDict_SetItemString(config, "HAVE_PWRITEV", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PWRITEV", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_PWRITEV2
if (PyDict_SetItemString(config, "HAVE_PWRITEV2", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_PWRITEV2", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_READLINKAT
if (PyDict_SetItemString(config, "HAVE_READLINKAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_READLINKAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_READV
if (PyDict_SetItemString(config, "HAVE_READV", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_READV", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_REALPATH
if (PyDict_SetItemString(config, "HAVE_REALPATH", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_REALPATH", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_RENAMEAT
if (PyDict_SetItemString(config, "HAVE_RENAMEAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_RENAMEAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_RL_APPEND_HISTORY
if (PyDict_SetItemString(config, "HAVE_RL_APPEND_HISTORY", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_RL_APPEND_HISTORY", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_RL_CATCH_SIGNAL
if (PyDict_SetItemString(config, "HAVE_RL_CATCH_SIGNAL", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_RL_CATCH_SIGNAL", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_RL_COMPDISP_FUNC_T
if (PyDict_SetItemString(config, "HAVE_RL_COMPDISP_FUNC_T", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_RL_COMPDISP_FUNC_T", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_RL_COMPLETION_APPEND_CHARACTER
if (PyDict_SetItemString(config, "HAVE_RL_COMPLETION_APPEND_CHARACTER", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_RL_COMPLETION_APPEND_CHARACTER", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_RL_COMPLETION_DISPLAY_MATCHES_HOOK
if (PyDict_SetItemString(config, "HAVE_RL_COMPLETION_DISPLAY_MATCHES_HOOK", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_RL_COMPLETION_DISPLAY_MATCHES_HOOK", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_RL_COMPLETION_MATCHES
if (PyDict_SetItemString(config, "HAVE_RL_COMPLETION_MATCHES", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_RL_COMPLETION_MATCHES", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_RL_COMPLETION_SUPPRESS_APPEND
if (PyDict_SetItemString(config, "HAVE_RL_COMPLETION_SUPPRESS_APPEND", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_RL_COMPLETION_SUPPRESS_APPEND", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_RL_PRE_INPUT_HOOK
if (PyDict_SetItemString(config, "HAVE_RL_PRE_INPUT_HOOK", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_RL_PRE_INPUT_HOOK", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_RL_RESIZE_TERMINAL
if (PyDict_SetItemString(config, "HAVE_RL_RESIZE_TERMINAL", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_RL_RESIZE_TERMINAL", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SCHED_GET_PRIORITY_MAX
if (PyDict_SetItemString(config, "HAVE_SCHED_GET_PRIORITY_MAX", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SCHED_GET_PRIORITY_MAX", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SCHED_H
if (PyDict_SetItemString(config, "HAVE_SCHED_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SCHED_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SCHED_RR_GET_INTERVAL
if (PyDict_SetItemString(config, "HAVE_SCHED_RR_GET_INTERVAL", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SCHED_RR_GET_INTERVAL", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SCHED_SETAFFINITY
if (PyDict_SetItemString(config, "HAVE_SCHED_SETAFFINITY", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SCHED_SETAFFINITY", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SCHED_SETPARAM
if (PyDict_SetItemString(config, "HAVE_SCHED_SETPARAM", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SCHED_SETPARAM", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SCHED_SETSCHEDULER
if (PyDict_SetItemString(config, "HAVE_SCHED_SETSCHEDULER", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SCHED_SETSCHEDULER", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SEM_CLOCKWAIT
if (PyDict_SetItemString(config, "HAVE_SEM_CLOCKWAIT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SEM_CLOCKWAIT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SEM_GETVALUE
if (PyDict_SetItemString(config, "HAVE_SEM_GETVALUE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SEM_GETVALUE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SEM_OPEN
if (PyDict_SetItemString(config, "HAVE_SEM_OPEN", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SEM_OPEN", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SEM_TIMEDWAIT
if (PyDict_SetItemString(config, "HAVE_SEM_TIMEDWAIT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SEM_TIMEDWAIT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SEM_UNLINK
if (PyDict_SetItemString(config, "HAVE_SEM_UNLINK", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SEM_UNLINK", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SENDFILE
if (PyDict_SetItemString(config, "HAVE_SENDFILE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SENDFILE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETEGID
if (PyDict_SetItemString(config, "HAVE_SETEGID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETEGID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETEUID
if (PyDict_SetItemString(config, "HAVE_SETEUID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETEUID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETGID
if (PyDict_SetItemString(config, "HAVE_SETGID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETGID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETGROUPS
if (PyDict_SetItemString(config, "HAVE_SETGROUPS", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETGROUPS", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETHOSTNAME
if (PyDict_SetItemString(config, "HAVE_SETHOSTNAME", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETHOSTNAME", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETITIMER
if (PyDict_SetItemString(config, "HAVE_SETITIMER", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETITIMER", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETJMP_H
if (PyDict_SetItemString(config, "HAVE_SETJMP_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETJMP_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETLOCALE
if (PyDict_SetItemString(config, "HAVE_SETLOCALE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETLOCALE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETNS
if (PyDict_SetItemString(config, "HAVE_SETNS", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETNS", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETPRIORITY
if (PyDict_SetItemString(config, "HAVE_SETPRIORITY", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETPRIORITY", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETREGID
if (PyDict_SetItemString(config, "HAVE_SETREGID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETREGID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETRESGID
if (PyDict_SetItemString(config, "HAVE_SETRESGID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETRESGID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETRESUID
if (PyDict_SetItemString(config, "HAVE_SETRESUID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETRESUID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETREUID
if (PyDict_SetItemString(config, "HAVE_SETREUID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETREUID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETUID
if (PyDict_SetItemString(config, "HAVE_SETUID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETUID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SHADOW_H
if (PyDict_SetItemString(config, "HAVE_SHADOW_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SHADOW_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SHM_OPEN
if (PyDict_SetItemString(config, "HAVE_SHM_OPEN", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SHM_OPEN", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SHM_UNLINK
if (PyDict_SetItemString(config, "HAVE_SHM_UNLINK", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SHM_UNLINK", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SIGACTION
if (PyDict_SetItemString(config, "HAVE_SIGACTION", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SIGACTION", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SIGALTSTACK
if (PyDict_SetItemString(config, "HAVE_SIGALTSTACK", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SIGALTSTACK", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SIGFILLSET
if (PyDict_SetItemString(config, "HAVE_SIGFILLSET", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SIGFILLSET", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SIGINFO_T_SI_BAND
if (PyDict_SetItemString(config, "HAVE_SIGINFO_T_SI_BAND", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SIGINFO_T_SI_BAND", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SIGPENDING
if (PyDict_SetItemString(config, "HAVE_SIGPENDING", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SIGPENDING", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SIGRELSE
if (PyDict_SetItemString(config, "HAVE_SIGRELSE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SIGRELSE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SIGTIMEDWAIT
if (PyDict_SetItemString(config, "HAVE_SIGTIMEDWAIT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SIGTIMEDWAIT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SIGWAIT
if (PyDict_SetItemString(config, "HAVE_SIGWAIT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SIGWAIT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SIGWAITINFO
if (PyDict_SetItemString(config, "HAVE_SIGWAITINFO", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SIGWAITINFO", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SNPRINTF
if (PyDict_SetItemString(config, "HAVE_SNPRINTF", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SNPRINTF", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SOCKADDR_ALG
if (PyDict_SetItemString(config, "HAVE_SOCKADDR_ALG", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SOCKADDR_ALG", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SOCKADDR_STORAGE
if (PyDict_SetItemString(config, "HAVE_SOCKADDR_STORAGE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SOCKADDR_STORAGE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SOCKETPAIR
if (PyDict_SetItemString(config, "HAVE_SOCKETPAIR", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SOCKETPAIR", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SOCKLEN_T
if (PyDict_SetItemString(config, "HAVE_SOCKLEN_T", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SOCKLEN_T", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SPAWN_H
if (PyDict_SetItemString(config, "HAVE_SPAWN_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SPAWN_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SPLICE
if (PyDict_SetItemString(config, "HAVE_SPLICE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SPLICE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SSIZE_T
if (PyDict_SetItemString(config, "HAVE_SSIZE_T", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SSIZE_T", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STATVFS
if (PyDict_SetItemString(config, "HAVE_STATVFS", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STATVFS", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STATX
if (PyDict_SetItemString(config, "HAVE_STATX", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STATX", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STAT_TV_NSEC
if (PyDict_SetItemString(config, "HAVE_STAT_TV_NSEC", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STAT_TV_NSEC", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STDINT_H
if (PyDict_SetItemString(config, "HAVE_STDINT_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STDINT_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STDIO_H
if (PyDict_SetItemString(config, "HAVE_STDIO_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STDIO_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STDLIB_H
if (PyDict_SetItemString(config, "HAVE_STDLIB_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STDLIB_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STD_ATOMIC
if (PyDict_SetItemString(config, "HAVE_STD_ATOMIC", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STD_ATOMIC", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STRINGS_H
if (PyDict_SetItemString(config, "HAVE_STRINGS_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STRINGS_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STRING_H
if (PyDict_SetItemString(config, "HAVE_STRING_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STRING_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STRLCPY
if (PyDict_SetItemString(config, "HAVE_STRLCPY", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STRLCPY", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STRSIGNAL
if (PyDict_SetItemString(config, "HAVE_STRSIGNAL", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STRSIGNAL", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STRUCT_PASSWD_PW_GECOS
if (PyDict_SetItemString(config, "HAVE_STRUCT_PASSWD_PW_GECOS", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STRUCT_PASSWD_PW_GECOS", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STRUCT_PASSWD_PW_PASSWD
if (PyDict_SetItemString(config, "HAVE_STRUCT_PASSWD_PW_PASSWD", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STRUCT_PASSWD_PW_PASSWD", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STRUCT_STATX_STX_DIO_MEM_ALIGN
if (PyDict_SetItemString(config, "HAVE_STRUCT_STATX_STX_DIO_MEM_ALIGN", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STRUCT_STATX_STX_DIO_MEM_ALIGN", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STRUCT_STATX_STX_MNT_ID
if (PyDict_SetItemString(config, "HAVE_STRUCT_STATX_STX_MNT_ID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STRUCT_STATX_STX_MNT_ID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
if (PyDict_SetItemString(config, "HAVE_STRUCT_STAT_ST_BLKSIZE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STRUCT_STAT_ST_BLKSIZE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
if (PyDict_SetItemString(config, "HAVE_STRUCT_STAT_ST_BLOCKS", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STRUCT_STAT_ST_BLOCKS", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STRUCT_STAT_ST_RDEV
if (PyDict_SetItemString(config, "HAVE_STRUCT_STAT_ST_RDEV", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STRUCT_STAT_ST_RDEV", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_STRUCT_TM_TM_ZONE
if (PyDict_SetItemString(config, "HAVE_STRUCT_TM_TM_ZONE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_STRUCT_TM_TM_ZONE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYMLINKAT
if (PyDict_SetItemString(config, "HAVE_SYMLINKAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYMLINKAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYNC
if (PyDict_SetItemString(config, "HAVE_SYNC", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYNC", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYSCONF
if (PyDict_SetItemString(config, "HAVE_SYSCONF", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYSCONF", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYSEXITS_H
if (PyDict_SetItemString(config, "HAVE_SYSEXITS_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYSEXITS_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYSLOG_H
if (PyDict_SetItemString(config, "HAVE_SYSLOG_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYSLOG_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYSTEM
if (PyDict_SetItemString(config, "HAVE_SYSTEM", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYSTEM", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_AUXV_H
if (PyDict_SetItemString(config, "HAVE_SYS_AUXV_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_AUXV_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_EPOLL_H
if (PyDict_SetItemString(config, "HAVE_SYS_EPOLL_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_EPOLL_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_EVENTFD_H
if (PyDict_SetItemString(config, "HAVE_SYS_EVENTFD_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_EVENTFD_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_FILE_H
if (PyDict_SetItemString(config, "HAVE_SYS_FILE_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_FILE_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_IOCTL_H
if (PyDict_SetItemString(config, "HAVE_SYS_IOCTL_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_IOCTL_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_MMAN_H
if (PyDict_SetItemString(config, "HAVE_SYS_MMAN_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_MMAN_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_PIDFD_H
if (PyDict_SetItemString(config, "HAVE_SYS_PIDFD_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_PIDFD_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_POLL_H
if (PyDict_SetItemString(config, "HAVE_SYS_POLL_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_POLL_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_RANDOM_H
if (PyDict_SetItemString(config, "HAVE_SYS_RANDOM_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_RANDOM_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_RESOURCE_H
if (PyDict_SetItemString(config, "HAVE_SYS_RESOURCE_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_RESOURCE_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_SENDFILE_H
if (PyDict_SetItemString(config, "HAVE_SYS_SENDFILE_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_SENDFILE_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_SOCKET_H
if (PyDict_SetItemString(config, "HAVE_SYS_SOCKET_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_SOCKET_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
if (PyDict_SetItemString(config, "HAVE_SYS_SOUNDCARD_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_SOUNDCARD_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_STATVFS_H
if (PyDict_SetItemString(config, "HAVE_SYS_STATVFS_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_STATVFS_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_SYSCALL_H
if (PyDict_SetItemString(config, "HAVE_SYS_SYSCALL_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_SYSCALL_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_SYSMACROS_H
if (PyDict_SetItemString(config, "HAVE_SYS_SYSMACROS_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_SYSMACROS_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_TIMERFD_H
if (PyDict_SetItemString(config, "HAVE_SYS_TIMERFD_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_TIMERFD_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_UIO_H
if (PyDict_SetItemString(config, "HAVE_SYS_UIO_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_UIO_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_WAIT_H
if (PyDict_SetItemString(config, "HAVE_SYS_WAIT_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_WAIT_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_XATTR_H
if (PyDict_SetItemString(config, "HAVE_SYS_XATTR_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_XATTR_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_TEMPNAM
if (PyDict_SetItemString(config, "HAVE_TEMPNAM", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_TEMPNAM", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_TERMIOS_H
if (PyDict_SetItemString(config, "HAVE_TERMIOS_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_TERMIOS_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_TERM_H
if (PyDict_SetItemString(config, "HAVE_TERM_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_TERM_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_TIMEGM
if (PyDict_SetItemString(config, "HAVE_TIMEGM", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_TIMEGM", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_TIMERFD_CREATE
if (PyDict_SetItemString(config, "HAVE_TIMERFD_CREATE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_TIMERFD_CREATE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_TMPNAM
if (PyDict_SetItemString(config, "HAVE_TMPNAM", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_TMPNAM", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_TMPNAM_R
if (PyDict_SetItemString(config, "HAVE_TMPNAM_R", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_TMPNAM_R", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_TRUNCATE
if (PyDict_SetItemString(config, "HAVE_TRUNCATE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_TRUNCATE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_TTYNAME_R
if (PyDict_SetItemString(config, "HAVE_TTYNAME_R", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_TTYNAME_R", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_UNLINKAT
if (PyDict_SetItemString(config, "HAVE_UNLINKAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_UNLINKAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_UNLOCKPT
if (PyDict_SetItemString(config, "HAVE_UNLOCKPT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_UNLOCKPT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_UNSHARE
if (PyDict_SetItemString(config, "HAVE_UNSHARE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_UNSHARE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_UTIMENSAT
if (PyDict_SetItemString(config, "HAVE_UTIMENSAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_UTIMENSAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_UTIMES
if (PyDict_SetItemString(config, "HAVE_UTIMES", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_UTIMES", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_UTMP_H
if (PyDict_SetItemString(config, "HAVE_UTMP_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_UTMP_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_UT_NAMESIZE
if (PyDict_SetItemString(config, "HAVE_UT_NAMESIZE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_UT_NAMESIZE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_UUID_GENERATE_TIME_SAFE
if (PyDict_SetItemString(config, "HAVE_UUID_GENERATE_TIME_SAFE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_UUID_GENERATE_TIME_SAFE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_UUID_H
if (PyDict_SetItemString(config, "HAVE_UUID_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_UUID_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_VFORK
if (PyDict_SetItemString(config, "HAVE_VFORK", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_VFORK", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_WAIT
if (PyDict_SetItemString(config, "HAVE_WAIT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_WAIT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_WAIT3
if (PyDict_SetItemString(config, "HAVE_WAIT3", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_WAIT3", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_WAIT4
if (PyDict_SetItemString(config, "HAVE_WAIT4", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_WAIT4", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_WAITID
if (PyDict_SetItemString(config, "HAVE_WAITID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_WAITID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_WMEMCMP
if (PyDict_SetItemString(config, "HAVE_WMEMCMP", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_WMEMCMP", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_WORKING_TZSET
if (PyDict_SetItemString(config, "HAVE_WORKING_TZSET", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_WORKING_TZSET", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_WRITEV
if (PyDict_SetItemString(config, "HAVE_WRITEV", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_WRITEV", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE___UINT128_T
if (PyDict_SetItemString(config, "HAVE___UINT128_T", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE___UINT128_T", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef MAJOR_IN_SYSMACROS
if (PyDict_SetItemString(config, "MAJOR_IN_SYSMACROS", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "MAJOR_IN_SYSMACROS", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef MVWDELCH_IS_EXPRESSION
if (PyDict_SetItemString(config, "MVWDELCH_IS_EXPRESSION", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "MVWDELCH_IS_EXPRESSION", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef PTHREAD_KEY_T_IS_COMPATIBLE_WITH_INT
if (PyDict_SetItemString(config, "PTHREAD_KEY_T_IS_COMPATIBLE_WITH_INT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "PTHREAD_KEY_T_IS_COMPATIBLE_WITH_INT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef PTHREAD_SYSTEM_SCHED_SUPPORTED
if (PyDict_SetItemString(config, "PTHREAD_SYSTEM_SCHED_SUPPORTED", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "PTHREAD_SYSTEM_SCHED_SUPPORTED", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef PY_COERCE_C_LOCALE
if (PyDict_SetItemString(config, "PY_COERCE_C_LOCALE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "PY_COERCE_C_LOCALE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef PY_HAVE_PERF_TRAMPOLINE
if (PyDict_SetItemString(config, "PY_HAVE_PERF_TRAMPOLINE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "PY_HAVE_PERF_TRAMPOLINE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef PY_SQLITE_HAVE_SERIALIZE
if (PyDict_SetItemString(config, "PY_SQLITE_HAVE_SERIALIZE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "PY_SQLITE_HAVE_SERIALIZE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef PY_SSL_DEFAULT_CIPHERS
if (PyDict_SetItemString(config, "PY_SSL_DEFAULT_CIPHERS", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "PY_SSL_DEFAULT_CIPHERS", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#endif /* MS_WINDOWS */

#ifdef DOUBLE_IS_ARM_MIXED_ENDIAN_IEEE754
    if (PyDict_SetItemString(config, "DOUBLE_IS_ARM_MIXED_ENDIAN_IEEE754", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#else
    if (PyDict_SetItemString(config, "DOUBLE_IS_ARM_MIXED_ENDIAN_IEEE754", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#endif

#ifdef DOUBLE_IS_BIG_ENDIAN_IEEE754
    if (PyDict_SetItemString(config, "DOUBLE_IS_BIG_ENDIAN_IEEE754", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#else
    if (PyDict_SetItemString(config, "DOUBLE_IS_BIG_ENDIAN_IEEE754", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#endif

#ifdef DOUBLE_IS_LITTLE_ENDIAN_IEEE754
    if (PyDict_SetItemString(config, "DOUBLE_IS_LITTLE_ENDIAN_IEEE754", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#else
    if (PyDict_SetItemString(config, "DOUBLE_IS_LITTLE_ENDIAN_IEEE754", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#endif

#ifdef HAVE_ACCEPT
    if (PyDict_SetItemString(config, "HAVE_ACCEPT", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#else
    if (PyDict_SetItemString(config, "HAVE_ACCEPT", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#endif

    if (PyDict_SetItemString(config, "HAVE_ALTZONE",
#ifdef HAVE_ALTZONE
        _PyLong_GetOne()
#else
        _PyLong_GetZero()
#endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

#ifdef HAVE_BIND
    if (PyDict_SetItemString(config, "HAVE_BIND", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#else
    if (PyDict_SetItemString(config, "HAVE_BIND", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#endif

    if (PyDict_SetItemString(config, "HAVE_CLOCK",
#ifdef HAVE_CLOCK
        _PyLong_GetOne()
#else
        _PyLong_GetZero()
#endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "HAVE_CONIO_H",
#ifdef HAVE_CONIO_H
        _PyLong_GetOne()
#else
        _PyLong_GetZero()
#endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

#ifdef HAVE_CONNECT
    if (PyDict_SetItemString(config, "HAVE_CONNECT", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#else
    if (PyDict_SetItemString(config, "HAVE_CONNECT", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#endif

#ifdef SIZEOF_DOUBLE
if (PyDict_SetItemString(config, "SIZEOF_DOUBLE", PyLong_FromLong(SIZEOF_DOUBLE)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_FLOAT
if (PyDict_SetItemString(config, "SIZEOF_FLOAT", PyLong_FromLong(SIZEOF_FLOAT)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_FPOS_T
if (PyDict_SetItemString(config, "SIZEOF_FPOS_T", PyLong_FromLong(SIZEOF_FPOS_T)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_INT
if (PyDict_SetItemString(config, "SIZEOF_INT", PyLong_FromLong(SIZEOF_INT)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_LONG
if (PyDict_SetItemString(config, "SIZEOF_LONG", PyLong_FromLong(SIZEOF_LONG)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_LONG_DOUBLE
if (PyDict_SetItemString(config, "SIZEOF_LONG_DOUBLE", PyLong_FromLong(SIZEOF_LONG_DOUBLE)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_LONG_LONG
if (PyDict_SetItemString(config, "SIZEOF_LONG_LONG", PyLong_FromLong(SIZEOF_LONG_LONG)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_OFF_T
if (PyDict_SetItemString(config, "SIZEOF_OFF_T", PyLong_FromLong(SIZEOF_OFF_T)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_PID_T
if (PyDict_SetItemString(config, "SIZEOF_PID_T", PyLong_FromLong(SIZEOF_PID_T)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_PTHREAD_KEY_T
if (PyDict_SetItemString(config, "SIZEOF_PTHREAD_KEY_T", PyLong_FromLong(SIZEOF_PTHREAD_KEY_T)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_PTHREAD_T
if (PyDict_SetItemString(config, "SIZEOF_PTHREAD_T", PyLong_FromLong(SIZEOF_PTHREAD_T)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_SHORT
if (PyDict_SetItemString(config, "SIZEOF_SHORT", PyLong_FromLong(SIZEOF_SHORT)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_SIZE_T
if (PyDict_SetItemString(config, "SIZEOF_SIZE_T", PyLong_FromLong(SIZEOF_SIZE_T)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_TIME_T
if (PyDict_SetItemString(config, "SIZEOF_TIME_T", PyLong_FromLong(SIZEOF_TIME_T)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_UINTPTR_T
if (PyDict_SetItemString(config, "SIZEOF_UINTPTR_T", PyLong_FromLong(SIZEOF_UINTPTR_T)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_VOID_P
if (PyDict_SetItemString(config, "SIZEOF_VOID_P", PyLong_FromLong(SIZEOF_VOID_P)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF_WCHAR_T
if (PyDict_SetItemString(config, "SIZEOF_WCHAR_T", PyLong_FromLong(SIZEOF_WCHAR_T)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef SIZEOF__BOOL
if (PyDict_SetItemString(config, "SIZEOF__BOOL", PyLong_FromLong(SIZEOF__BOOL)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef PY_BUILTIN_HASHLIB_HASHES
if (PyDict_SetItemString(config, "PY_BUILTIN_HASHLIB_HASHES", PyUnicode_FromString(PY_BUILTIN_HASHLIB_HASHES)) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

    if (PyDict_SetItemString(config, "HAVE_TMPFILE",
#ifdef HAVE_TMPFILE
        _PyLong_GetOne()
#else
        _PyLong_GetZero()
#endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    if (PyDict_SetItemString(config, "HAVE_STRFTIME",
#ifdef HAVE_STRFTIME
        _PyLong_GetOne()
#else
        _PyLong_GetZero()
#endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

#ifdef PY_SUPPORT_TIER
    if (PyDict_SetItemString(config, "PY_SUPPORT_TIER", PyLong_FromLong(PY_SUPPORT_TIER)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#endif

#ifdef ALIGNOF_SIZE_T
    if (PyDict_SetItemString(config, "ALIGNOF_SIZE_T", PyLong_FromLong(ALIGNOF_SIZE_T)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#endif

#ifdef ALIGNOF_MAX_ALIGN_T
    if (PyDict_SetItemString(config, "ALIGNOF_MAX_ALIGN_T", PyLong_FromLong(ALIGNOF_MAX_ALIGN_T)) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#endif

    if (PyDict_SetItemString(config, "HAVE_TM_ZONE",
#ifdef HAVE_TM_ZONE
        _PyLong_GetOne()
#else
        _PyLong_GetZero()
#endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

        if (PyDict_SetItemString(config, "STDC_HEADERS",
#ifdef STDC_HEADERS
        _PyLong_GetOne()
#else
        _PyLong_GetZero()
#endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

        if (PyDict_SetItemString(config, "HAVE_PROTOTYPES",
#ifdef HAVE_PROTOTYPES
        _PyLong_GetOne()
#else
        _PyLong_GetZero()
#endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

        if (PyDict_SetItemString(config, "SYS_SELECT_WITH_SYS_TIME",
#ifdef SYS_SELECT_WITH_SYS_TIME
        _PyLong_GetOne()
#else
        _PyLong_GetZero()
#endif
    ) < 0) {
        Py_DECREF(config);
        return NULL;
    }

#ifdef WITH_DECIMAL_CONTEXTVAR
    if (PyDict_SetItemString(config, "WITH_DECIMAL_CONTEXTVAR", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#else
    if (PyDict_SetItemString(config, "WITH_DECIMAL_CONTEXTVAR", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#endif

#ifdef WITH_DOC_STRINGS
    if (PyDict_SetItemString(config, "WITH_DOC_STRINGS", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#else
    if (PyDict_SetItemString(config, "WITH_DOC_STRINGS", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#endif

#ifdef WITH_PYMALLOC
    if (PyDict_SetItemString(config, "WITH_PYMALLOC", _PyLong_GetOne()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#else
    if (PyDict_SetItemString(config, "WITH_PYMALLOC", _PyLong_GetZero()) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#endif

#ifdef WITH_MIMALLOC
if (PyDict_SetItemString(config, "WITH_MIMALLOC", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "WITH_MIMALLOC", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DYNAMIC_LOADING
if (PyDict_SetItemString(config, "HAVE_DYNAMIC_LOADING", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DYNAMIC_LOADING", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FTIME
if (PyDict_SetItemString(config, "HAVE_FTIME", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FTIME", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETPEERNAME
if (PyDict_SetItemString(config, "HAVE_GETPEERNAME", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETPEERNAME", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETPGRP
if (PyDict_SetItemString(config, "HAVE_GETPGRP", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETPGRP", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETPID
if (PyDict_SetItemString(config, "HAVE_GETPID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETPID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETWD
if (PyDict_SetItemString(config, "HAVE_GETWD", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETWD", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LSTAT
if (PyDict_SetItemString(config, "HAVE_LSTAT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LSTAT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_MKTIME
if (PyDict_SetItemString(config, "HAVE_MKTIME", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_MKTIME", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_NICE
if (PyDict_SetItemString(config, "HAVE_NICE", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_NICE", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_READLINK
if (PyDict_SetItemString(config, "HAVE_READLINK", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_READLINK", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETPGID
if (PyDict_SetItemString(config, "HAVE_SETPGID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETPGID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETPGRP
if (PyDict_SetItemString(config, "HAVE_SETPGRP", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETPGRP", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETSID
if (PyDict_SetItemString(config, "HAVE_SETSID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETSID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETVBUF
if (PyDict_SetItemString(config, "HAVE_SETVBUF", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETVBUF", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SIGINTERRUPT
if (PyDict_SetItemString(config, "HAVE_SIGINTERRUPT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SIGINTERRUPT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SHUTDOWN
if (PyDict_SetItemString(config, "HAVE_SHUTDOWN", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SHUTDOWN", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYMLINK
if (PyDict_SetItemString(config, "HAVE_SYMLINK", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYMLINK", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_TCGETPGRP
if (PyDict_SetItemString(config, "HAVE_TCGETPGRP", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_TCGETPGRP", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_TCSETPGRP
if (PyDict_SetItemString(config, "HAVE_TCSETPGRP", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_TCSETPGRP", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_TIMES
if (PyDict_SetItemString(config, "HAVE_TIMES", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_TIMES", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_UMASK
if (PyDict_SetItemString(config, "HAVE_UMASK", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_UMASK", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_UNAME
if (PyDict_SetItemString(config, "HAVE_UNAME", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_UNAME", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_WAITPID
if (PyDict_SetItemString(config, "HAVE_WAITPID", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_WAITPID", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_WCSFTIME
if (PyDict_SetItemString(config, "HAVE_WCSFTIME", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_WCSFTIME", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_WCSCOLL
if (PyDict_SetItemString(config, "HAVE_WCSCOLL", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_WCSCOLL", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_WCSXFRM
if (PyDict_SetItemString(config, "HAVE_WCSXFRM", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_WCSXFRM", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_ZLIB_COPY
if (PyDict_SetItemString(config, "HAVE_ZLIB_COPY", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_ZLIB_COPY", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DLFCN_H
if (PyDict_SetItemString(config, "HAVE_DLFCN_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DLFCN_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_ERRNO_H
if (PyDict_SetItemString(config, "HAVE_ERRNO_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_ERRNO_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_FCNTL_H
if (PyDict_SetItemString(config, "HAVE_FCNTL_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_FCNTL_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SIGNAL_H
if (PyDict_SetItemString(config, "HAVE_SIGNAL_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SIGNAL_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_PARAM_H
if (PyDict_SetItemString(config, "HAVE_SYS_PARAM_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_PARAM_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_SELECT_H
if (PyDict_SetItemString(config, "HAVE_SYS_SELECT_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_SELECT_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_STAT_H
if (PyDict_SetItemString(config, "HAVE_SYS_STAT_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_STAT_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_TIME_H
if (PyDict_SetItemString(config, "HAVE_SYS_TIME_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_TIME_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_TIMES_H
if (PyDict_SetItemString(config, "HAVE_SYS_TIMES_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_TIMES_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_TYPES_H
if (PyDict_SetItemString(config, "HAVE_SYS_TYPES_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_TYPES_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_UN_H
if (PyDict_SetItemString(config, "HAVE_SYS_UN_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_UN_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SYS_UTSNAME_H
if (PyDict_SetItemString(config, "HAVE_SYS_UTSNAME_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SYS_UTSNAME_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_UNISTD_H
if (PyDict_SetItemString(config, "HAVE_UNISTD_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_UNISTD_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_UTIME_H
if (PyDict_SetItemString(config, "HAVE_UTIME_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_UTIME_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_WCHAR_H
if (PyDict_SetItemString(config, "HAVE_WCHAR_H", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_WCHAR_H", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LIBDL
if (PyDict_SetItemString(config, "HAVE_LIBDL", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LIBDL", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_ERF
if (PyDict_SetItemString(config, "HAVE_ERF", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_ERF", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_ERFC
if (PyDict_SetItemString(config, "HAVE_ERFC", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_ERFC", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETHOSTNAME
if (PyDict_SetItemString(config, "HAVE_GETHOSTNAME", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETHOSTNAME", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETHOSTBYADDR
if (PyDict_SetItemString(config, "HAVE_GETHOSTBYADDR", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETHOSTBYADDR", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETHOSTBYNAME
if (PyDict_SetItemString(config, "HAVE_GETHOSTBYNAME", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETHOSTBYNAME", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETPROTOBYNAME
if (PyDict_SetItemString(config, "HAVE_GETPROTOBYNAME", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETPROTOBYNAME", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETSERVBYNAME
if (PyDict_SetItemString(config, "HAVE_GETSERVBYNAME", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETSERVBYNAME", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETSERVBYPORT
if (PyDict_SetItemString(config, "HAVE_GETSERVBYPORT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETSERVBYPORT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_INET_PTON
if (PyDict_SetItemString(config, "HAVE_INET_PTON", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_INET_PTON", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_INET_NTOA
if (PyDict_SetItemString(config, "HAVE_INET_NTOA", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_INET_NTOA", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_GETSOCKNAME
if (PyDict_SetItemString(config, "HAVE_GETSOCKNAME", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_GETSOCKNAME", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_LISTEN
if (PyDict_SetItemString(config, "HAVE_LISTEN", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_LISTEN", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_RECVFROM
if (PyDict_SetItemString(config, "HAVE_RECVFROM", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_RECVFROM", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SENDTO
if (PyDict_SetItemString(config, "HAVE_SENDTO", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SENDTO", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SETSOCKOPT
if (PyDict_SetItemString(config, "HAVE_SETSOCKOPT", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SETSOCKOPT", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_SOCKET
if (PyDict_SetItemString(config, "HAVE_SOCKET", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_SOCKET", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef HAVE_DUP
if (PyDict_SetItemString(config, "HAVE_DUP", _PyLong_GetOne()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#else
if (PyDict_SetItemString(config, "HAVE_DUP", _PyLong_GetZero()) < 0) {
    Py_DECREF(config);
    return NULL;
}
#endif

#ifdef Py_GIL_DISABLED
    PyObject *py_gil_disabled = _PyLong_GetOne();
#else
    PyObject *py_gil_disabled = _PyLong_GetZero();
#endif
    if (PyDict_SetItemString(config, "Py_GIL_DISABLED", py_gil_disabled) < 0) {
        Py_DECREF(config);
        return NULL;
    }

#ifdef Py_DEBUG
    PyObject *py_debug = _PyLong_GetOne();
#else
    PyObject *py_debug = _PyLong_GetZero();
#endif
    if (PyDict_SetItemString(config, "Py_DEBUG", py_debug) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    return config;
}

PyDoc_STRVAR(sysconfig__doc__,
"A helper for the sysconfig module.");

static struct PyMethodDef sysconfig_methods[] = {
    _SYSCONFIG_CONFIG_VARS_METHODDEF
    {NULL, NULL}
};

static PyModuleDef_Slot sysconfig_slots[] = {
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static PyModuleDef sysconfig_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_sysconfig",
    .m_doc = sysconfig__doc__,
    .m_methods = sysconfig_methods,
    .m_slots = sysconfig_slots,
};

PyMODINIT_FUNC
PyInit__sysconfig(void)
{
    return PyModuleDef_Init(&sysconfig_module);
}
