#ifndef Py_INTERNAL_UTILS_H
#define Py_INTERNAL_UTILS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* Define a pair of assertion macros:
 * _PyObject_ASSERT_FROM(), _PyObject_ASSERT_WITH_MSG() and _PyObject_ASSERT().
 *
 * These work like the regular C assert(), in that they will abort the
 * process with a message on stderr if the given condition fails to hold,
 * but compile away to nothing if NDEBUG is defined.
 *
 * However, before aborting, Python will also try to call _PyObject_Dump() on
 * the given object.  This may be of use when investigating bugs in which a
 * particular object is corrupt (e.g. buggy a tp_visit method in an extension
 * module breaking the garbage collector), to help locate the broken objects.
 *
 * The WITH_MSG variant allows you to supply an additional message that Python
 * will attempt to print to stderr, after the object dump. */
#ifdef NDEBUG
   /* No debugging: compile away the assertions: */
#  define _PyObject_ASSERT_FROM(obj, expr, msg, filename, lineno, func) \
    ((void)0)
#else
   /* With debugging: generate checks: */
#  define _PyObject_ASSERT_FROM(obj, expr, msg, filename, lineno, func) \
    ((expr) \
      ? (void)(0) \
      : _PyObject_AssertFailed((obj), Py_STRINGIFY(expr), \
                               (msg), (filename), (lineno), (func)))
#endif

#define _PyObject_ASSERT_WITH_MSG(obj, expr, msg) \
    _PyObject_ASSERT_FROM((obj), expr, (msg), __FILE__, __LINE__, __func__)
#define _PyObject_ASSERT(obj, expr) \
    _PyObject_ASSERT_WITH_MSG((obj), expr, NULL)

#define _PyObject_ASSERT_FAILED_MSG(obj, msg) \
    _PyObject_AssertFailed((obj), NULL, (msg), __FILE__, __LINE__, __func__)

/* Declare and define _PyObject_AssertFailed() even when NDEBUG is defined,
   to avoid causing compiler/linker errors when building extensions without
   NDEBUG against a Python built with NDEBUG defined.

   msg, expr and function can be NULL. */
PyAPI_FUNC(void) _Py_NO_RETURN _PyObject_AssertFailed(
    PyObject *obj,
    const char *expr,
    const char *msg,
    const char *file,
    int line,
    const char *function);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UTILS_H */
