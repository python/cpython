#include "Python.h"
#include "dictobject.h"
#include "frameobject.h"
#include "pycore_code.h"
#include "opcode_cache.h"

#if OPCACHE_STATS
size_t _Py_opcache_code_objects = 0;
size_t _Py_opcache_code_objects_extra_mem = 0;

size_t _Py_opcache_global_opts = 0;
size_t _Py_opcache_global_hits = 0;
size_t _Py_opcache_global_misses = 0;

size_t _Py_opcache_attr_opts = 0;
size_t _Py_opcache_attr_hits = 0;
size_t _Py_opcache_attr_misses = 0;
size_t _Py_opcache_attr_deopts = 0;
size_t _Py_opcache_attr_total = 0;
#endif

void _PyOpcodeCache_PrintStats() {
#if OPCACHE_STATS
    fprintf(stderr, "-- Opcode cache number of objects  = %zd\n",
            _Py_opcache_code_objects);

    fprintf(stderr, "-- Opcode cache total extra mem    = %zd\n",
            _Py_opcache_code_objects_extra_mem);

    fprintf(stderr, "\n");

    fprintf(stderr, "-- Opcode cache LOAD_GLOBAL hits   = %zd (%d%%)\n",
            _Py_opcache_global_hits,
            (int) (100.0 * _Py_opcache_global_hits /
                (_Py_opcache_global_hits + _Py_opcache_global_misses)));

    fprintf(stderr, "-- Opcode cache LOAD_GLOBAL misses = %zd (%d%%)\n",
            _Py_opcache_global_misses,
            (int) (100.0 * _Py_opcache_global_misses /
                (_Py_opcache_global_hits + _Py_opcache_global_misses)));

    fprintf(stderr, "-- Opcode cache LOAD_GLOBAL opts   = %zd\n",
            _Py_opcache_global_opts);

    fprintf(stderr, "\n");

    fprintf(stderr, "-- Opcode cache LOAD_ATTR hits     = %zd (%d%%)\n",
            _Py_opcache_attr_hits,
            (int) (100.0 * _Py_opcache_attr_hits /
                _Py_opcache_attr_total));

    fprintf(stderr, "-- Opcode cache LOAD_ATTR misses   = %zd (%d%%)\n",
            _Py_opcache_attr_misses,
            (int) (100.0 * _Py_opcache_attr_misses /
                _Py_opcache_attr_total));

    fprintf(stderr, "-- Opcode cache LOAD_ATTR opts     = %zd\n",
            _Py_opcache_attr_opts);

    fprintf(stderr, "-- Opcode cache LOAD_ATTR deopts   = %zd\n",
            _Py_opcache_attr_deopts);

    fprintf(stderr, "-- Opcode cache LOAD_ATTR total    = %zd\n",
            _Py_opcache_attr_total);
#endif
}

static int
_PyOpcodeCache_ReloadLoadGlobal(PyCodeObject *co, _PyOpcache **co_opcache_ptr,
                                PyFrameObject *f, PyObject* name, PyObject** result) {

    _PyOpcache* co_opcache = *co_opcache_ptr;
    *result = _PyDict_LoadGlobal((PyDictObject *)f->f_globals,
                                 (PyDictObject *)f->f_builtins,
                                     name);
    if (*result == NULL) {
        if (PyErr_Occurred()) {
            return -2;
        }
        return -1;
    }

    _PyOpcache_LoadGlobal *lg = &co_opcache->u.lg;

    if (co_opcache->optimized == 0) {
        /* Wasn't optimized before. */
        OPCACHE_STAT_GLOBAL_OPT();
    } else {
        OPCACHE_STAT_GLOBAL_MISS();
    }

    co_opcache->optimized = 1;
    lg->globals_ver = ((PyDictObject *)f->f_globals)->ma_version_tag;
    lg->builtins_ver = ((PyDictObject *)f->f_builtins)->ma_version_tag;
    lg->ptr = *result;
    Py_INCREF(*result);
    return 0;
}


int _PyOpcodeCache_LoadGlobal(PyCodeObject *co, _PyOpcache **co_opcache_ptr,
                              PyFrameObject *f, PyObject* name, PyObject** result) {

    *result = NULL;
    _PyOpcache* co_opcache = *co_opcache_ptr;

    if (co_opcache == NULL || !PyDict_CheckExact(f->f_globals) || !PyDict_CheckExact(f->f_builtins))
    {
        return 0;
    }

    _PyOpcache_LoadGlobal *lg = &co_opcache->u.lg;

    if (co_opcache->optimized > 0 &&
        lg->globals_ver == ((PyDictObject *)f->f_globals)->ma_version_tag &&
        lg->builtins_ver == ((PyDictObject *)f->f_builtins)->ma_version_tag
    ) {

        OPCACHE_STAT_GLOBAL_HIT();
        *result = lg->ptr;
        assert(*result != NULL);
        Py_INCREF(*result);
        return 0;
    }

    return _PyOpcodeCache_ReloadLoadGlobal(co, co_opcache_ptr, f, name, result);
}



