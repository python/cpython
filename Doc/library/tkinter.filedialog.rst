:mod:`tkinter.filedialog` --- File selection dialogs
====================================================

.. module:: tkinter.filedialog
   :platform: Tk
   :synopsis: Dialog classes for file selection

**Source code:** :source:`Lib/tkinter/filedialog.py`

--------------

The :mod:`tkinter.filedialog` module provides classes and factory methods that facilitate file/directory selection. 


.. FIXME Methods that should (or should not) be documented should be added in
   (or removed) as appropriate

.. class:: Directory(self, master=None, **options)

   Open dialog prompting user to select a directory
   

.. class:: FileDialog(self, master, title=None)

   Standard file selection dialog
   
    .. function:: cancel_command(self, event=None)

       Exit the file selection dialog
   
    .. function:: dirs_double_event(self, event)

       Event handler for double-click event on directory

    .. function:: dirs_select_event(self, event)

       Event handler for click event on directory

    .. function:: files_double_event(self, event)

       Event handler for double-click event on file

    .. function:: files_select_event(self, event)

       Event handler for double-click event on file

    .. function:: filter_command(self, event=None)

       Filters the files by directory

    .. function:: get_filter(self)

       Retrieve the file filter currently in use

    .. function:: get_selection(self)

       Retrieve the currently selected item
   
    .. function:: go(self, dir_or_file=os.curdir, pattern="*", default="", key=None)
 
        Displays dialog and starts event loop
		
    .. function:: ok_event(self, event)

       Exit dialog returning current selection
   
    .. function:: quit(self, how=None)

       Exit dialog returning filename, if any

    .. function:: set_filter(self, dir, pat)

        Set the file filter

    .. function:: set_selection(self, file)


.. class:: LoadFileDialog(self, master, title=None)

   File selection dialog that checks that the file exists
   
    .. function:: ok_command(self)

       Exit dialog returning current selection

	   
.. class:: Open(self, master=None, **options)

   Native tk dialog for retrieving filename to open

   
.. class:: SaveAs(self, master=None, **options)

   Native tk dialog for retrieving filename for saving	   

   
.. class:: SaveFileDialog(self, master, title=None)

   File selection dialog that checks that the file can be created

    .. function:: ok_command(self)

       Exit dialog returning current selection, prompting overwrite if existing
	   
   
   
.. Static factory methods
   
.. method:: askdirectory (**options)

   Prompt user to select a directory
   
.. method:: askopenfile(mode = "r", **options)
   
   Prompt for a filename to open, and returned the opened file
   
.. method:: askopenfilename(**options)

   Static factory method for Open dialog (single selection)

.. method:: askopenfilenames(**options)

   Static factory method for Open dialog (multi-selection)
   
.. method:: askopenfiles(mode = "r", **options)

   Prompt for filenames (multi-selection) and return opened file object(s)

.. method:: asksaveasfile(mode = "w", **options)

   Prompt for filename to write to and return opened file object
   
.. method:: asksaveasfilename(**options)

   Static factory method for 'Save As' dialog

   
.. seealso::

   Module :mod:`tkinter.commondialog`
      Tkinter standard dialog module