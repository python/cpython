#ifndef CSTRINGIO_INCLUDED
#define CSTRINGIO_INCLUDED
/*

  cStringIO.h,v 1.4 1997/12/07 14:27:00 jim Exp

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

    PycString_IMPORT

  This would typically be done in your init function.

*/

/* Basic functions to manipulate cStringIO objects from C */

static struct PycStringIO_CAPI {
  
  /* Read a string.  If the last argument is -1, the remainder will be read. */
  int(*cread)(PyObject *, char **, int);

  /* Read a line */
  int(*creadline)(PyObject *, char **);

  /* Write a string */
  int(*cwrite)(PyObject *, char *, int);

  /* Get the cStringIO object as a Python string */
  PyObject *(*cgetvalue)(PyObject *);

  /* Create a new output object */
  PyObject *(*NewOutput)(int);

  /* Create an input object from a Python string */
  PyObject *(*NewInput)(PyObject *);

  /* The Python types for cStringIO input and output objects.
     Note that you can do input on an output object.
     */
  PyTypeObject *InputType, *OutputType;

} * PycStringIO = NULL;

/* These can be used to test if you have one */
#define PycStringIO_InputCheck(O) \
  ((O)->ob_type==PycStringIO->InputType)
#define PycStringIO_OutputCheck(O) \
  ((O)->ob_type==PycStringIO->OutputType)

static void *
xxxPyCObject_Import(char *module_name, char *name)
{
  PyObject *m, *c;
  void *r=NULL;
  
  if((m=PyImport_ImportModule(module_name)))
    {
      if((c=PyObject_GetAttrString(m,name)))
	{
	  r=PyCObject_AsVoidPtr(c);
	  Py_DECREF(c);
	}
      Py_DECREF(m);
    }

  return r;
}

#define PycString_IMPORT \
  PycStringIO=xxxPyCObject_Import("cStringIO", "cStringIO_CAPI")

#endif /* CSTRINGIO_INCLUDED */
