#include "Python.h"
#include "pycore_long.h"          // _PyLong_GetOne()
#include "structmember.h"

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>

#include "datetime.h"

// Imports
static PyObject *io_open = NULL;
static PyObject *_tzpath_find_tzfile = NULL;
static PyObject *_common_mod = NULL;

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

struct TransitionRuleType {
    int64_t (*year_to_timestamp)(TransitionRuleType *, int);
};

typedef struct {
    TransitionRuleType base;
    uint8_t month;
    uint8_t week;
    uint8_t day;
    int8_t hour;
    int8_t minute;
    int8_t second;
} CalendarRule;

typedef struct {
    TransitionRuleType base;
    uint8_t julian;
    unsigned int day;
    int8_t hour;
    int8_t minute;
    int8_t second;
} DayRule;

struct StrongCacheNode {
    StrongCacheNode *next;
    StrongCacheNode *prev;
    PyObject *key;
    PyObject *zone;
};

static PyTypeObject PyZoneInfo_ZoneInfoType;

// Globals
static PyObject *TIMEDELTA_CACHE = NULL;
static PyObject *ZONEINFO_WEAK_CACHE = NULL;
static StrongCacheNode *ZONEINFO_STRONG_CACHE = NULL;
static size_t ZONEINFO_STRONG_CACHE_MAX_SIZE = 8;

static _ttinfo NO_TTINFO = {NULL, NULL, NULL, 0};

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

// Forward declarations
static int
load_data(PyZoneInfo_ZoneInfo *self, PyObject *file_obj);
static void
utcoff_to_dstoff(size_t *trans_idx, long *utcoffs, long *dstoffs,
                 unsigned char *isdsts, size_t num_transitions,
                 size_t num_ttinfos);
static int
ts_to_local(size_t *trans_idx, int64_t *trans_utc, long *utcoff,
            int64_t *trans_local[2], size_t num_ttinfos,
            size_t num_transitions);

static int
parse_tz_str(PyObject *tz_str_obj, _tzrule *out);

static Py_ssize_t
parse_abbr(const char *const p, PyObject **abbr);
static Py_ssize_t
parse_tz_delta(const char *const p, long *total_seconds);
static Py_ssize_t
parse_transition_time(const char *const p, int8_t *hour, int8_t *minute,
                      int8_t *second);
static Py_ssize_t
parse_transition_rule(const char *const p, TransitionRuleType **out);

static _ttinfo *
find_tzrule_ttinfo(_tzrule *rule, int64_t ts, unsigned char fold, int year);
static _ttinfo *
find_tzrule_ttinfo_fromutc(_tzrule *rule, int64_t ts, int year,
                           unsigned char *fold);

static int
build_ttinfo(long utcoffset, long dstoffset, PyObject *tzname, _ttinfo *out);
static void
xdecref_ttinfo(_ttinfo *ttinfo);
static int
ttinfo_eq(const _ttinfo *const tti0, const _ttinfo *const tti1);

static int
build_tzrule(PyObject *std_abbr, PyObject *dst_abbr, long std_offset,
             long dst_offset, TransitionRuleType *start,
             TransitionRuleType *end, _tzrule *out);
static void
free_tzrule(_tzrule *tzrule);

static PyObject *
load_timedelta(long seconds);

static int
get_local_timestamp(PyObject *dt, int64_t *local_ts);
static _ttinfo *
find_ttinfo(PyZoneInfo_ZoneInfo *self, PyObject *dt);

static int
ymd_to_ord(int y, int m, int d);
static int
is_leap_year(int year);

static size_t
_bisect(const int64_t value, const int64_t *arr, size_t size);

static void
eject_from_strong_cache(const PyTypeObject *const type, PyObject *key);
static void
clear_strong_cache(const PyTypeObject *const type);
static void
update_strong_cache(const PyTypeObject *const type, PyObject *key,
                    PyObject *zone);
static PyObject *
zone_from_strong_cache(const PyTypeObject *const type, PyObject *const key);

