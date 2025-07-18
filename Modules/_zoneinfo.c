#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_critical_section.h"  // _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED()
#include "pycore_long.h"          // _PyLong_GetOne()
#include "pycore_pyerrors.h"      // _PyErr_ChainExceptions1()
#include "pycore_typeobject.h"    // _PyType_GetModuleState()
#include "pycore_weakref.h"       // FT_CLEAR_WEAKREFS()

#include "datetime.h"             // PyDateTime_TZInfo

#include <stddef.h>               // offsetof()
#include <stdint.h>               // int64_t


#include "clinic/_zoneinfo.c.h"
/*[clinic input]
module zoneinfo
class zoneinfo.ZoneInfo "PyObject *" "PyTypeObject *"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=d12c73c0eef36df8]*/


typedef struct TransitionRuleType TransitionRuleType;
typedef struct StrongCacheNode StrongCacheNode;

typedef struct {
    PyObject *utcoff;
    PyObject *dstoff;
    PyObject *tzname;
    long utcoff_seconds;
} _ttinfo;

typedef struct {
    _ttinfo std;
    _ttinfo dst;
    int dst_diff;
    TransitionRuleType *start;
    TransitionRuleType *end;
    unsigned char std_only;
} _tzrule;

typedef struct {
    PyDateTime_TZInfo base;
    PyObject *key;
    PyObject *file_repr;
    PyObject *weakreflist;
    size_t num_transitions;
    size_t num_ttinfos;
    int64_t *trans_list_utc;
    int64_t *trans_list_wall[2];
    _ttinfo **trans_ttinfos;  // References to the ttinfo for each transition
    _ttinfo *ttinfo_before;
    _tzrule tzrule_after;
    _ttinfo *_ttinfos;  // Unique array of ttinfos for ease of deallocation
    unsigned char fixed_offset;
    unsigned char source;
} PyZoneInfo_ZoneInfo;

#define PyZoneInfo_ZoneInfo_CAST(op)    ((PyZoneInfo_ZoneInfo *)(op))

struct TransitionRuleType {
    int64_t (*year_to_timestamp)(TransitionRuleType *, int);
};

typedef struct {
    TransitionRuleType base;
    uint8_t month;      /* 1 - 12 */
    uint8_t week;       /* 1 - 5 */
    uint8_t day;        /* 0 - 6 */
    int16_t hour;       /* -167 - 167, RFC 8536 §3.3.1 */
    int8_t minute;      /* signed 2 digits */
    int8_t second;      /* signed 2 digits */
} CalendarRule;

typedef struct {
    TransitionRuleType base;
    uint8_t julian;     /* 0, 1 */
    uint16_t day;       /* 0 - 365 */
    int16_t hour;       /* -167 - 167, RFC 8536 §3.3.1 */
    int8_t minute;      /* signed 2 digits */
    int8_t second;      /* signed 2 digits */
} DayRule;

struct StrongCacheNode {
    StrongCacheNode *next;
    StrongCacheNode *prev;
    PyObject *key;
    PyObject *zone;
};

typedef struct {
    PyTypeObject *ZoneInfoType;

    // Imports
    PyObject *io_open;
    PyObject *_tzpath_find_tzfile;
    PyObject *_common_mod;

    // Caches
    PyObject *TIMEDELTA_CACHE;
    PyObject *ZONEINFO_WEAK_CACHE;
    StrongCacheNode *ZONEINFO_STRONG_CACHE;

    _ttinfo NO_TTINFO;
} zoneinfo_state;

// Constants
static const int EPOCHORDINAL = 719163;
static int DAYS_IN_MONTH[] = {
    -1, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
};

static int DAYS_BEFORE_MONTH[] = {
    -1, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,
};

static const int SOURCE_NOCACHE = 0;
static const int SOURCE_CACHE = 1;
static const int SOURCE_FILE = 2;

static const size_t ZONEINFO_STRONG_CACHE_MAX_SIZE = 8;

// Forward declarations
static int
load_data(zoneinfo_state *state, PyZoneInfo_ZoneInfo *self,
          PyObject *file_obj);
static void
utcoff_to_dstoff(size_t *trans_idx, long *utcoffs, long *dstoffs,
                 unsigned char *isdsts, size_t num_transitions,
                 size_t num_ttinfos);
static int
ts_to_local(size_t *trans_idx, int64_t *trans_utc, long *utcoff,
            int64_t *trans_local[2], size_t num_ttinfos,
            size_t num_transitions);

static int
parse_tz_str(zoneinfo_state *state, PyObject *tz_str_obj, _tzrule *out);

static int
parse_abbr(const char **p, PyObject **abbr);
static int
parse_tz_delta(const char **p, long *total_seconds);
static int
parse_transition_time(const char **p, int *hour, int *minute, int *second);
static int
parse_transition_rule(const char **p, TransitionRuleType **out);

static _ttinfo *
find_tzrule_ttinfo(_tzrule *rule, int64_t ts, unsigned char fold, int year);
static _ttinfo *
find_tzrule_ttinfo_fromutc(_tzrule *rule, int64_t ts, int year,
                           unsigned char *fold);

static int
build_ttinfo(zoneinfo_state *state, long utcoffset, long dstoffset,
             PyObject *tzname, _ttinfo *out);
static void
xdecref_ttinfo(_ttinfo *ttinfo);
static int
ttinfo_eq(const _ttinfo *const tti0, const _ttinfo *const tti1);

static int
build_tzrule(zoneinfo_state *state, PyObject *std_abbr, PyObject *dst_abbr,
             long std_offset, long dst_offset, TransitionRuleType *start,
             TransitionRuleType *end, _tzrule *out);
static void
free_tzrule(_tzrule *tzrule);

static PyObject *
load_timedelta(zoneinfo_state *state, long seconds);

static int
get_local_timestamp(PyObject *dt, int64_t *local_ts);
static _ttinfo *
find_ttinfo(zoneinfo_state *state, PyZoneInfo_ZoneInfo *self, PyObject *dt);

static int
ymd_to_ord(int y, int m, int d);
static int
is_leap_year(int year);

static size_t
_bisect(const int64_t value, const int64_t *arr, size_t size);

static int
eject_from_strong_cache(zoneinfo_state *state, const PyTypeObject *const type,
                        PyObject *key);
static void
clear_strong_cache(zoneinfo_state *state, const PyTypeObject *const type);
static void
update_strong_cache(zoneinfo_state *state, const PyTypeObject *const type,
                    PyObject *key, PyObject *zone);
static PyObject *
zone_from_strong_cache(zoneinfo_state *state, const PyTypeObject *const type,
                       PyObject *const key);

static inline zoneinfo_state *
zoneinfo_get_state(PyObject *mod)
{
    zoneinfo_state *state = (zoneinfo_state *)PyModule_GetState(mod);
    assert(state != NULL);
    return state;
}

static inline zoneinfo_state *
zoneinfo_get_state_by_cls(PyTypeObject *cls)
{
    zoneinfo_state *state = (zoneinfo_state *)_PyType_GetModuleState(cls);
    assert(state != NULL);
    return state;
}

static struct PyModuleDef zoneinfomodule;

static inline zoneinfo_state *
zoneinfo_get_state_by_self(PyTypeObject *self)
{
    PyObject *mod = PyType_GetModuleByDef(self, &zoneinfomodule);
    assert(mod != NULL);
    return zoneinfo_get_state(mod);
}

static PyObject *
zoneinfo_new_instance(zoneinfo_state *state, PyTypeObject *type, PyObject *key)
{
    PyObject *file_obj = NULL;
    PyObject *file_path = NULL;

    file_path = PyObject_CallFunctionObjArgs(state->_tzpath_find_tzfile,
                                             key, NULL);
    if (file_path == NULL) {
        return NULL;
    }
    else if (file_path == Py_None) {
        PyObject *meth = state->_common_mod;
        file_obj = PyObject_CallMethod(meth, "load_tzdata", "O", key);
        if (file_obj == NULL) {
            Py_DECREF(file_path);
            return NULL;
        }
    }

    PyObject *self = type->tp_alloc(type, 0);
    if (self == NULL) {
        goto error;
    }

    if (file_obj == NULL) {
        PyObject *func = state->io_open;
        file_obj = PyObject_CallFunction(func, "Os", file_path, "rb");
        if (file_obj == NULL) {
            goto error;
        }
    }

    PyZoneInfo_ZoneInfo *self_zinfo = (PyZoneInfo_ZoneInfo *)self;
    if (load_data(state, self_zinfo, file_obj)) {
        goto error;
    }

    PyObject *rv = PyObject_CallMethod(file_obj, "close", NULL);
    Py_SETREF(file_obj, NULL);
    if (rv == NULL) {
        goto error;
    }
    Py_DECREF(rv);

    self_zinfo->key = Py_NewRef(key);

    goto cleanup;
error:
    Py_CLEAR(self);
cleanup:
    if (file_obj != NULL) {
        PyObject *exc = PyErr_GetRaisedException();
        PyObject *tmp = PyObject_CallMethod(file_obj, "close", NULL);
        _PyErr_ChainExceptions1(exc);
        if (tmp == NULL) {
            Py_CLEAR(self);
        }
        Py_XDECREF(tmp);
        Py_DECREF(file_obj);
    }
    Py_DECREF(file_path);
    return self;
}

static PyObject *
get_weak_cache(zoneinfo_state *state, PyTypeObject *type)
{
    if (type == state->ZoneInfoType) {
        return state->ZONEINFO_WEAK_CACHE;
    }
    else {
        PyObject *cache =
            PyObject_GetAttrString((PyObject *)type, "_weak_cache");
        // We are assuming that the type lives at least as long as the function
        // that calls get_weak_cache, and that it holds a reference to the
        // cache, so we'll return a "borrowed reference".
        Py_XDECREF(cache);
        return cache;
    }
}

/*[clinic input]
@critical_section
@classmethod
zoneinfo.ZoneInfo.__new__

    key: object

Create a new ZoneInfo instance.
[clinic start generated code]*/

static PyObject *
zoneinfo_ZoneInfo_impl(PyTypeObject *type, PyObject *key)
/*[clinic end generated code: output=95e61dab86bb95c3 input=ef73d7a83bf8790e]*/
{
    zoneinfo_state *state = zoneinfo_get_state_by_self(type);
    PyObject *instance = zone_from_strong_cache(state, type, key);
    if (instance != NULL || PyErr_Occurred()) {
        return instance;
    }

    PyObject *weak_cache = get_weak_cache(state, type);
    instance = PyObject_CallMethod(weak_cache, "get", "O", key, Py_None);
    if (instance == NULL) {
        return NULL;
    }

    if (instance == Py_None) {
        Py_DECREF(instance);
        PyObject *tmp = zoneinfo_new_instance(state, type, key);
        if (tmp == NULL) {
            return NULL;
        }

        instance =
            PyObject_CallMethod(weak_cache, "setdefault", "OO", key, tmp);
        Py_DECREF(tmp);
        if (instance == NULL) {
            return NULL;
        }
        ((PyZoneInfo_ZoneInfo *)instance)->source = SOURCE_CACHE;
    }

    update_strong_cache(state, type, key, instance);
    return instance;
}

