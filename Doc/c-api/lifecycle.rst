.. highlight:: c

.. _life-cycle:

Object Life Cycle
=================

This section explains how a type's slots relate to each other throughout the
life of an object.  It is not intended to be a complete canonical reference for
the slots; instead, refer to the slot-specific documentation in
:ref:`type-structs` for details about a particular slot.


Life Events
-----------

The figure below illustrates the order of events that can occur throughout an
object's life.  An arrow from *A* to *B* indicates that event *B* can occur
after event *A* has occurred, with the arrow's label indicating the condition
that must be true for *B* to occur after *A*.

.. only:: html and not epub

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

.. only:: epub or not (html or latex)

   .. image:: lifecycle.dot.svg
      :align: center
      :class: invert-in-dark-mode
      :alt: Diagram showing events in an object's life.  Explained in detail below.

.. only:: latex

   .. image:: lifecycle.dot.pdf
      :align: center
      :class: invert-in-dark-mode
      :alt: Diagram showing events in an object's life.  Explained in detail below.

.. container::
   :name: life-events-graph-description

   Explanation:

   * When a new object is constructed by calling its type:

     #. :c:member:`~PyTypeObject.tp_new` is called to create a new object.
     #. :c:member:`~PyTypeObject.tp_alloc` is directly called by
        :c:member:`~PyTypeObject.tp_new` to allocate the memory for the new
        object.
     #. :c:member:`~PyTypeObject.tp_init` initializes the newly created object.
        :c:member:`!tp_init` can be called again to re-initialize an object, if
        desired. The :c:member:`!tp_init` call can also be skipped entirely,
        for example by Python code calling :py:meth:`~object.__new__`.

   * After :c:member:`!tp_init` completes, the object is ready to use.
   * Some time after the last reference to an object is removed:

     #. If an object is not marked as *finalized*, it might be finalized by
        marking it as *finalized* and calling its
        :c:member:`~PyTypeObject.tp_finalize` function.  Python does
        *not* finalize an object when the last reference to it is deleted; use
        :c:func:`PyObject_CallFinalizerFromDealloc` to ensure that
        :c:member:`~PyTypeObject.tp_finalize` is always called.
     #. If the object is marked as finalized,
        :c:member:`~PyTypeObject.tp_clear` might be called by the garbage collector
        to clear references held by the object.  It is *not* called when the
        object's reference count reaches zero.
     #. :c:member:`~PyTypeObject.tp_dealloc` is called to destroy the object.
        To avoid code duplication, :c:member:`~PyTypeObject.tp_dealloc` typically
        calls into :c:member:`~PyTypeObject.tp_clear` to free up the object's
        references.
     #. When :c:member:`~PyTypeObject.tp_dealloc` finishes object destruction,
        it directly calls :c:member:`~PyTypeObject.tp_free` (usually set to
        :c:func:`PyObject_Free` or :c:func:`PyObject_GC_Del` automatically as
        appropriate for the type) to deallocate the memory.

   * The :c:member:`~PyTypeObject.tp_finalize` function is permitted to add a
     reference to the object if desired.  If it does, the object is
     *resurrected*, preventing its pending destruction.  (Only
     :c:member:`!tp_finalize` is allowed to resurrect an object;
     :c:member:`~PyTypeObject.tp_clear` and
     :c:member:`~PyTypeObject.tp_dealloc` cannot without calling into
     :c:member:`!tp_finalize`.)  Resurrecting an object may
     or may not cause the object's *finalized* mark to be removed.  Currently,
     Python does not remove the *finalized* mark from a resurrected object if
     it supports garbage collection (i.e., the :c:macro:`Py_TPFLAGS_HAVE_GC`
     flag is set) but does remove the mark if the object does not support
     garbage collection; either or both of these behaviors may change in the
     future.
   * :c:member:`~PyTypeObject.tp_dealloc` can optionally call
     :c:member:`~PyTypeObject.tp_finalize` via
     :c:func:`PyObject_CallFinalizerFromDealloc` if it wishes to reuse that
     code to help with object destruction.  This is recommended because it
     guarantees that :c:member:`!tp_finalize` is always called before
     destruction.  See the :c:member:`~PyTypeObject.tp_dealloc` documentation
     for example code.
   * If the object is a member of a :term:`cyclic isolate` and either
     :c:member:`~PyTypeObject.tp_clear` fails to break the reference cycle or
     the cyclic isolate is not detected (perhaps :func:`gc.disable` was called,
     or the :c:macro:`Py_TPFLAGS_HAVE_GC` flag was erroneously omitted in one
     of the involved types), the objects remain indefinitely uncollectable
     (they "leak").  See :data:`gc.garbage`.

   If the object is marked as supporting garbage collection (the
   :c:macro:`Py_TPFLAGS_HAVE_GC` flag is set in
   :c:member:`~PyTypeObject.tp_flags`), the following events are also possible:

   * The garbage collector occasionally calls
     :c:member:`~PyTypeObject.tp_traverse` to identify :term:`cyclic isolates
     <cyclic isolate>`.
   * When the garbage collector discovers a :term:`cyclic isolate`, it
     finalizes one of the objects in the group by marking it as *finalized* and
     calling its :c:member:`~PyTypeObject.tp_finalize` function, if it has one.
     This repeats until the cyclic isolate doesn't exist or all of the objects
     have been finalized.
   * :c:member:`~PyTypeObject.tp_finalize` is permitted to resurrect the object
     by adding a reference from outside the :term:`cyclic isolate`.  The new
     reference causes the group of objects to no longer form a cyclic isolate
     (the reference cycle may still exist, but if it does the objects are no
     longer isolated).
   * When the garbage collector discovers a :term:`cyclic isolate` and all of
     the objects in the group have already been marked as *finalized*, the
     garbage collector clears one or more of the uncleared objects in the group
     (possibly concurrently) by calling each's
     :c:member:`~PyTypeObject.tp_clear` function.  This repeats as long as the
     cyclic isolate still exists and not all of the objects have been cleared.


