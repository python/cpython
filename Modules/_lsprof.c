#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "rotatingtree.h"

/************************************************************/
/* Written by Brett Rosen and Ted Czotter */

struct _ProfilerEntry;

/* represents a function called from another function */
typedef struct _ProfilerSubEntry {
    rotating_node_t header;
    _PyTime_t tt;
    _PyTime_t it;
    long callcount;
    long recursivecallcount;
    long recursionLevel;
} ProfilerSubEntry;

/* represents a function or user defined block */
typedef struct _ProfilerEntry {
    rotating_node_t header;
    PyObject *userObj; /* PyCodeObject, or a descriptive str for builtins */
    _PyTime_t tt; /* total time in this entry */
    _PyTime_t it; /* inline time in this entry (not in subcalls) */
    long callcount; /* how many times this was called */
    long recursivecallcount; /* how many times called recursively */
    long recursionLevel;
    rotating_node_t *calls;
} ProfilerEntry;

typedef struct _ProfilerContext {
    _PyTime_t t0;
    _PyTime_t subt;
    struct _ProfilerContext *previous;
    ProfilerEntry *ctxEntry;
} ProfilerContext;

typedef struct {
    PyObject_HEAD
    rotating_node_t *profilerEntries;
    ProfilerContext *currentProfilerContext;
    ProfilerContext *freelistProfilerContext;
    int flags;
    PyObject *externalTimer;
    double externalTimerUnit;
    int tool_id;
    PyObject* missing;
} ProfilerObject;

#define POF_ENABLED     0x001
#define POF_SUBCALLS    0x002
#define POF_BUILTINS    0x004
#define POF_EXT_TIMER   0x008
#define POF_NOMEMORY    0x100

/*[clinic input]
module _lsprof
class _lsprof.Profiler "ProfilerObject *" "&ProfilerType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e349ac952152f336]*/

#include "clinic/_lsprof.c.h"

typedef struct {
    PyTypeObject *profiler_type;
    PyTypeObject *stats_entry_type;
    PyTypeObject *stats_subentry_type;
} _lsprof_state;

static inline _lsprof_state*
_lsprof_get_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_lsprof_state *)state;
}

/*** External Timers ***/

static _PyTime_t CallExternalTimer(ProfilerObject *pObj)
{
    PyObject *o = NULL;

    // External timer can do arbitrary things so we need a flag to prevent
    // horrible things to happen
    pObj->flags |= POF_EXT_TIMER;
    o = _PyObject_CallNoArgs(pObj->externalTimer);
    pObj->flags &= ~POF_EXT_TIMER;

    if (o == NULL) {
        PyErr_WriteUnraisable(pObj->externalTimer);
        return 0;
    }

    _PyTime_t result;
    int err;
    if (pObj->externalTimerUnit > 0.0) {
        /* interpret the result as an integer that will be scaled
           in profiler_getstats() */
        err = _PyTime_FromNanosecondsObject(&result, o);
    }
    else {
        /* interpret the result as a double measured in seconds.
           As the profiler works with _PyTime_t internally
           we convert it to a large integer */
        err = _PyTime_FromSecondsObject(&result, o, _PyTime_ROUND_FLOOR);
    }
    Py_DECREF(o);
    if (err < 0) {
        PyErr_WriteUnraisable(pObj->externalTimer);
        return 0;
    }
    return result;
}

static inline _PyTime_t
call_timer(ProfilerObject *pObj)
{
    if (pObj->externalTimer != NULL) {
        return CallExternalTimer(pObj);
    }
    else {
        return _PyTime_GetPerfCounter();
    }
}


/*** ProfilerObject ***/

