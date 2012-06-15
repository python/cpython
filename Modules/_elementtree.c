/*
 * ElementTree
 * $Id: _elementtree.c 3473 2009-01-11 22:53:55Z fredrik $
 *
 * elementtree accelerator
 *
 * History:
 * 1999-06-20 fl  created (as part of sgmlop)
 * 2001-05-29 fl  effdom edition
 * 2003-02-27 fl  elementtree edition (alpha)
 * 2004-06-03 fl  updates for elementtree 1.2
 * 2005-01-05 fl  major optimization effort
 * 2005-01-11 fl  first public release (cElementTree 0.8)
 * 2005-01-12 fl  split element object into base and extras
 * 2005-01-13 fl  use tagged pointers for tail/text (cElementTree 0.9)
 * 2005-01-17 fl  added treebuilder close method
 * 2005-01-17 fl  fixed crash in getchildren
 * 2005-01-18 fl  removed observer api, added iterparse (cElementTree 0.9.3)
 * 2005-01-23 fl  revised iterparse api; added namespace event support (0.9.8)
 * 2005-01-26 fl  added VERSION module property (cElementTree 1.0)
 * 2005-01-28 fl  added remove method (1.0.1)
 * 2005-03-01 fl  added iselement function; fixed makeelement aliasing (1.0.2)
 * 2005-03-13 fl  export Comment and ProcessingInstruction/PI helpers
 * 2005-03-26 fl  added Comment and PI support to XMLParser
 * 2005-03-27 fl  event optimizations; complain about bogus events
 * 2005-08-08 fl  fixed read error handling in parse
 * 2005-08-11 fl  added runtime test for copy workaround (1.0.3)
 * 2005-12-13 fl  added expat_capi support (for xml.etree) (1.0.4)
 * 2005-12-16 fl  added support for non-standard encodings
 * 2006-03-08 fl  fixed a couple of potential null-refs and leaks
 * 2006-03-12 fl  merge in 2.5 ssize_t changes
 * 2007-08-25 fl  call custom builder's close method from XMLParser
 * 2007-08-31 fl  added iter, extend from ET 1.3
 * 2007-09-01 fl  fixed ParseError exception, setslice source type, etc
 * 2007-09-03 fl  fixed handling of negative insert indexes
 * 2007-09-04 fl  added itertext from ET 1.3
 * 2007-09-06 fl  added position attribute to ParseError exception
 * 2008-06-06 fl  delay error reporting in iterparse (from Hrvoje Niksic)
 *
 * Copyright (c) 1999-2009 by Secret Labs AB.  All rights reserved.
 * Copyright (c) 1999-2009 by Fredrik Lundh.
 *
 * info@pythonware.com
 * http://www.pythonware.com
 */

/* Licensed to PSF under a Contributor Agreement. */
/* See http://www.python.org/psf/license for licensing details. */

#include "Python.h"
#include "structmember.h"

#define VERSION "1.0.6"

/* -------------------------------------------------------------------- */
/* configuration */

/* Leave defined to include the expat-based XMLParser type */
#define USE_EXPAT

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

#if 0
static int memory = 0;
#define ALLOC(size, comment)\
do { memory += size; printf("%8d - %s\n", memory, comment); } while (0)
#define RELEASE(size, comment)\
do { memory -= size; printf("%8d - %s\n", memory, comment); } while (0)
#else
#define ALLOC(size, comment)
#define RELEASE(size, comment)
#endif

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
#define JOIN_GET(p) ((Py_uintptr_t) (p) & 1)
#define JOIN_SET(p, flag) ((void*) ((Py_uintptr_t) (JOIN_OBJ(p)) | (flag)))
#define JOIN_OBJ(p) ((PyObject*) ((Py_uintptr_t) (p) & ~1))

/* glue functions (see the init function for details) */
static PyObject* elementtree_parseerror_obj;
static PyObject* elementtree_deepcopy_obj;
static PyObject* elementpath_obj;

/* helpers */

LOCAL(PyObject*)
deepcopy(PyObject* object, PyObject* memo)
{
    /* do a deep copy of the given object */

    PyObject* args;
    PyObject* result;

    if (!elementtree_deepcopy_obj) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "deepcopy helper not found"
            );
        return NULL;
    }

    args = PyTuple_New(2);
    if (!args)
        return NULL;

    Py_INCREF(object); PyTuple_SET_ITEM(args, 0, (PyObject*) object);
    Py_INCREF(memo);   PyTuple_SET_ITEM(args, 1, (PyObject*) memo);

    result = PyObject_CallObject(elementtree_deepcopy_obj, args);

    Py_DECREF(args);

    return result;
}

LOCAL(PyObject*)
list_join(PyObject* list)
{
    /* join list elements (destroying the list in the process) */

    PyObject* joiner;
    PyObject* function;
    PyObject* args;
    PyObject* result;

    switch (PyList_GET_SIZE(list)) {
    case 0:
        Py_DECREF(list);
        return PyBytes_FromString("");
    case 1:
        result = PyList_GET_ITEM(list, 0);
        Py_INCREF(result);
        Py_DECREF(list);
        return result;
    }

    /* two or more elements: slice out a suitable separator from the
       first member, and use that to join the entire list */

    joiner = PySequence_GetSlice(PyList_GET_ITEM(list, 0), 0, 0);
    if (!joiner)
        return NULL;

    function = PyObject_GetAttrString(joiner, "join");
    if (!function) {
        Py_DECREF(joiner);
        return NULL;
    }

    args = PyTuple_New(1);
    if (!args)
        return NULL;

    PyTuple_SET_ITEM(args, 0, list);

    result = PyObject_CallObject(function, args);

    Py_DECREF(args); /* also removes list */
    Py_DECREF(function);
    Py_DECREF(joiner);

    return result;
}

/* Is the given object an empty dictionary?
*/
static int
is_empty_dict(PyObject *obj)
{
    return PyDict_CheckExact(obj) && PyDict_Size(obj) == 0;
}


/* -------------------------------------------------------------------- */
/* the Element type */

typedef struct {

    /* attributes (a dictionary object), or None if no attributes */
    PyObject* attrib;

    /* child elements */
    int length; /* actual number of items */
    int allocated; /* allocated items */

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

static PyTypeObject Element_Type;

#define Element_CheckExact(op) (Py_TYPE(op) == &Element_Type)

/* -------------------------------------------------------------------- */
/* Element constructors and destructor */

LOCAL(int)
create_extra(ElementObject* self, PyObject* attrib)
{
    self->extra = PyObject_Malloc(sizeof(ElementObjectExtra));
    if (!self->extra)
        return -1;

    if (!attrib)
        attrib = Py_None;

    Py_INCREF(attrib);
    self->extra->attrib = attrib;

    self->extra->length = 0;
    self->extra->allocated = STATIC_CHILDREN;
    self->extra->children = self->extra->_children;

    return 0;
}

LOCAL(void)
dealloc_extra(ElementObject* self)
{
    ElementObjectExtra *myextra;
    int i;

    if (!self->extra)
        return;

    /* Avoid DECREFs calling into this code again (cycles, etc.)
    */
    myextra = self->extra;
    self->extra = NULL;

    Py_DECREF(myextra->attrib);

    for (i = 0; i < myextra->length; i++)
        Py_DECREF(myextra->children[i]);

    if (myextra->children != myextra->_children)
        PyObject_Free(myextra->children);

    PyObject_Free(myextra);
}

/* Convenience internal function to create new Element objects with the given
 * tag and attributes.
*/
LOCAL(PyObject*)
create_new_element(PyObject* tag, PyObject* attrib)
{
    ElementObject* self;

    self = PyObject_GC_New(ElementObject, &Element_Type);
    if (self == NULL)
        return NULL;
    self->extra = NULL;

    if (attrib != Py_None && !is_empty_dict(attrib)) {
        if (create_extra(self, attrib) < 0) {
            PyObject_Del(self);
            return NULL;
        }
    }

    Py_INCREF(tag);
    self->tag = tag;

    Py_INCREF(Py_None);
    self->text = Py_None;

    Py_INCREF(Py_None);
    self->tail = Py_None;

    self->weakreflist = NULL;

    ALLOC(sizeof(ElementObject), "create element");
    PyObject_GC_Track(self);
    return (PyObject*) self;
}

static PyObject *
element_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ElementObject *e = (ElementObject *)type->tp_alloc(type, 0);
    if (e != NULL) {
        Py_INCREF(Py_None);
        e->tag = Py_None;

        Py_INCREF(Py_None);
        e->text = Py_None;

        Py_INCREF(Py_None);
        e->tail = Py_None;

        e->extra = NULL;
        e->weakreflist = NULL;
    }
    return (PyObject *)e;
}

/* Helper function for extracting the attrib dictionary from a keywords dict.
 * This is required by some constructors/functions in this module that can
 * either accept attrib as a keyword argument or all attributes splashed 
 * directly into *kwds.
 * If there is no 'attrib' keyword, return an empty dict.
 */
static PyObject*
get_attrib_from_keywords(PyObject *kwds)
{
    PyObject *attrib_str = PyUnicode_FromString("attrib");
    PyObject *attrib = PyDict_GetItem(kwds, attrib_str);

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
        PyDict_DelItem(kwds, attrib_str);
    } else {
        attrib = PyDict_New();
    }

    Py_DECREF(attrib_str);

    if (attrib)
        PyDict_Update(attrib, kwds);
    return attrib;
}

static int
element_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *tag;
    PyObject *tmp;
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
                return -1;
            }
        }
    } else if (kwds) {
        /* have keywords args */
        attrib = get_attrib_from_keywords(kwds);
        if (!attrib)
            return -1;
    } else {
        /* no attrib arg, no kwds, so no attributes */
        Py_INCREF(Py_None);
        attrib = Py_None;
    }

    self_elem = (ElementObject *)self;

    if (attrib != Py_None && !is_empty_dict(attrib)) {
        if (create_extra(self_elem, attrib) < 0) {
            PyObject_Del(self_elem);
            return -1;
        }
    }

    /* We own a reference to attrib here and it's no longer needed. */
    Py_DECREF(attrib);

    /* Replace the objects already pointed to by tag, text and tail. */
    tmp = self_elem->tag;
    self_elem->tag = tag;
    Py_INCREF(tag);
    Py_DECREF(tmp);

    tmp = self_elem->text;
    self_elem->text = Py_None;
    Py_INCREF(Py_None);
    Py_DECREF(JOIN_OBJ(tmp));

    tmp = self_elem->tail;
    self_elem->tail = Py_None;
    Py_INCREF(Py_None);
    Py_DECREF(JOIN_OBJ(tmp));

    return 0;
}

