:mod:`!tkinter.dnd` --- Drag and drop support
=============================================

.. module:: tkinter.dnd
   :synopsis: Tkinter drag-and-drop interface

**Source code:** :source:`Lib/tkinter/dnd.py`

--------------

.. note:: This is experimental and due to be deprecated when it is replaced
   with the Tk DND.

The :mod:`!tkinter.dnd` module provides drag-and-drop support for objects within
a single application, within the same window or between windows. To enable an
object to be dragged, you must create an event binding for it that starts the
drag-and-drop process. Typically, you bind a ButtonPress event to a callback
function that you write (see :ref:`Bindings-and-Events`). The function should
call :func:`dnd_start`, where *source* is the object to be dragged, and *event*
is the event that invoked the call (the argument to your callback function).

Selection of a target object occurs as follows:

#. Top-down search of the area under the mouse for a target widget:

   * the target widget should have a callable *dnd_accept* attribute;
   * if *dnd_accept* is not present or returns ``None``,
     the search moves to the parent widget;
   * if no target widget is found, the target object is ``None``.

#. Call to ``<old_target>.dnd_leave(source, event)``.
#. Call to ``<new_target>.dnd_enter(source, event)``.
#. Call to ``<target>.dnd_commit(source, event)`` to notify of the drop.
#. Call to ``<source>.dnd_end(target, event)`` to signal the end of drag-and-drop.


.. class:: DndHandler(source, event)

   The *DndHandler* class handles drag-and-drop events tracking Motion and
   ButtonRelease events on the root of the event widget.

   .. method:: cancel(event=None)

      Cancel the drag-and-drop process.

   .. method:: finish(event, commit=0)

      Execute end of drag-and-drop functions.

   .. method:: on_motion(event)

      Inspect area below mouse for target objects while a drag
      is performed.

   .. method:: on_release(event)

      Signal end of drag when the release pattern is triggered.

.. function:: dnd_start(source, event)

   Factory function for the drag-and-drop process.
   Return the :class:`DndHandler` instance managing the drag, or ``None`` if a
   drag could not be started.

.. seealso::

   :ref:`Bindings-and-Events`
