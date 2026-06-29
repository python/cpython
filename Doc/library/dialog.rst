Tkinter dialogs
===============

:mod:`!tkinter.simpledialog` --- Standard Tkinter input dialogs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. module:: tkinter.simpledialog
   :synopsis: Simple dialog windows

**Source code:** :source:`Lib/tkinter/simpledialog.py`

--------------

The :mod:`!tkinter.simpledialog` module contains convenience classes and
functions for creating simple modal dialogs to get a value from the user.


.. function:: askfloat(title, prompt, *, initialvalue=None, minvalue=None, maxvalue=None, parent=None)
              askinteger(title, prompt, *, initialvalue=None, minvalue=None, maxvalue=None, parent=None)
              askstring(title, prompt, *, initialvalue=None, show=None, parent=None)

   Prompt the user to enter a value of the desired type and return it, or
   ``None`` if the dialog is cancelled.

   *title* is the dialog title and *prompt* the message shown above the entry.
   *initialvalue* is the value initially placed in the entry.
   *parent* is the window over which the dialog is shown.
   :func:`askinteger` and :func:`askfloat` also accept *minvalue* and
   *maxvalue*, which bound the accepted value.
   :func:`askstring` also accepts *show*, a character used to mask the entered
   text, for example ``'*'`` to hide a password.

.. class:: Dialog(parent, title=None)

   The base class for custom dialogs.
   Instantiating it shows the dialog modally and returns once the user closes
   it; the entered value is then available in the :attr:`!result` attribute.

   .. attribute:: result

      The value produced by :meth:`apply`, or ``None`` if the dialog was
      cancelled.

   .. method:: body(master)

      Override to construct the dialog's interface and return the widget that
      should have initial focus.

   .. method:: buttonbox()

      Default behaviour adds OK and Cancel buttons. Override for custom button
      layouts.

   .. method:: validate()

      Validate the data entered by the user.
      Return true if it is valid, in which case the dialog proceeds to
      :meth:`apply`; return false to keep the dialog open.
      The default implementation always returns true; override it to check the
      input.

   .. method:: apply()

      Process the data entered by the user, for example by storing it in the
      :attr:`!result` attribute.
      Called after :meth:`validate` succeeds and just before the dialog is
      destroyed.
      The default implementation does nothing; override it to act on or store
      the result.

   .. method:: destroy()

      Destroy the dialog window, clearing the reference to the widget that had
      the initial focus.


.. class:: SimpleDialog(master, text='', buttons=[], default=None, cancel=None, title=None, class_=None)

   A simple modal dialog that displays the message *text* above a row of push
   buttons whose labels are given by *buttons*, and returns the index of the
   button the user presses.
   *default* is the index of the button activated by the Return key, *cancel*
   the index returned when the window is closed through the window manager,
   *title* the window title, and *class_* the Tk class name of the window.

   .. method:: go()

      Display the dialog, wait until the user presses a button or closes the
      window, and return the index of the chosen button.



:mod:`!tkinter.filedialog` --- File selection dialogs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. module:: tkinter.filedialog
   :synopsis: Dialog classes for file selection

**Source code:** :source:`Lib/tkinter/filedialog.py`

--------------

The :mod:`!tkinter.filedialog` module provides classes and factory functions for
creating file/directory selection windows.

Native load/save dialogs
------------------------

The following classes and functions provide file dialog windows that combine a
native look-and-feel with configuration options to customize behaviour.
The following keyword arguments are applicable to the classes and functions
listed below:

 | *parent* - the window to place the dialog on top of

 | *title* - the title of the window

 | *initialdir* - the directory that the dialog starts in

 | *initialfile* - the file selected upon opening of the dialog

 | *filetypes* - a sequence of (label, pattern) tuples, '*' wildcard is allowed

 | *defaultextension* - default extension to append to file (save dialogs)

 | *multiple* - when true, selection of multiple items is allowed


**Static factory functions**

The below functions when called create a modal, native look-and-feel dialog,
wait for the user's selection, and return it.
The exact return value depends on the function (see below); when the dialog is
cancelled it is an empty string, an empty tuple or ``None``.
The precise type of this empty value may vary between platforms and Tk
versions, so test the result for truth rather than comparing it with a
specific value.

.. function:: askopenfile(mode="r", **options)
              askopenfiles(mode="r", **options)

   Create an :class:`Open` dialog.
   :func:`askopenfile` returns the opened file object, or ``None`` if the
   dialog is cancelled.
   :func:`askopenfiles` returns a list of the opened file objects, or an empty
   tuple if cancelled.
   The files are opened in mode *mode* (read-only ``'r'`` by default).

