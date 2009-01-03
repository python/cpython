.. _tkinter:

*********************************
Graphical User Interfaces with Tk
*********************************

.. index::
   single: GUI
   single: Graphical User Interface
   single: Tkinter
   single: Tk

Tk/Tcl has long been an integral part of Python.  It provides a robust and
platform independent windowing toolkit, that is available to Python programmers
using the :mod:`Tkinter` module, and its extension, the :mod:`Tix` module.

The :mod:`Tkinter` module is a thin object-oriented layer on top of Tcl/Tk. To
use :mod:`Tkinter`, you don't need to write Tcl code, but you will need to
consult the Tk documentation, and occasionally the Tcl documentation.
:mod:`Tkinter` is a set of wrappers that implement the Tk widgets as Python
classes.  In addition, the internal module :mod:`_tkinter` provides a threadsafe
mechanism which allows Python and Tcl to interact.

:mod:`Tkinter`'s chief virtues are that it is fast, and that it usually comes
bundled with Python. Although its standard documentation is weak, good
material is available, which includes: references, tutorials, a book and
others. :mod:`Tkinter` is also famous for having an outdated look and feel,
which has been vastly improved in Tk 8.5. Nevertheless, there are many other
GUI libraries that you could be interested in. For more information about
alternatives, see the :ref:`other-gui-packages` section.

.. toctree::

   tkinter.rst
   tix.rst
   scrolledtext.rst
   turtle.rst
   idle.rst
   othergui.rst

.. Other sections I have in mind are
   Tkinter internals
   Freezing Tkinter applications