LOCAL(int)
element_resize(ElementObject* self, int extra)
{
    int size;
    PyObject* *children;

    /* make sure self->children can hold the given number of extra
       elements.  set an exception and return -1 if allocation failed */

    if (!self->extra)
        create_extra(self, NULL);

    size = self->extra->length + extra;

    if (size > self->extra->allocated) {
        /* use Python 2.4's list growth strategy */
        size = (size >> 3) + (size < 9 ? 3 : 6) + size;
        /* Coverity CID #182 size_error: Allocating 1 bytes to pointer "children"
         * which needs at least 4 bytes.
         * Although it's a false alarm always assume at least one child to
         * be safe.
         */
        size = size ? size : 1;
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

LOCAL(int)
element_add_subelement(ElementObject* self, PyObject* element)
{
    /* add a child element to a parent */

    if (element_resize(self, 1) < 0)
        return -1;

    Py_INCREF(element);
    self->extra->children[self->extra->length] = element;

    self->extra->length++;

    return 0;
}

LOCAL(PyObject*)
element_get_attrib(ElementObject* self)
{
    /* return borrowed reference to attrib dictionary */
    /* note: this function assumes that the extra section exists */

    PyObject* res = self->extra->attrib;

    if (res == Py_None) {
        Py_DECREF(res);
        /* create missing dictionary */
        res = PyDict_New();
        if (!res)
            return NULL;
        self->extra->attrib = res;
    }

    return res;
}

LOCAL(PyObject*)
element_get_text(ElementObject* self)
{
    /* return borrowed reference to text attribute */

    PyObject* res = self->text;

    if (JOIN_GET(res)) {
        res = JOIN_OBJ(res);
        if (PyList_CheckExact(res)) {
            res = list_join(res);
            if (!res)
                return NULL;
            self->text = res;
        }
    }

    return res;
}

LOCAL(PyObject*)
element_get_tail(ElementObject* self)
{
    /* return borrowed reference to text attribute */

    PyObject* res = self->tail;

    if (JOIN_GET(res)) {
        res = JOIN_OBJ(res);
        if (PyList_CheckExact(res)) {
            res = list_join(res);
            if (!res)
                return NULL;
            self->tail = res;
        }
    }

    return res;
}

static PyObject*
subelement(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject* elem;

    ElementObject* parent;
    PyObject* tag;
    PyObject* attrib = NULL;
    if (!PyArg_ParseTuple(args, "O!O|O!:SubElement",
                          &Element_Type, &parent, &tag,
                          &PyDict_Type, &attrib))
        return NULL;

    if (attrib) {
        /* attrib passed as positional arg */
        attrib = PyDict_Copy(attrib);
        if (!attrib)
            return NULL;
        if (kwds) {
            if (PyDict_Update(attrib, kwds) < 0) {
                return NULL;
            }
        }
    } else if (kwds) {
        /* have keyword args */
        attrib = get_attrib_from_keywords(kwds);
        if (!attrib)
            return NULL;
    } else {
        /* no attrib arg, no kwds, so no attribute */
        Py_INCREF(Py_None);
        attrib = Py_None;
    }

    elem = create_new_element(tag, attrib);

    Py_DECREF(attrib);

    if (element_add_subelement(parent, elem) < 0) {
        Py_DECREF(elem);
        return NULL;
    }

    return elem;
}

static int
element_gc_traverse(ElementObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->tag);
    Py_VISIT(JOIN_OBJ(self->text));
    Py_VISIT(JOIN_OBJ(self->tail));

    if (self->extra) {
        int i;
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

    /* The following is like Py_CLEAR for self->text and self->tail, but
     * written explicitily because the real pointers hide behind access
     * macros.
    */
    if (self->text) {
        PyObject *tmp = JOIN_OBJ(self->text);
        self->text = NULL;
        Py_DECREF(tmp);
    }

    if (self->tail) {
        PyObject *tmp = JOIN_OBJ(self->tail);
        self->tail = NULL;
        Py_DECREF(tmp);
    }

    /* After dropping all references from extra, it's no longer valid anyway,
     * so fully deallocate it.
    */
    dealloc_extra(self);
    return 0;
}

static void
element_dealloc(ElementObject* self)
{
    PyObject_GC_UnTrack(self);

    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    /* element_gc_clear clears all references and deallocates extra
    */
    element_gc_clear(self);

    RELEASE(sizeof(ElementObject), "destroy element");
    Py_TYPE(self)->tp_free((PyObject *)self);
}

/* -------------------------------------------------------------------- */
/* methods (in alphabetical order) */

static PyObject*
element_append(ElementObject* self, PyObject* args)
{
    PyObject* element;
    if (!PyArg_ParseTuple(args, "O!:append", &Element_Type, &element))
        return NULL;

    if (element_add_subelement(self, element) < 0)
        return NULL;

    Py_RETURN_NONE;
}

static PyObject*
element_clearmethod(ElementObject* self, PyObject* args)
{
    if (!PyArg_ParseTuple(args, ":clear"))
        return NULL;

    dealloc_extra(self);

    Py_INCREF(Py_None);
    Py_DECREF(JOIN_OBJ(self->text));
    self->text = Py_None;

    Py_INCREF(Py_None);
    Py_DECREF(JOIN_OBJ(self->tail));
    self->tail = Py_None;

    Py_RETURN_NONE;
}

static PyObject*
element_copy(ElementObject* self, PyObject* args)
{
    int i;
    ElementObject* element;

    if (!PyArg_ParseTuple(args, ":__copy__"))
        return NULL;

    element = (ElementObject*) create_new_element(
        self->tag, (self->extra) ? self->extra->attrib : Py_None
        );
    if (!element)
        return NULL;

    Py_DECREF(JOIN_OBJ(element->text));
    element->text = self->text;
    Py_INCREF(JOIN_OBJ(element->text));

    Py_DECREF(JOIN_OBJ(element->tail));
    element->tail = self->tail;
    Py_INCREF(JOIN_OBJ(element->tail));

    if (self->extra) {

        if (element_resize(element, self->extra->length) < 0) {
            Py_DECREF(element);
            return NULL;
        }

        for (i = 0; i < self->extra->length; i++) {
            Py_INCREF(self->extra->children[i]);
            element->extra->children[i] = self->extra->children[i];
        }

        element->extra->length = self->extra->length;

    }

    return (PyObject*) element;
}

static PyObject*
element_deepcopy(ElementObject* self, PyObject* args)
{
    int i;
    ElementObject* element;
    PyObject* tag;
    PyObject* attrib;
    PyObject* text;
    PyObject* tail;
    PyObject* id;

    PyObject* memo;
    if (!PyArg_ParseTuple(args, "O:__deepcopy__", &memo))
        return NULL;

    tag = deepcopy(self->tag, memo);
    if (!tag)
        return NULL;

    if (self->extra) {
        attrib = deepcopy(self->extra->attrib, memo);
        if (!attrib) {
            Py_DECREF(tag);
            return NULL;
        }
    } else {
        Py_INCREF(Py_None);
        attrib = Py_None;
    }

    element = (ElementObject*) create_new_element(tag, attrib);

    Py_DECREF(tag);
    Py_DECREF(attrib);

    if (!element)
        return NULL;

    text = deepcopy(JOIN_OBJ(self->text), memo);
    if (!text)
        goto error;
    Py_DECREF(element->text);
    element->text = JOIN_SET(text, JOIN_GET(self->text));

    tail = deepcopy(JOIN_OBJ(self->tail), memo);
    if (!tail)
        goto error;
    Py_DECREF(element->tail);
    element->tail = JOIN_SET(tail, JOIN_GET(self->tail));

    if (self->extra) {

        if (element_resize(element, self->extra->length) < 0)
            goto error;

        for (i = 0; i < self->extra->length; i++) {
            PyObject* child = deepcopy(self->extra->children[i], memo);
            if (!child) {
                element->extra->length = i;
                goto error;
            }
            element->extra->children[i] = child;
        }

        element->extra->length = self->extra->length;

    }

    /* add object to memo dictionary (so deepcopy won't visit it again) */
    id = PyLong_FromLong((Py_uintptr_t) self);
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
        void *data = PyUnicode_DATA(tag);
        unsigned int kind = PyUnicode_KIND(tag);
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
        char *p = PyBytes_AS_STRING(tag);
        for (i = 0; i < PyBytes_GET_SIZE(tag); i++) {
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

static PyObject*
element_extend(ElementObject* self, PyObject* args)
{
    PyObject* seq;
    Py_ssize_t i, seqlen = 0;

    PyObject* seq_in;
    if (!PyArg_ParseTuple(args, "O:extend", &seq_in))
        return NULL;

    seq = PySequence_Fast(seq_in, "");
    if (!seq) {
        PyErr_Format(
            PyExc_TypeError,
            "expected sequence, not \"%.200s\"", Py_TYPE(seq_in)->tp_name
            );
        return NULL;
    }

    seqlen = PySequence_Size(seq);
    for (i = 0; i < seqlen; i++) {
        PyObject* element = PySequence_Fast_GET_ITEM(seq, i);
        if (!PyObject_IsInstance(element, (PyObject *)&Element_Type)) {
            Py_DECREF(seq);
            PyErr_Format(
                PyExc_TypeError,
                "expected an Element, not \"%.200s\"",
                Py_TYPE(element)->tp_name);
            return NULL;
        }

        if (element_add_subelement(self, element) < 0) {
            Py_DECREF(seq);
            return NULL;
        }
    }

    Py_DECREF(seq);

    Py_RETURN_NONE;
}

static PyObject*
element_find(ElementObject *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject* tag;
    PyObject* namespaces = Py_None;
    static char *kwlist[] = {"path", "namespaces", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O:find", kwlist,
                                     &tag, &namespaces))
        return NULL;

    if (checkpath(tag) || namespaces != Py_None) {
        _Py_IDENTIFIER(find);
        return _PyObject_CallMethodId(
            elementpath_obj, &PyId_find, "OOO", self, tag, namespaces
            );
    }

    if (!self->extra)
        Py_RETURN_NONE;

    for (i = 0; i < self->extra->length; i++) {
        PyObject* item = self->extra->children[i];
        if (Element_CheckExact(item) &&
            PyObject_RichCompareBool(((ElementObject*)item)->tag, tag, Py_EQ) == 1) {
            Py_INCREF(item);
            return item;
        }
    }

    Py_RETURN_NONE;
}

static PyObject*
element_findtext(ElementObject *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject* tag;
    PyObject* default_value = Py_None;
    PyObject* namespaces = Py_None;
    _Py_IDENTIFIER(findtext);
    static char *kwlist[] = {"path", "default", "namespaces", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|OO:findtext", kwlist,
                                     &tag, &default_value, &namespaces))
        return NULL;

    if (checkpath(tag) || namespaces != Py_None)
        return _PyObject_CallMethodId(
            elementpath_obj, &PyId_findtext, "OOOO", self, tag, default_value, namespaces
            );

    if (!self->extra) {
        Py_INCREF(default_value);
        return default_value;
    }

    for (i = 0; i < self->extra->length; i++) {
        ElementObject* item = (ElementObject*) self->extra->children[i];
        if (Element_CheckExact(item) && (PyObject_RichCompareBool(item->tag, tag, Py_EQ) == 1)) {

            PyObject* text = element_get_text(item);
            if (text == Py_None)
                return PyBytes_FromString("");
            Py_XINCREF(text);
            return text;
        }
    }

    Py_INCREF(default_value);
    return default_value;
}

static PyObject*
element_findall(ElementObject *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject* out;
    PyObject* tag;
    PyObject* namespaces = Py_None;
    static char *kwlist[] = {"path", "namespaces", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O:findall", kwlist,
                                     &tag, &namespaces))
        return NULL;

    if (checkpath(tag) || namespaces != Py_None) {
        _Py_IDENTIFIER(findall);
        return _PyObject_CallMethodId(
            elementpath_obj, &PyId_findall, "OOO", self, tag, namespaces
            );
    }

    out = PyList_New(0);
    if (!out)
        return NULL;

    if (!self->extra)
        return out;

    for (i = 0; i < self->extra->length; i++) {
        PyObject* item = self->extra->children[i];
        if (Element_CheckExact(item) &&
            PyObject_RichCompareBool(((ElementObject*)item)->tag, tag, Py_EQ) == 1) {
            if (PyList_Append(out, item) < 0) {
                Py_DECREF(out);
                return NULL;
            }
        }
    }

    return out;
}

static PyObject*
element_iterfind(ElementObject *self, PyObject *args, PyObject *kwds)
{
    PyObject* tag;
    PyObject* namespaces = Py_None;
    _Py_IDENTIFIER(iterfind);
    static char *kwlist[] = {"path", "namespaces", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O:iterfind", kwlist,
                                     &tag, &namespaces))
        return NULL;

    return _PyObject_CallMethodId(
        elementpath_obj, &PyId_iterfind, "OOO", self, tag, namespaces
        );
}

