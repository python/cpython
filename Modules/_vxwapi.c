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
#include "oscar.h"            
// TODO use python Clinic

static PyObject * oscar_wrapper(PyObject * self, PyObject * args){
    return PyUnicode_FromString(oscar());
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

static PyObject * rtpSpawn_wrapper(PyObject * self, PyObject * args){
    char * rtpFileName;
    PyObject *argv;
    PyObject *envp;
    int priority;
    size_t uStackSize;
    int options;
    int taskOptions;
    if(!PyArg_ParseTuple(args,""))
}

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
/*[clinic end generated code: output=4a3c98870a33cf6a input=86238fe5131c82ba]*/

static PyMethodDef OscarMethods[] = {
    { "oscar", oscar_wrapper, METH_VARARGS, "Is this documentation?" },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef oscarmodule = {
    PyModuleDef_HEAD_INIT,
    "oscar",
    NULL,
    -1,
    OscarMethods
};

PyMODINIT_FUNC
PyInit_oscar(void){
    return PyModule_Create(&oscarmodule);
}





#endif           
