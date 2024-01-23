/*--------------------------------------------------------------------
 * Licensed to PSF under a Contributor Agreement.
 * See https://www.python.org/psf/license for licensing details.
 *
 * _elementtree - C accelerator for xml.etree.ElementTree
 * Copyright (c) 1999-2009 by Secret Labs AB.  All rights reserved.
 * Copyright (c) 1999-2009 by Fredrik Lundh.
 *
 * info@pythonware.com
 * http://www.pythonware.com
 *--------------------------------------------------------------------
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_import.h"        // _PyImport_GetModuleAttrString()
#include "pycore_pyhash.h"        // _Py_HashSecret

#include <stddef.h>               // offsetof()
#include "expat.h"
#include "pyexpat.h"

/* -------------------------------------------------------------------- */
/* configuration */

/* An element can hold this many children without extra memory
   allocations. */
#define STATIC_CHILDREN 4

/* For best performance, chose a value so that 80-90% of all nodes
   have no more than the given number of children.  Set this to zero
   to minimize the size of the element structure itself (this only
   helps if you have lots of leaf nodes with attributes). */

/* Also note that pymalloc always allocates blocks in multiples of
   eight bytes.  For the current C version of ElementTree, this means
   that the number of children should be an even number, at least on
   32-bit platforms. */

/* -------------------------------------------------------------------- */

/* compiler tweaks */
#if defined(_MSC_VER)
#define LOCAL(type) static __inline type __fastcall
#else
#define LOCAL(type) static type
#endif

/* macros used to store 'join' flags in string object pointers.  note
   that all use of text and tail as object pointers must be wrapped in
   JOIN_OBJ.  see comments in the ElementObject definition for more
   info. */
#define JOIN_GET(p) ((uintptr_t) (p) & 1)
#define JOIN_SET(p, flag) ((void*) ((uintptr_t) (JOIN_OBJ(p)) | (flag)))
#define JOIN_OBJ(p) ((PyObject*) ((uintptr_t) (p) & ~(uintptr_t)1))

/* Py_SETREF for a PyObject* that uses a join flag. */
Py_LOCAL_INLINE(void)
_set_joined_ptr(PyObject **p, PyObject *new_joined_ptr)
{
    PyObject *tmp = JOIN_OBJ(*p);
    *p = new_joined_ptr;
    Py_DECREF(tmp);
}

/* Py_CLEAR for a PyObject* that uses a join flag. Pass the pointer by
 * reference since this function sets it to NULL.
*/
static void _clear_joined_ptr(PyObject **p)
{
    if (*p) {
        _set_joined_ptr(p, NULL);
    }
}

/* Per-module state; PEP 3121 */
typedef struct {
    PyObject *parseerror_obj;
    PyObject *deepcopy_obj;
    PyObject *elementpath_obj;
    PyObject *comment_factory;
    PyObject *pi_factory;
    /* Interned strings */
    PyObject *str_text;
    PyObject *str_tail;
    PyObject *str_append;
    PyObject *str_find;
    PyObject *str_findtext;
    PyObject *str_findall;
    PyObject *str_iterfind;
    PyObject *str_doctype;
    /* Types defined by this extension */
    PyTypeObject *Element_Type;
    PyTypeObject *ElementIter_Type;
    PyTypeObject *TreeBuilder_Type;
    PyTypeObject *XMLParser_Type;

    PyObject *expat_capsule;
    struct PyExpat_CAPI *expat_capi;
} elementtreestate;

static struct PyModuleDef elementtreemodule;

/* Given a module object (assumed to be _elementtree), get its per-module
 * state.
 */
static inline elementtreestate*
get_elementtree_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (elementtreestate *)state;
}

static inline elementtreestate *
get_elementtree_state_by_cls(PyTypeObject *cls)
{
    void *state = PyType_GetModuleState(cls);
    assert(state != NULL);
    return (elementtreestate *)state;
}

static inline elementtreestate *
get_elementtree_state_by_type(PyTypeObject *tp)
{
    PyObject *mod = PyType_GetModuleByDef(tp, &elementtreemodule);
    assert(mod != NULL);
    return get_elementtree_state(mod);
}

static int
elementtree_clear(PyObject *m)
{
    elementtreestate *st = get_elementtree_state(m);
    Py_CLEAR(st->parseerror_obj);
    Py_CLEAR(st->deepcopy_obj);
    Py_CLEAR(st->elementpath_obj);
    Py_CLEAR(st->comment_factory);
    Py_CLEAR(st->pi_factory);

    // Interned strings
    Py_CLEAR(st->str_append);
    Py_CLEAR(st->str_find);
    Py_CLEAR(st->str_findall);
    Py_CLEAR(st->str_findtext);
    Py_CLEAR(st->str_iterfind);
    Py_CLEAR(st->str_tail);
    Py_CLEAR(st->str_text);
    Py_CLEAR(st->str_doctype);

    // Heap types
    Py_CLEAR(st->Element_Type);
    Py_CLEAR(st->ElementIter_Type);
    Py_CLEAR(st->TreeBuilder_Type);
    Py_CLEAR(st->XMLParser_Type);
    Py_CLEAR(st->expat_capsule);

    st->expat_capi = NULL;
    return 0;
}

static int
elementtree_traverse(PyObject *m, visitproc visit, void *arg)
{
    elementtreestate *st = get_elementtree_state(m);
    Py_VISIT(st->parseerror_obj);
    Py_VISIT(st->deepcopy_obj);
    Py_VISIT(st->elementpath_obj);
    Py_VISIT(st->comment_factory);
    Py_VISIT(st->pi_factory);

    // Heap types
    Py_VISIT(st->Element_Type);
    Py_VISIT(st->ElementIter_Type);
    Py_VISIT(st->TreeBuilder_Type);
    Py_VISIT(st->XMLParser_Type);
    Py_VISIT(st->expat_capsule);
    return 0;
}

static void
elementtree_free(void *m)
{
    elementtree_clear((PyObject *)m);
}

/* helpers */

LOCAL(PyObject*)
list_join(PyObject* list)
{
    /* join list elements */
    PyObject* joiner;
    PyObject* result;

    joiner = PyUnicode_FromStringAndSize("", 0);
    if (!joiner)
        return NULL;
    result = PyUnicode_Join(joiner, list);
    Py_DECREF(joiner);
    return result;
}

/* Is the given object an empty dictionary?
*/
static int
is_empty_dict(PyObject *obj)
{
    return PyDict_CheckExact(obj) && PyDict_GET_SIZE(obj) == 0;
}


/* -------------------------------------------------------------------- */
/* the Element type */

typedef struct {

    /* attributes (a dictionary object), or NULL if no attributes */
    PyObject* attrib;

    /* child elements */
    Py_ssize_t length; /* actual number of items */
    Py_ssize_t allocated; /* allocated items */

    /* this either points to _children or to a malloced buffer */
    PyObject* *children;

    PyObject* _children[STATIC_CHILDREN];

} ElementObjectExtra;

typedef struct {
    PyObject_HEAD

    /* element tag (a string). */
    PyObject* tag;

    /* text before first child.  note that this is a tagged pointer;
       use JOIN_OBJ to get the object pointer.  the join flag is used
       to distinguish lists created by the tree builder from lists
       assigned to the attribute by application code; the former
       should be joined before being returned to the user, the latter
       should be left intact. */
    PyObject* text;

    /* text after this element, in parent.  note that this is a tagged
       pointer; use JOIN_OBJ to get the object pointer. */
    PyObject* tail;

    ElementObjectExtra* extra;

    PyObject *weakreflist; /* For tp_weaklistoffset */

} ElementObject;


#define Element_CheckExact(st, op) Py_IS_TYPE(op, (st)->Element_Type)
#define Element_Check(st, op) PyObject_TypeCheck(op, (st)->Element_Type)


/* -------------------------------------------------------------------- */
/* Element constructors and destructor */

LOCAL(int)
create_extra(ElementObject* self, PyObject* attrib)
{
    self->extra = PyObject_Malloc(sizeof(ElementObjectExtra));
    if (!self->extra) {
        PyErr_NoMemory();
        return -1;
    }

    self->extra->attrib = Py_XNewRef(attrib);

    self->extra->length = 0;
    self->extra->allocated = STATIC_CHILDREN;
    self->extra->children = self->extra->_children;

    return 0;
}

LOCAL(void)
dealloc_extra(ElementObjectExtra *extra)
{
    Py_ssize_t i;

    if (!extra)
        return;

    Py_XDECREF(extra->attrib);

    for (i = 0; i < extra->length; i++)
        Py_DECREF(extra->children[i]);

    if (extra->children != extra->_children)
        PyObject_Free(extra->children);

    PyObject_Free(extra);
}

LOCAL(void)
clear_extra(ElementObject* self)
{
    ElementObjectExtra *myextra;

    if (!self->extra)
        return;

    /* Avoid DECREFs calling into this code again (cycles, etc.)
    */
    myextra = self->extra;
    self->extra = NULL;

    dealloc_extra(myextra);
}

/* Convenience internal function to create new Element objects with the given
 * tag and attributes.
*/
LOCAL(PyObject*)
create_new_element(elementtreestate *st, PyObject *tag, PyObject *attrib)
{
    ElementObject* self;

    self = PyObject_GC_New(ElementObject, st->Element_Type);
    if (self == NULL)
        return NULL;
    self->extra = NULL;
    self->tag = Py_NewRef(tag);
    self->text = Py_NewRef(Py_None);
    self->tail = Py_NewRef(Py_None);
    self->weakreflist = NULL;

    PyObject_GC_Track(self);

    if (attrib != NULL && !is_empty_dict(attrib)) {
        if (create_extra(self, attrib) < 0) {
            Py_DECREF(self);
            return NULL;
        }
    }

    return (PyObject*) self;
}

static PyObject *
element_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ElementObject *e = (ElementObject *)type->tp_alloc(type, 0);
    if (e != NULL) {
        e->tag = Py_NewRef(Py_None);
        e->text = Py_NewRef(Py_None);
        e->tail = Py_NewRef(Py_None);
        e->extra = NULL;
        e->weakreflist = NULL;
    }
    return (PyObject *)e;
}

/* Helper function for extracting the attrib dictionary from a keywords dict.
 * This is required by some constructors/functions in this module that can
 * either accept attrib as a keyword argument or all attributes splashed
 * directly into *kwds.
 *
 * Return a dictionary with the content of kwds merged into the content of
 * attrib. If there is no attrib keyword, return a copy of kwds.
 */
static PyObject*
get_attrib_from_keywords(PyObject *kwds)
{
    PyObject *attrib_str = PyUnicode_FromString("attrib");
    if (attrib_str == NULL) {
        return NULL;
    }
    PyObject *attrib = PyDict_GetItemWithError(kwds, attrib_str);

    if (attrib) {
        /* If attrib was found in kwds, copy its value and remove it from
         * kwds
         */
        if (!PyDict_Check(attrib)) {
            Py_DECREF(attrib_str);
            PyErr_Format(PyExc_TypeError, "attrib must be dict, not %.100s",
                         Py_TYPE(attrib)->tp_name);
            return NULL;
        }
        attrib = PyDict_Copy(attrib);
        if (attrib && PyDict_DelItem(kwds, attrib_str) < 0) {
            Py_SETREF(attrib, NULL);
        }
    }
    else if (!PyErr_Occurred()) {
        attrib = PyDict_New();
    }

    Py_DECREF(attrib_str);

    if (attrib != NULL && PyDict_Update(attrib, kwds) < 0) {
        Py_DECREF(attrib);
        return NULL;
    }
    return attrib;
}

/*[clinic input]
module _elementtree
class _elementtree.Element "ElementObject *" "clinic_state()->Element_Type"
class _elementtree.TreeBuilder "TreeBuilderObject *" "clinic_state()->TreeBuilder_Type"
class _elementtree.XMLParser "XMLParserObject *" "clinic_state()->XMLParser_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6c83ea832d2b0ef1]*/

static int
element_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *tag;
    PyObject *attrib = NULL;
    ElementObject *self_elem;

    if (!PyArg_ParseTuple(args, "O|O!:Element", &tag, &PyDict_Type, &attrib))
        return -1;

    if (attrib) {
        /* attrib passed as positional arg */
        attrib = PyDict_Copy(attrib);
        if (!attrib)
            return -1;
        if (kwds) {
            if (PyDict_Update(attrib, kwds) < 0) {
                Py_DECREF(attrib);
                return -1;
            }
        }
    } else if (kwds) {
        /* have keywords args */
        attrib = get_attrib_from_keywords(kwds);
        if (!attrib)
            return -1;
    }

    self_elem = (ElementObject *)self;

    if (attrib != NULL && !is_empty_dict(attrib)) {
        if (create_extra(self_elem, attrib) < 0) {
            Py_DECREF(attrib);
            return -1;
        }
    }

    /* We own a reference to attrib here and it's no longer needed. */
    Py_XDECREF(attrib);

    /* Replace the objects already pointed to by tag, text and tail. */
    Py_XSETREF(self_elem->tag, Py_NewRef(tag));

    _set_joined_ptr(&self_elem->text, Py_NewRef(Py_None));
    _set_joined_ptr(&self_elem->tail, Py_NewRef(Py_None));

    return 0;
}

LOCAL(int)
element_resize(ElementObject* self, Py_ssize_t extra)
{
    Py_ssize_t size;
    PyObject* *children;

    assert(extra >= 0);
    /* make sure self->children can hold the given number of extra
       elements.  set an exception and return -1 if allocation failed */

    if (!self->extra) {
        if (create_extra(self, NULL) < 0)
            return -1;
    }

    size = self->extra->length + extra;  /* never overflows */

    if (size > self->extra->allocated) {
        /* use Python 2.4's list growth strategy */
        size = (size >> 3) + (size < 9 ? 3 : 6) + size;
        /* Coverity CID #182 size_error: Allocating 1 bytes to pointer "children"
         * which needs at least 4 bytes.
         * Although it's a false alarm always assume at least one child to
         * be safe.
         */
        size = size ? size : 1;
        if ((size_t)size > PY_SSIZE_T_MAX/sizeof(PyObject*))
            goto nomemory;
        if (self->extra->children != self->extra->_children) {
            /* Coverity CID #182 size_error: Allocating 1 bytes to pointer
             * "children", which needs at least 4 bytes. Although it's a
             * false alarm always assume at least one child to be safe.
             */
            children = PyObject_Realloc(self->extra->children,
                                        size * sizeof(PyObject*));
            if (!children)
                goto nomemory;
        } else {
            children = PyObject_Malloc(size * sizeof(PyObject*));
            if (!children)
                goto nomemory;
            /* copy existing children from static area to malloc buffer */
            memcpy(children, self->extra->children,
                   self->extra->length * sizeof(PyObject*));
        }
        self->extra->children = children;
        self->extra->allocated = size;
    }

    return 0;

  nomemory:
    PyErr_NoMemory();
    return -1;
}

LOCAL(void)
raise_type_error(PyObject *element)
{
    PyErr_Format(PyExc_TypeError,
                 "expected an Element, not \"%.200s\"",
                 Py_TYPE(element)->tp_name);
}

LOCAL(int)
element_add_subelement(elementtreestate *st, ElementObject *self,
                       PyObject *element)
{
    /* add a child element to a parent */
    if (!Element_Check(st, element)) {
        raise_type_error(element);
        return -1;
    }

    if (element_resize(self, 1) < 0)
        return -1;

    self->extra->children[self->extra->length] = Py_NewRef(element);

    self->extra->length++;

    return 0;
}

LOCAL(PyObject*)
element_get_attrib(ElementObject* self)
{
    /* return borrowed reference to attrib dictionary */
    /* note: this function assumes that the extra section exists */

    PyObject* res = self->extra->attrib;

    if (!res) {
        /* create missing dictionary */
        res = self->extra->attrib = PyDict_New();
    }

    return res;
}

