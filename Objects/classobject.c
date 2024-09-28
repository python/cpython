/* Class object implementation (dead now except for methods) */

#include "Python.h"
#include "pycore_call.h"          // _PyObject_VectorcallTstate()
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_object.h"
#include "pycore_pyerrors.h"
#include "pycore_pystate.h"       // _PyThreadState_GET()


#include "clinic/classobject.c.h"

#define TP_DESCR_GET(t) ((t)->tp_descr_get)

/*[clinic input]
class method "PyMethodObject *" "&PyMethod_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=b16e47edf6107c23]*/


PyObject *
PyMethod_Function(PyObject *im)
{
    if (!PyMethod_Check(im)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyMethodObject *)im)->im_func;
}

PyObject *
PyMethod_Self(PyObject *im)
{
    if (!PyMethod_Check(im)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyMethodObject *)im)->im_self;
}


static PyObject *
method_vectorcall(PyObject *method, PyObject *const *args,
                  size_t nargsf, PyObject *kwnames)
{
    assert(Py_IS_TYPE(method, &PyMethod_Type));

    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *self = PyMethod_GET_SELF(method);
    PyObject *func = PyMethod_GET_FUNCTION(method);
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(nargs == 0 || args[nargs-1]);

    PyObject *result;
    if (nargsf & PY_VECTORCALL_ARGUMENTS_OFFSET) {
        /* PY_VECTORCALL_ARGUMENTS_OFFSET is set, so we are allowed to mutate the vector */
        PyObject **newargs = (PyObject**)args - 1;
        nargs += 1;
        PyObject *tmp = newargs[0];
        newargs[0] = self;
        assert(newargs[nargs-1]);
        result = _PyObject_VectorcallTstate(tstate, func, newargs,
                                            nargs, kwnames);
        newargs[0] = tmp;
    }
    else {
        Py_ssize_t nkwargs = (kwnames == NULL) ? 0 : PyTuple_GET_SIZE(kwnames);
        Py_ssize_t totalargs = nargs + nkwargs;
        if (totalargs == 0) {
            return _PyObject_VectorcallTstate(tstate, func, &self, 1, NULL);
        }

        PyObject *newargs_stack[_PY_FASTCALL_SMALL_STACK];
        PyObject **newargs;
        if (totalargs <= (Py_ssize_t)Py_ARRAY_LENGTH(newargs_stack) - 1) {
            newargs = newargs_stack;
        }
        else {
            newargs = PyMem_Malloc((totalargs+1) * sizeof(PyObject *));
            if (newargs == NULL) {
                _PyErr_NoMemory(tstate);
                return NULL;
            }
        }
        /* use borrowed references */
        newargs[0] = self;
        /* bpo-37138: since totalargs > 0, it's impossible that args is NULL.
         * We need this, since calling memcpy() with a NULL pointer is
         * undefined behaviour. */
        assert(args != NULL);
        memcpy(newargs + 1, args, totalargs * sizeof(PyObject *));
        result = _PyObject_VectorcallTstate(tstate, func,
                                            newargs, nargs+1, kwnames);
        if (newargs != newargs_stack) {
            PyMem_Free(newargs);
        }
    }
    return result;
}


/* Method objects are used for bound instance methods returned by
   instancename.methodname. ClassName.methodname returns an ordinary
   function.
*/

PyObject *
PyMethod_New(PyObject *func, PyObject *self)
{
    if (self == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }
    PyMethodObject *im = PyObject_GC_New(PyMethodObject, &PyMethod_Type);
    if (im == NULL) {
        return NULL;
    }
    im->im_weakreflist = NULL;
    im->im_func = Py_NewRef(func);
    im->im_self = Py_NewRef(self);
    im->vectorcall = method_vectorcall;
    _PyObject_GC_TRACK(im);
    return (PyObject *)im;
}

/*[clinic input]
method.__reduce__
[clinic start generated code]*/

