/* csv module */

/*

This module provides the low-level underpinnings of a CSV reading/writing
module.  Users should not use this module directly, but import the csv.py
module instead.

**** For people modifying this code, please note that as of this writing
**** (2003-03-23), it is intended that this code should work with Python
**** 2.2.

*/

#define MODULE_VERSION "1.0"

#include "Python.h"
#include "structmember.h"


/* begin 2.2 compatibility macros */
#ifndef PyDoc_STRVAR
/* Define macros for inline documentation. */
#define PyDoc_VAR(name) static char name[]
#define PyDoc_STRVAR(name,str) PyDoc_VAR(name) = PyDoc_STR(str)
#ifdef WITH_DOC_STRINGS
#define PyDoc_STR(str) str
#else
#define PyDoc_STR(str) ""
#endif
#endif /* ifndef PyDoc_STRVAR */

#ifndef PyMODINIT_FUNC
#	if defined(__cplusplus)
#		define PyMODINIT_FUNC extern "C" void
#	else /* __cplusplus */
#		define PyMODINIT_FUNC void
#	endif /* __cplusplus */
#endif
/* end 2.2 compatibility macros */

static PyObject *error_obj;	/* CSV exception */
static PyObject *dialects;      /* Dialect registry */

typedef enum {
	START_RECORD, START_FIELD, ESCAPED_CHAR, IN_FIELD, 
	IN_QUOTED_FIELD, ESCAPE_IN_QUOTED_FIELD, QUOTE_IN_QUOTED_FIELD
} ParserState;

typedef enum {
	QUOTE_MINIMAL, QUOTE_ALL, QUOTE_NONNUMERIC, QUOTE_NONE
} QuoteStyle;

typedef struct {
	QuoteStyle style;
	char *name;
} StyleDesc;

static StyleDesc quote_styles[] = {
	{ QUOTE_MINIMAL,    "QUOTE_MINIMAL" },
	{ QUOTE_ALL,        "QUOTE_ALL" },
	{ QUOTE_NONNUMERIC, "QUOTE_NONNUMERIC" },
	{ QUOTE_NONE,       "QUOTE_NONE" },
	{ 0 }
};

typedef struct {
        PyObject_HEAD
        
	int doublequote;	/* is " represented by ""? */
	char delimiter;		/* field separator */
	char quotechar;		/* quote character */
	char escapechar;	/* escape character */
	int skipinitialspace;	/* ignore spaces following delimiter? */
	PyObject *lineterminator; /* string to write between records */
	QuoteStyle quoting;	/* style of quoting to write */

	int strict;		/* raise exception on bad CSV */
} DialectObj;

staticforward PyTypeObject Dialect_Type;

typedef struct {
        PyObject_HEAD

        PyObject *input_iter;   /* iterate over this for input lines */

        DialectObj *dialect;    /* parsing dialect */

	PyObject *fields;	/* field list for current record */
	ParserState state;	/* current CSV parse state */
	char *field;		/* build current field in here */
	int field_size;		/* size of allocated buffer */
	int field_len;		/* length of current field */
	int had_parse_error;	/* did we have a parse error? */
} ReaderObj;

staticforward PyTypeObject Reader_Type;

#define ReaderObject_Check(v)   ((v)->ob_type == &Reader_Type)

typedef struct {
        PyObject_HEAD

        PyObject *writeline;    /* write output lines to this file */

        DialectObj *dialect;    /* parsing dialect */

	char *rec;		/* buffer for parser.join */
	int rec_size;		/* size of allocated record */
	int rec_len;		/* length of record */
	int num_fields;		/* number of fields in record */
} WriterObj;        

staticforward PyTypeObject Writer_Type;

/*
 * DIALECT class
 */

static PyObject *
get_dialect_from_registry(PyObject * name_obj)
{
        PyObject *dialect_obj;

        dialect_obj = PyDict_GetItem(dialects, name_obj);
        if (dialect_obj == NULL)
            return PyErr_Format(error_obj, "unknown dialect");
        Py_INCREF(dialect_obj);
        return dialect_obj;
}

static int
check_delattr(PyObject *v)
{
	if (v == NULL) {
		PyErr_SetString(PyExc_TypeError, 
                                "Cannot delete attribute");
		return -1;
        }
        return 0;
}

static PyObject *
get_string(PyObject *str)
{
        Py_XINCREF(str);
        return str;
}

static int
set_string(PyObject **str, PyObject *v)
{
        if (check_delattr(v) < 0)
                return -1;
        if (!PyString_Check(v)
#ifdef Py_USING_UNICODE
&& !PyUnicode_Check(v)
#endif
) {
                PyErr_BadArgument();
                return -1;
        }
        Py_XDECREF(*str);
        Py_INCREF(v);
        *str = v;
        return 0;
}

static PyObject *
get_nullchar_as_None(char c)
{
        if (c == '\0') {
                Py_INCREF(Py_None);
                return Py_None;
        }
        else
                return PyString_FromStringAndSize((char*)&c, 1);
}

static int
set_None_as_nullchar(char * addr, PyObject *v)
{
        if (check_delattr(v) < 0)
                return -1;
        if (v == Py_None)
                *addr = '\0';
        else if (!PyString_Check(v) || PyString_Size(v) != 1) {
                PyErr_BadArgument();
                return -1;
        }
        else {
		char *s = PyString_AsString(v);
		if (s == NULL)
			return -1;
		*addr = s[0];
	}
        return 0;
}

static PyObject *
Dialect_get_lineterminator(DialectObj *self)
{
        return get_string(self->lineterminator);
}

