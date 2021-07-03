:mod:`tkinter.colorchooser` --- Color choosing dialog
=====================================================

.. module:: tkinter.colorchooser
   :platform: Tk
   :synopsis: Color choosing dialog

**Source code:** :source:`Lib/tkinter/colorchooser.py`

--------------

The :mod:`tkinter.colorchooser` module provides the :class:`Chooser` class
as an interface to the native color picker dialog. ``Chooser`` implements
a modal color choosing dialog window. The ``Chooser`` class inherits from
the :class:`~tkinter.commondialog.Dialog` class.

.. class:: Chooser(master=None, **options)

.. function:: askcolor(color=None, **options)

   Create a color choosing dialog. A call to this method will show the window,
   wait for the user to make a selection, and return the selected color (or
   ``None``) to the caller.


.. seealso::

   Module :mod:`tkinter.commondialog`
      Tkinter standard dialog module