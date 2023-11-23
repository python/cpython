#include <stdbool.h>

#include "Python.h"
#include "opcode.h"
#include "structmember.h"         // PyMemberDef
#include "pycore_code.h"          // _PyCodeConstructor
#include "pycore_frame.h"         // FRAME_SPECIALS_SIZE
#include "pycore_interp.h"        // PyInterpreterState.co_extra_freefuncs
#include "pycore_opcode.h"        // _PyOpcode_Deopt
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "clinic/codeobject.c.h"

static PyObject* code_repr(PyCodeObject *co);

static const char *
code_event_name(PyCodeEvent event) {
    switch (event) {
        #define CASE(op)                \
        case PY_CODE_EVENT_##op:         \
            return "PY_CODE_EVENT_" #op;
        PY_FOREACH_CODE_EVENT(CASE)
        #undef CASE
    }
    Py_UNREACHABLE();
}

static void
notify_code_watchers(PyCodeEvent event, PyCodeObject *co)
{
    assert(Py_REFCNT(co) > 0);
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->_initialized);
    uint8_t bits = interp->active_code_watchers;
    int i = 0;
    while (bits) {
        assert(i < CODE_MAX_WATCHERS);
        if (bits & 1) {
            PyCode_WatchCallback cb = interp->code_watchers[i];
            // callback must be non-null if the watcher bit is set
            assert(cb != NULL);
            if (cb(event, co) < 0) {
                // Don't risk resurrecting the object if an unraisablehook keeps
                // a reference; pass a string as context.
                PyObject *context = NULL;
                PyObject *repr = code_repr(co);
                if (repr) {
                    context = PyUnicode_FromFormat(
                        "%s watcher callback for %U",
                        code_event_name(event), repr);
                    Py_DECREF(repr);
                }
                if (context == NULL) {
                    context = Py_NewRef(Py_None);
                }
                PyErr_WriteUnraisable(context);
                Py_DECREF(context);
            }
        }
        i++;
        bits >>= 1;
    }
}

int
PyCode_AddWatcher(PyCode_WatchCallback callback)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->_initialized);

    for (int i = 0; i < CODE_MAX_WATCHERS; i++) {
        if (!interp->code_watchers[i]) {
            interp->code_watchers[i] = callback;
            interp->active_code_watchers |= (1 << i);
            return i;
        }
    }

    PyErr_SetString(PyExc_RuntimeError, "no more code watcher IDs available");
    return -1;
}

static inline int
validate_watcher_id(PyInterpreterState *interp, int watcher_id)
{
    if (watcher_id < 0 || watcher_id >= CODE_MAX_WATCHERS) {
        PyErr_Format(PyExc_ValueError, "Invalid code watcher ID %d", watcher_id);
        return -1;
    }
    if (!interp->code_watchers[watcher_id]) {
        PyErr_Format(PyExc_ValueError, "No code watcher set for ID %d", watcher_id);
        return -1;
    }
    return 0;
}

int
PyCode_ClearWatcher(int watcher_id)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->_initialized);
    if (validate_watcher_id(interp, watcher_id) < 0) {
        return -1;
    }
    interp->code_watchers[watcher_id] = NULL;
    interp->active_code_watchers &= ~(1 << watcher_id);
    return 0;
}

/******************
 * generic helpers
 ******************/

/* all_name_chars(s): true iff s matches [a-zA-Z0-9_]* */
static int
all_name_chars(PyObject *o)
{
    const unsigned char *s, *e;

    if (!PyUnicode_IS_ASCII(o))
        return 0;

    s = PyUnicode_1BYTE_DATA(o);
    e = s + PyUnicode_GET_LENGTH(o);
    for (; s != e; s++) {
        if (!Py_ISALNUM(*s) && *s != '_')
            return 0;
    }
    return 1;
}

static int
intern_strings(PyObject *tuple)
{
    Py_ssize_t i;

    for (i = PyTuple_GET_SIZE(tuple); --i >= 0; ) {
        PyObject *v = PyTuple_GET_ITEM(tuple, i);
        if (v == NULL || !PyUnicode_CheckExact(v)) {
            PyErr_SetString(PyExc_SystemError,
                            "non-string found in code slot");
            return -1;
        }
        PyUnicode_InternInPlace(&_PyTuple_ITEMS(tuple)[i]);
    }
    return 0;
}

/* Intern selected string constants */
static int
intern_string_constants(PyObject *tuple, int *modified)
{
    for (Py_ssize_t i = PyTuple_GET_SIZE(tuple); --i >= 0; ) {
        PyObject *v = PyTuple_GET_ITEM(tuple, i);
        if (PyUnicode_CheckExact(v)) {
            if (PyUnicode_READY(v) == -1) {
                return -1;
            }

            if (all_name_chars(v)) {
                PyObject *w = v;
                PyUnicode_InternInPlace(&v);
                if (w != v) {
                    PyTuple_SET_ITEM(tuple, i, v);
                    if (modified) {
                        *modified = 1;
                    }
                }
            }
        }
        else if (PyTuple_CheckExact(v)) {
            if (intern_string_constants(v, NULL) < 0) {
                return -1;
            }
        }
        else if (PyFrozenSet_CheckExact(v)) {
            PyObject *w = v;
            PyObject *tmp = PySequence_Tuple(v);
            if (tmp == NULL) {
                return -1;
            }
            int tmp_modified = 0;
            if (intern_string_constants(tmp, &tmp_modified) < 0) {
                Py_DECREF(tmp);
                return -1;
            }
            if (tmp_modified) {
                v = PyFrozenSet_New(tmp);
                if (v == NULL) {
                    Py_DECREF(tmp);
                    return -1;
                }

                PyTuple_SET_ITEM(tuple, i, v);
                Py_DECREF(w);
                if (modified) {
                    *modified = 1;
                }
            }
            Py_DECREF(tmp);
        }
    }
    return 0;
}

/* Return a shallow copy of a tuple that is
   guaranteed to contain exact strings, by converting string subclasses
   to exact strings and complaining if a non-string is found. */
static PyObject*
validate_and_copy_tuple(PyObject *tup)
{
    PyObject *newtuple;
    PyObject *item;
    Py_ssize_t i, len;

    len = PyTuple_GET_SIZE(tup);
    newtuple = PyTuple_New(len);
    if (newtuple == NULL)
        return NULL;

    for (i = 0; i < len; i++) {
        item = PyTuple_GET_ITEM(tup, i);
        if (PyUnicode_CheckExact(item)) {
            Py_INCREF(item);
        }
        else if (!PyUnicode_Check(item)) {
            PyErr_Format(
                PyExc_TypeError,
                "name tuples must contain only "
                "strings, not '%.500s'",
                Py_TYPE(item)->tp_name);
            Py_DECREF(newtuple);
            return NULL;
        }
        else {
            item = _PyUnicode_Copy(item);
            if (item == NULL) {
                Py_DECREF(newtuple);
                return NULL;
            }
        }
        PyTuple_SET_ITEM(newtuple, i, item);
    }

    return newtuple;
}

static int
init_co_cached(PyCodeObject *self) {
    if (self->_co_cached == NULL) {
        self->_co_cached = PyMem_New(_PyCoCached, 1);
        if (self->_co_cached == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        self->_co_cached->_co_code = NULL;
        self->_co_cached->_co_cellvars = NULL;
        self->_co_cached->_co_freevars = NULL;
        self->_co_cached->_co_varnames = NULL;
    }
    return 0;

}
/******************
 * _PyCode_New()
 ******************/

// This is also used in compile.c.
void
_Py_set_localsplus_info(int offset, PyObject *name, _PyLocals_Kind kind,
                        PyObject *names, PyObject *kinds)
{
    PyTuple_SET_ITEM(names, offset, Py_NewRef(name));
    _PyLocals_SetKind(kinds, offset, kind);
}

static void
get_localsplus_counts(PyObject *names, PyObject *kinds,
                      int *pnlocals, int *pncellvars,
                      int *pnfreevars)
{
    int nlocals = 0;
    int ncellvars = 0;
    int nfreevars = 0;
    Py_ssize_t nlocalsplus = PyTuple_GET_SIZE(names);
    for (int i = 0; i < nlocalsplus; i++) {
        _PyLocals_Kind kind = _PyLocals_GetKind(kinds, i);
        if (kind & CO_FAST_LOCAL) {
            nlocals += 1;
            if (kind & CO_FAST_CELL) {
                ncellvars += 1;
            }
        }
        else if (kind & CO_FAST_CELL) {
            ncellvars += 1;
        }
        else if (kind & CO_FAST_FREE) {
            nfreevars += 1;
        }
    }
    if (pnlocals != NULL) {
        *pnlocals = nlocals;
    }
    if (pncellvars != NULL) {
        *pncellvars = ncellvars;
    }
    if (pnfreevars != NULL) {
        *pnfreevars = nfreevars;
    }
}

static PyObject *
get_localsplus_names(PyCodeObject *co, _PyLocals_Kind kind, int num)
{
    PyObject *names = PyTuple_New(num);
    if (names == NULL) {
        return NULL;
    }
    int index = 0;
    for (int offset = 0; offset < co->co_nlocalsplus; offset++) {
        _PyLocals_Kind k = _PyLocals_GetKind(co->co_localspluskinds, offset);
        if ((k & kind) == 0) {
            continue;
        }
        assert(index < num);
        PyObject *name = PyTuple_GET_ITEM(co->co_localsplusnames, offset);
        PyTuple_SET_ITEM(names, index, Py_NewRef(name));
        index += 1;
    }
    assert(index == num);
    return names;
}

int
_PyCode_Validate(struct _PyCodeConstructor *con)
{
    /* Check argument types */
    if (con->argcount < con->posonlyargcount || con->posonlyargcount < 0 ||
        con->kwonlyargcount < 0 ||
        con->stacksize < 0 || con->flags < 0 ||
        con->code == NULL || !PyBytes_Check(con->code) ||
        con->consts == NULL || !PyTuple_Check(con->consts) ||
        con->names == NULL || !PyTuple_Check(con->names) ||
        con->localsplusnames == NULL || !PyTuple_Check(con->localsplusnames) ||
        con->localspluskinds == NULL || !PyBytes_Check(con->localspluskinds) ||
        PyTuple_GET_SIZE(con->localsplusnames)
            != PyBytes_GET_SIZE(con->localspluskinds) ||
        con->name == NULL || !PyUnicode_Check(con->name) ||
        con->qualname == NULL || !PyUnicode_Check(con->qualname) ||
        con->filename == NULL || !PyUnicode_Check(con->filename) ||
        con->linetable == NULL || !PyBytes_Check(con->linetable) ||
        con->exceptiontable == NULL || !PyBytes_Check(con->exceptiontable)
        ) {
        PyErr_BadInternalCall();
        return -1;
    }

    /* Make sure that code is indexable with an int, this is
       a long running assumption in ceval.c and many parts of
       the interpreter. */
    if (PyBytes_GET_SIZE(con->code) > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "code: co_code larger than INT_MAX");
        return -1;
    }
    if (PyBytes_GET_SIZE(con->code) % sizeof(_Py_CODEUNIT) != 0 ||
        !_Py_IS_ALIGNED(PyBytes_AS_STRING(con->code), sizeof(_Py_CODEUNIT))
        ) {
        PyErr_SetString(PyExc_ValueError, "code: co_code is malformed");
        return -1;
    }

    /* Ensure that the co_varnames has enough names to cover the arg counts.
     * Note that totalargs = nlocals - nplainlocals.  We check nplainlocals
     * here to avoid the possibility of overflow (however remote). */
    int nlocals;
    get_localsplus_counts(con->localsplusnames, con->localspluskinds,
                          &nlocals, NULL, NULL);
    int nplainlocals = nlocals -
                       con->argcount -
                       con->kwonlyargcount -
                       ((con->flags & CO_VARARGS) != 0) -
                       ((con->flags & CO_VARKEYWORDS) != 0);
    if (nplainlocals < 0) {
        PyErr_SetString(PyExc_ValueError, "code: co_varnames is too small");
        return -1;
    }

    return 0;
}

