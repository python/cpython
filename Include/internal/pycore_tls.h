
#ifndef Py_CORE_TLS_H
#define Py_CORE_TLS_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE to be defined"
#endif

#ifndef _Py_THREAD_LOCAL
#if defined(__GNUC__)
#define _Py_THREAD_LOCAL __thread
#endif
#if defined(_MSC_VER)
#define _Py_THREAD_LOCAL __declspec(thread)
#endif
#endif

#endif /*  Py_CORE_TLS_H */
