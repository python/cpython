"""framer's C code templates.

Templates use the following variables:

FileName: name of the file that contains the C source code
ModuleName: name of the module, as in "import ModuleName"
ModuleDocstring: C string containing the module doc string
"""

module_start = '#include "Python.h"'
member_include = '#include "structmember.h"'

module_doc = """\
PyDoc_STRVAR(%(ModuleName)s_doc,
%(ModuleDocstring)s);
"""

methoddef_start = """\
static struct PyMethodDef %(MethodDefName)s[] = {"""

methoddef_def = """\
        {"%(PythonName)s", (PyCFunction)%(CName)s, %(MethType)s},"""

methoddef_def_doc = """\
        {"%(PythonName)s", (PyCFunction)%(CName)s, %(MethType)s,
         %(DocstringVar)s},"""

methoddef_end = """\
        {NULL, NULL}
};
"""

memberdef_start = """\
#define OFF(X) offsetof(%(StructName)s, X)

static struct PyMemberDef %(MemberDefName)s[] = {"""

memberdef_def_doc = """\
        {"%(PythonName)s", %(Type)s, OFF(%(CName)s), %(Flags)s,
         %(Docstring)s},"""

memberdef_def = """\
        {"%(PythonName)s", %(Type)s, OFF(%(CName)s), %(Flags)s},"""

memberdef_end = """\
        {NULL}
};

#undef OFF
"""

dealloc_func = """static void
%(name)s(PyObject *ob)
{
}
"""

docstring = """\
PyDoc_STRVAR(%(DocstringVar)s,
%(Docstring)s);
"""

funcdef_start = """\
static PyObject *
%(name)s(%(args)s)
{"""

funcdef_end = """\
}
"""

varargs = """\
        if (!PyArg_ParseTuple(args, \"%(ArgParse)s:%(PythonName)s\",
                              %(ArgTargets)s))
                return NULL;"""

module_init_start = """\
PyMODINIT_FUNC
init%(ModuleName)s(void)
{
        PyObject *mod;

        mod = Py_InitModule3("%(ModuleName)s", %(MethodDefName)s,
                             %(ModuleName)s_doc);
        if (mod == NULL)
                return;
"""

type_init_type = "        %(CTypeName)s.ob_type = &PyType_Type;"
module_add_type = """\
        if (!PyObject_SetAttrString(mod, "%(TypeName)s",
                                    (PyObject *)&%(CTypeName)s))
                return;
"""

type_struct_start = """\
static PyTypeObject %(CTypeName)s = {
        PyObject_HEAD_INIT(0)"""

type_struct_end = """\
};
"""
