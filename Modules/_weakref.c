#include "Python.h"
#include "structmember.h"


typedef struct _PyWeakReference PyWeakReference;

struct _PyWeakReference {
    PyObject_HEAD
    PyObject *wr_object;
    PyObject *wr_callback;
    long hash;
    PyWeakReference *wr_prev;
    PyWeakReference *wr_next;
};


#define GET_WEAKREFS_LISTPTR(o) \
        ((PyWeakReference **) PyObject_GET_WEAKREFS_LISTPTR(o))

static PyObject *
ReferenceError;

static PyWeakReference *
free_list = NULL;

staticforward PyTypeObject
PyWeakReference_Type;

static PyWeakReference *
new_weakref(void)
{
    PyWeakReference *result;

    if (free_list != NULL) {
        result = free_list;
        free_list = result->wr_next;
        result->ob_type = &PyWeakReference_Type;
        _Py_NewReference((PyObject *)result);
    }
    else {
        result = PyObject_NEW(PyWeakReference, &PyWeakReference_Type);
    }
    if (result)
        result->hash = -1;
    return result;
}


/* This function clears the passed-in reference and removes it from the
 * list of weak references for the referent.  This is the only code that
 * removes an item from the doubly-linked list of weak references for an
 * object; it is also responsible for clearing the callback slot.
 */
static void
clear_weakref(PyWeakReference *self)
{
    PyObject *callback = self->wr_callback;

    if (self->wr_object != Py_None) {
        PyWeakReference **list = GET_WEAKREFS_LISTPTR(self->wr_object);

        if (*list == self)
            *list = self->wr_next;
        self->wr_object = Py_None;
        self->wr_callback = NULL;
        if (self->wr_prev != NULL)
            self->wr_prev->wr_next = self->wr_next;
        if (self->wr_next != NULL)
            self->wr_next->wr_prev = self->wr_prev;
        self->wr_prev = NULL;
        self->wr_next = NULL;
        Py_XDECREF(callback);
    }
}


static void
weakref_dealloc(PyWeakReference *self)
{
    clear_weakref(self);
    PyObject_GC_Fini((PyObject *)self);
    self->wr_next = free_list;
    free_list = self;
}


static int
gc_traverse(PyWeakReference *self, visitproc visit, void *arg)
{
    if (self->wr_callback != NULL)
        return visit(self->wr_callback, arg);
    return 0;
}


static int
gc_clear(PyWeakReference *self)
{
    clear_weakref(self);
    return 0;
}


static PyObject *
weakref_call(PyWeakReference *self, PyObject *args, PyObject *kw)
{
    static char *argnames[] = {NULL};

    if (PyArg_ParseTupleAndKeywords(args, kw, ":__call__", argnames)) {
        PyObject *object = self->wr_object;
        Py_INCREF(object);
        return (object);
    }
    return NULL;
}


static long
weakref_hash(PyWeakReference *self)
{
    if (self->hash != -1)
        return self->hash;
    if (self->wr_object == Py_None) {
        PyErr_SetString(PyExc_TypeError, "weak object has gone away");
        return -1;
    }
    self->hash = PyObject_Hash(self->wr_object);
    return self->hash;
}
    

static PyObject *
weakref_repr(PyWeakReference *self)
{
    char buffer[256];
    if (self->wr_object == Py_None) {
        sprintf(buffer, "<weakref at %lx; dead>",
                (long)(self));
    }
    else {
        sprintf(buffer, "<weakref at %#lx; to '%s' at %#lx>",
                (long)(self), self->wr_object->ob_type->tp_name,
                (long)(self->wr_object));
    }
    return PyString_FromString(buffer);
}

/* Weak references only support equality, not ordering. Two weak references
   are equal if the underlying objects are equal. If the underlying object has
   gone away, they are equal if they are identical. */

static PyObject *
weakref_richcompare(PyWeakReference* self, PyWeakReference* other, int op)
{
    if (op != Py_EQ || self->ob_type != other->ob_type) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
    if (self->wr_object == Py_None || other->wr_object == Py_None) {
        PyObject *res = self==other ? Py_True : Py_False;
        Py_INCREF(res);
        return res;
    }
    return PyObject_RichCompare(self->wr_object, other->wr_object, op);
}


