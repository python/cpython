/* Descriptors -- a new, flexible way to describe attributes */

#include "Python.h"
#include "pycore_ceval.h"         // _Py_EnterRecursiveCallTstate()
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "structmember.h"         // PyMemberDef
#include "pycore_descrobject.h"

/*[clinic input]
class mappingproxy "mappingproxyobject *" "&PyDictProxy_Type"
class property "propertyobject *" "&PyProperty_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=556352653fd4c02e]*/

// see pycore_object.h
#if defined(__EMSCRIPTEN__) && defined(PY_CALL_TRAMPOLINE)
#include <emscripten.h>
EM_JS(int, descr_set_trampoline_call, (setter set, PyObject *obj, PyObject *value, void *closure), {
    return wasmTable.get(set)(obj, value, closure);
});

EM_JS(PyObject*, descr_get_trampoline_call, (getter get, PyObject *obj, void *closure), {
    return wasmTable.get(get)(obj, closure);
});
#else
#define descr_set_trampoline_call(set, obj, value, closure) \
    (set)((obj), (value), (closure))

#define descr_get_trampoline_call(get, obj, closure) \
    (get)((obj), (closure))

#endif // __EMSCRIPTEN__ && PY_CALL_TRAMPOLINE

static void
descr_dealloc(PyDescrObject *descr)
{
    _PyObject_GC_UNTRACK(descr);
    Py_XDECREF(descr->d_type);
    Py_XDECREF(descr->d_name);
    Py_XDECREF(descr->d_qualname);
    PyObject_GC_Del(descr);
}

static PyObject *
descr_name(PyDescrObject *descr)
{
    if (descr->d_name != NULL && PyUnicode_Check(descr->d_name))
        return descr->d_name;
    return NULL;
}

static PyObject *
descr_repr(PyDescrObject *descr, const char *format)
{
    PyObject *name = NULL;
    if (descr->d_name != NULL && PyUnicode_Check(descr->d_name))
        name = descr->d_name;

    return PyUnicode_FromFormat(format, name, "?", descr->d_type->tp_name);
}

static PyObject *
method_repr(PyMethodDescrObject *descr)
{
    return descr_repr((PyDescrObject *)descr,
                      "<method '%V' of '%s' objects>");
}

static PyObject *
member_repr(PyMemberDescrObject *descr)
{
    return descr_repr((PyDescrObject *)descr,
                      "<member '%V' of '%s' objects>");
}

static PyObject *
getset_repr(PyGetSetDescrObject *descr)
{
    return descr_repr((PyDescrObject *)descr,
                      "<attribute '%V' of '%s' objects>");
}

static PyObject *
wrapperdescr_repr(PyWrapperDescrObject *descr)
{
    return descr_repr((PyDescrObject *)descr,
                      "<slot wrapper '%V' of '%s' objects>");
}

static int
descr_check(PyDescrObject *descr, PyObject *obj)
{
    if (!PyObject_TypeCheck(obj, descr->d_type)) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' for '%.100s' objects "
                     "doesn't apply to a '%.100s' object",
                     descr_name((PyDescrObject *)descr), "?",
                     descr->d_type->tp_name,
                     Py_TYPE(obj)->tp_name);
        return -1;
    }
    return 0;
}

static PyObject *
classmethod_get(PyMethodDescrObject *descr, PyObject *obj, PyObject *type)
{
    /* Ensure a valid type.  Class methods ignore obj. */
    if (type == NULL) {
        if (obj != NULL)
            type = (PyObject *)Py_TYPE(obj);
        else {
            /* Wot - no type?! */
            PyErr_Format(PyExc_TypeError,
                         "descriptor '%V' for type '%.100s' "
                         "needs either an object or a type",
                         descr_name((PyDescrObject *)descr), "?",
                         PyDescr_TYPE(descr)->tp_name);
            return NULL;
        }
    }
    if (!PyType_Check(type)) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' for type '%.100s' "
                     "needs a type, not a '%.100s' as arg 2",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name,
                     Py_TYPE(type)->tp_name);
        return NULL;
    }
    if (!PyType_IsSubtype((PyTypeObject *)type, PyDescr_TYPE(descr))) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' requires a subtype of '%.100s' "
                     "but received '%.100s'",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name,
                     ((PyTypeObject *)type)->tp_name);
        return NULL;
    }
    PyTypeObject *cls = NULL;
    if (descr->d_method->ml_flags & METH_METHOD) {
        cls = descr->d_common.d_type;
    }
    return PyCMethod_New(descr->d_method, type, NULL, cls);
}

static PyObject *
method_get(PyMethodDescrObject *descr, PyObject *obj, PyObject *type)
{
    if (obj == NULL) {
        return Py_NewRef(descr);
    }
    if (descr_check((PyDescrObject *)descr, obj) < 0) {
        return NULL;
    }
    if (descr->d_method->ml_flags & METH_METHOD) {
        if (PyType_Check(type)) {
            return PyCMethod_New(descr->d_method, obj, NULL, descr->d_common.d_type);
        } else {
            PyErr_Format(PyExc_TypeError,
                        "descriptor '%V' needs a type, not '%s', as arg 2",
                        descr_name((PyDescrObject *)descr),
                        Py_TYPE(type)->tp_name);
            return NULL;
        }
    } else {
        return PyCFunction_NewEx(descr->d_method, obj, NULL);
    }
}

static PyObject *
member_get(PyMemberDescrObject *descr, PyObject *obj, PyObject *type)
{
    if (obj == NULL) {
        return Py_NewRef(descr);
    }
    if (descr_check((PyDescrObject *)descr, obj) < 0) {
        return NULL;
    }

    if (descr->d_member->flags & PY_AUDIT_READ) {
        if (PySys_Audit("object.__getattr__", "Os",
            obj ? obj : Py_None, descr->d_member->name) < 0) {
            return NULL;
        }
    }

    return PyMember_GetOne((char *)obj, descr->d_member);
}

static PyObject *
getset_get(PyGetSetDescrObject *descr, PyObject *obj, PyObject *type)
{
    if (obj == NULL) {
        return Py_NewRef(descr);
    }
    if (descr_check((PyDescrObject *)descr, obj) < 0) {
        return NULL;
    }
    if (descr->d_getset->get != NULL)
        return descr_get_trampoline_call(
            descr->d_getset->get, obj, descr->d_getset->closure);
    PyErr_Format(PyExc_AttributeError,
                 "attribute '%V' of '%.100s' objects is not readable",
                 descr_name((PyDescrObject *)descr), "?",
                 PyDescr_TYPE(descr)->tp_name);
    return NULL;
}

static PyObject *
wrapperdescr_get(PyWrapperDescrObject *descr, PyObject *obj, PyObject *type)
{
    if (obj == NULL) {
        return Py_NewRef(descr);
    }
    if (descr_check((PyDescrObject *)descr, obj) < 0) {
        return NULL;
    }
    return PyWrapper_New((PyObject *)descr, obj);
}

