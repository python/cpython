/***********************************************************
Copyright 2000 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.

******************************************************************/

#include "Python.h"
#include "xmlparse.h"

/*
** The version number should match the one in _checkversion
*/
#define VERSION "1.9"

enum HandlerTypes{
	StartElement, 
	EndElement, 
	ProcessingInstruction, 
	CharacterData,
	UnparsedEntityDecl,
	NotationDecl,
	StartNamespaceDecl,
	EndNamespaceDecl,
	Comment,
	StartCdataSection,
	EndCdataSection,
	Default,
	DefaultHandlerExpand,
	NotStandalone,
	ExternalEntityRef
};

static PyObject *ErrorObject;

/* ----------------------------------------------------- */

/* Declarations for objects of type xmlparser */

typedef struct {
	PyObject_HEAD

	XML_Parser itself;
        int returns_unicode; /* True if Unicode strings are returned;
				if false, UTF-8 strings are returned */
	PyObject **handlers;
} xmlparseobject;

staticforward PyTypeObject Xmlparsetype;

typedef void (*xmlhandlersetter)( XML_Parser *self, void *meth );
typedef void* xmlhandler;

struct HandlerInfo {
	const char *name;
	xmlhandlersetter setter;
	xmlhandler handler;
};

staticforward struct HandlerInfo handler_info[64];

/* Convert an array of attributes and their values into a Python dict */

static PyObject *conv_atts_using_string( XML_Char **atts){
	PyObject *attrs_obj=NULL;
	XML_Char **attrs_p, **attrs_k;
	int attrs_len;
	PyObject *rv;

	if( (attrs_obj = PyDict_New()) == NULL ) 
		goto finally;
	for(attrs_len=0, attrs_p = atts; 
	    *attrs_p;
	    attrs_p++, attrs_len++) {
		if (attrs_len%2) {
			rv=PyString_FromString(*attrs_p);  
			if (! rv) {
				Py_DECREF(attrs_obj);
				attrs_obj=NULL;
				goto finally;
			}
			if (PyDict_SetItemString(
			       attrs_obj,
			       (char*)*attrs_k, rv) < 0){
				Py_DECREF(attrs_obj);
				attrs_obj=NULL;
				goto finally;
			}
			Py_DECREF(rv);
		}
		else attrs_k=attrs_p;
	}
	finally:
	return attrs_obj;
}

#if !(PY_MAJOR_VERSION == 1 && PY_MINOR_VERSION < 6)
static PyObject *conv_atts_using_unicode( XML_Char **atts){
        PyObject *attrs_obj=NULL;
        XML_Char **attrs_p, **attrs_k;
        int attrs_len;

        if( (attrs_obj = PyDict_New()) == NULL ) 
                goto finally;
        for(attrs_len=0, attrs_p = atts; 
            *attrs_p;
            attrs_p++, attrs_len++) {
                if (attrs_len%2) {
		        PyObject *attr_str, *value_str;
		        const char *p = (const char *) (*attrs_k);
                        attr_str=PyUnicode_DecodeUTF8(p, strlen(p), "strict"); 
                        if (! attr_str) {
                                Py_DECREF(attrs_obj);
                                attrs_obj=NULL;
                                goto finally;
                        }
			p = (const char *) *attrs_p;
                        value_str=PyUnicode_DecodeUTF8(p, strlen(p), "strict");
                        if (! value_str) {
                                Py_DECREF(attrs_obj);
                                Py_DECREF(attr_str);
                                attrs_obj=NULL;
                                goto finally;
                        }
                        if (PyDict_SetItem(attrs_obj, attr_str, value_str) < 0) {
                                Py_DECREF(attrs_obj);
                                attrs_obj=NULL;
                                goto finally;
                        }
                        Py_DECREF(attr_str);
                        Py_DECREF(value_str);
                }
                else attrs_k=attrs_p;
        }
        finally:
        return attrs_obj;
}

/* Convert a string of XML_Chars into a Unicode string.
   Returns None if str is a null pointer. */

static PyObject *conv_string_to_unicode( XML_Char *str ) {
  /* XXX currently this code assumes that XML_Char is 8-bit, 
     and hence in UTF-8.  */
  /* UTF-8 from Expat, Unicode desired */
  if (str == NULL) {Py_INCREF(Py_None); return Py_None;}
  return PyUnicode_DecodeUTF8( (const char *)str, 
			       strlen( (const char *)str ), 
			       "strict" );
}

