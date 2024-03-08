
/* New getargs implementation */

#include "Python.h"
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_pylifecycle.h"   // _PyArg_Fini

#include <ctype.h>
#include <float.h>


#ifdef __cplusplus
extern "C" {
#endif
int PyArg_Parse(PyObject *, const char *, ...);
int PyArg_ParseTuple(PyObject *, const char *, ...);
int PyArg_VaParse(PyObject *, const char *, va_list);

int PyArg_ParseTupleAndKeywords(PyObject *, PyObject *,
                                const char *, char **, ...);
int PyArg_VaParseTupleAndKeywords(PyObject *, PyObject *,
                                const char *, char **, va_list);

int _PyArg_ParseTupleAndKeywordsFast(PyObject *, PyObject *,
                                            struct _PyArg_Parser *, ...);
int _PyArg_VaParseTupleAndKeywordsFast(PyObject *, PyObject *,
                                            struct _PyArg_Parser *, va_list);

#ifdef HAVE_DECLSPEC_DLL
/* Export functions */
PyAPI_FUNC(int) _PyArg_Parse_SizeT(PyObject *, const char *, ...);
PyAPI_FUNC(int) _PyArg_ParseStack_SizeT(PyObject *const *args, Py_ssize_t nargs,
                                        const char *format, ...);
PyAPI_FUNC(int) _PyArg_ParseStackAndKeywords_SizeT(PyObject *const *args, Py_ssize_t nargs,
                                        PyObject *kwnames,
                                        struct _PyArg_Parser *parser, ...);
PyAPI_FUNC(int) _PyArg_ParseTuple_SizeT(PyObject *, const char *, ...);
PyAPI_FUNC(int) _PyArg_ParseTupleAndKeywords_SizeT(PyObject *, PyObject *,
                                                  const char *, char **, ...);
PyAPI_FUNC(PyObject *) _Py_BuildValue_SizeT(const char *, ...);
PyAPI_FUNC(int) _PyArg_VaParse_SizeT(PyObject *, const char *, va_list);
PyAPI_FUNC(int) _PyArg_VaParseTupleAndKeywords_SizeT(PyObject *, PyObject *,
                                              const char *, char **, va_list);

PyAPI_FUNC(int) _PyArg_ParseTupleAndKeywordsFast_SizeT(PyObject *, PyObject *,
                                            struct _PyArg_Parser *, ...);
PyAPI_FUNC(int) _PyArg_VaParseTupleAndKeywordsFast_SizeT(PyObject *, PyObject *,
                                            struct _PyArg_Parser *, va_list);
#endif

#define FLAG_COMPAT 1
#define FLAG_SIZE_T 2

typedef int (*destr_t)(PyObject *, void *);


/* Keep track of "objects" that have been allocated or initialized and
   which will need to be deallocated or cleaned up somehow if overall
   parsing fails.
*/
typedef struct {
  void *item;
  destr_t destructor;
} freelistentry_t;

typedef struct {
  freelistentry_t *entries;
  int first_available;
  int entries_malloced;
} freelist_t;

#define STATIC_FREELIST_ENTRIES 8

/* Forward */
static int vgetargs1_impl(PyObject *args, PyObject *const *stack, Py_ssize_t nargs,
                          const char *format, va_list *p_va, int flags);
static int vgetargs1(PyObject *, const char *, va_list *, int);
static void seterror(Py_ssize_t, const char *, int *, const char *, const char *);
static const char *convertitem(PyObject *, const char **, va_list *, int, int *,
                               char *, size_t, freelist_t *);
static const char *converttuple(PyObject *, const char **, va_list *, int,
                                int *, char *, size_t, int, freelist_t *);
static const char *convertsimple(PyObject *, const char **, va_list *, int,
                                 char *, size_t, freelist_t *);
static Py_ssize_t convertbuffer(PyObject *, const void **p, const char **);
static int getbuffer(PyObject *, Py_buffer *, const char**);

static int vgetargskeywords(PyObject *, PyObject *,
                            const char *, char **, va_list *, int);
static int vgetargskeywordsfast(PyObject *, PyObject *,
                            struct _PyArg_Parser *, va_list *, int);
static int vgetargskeywordsfast_impl(PyObject *const *args, Py_ssize_t nargs,
                          PyObject *keywords, PyObject *kwnames,
                          struct _PyArg_Parser *parser,
                          va_list *p_va, int flags);
static const char *skipitem(const char **, va_list *, int);

int
PyArg_Parse(PyObject *args, const char *format, ...)
{
    int retval;
    va_list va;

    va_start(va, format);
    retval = vgetargs1(args, format, &va, FLAG_COMPAT);
    va_end(va);
    return retval;
}

PyAPI_FUNC(int)
_PyArg_Parse_SizeT(PyObject *args, const char *format, ...)
{
    int retval;
    va_list va;

    va_start(va, format);
    retval = vgetargs1(args, format, &va, FLAG_COMPAT|FLAG_SIZE_T);
    va_end(va);
    return retval;
}


int
PyArg_ParseTuple(PyObject *args, const char *format, ...)
{
    int retval;
    va_list va;

    va_start(va, format);
    retval = vgetargs1(args, format, &va, 0);
    va_end(va);
    return retval;
}

PyAPI_FUNC(int)
_PyArg_ParseTuple_SizeT(PyObject *args, const char *format, ...)
{
    int retval;
    va_list va;

    va_start(va, format);
    retval = vgetargs1(args, format, &va, FLAG_SIZE_T);
    va_end(va);
    return retval;
}


int
_PyArg_ParseStack(PyObject *const *args, Py_ssize_t nargs, const char *format, ...)
{
    int retval;
    va_list va;

    va_start(va, format);
    retval = vgetargs1_impl(NULL, args, nargs, format, &va, 0);
    va_end(va);
    return retval;
}

PyAPI_FUNC(int)
_PyArg_ParseStack_SizeT(PyObject *const *args, Py_ssize_t nargs, const char *format, ...)
{
    int retval;
    va_list va;

    va_start(va, format);
    retval = vgetargs1_impl(NULL, args, nargs, format, &va, FLAG_SIZE_T);
    va_end(va);
    return retval;
}


int
PyArg_VaParse(PyObject *args, const char *format, va_list va)
{
    va_list lva;
    int retval;

    va_copy(lva, va);

    retval = vgetargs1(args, format, &lva, 0);
    va_end(lva);
    return retval;
}

PyAPI_FUNC(int)
_PyArg_VaParse_SizeT(PyObject *args, const char *format, va_list va)
{
    va_list lva;
    int retval;

    va_copy(lva, va);

    retval = vgetargs1(args, format, &lva, FLAG_SIZE_T);
    va_end(lva);
    return retval;
}


/* Handle cleanup of allocated memory in case of exception */

static int
cleanup_ptr(PyObject *self, void *ptr)
{
    void **pptr = (void **)ptr;
    PyMem_Free(*pptr);
    *pptr = NULL;
    return 0;
}

static int
cleanup_buffer(PyObject *self, void *ptr)
{
    Py_buffer *buf = (Py_buffer *)ptr;
    if (buf) {
        PyBuffer_Release(buf);
    }
    return 0;
}

static int
addcleanup(void *ptr, freelist_t *freelist, destr_t destructor)
{
    int index;

    index = freelist->first_available;
    freelist->first_available += 1;

    freelist->entries[index].item = ptr;
    freelist->entries[index].destructor = destructor;

    return 0;
}

static int
cleanreturn(int retval, freelist_t *freelist)
{
    int index;

    if (retval == 0) {
      /* A failure occurred, therefore execute all of the cleanup
         functions.
      */
      for (index = 0; index < freelist->first_available; ++index) {
          freelist->entries[index].destructor(NULL,
                                              freelist->entries[index].item);
      }
    }
    if (freelist->entries_malloced)
        PyMem_Free(freelist->entries);
    return retval;
}


static int
vgetargs1_impl(PyObject *compat_args, PyObject *const *stack, Py_ssize_t nargs, const char *format,
               va_list *p_va, int flags)
{
    char msgbuf[256];
    int levels[32];
    const char *fname = NULL;
    const char *message = NULL;
    int min = -1;
    int max = 0;
    int level = 0;
    int endfmt = 0;
    const char *formatsave = format;
    Py_ssize_t i;
    const char *msg;
    int compat = flags & FLAG_COMPAT;
    freelistentry_t static_entries[STATIC_FREELIST_ENTRIES];
    freelist_t freelist;

    assert(nargs == 0 || stack != NULL);

    freelist.entries = static_entries;
    freelist.first_available = 0;
    freelist.entries_malloced = 0;

    flags = flags & ~FLAG_COMPAT;

    while (endfmt == 0) {
        int c = *format++;
        switch (c) {
        case '(':
            if (level == 0)
                max++;
            level++;
            if (level >= 30)
                Py_FatalError("too many tuple nesting levels "
                              "in argument format string");
            break;
        case ')':
            if (level == 0)
                Py_FatalError("excess ')' in getargs format");
            else
                level--;
            break;
        case '\0':
            endfmt = 1;
            break;
        case ':':
            fname = format;
            endfmt = 1;
            break;
        case ';':
            message = format;
            endfmt = 1;
            break;
        case '|':
            if (level == 0)
                min = max;
            break;
        default:
            if (level == 0) {
                if (Py_ISALPHA(c))
                    if (c != 'e') /* skip encoded */
                        max++;
            }
            break;
        }
    }

    if (level != 0)
        Py_FatalError(/* '(' */ "missing ')' in getargs format");

    if (min < 0)
        min = max;

    format = formatsave;

    if (max > STATIC_FREELIST_ENTRIES) {
        freelist.entries = PyMem_NEW(freelistentry_t, max);
        if (freelist.entries == NULL) {
            PyErr_NoMemory();
            return 0;
        }
        freelist.entries_malloced = 1;
    }

    if (compat) {
        if (max == 0) {
            if (compat_args == NULL)
                return 1;
            PyErr_Format(PyExc_TypeError,
                         "%.200s%s takes no arguments",
                         fname==NULL ? "function" : fname,
                         fname==NULL ? "" : "()");
            return cleanreturn(0, &freelist);
        }
        else if (min == 1 && max == 1) {
            if (compat_args == NULL) {
                PyErr_Format(PyExc_TypeError,
                             "%.200s%s takes at least one argument",
                             fname==NULL ? "function" : fname,
                             fname==NULL ? "" : "()");
                return cleanreturn(0, &freelist);
            }
            msg = convertitem(compat_args, &format, p_va, flags, levels,
                              msgbuf, sizeof(msgbuf), &freelist);
            if (msg == NULL)
                return cleanreturn(1, &freelist);
            seterror(levels[0], msg, levels+1, fname, message);
            return cleanreturn(0, &freelist);
        }
        else {
            PyErr_SetString(PyExc_SystemError,
                "old style getargs format uses new features");
            return cleanreturn(0, &freelist);
        }
    }

    if (nargs < min || max < nargs) {
        if (message == NULL)
            PyErr_Format(PyExc_TypeError,
                         "%.150s%s takes %s %d argument%s (%zd given)",
                         fname==NULL ? "function" : fname,
                         fname==NULL ? "" : "()",
                         min==max ? "exactly"
                         : nargs < min ? "at least" : "at most",
                         nargs < min ? min : max,
                         (nargs < min ? min : max) == 1 ? "" : "s",
                         nargs);
        else
            PyErr_SetString(PyExc_TypeError, message);
        return cleanreturn(0, &freelist);
    }

    for (i = 0; i < nargs; i++) {
        if (*format == '|')
            format++;
        msg = convertitem(stack[i], &format, p_va,
                          flags, levels, msgbuf,
                          sizeof(msgbuf), &freelist);
        if (msg) {
            seterror(i+1, msg, levels, fname, message);
            return cleanreturn(0, &freelist);
        }
    }

    if (*format != '\0' && !Py_ISALPHA(*format) &&
        *format != '(' &&
        *format != '|' && *format != ':' && *format != ';') {
        PyErr_Format(PyExc_SystemError,
                     "bad format string: %.200s", formatsave);
        return cleanreturn(0, &freelist);
    }

    return cleanreturn(1, &freelist);
}

static int
vgetargs1(PyObject *args, const char *format, va_list *p_va, int flags)
{
    PyObject **stack;
    Py_ssize_t nargs;

    if (!(flags & FLAG_COMPAT)) {
        assert(args != NULL);

        if (!PyTuple_Check(args)) {
            PyErr_SetString(PyExc_SystemError,
                "new style getargs format but argument is not a tuple");
            return 0;
        }

        stack = _PyTuple_ITEMS(args);
        nargs = PyTuple_GET_SIZE(args);
    }
    else {
        stack = NULL;
        nargs = 0;
    }

    return vgetargs1_impl(args, stack, nargs, format, p_va, flags);
}


static void
seterror(Py_ssize_t iarg, const char *msg, int *levels, const char *fname,
         const char *message)
{
    char buf[512];
    int i;
    char *p = buf;

    if (PyErr_Occurred())
        return;
    else if (message == NULL) {
        if (fname != NULL) {
            PyOS_snprintf(p, sizeof(buf), "%.200s() ", fname);
            p += strlen(p);
        }
        if (iarg != 0) {
            PyOS_snprintf(p, sizeof(buf) - (p - buf),
                          "argument %zd", iarg);
            i = 0;
            p += strlen(p);
            while (i < 32 && levels[i] > 0 && (int)(p-buf) < 220) {
                PyOS_snprintf(p, sizeof(buf) - (p - buf),
                              ", item %d", levels[i]-1);
                p += strlen(p);
                i++;
            }
        }
        else {
            PyOS_snprintf(p, sizeof(buf) - (p - buf), "argument");
            p += strlen(p);
        }
        PyOS_snprintf(p, sizeof(buf) - (p - buf), " %.256s", msg);
        message = buf;
    }
    if (msg[0] == '(') {
        PyErr_SetString(PyExc_SystemError, message);
    }
    else {
        PyErr_SetString(PyExc_TypeError, message);
    }
}


/* Convert a tuple argument.
   On entry, *p_format points to the character _after_ the opening '('.
   On successful exit, *p_format points to the closing ')'.
   If successful:
      *p_format and *p_va are updated,
      *levels and *msgbuf are untouched,
      and NULL is returned.
   If the argument is invalid:
      *p_format is unchanged,
      *p_va is undefined,
      *levels is a 0-terminated list of item numbers,
      *msgbuf contains an error message, whose format is:
     "must be <typename1>, not <typename2>", where:
        <typename1> is the name of the expected type, and
        <typename2> is the name of the actual type,
      and msgbuf is returned.
*/

static const char *
converttuple(PyObject *arg, const char **p_format, va_list *p_va, int flags,
             int *levels, char *msgbuf, size_t bufsize, int toplevel,
             freelist_t *freelist)
{
    int level = 0;
    int n = 0;
    const char *format = *p_format;
    int i;
    Py_ssize_t len;

    for (;;) {
        int c = *format++;
        if (c == '(') {
            if (level == 0)
                n++;
            level++;
        }
        else if (c == ')') {
            if (level == 0)
                break;
            level--;
        }
        else if (c == ':' || c == ';' || c == '\0')
            break;
        else if (level == 0 && Py_ISALPHA(c) && c != 'e')
            n++;
    }

    if (!PySequence_Check(arg) || PyBytes_Check(arg)) {
        levels[0] = 0;
        PyOS_snprintf(msgbuf, bufsize,
                      toplevel ? "expected %d arguments, not %.50s" :
                      "must be %d-item sequence, not %.50s",
                  n,
                  arg == Py_None ? "None" : Py_TYPE(arg)->tp_name);
        return msgbuf;
    }

    len = PySequence_Size(arg);
    if (len != n) {
        levels[0] = 0;
        if (toplevel) {
            PyOS_snprintf(msgbuf, bufsize,
                          "expected %d argument%s, not %zd",
                          n,
                          n == 1 ? "" : "s",
                          len);
        }
        else {
            PyOS_snprintf(msgbuf, bufsize,
                          "must be sequence of length %d, not %zd",
                          n, len);
        }
        return msgbuf;
    }

    format = *p_format;
    for (i = 0; i < n; i++) {
        const char *msg;
        PyObject *item;
        item = PySequence_GetItem(arg, i);
        if (item == NULL) {
            PyErr_Clear();
            levels[0] = i+1;
            levels[1] = 0;
            strncpy(msgbuf, "is not retrievable", bufsize);
            return msgbuf;
        }
        msg = convertitem(item, &format, p_va, flags, levels+1,
                          msgbuf, bufsize, freelist);
        /* PySequence_GetItem calls tp->sq_item, which INCREFs */
        Py_XDECREF(item);
        if (msg != NULL) {
            levels[0] = i+1;
            return msg;
        }
    }

    *p_format = format;
    return NULL;
}


/* Convert a single item. */

static const char *
convertitem(PyObject *arg, const char **p_format, va_list *p_va, int flags,
            int *levels, char *msgbuf, size_t bufsize, freelist_t *freelist)
{
    const char *msg;
    const char *format = *p_format;

    if (*format == '(' /* ')' */) {
        format++;
        msg = converttuple(arg, &format, p_va, flags, levels, msgbuf,
                           bufsize, 0, freelist);
        if (msg == NULL)
            format++;
    }
    else {
        msg = convertsimple(arg, &format, p_va, flags,
                            msgbuf, bufsize, freelist);
        if (msg != NULL)
            levels[0] = 0;
    }
    if (msg == NULL)
        *p_format = format;
    return msg;
}



/* Format an error message generated by convertsimple().
   displayname must be UTF-8 encoded.
*/

void
_PyArg_BadArgument(const char *fname, const char *displayname,
                   const char *expected, PyObject *arg)
{
    PyErr_Format(PyExc_TypeError,
                 "%.200s() %.200s must be %.50s, not %.50s",
                 fname, displayname, expected,
                 arg == Py_None ? "None" : Py_TYPE(arg)->tp_name);
}

static const char *
converterr(const char *expected, PyObject *arg, char *msgbuf, size_t bufsize)
{
    assert(expected != NULL);
    assert(arg != NULL);
    if (expected[0] == '(') {
        PyOS_snprintf(msgbuf, bufsize,
                      "%.100s", expected);
    }
    else {
        PyOS_snprintf(msgbuf, bufsize,
                      "must be %.50s, not %.50s", expected,
                      arg == Py_None ? "None" : Py_TYPE(arg)->tp_name);
    }
    return msgbuf;
}

#define CONV_UNICODE "(unicode conversion error)"

/* Convert a non-tuple argument.  Return NULL if conversion went OK,
   or a string with a message describing the failure.  The message is
   formatted as "must be <desired type>, not <actual type>".
   When failing, an exception may or may not have been raised.
   Don't call if a tuple is expected.

   When you add new format codes, please don't forget poor skipitem() below.
*/

static const char *
convertsimple(PyObject *arg, const char **p_format, va_list *p_va, int flags,
              char *msgbuf, size_t bufsize, freelist_t *freelist)
{
#define RETURN_ERR_OCCURRED return msgbuf
    /* For # codes */
#define REQUIRE_PY_SSIZE_T_CLEAN \
    if (!(flags & FLAG_SIZE_T)) { \
        PyErr_SetString(PyExc_SystemError, \
                        "PY_SSIZE_T_CLEAN macro must be defined for '#' formats"); \
        RETURN_ERR_OCCURRED; \
    }

    const char *format = *p_format;
    char c = *format++;
    const char *sarg;

    switch (c) {

    case 'b': { /* unsigned byte -- very short int */
        unsigned char *p = va_arg(*p_va, unsigned char *);
        long ival = PyLong_AsLong(arg);
        if (ival == -1 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else if (ival < 0) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is less than minimum");
            RETURN_ERR_OCCURRED;
        }
        else if (ival > UCHAR_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is greater than maximum");
            RETURN_ERR_OCCURRED;
        }
        else
            *p = (unsigned char) ival;
        break;
    }

    case 'B': {/* byte sized bitfield - both signed and unsigned
                  values allowed */
        unsigned char *p = va_arg(*p_va, unsigned char *);
        unsigned long ival = PyLong_AsUnsignedLongMask(arg);
        if (ival == (unsigned long)-1 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else
            *p = (unsigned char) ival;
        break;
    }

    case 'h': {/* signed short int */
        short *p = va_arg(*p_va, short *);
        long ival = PyLong_AsLong(arg);
        if (ival == -1 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else if (ival < SHRT_MIN) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is less than minimum");
            RETURN_ERR_OCCURRED;
        }
        else if (ival > SHRT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is greater than maximum");
            RETURN_ERR_OCCURRED;
        }
        else
            *p = (short) ival;
        break;
    }

    case 'H': { /* short int sized bitfield, both signed and
                   unsigned allowed */
        unsigned short *p = va_arg(*p_va, unsigned short *);
        unsigned long ival = PyLong_AsUnsignedLongMask(arg);
        if (ival == (unsigned long)-1 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else
            *p = (unsigned short) ival;
        break;
    }

    case 'i': {/* signed int */
        int *p = va_arg(*p_va, int *);
        long ival = PyLong_AsLong(arg);
        if (ival == -1 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else if (ival > INT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed integer is greater than maximum");
            RETURN_ERR_OCCURRED;
        }
        else if (ival < INT_MIN) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed integer is less than minimum");
            RETURN_ERR_OCCURRED;
        }
        else
            *p = ival;
        break;
    }

    case 'I': { /* int sized bitfield, both signed and
                   unsigned allowed */
        unsigned int *p = va_arg(*p_va, unsigned int *);
        unsigned long ival = PyLong_AsUnsignedLongMask(arg);
        if (ival == (unsigned long)-1 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else
            *p = (unsigned int) ival;
        break;
    }

    case 'n': /* Py_ssize_t */
    {
        PyObject *iobj;
        Py_ssize_t *p = va_arg(*p_va, Py_ssize_t *);
        Py_ssize_t ival = -1;
        iobj = _PyNumber_Index(arg);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        *p = ival;
        break;
    }
    case 'l': {/* long int */
        long *p = va_arg(*p_va, long *);
        long ival = PyLong_AsLong(arg);
        if (ival == -1 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else
            *p = ival;
        break;
    }

    case 'k': { /* long sized bitfield */
        unsigned long *p = va_arg(*p_va, unsigned long *);
        unsigned long ival;
        if (PyLong_Check(arg))
            ival = PyLong_AsUnsignedLongMask(arg);
        else
            return converterr("int", arg, msgbuf, bufsize);
        *p = ival;
        break;
    }

    case 'L': {/* long long */
        long long *p = va_arg( *p_va, long long * );
        long long ival = PyLong_AsLongLong(arg);
        if (ival == (long long)-1 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else
            *p = ival;
        break;
    }

    case 'K': { /* long long sized bitfield */
        unsigned long long *p = va_arg(*p_va, unsigned long long *);
        unsigned long long ival;
        if (PyLong_Check(arg))
            ival = PyLong_AsUnsignedLongLongMask(arg);
        else
            return converterr("int", arg, msgbuf, bufsize);
        *p = ival;
        break;
    }

    case 'f': {/* float */
        float *p = va_arg(*p_va, float *);
        double dval = PyFloat_AsDouble(arg);
        if (dval == -1.0 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else
            *p = (float) dval;
        break;
    }

    case 'd': {/* double */
        double *p = va_arg(*p_va, double *);
        double dval = PyFloat_AsDouble(arg);
        if (dval == -1.0 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else
            *p = dval;
        break;
    }

    case 'D': {/* complex double */
        Py_complex *p = va_arg(*p_va, Py_complex *);
        Py_complex cval;
        cval = PyComplex_AsCComplex(arg);
        if (PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else
            *p = cval;
        break;
    }

    case 'c': {/* char */
        char *p = va_arg(*p_va, char *);
        if (PyBytes_Check(arg) && PyBytes_Size(arg) == 1)
            *p = PyBytes_AS_STRING(arg)[0];
        else if (PyByteArray_Check(arg) && PyByteArray_Size(arg) == 1)
            *p = PyByteArray_AS_STRING(arg)[0];
        else
            return converterr("a byte string of length 1", arg, msgbuf, bufsize);
        break;
    }

    case 'C': {/* unicode char */
        int *p = va_arg(*p_va, int *);
        int kind;
        const void *data;

        if (!PyUnicode_Check(arg))
            return converterr("a unicode character", arg, msgbuf, bufsize);

        if (PyUnicode_READY(arg))
            RETURN_ERR_OCCURRED;

        if (PyUnicode_GET_LENGTH(arg) != 1)
            return converterr("a unicode character", arg, msgbuf, bufsize);

        kind = PyUnicode_KIND(arg);
        data = PyUnicode_DATA(arg);
        *p = PyUnicode_READ(kind, data, 0);
        break;
    }

    case 'p': {/* boolean *p*redicate */
        int *p = va_arg(*p_va, int *);
        int val = PyObject_IsTrue(arg);
        if (val > 0)
            *p = 1;
        else if (val == 0)
            *p = 0;
        else
            RETURN_ERR_OCCURRED;
        break;
    }

    /* XXX WAAAAH!  's', 'y', 'z', 'u', 'Z', 'e', 'w' codes all
       need to be cleaned up! */

    case 'y': {/* any bytes-like object */
        void **p = (void **)va_arg(*p_va, char **);
        const char *buf;
        Py_ssize_t count;
        if (*format == '*') {
            if (getbuffer(arg, (Py_buffer*)p, &buf) < 0)
                return converterr(buf, arg, msgbuf, bufsize);
            format++;
            if (addcleanup(p, freelist, cleanup_buffer)) {
                return converterr(
                    "(cleanup problem)",
                    arg, msgbuf, bufsize);
            }
            break;
        }
        count = convertbuffer(arg, (const void **)p, &buf);
        if (count < 0)
            return converterr(buf, arg, msgbuf, bufsize);
        if (*format == '#') {
            REQUIRE_PY_SSIZE_T_CLEAN;
            Py_ssize_t *psize = va_arg(*p_va, Py_ssize_t*);
            *psize = count;
            format++;
        } else {
            if (strlen(*p) != (size_t)count) {
                PyErr_SetString(PyExc_ValueError, "embedded null byte");
                RETURN_ERR_OCCURRED;
            }
        }
        break;
    }

    case 's': /* text string or bytes-like object */
    case 'z': /* text string, bytes-like object or None */
    {
        if (*format == '*') {
            /* "s*" or "z*" */
            Py_buffer *p = (Py_buffer *)va_arg(*p_va, Py_buffer *);

            if (c == 'z' && arg == Py_None)
                PyBuffer_FillInfo(p, NULL, NULL, 0, 1, 0);
            else if (PyUnicode_Check(arg)) {
                Py_ssize_t len;
                sarg = PyUnicode_AsUTF8AndSize(arg, &len);
                if (sarg == NULL)
                    return converterr(CONV_UNICODE,
                                      arg, msgbuf, bufsize);
                PyBuffer_FillInfo(p, arg, (void *)sarg, len, 1, 0);
            }
            else { /* any bytes-like object */
                const char *buf;
                if (getbuffer(arg, p, &buf) < 0)
                    return converterr(buf, arg, msgbuf, bufsize);
            }
            if (addcleanup(p, freelist, cleanup_buffer)) {
                return converterr(
                    "(cleanup problem)",
                    arg, msgbuf, bufsize);
            }
            format++;
        } else if (*format == '#') { /* a string or read-only bytes-like object */
            /* "s#" or "z#" */
            const void **p = (const void **)va_arg(*p_va, const char **);
            REQUIRE_PY_SSIZE_T_CLEAN;
            Py_ssize_t *psize = va_arg(*p_va, Py_ssize_t*);

            if (c == 'z' && arg == Py_None) {
                *p = NULL;
                *psize = 0;
            }
            else if (PyUnicode_Check(arg)) {
                Py_ssize_t len;
                sarg = PyUnicode_AsUTF8AndSize(arg, &len);
                if (sarg == NULL)
                    return converterr(CONV_UNICODE,
                                      arg, msgbuf, bufsize);
                *p = sarg;
                *psize = len;
            }
            else { /* read-only bytes-like object */
                /* XXX Really? */
                const char *buf;
                Py_ssize_t count = convertbuffer(arg, p, &buf);
                if (count < 0)
                    return converterr(buf, arg, msgbuf, bufsize);
                *psize = count;
            }
            format++;
        } else {
            /* "s" or "z" */
            const char **p = va_arg(*p_va, const char **);
            Py_ssize_t len;
            sarg = NULL;

            if (c == 'z' && arg == Py_None)
                *p = NULL;
            else if (PyUnicode_Check(arg)) {
                sarg = PyUnicode_AsUTF8AndSize(arg, &len);
                if (sarg == NULL)
                    return converterr(CONV_UNICODE,
                                      arg, msgbuf, bufsize);
                if (strlen(sarg) != (size_t)len) {
                    PyErr_SetString(PyExc_ValueError, "embedded null character");
                    RETURN_ERR_OCCURRED;
                }
                *p = sarg;
            }
            else
                return converterr(c == 'z' ? "str or None" : "str",
                                  arg, msgbuf, bufsize);
        }
        break;
    }

    case 'e': {/* encoded string */
        char **buffer;
        const char *encoding;
        PyObject *s;
        int recode_strings;
        Py_ssize_t size;
        const char *ptr;

        /* Get 'e' parameter: the encoding name */
        encoding = (const char *)va_arg(*p_va, const char *);
        if (encoding == NULL)
            encoding = PyUnicode_GetDefaultEncoding();

        /* Get output buffer parameter:
           's' (recode all objects via Unicode) or
           't' (only recode non-string objects)
        */
        if (*format == 's')
            recode_strings = 1;
        else if (*format == 't')
            recode_strings = 0;
        else
            return converterr(
                "(unknown parser marker combination)",
                arg, msgbuf, bufsize);
        buffer = (char **)va_arg(*p_va, char **);
        format++;
        if (buffer == NULL)
            return converterr("(buffer is NULL)",
                              arg, msgbuf, bufsize);

        /* Encode object */
        if (!recode_strings &&
            (PyBytes_Check(arg) || PyByteArray_Check(arg))) {
            s = Py_NewRef(arg);
            if (PyBytes_Check(arg)) {
                size = PyBytes_GET_SIZE(s);
                ptr = PyBytes_AS_STRING(s);
            }
            else {
                size = PyByteArray_GET_SIZE(s);
                ptr = PyByteArray_AS_STRING(s);
            }
        }
        else if (PyUnicode_Check(arg)) {
            /* Encode object; use default error handling */
            s = PyUnicode_AsEncodedString(arg,
                                          encoding,
                                          NULL);
            if (s == NULL)
                return converterr("(encoding failed)",
                                  arg, msgbuf, bufsize);
            assert(PyBytes_Check(s));
            size = PyBytes_GET_SIZE(s);
            ptr = PyBytes_AS_STRING(s);
            if (ptr == NULL)
                ptr = "";
        }
        else {
            return converterr(
                recode_strings ? "str" : "str, bytes or bytearray",
                arg, msgbuf, bufsize);
        }

        /* Write output; output is guaranteed to be 0-terminated */
        if (*format == '#') {
            /* Using buffer length parameter '#':

               - if *buffer is NULL, a new buffer of the
               needed size is allocated and the data
               copied into it; *buffer is updated to point
               to the new buffer; the caller is
               responsible for PyMem_Free()ing it after
               usage

               - if *buffer is not NULL, the data is
               copied to *buffer; *buffer_len has to be
               set to the size of the buffer on input;
               buffer overflow is signalled with an error;
               buffer has to provide enough room for the
               encoded string plus the trailing 0-byte

               - in both cases, *buffer_len is updated to
               the size of the buffer /excluding/ the
               trailing 0-byte

            */
            REQUIRE_PY_SSIZE_T_CLEAN;
            Py_ssize_t *psize = va_arg(*p_va, Py_ssize_t*);

            format++;
            if (psize == NULL) {
                Py_DECREF(s);
                return converterr(
                    "(buffer_len is NULL)",
                    arg, msgbuf, bufsize);
            }
            if (*buffer == NULL) {
                *buffer = PyMem_NEW(char, size + 1);
                if (*buffer == NULL) {
                    Py_DECREF(s);
                    PyErr_NoMemory();
                    RETURN_ERR_OCCURRED;
                }
                if (addcleanup(buffer, freelist, cleanup_ptr)) {
                    Py_DECREF(s);
                    return converterr(
                        "(cleanup problem)",
                        arg, msgbuf, bufsize);
                }
            } else {
                if (size + 1 > *psize) {
                    Py_DECREF(s);
                    PyErr_Format(PyExc_ValueError,
                                 "encoded string too long "
                                 "(%zd, maximum length %zd)",
                                 (Py_ssize_t)size, (Py_ssize_t)(*psize - 1));
                    RETURN_ERR_OCCURRED;
                }
            }
            memcpy(*buffer, ptr, size+1);

            *psize = size;
        }
        else {
            /* Using a 0-terminated buffer:

               - the encoded string has to be 0-terminated
               for this variant to work; if it is not, an
               error raised

               - a new buffer of the needed size is
               allocated and the data copied into it;
               *buffer is updated to point to the new
               buffer; the caller is responsible for
               PyMem_Free()ing it after usage

            */
            if ((Py_ssize_t)strlen(ptr) != size) {
                Py_DECREF(s);
                return converterr(
                    "encoded string without null bytes",
                    arg, msgbuf, bufsize);
            }
            *buffer = PyMem_NEW(char, size + 1);
            if (*buffer == NULL) {
                Py_DECREF(s);
                PyErr_NoMemory();
                RETURN_ERR_OCCURRED;
            }
            if (addcleanup(buffer, freelist, cleanup_ptr)) {
                Py_DECREF(s);
                return converterr("(cleanup problem)",
                                arg, msgbuf, bufsize);
            }
            memcpy(*buffer, ptr, size+1);
        }
        Py_DECREF(s);
        break;
    }

    case 'S': { /* PyBytes object */
        PyObject **p = va_arg(*p_va, PyObject **);
        if (PyBytes_Check(arg))
            *p = arg;
        else
            return converterr("bytes", arg, msgbuf, bufsize);
        break;
    }

    case 'Y': { /* PyByteArray object */
        PyObject **p = va_arg(*p_va, PyObject **);
        if (PyByteArray_Check(arg))
            *p = arg;
        else
            return converterr("bytearray", arg, msgbuf, bufsize);
        break;
    }

    case 'U': { /* PyUnicode object */
        PyObject **p = va_arg(*p_va, PyObject **);
        if (PyUnicode_Check(arg)) {
            if (PyUnicode_READY(arg) == -1)
                RETURN_ERR_OCCURRED;
            *p = arg;
        }
        else
            return converterr("str", arg, msgbuf, bufsize);
        break;
    }

    case 'O': { /* object */
        PyTypeObject *type;
        PyObject **p;
        if (*format == '!') {
            type = va_arg(*p_va, PyTypeObject*);
            p = va_arg(*p_va, PyObject **);
            format++;
            if (PyType_IsSubtype(Py_TYPE(arg), type))
                *p = arg;
            else
                return converterr(type->tp_name, arg, msgbuf, bufsize);

        }
        else if (*format == '&') {
            typedef int (*converter)(PyObject *, void *);
            converter convert = va_arg(*p_va, converter);
            void *addr = va_arg(*p_va, void *);
            int res;
            format++;
            if (! (res = (*convert)(arg, addr)))
                return converterr("(unspecified)",
                                  arg, msgbuf, bufsize);
            if (res == Py_CLEANUP_SUPPORTED &&
                addcleanup(addr, freelist, convert) == -1)
                return converterr("(cleanup problem)",
                                arg, msgbuf, bufsize);
        }
        else {
            p = va_arg(*p_va, PyObject **);
            *p = arg;
        }
        break;
    }


    case 'w': { /* "w*": memory buffer, read-write access */
        void **p = va_arg(*p_va, void **);

        if (*format != '*')
            return converterr(
                "(invalid use of 'w' format character)",
                arg, msgbuf, bufsize);
        format++;

        /* Caller is interested in Py_buffer, and the object
           supports it directly. */
        if (PyObject_GetBuffer(arg, (Py_buffer*)p, PyBUF_WRITABLE) < 0) {
            PyErr_Clear();
            return converterr("read-write bytes-like object",
                              arg, msgbuf, bufsize);
        }
        if (!PyBuffer_IsContiguous((Py_buffer*)p, 'C')) {
            PyBuffer_Release((Py_buffer*)p);
            return converterr("contiguous buffer", arg, msgbuf, bufsize);
        }
        if (addcleanup(p, freelist, cleanup_buffer)) {
            return converterr(
                "(cleanup problem)",
                arg, msgbuf, bufsize);
        }
        break;
    }

    default:
        return converterr("(impossible<bad format char>)", arg, msgbuf, bufsize);

    }

    *p_format = format;
    return NULL;

#undef REQUIRE_PY_SSIZE_T_CLEAN
#undef RETURN_ERR_OCCURRED
}

static Py_ssize_t
convertbuffer(PyObject *arg, const void **p, const char **errmsg)
{
    PyBufferProcs *pb = Py_TYPE(arg)->tp_as_buffer;
    Py_ssize_t count;
    Py_buffer view;

    *errmsg = NULL;
    *p = NULL;
    if (pb != NULL && pb->bf_releasebuffer != NULL) {
        *errmsg = "read-only bytes-like object";
        return -1;
    }

    if (getbuffer(arg, &view, errmsg) < 0)
        return -1;
    count = view.len;
    *p = view.buf;
    PyBuffer_Release(&view);
    return count;
}

static int
getbuffer(PyObject *arg, Py_buffer *view, const char **errmsg)
{
    if (PyObject_GetBuffer(arg, view, PyBUF_SIMPLE) != 0) {
        *errmsg = "bytes-like object";
        return -1;
    }
    if (!PyBuffer_IsContiguous(view, 'C')) {
        PyBuffer_Release(view);
        *errmsg = "contiguous buffer";
        return -1;
    }
    return 0;
}

/* Support for keyword arguments donated by
   Geoff Philbrick <philbric@delphi.hks.com> */

/* Return false (0) for error, else true. */
int
PyArg_ParseTupleAndKeywords(PyObject *args,
                            PyObject *keywords,
                            const char *format,
                            char **kwlist, ...)
{
    int retval;
    va_list va;

    if ((args == NULL || !PyTuple_Check(args)) ||
        (keywords != NULL && !PyDict_Check(keywords)) ||
        format == NULL ||
        kwlist == NULL)
    {
        PyErr_BadInternalCall();
        return 0;
    }

    va_start(va, kwlist);
    retval = vgetargskeywords(args, keywords, format, kwlist, &va, 0);
    va_end(va);
    return retval;
}

PyAPI_FUNC(int)
_PyArg_ParseTupleAndKeywords_SizeT(PyObject *args,
                                  PyObject *keywords,
                                  const char *format,
                                  char **kwlist, ...)
{
    int retval;
    va_list va;

    if ((args == NULL || !PyTuple_Check(args)) ||
        (keywords != NULL && !PyDict_Check(keywords)) ||
        format == NULL ||
        kwlist == NULL)
    {
        PyErr_BadInternalCall();
        return 0;
    }

    va_start(va, kwlist);
    retval = vgetargskeywords(args, keywords, format,
                              kwlist, &va, FLAG_SIZE_T);
    va_end(va);
    return retval;
}


int
PyArg_VaParseTupleAndKeywords(PyObject *args,
                              PyObject *keywords,
                              const char *format,
                              char **kwlist, va_list va)
{
    int retval;
    va_list lva;

    if ((args == NULL || !PyTuple_Check(args)) ||
        (keywords != NULL && !PyDict_Check(keywords)) ||
        format == NULL ||
        kwlist == NULL)
    {
        PyErr_BadInternalCall();
        return 0;
    }

    va_copy(lva, va);

    retval = vgetargskeywords(args, keywords, format, kwlist, &lva, 0);
    va_end(lva);
    return retval;
}

PyAPI_FUNC(int)
_PyArg_VaParseTupleAndKeywords_SizeT(PyObject *args,
                                    PyObject *keywords,
                                    const char *format,
                                    char **kwlist, va_list va)
{
    int retval;
    va_list lva;

    if ((args == NULL || !PyTuple_Check(args)) ||
        (keywords != NULL && !PyDict_Check(keywords)) ||
        format == NULL ||
        kwlist == NULL)
    {
        PyErr_BadInternalCall();
        return 0;
    }

    va_copy(lva, va);

    retval = vgetargskeywords(args, keywords, format,
                              kwlist, &lva, FLAG_SIZE_T);
    va_end(lva);
    return retval;
}

PyAPI_FUNC(int)
_PyArg_ParseTupleAndKeywordsFast(PyObject *args, PyObject *keywords,
                            struct _PyArg_Parser *parser, ...)
{
    int retval;
    va_list va;

    va_start(va, parser);
    retval = vgetargskeywordsfast(args, keywords, parser, &va, 0);
    va_end(va);
    return retval;
}

PyAPI_FUNC(int)
_PyArg_ParseTupleAndKeywordsFast_SizeT(PyObject *args, PyObject *keywords,
                            struct _PyArg_Parser *parser, ...)
{
    int retval;
    va_list va;

    va_start(va, parser);
    retval = vgetargskeywordsfast(args, keywords, parser, &va, FLAG_SIZE_T);
    va_end(va);
    return retval;
}

PyAPI_FUNC(int)
_PyArg_ParseStackAndKeywords(PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames,
                  struct _PyArg_Parser *parser, ...)
{
    int retval;
    va_list va;

    va_start(va, parser);
    retval = vgetargskeywordsfast_impl(args, nargs, NULL, kwnames, parser, &va, 0);
    va_end(va);
    return retval;
}

PyAPI_FUNC(int)
_PyArg_ParseStackAndKeywords_SizeT(PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames,
                        struct _PyArg_Parser *parser, ...)
{
    int retval;
    va_list va;

    va_start(va, parser);
    retval = vgetargskeywordsfast_impl(args, nargs, NULL, kwnames, parser, &va, FLAG_SIZE_T);
    va_end(va);
    return retval;
}


PyAPI_FUNC(int)
_PyArg_VaParseTupleAndKeywordsFast(PyObject *args, PyObject *keywords,
                            struct _PyArg_Parser *parser, va_list va)
{
    int retval;
    va_list lva;

    va_copy(lva, va);

    retval = vgetargskeywordsfast(args, keywords, parser, &lva, 0);
    va_end(lva);
    return retval;
}

PyAPI_FUNC(int)
_PyArg_VaParseTupleAndKeywordsFast_SizeT(PyObject *args, PyObject *keywords,
                            struct _PyArg_Parser *parser, va_list va)
{
    int retval;
    va_list lva;

    va_copy(lva, va);

    retval = vgetargskeywordsfast(args, keywords, parser, &lva, FLAG_SIZE_T);
    va_end(lva);
    return retval;
}

static void
error_unexpected_keyword_arg(PyObject *kwargs, PyObject *kwnames, PyObject *kwtuple, const char *fname)
{
    /* make sure there are no extraneous keyword arguments */
    Py_ssize_t j = 0;
    while (1) {
        PyObject *keyword;
        if (kwargs != NULL) {
            if (!PyDict_Next(kwargs, &j, &keyword, NULL))
                break;
        }
        else {
            if (j >= PyTuple_GET_SIZE(kwnames))
                break;
            keyword = PyTuple_GET_ITEM(kwnames, j);
            j++;
        }
        if (!PyUnicode_Check(keyword)) {
            PyErr_SetString(PyExc_TypeError,
                            "keywords must be strings");
            return;
        }

        int match = PySequence_Contains(kwtuple, keyword);
        if (match <= 0) {
            if (!match) {
                PyErr_Format(PyExc_TypeError,
                             "'%S' is an invalid keyword "
                             "argument for %.200s%s",
                             keyword,
                             (fname == NULL) ? "this function" : fname,
                             (fname == NULL) ? "" : "()");
            }
            return;
        }
    }
    /* Something wrong happened. There are extraneous keyword arguments,
     * but we don't know what. And we don't bother. */
    PyErr_Format(PyExc_TypeError,
                 "invalid keyword argument for %.200s%s",
                 (fname == NULL) ? "this function" : fname,
                 (fname == NULL) ? "" : "()");
}

int
PyArg_ValidateKeywordArguments(PyObject *kwargs)
{
    if (!PyDict_Check(kwargs)) {
        PyErr_BadInternalCall();
        return 0;
    }
    if (!_PyDict_HasOnlyStringKeys(kwargs)) {
        PyErr_SetString(PyExc_TypeError,
                        "keywords must be strings");
        return 0;
    }
    return 1;
}

#define IS_END_OF_FORMAT(c) (c == '\0' || c == ';' || c == ':')

static int
vgetargskeywords(PyObject *args, PyObject *kwargs, const char *format,
                 char **kwlist, va_list *p_va, int flags)
{
    char msgbuf[512];
    int levels[32];
    const char *fname, *msg, *custom_msg;
    int min = INT_MAX;
    int max = INT_MAX;
    int i, pos, len;
    int skip = 0;
    Py_ssize_t nargs, nkwargs;
    PyObject *current_arg;
    freelistentry_t static_entries[STATIC_FREELIST_ENTRIES];
    freelist_t freelist;

    freelist.entries = static_entries;
    freelist.first_available = 0;
    freelist.entries_malloced = 0;

    assert(args != NULL && PyTuple_Check(args));
    assert(kwargs == NULL || PyDict_Check(kwargs));
    assert(format != NULL);
    assert(kwlist != NULL);
    assert(p_va != NULL);

    /* grab the function name or custom error msg first (mutually exclusive) */
    fname = strchr(format, ':');
    if (fname) {
        fname++;
        custom_msg = NULL;
    }
    else {
        custom_msg = strchr(format,';');
        if (custom_msg)
            custom_msg++;
    }

    /* scan kwlist and count the number of positional-only parameters */
    for (pos = 0; kwlist[pos] && !*kwlist[pos]; pos++) {
    }
    /* scan kwlist and get greatest possible nbr of args */
    for (len = pos; kwlist[len]; len++) {
        if (!*kwlist[len]) {
            PyErr_SetString(PyExc_SystemError,
                            "Empty keyword parameter name");
            return cleanreturn(0, &freelist);
        }
    }

    if (len > STATIC_FREELIST_ENTRIES) {
        freelist.entries = PyMem_NEW(freelistentry_t, len);
        if (freelist.entries == NULL) {
            PyErr_NoMemory();
            return 0;
        }
        freelist.entries_malloced = 1;
    }

    nargs = PyTuple_GET_SIZE(args);
    nkwargs = (kwargs == NULL) ? 0 : PyDict_GET_SIZE(kwargs);
    if (nargs + nkwargs > len) {
        /* Adding "keyword" (when nargs == 0) prevents producing wrong error
           messages in some special cases (see bpo-31229). */
        PyErr_Format(PyExc_TypeError,
                     "%.200s%s takes at most %d %sargument%s (%zd given)",
                     (fname == NULL) ? "function" : fname,
                     (fname == NULL) ? "" : "()",
                     len,
                     (nargs == 0) ? "keyword " : "",
                     (len == 1) ? "" : "s",
                     nargs + nkwargs);
        return cleanreturn(0, &freelist);
    }

    /* convert tuple args and keyword args in same loop, using kwlist to drive process */
    for (i = 0; i < len; i++) {
        if (*format == '|') {
            if (min != INT_MAX) {
                PyErr_SetString(PyExc_SystemError,
                                "Invalid format string (| specified twice)");
                return cleanreturn(0, &freelist);
            }

            min = i;
            format++;

            if (max != INT_MAX) {
                PyErr_SetString(PyExc_SystemError,
                                "Invalid format string ($ before |)");
                return cleanreturn(0, &freelist);
            }
        }
        if (*format == '$') {
            if (max != INT_MAX) {
                PyErr_SetString(PyExc_SystemError,
                                "Invalid format string ($ specified twice)");
                return cleanreturn(0, &freelist);
            }

            max = i;
            format++;

            if (max < pos) {
                PyErr_SetString(PyExc_SystemError,
                                "Empty parameter name after $");
                return cleanreturn(0, &freelist);
            }
            if (skip) {
                /* Now we know the minimal and the maximal numbers of
                 * positional arguments and can raise an exception with
                 * informative message (see below). */
                break;
            }
            if (max < nargs) {
                if (max == 0) {
                    PyErr_Format(PyExc_TypeError,
                                 "%.200s%s takes no positional arguments",
                                 (fname == NULL) ? "function" : fname,
                                 (fname == NULL) ? "" : "()");
                }
                else {
                    PyErr_Format(PyExc_TypeError,
                                 "%.200s%s takes %s %d positional argument%s"
                                 " (%zd given)",
                                 (fname == NULL) ? "function" : fname,
                                 (fname == NULL) ? "" : "()",
                                 (min != INT_MAX) ? "at most" : "exactly",
                                 max,
                                 max == 1 ? "" : "s",
                                 nargs);
                }
                return cleanreturn(0, &freelist);
            }
        }
        if (IS_END_OF_FORMAT(*format)) {
            PyErr_Format(PyExc_SystemError,
                         "More keyword list entries (%d) than "
                         "format specifiers (%d)", len, i);
            return cleanreturn(0, &freelist);
        }
        if (!skip) {
            if (i < nargs) {
                current_arg = PyTuple_GET_ITEM(args, i);
            }
            else if (nkwargs && i >= pos) {
                current_arg = _PyDict_GetItemStringWithError(kwargs, kwlist[i]);
                if (current_arg) {
                    --nkwargs;
                }
                else if (PyErr_Occurred()) {
                    return cleanreturn(0, &freelist);
                }
            }
            else {
                current_arg = NULL;
            }

            if (current_arg) {
                msg = convertitem(current_arg, &format, p_va, flags,
                    levels, msgbuf, sizeof(msgbuf), &freelist);
                if (msg) {
                    seterror(i+1, msg, levels, fname, custom_msg);
                    return cleanreturn(0, &freelist);
                }
                continue;
            }

            if (i < min) {
                if (i < pos) {
                    assert (min == INT_MAX);
                    assert (max == INT_MAX);
                    skip = 1;
                    /* At that moment we still don't know the minimal and
                     * the maximal numbers of positional arguments.  Raising
                     * an exception is deferred until we encounter | and $
                     * or the end of the format. */
                }
                else {
                    PyErr_Format(PyExc_TypeError,  "%.200s%s missing required "
                                 "argument '%s' (pos %d)",
                                 (fname == NULL) ? "function" : fname,
                                 (fname == NULL) ? "" : "()",
                                 kwlist[i], i+1);
                    return cleanreturn(0, &freelist);
                }
            }
            /* current code reports success when all required args
             * fulfilled and no keyword args left, with no further
             * validation. XXX Maybe skip this in debug build ?
             */
            if (!nkwargs && !skip) {
                return cleanreturn(1, &freelist);
            }
        }

        /* We are into optional args, skip through to any remaining
         * keyword args */
        msg = skipitem(&format, p_va, flags);
        if (msg) {
            PyErr_Format(PyExc_SystemError, "%s: '%s'", msg,
                         format);
            return cleanreturn(0, &freelist);
        }
    }

    if (skip) {
        PyErr_Format(PyExc_TypeError,
                     "%.200s%s takes %s %d positional argument%s"
                     " (%zd given)",
                     (fname == NULL) ? "function" : fname,
                     (fname == NULL) ? "" : "()",
                     (Py_MIN(pos, min) < i) ? "at least" : "exactly",
                     Py_MIN(pos, min),
                     Py_MIN(pos, min) == 1 ? "" : "s",
                     nargs);
        return cleanreturn(0, &freelist);
    }

    if (!IS_END_OF_FORMAT(*format) && (*format != '|') && (*format != '$')) {
        PyErr_Format(PyExc_SystemError,
            "more argument specifiers than keyword list entries "
            "(remaining format:'%s')", format);
        return cleanreturn(0, &freelist);
    }

    if (nkwargs > 0) {
        PyObject *key;
        Py_ssize_t j;
        /* make sure there are no arguments given by name and position */
        for (i = pos; i < nargs; i++) {
            current_arg = _PyDict_GetItemStringWithError(kwargs, kwlist[i]);
            if (current_arg) {
                /* arg present in tuple and in dict */
                PyErr_Format(PyExc_TypeError,
                             "argument for %.200s%s given by name ('%s') "
                             "and position (%d)",
                             (fname == NULL) ? "function" : fname,
                             (fname == NULL) ? "" : "()",
                             kwlist[i], i+1);
                return cleanreturn(0, &freelist);
            }
            else if (PyErr_Occurred()) {
                return cleanreturn(0, &freelist);
            }
        }
        /* make sure there are no extraneous keyword arguments */
        j = 0;
        while (PyDict_Next(kwargs, &j, &key, NULL)) {
            int match = 0;
            if (!PyUnicode_Check(key)) {
                PyErr_SetString(PyExc_TypeError,
                                "keywords must be strings");
                return cleanreturn(0, &freelist);
            }
            for (i = pos; i < len; i++) {
                if (_PyUnicode_EqualToASCIIString(key, kwlist[i])) {
                    match = 1;
                    break;
                }
            }
            if (!match) {
                PyErr_Format(PyExc_TypeError,
                             "'%U' is an invalid keyword "
                             "argument for %.200s%s",
                             key,
                             (fname == NULL) ? "this function" : fname,
                             (fname == NULL) ? "" : "()");
                return cleanreturn(0, &freelist);
            }
        }
        /* Something wrong happened. There are extraneous keyword arguments,
         * but we don't know what. And we don't bother. */
        PyErr_Format(PyExc_TypeError,
                     "invalid keyword argument for %.200s%s",
                     (fname == NULL) ? "this function" : fname,
                     (fname == NULL) ? "" : "()");
        return cleanreturn(0, &freelist);
    }

    return cleanreturn(1, &freelist);
}


static int
scan_keywords(const char * const *keywords, int *ptotal, int *pposonly)
{
    /* scan keywords and count the number of positional-only parameters */
    int i;
    for (i = 0; keywords[i] && !*keywords[i]; i++) {
    }
    *pposonly = i;

    /* scan keywords and get greatest possible nbr of args */
    for (; keywords[i]; i++) {
        if (!*keywords[i]) {
            PyErr_SetString(PyExc_SystemError,
                            "Empty keyword parameter name");
            return -1;
        }
    }
    *ptotal = i;
    return 0;
}

static int
parse_format(const char *format, int total, int npos,
             const char **pfname, const char **pcustommsg,
             int *pmin, int *pmax)
{
    /* grab the function name or custom error msg first (mutually exclusive) */
    const char *custommsg;
    const char *fname = strchr(format, ':');
    if (fname) {
        fname++;
        custommsg = NULL;
    }
    else {
        custommsg = strchr(format,';');
        if (custommsg) {
            custommsg++;
        }
    }

    int min = INT_MAX;
    int max = INT_MAX;
    for (int i = 0; i < total; i++) {
        if (*format == '|') {
            if (min != INT_MAX) {
                PyErr_SetString(PyExc_SystemError,
                                "Invalid format string (| specified twice)");
                return -1;
            }
            if (max != INT_MAX) {
                PyErr_SetString(PyExc_SystemError,
                                "Invalid format string ($ before |)");
                return -1;
            }
            min = i;
            format++;
        }
        if (*format == '$') {
            if (max != INT_MAX) {
                PyErr_SetString(PyExc_SystemError,
                                "Invalid format string ($ specified twice)");
                return -1;
            }
            if (i < npos) {
                PyErr_SetString(PyExc_SystemError,
                                "Empty parameter name after $");
                return -1;
            }
            max = i;
            format++;
        }
        if (IS_END_OF_FORMAT(*format)) {
            PyErr_Format(PyExc_SystemError,
                        "More keyword list entries (%d) than "
                        "format specifiers (%d)", total, i);
            return -1;
        }

        const char *msg = skipitem(&format, NULL, 0);
        if (msg) {
            PyErr_Format(PyExc_SystemError, "%s: '%s'", msg,
                        format);
            return -1;
        }
    }
    min = Py_MIN(min, total);
    max = Py_MIN(max, total);

    if (!IS_END_OF_FORMAT(*format) && (*format != '|') && (*format != '$')) {
        PyErr_Format(PyExc_SystemError,
            "more argument specifiers than keyword list entries "
            "(remaining format:'%s')", format);
        return -1;
    }

    *pfname = fname;
    *pcustommsg = custommsg;
    *pmin = min;
    *pmax = max;
    return 0;
}

static PyObject *
new_kwtuple(const char * const *keywords, int total, int pos)
{
    int nkw = total - pos;
    PyObject *kwtuple = PyTuple_New(nkw);
    if (kwtuple == NULL) {
        return NULL;
    }
    keywords += pos;
    for (int i = 0; i < nkw; i++) {
        PyObject *str = PyUnicode_FromString(keywords[i]);
        if (str == NULL) {
            Py_DECREF(kwtuple);
            return NULL;
        }
        PyUnicode_InternInPlace(&str);
        PyTuple_SET_ITEM(kwtuple, i, str);
    }
    return kwtuple;
}

static int
_parser_init(struct _PyArg_Parser *parser)
{
    const char * const *keywords = parser->keywords;
    assert(keywords != NULL);
    assert(parser->pos == 0 &&
           (parser->format == NULL || parser->fname == NULL) &&
           parser->custom_msg == NULL &&
           parser->min == 0 &&
           parser->max == 0);

    int len, pos;
    if (scan_keywords(keywords, &len, &pos) < 0) {
        return 0;
    }

    const char *fname, *custommsg = NULL;
    int min = 0, max = 0;
    if (parser->format) {
        assert(parser->fname == NULL);
        if (parse_format(parser->format, len, pos,
                         &fname, &custommsg, &min, &max) < 0) {
            return 0;
        }
    }
    else {
        assert(parser->fname != NULL);
        fname = parser->fname;
    }

    int owned;
    PyObject *kwtuple = parser->kwtuple;
    if (kwtuple == NULL) {
        kwtuple = new_kwtuple(keywords, len, pos);
        if (kwtuple == NULL) {
            return 0;
        }
        owned = 1;
    }
    else {
        owned = 0;
    }

    parser->pos = pos;
    parser->fname = fname;
    parser->custom_msg = custommsg;
    parser->min = min;
    parser->max = max;
    parser->kwtuple = kwtuple;
    parser->initialized = owned ? 1 : -1;

    assert(parser->next == NULL);
    parser->next = _PyRuntime.getargs.static_parsers;
    _PyRuntime.getargs.static_parsers = parser;
    return 1;
}

static int
parser_init(struct _PyArg_Parser *parser)
{
    // volatile as it can be modified by other threads
    // and should not be optimized or reordered by compiler
    if (*((volatile int *)&parser->initialized)) {
        assert(parser->kwtuple != NULL);
        return 1;
    }
    PyThread_acquire_lock(_PyRuntime.getargs.mutex, WAIT_LOCK);
    // Check again if another thread initialized the parser
    // while we were waiting for the lock.
    if (*((volatile int *)&parser->initialized)) {
        assert(parser->kwtuple != NULL);
        PyThread_release_lock(_PyRuntime.getargs.mutex);
        return 1;
    }
    int ret = _parser_init(parser);
    PyThread_release_lock(_PyRuntime.getargs.mutex);
    return ret;
}

static void
parser_clear(struct _PyArg_Parser *parser)
{
    if (parser->initialized == 1) {
        Py_CLEAR(parser->kwtuple);
    }
}

static PyObject*
find_keyword(PyObject *kwnames, PyObject *const *kwstack, PyObject *key)
{
    Py_ssize_t i, nkwargs;

    nkwargs = PyTuple_GET_SIZE(kwnames);
    for (i = 0; i < nkwargs; i++) {
        PyObject *kwname = PyTuple_GET_ITEM(kwnames, i);

        /* kwname == key will normally find a match in since keyword keys
           should be interned strings; if not retry below in a new loop. */
        if (kwname == key) {
            return kwstack[i];
        }
    }

    for (i = 0; i < nkwargs; i++) {
        PyObject *kwname = PyTuple_GET_ITEM(kwnames, i);
        assert(PyUnicode_Check(kwname));
        if (_PyUnicode_EQ(kwname, key)) {
            return kwstack[i];
        }
    }
    return NULL;
}

static int
vgetargskeywordsfast_impl(PyObject *const *args, Py_ssize_t nargs,
                          PyObject *kwargs, PyObject *kwnames,
                          struct _PyArg_Parser *parser,
                          va_list *p_va, int flags)
{
    PyObject *kwtuple;
    char msgbuf[512];
    int levels[32];
    const char *format;
    const char *msg;
    PyObject *keyword;
    int i, pos, len;
    Py_ssize_t nkwargs;
    PyObject *current_arg;
    freelistentry_t static_entries[STATIC_FREELIST_ENTRIES];
    freelist_t freelist;
    PyObject *const *kwstack = NULL;

    freelist.entries = static_entries;
    freelist.first_available = 0;
    freelist.entries_malloced = 0;

    assert(kwargs == NULL || PyDict_Check(kwargs));
    assert(kwargs == NULL || kwnames == NULL);
    assert(p_va != NULL);

    if (parser == NULL) {
        PyErr_BadInternalCall();
        return 0;
    }

    if (kwnames != NULL && !PyTuple_Check(kwnames)) {
        PyErr_BadInternalCall();
        return 0;
    }

    if (!parser_init(parser)) {
        return 0;
    }

    kwtuple = parser->kwtuple;
    pos = parser->pos;
    len = pos + (int)PyTuple_GET_SIZE(kwtuple);

    if (len > STATIC_FREELIST_ENTRIES) {
        freelist.entries = PyMem_NEW(freelistentry_t, len);
        if (freelist.entries == NULL) {
            PyErr_NoMemory();
            return 0;
        }
        freelist.entries_malloced = 1;
    }

    if (kwargs != NULL) {
        nkwargs = PyDict_GET_SIZE(kwargs);
    }
    else if (kwnames != NULL) {
        nkwargs = PyTuple_GET_SIZE(kwnames);
        kwstack = args + nargs;
    }
    else {
        nkwargs = 0;
    }
    if (nargs + nkwargs > len) {
        /* Adding "keyword" (when nargs == 0) prevents producing wrong error
           messages in some special cases (see bpo-31229). */
        PyErr_Format(PyExc_TypeError,
                     "%.200s%s takes at most %d %sargument%s (%zd given)",
                     (parser->fname == NULL) ? "function" : parser->fname,
                     (parser->fname == NULL) ? "" : "()",
                     len,
                     (nargs == 0) ? "keyword " : "",
                     (len == 1) ? "" : "s",
                     nargs + nkwargs);
        return cleanreturn(0, &freelist);
    }
    if (parser->max < nargs) {
        if (parser->max == 0) {
            PyErr_Format(PyExc_TypeError,
                         "%.200s%s takes no positional arguments",
                         (parser->fname == NULL) ? "function" : parser->fname,
                         (parser->fname == NULL) ? "" : "()");
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "%.200s%s takes %s %d positional argument%s (%zd given)",
                         (parser->fname == NULL) ? "function" : parser->fname,
                         (parser->fname == NULL) ? "" : "()",
                         (parser->min < parser->max) ? "at most" : "exactly",
                         parser->max,
                         parser->max == 1 ? "" : "s",
                         nargs);
        }
        return cleanreturn(0, &freelist);
    }

    format = parser->format;
    assert(format != NULL || len == 0);
    /* convert tuple args and keyword args in same loop, using kwtuple to drive process */
    for (i = 0; i < len; i++) {
        if (*format == '|') {
            format++;
        }
        if (*format == '$') {
            format++;
        }
        assert(!IS_END_OF_FORMAT(*format));

        if (i < nargs) {
            current_arg = args[i];
        }
        else if (nkwargs && i >= pos) {
            keyword = PyTuple_GET_ITEM(kwtuple, i - pos);
            if (kwargs != NULL) {
                current_arg = PyDict_GetItemWithError(kwargs, keyword);
                if (!current_arg && PyErr_Occurred()) {
                    return cleanreturn(0, &freelist);
                }
            }
            else {
                current_arg = find_keyword(kwnames, kwstack, keyword);
            }
            if (current_arg) {
                --nkwargs;
            }
        }
        else {
            current_arg = NULL;
        }

        if (current_arg) {
            msg = convertitem(current_arg, &format, p_va, flags,
                levels, msgbuf, sizeof(msgbuf), &freelist);
            if (msg) {
                seterror(i+1, msg, levels, parser->fname, parser->custom_msg);
                return cleanreturn(0, &freelist);
            }
            continue;
        }

        if (i < parser->min) {
            /* Less arguments than required */
            if (i < pos) {
                Py_ssize_t min = Py_MIN(pos, parser->min);
                PyErr_Format(PyExc_TypeError,
                             "%.200s%s takes %s %d positional argument%s"
                             " (%zd given)",
                             (parser->fname == NULL) ? "function" : parser->fname,
                             (parser->fname == NULL) ? "" : "()",
                             min < parser->max ? "at least" : "exactly",
                             min,
                             min == 1 ? "" : "s",
                             nargs);
            }
            else {
                keyword = PyTuple_GET_ITEM(kwtuple, i - pos);
                PyErr_Format(PyExc_TypeError,  "%.200s%s missing required "
                             "argument '%U' (pos %d)",
                             (parser->fname == NULL) ? "function" : parser->fname,
                             (parser->fname == NULL) ? "" : "()",
                             keyword, i+1);
            }
            return cleanreturn(0, &freelist);
        }
        /* current code reports success when all required args
         * fulfilled and no keyword args left, with no further
         * validation. XXX Maybe skip this in debug build ?
         */
        if (!nkwargs) {
            return cleanreturn(1, &freelist);
        }

        /* We are into optional args, skip through to any remaining
         * keyword args */
        msg = skipitem(&format, p_va, flags);
        assert(msg == NULL);
    }

    assert(IS_END_OF_FORMAT(*format) || (*format == '|') || (*format == '$'));

    if (nkwargs > 0) {
        /* make sure there are no arguments given by name and position */
        for (i = pos; i < nargs; i++) {
            keyword = PyTuple_GET_ITEM(kwtuple, i - pos);
            if (kwargs != NULL) {
                current_arg = PyDict_GetItemWithError(kwargs, keyword);
                if (!current_arg && PyErr_Occurred()) {
                    return cleanreturn(0, &freelist);
                }
            }
            else {
                current_arg = find_keyword(kwnames, kwstack, keyword);
            }
            if (current_arg) {
                /* arg present in tuple and in dict */
                PyErr_Format(PyExc_TypeError,
                             "argument for %.200s%s given by name ('%U') "
                             "and position (%d)",
                             (parser->fname == NULL) ? "function" : parser->fname,
                             (parser->fname == NULL) ? "" : "()",
                             keyword, i+1);
                return cleanreturn(0, &freelist);
            }
        }

        error_unexpected_keyword_arg(kwargs, kwnames, kwtuple, parser->fname);
        return cleanreturn(0, &freelist);
    }

    return cleanreturn(1, &freelist);
}

static int
vgetargskeywordsfast(PyObject *args, PyObject *keywords,
                     struct _PyArg_Parser *parser, va_list *p_va, int flags)
{
    PyObject **stack;
    Py_ssize_t nargs;

    if (args == NULL
        || !PyTuple_Check(args)
        || (keywords != NULL && !PyDict_Check(keywords)))
    {
        PyErr_BadInternalCall();
        return 0;
    }

    stack = _PyTuple_ITEMS(args);
    nargs = PyTuple_GET_SIZE(args);
    return vgetargskeywordsfast_impl(stack, nargs, keywords, NULL,
                                     parser, p_va, flags);
}


#undef _PyArg_UnpackKeywords

PyObject * const *
_PyArg_UnpackKeywords(PyObject *const *args, Py_ssize_t nargs,
                      PyObject *kwargs, PyObject *kwnames,
                      struct _PyArg_Parser *parser,
                      int minpos, int maxpos, int minkw,
                      PyObject **buf)
{
    PyObject *kwtuple;
    PyObject *keyword;
    int i, posonly, minposonly, maxargs;
    int reqlimit = minkw ? maxpos + minkw : minpos;
    Py_ssize_t nkwargs;
    PyObject *current_arg;
    PyObject * const *kwstack = NULL;

    assert(kwargs == NULL || PyDict_Check(kwargs));
    assert(kwargs == NULL || kwnames == NULL);

    if (parser == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (kwnames != NULL && !PyTuple_Check(kwnames)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (args == NULL && nargs == 0) {
        args = buf;
    }

    if (!parser_init(parser)) {
        return NULL;
    }

    kwtuple = parser->kwtuple;
    posonly = parser->pos;
    minposonly = Py_MIN(posonly, minpos);
    maxargs = posonly + (int)PyTuple_GET_SIZE(kwtuple);

    if (kwargs != NULL) {
        nkwargs = PyDict_GET_SIZE(kwargs);
    }
    else if (kwnames != NULL) {
        nkwargs = PyTuple_GET_SIZE(kwnames);
        kwstack = args + nargs;
    }
    else {
        nkwargs = 0;
    }
    if (nkwargs == 0 && minkw == 0 && minpos <= nargs && nargs <= maxpos) {
        /* Fast path. */
        return args;
    }
    if (nargs + nkwargs > maxargs) {
        /* Adding "keyword" (when nargs == 0) prevents producing wrong error
           messages in some special cases (see bpo-31229). */
        PyErr_Format(PyExc_TypeError,
                     "%.200s%s takes at most %d %sargument%s (%zd given)",
                     (parser->fname == NULL) ? "function" : parser->fname,
                     (parser->fname == NULL) ? "" : "()",
                     maxargs,
                     (nargs == 0) ? "keyword " : "",
                     (maxargs == 1) ? "" : "s",
                     nargs + nkwargs);
        return NULL;
    }
    if (nargs > maxpos) {
        if (maxpos == 0) {
            PyErr_Format(PyExc_TypeError,
                         "%.200s%s takes no positional arguments",
                         (parser->fname == NULL) ? "function" : parser->fname,
                         (parser->fname == NULL) ? "" : "()");
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "%.200s%s takes %s %d positional argument%s (%zd given)",
                         (parser->fname == NULL) ? "function" : parser->fname,
                         (parser->fname == NULL) ? "" : "()",
                         (minpos < maxpos) ? "at most" : "exactly",
                         maxpos,
                         (maxpos == 1) ? "" : "s",
                         nargs);
        }
        return NULL;
    }
    if (nargs < minposonly) {
        PyErr_Format(PyExc_TypeError,
                     "%.200s%s takes %s %d positional argument%s"
                     " (%zd given)",
                     (parser->fname == NULL) ? "function" : parser->fname,
                     (parser->fname == NULL) ? "" : "()",
                     minposonly < maxpos ? "at least" : "exactly",
                     minposonly,
                     minposonly == 1 ? "" : "s",
                     nargs);
        return NULL;
    }

    /* copy tuple args */
    for (i = 0; i < nargs; i++) {
        buf[i] = args[i];
    }

    /* copy keyword args using kwtuple to drive process */
    for (i = Py_MAX((int)nargs, posonly); i < maxargs; i++) {
        if (nkwargs) {
            keyword = PyTuple_GET_ITEM(kwtuple, i - posonly);
            if (kwargs != NULL) {
                current_arg = PyDict_GetItemWithError(kwargs, keyword);
                if (!current_arg && PyErr_Occurred()) {
                    return NULL;
                }
            }
            else {
                current_arg = find_keyword(kwnames, kwstack, keyword);
            }
        }
        else if (i >= reqlimit) {
            break;
        }
        else {
            current_arg = NULL;
        }

        buf[i] = current_arg;

        if (current_arg) {
            --nkwargs;
        }
        else if (i < minpos || (maxpos <= i && i < reqlimit)) {
            /* Less arguments than required */
            keyword = PyTuple_GET_ITEM(kwtuple, i - posonly);
            PyErr_Format(PyExc_TypeError,  "%.200s%s missing required "
                         "argument '%U' (pos %d)",
                         (parser->fname == NULL) ? "function" : parser->fname,
                         (parser->fname == NULL) ? "" : "()",
                         keyword, i+1);
            return NULL;
        }
    }

    if (nkwargs > 0) {
        /* make sure there are no arguments given by name and position */
        for (i = posonly; i < nargs; i++) {
            keyword = PyTuple_GET_ITEM(kwtuple, i - posonly);
            if (kwargs != NULL) {
                current_arg = PyDict_GetItemWithError(kwargs, keyword);
                if (!current_arg && PyErr_Occurred()) {
                    return NULL;
                }
            }
            else {
                current_arg = find_keyword(kwnames, kwstack, keyword);
            }
            if (current_arg) {
                /* arg present in tuple and in dict */
                PyErr_Format(PyExc_TypeError,
                             "argument for %.200s%s given by name ('%U') "
                             "and position (%d)",
                             (parser->fname == NULL) ? "function" : parser->fname,
                             (parser->fname == NULL) ? "" : "()",
                             keyword, i+1);
                return NULL;
            }
        }

        error_unexpected_keyword_arg(kwargs, kwnames, kwtuple, parser->fname);
        return NULL;
    }

    return buf;
}

PyObject * const *
_PyArg_UnpackKeywordsWithVararg(PyObject *const *args, Py_ssize_t nargs,
                                PyObject *kwargs, PyObject *kwnames,
                                struct _PyArg_Parser *parser,
                                int minpos, int maxpos, int minkw,
                                int vararg, PyObject **buf)
{
    PyObject *kwtuple;
    PyObject *keyword;
    Py_ssize_t varargssize = 0;
    int i, posonly, minposonly, maxargs;
    int reqlimit = minkw ? maxpos + minkw : minpos;
    Py_ssize_t nkwargs;
    PyObject *current_arg;
    PyObject * const *kwstack = NULL;

    assert(kwargs == NULL || PyDict_Check(kwargs));
    assert(kwargs == NULL || kwnames == NULL);

    if (parser == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (kwnames != NULL && !PyTuple_Check(kwnames)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (args == NULL && nargs == 0) {
        args = buf;
    }

    if (!parser_init(parser)) {
        return NULL;
    }

    kwtuple = parser->kwtuple;
    posonly = parser->pos;
    minposonly = Py_MIN(posonly, minpos);
    maxargs = posonly + (int)PyTuple_GET_SIZE(kwtuple);
    if (kwargs != NULL) {
        nkwargs = PyDict_GET_SIZE(kwargs);
    }
    else if (kwnames != NULL) {
        nkwargs = PyTuple_GET_SIZE(kwnames);
        kwstack = args + nargs;
    }
    else {
        nkwargs = 0;
    }
    if (nargs < minposonly) {
        PyErr_Format(PyExc_TypeError,
                     "%.200s%s takes %s %d positional argument%s"
                     " (%zd given)",
                     (parser->fname == NULL) ? "function" : parser->fname,
                     (parser->fname == NULL) ? "" : "()",
                     minposonly < maxpos ? "at least" : "exactly",
                     minposonly,
                     minposonly == 1 ? "" : "s",
                     nargs);
        return NULL;
    }

    /* create varargs tuple */
    varargssize = nargs - maxpos;
    if (varargssize < 0) {
        varargssize = 0;
    }
    buf[vararg] = PyTuple_New(varargssize);
    if (!buf[vararg]) {
        return NULL;
    }

    /* copy tuple args */
    for (i = 0; i < nargs; i++) {
        if (i >= vararg) {
            PyTuple_SET_ITEM(buf[vararg], i - vararg, Py_NewRef(args[i]));
            continue;
        }
        else {
            buf[i] = args[i];
        }
    }

    /* copy keyword args using kwtuple to drive process */
    for (i = Py_MAX((int)nargs, posonly) -
         Py_SAFE_DOWNCAST(varargssize, Py_ssize_t, int); i < maxargs; i++) {
        if (nkwargs) {
            keyword = PyTuple_GET_ITEM(kwtuple, i - posonly);
            if (kwargs != NULL) {
                current_arg = PyDict_GetItemWithError(kwargs, keyword);
                if (!current_arg && PyErr_Occurred()) {
                    goto exit;
                }
            }
            else {
                current_arg = find_keyword(kwnames, kwstack, keyword);
            }
        }
        else {
            current_arg = NULL;
        }

        /* If an arguments is passed in as a keyword argument,
         * it should be placed before `buf[vararg]`.
         *
         * For example:
         * def f(a, /, b, *args):
         *     pass
         * f(1, b=2)
         *
         * This `buf` array should be: [1, 2, NULL].
         * In this case, nargs < vararg.
         *
         * Otherwise, we leave a place at `buf[vararg]` for vararg tuple
         * so the index is `i + 1`. */
        if (nargs < vararg) {
            buf[i] = current_arg;
        }
        else {
            buf[i + 1] = current_arg;
        }

        if (current_arg) {
            --nkwargs;
        }
        else if (i < minpos || (maxpos <= i && i < reqlimit)) {
            /* Less arguments than required */
            keyword = PyTuple_GET_ITEM(kwtuple, i - posonly);
            PyErr_Format(PyExc_TypeError,  "%.200s%s missing required "
                         "argument '%U' (pos %d)",
                         (parser->fname == NULL) ? "function" : parser->fname,
                         (parser->fname == NULL) ? "" : "()",
                         keyword, i+1);
            goto exit;
        }
    }

    if (nkwargs > 0) {
        error_unexpected_keyword_arg(kwargs, kwnames, kwtuple, parser->fname);
        goto exit;
    }

    return buf;

exit:
    Py_XDECREF(buf[vararg]);
    return NULL;
}


static const char *
skipitem(const char **p_format, va_list *p_va, int flags)
{
    const char *format = *p_format;
    char c = *format++;

    switch (c) {

    /*
     * codes that take a single data pointer as an argument
     * (the type of the pointer is irrelevant)
     */

    case 'b': /* byte -- very short int */
    case 'B': /* byte as bitfield */
    case 'h': /* short int */
    case 'H': /* short int as bitfield */
    case 'i': /* int */
    case 'I': /* int sized bitfield */
    case 'l': /* long int */
    case 'k': /* long int sized bitfield */
    case 'L': /* long long */
    case 'K': /* long long sized bitfield */
    case 'n': /* Py_ssize_t */
    case 'f': /* float */
    case 'd': /* double */
    case 'D': /* complex double */
    case 'c': /* char */
    case 'C': /* unicode char */
    case 'p': /* boolean predicate */
    case 'S': /* string object */
    case 'Y': /* string object */
    case 'U': /* unicode string object */
        {
            if (p_va != NULL) {
                (void) va_arg(*p_va, void *);
            }
            break;
        }

    /* string codes */

    case 'e': /* string with encoding */
        {
            if (p_va != NULL) {
                (void) va_arg(*p_va, const char *);
            }
            if (!(*format == 's' || *format == 't'))
                /* after 'e', only 's' and 't' is allowed */
                goto err;
            format++;
        }
        /* fall through */

    case 's': /* string */
    case 'z': /* string or None */
    case 'y': /* bytes */
    case 'w': /* buffer, read-write */
        {
            if (p_va != NULL) {
                (void) va_arg(*p_va, char **);
            }
            if (*format == '#') {
                if (p_va != NULL) {
                    if (!(flags & FLAG_SIZE_T)) {
                        return "PY_SSIZE_T_CLEAN macro must be defined for '#' formats";
                    }
                    (void) va_arg(*p_va, Py_ssize_t *);
                }
                format++;
            } else if ((c == 's' || c == 'z' || c == 'y' || c == 'w')
                       && *format == '*')
            {
                format++;
            }
            break;
        }

    case 'O': /* object */
        {
            if (*format == '!') {
                format++;
                if (p_va != NULL) {
                    (void) va_arg(*p_va, PyTypeObject*);
                    (void) va_arg(*p_va, PyObject **);
                }
            }
            else if (*format == '&') {
                typedef int (*converter)(PyObject *, void *);
                if (p_va != NULL) {
                    (void) va_arg(*p_va, converter);
                    (void) va_arg(*p_va, void *);
                }
                format++;
            }
            else {
                if (p_va != NULL) {
                    (void) va_arg(*p_va, PyObject **);
                }
            }
            break;
        }

    case '(':           /* bypass tuple, not handled at all previously */
        {
            const char *msg;
            for (;;) {
                if (*format==')')
                    break;
                if (IS_END_OF_FORMAT(*format))
                    return "Unmatched left paren in format "
                           "string";
                msg = skipitem(&format, p_va, flags);
                if (msg)
                    return msg;
            }
            format++;
            break;
        }

    case ')':
        return "Unmatched right paren in format string";

    default:
err:
        return "impossible<bad format char>";

    }

    *p_format = format;
    return NULL;
}


#undef _PyArg_CheckPositional

int
_PyArg_CheckPositional(const char *name, Py_ssize_t nargs,
                       Py_ssize_t min, Py_ssize_t max)
{
    assert(min >= 0);
    assert(min <= max);

    if (nargs < min) {
        if (name != NULL)
            PyErr_Format(
                PyExc_TypeError,
                "%.200s expected %s%zd argument%s, got %zd",
                name, (min == max ? "" : "at least "), min, min == 1 ? "" : "s", nargs);
        else
            PyErr_Format(
                PyExc_TypeError,
                "unpacked tuple should have %s%zd element%s,"
                " but has %zd",
                (min == max ? "" : "at least "), min, min == 1 ? "" : "s", nargs);
        return 0;
    }

    if (nargs == 0) {
        return 1;
    }

    if (nargs > max) {
        if (name != NULL)
            PyErr_Format(
                PyExc_TypeError,
                "%.200s expected %s%zd argument%s, got %zd",
                name, (min == max ? "" : "at most "), max, max == 1 ? "" : "s", nargs);
        else
            PyErr_Format(
                PyExc_TypeError,
                "unpacked tuple should have %s%zd element%s,"
                " but has %zd",
                (min == max ? "" : "at most "), max, max == 1 ? "" : "s", nargs);
        return 0;
    }

    return 1;
}

static int
unpack_stack(PyObject *const *args, Py_ssize_t nargs, const char *name,
             Py_ssize_t min, Py_ssize_t max, va_list vargs)
{
    Py_ssize_t i;
    PyObject **o;

    if (!_PyArg_CheckPositional(name, nargs, min, max)) {
        return 0;
    }

    for (i = 0; i < nargs; i++) {
        o = va_arg(vargs, PyObject **);
        *o = args[i];
    }
    return 1;
}

int
PyArg_UnpackTuple(PyObject *args, const char *name, Py_ssize_t min, Py_ssize_t max, ...)
{
    PyObject **stack;
    Py_ssize_t nargs;
    int retval;
    va_list vargs;

    if (!PyTuple_Check(args)) {
        PyErr_SetString(PyExc_SystemError,
            "PyArg_UnpackTuple() argument list is not a tuple");
        return 0;
    }
    stack = _PyTuple_ITEMS(args);
    nargs = PyTuple_GET_SIZE(args);

    va_start(vargs, max);
    retval = unpack_stack(stack, nargs, name, min, max, vargs);
    va_end(vargs);
    return retval;
}

int
_PyArg_UnpackStack(PyObject *const *args, Py_ssize_t nargs, const char *name,
                   Py_ssize_t min, Py_ssize_t max, ...)
{
    int retval;
    va_list vargs;

    va_start(vargs, max);
    retval = unpack_stack(args, nargs, name, min, max, vargs);
    va_end(vargs);
    return retval;
}


#undef _PyArg_NoKeywords
#undef _PyArg_NoKwnames
#undef _PyArg_NoPositional

/* For type constructors that don't take keyword args
 *
 * Sets a TypeError and returns 0 if the args/kwargs is
 * not empty, returns 1 otherwise
 */
int
_PyArg_NoKeywords(const char *funcname, PyObject *kwargs)
{
    if (kwargs == NULL) {
        return 1;
    }
    if (!PyDict_CheckExact(kwargs)) {
        PyErr_BadInternalCall();
        return 0;
    }
    if (PyDict_GET_SIZE(kwargs) == 0) {
        return 1;
    }

    PyErr_Format(PyExc_TypeError, "%.200s() takes no keyword arguments",
                    funcname);
    return 0;
}

int
_PyArg_NoPositional(const char *funcname, PyObject *args)
{
    if (args == NULL)
        return 1;
    if (!PyTuple_CheckExact(args)) {
        PyErr_BadInternalCall();
        return 0;
    }
    if (PyTuple_GET_SIZE(args) == 0)
        return 1;

    PyErr_Format(PyExc_TypeError, "%.200s() takes no positional arguments",
                    funcname);
    return 0;
}

int
_PyArg_NoKwnames(const char *funcname, PyObject *kwnames)
{
    if (kwnames == NULL) {
        return 1;
    }

    assert(PyTuple_CheckExact(kwnames));

    if (PyTuple_GET_SIZE(kwnames) == 0) {
        return 1;
    }

    PyErr_Format(PyExc_TypeError, "%s() takes no keyword arguments", funcname);
    return 0;
}

void
_PyArg_Fini(void)
{
    struct _PyArg_Parser *tmp, *s = _PyRuntime.getargs.static_parsers;
    while (s) {
        tmp = s->next;
        s->next = NULL;
        parser_clear(s);
        s = tmp;
    }
    _PyRuntime.getargs.static_parsers = NULL;
}

#ifdef __cplusplus
};
#endif
