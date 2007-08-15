
:mod:`FrameWork` --- Interactive application framework
======================================================

.. module:: FrameWork
   :platform: Mac
   :synopsis: Interactive application framework.


The :mod:`FrameWork` module contains classes that together provide a framework
for an interactive Macintosh application. The programmer builds an application
by creating subclasses that override various methods of the bases classes,
thereby implementing the functionality wanted. Overriding functionality can
often be done on various different levels, i.e. to handle clicks in a single
dialog window in a non-standard way it is not necessary to override the complete
event handling.

Work on the :mod:`FrameWork` has pretty much stopped, now that :mod:`PyObjC` is
available for full Cocoa access from Python, and the documentation describes
only the most important functionality, and not in the most logical manner at
that. Examine the source or the examples for more details.  The following are
some comments posted on the MacPython newsgroup about the strengths and
limitations of :mod:`FrameWork`:


.. epigraph::

   The strong point of :mod:`FrameWork` is that it allows you to break into the
   control-flow at many different places. :mod:`W`, for instance, uses a different
   way to enable/disable menus and that plugs right in leaving the rest intact.
   The weak points of :mod:`FrameWork` are that it has no abstract command
   interface (but that shouldn't be difficult), that its dialog support is minimal
   and that its control/toolbar support is non-existent.

The :mod:`FrameWork` module defines the following functions:


.. function:: Application()

   An object representing the complete application. See below for a description of
   the methods. The default :meth:`__init__` routine creates an empty window
   dictionary and a menu bar with an apple menu.


.. function:: MenuBar()

   An object representing the menubar. This object is usually not created by the
   user.


.. function:: Menu(bar, title[, after])

   An object representing a menu. Upon creation you pass the ``MenuBar`` the menu
   appears in, the *title* string and a position (1-based) *after* where the menu
   should appear (default: at the end).


.. function:: MenuItem(menu, title[, shortcut, callback])

   Create a menu item object. The arguments are the menu to create, the item title
   string and optionally the keyboard shortcut and a callback routine. The callback
   is called with the arguments menu-id, item number within menu (1-based), current
   front window and the event record.

   Instead of a callable object the callback can also be a string. In this case
   menu selection causes the lookup of a method in the topmost window and the
   application. The method name is the callback string with ``'domenu_'``
   prepended.

   Calling the ``MenuBar`` :meth:`fixmenudimstate` method sets the correct dimming
   for all menu items based on the current front window.


.. function:: Separator(menu)

   Add a separator to the end of a menu.


.. function:: SubMenu(menu, label)

   Create a submenu named *label* under menu *menu*. The menu object is returned.


.. function:: Window(parent)

   Creates a (modeless) window. *Parent* is the application object to which the
   window belongs. The window is not displayed until later.


.. function:: DialogWindow(parent)

   Creates a modeless dialog window.


.. function:: windowbounds(width, height)

   Return a ``(left, top, right, bottom)`` tuple suitable for creation of a window
   of given width and height. The window will be staggered with respect to previous
   windows, and an attempt is made to keep the whole window on-screen. However, the
   window will however always be the exact size given, so parts may be offscreen.


.. function:: setwatchcursor()

   Set the mouse cursor to a watch.


.. function:: setarrowcursor()

   Set the mouse cursor to an arrow.


.. _application-objects:

Application Objects
-------------------

Application objects have the following methods, among others:


.. method:: Application.makeusermenus()

   Override this method if you need menus in your application. Append the menus to
   the attribute :attr:`menubar`.


.. method:: Application.getabouttext()

   Override this method to return a text string describing your application.
   Alternatively, override the :meth:`do_about` method for more elaborate "about"
   messages.


.. method:: Application.mainloop([mask[, wait]])

   This routine is the main event loop, call it to set your application rolling.
   *Mask* is the mask of events you want to handle, *wait* is the number of ticks
   you want to leave to other concurrent application (default 0, which is probably
   not a good idea). While raising *self* to exit the mainloop is still supported
   it is not recommended: call ``self._quit()`` instead.

   The event loop is split into many small parts, each of which can be overridden.
   The default methods take care of dispatching events to windows and dialogs,
   handling drags and resizes, Apple Events, events for non-FrameWork windows, etc.

   In general, all event handlers should return ``1`` if the event is fully handled
   and ``0`` otherwise (because the front window was not a FrameWork window, for
   instance). This is needed so that update events and such can be passed on to
   other windows like the Sioux console window. Calling :func:`MacOS.HandleEvent`
   is not allowed within *our_dispatch* or its callees, since this may result in an
   infinite loop if the code is called through the Python inner-loop event handler.


.. method:: Application.asyncevents(onoff)

   Call this method with a nonzero parameter to enable asynchronous event handling.
   This will tell the inner interpreter loop to call the application event handler
   *async_dispatch* whenever events are available. This will cause FrameWork window
   updates and the user interface to remain working during long computations, but
   will slow the interpreter down and may cause surprising results in non-reentrant
   code (such as FrameWork itself). By default *async_dispatch* will immediately
   call *our_dispatch* but you may override this to handle only certain events
   asynchronously. Events you do not handle will be passed to Sioux and such.

   The old on/off value is returned.


.. method:: Application._quit()

   Terminate the running :meth:`mainloop` call at the next convenient moment.


.. method:: Application.do_char(c, event)

   The user typed character *c*. The complete details of the event can be found in
   the *event* structure. This method can also be provided in a ``Window`` object,
   which overrides the application-wide handler if the window is frontmost.


.. method:: Application.do_dialogevent(event)

   Called early in the event loop to handle modeless dialog events. The default
   method simply dispatches the event to the relevant dialog (not through the
   ``DialogWindow`` object involved). Override if you need special handling of
   dialog events (keyboard shortcuts, etc).


.. method:: Application.idle(event)

   Called by the main event loop when no events are available. The null-event is
   passed (so you can look at mouse position, etc).


.. _window-objects:

Window Objects
--------------

Window objects have the following methods, among others:


.. method:: Window.open()

   Override this method to open a window. Store the MacOS window-id in
   :attr:`self.wid` and call the :meth:`do_postopen` method to register the window
   with the parent application.


.. method:: Window.close()

   Override this method to do any special processing on window close. Call the
   :meth:`do_postclose` method to cleanup the parent state.


.. method:: Window.do_postresize(width, height, macoswindowid)

   Called after the window is resized. Override if more needs to be done than
   calling ``InvalRect``.


.. method:: Window.do_contentclick(local, modifiers, event)

   The user clicked in the content part of a window. The arguments are the
   coordinates (window-relative), the key modifiers and the raw event.


.. method:: Window.do_update(macoswindowid, event)

   An update event for the window was received. Redraw the window.


.. method:: Window.do_activate(activate, event)

   The window was activated (``activate == 1``) or deactivated (``activate == 0``).
   Handle things like focus highlighting, etc.


.. _controlswindow-object:

ControlsWindow Object
---------------------

ControlsWindow objects have the following methods besides those of ``Window``
objects:


.. method:: ControlsWindow.do_controlhit(window, control, pcode, event)

   Part *pcode* of control *control* was hit by the user. Tracking and such has
   already been taken care of.


.. _scrolledwindow-object:

ScrolledWindow Object
---------------------

ScrolledWindow objects are ControlsWindow objects with the following extra
methods:


.. method:: ScrolledWindow.scrollbars([wantx[, wanty]])

   Create (or destroy) horizontal and vertical scrollbars. The arguments specify
   which you want (default: both). The scrollbars always have minimum ``0`` and
   maximum ``32767``.


.. method:: ScrolledWindow.getscrollbarvalues()

   You must supply this method. It should return a tuple ``(x, y)`` giving the
   current position of the scrollbars (between ``0`` and ``32767``). You can return
   ``None`` for either to indicate the whole document is visible in that direction.


.. method:: ScrolledWindow.updatescrollbars()

   Call this method when the document has changed. It will call
   :meth:`getscrollbarvalues` and update the scrollbars.


.. method:: ScrolledWindow.scrollbar_callback(which, what, value)

   Supplied by you and called after user interaction. *which* will be ``'x'`` or
   ``'y'``, *what* will be ``'-'``, ``'--'``, ``'set'``, ``'++'`` or ``'+'``. For
   ``'set'``, *value* will contain the new scrollbar position.


.. method:: ScrolledWindow.scalebarvalues(absmin, absmax, curmin, curmax)

   Auxiliary method to help you calculate values to return from
   :meth:`getscrollbarvalues`. You pass document minimum and maximum value and
   topmost (leftmost) and bottommost (rightmost) visible values and it returns the
   correct number or ``None``.


.. method:: ScrolledWindow.do_activate(onoff, event)

   Takes care of dimming/highlighting scrollbars when a window becomes frontmost.
   If you override this method, call this one at the end of your method.


.. method:: ScrolledWindow.do_postresize(width, height, window)

   Moves scrollbars to the correct position. Call this method initially if you
   override it.


.. method:: ScrolledWindow.do_controlhit(window, control, pcode, event)

   Handles scrollbar interaction. If you override it call this method first, a
   nonzero return value indicates the hit was in the scrollbars and has been
   handled.


.. _dialogwindow-objects:

DialogWindow Objects
--------------------

DialogWindow objects have the following methods besides those of ``Window``
objects:


.. method:: DialogWindow.open(resid)

   Create the dialog window, from the DLOG resource with id *resid*. The dialog
   object is stored in :attr:`self.wid`.


.. method:: DialogWindow.do_itemhit(item, event)

   Item number *item* was hit. You are responsible for redrawing toggle buttons,
   etc.