static PyObject *conv_string_len_to_unicode( const XML_Char *str, int len ) {
  /* XXX currently this code assumes that XML_Char is 8-bit, 
     and hence in UTF-8.  */
  /* UTF-8 from Expat, Unicode desired */
  if (str == NULL) {Py_INCREF(Py_None); return Py_None;}
  return PyUnicode_DecodeUTF8( (const char *)str, 
			       len, 
			       "strict" );
}
#endif

/* Convert a string of XML_Chars into an 8-bit Python string.
   Returns None if str is a null pointer. */

static PyObject *conv_string_to_utf8( XML_Char *str ) {
  /* XXX currently this code assumes that XML_Char is 8-bit, 
     and hence in UTF-8.  */
  /* UTF-8 from Expat, UTF-8 desired */
  if (str == NULL) {Py_INCREF(Py_None); return Py_None;}
  return PyString_FromString( (const char *)str );
}

static PyObject *conv_string_len_to_utf8( const XML_Char *str, 
					  int len ) 
{
  /* XXX currently this code assumes that XML_Char is 8-bit, 
     and hence in UTF-8.  */
  /* UTF-8 from Expat, UTF-8 desired */
  if (str == NULL) {Py_INCREF(Py_None); return Py_None;}
  return PyString_FromStringAndSize( (const char *)str, len );
}

/* Callback routines */

static void clear_handlers( xmlparseobject *self );

static void flag_error( xmlparseobject *self ){
	clear_handlers(self);
}

#define RC_HANDLER( RC, NAME, PARAMS, INIT, PARAM_FORMAT, CONVERSION, \
		RETURN, GETUSERDATA) \
\
static RC my_##NAME##Handler PARAMS {\
	xmlparseobject *self = GETUSERDATA ; \
	PyObject *args=NULL; \
	PyObject *rv=NULL; \
	INIT \
\
	if (self->handlers[NAME] \
	    && self->handlers[NAME] != Py_None) { \
		args = Py_BuildValue PARAM_FORMAT ;\
		if (!args) return RETURN; \
		rv = PyEval_CallObject(self->handlers[NAME], args); \
		Py_DECREF(args); \
		if (rv == NULL) { \
			flag_error( self ); \
			return RETURN; \
		} \
		CONVERSION \
		Py_DECREF(rv); \
	} \
	return RETURN; \
}

#if PY_MAJOR_VERSION == 1 && PY_MINOR_VERSION < 6
#define STRING_CONV_FUNC conv_string_to_utf8
#else
/* Python 1.6 and later versions */
#define STRING_CONV_FUNC (self->returns_unicode \
                          ? conv_string_to_unicode : conv_string_to_utf8)
#endif

#define VOID_HANDLER( NAME, PARAMS, PARAM_FORMAT ) \
	RC_HANDLER( void, NAME, PARAMS, ;, PARAM_FORMAT, ;, ;,\
	(xmlparseobject *)userData )

#define INT_HANDLER( NAME, PARAMS, PARAM_FORMAT )\
	RC_HANDLER( int, NAME, PARAMS, int rc=0;, PARAM_FORMAT,  \
			rc = PyInt_AsLong( rv );, rc, \
	(xmlparseobject *)userData )

#if PY_MAJOR_VERSION == 1 && PY_MINOR_VERSION < 6
VOID_HANDLER( StartElement, 
		(void *userData, const XML_Char *name, const XML_Char **atts ), 
                ("(O&O&)", STRING_CONV_FUNC, name, 
		 conv_atts_using_string, atts ) )
#else
/* Python 1.6 and later */
VOID_HANDLER( StartElement, 
		(void *userData, const XML_Char *name, const XML_Char **atts ), 
                ("(O&O&)", STRING_CONV_FUNC, name, 
		 (self->returns_unicode  
		  ? conv_atts_using_unicode 
		  : conv_atts_using_string), atts ) )
#endif

VOID_HANDLER( EndElement, 
		(void *userData, const XML_Char *name ), 
                ("(O&)", STRING_CONV_FUNC, name) )

VOID_HANDLER( ProcessingInstruction,
		(void *userData, 
		const XML_Char *target, 
		const XML_Char *data),
                ("(O&O&)",STRING_CONV_FUNC,target, STRING_CONV_FUNC,data ))

