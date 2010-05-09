/*
** An interface to the application scripting related functions of the OSA API.
**
** GetAppTerminology - given an FSSpec/posix path to an application,
**                         returns its aevt (scripting terminology) resource(s)
**
** GetSysTerminology - returns the AppleScript language component's
**                         aeut (scripting terminology) resource
**
** Written by Donovan Preston and slightly modified by Jack and HAS.
*/
#include "Python.h"
#include "pymactoolbox.h"

#include <Carbon/Carbon.h>

#ifndef __LP64__
static PyObject *
PyOSA_GetAppTerminology(PyObject* self, PyObject* args)
{
    AEDesc theDesc = {0,0};
    FSSpec fss;
    ComponentInstance defaultComponent = NULL;
    SInt16 defaultTerminology = 0;
    Boolean didLaunch = 0;
    OSAError err;
    long modeFlags = 0;

    if (!PyArg_ParseTuple(args, "O&|i", PyMac_GetFSSpec, &fss, &modeFlags))
             return NULL;

    /*
    ** Note that we have to use the AppleScript component here. Who knows why
    ** OSAGetAppTerminology should require a scripting component in the
    ** first place, but it does. Note: doesn't work with the generic scripting
    ** component, which is unfortunate as the AS component is currently very
    ** slow (~1 sec?) to load, but we just have to live with this.
    */
    defaultComponent = OpenDefaultComponent (kOSAComponentType, 'ascr');
    err = GetComponentInstanceError (defaultComponent);
    if (err) return PyMac_Error(err);
    err = OSAGetAppTerminology (
    defaultComponent,
    kOSAModeNull,
    &fss,
    defaultTerminology,
    &didLaunch,
    &theDesc
    );
    if (err) return PyMac_Error(err);
    return Py_BuildValue("O&i", AEDesc_New, &theDesc, didLaunch);
}

static PyObject *
PyOSA_GetSysTerminology(PyObject* self, PyObject* args)
{
    AEDesc theDesc = {0,0};
    ComponentInstance defaultComponent = NULL;
    SInt16 defaultTerminology = 0;
    OSAError err;

    /* Accept any args for sake of backwards compatibility, then ignore them. */

    defaultComponent = OpenDefaultComponent (kOSAComponentType, 'ascr');
    err = GetComponentInstanceError (defaultComponent);
    if (err) return PyMac_Error(err);
    err = OSAGetSysTerminology (
    defaultComponent,
    kOSAModeNull,
    defaultTerminology,
    &theDesc
    );
    if (err) return PyMac_Error(err);
    return Py_BuildValue("O&", AEDesc_New, &theDesc);
}
#endif /* !__LP64__ */

/*
 * List of methods defined in the module
 */
static struct PyMethodDef OSATerminology_methods[] =
{
#ifndef __LP64__
    {"GetAppTerminology",
        (PyCFunction) PyOSA_GetAppTerminology,
        METH_VARARGS,
        "Get an application's terminology. GetAppTerminology(path) --> AEDesc"},
    {"GetSysTerminology",
        (PyCFunction) PyOSA_GetSysTerminology,
        METH_VARARGS,
        "Get the AppleScript language's terminology. GetSysTerminology() --> AEDesc"},
#endif /* !__LP64__ */
    {NULL, (PyCFunction) NULL, 0, NULL}
};

void
initOSATerminology(void)
{
    if (PyErr_WarnPy3k("In 3.x, the OSATerminology module is removed.", 1) < 0)
        return;
    Py_InitModule("OSATerminology", OSATerminology_methods);
}
