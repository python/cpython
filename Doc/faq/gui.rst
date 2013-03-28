:tocdepth: 2

==========================
Graphic User Interface FAQ
==========================

.. only:: html

   .. contents::

.. XXX need review for Python 3.


General GUI Questions
=====================

What platform-independent GUI toolkits exist for Python?
========================================================

Depending on what platform(s) you are aiming at, there are several.  Some
of them haven't been ported to Python 3 yet.  At least `Tkinter`_ and `Qt`_
are known to be Python 3-compatible.

.. XXX check links

Tkinter
-------

Standard builds of Python include an object-oriented interface to the Tcl/Tk
widget set, called :ref:`tkinter <Tkinter>`.  This is probably the easiest to
install (since it comes included with most
`binary distributions <http://www.python.org/download/>`_ of Python) and use.
For more info about Tk, including pointers to the source, see the
`Tcl/Tk home page <http://www.tcl.tk>`_.  Tcl/Tk is fully portable to the
MacOS, Windows, and Unix platforms.

wxWidgets
---------

wxWidgets (http://www.wxwidgets.org) is a free, portable GUI class
library written in C++ that provides a native look and feel on a
number of platforms, with Windows, MacOS X, GTK, X11, all listed as
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
<http://www.riverbankcomputing.co.uk/software/pyqt/>`_ or `PySide
<http://www.pyside.org/>`_) and for KDE (`PyKDE <http://www.riverbankcomputing.co.uk/software/pykde/intro>`__).
PyQt is currently more mature than PySide, but you must buy a PyQt license from
`Riverbank Computing <http://www.riverbankcomputing.co.uk/software/pyqt/license>`_
if you want to write proprietary applications.  PySide is free for all applications.

Qt 4.5 upwards is licensed under the LGPL license; also, commercial licenses
are available from `Nokia <http://qt.nokia.com/>`_.

Gtk+
----

The `GObject introspection bindings <https://live.gnome.org/PyGObject>`_
for Python allow you to write GTK+ 3 applications.  There is also a
`Python GTK+ 3 Tutorial <http://python-gtk-3-tutorial.readthedocs.org/en/latest/>`_.

The older PyGtk bindings for the `Gtk+ 2 toolkit <http://www.gtk.org>`_ have
been implemented by James Henstridge; see <http://www.pygtk.org>.

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

`The Mac port <http://python.org/download/mac>`_ by Jack Jansen has a rich and
ever-growing set of modules that support the native Mac toolbox calls.  The port
supports MacOS X's Carbon libraries.

By installing the `PyObjc Objective-C bridge
<http://pyobjc.sourceforge.net>`_, Python programs can use MacOS X's
Cocoa libraries. See the documentation that comes with the Mac port.

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

Yes, and you don't even need threads!  But you'll have to restructure your I/O
code a bit.  Tk has the equivalent of Xt's :c:func:`XtAddInput()` call, which allows you
to register a callback function which will be called from the Tk mainloop when
I/O is possible on a file descriptor.  Here's what you need::

   from Tkinter import tkinter
   tkinter.createfilehandler(file, mask, callback)

The file may be a Python file or socket object (actually, anything with a
fileno() method), or an integer file descriptor.  The mask is one of the
constants tkinter.READABLE or tkinter.WRITABLE.  The callback is called as
follows::

   callback(file, mask)

You must unregister the callback when you're done, using ::

   tkinter.deletefilehandler(file)

Note: since you don't know *how many bytes* are available for reading, you can't
use the Python file object's read or readline methods, since these will insist
on reading a predefined number of bytes.  For sockets, the :meth:`recv` or
:meth:`recvfrom` methods will work fine; for other files, use
``os.read(file.fileno(), maxbytecount)``.


I can't get key bindings to work in Tkinter: why?
-------------------------------------------------

An often-heard complaint is that event handlers bound to events with the
:meth:`bind` method don't get handled even when the appropriate key is pressed.

The most common cause is that the widget to which the binding applies doesn't
have "keyboard focus".  Check out the Tk documentation for the focus command.
Usually a widget is given the keyboard focus by clicking in it (but not for
labels; see the takefocus option).
