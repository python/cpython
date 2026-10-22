:mod:`!tkinter.systray` --- System tray icon and notifications
==============================================================

.. module:: tkinter.systray
   :synopsis: System tray icon and desktop notifications

**Source code:** :source:`Lib/tkinter/systray.py`

.. versionadded:: next

--------------

The :mod:`!tkinter.systray` module provides the :class:`SysTrayIcon` class
as an interface to the system tray (or taskbar) icon,
and the :func:`notify` function which sends a desktop notification.
They require Tk 8.7/9.0 or newer.

Only one system tray icon is supported per Tcl interpreter.

.. class:: SysTrayIcon(master=None, *, exists=False, **options)

   The class implementing the system tray icon.

   With *exists* false (the default), a new icon is created;
   creating a second one raises :exc:`~tkinter.TclError`.
   With *exists* true, the instance refers to the already-existing icon
   instead of creating one, reconfiguring it with any given options.

   The supported configuration options are:

   * *image* --- the image displayed in the system tray
     (required when creating an icon).
     On Windows, it must be a :class:`!PhotoImage`.
   * *text* --- the text displayed in the tooltip of the icon.
   * *button1* --- a callback that is called without arguments
     when the icon is clicked with the left mouse button.
   * *button3* --- a callback that is called without arguments
     when the icon is clicked with the right mouse button.

   .. method:: configure(**options)
               config(**options)

      Query or modify the options of the system tray icon.
      With no arguments, return a dict of all option values.
      With a string argument, return the value of that option.
      Otherwise, set the given options.

   .. method:: cget(option)

      Return the value of the given option of the system tray icon.

   .. method:: exists()

      Return whether the system tray icon exists.

   .. method:: destroy()

      Destroy the system tray icon.
      A new icon can be created afterwards.

   .. method:: notify(title, message)

      Send a desktop notification with the given title and message.


.. function:: notify(title, message, *, master=None)

   Send a desktop notification with the given title and message
   without creating a system tray icon first.
   On Windows, sending a notification requires an existing system
   tray icon, which is also displayed in the notification;
   use the :meth:`SysTrayIcon.notify` method instead.