static PyObject*
element_get(ElementObject* self, PyObject* args)
{
    PyObject* value;

    PyObject* key;
    PyObject* default_value = Py_None;
    if (!PyArg_ParseTuple(args, "O|O:get", &key, &default_value))
        return NULL;

    if (!self->extra || self->extra->attrib == Py_None)
        value = default_value;
    else {
        value = PyDict_GetItem(self->extra->attrib, key);
        if (!value)
            value = default_value;
    }

    Py_INCREF(value);
    return value;
}

static PyObject*
element_getchildren(ElementObject* self, PyObject* args)
{
    int i;
    PyObject* list;

    /* FIXME: report as deprecated? */

    if (!PyArg_ParseTuple(args, ":getchildren"))
        return NULL;

    if (!self->extra)
        return PyList_New(0);

    list = PyList_New(self->extra->length);
    if (!list)
        return NULL;

    for (i = 0; i < self->extra->length; i++) {
        PyObject* item = self->extra->children[i];
        Py_INCREF(item);
        PyList_SET_ITEM(list, i, item);
    }

    return list;
}


static PyObject *
create_elementiter(ElementObject *self, PyObject *tag, int gettext);


static PyObject *
element_iter(ElementObject *self, PyObject *args)
{
    PyObject* tag = Py_None;
    if (!PyArg_ParseTuple(args, "|O:iter", &tag))
        return NULL;

    return create_elementiter(self, tag, 0);
}


static PyObject*
element_itertext(ElementObject* self, PyObject* args)
{
    if (!PyArg_ParseTuple(args, ":itertext"))
        return NULL;

    return create_elementiter(self, Py_None, 1);
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

    Py_INCREF(self->extra->children[index]);
    return self->extra->children[index];
}

static PyObject*
element_insert(ElementObject* self, PyObject* args)
{
    int i;

    int index;
    PyObject* element;
    if (!PyArg_ParseTuple(args, "iO!:insert", &index,
                          &Element_Type, &element))
        return NULL;

    if (!self->extra)
        create_extra(self, NULL);

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

    Py_INCREF(element);
    self->extra->children[index] = element;

    self->extra->length++;

    Py_RETURN_NONE;
}

static PyObject*
element_items(ElementObject* self, PyObject* args)
{
    if (!PyArg_ParseTuple(args, ":items"))
        return NULL;

    if (!self->extra || self->extra->attrib == Py_None)
        return PyList_New(0);

    return PyDict_Items(self->extra->attrib);
}

