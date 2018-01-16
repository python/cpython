/************************************************
 * VxWorks Compatibility Wrapper
 *
 * Python interface to vxWorks methods
 *
 * Author: Oscar Shi (co-op winter2018)
 *
 * modification history
 * --------------------
 *  12jan18     created
 *
 ************************************************/

#if defined(__VXWORKS__)
#include "Python.h"
#include <rtpLib.h>
#include "clinic/_vxwapi.c.h"

// Largely copied from
// https://github.com/python-ldap/python-ldap/blob/master/Modules/LDAPObject.c#L276
static void
free_attrs( char*** attrsp ) {
    char **attrs = *attrsp;
    char **p;

    if (attrs == NULL)
        return;

    *attrsp = NULL;
    for (p = attrs; *p != NULL; p++) {
            PyMem_DEL(*p);
        }
    PyMem_DEL(attrs);
}

int
cStrings_from_PyList( PyObject *attrlist, char***attrsp ) {

    char **attrs = NULL;
    PyObject *seq = NULL;

    if (attrlist == Py_None) {
        /* None means a NULL attrlist */
#if PY_MAJOR_VERSION == 2
    } else if (PyBytes_Check(attrlist)) {
#else
    } else if (PyUnicode_Check(attrlist)) {
#endif
        /* caught by John Benninghoff <johnb@netscape.com> */
        goto error;
    } else {
        PyObject *item = NULL;
        Py_ssize_t i, len, strlen;
#if PY_MAJOR_VERSION >= 3
        const char *str;
#else
        char *str;
#endif

        seq = PySequence_Fast(attrlist, "expected list of strings or None");
        if (seq == NULL)
            goto error;

        len = PySequence_Length(attrlist);

        attrs = PyMem_NEW(char *, len + 1);
        if (attrs == NULL)
            goto nomem;

        for (i = 0; i < len; i++) {
            attrs[i] = NULL;
            item = PySequence_Fast_GET_ITEM(seq, i);
            if (item == NULL)
                goto error;
#if PY_MAJOR_VERSION == 2
            /* Encoded in Python to UTF-8 */
            if (!PyBytes_Check(item)) {
                goto error;
            }
            if (PyBytes_AsStringAndSize(item, &str, &strlen) == -1) {
                goto error;
            }
#else
            if (!PyUnicode_Check(item)) {
                goto error;
            }
            str = PyUnicode_AsUTF8AndSize(item, &strlen);
#endif
            /* Make a copy. PyBytes_AsString* /
             * PyUnicode_AsUTF8* return
             *              * internal values that must
             *              be treated like const char.
             *              Python
             *                           * 3.7 actually
             *                           returns a const
             *                           char.
             *                                        */
            attrs[i] = (char *)PyMem_NEW(char *, strlen + 1);
            if (attrs[i] == NULL)
                goto nomem;
            memcpy(attrs[i], str, strlen + 1);
        }
        attrs[len] = NULL;
        Py_DECREF(seq);
    }

    *attrsp = attrs;
    return 1;

nomem:
    PyErr_NoMemory();
error:
    Py_XDECREF(seq);
    free_attrs(&attrs);
    return 0;
}



/*
 * RTP_ID rtpSpawn
 *    (
 *      const char  *rtpFileName,    Null terminated path to executable 
 *      const char  *argv[],         Pointer to NULL terminated argv array 
 *      const char  *envp[],         Pointer to NULL terminated envp array 
 *      int         priority,        Priority of initial task 
 *      size_t      uStackSize,      User space stack size for initial task 
 *      int         options,         The options passed to the RTP 
 *      int         taskOptions      Task options for the RTPs initial task 
 *    )
 *
 */


/*[clinic input]
  module _vxwapi
  [clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6efcf3b26a262ef1]*/

/*[clinic input]
  _vxwapi.rtpSpawn

rtpFileName: str
argv: object(subclass_of='&PyList_Type')
envp: object(subclass_of='&PyList_Type')
priority: int
uStackSize: unsigned_int(bitwise=True)
options: int
taskOptions: int
/

Spawn a real time process in the vxWorks OS
[clinic start generated code]*/

static PyObject *
_vxwapi_rtpSpawn_impl(PyObject *module, const char *rtpFileName,
        PyObject *argv, PyObject *envp, int priority,
        unsigned int uStackSize, int options, int taskOptions)
    /*[clinic end generated code output=4a3c98870a33cf6a input=86238fe5131c82ba]*/
{
    char*** argvp = NULL;
    char*** envpp = NULL;
    if(!cStrings_from_PyList(argv,argvp))
        goto error;
    if(!cStrings_from_PyList(envp,envpp))
        goto error;
    int PID = (int) rtpSpawn(rtpFileName,(const char ** ) *argvp, 
               (const char **) *envpp, priority, uStackSize, options, taskOptions);
    if(PID == RTP_ID_ERROR) 
        goto error;
    return Py_BuildValue("i", PID);


    //return PyUnicode_FromString(((char**) argv)[0]);
    // return PyLong_FromSize_t( rtpSpawn(rtpFileName, (const char**)argv,
    //         (const char**) envp, priority, uStackSize, options, taskOptions));
error:
    return NULL;
}
static PyMethodDef _vxwapiMethods[] = {
    _VXWAPI_RTPSPAWN_METHODDEF  
    { NULL, NULL }
};

static struct PyModuleDef _vxwapimodule = {
    PyModuleDef_HEAD_INIT,
    "_vxwapi",
    NULL,
    -1,
    _vxwapiMethods
};

PyMODINIT_FUNC
PyInit__vxwapi(void){
    return PyModule_Create(&_vxwapimodule);
}





#endif           