static int
Dialect_set_lineterminator(DialectObj *self, PyObject *value)
{
        return set_string(&self->lineterminator, value);
}

static PyObject *
Dialect_get_escapechar(DialectObj *self)
{
        return get_nullchar_as_None(self->escapechar);
}

static int
Dialect_set_escapechar(DialectObj *self, PyObject *value)
{
        return set_None_as_nullchar(&self->escapechar, value);
}

static PyObject *
Dialect_get_quoting(DialectObj *self)
{
        return PyInt_FromLong(self->quoting);
}

static int
Dialect_set_quoting(DialectObj *self, PyObject *v)
{
        int quoting;
        StyleDesc *qs = quote_styles;

        if (check_delattr(v) < 0)
                return -1;
        if (!PyInt_Check(v)) {
                PyErr_BadArgument();
                return -1;
        }
        quoting = PyInt_AsLong(v);
	for (qs = quote_styles; qs->name; qs++) {
		if (qs->style == quoting) {
                        self->quoting = quoting;
                        return 0;
                }
        }
        PyErr_BadArgument();
        return -1;
}

static struct PyMethodDef Dialect_methods[] = {
	{ NULL, NULL }
};

#define D_OFF(x) offsetof(DialectObj, x)

static struct PyMemberDef Dialect_memberlist[] = {
	{ "quotechar",          T_CHAR, D_OFF(quotechar) },
	{ "delimiter",          T_CHAR, D_OFF(delimiter) },
	{ "skipinitialspace",   T_INT, D_OFF(skipinitialspace) },
	{ "doublequote",        T_INT, D_OFF(doublequote) },
	{ "strict",             T_INT, D_OFF(strict) },
	{ NULL }
};

static PyGetSetDef Dialect_getsetlist[] = {
        { "escapechar", (getter)Dialect_get_escapechar, 
                (setter)Dialect_set_escapechar },
        { "lineterminator", (getter)Dialect_get_lineterminator, 
                (setter)Dialect_set_lineterminator },
        { "quoting", (getter)Dialect_get_quoting, 
                (setter)Dialect_set_quoting },
        {NULL},
};

static void
Dialect_dealloc(DialectObj *self)
{
        Py_XDECREF(self->lineterminator);
        self->ob_type->tp_free((PyObject *)self);
}

static int
dialect_init(DialectObj * self, PyObject * args, PyObject * kwargs)
{
        PyObject *dialect = NULL, *name_obj, *value_obj;

	self->quotechar = '"';
	self->delimiter = ',';
	self->escapechar = '\0';
	self->skipinitialspace = 0;
        Py_XDECREF(self->lineterminator);
	self->lineterminator = PyString_FromString("\r\n");
        if (self->lineterminator == NULL)
                return -1;
	self->quoting = QUOTE_MINIMAL;
	self->doublequote = 1;
	self->strict = 0;

	if (!PyArg_UnpackTuple(args, "", 0, 1, &dialect))
                return -1;
        Py_XINCREF(dialect);
        if (kwargs != NULL) {
                PyObject * key = PyString_FromString("dialect");
                PyObject * d;

                d = PyDict_GetItem(kwargs, key);
                if (d) {
                        Py_INCREF(d);
                        Py_XDECREF(dialect);
                        PyDict_DelItem(kwargs, key);
                        dialect = d;
                }
                Py_DECREF(key);
        }
        if (dialect != NULL) {
                int i;
                PyObject * dir_list;

                /* If dialect is a string, look it up in our registry */
                if (PyString_Check(dialect)
#ifdef Py_USING_UNICODE
		    || PyUnicode_Check(dialect)
#endif
			) {
                        PyObject * new_dia;
                        new_dia = get_dialect_from_registry(dialect);
                        Py_DECREF(dialect);
                        if (new_dia == NULL)
                                return -1;
                        dialect = new_dia;
                }
                /* A class rather than an instance? Instantiate */
                if (PyObject_TypeCheck(dialect, &PyClass_Type)) {
                        PyObject * new_dia;
                        new_dia = PyObject_CallFunction(dialect, "");
                        Py_DECREF(dialect);
                        if (new_dia == NULL)
                                return -1;
                        dialect = new_dia;
                }
                /* Make sure we finally have an instance */
                if (!PyInstance_Check(dialect) ||
                    (dir_list = PyObject_Dir(dialect)) == NULL) {
                        PyErr_SetString(PyExc_TypeError,
                                        "dialect must be an instance");
                        Py_DECREF(dialect);
                        return -1;
                }
                /* And extract the attributes */
                for (i = 0; i < PyList_GET_SIZE(dir_list); ++i) {
			char *s;
                        name_obj = PyList_GET_ITEM(dir_list, i);
			s = PyString_AsString(name_obj);
			if (s == NULL)
				return -1;
                        if (s[0] == '_')
                                continue;
                        value_obj = PyObject_GetAttr(dialect, name_obj);
                        if (value_obj) {
                                if (PyObject_SetAttr((PyObject *)self, 
                                                     name_obj, value_obj)) {
					Py_DECREF(value_obj);
                                        Py_DECREF(dir_list);
					Py_DECREF(dialect);
                                        return -1;
                                }
                                Py_DECREF(value_obj);
                        }
                }
                Py_DECREF(dir_list);
                Py_DECREF(dialect);
        }
        if (kwargs != NULL) {
                int pos = 0;

                while (PyDict_Next(kwargs, &pos, &name_obj, &value_obj)) {
                        if (PyObject_SetAttr((PyObject *)self, 
                                             name_obj, value_obj))
                                return -1;
                }
        }
        return 0;
}

