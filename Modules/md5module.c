
/* MD5 module */

/* This module provides an interface to the RSA Data Security,
   Inc. MD5 Message-Digest Algorithm, described in RFC 1321.
   It requires the files md5c.c and md5.h (which are slightly changed
   from the versions in the RFC to avoid the "global.h" file.) */


/* MD5 objects */

#include "Python.h"
#include "md5.h"

typedef struct {
	PyObject_HEAD
        MD5_CTX	md5;		/* the context holder */
} md5object;

staticforward PyTypeObject MD5type;

#define is_md5object(v)		((v)->ob_type == &MD5type)

static md5object *
newmd5object(void)
{
	md5object *md5p;

	md5p = PyObject_New(md5object, &MD5type);
	if (md5p == NULL)
		return NULL;

	MD5Init(&md5p->md5);	/* actual initialisation */
	return md5p;
}


/* MD5 methods */

static void
md5_dealloc(md5object *md5p)
{
	PyObject_Del(md5p);
}


/* MD5 methods-as-attributes */

static PyObject *
md5_update(md5object *self, PyObject *args)
{
	unsigned char *cp;
	int len;

	if (!PyArg_Parse(args, "s#", &cp, &len))
		return NULL;

	MD5Update(&self->md5, cp, len);

	Py_INCREF(Py_None);
	return Py_None;
}

static char update_doc [] =
"update (arg)\n\
\n\
Update the md5 object with the string arg. Repeated calls are\n\
equivalent to a single call with the concatenation of all the\n\
arguments.";


static PyObject *
md5_digest(md5object *self, PyObject *args)
{
 	MD5_CTX mdContext;
	unsigned char aDigest[16];

	if (!PyArg_NoArgs(args))
		return NULL;

	/* make a temporary copy, and perform the final */
	mdContext = self->md5;
	MD5Final(aDigest, &mdContext);

	return PyString_FromStringAndSize((char *)aDigest, 16);
}

static char digest_doc [] =
"digest() -> string\n\
\n\
Return the digest of the strings passed to the update() method so\n\
far. This is an 16-byte string which may contain non-ASCII characters,\n\
including null bytes.";


static PyObject *
md5_hexdigest(md5object *self, PyObject *args)
{
 	MD5_CTX mdContext;
	unsigned char digest[16];
	unsigned char hexdigest[32];
	int i, j;

	if (!PyArg_NoArgs(args))
		return NULL;

	/* make a temporary copy, and perform the final */
	mdContext = self->md5;
	MD5Final(digest, &mdContext);

	/* Make hex version of the digest */
	for(i=j=0; i<16; i++) {
		char c;
		c = (digest[i] >> 4) & 0xf;
		c = (c>9) ? c+'a'-10 : c + '0';
		hexdigest[j++] = c;
		c = (digest[i] & 0xf);
		c = (c>9) ? c+'a'-10 : c + '0';
		hexdigest[j++] = c;
	}
	return PyString_FromStringAndSize((char*)hexdigest, 32);
}


static char hexdigest_doc [] =
"hexdigest() -> string\n\
\n\
Like digest(), but returns the digest as a string of hexadecimal digits.";


static PyObject *
md5_copy(md5object *self, PyObject *args)
{
	md5object *md5p;

	if (!PyArg_NoArgs(args))
		return NULL;

	if ((md5p = newmd5object()) == NULL)
		return NULL;

	md5p->md5 = self->md5;

	return (PyObject *)md5p;
}

static char copy_doc [] =
"copy() -> md5 object\n\
\n\
Return a copy (``clone'') of the md5 object.";


static PyMethodDef md5_methods[] = {
	{"update",    (PyCFunction)md5_update,    METH_OLDARGS, update_doc},
	{"digest",    (PyCFunction)md5_digest,    METH_OLDARGS, digest_doc},
	{"hexdigest", (PyCFunction)md5_hexdigest, METH_OLDARGS, hexdigest_doc},
	{"copy",      (PyCFunction)md5_copy,      METH_OLDARGS, copy_doc},
	{NULL, NULL}			     /* sentinel */
};

static PyObject *
md5_getattr(md5object *self, char *name)
{
	return Py_FindMethod(md5_methods, (PyObject *)self, name);
}

static char module_doc [] =

"This module implements the interface to RSA's MD5 message digest\n\
algorithm (see also Internet RFC 1321). Its use is quite\n\
straightforward: use the new() to create an md5 object. You can now\n\
feed this object with arbitrary strings using the update() method, and\n\
at any point you can ask it for the digest (a strong kind of 128-bit\n\
checksum, a.k.a. ``fingerprint'') of the concatenation of the strings\n\
fed to it so far using the digest() method.\n\
\n\
Functions:\n\
\n\
new([arg]) -- return a new md5 object, initialized with arg if provided\n\
md5([arg]) -- DEPRECATED, same as new, but for compatibility\n\
\n\
Special Objects:\n\
\n\
MD5Type -- type object for md5 objects\n\
";

static char md5type_doc [] =
"An md5 represents the object used to calculate the MD5 checksum of a\n\
string of information.\n\
\n\
Methods:\n\
\n\
update() -- updates the current digest with an additional string\n\
digest() -- return the current digest value\n\
copy() -- return a copy of the current md5 object\n\
";

statichere PyTypeObject MD5type = {
	PyObject_HEAD_INIT(NULL)
	0,			  /*ob_size*/
	"md5",			  /*tp_name*/
	sizeof(md5object),	  /*tp_size*/
	0,			  /*tp_itemsize*/
	/* methods */
	(destructor)md5_dealloc,  /*tp_dealloc*/
	0,			  /*tp_print*/
	(getattrfunc)md5_getattr, /*tp_getattr*/
	0,			  /*tp_setattr*/
	0,			  /*tp_compare*/
	0,			  /*tp_repr*/
        0,			  /*tp_as_number*/
	0,                        /*tp_as_sequence*/
	0,			  /*tp_as_mapping*/
	0, 			  /*tp_hash*/
	0,			  /*tp_call*/
	0,			  /*tp_str*/
	0,			  /*tp_getattro*/
	0,			  /*tp_setattro*/
	0,	                  /*tp_as_buffer*/
	0,			  /*tp_xxx4*/
	md5type_doc,		  /*tp_doc*/
};


/* MD5 functions */

static PyObject *
MD5_new(PyObject *self, PyObject *args)
{
	md5object *md5p;
	unsigned char *cp = NULL;
	int len = 0;

	if (!PyArg_ParseTuple(args, "|s#:new", &cp, &len))
		return NULL;

	if ((md5p = newmd5object()) == NULL)
		return NULL;

	if (cp)
		MD5Update(&md5p->md5, cp, len);

	return (PyObject *)md5p;
}

static char new_doc [] =
"new([arg]) -> md5 object\n\
\n\
Return a new md5 object. If arg is present, the method call update(arg)\n\
is made.";


/* List of functions exported by this module */

static PyMethodDef md5_functions[] = {
	{"new",		(PyCFunction)MD5_new, METH_VARARGS, new_doc},
	{"md5",		(PyCFunction)MD5_new, METH_VARARGS, new_doc}, /* Backward compatibility */
	{NULL,		NULL}	/* Sentinel */
};


/* Initialize this module. */

DL_EXPORT(void)
initmd5(void)
{
	PyObject *m, *d;

        MD5type.ob_type = &PyType_Type;
	m = Py_InitModule3("md5", md5_functions, module_doc);
	d = PyModule_GetDict(m);
	PyDict_SetItemString(d, "MD5Type", (PyObject *)&MD5type);
	/* No need to check the error here, the caller will do that */
}