static PyObject*
element_keys(ElementObject* self, PyObject* args)
{
    if (!PyArg_ParseTuple(args, ":keys"))
        return NULL;

    if (!self->extra || self->extra->attrib == Py_None)
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

static PyObject*
element_makeelement(PyObject* self, PyObject* args, PyObject* kw)
{
    PyObject* elem;

    PyObject* tag;
    PyObject* attrib;
    if (!PyArg_ParseTuple(args, "OO:makeelement", &tag, &attrib))
        return NULL;

    attrib = PyDict_Copy(attrib);
    if (!attrib)
        return NULL;

    elem = create_new_element(tag, attrib);

    Py_DECREF(attrib);

    return elem;
}

static PyObject*
element_remove(ElementObject* self, PyObject* args)
{
    int i;

    PyObject* element;
    if (!PyArg_ParseTuple(args, "O!:remove", &Element_Type, &element))
        return NULL;

    if (!self->extra) {
        /* element has no children, so raise exception */
        PyErr_SetString(
            PyExc_ValueError,
            "list.remove(x): x not in list"
            );
        return NULL;
    }

    for (i = 0; i < self->extra->length; i++) {
        if (self->extra->children[i] == element)
            break;
        if (PyObject_RichCompareBool(self->extra->children[i], element, Py_EQ) == 1)
            break;
    }

    if (i == self->extra->length) {
        /* element is not in children, so raise exception */
        PyErr_SetString(
            PyExc_ValueError,
            "list.remove(x): x not in list"
            );
        return NULL;
    }

    Py_DECREF(self->extra->children[i]);

    self->extra->length--;

    for (; i < self->extra->length; i++)
        self->extra->children[i] = self->extra->children[i+1];

    Py_RETURN_NONE;
}

static PyObject*
element_repr(ElementObject* self)
{
    if (self->tag)
        return PyUnicode_FromFormat("<Element %R at %p>", self->tag, self);
    else
        return PyUnicode_FromFormat("<Element at %p>", self);
}

static PyObject*
element_set(ElementObject* self, PyObject* args)
{
    PyObject* attrib;

    PyObject* key;
    PyObject* value;
    if (!PyArg_ParseTuple(args, "OO:set", &key, &value))
        return NULL;

    if (!self->extra)
        create_extra(self, NULL);

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
    int i;
    PyObject* old;

    if (!self->extra || index < 0 || index >= self->extra->length) {
        PyErr_SetString(
            PyExc_IndexError,
            "child assignment index out of range");
        return -1;
    }

    old = self->extra->children[index];

    if (item) {
        Py_INCREF(item);
        self->extra->children[index] = item;
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
        Py_ssize_t start, stop, step, slicelen, cur, i;
        PyObject* list;

        if (!self->extra)
            return PyList_New(0);

        if (PySlice_GetIndicesEx(item,
                self->extra->length,
                &start, &stop, &step, &slicelen) < 0) {
            return NULL;
        }

        if (slicelen <= 0)
            return PyList_New(0);
        else {
            list = PyList_New(slicelen);
            if (!list)
                return NULL;

            for (cur = start, i = 0; i < slicelen;
                 cur += step, i++) {
                PyObject* item = self->extra->children[cur];
                Py_INCREF(item);
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
        Py_ssize_t start, stop, step, slicelen, newlen, cur, i;

        PyObject* recycle = NULL;
        PyObject* seq = NULL;

        if (!self->extra)
            create_extra(self, NULL);

        if (PySlice_GetIndicesEx(item,
                self->extra->length,
                &start, &stop, &step, &slicelen) < 0) {
            return -1;
        }

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

            assert((size_t)slicelen <= PY_SIZE_MAX / sizeof(PyObject *));

            /* recycle is a list that will contain all the children
             * scheduled for removal.
            */
            if (!(recycle = PyList_New(slicelen))) {
                PyErr_NoMemory();
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
            Py_XDECREF(recycle);
            return 0;
        }
        else {
            /* A new slice is actually being assigned */
            seq = PySequence_Fast(value, "");
            if (!seq) {
                PyErr_Format(
                    PyExc_TypeError,
                    "expected sequence, not \"%.200s\"", Py_TYPE(value)->tp_name
                    );
                return -1;
            }
            newlen = PySequence_Size(seq);
        }

        if (step !=  1 && newlen != slicelen)
        {
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
                if (seq) {
                    Py_DECREF(seq);
                }
                return -1;
            }
        }

        if (slicelen > 0) {
            /* to avoid recursive calls to this method (via decref), move
               old items to the recycle bin here, and get rid of them when
               we're done modifying the element */
            recycle = PyList_New(slicelen);
            if (!recycle) {
                if (seq) {
                    Py_DECREF(seq);
                }
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
            Py_INCREF(element);
            self->extra->children[cur] = element;
        }

        self->extra->length += newlen - slicelen;

        if (seq) {
            Py_DECREF(seq);
        }

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

static PyMethodDef element_methods[] = {

    {"clear", (PyCFunction) element_clearmethod, METH_VARARGS},

    {"get", (PyCFunction) element_get, METH_VARARGS},
    {"set", (PyCFunction) element_set, METH_VARARGS},

    {"find", (PyCFunction) element_find, METH_VARARGS | METH_KEYWORDS},
    {"findtext", (PyCFunction) element_findtext, METH_VARARGS | METH_KEYWORDS},
    {"findall", (PyCFunction) element_findall, METH_VARARGS | METH_KEYWORDS},

    {"append", (PyCFunction) element_append, METH_VARARGS},
    {"extend", (PyCFunction) element_extend, METH_VARARGS},
    {"insert", (PyCFunction) element_insert, METH_VARARGS},
    {"remove", (PyCFunction) element_remove, METH_VARARGS},

    {"iter", (PyCFunction) element_iter, METH_VARARGS},
    {"itertext", (PyCFunction) element_itertext, METH_VARARGS},
    {"iterfind", (PyCFunction) element_iterfind, METH_VARARGS | METH_KEYWORDS},

    {"getiterator", (PyCFunction) element_iter, METH_VARARGS},
    {"getchildren", (PyCFunction) element_getchildren, METH_VARARGS},

    {"items", (PyCFunction) element_items, METH_VARARGS},
    {"keys", (PyCFunction) element_keys, METH_VARARGS},

    {"makeelement", (PyCFunction) element_makeelement, METH_VARARGS},

    {"__copy__", (PyCFunction) element_copy, METH_VARARGS},
    {"__deepcopy__", (PyCFunction) element_deepcopy, METH_VARARGS},

    {NULL, NULL}
};

static PyObject*
element_getattro(ElementObject* self, PyObject* nameobj)
{
    PyObject* res;
    char *name = "";

    if (PyUnicode_Check(nameobj))
        name = _PyUnicode_AsString(nameobj);

    if (name == NULL)
        return NULL;

    /* handle common attributes first */
    if (strcmp(name, "tag") == 0) {
        res = self->tag;
        Py_INCREF(res);
        return res;
    } else if (strcmp(name, "text") == 0) {
        res = element_get_text(self);
        Py_INCREF(res);
        return res;
    }

    /* methods */
    res = PyObject_GenericGetAttr((PyObject*) self, nameobj);
    if (res)
        return res;

    /* less common attributes */
    if (strcmp(name, "tail") == 0) {
        PyErr_Clear();
        res = element_get_tail(self);
    } else if (strcmp(name, "attrib") == 0) {
        PyErr_Clear();
        if (!self->extra)
            create_extra(self, NULL);
        res = element_get_attrib(self);
    }

    if (!res)
        return NULL;

    Py_INCREF(res);
    return res;
}

static PyObject*
element_setattro(ElementObject* self, PyObject* nameobj, PyObject* value)
{
    char *name = "";
    if (PyUnicode_Check(nameobj))
        name = _PyUnicode_AsString(nameobj);

    if (name == NULL)
        return NULL;

    if (strcmp(name, "tag") == 0) {
        Py_DECREF(self->tag);
        self->tag = value;
        Py_INCREF(self->tag);
    } else if (strcmp(name, "text") == 0) {
        Py_DECREF(JOIN_OBJ(self->text));
        self->text = value;
        Py_INCREF(self->text);
    } else if (strcmp(name, "tail") == 0) {
        Py_DECREF(JOIN_OBJ(self->tail));
        self->tail = value;
        Py_INCREF(self->tail);
    } else if (strcmp(name, "attrib") == 0) {
        if (!self->extra)
            create_extra(self, NULL);
        Py_DECREF(self->extra->attrib);
        self->extra->attrib = value;
        Py_INCREF(self->extra->attrib);
    } else {
        PyErr_SetString(PyExc_AttributeError, name);
        return NULL;
    }

    return NULL;
}

static PySequenceMethods element_as_sequence = {
    (lenfunc) element_length,
    0, /* sq_concat */
    0, /* sq_repeat */
    element_getitem,
    0,
    element_setitem,
    0,
};

static PyMappingMethods element_as_mapping = {
    (lenfunc) element_length,
    (binaryfunc) element_subscr,
    (objobjargproc) element_ass_subscr,
};

static PyTypeObject Element_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Element", sizeof(ElementObject), 0,
    /* methods */
    (destructor)element_dealloc,                    /* tp_dealloc */
    0,                                              /* tp_print */
    0,                                              /* tp_getattr */
    0,                                              /* tp_setattr */
    0,                                              /* tp_reserved */
    (reprfunc)element_repr,                         /* tp_repr */
    0,                                              /* tp_as_number */
    &element_as_sequence,                           /* tp_as_sequence */
    &element_as_mapping,                            /* tp_as_mapping */
    0,                                              /* tp_hash */
    0,                                              /* tp_call */
    0,                                              /* tp_str */
    (getattrofunc)element_getattro,                 /* tp_getattro */
    (setattrofunc)element_setattro,                 /* tp_setattro */
    0,                                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
                                                    /* tp_flags */
    0,                                              /* tp_doc */
    (traverseproc)element_gc_traverse,              /* tp_traverse */
    (inquiry)element_gc_clear,                      /* tp_clear */
    0,                                              /* tp_richcompare */
    offsetof(ElementObject, weakreflist),           /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    element_methods,                                /* tp_methods */
    0,                                              /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)element_init,                         /* tp_init */
    PyType_GenericAlloc,                            /* tp_alloc */
    element_new,                                    /* tp_new */
    0,                                              /* tp_free */
};

/******************************* Element iterator ****************************/

/* ElementIterObject represents the iteration state over an XML element in
 * pre-order traversal. To keep track of which sub-element should be returned
 * next, a stack of parents is maintained. This is a standard stack-based
 * iterative pre-order traversal of a tree.
 * The stack is managed using a single-linked list starting at parent_stack.
 * Each stack node contains the saved parent to which we should return after
 * the current one is exhausted, and the next child to examine in that parent.
 */
typedef struct ParentLocator_t {
    ElementObject *parent;
    Py_ssize_t child_index;
    struct ParentLocator_t *next;
} ParentLocator;

typedef struct {
    PyObject_HEAD
    ParentLocator *parent_stack;
    ElementObject *root_element;
    PyObject *sought_tag;
    int root_done;
    int gettext;
} ElementIterObject;


static void
elementiter_dealloc(ElementIterObject *it)
{
    ParentLocator *p = it->parent_stack;
    while (p) {
        ParentLocator *temp = p;
        Py_XDECREF(p->parent);
        p = p->next;
        PyObject_Free(temp);
    }

    Py_XDECREF(it->sought_tag);
    Py_XDECREF(it->root_element);

    PyObject_GC_UnTrack(it);
    PyObject_GC_Del(it);
}

static int
elementiter_traverse(ElementIterObject *it, visitproc visit, void *arg)
{
    ParentLocator *p = it->parent_stack;
    while (p) {
        Py_VISIT(p->parent);
        p = p->next;
    }

    Py_VISIT(it->root_element);
    Py_VISIT(it->sought_tag);
    return 0;
}

/* Helper function for elementiter_next. Add a new parent to the parent stack.
 */
static ParentLocator *
parent_stack_push_new(ParentLocator *stack, ElementObject *parent)
{
    ParentLocator *new_node = PyObject_Malloc(sizeof(ParentLocator));
    if (new_node) {
        new_node->parent = parent;
        Py_INCREF(parent);
        new_node->child_index = 0;
        new_node->next = stack;
    }
    return new_node;
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
    ElementObject *cur_parent;
    Py_ssize_t child_index;

    while (1) {
        /* Handle the case reached in the beginning and end of iteration, where
         * the parent stack is empty. The root_done flag gives us indication
         * whether we've just started iterating (so root_done is 0), in which
         * case the root is returned. If root_done is 1 and we're here, the
         * iterator is exhausted.
         */
        if (!it->parent_stack->parent) {
            if (it->root_done) {
                PyErr_SetNone(PyExc_StopIteration);
                return NULL;
            } else {
                it->parent_stack = parent_stack_push_new(it->parent_stack,
                                                         it->root_element);
                if (!it->parent_stack) {
                    PyErr_NoMemory();
                    return NULL;
                }

                it->root_done = 1;
                if (it->sought_tag == Py_None ||
                    PyObject_RichCompareBool(it->root_element->tag,
                                             it->sought_tag, Py_EQ) == 1) {
                    if (it->gettext) {
                        PyObject *text = JOIN_OBJ(it->root_element->text);
                        if (PyObject_IsTrue(text)) {
                            Py_INCREF(text);
                            return text;
                        }
                    } else {
                        Py_INCREF(it->root_element);
                        return (PyObject *)it->root_element;
                    }
                }
            }
        }

        /* See if there are children left to traverse in the current parent. If
         * yes, visit the next child. If not, pop the stack and try again.
         */
        cur_parent = it->parent_stack->parent;
        child_index = it->parent_stack->child_index;
        if (cur_parent->extra && child_index < cur_parent->extra->length) {
            ElementObject *child = (ElementObject *)
                cur_parent->extra->children[child_index];
            it->parent_stack->child_index++;
            it->parent_stack = parent_stack_push_new(it->parent_stack,
                                                     child);
            if (!it->parent_stack) {
                PyErr_NoMemory();
                return NULL;
            }

            if (it->gettext) {
                PyObject *text = JOIN_OBJ(child->text);
                if (PyObject_IsTrue(text)) {
                    Py_INCREF(text);
                    return text;
                }
            } else if (it->sought_tag == Py_None ||
                PyObject_RichCompareBool(child->tag,
                                         it->sought_tag, Py_EQ) == 1) {
                Py_INCREF(child);
                return (PyObject *)child;
            }
            else
                continue;
        }
        else {
            PyObject *tail = it->gettext ? JOIN_OBJ(cur_parent->tail) : Py_None;            
            ParentLocator *next = it->parent_stack->next;
            Py_XDECREF(it->parent_stack->parent);
            PyObject_Free(it->parent_stack);
            it->parent_stack = next;

            /* Note that extra condition on it->parent_stack->parent here;
             * this is because itertext() is supposed to only return *inner*
             * text, not text following the element it began iteration with.
             */
            if (it->parent_stack->parent && PyObject_IsTrue(tail)) {
                Py_INCREF(tail);
                return tail;
            }
        }
    }

    return NULL;
}


static PyTypeObject ElementIter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_elementtree._element_iterator",           /* tp_name */
    sizeof(ElementIterObject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)elementiter_dealloc,            /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)elementiter_traverse,         /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)elementiter_next,             /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    0,                                          /* tp_new */
};


