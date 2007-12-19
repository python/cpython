/*
 * This is the High Performance Python Profiler portion of HotShot.
 */

#include "Python.h"
#include "code.h"
#include "eval.h"
#include "frameobject.h"
#include "structmember.h"

/*
 * Which timer to use should be made more configurable, but that should not
 * be difficult.  This will do for now.
 */
#ifdef MS_WINDOWS
#include <windows.h>

#ifdef HAVE_DIRECT_H
#include <direct.h>    /* for getcwd() */
#endif

typedef __int64 hs_time;
#define GETTIMEOFDAY(P_HS_TIME) \
	{ LARGE_INTEGER _temp; \
	  QueryPerformanceCounter(&_temp); \
	  *(P_HS_TIME) = _temp.QuadPart; }
	  

#else
#ifndef HAVE_GETTIMEOFDAY
#error "This module requires gettimeofday() on non-Windows platforms!"
#endif
#if (defined(PYOS_OS2) && defined(PYCC_GCC)) || defined(__QNX__)
#include <sys/time.h>
#else
#include <sys/resource.h>
#include <sys/times.h>
#endif
typedef struct timeval hs_time;
#endif

#if !defined(__cplusplus) && !defined(inline)
#ifdef __GNUC__
#define inline __inline
#endif
#endif

#ifndef inline
#define inline
#endif

#define BUFFERSIZE 10240

#if defined(PYOS_OS2) && defined(PYCC_GCC)
#define PATH_MAX 260
#endif

#if defined(__sgi) && _COMPILER_VERSION>700 && !defined(PATH_MAX)
/* fix PATH_MAX not being defined with MIPSPro 7.x
   if mode is ANSI C (default) */
#define PATH_MAX 1024
#endif

#ifndef PATH_MAX
#   ifdef MAX_PATH
#       define PATH_MAX MAX_PATH
#   elif defined (_POSIX_PATH_MAX)
#       define PATH_MAX _POSIX_PATH_MAX
#   else
#       error "Need a defn. for PATH_MAX in _hotshot.c"
#   endif
#endif

typedef struct {
    PyObject_HEAD
    PyObject *filemap;
    PyObject *logfilename;
    Py_ssize_t index;
    unsigned char buffer[BUFFERSIZE];
    FILE *logfp;
    int lineevents;
    int linetimings;
    int frametimings;
    /* size_t filled; */
    int active;
    int next_fileno;
    hs_time prev_timeofday;
} ProfilerObject;

typedef struct {
    PyObject_HEAD
    PyObject *info;
    FILE *logfp;
    int linetimings;
    int frametimings;
} LogReaderObject;

static PyObject * ProfilerError = NULL;


#ifndef MS_WINDOWS
#ifdef GETTIMEOFDAY_NO_TZ
#define GETTIMEOFDAY(ptv) gettimeofday((ptv))
#else
#define GETTIMEOFDAY(ptv) gettimeofday((ptv), (struct timezone *)NULL)
#endif
#endif


/* The log reader... */

PyDoc_STRVAR(logreader_close__doc__,
"close()\n"
"Close the log file, preventing additional records from being read.");

static PyObject *
logreader_close(LogReaderObject *self, PyObject *args)
{
    if (self->logfp != NULL) {
        fclose(self->logfp);
        self->logfp = NULL;
    }
    Py_INCREF(Py_None);

    return Py_None;
}

PyDoc_STRVAR(logreader_fileno__doc__,
"fileno() -> file descriptor\n"
"Returns the file descriptor for the log file, if open.\n"
"Raises ValueError if the log file is closed.");

static PyObject *
logreader_fileno(LogReaderObject *self)
{
    if (self->logfp == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "logreader's file object already closed");
        return NULL;
    }
    return PyInt_FromLong(fileno(self->logfp));
}


/* Log File Format
 * ---------------
 *
 * The log file consists of a sequence of variable-length records.
 * Each record is identified with a record type identifier in two
 * bits of the first byte.  The two bits are the "least significant"
 * bits of the byte.
 *
 * Low bits:    Opcode:        Meaning:
 *       0x00         ENTER     enter a frame
 *       0x01          EXIT     exit a frame
 *       0x02        LINENO     execution moved onto a different line
 *       0x03         OTHER     more bits are needed to deecode
 *
 * If the type is OTHER, the record is not packed so tightly, and the
 * remaining bits are used to disambiguate the record type.  These
 * records are not used as frequently so compaction is not an issue.
 * Each of the first three record types has a highly tailored
 * structure that allows it to be packed tightly.
 *
 * The OTHER records have the following identifiers:
 *
 * First byte:  Opcode:        Meaning:
 *       0x13      ADD_INFO     define a key/value pair
 *       0x23   DEFINE_FILE     define an int->filename mapping
 *       0x33    LINE_TIMES     indicates if LINENO events have tdeltas
 *       0x43   DEFINE_FUNC     define a (fileno,lineno)->funcname mapping
 *       0x53   FRAME_TIMES     indicates if ENTER/EXIT events have tdeltas
 *
 * Packed Integers
 *
 * "Packed integers" are non-negative integer values encoded as a
 * sequence of bytes.  Each byte is encoded such that the most
 * significant bit is set if the next byte is also part of the
 * integer.  Each byte provides bits to the least-significant end of
 * the result; the accumulated value must be shifted up to place the
 * new bits into the result.
 *
 * "Modified packed integers" are packed integers where only a portion
 * of the first byte is used.  In the rest of the specification, these
 * are referred to as "MPI(n,name)", where "n" is the number of bits
 * discarded from the least-signicant positions of the byte, and
 * "name" is a name being given to those "discarded" bits, since they
 * are a field themselves.
 *
 * ENTER records:
 *
 *      MPI(2,type)  fileno          -- type is 0x00
 *      PI           lineno
 *      PI           tdelta          -- iff frame times are enabled
 *
 * EXIT records
 *
 *      MPI(2,type)  tdelta          -- type is 0x01; tdelta will be 0
 *                                      if frame times are disabled
 *
 * LINENO records
 *
 *      MPI(2,type)  lineno          -- type is 0x02
 *      PI           tdelta          -- iff LINENO includes it
 *
 * ADD_INFO records
 *
 *      BYTE         type            -- always 0x13
 *      PI           len1            -- length of first string
 *      BYTE         string1[len1]   -- len1 bytes of string data
 *      PI           len2            -- length of second string
 *      BYTE         string2[len2]   -- len2 bytes of string data
 *
 * DEFINE_FILE records
 *
 *      BYTE         type            -- always 0x23
 *      PI           fileno
 *      PI           len             -- length of filename
 *      BYTE         filename[len]   -- len bytes of string data
 *
 * DEFINE_FUNC records
 *
 *      BYTE         type            -- always 0x43
 *      PI           fileno
 *      PI           lineno
 *      PI           len             -- length of funcname
 *      BYTE         funcname[len]   -- len bytes of string data
 *
 * LINE_TIMES records
 *
 * This record can be used only before the start of ENTER/EXIT/LINENO
 * records.  If have_tdelta is true, LINENO records will include the
 * tdelta field, otherwise it will be omitted.  If this record is not
 * given, LINENO records will not contain the tdelta field.
 *
 *      BYTE         type            -- always 0x33
 *      BYTE         have_tdelta     -- 0 if LINENO does *not* have
 *                                      timing information
 * FRAME_TIMES records
 *
 * This record can be used only before the start of ENTER/EXIT/LINENO
 * records.  If have_tdelta is true, ENTER and EXIT records will
 * include the tdelta field, otherwise it will be omitted.  If this
 * record is not given, ENTER and EXIT records will contain the tdelta
 * field.
 *
 *      BYTE         type            -- always 0x53
 *      BYTE         have_tdelta     -- 0 if ENTER/EXIT do *not* have
 *                                      timing information
 */

