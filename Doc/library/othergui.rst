.. _other-gui-packages:

Other Graphical User Interface Packages
=======================================

Major cross-platform (Windows, Mac OS X, Unix-like) GUI toolkits are
available for Python:

.. seealso::

   `PyGObject <https://live.gnome.org/PyGObject>`_
      provides introspection bindings for C libraries using
      `GObject <https://developer.gnome.org/gobject/stable/>`_.  One of
      these libraries is the `GTK+ 3 <http://www.gtk.org/>`_ widget set.
      GTK+ comes with many more widgets than Tkinter provides.  An online
      `Python GTK+ 3 Tutorial <http://python-gtk-3-tutorial.readthedocs.org/en/latest/>`_
      is available.

      `PyGTK <http://www.pygtk.org/>`_ provides bindings for an older version
      of the library, GTK+ 2.  It provides an object oriented interface that
      is slightly higher level than the C one.  There are also bindings to
      `GNOME <http://www.gnome.org>`_.  An online `tutorial
      <http://www.pygtk.org/pygtk2tutorial/index.html>`_ is available.

   `PyQt <http://www.riverbankcomputing.co.uk/software/pyqt/intro>`_
      PyQt is a :program:`sip`\ -wrapped binding to the Qt toolkit.  Qt is an
      extensive C++ GUI application development framework that is
      available for Unix, Windows and Mac OS X. :program:`sip` is a tool
      for generating bindings for C++ libraries as Python classes, and
      is specifically designed for Python. The *PyQt3* bindings have a
      book, `GUI Programming with Python: QT Edition
      <http://www.commandprompt.com/community/pyqt/>`_ by Boudewijn
      Rempt. The *PyQt4* bindings also have a book, `Rapid GUI Programming
      with Python and Qt <http://www.qtrac.eu/pyqtbook.html>`_, by Mark
      Summerfield.

   `PySide <http://qt-project.org/wiki/PySide>`_
      is a newer binding to the Qt toolkit, provided by Nokia.
      Compared to PyQt, its licensing scheme is friendlier to non-open source
      applications.

   `wxPython <http://www.wxpython.org>`_
      wxPython is a cross-platform GUI toolkit for Python that is built around
      the popular `wxWidgets <http://www.wxwidgets.org/>`_ (formerly wxWindows)
      C++ toolkit.  It provides a native look and feel for applications on
      Windows, Mac OS X, and Unix systems by using each platform's native
      widgets where ever possible, (GTK+ on Unix-like systems).  In addition to
      an extensive set of widgets, wxPython provides classes for online
      documentation and context sensitive help, printing, HTML viewing,
      low-level device context drawing, drag and drop, system clipboard access,
      an XML-based resource format and more, including an ever growing library
      of user-contributed modules.  wxPython has a book, `wxPython in Action
      <http://www.manning.com/rappin/>`_, by Noel Rappin and
      Robin Dunn.

PyGTK, PyQt, and wxPython, all have a modern look and feel and more
widgets than Tkinter. In addition, there are many other GUI toolkits for
Python, both cross-platform, and platform-specific. See the `GUI Programming
<https://wiki.python.org/moin/GuiProgramming>`_ page in the Python Wiki for a
much more complete list, and also for links to documents where the
different GUI toolkits are compared.