LOCAL(PyObject*)
element_get_text(ElementObject* self)
{
    /* return borrowed reference to text attribute */

    PyObject *res = self->text;

    if (JOIN_GET(res)) {
        res = JOIN_OBJ(res);
        if (PyList_CheckExact(res)) {
            PyObject *tmp = list_join(res);
            if (!tmp)
                return NULL;
            self->text = tmp;
            Py_SETREF(res, tmp);
        }
    }

    return res;
}

LOCAL(PyObject*)
element_get_tail(ElementObject* self)
{
    /* return borrowed reference to text attribute */

    PyObject *res = self->tail;

    if (JOIN_GET(res)) {
        res = JOIN_OBJ(res);
        if (PyList_CheckExact(res)) {
            PyObject *tmp = list_join(res);
            if (!tmp)
                return NULL;
            self->tail = tmp;
            Py_SETREF(res, tmp);
        }
    }

    return res;
}

static PyObject*
subelement(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject* elem;

    elementtreestate *st = get_elementtree_state(self);
    ElementObject* parent;
    PyObject* tag;
    PyObject* attrib = NULL;
    if (!PyArg_ParseTuple(args, "O!O|O!:SubElement",
                          st->Element_Type, &parent, &tag,
                          &PyDict_Type, &attrib)) {
        return NULL;
    }

    if (attrib) {
        /* attrib passed as positional arg */
        attrib = PyDict_Copy(attrib);
        if (!attrib)
            return NULL;
        if (kwds != NULL && PyDict_Update(attrib, kwds) < 0) {
            Py_DECREF(attrib);
            return NULL;
        }
    } else if (kwds) {
        /* have keyword args */
        attrib = get_attrib_from_keywords(kwds);
        if (!attrib)
            return NULL;
    } else {
        /* no attrib arg, no kwds, so no attribute */
    }

    elem = create_new_element(st, tag, attrib);
    Py_XDECREF(attrib);
    if (elem == NULL)
        return NULL;

    if (element_add_subelement(st, parent, elem) < 0) {
        Py_DECREF(elem);
        return NULL;
    }

    return elem;
}

static int
element_gc_traverse(ElementObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->tag);
    Py_VISIT(JOIN_OBJ(self->text));
    Py_VISIT(JOIN_OBJ(self->tail));

    if (self->extra) {
        Py_ssize_t i;
        Py_VISIT(self->extra->attrib);

        for (i = 0; i < self->extra->length; ++i)
            Py_VISIT(self->extra->children[i]);
    }
    return 0;
}

static int
element_gc_clear(ElementObject *self)
{
    Py_CLEAR(self->tag);
    _clear_joined_ptr(&self->text);
    _clear_joined_ptr(&self->tail);

    /* After dropping all references from extra, it's no longer valid anyway,
     * so fully deallocate it.
    */
    clear_extra(self);
    return 0;
}

static void
element_dealloc(ElementObject* self)
{
    PyTypeObject *tp = Py_TYPE(self);

    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(self);
    Py_TRASHCAN_BEGIN(self, element_dealloc)

    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    /* element_gc_clear clears all references and deallocates extra
    */
    element_gc_clear(self);

    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
    Py_TRASHCAN_END
}

/* -------------------------------------------------------------------- */

/*[clinic input]
_elementtree.Element.append

    cls: defining_class
    subelement: object(subclass_of='clinic_state()->Element_Type')
    /

[clinic start generated code]*/

static PyObject *
_elementtree_Element_append_impl(ElementObject *self, PyTypeObject *cls,
                                 PyObject *subelement)
/*[clinic end generated code: output=d00923711ea317fc input=8baf92679f9717b8]*/
{
    elementtreestate *st = get_elementtree_state_by_cls(cls);
    if (element_add_subelement(st, self, subelement) < 0)
        return NULL;

    Py_RETURN_NONE;
}

/*[clinic input]
_elementtree.Element.clear

[clinic start generated code]*/

static PyObject *
_elementtree_Element_clear_impl(ElementObject *self)
/*[clinic end generated code: output=8bcd7a51f94cfff6 input=3c719ff94bf45dd6]*/
{
    clear_extra(self);

    _set_joined_ptr(&self->text, Py_NewRef(Py_None));
    _set_joined_ptr(&self->tail, Py_NewRef(Py_None));

    Py_RETURN_NONE;
}

/*[clinic input]
_elementtree.Element.__copy__

    cls: defining_class
    /

[clinic start generated code]*/

static PyObject *
_elementtree_Element___copy___impl(ElementObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=da22894421ff2b36 input=91edb92d9f441213]*/
{
    Py_ssize_t i;
    ElementObject* element;
    elementtreestate *st = get_elementtree_state_by_cls(cls);

    element = (ElementObject*) create_new_element(
        st, self->tag, self->extra ? self->extra->attrib : NULL);
    if (!element)
        return NULL;

    Py_INCREF(JOIN_OBJ(self->text));
    _set_joined_ptr(&element->text, self->text);

    Py_INCREF(JOIN_OBJ(self->tail));
    _set_joined_ptr(&element->tail, self->tail);

    assert(!element->extra || !element->extra->length);
    if (self->extra) {
        if (element_resize(element, self->extra->length) < 0) {
            Py_DECREF(element);
            return NULL;
        }

        for (i = 0; i < self->extra->length; i++) {
            element->extra->children[i] = Py_NewRef(self->extra->children[i]);
        }

        assert(!element->extra->length);
        element->extra->length = self->extra->length;
    }

    return (PyObject*) element;
}

/* Helper for a deep copy. */
LOCAL(PyObject *) deepcopy(elementtreestate *, PyObject *, PyObject *);

/*[clinic input]
_elementtree.Element.__deepcopy__

    memo: object(subclass_of="&PyDict_Type")
    /

[clinic start generated code]*/

static PyObject *
_elementtree_Element___deepcopy___impl(ElementObject *self, PyObject *memo)
/*[clinic end generated code: output=eefc3df50465b642 input=a2d40348c0aade10]*/
{
    Py_ssize_t i;
    ElementObject* element;
    PyObject* tag;
    PyObject* attrib;
    PyObject* text;
    PyObject* tail;
    PyObject* id;

    PyTypeObject *tp = Py_TYPE(self);
    elementtreestate *st = get_elementtree_state_by_type(tp);
    tag = deepcopy(st, self->tag, memo);
    if (!tag)
        return NULL;

    if (self->extra && self->extra->attrib) {
        attrib = deepcopy(st, self->extra->attrib, memo);
        if (!attrib) {
            Py_DECREF(tag);
            return NULL;
        }
    } else {
        attrib = NULL;
    }

    element = (ElementObject*) create_new_element(st, tag, attrib);

    Py_DECREF(tag);
    Py_XDECREF(attrib);

    if (!element)
        return NULL;

    text = deepcopy(st, JOIN_OBJ(self->text), memo);
    if (!text)
        goto error;
    _set_joined_ptr(&element->text, JOIN_SET(text, JOIN_GET(self->text)));

    tail = deepcopy(st, JOIN_OBJ(self->tail), memo);
    if (!tail)
        goto error;
    _set_joined_ptr(&element->tail, JOIN_SET(tail, JOIN_GET(self->tail)));

    assert(!element->extra || !element->extra->length);
    if (self->extra) {
        if (element_resize(element, self->extra->length) < 0)
            goto error;

        for (i = 0; i < self->extra->length; i++) {
            PyObject* child = deepcopy(st, self->extra->children[i], memo);
            if (!child || !Element_Check(st, child)) {
                if (child) {
                    raise_type_error(child);
                    Py_DECREF(child);
                }
                element->extra->length = i;
                goto error;
            }
            element->extra->children[i] = child;
        }

        assert(!element->extra->length);
        element->extra->length = self->extra->length;
    }

    /* add object to memo dictionary (so deepcopy won't visit it again) */
    id = PyLong_FromSsize_t((uintptr_t) self);
    if (!id)
        goto error;

    i = PyDict_SetItem(memo, id, (PyObject*) element);

    Py_DECREF(id);

    if (i < 0)
        goto error;

    return (PyObject*) element;

  error:
    Py_DECREF(element);
    return NULL;
}

LOCAL(PyObject *)
deepcopy(elementtreestate *st, PyObject *object, PyObject *memo)
{
    /* do a deep copy of the given object */

    /* Fast paths */
    if (object == Py_None || PyUnicode_CheckExact(object)) {
        return Py_NewRef(object);
    }

    if (Py_REFCNT(object) == 1) {
        if (PyDict_CheckExact(object)) {
            PyObject *key, *value;
            Py_ssize_t pos = 0;
            int simple = 1;
            while (PyDict_Next(object, &pos, &key, &value)) {
                if (!PyUnicode_CheckExact(key) || !PyUnicode_CheckExact(value)) {
                    simple = 0;
                    break;
                }
            }
            if (simple)
                return PyDict_Copy(object);
            /* Fall through to general case */
        }
        else if (Element_CheckExact(st, object)) {
            return _elementtree_Element___deepcopy___impl(
                (ElementObject *)object, memo);
        }
    }

    /* General case */
    if (!st->deepcopy_obj) {
        PyErr_SetString(PyExc_RuntimeError,
                        "deepcopy helper not found");
        return NULL;
    }

    PyObject *args[2] = {object, memo};
    return PyObject_Vectorcall(st->deepcopy_obj, args, 2, NULL);
}


/*[clinic input]
_elementtree.Element.__sizeof__ -> size_t

[clinic start generated code]*/

static size_t
_elementtree_Element___sizeof___impl(ElementObject *self)
/*[clinic end generated code: output=baae4e7ae9fe04ec input=54e298c501f3e0d0]*/
{
    size_t result = _PyObject_SIZE(Py_TYPE(self));
    if (self->extra) {
        result += sizeof(ElementObjectExtra);
        if (self->extra->children != self->extra->_children) {
            result += (size_t)self->extra->allocated * sizeof(PyObject*);
        }
    }
    return result;
}

/* dict keys for getstate/setstate. */
#define PICKLED_TAG "tag"
#define PICKLED_CHILDREN "_children"
#define PICKLED_ATTRIB "attrib"
#define PICKLED_TAIL "tail"
#define PICKLED_TEXT "text"

/* __getstate__ returns a fabricated instance dict as in the pure-Python
 * Element implementation, for interoperability/interchangeability.  This
 * makes the pure-Python implementation details an API, but (a) there aren't
 * any unnecessary structures there; and (b) it buys compatibility with 3.2
 * pickles.  See issue #16076.
 */
/*[clinic input]
_elementtree.Element.__getstate__

[clinic start generated code]*/

static PyObject *
_elementtree_Element___getstate___impl(ElementObject *self)
/*[clinic end generated code: output=37279aeeb6bb5b04 input=f0d16d7ec2f7adc1]*/
{
    Py_ssize_t i;
    PyObject *children, *attrib;

    /* Build a list of children. */
    children = PyList_New(self->extra ? self->extra->length : 0);
    if (!children)
        return NULL;
    for (i = 0; i < PyList_GET_SIZE(children); i++) {
        PyObject *child = Py_NewRef(self->extra->children[i]);
        PyList_SET_ITEM(children, i, child);
    }

    if (self->extra && self->extra->attrib) {
        attrib = Py_NewRef(self->extra->attrib);
    }
    else {
        attrib = PyDict_New();
        if (!attrib) {
            Py_DECREF(children);
            return NULL;
        }
    }

    return Py_BuildValue("{sOsNsNsOsO}",
                         PICKLED_TAG, self->tag,
                         PICKLED_CHILDREN, children,
                         PICKLED_ATTRIB, attrib,
                         PICKLED_TEXT, JOIN_OBJ(self->text),
                         PICKLED_TAIL, JOIN_OBJ(self->tail));
}

static PyObject *
element_setstate_from_attributes(elementtreestate *st,
                                 ElementObject *self,
                                 PyObject *tag,
                                 PyObject *attrib,
                                 PyObject *text,
                                 PyObject *tail,
                                 PyObject *children)
{
    Py_ssize_t i, nchildren;
    ElementObjectExtra *oldextra = NULL;

    if (!tag) {
        PyErr_SetString(PyExc_TypeError, "tag may not be NULL");
        return NULL;
    }

    Py_XSETREF(self->tag, Py_NewRef(tag));

    text = text ? JOIN_SET(text, PyList_CheckExact(text)) : Py_None;
    Py_INCREF(JOIN_OBJ(text));
    _set_joined_ptr(&self->text, text);

    tail = tail ? JOIN_SET(tail, PyList_CheckExact(tail)) : Py_None;
    Py_INCREF(JOIN_OBJ(tail));
    _set_joined_ptr(&self->tail, tail);

    /* Handle ATTRIB and CHILDREN. */
    if (!children && !attrib) {
        Py_RETURN_NONE;
    }

    /* Compute 'nchildren'. */
    if (children) {
        if (!PyList_Check(children)) {
            PyErr_SetString(PyExc_TypeError, "'_children' is not a list");
            return NULL;
        }
        nchildren = PyList_GET_SIZE(children);

        /* (Re-)allocate 'extra'.
           Avoid DECREFs calling into this code again (cycles, etc.)
         */
        oldextra = self->extra;
        self->extra = NULL;
        if (element_resize(self, nchildren)) {
            assert(!self->extra || !self->extra->length);
            clear_extra(self);
            self->extra = oldextra;
            return NULL;
        }
        assert(self->extra);
        assert(self->extra->allocated >= nchildren);
        if (oldextra) {
            assert(self->extra->attrib == NULL);
            self->extra->attrib = oldextra->attrib;
            oldextra->attrib = NULL;
        }

        /* Copy children */
        for (i = 0; i < nchildren; i++) {
            PyObject *child = PyList_GET_ITEM(children, i);
            if (!Element_Check(st, child)) {
                raise_type_error(child);
                self->extra->length = i;
                dealloc_extra(oldextra);
                return NULL;
            }
            self->extra->children[i] = Py_NewRef(child);
        }

        assert(!self->extra->length);
        self->extra->length = nchildren;
    }
    else {
        if (element_resize(self, 0)) {
            return NULL;
        }
    }

    /* Stash attrib. */
    Py_XSETREF(self->extra->attrib, Py_XNewRef(attrib));
    dealloc_extra(oldextra);

    Py_RETURN_NONE;
}

/* __setstate__ for Element instance from the Python implementation.
 * 'state' should be the instance dict.
 */

static PyObject *
element_setstate_from_Python(elementtreestate *st, ElementObject *self,
                             PyObject *state)
{
    static char *kwlist[] = {PICKLED_TAG, PICKLED_ATTRIB, PICKLED_TEXT,
                             PICKLED_TAIL, PICKLED_CHILDREN, 0};
    PyObject *args;
    PyObject *tag, *attrib, *text, *tail, *children;
    PyObject *retval;

    tag = attrib = text = tail = children = NULL;
    args = PyTuple_New(0);
    if (!args)
        return NULL;

    if (PyArg_ParseTupleAndKeywords(args, state, "|$OOOOO", kwlist, &tag,
                                    &attrib, &text, &tail, &children))
        retval = element_setstate_from_attributes(st, self, tag, attrib, text,
                                                  tail, children);
    else
        retval = NULL;

    Py_DECREF(args);
    return retval;
}

/*[clinic input]
_elementtree.Element.__setstate__

    cls: defining_class
    state: object
    /

[clinic start generated code]*/

static PyObject *
_elementtree_Element___setstate___impl(ElementObject *self,
                                       PyTypeObject *cls, PyObject *state)
/*[clinic end generated code: output=598bfb5730f71509 input=13830488d35d51f7]*/
{
    if (!PyDict_CheckExact(state)) {
        PyErr_Format(PyExc_TypeError,
                     "Don't know how to unpickle \"%.200R\" as an Element",
                     state);
        return NULL;
    }
    else {
        elementtreestate *st = get_elementtree_state_by_cls(cls);
        return element_setstate_from_Python(st, self, state);
    }
}