static PyObject *
normalizeUserObj(PyObject *obj)
{
    PyCFunctionObject *fn;
    if (!PyCFunction_Check(obj)) {
        return Py_NewRef(obj);
    }
    /* Replace built-in function objects with a descriptive string
       because of built-in methods -- keeping a reference to
       __self__ is probably not a good idea. */
    fn = (PyCFunctionObject *)obj;

    if (fn->m_self == NULL) {
        /* built-in function: look up the module name */
        PyObject *mod = fn->m_module;
        PyObject *modname = NULL;
        if (mod != NULL) {
            if (PyUnicode_Check(mod)) {
                modname = Py_NewRef(mod);
            }
            else if (PyModule_Check(mod)) {
                modname = PyModule_GetNameObject(mod);
                if (modname == NULL)
                    PyErr_Clear();
            }
        }
        if (modname != NULL) {
            if (!_PyUnicode_EqualToASCIIString(modname, "builtins")) {
                PyObject *result;
                result = PyUnicode_FromFormat("<%U.%s>", modname,
                                              fn->m_ml->ml_name);
                Py_DECREF(modname);
                return result;
            }
            Py_DECREF(modname);
        }
        return PyUnicode_FromFormat("<%s>", fn->m_ml->ml_name);
    }
    else {
        /* built-in method: try to return
            repr(getattr(type(__self__), __name__))
        */
        PyObject *self = fn->m_self;
        PyObject *name = PyUnicode_FromString(fn->m_ml->ml_name);
        PyObject *modname = fn->m_module;

        if (name != NULL) {
            PyObject *mo = _PyType_Lookup(Py_TYPE(self), name);
            Py_XINCREF(mo);
            Py_DECREF(name);
            if (mo != NULL) {
                PyObject *res = PyObject_Repr(mo);
                Py_DECREF(mo);
                if (res != NULL)
                    return res;
            }
        }
        /* Otherwise, use __module__ */
        PyErr_Clear();
        if (modname != NULL && PyUnicode_Check(modname))
            return PyUnicode_FromFormat("<built-in method %S.%s>",
                                        modname,  fn->m_ml->ml_name);
        else
            return PyUnicode_FromFormat("<built-in method %s>",
                                        fn->m_ml->ml_name);
    }
}

static ProfilerEntry*
newProfilerEntry(ProfilerObject *pObj, void *key, PyObject *userObj)
{
    ProfilerEntry *self;
    self = (ProfilerEntry*) PyMem_Malloc(sizeof(ProfilerEntry));
    if (self == NULL) {
        pObj->flags |= POF_NOMEMORY;
        return NULL;
    }
    userObj = normalizeUserObj(userObj);
    if (userObj == NULL) {
        PyErr_Clear();
        PyMem_Free(self);
        pObj->flags |= POF_NOMEMORY;
        return NULL;
    }
    self->header.key = key;
    self->userObj = userObj;
    self->tt = 0;
    self->it = 0;
    self->callcount = 0;
    self->recursivecallcount = 0;
    self->recursionLevel = 0;
    self->calls = EMPTY_ROTATING_TREE;
    RotatingTree_Add(&pObj->profilerEntries, &self->header);
    return self;
}

static ProfilerEntry*
getEntry(ProfilerObject *pObj, void *key)
{
    return (ProfilerEntry*) RotatingTree_Get(&pObj->profilerEntries, key);
}

static ProfilerSubEntry *
getSubEntry(ProfilerObject *pObj, ProfilerEntry *caller, ProfilerEntry* entry)
{
    return (ProfilerSubEntry*) RotatingTree_Get(&caller->calls,
                                                (void *)entry);
}

static ProfilerSubEntry *
newSubEntry(ProfilerObject *pObj,  ProfilerEntry *caller, ProfilerEntry* entry)
{
    ProfilerSubEntry *self;
    self = (ProfilerSubEntry*) PyMem_Malloc(sizeof(ProfilerSubEntry));
    if (self == NULL) {
        pObj->flags |= POF_NOMEMORY;
        return NULL;
    }
    self->header.key = (void *)entry;
    self->tt = 0;
    self->it = 0;
    self->callcount = 0;
    self->recursivecallcount = 0;
    self->recursionLevel = 0;
    RotatingTree_Add(&caller->calls, &self->header);
    return self;
}

static int freeSubEntry(rotating_node_t *header, void *arg)
{
    ProfilerSubEntry *subentry = (ProfilerSubEntry*) header;
    PyMem_Free(subentry);
    return 0;
}

