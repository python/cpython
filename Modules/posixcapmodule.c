/* POSIXCAP module implementation */

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#ifdef MS_WINDOWS
   /* include <windows.h> early to avoid conflict with pycore_condvar.h:

        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>

      FSCTL_GET_REPARSE_POINT is not exported with WIN32_LEAN_AND_MEAN. */
#  include <windows.h>
#endif

#include "pythread.h"
#include "structmember.h"
#ifndef MS_WINDOWS
#  include "posixcapmodule.h"
#else
#  include "winreparse.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

PyDoc_STRVAR(posixcap__doc__,
"This module provides an interface to POSIX 1003.1e capabilities.  On
supported systems, capabilities partition the all powerful root or
administrator privilege into a set of distinct privileges.");

#ifdef HAVE_SYS_CAPABILITY_H
#ifdef SIZEOF_CAP_FLAG_T
#ifdef SIZEOF_CAP_VALUE_T
#ifdef SIZEOF_CAP_FLAG_VALUE_T
#include <sys/capability.h>
#endif /* defined SIZEOF_CAP_FLAG_VALUE_T */
#endif /* defined SIZEOF_CAP_VALUE_T */
#endif /* defined SIZEOF_CAP_FLAG_T */
#endif /* HAVE_SYS_CAPABILITY_H */

#ifdef HAVE_SYS_CAPABILITY_H
#ifdef SIZEOF_CAP_FLAG_T
#ifdef SIZEOF_CAP_VALUE_T
#ifdef SIZEOF_CAP_FLAG_VALUE_T

#if SIZEOF_CAP_FLAG_T == 1
#define CAP_FLAG_T_FORMAT "b"
#elif SIZEOF_CAP_FLAG_T == SIZEOF_SHORT
#define CAP_FLAG_T_FORMAT "h"
#elif SIZEOF_CAP_FLAG_T == SIZEOF_INT
#define CAP_FLAG_T_FORMAT "i"
#elif SIZEOF_CAP_FLAG_T == SIZEOF_LONG
#define CAP_FLAG_T_FORMAT "l"
#elif defined HAVE_LONG_LONG && SIZEOF_CAP_FLAG_T == SIZEOF_LONG_LONG
#define CAP_FLAG_T_FORMAT "L"
#else
#error "Unsupported sizeof(cap_flag_t)"
#endif

#if SIZEOF_CAP_VALUE_T == 1
#define CAP_VALUE_T_FORMAT "b"
#elif SIZEOF_CAP_VALUE_T == SIZEOF_SHORT
#define CAP_VALUE_T_FORMAT "h"
#elif SIZEOF_CAP_VALUE_T == SIZEOF_INT
#define CAP_VALUE_T_FORMAT "i"
#elif SIZEOF_CAP_VALUE_T == SIZEOF_LONG
#define CAP_VALUE_T_FORMAT "l"
#elif defined HAVE_LONG_LONG && SIZEOF_CAP_VALUE_T == SIZEOF_LONG_LONG
#define CAP_VALUE_T_FORMAT "L"
#else
#error "Unsupported sizeof(cap_value_t)"
#endif

#if SIZEOF_CAP_FLAG_VALUE_T == 1
#define CAP_FLAG_VALUE_T_FORMAT "b"
#elif SIZEOF_CAP_FLAG_VALUE_T == SIZEOF_SHORT
#define CAP_FLAG_VALUE_T_FORMAT "h"
#elif SIZEOF_CAP_FLAG_VALUE_T == SIZEOF_INT
#define CAP_FLAG_VALUE_T_FORMAT "i"
#elif SIZEOF_CAP_FLAG_VALUE_T == SIZEOF_LONG
#define CAP_FLAG_VALUE_T_FORMAT "l"
#elif defined HAVE_LONG_LONG && SIZEOF_CAP_FLAG_VALUE_T == SIZEOF_LONG_LONG
#define CAP_FLAG_VALUE_T_FORMAT "L"
#else
#error "Unsupported sizeof(cap_flag_value_t)"
#endif

typedef struct {
	PyObject_HEAD
	cap_t caps;
} CapabilityState;

static void
posix_CapabilityState_dealloc(CapabilityState *self)
{
	cap_free(self->caps);
	self->ob_type->tp_free((PyObject *)self);
}

/* forward declaraction */
static PyTypeObject CapabilityStateType;

PyDoc_STRVAR(posix_cap_clear__doc__,
"cap_clear() -> clear capability state in working storage\n\n\
Clears a capability state in working storage.");

static PyObject *
posix_cap_clear(PyObject *self, PyObject *args)
{
	CapabilityState *capso = (CapabilityState *)self;
	int res;

	res = cap_clear(capso->caps);
	if (res < 0)
		return posix_error();

	Py_INCREF(capso);
	return (PyObject *)capso;
}

PyDoc_STRVAR(posix_cap_copy_ext__doc__,
"cap_copy_ext() -> externalise representation of capability state\n\n\
Translates a capability state in working storage into an external\n\
representation.");

static PyObject *
posix_cap_copy_ext(PyObject *self, PyObject *args)
{
	CapabilityState *capso = (CapabilityState *)self;
	ssize_t size, copy_size;
	PyObject *extstring;

	size = cap_size(capso->caps);
	if (size < 1  ||  size > 102400) {
		PyErr_SetString(PyExc_OSError, "cap_copy_ext() invalid external size");
		return NULL;
	}

	extstring = PyString_FromStringAndSize(NULL, size);
	if (extstring) {
		copy_size = cap_copy_ext(PyString_AS_STRING(extstring), capso->caps, size);
		if (size != copy_size) {
			Py_DECREF(extstring);
			PyErr_SetString(PyExc_OSError, "cap_copy_ext() external size changed");
			return NULL;
		}
	}

	return extstring;
}

PyDoc_STRVAR(posix_cap_copy_int__doc__,
"cap_copy_int(ext) -> capability state in working storage\n\n\
Translates an external representation of a capability state into a capability\n\
state in working storage.");

static PyObject *
posix_cap_copy_int(PyObject *self, PyObject *args)
{
	CapabilityState *capso = (CapabilityState *)self;
	const char *extstring;
	int extstring_len;
	cap_t caps;

	if (!PyArg_ParseTuple(args, "s#:cap_copy_int", &extstring, &extstring_len))
		return NULL;

	caps = cap_copy_int(extstring); /* no length arg */
	if (!caps)
		return posix_error();

	cap_free(capso->caps);
	capso->caps = caps;

	Py_INCREF(capso);
	return (PyObject *)capso;
}

PyDoc_STRVAR(posix_cap_dup__doc__,
"cap_dup() -> duplicate capability state in working storage\n\n\
Duplicate a capability state in working storage.");

static PyObject *
posix_cap_dup(PyObject *self, PyObject *args)
{
	CapabilityState *capso = (CapabilityState *)self, *capso2;
	cap_t caps;

	caps = cap_dup(capso->caps);
	if (!caps)
		return posix_error();

	capso2 = PyObject_New(CapabilityState, &CapabilityStateType);
	if (!capso2) {
		cap_free(caps);
		return NULL;
	}

	capso2->caps = caps;

	return (PyObject *)capso2;
}

PyDoc_STRVAR(posix_cap_from_text__doc__,
"cap_from_text(text) -> capability state in working storage\n\n\
Translates an textual representation of a capability state into a capability\n\
state in working storage.");

static PyObject *
posix_cap_from_text(PyObject *self, PyObject *args)
{
	CapabilityState *capso = (CapabilityState *)self;
	const char *text;
	cap_t caps;

	if (!PyArg_ParseTuple(args, "s:cap_from_text", &text))
		return NULL;

	caps = cap_from_text(text);
	if (!caps)
		return posix_error();

	cap_free (capso->caps);
	capso->caps = caps;

	Py_INCREF(capso);
	return (PyObject *)capso;
}

PyDoc_STRVAR(posix_cap_get_flag__doc__,
"cap_get_flag(cap, flag) -> get capability state flag\n\n\
Get a flag value of a capability state in working storage.");

static PyObject *
posix_cap_get_flag(PyObject *self, PyObject *args)
{
	CapabilityState *capso = (CapabilityState *)self;
	cap_value_t cap;
	cap_flag_t flag;
	cap_flag_value_t value;
	int res;

	if (!PyArg_ParseTuple(args,
			      CAP_VALUE_T_FORMAT CAP_FLAG_T_FORMAT ":cap_get_flag",
			      &cap, &flag))
		return NULL;

	res = cap_get_flag(capso->caps, cap, flag, &value);
	if (res < 0)
		return posix_error();

	if (value != CAP_SET  &&  value != CAP_CLEAR) {
		PyErr_SetString(PyExc_OSError, "cap_get_flag() invalid flag value");
		return NULL;
	}

	if (value == CAP_SET) {
		Py_INCREF(Py_True);
		return Py_True;
	} else if (value == CAP_CLEAR) {
		Py_INCREF(Py_False);
		return Py_False;
	} else {
		PyErr_SetString(PyExc_OSError, "cap_get_flag() invalid flag value");
		return NULL;
	}
}

PyDoc_STRVAR(posix_cap_get_proc__doc__,
"cap_get_proc() -> capability state in working storage\n\n\
Return the capability state of the calling process in working storage.");

static PyObject *
posix_cap_get_proc(PyObject *self, PyObject *args)
{
	CapabilityState *capso = (CapabilityState *)self;
	cap_t caps;

	caps = cap_get_proc();
	if (!caps)
		return posix_error();

	cap_free(capso->caps);
	capso->caps = caps;

	Py_INCREF(capso);
	return (PyObject *)capso;
}

PyDoc_STRVAR(posix_cap_init__doc__,
"cap_init() -> create a capability state in working storage\n\n\
Creates a capability state in working storage.  The initial value of\n\
all flags are cleared.");

static PyObject *
posix_cap_init(PyObject *self, PyObject *args)
{
	cap_t caps;
	CapabilityState *capso;

	caps = cap_init();
	if (!caps)
		return posix_error();

	capso = PyObject_New(CapabilityState, &CapabilityStateType);
	if (!capso) {
		cap_free(caps);
		return NULL;
	}

	capso->caps = caps;

	return (PyObject *)capso;
}

PyDoc_STRVAR(posix_cap_set_flag__doc__,
"cap_set_flag(cap, flag, value) -> set capability state flag\n\n\
Set a flag value of a capability state in working storage.");

static PyObject *
posix_cap_set_flag(PyObject *self, PyObject *args)
{
	CapabilityState *capso = (CapabilityState *)self;
	cap_value_t cap;
	cap_flag_t flag;
	int value_int = 1;
	cap_flag_value_t value;
	int res;

	if (!PyArg_ParseTuple(args,
			      CAP_VALUE_T_FORMAT CAP_FLAG_T_FORMAT "|i:cap_set_flag",
			      &cap, &flag, &value_int))
		return NULL;

	if (value_int)
		value = CAP_SET;
	else
		value = CAP_CLEAR;

	res = cap_set_flag(capso->caps, flag, 1, &cap, value);
	if (res < 0)
		return posix_error();

	Py_INCREF(capso);
	return (PyObject *)capso;
}

PyDoc_STRVAR(posix_cap_set_proc__doc__,
"cap_set_proc()\n\n\
Sets the capability state of the calling process.");

static PyObject *
posix_cap_set_proc(PyObject *self, PyObject *args)
{
	CapabilityState *capso = (CapabilityState *)self;
	int res;

	res = cap_set_proc(capso->caps);
	if (res < 0)
		return posix_error();

	Py_INCREF(capso);
	return (PyObject *)capso;
}

PyDoc_STRVAR(posix_cap_size__doc__,
"cap_size() -> size of external representation of capability state\n\n\
Returns the size of the translation of a capability state in working\n\
storage into an external representation.");

static PyObject *
posix_cap_size(PyObject *self, PyObject *args)
{
	CapabilityState *capso = (CapabilityState *)self;
	ssize_t size;

	size = cap_size(capso->caps);
	if (size < 1  ||  size > 102400) {
		PyErr_SetString(PyExc_OSError, "cap_size() invalid external size");
		return NULL;
	}

	return PyInt_FromLong(size);
}
+
PyDoc_STRVAR(posix_cap_to_text__doc__,
"cap_to_text() -> textual representation of capability state\n\n\
Translates a capability state in working storage into a textual\n\
representation.");

static PyObject *
posix_cap_to_text(PyObject *self, PyObject *args)
{
	CapabilityState *capso = (CapabilityState *)self;
	ssize_t size;
	char *data;
	PyObject *text;

	data = cap_to_text(capso->caps, &size);
	if (!data)
		return posix_error();
	if (size < 1) {
		cap_free(data);
		PyErr_SetString(PyExc_OSError, "cap_to_text() invalid textual representation");
		return NULL;
	}

	text = PyString_FromStringAndSize(data, size);

	cap_free(data);

	return text;
}

static PyMethodDef
CapabilityStateMethods[] = {
	{"cap_clear", (PyCFunction)posix_cap_clear, METH_NOARGS, posix_cap_clear__doc__},
	{"cap_copy_ext", (PyCFunction)posix_cap_copy_ext, METH_NOARGS, posix_cap_copy_ext__doc__},
	{"cap_copy_int", (PyCFunction)posix_cap_copy_int, METH_VARARGS, posix_cap_copy_int__doc__},
	{"cap_dup", (PyCFunction)posix_cap_dup, METH_NOARGS, posix_cap_dup__doc__},
	{"cap_from_text", (PyCFunction)posix_cap_from_text, METH_VARARGS, posix_cap_from_text__doc__},
	{"cap_get_flag", (PyCFunction)posix_cap_get_flag, METH_VARARGS, posix_cap_get_flag__doc__},
	{"cap_get_proc", (PyCFunction)posix_cap_get_proc, METH_NOARGS, posix_cap_get_proc__doc__},
	{"cap_set_flag", (PyCFunction)posix_cap_set_flag, METH_VARARGS, posix_cap_set_flag__doc__},
	{"cap_set_proc", (PyCFunction)posix_cap_set_proc, METH_NOARGS, posix_cap_set_proc__doc__},
	{"cap_size", (PyCFunction)posix_cap_size, METH_NOARGS, posix_cap_size__doc__},
	{"cap_to_text", (PyCFunction)posix_cap_to_text, METH_NOARGS, posix_cap_to_text__doc__},
	{NULL, NULL}	/* sentinel */
};

PyDoc_STRVAR(CapabilityStateType__doc__,
"CapabilityState() -> POSIX.1e capability state object\n\
\n\
Create a new capability state object. May be manipulated using that\n\
standard capability manipulation functions.");

static PyModuleDef posixcapmodule = {
	PyModuleDef_HEAD_INIT,
	0,
	"posix.CapabilityState", /*tp_name*/
	sizeof(CapabilityState), /*tp_basicsize*/
	0,			/*tp_itemsize*/
	(destructor)posix_CapabilityState_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,			/*tp_call*/
	0,			/*tp_str*/
	PyObject_GenericGetAttr, /*tp_getattro*/
	0,			/*tp_setattro*/
	0,			/*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,	/*tp_flags*/
	CapabilityStateType__doc__, /*tp_doc*/
	0,			/*tp_traverse*/
	0,			/*tp_clear*/
	0,			/*tp_richcompare*/
	0,			/*tp_weaklistoffset*/
	0,			/*tp_iter*/
	0,			/*tp_iternext*/
	CapabilityStateMethods,	/*tp_methods*/
	0,			/*tp_members*/
	0,			/*tp_getset*/
	0,			/*tp_base*/
	0,			/*tp_dict*/
	0,			/*tp_descr_get*/
	0,			/*tp_descr_set*/
	0,			/*tp_dictoffset*/
	0,			/*tp_init*/
	PyType_GenericAlloc,	/*tp_alloc*/
	PyType_GenericNew,	/*tp_new*/
	_PyObject_Del,		/*tp_free*/
	0,			/*tp_is_gc*/
};

PyMODINIT_FUNC
PyInit__posixcap(void)
{
    PyObject *m = PyModule_Create(&posixcapmodule);
    if (!m) return NULL;
        return m;
}

#endif /* defined SIZEOF_CAP_FLAG_VALUE_T */
#endif /* defined SIZEOF_CAP_VALUE_T */
#endif /* defined SIZEOF_CAP_FLAG_T */
#endif /* HAVE_SYS_CAPABILITY_H */


#if defined(HAVE_FSTATVFS) && defined(HAVE_SYS_STATVFS_H)
#ifdef _SCO_DS
#ifdef HAVE_SYS_CAPABILITY_H
#ifdef SIZEOF_CAP_FLAG_T
#ifdef SIZEOF_CAP_VALUE_T
#ifdef SIZEOF_CAP_FLAG_VALUE_T
	{"cap_init", posix_cap_init, METH_NOARGS, posix_cap_init__doc__},
#endif /* defined SIZEOF_CAP_FLAG_VALUE_T */
#endif /* defined SIZEOF_CAP_VALUE_T */
#endif /* defined SIZEOF_CAP_FLAG_T */
#endif /* HAVE_SYS_CAPABILITY_H */
 	{"chdir",	posix_chdir, METH_VARARGS, posix_chdir__doc__},
 #ifdef HAVE_CHFLAGS
 	{"chflags",	posix_chflags, METH_VARARGS, posix_chflags__doc__},
 #ifdef WUNTRACED
         if (ins(d, "WUNTRACED", (long)WUNTRACED)) return -1;
 #endif
#ifdef HAVE_SYS_CAPABILITY_H
#ifdef SIZEOF_CAP_FLAG_T
#ifdef SIZEOF_CAP_VALUE_T
#ifdef SIZEOF_CAP_FLAG_VALUE_T
	if (ins(d, "CAP_EFFECTIVE", CAP_EFFECTIVE)) return -1;
	if (ins(d, "CAP_PERMITTED", CAP_PERMITTED)) return -1;
	if (ins(d, "CAP_INHERITABLE", CAP_INHERITABLE)) return -1;
	Py_INCREF(Py_False);
	if (PyModule_AddObject(d, "CAP_CLEAR", Py_False)) return -1;
	Py_INCREF(Py_True);
	if (PyModule_AddObject(d, "CAP_SET", Py_True)) return -1;
#ifdef CAP_CHOWN
	if (ins(d, "CAP_CHOWN", CAP_CHOWN)) return -1;
#endif
#ifdef CAP_DAC_EXECUTE
	if (ins(d, "CAP_DAC_EXECUTE", CAP_DAC_EXECUTE)) return -1;
#endif
#ifdef CAP_DAC_WRITE
	if (ins(d, "CAP_DAC_WRITE", CAP_DAC_WRITE)) return -1;
#endif
#ifdef CAP_DAC_OVERRIDE
	if (ins(d, "CAP_DAC_OVERRIDE", CAP_DAC_OVERRIDE)) return -1;
#endif
#ifdef CAP_DAC_READ_SEARCH
	if (ins(d, "CAP_DAC_READ_SEARCH", CAP_DAC_READ_SEARCH)) return -1;
#endif
#ifdef CAP_FOWNER
	if (ins(d, "CAP_FOWNER", CAP_FOWNER)) return -1;
#endif
#ifdef CAP_FSETID
	if (ins(d, "CAP_FSETID", CAP_FSETID)) return -1;
#endif
#ifdef CAP_FS_MASK
	if (ins(d, "CAP_FS_MASK", CAP_FS_MASK)) return -1;
#endif
#ifdef CAP_KILL
	if (ins(d, "CAP_KILL", CAP_KILL)) return -1;
#endif
#ifdef CAP_LINK_DIR
	if (ins(d, "CAP_LINK_DIR", CAP_LINK_DIR)) return -1;
#endif
#ifdef CAP_SETFCAP
	if (ins(d, "CAP_SETFCAP", CAP_SETFCAP)) return -1;
#endif
#ifdef CAP_SETGID
	if (ins(d, "CAP_SETGID", CAP_SETGID)) return -1;
#endif
#ifdef CAP_SETUID
	if (ins(d, "CAP_SETUID", CAP_SETUID)) return -1;
#endif
#ifdef CAP_AUDIT_CONTROL
	if (ins(d, "CAP_AUDIT_CONTROL", CAP_AUDIT_CONTROL)) return -1;
#endif
#ifdef CAP_AUDIT_WRITE
	if (ins(d, "CAP_AUDIT_WRITE", CAP_AUDIT_WRITE)) return -1;
#endif
#ifdef CAP_SETPCAP
	if (ins(d, "CAP_SETPCAP", CAP_SETPCAP)) return -1;
#endif
#ifdef CAP_LINUX_IMMUTABLE
	if (ins(d, "CAP_LINUX_IMMUTABLE", CAP_LINUX_IMMUTABLE)) return -1;
#endif
#ifdef CAP_NET_BIND_SERVICE
	if (ins(d, "CAP_NET_BIND_SERVICE", CAP_NET_BIND_SERVICE)) return -1;
#endif
#ifdef CAP_NET_BROADCAST
	if (ins(d, "CAP_NET_BROADCAST", CAP_NET_BROADCAST)) return -1;
#endif
#ifdef CAP_NET_ADMIN
	if (ins(d, "CAP_NET_ADMIN", CAP_NET_ADMIN)) return -1;
#endif
#ifdef CAP_NET_RAW
	if (ins(d, "CAP_NET_RAW", CAP_NET_RAW)) return -1;
#endif
#ifdef CAP_IPC_LOCK
	if (ins(d, "CAP_IPC_LOCK", CAP_IPC_LOCK)) return -1;
#endif
#ifdef CAP_IPC_OWNER
	if (ins(d, "CAP_IPC_OWNER", CAP_IPC_OWNER)) return -1;
#endif
#ifdef CAP_SYS_MODULE
	if (ins(d, "CAP_SYS_MODULE", CAP_SYS_MODULE)) return -1;
#endif
#ifdef CAP_SYS_RAWIO
	if (ins(d, "CAP_SYS_RAWIO", CAP_SYS_RAWIO)) return -1;
#endif
#ifdef CAP_SYS_CHROOT
	if (ins(d, "CAP_SYS_CHROOT", CAP_SYS_CHROOT)) return -1;
#endif
#ifdef CAP_SYS_PTRACE
	if (ins(d, "CAP_SYS_PTRACE", CAP_SYS_PTRACE)) return -1;
#endif
#ifdef CAP_SYS_PACCT
	if (ins(d, "CAP_SYS_PACCT", CAP_SYS_PACCT)) return -1;
#endif
#ifdef CAP_SYS_ADMIN
	if (ins(d, "CAP_SYS_ADMIN", CAP_SYS_ADMIN)) return -1;
#endif
#ifdef CAP_SYS_BOOT
	if (ins(d, "CAP_SYS_BOOT", CAP_SYS_BOOT)) return -1;
#endif
#ifdef CAP_SYS_NICE
	if (ins(d, "CAP_SYS_NICE", CAP_SYS_NICE)) return -1;
#endif
#ifdef CAP_SYS_RESOURCE
	if (ins(d, "CAP_SYS_RESOURCE", CAP_SYS_RESOURCE)) return -1;
#endif
#ifdef CAP_SYS_TIME
	if (ins(d, "CAP_SYS_TIME", CAP_SYS_TIME)) return -1;
#endif
#ifdef CAP_SYS_TTY_CONFIG
	if (ins(d, "CAP_SYS_TTY_CONFIG", CAP_SYS_TTY_CONFIG)) return -1;
#endif
#ifdef CAP_MKNOD
	if (ins(d, "CAP_MKNOD", CAP_MKNOD)) return -1;
#endif
#ifdef CAP_LEASE
	if (ins(d, "CAP_LEASE", CAP_LEASE)) return -1;
#endif
#endif /* defined SIZEOF_CAP_FLAG_VALUE_T */
#endif /* defined SIZEOF_CAP_VALUE_T */
#endif /* defined SIZEOF_CAP_FLAG_T */
#endif /* HAVE_SYS_CAPABILITY_H */
#ifdef O_RDONLY
         if (ins(d, "O_RDONLY", (long)O_RDONLY)) return -1;
#endif



#ifdef __cplusplus
}
#endif
