/*
 * This is the High Performance Python Profiler portion of HotShot.
 */

#include "Python.h"
#include "compile.h"
#include "eval.h"
#include "frameobject.h"
#include "structmember.h"

/*
 * Which timer to use should be made more configurable, but that should not
 * be difficult.  This will do for now.
 */
#ifdef MS_WIN32
#include <windows.h>
#include <largeint.h>
#include <direct.h>    /* for getcwd() */
typedef __int64 hs_time;
#define GETTIMEOFDAY(P_HS_TIME) \
	{ LARGE_INTEGER _temp; \
	  QueryPerformanceCounter(&_temp); \
	  *(P_HS_TIME) = _temp.QuadPart; }
	  

#else
#ifndef HAVE_GETTIMEOFDAY
#error "This module requires gettimeofday() on non-Windows platforms!"
#endif
#ifdef macintosh
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

#ifdef macintosh
#define PATH_MAX 254
#endif

#ifndef PATH_MAX
#   ifdef MAX_PATH
#       define PATH_MAX MAX_PATH
#   else
#       error "Need a defn. for PATH_MAX in _hotshot.c"
#   endif
#endif

typedef struct {
    PyObject_HEAD
    PyObject *filemap;
    PyObject *logfilename;
    int index;
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
    int filled;
    int index;
    int linetimings;
    int frametimings;
    unsigned char buffer[BUFFERSIZE];
} LogReaderObject;

static PyObject * ProfilerError = NULL;


#ifndef MS_WIN32
#ifdef GETTIMEOFDAY_NO_TZ
#define GETTIMEOFDAY(ptv) gettimeofday((ptv))
#else
#define GETTIMEOFDAY(ptv) gettimeofday((ptv), (struct timezone *)NULL)
#endif
#endif


/* The log reader... */

static char logreader_close__doc__[] =
"close()\n"
"Close the log file, preventing additional records from being read.";

static PyObject *
logreader_close(LogReaderObject *self, PyObject *args)
{
    PyObject *result = NULL;
    if (PyArg_ParseTuple(args, ":close")) {
        if (self->logfp != NULL) {
            fclose(self->logfp);
            self->logfp = NULL;
        }
        result = Py_None;
        Py_INCREF(result);
    }
    return result;
}

#if Py_TPFLAGS_HAVE_ITER
/* This is only used if the interpreter has iterator support; the
 * iternext handler is also used as a helper for other functions, so
 * does not need to be included in this conditional section.
 */
static PyObject *
logreader_tp_iter(LogReaderObject *self)
{
    Py_INCREF(self);
    return (PyObject *) self;
}
#endif


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
 *       0x02        LINENO     SET_LINENO instruction was executed
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
    int accum = 0;
    int bits = 0;
    int index = self->index;
    int cont;

    do {
        if (index >= self->filled)
            return ERR_EOF;
        /* read byte */
        accum |= ((self->buffer[index] & 0x7F) >> discard) << bits;
        bits += (7 - discard);
        cont = self->buffer[index] & 0x80;
        /* move to next */
        discard = 0;
        index++;
    } while (cont);

    /* save state */
    self->index = index;
    *pvalue = accum;

    return 0;
}

/* Unpack a string, which is encoded as a packed integer giving the
 * length of the string, followed by the string data.
 */
static int
unpack_string(LogReaderObject *self, PyObject **pvalue)
{
    int len;
    int oldindex = self->index;
    int err = unpack_packed_int(self, &len, 0);

    if (!err) {
        /* need at least len bytes in buffer */
        if (len > (self->filled - self->index)) {
            self->index = oldindex;
            err = ERR_EOF;
        }
        else {
            *pvalue = PyString_FromStringAndSize((char *)self->buffer + self->index,
                                                 len);
            if (*pvalue == NULL) {
                self->index = oldindex;
                err = ERR_EXCEPTION;
            }
            else
                self->index += len;
        }
    }
    return err;
}


static int
unpack_add_info(LogReaderObject *self, int skip_opcode)
{
    PyObject *key;
    PyObject *value = NULL;
    int err;

    if (skip_opcode) {
        if (self->buffer[self->index] != WHAT_ADD_INFO)
            return ERR_BAD_RECTYPE;
        self->index++;
    }
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
                    err = ERR_EXCEPTION;
                    goto finally;
                }
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
logreader_refill(LogReaderObject *self)
{
    int needed;
    size_t res;

    if (self->index) {
        memmove(self->buffer, &self->buffer[self->index],
                self->filled - self->index);
        self->filled = self->filled - self->index;
        self->index = 0;
    }
    needed = BUFFERSIZE - self->filled;
    if (needed > 0) {
        res = fread(&self->buffer[self->filled], 1, needed, self->logfp);
        self->filled += res;
    }
}

