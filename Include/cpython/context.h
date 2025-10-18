#ifndef Py_LIMITED_API
#ifndef Py_CONTEXT_H
#define Py_CONTEXT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) PyContext_Type;
typedef struct _pycontextobject PyContext;

PyAPI_DATA(PyTypeObject) PyContextVar_Type;
typedef struct _pycontextvarobject PyContextVar;

PyAPI_DATA(PyTypeObject) PyContextToken_Type;
typedef struct _pycontexttokenobject PyContextToken;


#define PyContext_CheckExact(o) Py_IS_TYPE((o), &PyContext_Type)
#define PyContextVar_CheckExact(o) Py_IS_TYPE((o), &PyContextVar_Type)
#define PyContextToken_CheckExact(o) Py_IS_TYPE((o), &PyContextToken_Type)


PyAPI_FUNC(PyObject *) PyContext_New(void);
PyAPI_FUNC(PyObject *) PyContext_Copy(PyObject *);
PyAPI_FUNC(PyObject *) PyContext_CopyCurrent(void);

PyAPI_FUNC(int) PyContext_Enter(PyObject *);
PyAPI_FUNC(int) PyContext_Exit(PyObject *);

typedef enum {
    /*
     * The current context has switched to a different context.  The object
     * passed to the watch callback is the now-current contextvars.Context
     * object, or None if no context is current.
     */
    Py_CONTEXT_SWITCHED = 1,
} PyContextEvent;

/*
 * Context object watcher callback function.  The object passed to the callback
 * is event-specific; see PyContextEvent for details.
 *
 * if the callback returns with an exception set, it must return -1. Otherwise
 * it should return 0
 */
typedef int (*PyContext_WatchCallback)(PyContextEvent, PyObject *);

/*
 * Register a per-interpreter callback that will be invoked for context object
 * enter/exit events.
 *
 * Returns a handle that may be passed to PyContext_ClearWatcher on success,
 * or -1 and sets and error if no more handles are available.
 */
PyAPI_FUNC(int) PyContext_AddWatcher(PyContext_WatchCallback callback);

/*
 * Clear the watcher associated with the watcher_id handle.
 *
 * Returns 0 on success or -1 if no watcher exists for the provided id.
 */
PyAPI_FUNC(int) PyContext_ClearWatcher(int watcher_id);

/* Create a new context variable.

   default_value can be NULL.
*/
PyAPI_FUNC(PyObject *) PyContextVar_New(
    const char *name, PyObject *default_value);


/* Get a value for the variable.

   Returns -1 if an error occurred during lookup.

   Returns 0 if value either was or was not found.

   If value was found, *value will point to it.
   If not, it will point to:

   - default_value, if not NULL;
   - the default value of "var", if not NULL;
   - NULL.

   '*value' will be a new ref, if not NULL.
*/
PyAPI_FUNC(int) PyContextVar_Get(
    PyObject *var, PyObject *default_value, PyObject **value);


/* Set a new value for the variable.
   Returns NULL if an error occurs.
*/
PyAPI_FUNC(PyObject *) PyContextVar_Set(PyObject *var, PyObject *value);


/* Reset a variable to its previous value.
   Returns 0 on success, -1 on error.
*/
PyAPI_FUNC(int) PyContextVar_Reset(PyObject *var, PyObject *token);


#ifdef __cplusplus
}
#endif
#endif /* !Py_CONTEXT_H */
#endif /* !Py_LIMITED_API */