LOCAL(int)
checkpath(PyObject* tag)
{
    Py_ssize_t i;
    int check = 1;

    /* check if a tag contains an xpath character */

#define PATHCHAR(ch) \
    (ch == '/' || ch == '*' || ch == '[' || ch == '@' || ch == '.')

    if (PyUnicode_Check(tag)) {
        const Py_ssize_t len = PyUnicode_GET_LENGTH(tag);
        const void *data = PyUnicode_DATA(tag);
        int kind = PyUnicode_KIND(tag);
        if (len >= 3 && PyUnicode_READ(kind, data, 0) == '{' && (
                PyUnicode_READ(kind, data, 1) == '}' || (
                PyUnicode_READ(kind, data, 1) == '*' &&
                PyUnicode_READ(kind, data, 2) == '}'))) {
            /* wildcard: '{}tag' or '{*}tag' */
            return 1;
        }
        for (i = 0; i < len; i++) {
            Py_UCS4 ch = PyUnicode_READ(kind, data, i);
            if (ch == '{')
                check = 0;
            else if (ch == '}')
                check = 1;
            else if (check && PATHCHAR(ch))
                return 1;
        }
        return 0;
    }
    if (PyBytes_Check(tag)) {
        const char *p = PyBytes_AS_STRING(tag);
        const Py_ssize_t len = PyBytes_GET_SIZE(tag);
        if (len >= 3 && p[0] == '{' && (
                p[1] == '}' || (p[1] == '*' && p[2] == '}'))) {
            /* wildcard: '{}tag' or '{*}tag' */
            return 1;
        }
        for (i = 0; i < len; i++) {
            if (p[i] == '{')
                check = 0;
            else if (p[i] == '}')
                check = 1;
            else if (check && PATHCHAR(p[i]))
                return 1;
        }
        return 0;
    }

    return 1; /* unknown type; might be path expression */
}

/*[clinic input]
_elementtree.Element.extend

    cls: defining_class
    elements: object
    /

[clinic start generated code]*/

static PyObject *
_elementtree_Element_extend_impl(ElementObject *self, PyTypeObject *cls,
                                 PyObject *elements)
/*[clinic end generated code: output=3e86d37fac542216 input=6479b1b5379d09ae]*/
{
    PyObject* seq;
    Py_ssize_t i;

    seq = PySequence_Fast(elements, "");
    if (!seq) {
        PyErr_Format(
            PyExc_TypeError,
            "expected sequence, not \"%.200s\"", Py_TYPE(elements)->tp_name
            );
        return NULL;
    }

    elementtreestate *st = get_elementtree_state_by_cls(cls);
    for (i = 0; i < PySequence_Fast_GET_SIZE(seq); i++) {
        PyObject* element = Py_NewRef(PySequence_Fast_GET_ITEM(seq, i));
        if (element_add_subelement(st, self, element) < 0) {
            Py_DECREF(seq);
            Py_DECREF(element);
            return NULL;
        }
        Py_DECREF(element);
    }

    Py_DECREF(seq);

    Py_RETURN_NONE;
}

/*[clinic input]
_elementtree.Element.find

    cls: defining_class
    /
    path: object
    namespaces: object = None

[clinic start generated code]*/

static PyObject *
_elementtree_Element_find_impl(ElementObject *self, PyTypeObject *cls,
                               PyObject *path, PyObject *namespaces)
/*[clinic end generated code: output=18f77d393c9fef1b input=94df8a83f956acc6]*/
{
    Py_ssize_t i;
    elementtreestate *st = get_elementtree_state_by_cls(cls);

    if (checkpath(path) || namespaces != Py_None) {
        return PyObject_CallMethodObjArgs(
            st->elementpath_obj, st->str_find, self, path, namespaces, NULL
            );
    }

    if (!self->extra)
        Py_RETURN_NONE;

    for (i = 0; i < self->extra->length; i++) {
        PyObject* item = self->extra->children[i];
        int rc;
        assert(Element_Check(st, item));
        Py_INCREF(item);
        rc = PyObject_RichCompareBool(((ElementObject*)item)->tag, path, Py_EQ);
        if (rc > 0)
            return item;
        Py_DECREF(item);
        if (rc < 0)
            return NULL;
    }

    Py_RETURN_NONE;
}

/*[clinic input]
_elementtree.Element.findtext

    cls: defining_class
    /
    path: object
    default: object = None
    namespaces: object = None

[clinic start generated code]*/

static PyObject *
_elementtree_Element_findtext_impl(ElementObject *self, PyTypeObject *cls,
                                   PyObject *path, PyObject *default_value,
                                   PyObject *namespaces)
/*[clinic end generated code: output=6af7a2d96aac32cb input=32f252099f62a3d2]*/
{
    Py_ssize_t i;
    elementtreestate *st = get_elementtree_state_by_cls(cls);

    if (checkpath(path) || namespaces != Py_None)
        return PyObject_CallMethodObjArgs(
            st->elementpath_obj, st->str_findtext,
            self, path, default_value, namespaces, NULL
            );

    if (!self->extra) {
        return Py_NewRef(default_value);
    }

    for (i = 0; i < self->extra->length; i++) {
        PyObject *item = self->extra->children[i];
        int rc;
        assert(Element_Check(st, item));
        Py_INCREF(item);
        rc = PyObject_RichCompareBool(((ElementObject*)item)->tag, path, Py_EQ);
        if (rc > 0) {
            PyObject* text = element_get_text((ElementObject*)item);
            if (text == Py_None) {
                Py_DECREF(item);
                return PyUnicode_New(0, 0);
            }
            Py_XINCREF(text);
            Py_DECREF(item);
            return text;
        }
        Py_DECREF(item);
        if (rc < 0)
            return NULL;
    }

    return Py_NewRef(default_value);
}

/*[clinic input]
_elementtree.Element.findall

    cls: defining_class
    /
    path: object
    namespaces: object = None

[clinic start generated code]*/

static PyObject *
_elementtree_Element_findall_impl(ElementObject *self, PyTypeObject *cls,
                                  PyObject *path, PyObject *namespaces)
/*[clinic end generated code: output=65e39a1208f3b59e input=7aa0db45673fc9a5]*/
{
    Py_ssize_t i;
    PyObject* out;
    elementtreestate *st = get_elementtree_state_by_cls(cls);

    if (checkpath(path) || namespaces != Py_None) {
        return PyObject_CallMethodObjArgs(
            st->elementpath_obj, st->str_findall, self, path, namespaces, NULL
            );
    }

    out = PyList_New(0);
    if (!out)
        return NULL;

    if (!self->extra)
        return out;

    for (i = 0; i < self->extra->length; i++) {
        PyObject* item = self->extra->children[i];
        int rc;
        assert(Element_Check(st, item));
        Py_INCREF(item);
        rc = PyObject_RichCompareBool(((ElementObject*)item)->tag, path, Py_EQ);
        if (rc != 0 && (rc < 0 || PyList_Append(out, item) < 0)) {
            Py_DECREF(item);
            Py_DECREF(out);
            return NULL;
        }
        Py_DECREF(item);
    }

    return out;
}

/*[clinic input]
_elementtree.Element.iterfind

    cls: defining_class
    /
    path: object
    namespaces: object = None

[clinic start generated code]*/

static PyObject *
_elementtree_Element_iterfind_impl(ElementObject *self, PyTypeObject *cls,
                                   PyObject *path, PyObject *namespaces)
/*[clinic end generated code: output=be5c3f697a14e676 input=88766875a5c9a88b]*/
{
    PyObject* tag = path;
    elementtreestate *st = get_elementtree_state_by_cls(cls);

    return PyObject_CallMethodObjArgs(
        st->elementpath_obj, st->str_iterfind, self, tag, namespaces, NULL);
}

/*[clinic input]
_elementtree.Element.get

    key: object
    default: object = None

[clinic start generated code]*/

static PyObject *
_elementtree_Element_get_impl(ElementObject *self, PyObject *key,
                              PyObject *default_value)
/*[clinic end generated code: output=523c614142595d75 input=ee153bbf8cdb246e]*/
{
    if (self->extra && self->extra->attrib) {
        PyObject *attrib = Py_NewRef(self->extra->attrib);
        PyObject *value = Py_XNewRef(PyDict_GetItemWithError(attrib, key));
        Py_DECREF(attrib);
        if (value != NULL || PyErr_Occurred()) {
            return value;
        }
    }

    return Py_NewRef(default_value);
}

static PyObject *
create_elementiter(elementtreestate *st, ElementObject *self, PyObject *tag,
                   int gettext);


/*[clinic input]
_elementtree.Element.iter

    cls: defining_class
    /
    tag: object = None

[clinic start generated code]*/

static PyObject *
_elementtree_Element_iter_impl(ElementObject *self, PyTypeObject *cls,
                               PyObject *tag)
/*[clinic end generated code: output=bff29dc5d4566c68 input=f6944c48d3f84c58]*/
{
    if (PyUnicode_Check(tag)) {
        if (PyUnicode_GET_LENGTH(tag) == 1 && PyUnicode_READ_CHAR(tag, 0) == '*')
            tag = Py_None;
    }
    else if (PyBytes_Check(tag)) {
        if (PyBytes_GET_SIZE(tag) == 1 && *PyBytes_AS_STRING(tag) == '*')
            tag = Py_None;
    }

    elementtreestate *st = get_elementtree_state_by_cls(cls);
    return create_elementiter(st, self, tag, 0);
}


/*[clinic input]
_elementtree.Element.itertext

    cls: defining_class
    /

[clinic start generated code]*/

static PyObject *
_elementtree_Element_itertext_impl(ElementObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=fdeb2a3bca0ae063 input=a1ef1f0fc872a586]*/
{
    elementtreestate *st = get_elementtree_state_by_cls(cls);
    return create_elementiter(st, self, Py_None, 1);
}


static PyObject*
element_getitem(PyObject* self_, Py_ssize_t index)
{
    ElementObject* self = (ElementObject*) self_;

    if (!self->extra || index < 0 || index >= self->extra->length) {
        PyErr_SetString(
            PyExc_IndexError,
            "child index out of range"
            );
        return NULL;
    }

    return Py_NewRef(self->extra->children[index]);
}

static int
element_bool(PyObject* self_)
{
    ElementObject* self = (ElementObject*) self_;
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                     "Testing an element's truth value will raise an exception "
                     "in future versions.  Use specific 'len(elem)' or "
                     "'elem is not None' test instead.",
                     1) < 0) {
        return -1;
    };
    if (self->extra ? self->extra->length : 0) {
        return 1;
    }
    return 0;
}

/*[clinic input]
_elementtree.Element.insert

    index: Py_ssize_t
    subelement: object(subclass_of='clinic_state()->Element_Type')
    /

[clinic start generated code]*/

static PyObject *
_elementtree_Element_insert_impl(ElementObject *self, Py_ssize_t index,
                                 PyObject *subelement)
/*[clinic end generated code: output=990adfef4d424c0b input=9530f4905aa401ca]*/
{
    Py_ssize_t i;

    if (!self->extra) {
        if (create_extra(self, NULL) < 0)
            return NULL;
    }

    if (index < 0) {
        index += self->extra->length;
        if (index < 0)
            index = 0;
    }
    if (index > self->extra->length)
        index = self->extra->length;

    if (element_resize(self, 1) < 0)
        return NULL;

    for (i = self->extra->length; i > index; i--)
        self->extra->children[i] = self->extra->children[i-1];

    self->extra->children[index] = Py_NewRef(subelement);

    self->extra->length++;

    Py_RETURN_NONE;
}

/*[clinic input]
_elementtree.Element.items

[clinic start generated code]*/

static PyObject *
_elementtree_Element_items_impl(ElementObject *self)
/*[clinic end generated code: output=6db2c778ce3f5a4d input=adbe09aaea474447]*/
{
    if (!self->extra || !self->extra->attrib)
        return PyList_New(0);

    return PyDict_Items(self->extra->attrib);
}

/*[clinic input]
_elementtree.Element.keys

[clinic start generated code]*/

static PyObject *
_elementtree_Element_keys_impl(ElementObject *self)
/*[clinic end generated code: output=bc5bfabbf20eeb3c input=f02caf5b496b5b0b]*/
{
    if (!self->extra || !self->extra->attrib)
        return PyList_New(0);

    return PyDict_Keys(self->extra->attrib);
}

static Py_ssize_t
element_length(ElementObject* self)
{
    if (!self->extra)
        return 0;

    return self->extra->length;
}

/*[clinic input]
_elementtree.Element.makeelement

    cls: defining_class
    tag: object
    attrib: object(subclass_of='&PyDict_Type')
    /

[clinic start generated code]*/

static PyObject *
_elementtree_Element_makeelement_impl(ElementObject *self, PyTypeObject *cls,
                                      PyObject *tag, PyObject *attrib)
/*[clinic end generated code: output=d50bb17a47077d47 input=589829dab92f26e8]*/
{
    PyObject* elem;

    attrib = PyDict_Copy(attrib);
    if (!attrib)
        return NULL;

    elementtreestate *st = get_elementtree_state_by_cls(cls);
    elem = create_new_element(st, tag, attrib);

    Py_DECREF(attrib);

    return elem;
}

/*[clinic input]
_elementtree.Element.remove

    subelement: object(subclass_of='clinic_state()->Element_Type')
    /

[clinic start generated code]*/

static PyObject *
_elementtree_Element_remove_impl(ElementObject *self, PyObject *subelement)
/*[clinic end generated code: output=38fe6c07d6d87d1f input=6133e1d05597d5ee]*/
{
    Py_ssize_t i;
    int rc;
    PyObject *found;

    if (!self->extra) {
        /* element has no children, so raise exception */
        PyErr_SetString(
            PyExc_ValueError,
            "list.remove(x): x not in list"
            );
        return NULL;
    }

    for (i = 0; i < self->extra->length; i++) {
        if (self->extra->children[i] == subelement)
            break;
        rc = PyObject_RichCompareBool(self->extra->children[i], subelement, Py_EQ);
        if (rc > 0)
            break;
        if (rc < 0)
            return NULL;
    }

    if (i >= self->extra->length) {
        /* subelement is not in children, so raise exception */
        PyErr_SetString(
            PyExc_ValueError,
            "list.remove(x): x not in list"
            );
        return NULL;
    }

    found = self->extra->children[i];

    self->extra->length--;
    for (; i < self->extra->length; i++)
        self->extra->children[i] = self->extra->children[i+1];

    Py_DECREF(found);
    Py_RETURN_NONE;
}

static PyObject*
element_repr(ElementObject* self)
{
    int status;

    if (self->tag == NULL)
        return PyUnicode_FromFormat("<Element at %p>", self);

    status = Py_ReprEnter((PyObject *)self);
    if (status == 0) {
        PyObject *res;
        res = PyUnicode_FromFormat("<Element %R at %p>", self->tag, self);
        Py_ReprLeave((PyObject *)self);
        return res;
    }
    if (status > 0)
        PyErr_Format(PyExc_RuntimeError,
                     "reentrant call inside %s.__repr__",
                     Py_TYPE(self)->tp_name);
    return NULL;
}

/*[clinic input]
_elementtree.Element.set

    key: object
    value: object
    /

[clinic start generated code]*/

static PyObject *
_elementtree_Element_set_impl(ElementObject *self, PyObject *key,
                              PyObject *value)
/*[clinic end generated code: output=fb938806be3c5656 input=1efe90f7d82b3fe9]*/
{
    PyObject* attrib;

    if (!self->extra) {
        if (create_extra(self, NULL) < 0)
            return NULL;
    }

    attrib = element_get_attrib(self);
    if (!attrib)
        return NULL;

    if (PyDict_SetItem(attrib, key, value) < 0)
        return NULL;

    Py_RETURN_NONE;
}

