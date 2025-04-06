#ifndef Py_INTERNAL_OBJECT_STATE_H
#define Py_INTERNAL_OBJECT_STATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

struct _py_object_runtime_state {
#ifdef Py_REF_DEBUG
    Py_ssize_t interpreter_leaks;
#endif
    int _not_used;
};

struct _py_object_state {
#ifdef Py_REF_DEBUG
    Py_ssize_t reftotal;
#endif
#ifdef Py_TRACE_REFS
    /* Head of circular doubly-linked list of all objects.  These are linked
     * together via the _ob_prev and _ob_next members of a PyObject, which
     * exist only in a Py_TRACE_REFS build.
     */
    PyObject *refchain;
    /* In most cases, refchain points to _refchain_obj.
     * In sub-interpreters that share objmalloc state with the main interp,
     * refchain points to the main interpreter's _refchain_obj, and their own
     * _refchain_obj is unused.
     */
    PyObject _refchain_obj;
#endif
    int _not_used;
};


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OBJECT_STATE_H */
