
/* Return the compiler identification, if possible. */

#include "Python.h"

#ifndef COMPILER

// Note the __clang__ conditional has to come before the __GNUC__ one because
// clang pretends to be GCC.
#if defined(__clang__)
#define COMPILER "\n[Clang " __clang_version__ "]"
#elif defined(__GNUC__)
#define COMPILER "\n[GCC " __VERSION__ "]"
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
