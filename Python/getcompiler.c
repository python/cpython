
/* Return the compiler identification, if possible. */

#include "Python.h"

#ifdef _Py_COMPILER
#  define COMPILER _Py_COMPILER
#endif

#ifndef COMPILER

// Note the __clang__ conditional has to come before the __GNUC__ one because
// clang pretends to be GCC.
#if defined(__clang__)
#define COMPILER "[Clang " __clang_version__ "]"
#elif defined(__GNUC__)
#define COMPILER "[GCC " __VERSION__ "]"
// Generic fallbacks.
#elif defined(__cplusplus)
#define COMPILER "[C++]"
#else
#define COMPILER "[C]"
#endif

#endif /* !COMPILER */

const char *
Py_GetCompiler(void)
{
    return COMPILER;
}
