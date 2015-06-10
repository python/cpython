/* Descriptors -- a new, flexible way to describe attributes */

#include "Python.h"
#include "structmember.h" /* Why is this not included in Python.h? */

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
descr_repr(PyDescrObject *descr, char *format)
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
descr_check(PyDescrObject *descr, PyObject *obj, PyObject **pres)
{
    if (obj == NULL) {
        Py_INCREF(descr);
        *pres = (PyObject *)descr;
        return 1;
    }
    if (!PyObject_TypeCheck(obj, descr->d_type)) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' for '%s' objects "
                     "doesn't apply to '%s' object",
                     descr_name((PyDescrObject *)descr), "?",
                     descr->d_type->tp_name,
                     obj->ob_type->tp_name);
        *pres = NULL;
        return 1;
    }
    return 0;
}

static PyObject *
classmethod_get(PyMethodDescrObject *descr, PyObject *obj, PyObject *type)
{
    /* Ensure a valid type.  Class methods ignore obj. */
    if (type == NULL) {
        if (obj != NULL)
            type = (PyObject *)obj->ob_type;
        else {
            /* Wot - no type?! */
            PyErr_Format(PyExc_TypeError,
                         "descriptor '%V' for type '%s' "
                         "needs either an object or a type",
                         descr_name((PyDescrObject *)descr), "?",
                         PyDescr_TYPE(descr)->tp_name);
            return NULL;
        }
    }
    if (!PyType_Check(type)) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' for type '%s' "
                     "needs a type, not a '%s' as arg 2",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name,
                     type->ob_type->tp_name);
        return NULL;
    }
    if (!PyType_IsSubtype((PyTypeObject *)type, PyDescr_TYPE(descr))) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' for type '%s' "
                     "doesn't apply to type '%s'",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name,
                     ((PyTypeObject *)type)->tp_name);
        return NULL;
    }
    return PyCFunction_NewEx(descr->d_method, type, NULL);
}

static PyObject *
method_get(PyMethodDescrObject *descr, PyObject *obj, PyObject *type)
{
    PyObject *res;

    if (descr_check((PyDescrObject *)descr, obj, &res))
        return res;
    return PyCFunction_NewEx(descr->d_method, obj, NULL);
}

static PyObject *
member_get(PyMemberDescrObject *descr, PyObject *obj, PyObject *type)
{
    PyObject *res;

    if (descr_check((PyDescrObject *)descr, obj, &res))
        return res;
    return PyMember_GetOne((char *)obj, descr->d_member);
}

static PyObject *
getset_get(PyGetSetDescrObject *descr, PyObject *obj, PyObject *type)
{
    PyObject *res;

    if (descr_check((PyDescrObject *)descr, obj, &res))
        return res;
    if (descr->d_getset->get != NULL)
        return descr->d_getset->get(obj, descr->d_getset->closure);
    PyErr_Format(PyExc_AttributeError,
                 "attribute '%V' of '%.100s' objects is not readable",
                 descr_name((PyDescrObject *)descr), "?",
                 PyDescr_TYPE(descr)->tp_name);
    return NULL;
}

static PyObject *
wrapperdescr_get(PyWrapperDescrObject *descr, PyObject *obj, PyObject *type)
{
    PyObject *res;

    if (descr_check((PyDescrObject *)descr, obj, &res))
        return res;
    return PyWrapper_New((PyObject *)descr, obj);
}

static int
descr_setcheck(PyDescrObject *descr, PyObject *obj, PyObject *value,
               int *pres)
{
    assert(obj != NULL);
    if (!PyObject_TypeCheck(obj, descr->d_type)) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' for '%.100s' objects "
                     "doesn't apply to '%.100s' object",
                     descr_name(descr), "?",
                     descr->d_type->tp_name,
                     obj->ob_type->tp_name);
        *pres = -1;
        return 1;
    }
    return 0;
}

static int
member_set(PyMemberDescrObject *descr, PyObject *obj, PyObject *value)
{
    int res;

    if (descr_setcheck((PyDescrObject *)descr, obj, value, &res))
        return res;
    return PyMember_SetOne((char *)obj, descr->d_member, value);
}

