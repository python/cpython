/* Macros that restrict available definitions and select implementations
 * to match an ABI stability promise:
 *
 * - internal API/ABI (may change at any time) -- Py_BUILD_CORE*
 * - general CPython API/ABI (may change in 3.x.0) -- default
 * - Stable ABI: abi3, abi3t (long-term stable) -- Py_LIMITED_API,
 *     Py_TARGET_ABI3T, _Py_OPAQUE_PYOBJECT
 * - Free-threading (incompatible with non-free-threading builds)
 *     -- Py_GIL_DISABLED
 */

#ifndef _Py_PYABI_H
#define _Py_PYABI_H

/* Defines to build Python and its standard library:
 *
 * - Py_BUILD_CORE: Build Python core. Gives access to Python internals; should
 *   not be used by third-party modules.
 * - Py_BUILD_CORE_BUILTIN: Build a Python stdlib module as a built-in module.
 * - Py_BUILD_CORE_MODULE: Build a Python stdlib module as a dynamic library.
 *
 * Py_BUILD_CORE_BUILTIN and Py_BUILD_CORE_MODULE imply Py_BUILD_CORE.
 *
 * On Windows, Py_BUILD_CORE_MODULE exports "PyInit_xxx" symbol, whereas
 * Py_BUILD_CORE_BUILTIN does not.
 */
#if defined(Py_BUILD_CORE_BUILTIN) && !defined(Py_BUILD_CORE)
#  define Py_BUILD_CORE
#endif
#if defined(Py_BUILD_CORE_MODULE) && !defined(Py_BUILD_CORE)
#  define Py_BUILD_CORE
#endif

/* Check valid values for target ABI macros.
 */
#if defined(Py_LIMITED_API) && Py_LIMITED_API+0 < 3
   // Empty Py_LIMITED_API used to work; redefine to
   // Python 3.2 to be explicit.
#  undef Py_LIMITED_API
#  define Py_LIMITED_API 0x03020000
#endif
#if defined(Py_TARGET_ABI3T) && Py_TARGET_ABI3T+0 < 0x030f0000
#  error "Py_TARGET_ABI3T must be 0x030f0000 (3.15) or above"
#endif

/* Stable ABI for free-threaded builds (abi3t, introduced in PEP 803)
 * is enabled by one of:
 *   - Py_TARGET_ABI3T, or
 *   - Py_LIMITED_API and Py_GIL_DISABLED.
 *
 * These affect set the following, which Python.h should use internally:
 *   - Py_LIMITED_API (defines the subset of API we expose)
 *   - _Py_OPAQUE_PYOBJECT (additionally hides what's ABI-incompatible between
 *     free-threaded & GIL)
 *
 *  (Don't use Py_TARGET_ABI3T directly. It's currently only used to set these
 *   2 macros, and defined for users' convenience.)
 */
#if defined(Py_LIMITED_API) && defined(Py_GIL_DISABLED) \
        && !defined(Py_TARGET_ABI3T)
#  define Py_TARGET_ABI3T Py_LIMITED_API
#endif
#if defined(Py_TARGET_ABI3T)
#  define _Py_OPAQUE_PYOBJECT
#  if !defined(Py_LIMITED_API)
#    define Py_LIMITED_API Py_TARGET_ABI3T
#  elif Py_LIMITED_API > Py_TARGET_ABI3T
     // if both are defined, use the *lower* version,
     // i.e. maximum compatibility
#    undef Py_LIMITED_API
#    define Py_LIMITED_API Py_TARGET_ABI3T
#  endif
#else
#  ifdef _Py_OPAQUE_PYOBJECT
     // _Py_OPAQUE_PYOBJECT is a private macro; do not define it directly.
#    error "Define Py_TARGET_ABI3T to target abi3t."
#  endif
#endif

#if defined(Py_TARGET_ABI3T)
#  if !defined(Py_GIL_DISABLED)
     // Define Py_GIL_DISABLED for users' needs. Users check this macro to see
     // whether they need extra synchronization.
#    define Py_GIL_DISABLED
#  endif
#  if defined(_Py_IS_TESTCEXT)
     // When compiling for abi3t, contents of Python.h should not depend
     // on Py_GIL_DISABLED.
     // We ask GCC to error if it sees the macro from this point on.
     // Since users are free to the macro, and there's no way to undo the
     // poisoning at the end of Python.h, we only do this in a test module
     // (test_cext).
     //
     // Clang's poisoning is stricter than GCC's: it looks in `#elif`
     // expressions after matching `#if`s. We disable it for now.
     // We also provide an undocumented, unsupported opt-out macro to help
     // porting to other compilers. Consider reaching out if you use it.
#    if defined(__GNUC__) && !defined(__clang__) && !defined(_Py_NO_GCC_POISON)
#      undef Py_GIL_DISABLED
#      pragma GCC poison Py_GIL_DISABLED
#    endif
#  endif
#endif

/* The internal C API must not be used with the limited C API: make sure
 * that Py_BUILD_CORE* macros are not defined in this case.
 * But, keep the "original" values, under different names, for "exports.h"
 */
#ifdef Py_BUILD_CORE
#  define _PyEXPORTS_CORE
#endif
#ifdef Py_BUILD_CORE_MODULE
#  define _PyEXPORTS_CORE_MODULE
#endif
#ifdef Py_LIMITED_API
#  undef Py_BUILD_CORE
#  undef Py_BUILD_CORE_BUILTIN
#  undef Py_BUILD_CORE_MODULE
#endif

#endif // _Py_PYABI_H
