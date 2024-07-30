/*
 * Return a string describing how Python was built.
 *
 * In general, we are only interested in whether --with-pydebug
 * or --enable-optimizations were specified and thus we will only
 * report three different type of builds:
 *
 *  "debug"     (./configure --with-pydebug)
 *  "release"   (./configure)
 *  "optimized" (./configure --enable-optimizations)
 *
 * Simple information on LTO and BOLT is also included.
 */

#include "Python.h"

#ifdef Py_DEBUG
#  define PY_CONFIG_INFO_BUILD_TYPE "debug"
#else
#  ifdef PY_CONFIGURE_WITH_OPTIMIZATIONS
#    define PY_CONFIG_INFO_BUILD_TYPE "optimized"
#  else
#    define PY_CONFIG_INFO_BUILD_TYPE "release"
#  endif
#endif

#if defined(PY_CONFIGURE_WITH_BOLT) && defined(PY_CONFIGURE_WITH_LTO)
#  define PY_CONFIG_INFO_BUILD_OPTIMIZATIONS ", BOLT/LTO"
#elif defined(PY_CONFIGURE_WITH_BOLT) && !defined(PY_CONFIGURE_WITH_LTO)
#  define PY_CONFIG_INFO_BUILD_OPTIMIZATIONS ", BOLT"
#elif !defined(PY_CONFIGURE_WITH_BOLT) && defined(PY_CONFIGURE_WITH_LTO)
#  define PY_CONFIG_INFO_BUILD_OPTIMIZATIONS ", LTO"
#else
#  define PY_CONFIG_INFO_BUILD_OPTIMIZATIONS ""
#endif

const char *
Py_GetConfigInfo(void)
{
    return PY_CONFIG_INFO_BUILD_TYPE PY_CONFIG_INFO_BUILD_OPTIMIZATIONS;
}
