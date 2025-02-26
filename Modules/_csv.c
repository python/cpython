/* csv module */

/*

This module provides the low-level underpinnings of a CSV reading/writing
module.  Users should not use this module directly, but import the csv.py
module instead.

*/

// clinic/_csv.c.h uses internal pycore_modsupport.h API
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_pyatomic_ft_wrappers.h"

#include <stddef.h>               // offsetof()
#include <stdbool.h>

/*[clinic input]
module _csv
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=385118b71aa43706]*/

#include "clinic/_csv.c.h"
#define NOT_SET ((Py_UCS4)-1)
#define EOL ((Py_UCS4)-2)


typedef struct {
    PyObject *error_obj;   /* CSV exception */
    PyObject *dialects;   /* Dialect registry */
    PyTypeObject *dialect_type;
    PyTypeObject *reader_type;
    PyTypeObject *writer_type;
    Py_ssize_t field_limit;   /* max parsed field size */
    PyObject *str_write;
} _csvstate;

static struct PyModuleDef _csvmodule;

static inline _csvstate*
get_csv_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_csvstate *)state;
}

static int
_csv_clear(PyObject *module)
{
    _csvstate *module_state = PyModule_GetState(module);
    Py_CLEAR(module_state->error_obj);
    Py_CLEAR(module_state->dialects);
    Py_CLEAR(module_state->dialect_type);
    Py_CLEAR(module_state->reader_type);
    Py_CLEAR(module_state->writer_type);
    Py_CLEAR(module_state->str_write);
    return 0;
}

static int
_csv_traverse(PyObject *module, visitproc visit, void *arg)
{
    _csvstate *module_state = PyModule_GetState(module);
    Py_VISIT(module_state->error_obj);
    Py_VISIT(module_state->dialects);
    Py_VISIT(module_state->dialect_type);
    Py_VISIT(module_state->reader_type);
    Py_VISIT(module_state->writer_type);
    return 0;
}

static void
_csv_free(void *module)
{
   _csv_clear((PyObject *)module);
}

typedef enum {
    START_RECORD, START_FIELD, ESCAPED_CHAR, IN_FIELD,
    IN_QUOTED_FIELD, ESCAPE_IN_QUOTED_FIELD, QUOTE_IN_QUOTED_FIELD,
    EAT_CRNL,AFTER_ESCAPED_CRNL
} ParserState;

typedef enum {
    QUOTE_MINIMAL, QUOTE_ALL, QUOTE_NONNUMERIC, QUOTE_NONE,
    QUOTE_STRINGS, QUOTE_NOTNULL
} QuoteStyle;

typedef struct {
    QuoteStyle style;
    const char *name;
} StyleDesc;

static const StyleDesc quote_styles[] = {
    { QUOTE_MINIMAL,    "QUOTE_MINIMAL" },
    { QUOTE_ALL,        "QUOTE_ALL" },
    { QUOTE_NONNUMERIC, "QUOTE_NONNUMERIC" },
    { QUOTE_NONE,       "QUOTE_NONE" },
    { QUOTE_STRINGS,    "QUOTE_STRINGS" },
    { QUOTE_NOTNULL,    "QUOTE_NOTNULL" },
    { 0 }
};

typedef struct {
    PyObject_HEAD

    char doublequote;           /* is " represented by ""? */
    char skipinitialspace;      /* ignore spaces following delimiter? */
    char strict;                /* raise exception on bad CSV */
    int quoting;                /* style of quoting to write */
    Py_UCS4 delimiter;          /* field separator */
    Py_UCS4 quotechar;          /* quote character */
    Py_UCS4 escapechar;         /* escape character */
    PyObject *lineterminator;   /* string to write between records */

} DialectObj;

typedef struct {
    PyObject_HEAD

    PyObject *input_iter;   /* iterate over this for input lines */

    DialectObj *dialect;    /* parsing dialect */

    PyObject *fields;           /* field list for current record */
    ParserState state;          /* current CSV parse state */
    Py_UCS4 *field;             /* temporary buffer */
    Py_ssize_t field_size;      /* size of allocated buffer */
    Py_ssize_t field_len;       /* length of current field */
    bool unquoted_field;        /* true if no quotes around the current field */
    unsigned long line_num;     /* Source-file line number */
} ReaderObj;

typedef struct {
    PyObject_HEAD

    PyObject *write;    /* write output lines to this file */

    DialectObj *dialect;    /* parsing dialect */

    Py_UCS4 *rec;            /* buffer for parser.join */
    Py_ssize_t rec_size;        /* size of allocated record */
    Py_ssize_t rec_len;         /* length of record */
    int num_fields;             /* number of fields in record */

    PyObject *error_obj;       /* cached error object */
} WriterObj;

/*
 * DIALECT class
 */

static PyObject *
get_dialect_from_registry(PyObject *name_obj, _csvstate *module_state)
{
    PyObject *dialect_obj;
    if (PyDict_GetItemRef(module_state->dialects, name_obj, &dialect_obj) == 0) {
        PyErr_SetString(module_state->error_obj, "unknown dialect");
    }
    return dialect_obj;
}

static PyObject *
get_char_or_None(Py_UCS4 c)
{
    if (c == NOT_SET) {
        Py_RETURN_NONE;
    }
    else
        return PyUnicode_FromOrdinal(c);
}

static PyObject *
Dialect_get_lineterminator(DialectObj *self, void *Py_UNUSED(ignored))
{
    return Py_XNewRef(self->lineterminator);
}

static PyObject *
Dialect_get_delimiter(DialectObj *self, void *Py_UNUSED(ignored))
{
    return get_char_or_None(self->delimiter);
}

static PyObject *
Dialect_get_escapechar(DialectObj *self, void *Py_UNUSED(ignored))
{
    return get_char_or_None(self->escapechar);
}

static PyObject *
Dialect_get_quotechar(DialectObj *self, void *Py_UNUSED(ignored))
{
    return get_char_or_None(self->quotechar);
}

static PyObject *
Dialect_get_quoting(DialectObj *self, void *Py_UNUSED(ignored))
{
    return PyLong_FromLong(self->quoting);
}

static int
_set_bool(const char *name, char *target, PyObject *src, bool dflt)
{
    if (src == NULL)
        *target = dflt;
    else {
        int b = PyObject_IsTrue(src);
        if (b < 0)
            return -1;
        *target = (char)b;
    }
    return 0;
}

static int
_set_int(const char *name, int *target, PyObject *src, int dflt)
{
    if (src == NULL)
        *target = dflt;
    else {
        int value;
        if (!PyLong_CheckExact(src)) {
            PyErr_Format(PyExc_TypeError,
                         "\"%s\" must be an integer", name);
            return -1;
        }
        value = PyLong_AsInt(src);
        if (value == -1 && PyErr_Occurred()) {
            return -1;
        }
        *target = value;
    }
    return 0;
}

