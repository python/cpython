/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

#include "Python.h"
#include "macglue.h"
#include "macdefs.h"

extern PyObject *ResObj_New Py_PROTO((Handle));
extern int ResObj_Convert Py_PROTO((PyObject *, Handle *));

#include <CodeFragments.h>

static PyObject *ErrorObject;

#define PARANOID(arg) \
	if ( arg == 0 ) {PyErr_SetString(ErrorObject, "Internal error: NULL arg!"); return 0; }
	
/* Prototype we use for routines */

typedef long anything;
typedef anything (*anyroutine) Py_PROTO((...));

#define MAXNAME 31	/* Maximum size of names, for printing only */
#define MAXARG 8	/* Maximum number of arguments */

/*
** Routines to convert arguments between Python and C
*/
typedef anything (*py2c_converter) Py_PROTO((PyObject *));
typedef PyObject *(*c2py_converter) Py_PROTO((anything));

/* Dummy routine for arguments that are output-only */
static anything
py2c_dummy(arg)
	PyObject  *arg;
{
	return 0;
}

/* Routine to allocate storage for output integers */
static anything
py2c_alloc(arg)
	PyObject *arg;
{
	char *ptr;
	
	if( (ptr=malloc(sizeof(anything))) == 0 )
		PyErr_NoMemory();
	return (anything)ptr;
}

/* Dummy routine for arguments that are input-only */
static PyObject *
c2py_dummy(arg)
	anything arg;
{
	return 0;
}

/* Routine to de-allocate storage for input-only arguments */
static PyObject *
c2py_free(arg)
	anything arg;
{
	if ( arg )
		free((char *)arg);
	return 0;
}

/*
** None
*/
static PyObject *
c2py_none(arg)
	anything arg;
{
	if ( arg )
		free((char *)arg);
	Py_INCREF(Py_None);
	return Py_None;
}