statichere PyTypeObject
PyWeakReference_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "weakref",
    sizeof(PyWeakReference) + PyGC_HEAD_SIZE,
    0,
    (destructor)weakref_dealloc,/*tp_dealloc*/
    0,	                        /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,	                        /*tp_compare*/
    (reprfunc)weakref_repr,     /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    (hashfunc)weakref_hash,      /*tp_hash*/
    (ternaryfunc)weakref_call,  /*tp_call*/
    0,                          /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_GC | Py_TPFLAGS_HAVE_RICHCOMPARE,
    0,                          /*tp_doc*/
    (traverseproc)gc_traverse,  /*tp_traverse*/
    (inquiry)gc_clear,          /*tp_clear*/
    (richcmpfunc)weakref_richcompare,	/*tp_richcompare*/
    0,				/*tp_weaklistoffset*/
};


static int
proxy_checkref(PyWeakReference *proxy)
{
    if (proxy->wr_object == Py_None) {
        PyErr_SetString(ReferenceError,
                        "weakly-referenced object no longer exists");
        return 0;
    }
    return 1;
}


#define WRAP_UNARY(method, generic) \
    static PyObject * \
    method(PyWeakReference *proxy) { \
        if (!proxy_checkref(proxy)) { \
            return NULL; \
        } \
        return generic(proxy->wr_object); \
    }

#define WRAP_BINARY(method, generic) \
    static PyObject * \
    method(PyWeakReference *proxy, PyObject *v) { \
        if (!proxy_checkref(proxy)) { \
            return NULL; \
        } \
        return generic(proxy->wr_object, v); \
    }

#define WRAP_TERNARY(method, generic) \
    static PyObject * \
    method(PyWeakReference *proxy, PyObject *v, PyObject *w) { \
        if (!proxy_checkref(proxy)) { \
            return NULL; \
	} \
        return generic(proxy->wr_object, v, w); \
    }


/* direct slots */

WRAP_BINARY(proxy_getattr, PyObject_GetAttr)
WRAP_UNARY(proxy_str, PyObject_Str)
WRAP_TERNARY(proxy_call, PyEval_CallObjectWithKeywords)

static int
proxy_print(PyWeakReference *proxy, FILE *fp, int flags)
{
    if (!proxy_checkref(proxy))
        return -1;
    return PyObject_Print(proxy->wr_object, fp, flags);
}

static PyObject *
proxy_repr(PyWeakReference *proxy)
{
    char buf[160];
    sprintf(buf, "<weakref at %p to %.100s at %p>", proxy,
            proxy->wr_object->ob_type->tp_name, proxy->wr_object);
    return PyString_FromString(buf);
}


static int
proxy_setattr(PyWeakReference *proxy, PyObject *name, PyObject *value)
{
    if (!proxy_checkref(proxy))
        return -1;
    return PyObject_SetAttr(proxy->wr_object, name, value);
}

static int
proxy_compare(PyWeakReference *proxy, PyObject *v)
{
    if (!proxy_checkref(proxy))
        return -1;
    return PyObject_Compare(proxy->wr_object, v);
}

/* number slots */
WRAP_BINARY(proxy_add, PyNumber_Add)
WRAP_BINARY(proxy_sub, PyNumber_Subtract)
WRAP_BINARY(proxy_mul, PyNumber_Multiply)
WRAP_BINARY(proxy_div, PyNumber_Divide)
WRAP_BINARY(proxy_mod, PyNumber_Remainder)
WRAP_BINARY(proxy_divmod, PyNumber_Divmod)
WRAP_TERNARY(proxy_pow, PyNumber_Power)
WRAP_UNARY(proxy_neg, PyNumber_Negative)
WRAP_UNARY(proxy_pos, PyNumber_Positive)
WRAP_UNARY(proxy_abs, PyNumber_Absolute)
WRAP_UNARY(proxy_invert, PyNumber_Invert)
WRAP_BINARY(proxy_lshift, PyNumber_Lshift)
WRAP_BINARY(proxy_rshift, PyNumber_Rshift)
WRAP_BINARY(proxy_and, PyNumber_And)
WRAP_BINARY(proxy_xor, PyNumber_Xor)
WRAP_BINARY(proxy_or, PyNumber_Or)
WRAP_UNARY(proxy_int, PyNumber_Int)
WRAP_UNARY(proxy_long, PyNumber_Long)
WRAP_UNARY(proxy_float, PyNumber_Float)
WRAP_BINARY(proxy_iadd, PyNumber_InPlaceAdd)
WRAP_BINARY(proxy_isub, PyNumber_InPlaceSubtract)
WRAP_BINARY(proxy_imul, PyNumber_InPlaceMultiply)
WRAP_BINARY(proxy_idiv, PyNumber_InPlaceDivide)
WRAP_BINARY(proxy_imod, PyNumber_InPlaceRemainder)
WRAP_TERNARY(proxy_ipow, PyNumber_InPlacePower)
WRAP_BINARY(proxy_ilshift, PyNumber_InPlaceLshift)
WRAP_BINARY(proxy_irshift, PyNumber_InPlaceRshift)
WRAP_BINARY(proxy_iand, PyNumber_InPlaceAnd)
WRAP_BINARY(proxy_ixor, PyNumber_InPlaceXor)
WRAP_BINARY(proxy_ior, PyNumber_InPlaceOr)