static PyObject *
method___reduce___impl(PyMethodObject *self)
/*[clinic end generated code: output=6c04506d0fa6fdcb input=143a0bf5e96de6e8]*/
{
    PyObject *funcself = PyMethod_GET_SELF(self);
    PyObject *func = PyMethod_GET_FUNCTION(self);
    PyObject *funcname = PyObject_GetAttr(func, &_Py_ID(__name__));
    if (funcname == NULL) {
        return NULL;
    }
    return Py_BuildValue(
            "N(ON)", _PyEval_GetBuiltin(&_Py_ID(getattr)), funcself, funcname);
}

static PyMethodDef method_methods[] = {
    METHOD___REDUCE___METHODDEF
    {NULL, NULL}
};

/* Descriptors for PyMethod attributes */

/* im_func and im_self are stored in the PyMethod object */

#define MO_OFF(x) offsetof(PyMethodObject, x)

static PyMemberDef method_memberlist[] = {
    {"__func__", _Py_T_OBJECT, MO_OFF(im_func), Py_READONLY,
     "the function (or other callable) implementing a method"},
    {"__self__", _Py_T_OBJECT, MO_OFF(im_self), Py_READONLY,
     "the instance to which a method is bound"},
    {NULL}      /* Sentinel */
};

/* Christian Tismer argued convincingly that method attributes should
   (nearly) always override function attributes.
   The one exception is __doc__; there's a default __doc__ which
   should only be used for the class, not for instances */

static PyObject *
method_get_doc(PyMethodObject *im, void *context)
{
    return PyObject_GetAttr(im->im_func, &_Py_ID(__doc__));
}

static PyGetSetDef method_getset[] = {
    {"__doc__", (getter)method_get_doc, NULL, NULL},
    {0}
};

static PyObject *
method_getattro(PyObject *obj, PyObject *name)
{
    PyMethodObject *im = (PyMethodObject *)obj;
    PyTypeObject *tp = Py_TYPE(obj);
    PyObject *descr = NULL;

    {
        if (!_PyType_IsReady(tp)) {
            if (PyType_Ready(tp) < 0)
                return NULL;
        }
        descr = _PyType_LookupRef(tp, name);
    }

    if (descr != NULL) {
        descrgetfunc f = TP_DESCR_GET(Py_TYPE(descr));
        if (f != NULL) {
            PyObject *res = f(descr, obj, (PyObject *)Py_TYPE(obj));
            Py_DECREF(descr);
            return res;
        }
        else {
            return descr;
        }
    }

    return PyObject_GetAttr(im->im_func, name);
}

/*[clinic input]
@classmethod
method.__new__ as method_new
    function: object
    instance: object
    /

Create a bound instance method object.
[clinic start generated code]*/

static PyObject *
method_new_impl(PyTypeObject *type, PyObject *function, PyObject *instance)
/*[clinic end generated code: output=d33ef4ebf702e1f7 input=4e32facc3c3108ae]*/
{
    if (!PyCallable_Check(function)) {
        PyErr_SetString(PyExc_TypeError,
                        "first argument must be callable");
        return NULL;
    }
    if (instance == NULL || instance == Py_None) {
        PyErr_SetString(PyExc_TypeError,
            "instance must not be None");
        return NULL;
    }

    return PyMethod_New(function, instance);
}

static void
method_dealloc(PyMethodObject *im)
{
    _PyObject_GC_UNTRACK(im);
    if (im->im_weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *)im);
    Py_DECREF(im->im_func);
    Py_XDECREF(im->im_self);
    PyObject_GC_Del(im);
}

static PyObject *
method_richcompare(PyObject *self, PyObject *other, int op)
{
    PyMethodObject *a, *b;
    PyObject *res;
    int eq;

    if ((op != Py_EQ && op != Py_NE) ||
        !PyMethod_Check(self) ||
        !PyMethod_Check(other))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }
    a = (PyMethodObject *)self;
    b = (PyMethodObject *)other;
    eq = PyObject_RichCompareBool(a->im_func, b->im_func, Py_EQ);
    if (eq == 1) {
        eq = (a->im_self == b->im_self);
    }
    else if (eq < 0)
        return NULL;
    if (op == Py_EQ)
        res = eq ? Py_True : Py_False;
    else
        res = eq ? Py_False : Py_True;
    return Py_NewRef(res);
}

