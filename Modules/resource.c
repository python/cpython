
#include "Python.h"
#include "structseq.h"
#include <sys/resource.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
/* for sysconf */
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

/* On some systems, these aren't in any header file.
   On others they are, with inconsistent prototypes.
   We declare the (default) return type, to shut up gcc -Wall;
   but we can't declare the prototype, to avoid errors
   when the header files declare it different.
   Worse, on some Linuxes, getpagesize() returns a size_t... */

#define doubletime(TV) ((double)(TV).tv_sec + (TV).tv_usec * 0.000001)

static PyObject *ResourceError;

PyDoc_STRVAR(struct_rusage__doc__,
"struct_rusage: Result from getrusage.\n\n"
"This object may be accessed either as a tuple of\n"
"    (utime,stime,maxrss,ixrss,idrss,isrss,minflt,majflt,\n"
"    nswap,inblock,oublock,msgsnd,msgrcv,nsignals,nvcsw,nivcsw)\n"
"or via the attributes ru_utime, ru_stime, ru_maxrss, and so on.");

static PyStructSequence_Field struct_rusage_fields[] = {
	{"ru_utime",	"user time used"},
	{"ru_stime",	"system time used"},
	{"ru_maxrss",	"max. resident set size"},
	{"ru_ixrss",	"shared memory size"},
	{"ru_idrss",	"unshared data size"},
	{"ru_isrss",	"unshared stack size"},
	{"ru_minflt",	"page faults not requiring I/O"},
	{"ru_majflt",	"page faults requiring I/O"},
	{"ru_nswap",	"number of swap outs"},
	{"ru_inblock",	"block input operations"},
	{"ru_oublock",	"block output operations"},
	{"ru_msgsnd",	"IPC messages sent"},
	{"ru_msgrcv",	"IPC messages received"},
	{"ru_nsignals",	"signals received"},
	{"ru_nvcsw",	"voluntary context switches"},
	{"ru_nivcsw",	"involuntary context switches"},
	{0}
};

static PyStructSequence_Desc struct_rusage_desc = {
	"resource.struct_rusage",	/* name */
	struct_rusage__doc__,	/* doc */
	struct_rusage_fields,	/* fields */
	16	/* n_in_sequence */
};

static PyTypeObject StructRUsageType;

static PyObject *
resource_getrusage(PyObject *self, PyObject *args)
{
	int who;
	struct rusage ru;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "i:getrusage", &who))
		return NULL;

	if (getrusage(who, &ru) == -1) {
		if (errno == EINVAL) {
			PyErr_SetString(PyExc_ValueError,
					"invalid who parameter");
			return NULL;
		} 
		PyErr_SetFromErrno(ResourceError);
		return NULL;
	}

	result = PyStructSequence_New(&StructRUsageType);
	if (!result)
		return NULL;

	PyStructSequence_SET_ITEM(result, 0,
			PyFloat_FromDouble(doubletime(ru.ru_utime)));
	PyStructSequence_SET_ITEM(result, 1,
			PyFloat_FromDouble(doubletime(ru.ru_stime)));
	PyStructSequence_SET_ITEM(result, 2, PyInt_FromLong(ru.ru_maxrss));
	PyStructSequence_SET_ITEM(result, 3, PyInt_FromLong(ru.ru_ixrss));
	PyStructSequence_SET_ITEM(result, 4, PyInt_FromLong(ru.ru_idrss));
	PyStructSequence_SET_ITEM(result, 5, PyInt_FromLong(ru.ru_isrss));
	PyStructSequence_SET_ITEM(result, 6, PyInt_FromLong(ru.ru_minflt));
	PyStructSequence_SET_ITEM(result, 7, PyInt_FromLong(ru.ru_majflt));
	PyStructSequence_SET_ITEM(result, 8, PyInt_FromLong(ru.ru_nswap));
	PyStructSequence_SET_ITEM(result, 9, PyInt_FromLong(ru.ru_inblock));
	PyStructSequence_SET_ITEM(result, 10, PyInt_FromLong(ru.ru_oublock));
	PyStructSequence_SET_ITEM(result, 11, PyInt_FromLong(ru.ru_msgsnd));
	PyStructSequence_SET_ITEM(result, 12, PyInt_FromLong(ru.ru_msgrcv));
	PyStructSequence_SET_ITEM(result, 13, PyInt_FromLong(ru.ru_nsignals));
	PyStructSequence_SET_ITEM(result, 14, PyInt_FromLong(ru.ru_nvcsw));
	PyStructSequence_SET_ITEM(result, 15, PyInt_FromLong(ru.ru_nivcsw));

	if (PyErr_Occurred()) {
		Py_DECREF(result);
		return NULL;
	}

	return result;
}