static int
element_setitem(PyObject* self_, Py_ssize_t index, PyObject* item)
{
    ElementObject* self = (ElementObject*) self_;
    Py_ssize_t i;
    PyObject* old;

    if (!self->extra || index < 0 || index >= self->extra->length) {
        PyErr_SetString(
            PyExc_IndexError,
            "child assignment index out of range");
        return -1;
    }

    old = self->extra->children[index];

    if (item) {
        PyTypeObject *tp = Py_TYPE(self);
        elementtreestate *st = get_elementtree_state_by_type(tp);
        if (!Element_Check(st, item)) {
            raise_type_error(item);
            return -1;
        }
        self->extra->children[index] = Py_NewRef(item);
    } else {
        self->extra->length--;
        for (i = index; i < self->extra->length; i++)
            self->extra->children[i] = self->extra->children[i+1];
    }

    Py_DECREF(old);

    return 0;
}

static PyObject*
element_subscr(PyObject* self_, PyObject* item)
{
    ElementObject* self = (ElementObject*) self_;

    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);

        if (i == -1 && PyErr_Occurred()) {
            return NULL;
        }
        if (i < 0 && self->extra)
            i += self->extra->length;
        return element_getitem(self_, i);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelen, i;
        size_t cur;
        PyObject* list;

        if (!self->extra)
            return PyList_New(0);

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelen = PySlice_AdjustIndices(self->extra->length, &start, &stop,
                                         step);

        if (slicelen <= 0)
            return PyList_New(0);
        else {
            list = PyList_New(slicelen);
            if (!list)
                return NULL;

            for (cur = start, i = 0; i < slicelen;
                 cur += step, i++) {
                PyObject* item = Py_NewRef(self->extra->children[cur]);
                PyList_SET_ITEM(list, i, item);
            }

            return list;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                "element indices must be integers");
        return NULL;
    }
}

static int
element_ass_subscr(PyObject* self_, PyObject* item, PyObject* value)
{
    ElementObject* self = (ElementObject*) self_;

    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);

        if (i == -1 && PyErr_Occurred()) {
            return -1;
        }
        if (i < 0 && self->extra)
            i += self->extra->length;
        return element_setitem(self_, i, value);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelen, newlen, i;
        size_t cur;

        PyObject* recycle = NULL;
        PyObject* seq;

        if (!self->extra) {
            if (create_extra(self, NULL) < 0)
                return -1;
        }

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return -1;
        }
        slicelen = PySlice_AdjustIndices(self->extra->length, &start, &stop,
                                         step);

        if (value == NULL) {
            /* Delete slice */
            size_t cur;
            Py_ssize_t i;

            if (slicelen <= 0)
                return 0;

            /* Since we're deleting, the direction of the range doesn't matter,
             * so for simplicity make it always ascending.
            */
            if (step < 0) {
                stop = start + 1;
                start = stop + step * (slicelen - 1) - 1;
                step = -step;
            }

            assert((size_t)slicelen <= SIZE_MAX / sizeof(PyObject *));

            /* recycle is a list that will contain all the children
             * scheduled for removal.
            */
            if (!(recycle = PyList_New(slicelen))) {
                return -1;
            }

            /* This loop walks over all the children that have to be deleted,
             * with cur pointing at them. num_moved is the amount of children
             * until the next deleted child that have to be "shifted down" to
             * occupy the deleted's places.
             * Note that in the ith iteration, shifting is done i+i places down
             * because i children were already removed.
            */
            for (cur = start, i = 0; cur < (size_t)stop; cur += step, ++i) {
                /* Compute how many children have to be moved, clipping at the
                 * list end.
                */
                Py_ssize_t num_moved = step - 1;
                if (cur + step >= (size_t)self->extra->length) {
                    num_moved = self->extra->length - cur - 1;
                }

                PyList_SET_ITEM(recycle, i, self->extra->children[cur]);

                memmove(
                    self->extra->children + cur - i,
                    self->extra->children + cur + 1,
                    num_moved * sizeof(PyObject *));
            }

            /* Leftover "tail" after the last removed child */
            cur = start + (size_t)slicelen * step;
            if (cur < (size_t)self->extra->length) {
                memmove(
                    self->extra->children + cur - slicelen,
                    self->extra->children + cur,
                    (self->extra->length - cur) * sizeof(PyObject *));
            }

            self->extra->length -= slicelen;

            /* Discard the recycle list with all the deleted sub-elements */
            Py_DECREF(recycle);
            return 0;
        }

        /* A new slice is actually being assigned */
        seq = PySequence_Fast(value, "");
        if (!seq) {
            PyErr_Format(
                PyExc_TypeError,
                "expected sequence, not \"%.200s\"", Py_TYPE(value)->tp_name
                );
            return -1;
        }
        newlen = PySequence_Fast_GET_SIZE(seq);

        if (step !=  1 && newlen != slicelen)
        {
            Py_DECREF(seq);
            PyErr_Format(PyExc_ValueError,
                "attempt to assign sequence of size %zd "
                "to extended slice of size %zd",
                newlen, slicelen
                );
            return -1;
        }

        /* Resize before creating the recycle bin, to prevent refleaks. */
        if (newlen > slicelen) {
            if (element_resize(self, newlen - slicelen) < 0) {
                Py_DECREF(seq);
                return -1;
            }
        }

        PyTypeObject *tp = Py_TYPE(self);
        elementtreestate *st = get_elementtree_state_by_type(tp);
        for (i = 0; i < newlen; i++) {
            PyObject *element = PySequence_Fast_GET_ITEM(seq, i);
            if (!Element_Check(st, element)) {
                raise_type_error(element);
                Py_DECREF(seq);
                return -1;
            }
        }

        if (slicelen > 0) {
            /* to avoid recursive calls to this method (via decref), move
               old items to the recycle bin here, and get rid of them when
               we're done modifying the element */
            recycle = PyList_New(slicelen);
            if (!recycle) {
                Py_DECREF(seq);
                return -1;
            }
            for (cur = start, i = 0; i < slicelen;
                 cur += step, i++)
                PyList_SET_ITEM(recycle, i, self->extra->children[cur]);
        }

        if (newlen < slicelen) {
            /* delete slice */
            for (i = stop; i < self->extra->length; i++)
                self->extra->children[i + newlen - slicelen] = self->extra->children[i];
        } else if (newlen > slicelen) {
            /* insert slice */
            for (i = self->extra->length-1; i >= stop; i--)
                self->extra->children[i + newlen - slicelen] = self->extra->children[i];
        }

        /* replace the slice */
        for (cur = start, i = 0; i < newlen;
             cur += step, i++) {
            PyObject* element = PySequence_Fast_GET_ITEM(seq, i);
            self->extra->children[cur] = Py_NewRef(element);
        }

        self->extra->length += newlen - slicelen;

        Py_DECREF(seq);

        /* discard the recycle bin, and everything in it */
        Py_XDECREF(recycle);

        return 0;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                "element indices must be integers");
        return -1;
    }
}

static PyObject*
element_tag_getter(ElementObject *self, void *closure)
{
    PyObject *res = self->tag;
    return Py_NewRef(res);
}

static PyObject*
element_text_getter(ElementObject *self, void *closure)
{
    PyObject *res = element_get_text(self);
    return Py_XNewRef(res);
}

static PyObject*
element_tail_getter(ElementObject *self, void *closure)
{
    PyObject *res = element_get_tail(self);
    return Py_XNewRef(res);
}

static PyObject*
element_attrib_getter(ElementObject *self, void *closure)
{
    PyObject *res;
    if (!self->extra) {
        if (create_extra(self, NULL) < 0)
            return NULL;
    }
    res = element_get_attrib(self);
    return Py_XNewRef(res);
}

/* macro for setter validation */
#define _VALIDATE_ATTR_VALUE(V)                     \
    if ((V) == NULL) {                              \
        PyErr_SetString(                            \
            PyExc_AttributeError,                   \
            "can't delete element attribute");      \
        return -1;                                  \
    }

static int
element_tag_setter(ElementObject *self, PyObject *value, void *closure)
{
    _VALIDATE_ATTR_VALUE(value);
    Py_SETREF(self->tag, Py_NewRef(value));
    return 0;
}

static int
element_text_setter(ElementObject *self, PyObject *value, void *closure)
{
    _VALIDATE_ATTR_VALUE(value);
    _set_joined_ptr(&self->text, Py_NewRef(value));
    return 0;
}

static int
element_tail_setter(ElementObject *self, PyObject *value, void *closure)
{
    _VALIDATE_ATTR_VALUE(value);
    _set_joined_ptr(&self->tail, Py_NewRef(value));
    return 0;
}

static int
element_attrib_setter(ElementObject *self, PyObject *value, void *closure)
{
    _VALIDATE_ATTR_VALUE(value);
    if (!PyDict_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                     "attrib must be dict, not %.200s",
                     Py_TYPE(value)->tp_name);
        return -1;
    }
    if (!self->extra) {
        if (create_extra(self, NULL) < 0)
            return -1;
    }
    Py_XSETREF(self->extra->attrib, Py_NewRef(value));
    return 0;
}

/******************************* Element iterator ****************************/

/* ElementIterObject represents the iteration state over an XML element in
 * pre-order traversal. To keep track of which sub-element should be returned
 * next, a stack of parents is maintained. This is a standard stack-based
 * iterative pre-order traversal of a tree.
 * The stack is managed using a continuous array.
 * Each stack item contains the saved parent to which we should return after
 * the current one is exhausted, and the next child to examine in that parent.
 */
typedef struct ParentLocator_t {
    ElementObject *parent;
    Py_ssize_t child_index;
} ParentLocator;

typedef struct {
    PyObject_HEAD
    ParentLocator *parent_stack;
    Py_ssize_t parent_stack_used;
    Py_ssize_t parent_stack_size;
    ElementObject *root_element;
    PyObject *sought_tag;
    int gettext;
} ElementIterObject;


static void
elementiter_dealloc(ElementIterObject *it)
{
    PyTypeObject *tp = Py_TYPE(it);
    Py_ssize_t i = it->parent_stack_used;
    it->parent_stack_used = 0;
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(it);
    while (i--)
        Py_XDECREF(it->parent_stack[i].parent);
    PyMem_Free(it->parent_stack);

    Py_XDECREF(it->sought_tag);
    Py_XDECREF(it->root_element);

    tp->tp_free(it);
    Py_DECREF(tp);
}

static int
elementiter_traverse(ElementIterObject *it, visitproc visit, void *arg)
{
    Py_ssize_t i = it->parent_stack_used;
    while (i--)
        Py_VISIT(it->parent_stack[i].parent);

    Py_VISIT(it->root_element);
    Py_VISIT(it->sought_tag);
    Py_VISIT(Py_TYPE(it));
    return 0;
}

/* Helper function for elementiter_next. Add a new parent to the parent stack.
 */
static int
parent_stack_push_new(ElementIterObject *it, ElementObject *parent)
{
    ParentLocator *item;

    if (it->parent_stack_used >= it->parent_stack_size) {
        Py_ssize_t new_size = it->parent_stack_size * 2;  /* never overflow */
        ParentLocator *parent_stack = it->parent_stack;
        PyMem_Resize(parent_stack, ParentLocator, new_size);
        if (parent_stack == NULL)
            return -1;
        it->parent_stack = parent_stack;
        it->parent_stack_size = new_size;
    }
    item = it->parent_stack + it->parent_stack_used++;
    item->parent = (ElementObject*)Py_NewRef(parent);
    item->child_index = 0;
    return 0;
}

static PyObject *
elementiter_next(ElementIterObject *it)
{
    /* Sub-element iterator.
     *
     * A short note on gettext: this function serves both the iter() and
     * itertext() methods to avoid code duplication. However, there are a few
     * small differences in the way these iterations work. Namely:
     *   - itertext() only yields text from nodes that have it, and continues
     *     iterating when a node doesn't have text (so it doesn't return any
     *     node like iter())
     *   - itertext() also has to handle tail, after finishing with all the
     *     children of a node.
     */
    int rc;
    ElementObject *elem;
    PyObject *text;

    while (1) {
        /* Handle the case reached in the beginning and end of iteration, where
         * the parent stack is empty. If root_element is NULL and we're here, the
         * iterator is exhausted.
         */
        if (!it->parent_stack_used) {
            if (!it->root_element) {
                PyErr_SetNone(PyExc_StopIteration);
                return NULL;
            }

            elem = it->root_element;  /* steals a reference */
            it->root_element = NULL;
        }
        else {
            /* See if there are children left to traverse in the current parent. If
             * yes, visit the next child. If not, pop the stack and try again.
             */
            ParentLocator *item = &it->parent_stack[it->parent_stack_used - 1];
            Py_ssize_t child_index = item->child_index;
            ElementObjectExtra *extra;
            elem = item->parent;
            extra = elem->extra;
            if (!extra || child_index >= extra->length) {
                it->parent_stack_used--;
                /* Note that extra condition on it->parent_stack_used here;
                 * this is because itertext() is supposed to only return *inner*
                 * text, not text following the element it began iteration with.
                 */
                if (it->gettext && it->parent_stack_used) {
                    text = element_get_tail(elem);
                    goto gettext;
                }
                Py_DECREF(elem);
                continue;
            }

#ifndef NDEBUG
            PyTypeObject *tp = Py_TYPE(it);
            elementtreestate *st = get_elementtree_state_by_type(tp);
            assert(Element_Check(st, extra->children[child_index]));
#endif
            elem = (ElementObject *)Py_NewRef(extra->children[child_index]);
            item->child_index++;
        }

        if (parent_stack_push_new(it, elem) < 0) {
            Py_DECREF(elem);
            PyErr_NoMemory();
            return NULL;
        }
        if (it->gettext) {
            text = element_get_text(elem);
            goto gettext;
        }

        if (it->sought_tag == Py_None)
            return (PyObject *)elem;

        rc = PyObject_RichCompareBool(elem->tag, it->sought_tag, Py_EQ);
        if (rc > 0)
            return (PyObject *)elem;

        Py_DECREF(elem);
        if (rc < 0)
            return NULL;
        continue;

gettext:
        if (!text) {
            Py_DECREF(elem);
            return NULL;
        }
        if (text == Py_None) {
            Py_DECREF(elem);
        }
        else {
            Py_INCREF(text);
            Py_DECREF(elem);
            rc = PyObject_IsTrue(text);
            if (rc > 0)
                return text;
            Py_DECREF(text);
            if (rc < 0)
                return NULL;
        }
    }

    return NULL;
}

static PyType_Slot elementiter_slots[] = {
    {Py_tp_dealloc, elementiter_dealloc},
    {Py_tp_traverse, elementiter_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, elementiter_next},
    {0, NULL},
};

static PyType_Spec elementiter_spec = {
    /* Using the module's name since the pure-Python implementation does not
       have such a type. */
    .name = "_elementtree._element_iterator",
    .basicsize = sizeof(ElementIterObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION),
    .slots = elementiter_slots,
};

#define INIT_PARENT_STACK_SIZE 8

static PyObject *
create_elementiter(elementtreestate *st, ElementObject *self, PyObject *tag,
                   int gettext)
{
    ElementIterObject *it;

    it = PyObject_GC_New(ElementIterObject, st->ElementIter_Type);
    if (!it)
        return NULL;

    it->sought_tag = Py_NewRef(tag);
    it->gettext = gettext;
    it->root_element = (ElementObject*)Py_NewRef(self);

    it->parent_stack = PyMem_New(ParentLocator, INIT_PARENT_STACK_SIZE);
    if (it->parent_stack == NULL) {
        Py_DECREF(it);
        PyErr_NoMemory();
        return NULL;
    }
    it->parent_stack_used = 0;
    it->parent_stack_size = INIT_PARENT_STACK_SIZE;

    PyObject_GC_Track(it);

    return (PyObject *)it;
}


