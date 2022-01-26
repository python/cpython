:tocdepth: 2

==========================
Graphic User Interface FAQ
==========================

.. only:: html

   .. contents::

.. XXX need review for Python 3.


General GUI Questions
=====================

What GUI toolkits exist for Python?
===================================

Standard builds of Python include an object-oriented interface to the Tcl/Tk
widget set, called :ref:`tkinter <Tkinter>`.  This is probably the easiest to
install (since it comes included with most
`binary distributions <https://www.python.org/downloads/>`_ of Python) and use.
For more info about Tk, including pointers to the source, see the
`Tcl/Tk home page <https://www.tcl.tk>`_.  Tcl/Tk is fully portable to the
macOS, Windows, and Unix platforms.

Depending on what platform(s) you are aiming at, there are also several
alternatives. A `list of cross-platform
<https://wiki.python.org/moin/GuiProgramming#Cross-Platform_Frameworks>`_ and
`platform-specific
<https://wiki.python.org/moin/GuiProgramming#Platform-specific_Frameworks>`_ GUI
frameworks can be found on the python wiki.

Tkinter questions
=================

How do I freeze Tkinter applications?
-------------------------------------

Freeze is a tool to create stand-alone applications.  When freezing Tkinter
applications, the applications will not be truly stand-alone, as the application
will still need the Tcl and Tk libraries.

One solution is to ship the application with the Tcl and Tk libraries, and point
to them at run-time using the :envvar:`TCL_LIBRARY` and :envvar:`TK_LIBRARY`
environment variables.

To get truly stand-alone applications, the Tcl scripts that form the library
have to be integrated into the application as well. One tool supporting that is
SAM (stand-alone modules), which is part of the Tix distribution
(http://tix.sourceforge.net/).

Build Tix with SAM enabled, perform the appropriate call to
:c:func:`Tclsam_init`, etc. inside Python's
:file:`Modules/tkappinit.c`, and link with libtclsam and libtksam (you
might include the Tix libraries as well).


Can I have Tk events handled while waiting for I/O?
---------------------------------------------------

On platforms other than Windows, yes, and you don't even
need threads!  But you'll have to restructure your I/O
code a bit.  Tk has the equivalent of Xt's :c:func:`XtAddInput()` call, which allows you
to register a callback function which will be called from the Tk mainloop when
I/O is possible on a file descriptor.  See :ref:`tkinter-file-handlers`.


I can't get key bindings to work in Tkinter: why?
-------------------------------------------------

An often-heard complaint is that event handlers bound to events with the
:meth:`bind` method don't get handled even when the appropriate key is pressed.

The most common cause is that the widget to which the binding applies doesn't
have "keyboard focus".  Check out the Tk documentation for the focus command.
Usually a widget is given the keyboard focus by clicking in it (but not for
labels; see the takefocus option).