static int 
proxy_nonzero(PyWeakReference *proxy)
{
    PyObject *o = proxy->wr_object;
    if (!proxy_checkref(proxy))
        return 1;
    if (o->ob_type->tp_as_number &&
        o->ob_type->tp_as_number->nb_nonzero)
        return (*o->ob_type->tp_as_number->nb_nonzero)(o);
    else
        return 1;
}

/* sequence slots */

static PyObject *
proxy_slice(PyWeakReference *proxy, int i, int j)
{
    if (!proxy_checkref(proxy))
        return NULL;
    return PySequence_GetSlice(proxy->wr_object, i, j);
}

static int
proxy_ass_slice(PyWeakReference *proxy, int i, int j, PyObject *value)
{
    if (!proxy_checkref(proxy))
        return -1;
    return PySequence_SetSlice(proxy->wr_object, i, j, value);
}

static int
proxy_contains(PyWeakReference *proxy, PyObject *value)
{
    if (!proxy_checkref(proxy))
        return -1;
    return PySequence_Contains(proxy->wr_object, value);
}


/* mapping slots */

static int
proxy_length(PyWeakReference *proxy)
{
    if (!proxy_checkref(proxy))
        return -1;
    return PyObject_Length(proxy->wr_object);
}

WRAP_BINARY(proxy_getitem, PyObject_GetItem)

static int
proxy_setitem(PyWeakReference *proxy, PyObject *key, PyObject *value)
{
    if (!proxy_checkref(proxy))
        return -1;
    return PyObject_SetItem(proxy->wr_object, key, value);
}


static PyNumberMethods proxy_as_number = {
    (binaryfunc)proxy_add,      /*nb_add*/
    (binaryfunc)proxy_sub,      /*nb_subtract*/
    (binaryfunc)proxy_mul,      /*nb_multiply*/
    (binaryfunc)proxy_div,      /*nb_divide*/
    (binaryfunc)proxy_mod,      /*nb_remainder*/
    (binaryfunc)proxy_divmod,   /*nb_divmod*/
    (ternaryfunc)proxy_pow,     /*nb_power*/
    (unaryfunc)proxy_neg,       /*nb_negative*/
    (unaryfunc)proxy_pos,       /*nb_positive*/
    (unaryfunc)proxy_abs,       /*nb_absolute*/
    (inquiry)proxy_nonzero,     /*nb_nonzero*/
    (unaryfunc)proxy_invert,    /*nb_invert*/
    (binaryfunc)proxy_lshift,   /*nb_lshift*/
    (binaryfunc)proxy_rshift,   /*nb_rshift*/
    (binaryfunc)proxy_and,      /*nb_and*/
    (binaryfunc)proxy_xor,      /*nb_xor*/
    (binaryfunc)proxy_or,       /*nb_or*/
    (coercion)0,                /*nb_coerce*/
    (unaryfunc)proxy_int,       /*nb_int*/
    (unaryfunc)proxy_long,      /*nb_long*/
    (unaryfunc)proxy_float,     /*nb_float*/
    (unaryfunc)0,               /*nb_oct*/
    (unaryfunc)0,               /*nb_hex*/
    (binaryfunc)proxy_iadd,     /*nb_inplace_add*/
    (binaryfunc)proxy_isub,     /*nb_inplace_subtract*/
    (binaryfunc)proxy_imul,     /*nb_inplace_multiply*/
    (binaryfunc)proxy_idiv,     /*nb_inplace_divide*/
    (binaryfunc)proxy_imod,     /*nb_inplace_remainder*/
    (ternaryfunc)proxy_ipow,    /*nb_inplace_power*/
    (binaryfunc)proxy_ilshift,  /*nb_inplace_lshift*/
    (binaryfunc)proxy_irshift,  /*nb_inplace_rshift*/
    (binaryfunc)proxy_iand,     /*nb_inplace_and*/
    (binaryfunc)proxy_ixor,     /*nb_inplace_xor*/
    (binaryfunc)proxy_ior,      /*nb_inplace_or*/
};