static int
_set_char_or_none(const char *name, Py_UCS4 *target, PyObject *src, Py_UCS4 dflt)
{
    if (src == NULL) {
        *target = dflt;
    }
    else {
        *target = NOT_SET;
        if (src != Py_None) {
            if (!PyUnicode_Check(src)) {
                PyErr_Format(PyExc_TypeError,
                    "\"%s\" must be string or None, not %.200s", name,
                    Py_TYPE(src)->tp_name);
                return -1;
            }
            Py_ssize_t len = PyUnicode_GetLength(src);
            if (len < 0) {
                return -1;
            }
            if (len != 1) {
                PyErr_Format(PyExc_TypeError,
                    "\"%s\" must be a 1-character string",
                    name);
                return -1;
            }
            *target = PyUnicode_READ_CHAR(src, 0);
        }
    }
    return 0;
}

static int
_set_char(const char *name, Py_UCS4 *target, PyObject *src, Py_UCS4 dflt)
{
    if (src == NULL) {
        *target = dflt;
    }
    else {
        if (!PyUnicode_Check(src)) {
            PyErr_Format(PyExc_TypeError,
                         "\"%s\" must be string, not %.200s", name,
                         Py_TYPE(src)->tp_name);
                return -1;
        }
        Py_ssize_t len = PyUnicode_GetLength(src);
        if (len < 0) {
            return -1;
        }
        if (len != 1) {
            PyErr_Format(PyExc_TypeError,
                         "\"%s\" must be a 1-character string",
                         name);
            return -1;
        }
        *target = PyUnicode_READ_CHAR(src, 0);
    }
    return 0;
}

static int
_set_str(const char *name, PyObject **target, PyObject *src, const char *dflt)
{
    if (src == NULL)
        *target = PyUnicode_DecodeASCII(dflt, strlen(dflt), NULL);
    else {
        if (src == Py_None)
            *target = NULL;
        else if (!PyUnicode_Check(src)) {
            PyErr_Format(PyExc_TypeError,
                         "\"%s\" must be a string", name);
            return -1;
        }
        else {
            Py_XSETREF(*target, Py_NewRef(src));
        }
    }
    return 0;
}

static int
dialect_check_quoting(int quoting)
{
    const StyleDesc *qs;

    for (qs = quote_styles; qs->name; qs++) {
        if ((int)qs->style == quoting)
            return 0;
    }
    PyErr_Format(PyExc_TypeError, "bad \"quoting\" value");
    return -1;
}

static int
dialect_check_char(const char *name, Py_UCS4 c, DialectObj *dialect, bool allowspace)
{
    if (c == '\r' || c == '\n' || (c == ' ' && !allowspace)) {
        PyErr_Format(PyExc_ValueError, "bad %s value", name);
        return -1;
    }
    if (PyUnicode_FindChar(
        dialect->lineterminator, c, 0,
        PyUnicode_GET_LENGTH(dialect->lineterminator), 1) >= 0)
    {
        PyErr_Format(PyExc_ValueError, "bad %s or lineterminator value", name);
        return -1;
    }
    return 0;
}

 static int
dialect_check_chars(const char *name1, const char *name2, Py_UCS4 c1, Py_UCS4 c2)
{
    if (c1 == c2 && c1 != NOT_SET) {
        PyErr_Format(PyExc_ValueError, "bad %s or %s value", name1, name2);
        return -1;
    }
    return 0;
}

#define D_OFF(x) offsetof(DialectObj, x)

static struct PyMemberDef Dialect_memberlist[] = {
    { "skipinitialspace",   Py_T_BOOL, D_OFF(skipinitialspace), Py_READONLY },
    { "doublequote",        Py_T_BOOL, D_OFF(doublequote), Py_READONLY },
    { "strict",             Py_T_BOOL, D_OFF(strict), Py_READONLY },
    { NULL }
};

static PyGetSetDef Dialect_getsetlist[] = {
    { "delimiter",          (getter)Dialect_get_delimiter},
    { "escapechar",             (getter)Dialect_get_escapechar},
    { "lineterminator",         (getter)Dialect_get_lineterminator},
    { "quotechar",              (getter)Dialect_get_quotechar},
    { "quoting",                (getter)Dialect_get_quoting},
    {NULL},
};

static void
Dialect_dealloc(DialectObj *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    tp->tp_clear((PyObject *)self);
    PyObject_GC_Del(self);
    Py_DECREF(tp);
}

static char *dialect_kws[] = {
    "dialect",
    "delimiter",
    "doublequote",
    "escapechar",
    "lineterminator",
    "quotechar",
    "quoting",
    "skipinitialspace",
    "strict",
    NULL
};

static _csvstate *
_csv_state_from_type(PyTypeObject *type, const char *name)
{
    PyObject *module = PyType_GetModuleByDef(type, &_csvmodule);
    if (module == NULL) {
        return NULL;
    }
    _csvstate *module_state = PyModule_GetState(module);
    if (module_state == NULL) {
        PyErr_Format(PyExc_SystemError,
                     "%s: No _csv module state found", name);
        return NULL;
    }
    return module_state;
}

