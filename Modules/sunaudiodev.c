
/* Sad objects */

#include "Python.h"
#include "structmember.h"

#ifdef HAVE_SYS_AUDIOIO_H
#define SOLARIS
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <stropts.h>
#include <sys/ioctl.h>
#ifdef SOLARIS
#include <sys/audioio.h>
#else
#include <sun/audioio.h>
#endif

/* #define offsetof(str,mem) ((int)(((str *)0)->mem)) */

typedef struct {
	PyObject_HEAD
	int	x_fd;		/* The open file */
	int	x_icount;	/* # samples read */
	int	x_ocount;	/* # samples written */
	int	x_isctl;	/* True if control device */
	
} sadobject;

typedef struct {
	PyObject_HEAD
	audio_info_t ai;
} sadstatusobject;

static PyTypeObject Sadtype;
static PyTypeObject Sadstatustype;
static sadstatusobject *sads_alloc(void);	/* Forward */

static PyObject *SunAudioError;

#define is_sadobject(v)		((v)->ob_type == &Sadtype)
#define is_sadstatusobject(v)	((v)->ob_type == &Sadstatustype)


static sadobject *
newsadobject(PyObject *args)
{
	sadobject *xp;
	int fd;
	char *mode;
	int imode;
	char* basedev;
	char* ctldev;
	char* opendev;

	/* Check arg for r/w/rw */
	if (!PyArg_ParseTuple(args, "s", &mode))
		return NULL;
	if (strcmp(mode, "r") == 0)
		imode = 0;
	else if (strcmp(mode, "w") == 0)
		imode = 1;
	else if (strcmp(mode, "rw") == 0)
		imode = 2;
	else if (strcmp(mode, "control") == 0)
		imode = -1;
	else {
		PyErr_SetString(SunAudioError,
			  "Mode should be one of 'r', 'w', 'rw' or 'control'");
		return NULL;
	}
	
	/* Open the correct device.  The base device name comes from the
	 * AUDIODEV environment variable first, then /dev/audio.  The
	 * control device tacks "ctl" onto the base device name.
	 */
	basedev = getenv("AUDIODEV");
	if (!basedev)
		basedev = "/dev/audio";
	ctldev = PyMem_NEW(char, strlen(basedev) + 4);
	if (!ctldev) {
		PyErr_NoMemory();
		return NULL;
	}
	strcpy(ctldev, basedev);
	strcat(ctldev, "ctl");

	if (imode < 0) {
		opendev = ctldev;
		fd = open(ctldev, 2);
	}
	else {
		opendev = basedev;
		fd = open(basedev, imode);
	}
	if (fd < 0) {
		PyErr_SetFromErrnoWithFilename(SunAudioError, opendev);
		PyMem_DEL(ctldev);
		return NULL;
	}
	PyMem_DEL(ctldev);

	/* Create and initialize the object */
	xp = PyObject_New(sadobject, &Sadtype);
	if (xp == NULL) {
		close(fd);
		return NULL;
	}
	xp->x_fd = fd;
	xp->x_icount = xp->x_ocount = 0;
	xp->x_isctl = (imode < 0);
	
	return xp;
}

/* Sad methods */

static void
sad_dealloc(sadobject *xp)
{
        close(xp->x_fd);
	PyObject_Del(xp);
}

static PyObject *
sad_read(sadobject *self, PyObject *args)
{
        int size, count;
	char *cp;
	PyObject *rv;
	
        if (!PyArg_ParseTuple(args, "i:read", &size))
		return NULL;
	rv = PyString_FromStringAndSize(NULL, size);
	if (rv == NULL)
		return NULL;

	if (!(cp = PyString_AsString(rv)))
		goto finally;

	count = read(self->x_fd, cp, size);
	if (count < 0) {
		PyErr_SetFromErrno(SunAudioError);
		goto finally;
	}
#if 0
	/* TBD: why print this message if you can handle the condition?
	 * assume it's debugging info which we can just as well get rid
	 * of.  in any case this message should *not* be using printf!
	 */
	if (count != size)
		printf("sunaudio: funny read rv %d wtd %d\n", count, size);
#endif
	self->x_icount += count;
	return rv;

  finally:
	Py_DECREF(rv);
	return NULL;
}

