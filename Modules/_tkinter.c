/* tkintermodule.c -- Interface to libtk.a and libtcl.a.
   Copyright (C) 1994 Steen Lumholt */

#if 0
#include <Py/Python.h>
#else

#include "allobjects.h"
#include "pythonrun.h"
#include "intrcheck.h"
#include "modsupport.h"
#include "sysmodule.h"

#define PyObject object
typedef struct methodlist PyMethodDef;
#define PyInit_tkinter inittkinter

#undef Py_True
#define Py_True ((object *) &TrueObject)
#undef True

#undef Py_False
#define Py_False ((object *) &FalseObject)
#undef False

#undef Py_None
#define Py_None (&NoObject)
#undef None

#endif /* 0 */

#include <tcl.h>
#include <tk.h>

extern char *getprogramname ();

/* Internal declarations from tkInt.h.  */ 
extern int tk_NumMainWindows;
extern struct { Tk_Window win; } *tkMainWindowList;

/**** Tkapp Object Declaration ****/

staticforward PyTypeObject Tkapp_Type;

typedef struct
  {
    PyObject_HEAD
    Tcl_Interp *interp;
    Tk_Window tkwin;
  }
TkappObject;

#define Tkapp_Check(v) ((v)->ob_type == &Tkapp_Type)
#define Tkapp_Tkwin(v)  (((TkappObject *) (v))->tkwin)
#define Tkapp_Interp(v) (((TkappObject *) (v))->interp)
#define Tkapp_Result(v) (((TkappObject *) (v))->interp->result)

#define DEBUG_REFCNT(v) (printf ("DEBUG: id=%p, refcnt=%i\n", \
				 (void *) v, ((PyObject *) v)->ob_refcnt))

/**** Error Handling ****/

static PyObject *Tkinter_TclError;
static int quitMainLoop = 0;
static int errorInCmd = 0;
static PyObject *excInCmd;
static PyObject *valInCmd;

static PyObject *
Tkinter_Error (v)
     PyObject *v;
{
  PyErr_SetString (Tkinter_TclError, Tkapp_Result (v));
  return NULL;
}

int
PythonCmd_Error (interp)
     Tcl_Interp *interp;
{
  errorInCmd = 1;
  PyErr_GetAndClear (&excInCmd, &valInCmd);
  return TCL_ERROR;
}

/**** Utils ****/

static char *
AsString (value, tmp)
     PyObject *value;
     PyObject *tmp;
{
  if (PyString_Check (value))
    return PyString_AsString (value);
  else
    {
      PyObject *v;

      v = strobject (value);
      PyList_Append (tmp, v);
      Py_DECREF (v);
      return PyString_AsString (v);
    }
}

#define ARGSZ 64

static char *
Merge (args)
     PyObject *args;
{
  PyObject *tmp;
  char *argvStore[ARGSZ];
  char **argv;
  int fvStore[ARGSZ];
  int *fv;
  int argc;
  char *res;
  int i;

  tmp = PyList_New (0);
  argv = argvStore;
  fv = fvStore;

  if (!PyTuple_Check (args))
    {
      argc = 1;
      fv[0] = 0;
      argv[0] = AsString (args, tmp);
    }
  else
    {
      PyObject *v;

      if (PyTuple_Size (args) > ARGSZ)
	{
	  argv = malloc (PyTuple_Size (args) * sizeof (char *));
	  fv = malloc (PyTuple_Size (args) * sizeof (int));
	  if (argv == NULL || fv == NULL)
	    PyErr_NoMemory ();
	}

      argc = PyTuple_Size (args);
      for (i = 0; i < argc; i++)
	{
	  v = PyTuple_GetItem (args, i);
	  if (PyTuple_Check (v))
	    {
	      fv[i] = 1;
	      argv[i] = Merge (v);
	    }
	  else if (v == Py_None)
	    {
	      argc = i;
	      break;
	    }
	  else
	    {
	      fv[i] = 0;
	      argv[i] = AsString (v, tmp);
	    }
	}
    }

  res = Tcl_Merge (argc, argv);

  Py_DECREF (tmp);
  for (i = 0; i < argc; i++)
    if (fv[i]) free (argv[i]);
  if (argv != argvStore)
    free (argv);
  if (fv != fvStore)
    free (fv);

  return res;
}