static int
descr_setcheck(PyDescrObject *descr, PyObject *obj, PyObject *value)
{
    assert(obj != NULL);
    if (!PyObject_TypeCheck(obj, descr->d_type)) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' for '%.100s' objects "
                     "doesn't apply to a '%.100s' object",
                     descr_name(descr), "?",
                     descr->d_type->tp_name,
                     Py_TYPE(obj)->tp_name);
        return -1;
    }
    return 0;
}

static int
member_set(PyMemberDescrObject *descr, PyObject *obj, PyObject *value)
{
    if (descr_setcheck((PyDescrObject *)descr, obj, value) < 0) {
        return -1;
    }
    return PyMember_SetOne((char *)obj, descr->d_member, value);
}

static int
getset_set(PyGetSetDescrObject *descr, PyObject *obj, PyObject *value)
{
    if (descr_setcheck((PyDescrObject *)descr, obj, value) < 0) {
        return -1;
    }
    if (descr->d_getset->set != NULL) {
        return descr_set_trampoline_call(
            descr->d_getset->set, obj, value,
            descr->d_getset->closure);
    }
    PyErr_Format(PyExc_AttributeError,
                 "attribute '%V' of '%.100s' objects is not writable",
                 descr_name((PyDescrObject *)descr), "?",
                 PyDescr_TYPE(descr)->tp_name);
    return -1;
}


/* Vectorcall functions for each of the PyMethodDescr calling conventions.
 *
 * First, common helpers
 */
static inline int
method_check_args(PyObject *func, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    assert(!PyErr_Occurred());
    if (nargs < 1) {
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            PyErr_Format(PyExc_TypeError,
                         "unbound method %U needs an argument", funcstr);
            Py_DECREF(funcstr);
        }
        return -1;
    }
    PyObject *self = args[0];
    if (descr_check((PyDescrObject *)func, self) < 0) {
        return -1;
    }
    if (kwnames && PyTuple_GET_SIZE(kwnames)) {
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            PyErr_Format(PyExc_TypeError,
                         "%U takes no keyword arguments", funcstr);
            Py_DECREF(funcstr);
        }
        return -1;
    }
    return 0;
}

typedef void (*funcptr)(void);

static inline funcptr
method_enter_call(PyThreadState *tstate, PyObject *func)
{
    if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object")) {
        return NULL;
    }
    return (funcptr)((PyMethodDescrObject *)func)->d_method->ml_meth;
}

/* Now the actual vectorcall functions */
static PyObject *
method_vectorcall_VARARGS(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_args(func, args, nargs, kwnames)) {
        return NULL;
    }
    PyObject *argstuple = _PyTuple_FromArray(args+1, nargs-1);
    if (argstuple == NULL) {
        return NULL;
    }
    PyCFunction meth = (PyCFunction)method_enter_call(tstate, func);
    if (meth == NULL) {
        Py_DECREF(argstuple);
        return NULL;
    }
    PyObject *result = _PyCFunction_TrampolineCall(
        meth, args[0], argstuple);
    Py_DECREF(argstuple);
    _Py_LeaveRecursiveCallTstate(tstate);
    return result;
}

static PyObject *
method_vectorcall_VARARGS_KEYWORDS(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_args(func, args, nargs, NULL)) {
        return NULL;
    }
    PyObject *argstuple = _PyTuple_FromArray(args+1, nargs-1);
    if (argstuple == NULL) {
        return NULL;
    }
    PyObject *result = NULL;
    /* Create a temporary dict for keyword arguments */
    PyObject *kwdict = NULL;
    if (kwnames != NULL && PyTuple_GET_SIZE(kwnames) > 0) {
        kwdict = _PyStack_AsDict(args + nargs, kwnames);
        if (kwdict == NULL) {
            goto exit;
        }
    }
    PyCFunctionWithKeywords meth = (PyCFunctionWithKeywords)
                                   method_enter_call(tstate, func);
    if (meth == NULL) {
        goto exit;
    }
    result = _PyCFunctionWithKeywords_TrampolineCall(
        meth, args[0], argstuple, kwdict);
    _Py_LeaveRecursiveCallTstate(tstate);
exit:
    Py_DECREF(argstuple);
    Py_XDECREF(kwdict);
    return result;
}

static PyObject *
method_vectorcall_FASTCALL_KEYWORDS_METHOD(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_args(func, args, nargs, NULL)) {
        return NULL;
    }
    PyCMethod meth = (PyCMethod) method_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    PyObject *result = meth(args[0],
                            ((PyMethodDescrObject *)func)->d_common.d_type,
                            args+1, nargs-1, kwnames);
    _Py_LeaveRecursiveCall();
    return result;
}

static PyObject *
method_vectorcall_FASTCALL(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_args(func, args, nargs, kwnames)) {
        return NULL;
    }
    _PyCFunctionFast meth = (_PyCFunctionFast)
                            method_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    PyObject *result = meth(args[0], args+1, nargs-1);
    _Py_LeaveRecursiveCallTstate(tstate);
    return result;
}

static PyObject *
method_vectorcall_FASTCALL_KEYWORDS(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_args(func, args, nargs, NULL)) {
        return NULL;
    }
    _PyCFunctionFastWithKeywords meth = (_PyCFunctionFastWithKeywords)
                                        method_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    PyObject *result = meth(args[0], args+1, nargs-1, kwnames);
    _Py_LeaveRecursiveCallTstate(tstate);
    return result;
}

static PyObject *
method_vectorcall_NOARGS(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_args(func, args, nargs, kwnames)) {
        return NULL;
    }
    if (nargs != 1) {
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            PyErr_Format(PyExc_TypeError,
                "%U takes no arguments (%zd given)", funcstr, nargs-1);
            Py_DECREF(funcstr);
        }
        return NULL;
    }
    PyCFunction meth = (PyCFunction)method_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    PyObject *result = _PyCFunction_TrampolineCall(meth, args[0], NULL);
    _Py_LeaveRecursiveCallTstate(tstate);
    return result;
}

static PyObject *
method_vectorcall_O(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_args(func, args, nargs, kwnames)) {
        return NULL;
    }
    if (nargs != 2) {
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            PyErr_Format(PyExc_TypeError,
                "%U takes exactly one argument (%zd given)",
                funcstr, nargs-1);
            Py_DECREF(funcstr);
        }
        return NULL;
    }
    PyCFunction meth = (PyCFunction)method_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    PyObject *result = _PyCFunction_TrampolineCall(meth, args[0], args[1]);
    _Py_LeaveRecursiveCallTstate(tstate);
    return result;
}