static PyObject *
zoneinfo_new_instance(PyTypeObject *type, PyObject *key)
{
    PyObject *file_obj = NULL;
    PyObject *file_path = NULL;

    file_path = PyObject_CallFunctionObjArgs(_tzpath_find_tzfile, key, NULL);
    if (file_path == NULL) {
        return NULL;
    }
    else if (file_path == Py_None) {
        file_obj = PyObject_CallMethod(_common_mod, "load_tzdata", "O", key);
        if (file_obj == NULL) {
            Py_DECREF(file_path);
            return NULL;
        }
    }

    PyObject *self = (PyObject *)(type->tp_alloc(type, 0));
    if (self == NULL) {
        goto error;
    }

    if (file_obj == NULL) {
        file_obj = PyObject_CallFunction(io_open, "Os", file_path, "rb");
        if (file_obj == NULL) {
            goto error;
        }
    }

    if (load_data((PyZoneInfo_ZoneInfo *)self, file_obj)) {
        goto error;
    }

    PyObject *rv = PyObject_CallMethod(file_obj, "close", NULL);
    Py_DECREF(file_obj);
    file_obj = NULL;
    if (rv == NULL) {
        goto error;
    }
    Py_DECREF(rv);

    ((PyZoneInfo_ZoneInfo *)self)->key = key;
    Py_INCREF(key);

    goto cleanup;
error:
    Py_XDECREF(self);
    self = NULL;
cleanup:
    if (file_obj != NULL) {
        PyObject *exc, *val, *tb;
        PyErr_Fetch(&exc, &val, &tb);
        PyObject *tmp = PyObject_CallMethod(file_obj, "close", NULL);
        _PyErr_ChainExceptions(exc, val, tb);
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
get_weak_cache(PyTypeObject *type)
{
    if (type == &PyZoneInfo_ZoneInfoType) {
        return ZONEINFO_WEAK_CACHE;
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

static PyObject *
zoneinfo_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PyObject *key = NULL;
    static char *kwlist[] = {"key", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kw, "O", kwlist, &key) == 0) {
        return NULL;
    }

    PyObject *instance = zone_from_strong_cache(type, key);
    if (instance != NULL) {
        return instance;
    }

    PyObject *weak_cache = get_weak_cache(type);
    instance = PyObject_CallMethod(weak_cache, "get", "O", key, Py_None);
    if (instance == NULL) {
        return NULL;
    }

    if (instance == Py_None) {
        Py_DECREF(instance);
        PyObject *tmp = zoneinfo_new_instance(type, key);
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

    update_strong_cache(type, key, instance);
    return instance;
}

static void
zoneinfo_dealloc(PyObject *obj_self)
{
    PyZoneInfo_ZoneInfo *self = (PyZoneInfo_ZoneInfo *)obj_self;

    if (self->weakreflist != NULL) {
        PyObject_ClearWeakRefs(obj_self);
    }

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

    Py_XDECREF(self->key);
    Py_XDECREF(self->file_repr);

    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
zoneinfo_from_file(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *file_obj = NULL;
    PyObject *file_repr = NULL;
    PyObject *key = Py_None;
    PyZoneInfo_ZoneInfo *self = NULL;

    static char *kwlist[] = {"", "key", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &file_obj,
                                     &key)) {
        return NULL;
    }

    PyObject *obj_self = (PyObject *)(type->tp_alloc(type, 0));
    self = (PyZoneInfo_ZoneInfo *)obj_self;
    if (self == NULL) {
        return NULL;
    }

    file_repr = PyUnicode_FromFormat("%R", file_obj);
    if (file_repr == NULL) {
        goto error;
    }

    if (load_data(self, file_obj)) {
        goto error;
    }

    self->source = SOURCE_FILE;
    self->file_repr = file_repr;
    self->key = key;
    Py_INCREF(key);

    return obj_self;
error:
    Py_XDECREF(file_repr);
    Py_XDECREF(self);
    return NULL;
}

static PyObject *
zoneinfo_no_cache(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"key", NULL};
    PyObject *key = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &key)) {
        return NULL;
    }

    PyObject *out = zoneinfo_new_instance(cls, key);
    if (out != NULL) {
        ((PyZoneInfo_ZoneInfo *)out)->source = SOURCE_NOCACHE;
    }

    return out;
}