static PyObject *
dialect_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    DialectObj *self;
    PyObject *ret = NULL;
    PyObject *dialect = NULL;
    PyObject *delimiter = NULL;
    PyObject *doublequote = NULL;
    PyObject *escapechar = NULL;
    PyObject *lineterminator = NULL;
    PyObject *quotechar = NULL;
    PyObject *quoting = NULL;
    PyObject *skipinitialspace = NULL;
    PyObject *strict = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "|OOOOOOOOO", dialect_kws,
                                     &dialect,
                                     &delimiter,
                                     &doublequote,
                                     &escapechar,
                                     &lineterminator,
                                     &quotechar,
                                     &quoting,
                                     &skipinitialspace,
                                     &strict))
        return NULL;

    _csvstate *module_state = _csv_state_from_type(type, "dialect_new");
    if (module_state == NULL) {
        return NULL;
    }

    if (dialect != NULL) {
        if (PyUnicode_Check(dialect)) {
            dialect = get_dialect_from_registry(dialect, module_state);
            if (dialect == NULL)
                return NULL;
        }
        else
            Py_INCREF(dialect);
        /* Can we reuse this instance? */
        if (PyObject_TypeCheck(dialect, module_state->dialect_type) &&
            delimiter == NULL &&
            doublequote == NULL &&
            escapechar == NULL &&
            lineterminator == NULL &&
            quotechar == NULL &&
            quoting == NULL &&
            skipinitialspace == NULL &&
            strict == NULL)
            return dialect;
    }

    self = (DialectObj *)type->tp_alloc(type, 0);
    if (self == NULL) {
        Py_CLEAR(dialect);
        return NULL;
    }
    self->lineterminator = NULL;

    Py_XINCREF(delimiter);
    Py_XINCREF(doublequote);
    Py_XINCREF(escapechar);
    Py_XINCREF(lineterminator);
    Py_XINCREF(quotechar);
    Py_XINCREF(quoting);
    Py_XINCREF(skipinitialspace);
    Py_XINCREF(strict);
    if (dialect != NULL) {
#define DIALECT_GETATTR(v, n)                            \
        do {                                             \
            if (v == NULL) {                             \
                v = PyObject_GetAttrString(dialect, n);  \
                if (v == NULL)                           \
                    PyErr_Clear();                       \
            }                                            \
        } while (0)
        DIALECT_GETATTR(delimiter, "delimiter");
        DIALECT_GETATTR(doublequote, "doublequote");
        DIALECT_GETATTR(escapechar, "escapechar");
        DIALECT_GETATTR(lineterminator, "lineterminator");
        DIALECT_GETATTR(quotechar, "quotechar");
        DIALECT_GETATTR(quoting, "quoting");
        DIALECT_GETATTR(skipinitialspace, "skipinitialspace");
        DIALECT_GETATTR(strict, "strict");
    }

    /* check types and convert to C values */
#define DIASET(meth, name, target, src, dflt) \
    if (meth(name, target, src, dflt)) \
        goto err
    DIASET(_set_char, "delimiter", &self->delimiter, delimiter, ',');
    DIASET(_set_bool, "doublequote", &self->doublequote, doublequote, true);
    DIASET(_set_char_or_none, "escapechar", &self->escapechar, escapechar, NOT_SET);
    DIASET(_set_str, "lineterminator", &self->lineterminator, lineterminator, "\r\n");
    DIASET(_set_char_or_none, "quotechar", &self->quotechar, quotechar, '"');
    DIASET(_set_int, "quoting", &self->quoting, quoting, QUOTE_MINIMAL);
    DIASET(_set_bool, "skipinitialspace", &self->skipinitialspace, skipinitialspace, false);
    DIASET(_set_bool, "strict", &self->strict, strict, false);

    /* validate options */
    if (dialect_check_quoting(self->quoting))
        goto err;
    if (self->delimiter == NOT_SET) {
        PyErr_SetString(PyExc_TypeError,
                        "\"delimiter\" must be a 1-character string");
        goto err;
    }
    if (quotechar == Py_None && quoting == NULL)
        self->quoting = QUOTE_NONE;
    if (self->quoting != QUOTE_NONE && self->quotechar == NOT_SET) {
        PyErr_SetString(PyExc_TypeError,
                        "quotechar must be set if quoting enabled");
        goto err;
    }
    if (self->lineterminator == NULL) {
        PyErr_SetString(PyExc_TypeError, "lineterminator must be set");
        goto err;
    }
    if (dialect_check_char("delimiter", self->delimiter, self, true) ||
        dialect_check_char("escapechar", self->escapechar, self,
                           !self->skipinitialspace) ||
        dialect_check_char("quotechar", self->quotechar, self,
                           !self->skipinitialspace) ||
        dialect_check_chars("delimiter", "escapechar",
                            self->delimiter, self->escapechar) ||
        dialect_check_chars("delimiter", "quotechar",
                            self->delimiter, self->quotechar) ||
        dialect_check_chars("escapechar", "quotechar",
                            self->escapechar, self->quotechar))
    {
        goto err;
    }

    ret = Py_NewRef(self);
err:
    Py_CLEAR(self);
    Py_CLEAR(dialect);
    Py_CLEAR(delimiter);
    Py_CLEAR(doublequote);
    Py_CLEAR(escapechar);
    Py_CLEAR(lineterminator);
    Py_CLEAR(quotechar);
    Py_CLEAR(quoting);
    Py_CLEAR(skipinitialspace);
    Py_CLEAR(strict);
    return ret;
}

/* Since dialect is now a heap type, it inherits pickling method for
 * protocol 0 and 1 from object, therefore it needs to be overridden */

PyDoc_STRVAR(dialect_reduce_doc, "raises an exception to avoid pickling");

static PyObject *
Dialect_reduce(PyObject *self, PyObject *args) {
    PyErr_Format(PyExc_TypeError,
        "cannot pickle '%.100s' instances", _PyType_Name(Py_TYPE(self)));
    return NULL;
}

static struct PyMethodDef dialect_methods[] = {
    {"__reduce__", Dialect_reduce, METH_VARARGS, dialect_reduce_doc},
    {"__reduce_ex__", Dialect_reduce, METH_VARARGS, dialect_reduce_doc},
    {NULL, NULL}
};

PyDoc_STRVAR(Dialect_Type_doc,
"CSV dialect\n"
"\n"
"The Dialect type records CSV parsing and generation options.\n");

static int
Dialect_clear(DialectObj *self)
{
    Py_CLEAR(self->lineterminator);
    return 0;
}

static int
Dialect_traverse(DialectObj *self, visitproc visit, void *arg)
{
    Py_VISIT(self->lineterminator);
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static PyType_Slot Dialect_Type_slots[] = {
    {Py_tp_doc, (char*)Dialect_Type_doc},
    {Py_tp_members, Dialect_memberlist},
    {Py_tp_getset, Dialect_getsetlist},
    {Py_tp_new, dialect_new},
    {Py_tp_methods, dialect_methods},
    {Py_tp_dealloc, Dialect_dealloc},
    {Py_tp_clear, Dialect_clear},
    {Py_tp_traverse, Dialect_traverse},
    {0, NULL}
};

PyType_Spec Dialect_Type_spec = {
    .name = "_csv.Dialect",
    .basicsize = sizeof(DialectObj),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = Dialect_Type_slots,
};


/*
 * Return an instance of the dialect type, given a Python instance or kwarg
 * description of the dialect
 */
static PyObject *
_call_dialect(_csvstate *module_state, PyObject *dialect_inst, PyObject *kwargs)
{
    PyObject *type = (PyObject *)module_state->dialect_type;
    if (dialect_inst) {
        return PyObject_VectorcallDict(type, &dialect_inst, 1, kwargs);
    }
    else {
        return PyObject_VectorcallDict(type, NULL, 0, kwargs);
    }
}

/*
 * READER
 */
static int
parse_save_field(ReaderObj *self)
{
    int quoting = self->dialect->quoting;
    PyObject *field;

    if (self->unquoted_field &&
        self->field_len == 0 &&
        (quoting == QUOTE_NOTNULL || quoting == QUOTE_STRINGS))
    {
        field = Py_NewRef(Py_None);
    }
    else {
        field = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND,
                                        (void *) self->field, self->field_len);
        if (field == NULL) {
            return -1;
        }
        if (self->unquoted_field &&
            self->field_len != 0 &&
            (quoting == QUOTE_NONNUMERIC || quoting == QUOTE_STRINGS))
        {
            PyObject *tmp = PyNumber_Float(field);
            Py_DECREF(field);
            if (tmp == NULL) {
                return -1;
            }
            field = tmp;
        }
        self->field_len = 0;
    }
    if (PyList_Append(self->fields, field) < 0) {
        Py_DECREF(field);
        return -1;
    }
    Py_DECREF(field);
    return 0;
}

