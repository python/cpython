/*[clinic input]
preserve
[clinic start generated code]*/

static PyObject *
mappingproxy_new_impl(PyTypeObject *type, PyObject *mapping);

static PyObject *
mappingproxy_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"mapping", NULL};
    static _PyArg_Parser _parser = {"O:mappingproxy", _keywords, 0};
    PyObject *mapping;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &mapping)) {
        goto exit;
    }
    return_value = mappingproxy_new_impl(type, mapping);

exit:
    return return_value;
}

PyDoc_STRVAR(property_init__doc__,
"property(fget=None, fset=None, fdel=None, doc=None)\n"
"--\n"
"\n"
"Property attribute.\n"
"\n"
"  fget\n"
"    function to be used for getting an attribute value\n"
"  fset\n"
"    function to be used for setting an attribute value\n"
"  fdel\n"
"    function to be used for del\'ing an attribute\n"
"  doc\n"
"    docstring\n"
"\n"
"Typical use is to define a managed attribute x:\n"
"\n"
"class C(object):\n"
"    def getx(self): return self._x\n"
"    def setx(self, value): self._x = value\n"
"    def delx(self): del self._x\n"
"    x = property(getx, setx, delx, \"I\'m the \'x\' property.\")\n"
"\n"
"Decorators make defining new properties or modifying existing ones easy:\n"
"\n"
"class C(object):\n"
"    @property\n"
"    def x(self):\n"
"        \"I am the \'x\' property.\"\n"
"        return self._x\n"
"    @x.setter\n"
"    def x(self, value):\n"
"        self._x = value\n"
"    @x.deleter\n"
"    def x(self):\n"
"        del self._x");

static int
property_init_impl(propertyobject *self, PyObject *fget, PyObject *fset,
                   PyObject *fdel, PyObject *doc);

static int
property_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"fget", "fset", "fdel", "doc", NULL};
    static _PyArg_Parser _parser = {"|OOOO:property", _keywords, 0};
    PyObject *fget = NULL;
    PyObject *fset = NULL;
    PyObject *fdel = NULL;
    PyObject *doc = NULL;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &fget, &fset, &fdel, &doc)) {
        goto exit;
    }
    return_value = property_init_impl((propertyobject *)self, fget, fset, fdel, doc);

exit:
    return return_value;
}
/*[clinic end generated code: output=729021fa9cdc46be input=a9049054013a1b77]*/
