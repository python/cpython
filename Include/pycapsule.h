
/* Capsule objects let you wrap a C "void *" pointer in a Python
   object.  They're a way of passing data through the Python interpreter
   without creating your own custom type.

   Capsules are used for communication between extension modules.
   They provide a way for an extension module to export a C interface
   to other extension modules, so that extension modules can use the
   Python import mechanism to link to one another.

   For more information, please see "c-api/capsule.html" in the
   documentation.
*/

#ifndef Py_CAPSULE_H
#define Py_CAPSULE_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) PyCapsule_Type;

typedef void (*PyCapsule_Destructor)(PyObject *);

#define PyCapsule_CheckExact(op) (Py_TYPE(op) == &PyCapsule_Type)


PyAPI_FUNC(PyObject *) PyCapsule_New(
    void *pointer,
    const char *name,
    PyCapsule_Destructor destructor);

PyAPI_FUNC(void *) PyCapsule_GetPointer(PyObject *capsule, const char *name);

PyAPI_FUNC(PyCapsule_Destructor) PyCapsule_GetDestructor(PyObject *capsule);

PyAPI_FUNC(const char *) PyCapsule_GetName(PyObject *capsule);

PyAPI_FUNC(void *) PyCapsule_GetContext(PyObject *capsule);

PyAPI_FUNC(int) PyCapsule_IsValid(PyObject *capsule, const char *name);

PyAPI_FUNC(int) PyCapsule_SetPointer(PyObject *capsule, void *pointer);

PyAPI_FUNC(int) PyCapsule_SetDestructor(PyObject *capsule, PyCapsule_Destructor destructor);

PyAPI_FUNC(int) PyCapsule_SetName(PyObject *capsule, const char *name);

PyAPI_FUNC(int) PyCapsule_SetContext(PyObject *capsule, void *context);

PyAPI_FUNC(void *) PyCapsule_Import(const char *name, int no_block);


#define PYTHON_USING_CAPSULE

#define PYCAPSULE_INSTANTIATE_DESTRUCTOR(name, destructor) \
static void pycapsule_destructor_ ## name(PyObject *ptr) \
{ \
	void *p = PyCapsule_GetPointer(ptr, name); \
	if (p) { \
		destructor(p); \
	} \
} \

#define PYCAPSULE_NEW(pointer, name) \
	(PyCapsule_New(pointer, name, capsule_destructor_ ## name))

#define PYCAPSULE_ISVALID(capsule, name) \
  (PyCapsule_IsValid(capsule, name))

#define PYCAPSULE_DEREFERENCE(capsule, name) \
  (PyCapsule_GetPointer(capsule, name))

#define PYCAPSULE_SET(capsule, name, value) \
  (PyCapsule_IsValid(capsule, name) && PyCapsule_SetPointer(capsule, value))

/* module and attribute should be specified as string constants */
#define PYCAPSULE_IMPORT(module, attribute) \
  (PyCapsule_Import(module "." attribute, 0))


/* begin public-domain code */
/* 
** This code was written by Larry Hastings, 
** and is dedicated to the public domain. 
** It's designed to make it easy to switch 
** from CObject to Capsule objects without losing 
** backwards compatibility with prior versions 
** of CPython.  You're encouraged to copy this code 
** (including this documentation) into your 
** Python C extension.
** 
** To use: 
**    * #define a name for the pointer you store in
**      the CObject.  If you make the CObject available
**      as part of your module's API, this name should
**      be "modulename.attributename", and it should be
**      considered part of your API (so put it in your
**      header file).
**    * Specify a PYCAPSULE_INSTANTIATE_DESTRUCTOR(), in
**      every C file that creates these CObjects.  This
**      is where you specify your object's destructor.
**    * Change all calls to CObject_FromVoidPtr()
**      and CObject_FromVoidPointerAndDesc() into
**      PYCAPSULE_NEW() calls.
**    * Change all calls to PyCObject_AsVoidPtr()
**      into PYCAPSULE_DEREFERENCE() calls.
**    * Change all calls to PyCObject_SetVoidPtr()
**      into PYCAPSULE_SET() calls.
**    * Change all calls to PyCObject_Import()
**      into PYCAPSULE_IMPORT() calls.  Note that
**      the two arguments to PYCAPSULE_IMPORT()
**      should both be string constants; that is,
**      you should call
**      PYCAPSULE_IMPORT("modulename", "attributename"),
**      not PYCAPSULE_IMPORT(charstar1, charstar2).
*/
#ifndef PYTHON_USING_CAPSULE

#define PYCAPSULE_INSTANTIATE_DESTRUCTOR(name, destructor) \
static void pycapsule_destructor_ ## name(void *ptr) \
{ \
	destructor(p); \
} \

#define PYCAPSULE_NEW(pointer, name) \
	(PyCObject_FromVoidPtr(pointer, pycapsule_destructor_ ## name))

#define PYCAPSULE_ISVALID(capsule, name) \
  (PyCObject_Check(capsule))

#define PYCAPSULE_DEREFERENCE(capsule, name) \
  (PyCObject_AsVoidPtr(capsule))

#define PYCAPSULE_SET(capsule, name, value) \
  (PyCObject_SetVoidPtr(capsule, value))

/* module and attribute should be specified as string constants */
#define PYCAPSULE_IMPORT(module, attribute) \
  (PyCObject_Import(module, attribute))

#endif /* PYTHON_USING_CAPSULE */
/* end public-domain code */

#ifdef __cplusplus
}
#endif
#endif /* !Py_CAPSULE_H */