/* Instances of classmethod_descriptor are unlikely to be called directly.
   For one, the analogous class "classmethod" (for Python classes) is not
   callable. Second, users are not likely to access a classmethod_descriptor
   directly, since it means pulling it from the class __dict__.

   This is just an excuse to say that this doesn't need to be optimized:
   we implement this simply by calling __get__ and then calling the result.
*/
static PyObject *
classmethoddescr_call(PyMethodDescrObject *descr, PyObject *args,
                      PyObject *kwds)
{
    Py_ssize_t argc = PyTuple_GET_SIZE(args);
    if (argc < 1) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' of '%.100s' "
                     "object needs an argument",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name);
        return NULL;
    }
    PyObject *self = PyTuple_GET_ITEM(args, 0);
    PyObject *bound = classmethod_get(descr, NULL, self);
    if (bound == NULL) {
        return NULL;
    }
    PyObject *res = PyObject_VectorcallDict(bound, _PyTuple_ITEMS(args)+1,
                                           argc-1, kwds);
    Py_DECREF(bound);
    return res;
}

Py_LOCAL_INLINE(PyObject *)
wrapperdescr_raw_call(PyWrapperDescrObject *descr, PyObject *self,
                      PyObject *args, PyObject *kwds)
{
    wrapperfunc wrapper = descr->d_base->wrapper;

    if (descr->d_base->flags & PyWrapperFlag_KEYWORDS) {
        wrapperfunc_kwds wk = (wrapperfunc_kwds)(void(*)(void))wrapper;
        return (*wk)(self, args, descr->d_wrapped, kwds);
    }

    if (kwds != NULL && (!PyDict_Check(kwds) || PyDict_GET_SIZE(kwds) != 0)) {
        PyErr_Format(PyExc_TypeError,
                     "wrapper %s() takes no keyword arguments",
                     descr->d_base->name);
        return NULL;
    }
    return (*wrapper)(self, args, descr->d_wrapped);
}

static PyObject *
wrapperdescr_call(PyWrapperDescrObject *descr, PyObject *args, PyObject *kwds)
{
    Py_ssize_t argc;
    PyObject *self, *result;

    /* Make sure that the first argument is acceptable as 'self' */
    assert(PyTuple_Check(args));
    argc = PyTuple_GET_SIZE(args);
    if (argc < 1) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' of '%.100s' "
                     "object needs an argument",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name);
        return NULL;
    }
    self = PyTuple_GET_ITEM(args, 0);
    if (!_PyObject_RealIsSubclass((PyObject *)Py_TYPE(self),
                                  (PyObject *)PyDescr_TYPE(descr))) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' "
                     "requires a '%.100s' object "
                     "but received a '%.100s'",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name,
                     Py_TYPE(self)->tp_name);
        return NULL;
    }

    args = PyTuple_GetSlice(args, 1, argc);
    if (args == NULL) {
        return NULL;
    }
    result = wrapperdescr_raw_call(descr, self, args, kwds);
    Py_DECREF(args);
    return result;
}


static PyObject *
method_get_doc(PyMethodDescrObject *descr, void *closure)
{
    return _PyType_GetDocFromInternalDoc(descr->d_method->ml_name, descr->d_method->ml_doc);
}

static PyObject *
method_get_text_signature(PyMethodDescrObject *descr, void *closure)
{
    return _PyType_GetTextSignatureFromInternalDoc(descr->d_method->ml_name, descr->d_method->ml_doc);
}

static PyObject *
calculate_qualname(PyDescrObject *descr)
{
    PyObject *type_qualname, *res;

    if (descr->d_name == NULL || !PyUnicode_Check(descr->d_name)) {
        PyErr_SetString(PyExc_TypeError,
                        "<descriptor>.__name__ is not a unicode object");
        return NULL;
    }

    type_qualname = PyObject_GetAttr(
            (PyObject *)descr->d_type, &_Py_ID(__qualname__));
    if (type_qualname == NULL)
        return NULL;

    if (!PyUnicode_Check(type_qualname)) {
        PyErr_SetString(PyExc_TypeError, "<descriptor>.__objclass__."
                        "__qualname__ is not a unicode object");
        Py_XDECREF(type_qualname);
        return NULL;
    }

    res = PyUnicode_FromFormat("%S.%S", type_qualname, descr->d_name);
    Py_DECREF(type_qualname);
    return res;
}

static PyObject *
descr_get_qualname(PyDescrObject *descr, void *Py_UNUSED(ignored))
{
    if (descr->d_qualname == NULL)
        descr->d_qualname = calculate_qualname(descr);
    return Py_XNewRef(descr->d_qualname);
}

static PyObject *
descr_reduce(PyDescrObject *descr, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("N(OO)", _PyEval_GetBuiltin(&_Py_ID(getattr)),
                         PyDescr_TYPE(descr), PyDescr_NAME(descr));
}

static PyMethodDef descr_methods[] = {
    {"__reduce__", (PyCFunction)descr_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

static PyMemberDef descr_members[] = {
    {"__objclass__", T_OBJECT, offsetof(PyDescrObject, d_type), READONLY},
    {"__name__", T_OBJECT, offsetof(PyDescrObject, d_name), READONLY},
    {0}
};

static PyGetSetDef method_getset[] = {
    {"__doc__", (getter)method_get_doc},
    {"__qualname__", (getter)descr_get_qualname},
    {"__text_signature__", (getter)method_get_text_signature},
    {0}
};

static PyObject *
member_get_doc(PyMemberDescrObject *descr, void *closure)
{
    if (descr->d_member->doc == NULL) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(descr->d_member->doc);
}

static PyGetSetDef member_getset[] = {
    {"__doc__", (getter)member_get_doc},
    {"__qualname__", (getter)descr_get_qualname},
    {0}
};

static PyObject *
getset_get_doc(PyGetSetDescrObject *descr, void *closure)
{
    if (descr->d_getset->doc == NULL) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(descr->d_getset->doc);
}

static PyGetSetDef getset_getset[] = {
    {"__doc__", (getter)getset_get_doc},
    {"__qualname__", (getter)descr_get_qualname},
    {0}
};

static PyObject *
wrapperdescr_get_doc(PyWrapperDescrObject *descr, void *closure)
{
    return _PyType_GetDocFromInternalDoc(descr->d_base->name, descr->d_base->doc);
}

static PyObject *
wrapperdescr_get_text_signature(PyWrapperDescrObject *descr, void *closure)
{
    return _PyType_GetTextSignatureFromInternalDoc(descr->d_base->name, descr->d_base->doc);
}

static PyGetSetDef wrapperdescr_getset[] = {
    {"__doc__", (getter)wrapperdescr_get_doc},
    {"__qualname__", (getter)descr_get_qualname},
    {"__text_signature__", (getter)wrapperdescr_get_text_signature},
    {0}
};

static int
descr_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyDescrObject *descr = (PyDescrObject *)self;
    Py_VISIT(descr->d_type);
    return 0;
}

PyTypeObject PyMethodDescr_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "method_descriptor",
    sizeof(PyMethodDescrObject),
    0,
    (destructor)descr_dealloc,                  /* tp_dealloc */
    offsetof(PyMethodDescrObject, vectorcall),  /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)method_repr,                      /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    PyVectorcall_Call,                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
    Py_TPFLAGS_HAVE_VECTORCALL |
    Py_TPFLAGS_METHOD_DESCRIPTOR,               /* tp_flags */
    0,                                          /* tp_doc */
    descr_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    descr_methods,                              /* tp_methods */
    descr_members,                              /* tp_members */
    method_getset,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    (descrgetfunc)method_get,                   /* tp_descr_get */
    0,                                          /* tp_descr_set */
};

