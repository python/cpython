
/* Testing module for single-phase initialization of extension modules
 */
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

//#include <time.h>
#include "Python.h"
#include "pycore_namespace.h"     // _PyNamespace_New()


typedef struct {
    _PyTime_t initialized;
    PyObject *error;
    PyObject *int_const;
    PyObject *str_const;
} module_state;


/* Process-global state is only used by _testsinglephase
   since it's the only one that does not support re-init. */
static struct {
    int initialized_count;
    module_state module;
} global_state = {

#define NOT_INITIALIZED -1
    .initialized_count = NOT_INITIALIZED,
};

static void clear_state(module_state *state);

static void
clear_global_state(void)
{
    clear_state(&global_state.module);
    global_state.initialized_count = NOT_INITIALIZED;
}


static inline module_state *
get_module_state(PyObject *module)
{
    PyModuleDef *def = PyModule_GetDef(module);
    if (def->m_size == -1) {
        return &global_state.module;
    }
    else if (def->m_size == 0) {
        return NULL;
    }
    else {
        module_state *state = (module_state*)PyModule_GetState(module);
        assert(state != NULL);
        return state;
    }
}

static void
clear_state(module_state *state)
{
    state->initialized = 0;
    Py_CLEAR(state->error);
    Py_CLEAR(state->int_const);
    Py_CLEAR(state->str_const);
}

static int
_set_initialized(_PyTime_t *initialized)
{
    /* We go strictly monotonic to ensure each time is unique. */
    _PyTime_t prev;
    if (_PyTime_GetMonotonicClockWithInfo(&prev, NULL) != 0) {
        return -1;
    }
    /* We do a busy sleep since the interval should be super short. */
    _PyTime_t t;
    do {
        if (_PyTime_GetMonotonicClockWithInfo(&t, NULL) != 0) {
            return -1;
        }
    } while (t == prev);

    *initialized = t;
    return 0;
}

static int
init_state(module_state *state)
{
    assert(state->initialized == 0 &&
           state->error == NULL &&
           state->int_const == NULL &&
           state->str_const == NULL);

    if (_set_initialized(&state->initialized) != 0) {
        goto error;
    }
    assert(state->initialized > 0);

    /* Add an exception type */
    state->error = PyErr_NewException("_testsinglephase.error", NULL, NULL);
    if (state->error == NULL) {
        goto error;
    }

    state->int_const = PyLong_FromLong(1969);
    if (state->int_const == NULL) {
        goto error;
    }

    state->str_const = PyUnicode_FromString("something different");
    if (state->str_const == NULL) {
        goto error;
    }

    return 0;

error:
    clear_state(state);
    return -1;
}


static int
init_module(PyObject *module, module_state *state)
{
    if (PyModule_AddObjectRef(module, "error", state->error) != 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "int_const", state->int_const) != 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "str_const", state->str_const) != 0) {
        return -1;
    }

    double d = _PyTime_AsSecondsDouble(state->initialized);
    PyObject *initialized = PyFloat_FromDouble(d);
    if (initialized == NULL) {
        return -1;
    }
    int rc = PyModule_AddObjectRef(module, "_module_initialized", initialized);
    Py_DECREF(initialized);
    if (rc < 0) {
        return -1;
    }

    return 0;
}


PyDoc_STRVAR(common_state_initialized_doc,
"state_initialized()\n\
\n\
Return the seconds-since-epoch when the module state was initialized.");

static PyObject *
common_state_initialized(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    module_state *state = get_module_state(self);
    if (state == NULL) {
        Py_RETURN_NONE;
    }
    double d = _PyTime_AsSecondsDouble(state->initialized);
    return PyFloat_FromDouble(d);
}

#define STATE_INITIALIZED_METHODDEF \
    {"state_initialized", common_state_initialized, METH_NOARGS, \
     common_state_initialized_doc}


PyDoc_STRVAR(common_look_up_self_doc,
"look_up_self()\n\
\n\
Return the module associated with this module's def.m_base.m_index.");

