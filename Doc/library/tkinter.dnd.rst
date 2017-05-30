:mod:`tkinter.dnd` --- Tkinter drag and drop
============================================

.. module:: tkinter.dnd
   :platform: Tk
   :synopsis: Tkinter drag-and-drop interface

**Source code:** :source:`Lib/tkinter/dnd.py`

--------------

The :mod:`tkinter.dnd` module provides drag-and-drop support for objcts within
a single application, within the same window or between windows. To enable an 
object to be dragged, you must create an event binding for it that starts the
drag-and-drop process. Typically, you bind <ButtonPress> to a callback function
that you write. The function should call Tkdnd.dnd_start(source, event), where
'source' is the object to be dragged, and 'event' is the event that invoked 
the call (the argument to your callback function).

Selection of a target object occurs as follows:

#. Bottom-up search of area under mouse for target widget
 
 * Target widget should have a callable dnd_accept attribute
 * If dnd_accept not present or returns None search moves to parent widget
 * If no target widget is found target object is None
  
2. Call to `dnd_leave (source, event)` on old (or initial) target object
#. Call to `dnd_enter (source, event)` on new target object
#. Call to `target.dnd_commit(source, event)` to notify of drop
#. Call to `source.dnd_end(target, event)` to signal end of drag-and-drop
 
 
 .. class:: DndHandler(self, source, event)
 
    The `DndHandler` class handles drag-and-drop events tracking Motions and
	ButtonReleases on the root widget of the event
 
     .. function:: cancel(self, event=None)

	    Cancels the drag-and-drop process
	 
     .. function:: finish(self, event, commit=0)
	 
	    Executes end of drag-and-drop functions
	 
     .. function:: on_motion(self, event)
	 
	    Inspects area below mouse for target objects while drag is performed
	 
     .. function:: on_release(self, event)
	 
	    Called when the release pattern is triggered and signals end of drag
 
 .. method:: dnd_start(source, event)
 
    Factory method for drag-and-drop process