/*
** OSErr
*/
static PyObject *
c2py_oserr(arg)
	anything arg;
{
	OSErr *ptr = (OSErr *)arg;
	
	PARANOID(arg);
	if (*ptr) {
		PyErr_Mac(PyMac_OSErrException, *ptr);
		free(ptr);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

/*
** integers of all sizes (PPC only)
*/
static anything
py2c_in_int(arg)
	PyObject  *arg;
{
	return PyInt_AsLong(arg);
}

static PyObject *
c2py_out_long(arg)
	anything arg;
{
	PyObject *rv;
	
	PARANOID(arg);
	rv = PyInt_FromLong(*(long *)arg);
	free((char *)arg);
	return rv;
}

static PyObject *
c2py_out_short(arg)
	anything arg;
{
	PyObject *rv;
	
	PARANOID(arg);
	rv =  PyInt_FromLong((long)*(short *)arg);
	free((char *)arg);
	return rv;
}

static PyObject *
c2py_out_byte(arg)
	anything arg;
{
	PyObject *rv;
	
	PARANOID(arg);
	rv =  PyInt_FromLong((long)*(char *)arg);
	free((char *)arg);
	return rv;
}

/*
** Strings
*/
static anything
py2c_in_string(arg)
	PyObject *arg;
{
	return (anything)PyString_AsString(arg);
}

/*
** Pascal-style strings
*/
static anything
py2c_in_pstring(arg)
	PyObject *arg;
{
	unsigned char *p;
	int size;
	
	if( (size = PyString_Size(arg)) < 0)
		return 0;
	if ( size > 255 ) {
		PyErr_SetString(ErrorObject, "Pstring must be <= 255 chars");
		return 0;
	}
	if( (p=(unsigned char *)malloc(256)) == 0 ) {
		PyErr_NoMemory();
		return 0;
	}
	p[0] = size;
	memcpy(p+1, PyString_AsString(arg), size);
	return (anything)p;
}

static anything
py2c_out_pstring(arg)
	PyObject *arg;
{
	unsigned char *p;
	
	if( (p=(unsigned char *)malloc(256)) == 0 ) {
		PyErr_NoMemory();
		return 0;
	}
	p[0] = 0;
	return (anything)p;
}

static PyObject *
c2py_out_pstring(arg)
	anything arg;
{
	unsigned char *p = (unsigned char *)arg;
	PyObject *rv;
	
	PARANOID(arg);
	rv = PyString_FromStringAndSize((char *)p+1, p[0]);
	free(p);
	return rv;
}

/*
** C objects.
*/
static anything
py2c_in_cobject(arg)
	PyObject *arg;
{
	if ( arg == Py_None )
		return 0;
	return (anything)PyCObject_AsVoidPtr(arg);
}

static PyObject *
c2py_out_cobject(arg)
	anything arg;
{
	void **ptr = (void **)arg;
	PyObject *rv;
	
	PARANOID(arg);
	if ( *ptr == 0 ) {
		Py_INCREF(Py_None);
		rv = Py_None;
	} else {
		rv = PyCObject_FromVoidPtr(*ptr, 0);
	}
	free((char *)ptr);
	return rv;
}

/*
** Handles.
*/
static anything
py2c_in_handle(arg)
	PyObject *arg;
{
	Handle h = 0;
	ResObj_Convert(arg, &h);
	return (anything)h;
}

static PyObject *
c2py_out_handle(arg)
	anything arg;
{
	Handle *rv = (Handle *)arg;
	PyObject *prv;
	
	PARANOID(arg);
	if ( *rv == 0 ) {
		Py_INCREF(Py_None);
		prv = Py_None;
	} else {
		prv = ResObj_New(*rv);
	}
	free((char *)rv);
	return prv;
}

typedef struct {
	char *name;		/* Name */
	py2c_converter	get;	/* Get argument */
	int	get_uses_arg;	/* True if the above consumes an argument */
	c2py_converter	put;	/* Put result value */
	int	put_gives_result;	/* True if above produces a result */
} conventry;

static conventry converters[] = {
	{"OutNone",	py2c_alloc,	0,	c2py_none,	1},
	{"OutOSErr",	py2c_alloc,	0,	c2py_oserr,	1},
#define OSERRORCONVERTER (&converters[1])
	{"InInt",	py2c_in_int,	1,	c2py_dummy,	0},
	{"OutLong",	py2c_alloc,	0,	c2py_out_long,	1},
	{"OutShort",	py2c_alloc,	0,	c2py_out_short,	1},
	{"OutByte",	py2c_alloc,	0,	c2py_out_byte,	1},
	{"InString",	py2c_in_string,	1,	c2py_dummy,	0},
	{"InPstring",	py2c_in_pstring,1,	c2py_free,	0},
	{"OutPstring",	py2c_out_pstring,0,	c2py_out_pstring,1},
	{"InCobject",	py2c_in_cobject,1,	c2py_dummy,	0},
	{"OutCobject",	py2c_alloc,	0,	c2py_out_cobject,0},
	{"InHandle",	py2c_in_handle,	1,	c2py_dummy,	0},
	{"OutHandle",	py2c_alloc,	0,	c2py_out_handle,1},
	{0, 0, 0, 0, 0}
};

static conventry *
getconverter(name)
	char *name;
{
	int i;
	char buf[256];
	
	for(i=0; converters[i].name; i++ )
		if ( strcmp(name, converters[i].name) == 0 )
			return &converters[i];
	sprintf(buf, "Unknown argtype: %s", name);
	PyErr_SetString(ErrorObject, buf);
	return 0;
}	

static int
argparse_conv(obj, ptr)
	PyObject *obj;
	conventry **ptr;
{
	char *name;
	int i;
	conventry *item;
	
	if( (name=PyString_AsString(obj)) == NULL )
		return 0;
	if( (item=getconverter(name)) == NULL )
		return 0;
	*ptr = item;
	return 1;
}

/* ----------------------------------------------------- */

/* Declarations for objects of type fragment */

typedef struct {
	PyObject_HEAD
	CFragConnectionID conn_id;
	char name[MAXNAME+1];
} cdfobject;

staticforward PyTypeObject Cdftype;



/* ---------------------------------------------------------------- */

/* Declarations for objects of type routine */

typedef struct {
	PyObject_HEAD
	anyroutine rtn;
	char name[MAXNAME+1];
} cdrobject;

staticforward PyTypeObject Cdrtype;



/* ---------------------------------------------------------------- */

/* Declarations for objects of type callable */

typedef struct {
	PyObject_HEAD
	cdrobject *routine;	/* The routine to call */
	int npargs;		/* Python argument count */
	int npreturn;		/* Python return value count */
	int ncargs;		/* C argument count + 1 */
	conventry *argconv[MAXARG+1];	/* Value converter list */
} cdcobject;

staticforward PyTypeObject Cdctype;



/* -------------------------------------------------------- */


static struct PyMethodDef cdr_methods[] = {
	
	{NULL,		NULL}		/* sentinel */
};

/* ---------- */


static cdrobject *
newcdrobject(name, routine)
	unsigned char *name;
	anyroutine routine;
{
	cdrobject *self;
	int nlen;
	
	self = PyObject_NEW(cdrobject, &Cdrtype);
	if (self == NULL)
		return NULL;
	if ( name[0] > MAXNAME )
		nlen = MAXNAME;
	else
		nlen = name[0];
	memcpy(self->name, name+1, nlen);
	self->name[nlen] = '\0';
	self->rtn = routine;
	return self;
}

static void
cdr_dealloc(self)
	cdrobject *self;
{
	PyMem_DEL(self);
}

static PyObject *
cdr_repr(self)
	cdrobject *self;
{
	PyObject *s;
	char buf[256];

	sprintf(buf, "<Calldll routine %s address 0x%x>", self->name, self->rtn);
	s = PyString_FromString(buf);
	return s;
}

static char Cdrtype__doc__[] = 
"C Routine address"
;

static PyTypeObject Cdrtype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"routine",			/*tp_name*/
	sizeof(cdrobject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)cdr_dealloc,	/*tp_dealloc*/
	(printfunc)0,			/*tp_print*/
	(getattrfunc)0,			/*tp_getattr*/
	(setattrfunc)0,			/*tp_setattr*/
	(cmpfunc)0,			/*tp_compare*/
	(reprfunc)cdr_repr,		/*tp_repr*/
	0,				/*tp_as_number*/
	0,				/*tp_as_sequence*/
	0,				/*tp_as_mapping*/
	(hashfunc)0,			/*tp_hash*/
	(ternaryfunc)0,			/*tp_call*/
	(reprfunc)0,			/*tp_str*/

	/* Space for future expansion */
	0L,0L,0L,0L,
	Cdrtype__doc__ /* Documentation string */
};

/* End of code for routine objects */
/* -------------------------------------------------------- */


static struct PyMethodDef cdc_methods[] = {
	
	{NULL,		NULL}		/* sentinel */
};

/* ---------- */


static cdcobject *
newcdcobject(routine, npargs, npreturn, ncargs, argconv)
	cdrobject *routine;
	int npargs;
	int npreturn;
	int ncargs;
	conventry *argconv[];
{
	cdcobject *self;
	int i;
	
	self = PyObject_NEW(cdcobject, &Cdctype);
	if (self == NULL)
		return NULL;
	self->routine = routine;
	Py_INCREF(routine);
	self->npargs = npargs;
	self->npreturn = npreturn;
	self->ncargs = ncargs;
	for(i=0; i<MAXARG+1; i++)
		if ( i < ncargs )
			self->argconv[i] = argconv[i];
		else
			self->argconv[i] = 0;
	return self;
}

static void
cdc_dealloc(self)
	cdcobject *self;
{
	Py_XDECREF(self->routine);
	PyMem_DEL(self);
}


static PyObject *
cdc_repr(self)
	cdcobject *self;
{
	PyObject *s;
	char buf[256];
	int i;
	
	sprintf(buf, "<callable %s = %s(", self->argconv[0]->name, self->routine->name);
	for(i=1; i< self->ncargs; i++) {
		strcat(buf, self->argconv[i]->name);
		if ( i < self->ncargs-1 )
			strcat(buf, ", ");
	}
	strcat(buf, ") >");

	s = PyString_FromString(buf);
	return s;
}

/*
** And this is what we all do it for: call a C function.
*/
static PyObject *
cdc_call(self, args, kwargs)
	cdcobject *self;
	PyObject *args;
	PyObject *kwargs;
{
	char buf[256];
	int i, pargindex;
	anything c_args[MAXARG+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	conventry *cp;
	PyObject *curarg;
	anyroutine func;
	PyObject *rv0, *rv;
	
	if( kwargs ) {
		PyErr_SetString(PyExc_TypeError, "Keyword args not allowed");
		return 0;
	}
	if( !PyTuple_Check(args) ) {
		PyErr_SetString(PyExc_TypeError, "Arguments not in tuple");
		return 0;
	}
	if( PyTuple_Size(args) != self->npargs ) {
		sprintf(buf, "%d arguments, expected %d", PyTuple_Size(args), self->npargs);
		PyErr_SetString(PyExc_TypeError, buf);
		return 0;
	}
	
	/* Decode arguments */
	pargindex = 0;
	for(i=0; i<self->ncargs; i++) {
		cp = self->argconv[i];
		if ( cp->get_uses_arg ) {
			curarg = PyTuple_GET_ITEM(args, pargindex);
			pargindex++;
		} else {
			curarg = (PyObject *)NULL;
		}
		c_args[i] = (*cp->get)(curarg);
	}
	if (PyErr_Occurred())
		return 0;
		
	/* Call function */
	func = self->routine->rtn;
	*(anything *)c_args[0] = (*func)(c_args[1], c_args[2], c_args[3], c_args[4],
			c_args[5], c_args[6], c_args[7], c_args[8]);

	/* Build return tuple (always a tuple, for now */
	if( (rv=PyTuple_New(self->npreturn)) == NULL )
		return NULL;
	pargindex = 0;
	for(i=0; i<self->ncargs; i++) {
		cp = self->argconv[i];
		curarg = (*cp->put)(c_args[i]);
		if( cp->put_gives_result )
			PyTuple_SET_ITEM(rv, pargindex, curarg);
		/* NOTE: We only check errors at the end (so we free() everything) */
	}
	if ( PyErr_Occurred() ) {
		Py_DECREF(rv);
		return NULL;
	}
	return rv;
}

static char Cdctype__doc__[] = 
""
;

static PyTypeObject Cdctype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"callable",			/*tp_name*/
	sizeof(cdcobject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)cdc_dealloc,	/*tp_dealloc*/
	(printfunc)0,			/*tp_print*/
	(getattrfunc)0,			/*tp_getattr*/
	(setattrfunc)0,			/*tp_setattr*/
	(cmpfunc)0,			/*tp_compare*/
	(reprfunc)cdc_repr,		/*tp_repr*/
	0,				/*tp_as_number*/
	0,				/*tp_as_sequence*/
	0,				/*tp_as_mapping*/
	(hashfunc)0,			/*tp_hash*/
	(ternaryfunc)cdc_call,		/*tp_call*/
	(reprfunc)0,			/*tp_str*/

	/* Space for future expansion */
	0L,0L,0L,0L,
	Cdctype__doc__ /* Documentation string */
};

/* End of code for callable objects */
/* ---------------------------------------------------------------- */

static struct PyMethodDef cdf_methods[] = {
	
	{NULL,		NULL}		/* sentinel */
};

/* ---------- */


static cdfobject *
newcdfobject(conn_id, name)
	CFragConnectionID conn_id;
	unsigned char *name;
{
	cdfobject *self;
	int nlen;
	
	self = PyObject_NEW(cdfobject, &Cdftype);
	if (self == NULL)
		return NULL;
	self->conn_id = conn_id;
	if ( name[0] > MAXNAME )
		nlen = MAXNAME;
	else
		nlen = name[0];
	strncpy(self->name, (char *)name+1, nlen);
	self->name[nlen] = '\0';
	return self;
}

static void
cdf_dealloc(self)
	cdfobject *self;
{
	PyMem_DEL(self);
}

static PyObject *
cdf_repr(self)
	cdfobject *self;
{
	PyObject *s;
	char buf[256];

	sprintf(buf, "<fragment %s connection, id 0x%x>", self->name, self->conn_id);
	s = PyString_FromString(buf);
	return s;
}

static PyObject *
cdf_getattr(self, name)
	cdfobject *self;
	char *name;
{
	unsigned char *rtn_name;
	anyroutine rtn;
	OSErr err;
	Str255 errMessage;
	CFragSymbolClass class;
	char buf[256];
	
	rtn_name = Pstring(name);
	err = FindSymbol(self->conn_id, rtn_name, (Ptr *)&rtn, &class);
	if ( err ) {
		sprintf(buf, "%.*s: %s", rtn_name[0], rtn_name+1, PyMac_StrError(err));
		PyErr_SetString(ErrorObject, buf);
		return NULL;
	}
	if( class != kTVectorCFragSymbol ) {
		PyErr_SetString(ErrorObject, "Symbol is not a routine");
		return NULL;
	}
	
	return (PyObject *)newcdrobject(rtn_name, rtn);
}
/* -------------------------------------------------------- */

static char Cdftype__doc__[] = 
"Code Fragment library symbol table"
;

static PyTypeObject Cdftype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"fragment",			/*tp_name*/
	sizeof(cdfobject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)cdf_dealloc,	/*tp_dealloc*/
	(printfunc)0,			/*tp_print*/
	(getattrfunc)cdf_getattr,	/*tp_getattr*/
	(setattrfunc)0,			/*tp_setattr*/
	(cmpfunc)0,			/*tp_compare*/
	(reprfunc)cdf_repr,		/*tp_repr*/
	0,				/*tp_as_number*/
	0,				/*tp_as_sequence*/
	0,				/*tp_as_mapping*/
	(hashfunc)0,			/*tp_hash*/
	(ternaryfunc)0,			/*tp_call*/
	(reprfunc)0,			/*tp_str*/

	/* Space for future expansion */
	0L,0L,0L,0L,
	Cdftype__doc__ /* Documentation string */
};

