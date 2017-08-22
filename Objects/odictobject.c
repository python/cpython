/* Ordered Dictionary object implementation.

Challenges from Subclassing dict
================================

OrderedDict subclasses dict, which is an unusual relationship between two
builtin types (other than the base object type).  Doing so results in
some complication and deserves further explanation.  There are two things
to consider here.  First, in what circumstances or with what adjustments
can OrderedDict be used as a drop-in replacement for dict (at the C level)?
Second, how can the OrderedDict implementation leverage the dict
implementation effectively without introducing unnecessary coupling or
inefficiencies?

This second point is reflected here and in the implementation, so the
further focus is on the first point.  It is worth noting that for
overridden methods, the dict implementation is deferred to as much as
possible.  Furthermore, coupling is limited to as little as is reasonable.

Concrete API Compatibility
--------------------------

Use of the concrete C-API for dict (PyDict_*) with OrderedDict is
problematic.  (See http://bugs.python.org/issue10977.)  The concrete API
has a number of hard-coded assumptions tied to the dict implementation.
This is, in part, due to performance reasons, which is understandable
given the part dict plays in Python.

Any attempt to replace dict with OrderedDict for any role in the
interpreter (e.g. **kwds) faces a challenge.  Such any effort must
recognize that the instances in affected locations currently interact with
the concrete API.

Here are some ways to address this challenge:

1. Change the relevant usage of the concrete API in CPython and add
   PyDict_CheckExact() calls to each of the concrete API functions.
2. Adjust the relevant concrete API functions to explicitly accommodate
   OrderedDict.
3. As with #1, add the checks, but improve the abstract API with smart fast
   paths for dict and OrderedDict, and refactor CPython to use the abstract
   API.  Improvements to the abstract API would be valuable regardless.

Adding the checks to the concrete API would help make any interpreter
switch to OrderedDict less painful for extension modules.  However, this
won't work.  The equivalent C API call to `dict.__setitem__(obj, k, v)`
is 'PyDict_SetItem(obj, k, v)`.  This illustrates how subclasses in C call
the base class's methods, since there is no equivalent of super() in the
C API.  Calling into Python for parent class API would work, but some
extension modules already rely on this feature of the concrete API.

For reference, here is a breakdown of some of the dict concrete API:

========================== ============= =======================
concrete API               uses          abstract API
========================== ============= =======================
PyDict_Check                             PyMapping_Check
(PyDict_CheckExact)                      -
(PyDict_New)                             -
(PyDictProxy_New)                        -
PyDict_Clear                             -
PyDict_Contains                          PySequence_Contains
PyDict_Copy                              -
PyDict_SetItem                           PyObject_SetItem
PyDict_SetItemString                     PyMapping_SetItemString
PyDict_DelItem                           PyMapping_DelItem
PyDict_DelItemString                     PyMapping_DelItemString
PyDict_GetItem                           -
PyDict_GetItemWithError                  PyObject_GetItem
_PyDict_GetItemIdWithError               -
PyDict_GetItemString                     PyMapping_GetItemString
PyDict_Items                             PyMapping_Items
PyDict_Keys                              PyMapping_Keys
PyDict_Values                            PyMapping_Values
PyDict_Size                              PyMapping_Size
                                         PyMapping_Length
PyDict_Next                              PyIter_Next
_PyDict_Next                             -
PyDict_Merge                             -
PyDict_Update                            -
PyDict_MergeFromSeq2                     -
PyDict_ClearFreeList                     -
-                                        PyMapping_HasKeyString
-                                        PyMapping_HasKey
========================== ============= =======================


The dict Interface Relative to OrderedDict
==========================================

Since OrderedDict subclasses dict, understanding the various methods and
attributes of dict is important for implementing OrderedDict.

Relevant Type Slots
-------------------

================= ================ =================== ================
slot              attribute        object              dict
================= ================ =================== ================
tp_dealloc        -                object_dealloc      dict_dealloc
tp_repr           __repr__         object_repr         dict_repr
sq_contains       __contains__     -                   dict_contains
mp_length         __len__          -                   dict_length
mp_subscript      __getitem__      -                   dict_subscript
mp_ass_subscript  __setitem__      -                   dict_ass_sub
                  __delitem__
tp_hash           __hash__         _Py_HashPointer     ..._HashNotImpl
tp_str            __str__          object_str          -
tp_getattro       __getattribute__ ..._GenericGetAttr  (repeated)
                  __getattr__
tp_setattro       __setattr__      ..._GenericSetAttr  (disabled)
tp_doc            __doc__          (literal)           dictionary_doc
tp_traverse       -                -                   dict_traverse
tp_clear          -                -                   dict_tp_clear
tp_richcompare    __eq__           object_richcompare  dict_richcompare
                  __ne__
tp_weaklistoffset (__weakref__)    -                   -
tp_iter           __iter__         -                   dict_iter
tp_dictoffset     (__dict__)       -                   -
tp_init           __init__         object_init         dict_init
tp_alloc          -                PyType_GenericAlloc (repeated)
tp_new            __new__          object_new          dict_new
tp_free           -                PyObject_Del        PyObject_GC_Del
================= ================ =================== ================

Relevant Methods
----------------

================ =================== ===============
method           object              dict
================ =================== ===============
__reduce__       object_reduce       -
__sizeof__       object_sizeof       dict_sizeof
clear            -                   dict_clear
copy             -                   dict_copy
fromkeys         -                   dict_fromkeys
get              -                   dict_get
items            -                   dictitems_new
keys             -                   dictkeys_new
pop              -                   dict_pop
popitem          -                   dict_popitem
setdefault       -                   dict_setdefault
update           -                   dict_update
values           -                   dictvalues_new
================ =================== ===============


Pure Python OrderedDict
=======================

As already noted, compatibility with the pure Python OrderedDict
implementation is a key goal of this C implementation.  To further that
goal, here's a summary of how OrderedDict-specific methods are implemented
in collections/__init__.py.  Also provided is an indication of which
methods directly mutate or iterate the object, as well as any relationship
with the underlying linked-list.

============= ============== == ================ === === ====
method        impl used      ll uses             inq mut iter
============= ============== == ================ === === ====
__contains__  dict           -  -                X
__delitem__   OrderedDict    Y  dict.__delitem__     X
__eq__        OrderedDict    N  OrderedDict      ~
                                dict.__eq__
                                __iter__
__getitem__   dict           -  -                X
__iter__      OrderedDict    Y  -                        X
__init__      OrderedDict    N  update
__len__       dict           -  -                X
__ne__        MutableMapping -  __eq__           ~
__reduce__    OrderedDict    N  OrderedDict      ~
                                __iter__
                                __getitem__
__repr__      OrderedDict    N  __class__        ~
                                items
__reversed__  OrderedDict    Y  -                        X
__setitem__   OrderedDict    Y  __contains__         X
                                dict.__setitem__
__sizeof__    OrderedDict    Y  __len__          ~
                                __dict__
clear         OrderedDict    Y  dict.clear           X
copy          OrderedDict    N  __class__
                                __init__
fromkeys      OrderedDict    N  __setitem__
get           dict           -  -                ~
items         MutableMapping -  ItemsView                X
keys          MutableMapping -  KeysView                 X
move_to_end   OrderedDict    Y  -                    X
pop           OrderedDict    N  __contains__         X
                                __getitem__
                                __delitem__
popitem       OrderedDict    Y  dict.pop             X
setdefault    OrderedDict    N  __contains__         ~
                                __getitem__
                                __setitem__
update        MutableMapping -  __setitem__          ~
values        MutableMapping -  ValuesView               X
============= ============== == ================ === === ====

__reversed__ and move_to_end are both exclusive to OrderedDict.


C OrderedDict Implementation
============================

================= ================
slot              impl
================= ================
tp_dealloc        odict_dealloc
tp_repr           odict_repr
tp_doc            odict_doc
tp_traverse       odict_traverse
tp_clear          odict_tp_clear
tp_richcompare    odict_richcompare
tp_weaklistoffset (offset)
tp_iter           dict_iter
tp_dictoffset     (offset)
tp_init           odict_init
tp_alloc          (repeated)
tp_new            odict_new
================= ================

================= ================
method            impl
================= ================
__reduce__        odict_reduce
__sizeof__        odict_sizeof
copy              odict_copy
fromkeys          odict_fromkeys
items             odictitems_new
keys              odictkeys_new
pop               odict_pop
popitem           odict_popitem
setdefault        odict_setdefault
values            odictvalues_new
================= ================

Inherited unchanged from object/dict:

================ ==========================
method           type field
================ ==========================
-                tp_free
__contains__     tp_as_sequence.sq_contains
__getattr__      tp_getattro
__getattribute__ tp_getattro
__getitem__      tp_as_mapping.mp_subscript
__hash__         tp_hash
__len__          tp_as_mapping.mp_length
__setattr__      tp_setattro
__str__          tp_str
get              -
================ ==========================


Other Challenges
================

Preserving Ordering During Iteration
------------------------------------
During iteration through an OrderedDict, it is possible that items could
get added, removed, or reordered.  For a linked-list implementation, as
with some other implementations, that situation may lead to undefined
behavior.  The documentation for dict mentions this in the `iter()` section
of http://docs.python.org/3.4/library/stdtypes.html#dictionary-view-objects.
In this implementation we follow dict's lead (as does the pure Python
implementation) for __iter__(), keys(), values(), and items().

For internal iteration (using _odict_FOREACH or not), there is still the
risk that not all nodes that we expect to be seen in the loop actually get
seen.  Thus, we are careful in each of those places to ensure that they
are.  This comes, of course, at a small price at each location.  The
solutions are much the same as those detailed in the `Situation that
Endangers Consistency` section above.


Potential Optimizations
=======================

* Allocate the nodes as a block via od_fast_nodes instead of individually.
  - Set node->key to NULL to indicate the node is not-in-use.
  - Add _odict_EXISTS()?
  - How to maintain consistency across resizes?  Existing node pointers
    would be invalidate after a resize, which is particularly problematic
    for the iterators.
* Use a more stream-lined implementation of update() and, likely indirectly,
  __init__().

*/

