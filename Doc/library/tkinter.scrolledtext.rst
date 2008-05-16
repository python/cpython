:mod:`tkinter.scrolledtext` --- Scrolled Text Widget
====================================================

.. module:: ScrolledText
   :synopsis: Old name for the tkinter.scrolledtext module.

.. module:: tkinter.scrolledtext
   :platform: Tk
   :synopsis: Text widget with a vertical scroll bar.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

.. note::
   The :mod:`ScrolledText` module has been renamed to
   :mod:`tkinter.scrolledtext` in Python 3.0.  It is importable under both names
   in Python 2.6 and the rest of the 2.x series.

The :mod:`tkinter.scrolledtext` module provides a class of the same name which
implements a basic text widget which has a vertical scroll bar configured to do
the "right thing."  Using the :class:`ScrolledText` class is a lot easier than
setting up a text widget and scroll bar directly.  The constructor is the same
as that of the :class:`tkinter.Text` class.

.. versionchanged:: 2.6
   Renamed from ``ScrolledText``.

The text widget and scrollbar are packed together in a :class:`Frame`, and the
methods of the :class:`Grid` and :class:`Pack` geometry managers are acquired
from the :class:`Frame` object.  This allows the :class:`ScrolledText` widget to
be used directly to achieve most normal geometry management behavior.

Should more specific control be necessary, the following attributes are
available:


.. attribute:: ScrolledText.frame

   The frame which surrounds the text and scroll bar widgets.


.. attribute:: ScrolledText.vbar

   The scroll bar widget.