static int
zoneinfo_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyZoneInfo_ZoneInfo *self = PyZoneInfo_ZoneInfo_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->key);
    return 0;
}

static int
zoneinfo_clear(PyObject *op)
{
    PyZoneInfo_ZoneInfo *self = PyZoneInfo_ZoneInfo_CAST(op);
    Py_CLEAR(self->key);
    Py_CLEAR(self->file_repr);
    return 0;
}

static void
zoneinfo_dealloc(PyObject *obj_self)
{
    PyZoneInfo_ZoneInfo *self = PyZoneInfo_ZoneInfo_CAST(obj_self);
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);

    FT_CLEAR_WEAKREFS(obj_self, self->weakreflist);

    if (self->trans_list_utc != NULL) {
        PyMem_Free(self->trans_list_utc);
    }

    for (size_t i = 0; i < 2; i++) {
        if (self->trans_list_wall[i] != NULL) {
            PyMem_Free(self->trans_list_wall[i]);
        }
    }

    if (self->_ttinfos != NULL) {
        for (size_t i = 0; i < self->num_ttinfos; ++i) {
            xdecref_ttinfo(&(self->_ttinfos[i]));
        }
        PyMem_Free(self->_ttinfos);
    }

    if (self->trans_ttinfos != NULL) {
        PyMem_Free(self->trans_ttinfos);
    }

    free_tzrule(&(self->tzrule_after));

    (void)zoneinfo_clear(obj_self);
    tp->tp_free(obj_self);
    Py_DECREF(tp);
}

/*[clinic input]
@classmethod
zoneinfo.ZoneInfo.from_file

    cls: defining_class
    file_obj: object
    /
    key: object = None

Create a ZoneInfo file from a file object.
[clinic start generated code]*/

static PyObject *
zoneinfo_ZoneInfo_from_file_impl(PyTypeObject *type, PyTypeObject *cls,
                                 PyObject *file_obj, PyObject *key)
