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

/* Sanity check */
#ifndef __powerc
#error Please port this code to your architecture first...
#endif

/*
** Define to include testroutines (at the end)
*/
#define TESTSUPPORT

#include "Python.h"
#include "macglue.h"
#include "macdefs.h"
#include <CodeFragments.h>

/* Prototypes for routines not in any include file (shame, shame) */
extern PyObject *ResObj_New Py_PROTO((Handle));
extern int ResObj_Convert Py_PROTO((PyObject *, Handle *));

static PyObject *ErrorObject;

/* Debugging macro */
#ifdef TESTSUPPORT
#define PARANOID(arg) \
	if ( arg == 0 ) {PyErr_SetString(ErrorObject, "Internal error: NULL arg!"); return 0; }
#else
#define PARANOID(arg) /*pass*/
#endif

/* Prototypes we use for routines and arguments */

typedef long anything;
typedef anything (*anyroutine) Py_PROTO((...));

/* Other constants */
#define MAXNAME 31	/* Maximum size of names, for printing only */
#define MAXARG 8	/* Maximum number of arguments */

/*
** Routines to convert arguments between Python and C.
** Note return-value converters return NULL if this argument (or return value)
** doesn't return anything. The call-wrapper code collects all return values,
** and does the expected thing based on the number of return values: return None, a single
** value or a tuple of values.
**
** Hence, optional return values are also implementable.
*/
typedef anything (*py2c_converter) Py_PROTO((PyObject *));
typedef PyObject *(*c2py_converter) Py_PROTO((anything));
typedef PyObject *(*rv2py_converter) Py_PROTO((anything));


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

/* Dummy routine for void return value */
static PyObject *
rv2py_none(arg)
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
** OSErr return value.
*/
static PyObject *
rv2py_oserr(arg)
	anything arg;
{
	OSErr err = (OSErr)arg;
	
	if (err)
		return PyMac_Error(err);
	return 0;
}

/*
** Input integers of all sizes (PPC only)
*/
static anything
py2c_in_int(arg)
	PyObject  *arg;
{
	return PyInt_AsLong(arg);
}

/*
** Integer return values of all sizes (PPC only)
*/
static PyObject *
rv2py_int(arg)
	anything arg;
{
	return PyInt_FromLong((long)arg);
}

/*
** Integer output parameters
*/
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

