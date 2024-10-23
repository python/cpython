/* Interpolation object implementation */
#include "Python.h"
#include <stddef.h>

#include "pycore_initconfig.h"  // _PyStatus_OK
#include "pycore_typeobject.h"  // _PyStaticType_InitBuiltin
#include "pycore_stackref.h"    // _PyStackRef
#include "pycore_object.h"      // _PyObject_GC_TRACK

#include "pycore_template.h"

typedef struct {
    PyObject_HEAD
    PyObject *args;
} templateobject;

static templateobject *
template_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    templateobject *self = (templateobject *) type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }

    static char *kwlist[] = {"args", NULL};
    PyObject *selfargs;

    if (PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist,
                                    &selfargs) < 0) {
        Py_DECREF(self);
        return NULL;
    }

    Py_XSETREF(self->args, Py_NewRef(selfargs));
    return self;
}

static void
template_dealloc(templateobject *self)
{
    Py_CLEAR(self->args);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
template_repr(templateobject *self)
{
    return PyUnicode_FromFormat("%s(%R)",
                                _PyType_Name(Py_TYPE(self)),
                                self->args);
}

static PyObject *
template_compare(templateobject *self, PyObject *other, int op)
{
    if (op == Py_LT || op == Py_LE || op == Py_GT || op == Py_GE) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (!PyObject_TypeCheck(other, &_PyTemplate_Type)) {
        return (op == Py_EQ) ? Py_False : Py_True;
    }

    return PyObject_RichCompare(self->args, ((templateobject *) other)->args, op);
}

static PyObject *
template_add_template_str(templateobject *template, PyUnicodeObject *str, int templateleft)
{
    Py_ssize_t templatesize = PyTuple_GET_SIZE(template->args);

    PyObject *tuple = PyTuple_New(templatesize + 1);
    if (!tuple) {
        return NULL;
    }

    Py_ssize_t i = 0;
    Py_ssize_t j = 0;
    if (!templateleft) {
        PyTuple_SET_ITEM(tuple, i++, Py_NewRef(str));
    }
    for (j = 0; j < templatesize; j++) {
        PyTuple_SET_ITEM(tuple, i + j, Py_NewRef(PyTuple_GET_ITEM(template->args, j)));
    }
    if (templateleft) {
        PyTuple_SET_ITEM(tuple, i + j, Py_NewRef(str));
    }

    PyObject *newtemplate = PyObject_CallOneArg((PyObject *) &_PyTemplate_Type, tuple);
    Py_DECREF(tuple);
    return newtemplate;
}

static PyObject *
template_add_templates(templateobject *self, templateobject *other)
{
    Py_ssize_t selfsize = PyTuple_GET_SIZE(self->args);
    Py_ssize_t othersize = PyTuple_GET_SIZE(other->args);

    PyObject *tuple = PyTuple_New(selfsize + othersize);
    if (!tuple) {
        return NULL;
    }

    Py_ssize_t i;
    for (i = 0; i < selfsize; i++) {
        PyTuple_SET_ITEM(tuple, i, Py_NewRef(PyTuple_GET_ITEM(self->args, i)));
    }
    for (Py_ssize_t j = 0; j < othersize; j++) {
        PyTuple_SET_ITEM(tuple, i + j, Py_NewRef(PyTuple_GET_ITEM(other->args, j)));
    }

    PyObject *newtemplate = PyObject_CallOneArg((PyObject *) &_PyTemplate_Type, tuple);
    Py_DECREF(tuple);
    return newtemplate;
}

static PyObject *
template_add(PyObject *self, PyObject *other)
{
    if (PyObject_TypeCheck(self, &_PyTemplate_Type) &&
            PyObject_TypeCheck(other, &_PyTemplate_Type)) {
        return template_add_templates((templateobject *) self, (templateobject *) other);
    }
    else if (PyObject_TypeCheck(self, &_PyTemplate_Type) && PyUnicode_Check(other)) {
        return template_add_template_str((templateobject *) self, (PyUnicodeObject *) other, 1);
    }
    else if (PyUnicode_Check(self) && PyObject_TypeCheck(other, &_PyTemplate_Type)) {
        return template_add_template_str((templateobject *) other, (PyUnicodeObject *) self, 0);
    }
    else {
        Py_RETURN_NOTIMPLEMENTED;
    }
}

static PyMemberDef template_members[] = {
    {"args", Py_T_OBJECT_EX, offsetof(templateobject, args), Py_READONLY, "Args"},
    {NULL}
};

static PyNumberMethods template_as_number = {
    .nb_add = template_add,
};

PyTypeObject _PyTemplate_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "templatelib.Template",
    .tp_doc = PyDoc_STR("Template object"),
    .tp_basicsize = sizeof(templateobject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | _Py_TPFLAGS_MATCH_SELF,
    .tp_as_number = &template_as_number,
    .tp_new = (newfunc) template_new,
    .tp_dealloc = (destructor) template_dealloc,
    .tp_repr = (reprfunc) template_repr,
    .tp_richcompare = (richcmpfunc) template_compare,
    .tp_members = template_members,
};

PyObject *
_PyTemplate_Create(PyObject **values, Py_ssize_t oparg)
{
    PyObject *tuple = PyTuple_New(oparg);
    if (!tuple) {
        return NULL;
    }

    for (Py_ssize_t i = 0; i < oparg; i++) {
        PyTuple_SET_ITEM(tuple, i, Py_NewRef(values[i]));
    }

    PyObject *template = PyObject_CallOneArg((PyObject *) &_PyTemplate_Type, tuple);
    Py_DECREF(tuple);
    return template;
}
