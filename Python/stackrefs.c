#include "Python.h"

#include "pycore_object.h"
#include "pycore_stackref.h"

#if !defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)

#if SIZEOF_VOID_P < 8
#error "Py_STACKREF_DEBUG requires 64 bit machine"
#endif

#include "pycore_interp.h"
#include "pycore_hashtable.h"

typedef struct _table_entry {
    PyObject *obj;
    const char *classname;
    const char *filename;
    int linenumber;
    const char *filename_borrow;
    int linenumber_borrow;
} TableEntry;

TableEntry *
make_table_entry(PyObject *obj, const char *filename, int linenumber)
{
    TableEntry *result = malloc(sizeof(TableEntry));
    if (result == NULL) {
        return NULL;
    }
    result->obj = obj;
    result->classname = Py_TYPE(obj)->tp_name;
    result->filename = filename;
    result->linenumber = linenumber;
    result->filename_borrow = NULL;
    result->linenumber_borrow = 0;
    return result;
}

PyObject *
_Py_stackref_get_object(_PyStackRef ref)
{
    if (ref.index == 0) {
        return NULL;
    }
    PyInterpreterState *interp = PyInterpreterState_Get();
    assert(interp != NULL);
    if (ref.index >= interp->next_stackref) {
        _Py_FatalErrorFormat(__func__, "Garbled stack ref with ID %" PRIu64 "\n", ref.index);
    }
    TableEntry *entry = _Py_hashtable_get(interp->open_stackrefs_table, (void *)ref.index);
    if (entry == NULL) {
        _Py_FatalErrorFormat(__func__, "Accessing closed stack ref with ID %" PRIu64 "\n", ref.index);
    }
    return entry->obj;
}

int
PyStackRef_Is(_PyStackRef a, _PyStackRef b)
{
    return _Py_stackref_get_object(a) == _Py_stackref_get_object(b);
}

PyObject *
_Py_stackref_close(_PyStackRef ref, const char *filename, int linenumber)
{
    PyInterpreterState *interp = PyInterpreterState_Get();
    if (ref.index >= interp->next_stackref) {
        _Py_FatalErrorFormat(__func__, "Invalid StackRef with ID %" PRIu64 " at %s:%d\n", (void *)ref.index, filename, linenumber);

    }
    PyObject *obj;
    if (ref.index < INITIAL_STACKREF_INDEX) {
        if (ref.index == 0) {
            _Py_FatalErrorFormat(__func__, "Passing NULL to PyStackRef_CLOSE at %s:%d\n", filename, linenumber);
        }
        // Pre-allocated reference to None, False or True -- Do not clear
        TableEntry *entry = _Py_hashtable_get(interp->open_stackrefs_table, (void *)ref.index);
        obj = entry->obj;
    }
    else {
        TableEntry *entry = _Py_hashtable_steal(interp->open_stackrefs_table, (void *)ref.index);
        if (entry == NULL) {
#ifdef Py_STACKREF_CLOSE_DEBUG
            entry = _Py_hashtable_get(interp->closed_stackrefs_table, (void *)ref.index);
            if (entry != NULL) {
                _Py_FatalErrorFormat(__func__,
                    "Double close of ref ID %" PRIu64 " at %s:%d. Referred to instance of %s at %p. Closed at %s:%d\n",
                    (void *)ref.index, filename, linenumber, entry->classname, entry->obj, entry->filename, entry->linenumber);
            }
#endif
            _Py_FatalErrorFormat(__func__, "Invalid StackRef with ID %" PRIu64 "\n", (void *)ref.index);
        }
        obj = entry->obj;
        free(entry);
#ifdef Py_STACKREF_CLOSE_DEBUG
        TableEntry *close_entry = make_table_entry(obj, filename, linenumber);
        if (close_entry == NULL) {
            Py_FatalError("No memory left for stackref debug table");
        }
        if (_Py_hashtable_set(interp->closed_stackrefs_table, (void *)ref.index, close_entry) < 0) {
            Py_FatalError("No memory left for stackref debug table");
        }
#endif
    }
    return obj;
}