#if PY_MAJOR_VERSION == 1 && PY_MINOR_VERSION < 6
VOID_HANDLER( CharacterData, 
	      (void *userData, const XML_Char *data, int len), 
	      ("(O)", conv_string_len_to_utf8(data,len) ) )
#else
VOID_HANDLER( CharacterData, 
	      (void *userData, const XML_Char *data, int len), 
	      ("(O)", (self->returns_unicode 
		       ? conv_string_len_to_unicode(data,len) 
		       : conv_string_len_to_utf8(data,len) ) ) )
#endif

VOID_HANDLER( UnparsedEntityDecl,
		(void *userData, 
			const XML_Char *entityName,
			const XML_Char *base,
			const XML_Char *systemId,
		        const XML_Char *publicId,
		        const XML_Char *notationName),
                ("(O&O&O&O&O&)", 
		 STRING_CONV_FUNC,entityName, STRING_CONV_FUNC,base, 
		 STRING_CONV_FUNC,systemId, STRING_CONV_FUNC,publicId, 
		 STRING_CONV_FUNC,notationName))

VOID_HANDLER( NotationDecl, 
		(void *userData,
			const XML_Char *notationName,
			const XML_Char *base,
			const XML_Char *systemId,
			const XML_Char *publicId),
                ("(O&O&O&O&)", 
		 STRING_CONV_FUNC,notationName, STRING_CONV_FUNC,base, 
		 STRING_CONV_FUNC,systemId, STRING_CONV_FUNC,publicId))

VOID_HANDLER( StartNamespaceDecl,
		(void *userData,
		      const XML_Char *prefix,
		      const XML_Char *uri),
                ("(O&O&)", STRING_CONV_FUNC,prefix, STRING_CONV_FUNC,uri ))

VOID_HANDLER( EndNamespaceDecl,
		(void *userData,
		    const XML_Char *prefix),
                ("(O&)", STRING_CONV_FUNC,prefix ))

VOID_HANDLER( Comment,
               (void *userData, const XML_Char *prefix),
                ("(O&)", STRING_CONV_FUNC,prefix))

VOID_HANDLER( StartCdataSection,
               (void *userData),
		("()" ))
		
VOID_HANDLER( EndCdataSection,
               (void *userData),
		("()" ))

#if PY_MAJOR_VERSION == 1 && PY_MINOR_VERSION < 6
VOID_HANDLER( Default,
	      (void *userData,  const XML_Char *s, int len),
	      ("(O)", conv_string_len_to_utf8(s,len) ) )

VOID_HANDLER( DefaultHandlerExpand,
	      (void *userData,  const XML_Char *s, int len),
	      ("(O)", conv_string_len_to_utf8(s,len) ) )
#else
VOID_HANDLER( Default,
	      (void *userData,  const XML_Char *s, int len),
	      ("(O)", (self->returns_unicode 
		       ? conv_string_len_to_unicode(s,len) 
		       : conv_string_len_to_utf8(s,len) ) ) )

VOID_HANDLER( DefaultHandlerExpand,
	      (void *userData,  const XML_Char *s, int len),
	      ("(O)", (self->returns_unicode 
		       ? conv_string_len_to_unicode(s,len) 
		       : conv_string_len_to_utf8(s,len) ) ) )
#endif

INT_HANDLER( NotStandalone, 
		(void *userData), 
		("()"))

RC_HANDLER( int, ExternalEntityRef,
		(XML_Parser parser,
		    const XML_Char *context,
		    const XML_Char *base,
		    const XML_Char *systemId,
		    const XML_Char *publicId),
		int rc=0;,
                ("(O&O&O&O&)", 
		 STRING_CONV_FUNC,context, STRING_CONV_FUNC,base, 
		 STRING_CONV_FUNC,systemId, STRING_CONV_FUNC,publicId ),
		rc = PyInt_AsLong( rv );, rc,
		XML_GetUserData( parser ) )
		


/* File reading copied from cPickle */

#define UNLESS(E) if (!(E))