static void
eof_error(void)
{
    PyErr_SetString(PyExc_EOFError,
                    "end of file with incomplete profile record");
}

static PyObject *
logreader_tp_iternext(LogReaderObject *self)
{
    int what, oldindex;
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
    if ((self->filled - self->index) < MAXEVENTSIZE)
        logreader_refill(self);

    /* end of input */
    if (self->filled == 0)
        return NULL;

    oldindex = self->index;

    /* decode the record type */
    what = self->buffer[self->index] & WHAT_OTHER;
    if (what == WHAT_OTHER) {
        what = self->buffer[self->index];
        self->index++;
    }
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
        err = unpack_add_info(self, 0);
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
        if (self->index >= self->filled)
            err = ERR_EOF;
        else {
            self->linetimings = self->buffer[self->index] ? 1 : 0;
            self->index++;
            goto restart;
        }
        break;
    case WHAT_FRAME_TIMES:
        if (self->index >= self->filled)
            err = ERR_EOF;
        else {
            self->frametimings = self->buffer[self->index] ? 1 : 0;
            self->index++;
            goto restart;
        }
        break;
    default:
        err = ERR_BAD_RECTYPE;
    }
    if (err == ERR_EOF && oldindex != 0) {
        /* It looks like we ran out of data before we had it all; this
         * could easily happen with large packed integers or string
         * data.  Try forcing the buffer to be re-filled before failing.
         */
        err = ERR_NONE;
        logreader_refill(self);
    }
    if (err == ERR_BAD_RECTYPE) {
        PyErr_SetString(PyExc_ValueError,
                        "unknown record type in log file");
    }
    else if (err == ERR_EOF) {
        /* Could not avoid end-of-buffer error. */
        eof_error();
    }
    else if (!err) {
        result = PyTuple_New(4);
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
    PyObject_Del(self);
}

static PyObject *
logreader_sq_item(LogReaderObject *self, int index)
{
    PyObject *result = logreader_tp_iternext(self);
    if (result == NULL && !PyErr_Occurred()) {
        PyErr_SetString(PyExc_IndexError, "no more events in log");
        return NULL;
    }
    return result;
}

static char next__doc__[] =
"next() -> event-info\n"
"Return the next event record from the log file.";

static PyObject *
logreader_next(LogReaderObject *self, PyObject *args)
{
    PyObject *result = NULL;

    if (PyArg_ParseTuple(args, ":next")) {
        result = logreader_tp_iternext(self);
        /* XXX return None if there's nothing left */
        /* tp_iternext does the right thing, though */
        if (result == NULL && !PyErr_Occurred()) {
            result = Py_None;
            Py_INCREF(result);
        }
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
pack_string(ProfilerObject *self, const char *s, int len)
{
    if (len + PISIZE + self->index >= BUFFERSIZE) {
        if (flush_data(self) < 0)
            return -1;
    }
    if (pack_packed_int(self, len) < 0)
        return -1;
    memcpy(self->buffer + self->index, s, len);
    self->index += len;
    return 0;
}

static int
pack_add_info(ProfilerObject *self, const char *s1, const char *s2)
{
    int len1 = strlen(s1);
    int len2 = strlen(s2);

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
    int len = strlen(filename);

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
    int len = strlen(funcname);

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
                                 PyString_AS_STRING(fcode->co_name)) < 0)
                return -1;
            if (PyDict_SetItem(dict, obj, fcode->co_name))
                return -1;
        }
    }
    return fileno;
}

static inline int
get_tdelta(ProfilerObject *self)
{
    int tdelta;
#ifdef MS_WIN32
    hs_time tv;
    hs_time diff;

    GETTIMEOFDAY(&tv);
    diff = tv - self->prev_timeofday;
    tdelta = (int)diff;
#else
    struct timeval tv;

    GETTIMEOFDAY(&tv);

    if (tv.tv_sec == self->prev_timeofday.tv_sec)
        tdelta = tv.tv_usec - self->prev_timeofday.tv_usec;
    else
        tdelta = ((tv.tv_sec - self->prev_timeofday.tv_sec) * 1000000
                  + tv.tv_usec);
#endif
    self->prev_timeofday = tv;
    return tdelta;
}


/* The workhorse:  the profiler callback function. */

