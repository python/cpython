/*
** This module is a one-trick pony: given an FSSpec it gets the aeut
** resources. It was written by Donovan Preston and slightly modified
** by Jack.
**
** It should be considered a placeholder, it will probably be replaced
** by a full interface to OpenScripting.
*/
#include "Python.h"
#include "pymactoolbox.h"

#include <Carbon/Carbon.h>

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
	
	defaultComponent = OpenDefaultComponent (kOSAComponentType, 'ascr');
	err = GetComponentInstanceError (defaultComponent);
	if (err) return PyMac_Error(err);
	err = OSAGetAppTerminology (
    	defaultComponent, 
    	modeFlags,
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
	FSSpec fss;
	ComponentInstance defaultComponent = NULL;
	SInt16 defaultTerminology = 0;
	Boolean didLaunch = 0;
	OSAError err;
	long modeFlags = 0;
	
	if (!PyArg_ParseTuple(args, "O&|i", PyMac_GetFSSpec, &fss, &modeFlags))
		 return NULL;
	
	defaultComponent = OpenDefaultComponent (kOSAComponentType, 'ascr');
	err = GetComponentInstanceError (defaultComponent);
	if (err) return PyMac_Error(err);
	err = OSAGetAppTerminology (
    	defaultComponent, 
    	modeFlags,
    	&fss, 
    	defaultTerminology, 
    	&didLaunch, 
    	&theDesc
	);
	if (err) return PyMac_Error(err);
	return Py_BuildValue("O&i", AEDesc_New, &theDesc, didLaunch);
}

/* 
 * List of methods defined in the module
 */
static struct PyMethodDef OSATerminology_methods[] =
{
  	{"GetAppTerminology", 
		(PyCFunction) PyOSA_GetAppTerminology,
		METH_VARARGS,
		"Get an applications terminology, as an AEDesc object."},
  	{"GetSysTerminology", 
		(PyCFunction) PyOSA_GetSysTerminology,
		METH_VARARGS,
		"Get an applications system terminology, as an AEDesc object."},
	{NULL, (PyCFunction) NULL, 0, NULL}
};


void
initOSATerminology(void)
{
	Py_InitModule("OSATerminology", OSATerminology_methods);
}