static PyObject *
common_look_up_self(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyModuleDef *def = PyModule_GetDef(self);
    if (def == NULL) {
        return NULL;
    }
    return Py_NewRef(
            PyState_FindModule(def));
}

#define LOOK_UP_SELF_METHODDEF \
    {"look_up_self", common_look_up_self, METH_NOARGS, common_look_up_self_doc}


/* Function of two integers returning integer */

PyDoc_STRVAR(common_sum_doc,
"sum(i,j)\n\
\n\
Return the sum of i and j.");

static PyObject *
common_sum(PyObject *self, PyObject *args)
{
    long i, j;
    long res;
    if (!PyArg_ParseTuple(args, "ll:sum", &i, &j))
        return NULL;
    res = i + j;
    return PyLong_FromLong(res);
}

#define SUM_METHODDEF \
    {"sum", common_sum, METH_VARARGS, common_sum_doc}


PyDoc_STRVAR(basic_initialized_count_doc,
"initialized_count()\n\
\n\
Return how many times the module has been initialized.");

static PyObject *
basic_initialized_count(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    assert(PyModule_GetDef(self)->m_size == -1);
    return PyLong_FromLong(global_state.initialized_count);
}

#define INITIALIZED_COUNT_METHODDEF \
    {"initialized_count", basic_initialized_count, METH_NOARGS, \
     basic_initialized_count_doc}


PyDoc_STRVAR(basic__clear_globals_doc,
"_clear_globals()\n\
\n\
Free all global state and set it to uninitialized.");

static PyObject *
basic__clear_globals(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    assert(PyModule_GetDef(self)->m_size == -1);
    clear_global_state();
    Py_RETURN_NONE;
}

#define _CLEAR_GLOBALS_METHODDEF \
    {"_clear_globals", basic__clear_globals, METH_NOARGS, \
     basic__clear_globals_doc}


PyDoc_STRVAR(basic__clear_module_state_doc, "_clear_module_state()\n\
\n\
Free the module state and set it to uninitialized.");

static PyObject *
basic__clear_module_state(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    module_state *state = get_module_state(self);
    if (state != NULL) {
        clear_state(state);
    }
    Py_RETURN_NONE;
}

#define _CLEAR_MODULE_STATE_METHODDEF \
    {"_clear_module_state", basic__clear_module_state, METH_NOARGS, \
     basic__clear_module_state_doc}


/*********************************************/
/* the _testsinglephase module (and aliases) */
/*********************************************/

/* This ia more typical of legacy extensions in the wild:
   - single-phase init
   - no module state
   - does not support repeated initialization
    (so m_copy is used)
   - the module def is cached in _PyRuntime.extensions
     (by name/filename)

   Also note that, because the module has single-phase init,
   it is cached in interp->module_by_index (using mod->md_def->m_base.m_index).
 */

static PyMethodDef TestMethods_Basic[] = {
    LOOK_UP_SELF_METHODDEF,
    SUM_METHODDEF,
    STATE_INITIALIZED_METHODDEF,
    INITIALIZED_COUNT_METHODDEF,
    _CLEAR_GLOBALS_METHODDEF,
    {NULL, NULL}           /* sentinel */
};

static struct PyModuleDef _testsinglephase_basic = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_testsinglephase",
    .m_doc = PyDoc_STR("Test module _testsinglephase"),
    .m_size = -1,  // no module state
    .m_methods = TestMethods_Basic,
};

static PyObject *
init__testsinglephase_basic(PyModuleDef *def)
{
    if (global_state.initialized_count == -1) {
        global_state.initialized_count = 0;
    }

    PyObject *module = PyModule_Create(def);
    if (module == NULL) {
        return NULL;
    }

    module_state *state = &global_state.module;
    // It may have been set by a previous run or under a different name.
    clear_state(state);
    if (init_state(state) < 0) {
        Py_CLEAR(module);
        return NULL;
    }

    if (init_module(module, state) < 0) {
        Py_CLEAR(module);
        goto finally;
    }

    global_state.initialized_count++;

finally:
    return module;
}

PyMODINIT_FUNC
PyInit__testsinglephase(void)
{
    return init__testsinglephase_basic(&_testsinglephase_basic);
}


