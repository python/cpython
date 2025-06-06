:mod:`tkinter.tix` --- Extension widgets for Tk
===============================================

.. module:: tkinter.tix
   :synopsis: Tk Extension Widgets for Tkinter

.. sectionauthor:: Mike Clarkson <mikeclarkson@users.sourceforge.net>

**Source code:** :source:`Lib/tkinter/tix.py`

.. index:: single: Tix

.. deprecated:: 3.6
   This Tk extension is unmaintained and should not be used in new code.  Use
   :mod:`tkinter.ttk` instead.

--------------

The :mod:`tkinter.tix` (Tk Interface Extension) module provides an additional
rich set of widgets. Although the standard Tk library has many useful widgets,
they are far from complete. The :mod:`tkinter.tix` library provides most of the
commonly needed widgets that are missing from standard Tk: :class:`HList`,
:class:`ComboBox`, :class:`Control` (a.k.a. SpinBox) and an assortment of
scrollable widgets.
:mod:`tkinter.tix` also includes many more widgets that are generally useful in
a wide range of applications: :class:`NoteBook`, :class:`FileEntry`,
:class:`PanedWindow`, etc; there are more than 40 of them.

With all these new widgets, you can introduce new interaction techniques into
applications, creating more useful and more intuitive user interfaces. You can
design your application by choosing the most appropriate widgets to match the
special needs of your application and users.

.. seealso::

   `Tix Homepage <https://tix.sourceforge.net/>`_
      The home page for :mod:`Tix`.  This includes links to additional documentation
      and downloads.

   `Tix Man Pages <https://tix.sourceforge.net/dist/current/man/>`_
      On-line version of the man pages and reference material.

   `Tix Programming Guide <https://tix.sourceforge.net/dist/current/docs/tix-book/tix.book.html>`_
      On-line version of the programmer's reference material.

   `Tix Development Applications <https://tix.sourceforge.net/Tixapps/src/Tide.html>`_
      Tix applications for development of Tix and Tkinter programs. Tide applications
      work under Tk or Tkinter, and include :program:`TixInspect`, an inspector to
      remotely modify and debug Tix/Tk/Tkinter applications.


Using Tix
---------


.. class:: Tk(screenName=None, baseName=None, className='Tix')

   Toplevel widget of Tix which represents mostly the main window of an
   application. It has an associated Tcl interpreter.

   Classes in the :mod:`tkinter.tix` module subclasses the classes in the
   :mod:`tkinter`. The former imports the latter, so to use :mod:`tkinter.tix`
   with Tkinter, all you need to do is to import one module. In general, you
   can just import :mod:`tkinter.tix`, and replace the toplevel call to
   :class:`tkinter.Tk` with :class:`tix.Tk`::

      from tkinter import tix
      from tkinter.constants import *
      root = tix.Tk()

To use :mod:`tkinter.tix`, you must have the Tix widgets installed, usually
alongside your installation of the Tk widgets. To test your installation, try
the following::

   from tkinter import tix
   root = tix.Tk()
   root.tk.eval('package require Tix')


Tix Widgets
-----------

`Tix <https://tix.sourceforge.net/dist/current/man/html/TixCmd/TixIntro.htm>`_
introduces over 40 widget classes to the :mod:`tkinter` repertoire.


Basic Widgets
^^^^^^^^^^^^^


.. class:: Balloon()

   A `Balloon
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixBalloon.htm>`_ that
   pops up over a widget to provide help.  When the user moves the cursor inside a
   widget to which a Balloon widget has been bound, a small pop-up window with a
   descriptive message will be shown on the screen.

