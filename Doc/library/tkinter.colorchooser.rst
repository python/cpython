:mod:`!tkinter.colorchooser` --- Color choosing dialog
======================================================

.. module:: tkinter.colorchooser
   :synopsis: Color choosing dialog

**Source code:** :source:`Lib/tkinter/colorchooser.py`

--------------

The :mod:`!tkinter.colorchooser` module provides the :class:`Chooser` class
as an interface to the native color picker dialog. ``Chooser`` implements
a modal color choosing dialog window. The ``Chooser`` class inherits from
the :class:`~tkinter.commondialog.Dialog` class.

.. class:: Chooser(master=None, **options)

   The class implementing the modal color-choosing dialog.
   Most applications use the :func:`askcolor` convenience function rather than
   instantiating this class directly.

.. function:: askcolor(color=None, **options)

   Show a modal color-choosing dialog and return the chosen color.
   *color* is the color selected when the dialog opens.
   The return value is a tuple ``((r, g, b), hexstr)``, where ``r``, ``g`` and
   ``b`` are the red, green and blue components as integers in the range 0–255
   and *hexstr* is the equivalent Tk color string, such as ``'#ff8000'``.
   If the user cancels the dialog, ``(None, None)`` is returned.

   .. versionchanged:: 3.10
      The RGB values in the returned color are now integers in the range 0–255
      instead of floats.


.. seealso::

   Module :mod:`tkinter.commondialog`
      Tkinter standard dialog module