extern void _PyCode_Quicken(PyCodeObject *code);

static void
init_code(PyCodeObject *co, struct _PyCodeConstructor *con)
{
    int nlocalsplus = (int)PyTuple_GET_SIZE(con->localsplusnames);
    int nlocals, ncellvars, nfreevars;
    get_localsplus_counts(con->localsplusnames, con->localspluskinds,
                          &nlocals, &ncellvars, &nfreevars);

    co->co_filename = Py_NewRef(con->filename);
    co->co_name = Py_NewRef(con->name);
    co->co_qualname = Py_NewRef(con->qualname);
    co->co_flags = con->flags;

    co->co_firstlineno = con->firstlineno;
    co->co_linetable = Py_NewRef(con->linetable);

    co->co_consts = Py_NewRef(con->consts);
    co->co_names = Py_NewRef(con->names);

    co->co_localsplusnames = Py_NewRef(con->localsplusnames);
    co->co_localspluskinds = Py_NewRef(con->localspluskinds);

    co->co_argcount = con->argcount;
    co->co_posonlyargcount = con->posonlyargcount;
    co->co_kwonlyargcount = con->kwonlyargcount;

    co->co_stacksize = con->stacksize;

    co->co_exceptiontable = Py_NewRef(con->exceptiontable);

    /* derived values */
    co->co_nlocalsplus = nlocalsplus;
    co->co_nlocals = nlocals;
    co->co_framesize = nlocalsplus + con->stacksize + FRAME_SPECIALS_SIZE;
    co->co_ncellvars = ncellvars;
    co->co_nfreevars = nfreevars;
    co->co_version = _Py_next_func_version;
    if (_Py_next_func_version != 0) {
        _Py_next_func_version++;
    }
    co->_co_monitoring = NULL;
    co->_co_instrumentation_version = 0;
    /* not set */
    co->co_weakreflist = NULL;
    co->co_extra = NULL;
    co->_co_cached = NULL;

    memcpy(_PyCode_CODE(co), PyBytes_AS_STRING(con->code),
           PyBytes_GET_SIZE(con->code));
    int entry_point = 0;
    while (entry_point < Py_SIZE(co) &&
        _PyCode_CODE(co)[entry_point].op.code != RESUME) {
        entry_point++;
    }
    co->_co_firsttraceable = entry_point;
    _PyCode_Quicken(co);
    notify_code_watchers(PY_CODE_EVENT_CREATE, co);
}

static int
scan_varint(const uint8_t *ptr)
{
    unsigned int read = *ptr++;
    unsigned int val = read & 63;
    unsigned int shift = 0;
    while (read & 64) {
        read = *ptr++;
        shift += 6;
        val |= (read & 63) << shift;
    }
    return val;
}

static int
scan_signed_varint(const uint8_t *ptr)
{
    unsigned int uval = scan_varint(ptr);
    if (uval & 1) {
        return -(int)(uval >> 1);
    }
    else {
        return uval >> 1;
    }
}

static int
get_line_delta(const uint8_t *ptr)
{
    int code = ((*ptr) >> 3) & 15;
    switch (code) {
        case PY_CODE_LOCATION_INFO_NONE:
            return 0;
        case PY_CODE_LOCATION_INFO_NO_COLUMNS:
        case PY_CODE_LOCATION_INFO_LONG:
            return scan_signed_varint(ptr+1);
        case PY_CODE_LOCATION_INFO_ONE_LINE0:
            return 0;
        case PY_CODE_LOCATION_INFO_ONE_LINE1:
            return 1;
        case PY_CODE_LOCATION_INFO_ONE_LINE2:
            return 2;
        default:
            /* Same line */
            return 0;
    }
}