static int _PyOpcodeCache_InitializeLoadAttr(PyCodeObject *co, _PyOpcache **co_opcache_ptr,
                                             PyTypeObject* type, PyObject* name,
                                             PyObject* owner, PyObject** result) {

    _PyOpcache* co_opcache = *co_opcache_ptr;

    if (co_opcache == NULL) {
        goto fail_to_optimize;
    }

    if(type->tp_getattro != PyObject_GenericGetAttr) {
        goto fail_to_optimize;
    }

    if (type->tp_dictoffset <= 0) {
        goto fail_to_optimize;
    }

    if (type->tp_dict == NULL) {
        if (PyType_Ready(type) < 0) {
            Py_DECREF(owner);
            return -1;
        }
    }

    PyObject *descr = _PyType_Lookup(type, name);
    if (descr != NULL && descr->ob_type->tp_descr_get != NULL && PyDescr_IsData(descr)) {
        goto fail_to_optimize;
    }

    PyObject *dict = *((PyObject **) ((char *)owner + type->tp_dictoffset));

    if (dict == NULL || !PyDict_CheckExact(dict)) {
        goto fail_to_optimize;
    }

    Py_INCREF(dict);
    Py_ssize_t ret = _PyDict_GetItemHint((PyDictObject*)dict, name, -1, result);
    Py_DECREF(dict);
    if (*result == NULL) {
        goto fail_to_optimize;
    }

    if (co_opcache->optimized == 0) {
        // First time we optimize this opcode. */
        OPCACHE_STAT_ATTR_OPT();
        co_opcache->optimized = OPCODE_CACHE_MAX_TRIES;
    }

    _PyOpCodeOpt_LoadAttr *la = &co_opcache->u.la;
    la->type = type;
    la->tp_version_tag = type->tp_version_tag;
    la->hint = ret;

    Py_INCREF(*result);
    return 0;

fail_to_optimize:
    OPCACHE_DEOPT_LOAD_ATTR(co_opcache_ptr, co);
    return 0;
}

int _PyOpcodeCache_LoadAttr(PyCodeObject *co, _PyOpcache **co_opcache_ptr,
                            PyTypeObject* type, PyObject* name,
                            PyObject* owner, PyObject** result) {

    _PyOpcache *co_opcache = *co_opcache_ptr;
    *result = NULL;

    if (co_opcache == NULL || !PyType_HasFeature(type, Py_TPFLAGS_VALID_VERSION_TAG))
    {
        return 0;
    }

    if (co_opcache->optimized <= 0) {
        // If we have not optimized this opcode, we need to try to prime the cache.
        return _PyOpcodeCache_InitializeLoadAttr(co, co_opcache_ptr, type, name, owner, result);
    }

    _PyOpCodeOpt_LoadAttr *la = &co_opcache->u.la;
    if (la->type != type || la->tp_version_tag != type->tp_version_tag)
    {
        // The type of the object has either been updated,
        // or is different.  Maybe it will stabilize?
        OPCACHE_MAYBE_DEOPT_LOAD_ATTR(co_opcache_ptr, co);
        goto cache_miss;
    }
    assert(type->tp_dict != NULL);
    assert(type->tp_dictoffset > 0);

    PyObject *dict = *((PyObject **) ((char *)owner + type->tp_dictoffset));

    if (dict == NULL || !PyDict_CheckExact(dict)) {
        OPCACHE_DEOPT_LOAD_ATTR(co_opcache_ptr, co);
        goto cache_miss;
    }

    Py_ssize_t hint = la->hint;
    Py_INCREF(dict);
    la->hint = _PyDict_GetItemHint((PyDictObject*)dict, name, hint, result);
    Py_DECREF(dict);

    if (*result == NULL) {
        // This attribute can be missing sometimes -- we
        // don't want to optimize this lookup.
        OPCACHE_DEOPT_LOAD_ATTR(co_opcache_ptr, co);
        goto cache_miss;
    }

    Py_INCREF(*result);

    if (la->hint != hint || hint < 0) {
        /* The hint we provided didn't work. Maybe next time? */
        OPCACHE_MAYBE_DEOPT_LOAD_ATTR(co_opcache_ptr, co);
        goto cache_miss;
    }

    OPCACHE_STAT_ATTR_HIT();
    return 0;

cache_miss:
    OPCACHE_STAT_ATTR_MISS();
    return 0;
}