/*[clinic end generated code: output=77887d1d56a48324 input=d26111f29eed6863]*/
{
    PyObject *file_repr = NULL;
    PyZoneInfo_ZoneInfo *self = NULL;

    self = (PyZoneInfo_ZoneInfo *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    file_repr = PyObject_Repr(file_obj);
    if (file_repr == NULL) {
        goto error;
    }

    zoneinfo_state *state = zoneinfo_get_state_by_cls(cls);
    if (load_data(state, self, file_obj)) {
        goto error;
    }

    self->source = SOURCE_FILE;
    self->file_repr = file_repr;
    self->key = Py_NewRef(key);
    return (PyObject *)self;

error:
    Py_XDECREF(file_repr);
    Py_XDECREF(self);
    return NULL;
}

/*[clinic input]
@classmethod
zoneinfo.ZoneInfo.no_cache

    cls: defining_class
    /
    key: object

Get a new instance of ZoneInfo, bypassing the cache.
[clinic start generated code]*/

static PyObject *
zoneinfo_ZoneInfo_no_cache_impl(PyTypeObject *type, PyTypeObject *cls,
                                PyObject *key)
/*[clinic end generated code: output=b0b09b3344c171b7 input=0238f3d56b1ea3f1]*/
{
    zoneinfo_state *state = zoneinfo_get_state_by_cls(cls);
    PyObject *out = zoneinfo_new_instance(state, type, key);
    if (out != NULL) {
        PyZoneInfo_ZoneInfo_CAST(out)->source = SOURCE_NOCACHE;
    }

    return out;
}

/*[clinic input]
@critical_section
@classmethod
zoneinfo.ZoneInfo.clear_cache

    cls: defining_class
    /
    *
    only_keys: object = None

Clear the ZoneInfo cache.
[clinic start generated code]*/

static PyObject *
zoneinfo_ZoneInfo_clear_cache_impl(PyTypeObject *type, PyTypeObject *cls,
                                   PyObject *only_keys)
/*[clinic end generated code: output=114d9b7c8a22e660 input=35944715df26d24e]*/
{
    zoneinfo_state *state = zoneinfo_get_state_by_cls(cls);
    PyObject *weak_cache = get_weak_cache(state, type);

    if (only_keys == NULL || only_keys == Py_None) {
        PyObject *rv = PyObject_CallMethod(weak_cache, "clear", NULL);
        if (rv != NULL) {
            Py_DECREF(rv);
        }

        clear_strong_cache(state, type);
    }
    else {
        PyObject *item = NULL;
        PyObject *pop = PyUnicode_FromString("pop");
        if (pop == NULL) {
            return NULL;
        }

        PyObject *iter = PyObject_GetIter(only_keys);
        if (iter == NULL) {
            Py_DECREF(pop);
            return NULL;
        }

        while ((item = PyIter_Next(iter))) {
            // Remove from strong cache
            if (eject_from_strong_cache(state, type, item) < 0) {
                Py_DECREF(item);
                break;
            }

            // Remove from weak cache
            PyObject *tmp = PyObject_CallMethodObjArgs(weak_cache, pop, item,
                                                       Py_None, NULL);

            Py_DECREF(item);
            if (tmp == NULL) {
                break;
            }
            Py_DECREF(tmp);
        }
        Py_DECREF(iter);
        Py_DECREF(pop);
    }

    if (PyErr_Occurred()) {
        return NULL;
    }

    Py_RETURN_NONE;
}

/*[clinic input]
zoneinfo.ZoneInfo.utcoffset

    cls: defining_class
    dt: object
    /

Retrieve a timedelta representing the UTC offset in a zone at the given datetime.
[clinic start generated code]*/

static PyObject *
zoneinfo_ZoneInfo_utcoffset_impl(PyObject *self, PyTypeObject *cls,
                                 PyObject *dt)
/*[clinic end generated code: output=b71016c319ba1f91 input=2bb6c5364938f19c]*/
{
    zoneinfo_state *state = zoneinfo_get_state_by_cls(cls);
    _ttinfo *tti = find_ttinfo(state, PyZoneInfo_ZoneInfo_CAST(self), dt);
    if (tti == NULL) {
        return NULL;
    }
    return Py_NewRef(tti->utcoff);
}

/*[clinic input]
zoneinfo.ZoneInfo.dst

    cls: defining_class
    dt: object
    /

Retrieve a timedelta representing the amount of DST applied in a zone at the given datetime.
[clinic start generated code]*/

static PyObject *
zoneinfo_ZoneInfo_dst_impl(PyObject *self, PyTypeObject *cls, PyObject *dt)
/*[clinic end generated code: output=cb6168d7723a6ae6 input=2167fb80cf8645c6]*/
{
    zoneinfo_state *state = zoneinfo_get_state_by_cls(cls);
    _ttinfo *tti = find_ttinfo(state, PyZoneInfo_ZoneInfo_CAST(self), dt);
    if (tti == NULL) {
        return NULL;
    }
    return Py_NewRef(tti->dstoff);
}

/*[clinic input]
zoneinfo.ZoneInfo.tzname

    cls: defining_class
    dt: object
    /

Retrieve a string containing the abbreviation for the time zone that applies in a zone at a given datetime.
[clinic start generated code]*/

static PyObject *
zoneinfo_ZoneInfo_tzname_impl(PyObject *self, PyTypeObject *cls,
                              PyObject *dt)
/*[clinic end generated code: output=3b6ae6c3053ea75a input=15a59a4f92ed1f1f]*/
{
    zoneinfo_state *state = zoneinfo_get_state_by_cls(cls);
    _ttinfo *tti = find_ttinfo(state, PyZoneInfo_ZoneInfo_CAST(self), dt);
    if (tti == NULL) {
        return NULL;
    }
    return Py_NewRef(tti->tzname);
}

#define GET_DT_TZINFO PyDateTime_DATE_GET_TZINFO

static PyObject *
zoneinfo_fromutc(PyObject *obj_self, PyObject *dt)
{
    if (!PyDateTime_Check(dt)) {
        PyErr_SetString(PyExc_TypeError,
                        "fromutc: argument must be a datetime");
        return NULL;
    }
    if (GET_DT_TZINFO(dt) != obj_self) {
        PyErr_SetString(PyExc_ValueError,
                        "fromutc: dt.tzinfo "
                        "is not self");
        return NULL;
    }

    PyZoneInfo_ZoneInfo *self = PyZoneInfo_ZoneInfo_CAST(obj_self);

    int64_t timestamp;
    if (get_local_timestamp(dt, &timestamp)) {
        return NULL;
    }
    size_t num_trans = self->num_transitions;

    _ttinfo *tti = NULL;
    unsigned char fold = 0;

    if (num_trans >= 1 && timestamp < self->trans_list_utc[0]) {
        tti = self->ttinfo_before;
    }
    else if (num_trans == 0 ||
             timestamp > self->trans_list_utc[num_trans - 1]) {
        tti = find_tzrule_ttinfo_fromutc(&(self->tzrule_after), timestamp,
                                         PyDateTime_GET_YEAR(dt), &fold);

        // Immediately after the last manual transition, the fold/gap is
        // between self->trans_ttinfos[num_transitions - 1] and whatever
        // ttinfo applies immediately after the last transition, not between
        // the STD and DST rules in the tzrule_after, so we may need to
        // adjust the fold value.
        if (num_trans) {
            _ttinfo *tti_prev = NULL;
            if (num_trans == 1) {
                tti_prev = self->ttinfo_before;
            }
            else {
                tti_prev = self->trans_ttinfos[num_trans - 2];
            }
            int64_t diff = tti_prev->utcoff_seconds - tti->utcoff_seconds;
            if (diff > 0 &&
                timestamp < (self->trans_list_utc[num_trans - 1] + diff)) {
                fold = 1;
            }
        }
    }
    else {
        size_t idx = _bisect(timestamp, self->trans_list_utc, num_trans);
        _ttinfo *tti_prev = NULL;

        if (idx >= 2) {
            tti_prev = self->trans_ttinfos[idx - 2];
            tti = self->trans_ttinfos[idx - 1];
        }
        else {
            tti_prev = self->ttinfo_before;
            tti = self->trans_ttinfos[0];
        }

        // Detect fold
        int64_t shift =
            (int64_t)(tti_prev->utcoff_seconds - tti->utcoff_seconds);
        if (shift > (timestamp - self->trans_list_utc[idx - 1])) {
            fold = 1;
        }
    }

    PyObject *tmp = PyNumber_Add(dt, tti->utcoff);
    if (tmp == NULL) {
        return NULL;
    }

    if (fold) {
        if (PyDateTime_CheckExact(tmp)) {
            ((PyDateTime_DateTime *)tmp)->fold = 1;
            dt = tmp;
        }
        else {
            PyObject *replace = PyObject_GetAttrString(tmp, "replace");
            Py_DECREF(tmp);
            if (replace == NULL) {
                return NULL;
            }
            PyObject *args = PyTuple_New(0);
            if (args == NULL) {
                Py_DECREF(replace);
                return NULL;
            }
            PyObject *kwargs = PyDict_New();
            if (kwargs == NULL) {
                Py_DECREF(replace);
                Py_DECREF(args);
                return NULL;
            }

            dt = NULL;
            if (!PyDict_SetItemString(kwargs, "fold", _PyLong_GetOne())) {
                dt = PyObject_Call(replace, args, kwargs);
            }

            Py_DECREF(args);
            Py_DECREF(kwargs);
            Py_DECREF(replace);

            if (dt == NULL) {
                return NULL;
            }
        }
    }
    else {
        dt = tmp;
    }
    return dt;
}

static PyObject *
zoneinfo_repr(PyObject *op)
{
    PyZoneInfo_ZoneInfo *self = PyZoneInfo_ZoneInfo_CAST(op);
    if (self->key != Py_None) {
        return PyUnicode_FromFormat("%T(key=%R)", self, self->key);
    }
    assert(PyUnicode_Check(self->file_repr));
    return PyUnicode_FromFormat("%T.from_file(%U)", self, self->file_repr);
}

static PyObject *
zoneinfo_str(PyObject *op)
{
    PyZoneInfo_ZoneInfo *self = PyZoneInfo_ZoneInfo_CAST(op);
    if (self->key != Py_None) {
        return Py_NewRef(self->key);
    }
    return zoneinfo_repr(op);
}

/* Pickles the ZoneInfo object by key and source.
 *
 * ZoneInfo objects are pickled by reference to the TZif file that they came
 * from, which means that the exact transitions may be different or the file
 * may not un-pickle if the data has changed on disk in the interim.
 *
 * It is necessary to include a bit indicating whether or not the object
 * was constructed from the cache, because from-cache objects will hit the
 * unpickling process's cache, whereas no-cache objects will bypass it.
 *
 * Objects constructed from ZoneInfo.from_file cannot be pickled.
 */
static PyObject *
zoneinfo_reduce(PyObject *obj_self, PyObject *Py_UNUSED(dummy))
{
    PyZoneInfo_ZoneInfo *self = PyZoneInfo_ZoneInfo_CAST(obj_self);
    if (self->source == SOURCE_FILE) {
        // Objects constructed from files cannot be pickled.
        PyObject *pickle_error =
            PyImport_ImportModuleAttrString("pickle", "PicklingError");
        if (pickle_error == NULL) {
            return NULL;
        }

        PyErr_Format(pickle_error,
                     "Cannot pickle a ZoneInfo file from a file stream.");
        Py_DECREF(pickle_error);
        return NULL;
    }

    unsigned char from_cache = self->source == SOURCE_CACHE ? 1 : 0;
    PyObject *constructor = PyObject_GetAttrString(obj_self, "_unpickle");

    if (constructor == NULL) {
        return NULL;
    }

    PyObject *rv = Py_BuildValue("O(OB)", constructor, self->key, from_cache);
    Py_DECREF(constructor);
    return rv;
}

/*[clinic input]
@classmethod
zoneinfo.ZoneInfo._unpickle

    cls: defining_class
    key: object
    from_cache: unsigned_char(bitwise=True)
    /

Private method used in unpickling.
[clinic start generated code]*/

static PyObject *
zoneinfo_ZoneInfo__unpickle_impl(PyTypeObject *type, PyTypeObject *cls,
                                 PyObject *key, unsigned char from_cache)
/*[clinic end generated code: output=556712fc709deecb input=6ac8c73eed3de316]*/
{
    if (from_cache) {
        PyObject *rv;
        Py_BEGIN_CRITICAL_SECTION(type);
        rv = zoneinfo_ZoneInfo_impl(type, key);
        Py_END_CRITICAL_SECTION();
        return rv;
    }
    else {
        zoneinfo_state *state = zoneinfo_get_state_by_cls(cls);
        return zoneinfo_new_instance(state, type, key);
    }
}

/* It is relatively expensive to construct new timedelta objects, and in most
 * cases we're looking at a relatively small number of timedeltas, such as
 * integer number of hours, etc. We will keep a cache so that we construct
 * a minimal number of these.
 *
 * Possibly this should be replaced with an LRU cache so that it's not possible
 * for the memory usage to explode from this, but in order for this to be a
 * serious problem, one would need to deliberately craft a malicious time zone
 * file with many distinct offsets. As of tzdb 2019c, loading every single zone
 * fills the cache with ~450 timedeltas for a total size of ~12kB.
 *
 * This returns a new reference to the timedelta.
 */
static PyObject *
load_timedelta(zoneinfo_state *state, long seconds)
{
    PyObject *rv;
    PyObject *pyoffset = PyLong_FromLong(seconds);
    if (pyoffset == NULL) {
        return NULL;
    }
    if (PyDict_GetItemRef(state->TIMEDELTA_CACHE, pyoffset, &rv) == 0) {
        PyObject *tmp = PyDateTimeAPI->Delta_FromDelta(
            0, seconds, 0, 1, PyDateTimeAPI->DeltaType);

        if (tmp != NULL) {
            PyDict_SetDefaultRef(state->TIMEDELTA_CACHE, pyoffset, tmp, &rv);
            Py_DECREF(tmp);
        }
    }

    Py_DECREF(pyoffset);
    return rv;
}

/* Constructor for _ttinfo object - this starts by initializing the _ttinfo
 * to { NULL, NULL, NULL }, so that Py_XDECREF will work on partially
 * initialized _ttinfo objects.
 */
static int
build_ttinfo(zoneinfo_state *state, long utcoffset, long dstoffset,
             PyObject *tzname, _ttinfo *out)
{
    out->utcoff = NULL;
    out->dstoff = NULL;
    out->tzname = NULL;

    out->utcoff_seconds = utcoffset;
    out->utcoff = load_timedelta(state, utcoffset);
    if (out->utcoff == NULL) {
        return -1;
    }

    out->dstoff = load_timedelta(state, dstoffset);
    if (out->dstoff == NULL) {
        return -1;
    }

    out->tzname = Py_NewRef(tzname);

    return 0;
}

/* Decrease reference count on any non-NULL members of a _ttinfo  */
static void
xdecref_ttinfo(_ttinfo *ttinfo)
{
    if (ttinfo != NULL) {
        Py_XDECREF(ttinfo->utcoff);
        Py_XDECREF(ttinfo->dstoff);
        Py_XDECREF(ttinfo->tzname);
    }
}

/* Equality function for _ttinfo. */
static int
ttinfo_eq(const _ttinfo *const tti0, const _ttinfo *const tti1)
{
    int rv;
    if ((rv = PyObject_RichCompareBool(tti0->utcoff, tti1->utcoff, Py_EQ)) <
        1) {
        goto end;
    }

    if ((rv = PyObject_RichCompareBool(tti0->dstoff, tti1->dstoff, Py_EQ)) <
        1) {
        goto end;
    }

    if ((rv = PyObject_RichCompareBool(tti0->tzname, tti1->tzname, Py_EQ)) <
        1) {
        goto end;
    }
end:
    return rv;
}

/* Given a file-like object, this populates a ZoneInfo object
 *
 * The current version calls into a Python function to read the data from
 * file into Python objects, and this translates those Python objects into
 * C values and calculates derived values (e.g. dstoff) in C.
 *
 * This returns 0 on success and -1 on failure.
 *
 * The function will never return while `self` is partially initialized —
 * the object only needs to be freed / deallocated if this succeeds.
 */
static int
load_data(zoneinfo_state *state, PyZoneInfo_ZoneInfo *self, PyObject *file_obj)
{
    int rv = 0;
    PyObject *data_tuple = NULL;

    long *utcoff = NULL;
    long *dstoff = NULL;
    size_t *trans_idx = NULL;
    unsigned char *isdst = NULL;

    self->trans_list_utc = NULL;
    self->trans_list_wall[0] = NULL;
    self->trans_list_wall[1] = NULL;
    self->trans_ttinfos = NULL;
    self->_ttinfos = NULL;
    self->file_repr = NULL;

    size_t ttinfos_allocated = 0;

    data_tuple = PyObject_CallMethod(state->_common_mod, "load_data", "O",
                                     file_obj);

    if (data_tuple == NULL) {
        goto error;
    }

    if (!PyTuple_CheckExact(data_tuple)) {
        PyErr_Format(PyExc_TypeError, "Invalid data result type: %r",
                     data_tuple);
        goto error;
    }

    // Unpack the data tuple
    PyObject *trans_idx_list = PyTuple_GetItem(data_tuple, 0);
    if (trans_idx_list == NULL) {
        goto error;
    }

    PyObject *trans_utc = PyTuple_GetItem(data_tuple, 1);
    if (trans_utc == NULL) {
        goto error;
    }

    PyObject *utcoff_list = PyTuple_GetItem(data_tuple, 2);
    if (utcoff_list == NULL) {
        goto error;
    }

    PyObject *isdst_list = PyTuple_GetItem(data_tuple, 3);
    if (isdst_list == NULL) {
        goto error;
    }

    PyObject *abbr = PyTuple_GetItem(data_tuple, 4);
    if (abbr == NULL) {
        goto error;
    }

    PyObject *tz_str = PyTuple_GetItem(data_tuple, 5);
    if (tz_str == NULL) {
        goto error;
    }

    // Load the relevant sizes
    Py_ssize_t num_transitions = PyTuple_Size(trans_utc);
    if (num_transitions < 0) {
        goto error;
    }

    Py_ssize_t num_ttinfos = PyTuple_Size(utcoff_list);
    if (num_ttinfos < 0) {
        goto error;
    }

    self->num_transitions = (size_t)num_transitions;
    self->num_ttinfos = (size_t)num_ttinfos;

    // Load the transition indices and list
    self->trans_list_utc =
        PyMem_Malloc(self->num_transitions * sizeof(int64_t));
    if (self->trans_list_utc == NULL) {
        goto error;
    }
    trans_idx = PyMem_Malloc(self->num_transitions * sizeof(Py_ssize_t));
    if (trans_idx == NULL) {
        goto error;
    }

    for (size_t i = 0; i < self->num_transitions; ++i) {
        PyObject *num = PyTuple_GetItem(trans_utc, i);
        if (num == NULL) {
            goto error;
        }
        self->trans_list_utc[i] = PyLong_AsLongLong(num);
        if (self->trans_list_utc[i] == -1 && PyErr_Occurred()) {
            goto error;
        }

        num = PyTuple_GetItem(trans_idx_list, i);
        if (num == NULL) {
            goto error;
        }

        Py_ssize_t cur_trans_idx = PyLong_AsSsize_t(num);
        if (cur_trans_idx == -1) {
            goto error;
        }

        trans_idx[i] = (size_t)cur_trans_idx;
        if (trans_idx[i] > self->num_ttinfos) {
            PyErr_Format(
                PyExc_ValueError,
                "Invalid transition index found while reading TZif: %zd",
                cur_trans_idx);

            goto error;
        }
    }

    // Load UTC offsets and isdst (size num_ttinfos)
    utcoff = PyMem_Malloc(self->num_ttinfos * sizeof(long));
    isdst = PyMem_Malloc(self->num_ttinfos * sizeof(unsigned char));

    if (utcoff == NULL || isdst == NULL) {
        goto error;
    }
    for (size_t i = 0; i < self->num_ttinfos; ++i) {
        PyObject *num = PyTuple_GetItem(utcoff_list, i);
        if (num == NULL) {
            goto error;
        }

        utcoff[i] = PyLong_AsLong(num);
        if (utcoff[i] == -1 && PyErr_Occurred()) {
            goto error;
        }

        num = PyTuple_GetItem(isdst_list, i);
        if (num == NULL) {
            goto error;
        }

        int isdst_with_error = PyObject_IsTrue(num);
        if (isdst_with_error == -1) {
            goto error;
        }
        else {
            isdst[i] = (unsigned char)isdst_with_error;
        }
    }

    dstoff = PyMem_Calloc(self->num_ttinfos, sizeof(long));
    if (dstoff == NULL) {
        goto error;
    }

    // Derive dstoff and trans_list_wall from the information we've loaded
    utcoff_to_dstoff(trans_idx, utcoff, dstoff, isdst, self->num_transitions,
                     self->num_ttinfos);

    if (ts_to_local(trans_idx, self->trans_list_utc, utcoff,
                    self->trans_list_wall, self->num_ttinfos,
                    self->num_transitions)) {
        goto error;
    }

    // Build _ttinfo objects from utcoff, dstoff and abbr
    self->_ttinfos = PyMem_Malloc(self->num_ttinfos * sizeof(_ttinfo));
    if (self->_ttinfos == NULL) {
        goto error;
    }
    for (size_t i = 0; i < self->num_ttinfos; ++i) {
        PyObject *tzname = PyTuple_GetItem(abbr, i);
        if (tzname == NULL) {
            goto error;
        }

        ttinfos_allocated++;
        int rc = build_ttinfo(state, utcoff[i], dstoff[i], tzname,
                              &(self->_ttinfos[i]));
        if (rc) {
            goto error;
        }
    }

    // Build our mapping from transition to the ttinfo that applies
    self->trans_ttinfos =
        PyMem_Calloc(self->num_transitions, sizeof(_ttinfo *));
    if (self->trans_ttinfos == NULL) {
        goto error;
    }
    for (size_t i = 0; i < self->num_transitions; ++i) {
        size_t ttinfo_idx = trans_idx[i];
        assert(ttinfo_idx < self->num_ttinfos);
        self->trans_ttinfos[i] = &(self->_ttinfos[ttinfo_idx]);
    }

    // Set ttinfo_before to the first non-DST transition
    for (size_t i = 0; i < self->num_ttinfos; ++i) {
        if (!isdst[i]) {
            self->ttinfo_before = &(self->_ttinfos[i]);
            break;
        }
    }

    // If there are only DST ttinfos, pick the first one, if there are no
    // ttinfos at all, set ttinfo_before to NULL
    if (self->ttinfo_before == NULL && self->num_ttinfos > 0) {
        self->ttinfo_before = &(self->_ttinfos[0]);
    }

    if (tz_str != Py_None && PyObject_IsTrue(tz_str)) {
        if (parse_tz_str(state, tz_str, &(self->tzrule_after))) {
            goto error;
        }
    }
    else {
        if (!self->num_ttinfos) {
            PyErr_Format(PyExc_ValueError, "No time zone information found.");
            goto error;
        }

        size_t idx;
        if (!self->num_transitions) {
            idx = self->num_ttinfos - 1;
        }
        else {
            idx = trans_idx[self->num_transitions - 1];
        }

        _ttinfo *tti = &(self->_ttinfos[idx]);
        build_tzrule(state, tti->tzname, NULL, tti->utcoff_seconds, 0, NULL,
                     NULL, &(self->tzrule_after));

        // We've abused the build_tzrule constructor to construct an STD-only
        // rule mimicking whatever ttinfo we've picked up, but it's possible
        // that the one we've picked up is a DST zone, so we need to make sure
        // that the dstoff is set correctly in that case.
        if (PyObject_IsTrue(tti->dstoff)) {
            _ttinfo *tti_after = &(self->tzrule_after.std);
            Py_SETREF(tti_after->dstoff, Py_NewRef(tti->dstoff));
        }
    }

    // Determine if this is a "fixed offset" zone, meaning that the output of
    // the utcoffset, dst and tzname functions does not depend on the specific
    // datetime passed.
    //
    // We make three simplifying assumptions here:
    //
    // 1. If tzrule_after is not std_only, it has transitions that might occur
    //    (it is possible to construct TZ strings that specify STD and DST but
    //    no transitions ever occur, such as AAA0BBB,0/0,J365/25).
    // 2. If self->_ttinfos contains more than one _ttinfo object, the objects
    //    represent different offsets.
    // 3. self->ttinfos contains no unused _ttinfos (in which case an otherwise
    //    fixed-offset zone with extra _ttinfos defined may appear to *not* be
    //    a fixed offset zone).
    //
    // Violations to these assumptions would be fairly exotic, and exotic
    // zones should almost certainly not be used with datetime.time (the
    // only thing that would be affected by this).
    if (self->num_ttinfos > 1 || !self->tzrule_after.std_only) {
        self->fixed_offset = 0;
    }
    else if (self->num_ttinfos == 0) {
        self->fixed_offset = 1;
    }
    else {
        int constant_offset =
            ttinfo_eq(&(self->_ttinfos[0]), &self->tzrule_after.std);
        if (constant_offset < 0) {
            goto error;
        }
        else {
            self->fixed_offset = constant_offset;
        }
    }

    goto cleanup;
error:
    // These resources only need to be freed if we have failed, if we succeed
    // in initializing a PyZoneInfo_ZoneInfo object, we can rely on its dealloc
    // method to free the relevant resources.
    if (self->trans_list_utc != NULL) {
        PyMem_Free(self->trans_list_utc);
        self->trans_list_utc = NULL;
    }

    for (size_t i = 0; i < 2; ++i) {
        if (self->trans_list_wall[i] != NULL) {
            PyMem_Free(self->trans_list_wall[i]);
            self->trans_list_wall[i] = NULL;
        }
    }

    if (self->_ttinfos != NULL) {
        for (size_t i = 0; i < ttinfos_allocated; ++i) {
            xdecref_ttinfo(&(self->_ttinfos[i]));
        }
        PyMem_Free(self->_ttinfos);
        self->_ttinfos = NULL;
    }

    if (self->trans_ttinfos != NULL) {
        PyMem_Free(self->trans_ttinfos);
        self->trans_ttinfos = NULL;
    }

    rv = -1;
cleanup:
    Py_XDECREF(data_tuple);

    if (utcoff != NULL) {
        PyMem_Free(utcoff);
    }

    if (dstoff != NULL) {
        PyMem_Free(dstoff);
    }

    if (isdst != NULL) {
        PyMem_Free(isdst);
    }

    if (trans_idx != NULL) {
        PyMem_Free(trans_idx);
    }

    return rv;
}

/* Function to calculate the local timestamp of a transition from the year. */
int64_t
calendarrule_year_to_timestamp(TransitionRuleType *base_self, int year)
{
    CalendarRule *self = (CalendarRule *)base_self;

    // We want (year, month, day of month); we have year and month, but we
    // need to turn (week, day-of-week) into day-of-month
    //
    // Week 1 is the first week in which day `day` (where 0 = Sunday) appears.
    // Week 5 represents the last occurrence of day `day`, so we need to know
    // the first weekday of the month and the number of days in the month.
    int8_t first_day = (ymd_to_ord(year, self->month, 1) + 6) % 7;
    uint8_t days_in_month = DAYS_IN_MONTH[self->month];
    if (self->month == 2 && is_leap_year(year)) {
        days_in_month += 1;
    }

    // This equation seems magical, so I'll break it down:
    // 1. calendar says 0 = Monday, POSIX says 0 = Sunday so we need first_day
    //    + 1 to get 1 = Monday -> 7 = Sunday, which is still equivalent
    //    because this math is mod 7
    // 2. Get first day - desired day mod 7 (adjusting by 7 for negative
    //    numbers so that -1 % 7 = 6).
    // 3. Add 1 because month days are a 1-based index.
    int8_t month_day = ((int8_t)(self->day) - (first_day + 1)) % 7;
    if (month_day < 0) {
        month_day += 7;
    }
    month_day += 1;

    // Now use a 0-based index version of `week` to calculate the w-th
    // occurrence of `day`
    month_day += ((int8_t)(self->week) - 1) * 7;

    // month_day will only be > days_in_month if w was 5, and `w` means "last
    // occurrence of `d`", so now we just check if we over-shot the end of the
    // month and if so knock off 1 week.
    if (month_day > days_in_month) {
        month_day -= 7;
    }

    int64_t ordinal = ymd_to_ord(year, self->month, month_day) - EPOCHORDINAL;
    return ordinal * 86400 + (int64_t)self->hour * 3600 +
            (int64_t)self->minute * 60 + self->second;
}

/* Constructor for CalendarRule. */
int
calendarrule_new(int month, int week, int day, int hour,
                 int minute, int second, CalendarRule *out)
{
    // These bounds come from the POSIX standard, which describes an Mm.n.d
    // rule as:
    //
    //   The d'th day (0 <= d <= 6) of week n of month m of the year (1 <= n <=
    //   5, 1 <= m <= 12, where week 5 means "the last d day in month m" which
    //   may occur in either the fourth or the fifth week). Week 1 is the first
    //   week in which the d'th day occurs. Day zero is Sunday.
    if (month < 1 || month > 12) {
        PyErr_Format(PyExc_ValueError, "Month must be in [1, 12]");
        return -1;
    }

    if (week < 1 || week > 5) {
        PyErr_Format(PyExc_ValueError, "Week must be in [1, 5]");
        return -1;
    }

    if (day < 0 || day > 6) {
        PyErr_Format(PyExc_ValueError, "Day must be in [0, 6]");
        return -1;
    }

    if (hour < -167 || hour > 167) {
        PyErr_Format(PyExc_ValueError, "Hour must be in [0, 167]");
        return -1;
    }

    TransitionRuleType base = {&calendarrule_year_to_timestamp};

    CalendarRule new_offset = {
        .base = base,
        .month = (uint8_t)month,
        .week = (uint8_t)week,
        .day = (uint8_t)day,
        .hour = (int16_t)hour,
        .minute = (int8_t)minute,
        .second = (int8_t)second,
    };

    *out = new_offset;
    return 0;
}

/* Function to calculate the local timestamp of a transition from the year.
 *
 * This translates the day of the year into a local timestamp — either a
 * 1-based Julian day, not including leap days, or the 0-based year-day,
 * including leap days.
 * */
int64_t
dayrule_year_to_timestamp(TransitionRuleType *base_self, int year)
{
    // The function signature requires a TransitionRuleType pointer, but this
    // function is only applicable to DayRule* objects.
    DayRule *self = (DayRule *)base_self;

    // ymd_to_ord calculates the number of days since 0001-01-01, but we want
    // to know the number of days since 1970-01-01, so we must subtract off
    // the equivalent of ymd_to_ord(1970, 1, 1).
    //
    // We subtract off an additional 1 day to account for January 1st (we want
    // the number of full days *before* the date of the transition - partial
    // days are accounted for in the hour, minute and second portions.
    int64_t days_before_year = ymd_to_ord(year, 1, 1) - EPOCHORDINAL - 1;

    // The Julian day specification skips over February 29th in leap years,
    // from the POSIX standard:
    //
    //   Leap days shall not be counted. That is, in all years-including leap
    //   years-February 28 is day 59 and March 1 is day 60. It is impossible to
    //   refer explicitly to the occasional February 29.
    //
    // This is actually more useful than you'd think — if you want a rule that
    // always transitions on a given calendar day (other than February 29th),
    // you would use a Julian day, e.g. J91 always refers to April 1st and J365
    // always refers to December 31st.
    uint16_t day = self->day;
    if (self->julian && day >= 59 && is_leap_year(year)) {
        day += 1;
    }

    return (days_before_year + day) * 86400 + (int64_t)self->hour * 3600 +
           (int64_t)self->minute * 60 + self->second;
}

/* Constructor for DayRule. */
static int
dayrule_new(int julian, int day, int hour, int minute,
            int second, DayRule *out)
{
    // The POSIX standard specifies that Julian days must be in the range (1 <=
    // n <= 365) and that non-Julian (they call it "0-based Julian") days must
    // be in the range (0 <= n <= 365).
    if (day < julian || day > 365) {
        PyErr_Format(PyExc_ValueError, "day must be in [%d, 365], not: %d",
                     julian, day);
        return -1;
    }

    if (hour < -167 || hour > 167) {
        PyErr_Format(PyExc_ValueError, "Hour must be in [0, 167]");
        return -1;
    }

    TransitionRuleType base = {
        &dayrule_year_to_timestamp,
    };

    DayRule tmp = {
        .base = base,
        .julian = (uint8_t)julian,
        .day = (int16_t)day,
        .hour = (int16_t)hour,
        .minute = (int8_t)minute,
        .second = (int8_t)second,
    };

    *out = tmp;

    return 0;
}

/* Calculate the start and end rules for a _tzrule in the given year. */
static void
tzrule_transitions(_tzrule *rule, int year, int64_t *start, int64_t *end)
{
    assert(rule->start != NULL);
    assert(rule->end != NULL);
    *start = rule->start->year_to_timestamp(rule->start, year);
    *end = rule->end->year_to_timestamp(rule->end, year);
}

/* Calculate the _ttinfo that applies at a given local time from a _tzrule.
 *
 * This takes a local timestamp and fold for disambiguation purposes; the year
 * could technically be calculated from the timestamp, but given that the
 * callers of this function already have the year information accessible from
 * the datetime struct, it is taken as an additional parameter to reduce
 * unnecessary calculation.
 * */
static _ttinfo *
find_tzrule_ttinfo(_tzrule *rule, int64_t ts, unsigned char fold, int year)
{
    if (rule->std_only) {
        return &(rule->std);
    }

    int64_t start, end;
    uint8_t isdst;

    tzrule_transitions(rule, year, &start, &end);

    // With fold = 0, the period (denominated in local time) with the smaller
    // offset starts at the end of the gap and ends at the end of the fold;
    // with fold = 1, it runs from the start of the gap to the beginning of the
    // fold.
    //
    // So in order to determine the DST boundaries we need to know both the
    // fold and whether DST is positive or negative (rare), and it turns out
    // that this boils down to fold XOR is_positive.
    if (fold == (rule->dst_diff >= 0)) {
        end -= rule->dst_diff;
    }
    else {
        start += rule->dst_diff;
    }

    if (start < end) {
        isdst = (ts >= start) && (ts < end);
    }
    else {
        isdst = (ts < end) || (ts >= start);
    }

    if (isdst) {
        return &(rule->dst);
    }
    else {
        return &(rule->std);
    }
}

/* Calculate the ttinfo and fold that applies for a _tzrule at an epoch time.
 *
 * This function can determine the _ttinfo that applies at a given epoch time,
 * (analogous to trans_list_utc), and whether or not the datetime is in a fold.
 * This is to be used in the .fromutc() function.
 *
 * The year is technically a redundant parameter, because it can be calculated
 * from the timestamp, but all callers of this function should have the year
 * in the datetime struct anyway, so taking it as a parameter saves unnecessary
 * calculation.
 **/
static _ttinfo *
find_tzrule_ttinfo_fromutc(_tzrule *rule, int64_t ts, int year,
                           unsigned char *fold)
{
    if (rule->std_only) {
        *fold = 0;
        return &(rule->std);
    }

    int64_t start, end;
    uint8_t isdst;
    tzrule_transitions(rule, year, &start, &end);
    start -= rule->std.utcoff_seconds;
    end -= rule->dst.utcoff_seconds;

    if (start < end) {
        isdst = (ts >= start) && (ts < end);
    }
    else {
        isdst = (ts < end) || (ts >= start);
    }

    // For positive DST, the ambiguous period is one dst_diff after the end of
    // DST; for negative DST, the ambiguous period is one dst_diff before the
    // start of DST.
    int64_t ambig_start, ambig_end;
    if (rule->dst_diff > 0) {
        ambig_start = end;
        ambig_end = end + rule->dst_diff;
    }
    else {
        ambig_start = start;
        ambig_end = start - rule->dst_diff;
    }

    *fold = (ts >= ambig_start) && (ts < ambig_end);

    if (isdst) {
        return &(rule->dst);
    }
    else {
        return &(rule->std);
    }
}

/* Parse a TZ string in the format specified by the POSIX standard:
 *
 *  std offset[dst[offset],start[/time],end[/time]]
 *
 *  std and dst must be 3 or more characters long and must not contain a
 *  leading colon, embedded digits, commas, nor a plus or minus signs; The
 *  spaces between "std" and "offset" are only for display and are not actually
 *  present in the string.
 *
 *  The format of the offset is ``[+|-]hh[:mm[:ss]]``
 *
 * See the POSIX.1 spec: IEE Std 1003.1-2018 §8.3:
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap08.html
 */
static int
parse_tz_str(zoneinfo_state *state, PyObject *tz_str_obj, _tzrule *out)
{
    PyObject *std_abbr = NULL;
    PyObject *dst_abbr = NULL;
    TransitionRuleType *start = NULL;
    TransitionRuleType *end = NULL;
    // Initialize offsets to invalid value (> 24 hours)
    long std_offset = 1 << 20;
    long dst_offset = 1 << 20;

    const char *tz_str = PyBytes_AsString(tz_str_obj);
    if (tz_str == NULL) {
        return -1;
    }
    const char *p = tz_str;

    // Read the `std` abbreviation, which must be at least 3 characters long.
    if (parse_abbr(&p, &std_abbr)) {
        if (!PyErr_Occurred()) {
            PyErr_Format(PyExc_ValueError, "Invalid STD format in %R", tz_str_obj);
        }
        goto error;
    }

    // Now read the STD offset, which is required
    if (parse_tz_delta(&p, &std_offset)) {
        PyErr_Format(PyExc_ValueError, "Invalid STD offset in %R", tz_str_obj);
        goto error;
    }

    // If the string ends here, there is no DST, otherwise we must parse the
    // DST abbreviation and start and end dates and times.
    if (*p == '\0') {
        goto complete;
    }

    if (parse_abbr(&p, &dst_abbr)) {
        if (!PyErr_Occurred()) {
            PyErr_Format(PyExc_ValueError, "Invalid DST format in %R", tz_str_obj);
        }
        goto error;
    }

    if (*p == ',') {
        // From the POSIX standard:
        //
        // If no offset follows dst, the alternative time is assumed to be one
        // hour ahead of standard time.
        dst_offset = std_offset + 3600;
    }
    else {
        if (parse_tz_delta(&p, &dst_offset)) {
            PyErr_Format(PyExc_ValueError, "Invalid DST offset in %R",
                         tz_str_obj);
            goto error;
        }
    }

    TransitionRuleType **transitions[2] = {&start, &end};
    for (size_t i = 0; i < 2; ++i) {
        if (*p != ',') {
            PyErr_Format(PyExc_ValueError,
                         "Missing transition rules in TZ string: %R",
                         tz_str_obj);
            goto error;
        }
        p++;

        if (parse_transition_rule(&p, transitions[i])) {
            PyErr_Format(PyExc_ValueError,
                         "Malformed transition rule in TZ string: %R",
                         tz_str_obj);
            goto error;
        }
    }

    if (*p != '\0') {
        PyErr_Format(PyExc_ValueError,
                     "Extraneous characters at end of TZ string: %R",
                     tz_str_obj);
        goto error;
    }

complete:
    build_tzrule(state, std_abbr, dst_abbr, std_offset, dst_offset,
                 start, end, out);
    Py_DECREF(std_abbr);
    Py_XDECREF(dst_abbr);

    return 0;
error:
    Py_XDECREF(std_abbr);
    if (dst_abbr != NULL && dst_abbr != Py_None) {
        Py_DECREF(dst_abbr);
    }

    if (start != NULL) {
        PyMem_Free(start);
    }

    if (end != NULL) {
        PyMem_Free(end);
    }

    return -1;
}

static int
parse_digits(const char **p, int min, int max, int *value)
{
    assert(max <= 3);
    *value = 0;
    for (int i = 0; i < max; i++, (*p)++) {
        if (!Py_ISDIGIT(**p)) {
            return (i < min) ? -1 : 0;
        }
        *value *= 10;
        *value += (**p) - '0';
    }
    return 0;
}

/* Parse the STD and DST abbreviations from a TZ string. */
static int
parse_abbr(const char **p, PyObject **abbr)
{
    const char *ptr = *p;
    const char *str_start;
    const char *str_end;

    if (*ptr == '<') {
        char buff;
        ptr++;
        str_start = ptr;
        while ((buff = *ptr) != '>') {
            // From the POSIX standard:
            //
            //   In the quoted form, the first character shall be the less-than
            //   ( '<' ) character and the last character shall be the
            //   greater-than ( '>' ) character. All characters between these
            //   quoting characters shall be alphanumeric characters from the
            //   portable character set in the current locale, the plus-sign (
            //   '+' ) character, or the minus-sign ( '-' ) character. The std
            //   and dst fields in this case shall not include the quoting
            //   characters.
            if (!Py_ISALPHA(buff) && !Py_ISDIGIT(buff) && buff != '+' &&
                buff != '-') {
                return -1;
            }
            ptr++;
        }
        str_end = ptr;
        ptr++;
    }
    else {
        str_start = ptr;
        // From the POSIX standard:
        //
        //   In the unquoted form, all characters in these fields shall be
        //   alphabetic characters from the portable character set in the
        //   current locale.
        while (Py_ISALPHA(*ptr)) {
            ptr++;
        }
        str_end = ptr;
        if (str_end == str_start) {
            return -1;
        }
    }

    *abbr = PyUnicode_FromStringAndSize(str_start, str_end - str_start);
    if (*abbr == NULL) {
        return -1;
    }

    *p = ptr;
    return 0;
}

/* Parse a UTC offset from a TZ str. */
static int
parse_tz_delta(const char **p, long *total_seconds)
{
    // From the POSIX spec:
    //
    //   Indicates the value added to the local time to arrive at Coordinated
    //   Universal Time. The offset has the form:
    //
    //   hh[:mm[:ss]]
    //
    //   One or more digits may be used; the value is always interpreted as a
    //   decimal number.
    //
    // The POSIX spec says that the values for `hour` must be between 0 and 24
    // hours, but RFC 8536 §3.3.1 specifies that the hours part of the
    // transition times may be signed and range from -167 to 167.
    int hours = 0;
    int minutes = 0;
    int seconds = 0;

    if (parse_transition_time(p, &hours, &minutes, &seconds)) {
        return -1;
    }

    if (hours > 24 || hours < -24) {
        return -1;
    }

    // Negative numbers correspond to *positive* offsets, from the spec:
    //
    //   If preceded by a '-', the timezone shall be east of the Prime
    //   Meridian; otherwise, it shall be west (which may be indicated by
    //   an optional preceding '+' ).
    *total_seconds = -((hours * 3600L) + (minutes * 60) + seconds);
    return 0;
}

/* Parse the date portion of a transition rule. */
static int
parse_transition_rule(const char **p, TransitionRuleType **out)
{
    // The full transition rule indicates when to change back and forth between
    // STD and DST, and has the form:
    //
    //   date[/time],date[/time]
    //
    // This function parses an individual date[/time] section, and returns
    // the number of characters that contributed to the transition rule. This
    // does not include the ',' at the end of the first rule.
    //
    // The POSIX spec states that if *time* is not given, the default is 02:00.
    const char *ptr = *p;
    int hour = 2;
    int minute = 0;
    int second = 0;

    // Rules come in one of three flavors:
    //
    //   1. Jn: Julian day n, with no leap days.
    //   2. n: Day of year (0-based, with leap days)
    //   3. Mm.n.d: Specifying by month, week and day-of-week.

    if (*ptr == 'M') {
        int month, week, day;
        ptr++;

        if (parse_digits(&ptr, 1, 2, &month)) {
            return -1;
        }
        if (*ptr++ != '.') {
            return -1;
        }
        if (parse_digits(&ptr, 1, 1, &week)) {
            return -1;
        }
        if (*ptr++ != '.') {
            return -1;
        }
        if (parse_digits(&ptr, 1, 1, &day)) {
            return -1;
        }

        if (*ptr == '/') {
            ptr++;
            if (parse_transition_time(&ptr, &hour, &minute, &second)) {
                return -1;
            }
        }

        CalendarRule *rv = PyMem_Calloc(1, sizeof(CalendarRule));
        if (rv == NULL) {
            return -1;
        }

        if (calendarrule_new(month, week, day, hour, minute, second, rv)) {
            PyMem_Free(rv);
            return -1;
        }

        *out = (TransitionRuleType *)rv;
    }
    else {
        int julian = 0;
        int day = 0;
        if (*ptr == 'J') {
            julian = 1;
            ptr++;
        }

        if (parse_digits(&ptr, 1, 3, &day)) {
            return -1;
        }

        if (*ptr == '/') {
            ptr++;
            if (parse_transition_time(&ptr, &hour, &minute, &second)) {
                return -1;
            }
        }

        DayRule *rv = PyMem_Calloc(1, sizeof(DayRule));
        if (rv == NULL) {
            return -1;
        }

        if (dayrule_new(julian, day, hour, minute, second, rv)) {
            PyMem_Free(rv);
            return -1;
        }
        *out = (TransitionRuleType *)rv;
    }

    *p = ptr;
    return 0;
}

/* Parse the time portion of a transition rule (e.g. following an /) */
static int
parse_transition_time(const char **p, int *hour, int *minute, int *second)
{
    // From the spec:
    //
    //   The time has the same format as offset except that no leading sign
    //   ( '-' or '+' ) is allowed.
    //
    // The format for the offset is:
    //
    //   h[h][:mm[:ss]]
    //
    // RFC 8536 also allows transition times to be signed and to range from
    // -167 to +167.
    const char *ptr = *p;
    int sign = 1;

    if (*ptr == '-' || *ptr == '+') {
        if (*ptr == '-') {
            sign = -1;
        }
        ptr++;
    }

    // The hour can be 1 to 3 numeric characters
    if (parse_digits(&ptr, 1, 3, hour)) {
        return -1;
    }
    *hour *= sign;

    // Minutes and seconds always of the format ":dd"
    if (*ptr == ':') {
        ptr++;
        if (parse_digits(&ptr, 2, 2, minute)) {
            return -1;
        }
        *minute *= sign;

        if (*ptr == ':') {
            ptr++;
            if (parse_digits(&ptr, 2, 2, second)) {
                return -1;
            }
            *second *= sign;
        }
    }

    *p = ptr;
    return 0;
}

/* Constructor for a _tzrule.
 *
 * If `dst_abbr` is NULL, this will construct an "STD-only" _tzrule, in which
 * case `dst_offset` will be ignored and `start` and `end` are expected to be
 * NULL as well.
 *
 * Returns 0 on success.
 */
static int
build_tzrule(zoneinfo_state *state, PyObject *std_abbr, PyObject *dst_abbr,
             long std_offset, long dst_offset, TransitionRuleType *start,
             TransitionRuleType *end, _tzrule *out)
{
    _tzrule rv = {{0}};

    rv.start = start;
    rv.end = end;

    if (build_ttinfo(state, std_offset, 0, std_abbr, &rv.std)) {
        goto error;
    }

    if (dst_abbr != NULL) {
        rv.dst_diff = dst_offset - std_offset;
        if (build_ttinfo(state, dst_offset, rv.dst_diff, dst_abbr, &rv.dst)) {
            goto error;
        }
    }
    else {
        rv.std_only = 1;
    }

    *out = rv;

    return 0;
error:
    xdecref_ttinfo(&rv.std);
    xdecref_ttinfo(&rv.dst);
    return -1;
}

/* Destructor for _tzrule. */
static void
free_tzrule(_tzrule *tzrule)
{
    xdecref_ttinfo(&(tzrule->std));
    if (!tzrule->std_only) {
        xdecref_ttinfo(&(tzrule->dst));
    }

    if (tzrule->start != NULL) {
        PyMem_Free(tzrule->start);
    }

    if (tzrule->end != NULL) {
        PyMem_Free(tzrule->end);
    }
}

/* Calculate DST offsets from transitions and UTC offsets
 *
 * This is necessary because each C `ttinfo` only contains the UTC offset,
 * time zone abbreviation and an isdst boolean - it does not include the
 * amount of the DST offset, but we need the amount for the dst() function.
 *
 * Thus function uses heuristics to infer what the offset should be, so it
 * is not guaranteed that this will work for all zones. If we cannot assign
 * a value for a given DST offset, we'll assume it's 1H rather than 0H, so
 * bool(dt.dst()) will always match ttinfo.isdst.
 */
static void
utcoff_to_dstoff(size_t *trans_idx, long *utcoffs, long *dstoffs,
                 unsigned char *isdsts, size_t num_transitions,
                 size_t num_ttinfos)
{
    size_t dst_count = 0;
    size_t dst_found = 0;
    for (size_t i = 0; i < num_ttinfos; ++i) {
        dst_count++;
    }

    for (size_t i = 1; i < num_transitions; ++i) {
        if (dst_count == dst_found) {
            break;
        }

        size_t idx = trans_idx[i];
        size_t comp_idx = trans_idx[i - 1];

        // Only look at DST offsets that have nto been assigned already
        if (!isdsts[idx] || dstoffs[idx] != 0) {
            continue;
        }

        long dstoff = 0;
        long utcoff = utcoffs[idx];

        if (!isdsts[comp_idx]) {
            dstoff = utcoff - utcoffs[comp_idx];
        }

        if (!dstoff && idx < (num_ttinfos - 1)) {
            comp_idx = trans_idx[i + 1];

            // If the following transition is also DST and we couldn't find
            // the DST offset by this point, we're going to have to skip it
            // and hope this transition gets assigned later
            if (isdsts[comp_idx]) {
                continue;
            }

            dstoff = utcoff - utcoffs[comp_idx];
        }

        if (dstoff) {
            dst_found++;
            dstoffs[idx] = dstoff;
        }
    }

    if (dst_found < dst_count) {
        // If there are time zones we didn't find a value for, we'll end up
        // with dstoff = 0 for something where isdst=1. This is obviously
        // wrong — one hour will be a much better guess than 0.
        for (size_t idx = 0; idx < num_ttinfos; ++idx) {
            if (isdsts[idx] && !dstoffs[idx]) {
                dstoffs[idx] = 3600;
            }
        }
    }
}

#define _swap(x, y, buffer) \
    buffer = x;             \
    x = y;                  \
    y = buffer;

/* Calculate transitions in local time from UTC time and offsets.
 *
 * We want to know when each transition occurs, denominated in the number of
 * nominal wall-time seconds between 1970-01-01T00:00:00 and the transition in
 * *local time* (note: this is *not* equivalent to the output of
 * datetime.timestamp, which is the total number of seconds actual elapsed
 * since 1970-01-01T00:00:00Z in UTC).
 *
 * This is an ambiguous question because "local time" can be ambiguous — but it
 * is disambiguated by the `fold` parameter, so we allocate two arrays:
 *
 *  trans_local[0]: The wall-time transitions for fold=0
 *  trans_local[1]: The wall-time transitions for fold=1
 *
 * This returns 0 on success and a negative number of failure. The trans_local
 * arrays must be freed if they are not NULL.
 */
static int
ts_to_local(size_t *trans_idx, int64_t *trans_utc, long *utcoff,
            int64_t *trans_local[2], size_t num_ttinfos,
            size_t num_transitions)
{
    if (num_transitions == 0) {
        return 0;
    }

    // Copy the UTC transitions into each array to be modified in place later
    for (size_t i = 0; i < 2; ++i) {
        trans_local[i] = PyMem_Malloc(num_transitions * sizeof(int64_t));
        if (trans_local[i] == NULL) {
            return -1;
        }

        memcpy(trans_local[i], trans_utc, num_transitions * sizeof(int64_t));
    }

    int64_t offset_0, offset_1, buff;
    if (num_ttinfos > 1) {
        offset_0 = utcoff[0];
        offset_1 = utcoff[trans_idx[0]];

        if (offset_1 > offset_0) {
            _swap(offset_0, offset_1, buff);
        }
    }
    else {
        offset_0 = utcoff[0];
        offset_1 = utcoff[0];
    }

    trans_local[0][0] += offset_0;
    trans_local[1][0] += offset_1;

    for (size_t i = 1; i < num_transitions; ++i) {
        offset_0 = utcoff[trans_idx[i - 1]];
        offset_1 = utcoff[trans_idx[i]];

        if (offset_1 > offset_0) {
            _swap(offset_1, offset_0, buff);
        }

        trans_local[0][i] += offset_0;
        trans_local[1][i] += offset_1;
    }

    return 0;
}

/* Simple bisect_right binary search implementation */
static size_t
_bisect(const int64_t value, const int64_t *arr, size_t size)
{
    size_t lo = 0;
    size_t hi = size;
    size_t m;

    while (lo < hi) {
        m = (lo + hi) / 2;
        if (arr[m] > value) {
            hi = m;
        }
        else {
            lo = m + 1;
        }
    }

    return hi;
}

/* Find the ttinfo rules that apply at a given local datetime. */
static _ttinfo *
find_ttinfo(zoneinfo_state *state, PyZoneInfo_ZoneInfo *self, PyObject *dt)
{
    // datetime.time has a .tzinfo attribute that passes None as the dt
    // argument; it only really has meaning for fixed-offset zones.
    if (dt == Py_None) {
        if (self->fixed_offset) {
            return &(self->tzrule_after.std);
        }
        else {
            return &(state->NO_TTINFO);
        }
    }

    int64_t ts;
    if (get_local_timestamp(dt, &ts)) {
        return NULL;
    }

    unsigned char fold = PyDateTime_DATE_GET_FOLD(dt);
    assert(fold < 2);
    int64_t *local_transitions = self->trans_list_wall[fold];
    size_t num_trans = self->num_transitions;

    if (num_trans && ts < local_transitions[0]) {
        return self->ttinfo_before;
    }
    else if (!num_trans || ts > local_transitions[self->num_transitions - 1]) {
        return find_tzrule_ttinfo(&(self->tzrule_after), ts, fold,
                                  PyDateTime_GET_YEAR(dt));
    }
    else {
        size_t idx = _bisect(ts, local_transitions, self->num_transitions) - 1;
        assert(idx < self->num_transitions);
        return self->trans_ttinfos[idx];
    }
}

static int
is_leap_year(int year)
{
    const unsigned int ayear = (unsigned int)year;
    return ayear % 4 == 0 && (ayear % 100 != 0 || ayear % 400 == 0);
}

/* Calculates ordinal datetime from year, month and day. */
static int
ymd_to_ord(int y, int m, int d)
{
    y -= 1;
    int days_before_year = (y * 365) + (y / 4) - (y / 100) + (y / 400);
    int yearday = DAYS_BEFORE_MONTH[m];
    if (m > 2 && is_leap_year(y + 1)) {
        yearday += 1;
    }

    return days_before_year + yearday + d;
}

/* Calculate the number of seconds since 1970-01-01 in local time.
 *
 * This gets a datetime in the same "units" as self->trans_list_wall so that we
 * can easily determine which transitions a datetime falls between. See the
 * comment above ts_to_local for more information.
 * */
static int
get_local_timestamp(PyObject *dt, int64_t *local_ts)
{
    assert(local_ts != NULL);

    int hour, minute, second;
    int ord;
    if (PyDateTime_CheckExact(dt)) {
        int y = PyDateTime_GET_YEAR(dt);
        int m = PyDateTime_GET_MONTH(dt);
        int d = PyDateTime_GET_DAY(dt);
        hour = PyDateTime_DATE_GET_HOUR(dt);
        minute = PyDateTime_DATE_GET_MINUTE(dt);
        second = PyDateTime_DATE_GET_SECOND(dt);

        ord = ymd_to_ord(y, m, d);
    }
    else {
        PyObject *num = PyObject_CallMethod(dt, "toordinal", NULL);
        if (num == NULL) {
            return -1;
        }

        ord = PyLong_AsLong(num);
        Py_DECREF(num);
        if (ord == -1 && PyErr_Occurred()) {
            return -1;
        }

        num = PyObject_GetAttrString(dt, "hour");
        if (num == NULL) {
            return -1;
        }
        hour = PyLong_AsLong(num);
        Py_DECREF(num);
        if (hour == -1) {
            return -1;
        }

        num = PyObject_GetAttrString(dt, "minute");
        if (num == NULL) {
            return -1;
        }
        minute = PyLong_AsLong(num);
        Py_DECREF(num);
        if (minute == -1) {
            return -1;
        }

        num = PyObject_GetAttrString(dt, "second");
        if (num == NULL) {
            return -1;
        }
        second = PyLong_AsLong(num);
        Py_DECREF(num);
        if (second == -1) {
            return -1;
        }
    }

    *local_ts = (int64_t)(ord - EPOCHORDINAL) * 86400L +
                (int64_t)(hour * 3600L + minute * 60 + second);

    return 0;
}

/////
// Functions for cache handling

/* Constructor for StrongCacheNode
 *
 * This function doesn't set MemoryError if PyMem_Malloc fails,
 * as the cache intentionally doesn't propagate exceptions
 * and fails silently if error occurs.
 */
static StrongCacheNode *
strong_cache_node_new(PyObject *key, PyObject *zone)
{
    StrongCacheNode *node = PyMem_Malloc(sizeof(StrongCacheNode));
    if (node == NULL) {
        return NULL;
    }

    node->next = NULL;
    node->prev = NULL;
    node->key = Py_NewRef(key);
    node->zone = Py_NewRef(zone);

    return node;
}

/* Destructor for StrongCacheNode */
void
strong_cache_node_free(StrongCacheNode *node)
{
    Py_XDECREF(node->key);
    Py_XDECREF(node->zone);

    PyMem_Free(node);
}

/* Frees all nodes at or after a specified root in the strong cache.
 *
 * This can be used on the root node to free the entire cache or it can be used
 * to clear all nodes that have been expired (which, if everything is going
 * right, will actually only be 1 node at a time).
 */
void
strong_cache_free(StrongCacheNode *root)
{
    StrongCacheNode *node = root;
    StrongCacheNode *next_node;
    while (node != NULL) {
        next_node = node->next;
        strong_cache_node_free(node);

        node = next_node;
    }
}

/* Removes a node from the cache and update its neighbors.
 *
 * This is used both when ejecting a node from the cache and when moving it to
 * the front of the cache.
 */
static void
remove_from_strong_cache(zoneinfo_state *state, StrongCacheNode *node)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(state->ZoneInfoType);
    if (state->ZONEINFO_STRONG_CACHE == node) {
        state->ZONEINFO_STRONG_CACHE = node->next;
    }

    if (node->prev != NULL) {
        node->prev->next = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }

    node->next = NULL;
    node->prev = NULL;
}