static PyObject *
remove_column_info(PyObject *locations)
{
    int offset = 0;
    const uint8_t *data = (const uint8_t *)PyBytes_AS_STRING(locations);
    PyObject *res = PyBytes_FromStringAndSize(NULL, 32);
    if (res == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    uint8_t *output = (uint8_t *)PyBytes_AS_STRING(res);
    while (offset < PyBytes_GET_SIZE(locations)) {
        Py_ssize_t write_offset = output - (uint8_t *)PyBytes_AS_STRING(res);
        if (write_offset + 16 >= PyBytes_GET_SIZE(res)) {
            if (_PyBytes_Resize(&res, PyBytes_GET_SIZE(res) * 2) < 0) {
                return NULL;
            }
            output = (uint8_t *)PyBytes_AS_STRING(res) + write_offset;
        }
        int code = (data[offset] >> 3) & 15;
        if (code == PY_CODE_LOCATION_INFO_NONE) {
            *output++ = data[offset];
        }
        else {
            int blength = (data[offset] & 7)+1;
            output += write_location_entry_start(
                output, PY_CODE_LOCATION_INFO_NO_COLUMNS, blength);
            int ldelta = get_line_delta(&data[offset]);
            output += write_signed_varint(output, ldelta);
        }
        offset++;
        while (offset < PyBytes_GET_SIZE(locations) &&
            (data[offset] & 128) == 0) {
            offset++;
        }
    }
    Py_ssize_t write_offset = output - (uint8_t *)PyBytes_AS_STRING(res);
    if (_PyBytes_Resize(&res, write_offset)) {
        return NULL;
    }
    return res;
}

/* The caller is responsible for ensuring that the given data is valid. */

PyCodeObject *
_PyCode_New(struct _PyCodeConstructor *con)
{
    /* Ensure that strings are ready Unicode string */
    if (PyUnicode_READY(con->name) < 0) {
        return NULL;
    }
    if (PyUnicode_READY(con->qualname) < 0) {
        return NULL;
    }
    if (PyUnicode_READY(con->filename) < 0) {
        return NULL;
    }

    if (intern_strings(con->names) < 0) {
        return NULL;
    }
    if (intern_string_constants(con->consts, NULL) < 0) {
        return NULL;
    }
    if (intern_strings(con->localsplusnames) < 0) {
        return NULL;
    }

    PyObject *replacement_locations = NULL;
    // Compact the linetable if we are opted out of debug
    // ranges.
    if (!_Py_GetConfig()->code_debug_ranges) {
        replacement_locations = remove_column_info(con->linetable);
        if (replacement_locations == NULL) {
            return NULL;
        }
        con->linetable = replacement_locations;
    }

    Py_ssize_t size = PyBytes_GET_SIZE(con->code) / sizeof(_Py_CODEUNIT);
    PyCodeObject *co = PyObject_NewVar(PyCodeObject, &PyCode_Type, size);
    if (co == NULL) {
        Py_XDECREF(replacement_locations);
        PyErr_NoMemory();
        return NULL;
    }
    init_code(co, con);
    Py_XDECREF(replacement_locations);
    return co;
}


/******************
 * the legacy "constructors"
 ******************/

PyCodeObject *
PyUnstable_Code_NewWithPosOnlyArgs(
                          int argcount, int posonlyargcount, int kwonlyargcount,
                          int nlocals, int stacksize, int flags,
                          PyObject *code, PyObject *consts, PyObject *names,
                          PyObject *varnames, PyObject *freevars, PyObject *cellvars,
                          PyObject *filename, PyObject *name,
                          PyObject *qualname, int firstlineno,
                          PyObject *linetable,
                          PyObject *exceptiontable)
{
    PyCodeObject *co = NULL;
    PyObject *localsplusnames = NULL;
    PyObject *localspluskinds = NULL;

    if (varnames == NULL || !PyTuple_Check(varnames) ||
        cellvars == NULL || !PyTuple_Check(cellvars) ||
        freevars == NULL || !PyTuple_Check(freevars)
        ) {
        PyErr_BadInternalCall();
        return NULL;
    }

    // Set the "fast locals plus" info.
    int nvarnames = (int)PyTuple_GET_SIZE(varnames);
    int ncellvars = (int)PyTuple_GET_SIZE(cellvars);
    int nfreevars = (int)PyTuple_GET_SIZE(freevars);
    int nlocalsplus = nvarnames + ncellvars + nfreevars;
    localsplusnames = PyTuple_New(nlocalsplus);
    if (localsplusnames == NULL) {
        goto error;
    }
    localspluskinds = PyBytes_FromStringAndSize(NULL, nlocalsplus);
    if (localspluskinds == NULL) {
        goto error;
    }
    int  offset = 0;
    for (int i = 0; i < nvarnames; i++, offset++) {
        PyObject *name = PyTuple_GET_ITEM(varnames, i);
        _Py_set_localsplus_info(offset, name, CO_FAST_LOCAL,
                               localsplusnames, localspluskinds);
    }
    for (int i = 0; i < ncellvars; i++, offset++) {
        PyObject *name = PyTuple_GET_ITEM(cellvars, i);
        int argoffset = -1;
        for (int j = 0; j < nvarnames; j++) {
            int cmp = PyUnicode_Compare(PyTuple_GET_ITEM(varnames, j),
                                        name);
            assert(!PyErr_Occurred());
            if (cmp == 0) {
                argoffset = j;
                break;
            }
        }
        if (argoffset >= 0) {
            // Merge the localsplus indices.
            nlocalsplus -= 1;
            offset -= 1;
            _PyLocals_Kind kind = _PyLocals_GetKind(localspluskinds, argoffset);
            _PyLocals_SetKind(localspluskinds, argoffset, kind | CO_FAST_CELL);
            continue;
        }
        _Py_set_localsplus_info(offset, name, CO_FAST_CELL,
                               localsplusnames, localspluskinds);
    }
    for (int i = 0; i < nfreevars; i++, offset++) {
        PyObject *name = PyTuple_GET_ITEM(freevars, i);
        _Py_set_localsplus_info(offset, name, CO_FAST_FREE,
                               localsplusnames, localspluskinds);
    }

    // gh-110543: Make sure the CO_FAST_HIDDEN flag is set correctly.
    if (!(flags & CO_OPTIMIZED)) {
        Py_ssize_t code_len = PyBytes_GET_SIZE(code);
        _Py_CODEUNIT *code_data = (_Py_CODEUNIT *)PyBytes_AS_STRING(code);
        Py_ssize_t num_code_units = code_len / sizeof(_Py_CODEUNIT);
        int extended_arg = 0;
        for (int i = 0; i < num_code_units; i += 1 + _PyOpcode_Caches[code_data[i].op.code]) {
            _Py_CODEUNIT *instr = &code_data[i];
            uint8_t opcode = instr->op.code;
            if (opcode == EXTENDED_ARG) {
                extended_arg = extended_arg << 8 | instr->op.arg;
                continue;
            }
            if (opcode == LOAD_FAST_AND_CLEAR) {
                int oparg = extended_arg << 8 | instr->op.arg;
                if (oparg >= nlocalsplus) {
                    PyErr_Format(PyExc_ValueError,
                                "code: LOAD_FAST_AND_CLEAR oparg %d out of range",
                                oparg);
                    goto error;
                }
                _PyLocals_Kind kind = _PyLocals_GetKind(localspluskinds, oparg);
                _PyLocals_SetKind(localspluskinds, oparg, kind | CO_FAST_HIDDEN);
            }
            extended_arg = 0;
        }
    }

    // If any cells were args then nlocalsplus will have shrunk.
    if (nlocalsplus != PyTuple_GET_SIZE(localsplusnames)) {
        if (_PyTuple_Resize(&localsplusnames, nlocalsplus) < 0
                || _PyBytes_Resize(&localspluskinds, nlocalsplus) < 0) {
            goto error;
        }
    }

    struct _PyCodeConstructor con = {
        .filename = filename,
        .name = name,
        .qualname = qualname,
        .flags = flags,

        .code = code,
        .firstlineno = firstlineno,
        .linetable = linetable,

        .consts = consts,
        .names = names,

        .localsplusnames = localsplusnames,
        .localspluskinds = localspluskinds,

        .argcount = argcount,
        .posonlyargcount = posonlyargcount,
        .kwonlyargcount = kwonlyargcount,

        .stacksize = stacksize,

        .exceptiontable = exceptiontable,
    };

    if (_PyCode_Validate(&con) < 0) {
        goto error;
    }
    assert(PyBytes_GET_SIZE(code) % sizeof(_Py_CODEUNIT) == 0);
    assert(_Py_IS_ALIGNED(PyBytes_AS_STRING(code), sizeof(_Py_CODEUNIT)));
    if (nlocals != PyTuple_GET_SIZE(varnames)) {
        PyErr_SetString(PyExc_ValueError,
                        "code: co_nlocals != len(co_varnames)");
        goto error;
    }

    co = _PyCode_New(&con);
    if (co == NULL) {
        goto error;
    }

error:
    Py_XDECREF(localsplusnames);
    Py_XDECREF(localspluskinds);
    return co;
}

PyCodeObject *
PyUnstable_Code_New(int argcount, int kwonlyargcount,
           int nlocals, int stacksize, int flags,
           PyObject *code, PyObject *consts, PyObject *names,
           PyObject *varnames, PyObject *freevars, PyObject *cellvars,
           PyObject *filename, PyObject *name, PyObject *qualname,
           int firstlineno,
           PyObject *linetable,
           PyObject *exceptiontable)
{
    return PyCode_NewWithPosOnlyArgs(argcount, 0, kwonlyargcount, nlocals,
                                     stacksize, flags, code, consts, names,
                                     varnames, freevars, cellvars, filename,
                                     name, qualname, firstlineno,
                                     linetable,
                                     exceptiontable);
}

// NOTE: When modifying the construction of PyCode_NewEmpty, please also change
// test.test_code.CodeLocationTest.test_code_new_empty to keep it in sync!

static const uint8_t assert0[6] = {
    RESUME, 0,
    LOAD_ASSERTION_ERROR, 0,
    RAISE_VARARGS, 1
};

static const uint8_t linetable[2] = {
    (1 << 7)  // New entry.
    | (PY_CODE_LOCATION_INFO_NO_COLUMNS << 3)
    | (3 - 1),  // Three code units.
    0,  // Offset from co_firstlineno.
};

PyCodeObject *
PyCode_NewEmpty(const char *filename, const char *funcname, int firstlineno)
{
    PyObject *nulltuple = NULL;
    PyObject *filename_ob = NULL;
    PyObject *funcname_ob = NULL;
    PyObject *code_ob = NULL;
    PyObject *linetable_ob = NULL;
    PyCodeObject *result = NULL;

    nulltuple = PyTuple_New(0);
    if (nulltuple == NULL) {
        goto failed;
    }
    funcname_ob = PyUnicode_FromString(funcname);
    if (funcname_ob == NULL) {
        goto failed;
    }
    filename_ob = PyUnicode_DecodeFSDefault(filename);
    if (filename_ob == NULL) {
        goto failed;
    }
    code_ob = PyBytes_FromStringAndSize((const char *)assert0, 6);
    if (code_ob == NULL) {
        goto failed;
    }
    linetable_ob = PyBytes_FromStringAndSize((const char *)linetable, 2);
    if (linetable_ob == NULL) {
        goto failed;
    }

#define emptystring (PyObject *)&_Py_SINGLETON(bytes_empty)
    struct _PyCodeConstructor con = {
        .filename = filename_ob,
        .name = funcname_ob,
        .qualname = funcname_ob,
        .code = code_ob,
        .firstlineno = firstlineno,
        .linetable = linetable_ob,
        .consts = nulltuple,
        .names = nulltuple,
        .localsplusnames = nulltuple,
        .localspluskinds = emptystring,
        .exceptiontable = emptystring,
        .stacksize = 1,
    };
    result = _PyCode_New(&con);

failed:
    Py_XDECREF(nulltuple);
    Py_XDECREF(funcname_ob);
    Py_XDECREF(filename_ob);
    Py_XDECREF(code_ob);
    Py_XDECREF(linetable_ob);
    return result;
}


/******************
 * source location tracking (co_lines/co_positions)
 ******************/

int
PyCode_Addr2Line(PyCodeObject *co, int addrq)
{
    if (addrq < 0) {
        return co->co_firstlineno;
    }
    assert(addrq >= 0 && addrq < _PyCode_NBYTES(co));
    PyCodeAddressRange bounds;
    _PyCode_InitAddressRange(co, &bounds);
    return _PyCode_CheckLineNumber(addrq, &bounds);
}

void
_PyLineTable_InitAddressRange(const char *linetable, Py_ssize_t length, int firstlineno, PyCodeAddressRange *range)
{
    range->opaque.lo_next = (const uint8_t *)linetable;
    range->opaque.limit = range->opaque.lo_next + length;
    range->ar_start = -1;
    range->ar_end = 0;
    range->opaque.computed_line = firstlineno;
    range->ar_line = -1;
}

int
_PyCode_InitAddressRange(PyCodeObject* co, PyCodeAddressRange *bounds)
{
    assert(co->co_linetable != NULL);
    const char *linetable = PyBytes_AS_STRING(co->co_linetable);
    Py_ssize_t length = PyBytes_GET_SIZE(co->co_linetable);
    _PyLineTable_InitAddressRange(linetable, length, co->co_firstlineno, bounds);
    return bounds->ar_line;
}

/* Update *bounds to describe the first and one-past-the-last instructions in
   the same line as lasti.  Return the number of that line, or -1 if lasti is out of bounds. */
int
_PyCode_CheckLineNumber(int lasti, PyCodeAddressRange *bounds)
{
    while (bounds->ar_end <= lasti) {
        if (!_PyLineTable_NextAddressRange(bounds)) {
            return -1;
        }
    }
    while (bounds->ar_start > lasti) {
        if (!_PyLineTable_PreviousAddressRange(bounds)) {
            return -1;
        }
    }
    return bounds->ar_line;
}

static int
is_no_line_marker(uint8_t b)
{
    return (b >> 3) == 0x1f;
}


#define ASSERT_VALID_BOUNDS(bounds) \
    assert(bounds->opaque.lo_next <=  bounds->opaque.limit && \
        (bounds->ar_line == -1 || bounds->ar_line == bounds->opaque.computed_line) && \
        (bounds->opaque.lo_next == bounds->opaque.limit || \
        (*bounds->opaque.lo_next) & 128))

static int
next_code_delta(PyCodeAddressRange *bounds)
{
    assert((*bounds->opaque.lo_next) & 128);
    return (((*bounds->opaque.lo_next) & 7) + 1) * sizeof(_Py_CODEUNIT);
}

static int
previous_code_delta(PyCodeAddressRange *bounds)
{
    if (bounds->ar_start == 0) {
        // If we looking at the first entry, the
        // "previous" entry has an implicit length of 1.
        return 1;
    }
    const uint8_t *ptr = bounds->opaque.lo_next-1;
    while (((*ptr) & 128) == 0) {
        ptr--;
    }
    return (((*ptr) & 7) + 1) * sizeof(_Py_CODEUNIT);
}

static int
read_byte(PyCodeAddressRange *bounds)
{
    return *bounds->opaque.lo_next++;
}

static int
read_varint(PyCodeAddressRange *bounds)
{
    unsigned int read = read_byte(bounds);
    unsigned int val = read & 63;
    unsigned int shift = 0;
    while (read & 64) {
        read = read_byte(bounds);
        shift += 6;
        val |= (read & 63) << shift;
    }
    return val;
}

static int
read_signed_varint(PyCodeAddressRange *bounds)
{
    unsigned int uval = read_varint(bounds);
    if (uval & 1) {
        return -(int)(uval >> 1);
    }
    else {
        return uval >> 1;
    }
}

static void
retreat(PyCodeAddressRange *bounds)
{
    ASSERT_VALID_BOUNDS(bounds);
    assert(bounds->ar_start >= 0);
    do {
        bounds->opaque.lo_next--;
    } while (((*bounds->opaque.lo_next) & 128) == 0);
    bounds->opaque.computed_line -= get_line_delta(bounds->opaque.lo_next);
    bounds->ar_end = bounds->ar_start;
    bounds->ar_start -= previous_code_delta(bounds);
    if (is_no_line_marker(bounds->opaque.lo_next[-1])) {
        bounds->ar_line = -1;
    }
    else {
        bounds->ar_line = bounds->opaque.computed_line;
    }
    ASSERT_VALID_BOUNDS(bounds);
}

static void
advance(PyCodeAddressRange *bounds)
{
    ASSERT_VALID_BOUNDS(bounds);
    bounds->opaque.computed_line += get_line_delta(bounds->opaque.lo_next);
    if (is_no_line_marker(*bounds->opaque.lo_next)) {
        bounds->ar_line = -1;
    }
    else {
        bounds->ar_line = bounds->opaque.computed_line;
    }
    bounds->ar_start = bounds->ar_end;
    bounds->ar_end += next_code_delta(bounds);
    do {
        bounds->opaque.lo_next++;
    } while (bounds->opaque.lo_next < bounds->opaque.limit &&
        ((*bounds->opaque.lo_next) & 128) == 0);
    ASSERT_VALID_BOUNDS(bounds);
}

static void
advance_with_locations(PyCodeAddressRange *bounds, int *endline, int *column, int *endcolumn)
{
    ASSERT_VALID_BOUNDS(bounds);
    int first_byte = read_byte(bounds);
    int code = (first_byte >> 3) & 15;
    bounds->ar_start = bounds->ar_end;
    bounds->ar_end = bounds->ar_start + ((first_byte & 7) + 1) * sizeof(_Py_CODEUNIT);
    switch(code) {
        case PY_CODE_LOCATION_INFO_NONE:
            bounds->ar_line = *endline = -1;
            *column =  *endcolumn = -1;
            break;
        case PY_CODE_LOCATION_INFO_LONG:
        {
            bounds->opaque.computed_line += read_signed_varint(bounds);
            bounds->ar_line = bounds->opaque.computed_line;
            *endline = bounds->ar_line + read_varint(bounds);
            *column = read_varint(bounds)-1;
            *endcolumn = read_varint(bounds)-1;
            break;
        }
        case PY_CODE_LOCATION_INFO_NO_COLUMNS:
        {
            /* No column */
            bounds->opaque.computed_line += read_signed_varint(bounds);
            *endline = bounds->ar_line = bounds->opaque.computed_line;
            *column = *endcolumn = -1;
            break;
        }
        case PY_CODE_LOCATION_INFO_ONE_LINE0:
        case PY_CODE_LOCATION_INFO_ONE_LINE1:
        case PY_CODE_LOCATION_INFO_ONE_LINE2:
        {
            /* one line form */
            int line_delta = code - 10;
            bounds->opaque.computed_line += line_delta;
            *endline = bounds->ar_line = bounds->opaque.computed_line;
            *column = read_byte(bounds);
            *endcolumn = read_byte(bounds);
            break;
        }
        default:
        {
            /* Short forms */
            int second_byte = read_byte(bounds);
            assert((second_byte & 128) == 0);
            *endline = bounds->ar_line = bounds->opaque.computed_line;
            *column = code << 3 | (second_byte >> 4);
            *endcolumn = *column + (second_byte & 15);
        }
    }
    ASSERT_VALID_BOUNDS(bounds);
}
int
PyCode_Addr2Location(PyCodeObject *co, int addrq,
                     int *start_line, int *start_column,
                     int *end_line, int *end_column)
{
    if (addrq < 0) {
        *start_line = *end_line = co->co_firstlineno;
        *start_column = *end_column = 0;
        return 1;
    }
    assert(addrq >= 0 && addrq < _PyCode_NBYTES(co));
    PyCodeAddressRange bounds;
    _PyCode_InitAddressRange(co, &bounds);
    _PyCode_CheckLineNumber(addrq, &bounds);
    retreat(&bounds);
    advance_with_locations(&bounds, end_line, start_column, end_column);
    *start_line = bounds.ar_line;
    return 1;
}


static inline int
at_end(PyCodeAddressRange *bounds) {
    return bounds->opaque.lo_next >= bounds->opaque.limit;
}

int
_PyLineTable_PreviousAddressRange(PyCodeAddressRange *range)
{
    if (range->ar_start <= 0) {
        return 0;
    }
    retreat(range);
    assert(range->ar_end > range->ar_start);
    return 1;
}

int
_PyLineTable_NextAddressRange(PyCodeAddressRange *range)
{
    if (at_end(range)) {
        return 0;
    }
    advance(range);
    assert(range->ar_end > range->ar_start);
    return 1;
}

static int
emit_pair(PyObject **bytes, int *offset, int a, int b)
{
    Py_ssize_t len = PyBytes_GET_SIZE(*bytes);
    if (*offset + 2 >= len) {
        if (_PyBytes_Resize(bytes, len * 2) < 0)
            return 0;
    }
    unsigned char *lnotab = (unsigned char *) PyBytes_AS_STRING(*bytes);
    lnotab += *offset;
    *lnotab++ = a;
    *lnotab++ = b;
    *offset += 2;
    return 1;
}

static int
emit_delta(PyObject **bytes, int bdelta, int ldelta, int *offset)
{
    while (bdelta > 255) {
        if (!emit_pair(bytes, offset, 255, 0)) {
            return 0;
        }
        bdelta -= 255;
    }
    while (ldelta > 127) {
        if (!emit_pair(bytes, offset, bdelta, 127)) {
            return 0;
        }
        bdelta = 0;
        ldelta -= 127;
    }
    while (ldelta < -128) {
        if (!emit_pair(bytes, offset, bdelta, -128)) {
            return 0;
        }
        bdelta = 0;
        ldelta += 128;
    }
    return emit_pair(bytes, offset, bdelta, ldelta);
}

static PyObject *
decode_linetable(PyCodeObject *code)
{
    PyCodeAddressRange bounds;
    PyObject *bytes;
    int table_offset = 0;
    int code_offset = 0;
    int line = code->co_firstlineno;
    bytes = PyBytes_FromStringAndSize(NULL, 64);
    if (bytes == NULL) {
        return NULL;
    }
    _PyCode_InitAddressRange(code, &bounds);
    while (_PyLineTable_NextAddressRange(&bounds)) {
        if (bounds.opaque.computed_line != line) {
            int bdelta = bounds.ar_start - code_offset;
            int ldelta = bounds.opaque.computed_line - line;
            if (!emit_delta(&bytes, bdelta, ldelta, &table_offset)) {
                Py_DECREF(bytes);
                return NULL;
            }
            code_offset = bounds.ar_start;
            line = bounds.opaque.computed_line;
        }
    }
    _PyBytes_Resize(&bytes, table_offset);
    return bytes;
}


typedef struct {
    PyObject_HEAD
    PyCodeObject *li_code;
    PyCodeAddressRange li_line;
} lineiterator;


static void
lineiter_dealloc(lineiterator *li)
{
    Py_DECREF(li->li_code);
    Py_TYPE(li)->tp_free(li);
}

static PyObject *
_source_offset_converter(int *value) {
    if (*value == -1) {
        Py_RETURN_NONE;
    }
    return PyLong_FromLong(*value);
}

static PyObject *
lineiter_next(lineiterator *li)
{
    PyCodeAddressRange *bounds = &li->li_line;
    if (!_PyLineTable_NextAddressRange(bounds)) {
        return NULL;
    }
    int start = bounds->ar_start;
    int line = bounds->ar_line;
    // Merge overlapping entries:
    while (_PyLineTable_NextAddressRange(bounds)) {
        if (bounds->ar_line != line) {
            _PyLineTable_PreviousAddressRange(bounds);
            break;
        }
    }
    return Py_BuildValue("iiO&", start, bounds->ar_end,
                         _source_offset_converter, &line);
}

PyTypeObject _PyLineIterator = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "line_iterator",                    /* tp_name */
    sizeof(lineiterator),               /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)lineiter_dealloc,       /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,       /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    (iternextfunc)lineiter_next,        /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    0,                                  /* tp_new */
    PyObject_Del,                       /* tp_free */
};