static int freeEntry(rotating_node_t *header, void *arg)
{
    ProfilerEntry *entry = (ProfilerEntry*) header;
    RotatingTree_Enum(entry->calls, freeSubEntry, NULL);
    Py_DECREF(entry->userObj);
    PyMem_Free(entry);
    return 0;
}

static void clearEntries(ProfilerObject *pObj)
{
    RotatingTree_Enum(pObj->profilerEntries, freeEntry, NULL);
    pObj->profilerEntries = EMPTY_ROTATING_TREE;
    /* release the memory hold by the ProfilerContexts */
    if (pObj->currentProfilerContext) {
        PyMem_Free(pObj->currentProfilerContext);
        pObj->currentProfilerContext = NULL;
    }
    while (pObj->freelistProfilerContext) {
        ProfilerContext *c = pObj->freelistProfilerContext;
        pObj->freelistProfilerContext = c->previous;
        PyMem_Free(c);
    }
    pObj->freelistProfilerContext = NULL;
}

static void
initContext(ProfilerObject *pObj, ProfilerContext *self, ProfilerEntry *entry)
{
    self->ctxEntry = entry;
    self->subt = 0;
    self->previous = pObj->currentProfilerContext;
    pObj->currentProfilerContext = self;
    ++entry->recursionLevel;
    if ((pObj->flags & POF_SUBCALLS) && self->previous) {
        /* find or create an entry for me in my caller's entry */
        ProfilerEntry *caller = self->previous->ctxEntry;
        ProfilerSubEntry *subentry = getSubEntry(pObj, caller, entry);
        if (subentry == NULL)
            subentry = newSubEntry(pObj, caller, entry);
        if (subentry)
            ++subentry->recursionLevel;
    }
    self->t0 = call_timer(pObj);
}

static void
Stop(ProfilerObject *pObj, ProfilerContext *self, ProfilerEntry *entry)
{
    _PyTime_t tt = call_timer(pObj) - self->t0;
    _PyTime_t it = tt - self->subt;
    if (self->previous)
        self->previous->subt += tt;
    pObj->currentProfilerContext = self->previous;
    if (--entry->recursionLevel == 0)
        entry->tt += tt;
    else
        ++entry->recursivecallcount;
    entry->it += it;
    entry->callcount++;
    if ((pObj->flags & POF_SUBCALLS) && self->previous) {
        /* find or create an entry for me in my caller's entry */
        ProfilerEntry *caller = self->previous->ctxEntry;
        ProfilerSubEntry *subentry = getSubEntry(pObj, caller, entry);
        if (subentry) {
            if (--subentry->recursionLevel == 0)
                subentry->tt += tt;
            else
                ++subentry->recursivecallcount;
            subentry->it += it;
            ++subentry->callcount;
        }
    }
}

static void
ptrace_enter_call(PyObject *self, void *key, PyObject *userObj)
{
    /* entering a call to the function identified by 'key'
       (which can be a PyCodeObject or a PyMethodDef pointer) */
    ProfilerObject *pObj = (ProfilerObject*)self;
    ProfilerEntry *profEntry;
    ProfilerContext *pContext;

    /* In the case of entering a generator expression frame via a
     * throw (gen_send_ex(.., 1)), we may already have an
     * Exception set here. We must not mess around with this
     * exception, and some of the code under here assumes that
     * PyErr_* is its own to mess around with, so we have to
     * save and restore any current exception. */
    PyObject *exc = PyErr_GetRaisedException();

    profEntry = getEntry(pObj, key);
    if (profEntry == NULL) {
        profEntry = newProfilerEntry(pObj, key, userObj);
        if (profEntry == NULL)
            goto restorePyerr;
    }
    /* grab a ProfilerContext out of the free list */
    pContext = pObj->freelistProfilerContext;
    if (pContext) {
        pObj->freelistProfilerContext = pContext->previous;
    }
    else {
        /* free list exhausted, allocate a new one */
        pContext = (ProfilerContext*)
            PyMem_Malloc(sizeof(ProfilerContext));
        if (pContext == NULL) {
            pObj->flags |= POF_NOMEMORY;
            goto restorePyerr;
        }
    }
    initContext(pObj, pContext, profEntry);

restorePyerr:
    PyErr_SetRaisedException(exc);
}

