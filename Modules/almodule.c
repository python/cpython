/**********************************************************
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

/* AL module -- interface to Mark Callow's Audio Library (AL). */

#include <audio.h>

/* Check which version audio library we have: */
#ifdef AL_ERROR_NUMBER
#define AL_405
/* XXXX 4.0.5 libaudio also allows us to provide better error
** handling (with ALseterrorhandler). We should implement that
** sometime.
*/

#endif

#include "Python.h"

/* Config objects */

typedef struct {
	PyObject_HEAD
	ALconfig ob_config;
} configobject;

staticforward PyTypeObject Configtype;

#define is_configobject(v) ((v)->ob_type == &Configtype)

/* Forward */
static int getconfigarg Py_PROTO((PyObject *, ALconfig *));
static int getstrstrconfigarg Py_PROTO((PyObject *, char **, char **,
					ALconfig *));

static PyObject *
setConfig (self, args, func)
	configobject *self;
	PyObject *args;
	void (*func)(ALconfig, long);
{
	long par;

	if (!PyArg_Parse (args, "l", &par)) return NULL;

	(*func) (self-> ob_config, par);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
getConfig (self, args, func)
	configobject *self;
	PyObject *args;
	long (*func)(ALconfig);
{	
	long par;

	if (!PyArg_NoArgs (args)) return NULL;
	
	par = (*func) (self-> ob_config);

	return PyInt_FromLong (par);
}

static PyObject *
al_setqueuesize (self, args)
	configobject *self;
	PyObject *args;
{
	return (setConfig (self, args, ALsetqueuesize));
}

static PyObject *
al_getqueuesize (self, args)
	configobject *self;
	PyObject *args;
{
	return (getConfig (self, args, ALgetqueuesize));
}

static PyObject *
al_setwidth (self, args)
	configobject *self;
	PyObject *args;
{
	return (setConfig (self, args, ALsetwidth));
}

static PyObject *
al_getwidth (self, args)
	configobject *self;
	PyObject *args;
{
	return (getConfig (self, args, ALgetwidth));	
}

static PyObject *
al_getchannels (self, args)
	configobject *self;
	PyObject *args;
{
	return (getConfig (self, args, ALgetchannels));	
}

static PyObject *
al_setchannels (self, args)
	configobject *self;
	PyObject *args;
{
	return (setConfig (self, args, ALsetchannels));
}

#ifdef AL_405

static PyObject *
al_getsampfmt (self, args)
	configobject *self;
	PyObject *args;
{
	return (getConfig (self, args, ALgetsampfmt));	
}

static PyObject *
al_setsampfmt (self, args)
	configobject *self;
	PyObject *args;
{
	return (setConfig (self, args, ALsetsampfmt));
}

static PyObject *
al_getfloatmax(self, args)
	configobject *self;
	PyObject *args;
{
	double arg;

	if ( !PyArg_NoArgs(args) )
	  return 0;
	arg = ALgetfloatmax(self->ob_config);
	return PyFloat_FromDouble(arg);
}

static PyObject *
al_setfloatmax(self, args)
	configobject *self;
	PyObject *args;
{
	double arg;

	if ( !PyArg_Parse(args, "d", &arg) )
	  return 0;
	ALsetfloatmax(self->ob_config, arg);
	Py_INCREF(Py_None);
	return Py_None;
}
#endif /* AL_405 */
	
static PyMethodDef config_methods[] = {
	{"getqueuesize",	(PyCFunction)al_getqueuesize},
	{"setqueuesize",	(PyCFunction)al_setqueuesize},
	{"getwidth",		(PyCFunction)al_getwidth},
	{"setwidth",		(PyCFunction)al_setwidth},
	{"getchannels",		(PyCFunction)al_getchannels},
	{"setchannels",		(PyCFunction)al_setchannels},
#ifdef AL_405
	{"getsampfmt",		(PyCFunction)al_getsampfmt},
	{"setsampfmt",		(PyCFunction)al_setsampfmt},
	{"getfloatmax",		(PyCFunction)al_getfloatmax},
	{"setfloatmax",		(PyCFunction)al_setfloatmax},
#endif /* AL_405 */
	{NULL,			NULL}		/* sentinel */
};

static void
config_dealloc(self)
	configobject *self;
{
	ALfreeconfig(self->ob_config);
	PyMem_DEL(self);
}

static PyObject *
config_getattr(self, name)
	configobject *self;
	char *name;
{
	return Py_FindMethod(config_methods, (PyObject *)self, name);
}

static PyTypeObject Configtype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"config",		/*tp_name*/
	sizeof(configobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)config_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)config_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