static lineiterator *
new_linesiterator(PyCodeObject *code)
{
    lineiterator *li = (lineiterator *)PyType_GenericAlloc(&_PyLineIterator, 0);
    if (li == NULL) {
        return NULL;
    }
    li->li_code = (PyCodeObject*)Py_NewRef(code);
    _PyCode_InitAddressRange(code, &li->li_line);
    return li;
}

/* co_positions iterator object. */
typedef struct {
    PyObject_HEAD
    PyCodeObject* pi_code;
    PyCodeAddressRange pi_range;
    int pi_offset;
    int pi_endline;
    int pi_column;
    int pi_endcolumn;
} positionsiterator;

static void
positionsiter_dealloc(positionsiterator* pi)
{
    Py_DECREF(pi->pi_code);
    Py_TYPE(pi)->tp_free(pi);
}

static PyObject*
positionsiter_next(positionsiterator* pi)
{
    if (pi->pi_offset >= pi->pi_range.ar_end) {
        assert(pi->pi_offset == pi->pi_range.ar_end);
        if (at_end(&pi->pi_range)) {
            return NULL;
        }
        advance_with_locations(&pi->pi_range, &pi->pi_endline, &pi->pi_column, &pi->pi_endcolumn);
    }
    pi->pi_offset += 2;
    return Py_BuildValue("(O&O&O&O&)",
        _source_offset_converter, &pi->pi_range.ar_line,
        _source_offset_converter, &pi->pi_endline,
        _source_offset_converter, &pi->pi_column,
        _source_offset_converter, &pi->pi_endcolumn);
}