/* Retrieves the node associated with a key, if it exists.
 *
 * This traverses the strong cache until it finds a matching key and returns a
 * pointer to the relevant node if found. Returns NULL if no node is found.
 *
 * root may be NULL, indicating an empty cache.
 */
static StrongCacheNode *
find_in_strong_cache(const StrongCacheNode *const root, PyObject *const key)
{
    const StrongCacheNode *node = root;
    while (node != NULL) {
        int rv = PyObject_RichCompareBool(key, node->key, Py_EQ);
        if (rv < 0) {
            return NULL;
        }
        if (rv) {
            return (StrongCacheNode *)node;
        }

        node = node->next;
    }

    return NULL;
}

/* Ejects a given key from the class's strong cache, if applicable.
 *
 * This function is used to enable the per-key functionality in clear_cache.
 */
static int
eject_from_strong_cache(zoneinfo_state *state, const PyTypeObject *const type,
                        PyObject *key)
{
    if (type != state->ZoneInfoType) {
        return 0;
    }

    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(state->ZoneInfoType);
    StrongCacheNode *cache = state->ZONEINFO_STRONG_CACHE;
    StrongCacheNode *node = find_in_strong_cache(cache, key);
    if (node != NULL) {
        remove_from_strong_cache(state, node);

        strong_cache_node_free(node);
    }
    else if (PyErr_Occurred()) {
        return -1;
    }
    return 0;
}