static PyObject *
newconfigobject(config)
	ALconfig config;
{
	configobject *p;
	
	p = PyObject_NEW(configobject, &Configtype);
	if (p == NULL)
		return NULL;
	p->ob_config = config;
	return (PyObject *)p;
}

/* Port objects */

typedef struct {
	PyObject_HEAD
	ALport ob_port;
} portobject;

staticforward PyTypeObject Porttype;

#define is_portobject(v) ((v)->ob_type == &Porttype)

static PyObject *
al_closeport (self, args)
	portobject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs (args)) return NULL;

	if (self->ob_port != NULL) {
		ALcloseport (self-> ob_port);
		self->ob_port = NULL;
		/* XXX Using a closed port may dump core! */
	}

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
al_getfd (self, args)
	portobject *self;
	PyObject *args;
{
	int fd;

	if (!PyArg_NoArgs (args)) return NULL;

	fd = ALgetfd (self-> ob_port);

	return PyInt_FromLong (fd);
}

static PyObject *
al_getfilled (self, args)
	portobject *self;
	PyObject *args;
{
	long count;

	if (!PyArg_NoArgs (args)) return NULL;
	
	count = ALgetfilled (self-> ob_port);

	return PyInt_FromLong (count);
}

static PyObject *
al_getfillable (self, args)
	portobject *self;
	PyObject *args;
{
	long count;

	if (!PyArg_NoArgs (args)) return NULL;
	
	count = ALgetfillable (self-> ob_port);

	return PyInt_FromLong (count);
}

static PyObject *
al_readsamps (self, args)
	portobject *self;
	PyObject *args;
{
	long count;
	PyObject *v;
	ALconfig c;
	int width;

	if (!PyArg_Parse (args, "l", &count)) return NULL;

	if (count <= 0)
	{
		PyErr_SetString (PyExc_RuntimeError,
				 "al.readsamps : arg <= 0");
		return NULL;
	}

	c = ALgetconfig(self->ob_port);
#ifdef AL_405
	width = ALgetsampfmt(c);
	if ( width == AL_SAMPFMT_FLOAT )
	  width = sizeof(float);
	else if ( width == AL_SAMPFMT_DOUBLE )
	  width = sizeof(double);
	else
	  width = ALgetwidth(c);
#else
	width = ALgetwidth(c);
#endif /* AL_405 */
	ALfreeconfig(c);
	v = PyString_FromStringAndSize ((char *)NULL, width * count);
	if (v == NULL) return NULL;

	Py_BEGIN_ALLOW_THREADS
	ALreadsamps (self-> ob_port, (void *) PyString_AsString(v), count);
	Py_END_ALLOW_THREADS

	return (v);
}