PyTypeObject _PyPositionsIterator = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "positions_iterator",               /* tp_name */
    sizeof(positionsiterator),          /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)positionsiter_dealloc,  /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,       /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    (iternextfunc)positionsiter_next,   /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    0,                                  /* tp_new */
    PyObject_Del,                       /* tp_free */
};

static PyObject*
code_positionsiterator(PyCodeObject* code, PyObject* Py_UNUSED(args))
{
    positionsiterator* pi = (positionsiterator*)PyType_GenericAlloc(&_PyPositionsIterator, 0);
    if (pi == NULL) {
        return NULL;
    }
    pi->pi_code = (PyCodeObject*)Py_NewRef(code);
    _PyCode_InitAddressRange(code, &pi->pi_range);
    pi->pi_offset = pi->pi_range.ar_end;
    return (PyObject*)pi;
}


/******************
 * "extra" frame eval info (see PEP 523)
 ******************/

/* Holder for co_extra information */
typedef struct {
    Py_ssize_t ce_size;
    void *ce_extras[1];
} _PyCodeObjectExtra;


int
PyUnstable_Code_GetExtra(PyObject *code, Py_ssize_t index, void **extra)
{
    if (!PyCode_Check(code)) {
        PyErr_BadInternalCall();
        return -1;
    }

    PyCodeObject *o = (PyCodeObject*) code;
    _PyCodeObjectExtra *co_extra = (_PyCodeObjectExtra*) o->co_extra;

    if (co_extra == NULL || index < 0 || co_extra->ce_size <= index) {
        *extra = NULL;
        return 0;
    }

    *extra = co_extra->ce_extras[index];
    return 0;
}


int
PyUnstable_Code_SetExtra(PyObject *code, Py_ssize_t index, void *extra)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();

    if (!PyCode_Check(code) || index < 0 ||
            index >= interp->co_extra_user_count) {
        PyErr_BadInternalCall();
        return -1;
    }

    PyCodeObject *o = (PyCodeObject*) code;
    _PyCodeObjectExtra *co_extra = (_PyCodeObjectExtra *) o->co_extra;

    if (co_extra == NULL || co_extra->ce_size <= index) {
        Py_ssize_t i = (co_extra == NULL ? 0 : co_extra->ce_size);
        co_extra = PyMem_Realloc(
                co_extra,
                sizeof(_PyCodeObjectExtra) +
                (interp->co_extra_user_count-1) * sizeof(void*));
        if (co_extra == NULL) {
            return -1;
        }
        for (; i < interp->co_extra_user_count; i++) {
            co_extra->ce_extras[i] = NULL;
        }
        co_extra->ce_size = interp->co_extra_user_count;
        o->co_extra = co_extra;
    }

    if (co_extra->ce_extras[index] != NULL) {
        freefunc free = interp->co_extra_freefuncs[index];
        if (free != NULL) {
            free(co_extra->ce_extras[index]);
        }
    }

    co_extra->ce_extras[index] = extra;
    return 0;
}


/******************
 * other PyCodeObject accessor functions
 ******************/

static PyObject *
get_cached_locals(PyCodeObject *co, PyObject **cached_field,
    _PyLocals_Kind kind, int num)
{
    assert(cached_field != NULL);
    assert(co->_co_cached != NULL);
    if (*cached_field != NULL) {
        return Py_NewRef(*cached_field);
    }
    assert(*cached_field == NULL);
    PyObject *varnames = get_localsplus_names(co, kind, num);
    if (varnames == NULL) {
        return NULL;
    }
    *cached_field = Py_NewRef(varnames);
    return varnames;
}

PyObject *
_PyCode_GetVarnames(PyCodeObject *co)
{
    if (init_co_cached(co)) {
        return NULL;
    }
    return get_cached_locals(co, &co->_co_cached->_co_varnames, CO_FAST_LOCAL, co->co_nlocals);
}

PyObject *
PyCode_GetVarnames(PyCodeObject *code)
{
    return _PyCode_GetVarnames(code);
}

PyObject *
_PyCode_GetCellvars(PyCodeObject *co)
{
    if (init_co_cached(co)) {
        return NULL;
    }
    return get_cached_locals(co, &co->_co_cached->_co_cellvars, CO_FAST_CELL, co->co_ncellvars);
}

PyObject *
PyCode_GetCellvars(PyCodeObject *code)
{
    return _PyCode_GetCellvars(code);
}

PyObject *
_PyCode_GetFreevars(PyCodeObject *co)
{
    if (init_co_cached(co)) {
        return NULL;
    }
    return get_cached_locals(co, &co->_co_cached->_co_freevars, CO_FAST_FREE, co->co_nfreevars);
}

PyObject *
PyCode_GetFreevars(PyCodeObject *code)
{
    return _PyCode_GetFreevars(code);
}

static void
deopt_code(PyCodeObject *code, _Py_CODEUNIT *instructions)
{
    Py_ssize_t len = Py_SIZE(code);
    for (int i = 0; i < len; i++) {
        int opcode = _Py_GetBaseOpcode(code, i);
        int caches = _PyOpcode_Caches[opcode];
        instructions[i].op.code = opcode;
        for (int j = 1; j <= caches; j++) {
            instructions[i+j].cache = 0;
        }
        i += caches;
    }
}

PyObject *
_PyCode_GetCode(PyCodeObject *co)
{
    if (init_co_cached(co)) {
        return NULL;
    }
    if (co->_co_cached->_co_code != NULL) {
        return Py_NewRef(co->_co_cached->_co_code);
    }
    PyObject *code = PyBytes_FromStringAndSize((const char *)_PyCode_CODE(co),
                                               _PyCode_NBYTES(co));
    if (code == NULL) {
        return NULL;
    }
    deopt_code(co, (_Py_CODEUNIT *)PyBytes_AS_STRING(code));
    assert(co->_co_cached->_co_code == NULL);
    co->_co_cached->_co_code = Py_NewRef(code);
    return code;
}

PyObject *
PyCode_GetCode(PyCodeObject *co)
{
    return _PyCode_GetCode(co);
}

/******************
 * PyCode_Type
 ******************/

/*[clinic input]
class code "PyCodeObject *" "&PyCode_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=78aa5d576683bb4b]*/

/*[clinic input]
@classmethod
code.__new__ as code_new

    argcount: int
    posonlyargcount: int
    kwonlyargcount: int
    nlocals: int
    stacksize: int
    flags: int
    codestring as code: object(subclass_of="&PyBytes_Type")
    constants as consts: object(subclass_of="&PyTuple_Type")
    names: object(subclass_of="&PyTuple_Type")
    varnames: object(subclass_of="&PyTuple_Type")
    filename: unicode
    name: unicode
    qualname: unicode
    firstlineno: int
    linetable: object(subclass_of="&PyBytes_Type")
    exceptiontable: object(subclass_of="&PyBytes_Type")
    freevars: object(subclass_of="&PyTuple_Type", c_default="NULL") = ()
    cellvars: object(subclass_of="&PyTuple_Type", c_default="NULL") = ()
    /

Create a code object.  Not for the faint of heart.
[clinic start generated code]*/