static int
parse_grow_buff(ReaderObj *self)
{
    assert((size_t)self->field_size <= PY_SSIZE_T_MAX / sizeof(Py_UCS4));

    Py_ssize_t field_size_new = self->field_size ? 2 * self->field_size : 4096;
    Py_UCS4 *field_new = self->field;
    PyMem_Resize(field_new, Py_UCS4, field_size_new);
    if (field_new == NULL) {
        PyErr_NoMemory();
        return 0;
    }
    self->field = field_new;
    self->field_size = field_size_new;
    return 1;
}

static int
parse_add_char(ReaderObj *self, _csvstate *module_state, Py_UCS4 c)
{
    Py_ssize_t field_limit = FT_ATOMIC_LOAD_SSIZE_RELAXED(module_state->field_limit);
    if (self->field_len >= field_limit) {
        PyErr_Format(module_state->error_obj,
                     "field larger than field limit (%zd)",
                     field_limit);
        return -1;
    }
    if (self->field_len == self->field_size && !parse_grow_buff(self))
        return -1;
    self->field[self->field_len++] = c;
    return 0;
}

static int
parse_process_char(ReaderObj *self, _csvstate *module_state, Py_UCS4 c)
{
    DialectObj *dialect = self->dialect;

    switch (self->state) {
    case START_RECORD:
        /* start of record */
        if (c == EOL)
            /* empty line - return [] */
            break;
        else if (c == '\n' || c == '\r') {
            self->state = EAT_CRNL;
            break;
        }
        /* normal character - handle as START_FIELD */
        self->state = START_FIELD;
        /* fallthru */
    case START_FIELD:
        /* expecting field */
        self->unquoted_field = true;
        if (c == '\n' || c == '\r' || c == EOL) {
            /* save empty field - return [fields] */
            if (parse_save_field(self) < 0)
                return -1;
            self->state = (c == EOL ? START_RECORD : EAT_CRNL);
        }
        else if (c == dialect->quotechar &&
                 dialect->quoting != QUOTE_NONE) {
            /* start quoted field */
            self->unquoted_field = false;
            self->state = IN_QUOTED_FIELD;
        }
        else if (c == dialect->escapechar) {
            /* possible escaped character */
            self->state = ESCAPED_CHAR;
        }
        else if (c == ' ' && dialect->skipinitialspace)
            /* ignore spaces at start of field */
            ;
        else if (c == dialect->delimiter) {
            /* save empty field */
            if (parse_save_field(self) < 0)
                return -1;
        }
        else {
            /* begin new unquoted field */
            if (parse_add_char(self, module_state, c) < 0)
                return -1;
            self->state = IN_FIELD;
        }
        break;

    case ESCAPED_CHAR:
        if (c == '\n' || c=='\r') {
            if (parse_add_char(self, module_state, c) < 0)
                return -1;
            self->state = AFTER_ESCAPED_CRNL;
            break;
        }
        if (c == EOL)
            c = '\n';
        if (parse_add_char(self, module_state, c) < 0)
            return -1;
        self->state = IN_FIELD;
        break;

    case AFTER_ESCAPED_CRNL:
        if (c == EOL)
            break;
        /*fallthru*/

    case IN_FIELD:
        /* in unquoted field */
        if (c == '\n' || c == '\r' || c == EOL) {
            /* end of line - return [fields] */
            if (parse_save_field(self) < 0)
                return -1;
            self->state = (c == EOL ? START_RECORD : EAT_CRNL);
        }
        else if (c == dialect->escapechar) {
            /* possible escaped character */
            self->state = ESCAPED_CHAR;
        }
        else if (c == dialect->delimiter) {
            /* save field - wait for new field */
            if (parse_save_field(self) < 0)
                return -1;
            self->state = START_FIELD;
        }
        else {
            /* normal character - save in field */
            if (parse_add_char(self, module_state, c) < 0)
                return -1;
        }
        break;

    case IN_QUOTED_FIELD:
        /* in quoted field */
        if (c == EOL)
            ;
        else if (c == dialect->escapechar) {
            /* Possible escape character */
            self->state = ESCAPE_IN_QUOTED_FIELD;
        }
        else if (c == dialect->quotechar &&
                 dialect->quoting != QUOTE_NONE) {
            if (dialect->doublequote) {
                /* doublequote; " represented by "" */
                self->state = QUOTE_IN_QUOTED_FIELD;
            }
            else {
                /* end of quote part of field */
                self->state = IN_FIELD;
            }
        }
        else {
            /* normal character - save in field */
            if (parse_add_char(self, module_state, c) < 0)
                return -1;
        }
        break;

    case ESCAPE_IN_QUOTED_FIELD:
        if (c == EOL)
            c = '\n';
        if (parse_add_char(self, module_state, c) < 0)
            return -1;
        self->state = IN_QUOTED_FIELD;
        break;

    case QUOTE_IN_QUOTED_FIELD:
        /* doublequote - seen a quote in a quoted field */
        if (dialect->quoting != QUOTE_NONE &&
            c == dialect->quotechar) {
            /* save "" as " */
            if (parse_add_char(self, module_state, c) < 0)
                return -1;
            self->state = IN_QUOTED_FIELD;
        }
        else if (c == dialect->delimiter) {
            /* save field - wait for new field */
            if (parse_save_field(self) < 0)
                return -1;
            self->state = START_FIELD;
        }
        else if (c == '\n' || c == '\r' || c == EOL) {
            /* end of line - return [fields] */
            if (parse_save_field(self) < 0)
                return -1;
            self->state = (c == EOL ? START_RECORD : EAT_CRNL);
        }
        else if (!dialect->strict) {
            if (parse_add_char(self, module_state, c) < 0)
                return -1;
            self->state = IN_FIELD;
        }
        else {
            /* illegal */
            PyErr_Format(module_state->error_obj, "'%c' expected after '%c'",
                            dialect->delimiter,
                            dialect->quotechar);
            return -1;
        }
        break;

    case EAT_CRNL:
        if (c == '\n' || c == '\r')
            ;
        else if (c == EOL)
            self->state = START_RECORD;
        else {
            PyErr_Format(module_state->error_obj,
                         "new-line character seen in unquoted field - "
                         "do you need to open the file with newline=''?");
            return -1;
        }
        break;

    }
    return 0;
}

