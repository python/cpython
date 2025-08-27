#ifndef Py_PYMACRO_H
#define Py_PYMACRO_H

// gh-91782: On FreeBSD 12, if the _POSIX_C_SOURCE and _XOPEN_SOURCE macros are
// defined, <sys/cdefs.h> disables C11 support and <assert.h> does not define
// the static_assert() macro.
// https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=255290
//
// macOS <= 10.10 doesn't define static_assert in assert.h at all despite
// having C11 compiler support.
//
// static_assert is defined in glibc from version 2.16. Compiler support for
// the C11 _Static_assert keyword is in gcc >= 4.6.
//
// MSVC makes static_assert a keyword in C11-17, contrary to the standards.
//
// In C++11 and C2x, static_assert is a keyword, redefining is undefined
// behaviour. So only define if building as C, not C++ (if __cplusplus is
// not defined), and only for C11-17.
#if !defined(static_assert) && (defined(__GNUC__) || defined(__clang__)) \
     && !defined(__cplusplus) && defined(__STDC_VERSION__) \
     && __STDC_VERSION__ >= 201112L && __STDC_VERSION__ <= 201710L
#  define static_assert _Static_assert
#endif


// _Py_ALIGNED_DEF(N, T): Define a variable/member with increased alignment
//
// `N`: the desired minimum alignment, an integer literal, number of bytes
// `T`: the type of the defined variable
//      (or a type with at least the defined variable's alignment)
//
// May not be used on a struct definition.
//
// Standards/compiler support for `alignas` alternatives:
// - `alignas` is a keyword in C23 and C++11.
// - `_Alignas` is a keyword in C11
// - GCC & clang has __attribute__((aligned))
//   (use that for older standards in pedantic mode)
// - MSVC has __declspec(align)
// - `_Alignas` is common C compiler extension
// Older compilers may name `alignas` differently; to allow compilation on such
// unsupported platforms, we don't redefine _Py_ALIGNED_DEF if it's already
// defined. Note that defining it wrong (including defining it to nothing) will
// cause ABI incompatibilities.
//
// Behavior of `alignas` alternatives:
// - `alignas` & `_Alignas`:
//   - Can be used multiple times; the greatest alignment applies.
//   - It is an *error* if the combined effect of all `alignas` modifiers would
//     decrease the alignment.
//   - Takes types or numbers.
//   - May not be used on a struct definition, unless also defining a variable.
// - `__declspec(align)`:
//   - Has no effect if it would decrease alignment.
//   - Only takes an integer literal.
//   - May be used on struct or variable definitions.
//     However, when defining both the struct and the variable at once,
//     `declspec(aligned)` causes compiler warning 5274 and possible ABI
//     incompatibility.
// - ` __attribute__((aligned))`:
//   - Has no effect if it would decrease alignment.
//   - Takes types or numbers
//   - May be used on struct or variable definitions.
#ifndef _Py_ALIGNED_DEF
#    ifdef __cplusplus
#        if __cplusplus >= 201103L
#            define _Py_ALIGNED_DEF(N, T) alignas(N) alignas(T) T
#        elif defined(__GNUC__) || defined(__clang__)
#            define _Py_ALIGNED_DEF(N, T) __attribute__((aligned(N))) T
#        elif defined(_MSC_VER)
#            define _Py_ALIGNED_DEF(N, T) __declspec(align(N)) T
#        else
#            define _Py_ALIGNED_DEF(N, T) alignas(N) alignas(T) T
#        endif
#    elif defined(_MSC_VER)
#        define _Py_ALIGNED_DEF(N, T) __declspec(align(N)) T
#    elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#        define _Py_ALIGNED_DEF(N, T) alignas(N) alignas(T) T
#    elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#        define _Py_ALIGNED_DEF(N, T)  _Alignas(N) _Alignas(T) T
#    elif (defined(__GNUC__) || defined(__clang__))
#        define _Py_ALIGNED_DEF(N, T) __attribute__((aligned(N))) T
#    else
#        define _Py_ALIGNED_DEF(N, T) _Alignas(N) _Alignas(T) T
#    endif
#endif