/*
static int 
read_other(xmlparseobject *self, char **s, int  n) {
    PyObject *bytes=NULL, *str=NULL, *arg=NULL;
    int res = -1;

    UNLESS(bytes = PyInt_FromLong(n)) {
        if (!PyErr_Occurred())
            PyErr_SetNone(PyExc_EOFError);

        goto finally;
    }

    UNLESS(arg)
        UNLESS(arg = PyTuple_New(1))
            goto finally;

    Py_INCREF(bytes);
    if (PyTuple_SetItem(arg, 0, bytes) < 0)
        goto finally;

    UNLESS(str = PyObject_CallObject(self->read, arg))
        goto finally;

    *s = PyString_AsString(str);

    res = n;

finally:
     Py_XDECREF(arg);
     Py_XDECREF(bytes);

     return res;
}

*/


    


/* ---------------------------------------------------------------- */

static char xmlparse_Parse__doc__[] = 
"(data [,isfinal]) - Parse XML data"
;

static PyObject *
xmlparse_Parse( xmlparseobject *self, PyObject *args )
{
	char *s;
	int slen;
	int isFinal = 0;
	int rv;

	if (!PyArg_ParseTuple(args, "s#|i", &s, &slen, &isFinal))
		return NULL;
	rv = XML_Parse(self->itself, s, slen, isFinal);
	if( PyErr_Occurred() ){	
		return NULL;
	}
	else if (rv == 0) {
                PyErr_Format(ErrorObject, "%.200s: line %i, column %i",
			     XML_ErrorString( XML_GetErrorCode(self->itself) ),
			     XML_GetErrorLineNumber(self->itself),
			     XML_GetErrorColumnNumber(self->itself) );
		return NULL;
	}

	return Py_BuildValue("i", rv);
}

#define BUF_SIZE 2048

int readinst(char *buf, int buf_size, PyObject *meth){
	PyObject *arg=NULL;
	PyObject *bytes=NULL;
	PyObject *str=NULL;
	int len;

	UNLESS(bytes = PyInt_FromLong(buf_size)) {
		if (!PyErr_Occurred())
		PyErr_SetNone(PyExc_EOFError);
		goto finally;
	}

	UNLESS(arg)
		UNLESS(arg = PyTuple_New(1))
		    goto finally;

	Py_INCREF(bytes);
	if (PyTuple_SetItem(arg, 0, bytes) < 0)
		goto finally;

	UNLESS(str = PyObject_CallObject(meth, arg))
		goto finally;

	UNLESS(PyString_Check( str ))
		goto finally;

	len=PyString_GET_SIZE( str );
	strncpy( buf, PyString_AsString(str), len );
	Py_XDECREF( str );
finally:
	return len;
}

static char xmlparse_ParseFile__doc__[] = 
"(file) - Parse XML data"
;

static PyObject *
xmlparse_ParseFile( xmlparseobject *self, PyObject *args )
{
	int rv=1;
	PyObject *f;
	FILE *fp;
	PyObject *readmethod=NULL;

	if (!PyArg_ParseTuple(args, "O", &f))
		return NULL;

	if (PyFile_Check(f)) {
		fp = PyFile_AsFile(f);
	}else{
		fp = NULL;
		UNLESS(readmethod = PyObject_GetAttrString(f, "read")) {
		    PyErr_Clear();
		    PyErr_SetString( PyExc_TypeError, 
			"argument must have 'read' attribute" );
		    return 0;
		}
	}
	
	for (;;) {
		  int bytes_read;
		  void *buf = XML_GetBuffer(self->itself, BUF_SIZE);
		  if (buf == NULL) {
		          PyErr_SetString(PyExc_MemoryError, "out of memory");
			  return NULL;
		  }

		  if( fp ){
		          bytes_read=fread( buf, sizeof( char ), BUF_SIZE, fp);
		  }else{
			  bytes_read=readinst( buf, BUF_SIZE, readmethod );
		  }

		  if (bytes_read < 0) {
			PyErr_SetFromErrno(PyExc_IOError);
			return NULL;
		  }
		  rv=XML_ParseBuffer(self->itself, bytes_read, bytes_read == 0);
		  if( PyErr_Occurred() ){
			return NULL;
		  }
		  if (!rv || bytes_read == 0)
			break;
	}

	return Py_BuildValue("i", rv);
}

static char xmlparse_SetBase__doc__[] = 
"(base_url) - Base URL string"
;