static PyObject *
code_new_impl(PyTypeObject *type, int argcount, int posonlyargcount,
              int kwonlyargcount, int nlocals, int stacksize, int flags,
              PyObject *code, PyObject *consts, PyObject *names,
              PyObject *varnames, PyObject *filename, PyObject *name,
              PyObject *qualname, int firstlineno, PyObject *linetable,
              PyObject *exceptiontable, PyObject *freevars,
              PyObject *cellvars)
/*[clinic end generated code: output=069fa20d299f9dda input=e31da3c41ad8064a]*/
{
    PyObject *co = NULL;
    PyObject *ournames = NULL;
    PyObject *ourvarnames = NULL;
    PyObject *ourfreevars = NULL;
    PyObject *ourcellvars = NULL;

    if (PySys_Audit("code.__new__", "OOOiiiiii",
                    code, filename, name, argcount, posonlyargcount,
                    kwonlyargcount, nlocals, stacksize, flags) < 0) {
        goto cleanup;
    }

    if (argcount < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "code: argcount must not be negative");
        goto cleanup;
    }

    if (posonlyargcount < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "code: posonlyargcount must not be negative");
        goto cleanup;
    }

    if (kwonlyargcount < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "code: kwonlyargcount must not be negative");
        goto cleanup;
    }
    if (nlocals < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "code: nlocals must not be negative");
        goto cleanup;
    }

    ournames = validate_and_copy_tuple(names);
    if (ournames == NULL)
        goto cleanup;
    ourvarnames = validate_and_copy_tuple(varnames);
    if (ourvarnames == NULL)
        goto cleanup;
    if (freevars)
        ourfreevars = validate_and_copy_tuple(freevars);
    else
        ourfreevars = PyTuple_New(0);
    if (ourfreevars == NULL)
        goto cleanup;
    if (cellvars)
        ourcellvars = validate_and_copy_tuple(cellvars);
    else
        ourcellvars = PyTuple_New(0);
    if (ourcellvars == NULL)
        goto cleanup;

    co = (PyObject *)PyCode_NewWithPosOnlyArgs(argcount, posonlyargcount,
                                               kwonlyargcount,
                                               nlocals, stacksize, flags,
                                               code, consts, ournames,
                                               ourvarnames, ourfreevars,
                                               ourcellvars, filename,
                                               name, qualname, firstlineno,
                                               linetable,
                                               exceptiontable
                                              );
  cleanup:
    Py_XDECREF(ournames);
    Py_XDECREF(ourvarnames);
    Py_XDECREF(ourfreevars);
    Py_XDECREF(ourcellvars);
    return co;
}

static void
free_monitoring_data(_PyCoMonitoringData *data)
{
    if (data == NULL) {
        return;
    }
    if (data->tools) {
        PyMem_Free(data->tools);
    }
    if (data->lines) {
        PyMem_Free(data->lines);
    }
    if (data->line_tools) {
        PyMem_Free(data->line_tools);
    }
    if (data->per_instruction_opcodes) {
        PyMem_Free(data->per_instruction_opcodes);
    }
    if (data->per_instruction_tools) {
        PyMem_Free(data->per_instruction_tools);
    }
    PyMem_Free(data);
}

static void
code_dealloc(PyCodeObject *co)
{
    assert(Py_REFCNT(co) == 0);
    Py_SET_REFCNT(co, 1);
    notify_code_watchers(PY_CODE_EVENT_DESTROY, co);
    if (Py_REFCNT(co) > 1) {
        Py_SET_REFCNT(co, Py_REFCNT(co) - 1);
        return;
    }
    Py_SET_REFCNT(co, 0);

    if (co->co_extra != NULL) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        _PyCodeObjectExtra *co_extra = co->co_extra;

        for (Py_ssize_t i = 0; i < co_extra->ce_size; i++) {
            freefunc free_extra = interp->co_extra_freefuncs[i];

            if (free_extra != NULL) {
                free_extra(co_extra->ce_extras[i]);
            }
        }

        PyMem_Free(co_extra);
    }

    Py_XDECREF(co->co_consts);
    Py_XDECREF(co->co_names);
    Py_XDECREF(co->co_localsplusnames);
    Py_XDECREF(co->co_localspluskinds);
    Py_XDECREF(co->co_filename);
    Py_XDECREF(co->co_name);
    Py_XDECREF(co->co_qualname);
    Py_XDECREF(co->co_linetable);
    Py_XDECREF(co->co_exceptiontable);
    if (co->_co_cached != NULL) {
        Py_XDECREF(co->_co_cached->_co_code);
        Py_XDECREF(co->_co_cached->_co_cellvars);
        Py_XDECREF(co->_co_cached->_co_freevars);
        Py_XDECREF(co->_co_cached->_co_varnames);
        PyMem_Free(co->_co_cached);
    }
    if (co->co_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject*)co);
    }
    free_monitoring_data(co->_co_monitoring);
    PyObject_Free(co);
}

static PyObject *
code_repr(PyCodeObject *co)
{
    int lineno;
    if (co->co_firstlineno != 0)
        lineno = co->co_firstlineno;
    else
        lineno = -1;
    if (co->co_filename && PyUnicode_Check(co->co_filename)) {
        return PyUnicode_FromFormat(
            "<code object %U at %p, file \"%U\", line %d>",
            co->co_name, co, co->co_filename, lineno);
    } else {
        return PyUnicode_FromFormat(
            "<code object %U at %p, file ???, line %d>",
            co->co_name, co, lineno);
    }
}

static PyObject *
code_richcompare(PyObject *self, PyObject *other, int op)
{
    PyCodeObject *co, *cp;
    int eq;
    PyObject *consts1, *consts2;
    PyObject *res;

    if ((op != Py_EQ && op != Py_NE) ||
        !PyCode_Check(self) ||
        !PyCode_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    co = (PyCodeObject *)self;
    cp = (PyCodeObject *)other;

    eq = PyObject_RichCompareBool(co->co_name, cp->co_name, Py_EQ);
    if (!eq) goto unequal;
    eq = co->co_argcount == cp->co_argcount;
    if (!eq) goto unequal;
    eq = co->co_posonlyargcount == cp->co_posonlyargcount;
    if (!eq) goto unequal;
    eq = co->co_kwonlyargcount == cp->co_kwonlyargcount;
    if (!eq) goto unequal;
    eq = co->co_flags == cp->co_flags;
    if (!eq) goto unequal;
    eq = co->co_firstlineno == cp->co_firstlineno;
    if (!eq) goto unequal;
    eq = Py_SIZE(co) == Py_SIZE(cp);
    if (!eq) {
        goto unequal;
    }
    for (int i = 0; i < Py_SIZE(co); i++) {
        _Py_CODEUNIT co_instr = _PyCode_CODE(co)[i];
        _Py_CODEUNIT cp_instr = _PyCode_CODE(cp)[i];
        co_instr.op.code = _Py_GetBaseOpcode(co, i);
        cp_instr.op.code = _Py_GetBaseOpcode(cp, i);
        eq = co_instr.cache == cp_instr.cache;
        if (!eq) {
            goto unequal;
        }
        i += _PyOpcode_Caches[co_instr.op.code];
    }

    /* compare constants */
    consts1 = _PyCode_ConstantKey(co->co_consts);
    if (!consts1)
        return NULL;
    consts2 = _PyCode_ConstantKey(cp->co_consts);
    if (!consts2) {
        Py_DECREF(consts1);
        return NULL;
    }
    eq = PyObject_RichCompareBool(consts1, consts2, Py_EQ);
    Py_DECREF(consts1);
    Py_DECREF(consts2);
    if (eq <= 0) goto unequal;

    eq = PyObject_RichCompareBool(co->co_names, cp->co_names, Py_EQ);
    if (eq <= 0) goto unequal;
    eq = PyObject_RichCompareBool(co->co_localsplusnames,
                                  cp->co_localsplusnames, Py_EQ);
    if (eq <= 0) goto unequal;
    eq = PyObject_RichCompareBool(co->co_linetable, cp->co_linetable, Py_EQ);
    if (eq <= 0) {
        goto unequal;
    }
    eq = PyObject_RichCompareBool(co->co_exceptiontable,
                                  cp->co_exceptiontable, Py_EQ);
    if (eq <= 0) {
        goto unequal;
    }

    if (op == Py_EQ)
        res = Py_True;
    else
        res = Py_False;
    goto done;

  unequal:
    if (eq < 0)
        return NULL;
    if (op == Py_NE)
        res = Py_True;
    else
        res = Py_False;

  done:
    return Py_NewRef(res);
}

static Py_hash_t
code_hash(PyCodeObject *co)
{
    Py_uhash_t uhash = 20221211;
    #define SCRAMBLE_IN(H) do {       \
        uhash ^= (Py_uhash_t)(H);     \
        uhash *= _PyHASH_MULTIPLIER;  \
    } while (0)
    #define SCRAMBLE_IN_HASH(EXPR) do {     \
        Py_hash_t h = PyObject_Hash(EXPR);  \
        if (h == -1) {                      \
            return -1;                      \
        }                                   \
        SCRAMBLE_IN(h);                     \
    } while (0)

    SCRAMBLE_IN_HASH(co->co_name);
    SCRAMBLE_IN_HASH(co->co_consts);
    SCRAMBLE_IN_HASH(co->co_names);
    SCRAMBLE_IN_HASH(co->co_localsplusnames);
    SCRAMBLE_IN_HASH(co->co_linetable);
    SCRAMBLE_IN_HASH(co->co_exceptiontable);
    SCRAMBLE_IN(co->co_argcount);
    SCRAMBLE_IN(co->co_posonlyargcount);
    SCRAMBLE_IN(co->co_kwonlyargcount);
    SCRAMBLE_IN(co->co_flags);
    SCRAMBLE_IN(co->co_firstlineno);
    SCRAMBLE_IN(Py_SIZE(co));
    for (int i = 0; i < Py_SIZE(co); i++) {
        int deop = _Py_GetBaseOpcode(co, i);
        SCRAMBLE_IN(deop);
        SCRAMBLE_IN(_PyCode_CODE(co)[i].op.arg);
        i += _PyOpcode_Caches[deop];
    }
    if ((Py_hash_t)uhash == -1) {
        return -2;
    }
    return (Py_hash_t)uhash;
}