static PySequenceMethods proxy_as_sequence = {
    (inquiry)proxy_length,      /*sq_length*/
    0,                          /*sq_concat*/
    0,                          /*sq_repeat*/
    0,                          /*sq_item*/
    (intintargfunc)proxy_slice, /*sq_slice*/
    0,                          /*sq_ass_item*/
    (intintobjargproc)proxy_ass_slice, /*sq_ass_slice*/
    (objobjproc)proxy_contains, /* sq_contains */
};

static PyMappingMethods proxy_as_mapping = {
    (inquiry)proxy_length,      /*mp_length*/
    (binaryfunc)proxy_getitem,  /*mp_subscript*/
    (objobjargproc)proxy_setitem, /*mp_ass_subscript*/
};


static PyTypeObject
PyWeakProxy_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "weakproxy",
    sizeof(PyWeakReference) + PyGC_HEAD_SIZE,
    0,
    /* methods */
    (destructor)weakref_dealloc,/*tp_dealloc*/
    (printfunc)proxy_print,     /*tp_print*/
    0,				/*tp_getattr*/
    0, 				/*tp_setattr*/
    (cmpfunc)proxy_compare,	/*tp_compare*/
    (unaryfunc)proxy_repr,	/*tp_repr*/
    &proxy_as_number,		/*tp_as_number*/
    &proxy_as_sequence,		/*tp_as_sequence*/
    &proxy_as_mapping,		/*tp_as_mapping*/
    0,	                        /*tp_hash*/
    (ternaryfunc)0,	        /*tp_call*/
    (unaryfunc)proxy_str,	/*tp_str*/
    (getattrofunc)proxy_getattr,/*tp_getattro*/
    (setattrofunc)proxy_setattr,/*tp_setattro*/
    0,				/*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_GC
    |Py_TPFLAGS_CHECKTYPES,     /*tp_flags*/
    0,                          /*tp_doc*/
    (traverseproc)gc_traverse,  /*tp_traverse*/
    (inquiry)gc_clear,          /*tp_clear*/
};


static PyTypeObject
PyWeakCallableProxy_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "weakcallableproxy",
    sizeof(PyWeakReference) + PyGC_HEAD_SIZE,
    0,
    /* methods */
    (destructor)weakref_dealloc,/*tp_dealloc*/
    (printfunc)proxy_print,     /*tp_print*/
    0,				/*tp_getattr*/
    0, 				/*tp_setattr*/
    (cmpfunc)proxy_compare,	/*tp_compare*/
    (unaryfunc)proxy_repr,	/*tp_repr*/
    &proxy_as_number,		/*tp_as_number*/
    &proxy_as_sequence,		/*tp_as_sequence*/
    &proxy_as_mapping,		/*tp_as_mapping*/
    0,	                        /*tp_hash*/
    (ternaryfunc)proxy_call,	/*tp_call*/
    (unaryfunc)proxy_str,	/*tp_str*/
    (getattrofunc)proxy_getattr,/*tp_getattro*/
    (setattrofunc)proxy_setattr,/*tp_setattro*/
    0,				/*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_GC
    |Py_TPFLAGS_CHECKTYPES,     /*tp_flags*/
    0,                          /*tp_doc*/
    (traverseproc)gc_traverse,  /*tp_traverse*/
    (inquiry)gc_clear,          /*tp_clear*/
};


static long
getweakrefcount(PyWeakReference *head)
{
    long count = 0;

    while (head != NULL) {
        ++count;
        head = head->wr_next;
    }
    return count;
}


static char weakref_getweakrefcount__doc__[] =
"getweakrefcount(object) -- return the number of weak references\n"
"to 'object'.";

static PyObject *
weakref_getweakrefcount(PyObject *self, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *object;

    if (PyArg_ParseTuple(args, "O:getweakrefcount", &object)) {
        if (PyType_SUPPORTS_WEAKREFS(object->ob_type)) {
            PyWeakReference **list = GET_WEAKREFS_LISTPTR(object);

            result = PyInt_FromLong(getweakrefcount(*list));
        }
        else
            result = PyInt_FromLong(0);
    }
    return result;
}


