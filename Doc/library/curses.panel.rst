:mod:`curses.panel` --- A panel stack extension for curses
==========================================================

.. module:: curses.panel
   :synopsis: A panel stack extension that adds depth to  curses windows.

.. sectionauthor:: A.M. Kuchling <amk@amk.ca>

--------------

Panels are windows with the added feature of depth, so they can be stacked on
top of each other, and only the visible portions of each window will be
displayed.  Panels can be added, moved up or down in the stack, and removed.


.. _cursespanel-functions:

Functions
---------

The module :mod:`curses.panel` defines the following functions:


.. function:: bottom_panel()

   Returns the bottom panel in the panel stack.


.. function:: new_panel(win)

   Returns a panel object, associating it with the given window *win*. Be aware
   that you need to keep the returned panel object referenced explicitly.  If you
   don't, the panel object is garbage collected and removed from the panel stack.


.. function:: top_panel()

   Returns the top panel in the panel stack.


.. function:: update_panels()

   Updates the virtual screen after changes in the panel stack. This does not call
   :func:`curses.doupdate`, so you'll have to do this yourself.


.. _curses-panel-objects:

Panel Objects
-------------

Panel objects, as returned by :func:`new_panel` above, are windows with a
stacking order. There's always a window associated with a panel which determines
the content, while the panel methods are responsible for the window's depth in
the panel stack.

Panel objects have the following methods:


.. method:: Panel.above()

   Returns the panel above the current panel.


.. method:: Panel.below()

   Returns the panel below the current panel.


.. method:: Panel.bottom()

   Push the panel to the bottom of the stack.


.. method:: Panel.hidden()

   Returns ``True`` if the panel is hidden (not visible), ``False`` otherwise.


.. method:: Panel.hide()

   Hide the panel. This does not delete the object, it just makes the window on
   screen invisible.


.. method:: Panel.move(y, x)

   Move the panel to the screen coordinates ``(y, x)``.


.. method:: Panel.replace(win)

   Change the window associated with the panel to the window *win*.


.. method:: Panel.set_userptr(obj)

   Set the panel's user pointer to *obj*. This is used to associate an arbitrary
   piece of data with the panel, and can be any Python object.


.. method:: Panel.show()

   Display the panel (which might have been hidden).


.. method:: Panel.top()

   Push panel to the top of the stack.


.. method:: Panel.userptr()

   Returns the user pointer for the panel.  This might be any Python object.


.. method:: Panel.window()

   Returns the window object associated with the panel.