_PyStackRef
_Py_stackref_create(PyObject *obj, const char *filename, int linenumber)
{
    if (obj == NULL) {
        Py_FatalError("Cannot create a stackref for NULL");
    }
    PyInterpreterState *interp = PyInterpreterState_Get();
    uint64_t new_id = interp->next_stackref;
    interp->next_stackref = new_id + 2;
    TableEntry *entry = make_table_entry(obj, filename, linenumber);
    if (entry == NULL) {
        Py_FatalError("No memory left for stackref debug table");
    }
    if (_Py_hashtable_set(interp->open_stackrefs_table, (void *)new_id, entry) < 0) {
        Py_FatalError("No memory left for stackref debug table");
    }
    return (_PyStackRef){ .index = new_id };
}

void
_Py_stackref_record_borrow(_PyStackRef ref, const char *filename, int linenumber)
{
    if (ref.index < INITIAL_STACKREF_INDEX) {
        return;
    }
    PyInterpreterState *interp = PyInterpreterState_Get();
    TableEntry *entry = _Py_hashtable_get(interp->open_stackrefs_table, (void *)ref.index);
    if (entry == NULL) {
#ifdef Py_STACKREF_CLOSE_DEBUG
        entry = _Py_hashtable_get(interp->closed_stackrefs_table, (void *)ref.index);
        if (entry != NULL) {
            _Py_FatalErrorFormat(__func__,
                "Borrow of closed ref ID %" PRIu64 " at %s:%d. Referred to instance of %s at %p. Closed at %s:%d\n",
                (void *)ref.index, filename, linenumber, entry->classname, entry->obj, entry->filename, entry->linenumber);
        }
#endif
        _Py_FatalErrorFormat(__func__, "Invalid StackRef with ID %" PRIu64 " at %s:%d\n", (void *)ref.index, filename, linenumber);
    }
    entry->filename_borrow = filename;
    entry->linenumber_borrow = linenumber;
}


void
_Py_stackref_associate(PyInterpreterState *interp, PyObject *obj, _PyStackRef ref)
{
    assert(ref.index < INITIAL_STACKREF_INDEX);
    TableEntry *entry = make_table_entry(obj, "builtin-object", 0);
    if (entry == NULL) {
        Py_FatalError("No memory left for stackref debug table");
    }
    if (_Py_hashtable_set(interp->open_stackrefs_table, (void *)ref.index, (void *)entry) < 0) {
        Py_FatalError("No memory left for stackref debug table");
    }
}


static int
report_leak(_Py_hashtable_t *ht, const void *key, const void *value, void *leak)
{
    TableEntry *entry = (TableEntry *)value;
    if (!_Py_IsStaticImmortal(entry->obj)) {
        *(int *)leak = 1;
        printf("Stackref leak. Refers to instance of %s at %p. Created at %s:%d",
               entry->classname, entry->obj, entry->filename, entry->linenumber);
        if (entry->filename_borrow != NULL) {
            printf(". Last borrow at %s:%d",entry->filename_borrow, entry->linenumber_borrow);
        }
        printf("\n");
    }
    return 0;
}

void
_Py_stackref_report_leaks(PyInterpreterState *interp)
{
    int leak = 0;
    _Py_hashtable_foreach(interp->open_stackrefs_table, report_leak, &leak);
    if (leak) {
        fflush(stdout);
        Py_FatalError("Stackrefs leaked.");
    }
}

void
_PyStackRef_CLOSE_SPECIALIZED(_PyStackRef ref, destructor destruct, const char *filename, int linenumber)
{
    PyObject *obj = _Py_stackref_close(ref, filename, linenumber);
    _Py_DECREF_SPECIALIZED(obj, destruct);
}

_PyStackRef PyStackRef_TagInt(intptr_t i)
{
    return (_PyStackRef){ .index = (i << 1) + 1 };
}

intptr_t
PyStackRef_UntagInt(_PyStackRef i)
{
    assert(PyStackRef_IsTaggedInt(i));
    intptr_t val = (intptr_t)i.index;
    return Py_ARITHMETIC_RIGHT_SHIFT(intptr_t, val, 1);
}

bool
PyStackRef_IsNullOrInt(_PyStackRef ref)
{
    return PyStackRef_IsNull(ref) || PyStackRef_IsTaggedInt(ref);
}

#endif
