static char operator_doc[] = "\
Operator interface.\n\
\n\
This module exports a set of functions implemented in C corresponding\n\
to the intrinsic operators of Python.  For example, operator.add(x, y)\n\
is equivalent to the expression x+y.  The function names are those\n\
used for special class methods; variants without leading and trailing\n\
'__' are also provided for convenience.\n\
";

/*

     Copyright 

       Copyright 1996 Digital Creations, L.C., 910 Princess Anne
       Street, Suite 300, Fredericksburg, Virginia 22401 U.S.A. All
       rights reserved.  Copyright in this software is owned by DCLC,
       unless otherwise indicated. Permission to use, copy and
       distribute this software is hereby granted, provided that the
       above copyright notice appear in all copies and that both that
       copyright notice and this permission notice appear. Note that
       any product, process or technology described in this software
       may be the subject of other Intellectual Property rights
       reserved by Digital Creations, L.C. and are not licensed
       hereunder.

     Trademarks 

       Digital Creations & DCLC, are trademarks of Digital Creations, L.C..
       All other trademarks are owned by their respective companies. 

     No Warranty 

       The software is provided "as is" without warranty of any kind,
       either express or implied, including, but not limited to, the
       implied warranties of merchantability, fitness for a particular
       purpose, or non-infringement. This software could include
       technical inaccuracies or typographical errors. Changes are
       periodically made to the software; these changes will be
       incorporated in new editions of the software. DCLC may make
       improvements and/or changes in this software at any time
       without notice.

     Limitation Of Liability 

       In no event will DCLC be liable for direct, indirect, special,
       incidental, economic, cover, or consequential damages arising
       out of the use of or inability to use this software even if
       advised of the possibility of such damages. Some states do not
       allow the exclusion or limitation of implied warranties or
       limitation of liability for incidental or consequential
       damages, so the above limitation or exclusion may not apply to
       you.


    If you have questions regarding this software,
    contact:
   
      Jim Fulton, jim@digicool.com
      Digital Creations L.C.  
   
      (540) 371-6909

    Modifications
    
      Renamed and slightly rearranged by Guido van Rossum

*/

#include "Python.h"

#define spam1(OP,AOP) static PyObject *OP(s,a) PyObject *s, *a; { \
  PyObject *a1; \
  if(! PyArg_ParseTuple(a,"O",&a1)) return NULL; \
  return AOP(a1); }

#define spam2(OP,AOP) static PyObject *OP(s,a) PyObject *s, *a; { \
  PyObject *a1, *a2; \
  if(! PyArg_ParseTuple(a,"OO",&a1,&a2)) return NULL; \
  return AOP(a1,a2); }

#define spamoi(OP,AOP) static PyObject *OP(s,a) PyObject *s, *a; { \
  PyObject *a1; int a2; \
  if(! PyArg_ParseTuple(a,"Oi",&a1,&a2)) return NULL; \
  return AOP(a1,a2); }

#define spam2n(OP,AOP) static PyObject *OP(s,a) PyObject *s, *a; { \
  PyObject *a1, *a2; \
  if(! PyArg_ParseTuple(a,"OO",&a1,&a2)) return NULL; \
  if(-1 == AOP(a1,a2)) return NULL; \
  Py_INCREF(Py_None); \
  return Py_None; }

#define spam3n(OP,AOP) static PyObject *OP(s,a) PyObject *s, *a; { \
  PyObject *a1, *a2, *a3; \
  if(! PyArg_ParseTuple(a,"OOO",&a1,&a2,&a3)) return NULL; \
  if(-1 == AOP(a1,a2,a3)) return NULL; \
  Py_INCREF(Py_None); \
  return Py_None; }

#define spami(OP,AOP) static PyObject *OP(s,a) PyObject *s, *a; { \
  PyObject *a1; long r; \
  if(! PyArg_ParseTuple(a,"O",&a1)) return NULL; \
  if(-1 == (r=AOP(a1))) return NULL; \
  return PyInt_FromLong(r); }

#define spami2(OP,AOP) static PyObject *OP(s,a) PyObject *s, *a; { \
  PyObject *a1, *a2; long r; \
  if(! PyArg_ParseTuple(a,"OO",&a1,&a2)) return NULL; \
  if(-1 == (r=AOP(a1,a2))) return NULL; \
  return PyInt_FromLong(r); }

spami(isCallable       , PyCallable_Check)
spami(isNumberType     , PyNumber_Check)
spami(truth            , PyObject_IsTrue)
spam2(__add          , PyNumber_Add)
spam2(__sub          , PyNumber_Subtract)
spam2(__mul          , PyNumber_Multiply)
spam2(__div          , PyNumber_Divide)
spam2(__mod          , PyNumber_Remainder)
spam1(__neg          , PyNumber_Negative)
spam1(__pos          , PyNumber_Positive)
spam1(__abs          , PyNumber_Absolute)
spam1(__inv          , PyNumber_Invert)
spam2(__lshift       , PyNumber_Lshift)
spam2(__rshift       , PyNumber_Rshift)
spam2(__and          , PyNumber_And)
spam2(__xor          , PyNumber_Xor)
spam2(__or           , PyNumber_Or)
spami(isSequenceType   , PySequence_Check)
spam2(__concat       , PySequence_Concat)
spamoi(__repeat       , PySequence_Repeat)
spami2(sequenceIncludes, PySequence_In)
spami2(indexOf         , PySequence_Index)
spami2(countOf         , PySequence_Count)
spami(isMappingType    , PyMapping_Check)
spam2(__getitem      , PyObject_GetItem)
spam3n(__setitem     , PyObject_SetItem)