#define WHAT_ENTER        0x00
#define WHAT_EXIT         0x01
#define WHAT_LINENO       0x02
#define WHAT_OTHER        0x03  /* only used in decoding */
#define WHAT_ADD_INFO     0x13
#define WHAT_DEFINE_FILE  0x23
#define WHAT_LINE_TIMES   0x33
#define WHAT_DEFINE_FUNC  0x43
#define WHAT_FRAME_TIMES  0x53

#define ERR_NONE          0
#define ERR_EOF          -1
#define ERR_EXCEPTION    -2
#define ERR_BAD_RECTYPE  -3

#define PISIZE            (sizeof(int) + 1)
#define MPISIZE           (PISIZE + 1)

/* Maximum size of "normal" events -- nothing that contains string data */
#define MAXEVENTSIZE      (MPISIZE + PISIZE*2)


/* Unpack a packed integer; if "discard" is non-zero, unpack a modified
 * packed integer with "discard" discarded bits.
 */
static int
unpack_packed_int(LogReaderObject *self, int *pvalue, int discard)
{
    int c;
    int accum = 0;
    int bits = 0;
    int cont;

    do {
        /* read byte */
	if ((c = fgetc(self->logfp)) == EOF)
            return ERR_EOF;
        accum |= ((c & 0x7F) >> discard) << bits;
        bits += (7 - discard);
        cont = c & 0x80;
        discard = 0;
    } while (cont);

    *pvalue = accum;

    return 0;
}

/* Unpack a string, which is encoded as a packed integer giving the
 * length of the string, followed by the string data.
 */
static int
unpack_string(LogReaderObject *self, PyObject **pvalue)
{
    int i;
    int len;
    int err;
    int ch;
    char *buf;
    
    if ((err = unpack_packed_int(self, &len, 0)))
        return err;

    buf = (char *)malloc(len);
    if (!buf) {
	PyErr_NoMemory();
	return ERR_EXCEPTION;
    }

    for (i=0; i < len; i++) {
        ch = fgetc(self->logfp);
	buf[i] = ch;
        if (ch == EOF) {
            free(buf);
            return ERR_EOF;
        }
    }
    *pvalue = PyString_FromStringAndSize(buf, len);
    free(buf);
    if (*pvalue == NULL) {
        return ERR_EXCEPTION;
    }
    return 0;
}


static int
unpack_add_info(LogReaderObject *self)
{
    PyObject *key;
    PyObject *value = NULL;
    int err;

    err = unpack_string(self, &key);
    if (!err) {
        err = unpack_string(self, &value);
        if (err)
            Py_DECREF(key);
        else {
            PyObject *list = PyDict_GetItem(self->info, key);
            if (list == NULL) {
                list = PyList_New(0);
                if (list == NULL) {
                    err = ERR_EXCEPTION;
                    goto finally;
                }
                if (PyDict_SetItem(self->info, key, list)) {
                    Py_DECREF(list);
                    err = ERR_EXCEPTION;
                    goto finally;
                }
                Py_DECREF(list);
            }
            if (PyList_Append(list, value))
                err = ERR_EXCEPTION;
        }
    }
 finally:
    Py_XDECREF(key);
    Py_XDECREF(value);
    return err;
}


static void
eof_error(LogReaderObject *self)
{
    fclose(self->logfp);
    self->logfp = NULL;
    PyErr_SetString(PyExc_EOFError,
                    "end of file with incomplete profile record");
}