static char weakref_getweakrefs__doc__[] =
"getweakrefs(object) -- return a list of all weak reference objects\n"
"that point to 'object'.";

static PyObject *
weakref_getweakrefs(PyObject *self, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *object;

    if (PyArg_ParseTuple(args, "O:getweakrefs", &object)) {
        if (PyType_SUPPORTS_WEAKREFS(object->ob_type)) {
            PyWeakReference **list = GET_WEAKREFS_LISTPTR(object);
            long count = getweakrefcount(*list);

            result = PyList_New(count);
            if (result != NULL) {
                PyWeakReference *current = *list;
                long i;
                for (i = 0; i < count; ++i) {
                    PyList_SET_ITEM(result, i, (PyObject *) current);
                    Py_INCREF(current);
                    current = current->wr_next;
                }
            }
        }
        else {
            result = PyList_New(0);
        }
    }
    return result;
}


/* Given the head of an object's list of weak references, extract the
 * two callback-less refs (ref and proxy).  Used to determine if the
 * shared references exist and to determine the back link for newly
 * inserted references.
 */
static void
get_basic_refs(PyWeakReference *head,
               PyWeakReference **refp, PyWeakReference **proxyp)
{
    *refp = NULL;
    *proxyp = NULL;

    if (head != NULL && head->wr_callback == NULL) {
        if (head->ob_type == &PyWeakReference_Type) {
            *refp = head;
            head = head->wr_next;
        }
        if (head != NULL && head->wr_callback == NULL) {
            *proxyp = head;
            head = head->wr_next;
        }
    }
}

/* Insert 'newref' in the list after 'prev'.  Both must be non-NULL. */
static void
insert_after(PyWeakReference *newref, PyWeakReference *prev)
{
    newref->wr_prev = prev;
    newref->wr_next = prev->wr_next;
    if (prev->wr_next != NULL)
        prev->wr_next->wr_prev = newref;
    prev->wr_next = newref;
}

/* Insert 'newref' at the head of the list; 'list' points to the variable
 * that stores the head.
 */
static void
insert_head(PyWeakReference *newref, PyWeakReference **list)
{
    PyWeakReference *next = *list;

    newref->wr_prev = NULL;
    newref->wr_next = next;
    if (next != NULL)
        next->wr_prev = newref;
    *list = newref;
}


static char weakref_ref__doc__[] =
"new(object[, callback]) -- create a weak reference to 'object';\n"
"when 'object' is finalized, 'callback' will be called and passed\n"
"a reference to 'object'.";

static PyObject *
weakref_ref(PyObject *self, PyObject *args)
{
    PyObject *object;
    PyObject *callback = NULL;
    PyWeakReference *result = NULL;

    if (PyArg_ParseTuple(args, "O|O:new", &object, &callback)) {
        PyWeakReference **list;
        PyWeakReference *ref, *proxy;

        if (!PyType_SUPPORTS_WEAKREFS(object->ob_type)) {
            PyErr_Format(PyExc_TypeError,
                         "'%s' objects are not weakly referencable",
                         object->ob_type->tp_name);
            return NULL;
        }
        list = GET_WEAKREFS_LISTPTR(object);
        get_basic_refs(*list, &ref, &proxy);
        if (callback == NULL) {
            /* return existing weak reference if it exists */
            result = ref;
            Py_XINCREF(result);
        }
        if (result == NULL) {
            result = new_weakref();
            if (result != NULL) {
                Py_XINCREF(callback);
                result->wr_callback = callback;
                result->wr_object = object;
                if (callback == NULL) {
                    insert_head(result, list);
                }
                else {
                    PyWeakReference *prev = (proxy == NULL) ? ref : proxy;

                    if (prev == NULL)
                        insert_head(result, list);
                    else
                        insert_after(result, prev);
                }
                PyObject_GC_Init((PyObject *) result);
            }
        }
    }
    return (PyObject *) result;
}


static char weakref_proxy__doc__[] =
"proxy(object[, callback]) -- create a proxy object that weakly\n"
"references 'object'.  'callback', if given, is called with a\n"
"reference to the proxy when it is about to be finalized.";

