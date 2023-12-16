/** This is an implementation of mananged memory for Python objects.
 *
 * Managed memory is an opaque memory structure that can be accessed
 * by for a class.   To create managed memory the class must use a
 * negative basesize when declaring itself.  The memory will be added
 * to the object when it is alllocated.
 *
 * Multiple inheritance with opaque data is always pure virtual.
 * There is no guarantees about how the memory will be organized with
 * respect to the base address of the object.  The offset may change
 * whenever the type is derived.
 *
 * Acces to opaque memory in which there is no multiple inheritance is
 * guaranteed to be O(1).   Access with multiple inheritance will make
 * best effort for fast operations.
 *
 * Managed memory is in the same memory block as the object and
 * has the same lifetime.
 *
 * This implementation is currently using the tp_cache slot for
 * fast access.
 */
#include "Python.h"

#ifdef __cplusplus
extern "C" {
#endif

// For now we will be using the tp_cache spot for the layout object
#define Py_LAYOUT(type) (PyLayout*)((type)->tp_cache)

typedef struct _layout PyLayout;
typedef struct _layout_entry PyLayoutEntry;

/* function that takes an object and products a memory address */
typedef void *(*_layoutfunc)(PyLayout*, PyObject *, PyObject *);

struct _layout_entry {
	PyObject* le_type; /* this is a borrowed reference as it is used only for identification of the type */
	int32_t le_offset;    /* this should be negative value describing how far back to go. */
};


struct _layout {
	PyVarObject ob_base;
	Py_ssize_t ly_allocsize;  /* The size to pass to the allocator.  Must be the first entry. */
	Py_ssize_t ly_basesize; /* The size of the memory to be added to object for this particular layout */

	/* Slot the determines how to perform a cast. Other slots may be added to improve lookup capabilities. */
	_layoutfunc ly_cast;

	/* Variables used to implement fast lookups. */
	uint32_t ly_id; /* For hash lookups */
	uint32_t ly_order; /* for direct lookups */
	uint32_t ly_shift;  /* Used to determine the best hashing algorithm. */
	uint32_t ly_search; /* Number of hash elements the need to be searched before a miss can be declared. */
};

static PyTypeObject* PyLayoutType;


void*
PyObject_Cast(PyObject* obj, PyTypeObject* type)
{
	PyLayout* objlayout = Py_LAYOUT(Py_TYPE(obj));
	PyLayout* typelayout = Py_LAYOUT(type);

	/* Both the objects type and the type requested must have a layout to proceed. */
	if (objlayout == NULL || typelayout == NULL)
		return NULL;

	return  objlayout->ly_cast(objlayout, obj, (PyObject*) type);
}

/**
 * Find the offset when the object has simple inheritance.
 *
 * This is used whenever the object does not have conflicts in the inheritance.
 * Lookup is simple array lookup.
 */
static void*
_layout_cast_ordered(PyLayout* layout, PyObject* obj, PyObject* type)
{
	Py_ssize_t nentries = Py_SIZE(layout);
	PyLayoutEntry* entries = (PyLayoutEntry*) (&layout[1]);
	char* bytes = (char*) obj;

	PyLayout* typelayout = Py_LAYOUT((PyTypeObject*) type);
	uint32_t requested = typelayout->ly_order;

	/* If request size outside of defined entries, then fail. */
	if (requested >= nentries
	     || entries[requested].le_type != type)
		return NULL;

	/* Send the memory with offset */
	return &(bytes[entries[requested].le_offset]);
}

/**
 * Find the offset with multiple inheritance.
 */
static void*
_layout_cast_hash(PyLayout* layout, PyObject* obj, PyObject* type)
{
	Py_ssize_t nentries = Py_SIZE(layout);
	PyLayoutEntry* entries = (PyLayoutEntry*) (&layout[1]);
	char* bytes = (char*) obj;

	PyLayout* typelayout = Py_LAYOUT((PyTypeObject*) type);
	uint32_t requested = typelayout->ly_id;

	requested >>= layout->ly_shift;
	requested %= nentries;
	for (size_t i=0; i<layout->ly_search; ++i)
	{
		if (entries[requested].le_type == type)
			return &(bytes[entries[requested].le_offset]);
		requested++;
		if (requested == nentries)
			requested = 0;
	}
	/* Not found */
	return NULL;
}

/**
 * Count layouts
 *
 * Given a tuple holding all the base classes for an object count how many unique layouts it desends from.
 */
static PyObject*
_layout_collect(PyObject* tuple)
{
	/* FIXME validate that this code can never produce an exception. */
	PyObject* dict = PyDict_New();

	/* This is most certainly a fatal error. */
	if (dict == NULL)
		return NULL;

	int sz = PyTuple_Size(tuple);
	for (int i = 0; i<sz; ++i)
	{
		PyTypeObject* base = (PyTypeObject*) PyTuple_GetItem(tuple, i); /* borrowed */
		PyObject* mro = base->tp_mro;
		int sz2 = PyTuple_Size(mro);
		for (int j = 0; j<sz2; ++j)
		{
			PyTypeObject* type = (PyTypeObject*) PyTuple_GetItem(mro, j); /* borrowed */
			if (Py_LAYOUT(type) != NULL)
			{
				PyDict_SetItem(dict, (PyObject*) type, (PyObject*) Py_LAYOUT(type));
			}
		}

	}
	return dict;
}

/**
 * Check if a list of layouts has no repeats in the order field.
 *
 * returns 1 if ordered and 0 otherwise.
 */
static int
_layout_check_ordered(PyObject* layouts)
{
	Py_ssize_t items = PySequence_Size(layouts);
	PyObject *key, *value;
	Py_ssize_t pos = 0;
	char *buf = (char *) malloc(items);
	memset(buf, 0, items);
	int is_ordered = 1;
	while (PyDict_Next(layouts, &pos, &key, &value)) {
		PyLayout* layout = (PyLayout*) value;
		uint32_t order = layout->ly_order;
		if (order >= items || buf[order]>0)
		{
			is_ordered = 0;
			break;
		}
		buf[order]++;
	}
	free(buf);
	return is_ordered;
}

static int
_layout_fast_hash(PyObject* layouts, Py_ssize_t hash_size, uint32_t id)
{
	/* We want this to be approximately an O(1) operation as users will avoid
	 * using managed memory if it is considered slow. Thus we try to find a
	 * good hashtable strategy once when it is created.
	 */
	PyObject *key, *value;
	Py_ssize_t pos = 0;
	char *buf = (char *) malloc(hash_size);

	/* consider up to 16 different hashtable layouts. */
	int best = 0;
	int best_cost = hash_size;
	for (int shift = 0; shift<16; ++shift)
	{
		memset(buf, 0, hash_size);

		/* Place our entry first */
		uint32_t position = (id>> shift) % hash_size;
		buf[position] = 1;

		/* Place the rest */
		int worst = 0;
		while (PyDict_Next(layouts, &pos, &key, &value)) {
			PyLayout* layout = (PyLayout*) value;
			position = ((layout->ly_id)>> shift) % hash_size;
			int cost = 1;
			while (buf[position] >0)
			{
				cost++;
				position++;
				if (position == hash_size)
					position =0;
			}
			buf[position] = cost;
			if (cost>worst)
				cost++;
		}

		/* We found a perfect hash no need to continue. */
		if (worst == 1)
		{
			best = shift;
			break;
		}

		/* Else try again. */
		if (worst<best_cost)
		{
			best_cost = worst;
			best = shift;
		}
	}
	free(buf);
	return best;
}

static Py_ssize_t
_align_up(Py_ssize_t size)
{
	    return (size + ALIGNOF_MAX_ALIGN_T - 1) & ~(ALIGNOF_MAX_ALIGN_T - 1);
}

static int32_t
_layout_base(PyTypeObject* type)
{
	/* We are placing managed memory in front of the object so we need to be aware of any
	 * other memory that is previously located before the object.
	 *
	 * This includes
	 *   GC
	 *   MANAGED_DICT
	 *   MANAGED_WEAKREF
	 *
	 * It would be nice if we could make managed stuff go away as it could just be part of
	 * the managed memory space, but that requires that managed objects inherit from
	 * a baseclass of managed which would bust up the tree.  So we will just place our stuff
	 * in front of all of it.
	 */
	int32_t base = 0;
	if (PyType_HasFeature(type, Py_TPFLAGS_MANAGED_DICT))
		base += sizeof(void*);
	if (PyType_HasFeature(type, Py_TPFLAGS_HAVE_GC))
		base += sizeof(void*)*2;
	if (PyType_HasFeature(type, Py_TPFLAGS_MANAGED_WEAKREF))
		base += sizeof(void*);
	return base;
}


static void
_layout_fill_ordered(PyLayout* layout, PyTypeObject* type, PyObject* layouts)
{
	int32_t offset = 0;
	int32_t base = _layout_base(type);

	PyLayoutEntry* entries = (PyLayoutEntry*) &(layout[1]);

	/* Fill the entry table and compute the offsets for all required memory */
	PyObject *key, *value;
	Py_ssize_t pos = 0;
	while (PyDict_Next(layouts, &pos, &key, &value)) {
		PyLayout* sub_layout = (PyLayout*) value;
		uint32_t position = sub_layout->ly_order;
		offset += _align_up(sub_layout->ly_basesize);
		entries[position].le_type = key;
		entries[position].le_offset = -base - offset;
	}
	/* Store the resulting size in the layout */
	layout->ly_allocsize = offset;
}	

static void
_layout_fill_hash(PyLayout* layout, PyTypeObject* type, PyObject* layouts)
{
	Py_ssize_t hash_size = Py_SIZE(layout);
	uint32_t shift = layout->ly_shift;
	int32_t offset = 0;
	int32_t base = _layout_base(type);

	PyLayoutEntry* entries = (PyLayoutEntry*) &(layout[1]);

	/* Fill the entry table and compute the offsets for all required memory */
	PyObject *key, *value;
	Py_ssize_t pos = 0;
	while (PyDict_Next(layouts, &pos, &key, &value)) {
		PyLayout* sub_layout = (PyLayout*) value;

		/* Build the hash table */
	 	uint32_t position = ((sub_layout->ly_id)>> shift) % hash_size;
		uint32_t cost = 1;
		while (entries[position].le_type !=NULL)
		{
			cost++;
			position++;
			if (position==hash_size)
				position = 0;
		}
		if (layout->ly_search<cost)
			layout->ly_search = cost;

		offset += _align_up(sub_layout->ly_basesize);
		entries[position].le_type = key;
		entries[position].le_offset = -base-offset;
	}
	/* Store the resulting size in the layout */
	layout->ly_allocsize = offset;
}

static void
_layout_clear(PyLayout* layout)
{
	PyLayoutEntry* entries = (PyLayoutEntry*) &(layout[1]);
	Py_ssize_t size = Py_SIZE(layout);
	for (Py_ssize_t i=0; i<size; i++)
		entries[i].le_type = NULL;
}

/**
 * Construct a layout for a new type.
 *
 * This is called after the type is set up but before the first object is allocated.
 * Layouts are required for
 *  - classes that request managed memory
 *  - classes that inherit from classes with managed memory
 *
 * In principle we can share layouts between objects assuming that they don't add
 * a new feature like managed dist, weakref, or GC.  But for now we will assume
 * every type gets its own layout.
 *
 * returns a new layout or NULL if no layout is required.
 */
PyObject*
_PyLayout_Create(PyTypeObject* type, PyObject* bases, Py_ssize_t size)
{
	/* Collect the layouts that this object will inherit from. */
	PyObject* layouts = _layout_collect(bases);

	/* We don't yet know how may entries will be required for this object.
	 * To determine that we must first decide if this is an ordered layout
	 */
	uint32_t order = PySequence_Size(layouts) + size>0?1:0;

	/** We don't need a layout for this object. */
	if (order == 0)
		return NULL;

	uint32_t entries = order;
	uint32_t id = rand();
	PyLayout* result;

	if ( _layout_check_ordered(layouts))
	{
		result = (PyLayout*) PyObject_NewVar(PyLayout, PyLayoutType, entries);
		result->ly_cast = _layout_cast_ordered;
		result->ly_shift = 0;
	}
	else
	{
		entries *= 2;
		result = (PyLayout*) PyObject_NewVar(PyLayout, PyLayoutType, entries);
		result->ly_cast = _layout_cast_hash;
		result->ly_shift = _layout_fast_hash(layouts, entries, id);
	}

	/* Fill out fields */
	result->ly_id = id;
	result->ly_order = order;
	result->ly_basesize = size;
	result->ly_allocsize = 0; /* this will be created by the fill operation */
	result->ly_search = 1; /* this will be created by the fill operation */

	_layout_clear(result);
	PyObject_InitVar((PyVarObject*)result, (PyTypeObject*) PyLayoutType, entries);

	/* Add our new layout to the memory allocation. */
	if (size>0)
		PyDict_SetItem(layouts, (PyObject*) type, (PyObject*) result);

	/* Fill the entries */
	if (result->ly_cast == _layout_cast_ordered)
	{
		_layout_fill_ordered(result, type, layouts);
	}
	else
	{
		_layout_fill_hash(result, type, layouts);
	}

	/* Check to see if an ordered layout arrangement works */
	Py_DECREF(layouts);
	return (PyObject*) result;
}

static PyType_Slot _layout_slots[] = {
	{0}
};

/** Layouts are currently opaque objects */
static PyType_Spec _layout_spec = {
	"layout",
	sizeof(PyLayout),
	sizeof(PyLayoutEntry),
	Py_TPFLAGS_DEFAULT,
	_layout_slots
};

/**
 * This is called once at the appropriate time to allocate the layout support.
 */
void _PyLayout_Initialize(void)
{
	PyLayoutType = (PyTypeObject*) PyType_FromSpec(&_layout_spec);
}

#ifdef __cplusplus
}
#endif