static int
parse_reset(ReaderObj *self)
{
    Py_XSETREF(self->fields, PyList_New(0));
    if (self->fields == NULL)
        return -1;
    self->field_len = 0;
    self->state = START_RECORD;
    self->unquoted_field = false;
    return 0;
}

static PyObject *
Reader_iternext(ReaderObj *self)
{
    PyObject *fields = NULL;
    Py_UCS4 c;
    Py_ssize_t pos, linelen;
    int kind;
    const void *data;
    PyObject *lineobj;

    _csvstate *module_state = _csv_state_from_type(Py_TYPE(self),
                                                   "Reader.__next__");
    if (module_state == NULL) {
        return NULL;
    }

    if (parse_reset(self) < 0)
        return NULL;
    do {
        lineobj = PyIter_Next(self->input_iter);
        if (lineobj == NULL) {
            /* End of input OR exception */
            if (!PyErr_Occurred() && (self->field_len != 0 ||
                                      self->state == IN_QUOTED_FIELD)) {
                if (self->dialect->strict)
                    PyErr_SetString(module_state->error_obj,
                                    "unexpected end of data");
                else if (parse_save_field(self) >= 0)
                    break;
            }
            return NULL;
        }
        if (!PyUnicode_Check(lineobj)) {
            PyErr_Format(module_state->error_obj,
                         "iterator should return strings, "
                         "not %.200s "
                         "(the file should be opened in text mode)",
                         Py_TYPE(lineobj)->tp_name
                );
            Py_DECREF(lineobj);
            return NULL;
        }
        ++self->line_num;
        kind = PyUnicode_KIND(lineobj);
        data = PyUnicode_DATA(lineobj);
        pos = 0;
        linelen = PyUnicode_GET_LENGTH(lineobj);
        while (linelen--) {
            c = PyUnicode_READ(kind, data, pos);
            if (parse_process_char(self, module_state, c) < 0) {
                Py_DECREF(lineobj);
                goto err;
            }
            pos++;
        }
        Py_DECREF(lineobj);
        if (parse_process_char(self, module_state, EOL) < 0)
            goto err;
    } while (self->state != START_RECORD);

    fields = self->fields;
    self->fields = NULL;
err:
    return fields;
}

static void
Reader_dealloc(ReaderObj *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    tp->tp_clear((PyObject *)self);
    if (self->field != NULL) {
        PyMem_Free(self->field);
        self->field = NULL;
    }
    PyObject_GC_Del(self);
    Py_DECREF(tp);
}

static int
Reader_traverse(ReaderObj *self, visitproc visit, void *arg)
{
    Py_VISIT(self->dialect);
    Py_VISIT(self->input_iter);
    Py_VISIT(self->fields);
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static int
Reader_clear(ReaderObj *self)
{
    Py_CLEAR(self->dialect);
    Py_CLEAR(self->input_iter);
    Py_CLEAR(self->fields);
    return 0;
}

PyDoc_STRVAR(Reader_Type_doc,
"CSV reader\n"
"\n"
"Reader objects are responsible for reading and parsing tabular data\n"
"in CSV format.\n"
);

static struct PyMethodDef Reader_methods[] = {
    { NULL, NULL }
};
#define R_OFF(x) offsetof(ReaderObj, x)

static struct PyMemberDef Reader_memberlist[] = {
    { "dialect", _Py_T_OBJECT, R_OFF(dialect), Py_READONLY },
    { "line_num", Py_T_ULONG, R_OFF(line_num), Py_READONLY },
    { NULL }
};


static PyType_Slot Reader_Type_slots[] = {
    {Py_tp_doc, (char*)Reader_Type_doc},
    {Py_tp_traverse, Reader_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, Reader_iternext},
    {Py_tp_methods, Reader_methods},
    {Py_tp_members, Reader_memberlist},
    {Py_tp_clear, Reader_clear},
    {Py_tp_dealloc, Reader_dealloc},
    {0, NULL}
};

PyType_Spec Reader_Type_spec = {
    .name = "_csv.reader",
    .basicsize = sizeof(ReaderObj),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION),
    .slots = Reader_Type_slots
};


static PyObject *
csv_reader(PyObject *module, PyObject *args, PyObject *keyword_args)
{
    PyObject * iterator, * dialect = NULL;
    _csvstate *module_state = get_csv_state(module);
    ReaderObj * self = PyObject_GC_New(
        ReaderObj,
        module_state->reader_type);

    if (!self)
        return NULL;

    self->dialect = NULL;
    self->fields = NULL;
    self->input_iter = NULL;
    self->field = NULL;
    self->field_size = 0;
    self->line_num = 0;

    if (parse_reset(self) < 0) {
        Py_DECREF(self);
        return NULL;
    }

    if (!PyArg_UnpackTuple(args, "", 1, 2, &iterator, &dialect)) {
        Py_DECREF(self);
        return NULL;
    }
    self->input_iter = PyObject_GetIter(iterator);
    if (self->input_iter == NULL) {
        Py_DECREF(self);
        return NULL;
    }
    self->dialect = (DialectObj *)_call_dialect(module_state, dialect,
                                                keyword_args);
    if (self->dialect == NULL) {
        Py_DECREF(self);
        return NULL;
    }

    PyObject_GC_Track(self);
    return (PyObject *)self;
}

/*
 * WRITER
 */
/* ---------------------------------------------------------------- */
static void
join_reset(WriterObj *self)
{
    self->rec_len = 0;
    self->num_fields = 0;
}

#define MEM_INCR 32768

/* Calculate new record length or append field to record.  Return new
 * record length.
 */
