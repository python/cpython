#ifndef PYSQLITE_CALLBACK_CONTEXT_H
#define PYSQLITE_CALLBACK_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h"

#include "module.h"

typedef struct {
    PyObject_HEAD
    PyObject *callable;
    PyObject *module;
    pysqlite_state *state;
} pysqlite_CallbackContext;

#define pysqlite_CallbackContext_CAST(op)   ((pysqlite_CallbackContext *)(op))

/* Allocate a UDF/callback context structure. In order to ensure that the state
 * pointer always outlives the callback context, we make sure it owns a
 * reference to the module itself. create_callback_context() is always called
 * from connection methods, so we use the defining class to fetch the module
 * pointer.
 */
PyObject *
pysqlite_create_callback_context(pysqlite_state *state, PyObject *callable);

int
pysqlite_context_setup_types(PyObject *module);

#ifdef __cplusplus
}
#endif

#endif // !PYSQLITE_CALLBACK_CONTEXT_H