static PyObject *
dialect_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
        DialectObj *self;
        self = (DialectObj *)type->tp_alloc(type, 0);
        if (self != NULL) {
                self->lineterminator = NULL;
        }
        return (PyObject *)self;
}


PyDoc_STRVAR(Dialect_Type_doc, 
"CSV dialect\n"
"\n"
"The Dialect type records CSV parsing and generation options.\n");

static PyTypeObject Dialect_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                                      /* ob_size */
	"_csv.Dialect",                         /* tp_name */
	sizeof(DialectObj),                     /* tp_basicsize */
	0,                                      /* tp_itemsize */
	/*  methods  */
	(destructor)Dialect_dealloc,            /* tp_dealloc */
	(printfunc)0,                           /* tp_print */
	(getattrfunc)0,                         /* tp_getattr */
	(setattrfunc)0,                         /* tp_setattr */
	(cmpfunc)0,                             /* tp_compare */
	(reprfunc)0,                            /* tp_repr */
	0,                                      /* tp_as_number */
	0,                                      /* tp_as_sequence */
	0,                                      /* tp_as_mapping */
	(hashfunc)0,                            /* tp_hash */
	(ternaryfunc)0,                         /* tp_call */
	(reprfunc)0,                    	/* tp_str */
	0,                                      /* tp_getattro */
        0,                                      /* tp_setattro */
        0,                                      /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	Dialect_Type_doc,                       /* tp_doc */
        0,                                      /* tp_traverse */
        0,                                      /* tp_clear */
        0,                                      /* tp_richcompare */
        0,                                      /* tp_weaklistoffset */
        0,                                      /* tp_iter */
        0,                                      /* tp_iternext */
        Dialect_methods,                        /* tp_methods */
        Dialect_memberlist,                     /* tp_members */
        Dialect_getsetlist,                     /* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	(initproc)dialect_init,			/* tp_init */
	PyType_GenericAlloc,	                /* tp_alloc */
	dialect_new,			        /* tp_new */
	0,                           		/* tp_free */
};

static void
parse_save_field(ReaderObj *self)
{
	PyObject *field;

	field = PyString_FromStringAndSize(self->field, self->field_len);
	if (field != NULL) {
		PyList_Append(self->fields, field);
		Py_XDECREF(field);
	}
	self->field_len = 0;
}

static int
parse_grow_buff(ReaderObj *self)
{
	if (self->field_size == 0) {
		self->field_size = 4096;
		if (self->field != NULL)
			PyMem_Free(self->field);
		self->field = PyMem_Malloc(self->field_size);
	}
	else {
		self->field_size *= 2;
		self->field = PyMem_Realloc(self->field, self->field_size);
	}
	if (self->field == NULL) {
		PyErr_NoMemory();
		return 0;
	}
	return 1;
}

static void
parse_add_char(ReaderObj *self, char c)
{
	if (self->field_len == self->field_size && !parse_grow_buff(self))
		return;
	self->field[self->field_len++] = c;
}

static void
parse_process_char(ReaderObj *self, char c)
{
        DialectObj *dialect = self->dialect;

	switch (self->state) {
	case START_RECORD:
		/* start of record */
		if (c == '\n')
			/* empty line - return [] */
			break;
		/* normal character - handle as START_FIELD */
		self->state = START_FIELD;
		/* fallthru */
	case START_FIELD:
		/* expecting field */
		if (c == '\n') {
			/* save empty field - return [fields] */
			parse_save_field(self);
			self->state = START_RECORD;
		}
		else if (c == dialect->quotechar) {
			/* start quoted field */
			self->state = IN_QUOTED_FIELD;
		}
		else if (c == dialect->escapechar) {
			/* possible escaped character */
			self->state = ESCAPED_CHAR;
		}
		else if (c == ' ' && dialect->skipinitialspace)
			/* ignore space at start of field */
			;
		else if (c == dialect->delimiter) {
			/* save empty field */
			parse_save_field(self);
		}
		else {
			/* begin new unquoted field */
			parse_add_char(self, c);
			self->state = IN_FIELD;
		}
		break;

	case ESCAPED_CHAR:
		if (c != dialect->escapechar && 
                    c != dialect->delimiter &&
		    c != dialect->quotechar)
			parse_add_char(self, dialect->escapechar);
		parse_add_char(self, c);
		self->state = IN_FIELD;
		break;

	case IN_FIELD:
		/* in unquoted field */
		if (c == '\n') {
			/* end of line - return [fields] */
			parse_save_field(self);
			self->state = START_RECORD;
		}
		else if (c == dialect->escapechar) {
			/* possible escaped character */
			self->state = ESCAPED_CHAR;
		}
		else if (c == dialect->delimiter) {
			/* save field - wait for new field */
			parse_save_field(self);
			self->state = START_FIELD;
		}
		else {
			/* normal character - save in field */
			parse_add_char(self, c);
		}
		break;

	case IN_QUOTED_FIELD:
		/* in quoted field */
		if (c == '\n') {
			/* end of line - save '\n' in field */
			parse_add_char(self, '\n');
		}
		else if (c == dialect->escapechar) {
			/* Possible escape character */
			self->state = ESCAPE_IN_QUOTED_FIELD;
		}
		else if (c == dialect->quotechar) {
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
			parse_add_char(self, c);
		}
		break;

	case ESCAPE_IN_QUOTED_FIELD:
		if (c != dialect->escapechar && 
                    c != dialect->delimiter &&
		    c != dialect->quotechar)
			parse_add_char(self, dialect->escapechar);
		parse_add_char(self, c);
		self->state = IN_QUOTED_FIELD;
		break;

	case QUOTE_IN_QUOTED_FIELD:
		/* doublequote - seen a quote in an quoted field */
		if (dialect->quoting != QUOTE_NONE && 
                    c == dialect->quotechar) {
			/* save "" as " */
			parse_add_char(self, c);
			self->state = IN_QUOTED_FIELD;
		}
		else if (c == dialect->delimiter) {
			/* save field - wait for new field */
			parse_save_field(self);
			self->state = START_FIELD;
		}
		else if (c == '\n') {
			/* end of line - return [fields] */
			parse_save_field(self);
			self->state = START_RECORD;
		}
		else if (!dialect->strict) {
			parse_add_char(self, c);
			self->state = IN_FIELD;
		}
		else {
			/* illegal */
			self->had_parse_error = 1;
			PyErr_Format(error_obj, "%c expected after %c", 
					dialect->delimiter, 
                                        dialect->quotechar);
		}
		break;

	}
}

