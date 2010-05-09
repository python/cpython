/* Example of embedding Python in another program */

#include "Python.h"

PyObject* PyInit_xyzzy(void); /* Forward */

main(int argc, char **argv)
{
    /* Ignore passed-in argc/argv. If desired, conversion
       should use mbstowcs to convert them. */
    wchar_t *args[] = {L"embed", L"hello", 0};

    /* Pass argv[0] to the Python interpreter */
    Py_SetProgramName(args[0]);

    /* Add a static module */
    PyImport_AppendInittab("xyzzy", PyInit_xyzzy);

    /* Initialize the Python interpreter.  Required. */
    Py_Initialize();

    /* Define sys.argv.  It is up to the application if you
       want this; you can also let it undefined (since the Python
       code is generally not a main program it has no business
       touching sys.argv...) */
    PySys_SetArgv(2, args);

    /* Do some application specific code */
    printf("Hello, brave new world\n\n");

    /* Execute some Python statements (in module __main__) */
    PyRun_SimpleString("import sys\n");
    PyRun_SimpleString("print(sys.builtin_module_names)\n");
    PyRun_SimpleString("print(sys.modules.keys())\n");
    PyRun_SimpleString("print(sys.executable)\n");
    PyRun_SimpleString("print(sys.argv)\n");

    /* Note that you can call any public function of the Python
       interpreter here, e.g. call_object(). */

    /* Some more application specific code */
    printf("\nGoodbye, cruel world\n");

    /* Exit, cleaning up the interpreter */
    Py_Exit(0);
    /*NOTREACHED*/
}

/* A static module */

/* 'self' is not used */
static PyObject *
xyzzy_foo(PyObject *self, PyObject* args)
{
    return PyLong_FromLong(42L);
}

static PyMethodDef xyzzy_methods[] = {
    {"foo",             xyzzy_foo,      METH_NOARGS,
     "Return the meaning of everything."},
    {NULL,              NULL}           /* sentinel */
};

static struct PyModuleDef xyzzymodule = {
    {}, /* m_base */
    "xyzzy",  /* m_name */
    0,  /* m_doc */
    0,  /* m_size */
    xyzzy_methods,  /* m_methods */
    0,  /* m_reload */
    0,  /* m_traverse */
    0,  /* m_clear */
    0,  /* m_free */
};

PyObject*
PyInit_xyzzy(void)
{
    return PyModule_Create(&xyzzymodule);
}