/* Moves a node to the front of the LRU cache.
 *
 * The strong cache is an LRU cache, so whenever a given node is accessed, if
 * it is not at the front of the cache, it needs to be moved there.
 */
static void
move_strong_cache_node_to_front(zoneinfo_state *state, StrongCacheNode **root,
                                StrongCacheNode *node)
{
    StrongCacheNode *root_p = *root;
    if (root_p == node) {
        return;
    }

    remove_from_strong_cache(state, node);

    node->prev = NULL;
    node->next = root_p;

    if (root_p != NULL) {
        root_p->prev = node;
    }

    *root = node;
}

/* Retrieves a ZoneInfo from the strong cache if it's present.
 *
 * This function finds the ZoneInfo by key and if found will move the node to
 * the front of the LRU cache and return a new reference to it. It returns NULL
 * if the key is not in the cache.
 *
 * The strong cache is currently only implemented for the base class, so this
 * always returns a cache miss for subclasses.
 */
static PyObject *
zone_from_strong_cache(zoneinfo_state *state, const PyTypeObject *const type,
                       PyObject *const key)
{
    if (type != state->ZoneInfoType) {
        return NULL;  // Strong cache currently only implemented for base class
    }

    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(state->ZoneInfoType);
    StrongCacheNode *cache = state->ZONEINFO_STRONG_CACHE;
    StrongCacheNode *node = find_in_strong_cache(cache, key);

    if (node != NULL) {
        StrongCacheNode **root = &(state->ZONEINFO_STRONG_CACHE);
        move_strong_cache_node_to_front(state, root, node);
        return Py_NewRef(node->zone);
    }

    return NULL;  // Cache miss
}