static int
getset_set(PyGetSetDescrObject *descr, PyObject *obj, PyObject *value)
{
    int res;

    if (descr_setcheck((PyDescrObject *)descr, obj, value, &res))
        return res;
    if (descr->d_getset->set != NULL)
        return descr->d_getset->set(obj, value,
                                    descr->d_getset->closure);
    PyErr_Format(PyExc_AttributeError,
                 "attribute '%V' of '%.100s' objects is not writable",
                 descr_name((PyDescrObject *)descr), "?",
                 PyDescr_TYPE(descr)->tp_name);
    return -1;
}

static PyObject *
methoddescr_call(PyMethodDescrObject *descr, PyObject *args, PyObject *kwds)
{
    Py_ssize_t argc;
    PyObject *self, *func, *result;

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
                     self->ob_type->tp_name);
        return NULL;
    }

    func = PyCFunction_NewEx(descr->d_method, self, NULL);
    if (func == NULL)
        return NULL;
    args = PyTuple_GetSlice(args, 1, argc);
    if (args == NULL) {
        Py_DECREF(func);
        return NULL;
    }
    result = PyEval_CallObjectWithKeywords(func, args, kwds);
    Py_DECREF(args);
    Py_DECREF(func);
    return result;
}

static PyObject *
classmethoddescr_call(PyMethodDescrObject *descr, PyObject *args,
                      PyObject *kwds)
{
    Py_ssize_t argc;
    PyObject *self, *func, *result;

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
    if (!PyType_Check(self)) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' requires a type "
                     "but received a '%.100s'",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name,
                     self->ob_type->tp_name);
        return NULL;
    }
    if (!PyType_IsSubtype((PyTypeObject *)self, PyDescr_TYPE(descr))) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' "
                     "requires a subtype of '%.100s' "
                     "but received '%.100s",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name,
                     self->ob_type->tp_name);
        return NULL;
    }

    func = PyCFunction_NewEx(descr->d_method, self, NULL);
    if (func == NULL)
        return NULL;
    args = PyTuple_GetSlice(args, 1, argc);
    if (args == NULL) {
        Py_DECREF(func);
        return NULL;
    }
    result = PyEval_CallObjectWithKeywords(func, args, kwds);
    Py_DECREF(func);
    Py_DECREF(args);
    return result;
}

static PyObject *
wrapperdescr_call(PyWrapperDescrObject *descr, PyObject *args, PyObject *kwds)
{
    Py_ssize_t argc;
    PyObject *self, *func, *result;

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
                     self->ob_type->tp_name);
        return NULL;
    }

    func = PyWrapper_New((PyObject *)descr, self);
    if (func == NULL)
        return NULL;
    args = PyTuple_GetSlice(args, 1, argc);
    if (args == NULL) {
        Py_DECREF(func);
        return NULL;
    }
    result = PyEval_CallObjectWithKeywords(func, args, kwds);
    Py_DECREF(args);
    Py_DECREF(func);
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
    _Py_IDENTIFIER(__qualname__);

    if (descr->d_name == NULL || !PyUnicode_Check(descr->d_name)) {
        PyErr_SetString(PyExc_TypeError,
                        "<descriptor>.__name__ is not a unicode object");
        return NULL;
    }

    type_qualname = _PyObject_GetAttrId((PyObject *)descr->d_type,
                                        &PyId___qualname__);
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
descr_get_qualname(PyDescrObject *descr)
{
    if (descr->d_qualname == NULL)
        descr->d_qualname = calculate_qualname(descr);
    Py_XINCREF(descr->d_qualname);
    return descr->d_qualname;
}

