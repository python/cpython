
/* New getargs implementation */

#include "Python.h"

#include <ctype.h>


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

#ifdef HAVE_DECLSPEC_DLL
/* Export functions */
PyAPI_FUNC(int) _PyArg_Parse_SizeT(PyObject *, char *, ...);
PyAPI_FUNC(int) _PyArg_ParseTuple_SizeT(PyObject *, char *, ...);
PyAPI_FUNC(int) _PyArg_ParseTupleAndKeywords_SizeT(PyObject *, PyObject *,
                                                  const char *, char **, ...);
PyAPI_FUNC(PyObject *) _Py_BuildValue_SizeT(const char *, ...);
PyAPI_FUNC(int) _PyArg_VaParse_SizeT(PyObject *, char *, va_list);
PyAPI_FUNC(int) _PyArg_VaParseTupleAndKeywords_SizeT(PyObject *, PyObject *,
                                              const char *, char **, va_list);
#endif

#define FLAG_COMPAT 1
#define FLAG_SIZE_T 2


/* Forward */
static int vgetargs1(PyObject *, const char *, va_list *, int);
static void seterror(int, const char *, int *, const char *, const char *);
static char *convertitem(PyObject *, const char **, va_list *, int, int *,
                         char *, size_t, PyObject **);
static char *converttuple(PyObject *, const char **, va_list *, int,
                          int *, char *, size_t, int, PyObject **);
static char *convertsimple(PyObject *, const char **, va_list *, int, char *,
                           size_t, PyObject **);
static Py_ssize_t convertbuffer(PyObject *, void **p, char **);
static int getbuffer(PyObject *, Py_buffer *, char**);

static int vgetargskeywords(PyObject *, PyObject *,
                            const char *, char **, va_list *, int);
static char *skipitem(const char **, va_list *, int);

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

int
_PyArg_Parse_SizeT(PyObject *args, char *format, ...)
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

int
_PyArg_ParseTuple_SizeT(PyObject *args, char *format, ...)
{
    int retval;
    va_list va;

    va_start(va, format);
    retval = vgetargs1(args, format, &va, FLAG_SIZE_T);
    va_end(va);
    return retval;
}


int
PyArg_VaParse(PyObject *args, const char *format, va_list va)
{
    va_list lva;

        Py_VA_COPY(lva, va);

    return vgetargs1(args, format, &lva, 0);
}

int
_PyArg_VaParse_SizeT(PyObject *args, char *format, va_list va)
{
    va_list lva;

        Py_VA_COPY(lva, va);

    return vgetargs1(args, format, &lva, FLAG_SIZE_T);
}


/* Handle cleanup of allocated memory in case of exception */

#define GETARGS_CAPSULE_NAME_CLEANUP_PTR "getargs.cleanup_ptr"
#define GETARGS_CAPSULE_NAME_CLEANUP_BUFFER "getargs.cleanup_buffer"
#define GETARGS_CAPSULE_NAME_CLEANUP_CONVERT "getargs.cleanup_convert"

static void
cleanup_ptr(PyObject *self)
{
    void *ptr = PyCapsule_GetPointer(self, GETARGS_CAPSULE_NAME_CLEANUP_PTR);
    if (ptr) {
        PyMem_FREE(ptr);
    }
}

static void
cleanup_buffer(PyObject *self)
{
    Py_buffer *ptr = (Py_buffer *)PyCapsule_GetPointer(self, GETARGS_CAPSULE_NAME_CLEANUP_BUFFER);
    if (ptr) {
        PyBuffer_Release(ptr);
    }
}

static int
addcleanup(void *ptr, PyObject **freelist, int is_buffer)
{
    PyObject *cobj;
    const char *name;
    PyCapsule_Destructor destr;

    if (is_buffer) {
        destr = cleanup_buffer;
        name = GETARGS_CAPSULE_NAME_CLEANUP_BUFFER;
    } else {
        destr = cleanup_ptr;
        name = GETARGS_CAPSULE_NAME_CLEANUP_PTR;
    }

    if (!*freelist) {
        *freelist = PyList_New(0);
        if (!*freelist) {
            destr(ptr);
            return -1;
        }
    }

    cobj = PyCapsule_New(ptr, name, destr);
    if (!cobj) {
        destr(ptr);
        return -1;
    }
    if (PyList_Append(*freelist, cobj)) {
        Py_DECREF(cobj);
        return -1;
    }
    Py_DECREF(cobj);
    return 0;
}