Cyclic Isolate Destruction
--------------------------

Listed below are the stages of life of a hypothetical :term:`cyclic isolate`
that continues to exist after each member object is finalized or cleared.  It
is a memory leak if a cyclic isolate progresses through all of these stages; it should
vanish once all objects are cleared, if not sooner.  A cyclic isolate can
vanish either because the reference cycle is broken or because the objects are
no longer isolated due to finalizer resurrection (see
:c:member:`~PyTypeObject.tp_finalize`).

0. **Reachable** (not yet a cyclic isolate): All objects are in their normal,
   reachable state.  A reference cycle could exist, but an external reference
   means the objects are not yet isolated.
#. **Unreachable but consistent:** The final reference from outside the cyclic
   group of objects has been removed, causing the objects to become isolated
   (thus a cyclic isolate is born).  None of the group's objects have been
   finalized or cleared yet.  The cyclic isolate remains at this stage until
   some future run of the garbage collector (not necessarily the next run
   because the next run might not scan every object).
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
   concurrently (but with the :term:`GIL` held); either way, some will finish
   before others.  A finalized object must be able to tolerate the clearing of
   a subset of its referents.  :pep:`442` calls this stage "cyclic trash".
#. **Leaked:** If a cyclic isolate still exists after all objects in the group
   have been finalized and cleared, then the objects remain indefinitely
   uncollectable (see :data:`gc.garbage`).  It is a bug if a cyclic isolate
   reaches this stage---it means the :c:member:`~PyTypeObject.tp_clear` methods
   of the participating objects have failed to break the reference cycle as
   required.

If :c:member:`~PyTypeObject.tp_clear` did not exist, then Python would have no
way to safely break a reference cycle.  Simply destroying an object in a cyclic
isolate would result in a dangling pointer, triggering undefined behavior when
an object referencing the destroyed object is itself destroyed.  The clearing
step makes object destruction a two-phase process: first
:c:member:`~PyTypeObject.tp_clear` is called to partially destroy the objects
enough to detangle them from each other, then
:c:member:`~PyTypeObject.tp_dealloc` is called to complete the destruction.

Unlike clearing, finalization is not a phase of destruction.  A finalized
object must still behave properly by continuing to fulfill its design
contracts.  An object's finalizer is allowed to execute arbitrary Python code,
and is even allowed to prevent the impending destruction by adding a reference.
The finalizer is only related to destruction by call order---if it runs, it runs
before destruction, which starts with :c:member:`~PyTypeObject.tp_clear` (if
called) and concludes with :c:member:`~PyTypeObject.tp_dealloc`.

The finalization step is not necessary to safely reclaim the objects in a
cyclic isolate, but its existence makes it easier to design types that behave
in a sane manner when objects are cleared.  Clearing an object might
necessarily leave it in a broken, partially destroyed state---it might be
unsafe to call any of the cleared object's methods or access any of its
attributes.  With finalization, only finalized objects can possibly interact
with cleared objects; non-finalized objects are guaranteed to interact with
only non-cleared (but potentially finalized) objects.

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
participated in a cyclic isolate.  Python currently only calls
:c:member:`~PyTypeObject.tp_finalize` and :c:member:`~PyTypeObject.tp_clear` as
needed to destroy a cyclic isolate; this may change in a future version.


Functions
---------

To allocate and free memory, see :ref:`allocating-objects`.


.. c:function:: void PyObject_CallFinalizer(PyObject *op)

   Finalizes the object as described in :c:member:`~PyTypeObject.tp_finalize`.
   Call this function (or :c:func:`PyObject_CallFinalizerFromDealloc`) instead
   of calling :c:member:`~PyTypeObject.tp_finalize` directly because this
   function may deduplicate multiple calls to :c:member:`!tp_finalize`.
   Currently, calls are only deduplicated if the type supports garbage
   collection (i.e., the :c:macro:`Py_TPFLAGS_HAVE_GC` flag is set); this may
   change in the future.


.. c:function:: int PyObject_CallFinalizerFromDealloc(PyObject *op)

   Same as :c:func:`PyObject_CallFinalizer` but meant to be called at the
   beginning of the object's destructor (:c:member:`~PyTypeObject.tp_dealloc`).
   There must not be any references to the object.  If the object's finalizer
   resurrects the object, this function returns -1; no further destruction
   should happen.  Otherwise, this function returns 0 and destruction can
   continue normally.

   .. seealso::

      :c:member:`~PyTypeObject.tp_dealloc` for example code.
