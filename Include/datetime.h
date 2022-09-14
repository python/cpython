/*  datetime.h
 */
#ifndef Py_LIMITED_API
#ifndef DATETIME_H
#define DATETIME_H
#ifdef __cplusplus
extern "C" {
#endif

/* Fields are packed into successive bytes, each viewed as unsigned and
 * big-endian, unless otherwise noted:
 *
 * byte offset
 *  0           year     2 bytes, 1-9999
 *  2           month    1 byte, 1-12
 *  3           day      1 byte, 1-31
 *  4           hour     1 byte, 0-23
 *  5           minute   1 byte, 0-59
 *  6           second   1 byte, 0-59
 *  7           usecond  3 bytes, 0-999999
 * 10
 */

/* # of bytes for year, month, and day. */
#define _PyDateTime_DATE_DATASIZE 4

/* # of bytes for hour, minute, second, and usecond. */
#define _PyDateTime_TIME_DATASIZE 6

/* # of bytes for year, month, day, hour, minute, second, and usecond. */
#define _PyDateTime_DATETIME_DATASIZE 10


typedef struct
{
    PyObject_HEAD
    Py_hash_t hashcode;         /* -1 when unknown */
    int days;                   /* -MAX_DELTA_DAYS <= days <= MAX_DELTA_DAYS */
    int seconds;                /* 0 <= seconds < 24*3600 is invariant */
    int microseconds;           /* 0 <= microseconds < 1000000 is invariant */
} PyDateTime_Delta;

typedef struct
{
    PyObject_HEAD               /* a pure abstract base class */
} PyDateTime_TZInfo;


/* The datetime and time types have hashcodes, and an optional tzinfo member,
 * present if and only if hastzinfo is true.
 */
#define _PyTZINFO_HEAD          \
    PyObject_HEAD               \
    Py_hash_t hashcode;         \
    char hastzinfo;             /* boolean flag */

/* No _PyDateTime_BaseTZInfo is allocated; it's just to have something
 * convenient to cast to, when getting at the hastzinfo member of objects
 * starting with _PyTZINFO_HEAD.
 */
typedef struct
{
    _PyTZINFO_HEAD
} _PyDateTime_BaseTZInfo;

/* All time objects are of PyDateTime_TimeType, but that can be allocated
 * in two ways, with or without a tzinfo member.  Without is the same as
 * tzinfo == None, but consumes less memory.  _PyDateTime_BaseTime is an
 * internal struct used to allocate the right amount of space for the
 * "without" case.
 */
#define _PyDateTime_TIMEHEAD    \
    _PyTZINFO_HEAD              \
    unsigned char data[_PyDateTime_TIME_DATASIZE];

typedef struct
{
    _PyDateTime_TIMEHEAD
} _PyDateTime_BaseTime;         /* hastzinfo false */

typedef struct
{
    _PyDateTime_TIMEHEAD
    unsigned char fold;
    PyObject *tzinfo;
} PyDateTime_Time;              /* hastzinfo true */


/* All datetime objects are of PyDateTime_DateTimeType, but that can be
 * allocated in two ways too, just like for time objects above.  In addition,
 * the plain date type is a base class for datetime, so it must also have
 * a hastzinfo member (although it's unused there).
 */
typedef struct
{
    _PyTZINFO_HEAD
    unsigned char data[_PyDateTime_DATE_DATASIZE];
} PyDateTime_Date;

#define _PyDateTime_DATETIMEHEAD        \
    _PyTZINFO_HEAD                      \
    unsigned char data[_PyDateTime_DATETIME_DATASIZE];

typedef struct
{
    _PyDateTime_DATETIMEHEAD
} _PyDateTime_BaseDateTime;     /* hastzinfo false */

typedef struct
{
    _PyDateTime_DATETIMEHEAD
    unsigned char fold;
    PyObject *tzinfo;
} PyDateTime_DateTime;          /* hastzinfo true */


/* Apply for date and datetime instances. */

// o is a pointer to a time or a datetime object.
#define _PyDateTime_HAS_TZINFO(o)  (((_PyDateTime_BaseTZInfo *)(o))->hastzinfo)

#define _PyDateTime_Date_CAST(op) ((PyDateTime_Date*)(op))

static inline int PyDateTime_GET_YEAR(PyObject *o) {
    PyDateTime_Date *date = _PyDateTime_Date_CAST(o);
    return ((date->data[0] << 8) | date->data[1]);
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_GET_YEAR(ob) PyDateTime_GET_YEAR(_PyObject_CAST(ob))
#endif

static inline int PyDateTime_GET_MONTH(PyObject *o) {
    PyDateTime_Date *date = _PyDateTime_Date_CAST(o);
    return date->data[2];
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_GET_MONTH(ob) PyDateTime_GET_MONTH(_PyObject_CAST(ob))
#endif
static inline int PyDateTime_GET_DAY(PyObject *o) {
    PyDateTime_Date *date = _PyDateTime_Date_CAST(o);
    return date->data[3];
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_GET_DAY(ob) PyDateTime_GET_DAY(_PyObject_CAST(ob))
#endif

#define _PyDateTime_DateTime_CAST(op) ((PyDateTime_DateTime*)(op))

static inline int PyDateTime_DATE_GET_HOUR(PyObject *o) {
    PyDateTime_DateTime *dt = _PyDateTime_DateTime_CAST(o);
    return dt->data[4];
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_DATE_GET_HOUR(ob) PyDateTime_DATE_GET_HOUR(_PyObject_CAST(ob))
#endif

static inline int PyDateTime_DATE_GET_MINUTE(PyObject *o) {
    PyDateTime_DateTime *dt = _PyDateTime_DateTime_CAST(o);
    return dt->data[5];
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_DATE_GET_MINUTE(ob) PyDateTime_DATE_GET_MINUTE(_PyObject_CAST(ob))
#endif

static inline int PyDateTime_DATE_GET_SECOND(PyObject *o) {
    PyDateTime_DateTime *dt = _PyDateTime_DateTime_CAST(o);
    return dt->data[6];
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_DATE_GET_SECOND(ob) PyDateTime_DATE_GET_SECOND(_PyObject_CAST(ob))
#endif

static inline int PyDateTime_DATE_GET_MICROSECOND(PyObject *o) {
    PyDateTime_DateTime *dt = _PyDateTime_DateTime_CAST(o);
    return ((dt->data[7] << 16) | (dt->data[8] << 8)  | dt->data[9]);
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_DATE_GET_MICROSECOND(ob) PyDateTime_DATE_GET_MICROSECOND(_PyObject_CAST(ob))
#endif

static inline int PyDateTime_DATE_GET_FOLD(PyObject *o) {
    PyDateTime_DateTime *dt = _PyDateTime_DateTime_CAST(o);
    return dt->fold;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_DATE_GET_FOLD(ob) PyDateTime_DATE_GET_FOLD(_PyObject_CAST(ob))
#endif

static inline PyObject* PyDateTime_DATE_GET_TZINFO(PyObject *o) {
    if (_PyDateTime_HAS_TZINFO((o))) {
        PyDateTime_DateTime *dt = _PyDateTime_DateTime_CAST(o);
        return dt->tzinfo;
    }
    else {
        return Py_None;
    }
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_DATE_GET_TZINFO(ob) PyDateTime_DATE_GET_TZINFO(_PyObject_CAST(ob))
#endif

#define _PyDateTime_Time_CAST(op) ((PyDateTime_Time*)(op))

/* Apply for time instances. */
static inline int PyDateTime_TIME_GET_HOUR(PyObject *o) {
    PyDateTime_Time *time = _PyDateTime_Time_CAST(o);
    return time->data[0];
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_TIME_GET_HOUR(ob) PyDateTime_TIME_GET_HOUR(_PyObject_CAST(ob))
#endif

static inline int PyDateTime_TIME_GET_MINUTE(PyObject *o) {
    PyDateTime_Time *time = _PyDateTime_Time_CAST(o);
    return time->data[1];
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_TIME_GET_MINUTE(ob) PyDateTime_TIME_GET_MINUTE(_PyObject_CAST(ob))
#endif

static inline int PyDateTime_TIME_GET_SECOND(PyObject *o) {
    PyDateTime_Time *time = _PyDateTime_Time_CAST(o);
    return time->data[2];
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_TIME_GET_SECOND(ob) PyDateTime_TIME_GET_SECOND(_PyObject_CAST(ob))
#endif

static inline int PyDateTime_TIME_GET_MICROSECOND(PyObject *o) {
    PyDateTime_Time *time = _PyDateTime_Time_CAST(o);
    return ((time->data[3] << 16) | (time->data[4] << 8)  | time->data[5]);
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_TIME_GET_MICROSECOND(ob) PyDateTime_TIME_GET_MICROSECOND(_PyObject_CAST(ob))
#endif

static inline int PyDateTime_TIME_GET_FOLD(PyObject *o) {
    PyDateTime_Time *time = _PyDateTime_Time_CAST(o);
    return time->fold;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_TIME_GET_FOLD(ob) PyDateTime_TIME_GET_FOLD(_PyObject_CAST(ob))
#endif

static inline PyObject* PyDateTime_TIME_GET_TZINFO(PyObject *o) {
    if (_PyDateTime_HAS_TZINFO(o)) {
        PyDateTime_Time *time = _PyDateTime_Time_CAST(o);
        return time->tzinfo;
    }
    else {
        return Py_None;
    }
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_TIME_GET_TZINFO(ob) PyDateTime_TIME_GET_TZINFO(_PyObject_CAST(ob))
#endif

#define _PyDateTime_DELTA_CAST(op) ((PyDateTime_Delta*)(op))

/* Apply for time delta instances */
static inline int PyDateTime_DELTA_GET_DAYS(PyObject *o) {
    PyDateTime_Delta *delta = _PyDateTime_DELTA_CAST(o);
    return delta->days;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_DELTA_GET_DAYS(ob) PyDateTime_DELTA_GET_DAYS(_PyObject_CAST(ob))
#endif

static inline int PyDateTime_DELTA_GET_SECONDS(PyObject *o){
    PyDateTime_Delta *delta = _PyDateTime_DELTA_CAST(o);
    return delta->seconds;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_DELTA_GET_SECONDS(ob) PyDateTime_DELTA_GET_SECONDS(_PyObject_CAST(ob))
#endif

static inline int PyDateTime_DELTA_GET_MICROSECONDS(PyObject *o) {
    PyDateTime_Delta *delta = _PyDateTime_DELTA_CAST(o);
    return delta->microseconds;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyDateTime_DELTA_GET_MICROSECONDS(ob) PyDateTime_DELTA_GET_MICROSECONDS(_PyObject_CAST(ob))
#endif


/* Define structure for C API. */
typedef struct {
    /* type objects */
    PyTypeObject *DateType;
    PyTypeObject *DateTimeType;
    PyTypeObject *TimeType;
    PyTypeObject *DeltaType;
    PyTypeObject *TZInfoType;

    /* singletons */
    PyObject *TimeZone_UTC;

    /* constructors */
    PyObject *(*Date_FromDate)(int, int, int, PyTypeObject*);
    PyObject *(*DateTime_FromDateAndTime)(int, int, int, int, int, int, int,
        PyObject*, PyTypeObject*);
    PyObject *(*Time_FromTime)(int, int, int, int, PyObject*, PyTypeObject*);
    PyObject *(*Delta_FromDelta)(int, int, int, int, PyTypeObject*);
    PyObject *(*TimeZone_FromTimeZone)(PyObject *offset, PyObject *name);

    /* constructors for the DB API */
    PyObject *(*DateTime_FromTimestamp)(PyObject*, PyObject*, PyObject*);
    PyObject *(*Date_FromTimestamp)(PyObject*, PyObject*);

    /* PEP 495 constructors */
    PyObject *(*DateTime_FromDateAndTimeAndFold)(int, int, int, int, int, int, int,
        PyObject*, int, PyTypeObject*);
    PyObject *(*Time_FromTimeAndFold)(int, int, int, int, PyObject*, int, PyTypeObject*);

} PyDateTime_CAPI;

#define PyDateTime_CAPSULE_NAME "datetime.datetime_CAPI"


/* This block is only used as part of the public API and should not be
 * included in _datetimemodule.c, which does not use the C API capsule.
 * See bpo-35081 for more details.
 * */
#ifndef _PY_DATETIME_IMPL
/* Define global variable for the C API and a macro for setting it. */
static PyDateTime_CAPI *PyDateTimeAPI = NULL;

#define PyDateTime_IMPORT \
    PyDateTimeAPI = (PyDateTime_CAPI *)PyCapsule_Import(PyDateTime_CAPSULE_NAME, 0)

/* Macro for access to the UTC singleton */
#define PyDateTime_TimeZone_UTC PyDateTimeAPI->TimeZone_UTC

/* Macros for type checking when not building the Python core. */
#define PyDate_Check(op) PyObject_TypeCheck((op), PyDateTimeAPI->DateType)
#define PyDate_CheckExact(op) Py_IS_TYPE((op), PyDateTimeAPI->DateType)

#define PyDateTime_Check(op) PyObject_TypeCheck((op), PyDateTimeAPI->DateTimeType)
#define PyDateTime_CheckExact(op) Py_IS_TYPE((op), PyDateTimeAPI->DateTimeType)

#define PyTime_Check(op) PyObject_TypeCheck((op), PyDateTimeAPI->TimeType)
#define PyTime_CheckExact(op) Py_IS_TYPE((op), PyDateTimeAPI->TimeType)

#define PyDelta_Check(op) PyObject_TypeCheck((op), PyDateTimeAPI->DeltaType)
#define PyDelta_CheckExact(op) Py_IS_TYPE((op), PyDateTimeAPI->DeltaType)

#define PyTZInfo_Check(op) PyObject_TypeCheck((op), PyDateTimeAPI->TZInfoType)
#define PyTZInfo_CheckExact(op) Py_IS_TYPE((op), PyDateTimeAPI->TZInfoType)


/* Macros for accessing constructors in a simplified fashion. */
#define PyDate_FromDate(year, month, day) \
    PyDateTimeAPI->Date_FromDate((year), (month), (day), PyDateTimeAPI->DateType)

#define PyDateTime_FromDateAndTime(year, month, day, hour, min, sec, usec) \
    PyDateTimeAPI->DateTime_FromDateAndTime((year), (month), (day), (hour), \
        (min), (sec), (usec), Py_None, PyDateTimeAPI->DateTimeType)

#define PyDateTime_FromDateAndTimeAndFold(year, month, day, hour, min, sec, usec, fold) \
    PyDateTimeAPI->DateTime_FromDateAndTimeAndFold((year), (month), (day), (hour), \
        (min), (sec), (usec), Py_None, (fold), PyDateTimeAPI->DateTimeType)

#define PyTime_FromTime(hour, minute, second, usecond) \
    PyDateTimeAPI->Time_FromTime((hour), (minute), (second), (usecond), \
        Py_None, PyDateTimeAPI->TimeType)

#define PyTime_FromTimeAndFold(hour, minute, second, usecond, fold) \
    PyDateTimeAPI->Time_FromTimeAndFold((hour), (minute), (second), (usecond), \
        Py_None, (fold), PyDateTimeAPI->TimeType)

#define PyDelta_FromDSU(days, seconds, useconds) \
    PyDateTimeAPI->Delta_FromDelta((days), (seconds), (useconds), 1, \
        PyDateTimeAPI->DeltaType)

#define PyTimeZone_FromOffset(offset) \
    PyDateTimeAPI->TimeZone_FromTimeZone((offset), NULL)

#define PyTimeZone_FromOffsetAndName(offset, name) \
    PyDateTimeAPI->TimeZone_FromTimeZone((offset), (name))

/* Macros supporting the DB API. */
#define PyDateTime_FromTimestamp(args) \
    PyDateTimeAPI->DateTime_FromTimestamp( \
        (PyObject*) (PyDateTimeAPI->DateTimeType), (args), NULL)

#define PyDate_FromTimestamp(args) \
    PyDateTimeAPI->Date_FromTimestamp( \
        (PyObject*) (PyDateTimeAPI->DateType), (args))

#endif   /* !defined(_PY_DATETIME_IMPL) */

#ifdef __cplusplus
}
#endif
#endif
#endif /* !Py_LIMITED_API */