static PyObject *
logreader_tp_iternext(LogReaderObject *self)
{
    int c;
    int what;
    int err = ERR_NONE;
    int lineno = -1;
    int fileno = -1;
    int tdelta = -1;
    PyObject *s1 = NULL, *s2 = NULL;
    PyObject *result = NULL;
#if 0
    unsigned char b0, b1;
#endif

    if (self->logfp == NULL) {
        PyErr_SetString(ProfilerError,
                        "cannot iterate over closed LogReader object");
        return NULL;
    }

restart:
    /* decode the record type */
    if ((c = fgetc(self->logfp)) == EOF) {
        fclose(self->logfp);
        self->logfp = NULL;
        return NULL;
    }
    what = c & WHAT_OTHER;
    if (what == WHAT_OTHER)
        what = c; /* need all the bits for type */
    else
        ungetc(c, self->logfp); /* type byte includes packed int */

    switch (what) {
    case WHAT_ENTER:
        err = unpack_packed_int(self, &fileno, 2);
        if (!err) {
            err = unpack_packed_int(self, &lineno, 0);
            if (self->frametimings && !err)
                err = unpack_packed_int(self, &tdelta, 0);
        }
        break;
    case WHAT_EXIT:
        err = unpack_packed_int(self, &tdelta, 2);
        break;
    case WHAT_LINENO:
        err = unpack_packed_int(self, &lineno, 2);
        if (self->linetimings && !err)
            err = unpack_packed_int(self, &tdelta, 0);
        break;
    case WHAT_ADD_INFO:
        err = unpack_add_info(self);
        break;
    case WHAT_DEFINE_FILE:
        err = unpack_packed_int(self, &fileno, 0);
        if (!err) {
            err = unpack_string(self, &s1);
            if (!err) {
                Py_INCREF(Py_None);
                s2 = Py_None;
            }
        }
        break;
    case WHAT_DEFINE_FUNC:
        err = unpack_packed_int(self, &fileno, 0);
        if (!err) {
            err = unpack_packed_int(self, &lineno, 0);
            if (!err)
                err = unpack_string(self, &s1);
        }
        break;
    case WHAT_LINE_TIMES:
        if ((c = fgetc(self->logfp)) == EOF)
            err = ERR_EOF;
        else {
            self->linetimings = c ? 1 : 0;
	    goto restart;
	}
        break;
    case WHAT_FRAME_TIMES:
        if ((c = fgetc(self->logfp)) == EOF)
            err = ERR_EOF;
        else {
            self->frametimings = c ? 1 : 0;
	    goto restart;
	}
        break;
    default:
        err = ERR_BAD_RECTYPE;
    }
    if (err == ERR_BAD_RECTYPE) {
        PyErr_SetString(PyExc_ValueError,
                        "unknown record type in log file");
    }
    else if (err == ERR_EOF) {
        eof_error(self);
    }
    else if (!err) {
        result = PyTuple_New(4);
        if (result == NULL)
            return NULL;
        PyTuple_SET_ITEM(result, 0, PyInt_FromLong(what));
        PyTuple_SET_ITEM(result, 2, PyInt_FromLong(fileno));
        if (s1 == NULL)
            PyTuple_SET_ITEM(result, 1, PyInt_FromLong(tdelta));
        else
            PyTuple_SET_ITEM(result, 1, s1);
        if (s2 == NULL)
            PyTuple_SET_ITEM(result, 3, PyInt_FromLong(lineno));
        else
            PyTuple_SET_ITEM(result, 3, s2);
    }
    /* The only other case is err == ERR_EXCEPTION, in which case the
     * exception is already set.
     */
#if 0
    b0 = self->buffer[self->index];
    b1 = self->buffer[self->index + 1];
    if (b0 & 1) {
        /* This is a line-number event. */
        what = PyTrace_LINE;
        lineno = ((b0 & ~1) << 7) + b1;
        self->index += 2;
    }
    else {
        what = (b0 & 0x0E) >> 1;
        tdelta = ((b0 & 0xF0) << 4) + b1;
        if (what == PyTrace_CALL) {
            /* we know there's a 2-byte file ID & 2-byte line number */
            fileno = ((self->buffer[self->index + 2] << 8)
                      + self->buffer[self->index + 3]);
            lineno = ((self->buffer[self->index + 4] << 8)
                      + self->buffer[self->index + 5]);
            self->index += 6;
        }
        else
            self->index += 2;
    }
#endif
    return result;
}

static void
logreader_dealloc(LogReaderObject *self)
{
    if (self->logfp != NULL) {
        fclose(self->logfp);
        self->logfp = NULL;
    }
    Py_XDECREF(self->info);
    PyObject_Del(self);
}

static PyObject *
logreader_sq_item(LogReaderObject *self, Py_ssize_t index)
{
    PyObject *result = logreader_tp_iternext(self);
    if (result == NULL && !PyErr_Occurred()) {
        PyErr_SetString(PyExc_IndexError, "no more events in log");
        return NULL;
    }
    return result;
}

static void
do_stop(ProfilerObject *self);

static int
flush_data(ProfilerObject *self)
{
    /* Need to dump data to the log file... */
    size_t written = fwrite(self->buffer, 1, self->index, self->logfp);
    if (written == (size_t)self->index)
        self->index = 0;
    else {
        memmove(self->buffer, &self->buffer[written],
                self->index - written);
        self->index -= written;
        if (written == 0) {
            char *s = PyString_AsString(self->logfilename);
            PyErr_SetFromErrnoWithFilename(PyExc_IOError, s);
            do_stop(self);
            return -1;
        }
    }
    if (written > 0) {
        if (fflush(self->logfp)) {
            char *s = PyString_AsString(self->logfilename);
            PyErr_SetFromErrnoWithFilename(PyExc_IOError, s);
            do_stop(self);
            return -1;
        }
    }
    return 0;
}

static inline int
pack_packed_int(ProfilerObject *self, int value)
{
    unsigned char partial;

    do {
        partial = value & 0x7F;
        value >>= 7;
        if (value)
            partial |= 0x80;
        self->buffer[self->index] = partial;
        self->index++;
    } while (value);
    return 0;
}

/* Encode a modified packed integer, with a subfield of modsize bits
 * containing the value "subfield".  The value of subfield is not
 * checked to ensure it actually fits in modsize bits.
 */
static inline int
pack_modified_packed_int(ProfilerObject *self, int value,
                         int modsize, int subfield)
{
    const int maxvalues[] = {-1, 1, 3, 7, 15, 31, 63, 127};

    int bits = 7 - modsize;
    int partial = value & maxvalues[bits];
    unsigned char b = subfield | (partial << modsize);

    if (partial != value) {
        b |= 0x80;
        self->buffer[self->index] = b;
        self->index++;
        return pack_packed_int(self, value >> bits);
    }
    self->buffer[self->index] = b;
    self->index++;
    return 0;
}

static int
pack_string(ProfilerObject *self, const char *s, Py_ssize_t len)
{
    if (len + PISIZE + self->index >= BUFFERSIZE) {
        if (flush_data(self) < 0)
            return -1;
    }
    assert(len < INT_MAX);
    if (pack_packed_int(self, (int)len) < 0)
        return -1;
    memcpy(self->buffer + self->index, s, len);
    self->index += len;
    return 0;
}

static int
pack_add_info(ProfilerObject *self, const char *s1, const char *s2)
{
    Py_ssize_t len1 = strlen(s1);
    Py_ssize_t len2 = strlen(s2);

    if (len1 + len2 + PISIZE*2 + 1 + self->index >= BUFFERSIZE) {
        if (flush_data(self) < 0)
            return -1;
    }
    self->buffer[self->index] = WHAT_ADD_INFO;
    self->index++;
    if (pack_string(self, s1, len1) < 0)
        return -1;
    return pack_string(self, s2, len2);
}