#define OFF(x) offsetof(PyCodeObject, x)

static PyMemberDef code_memberlist[] = {
    {"co_argcount",        T_INT,    OFF(co_argcount),        READONLY},
    {"co_posonlyargcount", T_INT,    OFF(co_posonlyargcount), READONLY},
    {"co_kwonlyargcount",  T_INT,    OFF(co_kwonlyargcount),  READONLY},
    {"co_stacksize",       T_INT,    OFF(co_stacksize),       READONLY},
    {"co_flags",           T_INT,    OFF(co_flags),           READONLY},
    {"co_nlocals",         T_INT,    OFF(co_nlocals),         READONLY},
    {"co_consts",          T_OBJECT, OFF(co_consts),          READONLY},
    {"co_names",           T_OBJECT, OFF(co_names),           READONLY},
    {"co_filename",        T_OBJECT, OFF(co_filename),        READONLY},
    {"co_name",            T_OBJECT, OFF(co_name),            READONLY},
    {"co_qualname",        T_OBJECT, OFF(co_qualname),        READONLY},
    {"co_firstlineno",     T_INT,    OFF(co_firstlineno),     READONLY},
    {"co_linetable",       T_OBJECT, OFF(co_linetable),       READONLY},
    {"co_exceptiontable",  T_OBJECT, OFF(co_exceptiontable),  READONLY},
    {NULL}      /* Sentinel */
};


static PyObject *
code_getlnotab(PyCodeObject *code, void *closure)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                     "co_lnotab is deprecated, use co_lines instead.",
                     1) < 0) {
        return NULL;
    }
    return decode_linetable(code);
}

static PyObject *
code_getvarnames(PyCodeObject *code, void *closure)
{
    return _PyCode_GetVarnames(code);
}

static PyObject *
code_getcellvars(PyCodeObject *code, void *closure)
{
    return _PyCode_GetCellvars(code);
}

static PyObject *
code_getfreevars(PyCodeObject *code, void *closure)
{
    return _PyCode_GetFreevars(code);
}

static PyObject *
code_getcodeadaptive(PyCodeObject *code, void *closure)
{
    return PyBytes_FromStringAndSize(code->co_code_adaptive,
                                     _PyCode_NBYTES(code));
}

static PyObject *
code_getcode(PyCodeObject *code, void *closure)
{
    return _PyCode_GetCode(code);
}

static PyGetSetDef code_getsetlist[] = {
    {"co_lnotab",         (getter)code_getlnotab,       NULL, NULL},
    {"_co_code_adaptive", (getter)code_getcodeadaptive, NULL, NULL},
    // The following old names are kept for backward compatibility.
    {"co_varnames",       (getter)code_getvarnames,     NULL, NULL},
    {"co_cellvars",       (getter)code_getcellvars,     NULL, NULL},
    {"co_freevars",       (getter)code_getfreevars,     NULL, NULL},
    {"co_code",           (getter)code_getcode,         NULL, NULL},
    {0}
};


static PyObject *
code_sizeof(PyCodeObject *co, PyObject *Py_UNUSED(args))
{
    size_t res = _PyObject_VAR_SIZE(Py_TYPE(co), Py_SIZE(co));
    _PyCodeObjectExtra *co_extra = (_PyCodeObjectExtra*) co->co_extra;
    if (co_extra != NULL) {
        res += sizeof(_PyCodeObjectExtra);
        res += ((size_t)co_extra->ce_size - 1) * sizeof(co_extra->ce_extras[0]);
    }
    return PyLong_FromSize_t(res);
}

static PyObject *
code_linesiterator(PyCodeObject *code, PyObject *Py_UNUSED(args))
{
    return (PyObject *)new_linesiterator(code);
}

/*[clinic input]
@text_signature "($self, /, **changes)"
code.replace

    *
    co_argcount: int(c_default="self->co_argcount") = unchanged
    co_posonlyargcount: int(c_default="self->co_posonlyargcount") = unchanged
    co_kwonlyargcount: int(c_default="self->co_kwonlyargcount") = unchanged
    co_nlocals: int(c_default="self->co_nlocals") = unchanged
    co_stacksize: int(c_default="self->co_stacksize") = unchanged
    co_flags: int(c_default="self->co_flags") = unchanged
    co_firstlineno: int(c_default="self->co_firstlineno") = unchanged
    co_code: object(subclass_of="&PyBytes_Type", c_default="NULL") = unchanged
    co_consts: object(subclass_of="&PyTuple_Type", c_default="self->co_consts") = unchanged
    co_names: object(subclass_of="&PyTuple_Type", c_default="self->co_names") = unchanged
    co_varnames: object(subclass_of="&PyTuple_Type", c_default="NULL") = unchanged
    co_freevars: object(subclass_of="&PyTuple_Type", c_default="NULL") = unchanged
    co_cellvars: object(subclass_of="&PyTuple_Type", c_default="NULL") = unchanged
    co_filename: unicode(c_default="self->co_filename") = unchanged
    co_name: unicode(c_default="self->co_name") = unchanged
    co_qualname: unicode(c_default="self->co_qualname") = unchanged
    co_linetable: object(subclass_of="&PyBytes_Type", c_default="self->co_linetable") = unchanged
    co_exceptiontable: object(subclass_of="&PyBytes_Type", c_default="self->co_exceptiontable") = unchanged

Return a copy of the code object with new values for the specified fields.
[clinic start generated code]*/

static PyObject *
code_replace_impl(PyCodeObject *self, int co_argcount,
                  int co_posonlyargcount, int co_kwonlyargcount,
                  int co_nlocals, int co_stacksize, int co_flags,
                  int co_firstlineno, PyObject *co_code, PyObject *co_consts,
                  PyObject *co_names, PyObject *co_varnames,
                  PyObject *co_freevars, PyObject *co_cellvars,
                  PyObject *co_filename, PyObject *co_name,
                  PyObject *co_qualname, PyObject *co_linetable,
                  PyObject *co_exceptiontable)
/*[clinic end generated code: output=e75c48a15def18b9 input=18e280e07846c122]*/
{
#define CHECK_INT_ARG(ARG) \
        if (ARG < 0) { \
            PyErr_SetString(PyExc_ValueError, \
                            #ARG " must be a positive integer"); \
            return NULL; \
        }

    CHECK_INT_ARG(co_argcount);
    CHECK_INT_ARG(co_posonlyargcount);
    CHECK_INT_ARG(co_kwonlyargcount);
    CHECK_INT_ARG(co_nlocals);
    CHECK_INT_ARG(co_stacksize);
    CHECK_INT_ARG(co_flags);
    CHECK_INT_ARG(co_firstlineno);

#undef CHECK_INT_ARG

    PyObject *code = NULL;
    if (co_code == NULL) {
        code = _PyCode_GetCode(self);
        if (code == NULL) {
            return NULL;
        }
        co_code = code;
    }

    if (PySys_Audit("code.__new__", "OOOiiiiii",
                    co_code, co_filename, co_name, co_argcount,
                    co_posonlyargcount, co_kwonlyargcount, co_nlocals,
                    co_stacksize, co_flags) < 0) {
        Py_XDECREF(code);
        return NULL;
    }

    PyCodeObject *co = NULL;
    PyObject *varnames = NULL;
    PyObject *cellvars = NULL;
    PyObject *freevars = NULL;
    if (co_varnames == NULL) {
        varnames = get_localsplus_names(self, CO_FAST_LOCAL, self->co_nlocals);
        if (varnames == NULL) {
            goto error;
        }
        co_varnames = varnames;
    }
    if (co_cellvars == NULL) {
        cellvars = get_localsplus_names(self, CO_FAST_CELL, self->co_ncellvars);
        if (cellvars == NULL) {
            goto error;
        }
        co_cellvars = cellvars;
    }
    if (co_freevars == NULL) {
        freevars = get_localsplus_names(self, CO_FAST_FREE, self->co_nfreevars);
        if (freevars == NULL) {
            goto error;
        }
        co_freevars = freevars;
    }

    co = PyCode_NewWithPosOnlyArgs(
        co_argcount, co_posonlyargcount, co_kwonlyargcount, co_nlocals,
        co_stacksize, co_flags, co_code, co_consts, co_names,
        co_varnames, co_freevars, co_cellvars, co_filename, co_name,
        co_qualname, co_firstlineno,
        co_linetable, co_exceptiontable);

error:
    Py_XDECREF(code);
    Py_XDECREF(varnames);
    Py_XDECREF(cellvars);
    Py_XDECREF(freevars);
    return (PyObject *)co;
}

/*[clinic input]
code._varname_from_oparg

    oparg: int

(internal-only) Return the local variable name for the given oparg.

WARNING: this method is for internal use only and may change or go away.
[clinic start generated code]*/

static PyObject *
code__varname_from_oparg_impl(PyCodeObject *self, int oparg)
/*[clinic end generated code: output=1fd1130413184206 input=c5fa3ee9bac7d4ca]*/
{
    PyObject *name = PyTuple_GetItem(self->co_localsplusnames, oparg);
    if (name == NULL) {
        return NULL;
    }
    return Py_NewRef(name);
}

/* XXX code objects need to participate in GC? */

static struct PyMethodDef code_methods[] = {
    {"__sizeof__", (PyCFunction)code_sizeof, METH_NOARGS},
    {"co_lines", (PyCFunction)code_linesiterator, METH_NOARGS},
    {"co_positions", (PyCFunction)code_positionsiterator, METH_NOARGS},
    CODE_REPLACE_METHODDEF
    CODE__VARNAME_FROM_OPARG_METHODDEF
    {NULL, NULL}                /* sentinel */
};


PyTypeObject PyCode_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "code",
    offsetof(PyCodeObject, co_code_adaptive),
    sizeof(_Py_CODEUNIT),
    (destructor)code_dealloc,           /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    (reprfunc)code_repr,                /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)code_hash,                /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    code_new__doc__,                    /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    code_richcompare,                   /* tp_richcompare */
    offsetof(PyCodeObject, co_weakreflist),     /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    code_methods,                       /* tp_methods */
    code_memberlist,                    /* tp_members */
    code_getsetlist,                    /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    code_new,                           /* tp_new */
};