static PyObject *
Split (self, list)
     PyObject *self;
     char *list;
{
  int argc;
  char **argv;
  PyObject *v;

  if (list == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  if (Tcl_SplitList (Tkapp_Interp (self), list, &argc, &argv) == TCL_ERROR)
    return Tkinter_Error (self);

  if (argc == 0)
    v = PyString_FromString ("");
  else if (argc == 1)
    v = PyString_FromString (argv[0]);
  else
    {
      int i;

      v = PyTuple_New (argc);
      for (i = 0; i < argc; i++)
	PyTuple_SetItem (v, i, Split (self, argv[i]));
    }

  free (argv);
  return v;
}

/**** Tkapp Object ****/

#ifndef WITH_APPINIT
int
Tcl_AppInit (interp)
     Tcl_Interp *interp;
{
  if (Tcl_Init (interp) == TCL_ERROR)
    return TCL_ERROR;
  if (Tk_Init (interp) == TCL_ERROR)
    return TCL_ERROR;
  return TCL_OK;
}
#endif /* !WITH_APPINIT */

/* Initialize the Tk application; see the `main' function in
   `tkMain.c'.  */
static TkappObject *
Tkapp_New (screenName, baseName, className, interactive)
     char *screenName;
     char *baseName;
     char *className;
     int interactive;
{
  TkappObject *v;
  
  v = PyObject_NEW (TkappObject, &Tkapp_Type);
  if (v == NULL)
    return NULL;

  v->interp = Tcl_CreateInterp ();
  v->tkwin = Tk_CreateMainWindow (v->interp, screenName, 
				  baseName, className);
  if (v->tkwin == NULL)
    return (TkappObject *) Tkinter_Error ((PyObject *) v);

  Tk_GeometryRequest (v->tkwin, 200, 200);

  if (screenName != NULL)
    Tcl_SetVar2 (v->interp, "env", "DISPLAY", screenName, TCL_GLOBAL_ONLY);

  if (interactive)
    Tcl_SetVar (v->interp, "tcl_interactive", "1", TCL_GLOBAL_ONLY);
  else
    Tcl_SetVar (v->interp, "tcl_interactive", "0", TCL_GLOBAL_ONLY);

  if (Tcl_AppInit (v->interp) != TCL_OK)
    {
      PyErr_SetString (Tkinter_TclError, "Tcl_AppInit failed"); /* XXX */
      return NULL;
    }

  return v;
}

/** Tcl Eval **/

static PyObject *
Tkapp_Call (self, args)
     PyObject *self;
     PyObject *args;
{
  char *cmd;

  cmd = Merge (args);
  if (Tcl_Eval (Tkapp_Interp (self), cmd) == TCL_ERROR)
    {
      free (cmd);
      return Tkinter_Error (self);
    }

  free (cmd);
  return PyString_FromString (Tkapp_Result (self));
}

static PyObject *
Tkapp_GlobalCall (self, args)
     PyObject *self;
     PyObject *args;
{
  char *cmd;

  cmd = Merge (args);
  if (Tcl_GlobalEval (Tkapp_Interp (self), cmd) == TCL_ERROR)
    {
      free (cmd);
      return Tkinter_Error (self);
    }
  
  free (cmd);
  return PyString_FromString (Tkapp_Result (self));
}

static PyObject *
Tkapp_Eval (self, args)
     PyObject *self;
     PyObject *args;
{
  char *script;
  
  if (!PyArg_Parse (args, "s", &script))
    return NULL;

  if (Tcl_Eval (Tkapp_Interp (self), script) == TCL_ERROR)
    return Tkinter_Error (self);
  
  return PyString_FromString (Tkapp_Result (self));
}

static PyObject *
Tkapp_GlobalEval (self, args)
     PyObject *self;
     PyObject *args;
{
  char *script;
  
  if (!PyArg_Parse (args, "s", &script))
    return NULL;

  if (Tcl_GlobalEval (Tkapp_Interp (self), script) == TCL_ERROR)
    return Tkinter_Error (self);

  return PyString_FromString (Tkapp_Result (self));
}

static PyObject *
Tkapp_EvalFile (self, args)
     PyObject *self;
     PyObject *args;
{
  char *fileName;

  if (!PyArg_Parse (args, "s", &fileName))
    return NULL;

  if (Tcl_EvalFile (Tkapp_Interp (self), fileName) == TCL_ERROR)
    return Tkinter_Error (self);

  return PyString_FromString (Tkapp_Result (self));
}

static PyObject *
Tkapp_Record (self, args)
     PyObject *self;
     PyObject *args;
{
  char *script;

  if (!PyArg_Parse (args, "s", &script))
    return NULL;

  if (Tcl_RecordAndEval (Tkapp_Interp (self), 
			 script, TCL_NO_EVAL) == TCL_ERROR)
    return Tkinter_Error (self);

  return PyString_FromString (Tkapp_Result (self));
}

static PyObject *
Tkapp_AddErrorInfo (self, args)
     PyObject *self;
     PyObject *args;
{
  char *msg;

  if (!PyArg_Parse (args, "s", &msg))
    return NULL;
  Tcl_AddErrorInfo (Tkapp_Interp (self), msg);

  Py_INCREF(Py_None);
  return Py_None;
}

/** Tcl Variable **/

static PyObject *
SetVar (self, args, flags)
     PyObject *self;
     PyObject *args;
     int flags;
{
  char *name1, *name2, *ok;
  PyObject *newValue;
  PyObject *tmp;

  tmp = PyList_New (0);

  if (PyArg_Parse (args, "(sO)", &name1, &newValue))
    ok = Tcl_SetVar (Tkapp_Interp (self), name1, 
		     AsString (newValue, tmp), flags); /* XXX Merge? */
  else if (PyArg_Parse (args, "(ssO)", &name1, &name2, &newValue))
    ok = Tcl_SetVar2 (Tkapp_Interp (self), name1, name2, 
		      AsString (newValue, tmp), flags);
  else
    {
      Py_DECREF (tmp);
      return NULL;
    }
  Py_DECREF (tmp);

  if (!ok)
    return Tkinter_Error (self);

  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
Tkapp_SetVar (self, args)
     PyObject *self;
     PyObject *args;
{
  return SetVar (self, args, TCL_LEAVE_ERR_MSG);
}

static PyObject *
Tkapp_GlobalSetVar (self, args)
     PyObject *self;
     PyObject *args;
{
  return SetVar (self, args, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
}

static PyObject *
GetVar (self, args, flags)
     PyObject *self;
     PyObject *args;
     int flags;
{
  char *name1, *name2, *s;

  if (PyArg_Parse (args, "s", &name1))
    s = Tcl_GetVar (Tkapp_Interp (self), name1, flags);
  else if (PyArg_Parse (args, "(ss)", &name1, &name2))
    s = Tcl_GetVar2 (Tkapp_Interp (self), name1, name2, flags);
  else
    return NULL;

  if (s == NULL)
    return Tkinter_Error (self);

  return PyString_FromString (s);
}

static PyObject *
Tkapp_GetVar (self, args)
     PyObject *self;
     PyObject *args;
{
  return GetVar (self, args, TCL_LEAVE_ERR_MSG);
}

static PyObject *
Tkapp_GlobalGetVar (self, args)
     PyObject *self;
     PyObject *args;
{
  return GetVar (self, args, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
}

static PyObject *
UnsetVar (self, args, flags)
     PyObject *self;
     PyObject *args;
     int flags;
{
  char *name1, *name2;
  int code;

  if (PyArg_Parse (args, "s", &name1))
    code = Tcl_UnsetVar (Tkapp_Interp (self), name1, flags);
  else if (PyArg_Parse (args, "(ss)", &name1, &name2))
    code = Tcl_UnsetVar2 (Tkapp_Interp (self), name1, name2, flags);
  else
    return NULL;

  if (code == TCL_ERROR)
    return Tkinter_Error (self);

  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
Tkapp_UnsetVar (self, args)
     PyObject *self;
     PyObject *args;
{
  return UnsetVar (self, args, TCL_LEAVE_ERR_MSG);
}

static PyObject *
Tkapp_GlobalUnsetVar (self, args)
     PyObject *self;
     PyObject *args;
{
  return UnsetVar (self, args, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
}

/** Tcl to Python **/

static PyObject *
Tkapp_GetInt (self, args)
     PyObject *self;
     PyObject *args;
{
  char *s;
  int v;

  if (!PyArg_Parse (args, "s", &s))
    return NULL;
  if (Tcl_GetInt (Tkapp_Interp (self), s, &v) == TCL_ERROR)
    return Tkinter_Error (self);
  return Py_BuildValue ("i", v);
}

static PyObject *
Tkapp_GetDouble (self, args)
     PyObject *self;
     PyObject *args;
{
  char *s;
  double v;

  if (!PyArg_Parse (args, "s", &s))
    return NULL;
  if (Tcl_GetDouble (Tkapp_Interp (self), s, &v) == TCL_ERROR)
    return Tkinter_Error (self);
  return Py_BuildValue ("d", v);
}

static PyObject *
Tkapp_GetBoolean (self, args)
     PyObject *self;
     PyObject *args;
{
  char *s;
  int v;

  if (!PyArg_Parse (args, "s", &s))
    return NULL;
  if (Tcl_GetBoolean (Tkapp_Interp (self), s, &v) == TCL_ERROR)
    return Tkinter_Error (self);
  return Py_BuildValue ("i", v);
}

static PyObject *
Tkapp_ExprString (self, args)
     PyObject *self;
     PyObject *args;
{
  char *s;

  if (!PyArg_Parse (args, "s", &s))
    return NULL;
  if (Tcl_ExprString (Tkapp_Interp (self), s) == TCL_ERROR)
    return Tkinter_Error (self);
  return Py_BuildValue ("s", Tkapp_Result (self));
}

static PyObject *
Tkapp_ExprLong (self, args)
     PyObject *self;
     PyObject *args;
{
  char *s;
  long v;

  if (!PyArg_Parse (args, "s", &s))
    return NULL;
  if (Tcl_ExprLong (Tkapp_Interp (self), s, &v) == TCL_ERROR)
    return Tkinter_Error (self);
  return Py_BuildValue ("l", v);
}

static PyObject *
Tkapp_ExprDouble (self, args)
     PyObject *self;
     PyObject *args;
{
  char *s;
  double v;

  if (!PyArg_Parse (args, "s", &s))
    return NULL;
  if (Tcl_ExprDouble (Tkapp_Interp (self), s, &v) == TCL_ERROR)
    return Tkinter_Error (self);
  return Py_BuildValue ("d", v);
}

static PyObject *
Tkapp_ExprBoolean (self, args)
     PyObject *self;
     PyObject *args;
{
  char *s;
  int v;

  if (!PyArg_Parse (args, "s", &s))
    return NULL;
  if (Tcl_ExprBoolean (Tkapp_Interp (self), s, &v) == TCL_ERROR)
    return Tkinter_Error (self);
  return Py_BuildValue ("i", v);
}

static PyObject *
Tkapp_SplitList (self, args)
     PyObject *self;
     PyObject *args;
{
  char *list;
  int argc;
  char **argv;
  PyObject *v;
  int i;

  if (!PyArg_Parse (args, "s", &list))
    return NULL;

  if (Tcl_SplitList (Tkapp_Interp (self), list, &argc, &argv) == TCL_ERROR)
    return Tkinter_Error (self);

  v = PyTuple_New (argc);
  for (i = 0; i < argc; i++)
    PyTuple_SetItem (v, i, PyString_FromString (argv[i]));

  free (argv);
  return v;
}

static PyObject *
Tkapp_Split (self, args)
     PyObject *self;
     PyObject *args;
{
  char *list;

  if (!PyArg_Parse (args, "s", &list))
    return NULL;
  return Split (self, list);
}

static PyObject *
Tkapp_Merge (self, args)
     PyObject *self;
     PyObject *args;
{
  char *s;
  PyObject *v;

  s = Merge (args);
  v = PyString_FromString (s);
  free (s);
  return v;
}

/** Tcl Command **/

/* This is the Tcl command that acts as a wrapper for Python
   function or method.  */
static int
PythonCmd (clientData, interp, argc, argv)
     ClientData clientData;	/* Is (self, func) */
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
  PyObject *self, *func, *arg, *res, *tmp;
  int i;

  self = PyTuple_GetItem ((PyObject *) clientData, 0);
  func = PyTuple_GetItem ((PyObject *) clientData, 1);

  /* Create argument list (argv1, ..., argvN) */
  arg = PyTuple_New (argc - 1);
  for (i = 0; i < (argc - 1); i++)
    PyTuple_SetItem (arg, i, PyString_FromString (argv[i + 1]));
  
  res = PyEval_CallObject (func, arg);
  Py_DECREF (arg);

  if (res == NULL)
    return PythonCmd_Error (interp);

  tmp = PyList_New (0);
  Tcl_SetResult (Tkapp_Interp (self), AsString (res, tmp), TCL_VOLATILE);
  Py_DECREF (res);
  Py_DECREF (tmp);

  return TCL_OK;
}

static void
PythonCmdDelete (clientData)
     ClientData clientData;	/* Is (self, func) */
{
  Py_DECREF ((PyObject *) clientData);
}

static PyObject *
Tkapp_CreateCommand (self, args)
     PyObject *self;
     PyObject *args;
{
  char *cmdName;
  PyObject *data;
  PyObject *func;
  
  /* Args is: (cmdName, func) */
  if (!PyTuple_Check (args) 
      || !(PyTuple_Size (args) == 2)
      || !PyString_Check (PyTuple_GetItem (args, 0))
      || !(PyMethod_Check (PyTuple_GetItem (args, 1)) 
	   || PyFunction_Check (PyTuple_GetItem (args, 1))))
    {
      PyErr_SetString (PyExc_TypeError, "bad argument list");
      return NULL;
    }

  cmdName = PyString_AsString (PyTuple_GetItem (args, 0));
  func = PyTuple_GetItem (args, 1);

  data = PyTuple_New (2);   /* ClientData is: (self, func) */

  Py_INCREF (self);
  PyTuple_SetItem (data, 0, self);

  Py_INCREF (func);
  PyTuple_SetItem (data, 1, func);

  Tcl_CreateCommand (Tkapp_Interp (self), cmdName, PythonCmd,
		     (ClientData) data, PythonCmdDelete);

  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
Tkapp_DeleteCommand (self, args)
     PyObject *self;
     PyObject *args;
{
  char *cmdName;

  if (!PyArg_Parse (args, "s", &cmdName))
    return NULL;
  if (Tcl_DeleteCommand (Tkapp_Interp (self), cmdName) == -1)
    {
      PyErr_SetString (Tkinter_TclError, "can't delete Tcl command");
      return NULL;
    }
  Py_INCREF (Py_None);
  return Py_None;
}

/** File Handler **/

void
FileHandler (clientData, mask)
     ClientData clientData;	/* Is: (func, file) */
     int mask;
{
  PyObject *func, *file, *arg, *res;

  func = PyTuple_GetItem ((PyObject *) clientData, 0);
  file = PyTuple_GetItem ((PyObject *) clientData, 1);

  arg = Py_BuildValue ("(Oi)", file, (long) mask);
  res = PyEval_CallObject (func, arg);
  Py_DECREF (arg);
  if (res == NULL)
    {
      errorInCmd = 1;
      PyErr_GetAndClear (&excInCmd, &valInCmd);
    }
  Py_DECREF (res);
}

static PyObject *
Tkapp_CreateFileHandler (self, args)
     PyObject *self;
     PyObject *args;		/* Is (file, mask, func) */
{
  PyObject *file, *func, *data;
  int mask, id;

  if (!PyArg_Parse (args, "(OiO)", &file, &mask, &func))
    return NULL;
  if (!PyFile_Check (file) 
      || !(PyMethod_Check(func) || PyFunction_Check(func)))
    {
      PyErr_SetString (PyExc_TypeError, "bad argument list");
      return NULL;
    }

  /* ClientData is: (func, file) */
  data = Py_BuildValue ("(OO)", func, file);

  id = fileno (PyFile_AsFile (file));
  Tk_CreateFileHandler (id, mask, FileHandler, (ClientData) data);
  /* XXX fileHandlerDict */

  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
Tkapp_DeleteFileHandler (self, args)
     PyObject *self;
     PyObject *args;		/* Args: file */
{
  int id;

  if (!PyFile_Check (args))
    {
      PyErr_SetString (PyExc_TypeError, "bad argument list");
      return NULL;
    }

  id = fileno (PyFile_AsFile (args));
  Tk_DeleteFileHandler (id);
  /* XXX fileHandlerDict */
  Py_INCREF (Py_None);
  return Py_None;
}

/** Event Loop **/

static PyObject *
Tkapp_MainLoop (self, args)
     PyObject *self;
     PyObject *args;
{
  if (!PyArg_Parse (args, ""))
    return NULL;

  quitMainLoop = 0;
  while (tk_NumMainWindows > 0 && !quitMainLoop && !errorInCmd)
    {
      if (PyOS_InterruptOccurred ())
	{
	  PyErr_SetNone (PyExc_KeyboardInterrupt);
	  return NULL;
	}
      Tk_DoOneEvent (0);
    }

  if (errorInCmd)
    {
      errorInCmd = 0;
      PyErr_SetObject (excInCmd, valInCmd);
      return NULL;
    }
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
Tkapp_Quit (self, args)
     PyObject *self;
     PyObject *args;
{

  if (!PyArg_Parse (args, ""))
    return NULL;
  quitMainLoop = 1;
  Py_INCREF (Py_None);
  return Py_None;
}

/**** Tkapp Method List ****/

static PyMethodDef Tkapp_methods[] =
{
  {"call", Tkapp_Call},
  {"globalcall", Tkapp_GlobalCall},
  {"eval", Tkapp_Eval},
  {"globaleval", Tkapp_GlobalEval},
  {"evalfile", Tkapp_EvalFile},
  {"record", Tkapp_Record},
  {"adderrorinfo", Tkapp_AddErrorInfo},
  {"setvar", Tkapp_SetVar},
  {"globalsetvar", Tkapp_GlobalSetVar},
  {"getvar", Tkapp_GetVar},
  {"globalgetvar", Tkapp_GlobalGetVar},
  {"unsetvar", Tkapp_UnsetVar},
  {"globalunsetvar", Tkapp_GlobalUnsetVar},
  {"getint", Tkapp_GetInt},
  {"getdouble", Tkapp_GetDouble},
  {"getboolean", Tkapp_GetBoolean},
  {"exprstring", Tkapp_ExprString},
  {"exprlong", Tkapp_ExprLong},
  {"exprdouble", Tkapp_ExprDouble},
  {"exprboolean", Tkapp_ExprBoolean},
  {"splitlist", Tkapp_SplitList},
  {"split", Tkapp_Split},
  {"merge", Tkapp_Merge},
  {"createcommand", Tkapp_CreateCommand},
  {"deletecommand", Tkapp_DeleteCommand},
  {"createfilehandler", Tkapp_CreateFileHandler},
  {"deletefilehandler", Tkapp_DeleteFileHandler},
  {"mainloop", Tkapp_MainLoop},
  {"quit", Tkapp_Quit},
  {NULL, NULL}
};

/**** Tkapp Type Methods ****/

static void
Tkapp_Dealloc (self)
     PyObject *self;
{
  Tk_DestroyWindow (Tkapp_Tkwin (self));
  Tcl_DeleteInterp (Tkapp_Interp (self));
  PyMem_DEL (self);
}

static PyObject *
Tkapp_GetAttr (self, name)
     PyObject *self;
     char *name;
{
  return Py_FindMethod (Tkapp_methods, self, name);
}

static PyTypeObject Tkapp_Type =
{
  OB_HEAD_INIT (&PyType_Type)
  0,				/*ob_size */
  "tkapp",			/*tp_name */
  sizeof (TkappObject),		/*tp_basicsize */
  0,				/*tp_itemsize */
  Tkapp_Dealloc,		/*tp_dealloc */
  0,				/*tp_print */
  Tkapp_GetAttr,		/*tp_getattr */
  0,				/*tp_setattr */
  0,				/*tp_compare */
  0,				/*tp_repr */
  0,				/*tp_as_number */
  0,				/*tp_as_sequence */
  0,				/*tp_as_mapping */
  0,				/*tp_hash */
};

/**** Tkinter Module ****/

static PyObject *
Tkinter_Create (self, args)
     PyObject *self;
     PyObject *args;
{
  char *screenName = NULL;
  char *baseName;
  char *className;
  int interactive = 0;

  baseName = strrchr (getprogramname (), '/');
  if (baseName != NULL)
    baseName++;
  else
    baseName = getprogramname ();
  className = "Tk";
  
  if (PyArg_Parse (args, ""))
    /* VOID */ ;
  else if (PyArg_Parse (args, "z", 
			&screenName))
    /* VOID */ ;
  else if (PyArg_Parse (args, "(zs)", 
			&screenName, &baseName))
    /* VOID */ ;
  else if (PyArg_Parse (args, "(zss)", 
			&screenName, &baseName, &className))
    /* VOID */ ;
  else if (PyArg_Parse (args, "(zssi)", 
			&screenName, &baseName, &className, &interactive))
    /* VOID */ ;
  else
    return NULL;

  return (PyObject *) Tkapp_New (screenName, baseName, className, 
				 interactive);
}

static PyMethodDef moduleMethods[] =
{
  {"create", Tkinter_Create},
  {NULL, NULL}
};

#ifdef WITH_READLINE
static int
EventHook ()
{
  if (errorInCmd)		/* XXX Reset tty */
    {
      errorInCmd = 0;
      PyErr_SetObject (excInCmd, valInCmd);
      PyErr_Print ();
     }
  if (tk_NumMainWindows > 0)
    Tk_DoOneEvent (TK_DONT_WAIT);
  return 0;
}
#endif /* WITH_READLINE */

static void
Tkinter_Cleanup ()
{
  /* XXX rl_deprep_terminal is static, damned! */
  while (tkMainWindowList != 0)
    Tk_DestroyWindow (tkMainWindowList->win);
}

void
PyInit_tkinter ()
{
  static inited = 0;

#ifdef WITH_READLINE
  extern int (*rl_event_hook) ();
#endif /* WITH_READLINE */
  PyObject *m, *d, *v;

  m = Py_InitModule ("tkinter", moduleMethods);

  d = PyModule_GetDict (m);
  Tkinter_TclError = Py_BuildValue ("s", "TclError");
  PyDict_SetItemString (d, "TclError", Tkinter_TclError);

  v = Py_BuildValue ("i", TK_READABLE);
  PyDict_SetItemString (d, "READABLE", v);
  v = Py_BuildValue ("i", TK_WRITABLE);
  PyDict_SetItemString (d, "WRITABLE", v);
  v = Py_BuildValue ("i", TK_EXCEPTION);
  PyDict_SetItemString (d, "EXCEPTION", v);

#ifdef WITH_READLINE
  rl_event_hook = EventHook;
#endif /* WITH_READLINE */

  if (!inited)
    {
      inited = 1;
      if (atexit (Tkinter_Cleanup))
	PyErr_SetFromErrno (Tkinter_TclError);
    }

  if (PyErr_Occurred ())
    Py_FatalError ("can't initialize module tkinter");
}
