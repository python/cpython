/*
 
  Reference Cycle Garbage Collection
  ==================================

  Neil Schemenauer <nas@arctrix.com>

  Based on a post on the python-dev list.  Ideas from Guido van Rossum,
  Eric Tiedemann, and various others.

  http://www.arctrix.com/nas/python/gc.html
  http://www.python.org/pipermail/python-dev/2000-March/003869.html
  http://www.python.org/pipermail/python-dev/2000-March/004010.html
  http://www.python.org/pipermail/python-dev/2000-March/004022.html

  For a highlevel view of the collection process, read the collect
  function.

*/


#include "Python.h"

#ifdef WITH_CYCLE_GC

/* magic gc_refs value */
#define GC_MOVED -1

/*** Global GC state ***/

/* linked lists of container objects */
static PyGC_Head generation0 = {&generation0, &generation0, 0};
static PyGC_Head generation1 = {&generation1, &generation1, 0};
static PyGC_Head generation2 = {&generation2, &generation2, 0};
static int generation = 0; /* current generation being collected */

/* collection frequencies, XXX tune these */
static int enabled = 1; /* automatic collection enabled? */
static int threshold0 = 700; /* net new containers before collection */
static int threshold1 = 10;  /* generation0 collections before collecting 1 */
static int threshold2 = 10;  /* generation1 collections before collecting 2 */

/* net new objects allocated since last collection */
static int allocated;

/* set for debugging information */
#define DEBUG_STATS		(1<<0) /* print collection statistics */
#define DEBUG_COLLECTABLE	(1<<1) /* print collectable objects */
#define DEBUG_UNCOLLECTABLE	(1<<2) /* print uncollectable objects */
#define DEBUG_INSTANCES		(1<<3) /* print instances */
#define DEBUG_OBJECTS		(1<<4) /* print other objects */
#define DEBUG_SAVEALL		(1<<5) /* save all garbage in gc.garbage */
#define DEBUG_LEAK		DEBUG_COLLECTABLE | \
				DEBUG_UNCOLLECTABLE | \
				DEBUG_INSTANCES | \
				DEBUG_OBJECTS | \
				DEBUG_SAVEALL
static int debug;

/* list of uncollectable objects */
static PyObject *garbage;

/* Python string to use if unhandled exception occurs */
static PyObject *gc_str;

/*** list functions ***/

static void
gc_list_init(PyGC_Head *list)
{
	list->gc_prev = list;
	list->gc_next = list;
}

static void
gc_list_append(PyGC_Head *node, PyGC_Head *list)
{
	node->gc_next = list;
	node->gc_prev = list->gc_prev;
	node->gc_prev->gc_next = node;
	list->gc_prev = node;
}

static void
gc_list_remove(PyGC_Head *node)
{
	node->gc_prev->gc_next = node->gc_next;
	node->gc_next->gc_prev = node->gc_prev;
#ifdef Py_DEBUG
	node->gc_prev = NULL;
	node->gc_next = NULL;
#endif
}

static void 
gc_list_move(PyGC_Head *from, PyGC_Head *to)
{
	if (from->gc_next == from) {
		/* empty from list */
		gc_list_init(to);
	}
	else {
		to->gc_next = from->gc_next;
		to->gc_next->gc_prev = to;
		to->gc_prev = from->gc_prev;
		to->gc_prev->gc_next = to;
	}
	gc_list_init(from);
}

/* append a list onto another list, from becomes an empty list */
static void
gc_list_merge(PyGC_Head *from, PyGC_Head *to)
{
	PyGC_Head *tail;
	if (from->gc_next != from) {
		tail = to->gc_prev;
		tail->gc_next = from->gc_next;
		tail->gc_next->gc_prev = tail;
		to->gc_prev = from->gc_prev;
		to->gc_prev->gc_next = to;
	}
	gc_list_init(from);
}

static long
gc_list_size(PyGC_Head *list)
{
	PyGC_Head *gc;
	long n = 0;
	for (gc = list->gc_next; gc != list; gc = gc->gc_next) {
		n++;
	}
	return n;
}

/*** end of list stuff ***/


/* Set all gc_refs = ob_refcnt */
static void
update_refs(PyGC_Head *containers)
{
	PyGC_Head *gc = containers->gc_next;
	for (; gc != containers; gc=gc->gc_next) {
		gc->gc_refs = PyObject_FROM_GC(gc)->ob_refcnt;
	}
}