static void
cleanup_convert(PyObject *self)
{
    typedef int (*destr_t)(PyObject *, void *);
    destr_t destr = (destr_t)PyCapsule_GetContext(self);
    void *ptr = PyCapsule_GetPointer(self,
                                     GETARGS_CAPSULE_NAME_CLEANUP_CONVERT);
    if (ptr && destr)
        destr(NULL, ptr);
}

static int
addcleanup_convert(void *ptr, PyObject **freelist, int (*destr)(PyObject*,void*))
{
    PyObject *cobj;
    if (!*freelist) {
        *freelist = PyList_New(0);
        if (!*freelist) {
            destr(NULL, ptr);
            return -1;
        }
    }
    cobj = PyCapsule_New(ptr, GETARGS_CAPSULE_NAME_CLEANUP_CONVERT,
                         cleanup_convert);
    if (!cobj) {
        destr(NULL, ptr);
        return -1;
    }
    if (PyCapsule_SetContext(cobj, destr) == -1) {
        /* This really should not happen. */
        Py_FatalError("capsule refused setting of context.");
    }
    if (PyList_Append(*freelist, cobj)) {
        Py_DECREF(cobj); /* This will also call destr. */
        return -1;
    }
    Py_DECREF(cobj);
    return 0;
}

static int
cleanreturn(int retval, PyObject *freelist)
{
    if (freelist && retval != 0) {
        /* We were successful, reset the destructors so that they
           don't get called. */
        Py_ssize_t len = PyList_GET_SIZE(freelist), i;
        for (i = 0; i < len; i++)
            PyCapsule_SetDestructor(PyList_GET_ITEM(freelist, i), NULL);
    }
    Py_XDECREF(freelist);
    return retval;
}


static int
vgetargs1(PyObject *args, const char *format, va_list *p_va, int flags)
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
    Py_ssize_t i, len;
    char *msg;
    PyObject *freelist = NULL;
    int compat = flags & FLAG_COMPAT;

    assert(compat || (args != (PyObject*)NULL));
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
        default:
            if (level == 0) {
                if (c == 'O')
                    max++;
                else if (isalpha(Py_CHARMASK(c))) {
                    if (c != 'e') /* skip encoded */
                        max++;
                } else if (c == '|')
                    min = max;
            }
            break;
        }
    }

    if (level != 0)
        Py_FatalError(/* '(' */ "missing ')' in getargs format");

    if (min < 0)
        min = max;

    format = formatsave;

    if (compat) {
        if (max == 0) {
            if (args == NULL)
                return 1;
            PyOS_snprintf(msgbuf, sizeof(msgbuf),
                          "%.200s%s takes no arguments",
                          fname==NULL ? "function" : fname,
                          fname==NULL ? "" : "()");
            PyErr_SetString(PyExc_TypeError, msgbuf);
            return 0;
        }
        else if (min == 1 && max == 1) {
            if (args == NULL) {
                PyOS_snprintf(msgbuf, sizeof(msgbuf),
                      "%.200s%s takes at least one argument",
                          fname==NULL ? "function" : fname,
                          fname==NULL ? "" : "()");
                PyErr_SetString(PyExc_TypeError, msgbuf);
                return 0;
            }
            msg = convertitem(args, &format, p_va, flags, levels,
                              msgbuf, sizeof(msgbuf), &freelist);
            if (msg == NULL)
                return cleanreturn(1, freelist);
            seterror(levels[0], msg, levels+1, fname, message);
            return cleanreturn(0, freelist);
        }
        else {
            PyErr_SetString(PyExc_SystemError,
                "old style getargs format uses new features");
            return 0;
        }
    }

    if (!PyTuple_Check(args)) {
        PyErr_SetString(PyExc_SystemError,
            "new style getargs format but argument is not a tuple");
        return 0;
    }

    len = PyTuple_GET_SIZE(args);

    if (len < min || max < len) {
        if (message == NULL) {
            PyOS_snprintf(msgbuf, sizeof(msgbuf),
                          "%.150s%s takes %s %d argument%s "
                          "(%ld given)",
                          fname==NULL ? "function" : fname,
                          fname==NULL ? "" : "()",
                          min==max ? "exactly"
                          : len < min ? "at least" : "at most",
                          len < min ? min : max,
                          (len < min ? min : max) == 1 ? "" : "s",
                          Py_SAFE_DOWNCAST(len, Py_ssize_t, long));
            message = msgbuf;
        }
        PyErr_SetString(PyExc_TypeError, message);
        return 0;
    }

    for (i = 0; i < len; i++) {
        if (*format == '|')
            format++;
        msg = convertitem(PyTuple_GET_ITEM(args, i), &format, p_va,
                          flags, levels, msgbuf,
                          sizeof(msgbuf), &freelist);
        if (msg) {
            seterror(i+1, msg, levels, fname, msg);
            return cleanreturn(0, freelist);
        }
    }

    if (*format != '\0' && !isalpha(Py_CHARMASK(*format)) &&
        *format != '(' &&
        *format != '|' && *format != ':' && *format != ';') {
        PyErr_Format(PyExc_SystemError,
                     "bad format string: %.200s", formatsave);
        return cleanreturn(0, freelist);
    }

    return cleanreturn(1, freelist);
}