static PyObject *
create_elementiter(ElementObject *self, PyObject *tag, int gettext)
{
    ElementIterObject *it;
    PyObject *star = NULL;

    it = PyObject_GC_New(ElementIterObject, &ElementIter_Type);
    if (!it)
        return NULL;
    if (!(it->parent_stack = PyObject_Malloc(sizeof(ParentLocator)))) {
        PyObject_GC_Del(it);
        return NULL;
    }

    it->parent_stack->parent = NULL;
    it->parent_stack->child_index = 0;
    it->parent_stack->next = NULL;

    if (PyUnicode_Check(tag))
        star = PyUnicode_FromString("*");
    else if (PyBytes_Check(tag))
        star = PyBytes_FromString("*");

    if (star && PyObject_RichCompareBool(tag, star, Py_EQ) == 1)
        tag = Py_None;

    Py_XDECREF(star);
    it->sought_tag = tag;
    it->root_done = 0;
    it->gettext = gettext;
    it->root_element = self;

    Py_INCREF(self);
    Py_INCREF(tag);

    PyObject_GC_Track(it);
    return (PyObject *)it;
}


/* ==================================================================== */
/* the tree builder type */

typedef struct {
    PyObject_HEAD

    PyObject *root; /* root node (first created node) */

    ElementObject *this; /* current node */
    ElementObject *last; /* most recently created node */

    PyObject *data; /* data collector (string or list), or NULL */

    PyObject *stack; /* element stack */
    Py_ssize_t index; /* current stack size (0 means empty) */

    PyObject *element_factory;

    /* element tracing */
    PyObject *events; /* list of events, or NULL if not collecting */
    PyObject *start_event_obj; /* event objects (NULL to ignore) */
    PyObject *end_event_obj;
    PyObject *start_ns_event_obj;
    PyObject *end_ns_event_obj;
} TreeBuilderObject;

static PyTypeObject TreeBuilder_Type;

#define TreeBuilder_CheckExact(op) (Py_TYPE(op) == &TreeBuilder_Type)

/* -------------------------------------------------------------------- */
/* constructor and destructor */

static PyObject *
treebuilder_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    TreeBuilderObject *t = (TreeBuilderObject *)type->tp_alloc(type, 0);
    if (t != NULL) {
        t->root = NULL;

        Py_INCREF(Py_None);
        t->this = (ElementObject *)Py_None;
        Py_INCREF(Py_None);
        t->last = (ElementObject *)Py_None;

        t->data = NULL;
        t->element_factory = NULL;
        t->stack = PyList_New(20);
        if (!t->stack) {
            Py_DECREF(t->this);
            Py_DECREF(t->last);
            return NULL;
        }
        t->index = 0;

        t->events = NULL;
        t->start_event_obj = t->end_event_obj = NULL;
        t->start_ns_event_obj = t->end_ns_event_obj = NULL;
    }
    return (PyObject *)t;
}

static int
treebuilder_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"element_factory", 0};
    PyObject *element_factory = NULL;
    TreeBuilderObject *self_tb = (TreeBuilderObject *)self;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O:TreeBuilder", kwlist,
                                     &element_factory)) {
        return -1;
    }

    if (element_factory) {
        Py_INCREF(element_factory);
        Py_XDECREF(self_tb->element_factory);
        self_tb->element_factory = element_factory;
    }

    return 0;
}

static int
treebuilder_gc_traverse(TreeBuilderObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->root);
    Py_VISIT(self->this);
    Py_VISIT(self->last);
    Py_VISIT(self->data);
    Py_VISIT(self->stack);
    Py_VISIT(self->element_factory);
    return 0;
}

static int
treebuilder_gc_clear(TreeBuilderObject *self)
{
    Py_XDECREF(self->end_ns_event_obj);
    Py_XDECREF(self->start_ns_event_obj);
    Py_XDECREF(self->end_event_obj);
    Py_XDECREF(self->start_event_obj);
    Py_XDECREF(self->events);
    Py_DECREF(self->stack);
    Py_XDECREF(self->data);
    Py_DECREF(self->last);
    Py_DECREF(self->this);
    Py_CLEAR(self->element_factory);
    Py_XDECREF(self->root);
    return 0;
}

static void
treebuilder_dealloc(TreeBuilderObject *self)
{
    PyObject_GC_UnTrack(self);
    treebuilder_gc_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

/* -------------------------------------------------------------------- */
/* handlers */

LOCAL(PyObject*)
treebuilder_handle_start(TreeBuilderObject* self, PyObject* tag,
                         PyObject* attrib)
{
    PyObject* node;
    PyObject* this;

    if (self->data) {
        if (self->this == self->last) {
            Py_DECREF(JOIN_OBJ(self->last->text));
            self->last->text = JOIN_SET(
                self->data, PyList_CheckExact(self->data)
                );
        } else {
            Py_DECREF(JOIN_OBJ(self->last->tail));
            self->last->tail = JOIN_SET(
                self->data, PyList_CheckExact(self->data)
                );
        }
        self->data = NULL;
    }

    if (self->element_factory) {
        node = PyObject_CallFunction(self->element_factory, "OO", tag, attrib);
    } else {
        node = create_new_element(tag, attrib);
    }
    if (!node) {
        return NULL;
    }

    this = (PyObject*) self->this;

    if (this != Py_None) {
        if (element_add_subelement((ElementObject*) this, node) < 0)
            goto error;
    } else {
        if (self->root) {
            PyErr_SetString(
                elementtree_parseerror_obj,
                "multiple elements on top level"
                );
            goto error;
        }
        Py_INCREF(node);
        self->root = node;
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

    Py_DECREF(this);
    Py_INCREF(node);
    self->this = (ElementObject*) node;

    Py_DECREF(self->last);
    Py_INCREF(node);
    self->last = (ElementObject*) node;

    if (self->start_event_obj) {
        PyObject* res;
        PyObject* action = self->start_event_obj;
        res = PyTuple_New(2);
        if (res) {
            Py_INCREF(action); PyTuple_SET_ITEM(res, 0, (PyObject*) action);
            Py_INCREF(node);   PyTuple_SET_ITEM(res, 1, (PyObject*) node);
            PyList_Append(self->events, res);
            Py_DECREF(res);
        } else
            PyErr_Clear(); /* FIXME: propagate error */
    }

    return node;

  error:
    Py_DECREF(node);
    return NULL;
}

LOCAL(PyObject*)
treebuilder_handle_data(TreeBuilderObject* self, PyObject* data)
{
    if (!self->data) {
        if (self->last == (ElementObject*) Py_None) {
            /* ignore calls to data before the first call to start */
            Py_RETURN_NONE;
        }
        /* store the first item as is */
        Py_INCREF(data); self->data = data;
    } else {
        /* more than one item; use a list to collect items */
        if (PyBytes_CheckExact(self->data) && Py_REFCNT(self->data) == 1 &&
            PyBytes_CheckExact(data) && PyBytes_GET_SIZE(data) == 1) {
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
            PyList_SET_ITEM(list, 0, self->data);
            Py_INCREF(data); PyList_SET_ITEM(list, 1, data);
            self->data = list;
        }
    }

    Py_RETURN_NONE;
}

LOCAL(PyObject*)
treebuilder_handle_end(TreeBuilderObject* self, PyObject* tag)
{
    PyObject* item;

    if (self->data) {
        if (self->this == self->last) {
            Py_DECREF(JOIN_OBJ(self->last->text));
            self->last->text = JOIN_SET(
                self->data, PyList_CheckExact(self->data)
                );
        } else {
            Py_DECREF(JOIN_OBJ(self->last->tail));
            self->last->tail = JOIN_SET(
                self->data, PyList_CheckExact(self->data)
                );
        }
        self->data = NULL;
    }

    if (self->index == 0) {
        PyErr_SetString(
            PyExc_IndexError,
            "pop from empty stack"
            );
        return NULL;
    }

    self->index--;

    item = PyList_GET_ITEM(self->stack, self->index);
    Py_INCREF(item);

    Py_DECREF(self->last);

    self->last = (ElementObject*) self->this;
    self->this = (ElementObject*) item;

    if (self->end_event_obj) {
        PyObject* res;
        PyObject* action = self->end_event_obj;
        PyObject* node = (PyObject*) self->last;
        res = PyTuple_New(2);
        if (res) {
            Py_INCREF(action); PyTuple_SET_ITEM(res, 0, (PyObject*) action);
            Py_INCREF(node);   PyTuple_SET_ITEM(res, 1, (PyObject*) node);
            PyList_Append(self->events, res);
            Py_DECREF(res);
        } else
            PyErr_Clear(); /* FIXME: propagate error */
    }

    Py_INCREF(self->last);
    return (PyObject*) self->last;
}

LOCAL(void)
treebuilder_handle_namespace(TreeBuilderObject* self, int start,
                             PyObject *prefix, PyObject *uri)
{
    PyObject* res;
    PyObject* action;
    PyObject* parcel;

    if (!self->events)
        return;

    if (start) {
        if (!self->start_ns_event_obj)
            return;
        action = self->start_ns_event_obj;
        parcel = Py_BuildValue("OO", prefix, uri);
        if (!parcel)
            return;
        Py_INCREF(action);
    } else {
        if (!self->end_ns_event_obj)
            return;
        action = self->end_ns_event_obj;
        Py_INCREF(action);
        parcel = Py_None;
        Py_INCREF(parcel);
    }

    res = PyTuple_New(2);

    if (res) {
        PyTuple_SET_ITEM(res, 0, action);
        PyTuple_SET_ITEM(res, 1, parcel);
        PyList_Append(self->events, res);
        Py_DECREF(res);
    } else
        PyErr_Clear(); /* FIXME: propagate error */
}

/* -------------------------------------------------------------------- */
/* methods (in alphabetical order) */

static PyObject*
treebuilder_data(TreeBuilderObject* self, PyObject* args)
{
    PyObject* data;
    if (!PyArg_ParseTuple(args, "O:data", &data))
        return NULL;

    return treebuilder_handle_data(self, data);
}

static PyObject*
treebuilder_end(TreeBuilderObject* self, PyObject* args)
{
    PyObject* tag;
    if (!PyArg_ParseTuple(args, "O:end", &tag))
        return NULL;

    return treebuilder_handle_end(self, tag);
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

    Py_INCREF(res);
    return res;
}

static PyObject*
treebuilder_close(TreeBuilderObject* self, PyObject* args)
{
    if (!PyArg_ParseTuple(args, ":close"))
        return NULL;

    return treebuilder_done(self);
}

static PyObject*
treebuilder_start(TreeBuilderObject* self, PyObject* args)
{
    PyObject* tag;
    PyObject* attrib = Py_None;
    if (!PyArg_ParseTuple(args, "O|O:start", &tag, &attrib))
        return NULL;

    return treebuilder_handle_start(self, tag, attrib);
}

static PyMethodDef treebuilder_methods[] = {
    {"data", (PyCFunction) treebuilder_data, METH_VARARGS},
    {"start", (PyCFunction) treebuilder_start, METH_VARARGS},
    {"end", (PyCFunction) treebuilder_end, METH_VARARGS},
    {"close", (PyCFunction) treebuilder_close, METH_VARARGS},
    {NULL, NULL}
};

static PyTypeObject TreeBuilder_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "TreeBuilder", sizeof(TreeBuilderObject), 0,
    /* methods */
    (destructor)treebuilder_dealloc,                /* tp_dealloc */
    0,                                              /* tp_print */
    0,                                              /* tp_getattr */
    0,                                              /* tp_setattr */
    0,                                              /* tp_reserved */
    0,                                              /* tp_repr */
    0,                                              /* tp_as_number */
    0,                                              /* tp_as_sequence */
    0,                                              /* tp_as_mapping */
    0,                                              /* tp_hash */
    0,                                              /* tp_call */
    0,                                              /* tp_str */
    0,                                              /* tp_getattro */
    0,                                              /* tp_setattro */
    0,                                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
                                                    /* tp_flags */
    0,                                              /* tp_doc */
    (traverseproc)treebuilder_gc_traverse,          /* tp_traverse */
    (inquiry)treebuilder_gc_clear,                  /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    treebuilder_methods,                            /* tp_methods */
    0,                                              /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)treebuilder_init,                     /* tp_init */
    PyType_GenericAlloc,                            /* tp_alloc */
    treebuilder_new,                                /* tp_new */
    0,                                              /* tp_free */
};