/* Inserts a new key into the strong LRU cache.
 *
 * This function is only to be used after a cache miss — it creates a new node
 * at the front of the cache and ejects any stale entries (keeping the size of
 * the cache to at most ZONEINFO_STRONG_CACHE_MAX_SIZE).
 */
static void
update_strong_cache(zoneinfo_state *state, const PyTypeObject *const type,
                    PyObject *key, PyObject *zone)
{
    if (type != state->ZoneInfoType) {
        return;
    }

    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(state->ZoneInfoType);
    StrongCacheNode *new_node = strong_cache_node_new(key, zone);
    if (new_node == NULL) {
        return;
    }
    StrongCacheNode **root = &(state->ZONEINFO_STRONG_CACHE);
    move_strong_cache_node_to_front(state, root, new_node);

    StrongCacheNode *node = new_node->next;
    for (size_t i = 1; i < ZONEINFO_STRONG_CACHE_MAX_SIZE; ++i) {
        if (node == NULL) {
            return;
        }
        node = node->next;
    }

    // Everything beyond this point needs to be freed
    if (node != NULL) {
        if (node->prev != NULL) {
            node->prev->next = NULL;
        }
        strong_cache_free(node);
    }
}

/* Clears all entries into a type's strong cache.
 *
 * Because the strong cache is not implemented for subclasses, this is a no-op
 * for everything except the base class.
 */