static Py_ssize_t
join_append_data(WriterObj *self, int field_kind, const void *field_data,
                 Py_ssize_t field_len, int *quoted,
                 int copy_phase)
{
    DialectObj *dialect = self->dialect;
    Py_ssize_t i;
    Py_ssize_t rec_len;

#define INCLEN \
    do {\
        if (!copy_phase && rec_len == PY_SSIZE_T_MAX) {    \
            goto overflow; \
        } \
        rec_len++; \
    } while(0)

#define ADDCH(c)                                \
    do {\
        if (copy_phase) \
            self->rec[rec_len] = c;\
        INCLEN;\
    } while(0)

    rec_len = self->rec_len;

    /* If this is not the first field we need a field separator */
    if (self->num_fields > 0)
        ADDCH(dialect->delimiter);

    /* Handle preceding quote */
    if (copy_phase && *quoted)
        ADDCH(dialect->quotechar);

    /* Copy/count field data */
    /* If field is null just pass over */
    for (i = 0; field_data && (i < field_len); i++) {
        Py_UCS4 c = PyUnicode_READ(field_kind, field_data, i);
        int want_escape = 0;

        if (c == dialect->delimiter ||
            c == dialect->escapechar ||
            c == dialect->quotechar  ||
            c == '\n'  ||
            c == '\r'  ||
            PyUnicode_FindChar(
                dialect->lineterminator, c, 0,
                PyUnicode_GET_LENGTH(dialect->lineterminator), 1) >= 0) {
            if (dialect->quoting == QUOTE_NONE)
                want_escape = 1;
            else {
                if (c == dialect->quotechar) {
                    if (dialect->doublequote)
                        ADDCH(dialect->quotechar);
                    else
                        want_escape = 1;
                }
                else if (c == dialect->escapechar) {
                    want_escape = 1;
                }
                if (!want_escape)
                    *quoted = 1;
            }
            if (want_escape) {
                if (dialect->escapechar == NOT_SET) {
                    PyErr_Format(self->error_obj,
                                 "need to escape, but no escapechar set");
                    return -1;
                }
                ADDCH(dialect->escapechar);
            }
        }
        /* Copy field character into record buffer.
         */
        ADDCH(c);
    }

    if (*quoted) {
        if (copy_phase)
            ADDCH(dialect->quotechar);
        else {
            INCLEN; /* starting quote */
            INCLEN; /* ending quote */
        }
    }
    return rec_len;

  overflow:
    PyErr_NoMemory();
    return -1;
#undef ADDCH
#undef INCLEN
}

static int
join_check_rec_size(WriterObj *self, Py_ssize_t rec_len)
{
    assert(rec_len >= 0);

    if (rec_len > self->rec_size) {
        size_t rec_size_new = (size_t)(rec_len / MEM_INCR + 1) * MEM_INCR;
        Py_UCS4 *rec_new = self->rec;
        PyMem_Resize(rec_new, Py_UCS4, rec_size_new);
        if (rec_new == NULL) {
            PyErr_NoMemory();
            return 0;
        }
        self->rec = rec_new;
        self->rec_size = (Py_ssize_t)rec_size_new;
    }
    return 1;
}

static int
join_append(WriterObj *self, PyObject *field, int quoted)
{
    DialectObj *dialect = self->dialect;
    int field_kind = -1;
    const void *field_data = NULL;
    Py_ssize_t field_len = 0;
    Py_ssize_t rec_len;

    if (field != NULL) {
        field_kind = PyUnicode_KIND(field);
        field_data = PyUnicode_DATA(field);
        field_len = PyUnicode_GET_LENGTH(field);
    }
    if (!field_len && dialect->delimiter == ' ' && dialect->skipinitialspace) {
        if (dialect->quoting == QUOTE_NONE ||
            (field == NULL &&
             (dialect->quoting == QUOTE_STRINGS ||
              dialect->quoting == QUOTE_NOTNULL)))
        {
            PyErr_Format(self->error_obj,
                         "empty field must be quoted if delimiter is a space "
                         "and skipinitialspace is true");
            return 0;
        }
        quoted = 1;
    }
    rec_len = join_append_data(self, field_kind, field_data, field_len,
                               &quoted, 0);
    if (rec_len < 0)
        return 0;

    /* grow record buffer if necessary */
    if (!join_check_rec_size(self, rec_len))
        return 0;

    self->rec_len = join_append_data(self, field_kind, field_data, field_len,
                                     &quoted, 1);
    self->num_fields++;

    return 1;
}

static int
join_append_lineterminator(WriterObj *self)
{
    Py_ssize_t terminator_len, i;
    int term_kind;
    const void *term_data;

    terminator_len = PyUnicode_GET_LENGTH(self->dialect->lineterminator);
    if (terminator_len == -1)
        return 0;

    /* grow record buffer if necessary */
    if (!join_check_rec_size(self, self->rec_len + terminator_len))
        return 0;

    term_kind = PyUnicode_KIND(self->dialect->lineterminator);
    term_data = PyUnicode_DATA(self->dialect->lineterminator);
    for (i = 0; i < terminator_len; i++)
        self->rec[self->rec_len + i] = PyUnicode_READ(term_kind, term_data, i);
    self->rec_len += terminator_len;

    return 1;
}

PyDoc_STRVAR(csv_writerow_doc,
"writerow(iterable)\n"
"\n"
"Construct and write a CSV record from an iterable of fields.  Non-string\n"
"elements will be converted to string.");

static PyObject *
csv_writerow(WriterObj *self, PyObject *seq)
{
    DialectObj *dialect = self->dialect;
    PyObject *iter, *field, *line, *result;
    bool null_field = false;

    iter = PyObject_GetIter(seq);
    if (iter == NULL) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_Format(self->error_obj,
                         "iterable expected, not %.200s",
                         Py_TYPE(seq)->tp_name);
        }
        return NULL;
    }

    /* Join all fields in internal buffer.
     */
    join_reset(self);
    while ((field = PyIter_Next(iter))) {
        int append_ok;
        int quoted;

        switch (dialect->quoting) {
        case QUOTE_NONNUMERIC:
            quoted = !PyNumber_Check(field);
            break;
        case QUOTE_ALL:
            quoted = 1;
            break;
        case QUOTE_STRINGS:
            quoted = PyUnicode_Check(field);
            break;
        case QUOTE_NOTNULL:
            quoted = field != Py_None;
            break;
        default:
            quoted = 0;
            break;
        }

        null_field = (field == Py_None);
        if (PyUnicode_Check(field)) {
            append_ok = join_append(self, field, quoted);
            Py_DECREF(field);
        }
        else if (null_field) {
            append_ok = join_append(self, NULL, quoted);
            Py_DECREF(field);
        }
        else {
            PyObject *str;

            str = PyObject_Str(field);
            Py_DECREF(field);
            if (str == NULL) {
                Py_DECREF(iter);
                return NULL;
            }
            append_ok = join_append(self, str, quoted);
            Py_DECREF(str);
        }
        if (!append_ok) {
            Py_DECREF(iter);
            return NULL;
        }
    }
    Py_DECREF(iter);
    if (PyErr_Occurred())
        return NULL;

    if (self->num_fields > 0 && self->rec_len == 0) {
        if (dialect->quoting == QUOTE_NONE ||
            (null_field &&
             (dialect->quoting == QUOTE_STRINGS ||
              dialect->quoting == QUOTE_NOTNULL)))
        {
            PyErr_Format(self->error_obj,
                "single empty field record must be quoted");
            return NULL;
        }
        self->num_fields--;
        if (!join_append(self, NULL, 1))
            return NULL;
    }

    /* Add line terminator.
     */
    if (!join_append_lineterminator(self)) {
        return NULL;
    }

    line = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND,
                                     (void *) self->rec, self->rec_len);
    if (line == NULL) {
        return NULL;
    }
    result = PyObject_CallOneArg(self->write, line);
    Py_DECREF(line);
    return result;
}