static int
visit_decref(PyObject *op, void *data)
{
	if (op && PyObject_IS_GC(op)) {
		PyObject_AS_GC(op)->gc_refs--;
	}
	return 0;
}

/* Subtract internal references from gc_refs */
static void
subtract_refs(PyGC_Head *containers)
{
	traverseproc traverse;
	PyGC_Head *gc = containers->gc_next;
	for (; gc != containers; gc=gc->gc_next) {
		traverse = PyObject_FROM_GC(gc)->ob_type->tp_traverse;
		(void) traverse(PyObject_FROM_GC(gc),
			       (visitproc)visit_decref,
			       NULL);
	}
}

/* Append objects with gc_refs > 0 to roots list */
static void
move_roots(PyGC_Head *containers, PyGC_Head *roots)
{
	PyGC_Head *next;
	PyGC_Head *gc = containers->gc_next;
	while (gc != containers) {
		next = gc->gc_next;
		if (gc->gc_refs > 0) {
			gc_list_remove(gc);
			gc_list_append(gc, roots);
			gc->gc_refs = GC_MOVED;
		}
		gc = next;
	}
}

static int
visit_reachable(PyObject *op, PyGC_Head *roots)
{
	if (PyObject_IS_GC(op)) {
		PyGC_Head *gc = PyObject_AS_GC(op);
		if (gc && gc->gc_refs != GC_MOVED) {
			gc_list_remove(gc);
			gc_list_append(gc, roots);
			gc->gc_refs = GC_MOVED;
		}
	}
	return 0;
}

/* Move objects referenced from reachable to reachable set. */
static void
move_root_reachable(PyGC_Head *reachable)
{
	traverseproc traverse;
	PyGC_Head *gc = reachable->gc_next;
	for (; gc != reachable; gc=gc->gc_next) {
		/* careful, reachable list is growing here */
		PyObject *op = PyObject_FROM_GC(gc);
		traverse = op->ob_type->tp_traverse;
		(void) traverse(op,
			       (visitproc)visit_reachable,
			       (void *)reachable);
	}
}

/* move all objects with finalizers (instances with __del__) */
static void
move_finalizers(PyGC_Head *unreachable, PyGC_Head *finalizers)
{
	PyGC_Head *next;
	PyGC_Head *gc = unreachable->gc_next;
	static PyObject *delstr = NULL;
	if (delstr == NULL) {
		delstr = PyString_InternFromString("__del__");
		if (delstr == NULL)
			Py_FatalError("PyGC: can't initialize __del__ string");
	}
	for (; gc != unreachable; gc=next) {
		PyObject *op = PyObject_FROM_GC(gc);
		next = gc->gc_next;
		if (PyInstance_Check(op) && PyObject_HasAttr(op, delstr)) {
			gc_list_remove(gc);
			gc_list_append(gc, finalizers);
		}
	}
}


/* called by tp_traverse */
static int
visit_finalizer_reachable(PyObject *op, PyGC_Head *finalizers)
{
	if (PyObject_IS_GC(op)) {
		PyGC_Head *gc = PyObject_AS_GC(op);
		if (gc && gc->gc_refs != GC_MOVED) {
			gc_list_remove(gc);
			gc_list_append(gc, finalizers);
			gc->gc_refs = GC_MOVED;
		}
	}
	return 0;
}

/* Move objects referenced from roots to roots */
static void
move_finalizer_reachable(PyGC_Head *finalizers)
{
	traverseproc traverse;
	PyGC_Head *gc = finalizers->gc_next;
	for (; gc != finalizers; gc=gc->gc_next) {
		/* careful, finalizers list is growing here */
		traverse = PyObject_FROM_GC(gc)->ob_type->tp_traverse;
		(void) traverse(PyObject_FROM_GC(gc), 
			       (visitproc)visit_finalizer_reachable,
			       (void *)finalizers);
	}
}

static void
debug_instance(char *msg, PyInstanceObject *inst)
{
	char *cname;
	/* be careful not to create new dictionaries */
	PyObject *classname = inst->in_class->cl_name;
	if (classname != NULL && PyString_Check(classname))
		cname = PyString_AsString(classname);
	else
		cname = "?";
	PySys_WriteStderr("gc: %.100s <%.100s instance at %p>\n",
			  msg, cname, inst);
}

static void
debug_cycle(char *msg, PyObject *op)
{
	if ((debug & DEBUG_INSTANCES) && PyInstance_Check(op)) {
		debug_instance(msg, (PyInstanceObject *)op);
	}
	else if (debug & DEBUG_OBJECTS) {
		PySys_WriteStderr("gc: %.100s <%.100s %p>\n",
				  msg, op->ob_type->tp_name, op);
	}
}