static PyObject*
__getslice(s,a)
     PyObject *s, *a;
{
  PyObject *a1;
  long a2,a3;

  if(! PyArg_ParseTuple(a,"Oii",&a1,&a2,&a3)) return NULL;

  return PySequence_GetSlice(a1,a2,a3);
}

static PyObject*
__setslice(s,a)
     PyObject *s, *a;
{
  PyObject *a1, *a4;
  long a2,a3;

  if(! PyArg_ParseTuple(a,"OiiO",&a1,&a2,&a3,&a4)) return NULL;

  if(-1 == PySequence_SetSlice(a1,a2,a3,a4)) return NULL;

  Py_INCREF(Py_None);
  return Py_None;
}

#undef spam1
#undef spam2
#define spam1(OP,DOC) {#OP, OP, 1, #OP "(o) -- " DOC},
#define spam2(OP,DOC) {#OP, OP, 1, #OP "(a,b) -- " DOC},
#define spam3(OP,DOC) {#OP, OP, 1, #OP "(a,b,c) -- " DOC},

static struct PyMethodDef operator_methods[] = {
spam1(isCallable      , "Return 1 if o is callable, and zero otherwise.")
spam1(isNumberType    , "Return 1 if o has a numeric type, and zero otherwise.")
spam1(isSequenceType  , "Return 1 if o has a sequence type, and zero otherwise.")
spam1(truth           , "Return 1 if o is true, and 0 otherwise.")
spam2(sequenceIncludes, "Return 1 is a includes b, and 0 otherwise.")
spam2(indexOf         , "Return the index of b in a.")
spam2(countOf         , "Return the number of times b occurs in a.")
spam1(isMappingType   , "Return 1 if o has a mapping type, and zero otherwise.")

#undef spam1
#undef spam2
#undef spam3
#define spam1(OP,DOC) {#OP, __ ## OP, 1, #OP "(o) -- " DOC}, \
            {"__" #OP "__", __ ## OP, 1, #OP "(o) -- " DOC}, 
#define spam2(OP,DOC) {#OP, __ ## OP, 1, #OP "(a,b) -- " DOC}, \
            {"__" #OP "__", __ ## OP, 1, #OP "(a,b) -- " DOC}, 
#define spam3(OP,DOC) {#OP, __ ## OP, 1, #OP "(a,b,c) -- " DOC}, \
            {"__" #OP "__", __ ## OP, 1, #OP "(a,b,c) -- " DOC}, 
#define spam4(OP,DOC) {#OP, __ ## OP, 1, #OP "(a,b,c,v) -- " DOC}, \
            {"__" #OP "__", __ ## OP, 1, #OP "(a,b,c,v) -- " DOC}, 


spam2(add         , "Return a + b, for a and b numbers.")
spam2(sub         , "Return a - b.")
spam2(mul         , "Return a * b, for a and b numbers.")
spam2(div         , "Return a / b.")
spam2(mod         , "Return a % b.")
spam1(neg         , "Return o negated.")
spam1(pos         , "Return o positive.")
spam1(abs         , "Return the absolute value of o.")
spam1(inv         , "Return the inverse of o.")
spam2(lshift      , "Return a shifted left by b.")
spam2(rshift      , "Return a shifted right by b.")
spam2(and         , "Return the bitwise and of a and b.")
spam2(xor         , "Return the bitwise exclusive-or of a and b.")
spam2(or          , "Return the bitwise or of a and b.")
spam2(concat      , "Return a + b, for a and b sequences.")
spam2(repeat      , "Return a + b, where a is a sequence, and b is an "
                        "integer.")
spam2(getitem     , "Return the value of a at index b.")
spam3(setitem     , "Set the value of a at b to c.")
spam2(getslice    , "Return the slice of a from b to c-1.")
spam4(setslice    , "Set the slice of a from b to c-1 to the sequence, v.")
	{NULL,		NULL}		/* sentinel */


};


/* Initialization function for the module (*must* be called initoperator) */

void
initoperator()
{
  PyObject *m, *d;
  
  /* Create the module and add the functions */
  m = Py_InitModule4("operator", operator_methods,
		     operator_doc,
		     (PyObject*)NULL,PYTHON_API_VERSION);

  /* Add some symbolic constants to the module */
  d = PyModule_GetDict(m);
  PyDict_SetItemString(d, "__version__",PyString_FromString("$Rev$"));
  
  /* Check for errors */
  if (PyErr_Occurred())
    Py_FatalError("can't initialize module operator");
}