static PyObject *
al_writesamps (self, args)
	portobject *self;
	PyObject *args;
{
	char *buf;
	int size, width;
	ALconfig c;

	if (!PyArg_Parse (args, "s#", &buf, &size)) return NULL;

	c = ALgetconfig(self->ob_port);
#ifdef AL_405
	width = ALgetsampfmt(c);
	if ( width == AL_SAMPFMT_FLOAT )
	  width = sizeof(float);
	else if ( width == AL_SAMPFMT_DOUBLE )
	  width = sizeof(double);
	else
	  width = ALgetwidth(c);
#else
	width = ALgetwidth(c);
#endif /* AL_405 */
	ALfreeconfig(c);
	Py_BEGIN_ALLOW_THREADS
	ALwritesamps (self-> ob_port, (void *) buf, (long) size / width);
	Py_END_ALLOW_THREADS

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
al_getfillpoint (self, args)
	portobject *self;
	PyObject *args;
{
	long count;

	if (!PyArg_NoArgs (args)) return NULL;
	
	count = ALgetfillpoint (self-> ob_port);

	return PyInt_FromLong (count);
}

static PyObject *
al_setfillpoint (self, args)
	portobject *self;
	PyObject *args;
{
	long count;

	if (!PyArg_Parse (args, "l", &count)) return NULL;
	
	ALsetfillpoint (self-> ob_port, count);

	Py_INCREF (Py_None);
	return (Py_None);
}

static PyObject *
al_setconfig (self, args)
	portobject *self;
	PyObject *args;
{
	ALconfig config;

	if (!getconfigarg (args, &config)) return NULL;
	
	ALsetconfig (self-> ob_port, config);

	Py_INCREF (Py_None);
	return (Py_None);
}

static PyObject *
al_getconfig (self, args)
	portobject *self;
	PyObject *args;
{
	ALconfig config;

	if (!PyArg_NoArgs (args)) return NULL;
	
	config = ALgetconfig (self-> ob_port);

	return newconfigobject (config);
}

#ifdef AL_405
static PyObject *
al_getstatus (self, args)
	portobject *self;
	PyObject *args;
{
	PyObject *list, *v;
	long *PVbuffer;
	long length;
	int i;
	
	if (!PyArg_Parse(args, "O", &list))
		return NULL;
	if (!PyList_Check(list)) {
		PyErr_BadArgument();
		return NULL;
	}
	length = PyList_Size(list);
	PVbuffer = PyMem_NEW(long, length);
	if (PVbuffer == NULL)
		return PyErr_NoMemory();
	for (i = 0; i < length; i++) {
		v = PyList_GetItem(list, i);
		if (!PyInt_Check(v)) {
			PyMem_DEL(PVbuffer);
			PyErr_BadArgument();
			return NULL;
		}
		PVbuffer[i] = PyInt_AsLong(v);
	}

	ALgetstatus(self->ob_port, PVbuffer, length);

	for (i = 0; i < length; i++)
	  PyList_SetItem(list, i, PyInt_FromLong(PVbuffer[i]));

	PyMem_DEL(PVbuffer);

	Py_INCREF(Py_None);
	return Py_None;
}
#endif /* AL_405 */

static PyMethodDef port_methods[] = {
	{"closeport",		(PyCFunction)al_closeport},
	{"getfd",		(PyCFunction)al_getfd},
        {"fileno",		(PyCFunction)al_getfd},
	{"getfilled",		(PyCFunction)al_getfilled},
	{"getfillable",		(PyCFunction)al_getfillable},
	{"readsamps",		(PyCFunction)al_readsamps},
	{"writesamps",		(PyCFunction)al_writesamps},
	{"setfillpoint",	(PyCFunction)al_setfillpoint},
	{"getfillpoint",	(PyCFunction)al_getfillpoint},
	{"setconfig",		(PyCFunction)al_setconfig},
	{"getconfig",		(PyCFunction)al_getconfig},
#ifdef AL_405
	{"getstatus",		(PyCFunction)al_getstatus},
#endif /* AL_405 */	    
	{NULL,			NULL}		/* sentinel */
};

static void
port_dealloc(p)
	portobject *p;
{
	if (p->ob_port != NULL)
		ALcloseport(p->ob_port);
	PyMem_DEL(p);
}

static PyObject *
port_getattr(p, name)
	portobject *p;
	char *name;
{
	return Py_FindMethod(port_methods, (PyObject *)p, name);
}

static PyTypeObject Porttype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"port",			/*tp_name*/
	sizeof(portobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)port_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)port_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

static PyObject *
newportobject(port)
	ALport port;
{
	portobject *p;
	
	p = PyObject_NEW(portobject, &Porttype);
	if (p == NULL)
		return NULL;
	p->ob_port = port;
	return (PyObject *)p;
}

/* the module al */

static PyObject *
al_openport (self, args)
	PyObject *self, *args;
{
	char *name, *dir;
	ALport port;
	ALconfig config = NULL;
	int size;

	if (args == NULL || !PyTuple_Check(args)) {
		PyErr_BadArgument ();
		return NULL;
	}
	size = PyTuple_Size(args);
	if (size == 2) {
		if (!PyArg_Parse (args, "(ss)", &name, &dir))
			return NULL;
	}
	else if (size == 3) {
		if (!getstrstrconfigarg (args, &name, &dir, &config))
			return NULL;
	}
	else {
		PyErr_BadArgument ();
		return NULL;
	}

	port = ALopenport(name, dir, config);

	if (port == NULL) {
		PyErr_SetFromErrno(PyExc_RuntimeError);
		return NULL;
	}

	return newportobject (port);
}

static PyObject *
al_newconfig (self, args)
	PyObject *self, *args;
{
	ALconfig config;

	if (!PyArg_NoArgs (args)) return NULL;

	config = ALnewconfig ();
	if (config == NULL) {
		PyErr_SetFromErrno(PyExc_RuntimeError);
		return NULL;
	}

	return newconfigobject (config);
}

