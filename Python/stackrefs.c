
#include "Python.h"

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
    TableEntry *entry = _Py_hashtable_get(interp->stackref_debug_table, (void *)ref.index);
    if (entry == NULL) {
        _Py_FatalErrorFormat(__func__, "Accessing closed stack ref with ID %" PRIu64 "\n", ref.index);
    }
    return entry->obj;
}

PyObject *_Py_stackref_close(_PyStackRef ref)
{
    PyInterpreterState *interp = PyInterpreterState_Get();
    if (ref.index >= interp->next_stackref) {
        _Py_FatalErrorFormat(__func__, "Garbled stack ref with ID %" PRIu64 "\n", ref.index);
    }
    PyObject *obj;
    if (ref.index <= 3) {
        // Pre-allocated reference to None, False or True -- Do not clear
        TableEntry *entry = _Py_hashtable_get(interp->stackref_debug_table, (void *)ref.index);
        obj = entry->obj;
    }
    else {
        TableEntry *entry = _Py_hashtable_steal(interp->stackref_debug_table, (void *)ref.index);
        if (entry == NULL) {
            _Py_FatalErrorFormat(__func__, "Invalid StackRef with ID %" PRIu64 "\n", (void *)ref.index);
        }
        obj = entry->obj;
        free(entry);
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
    uint64_t new_id = interp->next_stackref++;
    TableEntry *entry = make_table_entry(obj, filename, linenumber);
    if (entry == NULL) {
        Py_FatalError("No memory left for stackref debug table");
    }
    if (_Py_hashtable_set(interp->stackref_debug_table, (void *)new_id, entry) < 0) {
        Py_FatalError("No memory left for stackref debug table");
    }
    return (_PyStackRef){ .index = new_id };
}

void
_Py_stackref_associate(PyInterpreterState *interp, PyObject *obj, _PyStackRef ref)
{
    assert(interp->next_stackref >= ref.index);
    interp->next_stackref = ref.index+1;
    TableEntry *entry = make_table_entry(obj, "builtin-object", 0);
    if (entry == NULL) {
        Py_FatalError("No memory left for stackref debug table");
    }
    if (_Py_hashtable_set(interp->stackref_debug_table, (void *)ref.index, (void *)entry) < 0) {
        Py_FatalError("No memory left for stackref debug table");
    }
}


static int
report_leak(_Py_hashtable_t *ht, const void *key, const void *value, void *user_data)
{
    TableEntry *entry = (TableEntry *)value;
    if (!_Py_IsStaticImmortal(entry->obj)) {
        printf("Stackref leak. Refers to instance of %s at %p. Created at %s:%d\n",
               entry->classname, entry->obj, entry->filename, entry->linenumber);
    }
    return 0;
}

void
_Py_stackref_report_leaks(PyInterpreterState *interp)
{
    _Py_hashtable_foreach(interp->stackref_debug_table, report_leak, NULL);
}

#endif
