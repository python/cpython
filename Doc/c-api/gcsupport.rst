.. highlight:: c

.. _supporting-cycle-detection:

Supporting Cyclic Garbage Collection
====================================

Python's support for detecting and collecting garbage which involves circular
references requires support from object types which are "containers" for other
objects which may also be containers.  Types which do not store references to
other objects, or which only store references to atomic types (such as numbers
or strings), do not need to provide any explicit support for garbage
collection.

To create a container type, the :c:member:`~PyTypeObject.tp_flags` field of the type object must
include the :c:macro:`Py_TPFLAGS_HAVE_GC` and provide an implementation of the
:c:member:`~PyTypeObject.tp_traverse` handler.  If instances of the type are mutable, a
:c:member:`~PyTypeObject.tp_clear` implementation must also be provided.


:c:macro:`Py_TPFLAGS_HAVE_GC`
   Objects with a type with this flag set must conform with the rules
   documented here.  For convenience these objects will be referred to as
   container objects.

Constructors for container types must conform to two rules:

#. The memory for the object must be allocated using :c:macro:`PyObject_GC_New`
   or :c:macro:`PyObject_GC_NewVar`.

#. Once all the fields which may contain references to other containers are
   initialized, it must call :c:func:`PyObject_GC_Track`.

Similarly, the deallocator for the object must conform to a similar pair of
rules:

#. Before fields which refer to other containers are invalidated,
   :c:func:`PyObject_GC_UnTrack` must be called.

#. The object's memory must be deallocated using :c:func:`PyObject_GC_Del`.

   .. warning::
      If a type adds the Py_TPFLAGS_HAVE_GC, then it *must* implement at least
      a :c:member:`~PyTypeObject.tp_traverse` handler or explicitly use one
      from its subclass or subclasses.

      When calling :c:func:`PyType_Ready` or some of the APIs that indirectly
      call it like :c:func:`PyType_FromSpecWithBases` or
      :c:func:`PyType_FromSpec` the interpreter will automatically populate the
      :c:member:`~PyTypeObject.tp_flags`, :c:member:`~PyTypeObject.tp_traverse`
      and :c:member:`~PyTypeObject.tp_clear` fields if the type inherits from a
      class that implements the garbage collector protocol and the child class
      does *not* include the :c:macro:`Py_TPFLAGS_HAVE_GC` flag.

.. c:macro:: PyObject_GC_New(TYPE, typeobj)

   Analogous to :c:macro:`PyObject_New` but for container objects with the
   :c:macro:`Py_TPFLAGS_HAVE_GC` flag set.

.. c:macro:: PyObject_GC_NewVar(TYPE, typeobj, size)

   Analogous to :c:macro:`PyObject_NewVar` but for container objects with the
   :c:macro:`Py_TPFLAGS_HAVE_GC` flag set.

.. c:function:: PyObject* PyUnstable_Object_GC_NewWithExtraData(PyTypeObject *type, size_t extra_size)

   Analogous to :c:macro:`PyObject_GC_New` but allocates *extra_size*
   bytes at the end of the object (at offset
   :c:member:`~PyTypeObject.tp_basicsize`).
   The allocated memory is initialized to zeros,
   except for the :c:type:`Python object header <PyObject>`.

   The extra data will be deallocated with the object, but otherwise it is
   not managed by Python.

   .. warning::
      The function is marked as unstable because the final mechanism
      for reserving extra data after an instance is not yet decided.
      For allocating a variable number of fields, prefer using
      :c:type:`PyVarObject` and :c:member:`~PyTypeObject.tp_itemsize`
      instead.

   .. versionadded:: 3.12


.. c:macro:: PyObject_GC_Resize(TYPE, op, newsize)

   Resize an object allocated by :c:macro:`PyObject_NewVar`.
   Returns the resized object of type ``TYPE*`` (refers to any C type)
   or ``NULL`` on failure.

   *op* must be of type :c:expr:`PyVarObject *`
   and must not be tracked by the collector yet.
   *newsize* must be of type :c:type:`Py_ssize_t`.


.. c:function:: void PyObject_GC_Track(PyObject *op)

   Adds the object *op* to the set of container objects tracked by the
   collector.  The collector can run at unexpected times so objects must be
   valid while being tracked.  This should be called once all the fields
   followed by the :c:member:`~PyTypeObject.tp_traverse` handler become valid, usually near the
   end of the constructor.


.. c:function:: int PyObject_IS_GC(PyObject *obj)

   Returns non-zero if the object implements the garbage collector protocol,
   otherwise returns 0.

   The object cannot be tracked by the garbage collector if this function returns 0.


.. c:function:: int PyObject_GC_IsTracked(PyObject *op)

   Returns 1 if the object type of *op* implements the GC protocol and *op* is being
   currently tracked by the garbage collector and 0 otherwise.

   This is analogous to the Python function :func:`gc.is_tracked`.

   .. versionadded:: 3.9


.. c:function:: int PyObject_GC_IsFinalized(PyObject *op)

   Returns 1 if the object type of *op* implements the GC protocol and *op* has been
   already finalized by the garbage collector and 0 otherwise.

   This is analogous to the Python function :func:`gc.is_finalized`.

   .. versionadded:: 3.9


.. c:function:: void PyObject_GC_Del(void *op)

   Releases memory allocated to an object using :c:macro:`PyObject_GC_New` or
   :c:macro:`PyObject_GC_NewVar`.


