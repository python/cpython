/*

  Reference Cycle Garbage Collection
  ==================================

  Neil Schemenauer <nas@arctrix.com>

  Based on a post on the python-dev list.  Ideas from Guido van Rossum,
  Eric Tiedemann, and various others.

  http://www.arctrix.com/nas/python/gc/
  http://www.python.org/pipermail/python-dev/2000-March/003869.html
  http://www.python.org/pipermail/python-dev/2000-March/004010.html
  http://www.python.org/pipermail/python-dev/2000-March/004022.html

  For a highlevel view of the collection process, read the collect
  function.

*/

#include "Python.h"

#ifdef WITH_CYCLE_GC

/* Get an object's GC head */
#define AS_GC(o) ((PyGC_Head *)(o)-1)

/* Get the object given the GC head */
#define FROM_GC(g) ((PyObject *)(((PyGC_Head *)g)+1))

/* True if an object is tracked by the GC */
#define IS_TRACKED(o) ((AS_GC(o))->gc.gc_next != NULL)

/*** Global GC state ***/

struct gc_generation {
	PyGC_Head head;
	int threshold; /* collection threshold */
	int count; /* count of allocations or collections of younger
		      generations */
};

#define NUM_GENERATIONS 3
#define GEN_HEAD(n) (&generations[n].head)

/* linked lists of container objects */
static struct gc_generation generations[NUM_GENERATIONS] = {
	/* PyGC_Head,				threshold,	count */
	{{{GEN_HEAD(0), GEN_HEAD(0), 0}},	700,		0},
	{{{GEN_HEAD(1), GEN_HEAD(1), 0}},	10,		0},
	{{{GEN_HEAD(2), GEN_HEAD(2), 0}},	10,		0},
};

PyGC_Head *_PyGC_generation0 = GEN_HEAD(0);

static int enabled = 1; /* automatic collection enabled? */

/* true if we are currently running the collector */
static int collecting;

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

/* When a collection begins, gc_refs is set to ob_refcnt for, and only for,
 * the objects in the generation being collected, called the "young"
 * generation at that point.  As collection proceeds, the gc_refs members
 * of young objects are set to GC_REACHABLE when it becomes known that they're
 * uncollectable, and to GC_TENTATIVELY_UNREACHABLE when the evidence
 * suggests they are collectable (this can't be known for certain until all
 * of the young generation is scanned).
 */

/* Special gc_refs values. */
#define GC_UNTRACKED			_PyGC_REFS_UNTRACKED
#define GC_REACHABLE			_PyGC_REFS_REACHABLE
#define GC_TENTATIVELY_UNREACHABLE	_PyGC_REFS_TENTATIVELY_UNREACHABLE

#define IS_REACHABLE(o) ((AS_GC(o))->gc.gc_refs == GC_REACHABLE)
#define IS_TENTATIVELY_UNREACHABLE(o) ( \
	(AS_GC(o))->gc.gc_refs == GC_TENTATIVELY_UNREACHABLE)

/* list of uncollectable objects */
static PyObject *garbage;

/* Python string to use if unhandled exception occurs */
static PyObject *gc_str;

/*** list functions ***/

static void
gc_list_init(PyGC_Head *list)
{
	list->gc.gc_prev = list;
	list->gc.gc_next = list;
}

static int
gc_list_is_empty(PyGC_Head *list)
{
	return (list->gc.gc_next == list);
}

static void
gc_list_append(PyGC_Head *node, PyGC_Head *list)
{
	node->gc.gc_next = list;
	node->gc.gc_prev = list->gc.gc_prev;
	node->gc.gc_prev->gc.gc_next = node;
	list->gc.gc_prev = node;
}

static void
gc_list_remove(PyGC_Head *node)
{
	node->gc.gc_prev->gc.gc_next = node->gc.gc_next;
	node->gc.gc_next->gc.gc_prev = node->gc.gc_prev;
	node->gc.gc_next = NULL; /* object is not currently tracked */
}

static void
gc_list_move(PyGC_Head *from, PyGC_Head *to)
{
	if (gc_list_is_empty(from)) {
		gc_list_init(to);
	}
	else {
		to->gc.gc_next = from->gc.gc_next;
		to->gc.gc_next->gc.gc_prev = to;
		to->gc.gc_prev = from->gc.gc_prev;
		to->gc.gc_prev->gc.gc_next = to;
	}
	gc_list_init(from);
}

