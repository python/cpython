/* obsolete -- remove this file! */

#include "Python.h"

static  
PyMethodDef ucnhash_methods[] =
{   
    {NULL, NULL},
};

static char *ucnhash_docstring = "ucnhash hash function module (obsolete)";

DL_EXPORT(void) 
initucnhash(void)
{
    Py_InitModule4(
        "ucnhash", /* Module name */
        ucnhash_methods, /* Method list */
        ucnhash_docstring, /* Module doc-string */
        (PyObject *)NULL, /* always pass this as *self */
        PYTHON_API_VERSION); /* API Version */
}