.. c:function:: void PyObject_GC_UnTrack(void *op)

   Remove the object *op* from the set of container objects tracked by the
   collector.  Note that :c:func:`PyObject_GC_Track` can be called again on
   this object to add it back to the set of tracked objects.  The deallocator
   (:c:member:`~PyTypeObject.tp_dealloc` handler) should call this for the object before any of
   the fields used by the :c:member:`~PyTypeObject.tp_traverse` handler become invalid.


.. versionchanged:: 3.8

   The :c:func:`!_PyObject_GC_TRACK` and :c:func:`!_PyObject_GC_UNTRACK` macros
   have been removed from the public C API.

The :c:member:`~PyTypeObject.tp_traverse` handler accepts a function parameter of this type:


.. c:type:: int (*visitproc)(PyObject *object, void *arg)

   Type of the visitor function passed to the :c:member:`~PyTypeObject.tp_traverse` handler.
   The function should be called with an object to traverse as *object* and
   the third parameter to the :c:member:`~PyTypeObject.tp_traverse` handler as *arg*.  The
   Python core uses several visitor functions to implement cyclic garbage
   detection; it's not expected that users will need to write their own
   visitor functions.

The :c:member:`~PyTypeObject.tp_traverse` handler must have the following type:


.. c:type:: int (*traverseproc)(PyObject *self, visitproc visit, void *arg)

   Traversal function for a container object.  Implementations must call the
   *visit* function for each object directly contained by *self*, with the
   parameters to *visit* being the contained object and the *arg* value passed
   to the handler.  The *visit* function must not be called with a ``NULL``
   object argument.  If *visit* returns a non-zero value that value should be
   returned immediately.

To simplify writing :c:member:`~PyTypeObject.tp_traverse` handlers, a :c:func:`Py_VISIT` macro is
provided.  In order to use this macro, the :c:member:`~PyTypeObject.tp_traverse` implementation
must name its arguments exactly *visit* and *arg*:


.. c:function:: void Py_VISIT(PyObject *o)

   If *o* is not ``NULL``, call the *visit* callback, with arguments *o*
   and *arg*.  If *visit* returns a non-zero value, then return it.
   Using this macro, :c:member:`~PyTypeObject.tp_traverse` handlers
   look like::

      static int
      my_traverse(Noddy *self, visitproc visit, void *arg)
      {
          Py_VISIT(self->foo);
          Py_VISIT(self->bar);
          return 0;
      }

The :c:member:`~PyTypeObject.tp_clear` handler must be of the :c:type:`inquiry` type, or ``NULL``
if the object is immutable.


.. c:type:: int (*inquiry)(PyObject *self)

   Drop references that may have created reference cycles.  Immutable objects
   do not have to define this method since they can never directly create
   reference cycles.  Note that the object must still be valid after calling
   this method (don't just call :c:func:`Py_DECREF` on a reference).  The
   collector will call this method if it detects that this object is involved
   in a reference cycle.


Controlling the Garbage Collector State
---------------------------------------

The C-API provides the following functions for controlling
garbage collection runs.

.. c:function:: Py_ssize_t PyGC_Collect(void)

   Perform a full garbage collection, if the garbage collector is enabled.
   (Note that :func:`gc.collect` runs it unconditionally.)

   Returns the number of collected + unreachable objects which cannot
   be collected.
   If the garbage collector is disabled or already collecting,
   returns ``0`` immediately.
   Errors during garbage collection are passed to :data:`sys.unraisablehook`.
   This function does not raise exceptions.


.. c:function:: int PyGC_Enable(void)

   Enable the garbage collector: similar to :func:`gc.enable`.
   Returns the previous state, 0 for disabled and 1 for enabled.

   .. versionadded:: 3.10


.. c:function:: int PyGC_Disable(void)

   Disable the garbage collector: similar to :func:`gc.disable`.
   Returns the previous state, 0 for disabled and 1 for enabled.

   .. versionadded:: 3.10


.. c:function:: int PyGC_IsEnabled(void)

   Query the state of the garbage collector: similar to :func:`gc.isenabled`.
   Returns the current state, 0 for disabled and 1 for enabled.

   .. versionadded:: 3.10


Querying Garbage Collector State
--------------------------------

The C-API provides the following interface for querying information about
the garbage collector.

.. c:function:: void PyUnstable_GC_VisitObjects(gcvisitobjects_t callback, void *arg)

   Run supplied *callback* on all live GC-capable objects. *arg* is passed through to
   all invocations of *callback*.

   .. warning::
      If new objects are (de)allocated by the callback it is undefined if they
      will be visited.

      Garbage collection is disabled during operation. Explicitly running a collection
      in the callback may lead to undefined behaviour e.g. visiting the same objects
      multiple times or not at all.

   .. versionadded:: 3.12

.. c:type:: int (*gcvisitobjects_t)(PyObject *object, void *arg)

   Type of the visitor function to be passed to :c:func:`PyUnstable_GC_VisitObjects`.
   *arg* is the same as the *arg* passed to ``PyUnstable_GC_VisitObjects``.
   Return ``0`` to continue iteration, return ``1`` to stop iteration. Other return
   values are reserved for now so behavior on returning anything else is undefined.

   .. versionadded:: 3.12