/* ==================================================================== */
/* the tree builder type */

typedef struct {
    PyObject_HEAD

    PyObject *root; /* root node (first created node) */

    PyObject *this; /* current node */
    PyObject *last; /* most recently created node */
    PyObject *last_for_tail; /* most recently created node that takes a tail */

    PyObject *data; /* data collector (string or list), or NULL */

    PyObject *stack; /* element stack */
    Py_ssize_t index; /* current stack size (0 means empty) */

    PyObject *element_factory;
    PyObject *comment_factory;
    PyObject *pi_factory;

    /* element tracing */
    PyObject *events_append; /* the append method of the list of events, or NULL */
    PyObject *start_event_obj; /* event objects (NULL to ignore) */
    PyObject *end_event_obj;
    PyObject *start_ns_event_obj;
    PyObject *end_ns_event_obj;
    PyObject *comment_event_obj;
    PyObject *pi_event_obj;

    char insert_comments;
    char insert_pis;
    elementtreestate *state;
} TreeBuilderObject;

#define TreeBuilder_CheckExact(st, op) Py_IS_TYPE((op), (st)->TreeBuilder_Type)

/* -------------------------------------------------------------------- */
/* constructor and destructor */

static PyObject *
treebuilder_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    TreeBuilderObject *t = (TreeBuilderObject *)type->tp_alloc(type, 0);
    if (t != NULL) {
        t->root = NULL;
        t->this = Py_NewRef(Py_None);
        t->last = Py_NewRef(Py_None);
        t->data = NULL;
        t->element_factory = NULL;
        t->comment_factory = NULL;
        t->pi_factory = NULL;
        t->stack = PyList_New(20);
        if (!t->stack) {
            Py_DECREF(t->this);
            Py_DECREF(t->last);
            Py_DECREF((PyObject *) t);
            return NULL;
        }
        t->index = 0;

        t->events_append = NULL;
        t->start_event_obj = t->end_event_obj = NULL;
        t->start_ns_event_obj = t->end_ns_event_obj = NULL;
        t->comment_event_obj = t->pi_event_obj = NULL;
        t->insert_comments = t->insert_pis = 0;
        t->state = get_elementtree_state_by_type(type);
    }
    return (PyObject *)t;
}

/*[clinic input]
_elementtree.TreeBuilder.__init__

    element_factory: object = None
    *
    comment_factory: object = None
    pi_factory: object = None
    insert_comments: bool = False
    insert_pis: bool = False

[clinic start generated code]*/

static int
_elementtree_TreeBuilder___init___impl(TreeBuilderObject *self,
                                       PyObject *element_factory,
                                       PyObject *comment_factory,
                                       PyObject *pi_factory,
                                       int insert_comments, int insert_pis)
/*[clinic end generated code: output=8571d4dcadfdf952 input=ae98a94df20b5cc3]*/
{
    if (element_factory != Py_None) {
        Py_XSETREF(self->element_factory, Py_NewRef(element_factory));
    } else {
        Py_CLEAR(self->element_factory);
    }

    if (comment_factory == Py_None) {
        elementtreestate *st = self->state;
        comment_factory = st->comment_factory;
    }
    if (comment_factory) {
        Py_XSETREF(self->comment_factory, Py_NewRef(comment_factory));
        self->insert_comments = insert_comments;
    } else {
        Py_CLEAR(self->comment_factory);
        self->insert_comments = 0;
    }

    if (pi_factory == Py_None) {
        elementtreestate *st = self->state;
        pi_factory = st->pi_factory;
    }
    if (pi_factory) {
        Py_XSETREF(self->pi_factory, Py_NewRef(pi_factory));
        self->insert_pis = insert_pis;
    } else {
        Py_CLEAR(self->pi_factory);
        self->insert_pis = 0;
    }

    return 0;
}

static int
treebuilder_gc_traverse(TreeBuilderObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->pi_event_obj);
    Py_VISIT(self->comment_event_obj);
    Py_VISIT(self->end_ns_event_obj);
    Py_VISIT(self->start_ns_event_obj);
    Py_VISIT(self->end_event_obj);
    Py_VISIT(self->start_event_obj);
    Py_VISIT(self->events_append);
    Py_VISIT(self->root);
    Py_VISIT(self->this);
    Py_VISIT(self->last);
    Py_VISIT(self->last_for_tail);
    Py_VISIT(self->data);
    Py_VISIT(self->stack);
    Py_VISIT(self->pi_factory);
    Py_VISIT(self->comment_factory);
    Py_VISIT(self->element_factory);
    return 0;
}

static int
treebuilder_gc_clear(TreeBuilderObject *self)
{
    Py_CLEAR(self->pi_event_obj);
    Py_CLEAR(self->comment_event_obj);
    Py_CLEAR(self->end_ns_event_obj);
    Py_CLEAR(self->start_ns_event_obj);
    Py_CLEAR(self->end_event_obj);
    Py_CLEAR(self->start_event_obj);
    Py_CLEAR(self->events_append);
    Py_CLEAR(self->stack);
    Py_CLEAR(self->data);
    Py_CLEAR(self->last);
    Py_CLEAR(self->last_for_tail);
    Py_CLEAR(self->this);
    Py_CLEAR(self->pi_factory);
    Py_CLEAR(self->comment_factory);
    Py_CLEAR(self->element_factory);
    Py_CLEAR(self->root);
    return 0;
}