static int
pack_define_file(ProfilerObject *self, int fileno, const char *filename)
{
    Py_ssize_t len = strlen(filename);

    if (len + PISIZE*2 + 1 + self->index >= BUFFERSIZE) {
        if (flush_data(self) < 0)
            return -1;
    }
    self->buffer[self->index] = WHAT_DEFINE_FILE;
    self->index++;
    if (pack_packed_int(self, fileno) < 0)
        return -1;
    return pack_string(self, filename, len);
}

static int
pack_define_func(ProfilerObject *self, int fileno, int lineno,
                 const char *funcname)
{
    Py_ssize_t len = strlen(funcname);

    if (len + PISIZE*3 + 1 + self->index >= BUFFERSIZE) {
        if (flush_data(self) < 0)
            return -1;
    }
    self->buffer[self->index] = WHAT_DEFINE_FUNC;
    self->index++;
    if (pack_packed_int(self, fileno) < 0)
        return -1;
    if (pack_packed_int(self, lineno) < 0)
        return -1;
    return pack_string(self, funcname, len);
}

static int
pack_line_times(ProfilerObject *self)
{
    if (2 + self->index >= BUFFERSIZE) {
        if (flush_data(self) < 0)
            return -1;
    }
    self->buffer[self->index] = WHAT_LINE_TIMES;
    self->buffer[self->index + 1] = self->linetimings ? 1 : 0;
    self->index += 2;
    return 0;
}

static int
pack_frame_times(ProfilerObject *self)
{
    if (2 + self->index >= BUFFERSIZE) {
        if (flush_data(self) < 0)
            return -1;
    }
    self->buffer[self->index] = WHAT_FRAME_TIMES;
    self->buffer[self->index + 1] = self->frametimings ? 1 : 0;
    self->index += 2;
    return 0;
}

static inline int
pack_enter(ProfilerObject *self, int fileno, int tdelta, int lineno)
{
    if (MPISIZE + PISIZE*2 + self->index >= BUFFERSIZE) {
        if (flush_data(self) < 0)
            return -1;
    }
    pack_modified_packed_int(self, fileno, 2, WHAT_ENTER);
    pack_packed_int(self, lineno);
    if (self->frametimings)
        return pack_packed_int(self, tdelta);
    else
        return 0;
}

static inline int
pack_exit(ProfilerObject *self, int tdelta)
{
    if (MPISIZE + self->index >= BUFFERSIZE) {
        if (flush_data(self) < 0)
            return -1;
    }
    if (self->frametimings)
        return pack_modified_packed_int(self, tdelta, 2, WHAT_EXIT);
    self->buffer[self->index] = WHAT_EXIT;
    self->index++;
    return 0;
}

static inline int
pack_lineno(ProfilerObject *self, int lineno)
{
    if (MPISIZE + self->index >= BUFFERSIZE) {
        if (flush_data(self) < 0)
            return -1;
    }
    return pack_modified_packed_int(self, lineno, 2, WHAT_LINENO);
}

static inline int
pack_lineno_tdelta(ProfilerObject *self, int lineno, int tdelta)
{
    if (MPISIZE + PISIZE + self->index >= BUFFERSIZE) {
        if (flush_data(self) < 0)
            return 0;
    }
    if (pack_modified_packed_int(self, lineno, 2, WHAT_LINENO) < 0)
        return -1;
    return pack_packed_int(self, tdelta);
}

static inline int
get_fileno(ProfilerObject *self, PyCodeObject *fcode)
{
    /* This is only used for ENTER events. */

    PyObject *obj;
    PyObject *dict;
    int fileno;

    obj = PyDict_GetItem(self->filemap, fcode->co_filename);
    if (obj == NULL) {
        /* first sighting of this file */
        dict = PyDict_New();
        if (dict == NULL) {
            return -1;
        }
        fileno = self->next_fileno;
        obj = Py_BuildValue("iN", fileno, dict);
        if (obj == NULL) {
            return -1;
        }
        if (PyDict_SetItem(self->filemap, fcode->co_filename, obj)) {
            Py_DECREF(obj);
            return -1;
        }
        self->next_fileno++;
        Py_DECREF(obj);
        if (pack_define_file(self, fileno,
                             PyString_AS_STRING(fcode->co_filename)) < 0)
            return -1;
    }
    else {
        /* already know this ID */
        fileno = PyInt_AS_LONG(PyTuple_GET_ITEM(obj, 0));
        dict = PyTuple_GET_ITEM(obj, 1);
    }
    /* make sure we save a function name for this (fileno, lineno) */
    obj = PyInt_FromLong(fcode->co_firstlineno);
    if (obj == NULL) {
        /* We just won't have it saved; too bad. */
        PyErr_Clear();
    }
    else {
        PyObject *name = PyDict_GetItem(dict, obj);
        if (name == NULL) {
            if (pack_define_func(self, fileno, fcode->co_firstlineno,
                                 PyString_AS_STRING(fcode->co_name)) < 0) {
                Py_DECREF(obj);
                return -1;
            }
            if (PyDict_SetItem(dict, obj, fcode->co_name)) {
                Py_DECREF(obj);
                return -1;
            }
        }
        Py_DECREF(obj);
    }
    return fileno;
}

static inline int
get_tdelta(ProfilerObject *self)
{
    int tdelta;
#ifdef MS_WINDOWS
    hs_time tv;
    hs_time diff;

    GETTIMEOFDAY(&tv);
    diff = tv - self->prev_timeofday;
    tdelta = (int)diff;
#else
    struct timeval tv;

    GETTIMEOFDAY(&tv);

    tdelta = tv.tv_usec - self->prev_timeofday.tv_usec;
    if (tv.tv_sec != self->prev_timeofday.tv_sec)
        tdelta += (tv.tv_sec - self->prev_timeofday.tv_sec) * 1000000;
#endif
    /* time can go backwards on some multiprocessor systems or by NTP */
    if (tdelta < 0)
        return 0;

    self->prev_timeofday = tv;
    return tdelta;
}


/* The workhorse:  the profiler callback function. */