static PyObject *
descr_reduce(PyDescrObject *descr)
{
    PyObject *builtins;
    PyObject *getattr;
    _Py_IDENTIFIER(getattr);

    builtins = PyEval_GetBuiltins();
    getattr = _PyDict_GetItemId(builtins, &PyId_getattr);
    return Py_BuildValue("O(OO)", getattr, PyDescr_TYPE(descr),
                         PyDescr_NAME(descr));
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
        Py_INCREF(Py_None);
        return Py_None;
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
        Py_INCREF(Py_None);
        return Py_None;
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
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)method_repr,                      /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    (ternaryfunc)methoddescr_call,              /* tp_call */
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
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
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
    descr_methods,                              /* tp_methods */
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
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
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
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
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
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
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
        Py_XINCREF(type);
        descr->d_type = type;
        descr->d_name = PyUnicode_InternFromString(name);
        if (descr->d_name == NULL) {
            Py_DECREF(descr);
            descr = NULL;
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
    PyMethodDescrObject *descr;

    descr = (PyMethodDescrObject *)descr_new(&PyMethodDescr_Type,
                                             type, method->ml_name);
    if (descr != NULL)
        descr->d_method = method;
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
mappingproxy_get(mappingproxyobject *pp, PyObject *args)
{
    PyObject *key, *def = Py_None;
    _Py_IDENTIFIER(get);

    if (!PyArg_UnpackTuple(args, "get", 1, 2, &key, &def))
        return NULL;
    return _PyObject_CallMethodId(pp->mapping, &PyId_get, "(OO)", key, def);
}

static PyObject *
mappingproxy_keys(mappingproxyobject *pp)
{
    _Py_IDENTIFIER(keys);
    return _PyObject_CallMethodId(pp->mapping, &PyId_keys, NULL);
}

static PyObject *
mappingproxy_values(mappingproxyobject *pp)
{
    _Py_IDENTIFIER(values);
    return _PyObject_CallMethodId(pp->mapping, &PyId_values, NULL);
}

static PyObject *
mappingproxy_items(mappingproxyobject *pp)
{
    _Py_IDENTIFIER(items);
    return _PyObject_CallMethodId(pp->mapping, &PyId_items, NULL);
}

static PyObject *
mappingproxy_copy(mappingproxyobject *pp)
{
    _Py_IDENTIFIER(copy);
    return _PyObject_CallMethodId(pp->mapping, &PyId_copy, NULL);
}

/* WARNING: mappingproxy methods must not give access
            to the underlying mapping */

static PyMethodDef mappingproxy_methods[] = {
    {"get",       (PyCFunction)mappingproxy_get,        METH_VARARGS,
     PyDoc_STR("D.get(k[,d]) -> D[k] if k in D, else d."
               "  d defaults to None.")},
    {"keys",      (PyCFunction)mappingproxy_keys,       METH_NOARGS,
     PyDoc_STR("D.keys() -> list of D's keys")},
    {"values",    (PyCFunction)mappingproxy_values,     METH_NOARGS,
     PyDoc_STR("D.values() -> list of D's values")},
    {"items",     (PyCFunction)mappingproxy_items,      METH_NOARGS,
     PyDoc_STR("D.items() -> list of D's (key, value) pairs, as 2-tuples")},
    {"copy",      (PyCFunction)mappingproxy_copy,       METH_NOARGS,
     PyDoc_STR("D.copy() -> a shallow copy of D")},
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

static PyObject*
mappingproxy_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"mapping", NULL};
    PyObject *mapping;
    mappingproxyobject *mappingproxy;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:mappingproxy",
                                     kwlist, &mapping))
        return NULL;

    if (mappingproxy_check_mapping(mapping) == -1)
        return NULL;

    mappingproxy = PyObject_GC_New(mappingproxyobject, &PyDictProxy_Type);
    if (mappingproxy == NULL)
        return NULL;
    Py_INCREF(mapping);
    mappingproxy->mapping = mapping;
    _PyObject_GC_TRACK(mappingproxy);
    return (PyObject *)mappingproxy;
}

PyTypeObject PyDictProxy_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "mappingproxy",                             /* tp_name */
    sizeof(mappingproxyobject),                 /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)mappingproxy_dealloc,           /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)mappingproxy_repr,                /* tp_repr */
    0,                                          /* tp_as_number */
    &mappingproxy_as_sequence,                  /* tp_as_sequence */
    &mappingproxy_as_mapping,                   /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    (reprfunc)mappingproxy_str,                 /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
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

