/* Coverity Scan model
 *
 * This is a modeling file for Coverity Scan. Modeling helps to avoid false
 * positives.
 *
 * - A model file can't import any header files.
 * - Therefore only some built-in primitives like int, char and void are
 *   available but not wchar_t, NULL etc.
 * - Modeling doesn't need full structs and typedefs. Rudimentary structs
 *   and similar types are sufficient.
 * - An uninitialized local pointer is not an error. It signifies that the
 *   variable could be either NULL or have some data.
 *
 * Coverity Scan doesn't pick up modifications automatically. The model file
 * must be uploaded by an admin in the analysis settings of
 * http://scan.coverity.com/projects/200
 */

/* dummy definitions, in most cases struct fields aren't required. */

#define NULL (void *)0
#define assert(op) /* empty */
typedef int sdigit;
typedef long Py_ssize_t;
typedef long long PY_LONG_LONG;
typedef unsigned short wchar_t;
typedef struct {} PyObject;
typedef struct {} grammar;
typedef struct {} DIR;
typedef struct {} RFILE;

/* Python/pythonrun.c
 * resourece leak false positive */

void Py_FatalError(const char *msg) {
    __coverity_panic__();
}

/* Objects/longobject.c
 * NEGATIVE_RETURNS false positive */

static PyObject *get_small_int(sdigit ival)
{
    /* Never returns NULL */
    PyObject *p;
    assert(p != NULL);
    return p;
}

PyObject *PyLong_FromLong(long ival)
{
    PyObject *p;
    int maybe;

    if ((ival >= -5) && (ival < 257 + 5)) {
        p = get_small_int(ival);
        assert(p != NULL);
        return p;
    }
    if (maybe)
        return p;
    else
        return NULL;
}

PyObject *PyLong_FromLongLong(PY_LONG_LONG ival)
{
    return PyLong_FromLong((long)ival);
}

PyObject *PyLong_FromSsize_t(Py_ssize_t ival)
{
    return PyLong_FromLong((long)ival);
}

/* tainted sinks
 *
 * Coverity considers argv, environ, read() data etc as tained.
 */

PyObject *PyErr_SetFromErrnoWithFilename(PyObject *exc, const char *filename)
{
    __coverity_tainted_data_sink__(filename);
    return NULL;
}

/* Python/fileutils.c */
wchar_t *_Py_char2wchar(const char* arg, size_t *size)
{
   wchar_t *w;
    __coverity_tainted_data_sink__(arg);
    __coverity_tainted_data_sink__(size);
   return w;
}

/* Parser/pgenmain.c */
grammar *getgrammar(char *filename)
{
    grammar *g;
    __coverity_tainted_data_sink__(filename);
    return g;
}

/* Python/marshal.c */

static Py_ssize_t r_string(char *s, Py_ssize_t n, RFILE *p)
{
    __coverity_tainted_string_argument__(s);
    return 0;
}

static long r_long(RFILE *p)
{
    long l;
    unsigned char buffer[4];

    r_string((char *)buffer, 4, p);
    __coverity_tainted_string_sanitize_content__(buffer);
    l = (long)buffer;
    return l;
}

/* Coverity doesn't understand that fdopendir() may take ownership of fd. */

DIR *fdopendir(int fd) {
    DIR *d;
    if (d) {
        __coverity_close__(fd);
    }
    return d;
}