/* This is for METH_CLASS in C, not for "f = classmethod(f)" in Python! */
PyTypeObject PyClassMethodDescr_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "classmethod_descriptor",
    sizeof(PyMethodDescrObject),
    0,
    (destructor)descr_dealloc,                  /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)method_repr,                      /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    (ternaryfunc)classmethoddescr_call,         /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    0,                                          /* tp_doc */
    descr_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    descr_members,                              /* tp_members */
    method_getset,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    (descrgetfunc)classmethod_get,              /* tp_descr_get */
    0,                                          /* tp_descr_set */
};

PyTypeObject PyMemberDescr_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "member_descriptor",
    sizeof(PyMemberDescrObject),
    0,
    (destructor)descr_dealloc,                  /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)member_repr,                      /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    0,                                          /* tp_doc */
    descr_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    descr_methods,                              /* tp_methods */
    descr_members,                              /* tp_members */
    member_getset,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    (descrgetfunc)member_get,                   /* tp_descr_get */
    (descrsetfunc)member_set,                   /* tp_descr_set */
};

PyTypeObject PyGetSetDescr_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "getset_descriptor",
    sizeof(PyGetSetDescrObject),
    0,
    (destructor)descr_dealloc,                  /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)getset_repr,                      /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    0,                                          /* tp_doc */
    descr_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    descr_members,                              /* tp_members */
    getset_getset,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    (descrgetfunc)getset_get,                   /* tp_descr_get */
    (descrsetfunc)getset_set,                   /* tp_descr_set */
};

PyTypeObject PyWrapperDescr_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "wrapper_descriptor",
    sizeof(PyWrapperDescrObject),
    0,
    (destructor)descr_dealloc,                  /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)wrapperdescr_repr,                /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    (ternaryfunc)wrapperdescr_call,             /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
    Py_TPFLAGS_METHOD_DESCRIPTOR,               /* tp_flags */
    0,                                          /* tp_doc */
    descr_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    descr_methods,                              /* tp_methods */
    descr_members,                              /* tp_members */
    wrapperdescr_getset,                        /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    (descrgetfunc)wrapperdescr_get,             /* tp_descr_get */
    0,                                          /* tp_descr_set */
};

static PyDescrObject *
descr_new(PyTypeObject *descrtype, PyTypeObject *type, const char *name)
{
    PyDescrObject *descr;

    descr = (PyDescrObject *)PyType_GenericAlloc(descrtype, 0);
    if (descr != NULL) {
        descr->d_type = (PyTypeObject*)Py_XNewRef(type);
        descr->d_name = PyUnicode_InternFromString(name);
        if (descr->d_name == NULL) {
            Py_SETREF(descr, NULL);
        }
        else {
            descr->d_qualname = NULL;
        }
    }
    return descr;
}

PyObject *
PyDescr_NewMethod(PyTypeObject *type, PyMethodDef *method)
{
    /* Figure out correct vectorcall function to use */
    vectorcallfunc vectorcall;
    switch (method->ml_flags & (METH_VARARGS | METH_FASTCALL | METH_NOARGS |
                                METH_O | METH_KEYWORDS | METH_METHOD))
    {
        case METH_VARARGS:
            vectorcall = method_vectorcall_VARARGS;
            break;
        case METH_VARARGS | METH_KEYWORDS:
            vectorcall = method_vectorcall_VARARGS_KEYWORDS;
            break;
        case METH_FASTCALL:
            vectorcall = method_vectorcall_FASTCALL;
            break;
        case METH_FASTCALL | METH_KEYWORDS:
            vectorcall = method_vectorcall_FASTCALL_KEYWORDS;
            break;
        case METH_NOARGS:
            vectorcall = method_vectorcall_NOARGS;
            break;
        case METH_O:
            vectorcall = method_vectorcall_O;
            break;
        case METH_METHOD | METH_FASTCALL | METH_KEYWORDS:
            vectorcall = method_vectorcall_FASTCALL_KEYWORDS_METHOD;
            break;
        default:
            PyErr_Format(PyExc_SystemError,
                         "%s() method: bad call flags", method->ml_name);
            return NULL;
    }

    PyMethodDescrObject *descr;

    descr = (PyMethodDescrObject *)descr_new(&PyMethodDescr_Type,
                                             type, method->ml_name);
    if (descr != NULL) {
        descr->d_method = method;
        descr->vectorcall = vectorcall;
    }
    return (PyObject *)descr;
}

PyObject *
PyDescr_NewClassMethod(PyTypeObject *type, PyMethodDef *method)
{
    PyMethodDescrObject *descr;

    descr = (PyMethodDescrObject *)descr_new(&PyClassMethodDescr_Type,
                                             type, method->ml_name);
    if (descr != NULL)
        descr->d_method = method;
    return (PyObject *)descr;
}

PyObject *
PyDescr_NewMember(PyTypeObject *type, PyMemberDef *member)
{
    PyMemberDescrObject *descr;

    if (member->flags & Py_RELATIVE_OFFSET) {
        PyErr_SetString(
            PyExc_SystemError,
            "PyDescr_NewMember used with Py_RELATIVE_OFFSET");
        return NULL;
    }
    descr = (PyMemberDescrObject *)descr_new(&PyMemberDescr_Type,
                                             type, member->name);
    if (descr != NULL)
        descr->d_member = member;
    return (PyObject *)descr;
}

PyObject *
PyDescr_NewGetSet(PyTypeObject *type, PyGetSetDef *getset)
{
    PyGetSetDescrObject *descr;

    descr = (PyGetSetDescrObject *)descr_new(&PyGetSetDescr_Type,
                                             type, getset->name);
    if (descr != NULL)
        descr->d_getset = getset;
    return (PyObject *)descr;
}

PyObject *
PyDescr_NewWrapper(PyTypeObject *type, struct wrapperbase *base, void *wrapped)
{
    PyWrapperDescrObject *descr;

    descr = (PyWrapperDescrObject *)descr_new(&PyWrapperDescr_Type,
                                             type, base->name);
    if (descr != NULL) {
        descr->d_base = base;
        descr->d_wrapped = wrapped;
    }
    return (PyObject *)descr;
}

int
PyDescr_IsData(PyObject *ob)
{
    return Py_TYPE(ob)->tp_descr_set != NULL;
}

/* --- mappingproxy: read-only proxy for mappings --- */

/* This has no reason to be in this file except that adding new files is a
   bit of a pain */

