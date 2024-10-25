.. highlight:: c

.. _life-cycle:

Object Life Cycle
=================

Stages
------

The following is an illustration of the stages of life of an object.  Arrows
indicate a "happens before" relationship.  Octagons indicate functions specific
to :ref:`garbage collection support <supporting-cycle-detection>`.

.. raw:: html
   :file: lifecycle.dot.svg

Explanation:

* :c:member:`~PyTypeObject.tp_alloc`, :c:member:`~PyTypeObject.tp_new`, and
  :c:member:`~PyTypeObject.tp_init` are called to allocate memory for a new
  object and initialize the object.
* If the reference count for an object drops to 0,
  :c:member:`~PyTypeObject.tp_dealloc` is called to destroy the object.
* :c:member:`~PyTypeObject.tp_dealloc` can optionally call
  :c:member:`~PyTypeObject.tp_finalize` (if non-``NULL``) via
  :c:func:`PyObject_CallFinalizerFromDealloc` if it wishes to reuse that code
  to help with object destruction.
* :c:member:`~PyTypeObject.tp_finalize` may increase the object's reference
  count, halting the destruction.  The object is said to be resurrected.
* :c:member:`~PyTypeObject.tp_dealloc` can optionally call
  :c:member:`~PyTypeObject.tp_clear` (if non-``NULL``) if it wishes to reuse
  that code to help with object destruction.
* When :c:member:`~PyTypeObject.tp_dealloc` finishes object destruction, it
  directly calls :c:member:`~PyTypeObject.tp_free` to deallocate the memory.

If the object is marked as supporting garbage collection (the
:c:macro:`Py_TPFLAGS_HAVE_GC` flag is set in
:c:member:`~PyTypeObject.tp_flags`), the following stages are also possible:

* The garbage collector occasionally calls
  :c:member:`~PyTypeObject.tp_traverse` to identify :term:`cyclic isolates
  <cyclic isolate>`.
* When the garbage collector discovers a cyclic isolate, it finalizes one of
  the objects in the group by calling its :c:member:`~PyTypeObject.tp_finalize`
  function.  This repeats until the cyclic isolate doesn't exist or all of the
  objects have been finalized.
* The :c:member:`~PyTypeObject.tp_finalize` function can optionally increase
  the object's reference count, causing it (and other objects it references) to
  become resurrected and no longer a member of a cyclic isolate.
* When the garbage collector discovers a cyclic isolate and all of the objects
  in the group have already been finalized, the garbage collector clears one of
  the objects in the group by calling its :c:member:`~PyTypeObject.tp_clear`
  function.  This repeats until the cyclic isolate doesn't exist or all of the
  objects have been cleared.


Functions
---------

To allocate and free memory, see :ref:`Allocating Objects on the Heap
<allocating-objects>`.


.. c:function:: void PyObject_CallFinalizer(PyObject *op)

   Calls the object's finalizer (:c:member:`~PyTypeObject.tp_finalize`) if it
   has not already been called.


.. c:function:: int PyObject_CallFinalizerFromDealloc(PyObject *op)

   Calls the object's finalizer (:c:member:`~PyTypeObject.tp_finalize`) if it
   has not already been called.  This function is intended to be called at the
   beginning of the object's destructor (:c:member:`~PyTypeObject.tp_dealloc`).
   The object's reference count must already be 0.  If the object's finalizer
   increases the object's reference count, the object is resurrected and this
   function returns -1; no further destruction should happen.  Otherwise, this
   function returns 0 and destruction can continue normally.