/* append a list onto another list, from becomes an empty list */
static void
gc_list_merge(PyGC_Head *from, PyGC_Head *to)
{
	PyGC_Head *tail;
	if (!gc_list_is_empty(from)) {
		tail = to->gc.gc_prev;
		tail->gc.gc_next = from->gc.gc_next;
		tail->gc.gc_next->gc.gc_prev = tail;
		to->gc.gc_prev = from->gc.gc_prev;
		to->gc.gc_prev->gc.gc_next = to;
	}
	gc_list_init(from);
}

static long
gc_list_size(PyGC_Head *list)
{
	PyGC_Head *gc;
	long n = 0;
	for (gc = list->gc.gc_next; gc != list; gc = gc->gc.gc_next) {
		n++;
	}
	return n;
}

/*** end of list stuff ***/


/* Set all gc_refs = ob_refcnt.  After this, gc_refs is > 0 for all objects
 * in containers, and is GC_REACHABLE for all tracked gc objects not in
 * containers.
 */
static void
update_refs(PyGC_Head *containers)
{
	PyGC_Head *gc = containers->gc.gc_next;
	for (; gc != containers; gc = gc->gc.gc_next) {
		assert(gc->gc.gc_refs == GC_REACHABLE);
		gc->gc.gc_refs = FROM_GC(gc)->ob_refcnt;
	}
}

/* A traversal callback for subtract_refs. */
static int
visit_decref(PyObject *op, void *data)
{
        assert(op != NULL);
	if (PyObject_IS_GC(op)) {
		PyGC_Head *gc = AS_GC(op);
		/* We're only interested in gc_refs for objects in the
		 * generation being collected, which can be recognized
		 * because only they have positive gc_refs.
		 */
		if (gc->gc.gc_refs > 0)
			gc->gc.gc_refs--;
	}
	return 0;
}

/* Subtract internal references from gc_refs.  After this, gc_refs is >= 0
 * for all objects in containers, and is GC_REACHABLE for all tracked gc
 * objects not in containers.  The ones with gc_refs > 0 are directly
 * reachable from outside containers, and so can't be collected.
 */
static void
subtract_refs(PyGC_Head *containers)
{
	traverseproc traverse;
	PyGC_Head *gc = containers->gc.gc_next;
	for (; gc != containers; gc=gc->gc.gc_next) {
		traverse = FROM_GC(gc)->ob_type->tp_traverse;
		(void) traverse(FROM_GC(gc),
			       (visitproc)visit_decref,
			       NULL);
	}
}

/* A traversal callback for move_unreachable. */
static int
visit_reachable(PyObject *op, PyGC_Head *reachable)
{
	if (PyObject_IS_GC(op)) {
		PyGC_Head *gc = AS_GC(op);
		const int gc_refs = gc->gc.gc_refs;

		if (gc_refs == 0) {
			/* This is in move_unreachable's 'young' list, but
			 * the traversal hasn't yet gotten to it.  All
			 * we need to do is tell move_unreachable that it's
			 * reachable.
			 */
			gc->gc.gc_refs = 1;
		}
		else if (gc_refs == GC_TENTATIVELY_UNREACHABLE) {
			/* This had gc_refs = 0 when move_unreachable got
			 * to it, but turns out it's reachable after all.
			 * Move it back to move_unreachable's 'young' list,
			 * and move_unreachable will eventually get to it
			 * again.
			 */
			gc_list_remove(gc);
			gc_list_append(gc, reachable);
			gc->gc.gc_refs = 1;
		}
		/* Else there's nothing to do.
		 * If gc_refs > 0, it must be in move_unreachable's 'young'
		 * list, and move_unreachable will eventually get to it.
		 * If gc_refs == GC_REACHABLE, it's either in some other
		 * generation so we don't care about it, or move_unreachable
		 * already deat with it.
		 * If gc_refs == GC_UNTRACKED, it must be ignored.
		 */
		 else {
		 	assert(gc_refs > 0
		 	       || gc_refs == GC_REACHABLE
		 	       || gc_refs == GC_UNTRACKED);
		 }
	}
	return 0;
}

