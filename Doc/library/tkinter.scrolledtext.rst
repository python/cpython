:mod:`!tkinter.scrolledtext` --- Scrolled text widget
=====================================================

.. module:: tkinter.scrolledtext
   :synopsis: Text widget with a vertical scroll bar.

**Source code:** :source:`Lib/tkinter/scrolledtext.py`

--------------

The :mod:`!tkinter.scrolledtext` module provides a class of the same name which
implements a basic text widget which has a vertical scroll bar configured to do
the "right thing."  Using the :class:`ScrolledText` class is a lot easier than
setting up a text widget and scroll bar directly.

The text widget and scrollbar are packed together in a :class:`~tkinter.Frame`,
and the methods of the :class:`~tkinter.Pack`, :class:`~tkinter.Grid` and
:class:`~tkinter.Place` geometry managers are acquired from the
:class:`~tkinter.Frame` object.
This allows the :class:`ScrolledText` widget to be used directly to achieve
most normal geometry management behavior.

Should more specific control be necessary, the following attributes are
available:

.. class:: ScrolledText(master=None, *, use_ttk=False, **kw)

   The keyword arguments are passed to the :class:`~tkinter.Text` widget.

   When *use_ttk* is true, the surrounding frame and the scroll bar are the
   themed :mod:`tkinter.ttk` widgets;
   the default is the classic :mod:`tkinter` widgets.

   .. versionchanged:: next
      Added the *use_ttk* parameter.


   .. attribute:: frame

      The frame which surrounds the text and scroll bar widgets.


   .. attribute:: vbar

      The scroll bar widget.
