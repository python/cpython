.. _other-gui-packages:

Other Graphical User Interface Packages
=======================================

There are an number of extension widget sets to :mod:`Tkinter`.

.. seealso::

   `Python megawidgets <http://pmw.sourceforge.net/>`_
      is a toolkit for building high-level compound widgets in Python using the
      :mod:`Tkinter` module.  It consists of a set of base classes and a library of
      flexible and extensible megawidgets built on this foundation. These megawidgets
      include notebooks, comboboxes, selection widgets, paned widgets, scrolled
      widgets, dialog windows, etc.  Also, with the Pmw.Blt interface to BLT, the
      busy, graph, stripchart, tabset and vector commands are be available.

      The initial ideas for Pmw were taken from the Tk ``itcl`` extensions ``[incr
      Tk]`` by Michael McLennan and ``[incr Widgets]`` by Mark Ulferts. Several of the
      megawidgets are direct translations from the itcl to Python. It offers most of
      the range of widgets that ``[incr Widgets]`` does, and is almost as complete as
      Tix, lacking however Tix's fast :class:`HList` widget for drawing trees.

   `Tkinter3000 Widget Construction Kit (WCK) <http://tkinter.effbot.org/>`_
      is a library that allows you to write new Tkinter widgets in pure Python.  The
      WCK framework gives you full control over widget creation, configuration, screen
      appearance, and event handling.  WCK widgets can be very fast and light-weight,
      since they can operate directly on Python data structures, without having to
      transfer data through the Tk/Tcl layer.


The major cross-platform (Windows, Mac OS X, Unix-like) GUI toolkits that are
also available for Python:

.. seealso::

   `PyGTK <http://www.pygtk.org/>`_
      is a set of bindings for the `GTK <http://www.gtk.org/>`_ widget set. It
      provides an object oriented interface that is slightly higher level than
      the C one. It comes with many more widgets than Tkinter provides, and has
      good Python-specific reference documentation. There are also bindings to
      `GNOME <http://www.gnome.org>`_.  One well known PyGTK application is
      `PythonCAD <http://www.pythoncad.org/>`_. An online `tutorial
      <http://www.pygtk.org/pygtk2tutorial/index.html>`_ is available.

   `PyQt <http://www.riverbankcomputing.co.uk/pyqt/index.php>`_
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
      <http://www.amazon.com/exec/obidos/ASIN/1932394621>`_, by Noel Rappin and
      Robin Dunn.

PyGTK, PyQt, and wxPython, all have a modern look and feel and far more
widgets and better documentation than Tkinter. In addition,
there are many other GUI toolkits for Python, both cross-platform, and
platform-specific. See the `GUI Programming
<http://wiki.python.org/moin/GuiProgramming>`_ page in the Python Wiki for a
much more complete list, and also for links to documents where the
different GUI toolkits are compared.