/* TODO

sooner:
- reentrancy (make sure everything is at a thread-safe state when calling
  into Python).  I've already checked this multiple times, but want to
  make one more pass.
- add unit tests for reentrancy?

later:
- make the dict views support the full set API (the pure Python impl does)
- implement a fuller MutableMapping API in C?
- move the MutableMapping implementation to abstract.c?
- optimize mutablemapping_update
- use PyObject_MALLOC (small object allocator) for odict nodes?
- support subclasses better (e.g. in odict_richcompare)

*/

#include "structmember.h"
#include "clinic/odictobject.c.h"

/*[clinic input]
class OrderedDict "PyODictObject *" "&PyODict_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=ca0641cf6143d4af]*/


/* PyODictObject */
struct _odictobject {
    PyDictObject od_dict;        /* the underlying dict */
    PyObject *od_inst_dict;      /* OrderedDict().__dict__ */
    PyObject *od_weakreflist;    /* holds weakrefs to the odict */
};

#define _odict_EMPTY(od) (((PyDictObject *)od)->ma_used == 0)

static int
_odict_keys_equal(PyODictObject *a, PyODictObject *b)
{
    PyObject *la = PySequence_List((PyObject*)a);
    if (la == NULL) {
        return -1;
    }
    PyObject *lb = PySequence_List((PyObject*)b);
    if (lb == NULL) {
        return -1;
    }

    int res = PyObject_RichCompareBool(la, lb, Py_EQ);
    Py_DECREF(la);
    Py_DECREF(lb);
    return res;
}