/*
 * READER
 */
#define R_OFF(x) offsetof(ReaderObj, x)

static struct PyMemberDef Reader_memberlist[] = {
	{ "dialect", T_OBJECT, R_OFF(dialect), RO },
	{ NULL }
};

static PyObject *
Reader_getiter(ReaderObj *self)
{
	Py_INCREF(self);
	return (PyObject *)self;
}

static PyObject *
Reader_iternext(ReaderObj *self)
{
        PyObject *lineobj;
        PyObject *fields;
        char *line;

        do {
                lineobj = PyIter_Next(self->input_iter);
                if (lineobj == NULL) {
                        /* End of input OR exception */
                        if (!PyErr_Occurred() && self->field_len != 0)
                                return PyErr_Format(error_obj,
                                                    "newline inside string");
                        return NULL;
                }

                if (self->had_parse_error) {
                        if (self->fields) {
                                Py_XDECREF(self->fields);
                        }
                        self->fields = PyList_New(0);
                        self->field_len = 0;
                        self->state = START_RECORD;
                        self->had_parse_error = 0;
                }
                line = PyString_AsString(lineobj);

                if (line == NULL) {
                        Py_DECREF(lineobj);
                        return NULL;
                }
		if (strlen(line) < (size_t)PyString_GET_SIZE(lineobj)) {
			self->had_parse_error = 1;
			Py_DECREF(lineobj);
			return PyErr_Format(error_obj,
					    "string with NUL bytes");
		}

                /* Process line of text - send '\n' to processing code to
                represent end of line.  End of line which is not at end of
                string is an error. */
                while (*line) {
                        char c;

                        c = *line++;
                        if (c == '\r') {
                                c = *line++;
                                if (c == '\0')
                                        /* macintosh end of line */
                                        break;
                                if (c == '\n') {
                                        c = *line++;
                                        if (c == '\0')
                                                /* DOS end of line */
                                                break;
                                }
                                self->had_parse_error = 1;
                                Py_DECREF(lineobj);
                                return PyErr_Format(error_obj,
                                                    "newline inside string");
                        }
                        if (c == '\n') {
                                c = *line++;
                                if (c == '\0')
                                        /* unix end of line */
                                        break;
                                self->had_parse_error = 1;
                                Py_DECREF(lineobj);
                                return PyErr_Format(error_obj, 
                                                    "newline inside string");
                        }
                        parse_process_char(self, c);
                        if (PyErr_Occurred()) {
                                Py_DECREF(lineobj);
                                return NULL;
                        }
                }
                parse_process_char(self, '\n');
                Py_DECREF(lineobj);
        } while (self->state != START_RECORD);

        fields = self->fields;
        self->fields = PyList_New(0);
        return fields;
}

static void
Reader_dealloc(ReaderObj *self)
{
        Py_XDECREF(self->dialect);
        Py_XDECREF(self->input_iter);
        Py_XDECREF(self->fields);
        if (self->field != NULL)
        	PyMem_Free(self->field);
	PyObject_GC_Del(self);
}

static int
Reader_traverse(ReaderObj *self, visitproc visit, void *arg)
{
	int err;
#define VISIT(SLOT) \
	if (SLOT) { \
		err = visit((PyObject *)(SLOT), arg); \
		if (err) \
			return err; \
	}
	VISIT(self->dialect);
	VISIT(self->input_iter);
	VISIT(self->fields);
	return 0;
}

static int
Reader_clear(ReaderObj *self)
{
        Py_XDECREF(self->dialect);
        Py_XDECREF(self->input_iter);
        Py_XDECREF(self->fields);
        self->dialect = NULL;
        self->input_iter = NULL;
        self->fields = NULL;
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

static PyTypeObject Reader_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                                      /*ob_size*/
	"_csv.reader",                          /*tp_name*/
	sizeof(ReaderObj),                      /*tp_basicsize*/
	0,                                      /*tp_itemsize*/
	/* methods */
	(destructor)Reader_dealloc,             /*tp_dealloc*/
	(printfunc)0,                           /*tp_print*/
	(getattrfunc)0,                         /*tp_getattr*/
	(setattrfunc)0,                         /*tp_setattr*/
	(cmpfunc)0,                             /*tp_compare*/
	(reprfunc)0,                            /*tp_repr*/
	0,                                      /*tp_as_number*/
	0,                                      /*tp_as_sequence*/
	0,                                      /*tp_as_mapping*/
	(hashfunc)0,                            /*tp_hash*/
	(ternaryfunc)0,                         /*tp_call*/
	(reprfunc)0,                    	/*tp_str*/
	0,                                      /*tp_getattro*/
        0,                                      /*tp_setattro*/
        0,                                      /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
		Py_TPFLAGS_HAVE_GC,		/*tp_flags*/
	Reader_Type_doc,                        /*tp_doc*/
        (traverseproc)Reader_traverse,          /*tp_traverse*/
        (inquiry)Reader_clear,                  /*tp_clear*/
        0,                                      /*tp_richcompare*/
        0,                                      /*tp_weaklistoffset*/
        (getiterfunc)Reader_getiter,            /*tp_iter*/
        (getiterfunc)Reader_iternext,           /*tp_iternext*/
        Reader_methods,                         /*tp_methods*/
        Reader_memberlist,                      /*tp_members*/
        0,                                      /*tp_getset*/

};