/* End of code for fragment objects */
/* -------------------------------------------------------- */


static char cdll_getlibrary__doc__[] =
"Load a shared library fragment and return the symbol table"
;

static PyObject *
cdll_getlibrary(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	Str255 frag_name;
	OSErr err;
	Str255 errMessage;
	Ptr main_addr;
	CFragConnectionID conn_id;
	char buf[256];
	
	if (!PyArg_ParseTuple(args, "O&", PyMac_GetStr255, frag_name))
		return NULL;

	/* Find the library connection ID */
	err = GetSharedLibrary(frag_name, kCurrentCFragArch, kLoadCFrag, &conn_id, &main_addr, 
			errMessage);
	if ( err ) {
		sprintf(buf, "%.*s: %s", errMessage[0], errMessage+1, PyMac_StrError(err));
		PyErr_SetString(ErrorObject, buf);
		return NULL;
	}
	return (PyObject *)newcdfobject(conn_id, frag_name);
}

static char cdll_getdiskfragment__doc__[] =
"Load a fragment from a disk file and return the symbol table"
;

static PyObject *
cdll_getdiskfragment(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	FSSpec fsspec;
	Str255 frag_name;
	OSErr err;
	Str255 errMessage;
	Ptr main_addr;
	CFragConnectionID conn_id;
	char buf[256];
	Boolean isfolder, didsomething;
	
	if (!PyArg_ParseTuple(args, "O&O&", PyMac_GetFSSpec, &fsspec,
			PyMac_GetStr255, frag_name))
		return NULL;
	err = ResolveAliasFile(&fsspec, 1, &isfolder, &didsomething);
	if ( err )
		return PyErr_Mac(ErrorObject, err);

	/* Load the fragment (or return the connID if it is already loaded */
	err = GetDiskFragment(&fsspec, 0, 0, frag_name, 
			      kLoadCFrag, &conn_id, &main_addr,
			      errMessage);
	if ( err ) {
		sprintf(buf, "%.*s: %s", errMessage[0], errMessage+1, PyMac_StrError(err));
		PyErr_SetString(ErrorObject, buf);
		return NULL;
	}
	return (PyObject *)newcdfobject(conn_id, frag_name);
}