/* ----------------------------------------------
 * OrderedDict methods
 */

/* __eq__() */

PyDoc_STRVAR(odict_eq__doc__,
"od.__eq__(y) <==> od==y.  Comparison to another OD is order-sensitive \n\
        while comparison to a regular mapping is order-insensitive.\n\
        ");

/* forward */
static PyObject * odict_richcompare(PyObject *v, PyObject *w, int op);

static PyObject *
odict_eq(PyObject *a, PyObject *b)
{
    return odict_richcompare(a, b, Py_EQ);
}

/* __init__() */

PyDoc_STRVAR(odict_init__doc__,
"Initialize an ordered dictionary.  The signature is the same as\n\
        regular dictionaries, but keyword arguments are not recommended because\n\
        their insertion order is arbitrary.\n\
\n\
        ");

/* forward */
static int odict_init(PyObject *self, PyObject *args, PyObject *kwds);

/* __iter__() */

PyDoc_STRVAR(odict_iter__doc__, "od.__iter__() <==> iter(od)");

/* __ne__() */

/* Mapping.__ne__() does not have a docstring. */
PyDoc_STRVAR(odict_ne__doc__, "");

static PyObject *
odict_ne(PyObject *a, PyObject *b)
{
    return odict_richcompare(a, b, Py_NE);
}

