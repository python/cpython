/*
 * @author Bénédikt Tran
 *
 * Implement the HMAC algorithm as described by RFC 2104 using HACL*.
 *
 * Using HACL* implementation implicitly assumes that the caller wants
 * a formally verified implementation. In particular, only algorithms
 * given by their names will be recognized.
 *
 * Some algorithms exposed by `_hashlib` such as truncated SHA-2-512-224/256
 * are not yet implemented by the HACL* project. Nonetheless, the supported
 * HMAC algorithms form a subset of those supported by '_hashlib'.
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

// --- HMAC module state ------------------------------------------------------

typedef struct hmacmodule_state {
} hmacmodule_state;

static inline hmacmodule_state *
get_hmacmodule_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (hmacmodule_state *)state;
}

// --- HMAC module clinic configuration ---------------------------------------

/*[clinic input]
module _hmac
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=799f0f10157d561f]*/

// --- HMAC module methods ----------------------------------------------------

static PyMethodDef hmacmodule_methods[] = {
    {NULL, NULL, 0, NULL} /* sentinel */
};

// --- HMAC module initialization and finalization functions ------------------

static int
hmacmodule_exec(PyObject *module)
{
    hmacmodule_state *state = get_hmacmodule_state(module);
    return 0;
}

static int
hmacmodule_traverse(PyObject *mod, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(mod));
    hmacmodule_state *state = get_hmacmodule_state(mod);
    return 0;
}

static int
hmacmodule_clear(PyObject *mod)
{
    hmacmodule_state *state = get_hmacmodule_state(mod);
    return 0;
}

static inline void
hmacmodule_free(void *mod)
{
    (void)hmacmodule_clear((PyObject *)mod);
}

static struct PyModuleDef_Slot hmacmodule_slots[] = {
    {Py_mod_exec, hmacmodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL} /* sentinel */
};

static struct PyModuleDef _hmacmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_hmac",
    .m_size = sizeof(hmacmodule_state),
    .m_methods = hmacmodule_methods,
    .m_slots = hmacmodule_slots,
    .m_traverse = hmacmodule_traverse,
    .m_clear = hmacmodule_clear,
    .m_free = hmacmodule_free,
};

PyMODINIT_FUNC
PyInit__hmac(void)
{
    return PyModuleDef_Init(&_hmacmodule);
}