static PyObject *
weakref_proxy(PyObject *self, PyObject *args)
{
    PyObject *object;
    PyObject *callback = NULL;
    PyWeakReference *result = NULL;

    if (PyArg_ParseTuple(args, "O|O:new", &object, &callback)) {
        PyWeakReference **list;
        PyWeakReference *ref, *proxy;

        if (!PyType_SUPPORTS_WEAKREFS(object->ob_type)) {
            PyErr_Format(PyExc_TypeError,
                         "'%s' objects are not weakly referencable",
                         object->ob_type->tp_name);
            return NULL;
        }
        list = GET_WEAKREFS_LISTPTR(object);
        get_basic_refs(*list, &ref, &proxy);
        if (callback == NULL) {
            /* attempt to return an existing weak reference if it exists */
            result = proxy;
            Py_XINCREF(result);
        }
        if (result == NULL) {
            result = new_weakref();
            if (result != NULL) {
                PyWeakReference *prev;

                if (PyCallable_Check(object))
                    result->ob_type = &PyWeakCallableProxy_Type;
                else
                    result->ob_type = &PyWeakProxy_Type;
                result->wr_object = object;
                Py_XINCREF(callback);
                result->wr_callback = callback;
                if (callback == NULL)
                    prev = ref;
                else
                    prev = (proxy == NULL) ? ref : proxy;

                if (prev == NULL)
                    insert_head(result, list);
                else
                    insert_after(result, prev);
                PyObject_GC_Init((PyObject *) result);
            }
        }
    }
    return (PyObject *) result;
}


/* This is the implementation of the PyObject_ClearWeakRefs() function; it
 * is installed in the init_weakref() function.  It is called by the
 * tp_dealloc handler to clear weak references.
 *
 * This iterates through the weak references for 'object' and calls callbacks
 * for those references which have one.  It returns when all callbacks have
 * been attempted.
 */
static void
cleanup_helper(PyObject *object)
{
    PyWeakReference **list;

    if (object == NULL
        || !PyType_SUPPORTS_WEAKREFS(object->ob_type)
        || object->ob_refcnt != 0) {
        PyErr_BadInternalCall();
        /* not sure what we should return here */
        return;
    }
    list = GET_WEAKREFS_LISTPTR(object);
    while (*list != NULL) {
        PyWeakReference *current = *list;
        PyObject *callback = current->wr_callback;

        Py_XINCREF(callback);
        clear_weakref(current);
        if (callback != NULL) {
            PyObject *cbresult;

            cbresult = PyObject_CallFunction(callback, "O", current);
            if (cbresult == NULL)
                PyErr_WriteUnraisable(callback);
            else
                Py_DECREF(cbresult);
            Py_DECREF(callback);
        }
    }
    return;
}


static PyMethodDef
weakref_functions[] =  {
    {"getweakrefcount", weakref_getweakrefcount,        METH_VARARGS,
     weakref_getweakrefcount__doc__},
    {"getweakrefs",     weakref_getweakrefs,            METH_VARARGS,
     weakref_getweakrefs__doc__},
    {"proxy",           weakref_proxy,                  METH_VARARGS,
     weakref_proxy__doc__},
    {"ref",             weakref_ref,                    METH_VARARGS,
     weakref_ref__doc__},
    {NULL, NULL, 0, NULL}
};


DL_EXPORT(void)
init_weakref(void)
{
    PyObject *m;

    PyWeakReference_Type.ob_type = &PyType_Type;
    PyWeakProxy_Type.ob_type = &PyType_Type;
    PyWeakCallableProxy_Type.ob_type = &PyType_Type;
    m = Py_InitModule3("_weakref", weakref_functions,
                       "Weak-reference support module.");
    if (m != NULL) {
        PyObject_ClearWeakRefs = cleanup_helper;
        Py_INCREF(&PyWeakReference_Type);
        PyModule_AddObject(m, "ReferenceType",
                           (PyObject *) &PyWeakReference_Type);
        Py_INCREF(&PyWeakProxy_Type);
        PyModule_AddObject(m, "ProxyType",
                           (PyObject *) &PyWeakProxy_Type);
        Py_INCREF(&PyWeakCallableProxy_Type);
        PyModule_AddObject(m, "CallableProxyType",
                           (PyObject *) &PyWeakCallableProxy_Type);
        ReferenceError = PyErr_NewException("weakref.ReferenceError",
                                            PyExc_RuntimeError, NULL);
        if (ReferenceError != NULL)
            PyModule_AddObject(m, "ReferenceError", ReferenceError);
    }
}