/* ==================================================================== */
/* the expat interface */

#if defined(USE_EXPAT)

#include "expat.h"
#include "pyexpat.h"
static struct PyExpat_CAPI *expat_capi;
#define EXPAT(func) (expat_capi->func)

static XML_Memory_Handling_Suite ExpatMemoryHandler = {
    PyObject_Malloc, PyObject_Realloc, PyObject_Free};

typedef struct {
    PyObject_HEAD

    XML_Parser parser;

    PyObject *target;
    PyObject *entity;

    PyObject *names;

    PyObject *handle_start;
    PyObject *handle_data;
    PyObject *handle_end;

    PyObject *handle_comment;
    PyObject *handle_pi;
    PyObject *handle_doctype;

    PyObject *handle_close;

} XMLParserObject;

static PyTypeObject XMLParser_Type;

#define XMLParser_CheckExact(op) (Py_TYPE(op) == &XMLParser_Type)

/* helpers */

LOCAL(PyObject*)
makeuniversal(XMLParserObject* self, const char* string)
{
    /* convert a UTF-8 tag/attribute name from the expat parser
       to a universal name string */

    int size = strlen(string);
    PyObject* key;
    PyObject* value;

    /* look the 'raw' name up in the names dictionary */
    key = PyBytes_FromStringAndSize(string, size);
    if (!key)
        return NULL;

    value = PyDict_GetItem(self->names, key);

    if (value) {
        Py_INCREF(value);
    } else {
        /* new name.  convert to universal name, and decode as
           necessary */

        PyObject* tag;
        char* p;
        int i;

        /* look for namespace separator */
        for (i = 0; i < size; i++)
            if (string[i] == '}')
                break;
        if (i != size) {
            /* convert to universal name */
            tag = PyBytes_FromStringAndSize(NULL, size+1);
            p = PyBytes_AS_STRING(tag);
            p[0] = '{';
            memcpy(p+1, string, size);
            size++;
        } else {
            /* plain name; use key as tag */
            Py_INCREF(key);
            tag = key;
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
expat_set_error(enum XML_Error error_code, int line, int column, char *message)
{
    PyObject *errmsg, *error, *position, *code;

    errmsg = PyUnicode_FromFormat("%s: line %d, column %d",
                message ? message : EXPAT(ErrorString)(error_code),
                line, column);
    if (errmsg == NULL)
        return;

    error = PyObject_CallFunction(elementtree_parseerror_obj, "O", errmsg);
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

    position = Py_BuildValue("(ii)", line, column);
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

    PyErr_SetObject(elementtree_parseerror_obj, error);
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

    key = PyUnicode_DecodeUTF8(data_in + 1, data_len - 2, "strict");
    if (!key)
        return;

    value = PyDict_GetItem(self->entity, key);

    if (value) {
        if (TreeBuilder_CheckExact(self->target))
            res = treebuilder_handle_data(
                (TreeBuilderObject*) self->target, value
                );
        else if (self->handle_data)
            res = PyObject_CallFunction(self->handle_data, "O", value);
        else
            res = NULL;
        Py_XDECREF(res);
    } else if (!PyErr_Occurred()) {
        /* Report the first error, not the last */
        char message[128] = "undefined entity ";
        strncat(message, data_in, data_len < 100?data_len:100);
        expat_set_error(
            XML_ERROR_UNDEFINED_ENTITY,
            EXPAT(GetErrorLineNumber)(self->parser),
            EXPAT(GetErrorColumnNumber)(self->parser),
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

    /* tag name */
    tag = makeuniversal(self, tag_in);
    if (!tag)
        return; /* parser will look for errors */

    /* attributes */
    if (attrib_in[0]) {
        attrib = PyDict_New();
        if (!attrib)
            return;
        while (attrib_in[0] && attrib_in[1]) {
            PyObject* key = makeuniversal(self, attrib_in[0]);
            PyObject* value = PyUnicode_DecodeUTF8(attrib_in[1], strlen(attrib_in[1]), "strict");
            if (!key || !value) {
                Py_XDECREF(value);
                Py_XDECREF(key);
                Py_DECREF(attrib);
                return;
            }
            ok = PyDict_SetItem(attrib, key, value);
            Py_DECREF(value);
            Py_DECREF(key);
            if (ok < 0) {
                Py_DECREF(attrib);
                return;
            }
            attrib_in += 2;
        }
    } else {
        Py_INCREF(Py_None);
        attrib = Py_None;
    }

    /* If we get None, pass an empty dictionary on */
    if (attrib == Py_None) {
        Py_DECREF(attrib);
        attrib = PyDict_New();
        if (!attrib)
            return;
    }

    if (TreeBuilder_CheckExact(self->target)) {
        /* shortcut */
        res = treebuilder_handle_start((TreeBuilderObject*) self->target,
                                       tag, attrib);
    }
    else if (self->handle_start) {
        res = PyObject_CallFunction(self->handle_start, "OO", tag, attrib);
    } else
        res = NULL;

    Py_DECREF(tag);
    Py_DECREF(attrib);

    Py_XDECREF(res);
}

static void
expat_data_handler(XMLParserObject* self, const XML_Char* data_in,
                   int data_len)
{
    PyObject* data;
    PyObject* res;

    data = PyUnicode_DecodeUTF8(data_in, data_len, "strict");
    if (!data)
        return; /* parser will look for errors */

    if (TreeBuilder_CheckExact(self->target))
        /* shortcut */
        res = treebuilder_handle_data((TreeBuilderObject*) self->target, data);
    else if (self->handle_data)
        res = PyObject_CallFunction(self->handle_data, "O", data);
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

    if (TreeBuilder_CheckExact(self->target))
        /* shortcut */
        /* the standard tree builder doesn't look at the end tag */
        res = treebuilder_handle_end(
            (TreeBuilderObject*) self->target, Py_None
            );
    else if (self->handle_end) {
        tag = makeuniversal(self, tag_in);
        if (tag) {
            res = PyObject_CallFunction(self->handle_end, "O", tag);
            Py_DECREF(tag);
        }
    }

    Py_XDECREF(res);
}

static void
expat_start_ns_handler(XMLParserObject* self, const XML_Char* prefix,
                       const XML_Char *uri)
{
    PyObject* sprefix = NULL;
    PyObject* suri = NULL;

    suri = PyUnicode_DecodeUTF8(uri, strlen(uri), "strict");
    if (!suri)
        return;

    if (prefix)
        sprefix = PyUnicode_DecodeUTF8(prefix, strlen(prefix), "strict");
    else
        sprefix = PyUnicode_FromString("");
    if (!sprefix) {
        Py_DECREF(suri);
        return;
    }

    treebuilder_handle_namespace(
        (TreeBuilderObject*) self->target, 1, sprefix, suri
        );

    Py_DECREF(sprefix);
    Py_DECREF(suri);
}

static void
expat_end_ns_handler(XMLParserObject* self, const XML_Char* prefix_in)
{
    treebuilder_handle_namespace(
        (TreeBuilderObject*) self->target, 0, NULL, NULL
        );
}

static void
expat_comment_handler(XMLParserObject* self, const XML_Char* comment_in)
{
    PyObject* comment;
    PyObject* res;

    if (self->handle_comment) {
        comment = PyUnicode_DecodeUTF8(comment_in, strlen(comment_in), "strict");
        if (comment) {
            res = PyObject_CallFunction(self->handle_comment, "O", comment);
            Py_XDECREF(res);
            Py_DECREF(comment);
        }
    }
}

static void 
expat_start_doctype_handler(XMLParserObject *self,
                            const XML_Char *doctype_name,
                            const XML_Char *sysid,
                            const XML_Char *pubid,
                            int has_internal_subset)
{
    PyObject *self_pyobj = (PyObject *)self;
    PyObject *doctype_name_obj, *sysid_obj, *pubid_obj;
    PyObject *parser_doctype = NULL;
    PyObject *res = NULL;

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
        Py_INCREF(Py_None);
        sysid_obj = Py_None;
    }

    if (pubid) {
        pubid_obj = makeuniversal(self, pubid);
        if (!pubid_obj) {
            Py_DECREF(doctype_name_obj);
            Py_DECREF(sysid_obj);
            return;
        }
    } else {
        Py_INCREF(Py_None);
        pubid_obj = Py_None;
    }

    /* If the target has a handler for doctype, call it. */
    if (self->handle_doctype) {
        res = PyObject_CallFunction(self->handle_doctype, "OOO",
                                    doctype_name_obj, pubid_obj, sysid_obj);
        Py_CLEAR(res);
    }

    /* Now see if the parser itself has a doctype method. If yes and it's
     * a subclass, call it but warn about deprecation. If it's not a subclass
     * (i.e. vanilla XMLParser), do nothing.
     */
    parser_doctype = PyObject_GetAttrString(self_pyobj, "doctype");
    if (parser_doctype) {
        if (!XMLParser_CheckExact(self_pyobj)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                            "This method of XMLParser is deprecated.  Define"
                            " doctype() method on the TreeBuilder target.",
                            1) < 0) {
                goto clear;
            }
            res = PyObject_CallFunction(parser_doctype, "OOO",
                                        doctype_name_obj, pubid_obj, sysid_obj);
            Py_CLEAR(res);
        }
    }

clear:
    Py_XDECREF(parser_doctype);
    Py_DECREF(doctype_name_obj);
    Py_DECREF(pubid_obj);
    Py_DECREF(sysid_obj);
}

static void
expat_pi_handler(XMLParserObject* self, const XML_Char* target_in,
                 const XML_Char* data_in)
{
    PyObject* target;
    PyObject* data;
    PyObject* res;

    if (self->handle_pi) {
        target = PyUnicode_DecodeUTF8(target_in, strlen(target_in), "strict");
        data = PyUnicode_DecodeUTF8(data_in, strlen(data_in), "strict");
        if (target && data) {
            res = PyObject_CallFunction(self->handle_pi, "OO", target, data);
            Py_XDECREF(res);
            Py_DECREF(data);
            Py_DECREF(target);
        } else {
            Py_XDECREF(data);
            Py_XDECREF(target);
        }
    }
}

static int
expat_unknown_encoding_handler(XMLParserObject *self, const XML_Char *name,
                               XML_Encoding *info)
{
    PyObject* u;
    unsigned char s[256];
    int i;
    void *data;
    unsigned int kind;

    memset(info, 0, sizeof(XML_Encoding));

    for (i = 0; i < 256; i++)
        s[i] = i;

    u = PyUnicode_Decode((char*) s, 256, name, "replace");
    if (!u)
        return XML_STATUS_ERROR;
    if (PyUnicode_READY(u))
        return XML_STATUS_ERROR;

    if (PyUnicode_GET_LENGTH(u) != 256) {
        Py_DECREF(u);
        return XML_STATUS_ERROR;
    }

    kind = PyUnicode_KIND(u);
    data = PyUnicode_DATA(u);
    for (i = 0; i < 256; i++) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if (ch != Py_UNICODE_REPLACEMENT_CHARACTER)
            info->map[i] = ch;
        else
            info->map[i] = -1;
    }

    Py_DECREF(u);

    return XML_STATUS_OK;
}

/* -------------------------------------------------------------------- */

static PyObject *
xmlparser_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    XMLParserObject *self = (XMLParserObject *)type->tp_alloc(type, 0);
    if (self) {
        self->parser = NULL;
        self->target = self->entity = self->names = NULL;
        self->handle_start = self->handle_data = self->handle_end = NULL;
        self->handle_comment = self->handle_pi = self->handle_close = NULL;
        self->handle_doctype = NULL;
    }
    return (PyObject *)self;
}

