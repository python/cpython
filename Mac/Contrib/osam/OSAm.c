/*
 *
 * This is a simple module to allow the 
 * user to compile and execute an applescript
 * which is passed in as a text item.
 *
 *  Sean Hummel <seanh@prognet.com>
 *  1/20/98
 *  RealNetworks
 *
 *  Jay Painter <jpaint@serv.net> <jpaint@gimp.org> <jpaint@real.com>
 *  
 *
 */
#include "OSAm.h"
#include "ScriptRunner.h"
#include <AppleEvents.h>



/*
 * Boiler plate generated from "genmodule.py"
 */
static PyObject *ErrorObject;
static char OSAm_DoCommand__doc__[] = "";



static PyObject *
OSAm_RunCompiledScript (self, args)
     PyObject *self;
     PyObject *args;
{
  char *commandStr = NULL;
  char *outpath = NULL;
  OSErr myErr;
  AEDesc temp;
  EventRecord event;

  temp.dataHandle = NULL;

  if (!PyArg_ParseTuple (args, "s", &commandStr))
    return NULL;

  myErr = ExecuteScriptFile (commandStr, NULL, &temp);

  if (temp.dataHandle != NULL && temp.descriptorType == 'TEXT')
    {
      char *line;
      DescType typeCode;
      long dataSize = 0;

      HLock (temp.dataHandle);

      dataSize = GetHandleSize (temp.dataHandle);

      if (dataSize > 0)
	{
	  PyObject *result = PyString_FromStringAndSize ((*temp.dataHandle), 
							 dataSize);

	  AEDisposeDesc (&temp);

	  if (!result)
	    {
	      printf ("OSAm.error Out of memory.\n");
	      Py_INCREF (Py_None);
	      return Py_None;
	    }

	  return result;
	}
    }

  if (myErr != noErr)
    {
      PyErr_Mac (ErrorObject, myErr);
      return NULL;
    }


  Py_INCREF (Py_None);
  return Py_None;
}




static PyObject *
OSAm_CompileAndSave (self, args)
     PyObject *self;
     PyObject *args;
{
  char *commandStr = NULL;
  char *outpath = NULL;
  OSErr myErr;
  AEDesc temp;
  EventRecord event;

  temp.dataHandle = NULL;

  if (!PyArg_ParseTuple (args, "ss", &commandStr, &outpath))
    return NULL;

  myErr = CompileAndSave (commandStr, outpath, NULL, &temp);


  if (temp.dataHandle != NULL && temp.descriptorType == 'TEXT')
    {
      char *line;
      DescType typeCode;
      long dataSize = 0;

      HLock (temp.dataHandle);

      dataSize = GetHandleSize (temp.dataHandle);

      if (dataSize > 0)
	{
	  PyObject *result = PyString_FromStringAndSize ((*temp.dataHandle), 
							 dataSize);

	  AEDisposeDesc (&temp);

	  if (!result)
	    {
	      printf ("OSAm.error Out of memory.\n");
	      Py_INCREF (Py_None);
	      return Py_None;
	    }

	  return result;
	}

    }

  if (myErr != noErr)
    {

      PyErr_Mac (ErrorObject, myErr);
      return NULL;
    }


  Py_INCREF (Py_None);
  return Py_None;
}



static PyObject *
OSAm_CompileAndExecute (self, args)
     PyObject *self;
     PyObject *args;
{
  char *commandStr;
  OSErr myErr;
  AEDesc temp;
  EventRecord event;

  temp.dataHandle = NULL;

  if (!PyArg_ParseTuple (args, "s", &commandStr))
    return NULL;

  myErr = CompileAndExecute (commandStr, &temp, NULL);

  if (temp.dataHandle != NULL && temp.descriptorType == 'TEXT')
    {
      char *line;
      DescType typeCode;
      long dataSize = 0;

      HLock (temp.dataHandle);

      dataSize = GetHandleSize (temp.dataHandle);

      if (dataSize > 0)
	{
	  PyObject *result = PyString_FromStringAndSize ((*temp.dataHandle), 
							 dataSize);

	  AEDisposeDesc (&temp);

	  if (!result)
	    {
	      printf ("OSAm.error Out of memory.\n");
	      Py_INCREF (Py_None);
	      return Py_None;
	    }

	  return result;
	}
    }

  if (myErr != noErr)
    {

      PyErr_Mac (ErrorObject, myErr);
      return NULL;
    }


  Py_INCREF (Py_None);
  return Py_None;
}



/* 
 * List of methods defined in the module
 */
static struct PyMethodDef OSAm_methods[] =
{
  {"CompileAndExecute", 
   (PyCFunction) OSAm_CompileAndExecute,
   METH_VARARGS,
   OSAm_DoCommand__doc__},

  {"CompileAndSave", 
   (PyCFunction) OSAm_CompileAndSave,
   METH_VARARGS, 
   OSAm_DoCommand__doc__},

  {"RunCompiledScript",
   (PyCFunction) OSAm_RunCompiledScript, 
   METH_VARARGS,
   OSAm_DoCommand__doc__},

  {NULL, (PyCFunction) NULL, 0, NULL}
};



static char OSAm_module_documentation[] = "";


/*
 * PYTHON Module Initalization
 */
void
initOSAm ()
{
  PyObject *m, *d;

  /* Create the module and add the functions */
  m = Py_InitModule4 ("OSAm",
		      OSAm_methods,
		      OSAm_module_documentation,
		      (PyObject *) NULL, PYTHON_API_VERSION);


  /* Add some symbolic constants to the module */
  d = PyModule_GetDict (m);
  ErrorObject = PyString_FromString ("OSAm.error");
  PyDict_SetItemString (d, "error", ErrorObject);


  /* Check for errors */
  if (PyErr_Occurred ())
    Py_FatalError ("can't initialize module OSAm");
}
