:mod:`tkinter.dnd` --- Drag and drop support
============================================

.. module:: tkinter.dnd
   :platform: Tk
   :synopsis: Tkinter drag-and-drop interface

**Source code:** :source:`Lib/tkinter/dnd.py`

--------------

.. note:: This is experimental and due to be deprecated when it is replaced
   with the Tk DND.

The :mod:`tkinter.dnd` module provides drag-and-drop support for objects within
a single application, within the same window or between windows. To enable an
object to be dragged, you must create an event binding for it that starts the
drag-and-drop process. Typically, you bind a ButtonPress event to a callback
function that you write (see :ref:`Bindings-and-Events`). The function should
call :func:`dnd_start`, where 'source' is the object to be dragged, and 'event'
is the event that invoked the call (the argument to your callback function).

Selection of a target object occurs as follows:

#. Top-down search of area under mouse for target widget

 * Target widget should have a callable *dnd_accept* attribute
 * If *dnd_accept* is not present or returns None, search moves to parent widget
 * If no target widget is found, then the target object is None

2. Call to *<old_target>.dnd_leave(source, event)*
#. Call to *<new_target>.dnd_enter(source, event)*
#. Call to *<target>.dnd_commit(source, event)* to notify of drop
#. Call to *<source>.dnd_end(target, event)* to signal end of drag-and-drop


.. class:: DndHandler(source, event)

   The *DndHandler* class handles drag-and-drop events tracking Motion and
   ButtonRelease events on the root of the event widget.

   .. method:: cancel(event=None)

      Cancel the drag-and-drop process.

   .. method:: finish(event, commit=0)

      Execute end of drag-and-drop functions.

   .. method:: on_motion(event)

      Inspect area below mouse for target objects while drag is performed.

   .. method:: on_release(event)

      Signal end of drag when the release pattern is triggered.

.. function:: dnd_start(source, event)

   Factory function for drag-and-drop process.

.. seealso::

   :ref:`Bindings-and-Events`