typedef struct {
    PyObject_HEAD
    PyObject *mapping;
} mappingproxyobject;

static Py_ssize_t
mappingproxy_len(mappingproxyobject *pp)
{
    return PyObject_Size(pp->mapping);
}

static PyObject *
mappingproxy_getitem(mappingproxyobject *pp, PyObject *key)
{
    return PyObject_GetItem(pp->mapping, key);
}

static PyMappingMethods mappingproxy_as_mapping = {
    (lenfunc)mappingproxy_len,                  /* mp_length */
    (binaryfunc)mappingproxy_getitem,           /* mp_subscript */
    0,                                          /* mp_ass_subscript */
};

static PyObject *
mappingproxy_or(PyObject *left, PyObject *right)
{
    if (PyObject_TypeCheck(left, &PyDictProxy_Type)) {
        left = ((mappingproxyobject*)left)->mapping;
    }
    if (PyObject_TypeCheck(right, &PyDictProxy_Type)) {
        right = ((mappingproxyobject*)right)->mapping;
    }
    return PyNumber_Or(left, right);
}

static PyObject *
mappingproxy_ior(PyObject *self, PyObject *Py_UNUSED(other))
{
    return PyErr_Format(PyExc_TypeError,
        "'|=' is not supported by %s; use '|' instead", Py_TYPE(self)->tp_name);
}

static PyNumberMethods mappingproxy_as_number = {
    .nb_or = mappingproxy_or,
    .nb_inplace_or = mappingproxy_ior,
};

static int
mappingproxy_contains(mappingproxyobject *pp, PyObject *key)
{
    if (PyDict_CheckExact(pp->mapping))
        return PyDict_Contains(pp->mapping, key);
    else
        return PySequence_Contains(pp->mapping, key);
}

static PySequenceMethods mappingproxy_as_sequence = {
    0,                                          /* sq_length */
    0,                                          /* sq_concat */
    0,                                          /* sq_repeat */
    0,                                          /* sq_item */
    0,                                          /* sq_slice */
    0,                                          /* sq_ass_item */
    0,                                          /* sq_ass_slice */
    (objobjproc)mappingproxy_contains,                 /* sq_contains */
    0,                                          /* sq_inplace_concat */
    0,                                          /* sq_inplace_repeat */
};

static PyObject *
mappingproxy_get(mappingproxyobject *pp, PyObject *const *args, Py_ssize_t nargs)
{
    /* newargs: mapping, key, default=None */
    PyObject *newargs[3];
    newargs[0] = pp->mapping;
    newargs[2] = Py_None;

    if (!_PyArg_UnpackStack(args, nargs, "get", 1, 2,
                            &newargs[1], &newargs[2]))
    {
        return NULL;
    }
    return _PyObject_VectorcallMethod(&_Py_ID(get), newargs,
                                        3 | PY_VECTORCALL_ARGUMENTS_OFFSET,
                                        NULL);
}

static PyObject *
mappingproxy_keys(mappingproxyobject *pp, PyObject *Py_UNUSED(ignored))
{
    return PyObject_CallMethodNoArgs(pp->mapping, &_Py_ID(keys));
}

static PyObject *
mappingproxy_values(mappingproxyobject *pp, PyObject *Py_UNUSED(ignored))
{
    return PyObject_CallMethodNoArgs(pp->mapping, &_Py_ID(values));
}

static PyObject *
mappingproxy_items(mappingproxyobject *pp, PyObject *Py_UNUSED(ignored))
{
    return PyObject_CallMethodNoArgs(pp->mapping, &_Py_ID(items));
}

static PyObject *
mappingproxy_copy(mappingproxyobject *pp, PyObject *Py_UNUSED(ignored))
{
    return PyObject_CallMethodNoArgs(pp->mapping, &_Py_ID(copy));
}

static PyObject *
mappingproxy_reversed(mappingproxyobject *pp, PyObject *Py_UNUSED(ignored))
{
    return PyObject_CallMethodNoArgs(pp->mapping, &_Py_ID(__reversed__));
}

/* WARNING: mappingproxy methods must not give access
            to the underlying mapping */

static PyMethodDef mappingproxy_methods[] = {
    {"get",       _PyCFunction_CAST(mappingproxy_get), METH_FASTCALL,
     PyDoc_STR("D.get(k[,d]) -> D[k] if k in D, else d."
               "  d defaults to None.")},
    {"keys",      (PyCFunction)mappingproxy_keys,       METH_NOARGS,
     PyDoc_STR("D.keys() -> a set-like object providing a view on D's keys")},
    {"values",    (PyCFunction)mappingproxy_values,     METH_NOARGS,
     PyDoc_STR("D.values() -> an object providing a view on D's values")},
    {"items",     (PyCFunction)mappingproxy_items,      METH_NOARGS,
     PyDoc_STR("D.items() -> a set-like object providing a view on D's items")},
    {"copy",      (PyCFunction)mappingproxy_copy,       METH_NOARGS,
     PyDoc_STR("D.copy() -> a shallow copy of D")},
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS,
     PyDoc_STR("See PEP 585")},
    {"__reversed__", (PyCFunction)mappingproxy_reversed, METH_NOARGS,
     PyDoc_STR("D.__reversed__() -> reverse iterator")},
    {0}
};

static void
mappingproxy_dealloc(mappingproxyobject *pp)
{
    _PyObject_GC_UNTRACK(pp);
    Py_DECREF(pp->mapping);
    PyObject_GC_Del(pp);
}

static PyObject *
mappingproxy_getiter(mappingproxyobject *pp)
{
    return PyObject_GetIter(pp->mapping);
}

static Py_hash_t
mappingproxy_hash(mappingproxyobject *pp)
{
    return PyObject_Hash(pp->mapping);
}

static PyObject *
mappingproxy_str(mappingproxyobject *pp)
{
    return PyObject_Str(pp->mapping);
}

static PyObject *
mappingproxy_repr(mappingproxyobject *pp)
{
    return PyUnicode_FromFormat("mappingproxy(%R)", pp->mapping);
}

static int
mappingproxy_traverse(PyObject *self, visitproc visit, void *arg)
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    Py_VISIT(pp->mapping);
    return 0;
}

static PyObject *
mappingproxy_richcompare(mappingproxyobject *v, PyObject *w, int op)
{
    return PyObject_RichCompare(v->mapping, w, op);
}

static int
mappingproxy_check_mapping(PyObject *mapping)
{
    if (!PyMapping_Check(mapping)
        || PyList_Check(mapping)
        || PyTuple_Check(mapping)) {
        PyErr_Format(PyExc_TypeError,
                    "mappingproxy() argument must be a mapping, not %s",
                    Py_TYPE(mapping)->tp_name);
        return -1;
    }
    return 0;
}

/*[clinic input]
@classmethod
mappingproxy.__new__ as mappingproxy_new

    mapping: object

[clinic start generated code]*/