static void
ptrace_leave_call(PyObject *self, void *key)
{
    /* leaving a call to the function identified by 'key' */
    ProfilerObject *pObj = (ProfilerObject*)self;
    ProfilerEntry *profEntry;
    ProfilerContext *pContext;

    pContext = pObj->currentProfilerContext;
    if (pContext == NULL)
        return;
    profEntry = getEntry(pObj, key);
    if (profEntry) {
        Stop(pObj, pContext, profEntry);
    }
    else {
        pObj->currentProfilerContext = pContext->previous;
    }
    /* put pContext into the free list */
    pContext->previous = pObj->freelistProfilerContext;
    pObj->freelistProfilerContext = pContext;
}

static int
pending_exception(ProfilerObject *pObj)
{
    if (pObj->flags & POF_NOMEMORY) {
        pObj->flags -= POF_NOMEMORY;
        PyErr_SetString(PyExc_MemoryError,
                        "memory was exhausted while profiling");
        return -1;
    }
    return 0;
}

/************************************************************/

static PyStructSequence_Field profiler_entry_fields[] = {
    {"code",         "code object or built-in function name"},
    {"callcount",    "how many times this was called"},
    {"reccallcount", "how many times called recursively"},
    {"totaltime",    "total time in this entry"},
    {"inlinetime",   "inline time in this entry (not in subcalls)"},
    {"calls",        "details of the calls"},
    {0}
};

static PyStructSequence_Field profiler_subentry_fields[] = {
    {"code",         "called code object or built-in function name"},
    {"callcount",    "how many times this is called"},
    {"reccallcount", "how many times this is called recursively"},
    {"totaltime",    "total time spent in this call"},
    {"inlinetime",   "inline time (not in further subcalls)"},
    {0}
};

static PyStructSequence_Desc profiler_entry_desc = {
    .name = "_lsprof.profiler_entry",
    .fields = profiler_entry_fields,
    .doc = NULL,
    .n_in_sequence = 6
};

static PyStructSequence_Desc profiler_subentry_desc = {
    .name = "_lsprof.profiler_subentry",
    .fields = profiler_subentry_fields,
    .doc = NULL,
    .n_in_sequence = 5
};

typedef struct {
    PyObject *list;
    PyObject *sublist;
    double factor;
    _lsprof_state *state;
} statscollector_t;

static int statsForSubEntry(rotating_node_t *node, void *arg)
{
    ProfilerSubEntry *sentry = (ProfilerSubEntry*) node;
    statscollector_t *collect = (statscollector_t*) arg;
    ProfilerEntry *entry = (ProfilerEntry*) sentry->header.key;
    int err;
    PyObject *sinfo;
    sinfo = PyObject_CallFunction((PyObject*) collect->state->stats_subentry_type,
                                  "((Olldd))",
                                  entry->userObj,
                                  sentry->callcount,
                                  sentry->recursivecallcount,
                                  collect->factor * sentry->tt,
                                  collect->factor * sentry->it);
    if (sinfo == NULL)
        return -1;
    err = PyList_Append(collect->sublist, sinfo);
    Py_DECREF(sinfo);
    return err;
}

static int statsForEntry(rotating_node_t *node, void *arg)
{
    ProfilerEntry *entry = (ProfilerEntry*) node;
    statscollector_t *collect = (statscollector_t*) arg;
    PyObject *info;
    int err;
    if (entry->callcount == 0)
        return 0;   /* skip */

    if (entry->calls != EMPTY_ROTATING_TREE) {
        collect->sublist = PyList_New(0);
        if (collect->sublist == NULL)
            return -1;
        if (RotatingTree_Enum(entry->calls,
                              statsForSubEntry, collect) != 0) {
            Py_DECREF(collect->sublist);
            return -1;
        }
    }
    else {
        collect->sublist = Py_NewRef(Py_None);
    }

    info = PyObject_CallFunction((PyObject*) collect->state->stats_entry_type,
                                 "((OllddO))",
                                 entry->userObj,
                                 entry->callcount,
                                 entry->recursivecallcount,
                                 collect->factor * entry->tt,
                                 collect->factor * entry->it,
                                 collect->sublist);
    Py_DECREF(collect->sublist);
    if (info == NULL)
        return -1;
    err = PyList_Append(collect->list, info);
    Py_DECREF(info);
    return err;
}