static PyObject *
csv_reader(PyObject *module, PyObject *args, PyObject *keyword_args)
{
        PyObject * iterator, * dialect = NULL, *ctor_args;
        ReaderObj * self = PyObject_GC_New(ReaderObj, &Reader_Type);

        if (!self)
                return NULL;

        self->dialect = NULL;
        self->input_iter = self->fields = NULL;

        self->fields = NULL;
        self->input_iter = NULL;
	self->had_parse_error = 0;
	self->field = NULL;
	self->field_size = 0;
	self->field_len = 0;
	self->state = START_RECORD;

	if (!PyArg_UnpackTuple(args, "", 1, 2, &iterator, &dialect)) {
                Py_DECREF(self);
                return NULL;
        }
        self->input_iter = PyObject_GetIter(iterator);
        if (self->input_iter == NULL) {
                PyErr_SetString(PyExc_TypeError, 
                                "argument 1 must be an iterator");
                Py_DECREF(self);
                return NULL;
        }
        ctor_args = Py_BuildValue(dialect ? "(O)" : "()", dialect);
        if (ctor_args == NULL) {
                Py_DECREF(self);
                return NULL;
        }
        self->dialect = (DialectObj *)PyObject_Call((PyObject *)&Dialect_Type,
                                                    ctor_args, keyword_args);
        Py_DECREF(ctor_args);
        if (self->dialect == NULL) {
                Py_DECREF(self);
                return NULL;
        }
	self->fields = PyList_New(0);
	if (self->fields == NULL) {
		Py_DECREF(self);
		return NULL;
	}

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
static int
join_append_data(WriterObj *self, char *field, int quote_empty,
		 int *quoted, int copy_phase)
{
        DialectObj *dialect = self->dialect;
	int i, rec_len;

	rec_len = self->rec_len;

	/* If this is not the first field we need a field separator.
	 */
	if (self->num_fields > 0) {
		if (copy_phase)
			self->rec[rec_len] = dialect->delimiter;
		rec_len++;
	}
	/* Handle preceding quote.
	 */
	switch (dialect->quoting) {
	case QUOTE_ALL:
		*quoted = 1;
		if (copy_phase)
			self->rec[rec_len] = dialect->quotechar;
		rec_len++;
		break;
	case QUOTE_MINIMAL:
	case QUOTE_NONNUMERIC:
		/* We only know about quoted in the copy phase.
		 */
		if (copy_phase && *quoted) {
			self->rec[rec_len] = dialect->quotechar;
			rec_len++;
		}
		break;
	case QUOTE_NONE:
		break;
	}
	/* Copy/count field data.
	 */
	for (i = 0;; i++) {
		char c = field[i];

		if (c == '\0')
			break;
		/* If in doublequote mode we escape quote chars with a
		 * quote.
		 */
		if (dialect->quoting != QUOTE_NONE && 
                    c == dialect->quotechar && dialect->doublequote) {
			if (copy_phase)
				self->rec[rec_len] = dialect->quotechar;
			*quoted = 1;
			rec_len++;
		}

		/* Some special characters need to be escaped.  If we have a
		 * quote character switch to quoted field instead of escaping
		 * individual characters.
		 */
		if (!*quoted
		    && (c == dialect->delimiter || 
                        c == dialect->escapechar || 
                        c == '\n' || c == '\r')) {
			if (dialect->quoting != QUOTE_NONE)
				*quoted = 1;
			else if (dialect->escapechar) {
				if (copy_phase)
					self->rec[rec_len] = dialect->escapechar;
				rec_len++;
			}
			else {
				PyErr_Format(error_obj, 
                                             "delimiter must be quoted or escaped");
				return -1;
			}
		}
		/* Copy field character into record buffer.
		 */
		if (copy_phase)
			self->rec[rec_len] = c;
		rec_len++;
	}

	/* If field is empty check if it needs to be quoted.
	 */
	if (i == 0 && quote_empty) {
		if (dialect->quoting == QUOTE_NONE) {
			PyErr_Format(error_obj,
                                     "single empty field record must be quoted");
			return -1;
		} else
			*quoted = 1;
	}

	/* Handle final quote character on field.
	 */
	if (*quoted) {
		if (copy_phase)
			self->rec[rec_len] = dialect->quotechar;
		else
			/* Didn't know about leading quote until we found it
			 * necessary in field data - compensate for it now.
			 */
			rec_len++;
		rec_len++;
	}

	return rec_len;
}

static int
join_check_rec_size(WriterObj *self, int rec_len)
{
	if (rec_len > self->rec_size) {
		if (self->rec_size == 0) {
			self->rec_size = (rec_len / MEM_INCR + 1) * MEM_INCR;
			if (self->rec != NULL)
				PyMem_Free(self->rec);
			self->rec = PyMem_Malloc(self->rec_size);
		}
		else {
			char *old_rec = self->rec;

			self->rec_size = (rec_len / MEM_INCR + 1) * MEM_INCR;
			self->rec = PyMem_Realloc(self->rec, self->rec_size);
			if (self->rec == NULL)
				PyMem_Free(old_rec);
		}
		if (self->rec == NULL) {
			PyErr_NoMemory();
			return 0;
		}
	}
	return 1;
}

static int
join_append(WriterObj *self, char *field, int *quoted, int quote_empty)
{
	int rec_len;

	rec_len = join_append_data(self, field, quote_empty, quoted, 0);
	if (rec_len < 0)
		return 0;

	/* grow record buffer if necessary */
	if (!join_check_rec_size(self, rec_len))
		return 0;

	self->rec_len = join_append_data(self, field, quote_empty, quoted, 1);
	self->num_fields++;

	return 1;
}

static int
join_append_lineterminator(WriterObj *self)
{
	int terminator_len;

	terminator_len = PyString_Size(self->dialect->lineterminator);

	/* grow record buffer if necessary */
	if (!join_check_rec_size(self, self->rec_len + terminator_len))
		return 0;

	memmove(self->rec + self->rec_len,
		/* should not be NULL */
		PyString_AsString(self->dialect->lineterminator), 
                terminator_len);
	self->rec_len += terminator_len;

	return 1;
}

PyDoc_STRVAR(csv_writerow_doc,
"writerow(sequence)\n"
"\n"
"Construct and write a CSV record from a sequence of fields.  Non-string\n"
"elements will be converted to string.");

static PyObject *
csv_writerow(WriterObj *self, PyObject *seq)
{
        DialectObj *dialect = self->dialect;
	int len, i;

	if (!PySequence_Check(seq))
		return PyErr_Format(error_obj, "sequence expected");

	len = PySequence_Length(seq);
	if (len < 0)
		return NULL;

	/* Join all fields in internal buffer.
	 */
	join_reset(self);
	for (i = 0; i < len; i++) {
		PyObject *field;
		int append_ok;
		int quoted;

		field = PySequence_GetItem(seq, i);
		if (field == NULL)
			return NULL;

		quoted = 0;
		if (dialect->quoting == QUOTE_NONNUMERIC) {
			PyObject *num;

			num = PyNumber_Float(field);
			if (num == NULL) {
				quoted = 1;
				PyErr_Clear();
			}
			else {
				Py_DECREF(num);
			}
		}

		if (PyString_Check(field)) {
			append_ok = join_append(self,
						PyString_AS_STRING(field),
                                                &quoted, len == 1);
			Py_DECREF(field);
		}
		else if (field == Py_None) {
			append_ok = join_append(self, "", &quoted, len == 1);
			Py_DECREF(field);
		}
		else {
			PyObject *str;

			str = PyObject_Str(field);
			Py_DECREF(field);
			if (str == NULL)
				return NULL;

			append_ok = join_append(self, PyString_AS_STRING(str), 
                                                &quoted, len == 1);
			Py_DECREF(str);
		}
		if (!append_ok)
			return NULL;
	}

	/* Add line terminator.
	 */
	if (!join_append_lineterminator(self))
		return 0;

	return PyObject_CallFunction(self->writeline, 
                                     "(s#)", self->rec, self->rec_len);
}

PyDoc_STRVAR(csv_writerows_doc,
"writerows(sequence of sequences)\n"
"\n"
"Construct and write a series of sequences to a csv file.  Non-string\n"
"elements will be converted to string.");

static PyObject *
csv_writerows(WriterObj *self, PyObject *seqseq)
{
        PyObject *row_iter, *row_obj, *result;

        row_iter = PyObject_GetIter(seqseq);
        if (row_iter == NULL) {
                PyErr_SetString(PyExc_TypeError,
                                "writerows() argument must be iterable");
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
        Py_INCREF(Py_None);
        return Py_None;
}

static struct PyMethodDef Writer_methods[] = {
        { "writerow", (PyCFunction)csv_writerow, METH_O, csv_writerow_doc},
        { "writerows", (PyCFunction)csv_writerows, METH_O, csv_writerows_doc},
	{ NULL, NULL }
};

#define W_OFF(x) offsetof(WriterObj, x)

static struct PyMemberDef Writer_memberlist[] = {
	{ "dialect", T_OBJECT, W_OFF(dialect), RO },
	{ NULL }
};

static void
Writer_dealloc(WriterObj *self)
{
        Py_XDECREF(self->dialect);
        Py_XDECREF(self->writeline);
	if (self->rec != NULL)
		PyMem_Free(self->rec);
	PyObject_GC_Del(self);
}

static int
Writer_traverse(WriterObj *self, visitproc visit, void *arg)
{
	int err;
#define VISIT(SLOT) \
	if (SLOT) { \
		err = visit((PyObject *)(SLOT), arg); \
		if (err) \
			return err; \
	}
	VISIT(self->dialect);
	VISIT(self->writeline);
	return 0;
}

static int
Writer_clear(WriterObj *self)
{
        Py_XDECREF(self->dialect);
        Py_XDECREF(self->writeline);
	self->dialect = NULL;
	self->writeline = NULL;
	return 0;
}

PyDoc_STRVAR(Writer_Type_doc, 
"CSV writer\n"
"\n"
"Writer objects are responsible for generating tabular data\n"
"in CSV format from sequence input.\n"
);

static PyTypeObject Writer_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                                      /*ob_size*/
	"_csv.writer",                          /*tp_name*/
	sizeof(WriterObj),                      /*tp_basicsize*/
	0,                                      /*tp_itemsize*/
	/* methods */
	(destructor)Writer_dealloc,             /*tp_dealloc*/
	(printfunc)0,                           /*tp_print*/
	(getattrfunc)0,                         /*tp_getattr*/
	(setattrfunc)0,                         /*tp_setattr*/
	(cmpfunc)0,                             /*tp_compare*/
	(reprfunc)0,                            /*tp_repr*/
	0,                                      /*tp_as_number*/
	0,                                      /*tp_as_sequence*/
	0,                                      /*tp_as_mapping*/
	(hashfunc)0,                            /*tp_hash*/
	(ternaryfunc)0,                         /*tp_call*/
	(reprfunc)0,                            /*tp_str*/
	0,                                      /*tp_getattro*/
        0,                                      /*tp_setattro*/
        0,                                      /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
		Py_TPFLAGS_HAVE_GC,		/*tp_flags*/
	Writer_Type_doc,
        (traverseproc)Writer_traverse,          /*tp_traverse*/
        (inquiry)Writer_clear,                  /*tp_clear*/
        0,                                      /*tp_richcompare*/
        0,                                      /*tp_weaklistoffset*/
        (getiterfunc)0,                         /*tp_iter*/
        (getiterfunc)0,                         /*tp_iternext*/
        Writer_methods,                         /*tp_methods*/
        Writer_memberlist,                      /*tp_members*/
        0,                                      /*tp_getset*/
};

static PyObject *
csv_writer(PyObject *module, PyObject *args, PyObject *keyword_args)
{
        PyObject * output_file, * dialect = NULL, *ctor_args;
        WriterObj * self = PyObject_GC_New(WriterObj, &Writer_Type);

        if (!self)
                return NULL;

        self->dialect = NULL;
        self->writeline = NULL;

	self->rec = NULL;
	self->rec_size = 0;
	self->rec_len = 0;
	self->num_fields = 0;

	if (!PyArg_UnpackTuple(args, "", 1, 2, &output_file, &dialect)) {
                Py_DECREF(self);
                return NULL;
        }
        self->writeline = PyObject_GetAttrString(output_file, "write");
        if (self->writeline == NULL || !PyCallable_Check(self->writeline)) {
                PyErr_SetString(PyExc_TypeError,
                                "argument 1 must be an instance with a write method");
                Py_DECREF(self);
                return NULL;
        }
        ctor_args = Py_BuildValue(dialect ? "(O)" : "()", dialect);
        if (ctor_args == NULL) {
                Py_DECREF(self);
                return NULL;
        }
        self->dialect = (DialectObj *)PyObject_Call((PyObject *)&Dialect_Type,
                                                    ctor_args, keyword_args);
        Py_DECREF(ctor_args);
        if (self->dialect == NULL) {
                Py_DECREF(self);
                return NULL;
        }
        return (PyObject *)self;
}

/*
 * DIALECT REGISTRY
 */
static PyObject *
csv_list_dialects(PyObject *module, PyObject *args)
{
        return PyDict_Keys(dialects);
}

static PyObject *
csv_register_dialect(PyObject *module, PyObject *args)
{
        PyObject *name_obj, *dialect_obj;

	if (!PyArg_UnpackTuple(args, "", 2, 2, &name_obj, &dialect_obj))
                return NULL;
        if (!PyString_Check(name_obj)
#ifdef Py_USING_UNICODE
&& !PyUnicode_Check(name_obj)
#endif
) {
                PyErr_SetString(PyExc_TypeError, 
                                "dialect name must be a string or unicode");
                return NULL;
        }
        Py_INCREF(dialect_obj);
        /* A class rather than an instance? Instanciate */
        if (PyObject_TypeCheck(dialect_obj, &PyClass_Type)) {
                PyObject * new_dia;
                new_dia = PyObject_CallFunction(dialect_obj, "");
                Py_DECREF(dialect_obj);
                if (new_dia == NULL)
                        return NULL;
                dialect_obj = new_dia;
        }
        /* Make sure we finally have an instance */
        if (!PyInstance_Check(dialect_obj)) {
                PyErr_SetString(PyExc_TypeError, "dialect must be an instance");
                Py_DECREF(dialect_obj);
                return NULL;
        }
        if (PyObject_SetAttrString(dialect_obj, "_name", name_obj) < 0) {
                Py_DECREF(dialect_obj);
                return NULL;
        }
        if (PyDict_SetItem(dialects, name_obj, dialect_obj) < 0) {
                Py_DECREF(dialect_obj);
                return NULL;
        }
        Py_DECREF(dialect_obj);
        Py_INCREF(Py_None);
        return Py_None;
}

static PyObject *
csv_unregister_dialect(PyObject *module, PyObject *name_obj)
{
        if (PyDict_DelItem(dialects, name_obj) < 0)
                return PyErr_Format(error_obj, "unknown dialect");
        Py_INCREF(Py_None);
        return Py_None;
}

static PyObject *
csv_get_dialect(PyObject *module, PyObject *name_obj)
{
        return get_dialect_from_registry(name_obj);
}

/*
 * MODULE
 */

PyDoc_STRVAR(csv_module_doc,
"CSV parsing and writing.\n"
"\n"
"This module provides classes that assist in the reading and writing\n"
"of Comma Separated Value (CSV) files, and implements the interface\n"
"described by PEP 305.  Although many CSV files are simple to parse,\n"
"the format is not formally defined by a stable specification and\n"
"is subtle enough that parsing lines of a CSV file with something\n"
"like line.split(\",\") is bound to fail.  The module supports three\n"
"basic APIs: reading, writing, and registration of dialects.\n"
"\n"
"\n"
"DIALECT REGISTRATION:\n"
"\n"
"Readers and writers support a dialect argument, which is a convenient\n"
"handle on a group of settings.  When the dialect argument is a string,\n"
"it identifies one of the dialects previously registered with the module.\n"
"If it is a class or instance, the attributes of the argument are used as\n"
"the settings for the reader or writer:\n"
"\n"
"    class excel:\n"
"        delimiter = ','\n"
"        quotechar = '\"'\n"
"        escapechar = None\n"
"        doublequote = True\n"
"        skipinitialspace = False\n"
"        lineterminator = '\\r\\n'\n"
"        quoting = QUOTE_MINIMAL\n"
"\n"
"SETTINGS:\n"
"\n"
"    * quotechar - specifies a one-character string to use as the \n"
"        quoting character.  It defaults to '\"'.\n"
"    * delimiter - specifies a one-character string to use as the \n"
"        field separator.  It defaults to ','.\n"
"    * skipinitialspace - specifies how to interpret whitespace which\n"
"        immediately follows a delimiter.  It defaults to False, which\n"
"        means that whitespace immediately following a delimiter is part\n"
"        of the following field.\n"
"    * lineterminator -  specifies the character sequence which should \n"
"        terminate rows.\n"
"    * quoting - controls when quotes should be generated by the writer.\n"
"        It can take on any of the following module constants:\n"
"\n"
"        csv.QUOTE_MINIMAL means only when required, for example, when a\n"
"            field contains either the quotechar or the delimiter\n"
"        csv.QUOTE_ALL means that quotes are always placed around fields.\n"
"        csv.QUOTE_NONNUMERIC means that quotes are always placed around\n"
"            fields which do not parse as integers or floating point\n"
"            numbers.\n"
"        csv.QUOTE_NONE means that quotes are never placed around fields.\n"
"    * escapechar - specifies a one-character string used to escape \n"
"        the delimiter when quoting is set to QUOTE_NONE.\n"
"    * doublequote - controls the handling of quotes inside fields.  When\n"
"        True, two consecutive quotes are interpreted as one during read,\n"
"        and when writing, each quote character embedded in the data is\n"
"        written as two quotes\n");

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
"of the CSV file (which can span multiple input lines):\n");