/* __repr__() */

PyDoc_STRVAR(odict_repr__doc__, "od.__repr__() <==> repr(od)");

static PyObject * odict_repr(PyODictObject *self);  /* forward */

/* __setitem__() */

PyDoc_STRVAR(odict_setitem__doc__, "od.__setitem__(i, y) <==> od[i]=y");

/* __sizeof__() */

static PyObject *
odict_sizeof(PyODictObject *od)
{
    Py_ssize_t res = _PyDict_SizeOf((PyDictObject *)od);
    return PyLong_FromSsize_t(res);
}

/* __reduce__() */

static PyObject *
odict_reduce(register PyODictObject *od)
{
    _Py_IDENTIFIER(__dict__);
    _Py_IDENTIFIER(items);
    PyObject *dict = NULL, *result = NULL;
    PyObject *items_iter, *items, *args = NULL;

    /* capture any instance state */
    dict = _PyObject_GetAttrId((PyObject *)od, &PyId___dict__);
    if (dict == NULL)
        goto Done;
    else {
        /* od.__dict__ isn't necessarily a dict... */
        Py_ssize_t dict_len = PyObject_Length(dict);
        if (dict_len == -1)
            goto Done;
        if (!dict_len) {
            /* nothing to pickle in od.__dict__ */
            Py_CLEAR(dict);
        }
    }

    /* build the result */
    args = PyTuple_New(0);
    if (args == NULL)
        goto Done;

    items = _PyObject_CallMethodIdObjArgs((PyObject *)od, &PyId_items, NULL);
    if (items == NULL)
        goto Done;

    items_iter = PyObject_GetIter(items);
    Py_DECREF(items);
    if (items_iter == NULL)
        goto Done;

    result = PyTuple_Pack(5, Py_TYPE(od), args, dict ? dict : Py_None, Py_None, items_iter);
    Py_DECREF(items_iter);

Done:
    Py_XDECREF(dict);
    Py_XDECREF(args);

    return result;
}


/* popitem() */

/*[clinic input]
OrderedDict.popitem

    last: bool = True

Remove and return a (key, value) pair from the dictionary.

Pairs are returned in LIFO order if last is true or FIFO order if false.
[clinic start generated code]*/

static PyObject *
OrderedDict_popitem_impl(PyODictObject *self, int last)
/*[clinic end generated code: output=98e7d986690d49eb input=d992ac5ee8305e1a]*/
{
    return dict_popitem_common((PyDictObject*)self, last);
}

/* keys() */
static PyObject * odictkeys_new(PyObject *od);  /* forward */

/* values() */
static PyObject * odictvalues_new(PyObject *od);  /* forward */

/* items() */
static PyObject * odictitems_new(PyObject *od);  /* forward */

/* copy() */
PyDoc_STRVAR(odict_copy__doc__, "od.copy() -> a shallow copy of od");

static PyObject *
odict_copy(register PyODictObject *od)
{
    PyObject *od_copy;

    if (PyODict_CheckExact(od))
        od_copy = PyODict_New();
    else
        od_copy = _PyObject_CallNoArg((PyObject *)Py_TYPE(od));
    if (od_copy == NULL)
        return NULL;

    if (PyDict_Merge(od_copy, (PyObject*)od, 1) == 0) {
        return od_copy;
    }
    Py_DECREF(od_copy);
    return NULL;
}

/* __reversed__() */

PyDoc_STRVAR(odict_reversed__doc__, "od.__reversed__() <==> reversed(od)");

static PyObject *
odict_reversed(PyODictObject *od)
{
    return dictiter_new((PyDictObject*)od, &PyDictIterKey_Type, 1);
}


/* move_to_end() */

/*[clinic input]
OrderedDict.move_to_end

    key: object
    last: bool = True

Move an existing element to the end (or beginning if last is false).

Raise KeyError if the element does not exist.
[clinic start generated code]*/