// _Py_ANONYMOUS: modifier for declaring an anonymous union.
// Usage: _Py_ANONYMOUS union { ... };
// Standards/compiler support:
// - C++ allows anonymous unions, but not structs
// - C11 and above allows anonymous unions and structs
// - MSVC has warning(disable: 4201) "nonstandard extension used : nameless
//   struct/union". This is specific enough that we disable it for all of
//   Python.h.
// - GCC & clang needs __extension__ before C11
// To allow unsupported platforms which need other spellings, we use a
// predefined value of _Py_ANONYMOUS if it exists.
#ifndef _Py_ANONYMOUS
#   if (defined(__GNUC__) || defined(__clang__)) \
          && !(defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)
#       define _Py_ANONYMOUS __extension__
#   else
#       define _Py_ANONYMOUS
#   endif
#endif


/* Minimum value between x and y */
#define Py_MIN(x, y) (((x) > (y)) ? (y) : (x))

/* Maximum value between x and y */
#define Py_MAX(x, y) (((x) > (y)) ? (x) : (y))

/* Absolute value of the number x */
#define Py_ABS(x) ((x) < 0 ? -(x) : (x))

#define _Py_XSTRINGIFY(x) #x

/* Convert the argument to a string. For example, Py_STRINGIFY(123) is replaced
   with "123" by the preprocessor. Defines are also replaced by their value.
   For example Py_STRINGIFY(__LINE__) is replaced by the line number, not
   by "__LINE__". */
#define Py_STRINGIFY(x) _Py_XSTRINGIFY(x)

/* Get the size of a structure member in bytes */
#define Py_MEMBER_SIZE(type, member) sizeof(((type *)0)->member)

/* Argument must be a char or an int in [-128, 127] or [0, 255]. */
#define Py_CHARMASK(c) ((unsigned char)((c) & 0xff))

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L \
     && !defined(__cplusplus) && !defined(_MSC_VER))
#  define Py_BUILD_ASSERT_EXPR(cond) \
    ((void)sizeof(struct { int dummy; _Static_assert(cond, #cond); }), \
     0)
#else
   /* Assert a build-time dependency, as an expression.
    *
    * Your compile will fail if the condition isn't true, or can't be evaluated
    * by the compiler. This can be used in an expression: its value is 0.
    *
    * Example:
    *
    * #define foo_to_char(foo)  \
    *     ((char *)(foo)        \
    *      + Py_BUILD_ASSERT_EXPR(offsetof(struct foo, string) == 0))
    *
    * Written by Rusty Russell, public domain, http://ccodearchive.net/
    */
#  define Py_BUILD_ASSERT_EXPR(cond) \
    (sizeof(char [1 - 2*!(cond)]) - 1)
#endif

#if ((defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) \
     || (defined(__cplusplus) && __cplusplus >= 201103L))
   // Use static_assert() on C11 and newer
#  define Py_BUILD_ASSERT(cond) \
        do { \
            static_assert((cond), #cond); \
        } while (0)
#else
#  define Py_BUILD_ASSERT(cond)  \
        do { \
            (void)Py_BUILD_ASSERT_EXPR(cond); \
        } while(0)
#endif

/* Get the number of elements in a visible array

   This does not work on pointers, or arrays declared as [], or function
   parameters. With correct compiler support, such usage will cause a build
   error (see Py_BUILD_ASSERT_EXPR).

   Written by Rusty Russell, public domain, http://ccodearchive.net/

   Requires at GCC 3.1+ */
#if (defined(__GNUC__) && !defined(__STRICT_ANSI__) && \
    (((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)) || (__GNUC__ >= 4)))
/* Two gcc extensions.
   &a[0] degrades to a pointer: a different type from an array */
#define Py_ARRAY_LENGTH(array) \
    (sizeof(array) / sizeof((array)[0]) \
     + Py_BUILD_ASSERT_EXPR(!__builtin_types_compatible_p(typeof(array), \
                                                          typeof(&(array)[0]))))
#else
#define Py_ARRAY_LENGTH(array) \
    (sizeof(array) / sizeof((array)[0]))
#endif


/* Define macros for inline documentation. */
#define PyDoc_VAR(name) static const char name[]
#define PyDoc_STRVAR(name,str) PyDoc_VAR(name) = PyDoc_STR(str)
#ifdef WITH_DOC_STRINGS
#define PyDoc_STR(str) str
#else
#define PyDoc_STR(str) ""
#endif

/* Below "a" is a power of 2. */
/* Round down size "n" to be a multiple of "a". */
#define _Py_SIZE_ROUND_DOWN(n, a) ((size_t)(n) & ~(size_t)((a) - 1))
/* Round up size "n" to be a multiple of "a". */
#define _Py_SIZE_ROUND_UP(n, a) (((size_t)(n) + \
        (size_t)((a) - 1)) & ~(size_t)((a) - 1))