PyDoc_STRVAR(csv_writer_doc,
"    csv_writer = csv.writer(fileobj [, dialect='excel']\n"
"                            [optional keyword args])\n"
"    for row in csv_writer:\n"
"        csv_writer.writerow(row)\n"
"\n"
"    [or]\n"
"\n"
"    csv_writer = csv.writer(fileobj [, dialect='excel']\n"
"                            [optional keyword args])\n"
"    csv_writer.writerows(rows)\n"
"\n"
"The \"fileobj\" argument can be any object that supports the file API.\n");

PyDoc_STRVAR(csv_list_dialects_doc,
"Return a list of all know dialect names.\n"
"    names = csv.list_dialects()");

PyDoc_STRVAR(csv_get_dialect_doc,
"Return the dialect instance associated with name.\n"
"    dialect = csv.get_dialect(name)");

PyDoc_STRVAR(csv_register_dialect_doc,
"Create a mapping from a string name to a dialect class.\n"
"    dialect = csv.register_dialect(name, dialect)");

PyDoc_STRVAR(csv_unregister_dialect_doc,
"Delete the name/dialect mapping associated with a string name.\n"
"    csv.unregister_dialect(name)");

static struct PyMethodDef csv_methods[] = {
        { "reader", (PyCFunction)csv_reader, 
            METH_VARARGS | METH_KEYWORDS, csv_reader_doc},
        { "writer", (PyCFunction)csv_writer, 
            METH_VARARGS | METH_KEYWORDS, csv_writer_doc},
        { "list_dialects", (PyCFunction)csv_list_dialects, 
            METH_NOARGS, csv_list_dialects_doc},
        { "register_dialect", (PyCFunction)csv_register_dialect, 
            METH_VARARGS, csv_register_dialect_doc},
        { "unregister_dialect", (PyCFunction)csv_unregister_dialect, 
            METH_O, csv_unregister_dialect_doc},
        { "get_dialect", (PyCFunction)csv_get_dialect, 
            METH_O, csv_get_dialect_doc},
        { NULL, NULL }
};