static char cdll_newcall__doc__[] =
""
;

static PyObject *
cdll_newcall(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	cdrobject *routine;
	conventry *argconv[MAXARG+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	int npargs, npreturn, ncargs;

	/* Note: the next format depends on MAXARG+1 */
	if (!PyArg_ParseTuple(args, "O!O&|O&O&O&O&O&O&O&O&", &Cdrtype, &routine,
		argparse_conv, &argconv[0], argparse_conv, &argconv[1],
		argparse_conv, &argconv[2], argparse_conv, &argconv[3],
		argparse_conv, &argconv[4], argparse_conv, &argconv[5],
		argparse_conv, &argconv[6], argparse_conv, &argconv[7],
		argparse_conv, &argconv[8]))
		return NULL;
	npargs = npreturn = 0;
	for(ncargs=0; ncargs < MAXARG+1 && argconv[ncargs]; ncargs++) {
		if( argconv[ncargs]->get_uses_arg ) npargs++;
		if( argconv[ncargs]->put_gives_result ) npreturn++;
	}
	return (PyObject *)newcdcobject(routine, npargs, npreturn, ncargs, argconv);
}

/* List of methods defined in the module */

static struct PyMethodDef cdll_methods[] = {
	{"getlibrary",		(PyCFunction)cdll_getlibrary,		METH_VARARGS,	
							cdll_getlibrary__doc__},
	{"getdiskfragment",	(PyCFunction)cdll_getdiskfragment,	METH_VARARGS,
							cdll_getdiskfragment__doc__},
	{"newcall",		(PyCFunction)cdll_newcall,		METH_VARARGS,
							cdll_newcall__doc__},
 
	{NULL,	 (PyCFunction)NULL, 0, NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initcalldll) */

static char calldll_module_documentation[] = 
""
;

void
initcalldll()
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule4("calldll", cdll_methods,
		calldll_module_documentation,
		(PyObject*)NULL,PYTHON_API_VERSION);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ErrorObject = PyString_FromString("calldll.error");
	PyDict_SetItemString(d, "error", ErrorObject);

	/* XXXX Add constants here */
	
	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module calldll");
}

/* Test routine */
int calldlltester(int a1,int  a2,int  a3,int  a4,int  a5,int  a6,int  a7,int  a8)
{
	printf("Tester1: %x %x %x %x %x %x %x %x\n", a1, a2, a3, a4, a5, a6, a7, a8);
	return a1;
}

