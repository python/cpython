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
newmd5object()
{
	md5object *md5p;

	md5p = PyObject_NEW(md5object, &MD5type);
	if (md5p == NULL)
		return NULL;

	MD5Init(&md5p->md5);	/* actual initialisation */
	return md5p;
}


/* MD5 methods */

static void
md5_dealloc(md5p)
	md5object *md5p;
{
	PyMem_DEL(md5p);
}


/* MD5 methods-as-attributes */

static PyObject *
md5_update(self, args)
	md5object *self;
	PyObject *args;
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
md5_digest(self, args)
	md5object *self;
	PyObject *args;
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
md5_copy(self, args)
	md5object *self;
	PyObject *args;
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
	{"update",		(PyCFunction)md5_update, 0, update_doc},
	{"digest",		(PyCFunction)md5_digest, 0, digest_doc},
	{"copy",		(PyCFunction)md5_copy, 0, copy_doc},
	{NULL,			NULL}		/* sentinel */
};

static PyObject *
md5_getattr(self, name)
	md5object *self;
	char *name;
{
	return Py_FindMethod(md5_methods, (PyObject *)self, name);
}

static char module_doc [] =

"This module implements the interface to RSA's MD5 message digest\n\
algorithm (see also Internet RFC 1321). Its use is quite\n\
straightforward: use the new() to create an md5 object. You can now\n\
feed this object with arbitrary strings using the update() method, and\n\
at any point you can ask it for the digest (a strong kind of 128-bit\n\
checksum, a.k.a. ``fingerprint'') of the contatenation of the strings\n\
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
	PyObject_HEAD_INIT(&PyType_Type)
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
MD5_new(self, args)
	PyObject *self;
	PyObject *args;
{
	md5object *md5p;
	unsigned char *cp = NULL;
	int len = 0;

	if (!PyArg_ParseTuple(args, "|s#", &cp, &len))
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
	{"new",		(PyCFunction)MD5_new, 1, new_doc},
	{"md5",		(PyCFunction)MD5_new, 1, new_doc}, /* Backward compatibility */
	{NULL,		NULL}	/* Sentinel */
};


/* Initialize this module. */

void
initmd5()
{
	PyObject *m, *d;
	m = Py_InitModule3("md5", md5_functions, module_doc);
	d = PyModule_GetDict(m);
	PyDict_SetItemString(d, "MD5Type", (PyObject *)&MD5type);
	/* No need to check the error here, the caller will do that */
}