static void
treebuilder_dealloc(TreeBuilderObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    treebuilder_gc_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

/* -------------------------------------------------------------------- */
/* helpers for handling of arbitrary element-like objects */

/*[clinic input]
_elementtree._set_factories

    comment_factory: object
    pi_factory: object
    /

Change the factories used to create comments and processing instructions.

For internal use only.
[clinic start generated code]*/

static PyObject *
_elementtree__set_factories_impl(PyObject *module, PyObject *comment_factory,
                                 PyObject *pi_factory)
/*[clinic end generated code: output=813b408adee26535 input=99d17627aea7fb3b]*/
{
    elementtreestate *st = get_elementtree_state(module);
    PyObject *old;

    if (!PyCallable_Check(comment_factory) && comment_factory != Py_None) {
        PyErr_Format(PyExc_TypeError, "Comment factory must be callable, not %.100s",
                     Py_TYPE(comment_factory)->tp_name);
        return NULL;
    }
    if (!PyCallable_Check(pi_factory) && pi_factory != Py_None) {
        PyErr_Format(PyExc_TypeError, "PI factory must be callable, not %.100s",
                     Py_TYPE(pi_factory)->tp_name);
        return NULL;
    }

    old = PyTuple_Pack(2,
        st->comment_factory ? st->comment_factory : Py_None,
        st->pi_factory ? st->pi_factory : Py_None);

    if (comment_factory == Py_None) {
        Py_CLEAR(st->comment_factory);
    } else {
        Py_XSETREF(st->comment_factory, Py_NewRef(comment_factory));
    }
    if (pi_factory == Py_None) {
        Py_CLEAR(st->pi_factory);
    } else {
        Py_XSETREF(st->pi_factory, Py_NewRef(pi_factory));
    }

    return old;
}

static int
treebuilder_extend_element_text_or_tail(elementtreestate *st, PyObject *element,
                                        PyObject **data, PyObject **dest,
                                        PyObject *name)
{
    /* Fast paths for the "almost always" cases. */
    if (Element_CheckExact(st, element)) {
        PyObject *dest_obj = JOIN_OBJ(*dest);
        if (dest_obj == Py_None) {
            *dest = JOIN_SET(*data, PyList_CheckExact(*data));
            *data = NULL;
            Py_DECREF(dest_obj);
            return 0;
        }
        else if (JOIN_GET(*dest)) {
            if (PyList_SetSlice(dest_obj, PY_SSIZE_T_MAX, PY_SSIZE_T_MAX, *data) < 0) {
                return -1;
            }
            Py_CLEAR(*data);
            return 0;
        }
    }

    /*  Fallback for the non-Element / non-trivial cases. */
    {
        int r;
        PyObject* joined;
        PyObject* previous = PyObject_GetAttr(element, name);
        if (!previous)
            return -1;
        joined = list_join(*data);
        if (!joined) {
            Py_DECREF(previous);
            return -1;
        }
        if (previous != Py_None) {
            PyObject *tmp = PyNumber_Add(previous, joined);
            Py_DECREF(joined);
            Py_DECREF(previous);
            if (!tmp)
                return -1;
            joined = tmp;
        } else {
            Py_DECREF(previous);
        }

        r = PyObject_SetAttr(element, name, joined);
        Py_DECREF(joined);
        if (r < 0)
            return -1;
        Py_CLEAR(*data);
        return 0;
    }
}

LOCAL(int)
treebuilder_flush_data(TreeBuilderObject* self)
{
    if (!self->data) {
        return 0;
    }
    elementtreestate *st = self->state;
    if (!self->last_for_tail) {
        PyObject *element = self->last;
        return treebuilder_extend_element_text_or_tail(
                st, element, &self->data,
                &((ElementObject *) element)->text, st->str_text);
    }
    else {
        PyObject *element = self->last_for_tail;
        return treebuilder_extend_element_text_or_tail(
                st, element, &self->data,
                &((ElementObject *) element)->tail, st->str_tail);
    }
}

static int
treebuilder_add_subelement(elementtreestate *st, PyObject *element,
                           PyObject *child)
{
    if (Element_CheckExact(st, element)) {
        ElementObject *elem = (ElementObject *) element;
        return element_add_subelement(st, elem, child);
    }
    else {
        PyObject *res;
        res = PyObject_CallMethodOneArg(element, st->str_append, child);
        if (res == NULL)
            return -1;
        Py_DECREF(res);
        return 0;
    }
}

LOCAL(int)
treebuilder_append_event(TreeBuilderObject *self, PyObject *action,
                         PyObject *node)
{
    if (action != NULL) {
        PyObject *res;
        PyObject *event = PyTuple_Pack(2, action, node);
        if (event == NULL)
            return -1;
        res = PyObject_CallOneArg(self->events_append, event);
        Py_DECREF(event);
        if (res == NULL)
            return -1;
        Py_DECREF(res);
    }
    return 0;
}

/* -------------------------------------------------------------------- */
/* handlers */

LOCAL(PyObject*)
treebuilder_handle_start(TreeBuilderObject* self, PyObject* tag,
                         PyObject* attrib)
{
    PyObject* node;
    PyObject* this;
    elementtreestate *st = self->state;

    if (treebuilder_flush_data(self) < 0) {
        return NULL;
    }

    if (!self->element_factory) {
        node = create_new_element(st, tag, attrib);
    }
    else if (attrib == NULL) {
        attrib = PyDict_New();
        if (!attrib)
            return NULL;
        node = PyObject_CallFunctionObjArgs(self->element_factory,
                                            tag, attrib, NULL);
        Py_DECREF(attrib);
    }
    else {
        node = PyObject_CallFunctionObjArgs(self->element_factory,
                                            tag, attrib, NULL);
    }
    if (!node) {
        return NULL;
    }

    this = self->this;
    Py_CLEAR(self->last_for_tail);

    if (this != Py_None) {
        if (treebuilder_add_subelement(st, this, node) < 0) {
            goto error;
        }
    } else {
        if (self->root) {
            PyErr_SetString(
                st->parseerror_obj,
                "multiple elements on top level"
                );
            goto error;
        }
        self->root = Py_NewRef(node);
    }

    if (self->index < PyList_GET_SIZE(self->stack)) {
        if (PyList_SetItem(self->stack, self->index, this) < 0)
            goto error;
        Py_INCREF(this);
    } else {
        if (PyList_Append(self->stack, this) < 0)
            goto error;
    }
    self->index++;

    Py_SETREF(self->this, Py_NewRef(node));
    Py_SETREF(self->last, Py_NewRef(node));

    if (treebuilder_append_event(self, self->start_event_obj, node) < 0)
        goto error;

    return node;

  error:
    Py_DECREF(node);
    return NULL;
}

LOCAL(PyObject*)
treebuilder_handle_data(TreeBuilderObject* self, PyObject* data)
{
    if (!self->data) {
        if (self->last == Py_None) {
            /* ignore calls to data before the first call to start */
            Py_RETURN_NONE;
        }
        /* store the first item as is */
        self->data = Py_NewRef(data);
    } else {
        /* more than one item; use a list to collect items */
        if (PyBytes_CheckExact(self->data) && Py_REFCNT(self->data) == 1 &&
            PyBytes_CheckExact(data) && PyBytes_GET_SIZE(data) == 1) {
            /* XXX this code path unused in Python 3? */
            /* expat often generates single character data sections; handle
               the most common case by resizing the existing string... */
            Py_ssize_t size = PyBytes_GET_SIZE(self->data);
            if (_PyBytes_Resize(&self->data, size + 1) < 0)
                return NULL;
            PyBytes_AS_STRING(self->data)[size] = PyBytes_AS_STRING(data)[0];
        } else if (PyList_CheckExact(self->data)) {
            if (PyList_Append(self->data, data) < 0)
                return NULL;
        } else {
            PyObject* list = PyList_New(2);
            if (!list)
                return NULL;
            PyList_SET_ITEM(list, 0, Py_NewRef(self->data));
            PyList_SET_ITEM(list, 1, Py_NewRef(data));
            Py_SETREF(self->data, list);
        }
    }

    Py_RETURN_NONE;
}

LOCAL(PyObject*)
treebuilder_handle_end(TreeBuilderObject* self, PyObject* tag)
{
    PyObject* item;

    if (treebuilder_flush_data(self) < 0) {
        return NULL;
    }

    if (self->index == 0) {
        PyErr_SetString(
            PyExc_IndexError,
            "pop from empty stack"
            );
        return NULL;
    }

    item = self->last;
    self->last = Py_NewRef(self->this);
    Py_XSETREF(self->last_for_tail, self->last);
    self->index--;
    self->this = Py_NewRef(PyList_GET_ITEM(self->stack, self->index));
    Py_DECREF(item);

    if (treebuilder_append_event(self, self->end_event_obj, self->last) < 0)
        return NULL;

    return Py_NewRef(self->last);
}

LOCAL(PyObject*)
treebuilder_handle_comment(TreeBuilderObject* self, PyObject* text)
{
    PyObject* comment;
    PyObject* this;

    if (treebuilder_flush_data(self) < 0) {
        return NULL;
    }

    if (self->comment_factory) {
        comment = PyObject_CallOneArg(self->comment_factory, text);
        if (!comment)
            return NULL;

        this = self->this;
        if (self->insert_comments && this != Py_None) {
            if (treebuilder_add_subelement(self->state, this, comment) < 0) {
                goto error;
            }
            Py_XSETREF(self->last_for_tail, Py_NewRef(comment));
        }
    } else {
        comment = Py_NewRef(text);
    }

    if (self->events_append && self->comment_event_obj) {
        if (treebuilder_append_event(self, self->comment_event_obj, comment) < 0)
            goto error;
    }

    return comment;

  error:
    Py_DECREF(comment);
    return NULL;
}

LOCAL(PyObject*)
treebuilder_handle_pi(TreeBuilderObject* self, PyObject* target, PyObject* text)
{
    PyObject* pi;
    PyObject* this;

    if (treebuilder_flush_data(self) < 0) {
        return NULL;
    }

    if (self->pi_factory) {
        PyObject* args[2] = {target, text};
        pi = PyObject_Vectorcall(self->pi_factory, args, 2, NULL);
        if (!pi) {
            return NULL;
        }

        this = self->this;
        if (self->insert_pis && this != Py_None) {
            if (treebuilder_add_subelement(self->state, this, pi) < 0) {
                goto error;
            }
            Py_XSETREF(self->last_for_tail, Py_NewRef(pi));
        }
    } else {
        pi = PyTuple_Pack(2, target, text);
        if (!pi) {
            return NULL;
        }
    }

    if (self->events_append && self->pi_event_obj) {
        if (treebuilder_append_event(self, self->pi_event_obj, pi) < 0)
            goto error;
    }

    return pi;

  error:
    Py_DECREF(pi);
    return NULL;
}

LOCAL(PyObject*)
treebuilder_handle_start_ns(TreeBuilderObject* self, PyObject* prefix, PyObject* uri)
{
    PyObject* parcel;

    if (self->events_append && self->start_ns_event_obj) {
        parcel = PyTuple_Pack(2, prefix, uri);
        if (!parcel) {
            return NULL;
        }

        if (treebuilder_append_event(self, self->start_ns_event_obj, parcel) < 0) {
            Py_DECREF(parcel);
            return NULL;
        }
        Py_DECREF(parcel);
    }

    Py_RETURN_NONE;
}

LOCAL(PyObject*)
treebuilder_handle_end_ns(TreeBuilderObject* self, PyObject* prefix)
{
    if (self->events_append && self->end_ns_event_obj) {
        if (treebuilder_append_event(self, self->end_ns_event_obj, prefix) < 0) {
            return NULL;
        }
    }

    Py_RETURN_NONE;
}

/* -------------------------------------------------------------------- */
/* methods (in alphabetical order) */

/*[clinic input]
_elementtree.TreeBuilder.data

    data: object
    /

[clinic start generated code]*/

static PyObject *
_elementtree_TreeBuilder_data(TreeBuilderObject *self, PyObject *data)
/*[clinic end generated code: output=69144c7100795bb2 input=a0540c532b284d29]*/
{
    return treebuilder_handle_data(self, data);
}

/*[clinic input]
_elementtree.TreeBuilder.end

    tag: object
    /

[clinic start generated code]*/

static PyObject *
_elementtree_TreeBuilder_end(TreeBuilderObject *self, PyObject *tag)
/*[clinic end generated code: output=9a98727cc691cd9d input=22dc3674236f5745]*/
{
    return treebuilder_handle_end(self, tag);
}

/*[clinic input]
_elementtree.TreeBuilder.comment

    text: object
    /

[clinic start generated code]*/

static PyObject *
_elementtree_TreeBuilder_comment(TreeBuilderObject *self, PyObject *text)
/*[clinic end generated code: output=22835be41deeaa27 input=47e7ebc48ed01dfa]*/
{
    return treebuilder_handle_comment(self, text);
}

/*[clinic input]
_elementtree.TreeBuilder.pi

    target: object
    text: object = None
    /

[clinic start generated code]*/

static PyObject *
_elementtree_TreeBuilder_pi_impl(TreeBuilderObject *self, PyObject *target,
                                 PyObject *text)
/*[clinic end generated code: output=21eb95ec9d04d1d9 input=349342bd79c35570]*/
{
    return treebuilder_handle_pi(self, target, text);
}

LOCAL(PyObject*)
treebuilder_done(TreeBuilderObject* self)
{
    PyObject* res;

    /* FIXME: check stack size? */

    if (self->root)
        res = self->root;
    else
        res = Py_None;

    return Py_NewRef(res);
}

/*[clinic input]
_elementtree.TreeBuilder.close

[clinic start generated code]*/

static PyObject *
_elementtree_TreeBuilder_close_impl(TreeBuilderObject *self)
/*[clinic end generated code: output=b441fee3202f61ee input=f7c9c65dc718de14]*/
{
    return treebuilder_done(self);
}

/*[clinic input]
_elementtree.TreeBuilder.start

    tag: object
    attrs: object(subclass_of='&PyDict_Type')
    /

[clinic start generated code]*/

static PyObject *
_elementtree_TreeBuilder_start_impl(TreeBuilderObject *self, PyObject *tag,
                                    PyObject *attrs)
/*[clinic end generated code: output=e7e9dc2861349411 input=7288e9e38e63b2b6]*/
{
    return treebuilder_handle_start(self, tag, attrs);
}

/* ==================================================================== */
/* the expat interface */

#define EXPAT(st, func) ((st)->expat_capi->func)

static XML_Memory_Handling_Suite ExpatMemoryHandler = {
    PyObject_Malloc, PyObject_Realloc, PyObject_Free};

typedef struct {
    PyObject_HEAD

    XML_Parser parser;

    PyObject *target;
    PyObject *entity;

    PyObject *names;

    PyObject *handle_start_ns;
    PyObject *handle_end_ns;
    PyObject *handle_start;
    PyObject *handle_data;
    PyObject *handle_end;

    PyObject *handle_comment;
    PyObject *handle_pi;
    PyObject *handle_doctype;

    PyObject *handle_close;

    elementtreestate *state;
    PyObject *elementtree_module;
} XMLParserObject;

/* helpers */

LOCAL(PyObject*)
makeuniversal(XMLParserObject* self, const char* string)
{
    /* convert a UTF-8 tag/attribute name from the expat parser
       to a universal name string */

    Py_ssize_t size = (Py_ssize_t) strlen(string);
    PyObject* key;
    PyObject* value;

    /* look the 'raw' name up in the names dictionary */
    key = PyBytes_FromStringAndSize(string, size);
    if (!key)
        return NULL;

    value = Py_XNewRef(PyDict_GetItemWithError(self->names, key));

    if (value == NULL && !PyErr_Occurred()) {
        /* new name.  convert to universal name, and decode as
           necessary */

        PyObject* tag;
        char* p;
        Py_ssize_t i;

        /* look for namespace separator */
        for (i = 0; i < size; i++)
            if (string[i] == '}')
                break;
        if (i != size) {
            /* convert to universal name */
            tag = PyBytes_FromStringAndSize(NULL, size+1);
            if (tag == NULL) {
                Py_DECREF(key);
                return NULL;
            }
            p = PyBytes_AS_STRING(tag);
            p[0] = '{';
            memcpy(p+1, string, size);
            size++;
        } else {
            /* plain name; use key as tag */
            tag = Py_NewRef(key);
        }

        /* decode universal name */
        p = PyBytes_AS_STRING(tag);
        value = PyUnicode_DecodeUTF8(p, size, "strict");
        Py_DECREF(tag);
        if (!value) {
            Py_DECREF(key);
            return NULL;
        }

        /* add to names dictionary */
        if (PyDict_SetItem(self->names, key, value) < 0) {
            Py_DECREF(key);
            Py_DECREF(value);
            return NULL;
        }
    }

    Py_DECREF(key);
    return value;
}

/* Set the ParseError exception with the given parameters.
 * If message is not NULL, it's used as the error string. Otherwise, the
 * message string is the default for the given error_code.
*/
static void
expat_set_error(elementtreestate *st, enum XML_Error error_code,
                Py_ssize_t line, Py_ssize_t column, const char *message)
{
    PyObject *errmsg, *error, *position, *code;

    errmsg = PyUnicode_FromFormat("%s: line %zd, column %zd",
                message ? message : EXPAT(st, ErrorString)(error_code),
                line, column);
    if (errmsg == NULL)
        return;

    error = PyObject_CallOneArg(st->parseerror_obj, errmsg);
    Py_DECREF(errmsg);
    if (!error)
        return;

    /* Add code and position attributes */
    code = PyLong_FromLong((long)error_code);
    if (!code) {
        Py_DECREF(error);
        return;
    }
    if (PyObject_SetAttrString(error, "code", code) == -1) {
        Py_DECREF(error);
        Py_DECREF(code);
        return;
    }
    Py_DECREF(code);

    position = Py_BuildValue("(nn)", line, column);
    if (!position) {
        Py_DECREF(error);
        return;
    }
    if (PyObject_SetAttrString(error, "position", position) == -1) {
        Py_DECREF(error);
        Py_DECREF(position);
        return;
    }
    Py_DECREF(position);

    PyErr_SetObject(st->parseerror_obj, error);
    Py_DECREF(error);
}

/* -------------------------------------------------------------------- */
/* handlers */

static void
expat_default_handler(XMLParserObject* self, const XML_Char* data_in,
                      int data_len)
{
    PyObject* key;
    PyObject* value;
    PyObject* res;

    if (data_len < 2 || data_in[0] != '&')
        return;

    if (PyErr_Occurred())
        return;

    key = PyUnicode_DecodeUTF8(data_in + 1, data_len - 2, "strict");
    if (!key)
        return;

    value = PyDict_GetItemWithError(self->entity, key);

    elementtreestate *st = self->state;
    if (value) {
        if (TreeBuilder_CheckExact(st, self->target))
            res = treebuilder_handle_data(
                (TreeBuilderObject*) self->target, value
                );
        else if (self->handle_data)
            res = PyObject_CallOneArg(self->handle_data, value);
        else
            res = NULL;
        Py_XDECREF(res);
    } else if (!PyErr_Occurred()) {
        /* Report the first error, not the last */
        char message[128] = "undefined entity ";
        strncat(message, data_in, data_len < 100?data_len:100);
        expat_set_error(
            st,
            XML_ERROR_UNDEFINED_ENTITY,
            EXPAT(st, GetErrorLineNumber)(self->parser),
            EXPAT(st, GetErrorColumnNumber)(self->parser),
            message
            );
    }

    Py_DECREF(key);
}

static void
expat_start_handler(XMLParserObject* self, const XML_Char* tag_in,
                    const XML_Char **attrib_in)
{
    PyObject* res;
    PyObject* tag;
    PyObject* attrib;
    int ok;

    if (PyErr_Occurred())
        return;

    /* tag name */
    tag = makeuniversal(self, tag_in);
    if (!tag)
        return; /* parser will look for errors */

    /* attributes */
    if (attrib_in[0]) {
        attrib = PyDict_New();
        if (!attrib) {
            Py_DECREF(tag);
            return;
        }
        while (attrib_in[0] && attrib_in[1]) {
            PyObject* key = makeuniversal(self, attrib_in[0]);
            if (key == NULL) {
                Py_DECREF(attrib);
                Py_DECREF(tag);
                return;
            }
            PyObject* value = PyUnicode_DecodeUTF8(attrib_in[1], strlen(attrib_in[1]), "strict");
            if (value == NULL) {
                Py_DECREF(key);
                Py_DECREF(attrib);
                Py_DECREF(tag);
                return;
            }
            ok = PyDict_SetItem(attrib, key, value);
            Py_DECREF(value);
            Py_DECREF(key);
            if (ok < 0) {
                Py_DECREF(attrib);
                Py_DECREF(tag);
                return;
            }
            attrib_in += 2;
        }
    } else {
        attrib = NULL;
    }

    elementtreestate *st = self->state;
    if (TreeBuilder_CheckExact(st, self->target)) {
        /* shortcut */
        res = treebuilder_handle_start((TreeBuilderObject*) self->target,
                                       tag, attrib);
    }
    else if (self->handle_start) {
        if (attrib == NULL) {
            attrib = PyDict_New();
            if (!attrib) {
                Py_DECREF(tag);
                return;
            }
        }
        res = PyObject_CallFunctionObjArgs(self->handle_start,
                                           tag, attrib, NULL);
    } else
        res = NULL;

    Py_DECREF(tag);
    Py_XDECREF(attrib);

    Py_XDECREF(res);
}

static void
expat_data_handler(XMLParserObject* self, const XML_Char* data_in,
                   int data_len)
{
    PyObject* data;
    PyObject* res;

    if (PyErr_Occurred())
        return;

    data = PyUnicode_DecodeUTF8(data_in, data_len, "strict");
    if (!data)
        return; /* parser will look for errors */

    elementtreestate *st = self->state;
    if (TreeBuilder_CheckExact(st, self->target))
        /* shortcut */
        res = treebuilder_handle_data((TreeBuilderObject*) self->target, data);
    else if (self->handle_data)
        res = PyObject_CallOneArg(self->handle_data, data);
    else
        res = NULL;

    Py_DECREF(data);

    Py_XDECREF(res);
}

static void
expat_end_handler(XMLParserObject* self, const XML_Char* tag_in)
{
    PyObject* tag;
    PyObject* res = NULL;

    if (PyErr_Occurred())
        return;

    elementtreestate *st = self->state;
    if (TreeBuilder_CheckExact(st, self->target))
        /* shortcut */
        /* the standard tree builder doesn't look at the end tag */
        res = treebuilder_handle_end(
            (TreeBuilderObject*) self->target, Py_None
            );
    else if (self->handle_end) {
        tag = makeuniversal(self, tag_in);
        if (tag) {
            res = PyObject_CallOneArg(self->handle_end, tag);
            Py_DECREF(tag);
        }
    }

    Py_XDECREF(res);
}

static void
expat_start_ns_handler(XMLParserObject* self, const XML_Char* prefix_in,
                       const XML_Char *uri_in)
{
    PyObject* res = NULL;
    PyObject* uri;
    PyObject* prefix;

    if (PyErr_Occurred())
        return;

    if (!uri_in)
        uri_in = "";
    if (!prefix_in)
        prefix_in = "";

    elementtreestate *st = self->state;
    if (TreeBuilder_CheckExact(st, self->target)) {
        /* shortcut - TreeBuilder does not actually implement .start_ns() */
        TreeBuilderObject *target = (TreeBuilderObject*) self->target;

        if (target->events_append && target->start_ns_event_obj) {
            prefix = PyUnicode_DecodeUTF8(prefix_in, strlen(prefix_in), "strict");
            if (!prefix)
                return;
            uri = PyUnicode_DecodeUTF8(uri_in, strlen(uri_in), "strict");
            if (!uri) {
                Py_DECREF(prefix);
                return;
            }

            res = treebuilder_handle_start_ns(target, prefix, uri);
            Py_DECREF(uri);
            Py_DECREF(prefix);
        }
    } else if (self->handle_start_ns) {
        prefix = PyUnicode_DecodeUTF8(prefix_in, strlen(prefix_in), "strict");
        if (!prefix)
            return;
        uri = PyUnicode_DecodeUTF8(uri_in, strlen(uri_in), "strict");
        if (!uri) {
            Py_DECREF(prefix);
            return;
        }

        PyObject* args[2] = {prefix, uri};
        res = PyObject_Vectorcall(self->handle_start_ns, args, 2, NULL);
        Py_DECREF(uri);
        Py_DECREF(prefix);
    }

    Py_XDECREF(res);
}

static void
expat_end_ns_handler(XMLParserObject* self, const XML_Char* prefix_in)
{
    PyObject *res = NULL;
    PyObject* prefix;

    if (PyErr_Occurred())
        return;

    if (!prefix_in)
        prefix_in = "";

    elementtreestate *st = self->state;
    if (TreeBuilder_CheckExact(st, self->target)) {
        /* shortcut - TreeBuilder does not actually implement .end_ns() */
        TreeBuilderObject *target = (TreeBuilderObject*) self->target;

        if (target->events_append && target->end_ns_event_obj) {
            res = treebuilder_handle_end_ns(target, Py_None);
        }
    } else if (self->handle_end_ns) {
        prefix = PyUnicode_DecodeUTF8(prefix_in, strlen(prefix_in), "strict");
        if (!prefix)
            return;

        res = PyObject_CallOneArg(self->handle_end_ns, prefix);
        Py_DECREF(prefix);
    }

    Py_XDECREF(res);
}

static void
expat_comment_handler(XMLParserObject* self, const XML_Char* comment_in)
{
    PyObject* comment;
    PyObject* res;

    if (PyErr_Occurred())
        return;

    elementtreestate *st = self->state;
    if (TreeBuilder_CheckExact(st, self->target)) {
        /* shortcut */
        TreeBuilderObject *target = (TreeBuilderObject*) self->target;

        comment = PyUnicode_DecodeUTF8(comment_in, strlen(comment_in), "strict");
        if (!comment)
            return; /* parser will look for errors */

        res = treebuilder_handle_comment(target,  comment);
        Py_XDECREF(res);
        Py_DECREF(comment);
    } else if (self->handle_comment) {
        comment = PyUnicode_DecodeUTF8(comment_in, strlen(comment_in), "strict");
        if (!comment)
            return;

        res = PyObject_CallOneArg(self->handle_comment, comment);
        Py_XDECREF(res);
        Py_DECREF(comment);
    }
}

static void
expat_start_doctype_handler(XMLParserObject *self,
                            const XML_Char *doctype_name,
                            const XML_Char *sysid,
                            const XML_Char *pubid,
                            int has_internal_subset)
{
    PyObject *doctype_name_obj, *sysid_obj, *pubid_obj;
    PyObject *res;

    if (PyErr_Occurred())
        return;

    doctype_name_obj = makeuniversal(self, doctype_name);
    if (!doctype_name_obj)
        return;

    if (sysid) {
        sysid_obj = makeuniversal(self, sysid);
        if (!sysid_obj) {
            Py_DECREF(doctype_name_obj);
            return;
        }
    } else {
        sysid_obj = Py_NewRef(Py_None);
    }

    if (pubid) {
        pubid_obj = makeuniversal(self, pubid);
        if (!pubid_obj) {
            Py_DECREF(doctype_name_obj);
            Py_DECREF(sysid_obj);
            return;
        }
    } else {
        pubid_obj = Py_NewRef(Py_None);
    }

    elementtreestate *st = self->state;
    /* If the target has a handler for doctype, call it. */
    if (self->handle_doctype) {
        res = PyObject_CallFunctionObjArgs(self->handle_doctype,
                                           doctype_name_obj, pubid_obj,
                                           sysid_obj, NULL);
        Py_XDECREF(res);
    }
    else if (PyObject_HasAttrWithError((PyObject *)self, st->str_doctype) > 0) {
        (void)PyErr_WarnEx(PyExc_RuntimeWarning,
                "The doctype() method of XMLParser is ignored.  "
                "Define doctype() method on the TreeBuilder target.",
                1);
    }

    Py_DECREF(doctype_name_obj);
    Py_DECREF(pubid_obj);
    Py_DECREF(sysid_obj);
}

static void
expat_pi_handler(XMLParserObject* self, const XML_Char* target_in,
                 const XML_Char* data_in)
{
    PyObject* pi_target;
    PyObject* data;
    PyObject* res;

    if (PyErr_Occurred())
        return;

    elementtreestate *st = self->state;
    if (TreeBuilder_CheckExact(st, self->target)) {
        /* shortcut */
        TreeBuilderObject *target = (TreeBuilderObject*) self->target;

        if ((target->events_append && target->pi_event_obj) || target->insert_pis) {
            pi_target = PyUnicode_DecodeUTF8(target_in, strlen(target_in), "strict");
            if (!pi_target)
                goto error;
            data = PyUnicode_DecodeUTF8(data_in, strlen(data_in), "strict");
            if (!data)
                goto error;
            res = treebuilder_handle_pi(target, pi_target, data);
            Py_XDECREF(res);
            Py_DECREF(data);
            Py_DECREF(pi_target);
        }
    } else if (self->handle_pi) {
        pi_target = PyUnicode_DecodeUTF8(target_in, strlen(target_in), "strict");
        if (!pi_target)
            goto error;
        data = PyUnicode_DecodeUTF8(data_in, strlen(data_in), "strict");
        if (!data)
            goto error;

        PyObject* args[2] = {pi_target, data};
        res = PyObject_Vectorcall(self->handle_pi, args, 2, NULL);
        Py_XDECREF(res);
        Py_DECREF(data);
        Py_DECREF(pi_target);
    }

    return;

  error:
    Py_XDECREF(pi_target);
    return;
}

/* -------------------------------------------------------------------- */

static PyObject *
xmlparser_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    XMLParserObject *self = (XMLParserObject *)type->tp_alloc(type, 0);
    if (self) {
        self->parser = NULL;
        self->target = self->entity = self->names = NULL;
        self->handle_start_ns = self->handle_end_ns = NULL;
        self->handle_start = self->handle_data = self->handle_end = NULL;
        self->handle_comment = self->handle_pi = self->handle_close = NULL;
        self->handle_doctype = NULL;
        self->elementtree_module = PyType_GetModuleByDef(type, &elementtreemodule);
        assert(self->elementtree_module != NULL);
        Py_INCREF(self->elementtree_module);
        // See gh-111784 for explanation why is reference to module needed here.
        self->state = get_elementtree_state(self->elementtree_module);
    }
    return (PyObject *)self;
}