static PyObject *
xmlparse_SetBase( xmlparseobject *self, PyObject *args ){
    char *base;

    if (!PyArg_ParseTuple(args, "s", &base))
        return NULL;
    if( !XML_SetBase( self->itself, base ) ){
	PyErr_SetNone(PyExc_MemoryError);
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static char xmlparse_GetBase__doc__[] = 
"() - returns base URL string "
;

static PyObject *
xmlparse_GetBase( xmlparseobject *self, PyObject *args ){
    const XML_Char *base;
    PyObject *rc;

    if( PyTuple_Size( args )!=0 ){
	    PyArg_ParseTuple(args, "()" ); /* get good error reporting */
        return NULL;
    }
    base=XML_GetBase( self->itself );
    if( base ){
	rc=Py_BuildValue("s", base);
    }else{
	Py_INCREF(Py_None);
	rc=Py_None;
    }
    return rc;

}

static struct PyMethodDef xmlparse_methods[] = {
	{"Parse",	(PyCFunction)xmlparse_Parse,
	 	METH_VARARGS,	xmlparse_Parse__doc__},
	{"ParseFile",	(PyCFunction)xmlparse_ParseFile,
	 	METH_VARARGS,	xmlparse_ParseFile__doc__},
	{"SetBase", (PyCFunction)xmlparse_SetBase,
	 	METH_VARARGS,      xmlparse_SetBase__doc__},
	{"GetBase", (PyCFunction)xmlparse_GetBase,
	 	METH_VARARGS,      xmlparse_GetBase__doc__},
	{NULL,		NULL}		/* sentinel */
};

/* ---------- */


static xmlparseobject *
newxmlparseobject( char *encoding, char *namespace_separator){
	int i;
	xmlparseobject *self;
	
#if PY_MAJOR_VERSION == 1 && PY_MINOR_VERSION < 6
	self = PyObject_NEW(xmlparseobject, &Xmlparsetype);
	if (self == NULL)
		return NULL;

	self->returns_unicode = 0;
#else
        /* Code for versions 1.6 and later */
        self = PyObject_New(xmlparseobject, &Xmlparsetype);
	if (self == NULL)
		return NULL;

	self->returns_unicode = 1;
#endif
	if (namespace_separator) {
		self->itself = XML_ParserCreateNS(encoding, 
						*namespace_separator);
	}else{
		self->itself = XML_ParserCreate(encoding);
	}

	if( self->itself==NULL ){
		        PyErr_SetString(PyExc_RuntimeError, 
					"XML_ParserCreate failed");
			Py_DECREF(self);
			return NULL;
	}

	XML_SetUserData(self->itself, (void *)self);

	for( i=0; handler_info[i].name!=NULL;i++);

	self->handlers=malloc( sizeof( PyObject *)*i );

	clear_handlers( self );

	return self;
}


static void
xmlparse_dealloc( xmlparseobject *self )
{
	int i;
	if (self->itself)
		XML_ParserFree(self->itself);
	self->itself = NULL;

	for( i=0; handler_info[i].name!=NULL; i++ ){
		Py_XDECREF( self->handlers[i] );
	}
#if PY_MAJOR_VERSION == 1 && PY_MINOR_VERSION < 6
        /* Code for versions before 1.6 */
        free(self);
#else
        /* Code for versions 1.6 and later */
        PyObject_Del(self);
#endif
}

static int handlername2int( const char *name ){
	int i;
	for( i=0;handler_info[i].name!=NULL;i++){
		if( strcmp( name, handler_info[i].name )==0 ){
			return i;
		}
	}
	return -1;
}

static PyObject *
xmlparse_getattr(xmlparseobject *self, char *name)
{
	int handlernum;
	if (strcmp(name, "ErrorCode") == 0)
		return Py_BuildValue("l",
				(long)XML_GetErrorCode(self->itself));
	if (strcmp(name, "ErrorLineNumber") == 0)
		return Py_BuildValue("l",
				(long)XML_GetErrorLineNumber(self->itself));
	if (strcmp(name, "ErrorColumnNumber") == 0)
		return Py_BuildValue("l",
				(long)XML_GetErrorColumnNumber(self->itself));
	if (strcmp(name, "ErrorByteIndex") == 0)
		return Py_BuildValue("l",
				XML_GetErrorByteIndex(self->itself));
        if (strcmp(name, "returns_unicode") == 0)
	        return Py_BuildValue("i", self->returns_unicode);

	handlernum=handlername2int( name );

	if( handlernum!=-1 && self->handlers[handlernum]!=NULL){
		Py_INCREF( self->handlers[handlernum] );
		return self->handlers[handlernum];
	}

	if (strcmp(name, "__members__") == 0){
		int i;
		PyObject *rc=PyList_New(0);
		for(i=0; handler_info[i].name!=NULL;i++ ){
			PyList_Append( rc, 
				PyString_FromString( handler_info[i].name ) );
		}
		PyList_Append( rc, PyString_FromString( "ErrorCode" )); 
		PyList_Append( rc, PyString_FromString( "ErrorLineNumber" ));
		PyList_Append( rc, PyString_FromString( "ErrorColumnNumber")); 
		PyList_Append( rc, PyString_FromString( "ErrorByteIndex" )); 

		return rc;
	}

	return Py_FindMethod(xmlparse_methods, (PyObject *)self, name);
}

static int sethandler( xmlparseobject *self, const char *name, PyObject* v ){
	int handlernum = handlername2int( name );
	if( handlernum!=-1 ){
		Py_INCREF( v );
		Py_XDECREF( self->handlers[handlernum] );
		self->handlers[handlernum]=v;
		handler_info[handlernum].setter( self->itself, 
				handler_info[handlernum].handler );
		return 1;
	}

	return 0;
}

static int
xmlparse_setattr( xmlparseobject *self, char *name, PyObject *v)
{
	/* Set attribute 'name' to value 'v'. v==NULL means delete */
	if (v==NULL) {
		PyErr_SetString(PyExc_RuntimeError, "Cannot delete attribute");
		return -1;
	}

        if (strcmp(name, "returns_unicode") == 0) {
		PyObject *intobj = PyNumber_Int(v);
		if (intobj == NULL) return -1;
		if (PyInt_AsLong(intobj)) {
#if PY_MAJOR_VERSION == 1 && PY_MINOR_VERSION < 6
		  PyErr_SetString(PyExc_ValueError, 
			 "Cannot return Unicode strings in Python 1.5");
		  return -1;
#else
		  self->returns_unicode = 1;
#endif
		}
		else                      self->returns_unicode = 0;
		Py_DECREF(intobj);
		return 0;
	}
	if( sethandler( self, name, v ) ){
		return 0;
	}

	PyErr_SetString( PyExc_AttributeError, name );
	return -1;
}

static char Xmlparsetype__doc__[] = 
"XML parser"
;

static PyTypeObject Xmlparsetype = {
	PyObject_HEAD_INIT(NULL)
	0,				/*ob_size*/
	"xmlparser",			/*tp_name*/
	sizeof(xmlparseobject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)xmlparse_dealloc,	/*tp_dealloc*/
	(printfunc)0,		/*tp_print*/
	(getattrfunc)xmlparse_getattr,	/*tp_getattr*/
	(setattrfunc)xmlparse_setattr,	/*tp_setattr*/
	(cmpfunc)0,		/*tp_compare*/
	(reprfunc)0,		/*tp_repr*/
	0,			/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)0,		/*tp_hash*/
	(ternaryfunc)0,		/*tp_call*/
	(reprfunc)0,		/*tp_str*/

	/* Space for future expansion */
	0L,0L,0L,0L,
	Xmlparsetype__doc__ /* Documentation string */
};

/* End of code for xmlparser objects */
/* -------------------------------------------------------- */


static char pyexpat_ParserCreate__doc__[] =
"([encoding, namespace_separator]) - Return a new XML parser object"
;

static PyObject *
pyexpat_ParserCreate(PyObject *notused, PyObject *args, PyObject *kw) {
	char *encoding  = NULL, *namespace_separator=NULL;
	static char *kwlist[] = {"encoding", "namespace_separator", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kw, "|zz", kwlist,
					 &encoding, &namespace_separator))
		return NULL;
	return (PyObject *)newxmlparseobject(encoding, namespace_separator);
}

