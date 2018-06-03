:mod:`tkinter.filedialog` --- File selection dialogs
====================================================

.. module:: tkinter.filedialog
   :platform: Tk
   :synopsis: Dialog classes for file selection

**Source code:** :source:`Lib/tkinter/filedialog.py`

--------------

The :mod:`tkinter.filedialog` module provides classes and factory functions for
creating file/directory selection windows.

Native Load/Save Dialogs
^^^^^^^^^^^^^^^^^^^^^^^^

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

 | *multiple* - when True, selection of multiple items is allowed


**Static factory functions**

The below functions when called create a modal, native look-and-feel dialog,
wait for the user's selection, then return the selected value(s) or None to the
caller.

.. function:: askopenfile(mode="r", **options)
              askopenfiles(mode = "r", **options)

   The above two functions create an :class:`Open` dialog and return the opened
   file object(s) in read-only mode.

.. function:: asksaveasfile(mode="w", **options)

   Creates a :class:`SaveAs` dialog and returns a file object opened in write-
   only mode.

.. function:: askopenfilename(**options)
              askopenfilenames(**options)

   The above two functions create an :class:`Open` dialog and return the
   selected filename(s) that correspond to existing file(s).

.. function:: asksaveasfilename(**options)

   Creates a :class:`SaveAs` dialog and returns the selected filename.

.. function:: askdirectory (**options)

 | Prompt user to select a directory.
 | Additional keyword option:
 |  *mustexist* - determines if selection must be an existing directory.

.. class:: Open(master=None, **options)
           SaveAs(master=None, **options)

   The above two classes provide native dialog windows for saving and loading
   files.

**Convenience classes**

The below classes are used for creating file/directory windows from scratch.
These do not emulate the native look-and-feel of the platform.

.. class:: Directory(master=None, **options)

   Creates a dialog prompting user to select a directory.

.. note::  The *FileDialog* class should be subclassed for custom event
   handling and behaviour.

.. class:: FileDialog(master, title=None)

   Creates a basic file selection dialog.

   .. method:: cancel_command(event=None)

      Triggers the termination of the dialog window.

   .. method:: dirs_double_event(event)

      Event handler for double-click event on directory.

   .. method:: dirs_select_event(event)

      Event handler for click event on directory.

   .. method:: files_double_event(event)

      Event handler for double-click event on file.

   .. method:: files_select_event(event)

      Event handler for single-click event on file.

   .. method:: filter_command(event=None)

      Filters the files by directory.

   .. method:: get_filter()

      Retrieve the file filter currently in use.

   .. method:: get_selection()

      Retrieve the currently selected item.

   .. method:: go(dir_or_file=os.curdir, pattern="*", default="", key=None)

      Displays dialog and starts event loop.

   .. method:: ok_event(event)

      Exit dialog returning current selection.

   .. method:: quit(how=None)

      Exit dialog returning filename, if any.

   .. method:: set_filter(dir, pat)

      Set the file filter.

   .. method:: set_selection(file)

      Updates the current file selection to *file*.


.. class:: LoadFileDialog

   A subclass of FileDialog that creates a dialog window for selecting an
   existing file.

   .. method:: ok_command()

      Tests that a file is provided and that the selection indicates an
      already existing file.

.. class:: SaveFileDialog

   A subclass of FileDialog that creates a dialog window for selecting a
   destination file.

    .. method:: ok_command()

      Tests whether or not the selection points to a valid file that is not a
      directory. Confirmation is required if an already existing file is
      selected.

.. seealso::

   :mod:`tkinter.commondialog`, :ref:`tut-files`