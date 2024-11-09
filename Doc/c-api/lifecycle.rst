.. highlight:: c

.. _life-cycle:

Object Life Cycle
=================

Life Events
-----------

The figure below illustrates the order of events that can occur throughout an
object's life.  An arrow from *A* to *B* indicates that event *B* can occur
after event *A* has occurred, with the arrow's label indicating the condition
that must be true for *B* to occur after *A*.

.. raw:: html

   <style type="text/css">

.. raw:: html
   :file: lifecycle.dot.css

.. raw:: html

   </style>

.. raw:: html
   :file: lifecycle.dot.svg

.. raw:: html

   <script>
       (() => {
           const g = document.getElementById('life_events_graph');
           const title = g.querySelector(':scope > title');
           title.id = 'life-events-graph-title';
           const svg = g.closest('svg');
           svg.role = 'img';
           svg.setAttribute('aria-describedby',
                            'life-events-graph-description');
           svg.setAttribute('aria-labelledby', 'life-events-graph-title');
       })();
   </script>

.. container::
   :name: life-events-graph-description

   Explanation:

   * When a new object is constructed by calling its type:

     * :c:member:`~PyTypeObject.tp_new` is called to create a new object.
     * :c:member:`~PyTypeObject.tp_alloc` is directly called by
       :c:member:`~PyTypeObject.tp_new` to allocate the memory for the new
       object.
     * :c:member:`~PyTypeObject.tp_init` initializes the newly created object.
       :c:member:`!tp_init` can be called again to re-initialize an object, if
       desired.

   * After :c:member:`!tp_init` completes, the object is ready to use.
   * After the last reference to an object is removed:

     #. :c:member:`~PyTypeObject.tp_finalize` might be called to finalize the
        object.  :term:`CPython` currently does not finalize an object when the
        last reference to it is deleted.  Also, for types that support garbage
        collection (the :c:macro:`Py_TPFLAGS_HAVE_GC` flag is set), CPython
        currently never finalizes the same object twice, even if it has been
        resurrected.  These behaviors may change in the future.
     #. If the object was previously finalized,
        :c:member:`~PyTypeObject.tp_clear` might be called to clear references
        held by the object.  :term:`CPython` currently does not clear an object
        in response to the deletion of the last reference, but this may change
        in the future.
     #. :c:member:`~PyTypeObject.tp_dealloc` is called to destroy the object.

   * The :c:member:`~PyTypeObject.tp_finalize` function can optionally add a
     reference to the object, *resurrecting* it and preventing its pending
     destruction.  (Only :c:member:`!tp_finalize` can resurrect an object;
     :c:member:`~PyTypeObject.tp_clear` and :c:member:`~PyTypeObject.tp_dealloc`
     cannot.)
   * :c:member:`~PyTypeObject.tp_dealloc` can optionally call
     :c:member:`~PyTypeObject.tp_finalize` via
     :c:func:`PyObject_CallFinalizerFromDealloc` if it wishes to reuse that
     code to help with object destruction.  This is recommended because it
     guarantees that :c:member:`!tp_finalize` is always called before
     destruction.
   * When :c:member:`~PyTypeObject.tp_dealloc` finishes object destruction, it
     directly calls :c:member:`~PyTypeObject.tp_free` to deallocate the memory.
   * If the object is a member of a :term:`cyclic isolate` and
     :c:member:`~PyTypeObject.tp_clear` fails to break the reference cycle (or
     that function is not called), the objects remain indefinitely uncollectable
     (they "leak").  See :data:`gc.garbage`.

   If the object is marked as supporting garbage collection (the
   :c:macro:`Py_TPFLAGS_HAVE_GC` flag is set in
   :c:member:`~PyTypeObject.tp_flags`), the following events are also possible:

   * The garbage collector occasionally calls
     :c:member:`~PyTypeObject.tp_traverse` to identify :term:`cyclic isolates
     <cyclic isolate>`.
   * When the garbage collector discovers a :term:`cyclic isolate`, it
     finalizes one of the objects in the group by calling its
     :c:member:`~PyTypeObject.tp_finalize` function.  This repeats until the
     cyclic isolate doesn't exist or all of the objects have been finalized.
   * :c:member:`~PyTypeObject.tp_finalize` can resurrect the object by adding a
     reference from outside the :term:`cyclic isolate`.  The new reference
     causes the group of objects to no longer form a cyclic isolate (the
     reference cycle may still exist, but the objects are no longer isolated).
   * When the garbage collector discovers a :term:`cyclic isolate` and all of
     the objects in the group have already been finalized, the garbage
     collector clears one or more of the uncleared objects in the group
     (possibly concurrently, but with the :term:`GIL` held :ref:`unless
     disabled <whatsnew313-free-threaded-cpython>`) by calling each's
     :c:member:`~PyTypeObject.tp_clear` function.  This repeats as long as the
     cyclic isolate still exists and not all of the objects have been cleared.


