#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _PyEval_SetProfile()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_time.h"          // _PyTime_FromLong()
#include "pycore_typeobject.h"    // _PyType_GetModuleState()
#include "pycore_unicodeobject.h" // _PyUnicode_EqualToASCIIString()

#include "rotatingtree.h"

/************************************************************/
/* Written by Brett Rosen and Ted Czotter */

struct _ProfilerEntry;

/* represents a function called from another function */
typedef struct _ProfilerSubEntry {
    rotating_node_t header;
    PyTime_t tt;
    PyTime_t it;
    long callcount;
    long recursivecallcount;
    long recursionLevel;
} ProfilerSubEntry;

/* represents a function or user defined block */
typedef struct _ProfilerEntry {
    rotating_node_t header;
    PyObject *userObj; /* PyCodeObject, or a descriptive str for builtins */
    PyTime_t tt; /* total time in this entry */
    PyTime_t it; /* inline time in this entry (not in subcalls) */
    long callcount; /* how many times this was called */
    long recursivecallcount; /* how many times called recursively */
    long recursionLevel;
    rotating_node_t *calls;
} ProfilerEntry;

typedef struct _ProfilerContext {
    PyTime_t t0;
    PyTime_t subt;
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

#define ProfilerObject_CAST(op) ((ProfilerObject *)(op))

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

static PyTime_t CallExternalTimer(ProfilerObject *pObj)
{
    PyObject *o = NULL;

    // External timer can do arbitrary things so we need a flag to prevent
    // horrible things to happen
    pObj->flags |= POF_EXT_TIMER;
    o = _PyObject_CallNoArgs(pObj->externalTimer);
    pObj->flags &= ~POF_EXT_TIMER;

    if (o == NULL) {
        PyErr_FormatUnraisable("Exception ignored while calling "
                               "_lsprof timer %R", pObj->externalTimer);
        return 0;
    }

    PyTime_t result;
    int err;
    if (pObj->externalTimerUnit > 0.0) {
        /* interpret the result as an integer that will be scaled
           in profiler_getstats() */
        err = _PyTime_FromLong(&result, o);
    }
    else {
        /* interpret the result as a double measured in seconds.
           As the profiler works with PyTime_t internally
           we convert it to a large integer */
        err = _PyTime_FromSecondsObject(&result, o, _PyTime_ROUND_FLOOR);
    }
    Py_DECREF(o);
    if (err < 0) {
        PyErr_FormatUnraisable("Exception ignored while calling "
                               "_lsprof timer %R", pObj->externalTimer);
        return 0;
    }
    return result;
}

static inline PyTime_t
call_timer(ProfilerObject *pObj)
{
    if (pObj->externalTimer != NULL) {
        return CallExternalTimer(pObj);
    }
    else {
        PyTime_t t;
        (void)PyTime_PerfCounterRaw(&t);
        return t;
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
            PyObject *mo = _PyType_LookupRef(Py_TYPE(self), name);
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
    PyTime_t tt = call_timer(pObj) - self->t0;
    PyTime_t it = tt - self->subt;
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
        PyTime_t onesec = _PyTime_FromSeconds(1);
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

/*[clinic input]
_lsprof.Profiler._pystart_callback

    code: object
    instruction_offset: object
    /

[clinic start generated code]*/

static PyObject *
_lsprof_Profiler__pystart_callback_impl(ProfilerObject *self, PyObject *code,
                                        PyObject *instruction_offset)
/*[clinic end generated code: output=5fec8b7ad5ed25e8 input=b166e6953c579cda]*/
{
    ptrace_enter_call((PyObject*)self, (void *)code, code);

    Py_RETURN_NONE;
}

/*[clinic input]
_lsprof.Profiler._pyreturn_callback

    code: object
    instruction_offset: object
    retval: object
    /

[clinic start generated code]*/

static PyObject *
_lsprof_Profiler__pyreturn_callback_impl(ProfilerObject *self,
                                         PyObject *code,
                                         PyObject *instruction_offset,
                                         PyObject *retval)
/*[clinic end generated code: output=9e2f6fc1b882c51e input=667ffaeb2fa6fd1f]*/
{
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

/*[clinic input]
_lsprof.Profiler._ccall_callback

    code: object
    instruction_offset: object
    callable: object
    self_arg: object
    /

[clinic start generated code]*/

static PyObject *
_lsprof_Profiler__ccall_callback_impl(ProfilerObject *self, PyObject *code,
                                      PyObject *instruction_offset,
                                      PyObject *callable, PyObject *self_arg)
/*[clinic end generated code: output=152db83cabd18cad input=0e66687cfb95c001]*/
{
    if (self->flags & POF_BUILTINS) {
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

/*[clinic input]
_lsprof.Profiler._creturn_callback

    code: object
    instruction_offset: object
    callable: object
    self_arg: object
    /

[clinic start generated code]*/

static PyObject *
_lsprof_Profiler__creturn_callback_impl(ProfilerObject *self, PyObject *code,
                                        PyObject *instruction_offset,
                                        PyObject *callable,
                                        PyObject *self_arg)
/*[clinic end generated code: output=1e886dde8fed8fb0 input=b18afe023746923a]*/
{
    if (self->flags & POF_BUILTINS) {
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


/*[clinic input]
_lsprof.Profiler.enable

    subcalls: bool = True
        If True, also records for each function
        statistics separated according to its current caller.

    builtins: bool = True
        If True, records the time spent in
        built-in functions separately from their caller.

Start collecting profiling information.
[clinic start generated code]*/

static PyObject *
_lsprof_Profiler_enable_impl(ProfilerObject *self, int subcalls,
                             int builtins)
/*[clinic end generated code: output=1e747f9dc1edd571 input=9ab81405107ab7f1]*/
{
    int all_events = 0;
    if (setSubcalls(self, subcalls) < 0 || setBuiltins(self, builtins) < 0) {
        return NULL;
    }

    PyObject* monitoring = PyImport_ImportModuleAttrString("sys", "monitoring");
    if (!monitoring) {
        return NULL;
    }

    PyObject *check = PyObject_CallMethod(monitoring,
                                          "use_tool_id", "is",
                                          self->tool_id, "cProfile");
    if (check == NULL) {
        PyErr_Format(PyExc_ValueError, "Another profiling tool is already active");
        goto error;
    }
    Py_DECREF(check);

    for (int i = 0; callback_table[i].callback_method; i++) {
        int event = (1 << callback_table[i].event);
        PyObject* callback = PyObject_GetAttrString((PyObject*)self, callback_table[i].callback_method);
        if (!callback) {
            goto error;
        }
        PyObject *register_result = PyObject_CallMethod(monitoring, "register_callback",
                                                        "iiO", self->tool_id,
                                                        event, callback);
        Py_DECREF(callback);
        if (register_result == NULL) {
            goto error;
        }
        Py_DECREF(register_result);
        all_events |= event;
    }

    PyObject *event_result = PyObject_CallMethod(monitoring, "set_events", "ii",
                                                 self->tool_id, all_events);
    if (event_result == NULL) {
        goto error;
    }

    Py_DECREF(event_result);
    Py_DECREF(monitoring);

    self->flags |= POF_ENABLED;
    Py_RETURN_NONE;

error:
    Py_DECREF(monitoring);
    return NULL;
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


/*[clinic input]
_lsprof.Profiler.disable

Stop collecting profiling information.
[clinic start generated code]*/

static PyObject *
_lsprof_Profiler_disable_impl(ProfilerObject *self)
/*[clinic end generated code: output=838cffef7f651870 input=05700b3fc68d1f50]*/
{
    if (self->flags & POF_EXT_TIMER) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot disable profiler in external timer");
        return NULL;
    }
    if (self->flags & POF_ENABLED) {
        PyObject* result = NULL;
        PyObject* monitoring = PyImport_ImportModuleAttrString("sys", "monitoring");

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

/*[clinic input]
_lsprof.Profiler.clear

Clear all profiling information collected so far.
[clinic start generated code]*/

static PyObject *
_lsprof_Profiler_clear_impl(ProfilerObject *self)
/*[clinic end generated code: output=dd1c668fb84b1335 input=fbe1f88c28be4f98]*/
{
    if (self->flags & POF_EXT_TIMER) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot clear profiler in external timer");
        return NULL;
    }
    clearEntries(self);
    Py_RETURN_NONE;
}

static int
profiler_traverse(PyObject *op, visitproc visit, void *arg)
{
    ProfilerObject *self = ProfilerObject_CAST(op);
    Py_VISIT(Py_TYPE(op));
    Py_VISIT(self->externalTimer);
    return 0;
}

static void
profiler_dealloc(PyObject *op)
{
    ProfilerObject *self = ProfilerObject_CAST(op);
    PyObject_GC_UnTrack(self);
    if (self->flags & POF_ENABLED) {
        PyThreadState *tstate = _PyThreadState_GET();
        if (_PyEval_SetProfile(tstate, NULL, NULL) < 0) {
            PyErr_FormatUnraisable("Exception ignored while "
                                   "destroying _lsprof profiler");
        }
    }

    flush_unmatched(self);
    clearEntries(self);
    Py_XDECREF(self->externalTimer);
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

/*[clinic input]
_lsprof.Profiler.__init__ as profiler_init

    timer: object(c_default='NULL') = None
    timeunit: double = 0.0
    subcalls: bool = True
    builtins: bool = True

Build a profiler object using the specified timer function.

The default timer is a fast built-in one based on real time.
For custom timer functions returning integers, 'timeunit' can
be a float specifying a scale (that is, how long each integer unit
is, in seconds).
[clinic start generated code]*/

static int
profiler_init_impl(ProfilerObject *self, PyObject *timer, double timeunit,
                   int subcalls, int builtins)
/*[clinic end generated code: output=ac523803ec9f9df2 input=8285ca746f96a414]*/
{
    if (setSubcalls(self, subcalls) < 0 || setBuiltins(self, builtins) < 0) {
        return -1;
    }
    self->externalTimerUnit = timeunit;
    Py_XSETREF(self->externalTimer, Py_XNewRef(timer));
    self->tool_id = PY_MONITORING_PROFILER_ID;

    PyObject* monitoring = PyImport_ImportModuleAttrString("sys", "monitoring");
    if (!monitoring) {
        return -1;
    }
    self->missing = PyObject_GetAttrString(monitoring, "MISSING");
    if (!self->missing) {
        Py_DECREF(monitoring);
        return -1;
    }
    Py_DECREF(monitoring);
    return 0;
}

static PyMethodDef profiler_methods[] = {
    _LSPROF_PROFILER_GETSTATS_METHODDEF
    _LSPROF_PROFILER_ENABLE_METHODDEF
    _LSPROF_PROFILER_DISABLE_METHODDEF
    _LSPROF_PROFILER_CLEAR_METHODDEF
    _LSPROF_PROFILER__PYSTART_CALLBACK_METHODDEF
    _LSPROF_PROFILER__PYRETURN_CALLBACK_METHODDEF
    _LSPROF_PROFILER__CCALL_CALLBACK_METHODDEF
    _LSPROF_PROFILER__CRETURN_CALLBACK_METHODDEF
    {NULL, NULL}
};

static PyType_Slot _lsprof_profiler_type_spec_slots[] = {
    {Py_tp_doc, (void *)profiler_init__doc__},
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
    (void)_lsprof_clear((PyObject *)module);
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
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
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
