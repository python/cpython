:mod:`!tkinter.fontchooser` --- Font selection dialog
=====================================================

.. module:: tkinter.fontchooser
   :synopsis: Font selection dialog

.. versionadded:: next

**Source code:** :source:`Lib/tkinter/fontchooser.py`

--------------

The :mod:`!tkinter.fontchooser` module provides the :class:`FontChooser` class
as an interface to the native font selection dialog.

The font dialog is application-global:
there is a single font dialog per Tcl interpreter,
and all :class:`FontChooser` instances configure the same dialog.

Depending on the platform, the dialog may be modal or modeless,
so :meth:`~FontChooser.show` may return immediately.
The selected font is not returned:
it is passed as a :class:`~tkinter.font.Font` object to the callback
specified with the *command* option.

The dialog also generates two virtual events on the parent window
(see the *parent* option):

``<<TkFontchooserVisibility>>``
   Generated when the dialog is shown or hidden.
   Query the *visible* option to tell which.

``<<TkFontchooserFontChanged>>``
   Generated when the selected font changes.

.. note::

   The *command* callback is the only reliable way to obtain the selected font.
   On some platforms the *font* option is not updated to the user's choice.

.. class:: FontChooser(master=None, **options)

   The class implementing the font selection dialog.

   *master* is the widget whose Tcl interpreter owns the dialog.
   If omitted, it defaults to *parent* if that is given,
   or to the default root window otherwise.

   The supported configuration options are:

   * *parent* --- the window to which the dialog and its virtual events are related.
     It defaults to the main window;
     on macOS the dialog is shown as a sheet attached to it,
     rather than as a free-standing panel.
   * *title* --- the title of the dialog.
   * *font* --- the font that is currently selected in the dialog.
   * *command* --- a callback that is called
     with a :class:`~tkinter.font.Font` object wrapping the selected font
     when the user selects a font.
   * *visible* --- whether the dialog is currently displayed (read-only).

   The *font* option accepts the forms supported by :class:`tkinter.font.Font`.

   .. method:: configure(**options)
               config(**options)

      Query or modify the options of the font dialog.
      With no arguments, return a dict of all option values.
      With a string argument, return the value of that option.
      Otherwise, set the given options.

   .. method:: cget(option)

      Return the value of the given option of the font dialog.

   .. method:: show()

      Display the font dialog.
      Depending on the platform, this method may return immediately
      or only once the dialog has been withdrawn.

   .. method:: hide()

      Hide the font dialog if it is displayed.


.. seealso::

   Module :mod:`tkinter.font`
      Tkinter font-handling utilities
