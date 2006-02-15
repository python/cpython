#ifndef Py_CSTRINGIO_H
#define Py_CSTRINGIO_H
#ifdef __cplusplus
extern "C" {
#endif
/*

  This header provides access to cStringIO objects from C.
  Functions are provided for calling cStringIO objects and
  macros are provided for testing whether you have cStringIO 
  objects.

  Before calling any of the functions or macros, you must initialize
  the routines with:

    PycString_IMPORT

  This would typically be done in your init function.

*/
#define PycString_IMPORT \
  PycStringIO = (struct PycStringIO_CAPI*)PyCObject_Import("cStringIO", \
                                                           "cStringIO_CAPI")

/* Basic functions to manipulate cStringIO objects from C */

static struct PycStringIO_CAPI {
  
 /* Read a string from an input object.  If the last argument
    is -1, the remainder will be read.
    */
  int(*cread)(PyObject *, char **, Py_ssize_t);

 /* Read a line from an input object.  Returns the length of the read
    line as an int and a pointer inside the object buffer as char** (so
    the caller doesn't have to provide its own buffer as destination).
    */
  int(*creadline)(PyObject *, char **);

  /* Write a string to an output object*/
  int(*cwrite)(PyObject *, const char *, Py_ssize_t);

  /* Get the output object as a Python string (returns new reference). */
  PyObject *(*cgetvalue)(PyObject *);

  /* Create a new output object */
  PyObject *(*NewOutput)(int);

  /* Create an input object from a Python string
     (copies the Python string reference).
     */
  PyObject *(*NewInput)(PyObject *);

  /* The Python types for cStringIO input and output objects.
     Note that you can do input on an output object.
     */
  PyTypeObject *InputType, *OutputType;

} *PycStringIO;

/* These can be used to test if you have one */
#define PycStringIO_InputCheck(O) \
  ((O)->ob_type==PycStringIO->InputType)
#define PycStringIO_OutputCheck(O) \
  ((O)->ob_type==PycStringIO->OutputType)

#ifdef __cplusplus
}
#endif
#endif /* !Py_CSTRINGIO_H */