PyMODINIT_FUNC
init_csv(void)
{
	PyObject *module;
	StyleDesc *style;

	if (PyType_Ready(&Dialect_Type) < 0)
		return;

	if (PyType_Ready(&Reader_Type) < 0)
		return;

	if (PyType_Ready(&Writer_Type) < 0)
		return;

	/* Create the module and add the functions */
	module = Py_InitModule3("_csv", csv_methods, csv_module_doc);
	if (module == NULL)
		return;

	/* Add version to the module. */
	if (PyModule_AddStringConstant(module, "__version__",
				       MODULE_VERSION) == -1)
		return;

        /* Add _dialects dictionary */
        dialects = PyDict_New();
        if (dialects == NULL)
                return;
        if (PyModule_AddObject(module, "_dialects", dialects))
                return;

	/* Add quote styles into dictionary */
	for (style = quote_styles; style->name; style++) {
		if (PyModule_AddIntConstant(module, style->name,
					    style->style) == -1)
			return;
	}

        /* Add the Dialect type */
        if (PyModule_AddObject(module, "Dialect", (PyObject *)&Dialect_Type))
                return;

	/* Add the CSV exception object to the module. */
	error_obj = PyErr_NewException("_csv.Error", NULL, NULL);
	if (error_obj == NULL)
		return;
	PyModule_AddObject(module, "Error", error_obj);
}