static int
xmlparser_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    XMLParserObject *self_xp = (XMLParserObject *)self;
    PyObject *target = NULL, *html = NULL;
    char *encoding = NULL;
    static char *kwlist[] = {"html", "target", "encoding", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOz:XMLParser", kwlist,
                                     &html, &target, &encoding)) {
        return -1;
    }

    self_xp->entity = PyDict_New();
    if (!self_xp->entity)
        return -1;

    self_xp->names = PyDict_New();
    if (!self_xp->names) {
        Py_XDECREF(self_xp->entity);
        return -1;
    }

    self_xp->parser = EXPAT(ParserCreate_MM)(encoding, &ExpatMemoryHandler, "}");
    if (!self_xp->parser) {
        Py_XDECREF(self_xp->entity);
        Py_XDECREF(self_xp->names);
        PyErr_NoMemory();
        return -1;
    }

    if (target) {
        Py_INCREF(target);
    } else {
        target = treebuilder_new(&TreeBuilder_Type, NULL, NULL);
        if (!target) {
            Py_XDECREF(self_xp->entity);
            Py_XDECREF(self_xp->names);
            EXPAT(ParserFree)(self_xp->parser);
            return -1;
        }
    }
    self_xp->target = target;

    self_xp->handle_start = PyObject_GetAttrString(target, "start");
    self_xp->handle_data = PyObject_GetAttrString(target, "data");
    self_xp->handle_end = PyObject_GetAttrString(target, "end");
    self_xp->handle_comment = PyObject_GetAttrString(target, "comment");
    self_xp->handle_pi = PyObject_GetAttrString(target, "pi");
    self_xp->handle_close = PyObject_GetAttrString(target, "close");
    self_xp->handle_doctype = PyObject_GetAttrString(target, "doctype");

    PyErr_Clear();
    
    /* configure parser */
    EXPAT(SetUserData)(self_xp->parser, self_xp);
    EXPAT(SetElementHandler)(
        self_xp->parser,
        (XML_StartElementHandler) expat_start_handler,
        (XML_EndElementHandler) expat_end_handler
        );
    EXPAT(SetDefaultHandlerExpand)(
        self_xp->parser,
        (XML_DefaultHandler) expat_default_handler
        );
    EXPAT(SetCharacterDataHandler)(
        self_xp->parser,
        (XML_CharacterDataHandler) expat_data_handler
        );
    if (self_xp->handle_comment)
        EXPAT(SetCommentHandler)(
            self_xp->parser,
            (XML_CommentHandler) expat_comment_handler
            );
    if (self_xp->handle_pi)
        EXPAT(SetProcessingInstructionHandler)(
            self_xp->parser,
            (XML_ProcessingInstructionHandler) expat_pi_handler
            );
    EXPAT(SetStartDoctypeDeclHandler)(
        self_xp->parser,
        (XML_StartDoctypeDeclHandler) expat_start_doctype_handler
        );
    EXPAT(SetUnknownEncodingHandler)(
        self_xp->parser,
        (XML_UnknownEncodingHandler) expat_unknown_encoding_handler, NULL
        );

    return 0;
}

static int
xmlparser_gc_traverse(XMLParserObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->handle_close);
    Py_VISIT(self->handle_pi);
    Py_VISIT(self->handle_comment);
    Py_VISIT(self->handle_end);
    Py_VISIT(self->handle_data);
    Py_VISIT(self->handle_start);

    Py_VISIT(self->target);
    Py_VISIT(self->entity);
    Py_VISIT(self->names);

    return 0;
}

static int
xmlparser_gc_clear(XMLParserObject *self)
{
    EXPAT(ParserFree)(self->parser);

    Py_XDECREF(self->handle_close);
    Py_XDECREF(self->handle_pi);
    Py_XDECREF(self->handle_comment);
    Py_XDECREF(self->handle_end);
    Py_XDECREF(self->handle_data);
    Py_XDECREF(self->handle_start);
    Py_XDECREF(self->handle_doctype);

    Py_XDECREF(self->target);
    Py_XDECREF(self->entity);
    Py_XDECREF(self->names);

    return 0;
}