static PyObject *
OrderedDict_move_to_end_impl(PyODictObject *self, PyObject *key, int last)
/*[clinic end generated code: output=fafa4c5cc9b92f20 input=d6ceff7132a2fcd7]*/
{
    if (_odict_EMPTY(self)) {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }

    Py_hash_t hash = PyObject_Hash(key);
    if (hash == -1) {
        return NULL;
    }

    PyDictObject *mp = (PyDictObject*)self;
    PyDictKeysObject *keys = mp->ma_keys;
    PyObject *value;
    Py_ssize_t ix = keys->dk_lookup(mp, key, hash, &value);
    if (ix == DKIX_EMPTY) {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }
    if (ix == DKIX_ERROR) {
        return NULL;
    }

    // Use key in dict instead of argument
    key = DK_ENTRIES(keys)[ix].me_key;
    Py_ssize_t offset = mp->ma_offset;

    if (last) {
        if (ix == keys->dk_nentries - 1) {
            Py_RETURN_NONE;
        }
        if (keys->dk_usable == 0) {
            if (dictresize(mp, GROWTH_RATE(mp) + offset, offset) < 0) {
                return NULL;
            }
            keys = mp->ma_keys;
            ix = lookdict_ident(keys, key, hash);
            assert(ix >= 0);
        }

        Py_ssize_t hashpos = lookdict_index(keys, hash, ix);
        dk_set_index(keys, hashpos, keys->dk_nentries);

        PyDictKeyEntry *ep0 = DK_ENTRIES(keys);
        ep0[keys->dk_nentries] = ep0[ix];
        ep0[ix].me_key = NULL;
        ep0[ix].me_value = NULL;
        ep0[ix].me_hash = 0;

        mp->ma_keys->dk_nentries++;
        mp->ma_keys->dk_usable--;
        if (ix == offset) {
            mp->ma_offset++;
        }
    }
    else {
        if (ix == offset) {
            Py_RETURN_NONE;
        }

        if (offset == 0) {
            offset = mp->ma_used / 2 + 2;  // reserve at least two space.
            if (dictresize(mp, GROWTH_RATE(mp) + offset, offset) < 0) {
                return NULL;
            }
            keys = mp->ma_keys;
            ix = lookdict_ident(keys, key, hash);
            assert(ix >= 0);
        }

        offset--;
        Py_ssize_t hashpos = lookdict_index(keys, hash, ix);
        dk_set_index(keys, hashpos, offset);

        PyDictKeyEntry *ep0 = DK_ENTRIES(keys);
        ep0[offset] = ep0[ix];
        ep0[ix].me_key = NULL;
        ep0[ix].me_value = NULL;
        ep0[ix].me_hash = 0;

        mp->ma_keys->dk_nentries++;
        mp->ma_keys->dk_usable--;
        mp->ma_offset = offset;
    }

    Py_RETURN_NONE;
}


/* tp_methods */

static PyMethodDef odict_methods[] = {

    /* explicitly defined so we can align docstrings with
     * collections.OrderedDict */
    {"__eq__",          (PyCFunction)odict_eq,          METH_NOARGS,
     odict_eq__doc__},
    {"__init__",        (PyCFunction)odict_init,        METH_NOARGS,
     odict_init__doc__},
    {"__iter__",        (PyCFunction)dict_iter,         METH_NOARGS,
     odict_iter__doc__},
    {"__ne__",          (PyCFunction)odict_ne,          METH_NOARGS,
     odict_ne__doc__},
    {"__repr__",        (PyCFunction)odict_repr,        METH_NOARGS,
     odict_repr__doc__},
    {"__setitem__",     (PyCFunction)dict_ass_sub,      METH_NOARGS,
     odict_setitem__doc__},

    /* overridden dict methods */
    {"__sizeof__",      (PyCFunction)odict_sizeof,      METH_NOARGS,
     sizeof__doc__},
    {"__reduce__",      (PyCFunction)odict_reduce,      METH_NOARGS,
     reduce_doc},
    ORDEREDDICT_POPITEM_METHODDEF
    {"keys",            (PyCFunction)odictkeys_new,     METH_NOARGS,
     keys__doc__},
    {"values",          (PyCFunction)odictvalues_new,   METH_NOARGS,
     values__doc__},
    {"items",           (PyCFunction)odictitems_new,    METH_NOARGS,
     items__doc__},
    {"copy",            (PyCFunction)odict_copy,        METH_NOARGS,
     odict_copy__doc__},

    /* new methods */
    {"__reversed__",    (PyCFunction)odict_reversed,    METH_NOARGS,
     odict_reversed__doc__},
    ORDEREDDICT_MOVE_TO_END_METHODDEF

    {NULL,              NULL}   /* sentinel */
};