.. Python Demo of:
.. \ulink{Balloon}{https://tix.sourceforge.net/dist/current/demos/samples/Balloon.tcl}


.. class:: ButtonBox()

   The `ButtonBox
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixButtonBox.htm>`_
   widget creates a box of buttons, such as is commonly used for ``Ok Cancel``.

.. Python Demo of:
.. \ulink{ButtonBox}{https://tix.sourceforge.net/dist/current/demos/samples/BtnBox.tcl}


.. class:: ComboBox()

   The `ComboBox
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixComboBox.htm>`_
   widget is similar to the combo box control in MS Windows. The user can select a
   choice by either typing in the entry subwidget or selecting from the listbox
   subwidget.

.. Python Demo of:
.. \ulink{ComboBox}{https://tix.sourceforge.net/dist/current/demos/samples/ComboBox.tcl}


.. class:: Control()

   The `Control
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixControl.htm>`_
   widget is also known as the :class:`SpinBox` widget. The user can adjust the
   value by pressing the two arrow buttons or by entering the value directly into
   the entry. The new value will be checked against the user-defined upper and
   lower limits.

.. Python Demo of:
.. \ulink{Control}{https://tix.sourceforge.net/dist/current/demos/samples/Control.tcl}


.. class:: LabelEntry()

   The `LabelEntry
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixLabelEntry.htm>`_
   widget packages an entry widget and a label into one mega widget. It can
   be used to simplify the creation of "entry-form" type of interface.

.. Python Demo of:
.. \ulink{LabelEntry}{https://tix.sourceforge.net/dist/current/demos/samples/LabEntry.tcl}


.. class:: LabelFrame()

   The `LabelFrame
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixLabelFrame.htm>`_
   widget packages a frame widget and a label into one mega widget.  To create
   widgets inside a LabelFrame widget, one creates the new widgets relative to the
   :attr:`frame` subwidget and manage them inside the :attr:`frame` subwidget.

.. Python Demo of:
.. \ulink{LabelFrame}{https://tix.sourceforge.net/dist/current/demos/samples/LabFrame.tcl}


.. class:: Meter()

   The `Meter
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixMeter.htm>`_ widget
   can be used to show the progress of a background job which may take a long time
   to execute.

.. Python Demo of:
.. \ulink{Meter}{https://tix.sourceforge.net/dist/current/demos/samples/Meter.tcl}


.. class:: OptionMenu()

   The `OptionMenu
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixOptionMenu.htm>`_
   creates a menu button of options.

.. Python Demo of:
.. \ulink{OptionMenu}{https://tix.sourceforge.net/dist/current/demos/samples/OptMenu.tcl}


.. class:: PopupMenu()

   The `PopupMenu
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixPopupMenu.htm>`_
   widget can be used as a replacement of the ``tk_popup`` command. The advantage
   of the :mod:`Tix` :class:`PopupMenu` widget is it requires less application code
   to manipulate.

.. Python Demo of:
.. \ulink{PopupMenu}{https://tix.sourceforge.net/dist/current/demos/samples/PopMenu.tcl}


.. class:: Select()

   The `Select
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixSelect.htm>`_ widget
   is a container of button subwidgets. It can be used to provide radio-box or
   check-box style of selection options for the user.

.. Python Demo of:
.. \ulink{Select}{https://tix.sourceforge.net/dist/current/demos/samples/Select.tcl}


.. class:: StdButtonBox()

   The `StdButtonBox
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixStdButtonBox.htm>`_
   widget is a group of standard buttons for Motif-like dialog boxes.

.. Python Demo of:
.. \ulink{StdButtonBox}{https://tix.sourceforge.net/dist/current/demos/samples/StdBBox.tcl}


File Selectors
^^^^^^^^^^^^^^


.. class:: DirList()

   The `DirList
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixDirList.htm>`_
   widget displays a list view of a directory, its previous directories and its
   sub-directories. The user can choose one of the directories displayed in the
   list or change to another directory.

.. Python Demo of:
.. \ulink{DirList}{https://tix.sourceforge.net/dist/current/demos/samples/DirList.tcl}


.. class:: DirTree()

   The `DirTree
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixDirTree.htm>`_
   widget displays a tree view of a directory, its previous directories and its
   sub-directories. The user can choose one of the directories displayed in the
   list or change to another directory.

.. Python Demo of:
.. \ulink{DirTree}{https://tix.sourceforge.net/dist/current/demos/samples/DirTree.tcl}


.. class:: DirSelectDialog()

   The `DirSelectDialog
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixDirSelectDialog.htm>`_
   widget presents the directories in the file system in a dialog window.  The user
   can use this dialog window to navigate through the file system to select the
   desired directory.

.. Python Demo of:
.. \ulink{DirSelectDialog}{https://tix.sourceforge.net/dist/current/demos/samples/DirDlg.tcl}


.. class:: DirSelectBox()

   The :class:`DirSelectBox` is similar to the standard Motif(TM)
   directory-selection box. It is generally used for the user to choose a
   directory.  DirSelectBox stores the directories mostly recently selected into
   a ComboBox widget so that they can be quickly selected again.


.. class:: ExFileSelectBox()

   The `ExFileSelectBox
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixExFileSelectBox.htm>`_
   widget is usually embedded in a tixExFileSelectDialog widget. It provides a
   convenient method for the user to select files. The style of the
   :class:`ExFileSelectBox` widget is very similar to the standard file dialog on
   MS Windows 3.1.

.. Python Demo of:
.. \ulink{ExFileSelectDialog}{https://tix.sourceforge.net/dist/current/demos/samples/EFileDlg.tcl}


.. class:: FileSelectBox()

   The `FileSelectBox
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixFileSelectBox.htm>`_
   is similar to the standard Motif(TM) file-selection box. It is generally used
   for the user to choose a file. FileSelectBox stores the files mostly recently
   selected into a :class:`ComboBox` widget so that they can be quickly selected
   again.

.. Python Demo of:
.. \ulink{FileSelectDialog}{https://tix.sourceforge.net/dist/current/demos/samples/FileDlg.tcl}


.. class:: FileEntry()

   The `FileEntry
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixFileEntry.htm>`_
   widget can be used to input a filename. The user can type in the filename
   manually. Alternatively, the user can press the button widget that sits next to
   the entry, which will bring up a file selection dialog.

.. Python Demo of:
.. \ulink{FileEntry}{https://tix.sourceforge.net/dist/current/demos/samples/FileEnt.tcl}


Hierarchical ListBox
^^^^^^^^^^^^^^^^^^^^


.. class:: HList()

   The `HList
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixHList.htm>`_ widget
   can be used to display any data that have a hierarchical structure, for example,
   file system directory trees. The list entries are indented and connected by
   branch lines according to their places in the hierarchy.

.. Python Demo of:
.. \ulink{HList}{https://tix.sourceforge.net/dist/current/demos/samples/HList1.tcl}


.. class:: CheckList()

   The `CheckList
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixCheckList.htm>`_
   widget displays a list of items to be selected by the user. CheckList acts
   similarly to the Tk checkbutton or radiobutton widgets, except it is capable of
   handling many more items than checkbuttons or radiobuttons.

.. Python Demo of:
.. \ulink{ CheckList}{https://tix.sourceforge.net/dist/current/demos/samples/ChkList.tcl}
.. Python Demo of:
.. \ulink{ScrolledHList (1)}{https://tix.sourceforge.net/dist/current/demos/samples/SHList.tcl}
.. Python Demo of:
.. \ulink{ScrolledHList (2)}{https://tix.sourceforge.net/dist/current/demos/samples/SHList2.tcl}


.. class:: Tree()

   The `Tree
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixTree.htm>`_ widget
   can be used to display hierarchical data in a tree form. The user can adjust the
   view of the tree by opening or closing parts of the tree.

.. Python Demo of:
.. \ulink{Tree}{https://tix.sourceforge.net/dist/current/demos/samples/Tree.tcl}
.. Python Demo of:
.. \ulink{Tree (Dynamic)}{https://tix.sourceforge.net/dist/current/demos/samples/DynTree.tcl}


Tabular ListBox
^^^^^^^^^^^^^^^


.. class:: TList()

   The `TList
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixTList.htm>`_ widget
   can be used to display data in a tabular format. The list entries of a
   :class:`TList` widget are similar to the entries in the Tk listbox widget.  The
   main differences are (1) the :class:`TList` widget can display the list entries
   in a two dimensional format and (2) you can use graphical images as well as
   multiple colors and fonts for the list entries.

.. Python Demo of:
.. \ulink{ScrolledTList (1)}{https://tix.sourceforge.net/dist/current/demos/samples/STList1.tcl}
.. Python Demo of:
.. \ulink{ScrolledTList (2)}{https://tix.sourceforge.net/dist/current/demos/samples/STList2.tcl}
.. Grid has yet to be added to Python
.. \subsubsection{Grid Widget}
.. Python Demo of:
.. \ulink{Simple Grid}{https://tix.sourceforge.net/dist/current/demos/samples/SGrid0.tcl}
.. Python Demo of:
.. \ulink{ScrolledGrid}{https://tix.sourceforge.net/dist/current/demos/samples/SGrid1.tcl}
.. Python Demo of:
.. \ulink{Editable Grid}{https://tix.sourceforge.net/dist/current/demos/samples/EditGrid.tcl}


Manager Widgets
^^^^^^^^^^^^^^^


.. class:: PanedWindow()

   The `PanedWindow
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixPanedWindow.htm>`_
   widget allows the user to interactively manipulate the sizes of several panes.
   The panes can be arranged either vertically or horizontally.  The user changes
   the sizes of the panes by dragging the resize handle between two panes.

.. Python Demo of:
.. \ulink{PanedWindow}{https://tix.sourceforge.net/dist/current/demos/samples/PanedWin.tcl}


.. class:: ListNoteBook()

   The `ListNoteBook
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixListNoteBook.htm>`_
   widget is very similar to the :class:`TixNoteBook` widget: it can be used to
   display many windows in a limited space using a notebook metaphor. The notebook
   is divided into a stack of pages (windows). At one time only one of these pages
   can be shown. The user can navigate through these pages by choosing the name of
   the desired page in the :attr:`hlist` subwidget.

.. Python Demo of:
.. \ulink{ListNoteBook}{https://tix.sourceforge.net/dist/current/demos/samples/ListNBK.tcl}


.. class:: NoteBook()

   The `NoteBook
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixNoteBook.htm>`_
   widget can be used to display many windows in a limited space using a notebook
   metaphor. The notebook is divided into a stack of pages. At one time only one of
   these pages can be shown. The user can navigate through these pages by choosing
   the visual "tabs" at the top of the NoteBook widget.

.. Python Demo of:
.. \ulink{NoteBook}{https://tix.sourceforge.net/dist/current/demos/samples/NoteBook.tcl}

.. \subsubsection{Scrolled Widgets}
.. Python Demo of:
.. \ulink{ScrolledListBox}{https://tix.sourceforge.net/dist/current/demos/samples/SListBox.tcl}
.. Python Demo of:
.. \ulink{ScrolledText}{https://tix.sourceforge.net/dist/current/demos/samples/SText.tcl}
.. Python Demo of:
.. \ulink{ScrolledWindow}{https://tix.sourceforge.net/dist/current/demos/samples/SWindow.tcl}
.. Python Demo of:
.. \ulink{Canvas Object View}{https://tix.sourceforge.net/dist/current/demos/samples/CObjView.tcl}


Image Types
^^^^^^^^^^^

The :mod:`tkinter.tix` module adds:

* `pixmap <https://tix.sourceforge.net/dist/current/man/html/TixCmd/pixmap.htm>`_
  capabilities to all :mod:`tkinter.tix` and :mod:`tkinter` widgets to create
  color images from XPM files.

  .. Python Demo of:
  .. \ulink{XPM Image In Button}{https://tix.sourceforge.net/dist/current/demos/samples/Xpm.tcl}
  .. Python Demo of:
  .. \ulink{XPM Image In Menu}{https://tix.sourceforge.net/dist/current/demos/samples/Xpm1.tcl}

* `Compound
  <https://tix.sourceforge.net/dist/current/man/html/TixCmd/compound.htm>`_ image
  types can be used to create images that consists of multiple horizontal lines;
  each line is composed of a series of items (texts, bitmaps, images or spaces)
  arranged from left to right. For example, a compound image can be used to
  display a bitmap and a text string simultaneously in a Tk :class:`Button`
  widget.

  .. Python Demo of:
  .. \ulink{Compound Image In Buttons}{https://tix.sourceforge.net/dist/current/demos/samples/CmpImg.tcl}
  .. Python Demo of:
  .. \ulink{Compound Image In NoteBook}{https://tix.sourceforge.net/dist/current/demos/samples/CmpImg2.tcl}
  .. Python Demo of:
  .. \ulink{Compound Image Notebook Color Tabs}{https://tix.sourceforge.net/dist/current/demos/samples/CmpImg4.tcl}
  .. Python Demo of:
  .. \ulink{Compound Image Icons}{https://tix.sourceforge.net/dist/current/demos/samples/CmpImg3.tcl}


Miscellaneous Widgets
^^^^^^^^^^^^^^^^^^^^^


.. class:: InputOnly()

   The `InputOnly
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixInputOnly.htm>`_
   widgets are to accept inputs from the user, which can be done with the ``bind``
   command (Unix only).


Form Geometry Manager
^^^^^^^^^^^^^^^^^^^^^

In addition, :mod:`tkinter.tix` augments :mod:`tkinter` by providing:


.. class:: Form()

   The `Form
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tixForm.htm>`_ geometry
   manager based on attachment rules for all Tk widgets.


Tix Commands
------------


.. class:: tixCommand()

   The `tix commands
   <https://tix.sourceforge.net/dist/current/man/html/TixCmd/tix.htm>`_ provide
   access to miscellaneous elements of :mod:`Tix`'s internal state and the
   :mod:`Tix` application context.  Most of the information manipulated by these
   methods pertains to the application as a whole, or to a screen or display,
   rather than to a particular window.

   To view the current settings, the common usage is::

      from tkinter import tix
      root = tix.Tk()
      print(root.tix_configure())


.. method:: tixCommand.tix_configure(cnf=None, **kw)

   Query or modify the configuration options of the Tix application context. If no
   option is specified, returns a dictionary all of the available options.  If
   option is specified with no value, then the method returns a list describing the
   one named option (this list will be identical to the corresponding sublist of
   the value returned if no option is specified).  If one or more option-value
   pairs are specified, then the method modifies the given option(s) to have the
   given value(s); in this case the method returns an empty string. Option may be
   any of the configuration options.


.. method:: tixCommand.tix_cget(option)

   Returns the current value of the configuration option given by *option*. Option
   may be any of the configuration options.


.. method:: tixCommand.tix_getbitmap(name)

   Locates a bitmap file of the name ``name.xpm`` or ``name`` in one of the bitmap
   directories (see the :meth:`tix_addbitmapdir` method).  By using
   :meth:`tix_getbitmap`, you can avoid hard coding the pathnames of the bitmap
   files in your application. When successful, it returns the complete pathname of
   the bitmap file, prefixed with the character ``@``.  The returned value can be
   used to configure the ``bitmap`` option of the Tk and Tix widgets.


.. method:: tixCommand.tix_addbitmapdir(directory)

   Tix maintains a list of directories under which the :meth:`tix_getimage` and
   :meth:`tix_getbitmap` methods will search for image files.  The standard bitmap
   directory is :file:`$TIX_LIBRARY/bitmaps`. The :meth:`tix_addbitmapdir` method
   adds *directory* into this list. By using this method, the image files of an
   applications can also be located using the :meth:`tix_getimage` or
   :meth:`tix_getbitmap` method.


.. method:: tixCommand.tix_filedialog([dlgclass])

   Returns the file selection dialog that may be shared among different calls from
   this application.  This method will create a file selection dialog widget when
   it is called the first time. This dialog will be returned by all subsequent
   calls to :meth:`tix_filedialog`.  An optional dlgclass parameter can be passed
   as a string to specified what type of file selection dialog widget is desired.
   Possible options are ``tix``, ``FileSelectDialog`` or ``tixExFileSelectDialog``.


.. method:: tixCommand.tix_getimage(self, name)

   Locates an image file of the name :file:`name.xpm`, :file:`name.xbm` or
   :file:`name.ppm` in one of the bitmap directories (see the
   :meth:`tix_addbitmapdir` method above). If more than one file with the same name
   (but different extensions) exist, then the image type is chosen according to the
   depth of the X display: xbm images are chosen on monochrome displays and color
   images are chosen on color displays. By using :meth:`tix_getimage`, you can
   avoid hard coding the pathnames of the image files in your application. When
   successful, this method returns the name of the newly created image, which can
   be used to configure the ``image`` option of the Tk and Tix widgets.


.. method:: tixCommand.tix_option_get(name)

   Gets the options maintained by the Tix scheme mechanism.


.. method:: tixCommand.tix_resetoptions(newScheme, newFontSet[, newScmPrio])

   Resets the scheme and fontset of the Tix application to *newScheme* and
   *newFontSet*, respectively.  This affects only those widgets created after this
   call.  Therefore, it is best to call the resetoptions method before the creation
   of any widgets in a Tix application.

   The optional parameter *newScmPrio* can be given to reset the priority level of
   the Tk options set by the Tix schemes.

   Because of the way Tk handles the X option database, after Tix has been has
   imported and inited, it is not possible to reset the color schemes and font sets
   using the :meth:`tix_config` method. Instead, the :meth:`tix_resetoptions`
   method must be used.