/* Move the unreachable objects from young to unreachable.  After this,
 * all objects in young have gc_refs = GC_REACHABLE, and all objects in
 * unreachable have gc_refs = GC_TENTATIVELY_UNREACHABLE.  All tracked
 * gc objects not in young or unreachable still have gc_refs = GC_REACHABLE.
 * All objects in young after this are directly or indirectly reachable
 * from outside the original young; and all objects in unreachable are
 * not.
 */
static void
move_unreachable(PyGC_Head *young, PyGC_Head *unreachable)
{
	PyGC_Head *gc = young->gc.gc_next;

	/* Invariants:  all objects "to the left" of us in young have gc_refs
	 * = GC_REACHABLE, and are indeed reachable (directly or indirectly)
	 * from outside the young list as it was at entry.  All other objects
	 * from the original young "to the left" of us are in unreachable now,
	 * and have gc_refs = GC_TENTATIVELY_UNREACHABLE.  All objects to the
	 * left of us in 'young' now have been scanned, and no objects here
	 * or to the right have been scanned yet.
	 */

	while (gc != young) {
		PyGC_Head *next;

		if (gc->gc.gc_refs == 0) {
			/* This *may* be unreachable.  To make progress,
			 * assume it is.  gc isn't directly reachable from
			 * any object we've already traversed, but may be
			 * reachable from an object we haven't gotten to yet.
			 * visit_reachable will eventually move gc back into
			 * young if that's so, and we'll see it again.
			 */
			next = gc->gc.gc_next;
			gc_list_remove(gc);
			gc_list_append(gc, unreachable);
			gc->gc.gc_refs = GC_TENTATIVELY_UNREACHABLE;
		}
		else {
			/* gc is definitely reachable from outside the
			 * original 'young'.  Mark it as such, and traverse
			 * its pointers to find any other objects that may
			 * be directly reachable from it.  Note that the
			 * call to tp_traverse may append objects to young,
			 * so we have to wait until it returns to determine
			 * the next object to visit.
			 */
			PyObject *op = FROM_GC(gc);
			traverseproc traverse = op->ob_type->tp_traverse;
			gc->gc.gc_refs = GC_REACHABLE;
			(void) traverse(op,
			       		(visitproc)visit_reachable,
			    		(void *)young);
			next = gc->gc.gc_next;
		}
		gc = next;
	}
}

/* return true if object has a finalization method */
static int
has_finalizer(PyObject *op)
{
	static PyObject *delstr = NULL;
	if (delstr == NULL) {
		delstr = PyString_InternFromString("__del__");
		if (delstr == NULL)
			Py_FatalError("PyGC: can't initialize __del__ string");
	}
	return (PyInstance_Check(op) ||
	        PyType_HasFeature(op->ob_type, Py_TPFLAGS_HEAPTYPE))
	       && PyObject_HasAttr(op, delstr);
}

/* Move all objects with finalizers (instances with __del__) */
static void
move_finalizers(PyGC_Head *unreachable, PyGC_Head *finalizers)
{
	PyGC_Head *next;
	PyGC_Head *gc = unreachable->gc.gc_next;
	for (; gc != unreachable; gc=next) {
		PyObject *op = FROM_GC(gc);
		next = gc->gc.gc_next;
		if (has_finalizer(op)) {
			gc_list_remove(gc);
			gc_list_append(gc, finalizers);
			gc->gc.gc_refs = GC_REACHABLE;
		}
	}
}

/* A traversal callback for move_finalizer_reachable. */
static int
visit_move(PyObject *op, PyGC_Head *tolist)
{
	if (PyObject_IS_GC(op)) {
		if (IS_TENTATIVELY_UNREACHABLE(op)) {
			PyGC_Head *gc = AS_GC(op);
			gc_list_remove(gc);
			gc_list_append(gc, tolist);
			gc->gc.gc_refs = GC_REACHABLE;
		}
	}
	return 0;
}

/* Move objects that are reachable from finalizers, from the unreachable set
 * into the finalizers set.
 */
