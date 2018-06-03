:mod:`tkinter.colorchooser` --- Color choosing dialog
=====================================================

.. module:: tkinter.colorchooser
   :platform: Tk
   :synopsis: Color choosing dialog

**Source code:** :source:`Lib/tkinter/colorchooser.py`

--------------

The :mod:`tkinter.colorchooser` module provides the :class:`Chooser` class
as an interface to the native color picker dialog available in Tk 4.2 and
newer. :class:`Chooser` implements a modal color choosing dialog window. The
:class:`Chooser` class inherits from the :class:`~tkinter.commondialog.Dialog`
class.

.. class:: Chooser(master=None, **options)

.. function:: askcolor(color=None, **options)

   The *askcolor* method is a factory method that creates a color choosing
   dialog. A call to this method will show the window, wait for the user to
   make a selection, and return the selected color (or None) to the caller.


.. seealso::

   Module :mod:`tkinter.commondialog`
      Tkinter standard dialog module