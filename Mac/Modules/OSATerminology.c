#include <Python/Python.h>
#include <Carbon/Carbon.h>
#include <Python/pymactoolbox.h>

PyObject * PyOSA_GetAppTerminology(PyObject* self, PyObject* args) {
	AEDesc temp;
	FSSpec tempFSSpec;
	
	ComponentInstance defaultComponent;
	ScriptingComponentSelector theType;
	SInt16 defaultTerminology;
	Boolean didLaunch;
	
	OSAError err;
	
	PyObject * returnVal;
	
	if (!PyArg_ParseTuple(args, "O&", PyMac_GetFSSpec, &tempFSSpec)) return NULL;
	
  defaultComponent = OpenDefaultComponent (kOSAComponentType,
					      'ascr');
//					      kOSAGenericScriptingComponentSubtype);

  err = GetComponentInstanceError (defaultComponent);
	printf("OpenDefaultComponent: %d\n", err);
	err = OSAGetCurrentDialect(defaultComponent, &defaultTerminology);
	printf("getcurrentdialect: %d\n", err);
	err = OSAGetAppTerminology (
    	defaultComponent, 
    	kOSAModeNull, 
    	&tempFSSpec, 
    	defaultTerminology, 
    	&didLaunch, 
    	&temp
	);

/*	err = ASGetAppTerminology (
    	defaultComponent, 
    	&tempFSSpec, 
    	defaultTerminology, 
    	&didLaunch, 
    	&temp
	);*/

	printf("getappterminology: %d\n", err);

	returnVal = Py_BuildValue("O&i",
			AEDesc_New, &temp, didLaunch);
	return returnVal;
}

/* 
 * List of methods defined in the module
 */
static struct PyMethodDef PyOSA_methods[] =
{
  {"GetAppTerminology", 
   (PyCFunction) PyOSA_GetAppTerminology,
   METH_VARARGS,
   "Get an applications terminology, as an AEDesc object."},

  {NULL, (PyCFunction) NULL, 0, NULL}
};


void
initPyOSA(void)
{
	Py_InitModule("PyOSA", PyOSA_methods);
}