static PyObject *
method_repr(PyMethodObject *a)
{
    PyObject *self = a->im_self;
    PyObject *func = a->im_func;
    PyObject *funcname, *result;
    const char *defname = "?";

    if (PyObject_GetOptionalAttr(func, &_Py_ID(__qualname__), &funcname) < 0 ||
        (funcname == NULL &&
         PyObject_GetOptionalAttr(func, &_Py_ID(__name__), &funcname) < 0))
    {
        return NULL;
    }

    if (funcname != NULL && !PyUnicode_Check(funcname)) {
        Py_SETREF(funcname, NULL);
    }

    /* XXX Shouldn't use repr()/%R here! */
    result = PyUnicode_FromFormat("<bound method %V of %R>",
                                  funcname, defname, self);

    Py_XDECREF(funcname);
    return result;
}

static Py_hash_t
method_hash(PyMethodObject *a)
{
    Py_hash_t x, y;
    x = PyObject_GenericHash(a->im_self);
    y = PyObject_Hash(a->im_func);
    if (y == -1)
        return -1;
    x = x ^ y;
    if (x == -1)
        x = -2;
    return x;
}

static int
method_traverse(PyMethodObject *im, visitproc visit, void *arg)
{
    Py_VISIT(im->im_func);
    Py_VISIT(im->im_self);
    return 0;
}

static PyObject *
method_descr_get(PyObject *meth, PyObject *obj, PyObject *cls)
{
    Py_INCREF(meth);
    return meth;
}

PyTypeObject PyMethod_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "method",
    .tp_basicsize = sizeof(PyMethodObject),
    .tp_dealloc = (destructor)method_dealloc,
    .tp_vectorcall_offset = offsetof(PyMethodObject, vectorcall),
    .tp_repr = (reprfunc)method_repr,
    .tp_hash = (hashfunc)method_hash,
    .tp_call = PyVectorcall_Call,
    .tp_getattro = method_getattro,
    .tp_setattro = PyObject_GenericSetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
                Py_TPFLAGS_HAVE_VECTORCALL,
    .tp_doc = method_new__doc__,
    .tp_traverse = (traverseproc)method_traverse,
    .tp_richcompare = method_richcompare,
    .tp_weaklistoffset = offsetof(PyMethodObject, im_weakreflist),
    .tp_methods = method_methods,
    .tp_members = method_memberlist,
    .tp_getset = method_getset,
    .tp_descr_get = method_descr_get,
    .tp_new = method_new,
};

/* ------------------------------------------------------------------------
 * instance method
 */

/*[clinic input]
class instancemethod "PyInstanceMethodObject *" "&PyInstanceMethod_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=28c9762a9016f4d2]*/

PyObject *
PyInstanceMethod_New(PyObject *func) {
    PyInstanceMethodObject *method;
    method = PyObject_GC_New(PyInstanceMethodObject,
                             &PyInstanceMethod_Type);
    if (method == NULL) return NULL;
    method->func = Py_NewRef(func);
    _PyObject_GC_TRACK(method);
    return (PyObject *)method;
}

PyObject *
PyInstanceMethod_Function(PyObject *im)
{
    if (!PyInstanceMethod_Check(im)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return PyInstanceMethod_GET_FUNCTION(im);
}

#define IMO_OFF(x) offsetof(PyInstanceMethodObject, x)

static PyMemberDef instancemethod_memberlist[] = {
    {"__func__", _Py_T_OBJECT, IMO_OFF(func), Py_READONLY,
     "the function (or other callable) implementing a method"},
    {NULL}      /* Sentinel */
};

static PyObject *
instancemethod_get_doc(PyObject *self, void *context)
{
    return PyObject_GetAttr(PyInstanceMethod_GET_FUNCTION(self),
                            &_Py_ID(__doc__));
}

static PyGetSetDef instancemethod_getset[] = {
    {"__doc__", (getter)instancemethod_get_doc, NULL, NULL},
    {0}
};

static PyObject *
instancemethod_getattro(PyObject *self, PyObject *name)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *descr = NULL;

    if (!_PyType_IsReady(tp)) {
        if (PyType_Ready(tp) < 0)
            return NULL;
    }
    descr = _PyType_LookupRef(tp, name);

    if (descr != NULL) {
        descrgetfunc f = TP_DESCR_GET(Py_TYPE(descr));
        if (f != NULL) {
            PyObject *res = f(descr, self, (PyObject *)Py_TYPE(self));
            Py_DECREF(descr);
            return res;
        }
        else {
            return descr;
        }
    }

    return PyObject_GetAttr(PyInstanceMethod_GET_FUNCTION(self), name);
}