/* ----------------------------------------------
 * OrderedDict members
 */

/* tp_getset */

static PyGetSetDef odict_getset[] = {
    {"__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict},
    {NULL}
};

/* ----------------------------------------------
 * OrderedDict type slot methods
 */

/* tp_dealloc */

static void
odict_dealloc(PyODictObject *self)
{
    PyThreadState *tstate = PyThreadState_GET();

    PyObject_GC_UnTrack(self);
    Py_TRASHCAN_SAFE_BEGIN(self)

    Py_XDECREF(self->od_inst_dict);
    if (self->od_weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *)self);

    /* Call the base tp_dealloc().  Since it too uses the trashcan mechanism,
     * temporarily decrement trash_delete_nesting to prevent triggering it
     * and putting the partially deallocated object on the trashcan's
     * to-be-deleted-later list.
     */
    --tstate->trash_delete_nesting;
    assert(_tstate->trash_delete_nesting < PyTrash_UNWIND_LEVEL);
    PyDict_Type.tp_dealloc((PyObject *)self);
    ++tstate->trash_delete_nesting;

    Py_TRASHCAN_SAFE_END(self)
}

/* tp_repr */

static PyObject *
odict_repr(PyODictObject *self)
{
    int i;
    _Py_IDENTIFIER(items);
    PyObject *pieces = NULL, *result = NULL;
    const char *classname;

    classname = strrchr(Py_TYPE(self)->tp_name, '.');
    if (classname == NULL)
        classname = Py_TYPE(self)->tp_name;
    else
        classname++;

    if (PyODict_SIZE(self) == 0)
        return PyUnicode_FromFormat("%s()", classname);

    i = Py_ReprEnter((PyObject *)self);
    if (i != 0) {
        return i > 0 ? PyUnicode_FromString("...") : NULL;
    }

    if (PyODict_CheckExact(self)) {
        Py_ssize_t count = 0;
        pieces = PyList_New(PyODict_SIZE(self));
        if (pieces == NULL)
            goto Done;

        Py_ssize_t it = 0;
        PyObject *key, *value;
        while (PyDict_Next((PyObject*)self, &it, &key, &value)) {
            PyObject *pair = PyTuple_Pack(2, key, value);
            if (pair == NULL) {
                goto Done;
            }

            if (count < PyList_GET_SIZE(pieces))
                PyList_SET_ITEM(pieces, count, pair);  /* steals reference */
            else {
                if (PyList_Append(pieces, pair) < 0) {
                    Py_DECREF(pair);
                    goto Done;
                }
                Py_DECREF(pair);
            }
            count++;
        }
        if (count < PyList_GET_SIZE(pieces))
            Py_SIZE(pieces) = count;
    }
    else {
        PyObject *items = _PyObject_CallMethodIdObjArgs((PyObject *)self,
                                                        &PyId_items, NULL);
        if (items == NULL)
            goto Done;
        pieces = PySequence_List(items);
        Py_DECREF(items);
        if (pieces == NULL)
            goto Done;
    }

    result = PyUnicode_FromFormat("%s(%R)", classname, pieces);

Done:
    Py_XDECREF(pieces);
    Py_ReprLeave((PyObject *)self);
    return result;
}

/* tp_doc */

PyDoc_STRVAR(odict_doc,
        "Dictionary that remembers insertion order");

/* tp_traverse */

static int
odict_traverse(PyODictObject *od, visitproc visit, void *arg)
{
    Py_VISIT(od->od_inst_dict);
    Py_VISIT(od->od_weakreflist);
    return PyDict_Type.tp_traverse((PyObject *)od, visit, arg);
}

/* tp_clear */

static int
odict_tp_clear(PyODictObject *od)
{
    Py_CLEAR(od->od_inst_dict);
    Py_CLEAR(od->od_weakreflist);
    PyDict_Clear((PyObject*)od);
    return 0;
}

/* tp_richcompare */