.. function:: asksaveasfile(mode="w", **options)

   Create a :class:`SaveAs` dialog and return the opened file object, or
   ``None`` if the dialog is cancelled.
   The file is opened in mode *mode* (``'w'`` by default).

.. function:: askopenfilename(**options)
              askopenfilenames(**options)

   Create an :class:`Open` dialog.
   :func:`askopenfilename` returns the selected filename as a string, or an
   empty string if the dialog is cancelled.
   :func:`askopenfilenames` returns a tuple of the selected filenames, or an
   empty tuple if cancelled.

.. function:: asksaveasfilename(**options)

   Create a :class:`SaveAs` dialog and return the selected filename as a
   string, or an empty string if the dialog is cancelled.

.. function:: askdirectory(**options)

   Prompt the user to select a directory, and return its path as a string, or
   an empty string if the dialog is cancelled.
   Additional keyword option: *mustexist* - if true, the user may only select
   an existing directory (false by default).

.. class:: Open(master=None, **options)
           SaveAs(master=None, **options)

   The above two classes provide native dialog windows for saving and loading
   files.

**Convenience classes**

The below classes are used for creating file/directory windows from scratch.
These do not emulate the native look-and-feel of the platform.

.. class:: Directory(master=None, **options)

   Create a dialog prompting the user to select a directory.

.. note::  The *FileDialog* class should be subclassed for custom event
   handling and behaviour.

.. class:: FileDialog(master, title=None)

   Create a basic file selection dialog.

   .. method:: cancel_command(event=None)

      Trigger the termination of the dialog window.

   .. method:: dirs_double_event(event)

      Event handler for double-click event on directory.

   .. method:: dirs_select_event(event)

      Event handler for click event on directory.

   .. method:: files_double_event(event)

      Event handler for double-click event on file.

   .. method:: files_select_event(event)

      Event handler for single-click event on file.

   .. method:: filter_command(event=None)

      Filter the files by directory.

   .. method:: get_filter()

      Retrieve the file filter currently in use.

   .. method:: get_selection()

      Retrieve the currently selected item.

   .. method:: go(dir_or_file=os.curdir, pattern="*", default="", key=None)

      Render dialog and start event loop.

   .. method:: ok_event(event)

      Exit dialog returning current selection.

   .. method:: ok_command()

      Called when the user confirms the current selection.
      The base implementation accepts the selection and closes the dialog;
      :class:`LoadFileDialog` and :class:`SaveFileDialog` override it to check
      the selection first.

   .. method:: quit(how=None)

      Exit dialog returning filename, if any.

   .. method:: set_filter(dir, pat)

      Set the file filter.

   .. method:: set_selection(file)

      Update the current file selection to *file*.


.. class:: LoadFileDialog(master, title=None)

   A subclass of FileDialog that creates a dialog window for selecting an
   existing file.

   .. method:: ok_command()

      Test that a file is provided and that the selection indicates an
      already existing file.

.. class:: SaveFileDialog(master, title=None)

   A subclass of FileDialog that creates a dialog window for selecting a
   destination file.

   .. method:: ok_command()

      Test whether or not the selection points to a valid file that is not a
      directory. Confirmation is required if an already existing file is
      selected.

:mod:`!tkinter.commondialog` --- Dialog window templates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. module:: tkinter.commondialog
   :synopsis: Tkinter base class for dialogs

**Source code:** :source:`Lib/tkinter/commondialog.py`

--------------

The :mod:`!tkinter.commondialog` module provides the :class:`Dialog` class that
is the base class for dialogs defined in other supporting modules.

.. class:: Dialog(master=None, **options)

   .. method:: show(**options)

      Render the Dialog window.


:mod:`!tkinter.dialog` --- Classic Tk dialog boxes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. module:: tkinter.dialog
   :synopsis: A simple dialog box built on the classic Tk widgets.

**Source code:** :source:`Lib/tkinter/dialog.py`

--------------

The :mod:`!tkinter.dialog` module provides a simple modal dialog box built on
the classic (non-themed) Tk widgets.

.. data:: DIALOG_ICON

   The name of the default bitmap (``'questhead'``) displayed by a
   :class:`Dialog`.

.. class:: Dialog(master=None, cnf={}, **kw)

   Display a modal dialog box built from the classic (non-themed) Tk widgets
   and wait for the user to press one of its buttons.
   The options, given through *cnf* or as keyword arguments, include *title*
   (the window title), *text* (the message), *bitmap* (an icon,
   :data:`DIALOG_ICON` by default), *default* (the index of the default button)
   and *strings* (the sequence of button labels).
   After construction, the :attr:`!num` attribute holds the index of the button
   the user pressed.

   .. method:: destroy()

      Destroy the dialog window.


.. seealso::

   Modules :mod:`tkinter.messagebox`, :ref:`tut-files`
