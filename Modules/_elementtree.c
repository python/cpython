/*
 * ElementTree
 * $Id: _elementtree.c 2657 2006-03-12 20:50:32Z fredrik $
 *
 * elementtree accelerator
 *
 * History:
 * 1999-06-20 fl  created (as part of sgmlop)
 * 2001-05-29 fl  effdom edition
 * 2001-06-05 fl  backported to unix; fixed bogus free in clear
 * 2001-07-10 fl  added findall helper
 * 2003-02-27 fl  elementtree edition (alpha)
 * 2004-06-03 fl  updates for elementtree 1.2
 * 2005-01-05 fl  added universal name cache, Element/SubElement factories
 * 2005-01-06 fl  moved python helpers into C module; removed 1.5.2 support
 * 2005-01-07 fl  added 2.1 support; work around broken __copy__ in 2.3
 * 2005-01-08 fl  added makeelement method; fixed path support
 * 2005-01-10 fl  optimized memory usage
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
 *
 * Copyright (c) 1999-2006 by Secret Labs AB.  All rights reserved.
 * Copyright (c) 1999-2006 by Fredrik Lundh.
 *
 * info@pythonware.com
 * http://www.pythonware.com
 */

/* Licensed to PSF under a Contributor Agreement. */
/* See http://www.python.org/2.4/license for licensing details. */

#include "Python.h"

#define VERSION "1.0.6"

/* -------------------------------------------------------------------- */
/* configuration */

/* Leave defined to include the expat-based XMLParser type */
#define USE_EXPAT

/* Define to to all expat calls via pyexpat's embedded expat library */
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
#if (PY_VERSION_HEX < 0x02050000)
typedef int Py_ssize_t;
#define lenfunc inquiry
#endif

#if (PY_VERSION_HEX < 0x02040000)
#define PyDict_CheckExact PyDict_Check
#if (PY_VERSION_HEX < 0x02020000)
#define PyList_CheckExact PyList_Check
#define PyString_CheckExact PyString_Check
#if (PY_VERSION_HEX >= 0x01060000)
#define Py_USING_UNICODE /* always enabled for 2.0 and 2.1 */
#endif
#endif
#endif

#if !defined(Py_RETURN_NONE)
#define Py_RETURN_NONE return Py_INCREF(Py_None), Py_None
#endif

/* macros used to store 'join' flags in string object pointers.  note
   that all use of text and tail as object pointers must be wrapped in
   JOIN_OBJ.  see comments in the ElementObject definition for more
   info. */
#define JOIN_GET(p) ((Py_uintptr_t) (p) & 1)
#define JOIN_SET(p, flag) ((void*) ((Py_uintptr_t) (JOIN_OBJ(p)) | (flag)))
#define JOIN_OBJ(p) ((PyObject*) ((Py_uintptr_t) (p) & ~1))

/* glue functions (see the init function for details) */
static PyObject* elementtree_copyelement_obj;
static PyObject* elementtree_deepcopy_obj;
static PyObject* elementtree_getiterator_obj;
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

#if (PY_VERSION_HEX < 0x02020000)
LOCAL(int)
PyDict_Update(PyObject* dict, PyObject* other)
{
    /* PyDict_Update emulation for 2.1 and earlier */

    PyObject* res;
    
    res = PyObject_CallMethod(dict, "update", "O", other);
    if (!res)
        return -1;

    Py_DECREF(res);
    return 0;
}
#endif

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

