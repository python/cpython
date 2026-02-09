:mod:`!tkinter.scrolledtext` --- Scrolled Text Widget
=====================================================

.. module:: tkinter.scrolledtext
   :platform: Tk
   :synopsis: Text widget with a vertical scroll bar.

.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

**Source code:** :source:`Lib/tkinter/scrolledtext.py`

--------------

The :mod:`!tkinter.scrolledtext` module provides a class of the same name which
implements a basic text widget which has a vertical scroll bar configured to do
the "right thing."  Using the :class:`ScrolledText` class is a lot easier than
setting up a text widget and scroll bar directly.

The text widget and scrollbar are packed together in a :class:`Frame`, and the
methods of the :class:`Grid` and :class:`Pack` geometry managers are acquired
from the :class:`Frame` object.  This allows the :class:`ScrolledText` widget to
be used directly to achieve most normal geometry management behavior.

Should more specific control be necessary, the following attributes are
available:

.. class:: ScrolledText(master=None, **kw)


   .. attribute:: frame

      The frame which surrounds the text and scroll bar widgets.


   .. attribute:: vbar

      The scroll bar widget.