/* Handle uncollectable garbage (cycles with finalizers). */
static void
handle_finalizers(PyGC_Head *finalizers, PyGC_Head *old)
{
	PyGC_Head *gc;
	if (garbage == NULL) {
		garbage = PyList_New(0);
	}
	for (gc = finalizers->gc_next; gc != finalizers;
			gc = finalizers->gc_next) {
		PyObject *op = PyObject_FROM_GC(gc);
		if ((debug & DEBUG_SAVEALL) || PyInstance_Check(op)) {
			/* If SAVEALL is not set then just append
			 * instances to the list of garbage.  We assume
			 * that all objects in the finalizers list are
			 * reachable from instances. */
			PyList_Append(garbage, op);
		}
		/* object is now reachable again */ 
		gc_list_remove(gc);
		gc_list_append(gc, old);
	}
}

/* Break reference cycles by clearing the containers involved.	This is
 * tricky business as the lists can be changing and we don't know which
 * objects may be freed.  It is possible I screwed something up here. */
static void
delete_garbage(PyGC_Head *unreachable, PyGC_Head *old)
{
	inquiry clear;

	while (unreachable->gc_next != unreachable) {
		PyGC_Head *gc = unreachable->gc_next;
		PyObject *op = PyObject_FROM_GC(gc);
		if (debug & DEBUG_SAVEALL) {
			PyList_Append(garbage, op);
		}
		else {
			if ((clear = op->ob_type->tp_clear) != NULL) {
				Py_INCREF(op);
				clear((PyObject *)op);
				Py_DECREF(op);
			}
		}
		if (unreachable->gc_next == gc) {
			/* object is still alive, move it, it may die later */
			gc_list_remove(gc);
			gc_list_append(gc, old);
		}
	}
}

/* This is the main function.  Read this to understand how the
 * collection process works. */
static long
collect(PyGC_Head *young, PyGC_Head *old)
{
	long n = 0;
	long m = 0;
	PyGC_Head reachable;
	PyGC_Head unreachable;
	PyGC_Head finalizers;
	PyGC_Head *gc;

	if (debug & DEBUG_STATS) {
		PySys_WriteStderr(
			"gc: collecting generation %d...\n"
			"gc: objects in each generation: %ld %ld %ld\n",
			generation,
			gc_list_size(&generation0),
			gc_list_size(&generation1),
			gc_list_size(&generation2));
	}

	/* Using ob_refcnt and gc_refs, calculate which objects in the
	 * container set are reachable from outside the set (ie. have a
	 * refcount greater than 0 when all the references within the
	 * set are taken into account */
	update_refs(young);
	subtract_refs(young);

	/* Move everything reachable from outside the set into the
	 * reachable set (ie. gc_refs > 0).  Next, move everything
	 * reachable from objects in the reachable set. */
	gc_list_init(&reachable);
	move_roots(young, &reachable);
	move_root_reachable(&reachable);

	/* move unreachable objects to a temporary list, new objects can be
	 * allocated after this point */
	gc_list_init(&unreachable);
	gc_list_move(young, &unreachable);

	/* move reachable objects to next generation */
	gc_list_merge(&reachable, old);

	/* Move objects reachable from finalizers, we can't safely delete
	 * them.  Python programmers should take care not to create such
	 * things.  For Python finalizers means instance objects with
	 * __del__ methods. */
	gc_list_init(&finalizers);
	move_finalizers(&unreachable, &finalizers);
	move_finalizer_reachable(&finalizers);

	/* Collect statistics on collectable objects found and print
	 * debugging information. */
	for (gc = unreachable.gc_next; gc != &unreachable;
			gc = gc->gc_next) {
		m++;
		if (debug & DEBUG_COLLECTABLE) {
			debug_cycle("collectable", PyObject_FROM_GC(gc));
		}
	}
	/* call tp_clear on objects in the collectable set.  This will cause
	 * the reference cycles to be broken. It may also cause some objects in
	 * finalizers to be freed */
	delete_garbage(&unreachable, old);

	/* Collect statistics on uncollectable objects found and print
	 * debugging information. */
	for (gc = finalizers.gc_next; gc != &finalizers;
			gc = gc->gc_next) {
		n++;
		if (debug & DEBUG_UNCOLLECTABLE) {
			debug_cycle("uncollectable", PyObject_FROM_GC(gc));
		}
	}
	if (debug & DEBUG_STATS) {
		if (m == 0 && n == 0) {
			PySys_WriteStderr("gc: done.\n");
		}
		else {
			PySys_WriteStderr(
			    "gc: done, %ld unreachable, %ld uncollectable.\n",
			    n+m, n);
		}
	}

	/* Append instances in the uncollectable set to a Python
	 * reachable list of garbage.  The programmer has to deal with
	 * this if they insist on creating this type of structure. */
	handle_finalizers(&finalizers, old);

	if (PyErr_Occurred()) {
		if (gc_str == NULL) {
		    gc_str = PyString_FromString("garbage collection");
		}
		PyErr_WriteUnraisable(gc_str);
		Py_FatalError("unexpected exception during garbage collection");
	}
	allocated = 0;
	return n+m;
}