static int
tracer_callback(ProfilerObject *self, PyFrameObject *frame, int what,
                PyObject *arg)
{
    int fileno;

    switch (what) {
    case PyTrace_CALL:
        fileno = get_fileno(self, frame->f_code);
        if (fileno < 0)
            return -1;
        return pack_enter(self, fileno,
                          self->frametimings ? get_tdelta(self) : -1,
                          frame->f_code->co_firstlineno);

    case PyTrace_RETURN:
        return pack_exit(self, get_tdelta(self));

    case PyTrace_LINE:  /* we only get these events if we asked for them */
        if (self->linetimings)
            return pack_lineno_tdelta(self, frame->f_lineno,
				      get_tdelta(self));
        else
            return pack_lineno(self, frame->f_lineno);

    default:
        /* ignore PyTrace_EXCEPTION */
        break;
    }
    return 0;
}


/* A couple of useful helper functions. */

#ifdef MS_WINDOWS
static LARGE_INTEGER frequency = {0, 0};
#endif

static unsigned long timeofday_diff = 0;
static unsigned long rusage_diff = 0;

static void
calibrate(void)
{
    hs_time tv1, tv2;

#ifdef MS_WINDOWS
    hs_time diff;
    QueryPerformanceFrequency(&frequency);
#endif

    GETTIMEOFDAY(&tv1);
    while (1) {
        GETTIMEOFDAY(&tv2);
#ifdef MS_WINDOWS
        diff = tv2 - tv1;
        if (diff != 0) {
            timeofday_diff = (unsigned long)diff;
            break;
        }
#else
        if (tv1.tv_sec != tv2.tv_sec || tv1.tv_usec != tv2.tv_usec) {
            if (tv1.tv_sec == tv2.tv_sec)
                timeofday_diff = tv2.tv_usec - tv1.tv_usec;
            else
                timeofday_diff = (1000000 - tv1.tv_usec) + tv2.tv_usec;
            break;
        }
#endif
    }
#if defined(MS_WINDOWS) || defined(PYOS_OS2) || \
    defined(__VMS) || defined (__QNX__)
    rusage_diff = -1;
#else
    {
        struct rusage ru1, ru2;

        getrusage(RUSAGE_SELF, &ru1);
        while (1) {
            getrusage(RUSAGE_SELF, &ru2);
            if (ru1.ru_utime.tv_sec != ru2.ru_utime.tv_sec) {
                rusage_diff = ((1000000 - ru1.ru_utime.tv_usec)
                               + ru2.ru_utime.tv_usec);
                break;
            }
            else if (ru1.ru_utime.tv_usec != ru2.ru_utime.tv_usec) {
                rusage_diff = ru2.ru_utime.tv_usec - ru1.ru_utime.tv_usec;
                break;
            }
            else if (ru1.ru_stime.tv_sec != ru2.ru_stime.tv_sec) {
                rusage_diff = ((1000000 - ru1.ru_stime.tv_usec)
                               + ru2.ru_stime.tv_usec);
                break;
            }
            else if (ru1.ru_stime.tv_usec != ru2.ru_stime.tv_usec) {
                rusage_diff = ru2.ru_stime.tv_usec - ru1.ru_stime.tv_usec;
                break;
            }
        }
    }
#endif
}

static void
do_start(ProfilerObject *self)
{
    self->active = 1;
    GETTIMEOFDAY(&self->prev_timeofday);
    if (self->lineevents)
        PyEval_SetTrace((Py_tracefunc) tracer_callback, (PyObject *)self);
    else
        PyEval_SetProfile((Py_tracefunc) tracer_callback, (PyObject *)self);
}

static void
do_stop(ProfilerObject *self)
{
    if (self->active) {
        self->active = 0;
        if (self->lineevents)
            PyEval_SetTrace(NULL, NULL);
        else
            PyEval_SetProfile(NULL, NULL);
    }
    if (self->index > 0) {
        /* Best effort to dump out any remaining data. */
        flush_data(self);
    }
}

static int
is_available(ProfilerObject *self)
{
    if (self->active) {
        PyErr_SetString(ProfilerError, "profiler already active");
        return 0;
    }
    if (self->logfp == NULL) {
        PyErr_SetString(ProfilerError, "profiler already closed");
        return 0;
    }
    return 1;
}


/* Profiler object interface methods. */

PyDoc_STRVAR(addinfo__doc__,
"addinfo(key, value)\n"
"Insert an ADD_INFO record into the log.");

static PyObject *
profiler_addinfo(ProfilerObject *self, PyObject *args)
{
    PyObject *result = NULL;
    char *key, *value;

    if (PyArg_ParseTuple(args, "ss:addinfo", &key, &value)) {
        if (self->logfp == NULL)
            PyErr_SetString(ProfilerError, "profiler already closed");
        else {
            if (pack_add_info(self, key, value) == 0) {
                result = Py_None;
                Py_INCREF(result);
            }
        }
    }
    return result;
}

PyDoc_STRVAR(close__doc__,
"close()\n"
"Shut down this profiler and close the log files, even if its active.");