static PyObject *
mappingproxy_new_impl(PyTypeObject *type, PyObject *mapping)
/*[clinic end generated code: output=65f27f02d5b68fa7 input=d2d620d4f598d4f8]*/
{
    mappingproxyobject *mappingproxy;

    if (mappingproxy_check_mapping(mapping) == -1)
        return NULL;

    mappingproxy = PyObject_GC_New(mappingproxyobject, &PyDictProxy_Type);
    if (mappingproxy == NULL)
        return NULL;
    mappingproxy->mapping = Py_NewRef(mapping);
    _PyObject_GC_TRACK(mappingproxy);
    return (PyObject *)mappingproxy;
}

PyObject *
PyDictProxy_New(PyObject *mapping)
{
    mappingproxyobject *pp;

    if (mappingproxy_check_mapping(mapping) == -1)
        return NULL;

    pp = PyObject_GC_New(mappingproxyobject, &PyDictProxy_Type);
    if (pp != NULL) {
        pp->mapping = Py_NewRef(mapping);
        _PyObject_GC_TRACK(pp);
    }
    return (PyObject *)pp;
}


/* --- Wrapper object for "slot" methods --- */

/* This has no reason to be in this file except that adding new files is a
   bit of a pain */

typedef struct {
    PyObject_HEAD
    PyWrapperDescrObject *descr;
    PyObject *self;
} wrapperobject;

#define Wrapper_Check(v) Py_IS_TYPE(v, &_PyMethodWrapper_Type)

static void
wrapper_dealloc(wrapperobject *wp)
{
    PyObject_GC_UnTrack(wp);
    Py_TRASHCAN_BEGIN(wp, wrapper_dealloc)
    Py_XDECREF(wp->descr);
    Py_XDECREF(wp->self);
    PyObject_GC_Del(wp);
    Py_TRASHCAN_END
}

static PyObject *
wrapper_richcompare(PyObject *a, PyObject *b, int op)
{
    wrapperobject *wa, *wb;
    int eq;

    assert(a != NULL && b != NULL);

    /* both arguments should be wrapperobjects */
    if ((op != Py_EQ && op != Py_NE)
        || !Wrapper_Check(a) || !Wrapper_Check(b))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }

    wa = (wrapperobject *)a;
    wb = (wrapperobject *)b;
    eq = (wa->descr == wb->descr && wa->self == wb->self);
    if (eq == (op == Py_EQ)) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

static Py_hash_t
wrapper_hash(wrapperobject *wp)
{
    Py_hash_t x, y;
    x = _Py_HashPointer(wp->self);
    y = _Py_HashPointer(wp->descr);
    x = x ^ y;
    if (x == -1)
        x = -2;
    return x;
}

static PyObject *
wrapper_repr(wrapperobject *wp)
{
    return PyUnicode_FromFormat("<method-wrapper '%s' of %s object at %p>",
                               wp->descr->d_base->name,
                               Py_TYPE(wp->self)->tp_name,
                               wp->self);
}

static PyObject *
wrapper_reduce(wrapperobject *wp, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("N(OO)", _PyEval_GetBuiltin(&_Py_ID(getattr)),
                         wp->self, PyDescr_NAME(wp->descr));
}

static PyMethodDef wrapper_methods[] = {
    {"__reduce__", (PyCFunction)wrapper_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

static PyMemberDef wrapper_members[] = {
    {"__self__", T_OBJECT, offsetof(wrapperobject, self), READONLY},
    {0}
};

static PyObject *
wrapper_objclass(wrapperobject *wp, void *Py_UNUSED(ignored))
{
    PyObject *c = (PyObject *)PyDescr_TYPE(wp->descr);

    return Py_NewRef(c);
}

static PyObject *
wrapper_name(wrapperobject *wp, void *Py_UNUSED(ignored))
{
    const char *s = wp->descr->d_base->name;

    return PyUnicode_FromString(s);
}

static PyObject *
wrapper_doc(wrapperobject *wp, void *Py_UNUSED(ignored))
{
    return _PyType_GetDocFromInternalDoc(wp->descr->d_base->name, wp->descr->d_base->doc);
}

static PyObject *
wrapper_text_signature(wrapperobject *wp, void *Py_UNUSED(ignored))
{
    return _PyType_GetTextSignatureFromInternalDoc(wp->descr->d_base->name, wp->descr->d_base->doc);
}

static PyObject *
wrapper_qualname(wrapperobject *wp, void *Py_UNUSED(ignored))
{
    return descr_get_qualname((PyDescrObject *)wp->descr, NULL);
}

static PyGetSetDef wrapper_getsets[] = {
    {"__objclass__", (getter)wrapper_objclass},
    {"__name__", (getter)wrapper_name},
    {"__qualname__", (getter)wrapper_qualname},
    {"__doc__", (getter)wrapper_doc},
    {"__text_signature__", (getter)wrapper_text_signature},
    {0}
};

static PyObject *
wrapper_call(wrapperobject *wp, PyObject *args, PyObject *kwds)
{
    return wrapperdescr_raw_call(wp->descr, wp->self, args, kwds);
}

static int
wrapper_traverse(PyObject *self, visitproc visit, void *arg)
{
    wrapperobject *wp = (wrapperobject *)self;
    Py_VISIT(wp->descr);
    Py_VISIT(wp->self);
    return 0;
}

PyTypeObject _PyMethodWrapper_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "method-wrapper",                           /* tp_name */
    sizeof(wrapperobject),                      /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)wrapper_dealloc,                /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)wrapper_repr,                     /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    (hashfunc)wrapper_hash,                     /* tp_hash */
    (ternaryfunc)wrapper_call,                  /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    0,                                          /* tp_doc */
    wrapper_traverse,                           /* tp_traverse */
    0,                                          /* tp_clear */
    wrapper_richcompare,                        /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    wrapper_methods,                            /* tp_methods */
    wrapper_members,                            /* tp_members */
    wrapper_getsets,                            /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
};

PyObject *
PyWrapper_New(PyObject *d, PyObject *self)
{
    wrapperobject *wp;
    PyWrapperDescrObject *descr;

    assert(PyObject_TypeCheck(d, &PyWrapperDescr_Type));
    descr = (PyWrapperDescrObject *)d;
    assert(_PyObject_RealIsSubclass((PyObject *)Py_TYPE(self),
                                    (PyObject *)PyDescr_TYPE(descr)));

    wp = PyObject_GC_New(wrapperobject, &_PyMethodWrapper_Type);
    if (wp != NULL) {
        wp->descr = (PyWrapperDescrObject*)Py_NewRef(descr);
        wp->self = Py_NewRef(self);
        _PyObject_GC_TRACK(wp);
    }
    return (PyObject *)wp;
}


/* A built-in 'property' type */