/*[clinic input]
_lsprof.Profiler.getstats

    cls: defining_class

list of profiler_entry objects.

getstats() -> list of profiler_entry objects

Return all information collected by the profiler.
Each profiler_entry is a tuple-like object with the
following attributes:

    code          code object
    callcount     how many times this was called
    reccallcount  how many times called recursively
    totaltime     total time in this entry
    inlinetime    inline time in this entry (not in subcalls)
    calls         details of the calls

The calls attribute is either None or a list of
profiler_subentry objects:

    code          called code object
    callcount     how many times this is called
    reccallcount  how many times this is called recursively
    totaltime     total time spent in this call
    inlinetime    inline time (not in further subcalls)
[clinic start generated code]*/

static PyObject *
_lsprof_Profiler_getstats_impl(ProfilerObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=1806ef720019ee03 input=445e193ef4522902]*/
{
    statscollector_t collect;
    collect.state = _PyType_GetModuleState(cls);
    if (pending_exception(self)) {
        return NULL;
    }
    if (!self->externalTimer || self->externalTimerUnit == 0.0) {
        _PyTime_t onesec = _PyTime_FromSeconds(1);
        collect.factor = (double)1 / onesec;
    }
    else {
        collect.factor = self->externalTimerUnit;
    }

    collect.list = PyList_New(0);
    if (collect.list == NULL)
        return NULL;
    if (RotatingTree_Enum(self->profilerEntries, statsForEntry, &collect)
        != 0) {
        Py_DECREF(collect.list);
        return NULL;
    }
    return collect.list;
}

static int
setSubcalls(ProfilerObject *pObj, int nvalue)
{
    if (nvalue == 0)
        pObj->flags &= ~POF_SUBCALLS;
    else if (nvalue > 0)
        pObj->flags |=  POF_SUBCALLS;
    return 0;
}

static int
setBuiltins(ProfilerObject *pObj, int nvalue)
{
    if (nvalue == 0)
        pObj->flags &= ~POF_BUILTINS;
    else if (nvalue > 0) {
        pObj->flags |=  POF_BUILTINS;
    }
    return 0;
}

PyObject* pystart_callback(ProfilerObject* self, PyObject *const *args, Py_ssize_t size)
{
    PyObject* code = args[0];
    ptrace_enter_call((PyObject*)self, (void *)code, (PyObject *)code);

    Py_RETURN_NONE;
}

PyObject* pyreturn_callback(ProfilerObject* self, PyObject *const *args, Py_ssize_t size)
{
    PyObject* code = args[0];
    ptrace_leave_call((PyObject*)self, (void *)code);

    Py_RETURN_NONE;
}

PyObject* get_cfunc_from_callable(PyObject* callable, PyObject* self_arg, PyObject* missing)
{
    // return a new reference
    if (PyCFunction_Check(callable)) {
        Py_INCREF(callable);
        return (PyObject*)((PyCFunctionObject *)callable);
    }
    if (Py_TYPE(callable) == &PyMethodDescr_Type) {
        /* For backwards compatibility need to
         * convert to builtin method */

        /* If no arg, skip */
        if (self_arg == missing) {
            return NULL;
        }
        PyObject *meth = Py_TYPE(callable)->tp_descr_get(
            callable, self_arg, (PyObject*)Py_TYPE(self_arg));
        if (meth == NULL) {
            return NULL;
        }
        if (PyCFunction_Check(meth)) {
            return (PyObject*)((PyCFunctionObject *)meth);
        }
    }
    return NULL;
}

