#ifndef Py_EXPORTS_H
#define Py_EXPORTS_H

#if defined _WIN32 || defined __CYGWIN__
    #define Py_IMPORTED_SYMBOL __declspec(dllimport)
    #define Py_EXPORTED_SYMBOL __declspec(dllexport)
    #define Py_LOCAL_SYMBOL
#else
    #if __GNUC__ >= 4
        #define Py_IMPORTED_SYMBOL __attribute__ ((visibility ("default")))
        #define Py_EXPORTED_SYMBOL __attribute__ ((visibility ("default")))
        #define Py_LOCAL_SYMBOL  __attribute__ ((visibility ("hidden")))
    #else
        #define Py_IMPORTED_SYMBOL
        #define Py_EXPORTED_SYMBOL
        #define Py_LOCAL_SYMBOL
    #endif
#endif

#endif /* Py_EXPORTS_H */
