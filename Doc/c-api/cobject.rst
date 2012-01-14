.. highlightlang:: c

.. _cobjects:

CObjects
--------

.. index:: object: CObject


.. warning::

   The CObject API is deprecated as of Python 2.7.  Please switch to the new
   :ref:`capsules` API.

.. c:type:: PyCObject

   This subtype of :c:type:`PyObject` represents an opaque value, useful for C
   extension modules who need to pass an opaque value (as a :c:type:`void\*`
   pointer) through Python code to other C code.  It is often used to make a C
   function pointer defined in one module available to other modules, so the
   regular import mechanism can be used to access C APIs defined in dynamically
   loaded modules.


.. c:function:: int PyCObject_Check(PyObject *p)

   Return true if its argument is a :c:type:`PyCObject`.


.. c:function:: PyObject* PyCObject_FromVoidPtr(void* cobj, void (*destr)(void *))

   Create a :c:type:`PyCObject` from the ``void *`` *cobj*.  The *destr* function
   will be called when the object is reclaimed, unless it is *NULL*.


.. c:function:: PyObject* PyCObject_FromVoidPtrAndDesc(void* cobj, void* desc, void (*destr)(void *, void *))

   Create a :c:type:`PyCObject` from the :c:type:`void \*` *cobj*.  The *destr*
   function will be called when the object is reclaimed. The *desc* argument can
   be used to pass extra callback data for the destructor function.


.. c:function:: void* PyCObject_AsVoidPtr(PyObject* self)

   Return the object :c:type:`void \*` that the :c:type:`PyCObject` *self* was
   created with.


.. c:function:: void* PyCObject_GetDesc(PyObject* self)

   Return the description :c:type:`void \*` that the :c:type:`PyCObject` *self* was
   created with.


.. c:function:: int PyCObject_SetVoidPtr(PyObject* self, void* cobj)

   Set the void pointer inside *self* to *cobj*. The :c:type:`PyCObject` must not
   have an associated destructor. Return true on success, false on failure.