static long
collect_generations(void)
{
	static long collections0 = 0;
	static long collections1 = 0;
	long n = 0;


	if (collections1 > threshold2) {
		generation = 2;
		gc_list_merge(&generation0, &generation2);
		gc_list_merge(&generation1, &generation2);
		if (generation2.gc_next != &generation2) {
			n = collect(&generation2, &generation2);
		}
		collections1 = 0;
	}
	else if (collections0 > threshold1) {
		generation = 1;
		collections1++;
		gc_list_merge(&generation0, &generation1);
		if (generation1.gc_next != &generation1) {
			n = collect(&generation1, &generation2);
		}
		collections0 = 0;
	}
	else {
		generation = 0;
		collections0++;
		if (generation0.gc_next != &generation0) {
			n = collect(&generation0, &generation1);
		}
	}
	return n;
}

void
_PyGC_Insert(PyObject *op)
{
	/* collection lock since collecting may cause allocations */
	static int collecting = 0;

#ifdef Py_DEBUG
	if (!PyObject_IS_GC(op)) {
		abort();
	}
#endif
	if (allocated > threshold0 &&
	    enabled &&
	    threshold0 &&
	    !collecting &&
	    !PyErr_Occurred()) {
		collecting++;
		collect_generations();
		collecting--;
	}
	allocated++;
	gc_list_append(PyObject_AS_GC(op), &generation0);
}

void
_PyGC_Remove(PyObject *op)
{
	PyGC_Head *g = PyObject_AS_GC(op);
#ifdef Py_DEBUG
	if (!PyObject_IS_GC(op)) {
		abort();
	}
#endif
	gc_list_remove(g);
	if (allocated > 0) {
		allocated--;
	}
}

static char gc_enable__doc__[] =
"enable() -> None\n"
"\n"
"Enable automatic garbage collection.\n"
;

static PyObject *
gc_enable(PyObject *self, PyObject *args)
{

	if (!PyArg_ParseTuple(args, ":enable"))	/* check no args */
		return NULL;

	enabled = 1;

	Py_INCREF(Py_None);
	return Py_None;
}

static char gc_disable__doc__[] =
"disable() -> None\n"
"\n"
"Disable automatic garbage collection.\n"
;

static PyObject *
gc_disable(PyObject *self, PyObject *args)
{

	if (!PyArg_ParseTuple(args, ":disable"))	/* check no args */
		return NULL;

	enabled = 0;

	Py_INCREF(Py_None);
	return Py_None;
}

static char gc_isenabled__doc__[] =
"isenabled() -> status\n"
"\n"
"Returns true if automatic garbage collection is enabled.\n"
;

static PyObject *
gc_isenabled(PyObject *self, PyObject *args)
{

	if (!PyArg_ParseTuple(args, ":isenabled"))	/* check no args */
		return NULL;

	return Py_BuildValue("i", enabled);
}

static char gc_collect__doc__[] =
"collect() -> n\n"
"\n"
"Run a full collection.  The number of unreachable objects is returned.\n"
;

static PyObject *
gc_collect(PyObject *self, PyObject *args)
{
	long n;

	if (!PyArg_ParseTuple(args, ":collect"))	/* check no args */
		return NULL;

	generation = 2;
	gc_list_merge(&generation0, &generation2);
	gc_list_merge(&generation1, &generation2);
	n = collect(&generation2, &generation2);

	return Py_BuildValue("l", n);
}