static int
profiler_callback(ProfilerObject *self, PyFrameObject *frame, int what,
                  PyObject *arg)
{
    int tdelta = -1;
    int fileno;

    if (self->frametimings)
        tdelta = get_tdelta(self);
    switch (what) {
    case PyTrace_CALL:
        fileno = get_fileno(self, frame->f_code);
        if (fileno < 0)
            return -1;
        if (pack_enter(self, fileno, tdelta,
                       frame->f_code->co_firstlineno) < 0)
            return -1;
        break;
    case PyTrace_RETURN:
        if (pack_exit(self, tdelta) < 0)
            return -1;
        break;
    default:
        /* should never get here */
        break;
    }
    return 0;
}


/* Alternate callback when we want PyTrace_LINE events */

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

    case PyTrace_LINE:
        if (self->linetimings)
            return pack_lineno_tdelta(self, frame->f_lineno, get_tdelta(self));
        else
            return pack_lineno(self, frame->f_lineno);

    default:
        /* ignore PyTrace_EXCEPTION */
        break;
    }
    return 0;
}


/* A couple of useful helper functions. */

#ifdef MS_WIN32
static LARGE_INTEGER frequency = {0, 0};
#endif

static unsigned long timeofday_diff = 0;
static unsigned long rusage_diff = 0;