/*
class property(object):

    def __init__(self, fget=None, fset=None, fdel=None, doc=None):
        if doc is None and fget is not None and hasattr(fget, "__doc__"):
            doc = fget.__doc__
        self.__get = fget
        self.__set = fset
        self.__del = fdel
        try:
            self.__doc__ = doc
        except AttributeError:  # read-only or dict-less class
            pass

    def __get__(self, inst, type=None):
        if inst is None:
            return self
        if self.__get is None:
            raise AttributeError, "property has no getter"
        return self.__get(inst)

    def __set__(self, inst, value):
        if self.__set is None:
            raise AttributeError, "property has no setter"
        return self.__set(inst, value)

    def __delete__(self, inst):
        if self.__del is None:
            raise AttributeError, "property has no deleter"
        return self.__del(inst)

*/

static PyObject * property_copy(PyObject *, PyObject *, PyObject *,
                                  PyObject *);

static PyMemberDef property_members[] = {
    {"fget", T_OBJECT, offsetof(propertyobject, prop_get), READONLY},
    {"fset", T_OBJECT, offsetof(propertyobject, prop_set), READONLY},
    {"fdel", T_OBJECT, offsetof(propertyobject, prop_del), READONLY},
    {"__doc__",  T_OBJECT, offsetof(propertyobject, prop_doc), 0},
    {0}
};


PyDoc_STRVAR(getter_doc,
             "Descriptor to obtain a copy of the property with a different getter.");

static PyObject *
property_getter(PyObject *self, PyObject *getter)
{
    return property_copy(self, getter, NULL, NULL);
}


PyDoc_STRVAR(setter_doc,
             "Descriptor to obtain a copy of the property with a different setter.");

static PyObject *
property_setter(PyObject *self, PyObject *setter)
{
    return property_copy(self, NULL, setter, NULL);
}


PyDoc_STRVAR(deleter_doc,
             "Descriptor to obtain a copy of the property with a different deleter.");

static PyObject *
property_deleter(PyObject *self, PyObject *deleter)
{
    return property_copy(self, NULL, NULL, deleter);
}


PyDoc_STRVAR(set_name_doc,
             "Method to set name of a property.");

static PyObject *
property_set_name(PyObject *self, PyObject *args) {
    if (PyTuple_GET_SIZE(args) != 2) {
        PyErr_Format(
                PyExc_TypeError,
                "__set_name__() takes 2 positional arguments but %d were given",
                PyTuple_GET_SIZE(args));
        return NULL;
    }

    propertyobject *prop = (propertyobject *)self;
    PyObject *name = PyTuple_GET_ITEM(args, 1);

    Py_XSETREF(prop->prop_name, Py_XNewRef(name));

    Py_RETURN_NONE;
}

static PyMethodDef property_methods[] = {
    {"getter", property_getter, METH_O, getter_doc},
    {"setter", property_setter, METH_O, setter_doc},
    {"deleter", property_deleter, METH_O, deleter_doc},
    {"__set_name__", property_set_name, METH_VARARGS, set_name_doc},
    {0}
};


static void
property_dealloc(PyObject *self)
{
    propertyobject *gs = (propertyobject *)self;

    _PyObject_GC_UNTRACK(self);
    Py_XDECREF(gs->prop_get);
    Py_XDECREF(gs->prop_set);
    Py_XDECREF(gs->prop_del);
    Py_XDECREF(gs->prop_doc);
    Py_XDECREF(gs->prop_name);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
property_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    if (obj == NULL || obj == Py_None) {
        return Py_NewRef(self);
    }

    propertyobject *gs = (propertyobject *)self;
    if (gs->prop_get == NULL) {
        PyObject *qualname = PyType_GetQualName(Py_TYPE(obj));
        if (gs->prop_name != NULL && qualname != NULL) {
            PyErr_Format(PyExc_AttributeError,
                         "property %R of %R object has no getter",
                         gs->prop_name,
                         qualname);
        }
        else if (qualname != NULL) {
            PyErr_Format(PyExc_AttributeError,
                         "property of %R object has no getter",
                         qualname);
        } else {
            PyErr_SetString(PyExc_AttributeError,
                            "property has no getter");
        }
        Py_XDECREF(qualname);
        return NULL;
    }

    return PyObject_CallOneArg(gs->prop_get, obj);
}

static int
property_descr_set(PyObject *self, PyObject *obj, PyObject *value)
{
    propertyobject *gs = (propertyobject *)self;
    PyObject *func, *res;

    if (value == NULL) {
        func = gs->prop_del;
    }
    else {
        func = gs->prop_set;
    }

    if (func == NULL) {
        PyObject *qualname = NULL;
        if (obj != NULL) {
            qualname = PyType_GetQualName(Py_TYPE(obj));
        }
        if (gs->prop_name != NULL && qualname != NULL) {
            PyErr_Format(PyExc_AttributeError,
                        value == NULL ?
                        "property %R of %R object has no deleter" :
                        "property %R of %R object has no setter",
                        gs->prop_name,
                        qualname);
        }
        else if (qualname != NULL) {
            PyErr_Format(PyExc_AttributeError,
                            value == NULL ?
                            "property of %R object has no deleter" :
                            "property of %R object has no setter",
                            qualname);
        }
        else {
            PyErr_SetString(PyExc_AttributeError,
                         value == NULL ?
                         "property has no deleter" :
                         "property has no setter");
        }
        Py_XDECREF(qualname);
        return -1;
    }

    if (value == NULL) {
        res = PyObject_CallOneArg(func, obj);
    }
    else {
        EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, func);
        PyObject *args[] = { obj, value };
        res = PyObject_Vectorcall(func, args, 2, NULL);
    }

    if (res == NULL) {
        return -1;
    }

    Py_DECREF(res);
    return 0;
}

static PyObject *
property_copy(PyObject *old, PyObject *get, PyObject *set, PyObject *del)
{
    propertyobject *pold = (propertyobject *)old;
    PyObject *new, *type, *doc;

    type = PyObject_Type(old);
    if (type == NULL)
        return NULL;

    if (get == NULL || get == Py_None) {
        get = pold->prop_get ? pold->prop_get : Py_None;
    }
    if (set == NULL || set == Py_None) {
        set = pold->prop_set ? pold->prop_set : Py_None;
    }
    if (del == NULL || del == Py_None) {
        del = pold->prop_del ? pold->prop_del : Py_None;
    }
    if (pold->getter_doc && get != Py_None) {
        /* make _init use __doc__ from getter */
        doc = Py_None;
    }
    else {
        doc = pold->prop_doc ? pold->prop_doc : Py_None;
    }

    new =  PyObject_CallFunctionObjArgs(type, get, set, del, doc, NULL);
    Py_DECREF(type);
    if (new == NULL)
        return NULL;

    if (PyObject_TypeCheck((new), &PyProperty_Type)) {
        Py_XSETREF(((propertyobject *) new)->prop_name, Py_XNewRef(pold->prop_name));
    }
    return new;
}