PyObject* ccall_callback(ProfilerObject* self, PyObject *const *args, Py_ssize_t size)
{
    if (self->flags & POF_BUILTINS) {
        PyObject* callable = args[2];
        PyObject* self_arg = args[3];

        PyObject* cfunc = get_cfunc_from_callable(callable, self_arg, self->missing);

        if (cfunc) {
            ptrace_enter_call((PyObject*)self,
                              ((PyCFunctionObject *)cfunc)->m_ml,
                              cfunc);
            Py_DECREF(cfunc);
        }
    }
    Py_RETURN_NONE;
}

PyObject* creturn_callback(ProfilerObject* self, PyObject *const *args, Py_ssize_t size)
{
    if (self->flags & POF_BUILTINS) {
        PyObject* callable = args[2];
        PyObject* self_arg = args[3];

        PyObject* cfunc = get_cfunc_from_callable(callable, self_arg, self->missing);

        if (cfunc) {
            ptrace_leave_call((PyObject*)self,
                              ((PyCFunctionObject *)cfunc)->m_ml);
            Py_DECREF(cfunc);
        }
    }
    Py_RETURN_NONE;
}

static const struct {
    int event;
    const char* callback_method;
} callback_table[] = {
    {PY_MONITORING_EVENT_PY_START, "_pystart_callback"},
    {PY_MONITORING_EVENT_PY_RESUME, "_pystart_callback"},
    {PY_MONITORING_EVENT_PY_THROW, "_pystart_callback"},
    {PY_MONITORING_EVENT_PY_RETURN, "_pyreturn_callback"},
    {PY_MONITORING_EVENT_PY_YIELD, "_pyreturn_callback"},
    {PY_MONITORING_EVENT_PY_UNWIND, "_pyreturn_callback"},
    {PY_MONITORING_EVENT_CALL, "_ccall_callback"},
    {PY_MONITORING_EVENT_C_RETURN, "_creturn_callback"},
    {PY_MONITORING_EVENT_C_RAISE, "_creturn_callback"},
    {0, NULL}
};

PyDoc_STRVAR(enable_doc, "\
enable(subcalls=True, builtins=True)\n\
\n\
Start collecting profiling information.\n\
If 'subcalls' is True, also records for each function\n\
statistics separated according to its current caller.\n\
If 'builtins' is True, records the time spent in\n\
built-in functions separately from their caller.\n\
");

static PyObject*
profiler_enable(ProfilerObject *self, PyObject *args, PyObject *kwds)
{
    int subcalls = -1;
    int builtins = -1;
    static char *kwlist[] = {"subcalls", "builtins", 0};
    int all_events = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|pp:enable",
                                     kwlist, &subcalls, &builtins))
        return NULL;
    if (setSubcalls(self, subcalls) < 0 || setBuiltins(self, builtins) < 0) {
        return NULL;
    }

    PyObject* monitoring = _PyImport_GetModuleAttrString("sys", "monitoring");
    if (!monitoring) {
        return NULL;
    }

    if (PyObject_CallMethod(monitoring, "use_tool_id", "is", self->tool_id, "cProfile") == NULL) {
        PyErr_Format(PyExc_ValueError, "Another profiling tool is already active");
        Py_DECREF(monitoring);
        return NULL;
    }

    for (int i = 0; callback_table[i].callback_method; i++) {
        PyObject* callback = PyObject_GetAttrString((PyObject*)self, callback_table[i].callback_method);
        if (!callback) {
            Py_DECREF(monitoring);
            return NULL;
        }
        Py_XDECREF(PyObject_CallMethod(monitoring, "register_callback", "iiO", self->tool_id,
                                       (1 << callback_table[i].event),
                                       callback));
        Py_DECREF(callback);
        all_events |= (1 << callback_table[i].event);
    }

    if (!PyObject_CallMethod(monitoring, "set_events", "ii", self->tool_id, all_events)) {
        Py_DECREF(monitoring);
        return NULL;
    }

    Py_DECREF(monitoring);

    self->flags |= POF_ENABLED;
    Py_RETURN_NONE;
}