/* Round pointer "p" down to the closest "a"-aligned address <= "p". */
#define _Py_ALIGN_DOWN(p, a) ((void *)((uintptr_t)(p) & ~(uintptr_t)((a) - 1)))
/* Round pointer "p" up to the closest "a"-aligned address >= "p". */
#define _Py_ALIGN_UP(p, a) ((void *)(((uintptr_t)(p) + \
        (uintptr_t)((a) - 1)) & ~(uintptr_t)((a) - 1)))
/* Check if pointer "p" is aligned to "a"-bytes boundary. */
#define _Py_IS_ALIGNED(p, a) (!((uintptr_t)(p) & (uintptr_t)((a) - 1)))

/* Use this for unused arguments in a function definition to silence compiler
 * warnings. Example:
 *
 * int func(int a, int Py_UNUSED(b)) { return a; }
 */
#if defined(__GNUC__) || defined(__clang__)
#  define Py_UNUSED(name) _unused_ ## name __attribute__((unused))
#elif defined(_MSC_VER)
   // Disable warning C4100: unreferenced formal parameter,
   // declare the parameter,
   // restore old compiler warnings.
#  define Py_UNUSED(name) \
        __pragma(warning(push)) \
        __pragma(warning(suppress: 4100)) \
        _unused_ ## name \
        __pragma(warning(pop))
#else
#  define Py_UNUSED(name) _unused_ ## name
#endif

#if defined(RANDALL_WAS_HERE)
#  define Py_UNREACHABLE() \
    Py_FatalError( \
        "If you're seeing this, the code is in what I thought was\n" \
        "an unreachable state.\n\n" \
        "I could give you advice for what to do, but honestly, why\n" \
        "should you trust me?  I clearly screwed this up.  I'm writing\n" \
        "a message that should never appear, yet I know it will\n" \
        "probably appear someday.\n\n" \
        "On a deep level, I know I'm not up to this task.\n" \
        "I'm so sorry.\n" \
        "https://xkcd.com/2200")
#elif defined(Py_DEBUG)
#  define Py_UNREACHABLE() \
    Py_FatalError( \
        "We've reached an unreachable state. Anything is possible.\n" \
        "The limits were in our heads all along. Follow your dreams.\n" \
        "https://xkcd.com/2200")
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#  define Py_UNREACHABLE() __builtin_unreachable()
#elif defined(__clang__) || defined(__INTEL_COMPILER)
#  define Py_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
#  define Py_UNREACHABLE() __assume(0)
#else
#  define Py_UNREACHABLE() \
    Py_FatalError("Unreachable C code path reached")
#endif

#define _Py_CONTAINER_OF(ptr, type, member) \
    (type*)((char*)ptr - offsetof(type, member))

// Prevent using an expression as a l-value.
// For example, "int x; _Py_RVALUE(x) = 1;" fails with a compiler error.
#define _Py_RVALUE(EXPR) ((void)0, (EXPR))

// Return non-zero if the type is signed, return zero if it's unsigned.
// Use "<= 0" rather than "< 0" to prevent the compiler warning:
// "comparison of unsigned expression in '< 0' is always false".
#define _Py_IS_TYPE_SIGNED(type) ((type)(-1) <= 0)

// Version helpers. These are primarily macros, but have exported equivalents.
#define _Py_PACK_VERSION(X, Y) _Py_PACK_FULL_VERSION(X, Y, 0, 0, 0)
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= _Py_PACK_VERSION(3, 14)
PyAPI_FUNC(uint32_t) Py_PACK_FULL_VERSION(int x, int y, int z, int level, int serial);
PyAPI_FUNC(uint32_t) Py_PACK_VERSION(int x, int y);
#define Py_PACK_FULL_VERSION _Py_PACK_FULL_VERSION
#define Py_PACK_VERSION _Py_PACK_VERSION
#endif // Py_LIMITED_API < 3.14


#endif /* Py_PYMACRO_H */