static char pyexpat_ErrorString__doc__[] =
"(errno) Returns string error for given number"
;

static PyObject *
pyexpat_ErrorString(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	long code;
	
	if (!PyArg_ParseTuple(args, "l", &code))
		return NULL;
	return Py_BuildValue("z", XML_ErrorString((int)code));
}

/* List of methods defined in the module */

static struct PyMethodDef pyexpat_methods[] = {
	{"ParserCreate",	(PyCFunction)pyexpat_ParserCreate,
		 METH_VARARGS|METH_KEYWORDS, pyexpat_ParserCreate__doc__},
	{"ErrorString",	(PyCFunction)pyexpat_ErrorString,
	 	METH_VARARGS,	pyexpat_ErrorString__doc__},
 
	{NULL,	 (PyCFunction)NULL, 0, NULL}		/* sentinel */
};

/* Module docstring */

static char pyexpat_module_documentation[] = 
"Python wrapper for Expat parser."
;

/* Initialization function for the module */

void
initpyexpat(){
	PyObject *m, *d;
	char *rev = "$Revision$";
	PyObject *errors_module, *errors_dict;
	PyObject *sys_modules;

        Xmlparsetype.ob_type = &PyType_Type;

	/* Create the module and add the functions */
	m = Py_InitModule4("pyexpat", pyexpat_methods,
			   pyexpat_module_documentation,
			   (PyObject*)NULL, PYTHON_API_VERSION);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ErrorObject = PyString_FromString("pyexpat.error");
	PyDict_SetItemString(d, "error", ErrorObject);

	PyDict_SetItemString(d, "__version__",
			     PyString_FromStringAndSize(rev+11,
							strlen(rev+11)-2));

	sys_modules = PySys_GetObject("modules");
	errors_module = PyModule_New("pyexpat.errors");
	PyDict_SetItemString(d, "errors", errors_module);
	PyDict_SetItemString(sys_modules, "pyexpat.errors", errors_module);

	/* XXX When Expat supports some way of figuring out how it was
	   compiled, this should check and set native_encoding 
	   appropriately. 
	*/
	PyDict_SetItemString(d, "native_encoding", 
			     PyString_FromString("UTF-8"));
	errors_dict = PyModule_GetDict(errors_module);

#define MYCONST(name) \
	PyDict_SetItemString(errors_dict, #name,  \
			     PyString_FromString(XML_ErrorString(name)))

	MYCONST(XML_ERROR_NO_MEMORY);
	MYCONST(XML_ERROR_SYNTAX);
	MYCONST(XML_ERROR_NO_ELEMENTS);
	MYCONST(XML_ERROR_INVALID_TOKEN);
	MYCONST(XML_ERROR_UNCLOSED_TOKEN);
	MYCONST(XML_ERROR_PARTIAL_CHAR);
	MYCONST(XML_ERROR_TAG_MISMATCH);
	MYCONST(XML_ERROR_DUPLICATE_ATTRIBUTE);
	MYCONST(XML_ERROR_JUNK_AFTER_DOC_ELEMENT);
	MYCONST(XML_ERROR_PARAM_ENTITY_REF);
	MYCONST(XML_ERROR_UNDEFINED_ENTITY);
	MYCONST(XML_ERROR_RECURSIVE_ENTITY_REF);
	MYCONST(XML_ERROR_ASYNC_ENTITY);
	MYCONST(XML_ERROR_BAD_CHAR_REF);
	MYCONST(XML_ERROR_BINARY_ENTITY_REF);
	MYCONST(XML_ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF);
	MYCONST(XML_ERROR_MISPLACED_XML_PI);
	MYCONST(XML_ERROR_UNKNOWN_ENCODING);
	MYCONST(XML_ERROR_INCORRECT_ENCODING);

	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module pyexpat");
}