/******************
 * other API
 ******************/

PyObject*
_PyCode_ConstantKey(PyObject *op)
{
    PyObject *key;

    /* Py_None and Py_Ellipsis are singletons. */
    if (op == Py_None || op == Py_Ellipsis
       || PyLong_CheckExact(op)
       || PyUnicode_CheckExact(op)
          /* code_richcompare() uses _PyCode_ConstantKey() internally */
       || PyCode_Check(op))
    {
        /* Objects of these types are always different from object of other
         * type and from tuples. */
        key = Py_NewRef(op);
    }
    else if (PyBool_Check(op) || PyBytes_CheckExact(op)) {
        /* Make booleans different from integers 0 and 1.
         * Avoid BytesWarning from comparing bytes with strings. */
        key = PyTuple_Pack(2, Py_TYPE(op), op);
    }
    else if (PyFloat_CheckExact(op)) {
        double d = PyFloat_AS_DOUBLE(op);
        /* all we need is to make the tuple different in either the 0.0
         * or -0.0 case from all others, just to avoid the "coercion".
         */
        if (d == 0.0 && copysign(1.0, d) < 0.0)
            key = PyTuple_Pack(3, Py_TYPE(op), op, Py_None);
        else
            key = PyTuple_Pack(2, Py_TYPE(op), op);
    }
    else if (PyComplex_CheckExact(op)) {
        Py_complex z;
        int real_negzero, imag_negzero;
        /* For the complex case we must make complex(x, 0.)
           different from complex(x, -0.) and complex(0., y)
           different from complex(-0., y), for any x and y.
           All four complex zeros must be distinguished.*/
        z = PyComplex_AsCComplex(op);
        real_negzero = z.real == 0.0 && copysign(1.0, z.real) < 0.0;
        imag_negzero = z.imag == 0.0 && copysign(1.0, z.imag) < 0.0;
        /* use True, False and None singleton as tags for the real and imag
         * sign, to make tuples different */
        if (real_negzero && imag_negzero) {
            key = PyTuple_Pack(3, Py_TYPE(op), op, Py_True);
        }
        else if (imag_negzero) {
            key = PyTuple_Pack(3, Py_TYPE(op), op, Py_False);
        }
        else if (real_negzero) {
            key = PyTuple_Pack(3, Py_TYPE(op), op, Py_None);
        }
        else {
            key = PyTuple_Pack(2, Py_TYPE(op), op);
        }
    }
    else if (PyTuple_CheckExact(op)) {
        Py_ssize_t i, len;
        PyObject *tuple;

        len = PyTuple_GET_SIZE(op);
        tuple = PyTuple_New(len);
        if (tuple == NULL)
            return NULL;

        for (i=0; i < len; i++) {
            PyObject *item, *item_key;

            item = PyTuple_GET_ITEM(op, i);
            item_key = _PyCode_ConstantKey(item);
            if (item_key == NULL) {
                Py_DECREF(tuple);
                return NULL;
            }

            PyTuple_SET_ITEM(tuple, i, item_key);
        }

        key = PyTuple_Pack(2, tuple, op);
        Py_DECREF(tuple);
    }
    else if (PyFrozenSet_CheckExact(op)) {
        Py_ssize_t pos = 0;
        PyObject *item;
        Py_hash_t hash;
        Py_ssize_t i, len;
        PyObject *tuple, *set;

        len = PySet_GET_SIZE(op);
        tuple = PyTuple_New(len);
        if (tuple == NULL)
            return NULL;

        i = 0;
        while (_PySet_NextEntry(op, &pos, &item, &hash)) {
            PyObject *item_key;

            item_key = _PyCode_ConstantKey(item);
            if (item_key == NULL) {
                Py_DECREF(tuple);
                return NULL;
            }

            assert(i < len);
            PyTuple_SET_ITEM(tuple, i, item_key);
            i++;
        }
        set = PyFrozenSet_New(tuple);
        Py_DECREF(tuple);
        if (set == NULL)
            return NULL;

        key = PyTuple_Pack(2, set, op);
        Py_DECREF(set);
        return key;
    }
    else {
        /* for other types, use the object identifier as a unique identifier
         * to ensure that they are seen as unequal. */
        PyObject *obj_id = PyLong_FromVoidPtr(op);
        if (obj_id == NULL)
            return NULL;

        key = PyTuple_Pack(2, obj_id, op);
        Py_DECREF(obj_id);
    }
    return key;
}

void
_PyStaticCode_Fini(PyCodeObject *co)
{
    deopt_code(co, _PyCode_CODE(co));
    PyMem_Free(co->co_extra);
    if (co->_co_cached != NULL) {
        Py_CLEAR(co->_co_cached->_co_code);
        Py_CLEAR(co->_co_cached->_co_cellvars);
        Py_CLEAR(co->_co_cached->_co_freevars);
        Py_CLEAR(co->_co_cached->_co_varnames);
        PyMem_Free(co->_co_cached);
        co->_co_cached = NULL;
    }
    co->co_extra = NULL;
    if (co->co_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *)co);
        co->co_weakreflist = NULL;
    }
    free_monitoring_data(co->_co_monitoring);
    co->_co_monitoring = NULL;
}

int
_PyStaticCode_Init(PyCodeObject *co)
{
    int res = intern_strings(co->co_names);
    if (res < 0) {
        return -1;
    }
    res = intern_string_constants(co->co_consts, NULL);
    if (res < 0) {
        return -1;
    }
    res = intern_strings(co->co_localsplusnames);
    if (res < 0) {
        return -1;
    }
    _PyCode_Quicken(co);
    return 0;
}

#define MAX_CODE_UNITS_PER_LOC_ENTRY 8

PyCodeObject *
_Py_MakeShimCode(const _PyShimCodeDef *codedef)
{
    PyObject *name = NULL;
    PyObject *co_code = NULL;
    PyObject *lines = NULL;
    PyCodeObject *codeobj = NULL;
    uint8_t *loc_table = NULL;

    name = _PyUnicode_FromASCII(codedef->cname, strlen(codedef->cname));
    if (name == NULL) {
        goto cleanup;
    }
    co_code = PyBytes_FromStringAndSize(
        (const char *)codedef->code, codedef->codelen);
    if (co_code == NULL) {
        goto cleanup;
    }
    int code_units = codedef->codelen / sizeof(_Py_CODEUNIT);
    int loc_entries = (code_units + MAX_CODE_UNITS_PER_LOC_ENTRY - 1) /
                      MAX_CODE_UNITS_PER_LOC_ENTRY;
    loc_table = PyMem_Malloc(loc_entries);
    if (loc_table == NULL) {
        PyErr_NoMemory();
        goto cleanup;
    }
    for (int i = 0; i < loc_entries-1; i++) {
         loc_table[i] = 0x80 | (PY_CODE_LOCATION_INFO_NONE << 3) | 7;
         code_units -= MAX_CODE_UNITS_PER_LOC_ENTRY;
    }
    assert(loc_entries > 0);
    assert(code_units > 0 && code_units <= MAX_CODE_UNITS_PER_LOC_ENTRY);
    loc_table[loc_entries-1] = 0x80 |
        (PY_CODE_LOCATION_INFO_NONE << 3) | (code_units-1);
    lines = PyBytes_FromStringAndSize((const char *)loc_table, loc_entries);
    PyMem_Free(loc_table);
    if (lines == NULL) {
        goto cleanup;
    }
    _Py_DECLARE_STR(shim_name, "<shim>");
    struct _PyCodeConstructor con = {
        .filename = &_Py_STR(shim_name),
        .name = name,
        .qualname = name,
        .flags = CO_NEWLOCALS | CO_OPTIMIZED,

        .code = co_code,
        .firstlineno = 1,
        .linetable = lines,

        .consts = (PyObject *)&_Py_SINGLETON(tuple_empty),
        .names = (PyObject *)&_Py_SINGLETON(tuple_empty),

        .localsplusnames = (PyObject *)&_Py_SINGLETON(tuple_empty),
        .localspluskinds = (PyObject *)&_Py_SINGLETON(bytes_empty),

        .argcount = 0,
        .posonlyargcount = 0,
        .kwonlyargcount = 0,

        .stacksize = codedef->stacksize,

        .exceptiontable = (PyObject *)&_Py_SINGLETON(bytes_empty),
    };

    codeobj = _PyCode_New(&con);
cleanup:
    Py_XDECREF(name);
    Py_XDECREF(co_code);
    Py_XDECREF(lines);
    return codeobj;
}