static PyObject *
zoneinfo_clear_cache(PyObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *only_keys = NULL;
    static char *kwlist[] = {"only_keys", NULL};

    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|$O", kwlist,
                                      &only_keys))) {
        return NULL;
    }

    PyTypeObject *type = (PyTypeObject *)cls;
    PyObject *weak_cache = get_weak_cache(type);

    if (only_keys == NULL || only_keys == Py_None) {
        PyObject *rv = PyObject_CallMethod(weak_cache, "clear", NULL);
        if (rv != NULL) {
            Py_DECREF(rv);
        }

        clear_strong_cache(type);
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
            eject_from_strong_cache(type, item);

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

static PyObject *
zoneinfo_utcoffset(PyObject *self, PyObject *dt)
{
    _ttinfo *tti = find_ttinfo((PyZoneInfo_ZoneInfo *)self, dt);
    if (tti == NULL) {
        return NULL;
    }
    Py_INCREF(tti->utcoff);
    return tti->utcoff;
}

static PyObject *
zoneinfo_dst(PyObject *self, PyObject *dt)
{
    _ttinfo *tti = find_ttinfo((PyZoneInfo_ZoneInfo *)self, dt);
    if (tti == NULL) {
        return NULL;
    }
    Py_INCREF(tti->dstoff);
    return tti->dstoff;
}

static PyObject *
zoneinfo_tzname(PyObject *self, PyObject *dt)
{
    _ttinfo *tti = find_ttinfo((PyZoneInfo_ZoneInfo *)self, dt);
    if (tti == NULL) {
        return NULL;
    }
    Py_INCREF(tti->tzname);
    return tti->tzname;
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

    PyZoneInfo_ZoneInfo *self = (PyZoneInfo_ZoneInfo *)obj_self;

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
            PyObject *args = PyTuple_New(0);
            PyObject *kwargs = PyDict_New();

            Py_DECREF(tmp);
            if (args == NULL || kwargs == NULL || replace == NULL) {
                Py_XDECREF(args);
                Py_XDECREF(kwargs);
                Py_XDECREF(replace);
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
zoneinfo_repr(PyZoneInfo_ZoneInfo *self)
{
    PyObject *rv = NULL;
    const char *type_name = Py_TYPE((PyObject *)self)->tp_name;
    if (!(self->key == Py_None)) {
        rv = PyUnicode_FromFormat("%s(key=%R)", type_name, self->key);
    }
    else {
        assert(PyUnicode_Check(self->file_repr));
        rv = PyUnicode_FromFormat("%s.from_file(%U)", type_name,
                                  self->file_repr);
    }

    return rv;
}

static PyObject *
zoneinfo_str(PyZoneInfo_ZoneInfo *self)
{
    if (!(self->key == Py_None)) {
        Py_INCREF(self->key);
        return self->key;
    }
    else {
        return zoneinfo_repr(self);
    }
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
zoneinfo_reduce(PyObject *obj_self, PyObject *unused)
{
    PyZoneInfo_ZoneInfo *self = (PyZoneInfo_ZoneInfo *)obj_self;
    if (self->source == SOURCE_FILE) {
        // Objects constructed from files cannot be pickled.
        PyObject *pickle = PyImport_ImportModule("pickle");
        if (pickle == NULL) {
            return NULL;
        }

        PyObject *pickle_error =
            PyObject_GetAttrString(pickle, "PicklingError");
        Py_DECREF(pickle);
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

static PyObject *
zoneinfo__unpickle(PyTypeObject *cls, PyObject *args)
{
    PyObject *key;
    unsigned char from_cache;
    if (!PyArg_ParseTuple(args, "OB", &key, &from_cache)) {
        return NULL;
    }

    if (from_cache) {
        PyObject *val_args = Py_BuildValue("(O)", key);
        if (val_args == NULL) {
            return NULL;
        }

        PyObject *rv = zoneinfo_new(cls, val_args, NULL);

        Py_DECREF(val_args);
        return rv;
    }
    else {
        return zoneinfo_new_instance(cls, key);
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
load_timedelta(long seconds)
{
    PyObject *rv;
    PyObject *pyoffset = PyLong_FromLong(seconds);
    if (pyoffset == NULL) {
        return NULL;
    }
    rv = PyDict_GetItemWithError(TIMEDELTA_CACHE, pyoffset);
    if (rv == NULL) {
        if (PyErr_Occurred()) {
            goto error;
        }
        PyObject *tmp = PyDateTimeAPI->Delta_FromDelta(
            0, seconds, 0, 1, PyDateTimeAPI->DeltaType);

        if (tmp == NULL) {
            goto error;
        }

        rv = PyDict_SetDefault(TIMEDELTA_CACHE, pyoffset, tmp);
        Py_DECREF(tmp);
    }

    Py_XINCREF(rv);
    Py_DECREF(pyoffset);
    return rv;
error:
    Py_DECREF(pyoffset);
    return NULL;
}

/* Constructor for _ttinfo object - this starts by initializing the _ttinfo
 * to { NULL, NULL, NULL }, so that Py_XDECREF will work on partially
 * initialized _ttinfo objects.
 */
static int
build_ttinfo(long utcoffset, long dstoffset, PyObject *tzname, _ttinfo *out)
{
    out->utcoff = NULL;
    out->dstoff = NULL;
    out->tzname = NULL;

    out->utcoff_seconds = utcoffset;
    out->utcoff = load_timedelta(utcoffset);
    if (out->utcoff == NULL) {
        return -1;
    }

    out->dstoff = load_timedelta(dstoffset);
    if (out->dstoff == NULL) {
        return -1;
    }

    out->tzname = tzname;
    Py_INCREF(tzname);

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
load_data(PyZoneInfo_ZoneInfo *self, PyObject *file_obj)
{
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

    data_tuple = PyObject_CallMethod(_common_mod, "load_data", "O", file_obj);

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
    trans_idx = PyMem_Malloc(self->num_transitions * sizeof(Py_ssize_t));

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
    for (size_t i = 0; i < self->num_ttinfos; ++i) {
        PyObject *tzname = PyTuple_GetItem(abbr, i);
        if (tzname == NULL) {
            goto error;
        }

        ttinfos_allocated++;
        if (build_ttinfo(utcoff[i], dstoff[i], tzname, &(self->_ttinfos[i]))) {
            goto error;
        }
    }

    // Build our mapping from transition to the ttinfo that applies
    self->trans_ttinfos =
        PyMem_Calloc(self->num_transitions, sizeof(_ttinfo *));
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
        if (parse_tz_str(tz_str, &(self->tzrule_after))) {
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
        build_tzrule(tti->tzname, NULL, tti->utcoff_seconds, 0, NULL, NULL,
                     &(self->tzrule_after));

        // We've abused the build_tzrule constructor to construct an STD-only
        // rule mimicking whatever ttinfo we've picked up, but it's possible
        // that the one we've picked up is a DST zone, so we need to make sure
        // that the dstoff is set correctly in that case.
        if (PyObject_IsTrue(tti->dstoff)) {
            _ttinfo *tti_after = &(self->tzrule_after.std);
            Py_DECREF(tti_after->dstoff);
            tti_after->dstoff = tti->dstoff;
            Py_INCREF(tti_after->dstoff);
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

    int rv = 0;
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
    return ((ordinal * 86400) + (int64_t)(self->hour * 3600) +
            (int64_t)(self->minute * 60) + (int64_t)(self->second));
}

/* Constructor for CalendarRule. */
int
calendarrule_new(uint8_t month, uint8_t week, uint8_t day, int8_t hour,
                 int8_t minute, int8_t second, CalendarRule *out)
{
    // These bounds come from the POSIX standard, which describes an Mm.n.d
    // rule as:
    //
    //   The d'th day (0 <= d <= 6) of week n of month m of the year (1 <= n <=
    //   5, 1 <= m <= 12, where week 5 means "the last d day in month m" which
    //   may occur in either the fourth or the fifth week). Week 1 is the first
    //   week in which the d'th day occurs. Day zero is Sunday.
    if (month <= 0 || month > 12) {
        PyErr_Format(PyExc_ValueError, "Month must be in (0, 12]");
        return -1;
    }

    if (week <= 0 || week > 5) {
        PyErr_Format(PyExc_ValueError, "Week must be in (0, 5]");
        return -1;
    }

    // If the 'day' parameter type is changed to a signed type,
    // "day < 0" check must be added.
    if (/* day < 0 || */ day > 6) {
        PyErr_Format(PyExc_ValueError, "Day must be in [0, 6]");
        return -1;
    }

    TransitionRuleType base = {&calendarrule_year_to_timestamp};

    CalendarRule new_offset = {
        .base = base,
        .month = month,
        .week = week,
        .day = day,
        .hour = hour,
        .minute = minute,
        .second = second,
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
    unsigned int day = self->day;
    if (self->julian && day >= 59 && is_leap_year(year)) {
        day += 1;
    }

    return ((days_before_year + day) * 86400) + (self->hour * 3600) +
           (self->minute * 60) + self->second;
}

/* Constructor for DayRule. */
static int
dayrule_new(uint8_t julian, unsigned int day, int8_t hour, int8_t minute,
            int8_t second, DayRule *out)
{
    // The POSIX standard specifies that Julian days must be in the range (1 <=
    // n <= 365) and that non-Julian (they call it "0-based Julian") days must
    // be in the range (0 <= n <= 365).
    if (day < julian || day > 365) {
        PyErr_Format(PyExc_ValueError, "day must be in [%u, 365], not: %u",
                     julian, day);
        return -1;
    }

    TransitionRuleType base = {
        &dayrule_year_to_timestamp,
    };

    DayRule tmp = {
        .base = base,
        .julian = julian,
        .day = day,
        .hour = hour,
        .minute = minute,
        .second = second,
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
 * unncessary calculation.
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
parse_tz_str(PyObject *tz_str_obj, _tzrule *out)
{
    PyObject *std_abbr = NULL;
    PyObject *dst_abbr = NULL;
    TransitionRuleType *start = NULL;
    TransitionRuleType *end = NULL;
    // Initialize offsets to invalid value (> 24 hours)
    long std_offset = 1 << 20;
    long dst_offset = 1 << 20;

    char *tz_str = PyBytes_AsString(tz_str_obj);
    if (tz_str == NULL) {
        return -1;
    }
    char *p = tz_str;

    // Read the `std` abbreviation, which must be at least 3 characters long.
    Py_ssize_t num_chars = parse_abbr(p, &std_abbr);
    if (num_chars < 1) {
        PyErr_Format(PyExc_ValueError, "Invalid STD format in %R", tz_str_obj);
        goto error;
    }

    p += num_chars;

    // Now read the STD offset, which is required
    num_chars = parse_tz_delta(p, &std_offset);
    if (num_chars < 0) {
        PyErr_Format(PyExc_ValueError, "Invalid STD offset in %R", tz_str_obj);
        goto error;
    }
    p += num_chars;

    // If the string ends here, there is no DST, otherwise we must parse the
    // DST abbreviation and start and end dates and times.
    if (*p == '\0') {
        goto complete;
    }

    num_chars = parse_abbr(p, &dst_abbr);
    if (num_chars < 1) {
        PyErr_Format(PyExc_ValueError, "Invalid DST format in %R", tz_str_obj);
        goto error;
    }
    p += num_chars;

    if (*p == ',') {
        // From the POSIX standard:
        //
        // If no offset follows dst, the alternative time is assumed to be one
        // hour ahead of standard time.
        dst_offset = std_offset + 3600;
    }
    else {
        num_chars = parse_tz_delta(p, &dst_offset);
        if (num_chars < 0) {
            PyErr_Format(PyExc_ValueError, "Invalid DST offset in %R",
                         tz_str_obj);
            goto error;
        }

        p += num_chars;
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

        num_chars = parse_transition_rule(p, transitions[i]);
        if (num_chars < 0) {
            PyErr_Format(PyExc_ValueError,
                         "Malformed transition rule in TZ string: %R",
                         tz_str_obj);
            goto error;
        }
        p += num_chars;
    }

    if (*p != '\0') {
        PyErr_Format(PyExc_ValueError,
                     "Extraneous characters at end of TZ string: %R",
                     tz_str_obj);
        goto error;
    }

complete:
    build_tzrule(std_abbr, dst_abbr, std_offset, dst_offset, start, end, out);
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
parse_uint(const char *const p, uint8_t *value)
{
    if (!isdigit(*p)) {
        return -1;
    }

    *value = (*p) - '0';
    return 0;
}

/* Parse the STD and DST abbreviations from a TZ string. */
static Py_ssize_t
parse_abbr(const char *const p, PyObject **abbr)
{
    const char *ptr = p;
    char buff = *ptr;
    const char *str_start;
    const char *str_end;

    if (*ptr == '<') {
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
            if (!isalpha(buff) && !isdigit(buff) && buff != '+' &&
                buff != '-') {
                return -1;
            }
            ptr++;
        }
        str_end = ptr;
        ptr++;
    }
    else {
        str_start = p;
        // From the POSIX standard:
        //
        //   In the unquoted form, all characters in these fields shall be
        //   alphabetic characters from the portable character set in the
        //   current locale.
        while (isalpha(*ptr)) {
            ptr++;
        }
        str_end = ptr;
    }

    *abbr = PyUnicode_FromStringAndSize(str_start, str_end - str_start);
    if (*abbr == NULL) {
        return -1;
    }

    return ptr - p;
}

/* Parse a UTC offset from a TZ str. */
static Py_ssize_t
parse_tz_delta(const char *const p, long *total_seconds)
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
    long sign = -1;
    long hours = 0;
    long minutes = 0;
    long seconds = 0;

    const char *ptr = p;
    char buff = *ptr;
    if (buff == '-' || buff == '+') {
        // Negative numbers correspond to *positive* offsets, from the spec:
        //
        //   If preceded by a '-', the timezone shall be east of the Prime
        //   Meridian; otherwise, it shall be west (which may be indicated by
        //   an optional preceding '+' ).
        if (buff == '-') {
            sign = 1;
        }

        ptr++;
    }

    // The hour can be 1 or 2 numeric characters
    for (size_t i = 0; i < 2; ++i) {
        buff = *ptr;
        if (!isdigit(buff)) {
            if (i == 0) {
                return -1;
            }
            else {
                break;
            }
        }

        hours *= 10;
        hours += buff - '0';
        ptr++;
    }

    if (hours > 24 || hours < 0) {
        return -1;
    }

    // Minutes and seconds always of the format ":dd"
    long *outputs[2] = {&minutes, &seconds};
    for (size_t i = 0; i < 2; ++i) {
        if (*ptr != ':') {
            goto complete;
        }
        ptr++;

        for (size_t j = 0; j < 2; ++j) {
            buff = *ptr;
            if (!isdigit(buff)) {
                return -1;
            }
            *(outputs[i]) *= 10;
            *(outputs[i]) += buff - '0';
            ptr++;
        }
    }

complete:
    *total_seconds = sign * ((hours * 3600) + (minutes * 60) + seconds);

    return ptr - p;
}

/* Parse the date portion of a transition rule. */
static Py_ssize_t
parse_transition_rule(const char *const p, TransitionRuleType **out)
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
    const char *ptr = p;
    int8_t hour = 2;
    int8_t minute = 0;
    int8_t second = 0;

    // Rules come in one of three flavors:
    //
    //   1. Jn: Julian day n, with no leap days.
    //   2. n: Day of year (0-based, with leap days)
    //   3. Mm.n.d: Specifying by month, week and day-of-week.

    if (*ptr == 'M') {
        uint8_t month, week, day;
        ptr++;
        if (parse_uint(ptr, &month)) {
            return -1;
        }
        ptr++;
        if (*ptr != '.') {
            uint8_t tmp;
            if (parse_uint(ptr, &tmp)) {
                return -1;
            }

            month *= 10;
            month += tmp;
            ptr++;
        }

        uint8_t *values[2] = {&week, &day};
        for (size_t i = 0; i < 2; ++i) {
            if (*ptr != '.') {
                return -1;
            }
            ptr++;

            if (parse_uint(ptr, values[i])) {
                return -1;
            }
            ptr++;
        }

        if (*ptr == '/') {
            ptr++;
            Py_ssize_t num_chars =
                parse_transition_time(ptr, &hour, &minute, &second);
            if (num_chars < 0) {
                return -1;
            }
            ptr += num_chars;
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
        uint8_t julian = 0;
        unsigned int day = 0;
        if (*ptr == 'J') {
            julian = 1;
            ptr++;
        }

        for (size_t i = 0; i < 3; ++i) {
            if (!isdigit(*ptr)) {
                if (i == 0) {
                    return -1;
                }
                break;
            }
            day *= 10;
            day += (*ptr) - '0';
            ptr++;
        }

        if (*ptr == '/') {
            ptr++;
            Py_ssize_t num_chars =
                parse_transition_time(ptr, &hour, &minute, &second);
            if (num_chars < 0) {
                return -1;
            }
            ptr += num_chars;
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

    return ptr - p;
}

/* Parse the time portion of a transition rule (e.g. following an /) */
static Py_ssize_t
parse_transition_time(const char *const p, int8_t *hour, int8_t *minute,
                      int8_t *second)
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
    // -167 to +167, but the current version only supports [0, 99].
    //
    // TODO: Support the full range of transition hours.
    int8_t *components[3] = {hour, minute, second};
    const char *ptr = p;
    int8_t sign = 1;

    if (*ptr == '-' || *ptr == '+') {
        if (*ptr == '-') {
            sign = -1;
        }
        ptr++;
    }

    for (size_t i = 0; i < 3; ++i) {
        if (i > 0) {
            if (*ptr != ':') {
                break;
            }
            ptr++;
        }

        uint8_t buff = 0;
        for (size_t j = 0; j < 2; j++) {
            if (!isdigit(*ptr)) {
                if (i == 0 && j > 0) {
                    break;
                }
                return -1;
            }

            buff *= 10;
            buff += (*ptr) - '0';
            ptr++;
        }

        *(components[i]) = sign * buff;
    }

    return ptr - p;
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
build_tzrule(PyObject *std_abbr, PyObject *dst_abbr, long std_offset,
             long dst_offset, TransitionRuleType *start,
             TransitionRuleType *end, _tzrule *out)
{
    _tzrule rv = {{0}};

    rv.start = start;
    rv.end = end;

    if (build_ttinfo(std_offset, 0, std_abbr, &rv.std)) {
        goto error;
    }

    if (dst_abbr != NULL) {
        rv.dst_diff = dst_offset - std_offset;
        if (build_ttinfo(dst_offset, rv.dst_diff, dst_abbr, &rv.dst)) {
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
find_ttinfo(PyZoneInfo_ZoneInfo *self, PyObject *dt)
{
    // datetime.time has a .tzinfo attribute that passes None as the dt
    // argument; it only really has meaning for fixed-offset zones.
    if (dt == Py_None) {
        if (self->fixed_offset) {
            return &(self->tzrule_after.std);
        }
        else {
            return &NO_TTINFO;
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

    *local_ts = (int64_t)(ord - EPOCHORDINAL) * 86400 +
                (int64_t)(hour * 3600 + minute * 60 + second);

    return 0;
}

/////
// Functions for cache handling

/* Constructor for StrongCacheNode */
static StrongCacheNode *
strong_cache_node_new(PyObject *key, PyObject *zone)
{
    StrongCacheNode *node = PyMem_Malloc(sizeof(StrongCacheNode));
    if (node == NULL) {
        return NULL;
    }

    Py_INCREF(key);
    Py_INCREF(zone);

    node->next = NULL;
    node->prev = NULL;
    node->key = key;
    node->zone = zone;

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
remove_from_strong_cache(StrongCacheNode *node)
{
    if (ZONEINFO_STRONG_CACHE == node) {
        ZONEINFO_STRONG_CACHE = node->next;
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
        if (PyObject_RichCompareBool(key, node->key, Py_EQ)) {
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
static void
eject_from_strong_cache(const PyTypeObject *const type, PyObject *key)
{
    if (type != &PyZoneInfo_ZoneInfoType) {
        return;
    }

    StrongCacheNode *node = find_in_strong_cache(ZONEINFO_STRONG_CACHE, key);
    if (node != NULL) {
        remove_from_strong_cache(node);

        strong_cache_node_free(node);
    }
}

/* Moves a node to the front of the LRU cache.
 *
 * The strong cache is an LRU cache, so whenever a given node is accessed, if
 * it is not at the front of the cache, it needs to be moved there.
 */
static void
move_strong_cache_node_to_front(StrongCacheNode **root, StrongCacheNode *node)
{
    StrongCacheNode *root_p = *root;
    if (root_p == node) {
        return;
    }

    remove_from_strong_cache(node);

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
zone_from_strong_cache(const PyTypeObject *const type, PyObject *const key)
{
    if (type != &PyZoneInfo_ZoneInfoType) {
        return NULL;  // Strong cache currently only implemented for base class
    }

    StrongCacheNode *node = find_in_strong_cache(ZONEINFO_STRONG_CACHE, key);

    if (node != NULL) {
        move_strong_cache_node_to_front(&ZONEINFO_STRONG_CACHE, node);
        Py_INCREF(node->zone);
        return node->zone;
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
update_strong_cache(const PyTypeObject *const type, PyObject *key,
                    PyObject *zone)
{
    if (type != &PyZoneInfo_ZoneInfoType) {
        return;
    }

    StrongCacheNode *new_node = strong_cache_node_new(key, zone);

    move_strong_cache_node_to_front(&ZONEINFO_STRONG_CACHE, new_node);

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
clear_strong_cache(const PyTypeObject *const type)
{
    if (type != &PyZoneInfo_ZoneInfoType) {
        return;
    }

    strong_cache_free(ZONEINFO_STRONG_CACHE);
    ZONEINFO_STRONG_CACHE = NULL;
}

static PyObject *
new_weak_cache(void)
{
    PyObject *weakref_module = PyImport_ImportModule("weakref");
    if (weakref_module == NULL) {
        return NULL;
    }

    PyObject *weak_cache =
        PyObject_CallMethod(weakref_module, "WeakValueDictionary", "");
    Py_DECREF(weakref_module);
    return weak_cache;
}

static int
initialize_caches(void)
{
    // TODO: Move to a PyModule_GetState / PEP 573 based caching system.
    if (TIMEDELTA_CACHE == NULL) {
        TIMEDELTA_CACHE = PyDict_New();
    }
    else {
        Py_INCREF(TIMEDELTA_CACHE);
    }

    if (TIMEDELTA_CACHE == NULL) {
        return -1;
    }

    if (ZONEINFO_WEAK_CACHE == NULL) {
        ZONEINFO_WEAK_CACHE = new_weak_cache();
    }
    else {
        Py_INCREF(ZONEINFO_WEAK_CACHE);
    }

    if (ZONEINFO_WEAK_CACHE == NULL) {
        return -1;
    }

    return 0;
}

static PyObject *
zoneinfo_init_subclass(PyTypeObject *cls, PyObject *args, PyObject **kwargs)
{
    PyObject *weak_cache = new_weak_cache();
    if (weak_cache == NULL) {
        return NULL;
    }

    PyObject_SetAttrString((PyObject *)cls, "_weak_cache", weak_cache);
    Py_DECREF(weak_cache);
    Py_RETURN_NONE;
}

/////
// Specify the ZoneInfo type
static PyMethodDef zoneinfo_methods[] = {
    {"clear_cache", (PyCFunction)(void (*)(void))zoneinfo_clear_cache,
     METH_VARARGS | METH_KEYWORDS | METH_CLASS,
     PyDoc_STR("Clear the ZoneInfo cache.")},
    {"no_cache", (PyCFunction)(void (*)(void))zoneinfo_no_cache,
     METH_VARARGS | METH_KEYWORDS | METH_CLASS,
     PyDoc_STR("Get a new instance of ZoneInfo, bypassing the cache.")},
    {"from_file", (PyCFunction)(void (*)(void))zoneinfo_from_file,
     METH_VARARGS | METH_KEYWORDS | METH_CLASS,
     PyDoc_STR("Create a ZoneInfo file from a file object.")},
    {"utcoffset", (PyCFunction)zoneinfo_utcoffset, METH_O,
     PyDoc_STR("Retrieve a timedelta representing the UTC offset in a zone at "
               "the given datetime.")},
    {"dst", (PyCFunction)zoneinfo_dst, METH_O,
     PyDoc_STR("Retrieve a timedelta representing the amount of DST applied "
               "in a zone at the given datetime.")},
    {"tzname", (PyCFunction)zoneinfo_tzname, METH_O,
     PyDoc_STR("Retrieve a string containing the abbreviation for the time "
               "zone that applies in a zone at a given datetime.")},
    {"fromutc", (PyCFunction)zoneinfo_fromutc, METH_O,
     PyDoc_STR("Given a datetime with local time in UTC, retrieve an adjusted "
               "datetime in local time.")},
    {"__reduce__", (PyCFunction)zoneinfo_reduce, METH_NOARGS,
     PyDoc_STR("Function for serialization with the pickle protocol.")},
    {"_unpickle", (PyCFunction)zoneinfo__unpickle, METH_VARARGS | METH_CLASS,
     PyDoc_STR("Private method used in unpickling.")},
    {"__init_subclass__", (PyCFunction)(void (*)(void))zoneinfo_init_subclass,
     METH_VARARGS | METH_KEYWORDS | METH_CLASS,
     PyDoc_STR("Function to initialize subclasses.")},
    {NULL} /* Sentinel */
};

static PyMemberDef zoneinfo_members[] = {
    {.name = "key",
     .offset = offsetof(PyZoneInfo_ZoneInfo, key),
     .type = T_OBJECT_EX,
     .flags = READONLY,
     .doc = NULL},
    {NULL}, /* Sentinel */
};

static PyTypeObject PyZoneInfo_ZoneInfoType = {
    PyVarObject_HEAD_INIT(NULL, 0)  //
        .tp_name = "zoneinfo.ZoneInfo",
    .tp_basicsize = sizeof(PyZoneInfo_ZoneInfo),
    .tp_weaklistoffset = offsetof(PyZoneInfo_ZoneInfo, weakreflist),
    .tp_repr = (reprfunc)zoneinfo_repr,
    .tp_str = (reprfunc)zoneinfo_str,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE),
    /* .tp_doc = zoneinfo_doc, */
    .tp_methods = zoneinfo_methods,
    .tp_members = zoneinfo_members,
    .tp_new = zoneinfo_new,
    .tp_dealloc = zoneinfo_dealloc,
};

/////
// Specify the _zoneinfo module
static PyMethodDef module_methods[] = {{NULL, NULL}};
static void
module_free()
{
    Py_XDECREF(_tzpath_find_tzfile);
    _tzpath_find_tzfile = NULL;

    Py_XDECREF(_common_mod);
    _common_mod = NULL;

    Py_XDECREF(io_open);
    io_open = NULL;

    xdecref_ttinfo(&NO_TTINFO);

    if (TIMEDELTA_CACHE != NULL && Py_REFCNT(TIMEDELTA_CACHE) > 1) {
        Py_DECREF(TIMEDELTA_CACHE);
    } else {
        Py_CLEAR(TIMEDELTA_CACHE);
    }

    if (ZONEINFO_WEAK_CACHE != NULL && Py_REFCNT(ZONEINFO_WEAK_CACHE) > 1) {
        Py_DECREF(ZONEINFO_WEAK_CACHE);
    } else {
        Py_CLEAR(ZONEINFO_WEAK_CACHE);
    }

    clear_strong_cache(&PyZoneInfo_ZoneInfoType);
}

static int
zoneinfomodule_exec(PyObject *m)
{
    PyDateTime_IMPORT;
    PyZoneInfo_ZoneInfoType.tp_base = PyDateTimeAPI->TZInfoType;
    if (PyType_Ready(&PyZoneInfo_ZoneInfoType) < 0) {
        goto error;
    }

    Py_INCREF(&PyZoneInfo_ZoneInfoType);
    PyModule_AddObject(m, "ZoneInfo", (PyObject *)&PyZoneInfo_ZoneInfoType);

    /* Populate imports */
    PyObject *_tzpath_module = PyImport_ImportModule("zoneinfo._tzpath");
    if (_tzpath_module == NULL) {
        goto error;
    }

    _tzpath_find_tzfile =
        PyObject_GetAttrString(_tzpath_module, "find_tzfile");
    Py_DECREF(_tzpath_module);
    if (_tzpath_find_tzfile == NULL) {
        goto error;
    }

    PyObject *io_module = PyImport_ImportModule("io");
    if (io_module == NULL) {
        goto error;
    }

    io_open = PyObject_GetAttrString(io_module, "open");
    Py_DECREF(io_module);
    if (io_open == NULL) {
        goto error;
    }

    _common_mod = PyImport_ImportModule("zoneinfo._common");
    if (_common_mod == NULL) {
        goto error;
    }

    if (NO_TTINFO.utcoff == NULL) {
        NO_TTINFO.utcoff = Py_None;
        NO_TTINFO.dstoff = Py_None;
        NO_TTINFO.tzname = Py_None;

        for (size_t i = 0; i < 3; ++i) {
            Py_INCREF(Py_None);
        }
    }

    if (initialize_caches()) {
        goto error;
    }

    return 0;

error:
    return -1;
}

static PyModuleDef_Slot zoneinfomodule_slots[] = {
    {Py_mod_exec, zoneinfomodule_exec}, {0, NULL}};

static struct PyModuleDef zoneinfomodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_zoneinfo",
    .m_doc = "C implementation of the zoneinfo module",
    .m_size = 0,
    .m_methods = module_methods,
    .m_slots = zoneinfomodule_slots,
    .m_free = (freefunc)module_free};

PyMODINIT_FUNC
PyInit__zoneinfo(void)
{
    return PyModuleDef_Init(&zoneinfomodule);
}