static void
flush_unmatched(ProfilerObject *pObj)
{
    while (pObj->currentProfilerContext) {
        ProfilerContext *pContext = pObj->currentProfilerContext;
        ProfilerEntry *profEntry= pContext->ctxEntry;
        if (profEntry)
            Stop(pObj, pContext, profEntry);
        else
            pObj->currentProfilerContext = pContext->previous;
        if (pContext)
            PyMem_Free(pContext);
    }

}

PyDoc_STRVAR(disable_doc, "\
disable()\n\
\n\
Stop collecting profiling information.\n\
");

static PyObject*
profiler_disable(ProfilerObject *self, PyObject* noarg)
{
    if (self->flags & POF_EXT_TIMER) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot disable profiler in external timer");
        return NULL;
    }
    if (self->flags & POF_ENABLED) {
        PyObject* result = NULL;
        PyObject* monitoring = _PyImport_GetModuleAttrString("sys", "monitoring");

        if (!monitoring) {
            return NULL;
        }

        for (int i = 0; callback_table[i].callback_method; i++) {
            result = PyObject_CallMethod(monitoring, "register_callback", "iiO", self->tool_id,
                                         (1 << callback_table[i].event), Py_None);
            if (!result) {
                Py_DECREF(monitoring);
                return NULL;
            }
            Py_DECREF(result);
        }

        result = PyObject_CallMethod(monitoring, "set_events", "ii", self->tool_id, 0);
        if (!result) {
            Py_DECREF(monitoring);
            return NULL;
        }
        Py_DECREF(result);

        result = PyObject_CallMethod(monitoring, "free_tool_id", "i", self->tool_id);
        if (!result) {
            Py_DECREF(monitoring);
            return NULL;
        }
        Py_DECREF(result);

        Py_DECREF(monitoring);

        self->flags &= ~POF_ENABLED;
        flush_unmatched(self);
    }

    if (pending_exception(self)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(clear_doc, "\
clear()\n\
\n\
Clear all profiling information collected so far.\n\
");

static PyObject*
profiler_clear(ProfilerObject *pObj, PyObject* noarg)
{
    if (pObj->flags & POF_EXT_TIMER) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot clear profiler in external timer");
        return NULL;
    }
    clearEntries(pObj);
    Py_RETURN_NONE;
}

static int
profiler_traverse(ProfilerObject *op, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(op));
    Py_VISIT(op->externalTimer);
    return 0;
}

static void
profiler_dealloc(ProfilerObject *op)
{
    PyObject_GC_UnTrack(op);
    if (op->flags & POF_ENABLED) {
        PyThreadState *tstate = _PyThreadState_GET();
        if (_PyEval_SetProfile(tstate, NULL, NULL) < 0) {
            _PyErr_WriteUnraisableMsg("When destroying _lsprof profiler", NULL);
        }
    }

    flush_unmatched(op);
    clearEntries(op);
    Py_XDECREF(op->externalTimer);
    PyTypeObject *tp = Py_TYPE(op);
    tp->tp_free(op);
    Py_DECREF(tp);
}

static int
profiler_init(ProfilerObject *pObj, PyObject *args, PyObject *kw)
{
    PyObject *timer = NULL;
    double timeunit = 0.0;
    int subcalls = 1;
    int builtins = 1;
    static char *kwlist[] = {"timer", "timeunit",
                                   "subcalls", "builtins", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "|Odpp:Profiler", kwlist,
                                     &timer, &timeunit,
                                     &subcalls, &builtins))
        return -1;

    if (setSubcalls(pObj, subcalls) < 0 || setBuiltins(pObj, builtins) < 0)
        return -1;
    pObj->externalTimerUnit = timeunit;
    Py_XSETREF(pObj->externalTimer, Py_XNewRef(timer));
    pObj->tool_id = PY_MONITORING_PROFILER_ID;

    PyObject* monitoring = _PyImport_GetModuleAttrString("sys", "monitoring");
    if (!monitoring) {
        return -1;
    }
    pObj->missing = PyObject_GetAttrString(monitoring, "MISSING");
    if (!pObj->missing) {
        Py_DECREF(monitoring);
        return -1;
    }
    Py_DECREF(monitoring);
    return 0;
}