static char gc_set_debug__doc__[] = 
"set_debug(flags) -> None\n"
"\n"
"Set the garbage collection debugging flags. Debugging information is\n"
"written to sys.stderr.\n"
"\n"
"flags is an integer and can have the following bits turned on:\n"
"\n"
"  DEBUG_STATS - Print statistics during collection.\n"
"  DEBUG_COLLECTABLE - Print collectable objects found.\n"
"  DEBUG_UNCOLLECTABLE - Print unreachable but uncollectable objects found.\n"
"  DEBUG_INSTANCES - Print instance objects.\n"
"  DEBUG_OBJECTS - Print objects other than instances.\n"
"  DEBUG_SAVEALL - Save objects to gc.garbage rather than freeing them.\n"
"  DEBUG_LEAK - Debug leaking programs (everything but STATS).\n"
;

static PyObject *
gc_set_debug(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, "i:set_debug", &debug))
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static char gc_get_debug__doc__[] = 
"get_debug() -> flags\n"
"\n"
"Get the garbage collection debugging flags.\n"
;

static PyObject *
gc_get_debug(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":get_debug"))	/* no args */
		return NULL;

	return Py_BuildValue("i", debug);
}

static char gc_set_thresh__doc__[] =
"set_threshold(threshold0, [threhold1, threshold2]) -> None\n"
"\n"
"Sets the collection thresholds.  Setting threshold0 to zero disables\n"
"collection.\n"
;

static PyObject *
gc_set_thresh(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, "i|ii:set_threshold", &threshold0, 
				&threshold1, &threshold2))
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static char gc_get_thresh__doc__[] =
"get_threshold() -> (threshold0, threshold1, threshold2)\n"
"\n"
"Return the current collection thresholds\n"
;

static PyObject *
gc_get_thresh(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":get_threshold"))	/* no args */
		return NULL;

	return Py_BuildValue("(iii)", threshold0, threshold1, threshold2);
}


static char gc__doc__ [] =
"This module provides access to the garbage collector for reference cycles.\n"
"\n"
"enable() -- Enable automatic garbage collection.\n"
"disable() -- Disable automatic garbage collection.\n"
"isenabled() -- Returns true if automatic collection is enabled.\n"
"collect() -- Do a full collection right now.\n"
"set_debug() -- Set debugging flags.\n"
"get_debug() -- Get debugging flags.\n"
"set_threshold() -- Set the collection thresholds.\n"
"get_threshold() -- Return the current the collection thresholds.\n"
;

static PyMethodDef GcMethods[] = {
	{"enable",	   gc_enable,	  METH_VARARGS, gc_enable__doc__},
	{"disable",	   gc_disable,	  METH_VARARGS, gc_disable__doc__},
	{"isenabled",	   gc_isenabled,  METH_VARARGS, gc_isenabled__doc__},
	{"set_debug",	   gc_set_debug,  METH_VARARGS, gc_set_debug__doc__},
	{"get_debug",	   gc_get_debug,  METH_VARARGS, gc_get_debug__doc__},
	{"set_threshold",  gc_set_thresh, METH_VARARGS, gc_set_thresh__doc__},
	{"get_threshold",  gc_get_thresh, METH_VARARGS, gc_get_thresh__doc__},
	{"collect",	   gc_collect,	  METH_VARARGS, gc_collect__doc__},
	{NULL,	NULL}		/* Sentinel */
};

void
initgc(void)
{
	PyObject *m;
	PyObject *d;

	m = Py_InitModule4("gc",
			      GcMethods,
			      gc__doc__,
			      NULL,
			      PYTHON_API_VERSION);
	d = PyModule_GetDict(m);
	if (garbage == NULL) {
		garbage = PyList_New(0);
	}
	PyDict_SetItemString(d, "garbage", garbage);
	PyDict_SetItemString(d, "DEBUG_STATS",
			PyInt_FromLong(DEBUG_STATS));
	PyDict_SetItemString(d, "DEBUG_COLLECTABLE",
			PyInt_FromLong(DEBUG_COLLECTABLE));
	PyDict_SetItemString(d, "DEBUG_UNCOLLECTABLE",
			PyInt_FromLong(DEBUG_UNCOLLECTABLE));
	PyDict_SetItemString(d, "DEBUG_INSTANCES",
			PyInt_FromLong(DEBUG_INSTANCES));
	PyDict_SetItemString(d, "DEBUG_OBJECTS",
			PyInt_FromLong(DEBUG_OBJECTS));
	PyDict_SetItemString(d, "DEBUG_SAVEALL",
			PyInt_FromLong(DEBUG_SAVEALL));
	PyDict_SetItemString(d, "DEBUG_LEAK",
			PyInt_FromLong(DEBUG_LEAK));
}

#endif /* WITH_CYCLE_GC */