/*[clinic input]
property.__init__ as property_init

    fget: object(c_default="NULL") = None
        function to be used for getting an attribute value
    fset: object(c_default="NULL") = None
        function to be used for setting an attribute value
    fdel: object(c_default="NULL") = None
        function to be used for del'ing an attribute
    doc: object(c_default="NULL") = None
        docstring

Property attribute.

Typical use is to define a managed attribute x:

class C(object):
    def getx(self): return self._x
    def setx(self, value): self._x = value
    def delx(self): del self._x
    x = property(getx, setx, delx, "I'm the 'x' property.")

Decorators make defining new properties or modifying existing ones easy:

class C(object):
    @property
    def x(self):
        "I am the 'x' property."
        return self._x
    @x.setter
    def x(self, value):
        self._x = value
    @x.deleter
    def x(self):
        del self._x
[clinic start generated code]*/

static int
property_init_impl(propertyobject *self, PyObject *fget, PyObject *fset,
                   PyObject *fdel, PyObject *doc)
/*[clinic end generated code: output=01a960742b692b57 input=dfb5dbbffc6932d5]*/
{
    if (fget == Py_None)
        fget = NULL;
    if (fset == Py_None)
        fset = NULL;
    if (fdel == Py_None)
        fdel = NULL;

    Py_XSETREF(self->prop_get, Py_XNewRef(fget));
    Py_XSETREF(self->prop_set, Py_XNewRef(fset));
    Py_XSETREF(self->prop_del, Py_XNewRef(fdel));
    Py_XSETREF(self->prop_doc, NULL);
    Py_XSETREF(self->prop_name, NULL);

    self->getter_doc = 0;
    PyObject *prop_doc = NULL;

    if (doc != NULL && doc != Py_None) {
        prop_doc = Py_XNewRef(doc);
    }
    /* if no docstring given and the getter has one, use that one */
    else if (fget != NULL) {
        int rc = _PyObject_LookupAttr(fget, &_Py_ID(__doc__), &prop_doc);
        if (rc <= 0) {
            return rc;
        }
        if (!Py_IS_TYPE(self, &PyProperty_Type) &&
            prop_doc != NULL && prop_doc != Py_None) {
            // This oddity preserves the long existing behavior of surfacing
            // an AttributeError when using a dict-less (__slots__) property
            // subclass as a decorator on a getter method with a docstring.
            // See PropertySubclassTest.test_slots_docstring_copy_exception.
            int err = PyObject_SetAttr(
                        (PyObject *)self, &_Py_ID(__doc__), prop_doc);
            if (err < 0) {
                Py_DECREF(prop_doc);  // release our new reference.
                return -1;
            }
        }
        if (prop_doc == Py_None) {
            prop_doc = NULL;
            Py_DECREF(Py_None);
        }
        if (prop_doc != NULL){
            self->getter_doc = 1;
        }
    }

    /* At this point `prop_doc` is either NULL or
       a non-None object with incremented ref counter */

    if (Py_IS_TYPE(self, &PyProperty_Type)) {
        Py_XSETREF(self->prop_doc, prop_doc);
    } else {
        /* If this is a property subclass, put __doc__ in the dict
           or designated slot of the subclass instance instead, otherwise
           it gets shadowed by __doc__ in the class's dict. */

        if (prop_doc == NULL) {
            prop_doc = Py_NewRef(Py_None);
        }
        int err = PyObject_SetAttr(
                    (PyObject *)self, &_Py_ID(__doc__), prop_doc);
        Py_DECREF(prop_doc);
        if (err < 0) {
            assert(PyErr_Occurred());
            if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
                PyErr_Clear();
                // https://github.com/python/cpython/issues/98963#issuecomment-1574413319
                // Python silently dropped this doc assignment through 3.11.
                // We preserve that behavior for backwards compatibility.
                //
                // If we ever want to deprecate this behavior, only raise a
                // warning or error when proc_doc is not None so that
                // property without a specific doc= still works.
                return 0;
            } else {
                return -1;
            }
        }
    }

    return 0;
}

static PyObject *
property_get___isabstractmethod__(propertyobject *prop, void *closure)
{
    int res = _PyObject_IsAbstract(prop->prop_get);
    if (res == -1) {
        return NULL;
    }
    else if (res) {
        Py_RETURN_TRUE;
    }

    res = _PyObject_IsAbstract(prop->prop_set);
    if (res == -1) {
        return NULL;
    }
    else if (res) {
        Py_RETURN_TRUE;
    }

    res = _PyObject_IsAbstract(prop->prop_del);
    if (res == -1) {
        return NULL;
    }
    else if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyGetSetDef property_getsetlist[] = {
    {"__isabstractmethod__",
     (getter)property_get___isabstractmethod__, NULL,
     NULL,
     NULL},
    {NULL} /* Sentinel */
};

static int
property_traverse(PyObject *self, visitproc visit, void *arg)
{
    propertyobject *pp = (propertyobject *)self;
    Py_VISIT(pp->prop_get);
    Py_VISIT(pp->prop_set);
    Py_VISIT(pp->prop_del);
    Py_VISIT(pp->prop_doc);
    Py_VISIT(pp->prop_name);
    return 0;
}

static int
property_clear(PyObject *self)
{
    propertyobject *pp = (propertyobject *)self;
    Py_CLEAR(pp->prop_doc);
    return 0;
}

#include "clinic/descrobject.c.h"

PyTypeObject PyDictProxy_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "mappingproxy",                             /* tp_name */
    sizeof(mappingproxyobject),                 /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)mappingproxy_dealloc,           /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)mappingproxy_repr,                /* tp_repr */
    &mappingproxy_as_number,                    /* tp_as_number */
    &mappingproxy_as_sequence,                  /* tp_as_sequence */
    &mappingproxy_as_mapping,                   /* tp_as_mapping */
    (hashfunc)mappingproxy_hash,                /* tp_hash */
    0,                                          /* tp_call */
    (reprfunc)mappingproxy_str,                 /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_MAPPING,                     /* tp_flags */
    0,                                          /* tp_doc */
    mappingproxy_traverse,                      /* tp_traverse */
    0,                                          /* tp_clear */
    (richcmpfunc)mappingproxy_richcompare,      /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    (getiterfunc)mappingproxy_getiter,          /* tp_iter */
    0,                                          /* tp_iternext */
    mappingproxy_methods,                       /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    mappingproxy_new,                           /* tp_new */
};

PyTypeObject PyProperty_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "property",                                 /* tp_name */
    sizeof(propertyobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    property_dealloc,                           /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,                    /* tp_flags */
    property_init__doc__,                       /* tp_doc */
    property_traverse,                          /* tp_traverse */
    (inquiry)property_clear,                    /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    property_methods,                           /* tp_methods */
    property_members,                           /* tp_members */
    property_getsetlist,                        /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    property_descr_get,                         /* tp_descr_get */
    property_descr_set,                         /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    property_init,                              /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    PyType_GenericNew,                          /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};