static PyObject *
rv2py_pstring(arg)
	anything arg;
{
	unsigned char *p = (unsigned char *)arg;
	PyObject *rv;
	
	if ( arg == NULL ) return NULL;
	rv = PyString_FromStringAndSize((char *)p+1, p[0]);
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

static PyObject *
rv2py_cobject(arg)
	anything arg;
{
	void *ptr = (void *)arg;
	PyObject *rv;
	
	if ( ptr == 0 ) return NULL;
	rv = PyCObject_FromVoidPtr(ptr, 0);
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

static PyObject *
rv2py_handle(arg)
	anything arg;
{
	Handle rv = (Handle)arg;
	
	if ( rv == NULL ) return NULL;
	return ResObj_New(rv);
}

typedef struct {
	char *name;		/* Name */
	py2c_converter	get;	/* Get argument */
	int	get_uses_arg;	/* True if the above consumes an argument */
	c2py_converter	put;	/* Put result value */
} conventry;

static conventry converters[] = {
	{"InByte",	py2c_in_int,	1,	c2py_dummy},
	{"InShort",	py2c_in_int,	1,	c2py_dummy},
	{"InLong",	py2c_in_int,	1,	c2py_dummy},
	{"OutLong",	py2c_alloc,	0,	c2py_out_long},
	{"OutShort",	py2c_alloc,	0,	c2py_out_short},
	{"OutByte",	py2c_alloc,	0,	c2py_out_byte},
	{"InString",	py2c_in_string,	1,	c2py_dummy},
	{"InPstring",	py2c_in_pstring,1,	c2py_free},
	{"OutPstring",	py2c_out_pstring,0,	c2py_out_pstring},
	{"InCobject",	py2c_in_cobject,1,	c2py_dummy},
	{"OutCobject",	py2c_alloc,	0,	c2py_out_cobject},
	{"InHandle",	py2c_in_handle,	1,	c2py_dummy},
	{"OutHandle",	py2c_alloc,	0,	c2py_out_handle},
	{0, 0, 0, 0}
};

typedef struct {
	char *name;
	rv2py_converter rtn;
} rvconventry;

static rvconventry rvconverters[] = {
	{"None",	rv2py_none},
	{"OSErr",	rv2py_oserr},
	{"Byte",	rv2py_int},
	{"Short",	rv2py_int},
	{"Long",	rv2py_int},
	{"Pstring",	rv2py_pstring},
	{"Cobject",	rv2py_cobject},
	{"Handle",	rv2py_handle},
	{0, 0}
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

static rvconventry *
getrvconverter(name)
	char *name;
{
	int i;
	char buf[256];
	
	for(i=0; rvconverters[i].name; i++ )
		if ( strcmp(name, rvconverters[i].name) == 0 )
			return &rvconverters[i];
	sprintf(buf, "Unknown return value type: %s", name);
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

static int
argparse_rvconv(obj, ptr)
	PyObject *obj;
	rvconventry **ptr;
{
	char *name;
	int i;
	rvconventry *item;
	
	if( (name=PyString_AsString(obj)) == NULL )
		return 0;
	if( (item=getrvconverter(name)) == NULL )
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
	cdrobject *routine;		/* The routine to call */
	int npargs;			/* Python argument count */
	int ncargs;			/* C argument count + 1 */
	rvconventry *rvconv;		/* Return value converter */
	conventry *argconv[MAXARG];	/* Value converter list */
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
newcdcobject(routine, npargs, ncargs, rvconv, argconv)
	cdrobject *routine;
	int npargs;
	int ncargs;
	rvconventry *rvconv;
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
	self->ncargs = ncargs;
	self->rvconv = rvconv;
	for(i=0; i<MAXARG; i++)
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
	
	sprintf(buf, "<callable %s = %s(", self->rvconv->name, self->routine->name);
	for(i=0; i< self->ncargs; i++) {
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
	anything c_args[MAXARG] = {0, 0, 0, 0, 0, 0, 0, 0};
	anything c_rv;
	conventry *cp;
	PyObject *curarg;
	anyroutine func;
	PyObject *returnvalues[MAXARG+1];
	PyObject *rv;
	
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
	c_rv = (*func)(c_args[0], c_args[1], c_args[2], c_args[3],
			c_args[4], c_args[5], c_args[6], c_args[7]);

	/* Decode return value, and store into returnvalues if needed */
	pargindex = 0;
	curarg = (*self->rvconv->rtn)(c_rv);
	if ( curarg )
		returnvalues[pargindex++] = curarg;
		
	/* Decode returnvalue parameters and cleanup any storage allocated */
	for(i=0; i<self->ncargs; i++) {
		cp = self->argconv[i];
		curarg = (*cp->put)(c_args[i]);
		if(curarg)
			returnvalues[pargindex++] = curarg;
		/* NOTE: We only check errors at the end (so we free() everything) */
	}
	if ( PyErr_Occurred() ) {
		/* An error did occur. Free the python objects created */
		for(i=0; i<pargindex; i++)
			Py_XDECREF(returnvalues[i]);
		return NULL;
	}
	
	/* Zero and one return values cases are special: */
	if ( pargindex == 0 ) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	if ( pargindex == 1 )
		return returnvalues[0];
		
	/* More than one return value: put in a tuple */
	rv = PyTuple_New(pargindex);
	for(i=0; i<pargindex; i++)
		if(rv)
			PyTuple_SET_ITEM(rv, i, returnvalues[i]);
		else
			Py_XDECREF(returnvalues[i]);
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

static char cdf_keys__doc__[] =
"Return list of symbol names in fragment";

static PyObject *
cdf_keys(self, args)
	cdfobject *self;
	PyObject *args;
{
	long symcount;
	PyObject *rv, *obj;
	Str255 symname;
	Ptr dummy1;
	CFragSymbolClass dummy2;
	int i;
	OSErr err;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	if ( (err=CountSymbols(self->conn_id, &symcount)) < 0 )
		return PyMac_Error(err);
	if ( (rv=PyList_New(symcount)) == NULL )
		return NULL;
	for (i=0; i<symcount; i++) {
		if ((err=GetIndSymbol(self->conn_id, i, symname, &dummy1, &dummy2)) < 0 ) {
			Py_XDECREF(rv);
			return PyMac_Error(err);
		}
		if ((obj=PyString_FromStringAndSize((char *)symname+1, symname[0])) == NULL ) {
			Py_XDECREF(rv);
			return PyMac_Error(err);
		}
		if (PyList_SetItem(rv, i, obj) < 0 ) {
			Py_XDECREF(rv);
			return NULL;
		}
	}
	return rv;
}
		

static struct PyMethodDef cdf_methods[] = {
	{"keys",		(PyCFunction)cdf_keys,		METH_VARARGS,	
							cdf_keys__doc__},
	
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
cdf_getattr_helper(self, name)
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

static PyObject *
cdf_getattr(self, name)
	cdfobject *self;
	char *name;
{
	PyObject *rv;
	
	if ((rv=Py_FindMethod(cdf_methods, (PyObject *)self, name)))
		return rv;
	PyErr_Clear();
	return cdf_getattr_helper(self, name);
}

/* -------------------------------------------------------- */
/* Code to access cdf objects as mappings */

static int
cdf_length(self)
	cdfobject *self;
{
	long symcount;
	OSErr err;
	
	err = CountSymbols(self->conn_id, &symcount);
	if ( err ) {
		PyMac_Error(err);
		return -1;
	}
	return symcount;
}

static PyObject *
cdf_subscript(self, key)
	cdfobject *self;
	PyObject *key;
{
	char *name;
	
	if ((name=PyString_AsString(key)) == 0 )
		return 0;
	return cdf_getattr_helper(self, name);
}

static int
cdf_ass_sub(self, v, w)
	cdfobject *self;
	PyObject *v, *w;
{
	/* XXXX Put w in self under key v */
	return 0;
}

static PyMappingMethods cdf_as_mapping = {
	(inquiry)cdf_length,		/*mp_length*/
	(binaryfunc)cdf_subscript,		/*mp_subscript*/
	(objobjargproc)cdf_ass_sub,	/*mp_ass_subscript*/
};

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
	&cdf_as_mapping,		/*tp_as_mapping*/
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
	err = GetSharedLibrary(frag_name, kCompiledCFragArch, kLoadCFrag, &conn_id, &main_addr, 
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
	conventry *argconv[MAXARG] = {0, 0, 0, 0, 0, 0, 0, 0};
	rv2py_converter rvconv;
	int npargs, ncargs;

	/* Note: the next format depends on MAXARG */
	if (!PyArg_ParseTuple(args, "O!O&|O&O&O&O&O&O&O&O&", &Cdrtype, &routine,
		argparse_rvconv, &rvconv,
		argparse_conv, &argconv[0], argparse_conv, &argconv[1],
		argparse_conv, &argconv[2], argparse_conv, &argconv[3],
		argparse_conv, &argconv[4], argparse_conv, &argconv[5],
		argparse_conv, &argconv[6], argparse_conv, &argconv[7]))
		return NULL;
	npargs = 0;
	for(ncargs=0; ncargs < MAXARG && argconv[ncargs]; ncargs++) {
		if( argconv[ncargs]->get_uses_arg ) npargs++;
	}
	return (PyObject *)newcdcobject(routine, npargs, ncargs, rvconv, argconv);
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

#ifdef TESTSUPPORT

/* Test routine */
int cdll_b_bbbbbbbb(char a1,char  a2,char  a3,char  a4,char  a5,char  a6,char  a7,char  a8)
{
	return a1+a2+a3+a4+a5+a6+a7+a8;
}

short cdll_h_hhhhhhhh(short a1,short  a2,short  a3,short  a4,short  a5,short  a6,short  a7,short  a8)
{
	return a1+a2+a3+a4+a5+a6+a7+a8;
}

int cdll_l_llllllll(int a1,int  a2,int  a3,int  a4,int  a5,int  a6,int  a7,int  a8)
{
	return a1+a2+a3+a4+a5+a6+a7+a8;
}

void cdll_N_ssssssss(char *a1,char  *a2,char  *a3,char  *a4,char  *a5,char  *a6,char  *a7,char *a8)
{
	printf("cdll_N_ssssssss args: %s %s %s %s %s %s %s %s\n", a1, a2, a3, a4, 
			a5, a6, a7, a8);
}

OSErr cdll_o_l(long l)
{
	return (OSErr)l;
}

void cdll_N_pp(unsigned char *in, unsigned char *out)
{
	out[0] = in[0] + 5;
	strcpy((char *)out+1, "Was: ");
	memcpy(out+6, in+1, in[0]);
}

void cdll_N_bb(char a1, char *a2)
{
	*a2 = a1;
}

void cdll_N_hh(short a1, short *a2)
{
	*a2 = a1;
}

void cdll_N_ll(long a1, long *a2)
{
	*a2 = a1;
}

void cdll_N_sH(char *a1, Handle a2)
{
	int len;
	
	len = strlen(a1);
	SetHandleSize(a2, len);
	HLock(a2);
	memcpy(*a2, a1, len);
	HUnlock(a2);
}
#endif