static PyObject *
profiler_close(ProfilerObject *self)
{
    do_stop(self);
    if (self->logfp != NULL) {
        fclose(self->logfp);
        self->logfp = NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

#define fileno__doc__ logreader_fileno__doc__

static PyObject *
profiler_fileno(ProfilerObject *self)
{
    if (self->logfp == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "profiler's file object already closed");
        return NULL;
    }
    return PyInt_FromLong(fileno(self->logfp));
}

PyDoc_STRVAR(runcall__doc__,
"runcall(callable[, args[, kw]]) -> callable()\n"
"Profile a specific function call, returning the result of that call.");

static PyObject *
profiler_runcall(ProfilerObject *self, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *callargs = NULL;
    PyObject *callkw = NULL;
    PyObject *callable;

    if (PyArg_UnpackTuple(args, "runcall", 1, 3,
                         &callable, &callargs, &callkw)) {
        if (is_available(self)) {
            do_start(self);
            result = PyEval_CallObjectWithKeywords(callable, callargs, callkw);
            do_stop(self);
        }
    }
    return result;
}

PyDoc_STRVAR(runcode__doc__,
"runcode(code, globals[, locals])\n"
"Execute a code object while collecting profile data.  If locals is\n"
"omitted, globals is used for the locals as well.");

static PyObject *
profiler_runcode(ProfilerObject *self, PyObject *args)
{
    PyObject *result = NULL;
    PyCodeObject *code;
    PyObject *globals;
    PyObject *locals = NULL;

    if (PyArg_ParseTuple(args, "O!O!|O:runcode",
                         &PyCode_Type, &code,
                         &PyDict_Type, &globals,
                         &locals)) {
        if (is_available(self)) {
            if (locals == NULL || locals == Py_None)
                locals = globals;
            else if (!PyDict_Check(locals)) {
                PyErr_SetString(PyExc_TypeError,
                                "locals must be a dictionary or None");
                return NULL;
            }
            do_start(self);
            result = PyEval_EvalCode(code, globals, locals);
            do_stop(self);
#if 0
            if (!PyErr_Occurred()) {
                result = Py_None;
                Py_INCREF(result);
            }
#endif
        }
    }
    return result;
}

PyDoc_STRVAR(start__doc__,
"start()\n"
"Install this profiler for the current thread.");

static PyObject *
profiler_start(ProfilerObject *self, PyObject *args)
{
    PyObject *result = NULL;

    if (is_available(self)) {
        do_start(self);
        result = Py_None;
        Py_INCREF(result);
    }
    return result;
}

PyDoc_STRVAR(stop__doc__,
"stop()\n"
"Remove this profiler from the current thread.");

static PyObject *
profiler_stop(ProfilerObject *self, PyObject *args)
{
    PyObject *result = NULL;

    if (!self->active)
        PyErr_SetString(ProfilerError, "profiler not active");
    else {
        do_stop(self);
        result = Py_None;
        Py_INCREF(result);
    }
    return result;
}


/* Python API support. */

static void
profiler_dealloc(ProfilerObject *self)
{
    do_stop(self);
    if (self->logfp != NULL)
        fclose(self->logfp);
    Py_XDECREF(self->filemap);
    Py_XDECREF(self->logfilename);
    PyObject_Del((PyObject *)self);
}

static PyMethodDef profiler_methods[] = {
    {"addinfo", (PyCFunction)profiler_addinfo, METH_VARARGS, addinfo__doc__},
    {"close",   (PyCFunction)profiler_close,   METH_NOARGS,  close__doc__},
    {"fileno",  (PyCFunction)profiler_fileno,  METH_NOARGS,  fileno__doc__},
    {"runcall", (PyCFunction)profiler_runcall, METH_VARARGS, runcall__doc__},
    {"runcode", (PyCFunction)profiler_runcode, METH_VARARGS, runcode__doc__},
    {"start",   (PyCFunction)profiler_start,   METH_NOARGS,  start__doc__},
    {"stop",    (PyCFunction)profiler_stop,    METH_NOARGS,  stop__doc__},
    {NULL, NULL}
};

static PyMemberDef profiler_members[] = {
    {"frametimings", T_LONG, offsetof(ProfilerObject, linetimings), READONLY},
    {"lineevents",   T_LONG, offsetof(ProfilerObject, lineevents), READONLY},
    {"linetimings",  T_LONG, offsetof(ProfilerObject, linetimings), READONLY},
    {NULL}
};

static PyObject *
profiler_get_closed(ProfilerObject *self, void *closure)
{
    PyObject *result = (self->logfp == NULL) ? Py_True : Py_False;
    Py_INCREF(result);
    return result;
}

static PyGetSetDef profiler_getsets[] = {
    {"closed", (getter)profiler_get_closed, NULL,
     PyDoc_STR("True if the profiler's output file has already been closed.")},
    {NULL}
};


PyDoc_STRVAR(profiler_object__doc__,
"High-performance profiler object.\n"
"\n"
"Methods:\n"
"\n"
"close():      Stop the profiler and close the log files.\n"
"fileno():     Returns the file descriptor of the log file.\n"
"runcall():    Run a single function call with profiling enabled.\n"
"runcode():    Execute a code object with profiling enabled.\n"
"start():      Install the profiler and return.\n"
"stop():       Remove the profiler.\n"
"\n"
"Attributes (read-only):\n"
"\n"
"closed:       True if the profiler has already been closed.\n"
"frametimings: True if ENTER/EXIT events collect timing information.\n"
"lineevents:   True if line events are reported to the profiler.\n"
"linetimings:  True if line events collect timing information.");

static PyTypeObject ProfilerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hotshot.ProfilerType",		/* tp_name		*/
    (int) sizeof(ProfilerObject),	/* tp_basicsize		*/
    0,					/* tp_itemsize		*/
    (destructor)profiler_dealloc,	/* tp_dealloc		*/
    0,					/* tp_print		*/
    0,					/* tp_getattr		*/
    0,					/* tp_setattr		*/
    0,					/* tp_compare		*/
    0,					/* tp_repr		*/
    0,					/* tp_as_number		*/
    0,					/* tp_as_sequence	*/
    0,					/* tp_as_mapping	*/
    0,					/* tp_hash		*/
    0,					/* tp_call		*/
    0,					/* tp_str		*/
    PyObject_GenericGetAttr,		/* tp_getattro		*/
    0,					/* tp_setattro		*/
    0,					/* tp_as_buffer		*/
    Py_TPFLAGS_DEFAULT,			/* tp_flags		*/
    profiler_object__doc__,		/* tp_doc		*/
    0,					/* tp_traverse		*/
    0,					/* tp_clear		*/
    0,					/* tp_richcompare	*/
    0,					/* tp_weaklistoffset	*/
    0,					/* tp_iter		*/
    0,					/* tp_iternext		*/
    profiler_methods,			/* tp_methods		*/
    profiler_members,			/* tp_members		*/
    profiler_getsets,			/* tp_getset		*/
    0,					/* tp_base		*/
    0,					/* tp_dict		*/
    0,					/* tp_descr_get		*/
    0,					/* tp_descr_set		*/
};


static PyMethodDef logreader_methods[] = {
    {"close",   (PyCFunction)logreader_close,  METH_NOARGS,
     logreader_close__doc__},
    {"fileno",  (PyCFunction)logreader_fileno, METH_NOARGS,
     logreader_fileno__doc__},
    {NULL, NULL}
};

static PyMemberDef logreader_members[] = {
    {"info", T_OBJECT, offsetof(LogReaderObject, info), RO,
     PyDoc_STR("Dictionary mapping informational keys to lists of values.")},
    {NULL}
};


PyDoc_STRVAR(logreader__doc__,
"logreader(filename) --> log-iterator\n\
Create a log-reader for the timing information file.");