static PyObject *
resource_getrlimit(PyObject *self, PyObject *args)
{
	struct rlimit rl;
	int resource;

	if (!PyArg_ParseTuple(args, "i:getrlimit", &resource)) 
		return NULL;

	if (resource < 0 || resource >= RLIM_NLIMITS) {
		PyErr_SetString(PyExc_ValueError,
				"invalid resource specified");
		return NULL;
	}

	if (getrlimit(resource, &rl) == -1) {
		PyErr_SetFromErrno(ResourceError);
		return NULL;
	}

#if defined(HAVE_LONG_LONG)
	if (sizeof(rl.rlim_cur) > sizeof(long)) {
		return Py_BuildValue("LL",
				     (PY_LONG_LONG) rl.rlim_cur,
				     (PY_LONG_LONG) rl.rlim_max);
	}
#endif
	return Py_BuildValue("ll", (long) rl.rlim_cur, (long) rl.rlim_max);
}

static PyObject *
resource_setrlimit(PyObject *self, PyObject *args)
{
	struct rlimit rl;
	int resource;
	PyObject *curobj, *maxobj;

	if (!PyArg_ParseTuple(args, "i(OO):setrlimit", 
			      &resource, &curobj, &maxobj))
		return NULL;

	if (resource < 0 || resource >= RLIM_NLIMITS) {
		PyErr_SetString(PyExc_ValueError,
				"invalid resource specified");
		return NULL;
	}

#if !defined(HAVE_LARGEFILE_SUPPORT)
	rl.rlim_cur = PyInt_AsLong(curobj);
	if (rl.rlim_cur == -1 && PyErr_Occurred())
	    return NULL;
	rl.rlim_max = PyInt_AsLong(maxobj);
	if (rl.rlim_max == -1 && PyErr_Occurred())
	    return NULL;
#else
	/* The limits are probably bigger than a long */
	rl.rlim_cur = PyLong_Check(curobj) ?
		PyLong_AsLongLong(curobj) : PyInt_AsLong(curobj);
	if (rl.rlim_cur == -1 && PyErr_Occurred())
	    return NULL;
	rl.rlim_max = PyLong_Check(maxobj) ?
		PyLong_AsLongLong(maxobj) : PyInt_AsLong(maxobj);
	if (rl.rlim_max == -1 && PyErr_Occurred())
	    return NULL;
#endif

	rl.rlim_cur = rl.rlim_cur & RLIM_INFINITY;
	rl.rlim_max = rl.rlim_max & RLIM_INFINITY;
	if (setrlimit(resource, &rl) == -1) {
		if (errno == EINVAL) 
			PyErr_SetString(PyExc_ValueError,
					"current limit exceeds maximum limit");
		else if (errno == EPERM)
			PyErr_SetString(PyExc_ValueError,
					"not allowed to raise maximum limit");
		else
			PyErr_SetFromErrno(ResourceError);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
resource_getpagesize(PyObject *self, PyObject *args)
{
	long pagesize = 0;
	if (!PyArg_ParseTuple(args, ":getpagesize"))
		return NULL;

#if defined(HAVE_GETPAGESIZE)
	pagesize = getpagesize();
#elif defined(HAVE_SYSCONF)
#if defined(_SC_PAGE_SIZE)
	pagesize = sysconf(_SC_PAGE_SIZE);
#else
	/* Irix 5.3 has _SC_PAGESIZE, but not _SC_PAGE_SIZE */
	pagesize = sysconf(_SC_PAGESIZE);
#endif
#endif
	return Py_BuildValue("i", pagesize);

}

/* List of functions */

static struct PyMethodDef
resource_methods[] = {
	{"getrusage",    resource_getrusage,   METH_VARARGS},
	{"getrlimit",    resource_getrlimit,   METH_VARARGS},
	{"setrlimit",    resource_setrlimit,   METH_VARARGS},
	{"getpagesize",  resource_getpagesize, METH_VARARGS},
	{NULL, NULL}			     /* sentinel */
};


/* Module initialization */

PyMODINIT_FUNC
initresource(void)
{
	PyObject *m, *v;

	/* Create the module and add the functions */
	m = Py_InitModule("resource", resource_methods);

	/* Add some symbolic constants to the module */
	if (ResourceError == NULL) {
		ResourceError = PyErr_NewException("resource.error",
						   NULL, NULL);
	}
	Py_INCREF(ResourceError);
	PyModule_AddObject(m, "error", ResourceError);
 	PyStructSequence_InitType(&StructRUsageType, &struct_rusage_desc);
 	PyModule_AddObject(m, "struct_rusage", 
			   (PyObject*) &StructRUsageType);

	/* insert constants */
#ifdef RLIMIT_CPU
	PyModule_AddIntConstant(m, "RLIMIT_CPU", RLIMIT_CPU);
#endif

#ifdef RLIMIT_FSIZE
	PyModule_AddIntConstant(m, "RLIMIT_FSIZE", RLIMIT_FSIZE);
#endif

#ifdef RLIMIT_DATA
	PyModule_AddIntConstant(m, "RLIMIT_DATA", RLIMIT_DATA);
#endif

#ifdef RLIMIT_STACK
	PyModule_AddIntConstant(m, "RLIMIT_STACK", RLIMIT_STACK);
#endif

#ifdef RLIMIT_CORE
	PyModule_AddIntConstant(m, "RLIMIT_CORE", RLIMIT_CORE);
#endif

#ifdef RLIMIT_NOFILE
	PyModule_AddIntConstant(m, "RLIMIT_NOFILE", RLIMIT_NOFILE);
#endif

#ifdef RLIMIT_OFILE
	PyModule_AddIntConstant(m, "RLIMIT_OFILE", RLIMIT_OFILE);
#endif

#ifdef RLIMIT_VMEM
	PyModule_AddIntConstant(m, "RLIMIT_VMEM", RLIMIT_VMEM);
#endif

#ifdef RLIMIT_AS
	PyModule_AddIntConstant(m, "RLIMIT_AS", RLIMIT_AS);
#endif

#ifdef RLIMIT_RSS
	PyModule_AddIntConstant(m, "RLIMIT_RSS", RLIMIT_RSS);
#endif

#ifdef RLIMIT_NPROC
	PyModule_AddIntConstant(m, "RLIMIT_NPROC", RLIMIT_NPROC);
#endif

#ifdef RLIMIT_MEMLOCK
	PyModule_AddIntConstant(m, "RLIMIT_MEMLOCK", RLIMIT_MEMLOCK);
#endif

#ifdef RUSAGE_SELF
	PyModule_AddIntConstant(m, "RUSAGE_SELF", RUSAGE_SELF);
#endif

#ifdef RUSAGE_CHILDREN
	PyModule_AddIntConstant(m, "RUSAGE_CHILDREN", RUSAGE_CHILDREN);
#endif

#ifdef RUSAGE_BOTH
	PyModule_AddIntConstant(m, "RUSAGE_BOTH", RUSAGE_BOTH);
#endif

#if defined(HAVE_LONG_LONG)
	if (sizeof(RLIM_INFINITY) > sizeof(long)) {
		v = PyLong_FromLongLong((PY_LONG_LONG) RLIM_INFINITY);
	} else 
#endif
	{
		v = PyInt_FromLong((long) RLIM_INFINITY);
	}
	if (v) {
		PyModule_AddObject(m, "RLIM_INFINITY", v);
	}
}