void
clear_strong_cache(zoneinfo_state *state, const PyTypeObject *const type)
{
    if (type != state->ZoneInfoType) {
        return;
    }

    strong_cache_free(state->ZONEINFO_STRONG_CACHE);
    state->ZONEINFO_STRONG_CACHE = NULL;
}

static PyObject *
new_weak_cache(void)
{
    PyObject *WeakValueDictionary =
            PyImport_ImportModuleAttrString("weakref", "WeakValueDictionary");
    if (WeakValueDictionary == NULL) {
        return NULL;
    }
    PyObject *weak_cache = PyObject_CallNoArgs(WeakValueDictionary);
    Py_DECREF(WeakValueDictionary);
    return weak_cache;
}

// This function is not idempotent and must be called on a new module object.
static int
initialize_caches(zoneinfo_state *state)
{
    state->TIMEDELTA_CACHE = PyDict_New();
    if (state->TIMEDELTA_CACHE == NULL) {
        return -1;
    }

    state->ZONEINFO_WEAK_CACHE = new_weak_cache();
    if (state->ZONEINFO_WEAK_CACHE == NULL) {
        return -1;
    }

    return 0;
}

static PyObject *
zoneinfo_init_subclass(PyObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *weak_cache = new_weak_cache();
    if (weak_cache == NULL) {
        return NULL;
    }

    if (PyObject_SetAttrString(cls, "_weak_cache",
                               weak_cache) < 0) {
        Py_DECREF(weak_cache);
        return NULL;
    }
    Py_DECREF(weak_cache);
    Py_RETURN_NONE;
}

