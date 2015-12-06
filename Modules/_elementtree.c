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

#define VERSION "1.0.6"

/* -------------------------------------------------------------------- */
/* configuration */

/* Leave defined to include the expat-based XMLParser type */
#define USE_EXPAT

/* Define to do all expat calls via pyexpat's embedded expat library */
/* #define USE_PYEXPAT_CAPI */

/* An element can hold this many children without extra memory
   allocations. */
#define STATIC_CHILDREN 4

/* For best performance, chose a value so that 80-90% of all nodes
   have no more than the given number of children.  Set this to zero
   to minimize the size of the element structure itself (this only
   helps if you have lots of leaf nodes with attributes). */

/* Also note that pymalloc always allocates blocks in multiples of
   eight bytes.  For the current version of cElementTree, this means
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

/* compatibility macros */
#if (PY_VERSION_HEX < 0x02060000)
#define Py_REFCNT(ob) (((PyObject*)(ob))->ob_refcnt)
#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif

#if (PY_VERSION_HEX < 0x02050000)
typedef int Py_ssize_t;
#define lenfunc inquiry
#endif

#if (PY_VERSION_HEX < 0x02040000)
#define PyDict_CheckExact PyDict_Check

#if !defined(Py_RETURN_NONE)
#define Py_RETURN_NONE return Py_INCREF(Py_None), Py_None
#endif
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
static PyObject* elementtree_copyelement_obj;
static PyObject* elementtree_deepcopy_obj;
static PyObject* elementtree_iter_obj;
static PyObject* elementtree_itertext_obj;
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
        return PyString_FromString("");
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

/* -------------------------------------------------------------------- */
/* the element type */

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

} ElementObject;

staticforward PyTypeObject Element_Type;

#define Element_CheckExact(op) (Py_TYPE(op) == &Element_Type)

/* -------------------------------------------------------------------- */
/* element constructor and destructor */

LOCAL(int)
element_new_extra(ElementObject* self, PyObject* attrib)
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
element_dealloc_extra(ElementObject* self)
{
    int i;

    Py_DECREF(self->extra->attrib);

    for (i = 0; i < self->extra->length; i++)
        Py_DECREF(self->extra->children[i]);

    if (self->extra->children != self->extra->_children)
        PyObject_Free(self->extra->children);

    PyObject_Free(self->extra);
}

LOCAL(PyObject*)
element_new(PyObject* tag, PyObject* attrib)
{
    ElementObject* self;

    self = PyObject_New(ElementObject, &Element_Type);
    if (self == NULL)
        return NULL;

    /* use None for empty dictionaries */
    if (PyDict_CheckExact(attrib) && !PyDict_Size(attrib))
        attrib = Py_None;

    self->extra = NULL;

    if (attrib != Py_None) {

        if (element_new_extra(self, attrib) < 0) {
            PyObject_Del(self);
            return NULL;
        }

        self->extra->length = 0;
        self->extra->allocated = STATIC_CHILDREN;
        self->extra->children = self->extra->_children;

    }

    Py_INCREF(tag);
    self->tag = tag;

    Py_INCREF(Py_None);
    self->text = Py_None;

    Py_INCREF(Py_None);
    self->tail = Py_None;

    ALLOC(sizeof(ElementObject), "create element");

    return (PyObject*) self;
}

LOCAL(int)
element_resize(ElementObject* self, Py_ssize_t extra)
{
    Py_ssize_t size;
    PyObject* *children;

    /* make sure self->children can hold the given number of extra
       elements.  set an exception and return -1 if allocation failed */

    if (!self->extra)
        element_new_extra(self, NULL);

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
        if ((size_t)size > PY_SSIZE_T_MAX/sizeof(PyObject*))
            goto nomemory;
        if (size > INT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "too many children");
            return -1;
        }
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
element(PyObject* self, PyObject* args, PyObject* kw)
{
    PyObject* elem;

    PyObject* tag;
    PyObject* attrib = NULL;
    if (!PyArg_ParseTuple(args, "O|O!:Element", &tag,
                          &PyDict_Type, &attrib))
        return NULL;

    if (attrib || kw) {
        attrib = (attrib) ? PyDict_Copy(attrib) : PyDict_New();
        if (!attrib)
            return NULL;
        if (kw)
            PyDict_Update(attrib, kw);
    } else {
        Py_INCREF(Py_None);
        attrib = Py_None;
    }

    elem = element_new(tag, attrib);

    Py_DECREF(attrib);

    return elem;
}

static PyObject*
subelement(PyObject* self, PyObject* args, PyObject* kw)
{
    PyObject* elem;

    ElementObject* parent;
    PyObject* tag;
    PyObject* attrib = NULL;
    if (!PyArg_ParseTuple(args, "O!O|O!:SubElement",
                          &Element_Type, &parent, &tag,
                          &PyDict_Type, &attrib))
        return NULL;

    if (attrib || kw) {
        attrib = (attrib) ? PyDict_Copy(attrib) : PyDict_New();
        if (!attrib)
            return NULL;
        if (kw)
            PyDict_Update(attrib, kw);
    } else {
        Py_INCREF(Py_None);
        attrib = Py_None;
    }

    elem = element_new(tag, attrib);

    Py_DECREF(attrib);

    if (element_add_subelement(parent, elem) < 0) {
        Py_DECREF(elem);
        return NULL;
    }

    return elem;
}