PyDoc_STRVAR(csv_writerows_doc,
"writerows(iterable of iterables)\n"
"\n"
"Construct and write a series of iterables to a csv file.  Non-string\n"
"elements will be converted to string.");

static PyObject *
csv_writerows(WriterObj *self, PyObject *seqseq)
{
    PyObject *row_iter, *row_obj, *result;

    row_iter = PyObject_GetIter(seqseq);
    if (row_iter == NULL) {
        return NULL;
    }
    while ((row_obj = PyIter_Next(row_iter))) {
        result = csv_writerow(self, row_obj);
        Py_DECREF(row_obj);
        if (!result) {
            Py_DECREF(row_iter);
            return NULL;
        }
        else
             Py_DECREF(result);
    }
    Py_DECREF(row_iter);
    if (PyErr_Occurred())
        return NULL;
    Py_RETURN_NONE;
}

static struct PyMethodDef Writer_methods[] = {
    { "writerow", (PyCFunction)csv_writerow, METH_O, csv_writerow_doc},
    { "writerows", (PyCFunction)csv_writerows, METH_O, csv_writerows_doc},
    { NULL, NULL }
};

#define W_OFF(x) offsetof(WriterObj, x)

static struct PyMemberDef Writer_memberlist[] = {
    { "dialect", _Py_T_OBJECT, W_OFF(dialect), Py_READONLY },
    { NULL }
};

static int
Writer_traverse(WriterObj *self, visitproc visit, void *arg)
{
    Py_VISIT(self->dialect);
    Py_VISIT(self->write);
    Py_VISIT(self->error_obj);
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static int
Writer_clear(WriterObj *self)
{
    Py_CLEAR(self->dialect);
    Py_CLEAR(self->write);
    Py_CLEAR(self->error_obj);
    return 0;
}

static void
Writer_dealloc(WriterObj *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    tp->tp_clear((PyObject *)self);
    if (self->rec != NULL) {
        PyMem_Free(self->rec);
    }
    PyObject_GC_Del(self);
    Py_DECREF(tp);
}

PyDoc_STRVAR(Writer_Type_doc,
"CSV writer\n"
"\n"
"Writer objects are responsible for generating tabular data\n"
"in CSV format from sequence input.\n"
);

static PyType_Slot Writer_Type_slots[] = {
    {Py_tp_doc, (char*)Writer_Type_doc},
    {Py_tp_traverse, Writer_traverse},
    {Py_tp_clear, Writer_clear},
    {Py_tp_dealloc, Writer_dealloc},
    {Py_tp_methods, Writer_methods},
    {Py_tp_members, Writer_memberlist},
    {0, NULL}
};

PyType_Spec Writer_Type_spec = {
    .name = "_csv.writer",
    .basicsize = sizeof(WriterObj),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION),
    .slots = Writer_Type_slots,
};


static PyObject *
csv_writer(PyObject *module, PyObject *args, PyObject *keyword_args)
{
    PyObject * output_file, * dialect = NULL;
    _csvstate *module_state = get_csv_state(module);
    WriterObj * self = PyObject_GC_New(WriterObj, module_state->writer_type);

    if (!self)
        return NULL;

    self->dialect = NULL;
    self->write = NULL;

    self->rec = NULL;
    self->rec_size = 0;
    self->rec_len = 0;
    self->num_fields = 0;

    self->error_obj = Py_NewRef(module_state->error_obj);

    if (!PyArg_UnpackTuple(args, "", 1, 2, &output_file, &dialect)) {
        Py_DECREF(self);
        return NULL;
    }
    if (PyObject_GetOptionalAttr(output_file,
                             module_state->str_write,
                             &self->write) < 0) {
        Py_DECREF(self);
        return NULL;
    }
    if (self->write == NULL || !PyCallable_Check(self->write)) {
        PyErr_SetString(PyExc_TypeError,
                        "argument 1 must have a \"write\" method");
        Py_DECREF(self);
        return NULL;
    }
    self->dialect = (DialectObj *)_call_dialect(module_state, dialect,
                                                keyword_args);
    if (self->dialect == NULL) {
        Py_DECREF(self);
        return NULL;
    }
    PyObject_GC_Track(self);
    return (PyObject *)self;
}

/*
 * DIALECT REGISTRY
 */

/*[clinic input]
_csv.list_dialects

Return a list of all known dialect names.

    names = csv.list_dialects()
[clinic start generated code]*/

static PyObject *
_csv_list_dialects_impl(PyObject *module)
/*[clinic end generated code: output=a5b92b215b006a6d input=8953943eb17d98ab]*/
{
    return PyDict_Keys(get_csv_state(module)->dialects);
}

static PyObject *
csv_register_dialect(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *name_obj, *dialect_obj = NULL;
    _csvstate *module_state = get_csv_state(module);
    PyObject *dialect;

    if (!PyArg_UnpackTuple(args, "", 1, 2, &name_obj, &dialect_obj))
        return NULL;
    if (!PyUnicode_Check(name_obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "dialect name must be a string");
        return NULL;
    }
    dialect = _call_dialect(module_state, dialect_obj, kwargs);
    if (dialect == NULL)
        return NULL;
    if (PyDict_SetItem(module_state->dialects, name_obj, dialect) < 0) {
        Py_DECREF(dialect);
        return NULL;
    }
    Py_DECREF(dialect);
    Py_RETURN_NONE;
}


/*[clinic input]
_csv.unregister_dialect

    name: object

Delete the name/dialect mapping associated with a string name.

    csv.unregister_dialect(name)
[clinic start generated code]*/