/////
// Specify the ZoneInfo type
static PyMethodDef zoneinfo_methods[] = {
    ZONEINFO_ZONEINFO_CLEAR_CACHE_METHODDEF
    ZONEINFO_ZONEINFO_NO_CACHE_METHODDEF
    ZONEINFO_ZONEINFO_FROM_FILE_METHODDEF
    ZONEINFO_ZONEINFO_UTCOFFSET_METHODDEF
    ZONEINFO_ZONEINFO_DST_METHODDEF
    ZONEINFO_ZONEINFO_TZNAME_METHODDEF
    {"fromutc", zoneinfo_fromutc, METH_O,
     PyDoc_STR("Given a datetime with local time in UTC, retrieve an adjusted "
               "datetime in local time.")},
    {"__reduce__", zoneinfo_reduce, METH_NOARGS,
     PyDoc_STR("Function for serialization with the pickle protocol.")},
    ZONEINFO_ZONEINFO__UNPICKLE_METHODDEF
    {"__init_subclass__", _PyCFunction_CAST(zoneinfo_init_subclass),
     METH_VARARGS | METH_KEYWORDS | METH_CLASS,
     PyDoc_STR("Function to initialize subclasses.")},
    {NULL} /* Sentinel */
};

static PyMemberDef zoneinfo_members[] = {
    {.name = "key",
     .offset = offsetof(PyZoneInfo_ZoneInfo, key),
     .type = Py_T_OBJECT_EX,
     .flags = Py_READONLY,
     .doc = NULL},
    {.name = "__weaklistoffset__",
     .offset = offsetof(PyZoneInfo_ZoneInfo, weakreflist),
     .type = Py_T_PYSSIZET,
     .flags = Py_READONLY},
    {NULL}, /* Sentinel */
};

static PyType_Slot zoneinfo_slots[] = {
    {Py_tp_repr, zoneinfo_repr},
    {Py_tp_str, zoneinfo_str},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_methods, zoneinfo_methods},
    {Py_tp_members, zoneinfo_members},
    {Py_tp_new, zoneinfo_ZoneInfo},
    {Py_tp_dealloc, zoneinfo_dealloc},
    {Py_tp_traverse, zoneinfo_traverse},
    {Py_tp_clear, zoneinfo_clear},
    {0, NULL},
};

static PyType_Spec zoneinfo_spec = {
    .name = "zoneinfo.ZoneInfo",
    .basicsize = sizeof(PyZoneInfo_ZoneInfo),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = zoneinfo_slots,
};

/////
// Specify the _zoneinfo module
static PyMethodDef module_methods[] = {{NULL, NULL}};

static int
module_traverse(PyObject *mod, visitproc visit, void *arg)
{
    zoneinfo_state *state = zoneinfo_get_state(mod);

    Py_VISIT(state->ZoneInfoType);
    Py_VISIT(state->io_open);
    Py_VISIT(state->_tzpath_find_tzfile);
    Py_VISIT(state->_common_mod);
    Py_VISIT(state->TIMEDELTA_CACHE);
    Py_VISIT(state->ZONEINFO_WEAK_CACHE);

    StrongCacheNode *node = state->ZONEINFO_STRONG_CACHE;
    while (node != NULL) {
        StrongCacheNode *next = node->next;
        Py_VISIT(node->key);
        Py_VISIT(node->zone);
        node = next;
    }

    Py_VISIT(state->NO_TTINFO.utcoff);
    Py_VISIT(state->NO_TTINFO.dstoff);
    Py_VISIT(state->NO_TTINFO.tzname);

    return 0;
}

static int
module_clear(PyObject *mod)
{
    zoneinfo_state *state = zoneinfo_get_state(mod);

    Py_CLEAR(state->ZoneInfoType);
    Py_CLEAR(state->io_open);
    Py_CLEAR(state->_tzpath_find_tzfile);
    Py_CLEAR(state->_common_mod);
    Py_CLEAR(state->TIMEDELTA_CACHE);
    Py_CLEAR(state->ZONEINFO_WEAK_CACHE);
    clear_strong_cache(state, state->ZoneInfoType);
    Py_CLEAR(state->NO_TTINFO.utcoff);
    Py_CLEAR(state->NO_TTINFO.dstoff);
    Py_CLEAR(state->NO_TTINFO.tzname);

    return 0;
}

static void
module_free(void *mod)
{
    (void)module_clear((PyObject *)mod);
}

static int
zoneinfomodule_exec(PyObject *m)
{
    PyDateTime_IMPORT;
    if (PyDateTimeAPI == NULL) {
        goto error;
    }

    zoneinfo_state *state = zoneinfo_get_state(m);
    PyObject *base = (PyObject *)PyDateTimeAPI->TZInfoType;
    state->ZoneInfoType = (PyTypeObject *)PyType_FromModuleAndSpec(m,
                                                        &zoneinfo_spec, base);
    if (state->ZoneInfoType == NULL) {
        goto error;
    }

    int rc = PyModule_AddObjectRef(m, "ZoneInfo",
                                   (PyObject *)state->ZoneInfoType);
    if (rc < 0) {
        goto error;
    }

    /* Populate imports */
    state->_tzpath_find_tzfile =
        PyImport_ImportModuleAttrString("zoneinfo._tzpath", "find_tzfile");
    if (state->_tzpath_find_tzfile == NULL) {
        goto error;
    }

    state->io_open = PyImport_ImportModuleAttrString("io", "open");
    if (state->io_open == NULL) {
        goto error;
    }

    state->_common_mod = PyImport_ImportModule("zoneinfo._common");
    if (state->_common_mod == NULL) {
        goto error;
    }

    if (state->NO_TTINFO.utcoff == NULL) {
        state->NO_TTINFO.utcoff = Py_NewRef(Py_None);
        state->NO_TTINFO.dstoff = Py_NewRef(Py_None);
        state->NO_TTINFO.tzname = Py_NewRef(Py_None);
    }

    if (initialize_caches(state)) {
        goto error;
    }

    return 0;

error:
    return -1;
}

static PyModuleDef_Slot zoneinfomodule_slots[] = {
    {Py_mod_exec, zoneinfomodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static struct PyModuleDef zoneinfomodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_zoneinfo",
    .m_doc = "C implementation of the zoneinfo module",
    .m_size = sizeof(zoneinfo_state),
    .m_methods = module_methods,
    .m_slots = zoneinfomodule_slots,
    .m_traverse = module_traverse,
    .m_clear = module_clear,
    .m_free = module_free,
};

PyMODINIT_FUNC
PyInit__zoneinfo(void)
{
    return PyModuleDef_Init(&zoneinfomodule);
}
