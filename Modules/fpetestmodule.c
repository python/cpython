/*
     ---------------------------------------------------------------------  
    /                       Copyright (c) 1996.                           \ 
   |          The Regents of the University of California.                 |
   |                        All rights reserved.                           |
   |                                                                       |
   |   Permission to use, copy, modify, and distribute this software for   |
   |   any purpose without fee is hereby granted, provided that this en-   |
   |   tire notice is included in all copies of any software which is or   |
   |   includes  a  copy  or  modification  of  this software and in all   |
   |   copies of the supporting documentation for such software.           |
   |                                                                       |
   |   This  work was produced at the University of California, Lawrence   |
   |   Livermore National Laboratory under  contract  no.  W-7405-ENG-48   |
   |   between  the  U.S.  Department  of  Energy and The Regents of the   |
   |   University of California for the operation of UC LLNL.              |
   |                                                                       |
   |                              DISCLAIMER                               |
   |                                                                       |
   |   This  software was prepared as an account of work sponsored by an   |
   |   agency of the United States Government. Neither the United States   |
   |   Government  nor the University of California nor any of their em-   |
   |   ployees, makes any warranty, express or implied, or  assumes  any   |
   |   liability  or  responsibility  for the accuracy, completeness, or   |
   |   usefulness of any information,  apparatus,  product,  or  process   |
   |   disclosed,   or  represents  that  its  use  would  not  infringe   |
   |   privately-owned rights. Reference herein to any specific  commer-   |
   |   cial  products,  process,  or  service  by trade name, trademark,   |
   |   manufacturer, or otherwise, does not  necessarily  constitute  or   |
   |   imply  its endorsement, recommendation, or favoring by the United   |
   |   States Government or the University of California. The views  and   |
   |   opinions  of authors expressed herein do not necessarily state or   |
   |   reflect those of the United States Government or  the  University   |
   |   of  California,  and shall not be used for advertising or product   |
    \  endorsement purposes.                                              / 
     ---------------------------------------------------------------------  
*/

/*
		  Floating point exception test module.

 */

#include "Python.h"

static PyObject *fpe_error;
PyMODINIT_FUNC initfpetest(void);
static PyObject *test(PyObject *self,PyObject *args);
static double db0(double);
static double overflow(double);
static double nest1(int, double);
static double nest2(int, double);
static double nest3(double);
static void printerr(double);

static PyMethodDef fpetest_methods[] = {
    {"test",		 (PyCFunction) test,		 METH_VARARGS},
    {0,0}
};

static PyObject *test(PyObject *self,PyObject *args)
{
    double r;

    fprintf(stderr,"overflow");
    r = overflow(1.e160);
    printerr(r);

    fprintf(stderr,"\ndiv by 0");
    r = db0(0.0);
    printerr(r);

    fprintf(stderr,"\nnested outer");
    r = nest1(0, 0.0);
    printerr(r);

    fprintf(stderr,"\nnested inner");
    r = nest1(1, 1.0);
    printerr(r);

    fprintf(stderr,"\ntrailing outer");
    r = nest1(2, 2.0);
    printerr(r);

    fprintf(stderr,"\nnested prior");
    r = nest2(0, 0.0);
    printerr(r);

    fprintf(stderr,"\nnested interior");
    r = nest2(1, 1.0);
    printerr(r);

    fprintf(stderr,"\nnested trailing");
    r = nest2(2, 2.0);
    printerr(r);

    Py_INCREF (Py_None);
    return Py_None;
}

static void printerr(double r)
{
    if(r == 3.1416){
      fprintf(stderr,"\tPASS\n");
      PyErr_Print();
    }else{
      fprintf(stderr,"\tFAIL\n");
    }
    PyErr_Clear();
}

static double nest1(int i, double x)
{
  double a = 1.0;

  PyFPE_START_PROTECT("Division by zero, outer zone", return 3.1416)
  if(i == 0){
    a = 1./x;
  }else if(i == 1){
    /* This (following) message is never seen. */
    PyFPE_START_PROTECT("Division by zero, inner zone", return 3.1416)
    a = 1./(1. - x);
    PyFPE_END_PROTECT(a)
  }else if(i == 2){
    a = 1./(2. - x);
  }
  PyFPE_END_PROTECT(a)

  return a;
}

static double nest2(int i, double x)
{
  double a = 1.0;
  PyFPE_START_PROTECT("Division by zero, prior error", return 3.1416)
  if(i == 0){
    a = 1./x;
  }else if(i == 1){
    a = nest3(x);
  }else if(i == 2){
    a = 1./(2. - x);
  }
  PyFPE_END_PROTECT(a)
  return a;
}

static double nest3(double x)
{
  double result;
  /* This (following) message is never seen. */
  PyFPE_START_PROTECT("Division by zero, nest3 error", return 3.1416)
  result = 1./(1. - x);
  PyFPE_END_PROTECT(result)
  return result;
}

static double db0(double x)
{
  double a;
  PyFPE_START_PROTECT("Division by zero", return 3.1416)
  a = 1./x;
  PyFPE_END_PROTECT(a)
  return a;
}

static double overflow(double b)
{
  double a;
  PyFPE_START_PROTECT("Overflow", return 3.1416)
  a = b*b;
  PyFPE_END_PROTECT(a)
  return a;
}

PyMODINIT_FUNC initfpetest(void)
{
    PyObject *m, *d;

    m = Py_InitModule("fpetest", fpetest_methods);
    d = PyModule_GetDict(m);
    fpe_error = PyErr_NewException("fpetest.error", NULL, NULL);
    if (fpe_error != NULL)
	    PyDict_SetItemString(d, "error", fpe_error);
}