static void
move_finalizer_reachable(PyGC_Head *finalizers)
{
	traverseproc traverse;
	PyGC_Head *gc = finalizers->gc.gc_next;
	for (; gc != finalizers; gc=gc->gc.gc_next) {
		/* careful, finalizers list is growing here */
		traverse = FROM_GC(gc)->ob_type->tp_traverse;
		(void) traverse(FROM_GC(gc),
			       (visitproc)visit_move,
			       (void *)finalizers);
	}
}

static void
debug_instance(char *msg, PyInstanceObject *inst)
{
	char *cname;
	/* simple version of instance_repr */
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
	for (gc = finalizers->gc.gc_next; gc != finalizers;
			gc = finalizers->gc.gc_next) {
		PyObject *op = FROM_GC(gc);
		if ((debug & DEBUG_SAVEALL) || has_finalizer(op)) {
			/* If SAVEALL is not set then just append objects with
			 * finalizers to the list of garbage.  All objects in
			 * the finalizers list are reachable from those
			 * objects.
			 */
			PyList_Append(garbage, op);
		}
		/* object is now reachable again */
		assert(IS_REACHABLE(op));
		gc_list_remove(gc);
		gc_list_append(gc, old);
	}
}

/* Break reference cycles by clearing the containers involved.	This is
 * tricky business as the lists can be changing and we don't know which
 * objects may be freed.  It is possible I screwed something up here.
 */
static void
delete_garbage(PyGC_Head *unreachable, PyGC_Head *old)
{
	inquiry clear;

	while (!gc_list_is_empty(unreachable)) {
		PyGC_Head *gc = unreachable->gc.gc_next;
		PyObject *op = FROM_GC(gc);

		assert(IS_TENTATIVELY_UNREACHABLE(op));
		if (debug & DEBUG_SAVEALL) {
			PyList_Append(garbage, op);
		}
		else {
			if ((clear = op->ob_type->tp_clear) != NULL) {
				Py_INCREF(op);
				clear(op);
				Py_DECREF(op);
			}
		}
		if (unreachable->gc.gc_next == gc) {
			/* object is still alive, move it, it may die later */
			gc_list_remove(gc);
			gc_list_append(gc, old);
			gc->gc.gc_refs = GC_REACHABLE;
		}
	}
}

/* This is the main function.  Read this to understand how the
 * collection process works. */
