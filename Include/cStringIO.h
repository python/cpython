#ifndef CSTRINGIO_INCLUDED
#define CSTRINGIO_INCLUDED
/*

  $Id$

  cStringIO C API

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
   
      info@digicool.com
      Digital Creations L.C.  
   
      (540) 371-6909


  This header provides access to cStringIO objects from C.
  Functions are provided for calling cStringIO objects and
  macros are provided for testing whether you have cStringIO 
  objects.

  Before calling any of the functions or macros, you must initialize
  the routines with:

    PycStringIO_IMPORT

  This would typically be done in your init function.

  $Log$
  Revision 2.2  1997/01/06 22:50:12  guido
  Jim's latest version

  Revision 1.1  1997/01/02 15:18:36  chris
  initial version


*/

/* Basic fuctions to manipulate cStringIO objects from C */

/* Read a string.  If the last argument is -1, the remainder will be read. */
static int(*PycStringIO_cread)(PyObject *, char **, int)=NULL;

/* Read a line */
static int(*PycStringIO_creadline)(PyObject *, char **)=NULL;

/* Write a string */
static int(*PycStringIO_cwrite)(PyObject *, char *, int)=NULL;

/* Get the cStringIO object as a Python string */
static PyObject *(*PycStringIO_cgetvalue)(PyObject *)=NULL;

/* Create a new output object */
static PyObject *(*PycStringIO_NewOutput)(int)=NULL;

/* Create an input object from a Python string */
static PyObject *(*PycStringIO_NewInput)(PyObject *)=NULL;

/* The Python types for cStringIO input and output objects.
   Note that you can do input on an output object.
*/
static PyObject *PycStringIO_InputType=NULL, *PycStringIO_OutputType=NULL;

/* These can be used to test if you have one */
#define PycStringIO_InputCheck(O) \
  ((O)->ob_type==(PyTypeObject*)PycStringIO_InputType)
#define PycStringIO_OutputCheck(O) \
  ((O)->ob_type==(PyTypeObject*)PycStringIO_OutputType)

/* The following is used to implement PycString_IMPORT: */
static PyObject *PycStringIO_Module=NULL, *PycStringIO_CObject=NULL;

#define IMPORT_C_OBJECT(N) \
  if((PycStringIO_CObject=PyObject_GetAttrString(PycStringIO_Module, #N))) { \
    PycStringIO_ ## N = PyCObject_AsVoidPtr(PycStringIO_CObject); \
    Py_DECREF(PycStringIO_CObject); }

#define PycString_IMPORT \
  if((PycStringIO_Module=PyImport_ImportModule("cStringIO"))) { \
    PycStringIO_InputType=PyObject_GetAttrString(PycStringIO_Module, \
						 "InputType"); \
    PycStringIO_OutputType=PyObject_GetAttrString(PycStringIO_Module, \
						  "OutputType"); \
    IMPORT_C_OBJECT(cread); \
    IMPORT_C_OBJECT(creadline); \
    IMPORT_C_OBJECT(cwrite); \
    IMPORT_C_OBJECT(NewInput); \
    IMPORT_C_OBJECT(NewOutput); \
    IMPORT_C_OBJECT(cgetvalue);   }

#endif /* CSTRINGIO_INCLUDED */