PyObject *
PyDictProxy_New(PyObject *mapping)
{
    mappingproxyobject *pp;

    if (mappingproxy_check_mapping(mapping) == -1)
        return NULL;

    pp = PyObject_GC_New(mappingproxyobject, &PyDictProxy_Type);
    if (pp != NULL) {
        Py_INCREF(mapping);
        pp->mapping = mapping;
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

#define Wrapper_Check(v) (Py_TYPE(v) == &_PyMethodWrapper_Type)

static void
wrapper_dealloc(wrapperobject *wp)
{
    PyObject_GC_UnTrack(wp);
    Py_TRASHCAN_SAFE_BEGIN(wp)
    Py_XDECREF(wp->descr);
    Py_XDECREF(wp->self);
    PyObject_GC_Del(wp);
    Py_TRASHCAN_SAFE_END(wp)
}

#define TEST_COND(cond) ((cond) ? Py_True : Py_False)

static PyObject *
wrapper_richcompare(PyObject *a, PyObject *b, int op)
{
    Py_intptr_t result;
    PyObject *v;
    PyWrapperDescrObject *a_descr, *b_descr;

    assert(a != NULL && b != NULL);

    /* both arguments should be wrapperobjects */
    if (!Wrapper_Check(a) || !Wrapper_Check(b)) {
        v = Py_NotImplemented;
        Py_INCREF(v);
        return v;
    }

    /* compare by descriptor address; if the descriptors are the same,
       compare by the objects they're bound to */
    a_descr = ((wrapperobject *)a)->descr;
    b_descr = ((wrapperobject *)b)->descr;
    if (a_descr == b_descr) {
        a = ((wrapperobject *)a)->self;
        b = ((wrapperobject *)b)->self;
        return PyObject_RichCompare(a, b, op);
    }

    result = a_descr - b_descr;
    switch (op) {
    case Py_EQ:
        v = TEST_COND(result == 0);
        break;
    case Py_NE:
        v = TEST_COND(result != 0);
        break;
    case Py_LE:
        v = TEST_COND(result <= 0);
        break;
    case Py_GE:
        v = TEST_COND(result >= 0);
        break;
    case Py_LT:
        v = TEST_COND(result < 0);
        break;
    case Py_GT:
        v = TEST_COND(result > 0);
        break;
    default:
        PyErr_BadArgument();
        return NULL;
    }
    Py_INCREF(v);
    return v;
}

static Py_hash_t
wrapper_hash(wrapperobject *wp)
{
    Py_hash_t x, y;
    x = _Py_HashPointer(wp->descr);
    if (x == -1)
        return -1;
    y = PyObject_Hash(wp->self);
    if (y == -1)
        return -1;
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
                               wp->self->ob_type->tp_name,
                               wp->self);
}

static PyObject *
wrapper_reduce(wrapperobject *wp)
{
    PyObject *builtins;
    PyObject *getattr;
    _Py_IDENTIFIER(getattr);

    builtins = PyEval_GetBuiltins();
    getattr = _PyDict_GetItemId(builtins, &PyId_getattr);
    return Py_BuildValue("O(OO)", getattr, wp->self, PyDescr_NAME(wp->descr));
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
wrapper_objclass(wrapperobject *wp)
{
    PyObject *c = (PyObject *)PyDescr_TYPE(wp->descr);

    Py_INCREF(c);
    return c;
}

static PyObject *
wrapper_name(wrapperobject *wp)
{
    const char *s = wp->descr->d_base->name;

    return PyUnicode_FromString(s);
}

static PyObject *
wrapper_doc(wrapperobject *wp, void *closure)
{
    return _PyType_GetDocFromInternalDoc(wp->descr->d_base->name, wp->descr->d_base->doc);
}

static PyObject *
wrapper_text_signature(wrapperobject *wp, void *closure)
{
    return _PyType_GetTextSignatureFromInternalDoc(wp->descr->d_base->name, wp->descr->d_base->doc);
}

static PyObject *
wrapper_qualname(wrapperobject *wp)
{
    return descr_get_qualname((PyDescrObject *)wp->descr);
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
    wrapperfunc wrapper = wp->descr->d_base->wrapper;
    PyObject *self = wp->self;

    if (wp->descr->d_base->flags & PyWrapperFlag_KEYWORDS) {
        wrapperfunc_kwds wk = (wrapperfunc_kwds)wrapper;
        return (*wk)(self, args, wp->descr->d_wrapped, kwds);
    }

    if (kwds != NULL && (!PyDict_Check(kwds) || PyDict_Size(kwds) != 0)) {
        PyErr_Format(PyExc_TypeError,
                     "wrapper %s doesn't take keyword arguments",
                     wp->descr->d_base->name);
        return NULL;
    }
    return (*wrapper)(self, args, wp->descr->d_wrapped);
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
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
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
        Py_INCREF(descr);
        wp->descr = descr;
        Py_INCREF(self);
        wp->self = self;
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
        self.__doc__ = doc

    def __get__(self, inst, type=None):
        if inst is None:
            return self
        if self.__get is None:
            raise AttributeError, "unreadable attribute"
        return self.__get(inst)

    def __set__(self, inst, value):
        if self.__set is None:
            raise AttributeError, "can't set attribute"
        return self.__set(inst, value)

    def __delete__(self, inst):
        if self.__del is None:
            raise AttributeError, "can't delete attribute"
        return self.__del(inst)

*/

typedef struct {
    PyObject_HEAD
    PyObject *prop_get;
    PyObject *prop_set;
    PyObject *prop_del;
    PyObject *prop_doc;
    int getter_doc;
} propertyobject;

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
             "Descriptor to change the getter on a property.");

static PyObject *
property_getter(PyObject *self, PyObject *getter)
{
    return property_copy(self, getter, NULL, NULL);
}


PyDoc_STRVAR(setter_doc,
             "Descriptor to change the setter on a property.");

static PyObject *
property_setter(PyObject *self, PyObject *setter)
{
    return property_copy(self, NULL, setter, NULL);
}


PyDoc_STRVAR(deleter_doc,
             "Descriptor to change the deleter on a property.");

static PyObject *
property_deleter(PyObject *self, PyObject *deleter)
{
    return property_copy(self, NULL, NULL, deleter);
}


static PyMethodDef property_methods[] = {
    {"getter", property_getter, METH_O, getter_doc},
    {"setter", property_setter, METH_O, setter_doc},
    {"deleter", property_deleter, METH_O, deleter_doc},
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
    self->ob_type->tp_free(self);
}

static PyObject *
property_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    static PyObject * volatile cached_args = NULL;
    PyObject *args;
    PyObject *ret;
    propertyobject *gs = (propertyobject *)self;

    if (obj == NULL || obj == Py_None) {
        Py_INCREF(self);
        return self;
    }
    if (gs->prop_get == NULL) {
        PyErr_SetString(PyExc_AttributeError, "unreadable attribute");
        return NULL;
    }
    args = cached_args;
    if (!args || Py_REFCNT(args) != 1) {
        Py_CLEAR(cached_args);
        if (!(cached_args = args = PyTuple_New(1)))
            return NULL;
    }
    Py_INCREF(args);
    assert (Py_REFCNT(args) == 2);
    Py_INCREF(obj);
    PyTuple_SET_ITEM(args, 0, obj);
    ret = PyObject_Call(gs->prop_get, args, NULL);
    if (args == cached_args) {
        if (Py_REFCNT(args) == 2) {
            obj = PyTuple_GET_ITEM(args, 0);
            PyTuple_SET_ITEM(args, 0, NULL);
            Py_XDECREF(obj);
        }
        else {
            Py_CLEAR(cached_args);
        }
    }
    Py_DECREF(args);
    return ret;
}

static int
property_descr_set(PyObject *self, PyObject *obj, PyObject *value)
{
    propertyobject *gs = (propertyobject *)self;
    PyObject *func, *res;

    if (value == NULL)
        func = gs->prop_del;
    else
        func = gs->prop_set;
    if (func == NULL) {
        PyErr_SetString(PyExc_AttributeError,
                        value == NULL ?
                        "can't delete attribute" :
                "can't set attribute");
        return -1;
    }
    if (value == NULL)
        res = PyObject_CallFunctionObjArgs(func, obj, NULL);
    else
        res = PyObject_CallFunctionObjArgs(func, obj, value, NULL);
    if (res == NULL)
        return -1;
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
        Py_XDECREF(get);
        get = pold->prop_get ? pold->prop_get : Py_None;
    }
    if (set == NULL || set == Py_None) {
        Py_XDECREF(set);
        set = pold->prop_set ? pold->prop_set : Py_None;
    }
    if (del == NULL || del == Py_None) {
        Py_XDECREF(del);
        del = pold->prop_del ? pold->prop_del : Py_None;
    }
    if (pold->getter_doc && get != Py_None) {
        /* make _init use __doc__ from getter */
        doc = Py_None;
    }
    else {
        doc = pold->prop_doc ? pold->prop_doc : Py_None;
    }

    new =  PyObject_CallFunction(type, "OOOO", get, set, del, doc);
    Py_DECREF(type);
    if (new == NULL)
        return NULL;
    return new;
}