static PySequenceMethods logreader_as_sequence = {
    0,					/* sq_length */
    0,					/* sq_concat */
    0,					/* sq_repeat */
    (ssizeargfunc)logreader_sq_item,	/* sq_item */
    0,					/* sq_slice */
    0,					/* sq_ass_item */
    0,					/* sq_ass_slice */
    0,					/* sq_contains */
    0,					/* sq_inplace_concat */
    0,					/* sq_inplace_repeat */
};

static PyObject *
logreader_get_closed(LogReaderObject *self, void *closure)
{
    PyObject *result = (self->logfp == NULL) ? Py_True : Py_False;
    Py_INCREF(result);
    return result;
}

static PyGetSetDef logreader_getsets[] = {
    {"closed", (getter)logreader_get_closed, NULL,
     PyDoc_STR("True if the logreader's input file has already been closed.")},
    {NULL}
};

static PyTypeObject LogReaderType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hotshot.LogReaderType",		/* tp_name		*/
    (int) sizeof(LogReaderObject),	/* tp_basicsize		*/
    0,					/* tp_itemsize		*/
    (destructor)logreader_dealloc,	/* tp_dealloc		*/
    0,					/* tp_print		*/
    0,					/* tp_getattr		*/
    0,					/* tp_setattr		*/
    0,					/* tp_compare		*/
    0,					/* tp_repr		*/
    0,					/* tp_as_number		*/
    &logreader_as_sequence,		/* tp_as_sequence	*/
    0,					/* tp_as_mapping	*/
    0,					/* tp_hash		*/
    0,					/* tp_call		*/
    0,					/* tp_str		*/
    PyObject_GenericGetAttr,		/* tp_getattro		*/
    0,					/* tp_setattro		*/
    0,					/* tp_as_buffer		*/
    Py_TPFLAGS_DEFAULT,			/* tp_flags		*/
    logreader__doc__,			/* tp_doc		*/
    0,					/* tp_traverse		*/
    0,					/* tp_clear		*/
    0,					/* tp_richcompare	*/
    0,					/* tp_weaklistoffset	*/
    PyObject_SelfIter,			/* tp_iter		*/
    (iternextfunc)logreader_tp_iternext,/* tp_iternext		*/
    logreader_methods,			/* tp_methods		*/
    logreader_members,			/* tp_members		*/
    logreader_getsets,			/* tp_getset		*/
    0,					/* tp_base		*/
    0,					/* tp_dict		*/
    0,					/* tp_descr_get		*/
    0,					/* tp_descr_set		*/
};

static PyObject *
hotshot_logreader(PyObject *unused, PyObject *args)
{
    LogReaderObject *self = NULL;
    char *filename;
    int c;
    int err = 0;

    if (PyArg_ParseTuple(args, "s:logreader", &filename)) {
        self = PyObject_New(LogReaderObject, &LogReaderType);
        if (self != NULL) {
            self->frametimings = 1;
            self->linetimings = 0;
            self->info = NULL;
            self->logfp = fopen(filename, "rb");
            if (self->logfp == NULL) {
                PyErr_SetFromErrnoWithFilename(PyExc_IOError, filename);
                Py_DECREF(self);
                self = NULL;
                goto finally;
            }
            self->info = PyDict_New();
            if (self->info == NULL) {
                Py_DECREF(self);
                goto finally;
            }
            /* read initial info */
            for (;;) {
                if ((c = fgetc(self->logfp)) == EOF) {
                    eof_error(self);
                    break;
                }
                if (c != WHAT_ADD_INFO) {
                    ungetc(c, self->logfp);
                    break;
                }
                err = unpack_add_info(self);
                if (err) {
                    if (err == ERR_EOF)
                        eof_error(self);
                    else
                        PyErr_SetString(PyExc_RuntimeError,
                                        "unexpected error");
                    break;
                }
            }
        }
    }
 finally:
    return (PyObject *) self;
}


/* Return a Python string that represents the version number without the
 * extra cruft added by revision control, even if the right options were
 * given to the "cvs export" command to make it not include the extra
 * cruft.
 */
static char *
get_version_string(void)
{
    static char *rcsid = "$Revision$";
    char *rev = rcsid;
    char *buffer;
    int i = 0;

    while (*rev && !isdigit(Py_CHARMASK(*rev)))
        ++rev;
    while (rev[i] != ' ' && rev[i] != '\0')
        ++i;
    buffer = (char *)malloc(i + 1);
    if (buffer != NULL) {
        memmove(buffer, rev, i);
        buffer[i] = '\0';
    }
    return buffer;
}

/* Write out a RFC 822-style header with various useful bits of
 * information to make the output easier to manage.
 */