static int
ignore_attribute_error(PyObject *value)
{
    if (value == NULL) {
        if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
            return -1;
        }
        PyErr_Clear();
    }
    return 0;
}

/*[clinic input]
_elementtree.XMLParser.__init__

    *
    target: object = None
    encoding: str(accept={str, NoneType}) = None

[clinic start generated code]*/

static int
_elementtree_XMLParser___init___impl(XMLParserObject *self, PyObject *target,
                                     const char *encoding)
/*[clinic end generated code: output=3ae45ec6cdf344e4 input=7e716dd6e4f3e439]*/
{
    self->entity = PyDict_New();
    if (!self->entity)
        return -1;

    self->names = PyDict_New();
    if (!self->names) {
        Py_CLEAR(self->entity);
        return -1;
    }
    elementtreestate *st = self->state;
    self->parser = EXPAT(st, ParserCreate_MM)(encoding, &ExpatMemoryHandler, "}");
    if (!self->parser) {
        Py_CLEAR(self->entity);
        Py_CLEAR(self->names);
        PyErr_NoMemory();
        return -1;
    }
    /* expat < 2.1.0 has no XML_SetHashSalt() */
    if (EXPAT(st, SetHashSalt) != NULL) {
        EXPAT(st, SetHashSalt)(self->parser,
                           (unsigned long)_Py_HashSecret.expat.hashsalt);
    }

    if (target != Py_None) {
        Py_INCREF(target);
    } else {
        target = treebuilder_new(st->TreeBuilder_Type, NULL, NULL);
        if (!target) {
            Py_CLEAR(self->entity);
            Py_CLEAR(self->names);
            return -1;
        }
    }
    self->target = target;

    self->handle_start_ns = PyObject_GetAttrString(target, "start_ns");
    if (ignore_attribute_error(self->handle_start_ns)) {
        return -1;
    }
    self->handle_end_ns = PyObject_GetAttrString(target, "end_ns");
    if (ignore_attribute_error(self->handle_end_ns)) {
        return -1;
    }
    self->handle_start = PyObject_GetAttrString(target, "start");
    if (ignore_attribute_error(self->handle_start)) {
        return -1;
    }
    self->handle_data = PyObject_GetAttrString(target, "data");
    if (ignore_attribute_error(self->handle_data)) {
        return -1;
    }
    self->handle_end = PyObject_GetAttrString(target, "end");
    if (ignore_attribute_error(self->handle_end)) {
        return -1;
    }
    self->handle_comment = PyObject_GetAttrString(target, "comment");
    if (ignore_attribute_error(self->handle_comment)) {
        return -1;
    }
    self->handle_pi = PyObject_GetAttrString(target, "pi");
    if (ignore_attribute_error(self->handle_pi)) {
        return -1;
    }
    self->handle_close = PyObject_GetAttrString(target, "close");
    if (ignore_attribute_error(self->handle_close)) {
        return -1;
    }
    self->handle_doctype = PyObject_GetAttrString(target, "doctype");
    if (ignore_attribute_error(self->handle_doctype)) {
        return -1;
    }

    /* configure parser */
    EXPAT(st, SetUserData)(self->parser, self);
    if (self->handle_start_ns || self->handle_end_ns)
        EXPAT(st, SetNamespaceDeclHandler)(
            self->parser,
            (XML_StartNamespaceDeclHandler) expat_start_ns_handler,
            (XML_EndNamespaceDeclHandler) expat_end_ns_handler
            );
    EXPAT(st, SetElementHandler)(
        self->parser,
        (XML_StartElementHandler) expat_start_handler,
        (XML_EndElementHandler) expat_end_handler
        );
    EXPAT(st, SetDefaultHandlerExpand)(
        self->parser,
        (XML_DefaultHandler) expat_default_handler
        );
    EXPAT(st, SetCharacterDataHandler)(
        self->parser,
        (XML_CharacterDataHandler) expat_data_handler
        );
    if (self->handle_comment)
        EXPAT(st, SetCommentHandler)(
            self->parser,
            (XML_CommentHandler) expat_comment_handler
            );
    if (self->handle_pi)
        EXPAT(st, SetProcessingInstructionHandler)(
            self->parser,
            (XML_ProcessingInstructionHandler) expat_pi_handler
            );
    EXPAT(st, SetStartDoctypeDeclHandler)(
        self->parser,
        (XML_StartDoctypeDeclHandler) expat_start_doctype_handler
        );
    EXPAT(st, SetUnknownEncodingHandler)(
        self->parser,
        EXPAT(st, DefaultUnknownEncodingHandler), NULL
        );

    return 0;
}

static int
xmlparser_gc_traverse(XMLParserObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->handle_close);
    Py_VISIT(self->handle_pi);
    Py_VISIT(self->handle_comment);
    Py_VISIT(self->handle_end);
    Py_VISIT(self->handle_data);
    Py_VISIT(self->handle_start);
    Py_VISIT(self->handle_start_ns);
    Py_VISIT(self->handle_end_ns);
    Py_VISIT(self->handle_doctype);

    Py_VISIT(self->target);
    Py_VISIT(self->entity);
    Py_VISIT(self->names);

    return 0;
}

static int
xmlparser_gc_clear(XMLParserObject *self)
{
    elementtreestate *st = self->state;
    if (self->parser != NULL) {
        XML_Parser parser = self->parser;
        self->parser = NULL;
        EXPAT(st, ParserFree)(parser);
    }

    Py_CLEAR(self->elementtree_module);
    Py_CLEAR(self->handle_close);
    Py_CLEAR(self->handle_pi);
    Py_CLEAR(self->handle_comment);
    Py_CLEAR(self->handle_end);
    Py_CLEAR(self->handle_data);
    Py_CLEAR(self->handle_start);
    Py_CLEAR(self->handle_start_ns);
    Py_CLEAR(self->handle_end_ns);
    Py_CLEAR(self->handle_doctype);

    Py_CLEAR(self->target);
    Py_CLEAR(self->entity);
    Py_CLEAR(self->names);

    return 0;
}

static void
xmlparser_dealloc(XMLParserObject* self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    xmlparser_gc_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

Py_LOCAL_INLINE(int)
_check_xmlparser(XMLParserObject* self)
{
    if (self->target == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "XMLParser.__init__() wasn't called");
        return 0;
    }
    return 1;
}

LOCAL(PyObject*)
expat_parse(elementtreestate *st, XMLParserObject *self, const char *data,
            int data_len, int final)
{
    int ok;

    assert(!PyErr_Occurred());
    ok = EXPAT(st, Parse)(self->parser, data, data_len, final);

    if (PyErr_Occurred())
        return NULL;

    if (!ok) {
        expat_set_error(
            st,
            EXPAT(st, GetErrorCode)(self->parser),
            EXPAT(st, GetErrorLineNumber)(self->parser),
            EXPAT(st, GetErrorColumnNumber)(self->parser),
            NULL
            );
        return NULL;
    }

    Py_RETURN_NONE;
}

/*[clinic input]
_elementtree.XMLParser.close

[clinic start generated code]*/

static PyObject *
_elementtree_XMLParser_close_impl(XMLParserObject *self)
/*[clinic end generated code: output=d68d375dd23bc7fb input=ca7909ca78c3abfe]*/
{
    /* end feeding data to parser */

    PyObject* res;

    if (!_check_xmlparser(self)) {
        return NULL;
    }
    elementtreestate *st = self->state;
    res = expat_parse(st, self, "", 0, 1);
    if (!res)
        return NULL;

    if (TreeBuilder_CheckExact(st, self->target)) {
        Py_DECREF(res);
        return treebuilder_done((TreeBuilderObject*) self->target);
    }
    else if (self->handle_close) {
        Py_DECREF(res);
        return PyObject_CallNoArgs(self->handle_close);
    }
    else {
        return res;
    }
}

/*[clinic input]
_elementtree.XMLParser.feed

    data: object
    /

[clinic start generated code]*/

static PyObject *
_elementtree_XMLParser_feed(XMLParserObject *self, PyObject *data)
/*[clinic end generated code: output=e42b6a78eec7446d input=fe231b6b8de3ce1f]*/
{
    /* feed data to parser */

    if (!_check_xmlparser(self)) {
        return NULL;
    }
    elementtreestate *st = self->state;
    if (PyUnicode_Check(data)) {
        Py_ssize_t data_len;
        const char *data_ptr = PyUnicode_AsUTF8AndSize(data, &data_len);
        if (data_ptr == NULL)
            return NULL;
        if (data_len > INT_MAX) {
            PyErr_SetString(PyExc_OverflowError, "size does not fit in an int");
            return NULL;
        }
        /* Explicitly set UTF-8 encoding. Return code ignored. */
        (void)EXPAT(st, SetEncoding)(self->parser, "utf-8");

        return expat_parse(st, self, data_ptr, (int)data_len, 0);
    }
    else {
        Py_buffer view;
        PyObject *res;
        if (PyObject_GetBuffer(data, &view, PyBUF_SIMPLE) < 0)
            return NULL;
        if (view.len > INT_MAX) {
            PyBuffer_Release(&view);
            PyErr_SetString(PyExc_OverflowError, "size does not fit in an int");
            return NULL;
        }
        res = expat_parse(st, self, view.buf, (int)view.len, 0);
        PyBuffer_Release(&view);
        return res;
    }
}

/*[clinic input]
_elementtree.XMLParser._parse_whole

    file: object
    /

[clinic start generated code]*/

static PyObject *
_elementtree_XMLParser__parse_whole(XMLParserObject *self, PyObject *file)
/*[clinic end generated code: output=f797197bb818dda3 input=19ecc893b6f3e752]*/
{
    /* (internal) parse the whole input, until end of stream */
    PyObject* reader;
    PyObject* buffer;
    PyObject* temp;
    PyObject* res;

    if (!_check_xmlparser(self)) {
        return NULL;
    }
    reader = PyObject_GetAttrString(file, "read");
    if (!reader)
        return NULL;

    /* read from open file object */
    elementtreestate *st = self->state;
    for (;;) {

        buffer = PyObject_CallFunction(reader, "i", 64*1024);

        if (!buffer) {
            /* read failed (e.g. due to KeyboardInterrupt) */
            Py_DECREF(reader);
            return NULL;
        }

        if (PyUnicode_CheckExact(buffer)) {
            /* A unicode object is encoded into bytes using UTF-8 */
            if (PyUnicode_GET_LENGTH(buffer) == 0) {
                Py_DECREF(buffer);
                break;
            }
            temp = PyUnicode_AsEncodedString(buffer, "utf-8", "surrogatepass");
            Py_DECREF(buffer);
            if (!temp) {
                /* Propagate exception from PyUnicode_AsEncodedString */
                Py_DECREF(reader);
                return NULL;
            }
            buffer = temp;
        }
        else if (!PyBytes_CheckExact(buffer) || PyBytes_GET_SIZE(buffer) == 0) {
            Py_DECREF(buffer);
            break;
        }

        if (PyBytes_GET_SIZE(buffer) > INT_MAX) {
            Py_DECREF(buffer);
            Py_DECREF(reader);
            PyErr_SetString(PyExc_OverflowError, "size does not fit in an int");
            return NULL;
        }
        res = expat_parse(
            st, self, PyBytes_AS_STRING(buffer), (int)PyBytes_GET_SIZE(buffer),
            0);

        Py_DECREF(buffer);

        if (!res) {
            Py_DECREF(reader);
            return NULL;
        }
        Py_DECREF(res);

    }

    Py_DECREF(reader);

    res = expat_parse(st, self, "", 0, 1);

    if (res && TreeBuilder_CheckExact(st, self->target)) {
        Py_DECREF(res);
        return treebuilder_done((TreeBuilderObject*) self->target);
    }

    return res;
}

