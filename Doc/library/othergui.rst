.. _other-gui-packages:

Other Graphical User Interface Packages
=======================================

Major cross-platform (Windows, macOS, Unix-like) GUI toolkits are
available for Python:

.. seealso::

   `PyGObject <https://wiki.gnome.org/Projects/PyGObject>`_
      PyGObject provides introspection bindings for C libraries using
      `GObject <https://developer.gnome.org/gobject/stable/>`_.  One of
      these libraries is the `GTK+ 3 <https://www.gtk.org/>`_ widget set.
      GTK+ comes with many more widgets than Tkinter provides.  An online
      `Python GTK+ 3 Tutorial <https://python-gtk-3-tutorial.readthedocs.io/>`_
      is available.

   `PyGTK <http://www.pygtk.org/>`_
      PyGTK provides bindings for an older version
      of the library, GTK+ 2.  It provides an object oriented interface that
      is slightly higher level than the C one.  There are also bindings to
      `GNOME <https://www.gnome.org/>`_.  An online `tutorial
      <http://www.pygtk.org/pygtk2tutorial/index.html>`_ is available.

   `PyQt <https://riverbankcomputing.com/software/pyqt/intro>`_
      PyQt is a :program:`sip`\ -wrapped binding to the Qt toolkit.  Qt is an
      extensive C++ GUI application development framework that is
      available for Unix, Windows and macOS. :program:`sip` is a tool
      for generating bindings for C++ libraries as Python classes, and
      is specifically designed for Python.

   `PySide2 <https://doc.qt.io/qtforpython/>`_
      Also known as the Qt for Python project, PySide2 is a newer binding to the
      Qt toolkit. It is provided by The Qt Company and aims to provide a
      complete port of PySide to Qt 5. Compared to PyQt, its licensing scheme is
      friendlier to non-open source applications.

   `wxPython <https://www.wxpython.org>`_
      wxPython is a cross-platform GUI toolkit for Python that is built around
      the popular `wxWidgets <https://www.wxwidgets.org/>`_ (formerly wxWindows)
      C++ toolkit.  It provides a native look and feel for applications on
      Windows, macOS, and Unix systems by using each platform's native
      widgets where ever possible, (GTK+ on Unix-like systems).  In addition to
      an extensive set of widgets, wxPython provides classes for online
      documentation and context sensitive help, printing, HTML viewing,
      low-level device context drawing, drag and drop, system clipboard access,
      an XML-based resource format and more, including an ever growing library
      of user-contributed modules.

PyGTK, PyQt, PySide2, and wxPython, all have a modern look and feel and more
widgets than Tkinter. In addition, there are many other GUI toolkits for
Python, both cross-platform, and platform-specific. See the `GUI Programming
<https://wiki.python.org/moin/GuiProgramming>`_ page in the Python Wiki for a
much more complete list, and also for links to documents where the
different GUI toolkits are compared.

