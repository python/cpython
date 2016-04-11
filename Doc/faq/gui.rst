:tocdepth: 2

==========================
Graphic User Interface FAQ
==========================

.. only:: html

   .. contents::

What platform-independent GUI toolkits exist for Python?
========================================================

Depending on what platform(s) you are aiming at, there are several.

.. XXX check links

Tkinter
-------

Standard builds of Python include an object-oriented interface to the Tcl/Tk
widget set, called Tkinter.  This is probably the easiest to install and use.
For more info about Tk, including pointers to the source, see the Tcl/Tk home
page at http://www.tcl.tk.  Tcl/Tk is fully portable to the Mac OS X, Windows,
and Unix platforms.

wxWidgets
---------

wxWidgets (http://www.wxwidgets.org) is a free, portable GUI class
library written in C++ that provides a native look and feel on a
number of platforms, with Windows, Mac OS X, GTK, X11, all listed as
current stable targets.  Language bindings are available for a number
of languages including Python, Perl, Ruby, etc.

wxPython (http://www.wxpython.org) is the Python binding for
wxwidgets.  While it often lags slightly behind the official wxWidgets
releases, it also offers a number of features via pure Python
extensions that are not available in other language bindings.  There
is an active wxPython user and developer community.

Both wxWidgets and wxPython are free, open source, software with
permissive licences that allow their use in commercial products as
well as in freeware or shareware.


Qt
---

There are bindings available for the Qt toolkit (using either `PyQt
<https://riverbankcomputing.com/software/pyqt/intro>`_ or `PySide
<https://wiki.qt.io/PySide>`_) and for KDE (`PyKDE4 <https://techbase.kde.org/Languages/Python/Using_PyKDE_4>`__).
PyQt is currently more mature than PySide, but you must buy a PyQt license from
`Riverbank Computing <https://www.riverbankcomputing.com/commercial/license-faq>`_
if you want to write proprietary applications.  PySide is free for all applications.

Qt 4.5 upwards is licensed under the LGPL license; also, commercial licenses
are available from `The Qt Company <https://www.qt.io/licensing/>`_.

Gtk+
----

PyGtk bindings for the `Gtk+ toolkit <http://www.gtk.org>`_ have been
implemented by James Henstridge; see <http://www.pygtk.org>.

FLTK
----

Python bindings for `the FLTK toolkit <http://www.fltk.org>`_, a simple yet
powerful and mature cross-platform windowing system, are available from `the
PyFLTK project <http://pyfltk.sourceforge.net>`_.


FOX
----

A wrapper for `the FOX toolkit <http://www.fox-toolkit.org/>`_ called `FXpy
<http://fxpy.sourceforge.net/>`_ is available.  FOX supports both Unix variants
and Windows.


OpenGL
------

For OpenGL bindings, see `PyOpenGL <http://pyopengl.sourceforge.net>`_.


What platform-specific GUI toolkits exist for Python?
========================================================

By installing the `PyObjc Objective-C bridge
<https://pythonhosted.org/pyobjc/>`_, Python programs can use Mac OS X's
Cocoa libraries.

:ref:`Pythonwin <windows-faq>` by Mark Hammond includes an interface to the
Microsoft Foundation Classes and a Python programming environment
that's written mostly in Python using the MFC classes.


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