static PyObject *
sad_write(sadobject *self, PyObject *args)
{
        char *cp;
	int count, size;
	
        if (!PyArg_ParseTuple(args, "s#:write", &cp, &size))
		return NULL;

	count = write(self->x_fd, cp, size);
	if (count < 0) {
		PyErr_SetFromErrno(SunAudioError);
		return NULL;
	}
#if 0
	if (count != size)
		printf("sunaudio: funny write rv %d wanted %d\n", count, size);
#endif
	self->x_ocount += count;
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sad_getinfo(sadobject *self)
{
	sadstatusobject *rv;

	if (!(rv = sads_alloc()))
		return NULL;

	if (ioctl(self->x_fd, AUDIO_GETINFO, &rv->ai) < 0) {
		PyErr_SetFromErrno(SunAudioError);
		Py_DECREF(rv);
		return NULL;
	}
	return (PyObject *)rv;
}

static PyObject *
sad_setinfo(sadobject *self, sadstatusobject *arg)
{
	if (!is_sadstatusobject(arg)) {
		PyErr_SetString(PyExc_TypeError,
				"Must be sun audio status object");
		return NULL;
	}
	if (ioctl(self->x_fd, AUDIO_SETINFO, &arg->ai) < 0) {
		PyErr_SetFromErrno(SunAudioError);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sad_ibufcount(sadobject *self)
{
	audio_info_t ai;
    
	if (ioctl(self->x_fd, AUDIO_GETINFO, &ai) < 0) {
		PyErr_SetFromErrno(SunAudioError);
		return NULL;
	}
	return PyInt_FromLong(ai.record.samples - self->x_icount);
}

static PyObject *
sad_obufcount(sadobject *self)
{
	audio_info_t ai;
    
	if (ioctl(self->x_fd, AUDIO_GETINFO, &ai) < 0) {
		PyErr_SetFromErrno(SunAudioError);
		return NULL;
	}
	/* x_ocount is in bytes, whereas play.samples is in frames */
	/* we want frames */
	return PyInt_FromLong(self->x_ocount / (ai.play.channels *
						ai.play.precision / 8) -
			      ai.play.samples);
}

static PyObject *
sad_drain(sadobject *self)
{
	if (ioctl(self->x_fd, AUDIO_DRAIN, 0) < 0) {
		PyErr_SetFromErrno(SunAudioError);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

#ifdef SOLARIS
static PyObject *
sad_getdev(sadobject *self)
{
	struct audio_device ad;

	if (ioctl(self->x_fd, AUDIO_GETDEV, &ad) < 0) {
		PyErr_SetFromErrno(SunAudioError);
		return NULL;
	}
	return Py_BuildValue("(sss)", ad.name, ad.version, ad.config);
}
#endif

static PyObject *
sad_flush(sadobject *self)
{
	if (ioctl(self->x_fd, I_FLUSH, FLUSHW) < 0) {
		PyErr_SetFromErrno(SunAudioError);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sad_close(sadobject *self)
{
    
	if (self->x_fd >= 0) {
		close(self->x_fd);
		self->x_fd = -1;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sad_fileno(sadobject *self)
{
	return PyInt_FromLong(self->x_fd);
}


static PyMethodDef sad_methods[] = {
        { "read",	(PyCFunction)sad_read, METH_VARARGS },
        { "write",	(PyCFunction)sad_write, METH_VARARGS },
        { "ibufcount",	(PyCFunction)sad_ibufcount, METH_NOARGS },
        { "obufcount",	(PyCFunction)sad_obufcount, METH_NOARGS },
#define CTL_METHODS 4
        { "getinfo",	(PyCFunction)sad_getinfo, METH_NOARGS },
        { "setinfo",	(PyCFunction)sad_setinfo, METH_O},
        { "drain",	(PyCFunction)sad_drain, METH_NOARGS },
        { "flush",	(PyCFunction)sad_flush, METH_NOARGS },
#ifdef SOLARIS
	{ "getdev",	(PyCFunction)sad_getdev, METH_NOARGS },
#endif
        { "close",	(PyCFunction)sad_close, METH_NOARGS },
	{ "fileno",     (PyCFunction)sad_fileno, METH_NOARGS },
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
sad_getattr(sadobject *xp, char *name)
{
	if (xp->x_isctl)
		return Py_FindMethod(sad_methods+CTL_METHODS,
				     (PyObject *)xp, name);
	else
		return Py_FindMethod(sad_methods, (PyObject *)xp, name);
}

/* ----------------------------------------------------------------- */

static sadstatusobject *
sads_alloc(void) {
	return PyObject_New(sadstatusobject, &Sadstatustype);
}

static void
sads_dealloc(sadstatusobject *xp)
{
	PyMem_DEL(xp);
}

#define OFF(x) offsetof(audio_info_t,x)
static struct memberlist sads_ml[] = {
	{ "i_sample_rate",	T_UINT,		OFF(record.sample_rate) },
	{ "i_channels",		T_UINT,		OFF(record.channels) },
	{ "i_precision",	T_UINT,		OFF(record.precision) },
	{ "i_encoding",		T_UINT,		OFF(record.encoding) },
	{ "i_gain",		T_UINT,		OFF(record.gain) },
	{ "i_port",		T_UINT,		OFF(record.port) },
	{ "i_samples",		T_UINT,		OFF(record.samples) },
	{ "i_eof",		T_UINT,		OFF(record.eof) },
	{ "i_pause",		T_UBYTE,	OFF(record.pause) },
	{ "i_error",		T_UBYTE,	OFF(record.error) },
	{ "i_waiting",		T_UBYTE,	OFF(record.waiting) },
	{ "i_open",		T_UBYTE,	OFF(record.open) ,	 RO},
	{ "i_active",		T_UBYTE,	OFF(record.active) ,	 RO},
#ifdef SOLARIS
	{ "i_buffer_size",	T_UINT,		OFF(record.buffer_size) },
	{ "i_balance",		T_UBYTE,	OFF(record.balance) },
	{ "i_avail_ports",	T_UINT,		OFF(record.avail_ports) },
#endif

	{ "o_sample_rate",	T_UINT,		OFF(play.sample_rate) },
	{ "o_channels",		T_UINT,		OFF(play.channels) },
	{ "o_precision",	T_UINT,		OFF(play.precision) },
	{ "o_encoding",		T_UINT,		OFF(play.encoding) },
	{ "o_gain",		T_UINT,		OFF(play.gain) },
	{ "o_port",		T_UINT,		OFF(play.port) },
	{ "o_samples",		T_UINT,		OFF(play.samples) },
	{ "o_eof",		T_UINT,		OFF(play.eof) },
	{ "o_pause",		T_UBYTE,	OFF(play.pause) },
	{ "o_error",		T_UBYTE,	OFF(play.error) },
	{ "o_waiting",		T_UBYTE,	OFF(play.waiting) },
	{ "o_open",		T_UBYTE,	OFF(play.open) ,	 RO},
	{ "o_active",		T_UBYTE,	OFF(play.active) ,	 RO},
#ifdef SOLARIS
	{ "o_buffer_size",	T_UINT,		OFF(play.buffer_size) },
	{ "o_balance",		T_UBYTE,	OFF(play.balance) },
	{ "o_avail_ports",	T_UINT,		OFF(play.avail_ports) },
#endif

	{ "monitor_gain",	T_UINT,		OFF(monitor_gain) },
        { NULL,                 0,              0},
};

static PyObject *
sads_getattr(sadstatusobject *xp, char *name)
{
	return PyMember_Get((char *)&xp->ai, sads_ml, name);
}

static int
sads_setattr(sadstatusobject *xp, char *name, PyObject *v)
{

	if (v == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"can't delete sun audio status attributes");
		return -1;
	}
	return PyMember_Set((char *)&xp->ai, sads_ml, name, v);
}

/* ------------------------------------------------------------------- */


static PyTypeObject Sadtype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"sunaudiodev.sun_audio_device",	/*tp_name*/
	sizeof(sadobject),		/*tp_size*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)sad_dealloc,	/*tp_dealloc*/
	0,				/*tp_print*/
	(getattrfunc)sad_getattr,	/*tp_getattr*/
	0,				/*tp_setattr*/
	0,				/*tp_compare*/
	0,				/*tp_repr*/
};

static PyTypeObject Sadstatustype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"sunaudiodev.sun_audio_device_status", /*tp_name*/
	sizeof(sadstatusobject),	/*tp_size*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)sads_dealloc,	/*tp_dealloc*/
	0,				/*tp_print*/
	(getattrfunc)sads_getattr,	/*tp_getattr*/
	(setattrfunc)sads_setattr,	/*tp_setattr*/
	0,				/*tp_compare*/
	0,				/*tp_repr*/
};
/* ------------------------------------------------------------------- */

static PyObject *
sadopen(PyObject *self, PyObject *args)
{
	return (PyObject *)newsadobject(args);
}
    
static PyMethodDef sunaudiodev_methods[] = {
    { "open", sadopen, METH_VARARGS },
    { 0, 0 },
};

void
initsunaudiodev(void)
{
	PyObject *m, *d;

	m = Py_InitModule("sunaudiodev", sunaudiodev_methods);
	d = PyModule_GetDict(m);
	SunAudioError = PyErr_NewException("sunaudiodev.error", NULL, NULL);
	if (SunAudioError)
		PyDict_SetItemString(d, "error", SunAudioError);
}
