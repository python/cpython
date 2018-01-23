
#include "Python.h"

#ifndef PLATFORM
#define PLATFORM "unknown"
#endif

/* bpo-32637: Force sys.platform == "android" on Android */
#ifdef __ANDROID__
#  undef PLATFORM
#  define PLATFORM "android"
#endif

const char *
Py_GetPlatform(void)
{
    return PLATFORM;
}