#define Element_CheckExact(op) (Py_Type(op) == &Element_Type)

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
element_resize(ElementObject* self, int extra)
{
    int size;
    PyObject* *children;

    /* make sure self->children can hold the given number of extra
       elements.  set an exception and return -1 if allocation failed */

    if (!self->extra)
        element_new_extra(self, NULL);

    size = self->extra->length + extra;

    if (size > self->extra->allocated) {
        /* use Python 2.4's list growth strategy */
        size = (size >> 3) + (size < 9 ? 3 : 6) + size;
        if (self->extra->children != self->extra->_children) {
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

#define PATHCHAR(ch) (ch == '/' || ch == '*' || ch == '[' || ch == '@')

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
element_find(ElementObject* self, PyObject* args)
{
    int i;

    PyObject* tag;
    if (!PyArg_ParseTuple(args, "O:find", &tag))
        return NULL;

    if (checkpath(tag))
        return PyObject_CallMethod(
            elementpath_obj, "find", "OO", self, tag
            );

    if (!self->extra)
        Py_RETURN_NONE;
        
    for (i = 0; i < self->extra->length; i++) {
        PyObject* item = self->extra->children[i];
        if (Element_CheckExact(item) &&
            PyObject_Compare(((ElementObject*)item)->tag, tag) == 0) {
            Py_INCREF(item);
            return item;
        }
    }

    Py_RETURN_NONE;
}

static PyObject*
element_findtext(ElementObject* self, PyObject* args)
{
    int i;

    PyObject* tag;
    PyObject* default_value = Py_None;
    if (!PyArg_ParseTuple(args, "O|O:findtext", &tag, &default_value))
        return NULL;

    if (checkpath(tag))
        return PyObject_CallMethod(
            elementpath_obj, "findtext", "OOO", self, tag, default_value
            );

    if (!self->extra) {
        Py_INCREF(default_value);
        return default_value;
    }

    for (i = 0; i < self->extra->length; i++) {
        ElementObject* item = (ElementObject*) self->extra->children[i];
        if (Element_CheckExact(item) && !PyObject_Compare(item->tag, tag)) {
            PyObject* text = element_get_text(item);
            if (text == Py_None)
                return PyString_FromString("");
            Py_XINCREF(text);
            return text;
        }
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
    if (!PyArg_ParseTuple(args, "O:findall", &tag))
        return NULL;

    if (checkpath(tag))
        return PyObject_CallMethod(
            elementpath_obj, "findall", "OO", self, tag
            );

    out = PyList_New(0);
    if (!out)
        return NULL;

    if (!self->extra)
        return out;

    for (i = 0; i < self->extra->length; i++) {
        PyObject* item = self->extra->children[i];
        if (Element_CheckExact(item) &&
            PyObject_Compare(((ElementObject*)item)->tag, tag) == 0) {
            if (PyList_Append(out, item) < 0) {
                Py_DECREF(out);
                return NULL;
            }
        }
    }

    return out;
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
element_getiterator(ElementObject* self, PyObject* args)
{
    PyObject* result;
    
    PyObject* tag = Py_None;
    if (!PyArg_ParseTuple(args, "|O:getiterator", &tag))
        return NULL;

    if (!elementtree_getiterator_obj) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "getiterator helper not found"
            );
        return NULL;
    }

    args = PyTuple_New(2);
    if (!args)
        return NULL;

    Py_INCREF(self); PyTuple_SET_ITEM(args, 0, (PyObject*) self);
    Py_INCREF(tag);  PyTuple_SET_ITEM(args, 1, (PyObject*) tag);

    result = PyObject_CallObject(elementtree_getiterator_obj, args);

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
element_getslice(PyObject* self_, Py_ssize_t start, Py_ssize_t end)
{
    ElementObject* self = (ElementObject*) self_;
    Py_ssize_t i;
    PyObject* list;

    if (!self->extra)
        return PyList_New(0);

    /* standard clamping */
    if (start < 0)
        start = 0;
    if (end < 0)
        end = 0;
    if (end > self->extra->length)
        end = self->extra->length;
    if (start > end)
        start = end;

    list = PyList_New(end - start);
    if (!list)
        return NULL;

    for (i = start; i < end; i++) {
        PyObject* item = self->extra->children[i];
        Py_INCREF(item);
        PyList_SET_ITEM(list, i - start, item);
    }

    return list;
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

    if (index < 0)
        index = 0;
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
        if (PyObject_Compare(self->extra->children[i], element) == 0)
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
    PyObject* repr;
    char buffer[100];
    
    repr = PyString_FromString("<Element ");

    PyString_ConcatAndDel(&repr, PyObject_Repr(self->tag));

    sprintf(buffer, " at %p>", self);
    PyString_ConcatAndDel(&repr, PyString_FromString(buffer));

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
element_setslice(PyObject* self_, Py_ssize_t start, Py_ssize_t end, PyObject* item)
{
    ElementObject* self = (ElementObject*) self_;
    Py_ssize_t i, new, old;
    PyObject* recycle = NULL;

    if (!self->extra)
        element_new_extra(self, NULL);

    /* standard clamping */
    if (start < 0)
        start = 0;
    if (end < 0)
        end = 0;
    if (end > self->extra->length)
        end = self->extra->length;
    if (start > end)
        start = end;

    old = end - start;

    if (item == NULL)
        new = 0;
    else if (PyList_CheckExact(item)) {
        new = PyList_GET_SIZE(item);
    } else {
        /* FIXME: support arbitrary sequences? */
        PyErr_Format(
            PyExc_TypeError,
            "expected list, not \"%.200s\"", Py_Type(item)->tp_name
            );
        return -1;
    }

    if (old > 0) {
        /* to avoid recursive calls to this method (via decref), move
           old items to the recycle bin here, and get rid of them when
           we're done modifying the element */
        recycle = PyList_New(old);
        for (i = 0; i < old; i++)
            PyList_SET_ITEM(recycle, i, self->extra->children[i + start]);
    }

    if (new < old) {
        /* delete slice */
        for (i = end; i < self->extra->length; i++)
            self->extra->children[i + new - old] = self->extra->children[i];
    } else if (new > old) {
        /* insert slice */
        if (element_resize(self, new - old) < 0)
            return -1;
        for (i = self->extra->length-1; i >= end; i--)
            self->extra->children[i + new - old] = self->extra->children[i];
    }

    /* replace the slice */
    for (i = 0; i < new; i++) {
        PyObject* element = PyList_GET_ITEM(item, i);
        Py_INCREF(element);
        self->extra->children[i + start] = element;
    }

    self->extra->length += new - old;

    /* discard the recycle bin, and everything in it */
    Py_XDECREF(recycle);

    return 0;
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

static PyMethodDef element_methods[] = {

    {"clear", (PyCFunction) element_clear, METH_VARARGS},

    {"get", (PyCFunction) element_get, METH_VARARGS},
    {"set", (PyCFunction) element_set, METH_VARARGS},

    {"find", (PyCFunction) element_find, METH_VARARGS},
    {"findtext", (PyCFunction) element_findtext, METH_VARARGS},
    {"findall", (PyCFunction) element_findall, METH_VARARGS},

    {"append", (PyCFunction) element_append, METH_VARARGS},
    {"insert", (PyCFunction) element_insert, METH_VARARGS},
    {"remove", (PyCFunction) element_remove, METH_VARARGS},

    {"getiterator", (PyCFunction) element_getiterator, METH_VARARGS},
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

    res = Py_FindMethod(element_methods, (PyObject*) self, name);
    if (res)
	return res;

    PyErr_Clear();

    if (strcmp(name, "tag") == 0)
	res = self->tag;
    else if (strcmp(name, "text") == 0)
        res = element_get_text(self);
    else if (strcmp(name, "tail") == 0) {
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
    element_getslice,
    element_setitem,
    element_setslice,
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

#define TreeBuilder_CheckExact(op) (Py_Type(op) == &TreeBuilder_Type)

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
                PyExc_SyntaxError,
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
        if (PyString_CheckExact(self->data) && Py_Refcnt(self->data) == 1 &&
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
                             const char* prefix, const char *uri)
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
        /* FIXME: prefix and uri use utf-8 encoding! */
        parcel = Py_BuildValue("ss", (prefix) ? prefix : "", uri);
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
    } else {
        PyErr_Format(
            PyExc_SyntaxError, "undefined entity &%s;: line %ld, column %ld",
            PyString_AS_STRING(key),
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
    else if (self->handle_start)
        res = PyObject_CallFunction(self->handle_start, "OO", tag, attrib);
    else
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
    treebuilder_handle_namespace(
        (TreeBuilderObject*) self->target, 1, prefix, uri
        );
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
        PyErr_Format(
            PyExc_SyntaxError, "%s: line %ld, column %ld",
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

    if (res && TreeBuilder_CheckExact(self->target)) {
        Py_DECREF(res);
        return treebuilder_done((TreeBuilderObject*) self->target);
    }

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

        res = expat_parse(
            self, PyString_AS_STRING(buffer), PyString_GET_SIZE(buffer), 0
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
    Py_XDECREF(target->start_event_obj); target->start_event_obj = NULL;
    Py_XDECREF(target->end_event_obj); target->end_event_obj = NULL;
    Py_XDECREF(target->start_ns_event_obj); target->start_ns_event_obj = NULL;
    Py_XDECREF(target->end_ns_event_obj); target->end_ns_event_obj = NULL;

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
#if defined(USE_PYEXPAT_CAPI)
    struct PyExpat_CAPI* capi;
#endif

    /* Patch object type */
    Py_Type(&Element_Type) = Py_Type(&TreeBuilder_Type) = &PyType_Type;
#if defined(USE_EXPAT)
    Py_Type(&XMLParser_Type) = &PyType_Type;
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

#if (PY_VERSION_HEX >= 0x02020000 && PY_VERSION_HEX < 0x02030000)
        "from __future__ import generators\n" /* enable yield under 2.2 */
#endif

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

        "def Comment(text=None):\n" /* public */
        "  element = cElementTree.Element(ET.Comment)\n"
        "  element.text = text\n"
        "  return element\n"
        "cElementTree.Comment = Comment\n"

        "class ElementTree(ET.ElementTree):\n" /* public */
        "  def parse(self, source, parser=None):\n"
        "    if not hasattr(source, 'read'):\n"
        "      source = open(source, 'rb')\n"
        "    if parser is not None:\n"
        "      while 1:\n"
        "        data = source.read(65536)\n"
        "        if not data:\n"
        "          break\n"
        "        parser.feed(data)\n"
        "      self._root = parser.close()\n"
        "    else:\n" 
        "      parser = cElementTree.XMLParser()\n"
        "      self._root = parser._parse(source)\n"
        "    return self._root\n"
        "cElementTree.ElementTree = ElementTree\n"

        "def getiterator(node, tag=None):\n" /* helper */
        "  if tag == '*':\n"
        "    tag = None\n"
#if (PY_VERSION_HEX < 0x02020000)
        "  nodes = []\n" /* 2.1 doesn't have yield */
        "  if tag is None or node.tag == tag:\n"
        "    nodes.append(node)\n"
        "  for node in node:\n"
        "    nodes.extend(getiterator(node, tag))\n"
        "  return nodes\n"
#else
        "  if tag is None or node.tag == tag:\n"
        "    yield node\n"
        "  for node in node:\n"
        "    for node in getiterator(node, tag):\n"
        "      yield node\n"
#endif

        "def parse(source, parser=None):\n" /* public */
        "  tree = ElementTree()\n"
        "  tree.parse(source, parser)\n"
        "  return tree\n"
        "cElementTree.parse = parse\n"

#if (PY_VERSION_HEX < 0x02020000)
        "if hasattr(ET, 'iterparse'):\n"
        "    cElementTree.iterparse = ET.iterparse\n" /* delegate on 2.1 */
#else
        "class iterparse(object):\n"
        " root = None\n"
        " def __init__(self, file, events=None):\n"
        "  if not hasattr(file, 'read'):\n"
        "    file = open(file, 'rb')\n"
        "  self._file = file\n"
        "  self._events = events\n"
        " def __iter__(self):\n" 
        "  events = []\n"
        "  b = cElementTree.TreeBuilder()\n"
        "  p = cElementTree.XMLParser(b)\n"
        "  p._setevents(events, self._events)\n"
        "  while 1:\n"
        "    data = self._file.read(16384)\n"
        "    if not data:\n"
        "      break\n"
        "    p.feed(data)\n"
        "    for event in events:\n"
        "      yield event\n"
        "    del events[:]\n"
        "  root = p.close()\n"
        "  for event in events:\n"
        "    yield event\n"
        "  self.root = root\n"
        "cElementTree.iterparse = iterparse\n"
#endif

        "def PI(target, text=None):\n" /* public */
        "  element = cElementTree.Element(ET.ProcessingInstruction)\n"
        "  element.text = target\n"
        "  if text:\n"
        "    element.text = element.text + ' ' + text\n"
        "  return element\n"

        "  elem = cElementTree.Element(ET.PI)\n"
        "  elem.text = text\n"
        "  return elem\n"
        "cElementTree.PI = cElementTree.ProcessingInstruction = PI\n"

        "def XML(text):\n" /* public */
        "  parser = cElementTree.XMLParser()\n"
        "  parser.feed(text)\n"
        "  return parser.close()\n"
        "cElementTree.XML = cElementTree.fromstring = XML\n"

        "def XMLID(text):\n" /* public */
        "  tree = XML(text)\n"
        "  ids = {}\n"
        "  for elem in tree.getiterator():\n"
        "    id = elem.get('id')\n"
        "    if id:\n"
        "      ids[id] = elem\n"
        "  return tree, ids\n"
        "cElementTree.XMLID = XMLID\n"

        "cElementTree.dump = ET.dump\n"
        "cElementTree.ElementPath = ElementPath = ET.ElementPath\n"
        "cElementTree.iselement = ET.iselement\n"
        "cElementTree.QName = ET.QName\n"
        "cElementTree.tostring = ET.tostring\n"
        "cElementTree.VERSION = '" VERSION "'\n"
        "cElementTree.__version__ = '" VERSION "'\n"
        "cElementTree.XMLParserError = SyntaxError\n"

       );

    PyRun_String(bootstrap, Py_file_input, g, NULL);

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
    elementtree_getiterator_obj = PyDict_GetItemString(g, "getiterator");

#if defined(USE_PYEXPAT_CAPI)
    /* link against pyexpat, if possible */
    capi = PyCObject_Import("pyexpat", "expat_CAPI");
    if (capi &&
        strcmp(capi->magic, PyExpat_CAPI_MAGIC) == 0 &&
        capi->size <= sizeof(*expat_capi) &&
        capi->MAJOR_VERSION == XML_MAJOR_VERSION &&
        capi->MINOR_VERSION == XML_MINOR_VERSION &&
        capi->MICRO_VERSION == XML_MICRO_VERSION)
        expat_capi = capi;
    else
        expat_capi = NULL;
#endif

}