static PyObject *
odict_richcompare(PyObject *v, PyObject *w, int op)
{
    if (!PyODict_Check(v) || !PyDict_Check(w)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (op == Py_EQ || op == Py_NE) {
        PyObject *res, *cmp;
        int eq;

        cmp = PyDict_Type.tp_richcompare(v, w, op);
        if (cmp == NULL)
            return NULL;
        if (!PyODict_Check(w))
            return cmp;
        if (op == Py_EQ && cmp == Py_False)
            return cmp;
        if (op == Py_NE && cmp == Py_True)
            return cmp;
        Py_DECREF(cmp);

        /* Try comparing odict keys. */
        eq = _odict_keys_equal((PyODictObject *)v, (PyODictObject *)w);
        if (eq < 0)
            return NULL;

        res = (eq == (op == Py_EQ)) ? Py_True : Py_False;
        Py_INCREF(res);
        return res;
    } else {
        Py_RETURN_NOTIMPLEMENTED;
    }
}

/* tp_init */

static int
odict_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    return dict_update_common(self, args, kwds, "OrderedDict");
}

/* tp_new */

static PyObject *
odict_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyODictObject *od;

    od = (PyODictObject *)PyDict_Type.tp_new(type, args, kwds);
    if (od == NULL)
        return NULL;

    return (PyObject*)od;
}

/* PyODict_Type */

PyTypeObject PyODict_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "collections.OrderedDict",                  /* tp_name */
    sizeof(PyODictObject),                      /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)odict_dealloc,                  /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)odict_repr,                       /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    odict_doc,                                  /* tp_doc */
    (traverseproc)odict_traverse,               /* tp_traverse */
    (inquiry)odict_tp_clear,                    /* tp_clear */
    (richcmpfunc)odict_richcompare,             /* tp_richcompare */
    offsetof(PyODictObject, od_weakreflist),    /* tp_weaklistoffset */
    (getiterfunc)dict_iter,                     /* tp_iter */
    0,                                          /* tp_iternext */
    odict_methods,                              /* tp_methods */
    0,                                          /* tp_members */
    odict_getset,                               /* tp_getset */
    &PyDict_Type,                               /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    offsetof(PyODictObject, od_inst_dict),      /* tp_dictoffset */
    (initproc)odict_init,                       /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    (newfunc)odict_new,                         /* tp_new */
    0,                                          /* tp_free */
};


/* ----------------------------------------------
 * the public OrderedDict API
 */

PyObject *
PyODict_New(void) {
    return odict_new(&PyODict_Type, NULL, NULL);
}

int
PyODict_SetItem(PyObject *od, PyObject *key, PyObject *value)
{
    return PyDict_SetItem(od, key, value);
}

int
PyODict_DelItem(PyObject *od, PyObject *key)
{
    return PyDict_DelItem(od, key);
}


/* -------------------------------------------
 * The OrderedDict views (keys/values/items)
 */

/* keys() */

static PyObject *
odictkeys_reversed(_PyDictViewObject *dv)
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictIterKey_Type, 1);
}

static PyMethodDef odictkeys_methods[] = {
    {"__reversed__", (PyCFunction)odictkeys_reversed, METH_NOARGS, NULL},
    {NULL,          NULL}           /* sentinel */
};

PyTypeObject PyODictKeys_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "odict_keys",
    .tp_methods = odictkeys_methods,
    .tp_base = &PyDictKeys_Type,
};

static PyObject *
odictkeys_new(PyObject *od)
{
    return _PyDictView_New(od, &PyODictKeys_Type);
}

/* items() */

static PyObject *
odictitems_reversed(_PyDictViewObject *dv)
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictIterItem_Type, 1);
}

static PyMethodDef odictitems_methods[] = {
    {"__reversed__", (PyCFunction)odictitems_reversed, METH_NOARGS, NULL},
    {NULL,          NULL}           /* sentinel */
};

PyTypeObject PyODictItems_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "odict_items",
    .tp_methods = odictitems_methods,
    .tp_base = &PyDictItems_Type,
};

static PyObject *
odictitems_new(PyObject *od)
{
    return _PyDictView_New(od, &PyODictItems_Type);
}

/* values() */

static PyObject *
odictvalues_reversed(_PyDictViewObject *dv)
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictIterValue_Type, 1);
}

static PyMethodDef odictvalues_methods[] = {
    {"__reversed__", (PyCFunction)odictvalues_reversed, METH_NOARGS, NULL},
    {NULL,          NULL}           /* sentinel */
};

PyTypeObject PyODictValues_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "odict_values",
    .tp_methods = odictvalues_methods,
    .tp_base = &PyDictValues_Type
};

static PyObject *
odictvalues_new(PyObject *od)
{
    return _PyDictView_New(od, &PyODictValues_Type);
}