static void
calibrate(void)
{
    hs_time tv1, tv2;

#ifdef MS_WIN32
    hs_time diff;
    QueryPerformanceFrequency(&frequency);
#endif

    GETTIMEOFDAY(&tv1);
    while (1) {
        GETTIMEOFDAY(&tv2);
#ifdef MS_WIN32
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
#if defined(MS_WIN32) || defined(macintosh)
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
        PyEval_SetProfile((Py_tracefunc) profiler_callback, (PyObject *)self);
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

static char addinfo__doc__[] =
"addinfo(key, value)\n"
"Insert an ADD_INFO record into the log.";

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

static char close__doc__[] =
"close()\n"
"Shut down this profiler and close the log files, even if its active.";

static PyObject *
profiler_close(ProfilerObject *self, PyObject *args)
{
    PyObject *result = NULL;

    if (PyArg_ParseTuple(args, ":close")) {
        do_stop(self);
        if (self->logfp != NULL) {
            fclose(self->logfp);
            self->logfp = NULL;
        }
        Py_INCREF(Py_None);
        result = Py_None;
    }
    return result;
}

static char runcall__doc__[] =
"runcall(callable[, args[, kw]]) -> callable()\n"
"Profile a specific function call, returning the result of that call.";

static PyObject *
profiler_runcall(ProfilerObject *self, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *callargs = NULL;
    PyObject *callkw = NULL;
    PyObject *callable;

    if (PyArg_ParseTuple(args, "O|OO:runcall",
                         &callable, &callargs, &callkw)) {
        if (is_available(self)) {
            do_start(self);
            result = PyEval_CallObjectWithKeywords(callable, callargs, callkw);
            do_stop(self);
        }
    }
    return result;
}

static char runcode__doc__[] =
"runcode(code, globals[, locals])\n"
"Execute a code object while collecting profile data.  If locals is\n"
"omitted, globals is used for the locals as well.";

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

static char start__doc__[] =
"start()\n"
"Install this profiler for the current thread.";

static PyObject *
profiler_start(ProfilerObject *self, PyObject *args)
{
    PyObject *result = NULL;

    if (PyArg_ParseTuple(args, ":start")) {
        if (is_available(self)) {
            do_start(self);
            result = Py_None;
            Py_INCREF(result);
        }
    }
    return result;
}

static char stop__doc__[] =
"stop()\n"
"Remove this profiler from the current thread.";

static PyObject *
profiler_stop(ProfilerObject *self, PyObject *args)
{
    PyObject *result = NULL;

    if (PyArg_ParseTuple(args, ":stop")) {
        if (!self->active)
            PyErr_SetString(ProfilerError, "profiler not active");
        else {
            do_stop(self);
            result = Py_None;
            Py_INCREF(result);
        }
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

/* Always use METH_VARARGS even though some of these could be METH_NOARGS;
 * this allows us to maintain compatibility with Python versions < 2.2
 * more easily, requiring only the changes to the dispatcher to be made.
 */
static PyMethodDef profiler_methods[] = {
    {"addinfo", (PyCFunction)profiler_addinfo, METH_VARARGS, addinfo__doc__},
    {"close",   (PyCFunction)profiler_close,   METH_VARARGS, close__doc__},
    {"runcall", (PyCFunction)profiler_runcall, METH_VARARGS, runcall__doc__},
    {"runcode", (PyCFunction)profiler_runcode, METH_VARARGS, runcode__doc__},
    {"start",   (PyCFunction)profiler_start,   METH_VARARGS, start__doc__},
    {"stop",    (PyCFunction)profiler_stop,    METH_VARARGS, stop__doc__},
    {NULL, NULL}
};

/* Use a table even though there's only one "simple" member; this allows
 * __members__ and therefore dir() to work.
 */
static struct memberlist profiler_members[] = {
    {"closed",       T_INT,  -1, READONLY},
    {"frametimings", T_LONG, offsetof(ProfilerObject, linetimings), READONLY},
    {"lineevents",   T_LONG, offsetof(ProfilerObject, lineevents), READONLY},
    {"linetimings",  T_LONG, offsetof(ProfilerObject, linetimings), READONLY},
    {NULL}
};

static PyObject *
profiler_getattr(ProfilerObject *self, char *name)
{
    PyObject *result;
    if (strcmp(name, "closed") == 0) {
        result = (self->logfp == NULL) ? Py_True : Py_False;
        Py_INCREF(result);
    }
    else {
        result = PyMember_Get((char *)self, profiler_members, name);
        if (result == NULL) {
            PyErr_Clear();
            result = Py_FindMethod(profiler_methods, (PyObject *)self, name);
        }
    }
    return result;
}


static char profiler_object__doc__[] =
"High-performance profiler object.\n"
"\n"
"Methods:\n"
"\n"
"close():      Stop the profiler and close the log files.\n"
"runcall():    Run a single function call with profiling enabled.\n"
"runcode():    Execute a code object with profiling enabled.\n"
"start():      Install the profiler and return.\n"
"stop():       Remove the profiler.\n"
"\n"
"Attributes (read-only):\n"
"\n"
"closed:       True if the profiler has already been closed.\n"
"frametimings: True if ENTER/EXIT events collect timing information.\n"
"lineevents:   True if SET_LINENO events are reported to the profiler.\n"
"linetimings:  True if SET_LINENO events collect timing information.";

static PyTypeObject ProfilerType = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size		*/
    "_hotshot.ProfilerType",		/* tp_name		*/
    (int) sizeof(ProfilerObject),	/* tp_basicsize		*/
    0,					/* tp_itemsize		*/
    (destructor)profiler_dealloc,	/* tp_dealloc		*/
    0,					/* tp_print		*/
    (getattrfunc)profiler_getattr,	/* tp_getattr		*/
    0,					/* tp_setattr		*/
    0,					/* tp_compare		*/
    0,					/* tp_repr		*/
    0,					/* tp_as_number		*/
    0,					/* tp_as_sequence	*/
    0,					/* tp_as_mapping	*/
    0,					/* tp_hash		*/
    0,					/* tp_call		*/
    0,					/* tp_str		*/
    0,					/* tp_getattro		*/
    0,					/* tp_setattro		*/
    0,					/* tp_as_buffer		*/
    Py_TPFLAGS_DEFAULT,			/* tp_flags		*/
    profiler_object__doc__,		/* tp_doc		*/
};


static PyMethodDef logreader_methods[] = {
    {"close",   (PyCFunction)logreader_close,  METH_VARARGS,
     logreader_close__doc__},
    {"next",    (PyCFunction)logreader_next,   METH_VARARGS,
     next__doc__},
    {NULL, NULL}
};

static PyObject *
logreader_getattr(LogReaderObject *self, char *name)
{
    if (strcmp(name, "info") == 0) {
        Py_INCREF(self->info);
        return self->info;
    }
    return Py_FindMethod(logreader_methods, (PyObject *)self, name);
}


static char logreader__doc__[] = "\
logreader(filename) --> log-iterator\n\
Create a log-reader for the timing information file.";

static PySequenceMethods logreader_as_sequence = {
    0,					/* sq_length */
    0,					/* sq_concat */
    0,					/* sq_repeat */
    (intargfunc)logreader_sq_item,	/* sq_item */
    0,					/* sq_slice */
    0,					/* sq_ass_item */
    0,					/* sq_ass_slice */
    0,					/* sq_contains */
    0,					/* sq_inplace_concat */
    0,					/* sq_inplace_repeat */
};

static PyTypeObject LogReaderType = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size		*/
    "_hotshot.LogReaderType",		/* tp_name		*/
    (int) sizeof(LogReaderObject),	/* tp_basicsize		*/
    0,					/* tp_itemsize		*/
    (destructor)logreader_dealloc,	/* tp_dealloc		*/
    0,					/* tp_print		*/
    (getattrfunc)logreader_getattr,	/* tp_getattr		*/
    0,					/* tp_setattr		*/
    0,					/* tp_compare		*/
    0,					/* tp_repr		*/
    0,					/* tp_as_number		*/
    &logreader_as_sequence,		/* tp_as_sequence	*/
    0,					/* tp_as_mapping	*/
    0,					/* tp_hash		*/
    0,					/* tp_call		*/
    0,					/* tp_str		*/
    0,					/* tp_getattro		*/
    0,					/* tp_setattro		*/
    0,					/* tp_as_buffer		*/
    Py_TPFLAGS_DEFAULT,			/* tp_flags		*/
    logreader__doc__,			/* tp_doc		*/
#if Py_TPFLAGS_HAVE_ITER
    0,					/* tp_traverse		*/
    0,					/* tp_clear		*/
    0,					/* tp_richcompare	*/
    0,					/* tp_weaklistoffset	*/
    (getiterfunc)logreader_tp_iter,	/* tp_iter		*/
    (iternextfunc)logreader_tp_iternext,/* tp_iternext		*/
#endif
};

static PyObject *
hotshot_logreader(PyObject *unused, PyObject *args)
{
    LogReaderObject *self = NULL;
    char *filename;

    if (PyArg_ParseTuple(args, "s:logreader", &filename)) {
        self = PyObject_New(LogReaderObject, &LogReaderType);
        if (self != NULL) {
            self->filled = 0;
            self->index = 0;
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
            /* Aggressively attempt to load all preliminary ADD_INFO
             * records from the log so the info records are available
             * from a fresh logreader object.
             */
            logreader_refill(self);
            while (self->filled > self->index
                   && self->buffer[self->index] == WHAT_ADD_INFO) {
                int err = unpack_add_info(self, 1);
                if (err) {
                    if (err == ERR_EOF)
                        eof_error();
                    else
                        PyErr_SetString(PyExc_RuntimeError,
                                        "unexpected error");
                    break;
                }
                /* Refill agressively so we can avoid EOF during
                 * initialization unless there's a real EOF condition
                 * (the tp_iternext handler loops attempts to refill
                 * and try again).
                 */
                logreader_refill(self);
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

    while (*rev && !isdigit(*rev))
        ++rev;
    while (rev[i] != ' ' && rev[i] != '\0')
        ++i;
    buffer = malloc(i + 1);
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
    int i, len;

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

#ifdef MS_WIN32
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
    len = PyList_GET_SIZE(temp);
    for (i = 0; i < len; ++i) {
        PyObject *item = PyList_GET_ITEM(temp, i);
        buffer = PyString_AsString(item);
        if (buffer == NULL)
            return -1;
        pack_add_info(self, "sys-path-entry", buffer);
    }
    pack_frame_times(self);
    pack_line_times(self);

    return 0;
}

static char profiler__doc__[] = "\
profiler(logfilename[, lineevents[, linetimes]]) -> profiler\n\
Create a new profiler object.";

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
        if (write_header(self))
            /* some error occurred, exception has been set */
            self = NULL;
    }
    return (PyObject *) self;
}

static char coverage__doc__[] = "\
coverage(logfilename) -> profiler\n\
Returns a profiler that doesn't collect any timing information, which is\n\
useful in building a coverage analysis tool.";

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

static char resolution__doc__[] =
#ifdef MS_WIN32
"resolution() -> (performance-counter-ticks, update-frequency)\n"
"Return the resolution of the timer provided by the QueryPerformanceCounter()\n"
"function.  The first value is the smallest observed change, and the second\n"
"is the result of QueryPerformanceFrequency().";
#else
"resolution() -> (gettimeofday-usecs, getrusage-usecs)\n"
"Return the resolution of the timers provided by the gettimeofday() and\n"
"getrusage() system calls, or -1 if the call is not supported.";
#endif

static PyObject *
hotshot_resolution(PyObject *unused, PyObject *args)
{
    PyObject *result = NULL;

    if (PyArg_ParseTuple(args, ":resolution")) {
        if (timeofday_diff == 0) {
            calibrate();
            calibrate();
            calibrate();
        }
#ifdef MS_WIN32
        result = Py_BuildValue("ii", timeofday_diff, frequency.LowPart);
#else
        result = Py_BuildValue("ii", timeofday_diff, rusage_diff);
#endif
    }
    return result;
}


static PyMethodDef functions[] = {
    {"coverage",   hotshot_coverage,   METH_VARARGS, coverage__doc__},
    {"profiler",   hotshot_profiler,   METH_VARARGS, profiler__doc__},
    {"logreader",  hotshot_logreader,  METH_VARARGS, logreader__doc__},
    {"resolution", hotshot_resolution, METH_VARARGS, resolution__doc__},
    {NULL, NULL}
};


void
init_hotshot(void)
{
    PyObject *module;

    LogReaderType.ob_type = &PyType_Type;
    ProfilerType.ob_type = &PyType_Type;
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