static int
write_header(ProfilerObject *self)
{
    char *buffer;
    char cwdbuffer[PATH_MAX];
    PyObject *temp;
    Py_ssize_t i, len;

    buffer = get_version_string();
    if (buffer == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    pack_add_info(self, "hotshot-version", buffer);
    pack_add_info(self, "requested-frame-timings",
                  (self->frametimings ? "yes" : "no"));
    pack_add_info(self, "requested-line-events",
                  (self->lineevents ? "yes" : "no"));
    pack_add_info(self, "requested-line-timings",
                  (self->linetimings ? "yes" : "no"));
    pack_add_info(self, "platform", Py_GetPlatform());
    pack_add_info(self, "executable", Py_GetProgramFullPath());
    free(buffer);
    buffer = (char *) Py_GetVersion();
    if (buffer == NULL)
        PyErr_Clear();
    else
        pack_add_info(self, "executable-version", buffer);

#ifdef MS_WINDOWS
    PyOS_snprintf(cwdbuffer, sizeof(cwdbuffer), "%I64d", frequency.QuadPart);
    pack_add_info(self, "reported-performance-frequency", cwdbuffer);
#else
    PyOS_snprintf(cwdbuffer, sizeof(cwdbuffer), "%lu", rusage_diff);
    pack_add_info(self, "observed-interval-getrusage", cwdbuffer);
    PyOS_snprintf(cwdbuffer, sizeof(cwdbuffer), "%lu", timeofday_diff);
    pack_add_info(self, "observed-interval-gettimeofday", cwdbuffer);
#endif

    pack_add_info(self, "current-directory",
                  getcwd(cwdbuffer, sizeof cwdbuffer));

    temp = PySys_GetObject("path");
    if (temp == NULL || !PyList_Check(temp)) {
	PyErr_SetString(PyExc_RuntimeError, "sys.path must be a list");
    	return -1;
    }
    len = PyList_GET_SIZE(temp);
    for (i = 0; i < len; ++i) {
        PyObject *item = PyList_GET_ITEM(temp, i);
        buffer = PyString_AsString(item);
        if (buffer == NULL) {
            pack_add_info(self, "sys-path-entry", "<non-string-path-entry>");
            PyErr_Clear();
        }
        else {
            pack_add_info(self, "sys-path-entry", buffer);
        }
    }
    pack_frame_times(self);
    pack_line_times(self);

    return 0;
}

PyDoc_STRVAR(profiler__doc__,
"profiler(logfilename[, lineevents[, linetimes]]) -> profiler\n\
Create a new profiler object.");

static PyObject *
hotshot_profiler(PyObject *unused, PyObject *args)
{
    char *logfilename;
    ProfilerObject *self = NULL;
    int lineevents = 0;
    int linetimings = 1;

    if (PyArg_ParseTuple(args, "s|ii:profiler", &logfilename,
                         &lineevents, &linetimings)) {
        self = PyObject_New(ProfilerObject, &ProfilerType);
        if (self == NULL)
            return NULL;
        self->frametimings = 1;
        self->lineevents = lineevents ? 1 : 0;
        self->linetimings = (lineevents && linetimings) ? 1 : 0;
        self->index = 0;
        self->active = 0;
        self->next_fileno = 0;
        self->logfp = NULL;
        self->logfilename = PyTuple_GET_ITEM(args, 0);
        Py_INCREF(self->logfilename);
        self->filemap = PyDict_New();
        if (self->filemap == NULL) {
            Py_DECREF(self);
            return NULL;
        }
        self->logfp = fopen(logfilename, "wb");
        if (self->logfp == NULL) {
            Py_DECREF(self);
            PyErr_SetFromErrnoWithFilename(PyExc_IOError, logfilename);
            return NULL;
        }
        if (timeofday_diff == 0) {
            /* Run this several times since sometimes the first
             * doesn't give the lowest values, and we're really trying
             * to determine the lowest.
             */
            calibrate();
            calibrate();
            calibrate();
        }
        if (write_header(self)) {
            /* some error occurred, exception has been set */
            Py_DECREF(self);
            self = NULL;
        }
    }
    return (PyObject *) self;
}

PyDoc_STRVAR(coverage__doc__,
"coverage(logfilename) -> profiler\n\
Returns a profiler that doesn't collect any timing information, which is\n\
useful in building a coverage analysis tool.");

static PyObject *
hotshot_coverage(PyObject *unused, PyObject *args)
{
    char *logfilename;
    PyObject *result = NULL;

    if (PyArg_ParseTuple(args, "s:coverage", &logfilename)) {
        result = hotshot_profiler(unused, args);
        if (result != NULL) {
            ProfilerObject *self = (ProfilerObject *) result;
            self->frametimings = 0;
            self->linetimings = 0;
            self->lineevents = 1;
        }
    }
    return result;
}

PyDoc_VAR(resolution__doc__) = 
#ifdef MS_WINDOWS
PyDoc_STR(
"resolution() -> (performance-counter-ticks, update-frequency)\n"
"Return the resolution of the timer provided by the QueryPerformanceCounter()\n"
"function.  The first value is the smallest observed change, and the second\n"
"is the result of QueryPerformanceFrequency()."
)
#else
PyDoc_STR(
"resolution() -> (gettimeofday-usecs, getrusage-usecs)\n"
"Return the resolution of the timers provided by the gettimeofday() and\n"
"getrusage() system calls, or -1 if the call is not supported."
)
#endif
;

static PyObject *
hotshot_resolution(PyObject *self, PyObject *unused)
{
    if (timeofday_diff == 0) {
        calibrate();
        calibrate();
        calibrate();
    }
#ifdef MS_WINDOWS
    return Py_BuildValue("ii", timeofday_diff, frequency.LowPart);
#else
    return Py_BuildValue("ii", timeofday_diff, rusage_diff);
#endif
}


static PyMethodDef functions[] = {
    {"coverage",   hotshot_coverage,   METH_VARARGS, coverage__doc__},
    {"profiler",   hotshot_profiler,   METH_VARARGS, profiler__doc__},
    {"logreader",  hotshot_logreader,  METH_VARARGS, logreader__doc__},
    {"resolution", hotshot_resolution, METH_NOARGS,  resolution__doc__},
    {NULL, NULL}
};


void
init_hotshot(void)
{
    PyObject *module;

    Py_TYPE(&LogReaderType) = &PyType_Type;
    Py_TYPE(&ProfilerType) = &PyType_Type;
    module = Py_InitModule("_hotshot", functions);
    if (module != NULL) {
        char *s = get_version_string();

        PyModule_AddStringConstant(module, "__version__", s);
        free(s);
        Py_INCREF(&LogReaderType);
        PyModule_AddObject(module, "LogReaderType",
                           (PyObject *)&LogReaderType);
        Py_INCREF(&ProfilerType);
        PyModule_AddObject(module, "ProfilerType",
                           (PyObject *)&ProfilerType);

        if (ProfilerError == NULL)
            ProfilerError = PyErr_NewException("hotshot.ProfilerError",
                                               NULL, NULL);
        if (ProfilerError != NULL) {
            Py_INCREF(ProfilerError);
            PyModule_AddObject(module, "ProfilerError", ProfilerError);
        }
        PyModule_AddIntConstant(module, "WHAT_ENTER", WHAT_ENTER);
        PyModule_AddIntConstant(module, "WHAT_EXIT", WHAT_EXIT);
        PyModule_AddIntConstant(module, "WHAT_LINENO", WHAT_LINENO);
        PyModule_AddIntConstant(module, "WHAT_OTHER", WHAT_OTHER);
        PyModule_AddIntConstant(module, "WHAT_ADD_INFO", WHAT_ADD_INFO);
        PyModule_AddIntConstant(module, "WHAT_DEFINE_FILE", WHAT_DEFINE_FILE);
        PyModule_AddIntConstant(module, "WHAT_DEFINE_FUNC", WHAT_DEFINE_FUNC);
        PyModule_AddIntConstant(module, "WHAT_LINE_TIMES", WHAT_LINE_TIMES);
    }
}