void clear_handlers( xmlparseobject *self ){
	int i=0;

	for( i=0;handler_info[i].name!=NULL;i++ ){
		self->handlers[i]=NULL;
		handler_info[i].setter( self->itself, NULL );
	}
}

typedef void (*pairsetter)( XML_Parser, void *handler1, void *handler2 );

void pyxml_UpdatePairedHandlers( xmlparseobject *self, 
				int startHandler, 
				int endHandler,
				pairsetter setter){
	void *start_handler=NULL;
	void *end_handler=NULL;

	if( self->handlers[startHandler] && 
			self->handlers[endHandler]!=Py_None ){
		start_handler=handler_info[startHandler].handler;
	}
	if( self->handlers[EndElement] && 
			self->handlers[EndElement] !=Py_None ){
		end_handler=handler_info[endHandler].handler;
	}
	
	setter(self->itself, 
			      start_handler,
			      end_handler);
}

void pyxml_SetStartElementHandler( XML_Parser *parser, 
				void *junk){
	pyxml_UpdatePairedHandlers(
		(xmlparseobject *)XML_GetUserData( parser ), 
		StartElement, EndElement,
		(pairsetter)XML_SetElementHandler);
}

void pyxml_SetEndElementHandler( XML_Parser *parser,
				void *junk){
	pyxml_UpdatePairedHandlers(
		(xmlparseobject *)XML_GetUserData( parser ), 
		StartElement, EndElement,
		(pairsetter)XML_SetElementHandler);
}