static long
collect(int generation)
{
	int i;
	long m = 0;	/* # objects collected */
	long n = 0;	/* # unreachable objects that couldn't be collected */
	PyGC_Head *young; /* the generation we are examining */
	PyGC_Head *old; /* next older generation */
	PyGC_Head unreachable;
	PyGC_Head finalizers;
	PyGC_Head *gc;

	if (debug & DEBUG_STATS) {
		PySys_WriteStderr("gc: collecting generation %d...\n",
				  generation);
		PySys_WriteStderr("gc: objects in each generation:");
		for (i = 0; i < NUM_GENERATIONS; i++) {
			PySys_WriteStderr(" %ld", gc_list_size(GEN_HEAD(i)));
		}
		PySys_WriteStderr("\n");
	}

	/* update collection and allocation counters */
	if (generation+1 < NUM_GENERATIONS)
		generations[generation+1].count += 1;
	for (i = 0; i <= generation; i++)
		generations[i].count = 0;

	/* merge younger generations with one we are currently collecting */
	for (i = 0; i < generation; i++) {
		gc_list_merge(GEN_HEAD(i), GEN_HEAD(generation));
	}

	/* handy references */
	young = GEN_HEAD(generation);
	if (generation < NUM_GENERATIONS-1)
		old = GEN_HEAD(generation+1);
	else
		old = young;

	/* Using ob_refcnt and gc_refs, calculate which objects in the
	 * container set are reachable from outside the set (ie. have a
	 * refcount greater than 0 when all the references within the
	 * set are taken into account
	 */
	update_refs(young);
	subtract_refs(young);

	/* Leave everything reachable from outside young in young, and move
	 * everything else (in young) to unreachable.
	 * NOTE:  This used to move the reachable objects into a reachable
	 * set instead.  But most things usually turn out to be reachable,
	 * so it's more efficient to move the unreachable things.
	 */
	gc_list_init(&unreachable);
	move_unreachable(young, &unreachable);

	/* Move reachable objects to next generation. */
	if (young != old)
		gc_list_merge(young, old);

	/* All objects in unreachable are trash, but objects reachable from
	 * finalizers can't safely be deleted.  Python programmers should take
	 * care not to create such things.  For Python, finalizers means
	 * instance objects with __del__ methods.
	 */
	gc_list_init(&finalizers);
	move_finalizers(&unreachable, &finalizers);
	move_finalizer_reachable(&finalizers);

	/* Collect statistics on collectable objects found and print
	 * debugging information. */
	for (gc = unreachable.gc.gc_next; gc != &unreachable;
			gc = gc->gc.gc_next) {
		m++;
		if (debug & DEBUG_COLLECTABLE) {
			debug_cycle("collectable", FROM_GC(gc));
		}
	}
	/* Call tp_clear on objects in the collectable set.  This will cause
	 * the reference cycles to be broken. It may also cause some objects in
	 * finalizers to be freed */
	delete_garbage(&unreachable, old);

	/* Collect statistics on uncollectable objects found and print
	 * debugging information. */
	for (gc = finalizers.gc.gc_next; gc != &finalizers;
			gc = gc->gc.gc_next) {
		n++;
		if (debug & DEBUG_UNCOLLECTABLE) {
			debug_cycle("uncollectable", FROM_GC(gc));
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
	return n+m;
}

static long
collect_generations(void)
{
	int i;
	long n = 0;

	/* Find the oldest generation (higest numbered) where the count
	 * exceeds the threshold.  Objects in the that generation and
	 * generations younger than it will be collected. */
	for (i = NUM_GENERATIONS-1; i >= 0; i--) {
		if (generations[i].count > generations[i].threshold) {
			n = collect(i);
			break;
		}
	}
	return n;
}

PyDoc_STRVAR(gc_enable__doc__,
"enable() -> None\n"
"\n"
"Enable automatic garbage collection.\n");

static PyObject *
gc_enable(PyObject *self, PyObject *args)
{

	if (!PyArg_ParseTuple(args, ":enable"))	/* check no args */
		return NULL;

	enabled = 1;

	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(gc_disable__doc__,
"disable() -> None\n"
"\n"
"Disable automatic garbage collection.\n");

static PyObject *
gc_disable(PyObject *self, PyObject *args)
{

	if (!PyArg_ParseTuple(args, ":disable"))	/* check no args */
		return NULL;

	enabled = 0;

	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(gc_isenabled__doc__,
"isenabled() -> status\n"
"\n"
"Returns true if automatic garbage collection is enabled.\n");

static PyObject *
gc_isenabled(PyObject *self, PyObject *args)
{

	if (!PyArg_ParseTuple(args, ":isenabled"))	/* check no args */
		return NULL;

	return Py_BuildValue("i", enabled);
}

PyDoc_STRVAR(gc_collect__doc__,
"collect() -> n\n"
"\n"
"Run a full collection.  The number of unreachable objects is returned.\n");

static PyObject *
gc_collect(PyObject *self, PyObject *args)
{
	long n;

	if (!PyArg_ParseTuple(args, ":collect"))	/* check no args */
		return NULL;

	if (collecting) {
		n = 0; /* already collecting, don't do anything */
	}
	else {
		collecting = 1;
		n = collect(NUM_GENERATIONS - 1);
		collecting = 0;
	}

	return Py_BuildValue("l", n);
}

PyDoc_STRVAR(gc_set_debug__doc__,
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
"  DEBUG_LEAK - Debug leaking programs (everything but STATS).\n");

static PyObject *
gc_set_debug(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, "i:set_debug", &debug))
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(gc_get_debug__doc__,
"get_debug() -> flags\n"
"\n"
"Get the garbage collection debugging flags.\n");

static PyObject *
gc_get_debug(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":get_debug"))	/* no args */
		return NULL;

	return Py_BuildValue("i", debug);
}

PyDoc_STRVAR(gc_set_thresh__doc__,
"set_threshold(threshold0, [threshold1, threshold2]) -> None\n"
"\n"
"Sets the collection thresholds.  Setting threshold0 to zero disables\n"
"collection.\n");

static PyObject *
gc_set_thresh(PyObject *self, PyObject *args)
{
	int i;
	if (!PyArg_ParseTuple(args, "i|ii:set_threshold",
			      &generations[0].threshold,
			      &generations[1].threshold,
			      &generations[2].threshold))
		return NULL;
	for (i = 2; i < NUM_GENERATIONS; i++) {
 		/* generations higher than 2 get the same threshold */
		generations[i].threshold = generations[2].threshold;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(gc_get_thresh__doc__,
"get_threshold() -> (threshold0, threshold1, threshold2)\n"
"\n"
"Return the current collection thresholds\n");

static PyObject *
gc_get_thresh(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":get_threshold"))	/* no args */
		return NULL;

	return Py_BuildValue("(iii)",
			     generations[0].threshold,
			     generations[1].threshold,
			     generations[2].threshold);
}

static int
referrersvisit(PyObject* obj, PyObject *objs)
{
	int i;
	for (i = 0; i < PyTuple_GET_SIZE(objs); i++)
		if (PyTuple_GET_ITEM(objs, i) == obj)
			return 1;
	return 0;
}

static int
gc_referrers_for(PyObject *objs, PyGC_Head *list, PyObject *resultlist)
{
	PyGC_Head *gc;
	PyObject *obj;
	traverseproc traverse;
	for (gc = list->gc.gc_next; gc != list; gc = gc->gc.gc_next) {
		obj = FROM_GC(gc);
		traverse = obj->ob_type->tp_traverse;
		if (obj == objs || obj == resultlist)
			continue;
		if (traverse(obj, (visitproc)referrersvisit, objs)) {
			if (PyList_Append(resultlist, obj) < 0)
				return 0; /* error */
		}
	}
	return 1; /* no error */
}

PyDoc_STRVAR(gc_get_referrers__doc__,
"get_referrers(*objs) -> list\n\
Return the list of objects that directly refer to any of objs.");

static PyObject *
gc_get_referrers(PyObject *self, PyObject *args)
{
	int i;
	PyObject *result = PyList_New(0);
	for (i = 0; i < NUM_GENERATIONS; i++) {
		if (!(gc_referrers_for(args, GEN_HEAD(i), result))) {
			Py_DECREF(result);
			return NULL;
		}
	}
	return result;
}

PyDoc_STRVAR(gc_get_objects__doc__,
"get_objects() -> [...]\n"
"\n"
"Return a list of objects tracked by the collector (excluding the list\n"
"returned).\n");

/* appending objects in a GC list to a Python list */
static int
append_objects(PyObject *py_list, PyGC_Head *gc_list)
{
	PyGC_Head *gc;
	for (gc = gc_list->gc.gc_next; gc != gc_list; gc = gc->gc.gc_next) {
		PyObject *op = FROM_GC(gc);
		if (op != py_list) {
			if (PyList_Append(py_list, op)) {
				return -1; /* exception */
			}
		}
	}
	return 0;
}

static PyObject *
gc_get_objects(PyObject *self, PyObject *args)
{
	int i;
	PyObject* result;

	if (!PyArg_ParseTuple(args, ":get_objects")) /* check no args */
		return NULL;
	result = PyList_New(0);
	if (result == NULL) {
		return NULL;
	}
	for (i = 0; i < NUM_GENERATIONS; i++) {
		if (append_objects(result, GEN_HEAD(i))) {
			Py_DECREF(result);
			return NULL;
		}
	}
	return result;
}


PyDoc_STRVAR(gc__doc__,
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
"get_objects() -- Return a list of all objects tracked by the collector.\n"
"get_referrers() -- Return the list of objects that refer to an object.\n");

static PyMethodDef GcMethods[] = {
	{"enable",	   gc_enable,	  METH_VARARGS, gc_enable__doc__},
	{"disable",	   gc_disable,	  METH_VARARGS, gc_disable__doc__},
	{"isenabled",	   gc_isenabled,  METH_VARARGS, gc_isenabled__doc__},
	{"set_debug",	   gc_set_debug,  METH_VARARGS, gc_set_debug__doc__},
	{"get_debug",	   gc_get_debug,  METH_VARARGS, gc_get_debug__doc__},
	{"set_threshold",  gc_set_thresh, METH_VARARGS, gc_set_thresh__doc__},
	{"get_threshold",  gc_get_thresh, METH_VARARGS, gc_get_thresh__doc__},
	{"collect",	   gc_collect,	  METH_VARARGS, gc_collect__doc__},
	{"get_objects",    gc_get_objects,METH_VARARGS, gc_get_objects__doc__},
	{"get_referrers",  gc_get_referrers, METH_VARARGS,
		gc_get_referrers__doc__},
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

/* for debugging */
void _PyGC_Dump(PyGC_Head *g)
{
	_PyObject_Dump(FROM_GC(g));
}

#endif /* WITH_CYCLE_GC */

/* extension modules might be compiled with GC support so these
   functions must always be available */

#undef PyObject_GC_Track
#undef PyObject_GC_UnTrack
#undef PyObject_GC_Del
#undef _PyObject_GC_Malloc

void
PyObject_GC_Track(void *op)
{
	_PyObject_GC_TRACK(op);
}

/* for binary compatibility with 2.2 */
void
_PyObject_GC_Track(PyObject *op)
{
    PyObject_GC_Track(op);
}

void
PyObject_GC_UnTrack(void *op)
{
#ifdef WITH_CYCLE_GC
	if (IS_TRACKED(op))
		_PyObject_GC_UNTRACK(op);
#endif
}

/* for binary compatibility with 2.2 */
void
_PyObject_GC_UnTrack(PyObject *op)
{
    PyObject_GC_UnTrack(op);
}

PyObject *
_PyObject_GC_Malloc(size_t basicsize)
{
	PyObject *op;
#ifdef WITH_CYCLE_GC
	PyGC_Head *g = PyObject_MALLOC(sizeof(PyGC_Head) + basicsize);
	if (g == NULL)
		return PyErr_NoMemory();
	g->gc.gc_next = NULL;
	g->gc.gc_refs = GC_UNTRACKED;
	generations[0].count++; /* number of allocated GC objects */
 	if (generations[0].count > generations[0].threshold &&
 	    enabled &&
 	    generations[0].threshold &&
 	    !collecting &&
 	    !PyErr_Occurred()) {
		collecting = 1;
		collect_generations();
		collecting = 0;
	}
	op = FROM_GC(g);
#else
	op = PyObject_MALLOC(basicsize);
	if (op == NULL)
		return PyErr_NoMemory();

#endif
	return op;
}

PyObject *
_PyObject_GC_New(PyTypeObject *tp)
{
	PyObject *op = _PyObject_GC_Malloc(_PyObject_SIZE(tp));
	if (op != NULL)
		op = PyObject_INIT(op, tp);
	return op;
}

PyVarObject *
_PyObject_GC_NewVar(PyTypeObject *tp, int nitems)
{
	const size_t size = _PyObject_VAR_SIZE(tp, nitems);
	PyVarObject *op = (PyVarObject *) _PyObject_GC_Malloc(size);
	if (op != NULL)
		op = PyObject_INIT_VAR(op, tp, nitems);
	return op;
}

PyVarObject *
_PyObject_GC_Resize(PyVarObject *op, int nitems)
{
	const size_t basicsize = _PyObject_VAR_SIZE(op->ob_type, nitems);
#ifdef WITH_CYCLE_GC
	PyGC_Head *g = AS_GC(op);
	g = PyObject_REALLOC(g,  sizeof(PyGC_Head) + basicsize);
	if (g == NULL)
		return (PyVarObject *)PyErr_NoMemory();
	op = (PyVarObject *) FROM_GC(g);
#else
	op = PyObject_REALLOC(op, basicsize);
	if (op == NULL)
		return (PyVarObject *)PyErr_NoMemory();
#endif
	op->ob_size = nitems;
	return op;
}

void
PyObject_GC_Del(void *op)
{
#ifdef WITH_CYCLE_GC
	PyGC_Head *g = AS_GC(op);
	if (IS_TRACKED(op))
		gc_list_remove(g);
	if (generations[0].count > 0) {
		generations[0].count--;
	}
	PyObject_FREE(g);
#else
	PyObject_FREE(op);
#endif
}

/* for binary compatibility with 2.2 */
#undef _PyObject_GC_Del
void
_PyObject_GC_Del(PyObject *op)
{
    PyObject_GC_Del(op);
}