static int
property_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *get = NULL, *set = NULL, *del = NULL, *doc = NULL;
    static char *kwlist[] = {"fget", "fset", "fdel", "doc", 0};
    propertyobject *prop = (propertyobject *)self;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOOO:property",
                                     kwlist, &get, &set, &del, &doc))
        return -1;

    if (get == Py_None)
        get = NULL;
    if (set == Py_None)
        set = NULL;
    if (del == Py_None)
        del = NULL;

    Py_XINCREF(get);
    Py_XINCREF(set);
    Py_XINCREF(del);
    Py_XINCREF(doc);

    prop->prop_get = get;
    prop->prop_set = set;
    prop->prop_del = del;
    prop->prop_doc = doc;
    prop->getter_doc = 0;

    /* if no docstring given and the getter has one, use that one */
    if ((doc == NULL || doc == Py_None) && get != NULL) {
        _Py_IDENTIFIER(__doc__);
        PyObject *get_doc = _PyObject_GetAttrId(get, &PyId___doc__);
        if (get_doc) {
            if (Py_TYPE(self) == &PyProperty_Type) {
                Py_XDECREF(prop->prop_doc);
                prop->prop_doc = get_doc;
            }
            else {
                /* If this is a property subclass, put __doc__
                in dict of the subclass instance instead,
                otherwise it gets shadowed by __doc__ in the
                class's dict. */
                int err = _PyObject_SetAttrId(self, &PyId___doc__, get_doc);
                Py_DECREF(get_doc);
                if (err < 0)
                    return -1;
            }
            prop->getter_doc = 1;
        }
        else if (PyErr_ExceptionMatches(PyExc_Exception)) {
            PyErr_Clear();
        }
        else {
            return -1;
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

PyDoc_STRVAR(property_doc,
"property(fget=None, fset=None, fdel=None, doc=None) -> property attribute\n"
"\n"
"fget is a function to be used for getting an attribute value, and likewise\n"
"fset is a function for setting, and fdel a function for del'ing, an\n"
"attribute.  Typical use is to define a managed attribute x:\n\n"
"class C(object):\n"
"    def getx(self): return self._x\n"
"    def setx(self, value): self._x = value\n"
"    def delx(self): del self._x\n"
"    x = property(getx, setx, delx, \"I'm the 'x' property.\")\n"
"\n"
"Decorators make defining new properties or modifying existing ones easy:\n\n"
"class C(object):\n"
"    @property\n"
"    def x(self):\n"
"        \"I am the 'x' property.\"\n"
"        return self._x\n"
"    @x.setter\n"
"    def x(self, value):\n"
"        self._x = value\n"
"    @x.deleter\n"
"    def x(self):\n"
"        del self._x\n"
);

static int
property_traverse(PyObject *self, visitproc visit, void *arg)
{
    propertyobject *pp = (propertyobject *)self;
    Py_VISIT(pp->prop_get);
    Py_VISIT(pp->prop_set);
    Py_VISIT(pp->prop_del);
    Py_VISIT(pp->prop_doc);
    return 0;
}

static int
property_clear(PyObject *self)
{
    propertyobject *pp = (propertyobject *)self;
    Py_CLEAR(pp->prop_doc);
    return 0;
}

PyTypeObject PyProperty_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "property",                                 /* tp_name */
    sizeof(propertyobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    property_dealloc,                           /* tp_dealloc */
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
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,                    /* tp_flags */
    property_doc,                               /* tp_doc */
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