/*[clinic input]
_elementtree.XMLParser._setevents

    events_queue: object
    events_to_report: object = None
    /

[clinic start generated code]*/

static PyObject *
_elementtree_XMLParser__setevents_impl(XMLParserObject *self,
                                       PyObject *events_queue,
                                       PyObject *events_to_report)
/*[clinic end generated code: output=1440092922b13ed1 input=abf90830a1c3b0fc]*/
{
    /* activate element event reporting */
    Py_ssize_t i;
    TreeBuilderObject *target;
    PyObject *events_append, *events_seq;

    if (!_check_xmlparser(self)) {
        return NULL;
    }
    elementtreestate *st = self->state;
    if (!TreeBuilder_CheckExact(st, self->target)) {
        PyErr_SetString(
            PyExc_TypeError,
            "event handling only supported for ElementTree.TreeBuilder "
            "targets"
            );
        return NULL;
    }

    target = (TreeBuilderObject*) self->target;

    events_append = PyObject_GetAttrString(events_queue, "append");
    if (events_append == NULL)
        return NULL;
    Py_XSETREF(target->events_append, events_append);

    /* clear out existing events */
    Py_CLEAR(target->start_event_obj);
    Py_CLEAR(target->end_event_obj);
    Py_CLEAR(target->start_ns_event_obj);
    Py_CLEAR(target->end_ns_event_obj);
    Py_CLEAR(target->comment_event_obj);
    Py_CLEAR(target->pi_event_obj);

    if (events_to_report == Py_None) {
        /* default is "end" only */
        target->end_event_obj = PyUnicode_FromString("end");
        Py_RETURN_NONE;
    }

    if (!(events_seq = PySequence_Fast(events_to_report,
                                       "events must be a sequence"))) {
        return NULL;
    }

    for (i = 0; i < PySequence_Fast_GET_SIZE(events_seq); ++i) {
        PyObject *event_name_obj = PySequence_Fast_GET_ITEM(events_seq, i);
        const char *event_name = NULL;
        if (PyUnicode_Check(event_name_obj)) {
            event_name = PyUnicode_AsUTF8(event_name_obj);
        } else if (PyBytes_Check(event_name_obj)) {
            event_name = PyBytes_AS_STRING(event_name_obj);
        }
        if (event_name == NULL) {
            Py_DECREF(events_seq);
            PyErr_Format(PyExc_ValueError, "invalid events sequence");
            return NULL;
        }

        if (strcmp(event_name, "start") == 0) {
            Py_XSETREF(target->start_event_obj, Py_NewRef(event_name_obj));
        } else if (strcmp(event_name, "end") == 0) {
            Py_XSETREF(target->end_event_obj, Py_NewRef(event_name_obj));
        } else if (strcmp(event_name, "start-ns") == 0) {
            Py_XSETREF(target->start_ns_event_obj, Py_NewRef(event_name_obj));
            EXPAT(st, SetNamespaceDeclHandler)(
                self->parser,
                (XML_StartNamespaceDeclHandler) expat_start_ns_handler,
                (XML_EndNamespaceDeclHandler) expat_end_ns_handler
                );
        } else if (strcmp(event_name, "end-ns") == 0) {
            Py_XSETREF(target->end_ns_event_obj, Py_NewRef(event_name_obj));
            EXPAT(st, SetNamespaceDeclHandler)(
                self->parser,
                (XML_StartNamespaceDeclHandler) expat_start_ns_handler,
                (XML_EndNamespaceDeclHandler) expat_end_ns_handler
                );
        } else if (strcmp(event_name, "comment") == 0) {
            Py_XSETREF(target->comment_event_obj, Py_NewRef(event_name_obj));
            EXPAT(st, SetCommentHandler)(
                self->parser,
                (XML_CommentHandler) expat_comment_handler
                );
        } else if (strcmp(event_name, "pi") == 0) {
            Py_XSETREF(target->pi_event_obj, Py_NewRef(event_name_obj));
            EXPAT(st, SetProcessingInstructionHandler)(
                self->parser,
                (XML_ProcessingInstructionHandler) expat_pi_handler
                );
        } else {
            Py_DECREF(events_seq);
            PyErr_Format(PyExc_ValueError, "unknown event '%s'", event_name);
            return NULL;
        }
    }

    Py_DECREF(events_seq);
    Py_RETURN_NONE;
}

static PyMemberDef xmlparser_members[] = {
    {"entity", _Py_T_OBJECT, offsetof(XMLParserObject, entity), Py_READONLY, NULL},
    {"target", _Py_T_OBJECT, offsetof(XMLParserObject, target), Py_READONLY, NULL},
    {NULL}
};

static PyObject*
xmlparser_version_getter(XMLParserObject *self, void *closure)
{
    return PyUnicode_FromFormat(
        "Expat %d.%d.%d", XML_MAJOR_VERSION,
        XML_MINOR_VERSION, XML_MICRO_VERSION);
}

static PyGetSetDef xmlparser_getsetlist[] = {
    {"version", (getter)xmlparser_version_getter, NULL, NULL},
    {NULL},
};

#define clinic_state() (get_elementtree_state_by_type(Py_TYPE(self)))
#include "clinic/_elementtree.c.h"
#undef clinic_state

static PyMethodDef element_methods[] = {

    _ELEMENTTREE_ELEMENT_CLEAR_METHODDEF

    _ELEMENTTREE_ELEMENT_GET_METHODDEF
    _ELEMENTTREE_ELEMENT_SET_METHODDEF

    _ELEMENTTREE_ELEMENT_FIND_METHODDEF
    _ELEMENTTREE_ELEMENT_FINDTEXT_METHODDEF
    _ELEMENTTREE_ELEMENT_FINDALL_METHODDEF

    _ELEMENTTREE_ELEMENT_APPEND_METHODDEF
    _ELEMENTTREE_ELEMENT_EXTEND_METHODDEF
    _ELEMENTTREE_ELEMENT_INSERT_METHODDEF
    _ELEMENTTREE_ELEMENT_REMOVE_METHODDEF

    _ELEMENTTREE_ELEMENT_ITER_METHODDEF
    _ELEMENTTREE_ELEMENT_ITERTEXT_METHODDEF
    _ELEMENTTREE_ELEMENT_ITERFIND_METHODDEF

    _ELEMENTTREE_ELEMENT_ITEMS_METHODDEF
    _ELEMENTTREE_ELEMENT_KEYS_METHODDEF

    _ELEMENTTREE_ELEMENT_MAKEELEMENT_METHODDEF

    _ELEMENTTREE_ELEMENT___COPY___METHODDEF
    _ELEMENTTREE_ELEMENT___DEEPCOPY___METHODDEF
    _ELEMENTTREE_ELEMENT___SIZEOF___METHODDEF
    _ELEMENTTREE_ELEMENT___GETSTATE___METHODDEF
    _ELEMENTTREE_ELEMENT___SETSTATE___METHODDEF

    {NULL, NULL}
};

static struct PyMemberDef element_members[] = {
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(ElementObject, weakreflist), Py_READONLY},
    {NULL},
};

static PyGetSetDef element_getsetlist[] = {
    {"tag",
        (getter)element_tag_getter,
        (setter)element_tag_setter,
        "A string identifying what kind of data this element represents"},
    {"text",
        (getter)element_text_getter,
        (setter)element_text_setter,
        "A string of text directly after the start tag, or None"},
    {"tail",
        (getter)element_tail_getter,
        (setter)element_tail_setter,
        "A string of text directly after the end tag, or None"},
    {"attrib",
        (getter)element_attrib_getter,
        (setter)element_attrib_setter,
        "A dictionary containing the element's attributes"},
    {NULL},
};

static PyType_Slot element_slots[] = {
    {Py_tp_dealloc, element_dealloc},
    {Py_tp_repr, element_repr},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_traverse, element_gc_traverse},
    {Py_tp_clear, element_gc_clear},
    {Py_tp_methods, element_methods},
    {Py_tp_members, element_members},
    {Py_tp_getset, element_getsetlist},
    {Py_tp_init, element_init},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_new, element_new},
    {Py_sq_length, element_length},
    {Py_sq_item, element_getitem},
    {Py_sq_ass_item, element_setitem},
    {Py_nb_bool, element_bool},
    {Py_mp_length, element_length},
    {Py_mp_subscript, element_subscr},
    {Py_mp_ass_subscript, element_ass_subscr},
    {0, NULL},
};

static PyType_Spec element_spec = {
    .name = "xml.etree.ElementTree.Element",
    .basicsize = sizeof(ElementObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = element_slots,
};

static PyMethodDef treebuilder_methods[] = {
    _ELEMENTTREE_TREEBUILDER_DATA_METHODDEF
    _ELEMENTTREE_TREEBUILDER_START_METHODDEF
    _ELEMENTTREE_TREEBUILDER_END_METHODDEF
    _ELEMENTTREE_TREEBUILDER_COMMENT_METHODDEF
    _ELEMENTTREE_TREEBUILDER_PI_METHODDEF
    _ELEMENTTREE_TREEBUILDER_CLOSE_METHODDEF
    {NULL, NULL}
};

static PyType_Slot treebuilder_slots[] = {
    {Py_tp_dealloc, treebuilder_dealloc},
    {Py_tp_traverse, treebuilder_gc_traverse},
    {Py_tp_clear, treebuilder_gc_clear},
    {Py_tp_methods, treebuilder_methods},
    {Py_tp_init, _elementtree_TreeBuilder___init__},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_new, treebuilder_new},
    {0, NULL},
};

static PyType_Spec treebuilder_spec = {
    .name = "xml.etree.ElementTree.TreeBuilder",
    .basicsize = sizeof(TreeBuilderObject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots = treebuilder_slots,
};

static PyMethodDef xmlparser_methods[] = {
    _ELEMENTTREE_XMLPARSER_FEED_METHODDEF
    _ELEMENTTREE_XMLPARSER_CLOSE_METHODDEF
    _ELEMENTTREE_XMLPARSER__PARSE_WHOLE_METHODDEF
    _ELEMENTTREE_XMLPARSER__SETEVENTS_METHODDEF
    {NULL, NULL}
};

static PyType_Slot xmlparser_slots[] = {
    {Py_tp_dealloc, xmlparser_dealloc},
    {Py_tp_traverse, xmlparser_gc_traverse},
    {Py_tp_clear, xmlparser_gc_clear},
    {Py_tp_methods, xmlparser_methods},
    {Py_tp_members, xmlparser_members},
    {Py_tp_getset, xmlparser_getsetlist},
    {Py_tp_init, _elementtree_XMLParser___init__},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_new, xmlparser_new},
    {0, NULL},
};

static PyType_Spec xmlparser_spec = {
    .name = "xml.etree.ElementTree.XMLParser",
    .basicsize = sizeof(XMLParserObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = xmlparser_slots,
};

/* ==================================================================== */
/* python module interface */

static PyMethodDef _functions[] = {
    {"SubElement", _PyCFunction_CAST(subelement), METH_VARARGS | METH_KEYWORDS},
    _ELEMENTTREE__SET_FACTORIES_METHODDEF
    {NULL, NULL}
};

#define CREATE_TYPE(module, type, spec) \
do {                                                                     \
    if (type != NULL) {                                                  \
        break;                                                           \
    }                                                                    \
    type = (PyTypeObject *)PyType_FromModuleAndSpec(module, spec, NULL); \
    if (type == NULL) {                                                  \
        goto error;                                                      \
    }                                                                    \
} while (0)

static int
module_exec(PyObject *m)
{
    elementtreestate *st = get_elementtree_state(m);

    /* Initialize object types */
    CREATE_TYPE(m, st->ElementIter_Type, &elementiter_spec);
    CREATE_TYPE(m, st->TreeBuilder_Type, &treebuilder_spec);
    CREATE_TYPE(m, st->Element_Type, &element_spec);
    CREATE_TYPE(m, st->XMLParser_Type, &xmlparser_spec);

    st->deepcopy_obj = _PyImport_GetModuleAttrString("copy", "deepcopy");
    if (st->deepcopy_obj == NULL) {
        goto error;
    }

    assert(!PyErr_Occurred());
    if (!(st->elementpath_obj = PyImport_ImportModule("xml.etree.ElementPath")))
        goto error;

    /* link against pyexpat */
    if (!(st->expat_capsule = _PyImport_GetModuleAttrString("pyexpat", "expat_CAPI")))
        goto error;
    if (!(st->expat_capi = PyCapsule_GetPointer(st->expat_capsule, PyExpat_CAPSULE_NAME)))
        goto error;
    if (st->expat_capi) {
        /* check that it's usable */
        if (strcmp(st->expat_capi->magic, PyExpat_CAPI_MAGIC) != 0 ||
            (size_t)st->expat_capi->size < sizeof(struct PyExpat_CAPI) ||
            st->expat_capi->MAJOR_VERSION != XML_MAJOR_VERSION ||
            st->expat_capi->MINOR_VERSION != XML_MINOR_VERSION ||
            st->expat_capi->MICRO_VERSION != XML_MICRO_VERSION) {
            PyErr_SetString(PyExc_ImportError,
                            "pyexpat version is incompatible");
            goto error;
        }
    } else {
        goto error;
    }

    st->str_append = PyUnicode_InternFromString("append");
    if (st->str_append == NULL) {
        goto error;
    }
    st->str_find = PyUnicode_InternFromString("find");
    if (st->str_find == NULL) {
        goto error;
    }
    st->str_findall = PyUnicode_InternFromString("findall");
    if (st->str_findall == NULL) {
        goto error;
    }
    st->str_findtext = PyUnicode_InternFromString("findtext");
    if (st->str_findtext == NULL) {
        goto error;
    }
    st->str_iterfind = PyUnicode_InternFromString("iterfind");
    if (st->str_iterfind == NULL) {
        goto error;
    }
    st->str_tail = PyUnicode_InternFromString("tail");
    if (st->str_tail == NULL) {
        goto error;
    }
    st->str_text = PyUnicode_InternFromString("text");
    if (st->str_text == NULL) {
        goto error;
    }
    st->str_doctype = PyUnicode_InternFromString("doctype");
    if (st->str_doctype == NULL) {
        goto error;
    }
    st->parseerror_obj = PyErr_NewException(
        "xml.etree.ElementTree.ParseError", PyExc_SyntaxError, NULL
        );
    if (PyModule_AddObjectRef(m, "ParseError", st->parseerror_obj) < 0) {
        goto error;
    }

    PyTypeObject *types[] = {
        st->Element_Type,
        st->TreeBuilder_Type,
        st->XMLParser_Type
    };

    for (size_t i = 0; i < Py_ARRAY_LENGTH(types); i++) {
        if (PyModule_AddType(m, types[i]) < 0) {
            goto error;
        }
    }

    return 0;

error:
    return -1;
}

static struct PyModuleDef_Slot elementtree_slots[] = {
    {Py_mod_exec, module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL},
};

static struct PyModuleDef elementtreemodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_elementtree",
    .m_size = sizeof(elementtreestate),
    .m_methods = _functions,
    .m_slots = elementtree_slots,
    .m_traverse = elementtree_traverse,
    .m_clear = elementtree_clear,
    .m_free = elementtree_free,
};

PyMODINIT_FUNC
PyInit__elementtree(void)
{
    return PyModuleDef_Init(&elementtreemodule);
}