void pyxml_SetStartNamespaceDeclHandler( XML_Parser *parser,
				void *junk){
	pyxml_UpdatePairedHandlers(
		(xmlparseobject *)XML_GetUserData( parser ), 
		StartNamespaceDecl, EndNamespaceDecl,
		(pairsetter)XML_SetNamespaceDeclHandler);
}

void pyxml_SetEndNamespaceDeclHandler( XML_Parser *parser,
				void *junk){
	pyxml_UpdatePairedHandlers(
		(xmlparseobject *)XML_GetUserData( parser ), 
		StartNamespaceDecl, EndNamespaceDecl, 
		(pairsetter)XML_SetNamespaceDeclHandler);
}

void pyxml_SetStartCdataSection( XML_Parser *parser,
				void *junk){

	pyxml_UpdatePairedHandlers(
		(xmlparseobject *)XML_GetUserData( parser ), 
		StartCdataSection, EndCdataSection, 
		(pairsetter)XML_SetCdataSectionHandler);
}

void pyxml_SetEndCdataSection( XML_Parser *parser,
				void *junk){
	pyxml_UpdatePairedHandlers(
		(xmlparseobject *)XML_GetUserData( parser ), 
		StartCdataSection, EndCdataSection, 
		(pairsetter)XML_SetCdataSectionHandler);
}

statichere struct HandlerInfo handler_info[]=
{{"StartElementHandler", 
	pyxml_SetStartElementHandler, 
        (xmlhandler)my_StartElementHandler},
{"EndElementHandler", 
	pyxml_SetEndElementHandler, 
	(xmlhandler)my_EndElementHandler},
{"ProcessingInstructionHandler", 
	(xmlhandlersetter)XML_SetProcessingInstructionHandler,
	(xmlhandler)my_ProcessingInstructionHandler},
{"CharacterDataHandler", 
	(xmlhandlersetter)XML_SetCharacterDataHandler,
	(xmlhandler)my_CharacterDataHandler},
{"UnparsedEntityDeclHandler", 
	(xmlhandlersetter)XML_SetUnparsedEntityDeclHandler,
	(xmlhandler)my_UnparsedEntityDeclHandler },
{"NotationDeclHandler", 
	(xmlhandlersetter)XML_SetNotationDeclHandler,
	(xmlhandler)my_NotationDeclHandler },
{"StartNamespaceDeclHandler", 
	pyxml_SetStartNamespaceDeclHandler,
	(xmlhandler)my_StartNamespaceDeclHandler },
{"EndNamespaceDeclHandler", 
	pyxml_SetEndNamespaceDeclHandler,
	(xmlhandler)my_EndNamespaceDeclHandler },
{"CommentHandler",
	(xmlhandlersetter)XML_SetCommentHandler,
	(xmlhandler)my_CommentHandler},
{"StartCdataSectionHandler",
	pyxml_SetStartCdataSection,
	(xmlhandler)my_StartCdataSectionHandler},
{"EndCdataSectionHandler",
	pyxml_SetEndCdataSection,
	(xmlhandler)my_EndCdataSectionHandler},
{"DefaultHandler",
	(xmlhandlersetter)XML_SetDefaultHandler,
	(xmlhandler)my_DefaultHandler},
{"DefaultHandlerExpand",
	(xmlhandlersetter)XML_SetDefaultHandlerExpand,
	(xmlhandler)my_DefaultHandlerExpandHandler},
{"NotStandaloneHandler",
	(xmlhandlersetter)XML_SetNotStandaloneHandler,
	(xmlhandler)my_NotStandaloneHandler},
{"ExternalEntityRefHandler",
	(xmlhandlersetter)XML_SetExternalEntityRefHandler,
	(xmlhandler)my_ExternalEntityRefHandler },

{NULL, NULL, NULL } /* sentinel */
};