static void
instancemethod_dealloc(PyObject *self) {
    _PyObject_GC_UNTRACK(self);
    Py_DECREF(PyInstanceMethod_GET_FUNCTION(self));
    PyObject_GC_Del(self);
}

static int
instancemethod_traverse(PyObject *self, visitproc visit, void *arg) {
    Py_VISIT(PyInstanceMethod_GET_FUNCTION(self));
    return 0;
}

static PyObject *
instancemethod_call(PyObject *self, PyObject *arg, PyObject *kw)
{
    return PyObject_Call(PyInstanceMethod_GET_FUNCTION(self), arg, kw);
}

static PyObject *
instancemethod_descr_get(PyObject *descr, PyObject *obj, PyObject *type) {
    PyObject *func = PyInstanceMethod_GET_FUNCTION(descr);
    if (obj == NULL) {
        return Py_NewRef(func);
    }
    else
        return PyMethod_New(func, obj);
}

static PyObject *
instancemethod_richcompare(PyObject *self, PyObject *other, int op)
{
    PyInstanceMethodObject *a, *b;
    PyObject *res;
    int eq;

    if ((op != Py_EQ && op != Py_NE) ||
        !PyInstanceMethod_Check(self) ||
        !PyInstanceMethod_Check(other))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }
    a = (PyInstanceMethodObject *)self;
    b = (PyInstanceMethodObject *)other;
    eq = PyObject_RichCompareBool(a->func, b->func, Py_EQ);
    if (eq < 0)
        return NULL;
    if (op == Py_EQ)
        res = eq ? Py_True : Py_False;
    else
        res = eq ? Py_False : Py_True;
    return Py_NewRef(res);
}

static PyObject *
instancemethod_repr(PyObject *self)
{
    PyObject *func = PyInstanceMethod_Function(self);
    PyObject *funcname, *result;
    const char *defname = "?";

    if (func == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (PyObject_GetOptionalAttr(func, &_Py_ID(__name__), &funcname) < 0) {
        return NULL;
    }
    if (funcname != NULL && !PyUnicode_Check(funcname)) {
        Py_SETREF(funcname, NULL);
    }

    result = PyUnicode_FromFormat("<instancemethod %V at %p>",
                                  funcname, defname, self);

    Py_XDECREF(funcname);
    return result;
}

/*[clinic input]
@classmethod
instancemethod.__new__ as instancemethod_new
    function: object
    /

Bind a function to a class.
[clinic start generated code]*/

static PyObject *
instancemethod_new_impl(PyTypeObject *type, PyObject *function)
/*[clinic end generated code: output=5e0397b2bdb750be input=cfc54e8b973664a8]*/
{
    if (!PyCallable_Check(function)) {
        PyErr_SetString(PyExc_TypeError,
                        "first argument must be callable");
        return NULL;
    }

    return PyInstanceMethod_New(function);
}

PyTypeObject PyInstanceMethod_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "instancemethod",
    .tp_basicsize = sizeof(PyInstanceMethodObject),
    .tp_dealloc = instancemethod_dealloc,
    .tp_repr = (reprfunc)instancemethod_repr,
    .tp_call = instancemethod_call,
    .tp_getattro = instancemethod_getattro,
    .tp_setattro = PyObject_GenericSetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_doc = instancemethod_new__doc__,
    .tp_traverse = instancemethod_traverse,
    .tp_richcompare = instancemethod_richcompare,
    .tp_members = instancemethod_memberlist,
    .tp_getset = instancemethod_getset,
    .tp_descr_get = instancemethod_descr_get,
    .tp_new = instancemethod_new,
};
