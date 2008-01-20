.. highlightlang:: c

.. _cobjects:

CObjects
--------

.. index:: object: CObject

Refer to :ref:`using-cobjects` for more information on using these objects.


.. ctype:: PyCObject

   This subtype of :ctype:`PyObject` represents an opaque value, useful for C
   extension modules who need to pass an opaque value (as a :ctype:`void\*`
   pointer) through Python code to other C code.  It is often used to make a C
   function pointer defined in one module available to other modules, so the
   regular import mechanism can be used to access C APIs defined in dynamically
   loaded modules.


.. cfunction:: int PyCObject_Check(PyObject *p)

   Return true if its argument is a :ctype:`PyCObject`.


.. cfunction:: PyObject* PyCObject_FromVoidPtr(void* cobj, void (*destr)(void *))

   Create a :ctype:`PyCObject` from the ``void *`` *cobj*.  The *destr* function
   will be called when the object is reclaimed, unless it is *NULL*.


.. cfunction:: PyObject* PyCObject_FromVoidPtrAndDesc(void* cobj, void* desc, void (*destr)(void *, void *))

   Create a :ctype:`PyCObject` from the :ctype:`void \*` *cobj*.  The *destr*
   function will be called when the object is reclaimed. The *desc* argument can
   be used to pass extra callback data for the destructor function.


.. cfunction:: void* PyCObject_AsVoidPtr(PyObject* self)

   Return the object :ctype:`void \*` that the :ctype:`PyCObject` *self* was
   created with.


.. cfunction:: void* PyCObject_GetDesc(PyObject* self)

   Return the description :ctype:`void \*` that the :ctype:`PyCObject` *self* was
   created with.


.. cfunction:: int PyCObject_SetVoidPtr(PyObject* self, void* cobj)

   Set the void pointer inside *self* to *cobj*. The :ctype:`PyCObject` must not
   have an associated destructor. Return true on success, false on failure.