static void
element_dealloc(ElementObject* self)
{
    if (self->extra)
        element_dealloc_extra(self);

    /* discard attributes */
    Py_DECREF(self->tag);
    Py_DECREF(JOIN_OBJ(self->text));
    Py_DECREF(JOIN_OBJ(self->tail));

    RELEASE(sizeof(ElementObject), "destroy element");

    PyObject_Del(self);
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
element_clear(ElementObject* self, PyObject* args)
{
    if (!PyArg_ParseTuple(args, ":clear"))
        return NULL;

    if (self->extra) {
        element_dealloc_extra(self);
        self->extra = NULL;
    }

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

    element = (ElementObject*) element_new(
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

    element = (ElementObject*) element_new(tag, attrib);

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
    id = PyInt_FromLong((Py_uintptr_t) self);
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

#if defined(Py_USING_UNICODE)
    if (PyUnicode_Check(tag)) {
        Py_UNICODE *p = PyUnicode_AS_UNICODE(tag);
        for (i = 0; i < PyUnicode_GET_SIZE(tag); i++) {
            if (p[i] == '{')
                check = 0;
            else if (p[i] == '}')
                check = 1;
            else if (check && PATHCHAR(p[i]))
                return 1;
        }
        return 0;
    }
#endif
    if (PyString_Check(tag)) {
        char *p = PyString_AS_STRING(tag);
        for (i = 0; i < PyString_GET_SIZE(tag); i++) {
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
    Py_ssize_t i;

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

    for (i = 0; i < PySequence_Fast_GET_SIZE(seq); i++) {
        PyObject* element = PySequence_Fast_GET_ITEM(seq, i);
        if (element_add_subelement(self, element) < 0) {
            Py_DECREF(seq);
            return NULL;
        }
    }

    Py_DECREF(seq);

    Py_RETURN_NONE;
}

static PyObject*
element_find(ElementObject* self, PyObject* args)
{
    int i;

    PyObject* tag;
    PyObject* namespaces = Py_None;
    if (!PyArg_ParseTuple(args, "O|O:find", &tag, &namespaces))
        return NULL;

    if (checkpath(tag) || namespaces != Py_None)
        return PyObject_CallMethod(
            elementpath_obj, "find", "OOO", self, tag, namespaces
            );

    if (!self->extra)
        Py_RETURN_NONE;
        
    for (i = 0; i < self->extra->length; i++) {
        PyObject* item = self->extra->children[i];
        int rc;
        if (!Element_CheckExact(item))
            continue;
        Py_INCREF(item);
        rc = PyObject_Compare(((ElementObject*)item)->tag, tag);
        if (rc == 0)
            return item;
        Py_DECREF(item);
        if (rc < 0 && PyErr_Occurred())
            return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject*
element_findtext(ElementObject* self, PyObject* args)
{
    int i;

    PyObject* tag;
    PyObject* default_value = Py_None;
    PyObject* namespaces = Py_None;
    if (!PyArg_ParseTuple(args, "O|OO:findtext", &tag, &default_value, &namespaces))
        return NULL;

    if (checkpath(tag) || namespaces != Py_None)
        return PyObject_CallMethod(
            elementpath_obj, "findtext", "OOOO", self, tag, default_value, namespaces
            );

    if (!self->extra) {
        Py_INCREF(default_value);
        return default_value;
    }

    for (i = 0; i < self->extra->length; i++) {
        ElementObject* item = (ElementObject*) self->extra->children[i];
        int rc;
        if (!Element_CheckExact(item))
            continue;
        Py_INCREF(item);
        rc = PyObject_Compare(item->tag, tag);
        if (rc == 0) {
            PyObject* text = element_get_text(item);
            if (text == Py_None) {
                Py_DECREF(item);
                return PyString_FromString("");
            }
            Py_XINCREF(text);
            Py_DECREF(item);
            return text;
        }
        Py_DECREF(item);
        if (rc < 0 && PyErr_Occurred())
            return NULL;
    }

    Py_INCREF(default_value);
    return default_value;
}

static PyObject*
element_findall(ElementObject* self, PyObject* args)
{
    int i;
    PyObject* out;

    PyObject* tag;
    PyObject* namespaces = Py_None;
    if (!PyArg_ParseTuple(args, "O|O:findall", &tag, &namespaces))
        return NULL;

    if (checkpath(tag) || namespaces != Py_None)
        return PyObject_CallMethod(
            elementpath_obj, "findall", "OOO", self, tag, namespaces
            );

    out = PyList_New(0);
    if (!out)
        return NULL;

    if (!self->extra)
        return out;

    for (i = 0; i < self->extra->length; i++) {
        PyObject* item = self->extra->children[i];
        int rc;
        if (!Element_CheckExact(item))
            continue;
        Py_INCREF(item);
        rc = PyObject_Compare(((ElementObject*)item)->tag, tag);
        if (rc == 0)
            rc = PyList_Append(out, item);
        Py_DECREF(item);
        if (rc < 0 && PyErr_Occurred()) {
            Py_DECREF(out);
            return NULL;
        }
    }

    return out;
}

static PyObject*
element_iterfind(ElementObject* self, PyObject* args)
{
    PyObject* tag;
    PyObject* namespaces = Py_None;
    if (!PyArg_ParseTuple(args, "O|O:iterfind", &tag, &namespaces))
        return NULL;

    return PyObject_CallMethod(
        elementpath_obj, "iterfind", "OOO", self, tag, namespaces
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

static PyObject*
element_iter(ElementObject* self, PyObject* args)
{
    PyObject* result;
    
    PyObject* tag = Py_None;
    if (!PyArg_ParseTuple(args, "|O:iter", &tag))
        return NULL;

    if (!elementtree_iter_obj) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "iter helper not found"
            );
        return NULL;
    }

    args = PyTuple_New(2);
    if (!args)
        return NULL;

    Py_INCREF(self); PyTuple_SET_ITEM(args, 0, (PyObject*) self);
    Py_INCREF(tag);  PyTuple_SET_ITEM(args, 1, (PyObject*) tag);

    result = PyObject_CallObject(elementtree_iter_obj, args);

    Py_DECREF(args);

    return result;
}


static PyObject*
element_itertext(ElementObject* self, PyObject* args)
{
    PyObject* result;
    
    if (!PyArg_ParseTuple(args, ":itertext"))
        return NULL;

    if (!elementtree_itertext_obj) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "itertext helper not found"
            );
        return NULL;
    }

    args = PyTuple_New(1);
    if (!args)
        return NULL;

    Py_INCREF(self); PyTuple_SET_ITEM(args, 0, (PyObject*) self);

    result = PyObject_CallObject(elementtree_itertext_obj, args);

    Py_DECREF(args);

    return result;
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
        element_new_extra(self, NULL);

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

    elem = element_new(tag, attrib);

    Py_DECREF(attrib);

    return elem;
}

static PyObject*
element_reduce(ElementObject* self, PyObject* args)
{
    if (!PyArg_ParseTuple(args, ":__reduce__"))
        return NULL;

    /* Hack alert: This method is used to work around a __copy__
       problem on certain 2.3 and 2.4 versions.  To save time and
       simplify the code, we create the copy in here, and use a dummy
       copyelement helper to trick the copy module into doing the
       right thing. */

    if (!elementtree_copyelement_obj) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "copyelement helper not found"
            );
        return NULL;
    }

    return Py_BuildValue(
        "O(N)", elementtree_copyelement_obj, element_copy(self, args)
        );
}

static PyObject*
element_remove(ElementObject* self, PyObject* args)
{
    int i;
    int rc;
    PyObject* element;
    PyObject* found;

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
        rc = PyObject_Compare(self->extra->children[i], element);
        if (rc == 0)
            break;
        if (rc < 0 && PyErr_Occurred())
            return NULL;
    }

    if (i >= self->extra->length) {
        /* element is not in children, so raise exception */
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
    PyObject *repr, *tag;

    tag = PyObject_Repr(self->tag);
    if (!tag)
        return NULL;

    repr = PyString_FromFormat("<Element %s at %p>",
                               PyString_AS_STRING(tag), self);

    Py_DECREF(tag);

    return repr;
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
        element_new_extra(self, NULL);

    attrib = element_get_attrib(self);
    if (!attrib)
        return NULL;

    if (PyDict_SetItem(attrib, key, value) < 0)
        return NULL;

    Py_RETURN_NONE;
}

static int
element_setitem(PyObject* self_, Py_ssize_t index_, PyObject* item)
{
    ElementObject* self = (ElementObject*) self_;
    int i, index;
    PyObject* old;

    if (!self->extra || index_ < 0 || index_ >= self->extra->length) {
        PyErr_SetString(
            PyExc_IndexError,
            "child assignment index out of range");
        return -1;
    }
    index = (int)index_;

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

#if (PY_VERSION_HEX < 0x02050000)
    if (PyInt_Check(item) || PyLong_Check(item)) {
        long i = PyInt_AsLong(item);
#else
    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
#endif

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

        if (PySlice_GetIndicesEx((PySliceObject *)item,
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

#if (PY_VERSION_HEX < 0x02050000)
    if (PyInt_Check(item) || PyLong_Check(item)) {
        long i = PyInt_AsLong(item);
#else
    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
#endif

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
            element_new_extra(self, NULL);

        if (PySlice_GetIndicesEx((PySliceObject *)item,
                self->extra->length,
                &start, &stop, &step, &slicelen) < 0) {
            return -1;
        }
        assert(slicelen <= self->extra->length);

        if (value == NULL)
            newlen = 0;
        else {
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
            Py_XDECREF(seq);
            PyErr_Format(PyExc_ValueError,
#if (PY_VERSION_HEX < 0x02050000)
                "attempt to assign sequence of size %d "
                "to extended slice of size %d",
                (int)newlen, (int)slicelen
#else
                "attempt to assign sequence of size %zd "
                "to extended slice of size %zd",
                newlen, slicelen
#endif
                );
            return -1;
        }


        /* Resize before creating the recycle bin, to prevent refleaks. */
        if (newlen > slicelen) {
            if (element_resize(self, newlen - slicelen) < 0) {
                Py_XDECREF(seq);
                return -1;
            }
        }
        assert(newlen - slicelen <= INT_MAX - self->extra->length);
        assert(newlen - slicelen >= -self->extra->length);

        if (slicelen > 0) {
            /* to avoid recursive calls to this method (via decref), move
               old items to the recycle bin here, and get rid of them when
               we're done modifying the element */
            recycle = PyList_New(slicelen);
            if (!recycle) {
                Py_XDECREF(seq);
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

        self->extra->length += (int)(newlen - slicelen);

        Py_XDECREF(seq);

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

    {"clear", (PyCFunction) element_clear, METH_VARARGS},

    {"get", (PyCFunction) element_get, METH_VARARGS},
    {"set", (PyCFunction) element_set, METH_VARARGS},

    {"find", (PyCFunction) element_find, METH_VARARGS},
    {"findtext", (PyCFunction) element_findtext, METH_VARARGS},
    {"findall", (PyCFunction) element_findall, METH_VARARGS},

    {"append", (PyCFunction) element_append, METH_VARARGS},
    {"extend", (PyCFunction) element_extend, METH_VARARGS},
    {"insert", (PyCFunction) element_insert, METH_VARARGS},
    {"remove", (PyCFunction) element_remove, METH_VARARGS},

    {"iter", (PyCFunction) element_iter, METH_VARARGS},
    {"itertext", (PyCFunction) element_itertext, METH_VARARGS},
    {"iterfind", (PyCFunction) element_iterfind, METH_VARARGS},

    {"getiterator", (PyCFunction) element_iter, METH_VARARGS},
    {"getchildren", (PyCFunction) element_getchildren, METH_VARARGS},

    {"items", (PyCFunction) element_items, METH_VARARGS},
    {"keys", (PyCFunction) element_keys, METH_VARARGS},

    {"makeelement", (PyCFunction) element_makeelement, METH_VARARGS},

    {"__copy__", (PyCFunction) element_copy, METH_VARARGS},
    {"__deepcopy__", (PyCFunction) element_deepcopy, METH_VARARGS},

    /* Some 2.3 and 2.4 versions do not handle the __copy__ method on
       C objects correctly, so we have to fake it using a __reduce__-
       based hack (see the element_reduce implementation above for
       details). */

    /* The behaviour has been changed in 2.3.5 and 2.4.1, so we're
       using a runtime test to figure out if we need to fake things
       or now (see the init code below).  The following entry is
       enabled only if the hack is needed. */

    {"!__reduce__", (PyCFunction) element_reduce, METH_VARARGS},

    {NULL, NULL}
};

static PyObject*  
element_getattr(ElementObject* self, char* name)
{
    PyObject* res;

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
    res = Py_FindMethod(element_methods, (PyObject*) self, name);
    if (res)
        return res;

    PyErr_Clear();

    /* less common attributes */
    if (strcmp(name, "tail") == 0) {
        res = element_get_tail(self);
    } else if (strcmp(name, "attrib") == 0) {
        if (!self->extra)
            element_new_extra(self, NULL);
        res = element_get_attrib(self);
    } else {
        PyErr_SetString(PyExc_AttributeError, name);
        return NULL;
    }

    if (!res)
        return NULL;

    Py_INCREF(res);
    return res;
}

static int
element_setattr(ElementObject* self, const char* name, PyObject* value)
{
    if (value == NULL) {
        PyErr_SetString(
            PyExc_AttributeError,
            "can't delete element attributes"
            );
        return -1;
    }

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
            element_new_extra(self, NULL);
        Py_DECREF(self->extra->attrib);
        self->extra->attrib = value;
        Py_INCREF(self->extra->attrib);
    } else {
        PyErr_SetString(PyExc_AttributeError, name);
        return -1;
    }

    return 0;
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

statichere PyTypeObject Element_Type = {
    PyObject_HEAD_INIT(NULL)
    0, "Element", sizeof(ElementObject), 0,
    /* methods */
    (destructor)element_dealloc, /* tp_dealloc */
    0, /* tp_print */
    (getattrfunc)element_getattr, /* tp_getattr */
    (setattrfunc)element_setattr, /* tp_setattr */
    0, /* tp_compare */
    (reprfunc)element_repr, /* tp_repr */
    0, /* tp_as_number */
    &element_as_sequence, /* tp_as_sequence */
    &element_as_mapping, /* tp_as_mapping */
};

/* ==================================================================== */
/* the tree builder type */

typedef struct {
    PyObject_HEAD

    PyObject* root; /* root node (first created node) */

    ElementObject* this; /* current node */
    ElementObject* last; /* most recently created node */

    PyObject* data; /* data collector (string or list), or NULL */

    PyObject* stack; /* element stack */
    Py_ssize_t index; /* current stack size (0=empty) */

    /* element tracing */
    PyObject* events; /* list of events, or NULL if not collecting */
    PyObject* start_event_obj; /* event objects (NULL to ignore) */
    PyObject* end_event_obj;
    PyObject* start_ns_event_obj;
    PyObject* end_ns_event_obj;

} TreeBuilderObject;

staticforward PyTypeObject TreeBuilder_Type;

#define TreeBuilder_CheckExact(op) (Py_TYPE(op) == &TreeBuilder_Type)

/* -------------------------------------------------------------------- */
/* constructor and destructor */

LOCAL(PyObject*)
treebuilder_new(void)
{
    TreeBuilderObject* self;

    self = PyObject_New(TreeBuilderObject, &TreeBuilder_Type);
    if (self == NULL)
        return NULL;

    self->root = NULL;

    Py_INCREF(Py_None);
    self->this = (ElementObject*) Py_None;

    Py_INCREF(Py_None);
    self->last = (ElementObject*) Py_None;

    self->data = NULL;

    self->stack = PyList_New(20);
    self->index = 0;

    self->events = NULL;
    self->start_event_obj = self->end_event_obj = NULL;
    self->start_ns_event_obj = self->end_ns_event_obj = NULL;

    ALLOC(sizeof(TreeBuilderObject), "create treebuilder");

    return (PyObject*) self;
}

static PyObject*
treebuilder(PyObject* self_, PyObject* args)
{
    if (!PyArg_ParseTuple(args, ":TreeBuilder"))
        return NULL;

    return treebuilder_new();
}

static void
treebuilder_dealloc(TreeBuilderObject* self)
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
    Py_XDECREF(self->root);

    RELEASE(sizeof(TreeBuilderObject), "destroy treebuilder");

    PyObject_Del(self);
}

LOCAL(int)
treebuilder_append_event(TreeBuilderObject *self, PyObject *action,
                         PyObject *node)
{
    if (action != NULL) {
        PyObject *res = PyTuple_Pack(2, action, node);
        if (res == NULL)
            return -1;
        if (PyList_Append(self->events, res) < 0) {
            Py_DECREF(res);
            return -1;
        }
        Py_DECREF(res);
    }
    return 0;
}

/* -------------------------------------------------------------------- */
/* handlers */

LOCAL(PyObject*)
treebuilder_handle_xml(TreeBuilderObject* self, PyObject* encoding,
                       PyObject* standalone)
{
    Py_RETURN_NONE;
}

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

    node = element_new(tag, attrib);
    if (!node)
        return NULL;

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
        if (self->last == (ElementObject*) Py_None) {
            /* ignore calls to data before the first call to start */
            Py_RETURN_NONE;
        }
        /* store the first item as is */
        Py_INCREF(data); self->data = data;
    } else {
        /* more than one item; use a list to collect items */
        if (PyString_CheckExact(self->data) && Py_REFCNT(self->data) == 1 &&
            PyString_CheckExact(data) && PyString_GET_SIZE(data) == 1) {
            /* expat often generates single character data sections; handle
               the most common case by resizing the existing string... */
            Py_ssize_t size = PyString_GET_SIZE(self->data);
            if (_PyString_Resize(&self->data, size + 1) < 0)
                return NULL;
            PyString_AS_STRING(self->data)[size] = PyString_AS_STRING(data)[0];
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

    if (treebuilder_append_event(self, self->end_event_obj, (PyObject*)self->last) < 0)
        return NULL;

    Py_INCREF(self->last);
    return (PyObject*) self->last;
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

static PyObject*
treebuilder_xml(TreeBuilderObject* self, PyObject* args)
{
    PyObject* encoding;
    PyObject* standalone;
    if (!PyArg_ParseTuple(args, "OO:xml", &encoding, &standalone))
        return NULL;

    return treebuilder_handle_xml(self, encoding, standalone);
}

static PyMethodDef treebuilder_methods[] = {
    {"data", (PyCFunction) treebuilder_data, METH_VARARGS},
    {"start", (PyCFunction) treebuilder_start, METH_VARARGS},
    {"end", (PyCFunction) treebuilder_end, METH_VARARGS},
    {"xml", (PyCFunction) treebuilder_xml, METH_VARARGS},
    {"close", (PyCFunction) treebuilder_close, METH_VARARGS},
    {NULL, NULL}
};

static PyObject*  
treebuilder_getattr(TreeBuilderObject* self, char* name)
{
    return Py_FindMethod(treebuilder_methods, (PyObject*) self, name);
}

statichere PyTypeObject TreeBuilder_Type = {
    PyObject_HEAD_INIT(NULL)
    0, "TreeBuilder", sizeof(TreeBuilderObject), 0,
    /* methods */
    (destructor)treebuilder_dealloc, /* tp_dealloc */
    0, /* tp_print */
    (getattrfunc)treebuilder_getattr, /* tp_getattr */
};

/* ==================================================================== */
/* the expat interface */

#if defined(USE_EXPAT)

#include "expat.h"

#if defined(USE_PYEXPAT_CAPI)
#include "pyexpat.h"
static struct PyExpat_CAPI* expat_capi;
#define EXPAT(func) (expat_capi->func)
#else
#define EXPAT(func) (XML_##func)
#endif

typedef struct {
    PyObject_HEAD

    XML_Parser parser;

    PyObject* target;
    PyObject* entity;

    PyObject* names;

    PyObject* handle_xml;

    PyObject* handle_start;
    PyObject* handle_data;
    PyObject* handle_end;

    PyObject* handle_comment;
    PyObject* handle_pi;

    PyObject* handle_close;

} XMLParserObject;

staticforward PyTypeObject XMLParser_Type;

/* helpers */

#if defined(Py_USING_UNICODE)
LOCAL(int)
checkstring(const char* string, int size)
{
    int i;

    /* check if an 8-bit string contains UTF-8 characters */
    for (i = 0; i < size; i++)
        if (string[i] & 0x80)
            return 1;

    return 0;
}
#endif

LOCAL(PyObject*)
makestring(const char* string, int size)
{
    /* convert a UTF-8 string to either a 7-bit ascii string or a
       Unicode string */

#if defined(Py_USING_UNICODE)
    if (checkstring(string, size))
        return PyUnicode_DecodeUTF8(string, size, "strict");
#endif

    return PyString_FromStringAndSize(string, size);
}

LOCAL(PyObject*)
makeuniversal(XMLParserObject* self, const char* string)
{
    /* convert a UTF-8 tag/attribute name from the expat parser
       to a universal name string */

    int size = strlen(string);
    PyObject* key;
    PyObject* value;

    /* look the 'raw' name up in the names dictionary */
    key = PyString_FromStringAndSize(string, size);
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
            tag = PyString_FromStringAndSize(NULL, size+1);
            p = PyString_AS_STRING(tag);
            p[0] = '{';
            memcpy(p+1, string, size);
            size++;
        } else {
            /* plain name; use key as tag */
            Py_INCREF(key);
            tag = key;
        }
        
        /* decode universal name */
#if defined(Py_USING_UNICODE)
        /* inline makestring, to avoid duplicating the source string if
           it's not an utf-8 string */
        p = PyString_AS_STRING(tag);
        if (checkstring(p, size)) {
            value = PyUnicode_DecodeUTF8(p, size, "strict");
            Py_DECREF(tag);
            if (!value) {
                Py_DECREF(key);
                return NULL;
            }
        } else
#endif
            value = tag; /* use tag as is */

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

static void
expat_set_error(const char* message, int line, int column)
{
    PyObject *error;
    PyObject *position;
    char buffer[256];

    sprintf(buffer, "%s: line %d, column %d", message, line, column);

    error = PyObject_CallFunction(elementtree_parseerror_obj, "s", buffer);
    if (!error)
        return;

    /* add position attribute */
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

    key = makestring(data_in + 1, data_len - 2);
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
        char message[128];
        sprintf(message, "undefined entity &%.100s;", PyString_AS_STRING(key));
        expat_set_error(
            message,
            EXPAT(GetErrorLineNumber)(self->parser),
            EXPAT(GetErrorColumnNumber)(self->parser)
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
            PyObject* value = makestring(attrib_in[1], strlen(attrib_in[1]));
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

    if (TreeBuilder_CheckExact(self->target))
        /* shortcut */
        res = treebuilder_handle_start((TreeBuilderObject*) self->target,
                                       tag, attrib);
    else if (self->handle_start) {
        if (attrib == Py_None) {
            Py_DECREF(attrib);
            attrib = PyDict_New();
            if (!attrib)
                return;
        }
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

    data = makestring(data_in, data_len);
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
    TreeBuilderObject *target = (TreeBuilderObject*) self->target;
    PyObject *parcel;
    PyObject *sprefix = NULL;
    PyObject *suri = NULL;

    if (PyErr_Occurred())
        return;

    if (!target->events || !target->start_ns_event_obj)
        return;

    if (uri)
        suri = makestring(uri, strlen(uri));
    else
        suri = PyString_FromStringAndSize("", 0);
    if (!suri)
        return;

    if (prefix)
        sprefix = makestring(prefix, strlen(prefix));
    else
        sprefix = PyString_FromStringAndSize("", 0);
    if (!sprefix) {
        Py_DECREF(suri);
        return;
    }

    parcel = PyTuple_Pack(2, sprefix, suri);
    Py_DECREF(sprefix);
    Py_DECREF(suri);
    if (!parcel)
        return;
    treebuilder_append_event(target, target->start_ns_event_obj, parcel);
    Py_DECREF(parcel);
}

static void
expat_end_ns_handler(XMLParserObject* self, const XML_Char* prefix_in)
{
    TreeBuilderObject *target = (TreeBuilderObject*) self->target;

    if (PyErr_Occurred())
        return;

    if (!target->events)
        return;

    treebuilder_append_event(target, target->end_ns_event_obj, Py_None);
}

static void
expat_comment_handler(XMLParserObject* self, const XML_Char* comment_in)
{
    PyObject* comment;
    PyObject* res;

    if (self->handle_comment) {
        comment = makestring(comment_in, strlen(comment_in));
        if (comment) {
            res = PyObject_CallFunction(self->handle_comment, "O", comment);
            Py_XDECREF(res);
            Py_DECREF(comment);
        }
    }
}

static void
expat_pi_handler(XMLParserObject* self, const XML_Char* target_in,
                 const XML_Char* data_in)
{
    PyObject* target;
    PyObject* data;
    PyObject* res;

    if (self->handle_pi) {
        target = makestring(target_in, strlen(target_in));
        data = makestring(data_in, strlen(data_in));
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

#if defined(Py_USING_UNICODE)
static int
expat_unknown_encoding_handler(XMLParserObject *self, const XML_Char *name,
                               XML_Encoding *info)
{
    PyObject* u;
    Py_UNICODE* p;
    unsigned char s[256];
    int i;

    memset(info, 0, sizeof(XML_Encoding));

    for (i = 0; i < 256; i++)
        s[i] = i;
    
    u = PyUnicode_Decode((char*) s, 256, name, "replace");
    if (!u)
        return XML_STATUS_ERROR;

    if (PyUnicode_GET_SIZE(u) != 256) {
        Py_DECREF(u);
        PyErr_SetString(PyExc_ValueError,
                        "multi-byte encodings are not supported");
        return XML_STATUS_ERROR;
    }

    p = PyUnicode_AS_UNICODE(u);

    for (i = 0; i < 256; i++) {
        if (p[i] != Py_UNICODE_REPLACEMENT_CHARACTER)
            info->map[i] = p[i];
        else
            info->map[i] = -1;
    }

    Py_DECREF(u);

    return XML_STATUS_OK;
}
#endif

/* -------------------------------------------------------------------- */
/* constructor and destructor */

static PyObject*
xmlparser(PyObject* self_, PyObject* args, PyObject* kw)
{
    XMLParserObject* self;
    /* FIXME: does this need to be static? */
    static XML_Memory_Handling_Suite memory_handler;

    PyObject* target = NULL;
    char* encoding = NULL;
    static char* kwlist[] = { "target", "encoding", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|Oz:XMLParser", kwlist,
                                     &target, &encoding))
        return NULL;

#if defined(USE_PYEXPAT_CAPI)
    if (!expat_capi) {
        PyErr_SetString(
            PyExc_RuntimeError, "cannot load dispatch table from pyexpat"
            );
        return NULL;
    }
#endif

    self = PyObject_New(XMLParserObject, &XMLParser_Type);
    if (self == NULL)
        return NULL;

    self->entity = PyDict_New();
    if (!self->entity) {
        PyObject_Del(self);
        return NULL;
    }
     
    self->names = PyDict_New();
    if (!self->names) {
        PyObject_Del(self->entity);
        PyObject_Del(self);
        return NULL;
    }

    memory_handler.malloc_fcn = PyObject_Malloc;
    memory_handler.realloc_fcn = PyObject_Realloc;
    memory_handler.free_fcn = PyObject_Free;

    self->parser = EXPAT(ParserCreate_MM)(encoding, &memory_handler, "}");
    if (!self->parser) {
        PyObject_Del(self->names);
        PyObject_Del(self->entity);
        PyObject_Del(self);
        PyErr_NoMemory();
        return NULL;
    }

    /* setup target handlers */
    if (!target) {
        target = treebuilder_new();
        if (!target) {
            EXPAT(ParserFree)(self->parser);
            PyObject_Del(self->names);
            PyObject_Del(self->entity);
            PyObject_Del(self);
            return NULL;
        }
    } else
        Py_INCREF(target);
    self->target = target;

    self->handle_xml = PyObject_GetAttrString(target, "xml");
    self->handle_start = PyObject_GetAttrString(target, "start");
    self->handle_data = PyObject_GetAttrString(target, "data");
    self->handle_end = PyObject_GetAttrString(target, "end");
    self->handle_comment = PyObject_GetAttrString(target, "comment");
    self->handle_pi = PyObject_GetAttrString(target, "pi");
    self->handle_close = PyObject_GetAttrString(target, "close");

    PyErr_Clear();

    /* configure parser */
    EXPAT(SetUserData)(self->parser, self);
    EXPAT(SetElementHandler)(
        self->parser,
        (XML_StartElementHandler) expat_start_handler,
        (XML_EndElementHandler) expat_end_handler
        );
    EXPAT(SetDefaultHandlerExpand)(
        self->parser,
        (XML_DefaultHandler) expat_default_handler
        );
    EXPAT(SetCharacterDataHandler)(
        self->parser,
        (XML_CharacterDataHandler) expat_data_handler
        );
    if (self->handle_comment)
        EXPAT(SetCommentHandler)(
            self->parser,
            (XML_CommentHandler) expat_comment_handler
            );
    if (self->handle_pi)
        EXPAT(SetProcessingInstructionHandler)(
            self->parser,
            (XML_ProcessingInstructionHandler) expat_pi_handler
            );
#if defined(Py_USING_UNICODE)
    EXPAT(SetUnknownEncodingHandler)(
        self->parser,
        (XML_UnknownEncodingHandler) expat_unknown_encoding_handler, NULL
        );
#endif

    ALLOC(sizeof(XMLParserObject), "create expatparser");

    return (PyObject*) self;
}

static void
xmlparser_dealloc(XMLParserObject* self)
{
    EXPAT(ParserFree)(self->parser);

    Py_XDECREF(self->handle_close);
    Py_XDECREF(self->handle_pi);
    Py_XDECREF(self->handle_comment);
    Py_XDECREF(self->handle_end);
    Py_XDECREF(self->handle_data);
    Py_XDECREF(self->handle_start);
    Py_XDECREF(self->handle_xml);

    Py_DECREF(self->target);
    Py_DECREF(self->entity);
    Py_DECREF(self->names);

    RELEASE(sizeof(XMLParserObject), "destroy expatparser");

    PyObject_Del(self);
}

/* -------------------------------------------------------------------- */
/* methods (in alphabetical order) */

LOCAL(PyObject*)
expat_parse(XMLParserObject* self, char* data, int data_len, int final)
{
    int ok;

    ok = EXPAT(Parse)(self->parser, data, data_len, final);

    if (PyErr_Occurred())
        return NULL;

    if (!ok) {
        expat_set_error(
            EXPAT(ErrorString)(EXPAT(GetErrorCode)(self->parser)),
            EXPAT(GetErrorLineNumber)(self->parser),
            EXPAT(GetErrorColumnNumber)(self->parser)
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

        if (!PyString_CheckExact(buffer) || PyString_GET_SIZE(buffer) == 0) {
            Py_DECREF(buffer);
            break;
        }

        if (PyString_GET_SIZE(buffer) > INT_MAX) {
            Py_DECREF(buffer);
            Py_DECREF(reader);
            PyErr_SetString(PyExc_OverflowError, "size does not fit in an int");
            return NULL;
        }
        res = expat_parse(
            self, PyString_AS_STRING(buffer), (int)PyString_GET_SIZE(buffer), 0
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
xmlparser_setevents(XMLParserObject* self, PyObject* args)
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
            "event handling only supported for cElementTree.Treebuilder "
            "targets"
            );
        return NULL;
    }

    target = (TreeBuilderObject*) self->target;

    Py_INCREF(events);
    Py_XDECREF(target->events);
    target->events = events;

    /* clear out existing events */
    Py_CLEAR(target->start_event_obj);
    Py_CLEAR(target->end_event_obj);
    Py_CLEAR(target->start_ns_event_obj);
    Py_CLEAR(target->end_ns_event_obj);

    if (event_set == Py_None) {
        /* default is "end" only */
        target->end_event_obj = PyString_FromString("end");
        Py_RETURN_NONE;
    }

    if (!PyTuple_Check(event_set)) /* FIXME: handle arbitrary sequences */
        goto error;

    for (i = 0; i < PyTuple_GET_SIZE(event_set); i++) {
        PyObject* item = PyTuple_GET_ITEM(event_set, i);
        char* event;
        if (!PyString_Check(item))
            goto error;
        event = PyString_AS_STRING(item);
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
    {NULL, NULL}
};

static PyObject*  
xmlparser_getattr(XMLParserObject* self, char* name)
{
    PyObject* res;

    res = Py_FindMethod(xmlparser_methods, (PyObject*) self, name);
    if (res)
        return res;

    PyErr_Clear();

    if (strcmp(name, "entity") == 0)
        res = self->entity;
    else if (strcmp(name, "target") == 0)
        res = self->target;
    else if (strcmp(name, "version") == 0) {
        char buffer[100];
        sprintf(buffer, "Expat %d.%d.%d", XML_MAJOR_VERSION,
                XML_MINOR_VERSION, XML_MICRO_VERSION);
        return PyString_FromString(buffer);
    } else {
        PyErr_SetString(PyExc_AttributeError, name);
        return NULL;
    }

    Py_INCREF(res);
    return res;
}

statichere PyTypeObject XMLParser_Type = {
    PyObject_HEAD_INIT(NULL)
    0, "XMLParser", sizeof(XMLParserObject), 0,
    /* methods */
    (destructor)xmlparser_dealloc, /* tp_dealloc */
    0, /* tp_print */
    (getattrfunc)xmlparser_getattr, /* tp_getattr */
};

#endif

/* ==================================================================== */
/* python module interface */

static PyMethodDef _functions[] = {
    {"Element", (PyCFunction) element, METH_VARARGS|METH_KEYWORDS},
    {"SubElement", (PyCFunction) subelement, METH_VARARGS|METH_KEYWORDS},
    {"TreeBuilder", (PyCFunction) treebuilder, METH_VARARGS},
#if defined(USE_EXPAT)
    {"XMLParser", (PyCFunction) xmlparser, METH_VARARGS|METH_KEYWORDS},
    {"XMLTreeBuilder", (PyCFunction) xmlparser, METH_VARARGS|METH_KEYWORDS},
#endif
    {NULL, NULL}
};

DL_EXPORT(void)
init_elementtree(void)
{
    PyObject* m;
    PyObject* g;
    char* bootstrap;

    /* Patch object type */
    Py_TYPE(&Element_Type) = Py_TYPE(&TreeBuilder_Type) = &PyType_Type;
#if defined(USE_EXPAT)
    Py_TYPE(&XMLParser_Type) = &PyType_Type;
#endif

    m = Py_InitModule("_elementtree", _functions);
    if (!m)
        return;

    /* python glue code */

    g = PyDict_New();
    if (!g)
        return;

    PyDict_SetItemString(g, "__builtins__", PyEval_GetBuiltins());

    bootstrap = (

        "from copy import copy, deepcopy\n"

        "try:\n"
        "  from xml.etree import ElementTree\n"
        "except ImportError:\n"
        "  import ElementTree\n"
        "ET = ElementTree\n"
        "del ElementTree\n"

        "import _elementtree as cElementTree\n"

        "try:\n" /* check if copy works as is */
        "  copy(cElementTree.Element('x'))\n"
        "except:\n"
        "  def copyelement(elem):\n"
        "    return elem\n"

        "class CommentProxy:\n"
        " def __call__(self, text=None):\n"
        "  element = cElementTree.Element(ET.Comment)\n"
        "  element.text = text\n"
        "  return element\n"
        " def __cmp__(self, other):\n"
        "  return cmp(ET.Comment, other)\n"
        "cElementTree.Comment = CommentProxy()\n"

        "class ElementTree(ET.ElementTree):\n" /* public */
        "  def parse(self, source, parser=None):\n"
        "    close_source = False\n"
        "    if not hasattr(source, 'read'):\n"
        "      source = open(source, 'rb')\n"
        "      close_source = False\n"
        "    try:\n"
        "      if parser is not None:\n"
        "        while 1:\n"
        "          data = source.read(65536)\n"
        "          if not data:\n"
        "            break\n"
        "          parser.feed(data)\n"
        "        self._root = parser.close()\n"
        "      else:\n" 
        "        parser = cElementTree.XMLParser()\n"
        "        self._root = parser._parse(source)\n"
        "      return self._root\n"
        "    finally:\n"
        "      if close_source:\n"
        "        source.close()\n"
        "cElementTree.ElementTree = ElementTree\n"

        "def iter(node, tag=None):\n" /* helper */
        "  if tag == '*':\n"
        "    tag = None\n"
        "  if tag is None or node.tag == tag:\n"
        "    yield node\n"
        "  for node in node:\n"
        "    for node in iter(node, tag):\n"
        "      yield node\n"

        "def itertext(node):\n" /* helper */
        "  if node.text:\n"
        "    yield node.text\n"
        "  for e in node:\n"
        "    for s in e.itertext():\n"
        "      yield s\n"
        "    if e.tail:\n"
        "      yield e.tail\n"

        "def parse(source, parser=None):\n" /* public */
        "  tree = ElementTree()\n"
        "  tree.parse(source, parser)\n"
        "  return tree\n"
        "cElementTree.parse = parse\n"

        "class iterparse(object):\n"
        " root = None\n"
        " def __init__(self, file, events=None):\n"
        "  self._close_file = False\n"
        "  if not hasattr(file, 'read'):\n"
        "    file = open(file, 'rb')\n"
        "    self._close_file = True\n"
        "  self._file = file\n"
        "  self._events = []\n"
        "  self._index = 0\n"
        "  self._error = None\n"
        "  self.root = self._root = None\n"
        "  b = cElementTree.TreeBuilder()\n"
        "  self._parser = cElementTree.XMLParser(b)\n"
        "  self._parser._setevents(self._events, events)\n"
        " def next(self):\n"
        "  while 1:\n"
        "    try:\n"
        "      item = self._events[self._index]\n"
        "      self._index += 1\n"
        "      return item\n"
        "    except IndexError:\n"
        "      pass\n"
        "    if self._error:\n"
        "      e = self._error\n"
        "      self._error = None\n"
        "      raise e\n"
        "    if self._parser is None:\n"
        "      self.root = self._root\n"
        "      if self._close_file:\n"
        "        self._file.close()\n"
        "      raise StopIteration\n"
        "    # load event buffer\n"
        "    del self._events[:]\n"
        "    self._index = 0\n"
        "    data = self._file.read(16384)\n"
        "    if data:\n"
        "      try:\n"
        "        self._parser.feed(data)\n"
        "      except SyntaxError as exc:\n"
        "        self._error = exc\n"
        "    else:\n"
        "      self._root = self._parser.close()\n"
        "      self._parser = None\n"
        " def __iter__(self):\n"
        "  return self\n"
        "cElementTree.iterparse = iterparse\n"

        "class PIProxy:\n"
        " def __call__(self, target, text=None):\n"
        "  element = cElementTree.Element(ET.PI)\n"
        "  element.text = target\n"
        "  if text:\n"
        "    element.text = element.text + ' ' + text\n"
        "  return element\n"
        " def __cmp__(self, other):\n"
        "  return cmp(ET.PI, other)\n"
        "cElementTree.PI = cElementTree.ProcessingInstruction = PIProxy()\n"

        "def XML(text):\n" /* public */
        "  parser = cElementTree.XMLParser()\n"
        "  parser.feed(text)\n"
        "  return parser.close()\n"
        "cElementTree.XML = cElementTree.fromstring = XML\n"

        "def XMLID(text):\n" /* public */
        "  tree = XML(text)\n"
        "  ids = {}\n"
        "  for elem in tree.iter():\n"
        "    id = elem.get('id')\n"
        "    if id:\n"
        "      ids[id] = elem\n"
        "  return tree, ids\n"
        "cElementTree.XMLID = XMLID\n"

        "try:\n"
        " register_namespace = ET.register_namespace\n"
        "except AttributeError:\n"
        " def register_namespace(prefix, uri):\n"
        "  ET._namespace_map[uri] = prefix\n"
        "cElementTree.register_namespace = register_namespace\n"

        "cElementTree.dump = ET.dump\n"
        "cElementTree.ElementPath = ElementPath = ET.ElementPath\n"
        "cElementTree.iselement = ET.iselement\n"
        "cElementTree.QName = ET.QName\n"
        "cElementTree.tostring = ET.tostring\n"
        "cElementTree.fromstringlist = ET.fromstringlist\n"
        "cElementTree.tostringlist = ET.tostringlist\n"
        "cElementTree.VERSION = '" VERSION "'\n"
        "cElementTree.__version__ = '" VERSION "'\n"

       );

    if (!PyRun_String(bootstrap, Py_file_input, g, NULL))
        return;

    elementpath_obj = PyDict_GetItemString(g, "ElementPath");

    elementtree_copyelement_obj = PyDict_GetItemString(g, "copyelement");
    if (elementtree_copyelement_obj) {
        /* reduce hack needed; enable reduce method */
        PyMethodDef* mp;
        for (mp = element_methods; mp->ml_name; mp++)
            if (mp->ml_meth == (PyCFunction) element_reduce) {
                mp->ml_name = "__reduce__";
                break;
            }
    } else
        PyErr_Clear();

    elementtree_deepcopy_obj = PyDict_GetItemString(g, "deepcopy");
    elementtree_iter_obj = PyDict_GetItemString(g, "iter");
    elementtree_itertext_obj = PyDict_GetItemString(g, "itertext");

#if defined(USE_PYEXPAT_CAPI)
    /* link against pyexpat, if possible */
    expat_capi = PyCapsule_Import(PyExpat_CAPSULE_NAME, 0);
    if (expat_capi) {
        /* check that it's usable */
        if (strcmp(expat_capi->magic, PyExpat_CAPI_MAGIC) != 0 ||
            expat_capi->size < sizeof(struct PyExpat_CAPI) ||
            expat_capi->MAJOR_VERSION != XML_MAJOR_VERSION ||
            expat_capi->MINOR_VERSION != XML_MINOR_VERSION ||
            expat_capi->MICRO_VERSION != XML_MICRO_VERSION)
            expat_capi = NULL;
    }
#endif

    elementtree_parseerror_obj = PyErr_NewException(
        "cElementTree.ParseError", PyExc_SyntaxError, NULL
        );
    Py_INCREF(elementtree_parseerror_obj);
    PyModule_AddObject(m, "ParseError", elementtree_parseerror_obj);
}
