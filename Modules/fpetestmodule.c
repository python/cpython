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
void initfpetest(void);
static PyObject *test(PyObject *self,PyObject *args);
static int db0(void);
static int overflow(void);
static int nest1(double);
static int nest2(double);
static double nest3(double);

static PyMethodDef fpetest_methods[] = {
    {"test",		 (PyCFunction) test,		 1},
    {0,0}
};

static PyObject *test(PyObject *self,PyObject *args)
{
    int i = 0, r;

    fprintf(stderr,"Test trapping overflow\n");
    r = overflow();
    if(r){
      fprintf(stderr,"(Note: No exception was raised.)\n");
      PyErr_Clear();
    }else{
      fprintf(stderr,"Trapped:: ");
      PyErr_Print();
      PyErr_Clear();
    }
    i += r;

    fprintf(stderr,"Test trapping division by zero\n");
    r = db0();
    if(r){
      fprintf(stderr,"(Note: No exception was raised.)\n");
      PyErr_Clear();
    }else{
      fprintf(stderr,"Trapped:: ");
      PyErr_Print();
      PyErr_Clear();
    }
    i += r;

    fprintf(stderr,"Test nested protection zones, outer zone\n");
    r = nest1(0.0);
    if(r){
      fprintf(stderr,"(Note: No exception was raised.)\n");
      PyErr_Clear();
    }else{
      fprintf(stderr,"Trapped:: ");
      PyErr_Print();
      PyErr_Clear();
    }
    i += r;

    fprintf(stderr,"Test nested protection zones, inner zone\n");
    fprintf(stderr,"(Note: Return will apparently come from outer zone.)\n");
    r = nest1(1.0);
    if(r){
      fprintf(stderr,"(Note: No exception was raised.)\n");
      PyErr_Clear();
    }else{
      fprintf(stderr,"Trapped:: ");
      PyErr_Print();
      PyErr_Clear();
    }
    i += r;

    fprintf(stderr,"Test nested protection zones, trailing outer zone\n");
    r = nest1(2.0);
    if(r){
      fprintf(stderr,"(Note: No exception was raised.)\n");
      PyErr_Clear();
    }else{
      fprintf(stderr,"Trapped:: ");
      PyErr_Print();
      PyErr_Clear();
    }
    i += r;

    fprintf(stderr,"Test nested function calls, prior error\n");
    r = nest2(0.0);
    if(r){
      fprintf(stderr,"(Note: No exception was raised.)\n");
      PyErr_Clear();
    }else{
      fprintf(stderr,"Trapped:: ");
      PyErr_Print();
      PyErr_Clear();
    }
    i += r;

    fprintf(stderr,"Test nested function calls, interior error\n");
    r = nest2(1.0);
    if(r){
      fprintf(stderr,"(Note: No exception was raised.)\n");
      PyErr_Clear();
    }else{
      fprintf(stderr,"Trapped:: ");
      PyErr_Print();
      PyErr_Clear();
    }
    i += r;

    fprintf(stderr,"Test nested function calls, trailing error\n");
    r = nest2(2.0);
    if(r){
      fprintf(stderr,"(Note: No exception was raised.)\n");
      PyErr_Clear();
    }else{
      fprintf(stderr,"Trapped:: ");
      PyErr_Print();
      PyErr_Clear();
    }
    i += r;

    fprintf(stderr,"Number of tests failed: %d\n", i);
    Py_INCREF (Py_None);
    return Py_None;
}

static int nest1(double x)
{
  double a = 1.0;
  PyFPE_START_PROTECT("Division by zero, outer zone", return 0)
  a = 1./x;
    PyFPE_START_PROTECT("Division by zero, inner zone", return 0)
    a = 1./(1. - x);
    PyFPE_END_PROTECT
  a = 1./(2. - x);
  PyFPE_END_PROTECT
  return(1);
}

static int nest2(double x)
{
  double a = 1.0;
  PyFPE_START_PROTECT("Division by zero, prior error", return 0)
  a = 1./x;
  a = nest3(x);
  a = 1./(2. - x);
  PyFPE_END_PROTECT
  return(1);
}

static double nest3(double x)
{
  double result;
  PyFPE_START_PROTECT("Division by zero, nest3 error", return 0)
  result = 1./(1. - x);
  PyFPE_END_PROTECT
  return result;
}

static int db0(void)
{
  double a = 1.0;
  PyFPE_START_PROTECT("Division by zero", return 0)
  a = 1./(a - 1.);
  PyFPE_END_PROTECT
  return(1);
}

static int overflow(void)
{
  double a, b = 1.e200;
  PyFPE_START_PROTECT("Overflow", return 0)
  a = b*b;
  PyFPE_END_PROTECT
  return(1);
}

void initfpetest(void)
{
    PyObject *m, *d;

    m = Py_InitModule("fpetest", fpetest_methods);
    d = PyModule_GetDict(m);
    fpe_error = PyString_FromString("fpetest.error");
    PyDict_SetItemString(d, "error", fpe_error);

    if (PyErr_Occurred())
	Py_FatalError("Cannot initialize module fpetest");
}