PyMODINIT_FUNC
PyInit__testsinglephase_basic_wrapper(void)
{
    return PyInit__testsinglephase();
}


PyMODINIT_FUNC
PyInit__testsinglephase_basic_copy(void)
{
    static struct PyModuleDef def = {
        PyModuleDef_HEAD_INIT,
        .m_name = "_testsinglephase_basic_copy",
        .m_doc = PyDoc_STR("Test module _testsinglephase_basic_copy"),
        .m_size = -1,  // no module state
        .m_methods = TestMethods_Basic,
    };
    return init__testsinglephase_basic(&def);
}


/*******************************************/
/* the _testsinglephase_with_reinit module */
/*******************************************/

/* This ia less typical of legacy extensions in the wild:
   - single-phase init  (same as _testsinglephase above)
   - no module state
   - supports repeated initialization
     (so m_copy is not used)
   - the module def is not cached in _PyRuntime.extensions

   At this point most modules would reach for multi-phase init (PEP 489).
   However, module state has been around a while (PEP 3121),
   and most extensions predate multi-phase init.

   (This module is basically the same as _testsinglephase,
    but supports repeated initialization.)
 */

static PyMethodDef TestMethods_Reinit[] = {
    LOOK_UP_SELF_METHODDEF,
    SUM_METHODDEF,
    STATE_INITIALIZED_METHODDEF,
    {NULL, NULL}           /* sentinel */
};

static struct PyModuleDef _testsinglephase_with_reinit = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_testsinglephase_with_reinit",
    .m_doc = PyDoc_STR("Test module _testsinglephase_with_reinit"),
    .m_size = 0,
    .m_methods = TestMethods_Reinit,
};

PyMODINIT_FUNC
PyInit__testsinglephase_with_reinit(void)
{
    /* We purposefully do not try PyState_FindModule() first here
       since we want to check the behavior of re-loading the module. */
    PyObject *module = PyModule_Create(&_testsinglephase_with_reinit);
    if (module == NULL) {
        return NULL;
    }

    assert(get_module_state(module) == NULL);

    module_state state = {0};
    if (init_state(&state) < 0) {
        Py_CLEAR(module);
        return NULL;
    }

    if (init_module(module, &state) < 0) {
        Py_CLEAR(module);
        goto finally;
    }

finally:
    /* We only needed the module state for setting the module attrs. */
    clear_state(&state);
    return module;
}


/******************************************/
/* the _testsinglephase_with_state module */
/******************************************/

/* This is less typical of legacy extensions in the wild:
   - single-phase init  (same as _testsinglephase above)
   - has some module state
   - supports repeated initialization
     (so m_copy is not used)
   - the module def is not cached in _PyRuntime.extensions

   At this point most modules would reach for multi-phase init (PEP 489).
   However, module state has been around a while (PEP 3121),
   and most extensions predate multi-phase init.
 */

static PyMethodDef TestMethods_WithState[] = {
    LOOK_UP_SELF_METHODDEF,
    SUM_METHODDEF,
    STATE_INITIALIZED_METHODDEF,
    _CLEAR_MODULE_STATE_METHODDEF,
    {NULL, NULL}           /* sentinel */
};

static struct PyModuleDef _testsinglephase_with_state = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_testsinglephase_with_state",
    .m_doc = PyDoc_STR("Test module _testsinglephase_with_state"),
    .m_size = sizeof(module_state),
    .m_methods = TestMethods_WithState,
};

PyMODINIT_FUNC
PyInit__testsinglephase_with_state(void)
{
    /* We purposefully do not try PyState_FindModule() first here
       since we want to check the behavior of re-loading the module. */
    PyObject *module = PyModule_Create(&_testsinglephase_with_state);
    if (module == NULL) {
        return NULL;
    }

    module_state *state = get_module_state(module);
    assert(state != NULL);
    if (init_state(state) < 0) {
        Py_CLEAR(module);
        return NULL;
    }

    if (init_module(module, state) < 0) {
        clear_state(state);
        Py_CLEAR(module);
        goto finally;
    }

finally:
    return module;
}