static void
seterror(int iarg, const char *msg, int *levels, const char *fname,
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
                          "argument %d", iarg);
            i = 0;
            p += strlen(p);
            while (levels[i] > 0 && i < 32 && (int)(p-buf) < 220) {
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
    PyErr_SetString(PyExc_TypeError, message);
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

static char *
converttuple(PyObject *arg, const char **p_format, va_list *p_va, int flags,
             int *levels, char *msgbuf, size_t bufsize, int toplevel,
             PyObject **freelist)
{
    int level = 0;
    int n = 0;
    const char *format = *p_format;
    int i;

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
        else if (level == 0 && isalpha(Py_CHARMASK(c)))
            n++;
    }

    if (!PySequence_Check(arg) || PyBytes_Check(arg)) {
        levels[0] = 0;
        PyOS_snprintf(msgbuf, bufsize,
                      toplevel ? "expected %d arguments, not %.50s" :
                      "must be %d-item sequence, not %.50s",
                  n,
                  arg == Py_None ? "None" : arg->ob_type->tp_name);
        return msgbuf;
    }

    if ((i = PySequence_Size(arg)) != n) {
        levels[0] = 0;
        PyOS_snprintf(msgbuf, bufsize,
                      toplevel ? "expected %d arguments, not %d" :
                     "must be sequence of length %d, not %d",
                  n, i);
        return msgbuf;
    }

    format = *p_format;
    for (i = 0; i < n; i++) {
        char *msg;
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

static char *
convertitem(PyObject *arg, const char **p_format, va_list *p_va, int flags,
            int *levels, char *msgbuf, size_t bufsize, PyObject **freelist)
{
    char *msg;
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



#define UNICODE_DEFAULT_ENCODING(arg) \
    _PyUnicode_AsDefaultEncodedString(arg, NULL)

/* Format an error message generated by convertsimple(). */

static char *
converterr(const char *expected, PyObject *arg, char *msgbuf, size_t bufsize)
{
    assert(expected != NULL);
    assert(arg != NULL);
    PyOS_snprintf(msgbuf, bufsize,
                  "must be %.50s, not %.50s", expected,
                  arg == Py_None ? "None" : arg->ob_type->tp_name);
    return msgbuf;
}

#define CONV_UNICODE "(unicode conversion error)"

/* Explicitly check for float arguments when integers are expected.
   Return 1 for error, 0 if ok. */
static int
float_argument_error(PyObject *arg)
{
    if (PyFloat_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        return 1;
    }
    else
        return 0;
}

/* Convert a non-tuple argument.  Return NULL if conversion went OK,
   or a string with a message describing the failure.  The message is
   formatted as "must be <desired type>, not <actual type>".
   When failing, an exception may or may not have been raised.
   Don't call if a tuple is expected.

   When you add new format codes, please don't forget poor skipitem() below.
*/

static char *
convertsimple(PyObject *arg, const char **p_format, va_list *p_va, int flags,
              char *msgbuf, size_t bufsize, PyObject **freelist)
{
    /* For # codes */
#define FETCH_SIZE      int *q=NULL;Py_ssize_t *q2=NULL;\
    if (flags & FLAG_SIZE_T) q2=va_arg(*p_va, Py_ssize_t*); \
    else q=va_arg(*p_va, int*);
#define STORE_SIZE(s)   \
    if (flags & FLAG_SIZE_T) \
        *q2=s; \
    else { \
        if (INT_MAX < s) { \
            PyErr_SetString(PyExc_OverflowError, \
                "size does not fit in an int"); \
            return converterr("", arg, msgbuf, bufsize); \
        } \
        *q=s; \
    }
#define BUFFER_LEN      ((flags & FLAG_SIZE_T) ? *q2:*q)
#define RETURN_ERR_OCCURRED return msgbuf

    const char *format = *p_format;
    char c = *format++;
    PyObject *uarg;

    switch (c) {

    case 'b': { /* unsigned byte -- very short int */
        char *p = va_arg(*p_va, char *);
        long ival;
        if (float_argument_error(arg))
            RETURN_ERR_OCCURRED;
        ival = PyLong_AsLong(arg);
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
        char *p = va_arg(*p_va, char *);
        long ival;
        if (float_argument_error(arg))
            RETURN_ERR_OCCURRED;
        ival = PyLong_AsUnsignedLongMask(arg);
        if (ival == -1 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else
            *p = (unsigned char) ival;
        break;
    }

    case 'h': {/* signed short int */
        short *p = va_arg(*p_va, short *);
        long ival;
        if (float_argument_error(arg))
            RETURN_ERR_OCCURRED;
        ival = PyLong_AsLong(arg);
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
        long ival;
        if (float_argument_error(arg))
            RETURN_ERR_OCCURRED;
        ival = PyLong_AsUnsignedLongMask(arg);
        if (ival == -1 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else
            *p = (unsigned short) ival;
        break;
    }

    case 'i': {/* signed int */
        int *p = va_arg(*p_va, int *);
        long ival;
        if (float_argument_error(arg))
            RETURN_ERR_OCCURRED;
        ival = PyLong_AsLong(arg);
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
        unsigned int ival;
        if (float_argument_error(arg))
            RETURN_ERR_OCCURRED;
        ival = (unsigned int)PyLong_AsUnsignedLongMask(arg);
        if (ival == (unsigned int)-1 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else
            *p = ival;
        break;
    }

    case 'n': /* Py_ssize_t */
    {
        PyObject *iobj;
        Py_ssize_t *p = va_arg(*p_va, Py_ssize_t *);
        Py_ssize_t ival = -1;
        if (float_argument_error(arg))
            RETURN_ERR_OCCURRED;
        iobj = PyNumber_Index(arg);
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
        long ival;
        if (float_argument_error(arg))
            RETURN_ERR_OCCURRED;
        ival = PyLong_AsLong(arg);
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
            return converterr("integer<k>", arg, msgbuf, bufsize);
        *p = ival;
        break;
    }

#ifdef HAVE_LONG_LONG
    case 'L': {/* PY_LONG_LONG */
        PY_LONG_LONG *p = va_arg( *p_va, PY_LONG_LONG * );
        PY_LONG_LONG ival;
        if (float_argument_error(arg))
            RETURN_ERR_OCCURRED;
        ival = PyLong_AsLongLong(arg);
        if (ival == (PY_LONG_LONG)-1 && PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else
            *p = ival;
        break;
    }

    case 'K': { /* long long sized bitfield */
        unsigned PY_LONG_LONG *p = va_arg(*p_va, unsigned PY_LONG_LONG *);
        unsigned PY_LONG_LONG ival;
        if (PyLong_Check(arg))
            ival = PyLong_AsUnsignedLongLongMask(arg);
        else
            return converterr("integer<K>", arg, msgbuf, bufsize);
        *p = ival;
        break;
    }
#endif

    case 'f': {/* float */
        float *p = va_arg(*p_va, float *);
        double dval = PyFloat_AsDouble(arg);
        if (PyErr_Occurred())
            RETURN_ERR_OCCURRED;
        else
            *p = (float) dval;
        break;
    }

    case 'd': {/* double */
        double *p = va_arg(*p_va, double *);
        double dval = PyFloat_AsDouble(arg);
        if (PyErr_Occurred())
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
        else
            return converterr("a byte string of length 1", arg, msgbuf, bufsize);
        break;
    }

    case 'C': {/* unicode char */
        int *p = va_arg(*p_va, int *);
        if (PyUnicode_Check(arg) &&
            PyUnicode_GET_SIZE(arg) == 1)
            *p = PyUnicode_AS_UNICODE(arg)[0];
        else
            return converterr("a unicode character", arg, msgbuf, bufsize);
        break;
    }

    /* XXX WAAAAH!  's', 'y', 'z', 'u', 'Z', 'e', 'w' codes all
       need to be cleaned up! */

    case 'y': {/* any buffer-like object, but not PyUnicode */
        void **p = (void **)va_arg(*p_va, char **);
        char *buf;
        Py_ssize_t count;
        if (*format == '*') {
            if (getbuffer(arg, (Py_buffer*)p, &buf) < 0)
                return converterr(buf, arg, msgbuf, bufsize);
            format++;
            if (addcleanup(p, freelist, 1)) {
                return converterr(
                    "(cleanup problem)",
                    arg, msgbuf, bufsize);
            }
            break;
        }
        count = convertbuffer(arg, p, &buf);
        if (count < 0)
            return converterr(buf, arg, msgbuf, bufsize);
        if (*format == '#') {
            FETCH_SIZE;
            STORE_SIZE(count);
            format++;
        } else {
            if (strlen(*p) != count)
                return converterr(
                    "bytes without null bytes",
                    arg, msgbuf, bufsize);
        }
        break;
    }

    case 's': /* text string */
    case 'z': /* text string or None */
    {
        if (*format == '*') {
            /* "s*" or "z*" */
            Py_buffer *p = (Py_buffer *)va_arg(*p_va, Py_buffer *);

            if (c == 'z' && arg == Py_None)
                PyBuffer_FillInfo(p, NULL, NULL, 0, 1, 0);
            else if (PyUnicode_Check(arg)) {
                uarg = UNICODE_DEFAULT_ENCODING(arg);
                if (uarg == NULL)
                    return converterr(CONV_UNICODE,
                                      arg, msgbuf, bufsize);
                PyBuffer_FillInfo(p, arg,
                                  PyBytes_AS_STRING(uarg), PyBytes_GET_SIZE(uarg),
                                  1, 0);
            }
            else { /* any buffer-like object */
                char *buf;
                if (getbuffer(arg, p, &buf) < 0)
                    return converterr(buf, arg, msgbuf, bufsize);
            }
            if (addcleanup(p, freelist, 1)) {
                return converterr(
                    "(cleanup problem)",
                    arg, msgbuf, bufsize);
            }
            format++;
        } else if (*format == '#') { /* any buffer-like object */
            /* "s#" or "z#" */
            void **p = (void **)va_arg(*p_va, char **);
            FETCH_SIZE;

            if (c == 'z' && arg == Py_None) {
                *p = NULL;
                STORE_SIZE(0);
            }
            else if (PyUnicode_Check(arg)) {
                uarg = UNICODE_DEFAULT_ENCODING(arg);
                if (uarg == NULL)
                    return converterr(CONV_UNICODE,
                                      arg, msgbuf, bufsize);
                *p = PyBytes_AS_STRING(uarg);
                STORE_SIZE(PyBytes_GET_SIZE(uarg));
            }
            else { /* any buffer-like object */
                /* XXX Really? */
                char *buf;
                Py_ssize_t count = convertbuffer(arg, p, &buf);
                if (count < 0)
                    return converterr(buf, arg, msgbuf, bufsize);
                STORE_SIZE(count);
            }
            format++;
        } else {
            /* "s" or "z" */
            char **p = va_arg(*p_va, char **);
            uarg = NULL;

            if (c == 'z' && arg == Py_None)
                *p = NULL;
            else if (PyUnicode_Check(arg)) {
                uarg = UNICODE_DEFAULT_ENCODING(arg);
                if (uarg == NULL)
                    return converterr(CONV_UNICODE,
                                      arg, msgbuf, bufsize);
                *p = PyBytes_AS_STRING(uarg);
            }
            else
                return converterr(c == 'z' ? "str or None" : "str",
                                  arg, msgbuf, bufsize);
            if (*p != NULL && uarg != NULL &&
                (Py_ssize_t) strlen(*p) != PyBytes_GET_SIZE(uarg))
                return converterr(
                    c == 'z' ? "str without null bytes or None"
                             : "str without null bytes",
                    arg, msgbuf, bufsize);
        }
        break;
    }

    case 'u': /* raw unicode buffer (Py_UNICODE *) */
    case 'Z': /* raw unicode buffer or None */
    {
        if (*format == '#') { /* any buffer-like object */
            /* "s#" or "Z#" */
            Py_UNICODE **p = va_arg(*p_va, Py_UNICODE **);
            FETCH_SIZE;

            if (c == 'Z' && arg == Py_None) {
                *p = NULL;
                STORE_SIZE(0);
            }
            else if (PyUnicode_Check(arg)) {
                *p = PyUnicode_AS_UNICODE(arg);
                STORE_SIZE(PyUnicode_GET_SIZE(arg));
            }
            else
                return converterr("str or None", arg, msgbuf, bufsize);
            format++;
        } else {
            /* "s" or "Z" */
            Py_UNICODE **p = va_arg(*p_va, Py_UNICODE **);

            if (c == 'Z' && arg == Py_None)
                *p = NULL;
            else if (PyUnicode_Check(arg)) {
                *p = PyUnicode_AS_UNICODE(arg);
                if (Py_UNICODE_strlen(*p) != PyUnicode_GET_SIZE(arg))
                    return converterr(
                        "str without null character or None",
                        arg, msgbuf, bufsize);
            } else
                return converterr(c == 'Z' ? "str or None" : "str",
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
            s = arg;
            Py_INCREF(s);
            if (PyObject_AsCharBuffer(s, &ptr, &size) < 0)
                return converterr("(AsCharBuffer failed)",
                                  arg, msgbuf, bufsize);
        }
        else {
            PyObject *u;

            /* Convert object to Unicode */
            u = PyUnicode_FromObject(arg);
            if (u == NULL)
                return converterr(
                    "string or unicode or text buffer",
                    arg, msgbuf, bufsize);

            /* Encode object; use default error handling */
            s = PyUnicode_AsEncodedString(u,
                                          encoding,
                                          NULL);
            Py_DECREF(u);
            if (s == NULL)
                return converterr("(encoding failed)",
                                  arg, msgbuf, bufsize);
            if (!PyBytes_Check(s)) {
                Py_DECREF(s);
                return converterr(
                    "(encoder failed to return bytes)",
                    arg, msgbuf, bufsize);
            }
            size = PyBytes_GET_SIZE(s);
            ptr = PyBytes_AS_STRING(s);
            if (ptr == NULL)
                ptr = "";
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
            FETCH_SIZE;

            format++;
            if (q == NULL && q2 == NULL) {
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
                if (addcleanup(*buffer, freelist, 0)) {
                    Py_DECREF(s);
                    return converterr(
                        "(cleanup problem)",
                        arg, msgbuf, bufsize);
                }
            } else {
                if (size + 1 > BUFFER_LEN) {
                    Py_DECREF(s);
                    return converterr(
                        "(buffer overflow)",
                        arg, msgbuf, bufsize);
                }
            }
            memcpy(*buffer, ptr, size+1);
            STORE_SIZE(size);
        } else {
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
                    "encoded string without NULL bytes",
                    arg, msgbuf, bufsize);
            }
            *buffer = PyMem_NEW(char, size + 1);
            if (*buffer == NULL) {
                Py_DECREF(s);
                PyErr_NoMemory();
                RETURN_ERR_OCCURRED;
            }
            if (addcleanup(*buffer, freelist, 0)) {
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
        if (PyUnicode_Check(arg))
            *p = arg;
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
            if (PyType_IsSubtype(arg->ob_type, type))
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
                addcleanup_convert(addr, freelist, convert) == -1)
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
                "invalid use of 'w' format character",
                arg, msgbuf, bufsize);
        format++;

        /* Caller is interested in Py_buffer, and the object
           supports it directly. */
        if (PyObject_GetBuffer(arg, (Py_buffer*)p, PyBUF_WRITABLE) < 0) {
            PyErr_Clear();
            return converterr("read-write buffer", arg, msgbuf, bufsize);
        }
        if (!PyBuffer_IsContiguous((Py_buffer*)p, 'C')) {
            PyBuffer_Release((Py_buffer*)p);
            return converterr("contiguous buffer", arg, msgbuf, bufsize);
        }
        if (addcleanup(p, freelist, 1)) {
            return converterr(
                "(cleanup problem)",
                arg, msgbuf, bufsize);
        }
        break;
    }

    default:
        return converterr("impossible<bad format char>", arg, msgbuf, bufsize);

    }

    *p_format = format;
    return NULL;

#undef FETCH_SIZE
#undef STORE_SIZE
#undef BUFFER_LEN
#undef RETURN_ERR_OCCURRED
}

static Py_ssize_t
convertbuffer(PyObject *arg, void **p, char **errmsg)
{
    PyBufferProcs *pb = Py_TYPE(arg)->tp_as_buffer;
    Py_ssize_t count;
    Py_buffer view;

    *errmsg = NULL;
    *p = NULL;
    if (pb != NULL && pb->bf_releasebuffer != NULL) {
        *errmsg = "read-only pinned buffer";
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
getbuffer(PyObject *arg, Py_buffer *view, char **errmsg)
{
    if (PyObject_GetBuffer(arg, view, PyBUF_SIMPLE) != 0) {
        *errmsg = "bytes or buffer";
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

int
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

        Py_VA_COPY(lva, va);

    retval = vgetargskeywords(args, keywords, format, kwlist, &lva, 0);
    return retval;
}

int
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

        Py_VA_COPY(lva, va);

    retval = vgetargskeywords(args, keywords, format,
                              kwlist, &lva, FLAG_SIZE_T);
    return retval;
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
                        "keyword arguments must be strings");
        return 0;
    }
    return 1;
}

#define IS_END_OF_FORMAT(c) (c == '\0' || c == ';' || c == ':')

static int
vgetargskeywords(PyObject *args, PyObject *keywords, const char *format,
                 char **kwlist, va_list *p_va, int flags)
{
    char msgbuf[512];
    int levels[32];
    const char *fname, *msg, *custom_msg, *keyword;
    int min = INT_MAX;
    int i, len, nargs, nkeywords;
    PyObject *freelist = NULL, *current_arg;

    assert(args != NULL && PyTuple_Check(args));
    assert(keywords == NULL || PyDict_Check(keywords));
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

    /* scan kwlist and get greatest possible nbr of args */
    for (len=0; kwlist[len]; len++)
        continue;

    nargs = PyTuple_GET_SIZE(args);
    nkeywords = (keywords == NULL) ? 0 : PyDict_Size(keywords);
    if (nargs + nkeywords > len) {
        PyErr_Format(PyExc_TypeError, "%s%s takes at most %d "
                     "argument%s (%d given)",
                     (fname == NULL) ? "function" : fname,
                     (fname == NULL) ? "" : "()",
                     len,
                     (len == 1) ? "" : "s",
                     nargs + nkeywords);
        return 0;
    }

    /* convert tuple args and keyword args in same loop, using kwlist to drive process */
    for (i = 0; i < len; i++) {
        keyword = kwlist[i];
        if (*format == '|') {
            min = i;
            format++;
        }
        if (IS_END_OF_FORMAT(*format)) {
            PyErr_Format(PyExc_RuntimeError,
                         "More keyword list entries (%d) than "
                         "format specifiers (%d)", len, i);
            return cleanreturn(0, freelist);
        }
        current_arg = NULL;
        if (nkeywords) {
            current_arg = PyDict_GetItemString(keywords, keyword);
        }
        if (current_arg) {
            --nkeywords;
            if (i < nargs) {
                /* arg present in tuple and in dict */
                PyErr_Format(PyExc_TypeError,
                             "Argument given by name ('%s') "
                             "and position (%d)",
                             keyword, i+1);
                return cleanreturn(0, freelist);
            }
        }
        else if (nkeywords && PyErr_Occurred())
            return cleanreturn(0, freelist);
        else if (i < nargs)
            current_arg = PyTuple_GET_ITEM(args, i);

        if (current_arg) {
            msg = convertitem(current_arg, &format, p_va, flags,
                levels, msgbuf, sizeof(msgbuf), &freelist);
            if (msg) {
                seterror(i+1, msg, levels, fname, custom_msg);
                return cleanreturn(0, freelist);
            }
            continue;
        }

        if (i < min) {
            PyErr_Format(PyExc_TypeError, "Required argument "
                         "'%s' (pos %d) not found",
                         keyword, i+1);
            return cleanreturn(0, freelist);
        }
        /* current code reports success when all required args
         * fulfilled and no keyword args left, with no further
         * validation. XXX Maybe skip this in debug build ?
         */
        if (!nkeywords)
            return cleanreturn(1, freelist);

        /* We are into optional args, skip thru to any remaining
         * keyword args */
        msg = skipitem(&format, p_va, flags);
        if (msg) {
            PyErr_Format(PyExc_RuntimeError, "%s: '%s'", msg,
                         format);
            return cleanreturn(0, freelist);
        }
    }

    if (!IS_END_OF_FORMAT(*format) && *format != '|') {
        PyErr_Format(PyExc_RuntimeError,
            "more argument specifiers than keyword list entries "
            "(remaining format:'%s')", format);
        return cleanreturn(0, freelist);
    }

    /* make sure there are no extraneous keyword arguments */
    if (nkeywords > 0) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(keywords, &pos, &key, &value)) {
            int match = 0;
            char *ks;
            if (!PyUnicode_Check(key)) {
                PyErr_SetString(PyExc_TypeError,
                                "keywords must be strings");
                return cleanreturn(0, freelist);
            }
            /* check that _PyUnicode_AsString() result is not NULL */
            ks = _PyUnicode_AsString(key);
            if (ks != NULL) {
                for (i = 0; i < len; i++) {
                    if (!strcmp(ks, kwlist[i])) {
                        match = 1;
                        break;
                    }
                }
            }
            if (!match) {
                PyErr_Format(PyExc_TypeError,
                             "'%U' is an invalid keyword "
                             "argument for this function",
                             key);
                return cleanreturn(0, freelist);
            }
        }
    }

    return cleanreturn(1, freelist);
}


static char *
skipitem(const char **p_format, va_list *p_va, int flags)
{
    const char *format = *p_format;
    char c = *format++;

    switch (c) {

    /* simple codes
     * The individual types (second arg of va_arg) are irrelevant */

    case 'b': /* byte -- very short int */
    case 'B': /* byte as bitfield */
    case 'h': /* short int */
    case 'H': /* short int as bitfield */
    case 'i': /* int */
    case 'I': /* int sized bitfield */
    case 'l': /* long int */
    case 'k': /* long int sized bitfield */
#ifdef HAVE_LONG_LONG
    case 'L': /* PY_LONG_LONG */
    case 'K': /* PY_LONG_LONG sized bitfield */
#endif
    case 'f': /* float */
    case 'd': /* double */
    case 'D': /* complex double */
    case 'c': /* char */
    case 'C': /* unicode char */
        {
            (void) va_arg(*p_va, void *);
            break;
        }

    case 'n': /* Py_ssize_t */
        {
            (void) va_arg(*p_va, Py_ssize_t *);
            break;
        }

    /* string codes */

    case 'e': /* string with encoding */
        {
            (void) va_arg(*p_va, const char *);
            if (!(*format == 's' || *format == 't'))
                /* after 'e', only 's' and 't' is allowed */
                goto err;
            format++;
            /* explicit fallthrough to string cases */
        }

    case 's': /* string */
    case 'z': /* string or None */
    case 'y': /* bytes */
    case 'u': /* unicode string */
    case 'w': /* buffer, read-write */
        {
            (void) va_arg(*p_va, char **);
            if (*format == '#') {
                if (flags & FLAG_SIZE_T)
                    (void) va_arg(*p_va, Py_ssize_t *);
                else
                    (void) va_arg(*p_va, int *);
                format++;
            } else if ((c == 's' || c == 'z' || c == 'y') && *format == '*') {
                format++;
            }
            break;
        }

    /* object codes */

    case 'S': /* string object */
    case 'Y': /* string object */
    case 'U': /* unicode string object */
        {
            (void) va_arg(*p_va, PyObject **);
            break;
        }

    case 'O': /* object */
        {
            if (*format == '!') {
                format++;
                (void) va_arg(*p_va, PyTypeObject*);
                (void) va_arg(*p_va, PyObject **);
            }
            else if (*format == '&') {
                typedef int (*converter)(PyObject *, void *);
                (void) va_arg(*p_va, converter);
                (void) va_arg(*p_va, void *);
                format++;
            }
            else {
                (void) va_arg(*p_va, PyObject **);
            }
            break;
        }

    case '(':           /* bypass tuple, not handled at all previously */
        {
            char *msg;
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


int
PyArg_UnpackTuple(PyObject *args, const char *name, Py_ssize_t min, Py_ssize_t max, ...)
{
    Py_ssize_t i, l;
    PyObject **o;
    va_list vargs;

#ifdef HAVE_STDARG_PROTOTYPES
    va_start(vargs, max);
#else
    va_start(vargs);
#endif

    assert(min >= 0);
    assert(min <= max);
    if (!PyTuple_Check(args)) {
        PyErr_SetString(PyExc_SystemError,
            "PyArg_UnpackTuple() argument list is not a tuple");
        return 0;
    }
    l = PyTuple_GET_SIZE(args);
    if (l < min) {
        if (name != NULL)
            PyErr_Format(
                PyExc_TypeError,
                "%s expected %s%zd arguments, got %zd",
                name, (min == max ? "" : "at least "), min, l);
        else
            PyErr_Format(
                PyExc_TypeError,
                "unpacked tuple should have %s%zd elements,"
                " but has %zd",
                (min == max ? "" : "at least "), min, l);
        va_end(vargs);
        return 0;
    }
    if (l > max) {
        if (name != NULL)
            PyErr_Format(
                PyExc_TypeError,
                "%s expected %s%zd arguments, got %zd",
                name, (min == max ? "" : "at most "), max, l);
        else
            PyErr_Format(
                PyExc_TypeError,
                "unpacked tuple should have %s%zd elements,"
                " but has %zd",
                (min == max ? "" : "at most "), max, l);
        va_end(vargs);
        return 0;
    }
    for (i = 0; i < l; i++) {
        o = va_arg(vargs, PyObject **);
        *o = PyTuple_GET_ITEM(args, i);
    }
    va_end(vargs);
    return 1;
}


/* For type constructors that don't take keyword args
 *
 * Sets a TypeError and returns 0 if the kwds dict is
 * not empty, returns 1 otherwise
 */
int
_PyArg_NoKeywords(const char *funcname, PyObject *kw)
{
    if (kw == NULL)
        return 1;
    if (!PyDict_CheckExact(kw)) {
        PyErr_BadInternalCall();
        return 0;
    }
    if (PyDict_Size(kw) == 0)
        return 1;

    PyErr_Format(PyExc_TypeError, "%s does not take keyword arguments",
                    funcname);
    return 0;
}
#ifdef __cplusplus
};
#endif