Cyclic Isolate Destruction
--------------------------

Listed below are the stages of life of a hypothetical :term:`cyclic isolate`
that continues to exist after each member object is finalized or cleared.  It
is a bug if a cyclic isolate progresses through all of these stages; it should
vanish once all objects are cleared, if not sooner.  A cyclic isolate can
vanish either because the reference cycle is broken or because the objects are
no longer isolated due to finalizer resurrection (see
:c:member:`~PyTypeObject.tp_finalize`).

0. **Reachable** (pre-cyclic isolate): All objects are in their normal,
   reachable state.  A reference cycle exists, but an external reference means
   the objects are not yet isolated.
#. **Unreachable but consistent:** The final reference from outside the group
   of objects has been removed, causing the objects to become isolated (thus a
   cyclic isolate is born).  None of the group's objects have been finalized or
   cleared yet.
#. **Mix of finalized and not finalized:** Objects in a cyclic isolate are
   finalized one at a time, which means that there is a period of time when the
   cyclic isolate is composed of a mix of finalized and non-finalized objects.
   Finalization order is unspecified, so it can appear random.  A finalized
   object must behave in a sane manner when non-finalized objects interact with
   it, and a non-finalized object must be able to tolerate the finalization of
   an arbitrary subset of its referents.
#. **All finalized:** All objects in a cyclic isolate are finalized before any
   of them are cleared.
#. **Mix of finalized and cleared:** The objects can be cleared serially or
   concurrently (but with the :term:`GIL` held :ref:`unless disabled
   <whatsnew313-free-threaded-cpython>`); either way, some will finish before
   others.  A finalized object must be able to tolerate the clearing of a
   subset of its referents.  :pep:`442` calls this stage "cyclic trash".
#. **Leaked:** If a cyclic isolate still exists after all objects in the group
   have been finalized and cleared, then the objects remain indefinitely
   uncollectable (see :data:`gc.garbage`).  It is a bug if a cyclic isolate
   reaches this stage---it means the :c:member:`~PyTypeObject.tp_clear` methods
   of the participating objects have failed to break the reference cycle as
   required.

If :c:member:`~PyTypeObject.tp_clear` did not exist, then Python would have no
way to break a reference cycle.  The destruction of an object in a cyclic
isolate would result in a dangling pointer, triggering undefined behavior when
an object referencing the destroyed object is itself destroyed.

:c:member:`~PyTypeObject.tp_finalize` is not needed to safely destroy a cyclic
isolate, but its existence makes it easier to design types that behave in a
sane manner when objects are cleared.  Clearing an object might necessarily
leave it in a broken state---it might be unsafe to call any of the cleared
object's methods or access any of its attributes.  With finalization, only
finalized objects can possibly interact with cleared objects; non-finalized
objects are guaranteed to interact with only non-cleared (but potentially
finalized) objects.

To summarize the possible interactions:

* A non-finalized object might have references to or from non-finalized and
  finalized objects, but not to or from cleared objects.
* A finalized object might have references to or from non-finalized, finalized,
  and cleared objects.
* A cleared object might have references to or from finalized and cleared
  objects, but not to or from non-finalized objects.

Without any reference cycles, an object can be simply destroyed once its last
reference is deleted; the finalization and clearing steps are not necessary to
safely reclaim unused objects.  However, it can be useful to automatically call
:c:member:`~PyTypeObject.tp_finalize` and :c:member:`~PyTypeObject.tp_clear`
before destruction anyway because type design is simplified when all objects
always experience the same series of events regardless of whether they
participated in a cyclic isolate.  :term:`CPython` currently only calls
:c:member:`~PyTypeObject.tp_finalize` and :c:member:`~PyTypeObject.tp_clear` as
needed to destroy a cyclic isolate; this may change in a future version.


Functions
---------

To allocate and free memory, see :ref:`allocating-objects`.


.. c:function:: void PyObject_CallFinalizer(PyObject *op)

   Finalizes the object as described in :c:member:`~PyTypeObject.tp_finalize`.
   Call this function (or :c:func:`PyObject_CallFinalizerFromDealloc`) instead
   of calling :c:member:`~PyTypeObject.tp_finalize` directly because this
   function can deduplicate multiple calls to :c:member:`!tp_finalize`.


.. c:function:: int PyObject_CallFinalizerFromDealloc(PyObject *op)

   Same as :c:func:`PyObject_CallFinalizer` but meant to be called at the
   beginning of the object's destructor (:c:member:`~PyTypeObject.tp_dealloc`).
   The object's reference count must already be 0.  If the object's finalizer
   resurrects the object, this function returns -1; no further destruction
   should happen.  Otherwise, this function returns 0 and destruction can
   continue normally.