static PyMethodDef profiler_methods[] = {
    _LSPROF_PROFILER_GETSTATS_METHODDEF
    {"enable",             _PyCFunction_CAST(profiler_enable),
                    METH_VARARGS | METH_KEYWORDS,       enable_doc},
    {"disable",            (PyCFunction)profiler_disable,
                    METH_NOARGS,                        disable_doc},
    {"clear",              (PyCFunction)profiler_clear,
                    METH_NOARGS,                        clear_doc},
    {"_pystart_callback",  _PyCFunction_CAST(pystart_callback),
                    METH_FASTCALL,                       NULL},
    {"_pyreturn_callback", _PyCFunction_CAST(pyreturn_callback),
                    METH_FASTCALL,                       NULL},
    {"_ccall_callback",    _PyCFunction_CAST(ccall_callback),
                    METH_FASTCALL,                       NULL},
    {"_creturn_callback", _PyCFunction_CAST(creturn_callback),
                    METH_FASTCALL,                       NULL},
    {NULL, NULL}
};

PyDoc_STRVAR(profiler_doc, "\
Profiler(timer=None, timeunit=None, subcalls=True, builtins=True)\n\
\n\
    Builds a profiler object using the specified timer function.\n\
    The default timer is a fast built-in one based on real time.\n\
    For custom timer functions returning integers, timeunit can\n\
    be a float specifying a scale (i.e. how long each integer unit\n\
    is, in seconds).\n\
");

static PyType_Slot _lsprof_profiler_type_spec_slots[] = {
    {Py_tp_doc, (void *)profiler_doc},
    {Py_tp_methods, profiler_methods},
    {Py_tp_dealloc, profiler_dealloc},
    {Py_tp_init, profiler_init},
    {Py_tp_traverse, profiler_traverse},
    {0, 0}
};

static PyType_Spec _lsprof_profiler_type_spec = {
    .name = "_lsprof.Profiler",
    .basicsize = sizeof(ProfilerObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = _lsprof_profiler_type_spec_slots,
};

static PyMethodDef moduleMethods[] = {
    {NULL, NULL}
};

static int
_lsprof_traverse(PyObject *module, visitproc visit, void *arg)
{
    _lsprof_state *state = _lsprof_get_state(module);
    Py_VISIT(state->profiler_type);
    Py_VISIT(state->stats_entry_type);
    Py_VISIT(state->stats_subentry_type);
    return 0;
}

static int
_lsprof_clear(PyObject *module)
{
    _lsprof_state *state = _lsprof_get_state(module);
    Py_CLEAR(state->profiler_type);
    Py_CLEAR(state->stats_entry_type);
    Py_CLEAR(state->stats_subentry_type);
    return 0;
}

static void
_lsprof_free(void *module)
{
    _lsprof_clear((PyObject *)module);
}

static int
_lsprof_exec(PyObject *module)
{
    _lsprof_state *state = PyModule_GetState(module);

    state->profiler_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &_lsprof_profiler_type_spec, NULL);
    if (state->profiler_type == NULL) {
        return -1;
    }

    if (PyModule_AddType(module, state->profiler_type) < 0) {
        return -1;
    }

    state->stats_entry_type = PyStructSequence_NewType(&profiler_entry_desc);
    if (state->stats_entry_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->stats_entry_type) < 0) {
        return -1;
    }

    state->stats_subentry_type = PyStructSequence_NewType(&profiler_subentry_desc);
    if (state->stats_subentry_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->stats_subentry_type) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot _lsprofslots[] = {
    {Py_mod_exec, _lsprof_exec},
    // XXX gh-103092: fix isolation.
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    //{Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};

static struct PyModuleDef _lsprofmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_lsprof",
    .m_doc = "Fast profiler",
    .m_size = sizeof(_lsprof_state),
    .m_methods = moduleMethods,
    .m_slots = _lsprofslots,
    .m_traverse = _lsprof_traverse,
    .m_clear = _lsprof_clear,
    .m_free = _lsprof_free
};

PyMODINIT_FUNC
PyInit__lsprof(void)
{
    return PyModuleDef_Init(&_lsprofmodule);
}