static void
xmlparser_dealloc(XMLParserObject* self)
{
    PyObject_GC_UnTrack(self);
    xmlparser_gc_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

LOCAL(PyObject*)
expat_parse(XMLParserObject* self, char* data, int data_len, int final)
{
    int ok;

    ok = EXPAT(Parse)(self->parser, data, data_len, final);

    if (PyErr_Occurred())
        return NULL;

    if (!ok) {
        expat_set_error(
            EXPAT(GetErrorCode)(self->parser),
            EXPAT(GetErrorLineNumber)(self->parser),
            EXPAT(GetErrorColumnNumber)(self->parser),
            NULL
            );
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject*
xmlparser_close(XMLParserObject* self, PyObject* args)
{
    /* end feeding data to parser */

    PyObject* res;
    if (!PyArg_ParseTuple(args, ":close"))
        return NULL;

    res = expat_parse(self, "", 0, 1);
    if (!res)
        return NULL;

    if (TreeBuilder_CheckExact(self->target)) {
        Py_DECREF(res);
        return treebuilder_done((TreeBuilderObject*) self->target);
    } if (self->handle_close) {
        Py_DECREF(res);
        return PyObject_CallFunction(self->handle_close, "");
    } else
        return res;
}

static PyObject*
xmlparser_feed(XMLParserObject* self, PyObject* args)
{
    /* feed data to parser */

    char* data;
    int data_len;
    if (!PyArg_ParseTuple(args, "s#:feed", &data, &data_len))
        return NULL;

    return expat_parse(self, data, data_len, 0);
}

static PyObject*
xmlparser_parse(XMLParserObject* self, PyObject* args)
{
    /* (internal) parse until end of input stream */

    PyObject* reader;
    PyObject* buffer;
    PyObject* temp;
    PyObject* res;

    PyObject* fileobj;
    if (!PyArg_ParseTuple(args, "O:_parse", &fileobj))
        return NULL;

    reader = PyObject_GetAttrString(fileobj, "read");
    if (!reader)
        return NULL;

    /* read from open file object */
    for (;;) {

        buffer = PyObject_CallFunction(reader, "i", 64*1024);

        if (!buffer) {
            /* read failed (e.g. due to KeyboardInterrupt) */
            Py_DECREF(reader);
            return NULL;
        }

        if (PyUnicode_CheckExact(buffer)) {
            /* A unicode object is encoded into bytes using UTF-8 */
            if (PyUnicode_GET_SIZE(buffer) == 0) {
                Py_DECREF(buffer);
                break;
            }
            temp = PyUnicode_AsEncodedString(buffer, "utf-8", "surrogatepass");
            if (!temp) {
                /* Propagate exception from PyUnicode_AsEncodedString */
                Py_DECREF(buffer);
                Py_DECREF(reader);
                return NULL;
            }

            /* Here we no longer need the original buffer since it contains
             * unicode. Make it point to the encoded bytes object.
            */
            Py_DECREF(buffer);
            buffer = temp;
        }
        else if (!PyBytes_CheckExact(buffer) || PyBytes_GET_SIZE(buffer) == 0) {
            Py_DECREF(buffer);
            break;
        }

        res = expat_parse(
            self, PyBytes_AS_STRING(buffer), PyBytes_GET_SIZE(buffer), 0
            );

        Py_DECREF(buffer);

        if (!res) {
            Py_DECREF(reader);
            return NULL;
        }
        Py_DECREF(res);

    }

    Py_DECREF(reader);

    res = expat_parse(self, "", 0, 1);

    if (res && TreeBuilder_CheckExact(self->target)) {
        Py_DECREF(res);
        return treebuilder_done((TreeBuilderObject*) self->target);
    }

    return res;
}

static PyObject*
xmlparser_doctype(XMLParserObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}

static PyObject*
xmlparser_setevents(XMLParserObject *self, PyObject* args)
{
    /* activate element event reporting */

    Py_ssize_t i;
    TreeBuilderObject* target;

    PyObject* events; /* event collector */
    PyObject* event_set = Py_None;
    if (!PyArg_ParseTuple(args, "O!|O:_setevents",  &PyList_Type, &events,
                          &event_set))
        return NULL;

    if (!TreeBuilder_CheckExact(self->target)) {
        PyErr_SetString(
            PyExc_TypeError,
            "event handling only supported for ElementTree.TreeBuilder "
            "targets"
            );
        return NULL;
    }

    target = (TreeBuilderObject*) self->target;

    Py_INCREF(events);
    Py_XDECREF(target->events);
    target->events = events;

    /* clear out existing events */
    Py_XDECREF(target->start_event_obj); target->start_event_obj = NULL;
    Py_XDECREF(target->end_event_obj); target->end_event_obj = NULL;
    Py_XDECREF(target->start_ns_event_obj); target->start_ns_event_obj = NULL;
    Py_XDECREF(target->end_ns_event_obj); target->end_ns_event_obj = NULL;

    if (event_set == Py_None) {
        /* default is "end" only */
        target->end_event_obj = PyUnicode_FromString("end");
        Py_RETURN_NONE;
    }

    if (!PyTuple_Check(event_set)) /* FIXME: handle arbitrary sequences */
        goto error;

    for (i = 0; i < PyTuple_GET_SIZE(event_set); i++) {
        PyObject* item = PyTuple_GET_ITEM(event_set, i);
        char* event;
        if (PyUnicode_Check(item)) {
            event = _PyUnicode_AsString(item);
            if (event == NULL)
                goto error;
        } else if (PyBytes_Check(item))
            event = PyBytes_AS_STRING(item);
        else {
            goto error;
        }
        if (strcmp(event, "start") == 0) {
            Py_INCREF(item);
            target->start_event_obj = item;
        } else if (strcmp(event, "end") == 0) {
            Py_INCREF(item);
            Py_XDECREF(target->end_event_obj);
            target->end_event_obj = item;
        } else if (strcmp(event, "start-ns") == 0) {
            Py_INCREF(item);
            Py_XDECREF(target->start_ns_event_obj);
            target->start_ns_event_obj = item;
            EXPAT(SetNamespaceDeclHandler)(
                self->parser,
                (XML_StartNamespaceDeclHandler) expat_start_ns_handler,
                (XML_EndNamespaceDeclHandler) expat_end_ns_handler
                );
        } else if (strcmp(event, "end-ns") == 0) {
            Py_INCREF(item);
            Py_XDECREF(target->end_ns_event_obj);
            target->end_ns_event_obj = item;
            EXPAT(SetNamespaceDeclHandler)(
                self->parser,
                (XML_StartNamespaceDeclHandler) expat_start_ns_handler,
                (XML_EndNamespaceDeclHandler) expat_end_ns_handler
                );
        } else {
            PyErr_Format(
                PyExc_ValueError,
                "unknown event '%s'", event
                );
            return NULL;
        }
    }

    Py_RETURN_NONE;

  error:
    PyErr_SetString(
        PyExc_TypeError,
        "invalid event tuple"
        );
    return NULL;
}

static PyMethodDef xmlparser_methods[] = {
    {"feed", (PyCFunction) xmlparser_feed, METH_VARARGS},
    {"close", (PyCFunction) xmlparser_close, METH_VARARGS},
    {"_parse", (PyCFunction) xmlparser_parse, METH_VARARGS},
    {"_setevents", (PyCFunction) xmlparser_setevents, METH_VARARGS},
    {"doctype", (PyCFunction) xmlparser_doctype, METH_VARARGS},
    {NULL, NULL}
};

static PyObject*
xmlparser_getattro(XMLParserObject* self, PyObject* nameobj)
{
    if (PyUnicode_Check(nameobj)) {
        PyObject* res;
        if (PyUnicode_CompareWithASCIIString(nameobj, "entity") == 0)
            res = self->entity;
        else if (PyUnicode_CompareWithASCIIString(nameobj, "target") == 0)
            res = self->target;
        else if (PyUnicode_CompareWithASCIIString(nameobj, "version") == 0) {
            return PyUnicode_FromFormat(
                "Expat %d.%d.%d", XML_MAJOR_VERSION,
                XML_MINOR_VERSION, XML_MICRO_VERSION);
        }
        else
            goto generic;

        Py_INCREF(res);
        return res;
    }
  generic:
    return PyObject_GenericGetAttr((PyObject*) self, nameobj);
}

static PyTypeObject XMLParser_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "XMLParser", sizeof(XMLParserObject), 0,
    /* methods */
    (destructor)xmlparser_dealloc,                  /* tp_dealloc */
    0,                                              /* tp_print */
    0,                                              /* tp_getattr */
    0,                                              /* tp_setattr */
    0,                                              /* tp_reserved */
    0,                                              /* tp_repr */
    0,                                              /* tp_as_number */
    0,                                              /* tp_as_sequence */
    0,                                              /* tp_as_mapping */
    0,                                              /* tp_hash */
    0,                                              /* tp_call */
    0,                                              /* tp_str */
    (getattrofunc)xmlparser_getattro,               /* tp_getattro */
    0,                                              /* tp_setattro */
    0,                                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
                                                    /* tp_flags */
    0,                                              /* tp_doc */
    (traverseproc)xmlparser_gc_traverse,            /* tp_traverse */
    (inquiry)xmlparser_gc_clear,                    /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    xmlparser_methods,                              /* tp_methods */
    0,                                              /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)xmlparser_init,                       /* tp_init */
    PyType_GenericAlloc,                            /* tp_alloc */
    xmlparser_new,                                  /* tp_new */
    0,                                              /* tp_free */
};

#endif

/* ==================================================================== */
/* python module interface */

static PyMethodDef _functions[] = {
    {"SubElement", (PyCFunction) subelement, METH_VARARGS|METH_KEYWORDS},
    {NULL, NULL}
};


static struct PyModuleDef _elementtreemodule = {
        PyModuleDef_HEAD_INIT,
        "_elementtree",
        NULL,
        -1,
        _functions,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit__elementtree(void)
{
    PyObject *m, *temp;

    /* Initialize object types */
    if (PyType_Ready(&TreeBuilder_Type) < 0)
        return NULL;
    if (PyType_Ready(&Element_Type) < 0)
        return NULL;
#if defined(USE_EXPAT)
    if (PyType_Ready(&XMLParser_Type) < 0)
        return NULL;
#endif

    m = PyModule_Create(&_elementtreemodule);
    if (!m)
        return NULL;

    if (!(temp = PyImport_ImportModule("copy")))
        return NULL;
    elementtree_deepcopy_obj = PyObject_GetAttrString(temp, "deepcopy");
    Py_XDECREF(temp);

    if (!(elementpath_obj = PyImport_ImportModule("xml.etree.ElementPath")))
        return NULL;

    /* link against pyexpat */
    expat_capi = PyCapsule_Import(PyExpat_CAPSULE_NAME, 0);
    if (expat_capi) {
        /* check that it's usable */
        if (strcmp(expat_capi->magic, PyExpat_CAPI_MAGIC) != 0 ||
            expat_capi->size < sizeof(struct PyExpat_CAPI) ||
            expat_capi->MAJOR_VERSION != XML_MAJOR_VERSION ||
            expat_capi->MINOR_VERSION != XML_MINOR_VERSION ||
            expat_capi->MICRO_VERSION != XML_MICRO_VERSION) {
            expat_capi = NULL;
        }
    }
    if (!expat_capi) {
        PyErr_SetString(
            PyExc_RuntimeError, "cannot load dispatch table from pyexpat"
            );
        return NULL;
    }

    elementtree_parseerror_obj = PyErr_NewException(
        "xml.etree.ElementTree.ParseError", PyExc_SyntaxError, NULL
        );
    Py_INCREF(elementtree_parseerror_obj);
    PyModule_AddObject(m, "ParseError", elementtree_parseerror_obj);

    Py_INCREF((PyObject *)&Element_Type);
    PyModule_AddObject(m, "Element", (PyObject *)&Element_Type);

    Py_INCREF((PyObject *)&TreeBuilder_Type);
    PyModule_AddObject(m, "TreeBuilder", (PyObject *)&TreeBuilder_Type);

#if defined(USE_EXPAT)
    Py_INCREF((PyObject *)&XMLParser_Type);
    PyModule_AddObject(m, "XMLParser", (PyObject *)&XMLParser_Type);
#endif

    return m;
}