static PyObject *
al_queryparams(self, args)
	PyObject *self, *args;
{
	long device;
	long length;
	long *PVbuffer;
	long PVdummy[2];
	PyObject *v;

	if (!PyArg_Parse (args, "l", &device))
		return NULL;
	length = ALqueryparams(device, PVdummy, 2L);
	PVbuffer = PyMem_NEW(long, length);
	if (PVbuffer == NULL)
		return PyErr_NoMemory();
	(void) ALqueryparams(device, PVbuffer, length);
	v = PyList_New((int)length);
	if (v != NULL) {
		int i;
		for (i = 0; i < length; i++)
			PyList_SetItem(v, i, PyInt_FromLong(PVbuffer[i]));
	}
	PyMem_DEL(PVbuffer);
	return v;
}

static PyObject *
doParams(args, func, modified)
	PyObject *args;
	void (*func)(long, long *, long);
	int modified;
{
	long device;
	PyObject *list, *v;
	long *PVbuffer;
	long length;
	int i;
	
	if (!PyArg_Parse(args, "(lO)", &device, &list))
		return NULL;
	if (!PyList_Check(list)) {
		PyErr_BadArgument();
		return NULL;
	}
	length = PyList_Size(list);
	PVbuffer = PyMem_NEW(long, length);
	if (PVbuffer == NULL)
		return PyErr_NoMemory();
	for (i = 0; i < length; i++) {
		v = PyList_GetItem(list, i);
		if (!PyInt_Check(v)) {
			PyMem_DEL(PVbuffer);
			PyErr_BadArgument();
			return NULL;
		}
		PVbuffer[i] = PyInt_AsLong(v);
	}

	(*func)(device, PVbuffer, length);

	if (modified) {
		for (i = 0; i < length; i++)
			PyList_SetItem(list, i, PyInt_FromLong(PVbuffer[i]));
	}

	PyMem_DEL(PVbuffer);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
al_getparams(self, args)
	PyObject *self, *args;
{
	return doParams(args, ALgetparams, 1);
}

static PyObject *
al_setparams(self, args)
	PyObject *self, *args;
{
	return doParams(args, ALsetparams, 0);
}

static PyObject *
al_getname(self, args)
	PyObject *self, *args;
{
	long device, descriptor;
	char *name;
	if (!PyArg_Parse(args, "(ll)", &device, &descriptor))
		return NULL;
	name = ALgetname(device, descriptor);
	if (name == NULL) {
		PyErr_SetString(PyExc_ValueError,
				"al.getname: bad descriptor");
		return NULL;
	}
	return PyString_FromString(name);
}

static PyObject *
al_getdefault(self, args)
	PyObject *self, *args;
{
	long device, descriptor, value;
	if (!PyArg_Parse(args, "(ll)", &device, &descriptor))
		return NULL;
	value = ALgetdefault(device, descriptor);
	return PyLong_FromLong(value);
}

static PyObject *
al_getminmax(self, args)
	PyObject *self, *args;
{
	long device, descriptor, min, max;
	if (!PyArg_Parse(args, "(ll)", &device, &descriptor))
		return NULL;
	min = -1;
	max = -1;
	ALgetminmax(device, descriptor, &min, &max);
	return Py_BuildValue("ll", min, max);
}

static PyMethodDef al_methods[] = {
	{"openport",		(PyCFunction)al_openport},
	{"newconfig",		(PyCFunction)al_newconfig},
	{"queryparams",		(PyCFunction)al_queryparams},
	{"getparams",		(PyCFunction)al_getparams},
	{"setparams",		(PyCFunction)al_setparams},
	{"getname",		(PyCFunction)al_getname},
	{"getdefault",		(PyCFunction)al_getdefault},
	{"getminmax",		(PyCFunction)al_getminmax},
	{NULL,			NULL}		/* sentinel */
};

void
inital()
{
	Py_InitModule("al", al_methods);
}

static int
getconfigarg(o, conf)
	PyObject *o;
	ALconfig *conf;
{
	if (o == NULL || !is_configobject(o))
		return PyErr_BadArgument ();
	
	*conf = ((configobject *) o) -> ob_config;
	
	return 1;
}

static int
getstrstrconfigarg(v, a, b, c)
	PyObject *v;
	char **a;
	char **b;
	ALconfig *c;
{
	PyObject *o;
	return PyArg_Parse(v, "(ssO)", a, b, &o) && getconfigarg(o, c);
}