static PyObject *
_csv_unregister_dialect_impl(PyObject *module, PyObject *name)
/*[clinic end generated code: output=0813ebca6c058df4 input=6b5c1557bf60c7e7]*/
{
    _csvstate *module_state = get_csv_state(module);
    int rc = PyDict_Pop(module_state->dialects, name, NULL);
    if (rc < 0) {
        return NULL;
    }
    if (rc == 0) {
        PyErr_Format(module_state->error_obj, "unknown dialect");
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_csv.get_dialect

    name: object

Return the dialect instance associated with name.

    dialect = csv.get_dialect(name)
[clinic start generated code]*/

static PyObject *
_csv_get_dialect_impl(PyObject *module, PyObject *name)
/*[clinic end generated code: output=aa988cd573bebebb input=edf9ddab32e448fb]*/
{
    return get_dialect_from_registry(name, get_csv_state(module));
}

/*[clinic input]
_csv.field_size_limit

    new_limit: object = NULL

Sets an upper limit on parsed fields.

    csv.field_size_limit([limit])

Returns old limit. If limit is not given, no new limit is set and
the old limit is returned
[clinic start generated code]*/

static PyObject *
_csv_field_size_limit_impl(PyObject *module, PyObject *new_limit)
/*[clinic end generated code: output=f2799ecd908e250b input=cec70e9226406435]*/
{
    _csvstate *module_state = get_csv_state(module);
    Py_ssize_t old_limit = FT_ATOMIC_LOAD_SSIZE_RELAXED(module_state->field_limit);
    if (new_limit != NULL) {
        if (!PyLong_CheckExact(new_limit)) {
            PyErr_Format(PyExc_TypeError,
                         "limit must be an integer");
            return NULL;
        }
        Py_ssize_t new_limit_value = PyLong_AsSsize_t(new_limit);
        if (new_limit_value == -1 && PyErr_Occurred()) {
            return NULL;
        }
        FT_ATOMIC_STORE_SSIZE_RELAXED(module_state->field_limit, new_limit_value);
    }
    return PyLong_FromSsize_t(old_limit);
}

static PyType_Slot error_slots[] = {
    {0, NULL},
};

PyType_Spec error_spec = {
    .name = "_csv.Error",
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = error_slots,
};

/*
 * MODULE
 */

PyDoc_STRVAR(csv_module_doc, "CSV parsing and writing.\n");

PyDoc_STRVAR(csv_reader_doc,
"    csv_reader = reader(iterable [, dialect='excel']\n"
"                        [optional keyword args])\n"
"    for row in csv_reader:\n"
"        process(row)\n"
"\n"
"The \"iterable\" argument can be any object that returns a line\n"
"of input for each iteration, such as a file object or a list.  The\n"
"optional \"dialect\" parameter is discussed below.  The function\n"
"also accepts optional keyword arguments which override settings\n"
"provided by the dialect.\n"
"\n"
"The returned object is an iterator.  Each iteration returns a row\n"
"of the CSV file (which can span multiple input lines).\n");

PyDoc_STRVAR(csv_writer_doc,
"    csv_writer = csv.writer(fileobj [, dialect='excel']\n"
"                            [optional keyword args])\n"
"    for row in sequence:\n"
"        csv_writer.writerow(row)\n"
"\n"
"    [or]\n"
"\n"
"    csv_writer = csv.writer(fileobj [, dialect='excel']\n"
"                            [optional keyword args])\n"
"    csv_writer.writerows(rows)\n"
"\n"
"The \"fileobj\" argument can be any object that supports the file API.\n");

PyDoc_STRVAR(csv_register_dialect_doc,
"Create a mapping from a string name to a dialect class.\n"
"    dialect = csv.register_dialect(name[, dialect[, **fmtparams]])");

static struct PyMethodDef csv_methods[] = {
    { "reader", _PyCFunction_CAST(csv_reader),
        METH_VARARGS | METH_KEYWORDS, csv_reader_doc},
    { "writer", _PyCFunction_CAST(csv_writer),
        METH_VARARGS | METH_KEYWORDS, csv_writer_doc},
    { "register_dialect", _PyCFunction_CAST(csv_register_dialect),
        METH_VARARGS | METH_KEYWORDS, csv_register_dialect_doc},
    _CSV_LIST_DIALECTS_METHODDEF
    _CSV_UNREGISTER_DIALECT_METHODDEF
    _CSV_GET_DIALECT_METHODDEF
    _CSV_FIELD_SIZE_LIMIT_METHODDEF
    { NULL, NULL }
};

static int
csv_exec(PyObject *module) {
    const StyleDesc *style;
    PyObject *temp;
    _csvstate *module_state = get_csv_state(module);

    temp = PyType_FromModuleAndSpec(module, &Dialect_Type_spec, NULL);
    module_state->dialect_type = (PyTypeObject *)temp;
    if (PyModule_AddObjectRef(module, "Dialect", temp) < 0) {
        return -1;
    }

    temp = PyType_FromModuleAndSpec(module, &Reader_Type_spec, NULL);
    module_state->reader_type = (PyTypeObject *)temp;
    if (PyModule_AddObjectRef(module, "Reader", temp) < 0) {
        return -1;
    }

    temp = PyType_FromModuleAndSpec(module, &Writer_Type_spec, NULL);
    module_state->writer_type = (PyTypeObject *)temp;
    if (PyModule_AddObjectRef(module, "Writer", temp) < 0) {
        return -1;
    }

    /* Set the field limit */
    module_state->field_limit = 128 * 1024;

    /* Add _dialects dictionary */
    module_state->dialects = PyDict_New();
    if (PyModule_AddObjectRef(module, "_dialects", module_state->dialects) < 0) {
        return -1;
    }

    /* Add quote styles into dictionary */
    for (style = quote_styles; style->name; style++) {
        if (PyModule_AddIntConstant(module, style->name,
                                    style->style) == -1)
            return -1;
    }

    /* Add the CSV exception object to the module. */
    PyObject *bases = PyTuple_Pack(1, PyExc_Exception);
    if (bases == NULL) {
        return -1;
    }
    module_state->error_obj = PyType_FromModuleAndSpec(module, &error_spec,
                                                       bases);
    Py_DECREF(bases);
    if (module_state->error_obj == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, (PyTypeObject *)module_state->error_obj) != 0) {
        return -1;
    }

    module_state->str_write = PyUnicode_InternFromString("write");
    if (module_state->str_write == NULL) {
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot csv_slots[] = {
    {Py_mod_exec, csv_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _csvmodule = {
    PyModuleDef_HEAD_INIT,
    "_csv",
    csv_module_doc,
    sizeof(_csvstate),
    csv_methods,
    csv_slots,
    _csv_traverse,
    _csv_clear,
    _csv_free
};

PyMODINIT_FUNC
PyInit__csv(void)
{
    return PyModuleDef_Init(&_csvmodule);
}
