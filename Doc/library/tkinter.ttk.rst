:mod:`!tkinter.ttk` --- Tk themed widgets
=========================================

.. module:: tkinter.ttk
   :synopsis: Tk themed widget set

**Source code:** :source:`Lib/tkinter/ttk.py`

.. index:: single: ttk

--------------

The :mod:`!tkinter.ttk` module provides access to the Tk themed widget set,
introduced in Tk 8.5.
Its widgets adapt their appearance to the platform's native theme,
giving an application a better and more consistent look and feel
than the classic :mod:`tkinter` widgets, whose appearance is fixed.

The basic idea for :mod:`!tkinter.ttk` is to separate, to the extent possible,
the code implementing a widget's behavior from the code implementing its
appearance.

Ttk widgets are used just like the classic :mod:`tkinter` widgets
and share the same machinery:
the widget hierarchy, the geometry managers, variable coupling and event binding.
Those foundational concepts are covered in the :mod:`tkinter` documentation
and are not repeated here.

.. versionadded:: 3.1


.. seealso::

   `Tk Widget Styling Support (TIP #48) <https://tip.tcl-lang.org/48.html>`_
      The Tcl Improvement Proposal that introduced the themed widget styling engine.


Using Ttk
---------

To start using Ttk, import its module::

   from tkinter import ttk

To override the basic Tk widgets, the import should follow the Tk import::

   from tkinter import *
   from tkinter.ttk import *

That code causes several :mod:`!tkinter.ttk` widgets (:class:`Button`,
:class:`Checkbutton`, :class:`Entry`, :class:`Frame`, :class:`Label`,
:class:`LabelFrame`, :class:`Menubutton`, :class:`OptionMenu`,
:class:`PanedWindow`, :class:`Radiobutton`, :class:`Scale`,
:class:`Scrollbar` and :class:`Spinbox`) to
automatically replace the Tk widgets.

.. note::

   Overriding the classic widgets with ``from tkinter.ttk import *``
   is convenient for adapting existing code,
   but new code is usually clearer if it imports the module
   as ``from tkinter import ttk`` and refers to the themed widgets explicitly,
   such as ``ttk.Button``.

This has the direct benefit of using the new widgets which gives a better look
and feel across platforms; however, the replacement widgets are not completely
compatible.
The main difference is that widget options such as ``fg``, ``bg`` and others
related to widget styling are no longer present in Ttk widgets.
Instead, use the :class:`ttk.Style <Style>` class for improved styling effects.


Ttk widgets
-----------

Ttk comes with 18 widgets, twelve of which already existed in tkinter:
:class:`Button`, :class:`Checkbutton`, :class:`Entry`, :class:`Frame`,
:class:`Label`, :class:`LabelFrame`, :class:`Menubutton`, :class:`PanedWindow`,
:class:`Radiobutton`, :class:`Scale`, :class:`Scrollbar`, and :class:`Spinbox`.
The other six are new: :class:`Combobox`, :class:`Notebook`,
:class:`Progressbar`, :class:`Separator`, :class:`Sizegrip` and
:class:`Treeview`. All of them are subclasses of :class:`Widget`.

Using the Ttk widgets gives the application an improved look and feel.
As discussed above, there are differences in how the styling is coded.

Tk code::

   l1 = tkinter.Label(text="Test", fg="black", bg="white")
   l2 = tkinter.Label(text="Test", fg="black", bg="white")


Ttk code::

   style = ttk.Style()
   style.configure("BW.TLabel", foreground="black", background="white")

   l1 = ttk.Label(text="Test", style="BW.TLabel")
   l2 = ttk.Label(text="Test", style="BW.TLabel")

For more information about TtkStyling_, see the :class:`Style` class
documentation.

Widget
------

:class:`ttk.Widget <Widget>` defines standard options and methods supported by
Tk themed widgets and is not supposed to be directly instantiated.


Standard options
^^^^^^^^^^^^^^^^

All the :mod:`!ttk` Widgets accept the following options:

.. tabularcolumns:: |l|L|

+-----------+--------------------------------------------------------------+
| Option    | Description                                                  |
+===========+==============================================================+
| class     | Specifies the window class. The class is used when querying  |
|           | the option database for the window's other options, to       |
|           | determine the default bindtags for the window, and to select |
|           | the widget's default layout and style. This option is        |
|           | read-only, and may only be specified when the window is      |
|           | created.                                                     |
+-----------+--------------------------------------------------------------+
| cursor    | Specifies the mouse cursor to be used for the widget.  See   |
|           | the *cursor* option type under :ref:`Tk-option-data-types`.  |
|           | If set to the empty string (the default), the cursor is      |
|           | inherited from the parent widget.                            |
+-----------+--------------------------------------------------------------+
| takefocus | Determines whether the window accepts the focus during       |
|           | keyboard traversal. 0, 1 or an empty string is returned.     |
|           | If 0 is returned, it means that the window should be skipped |
|           | entirely during keyboard traversal. If 1, it means that the  |
|           | window should receive the input focus as long as it is       |
|           | viewable. And an empty string means that the traversal       |
|           | scripts make the decision about whether or not to focus      |
|           | on the window.                                               |
+-----------+--------------------------------------------------------------+
| style     | May be used to specify a custom widget style.                |
+-----------+--------------------------------------------------------------+


Scrollable widget options
^^^^^^^^^^^^^^^^^^^^^^^^^

The following options are supported by widgets that are controlled by a
scrollbar.

.. tabularcolumns:: |l|L|

+----------------+---------------------------------------------------------+
| Option         | Description                                             |
+================+=========================================================+
| xscrollcommand | Used to communicate with horizontal scrollbars.         |
|                |                                                         |
|                | When the view in the widget's window changes, the widget|
|                | calls the *xscrollcommand* callback.                    |
|                |                                                         |
|                | Usually this option consists of the method              |
|                | :meth:`Scrollbar.set <tkinter.Scrollbar.set>` of some   |
|                | scrollbar. This will cause the scrollbar to be updated  |
|                | whenever the view in the window changes.                |
+----------------+---------------------------------------------------------+
| yscrollcommand | Used to communicate with vertical scrollbars.           |
|                | For some more information, see above.                   |
+----------------+---------------------------------------------------------+


Label options
^^^^^^^^^^^^^

The following options are supported by labels, buttons and other button-like
widgets.

.. tabularcolumns:: |l|p{0.7\linewidth}|

+--------------+-----------------------------------------------------------+
| Option       | Description                                               |
+==============+===========================================================+
| text         | Specifies a text string to be displayed inside the widget.|
+--------------+-----------------------------------------------------------+
| textvariable | Specifies a name whose value will be used in place of the |
|              | text option resource.                                     |
+--------------+-----------------------------------------------------------+
| underline    | If set, specifies the index (0-based) of a character to   |
|              | underline in the text string. The underline character is  |
|              | used for mnemonic activation.                             |
+--------------+-----------------------------------------------------------+
| image        | Specifies an image to display. This is a list of 1 or more|
|              | elements. The first element is the default image name. The|
|              | rest of the list if a sequence of statespec/value pairs as|
|              | defined by :meth:`Style.map`, specifying different images |
|              | to use when the widget is in a particular state or a      |
|              | combination of states. All images in the list should have |
|              | the same size.                                            |
+--------------+-----------------------------------------------------------+
| compound     | Specifies how to display the image relative to the text,  |
|              | in the case both text and images options are present.     |
|              | Valid values are:                                         |
|              |                                                           |
|              | * text: display text only                                 |
|              | * image: display image only                               |
|              | * top, bottom, left, right: display image above, below,   |
|              |   left of, or right of the text, respectively.            |
|              | * none: the default. display the image if present,        |
|              |   otherwise the text.                                     |
+--------------+-----------------------------------------------------------+
| width        | If greater than zero, specifies how much space, in        |
|              | character widths, to allocate for the text label, if less |
|              | than zero, specifies a minimum width. If zero or          |
|              | unspecified, the natural width of the text label is used. |
+--------------+-----------------------------------------------------------+


Compatibility options
^^^^^^^^^^^^^^^^^^^^^

.. tabularcolumns:: |l|L|

+--------+----------------------------------------------------------------+
| Option | Description                                                    |
+========+================================================================+
| state  | May be set to "normal" or "disabled" to control the "disabled" |
|        | state bit. This is a write-only option: setting it changes the |
|        | widget state, but the :meth:`Widget.state` method does not     |
|        | affect this option.                                            |
+--------+----------------------------------------------------------------+

Widget states
^^^^^^^^^^^^^

The widget state is a bitmap of independent state flags.

.. tabularcolumns:: |l|L|

+------------+-------------------------------------------------------------+
| Flag       | Description                                                 |
+============+=============================================================+
| active     | The mouse cursor is over the widget and pressing a mouse    |
|            | button will cause some action to occur                      |
+------------+-------------------------------------------------------------+
| disabled   | Widget is disabled under program control                    |
+------------+-------------------------------------------------------------+
| focus      | Widget has keyboard focus                                   |
+------------+-------------------------------------------------------------+
| pressed    | Widget is being pressed                                     |
+------------+-------------------------------------------------------------+
| selected   | "On", "true", or "current" for things like Checkbuttons and |
|            | radiobuttons                                                |
+------------+-------------------------------------------------------------+
| background | Windows and Mac have a notion of an "active" or foreground  |
|            | window. The *background* state is set for widgets in a      |
|            | background window, and cleared for those in the foreground  |
|            | window                                                      |
+------------+-------------------------------------------------------------+
| readonly   | Widget should not allow user modification                   |
+------------+-------------------------------------------------------------+
| alternate  | A widget-specific alternate display format                  |
+------------+-------------------------------------------------------------+
| invalid    | The widget's value is invalid                               |
+------------+-------------------------------------------------------------+

A state specification is a sequence of state names, optionally prefixed with
an exclamation point indicating that the bit is off.


ttk.Widget
^^^^^^^^^^

Besides the methods described below, the :class:`ttk.Widget <Widget>` supports
the methods :meth:`tkinter.Widget.cget <tkinter.Misc.cget>` and
:meth:`tkinter.Widget.configure <tkinter.Misc.configure>`.

.. class:: Widget

   .. method:: identify(x, y)

      Returns the name of the element at position *x* *y*, or the empty string
      if the point does not lie within any element.

      *x* and *y* are pixel coordinates relative to the widget.


   .. method:: instate(statespec, callback=None, *args, **kw)

      Test the widget's state. If a callback is not specified, returns ``True``
      if the widget state matches *statespec* and ``False`` otherwise. If callback
      is specified then it is called with args if widget state matches
      *statespec*.


   .. method:: state(statespec=None)

      Modify or inquire widget state. If *statespec* is specified, sets the
      widget state according to it and return a new *statespec* indicating
      which flags were changed. If *statespec* is not specified, returns
      the currently enabled state flags.

      Not to be confused with :meth:`Wm.state <tkinter.Wm.state>`.

   *statespec* will usually be a list or a tuple.


Combobox
--------

The :class:`ttk.Combobox <Combobox>` widget combines a text field with a
pop-down list of values.
This widget is a subclass of :class:`Entry`.

Besides the methods inherited from :class:`Widget`: :meth:`~tkinter.Misc.cget`,
:meth:`~tkinter.Misc.configure`, :meth:`~Widget.identify`,
:meth:`~Widget.instate` and :meth:`~Widget.state`, and the following inherited
from :class:`Entry`: :meth:`~Entry.bbox`, :meth:`~tkinter.Entry.delete`,
:meth:`~tkinter.Entry.icursor`, :meth:`~tkinter.Entry.index`,
:meth:`~tkinter.Entry.insert`,
:meth:`selection* <tkinter.Entry.selection_adjust>`,
:meth:`xview* <tkinter.XView.xview>`, it has some other methods, described at
:class:`ttk.Combobox <Combobox>`.


Options
^^^^^^^

This widget accepts the following specific options:

.. tabularcolumns:: |l|L|

+-----------------+--------------------------------------------------------+
| Option          | Description                                            |
+=================+========================================================+
| exportselection | Boolean value. If set, the widget selection is linked  |
|                 | to the X selection (which can be returned              |
|                 | by invoking Misc.selection_get, for example).          |
+-----------------+--------------------------------------------------------+
| justify         | Specifies how the text is aligned within the widget.   |
|                 | One of "left", "center", or "right".                   |
+-----------------+--------------------------------------------------------+
| height          | Specifies the height of the pop-down listbox, in rows. |
+-----------------+--------------------------------------------------------+
| postcommand     | A script (possibly registered with Misc.register) that |
|                 | is called immediately before displaying the values. It |
|                 | may specify which values to display.                   |
+-----------------+--------------------------------------------------------+
| state           | One of "normal", "readonly", or "disabled". In the     |
|                 | "readonly" state, the value may not be edited directly,|
|                 | and the user can only select one of the values from the|
|                 | dropdown list. In the "normal" state, the text field is|
|                 | directly editable. In the "disabled" state, no         |
|                 | interaction is possible.                               |
+-----------------+--------------------------------------------------------+
| textvariable    | Specifies a name whose value is linked to the widget   |
|                 | value. Whenever the value associated with that name    |
|                 | changes, the widget value is updated, and vice versa.  |
|                 | See :class:`tkinter.StringVar`.                        |
+-----------------+--------------------------------------------------------+
| values          | Specifies the list of values to display in the         |
|                 | drop-down listbox.                                     |
+-----------------+--------------------------------------------------------+
| width           | Specifies an integer value indicating the desired width|
|                 | of the entry window, in average-size characters of the |
|                 | widget's font.                                         |
+-----------------+--------------------------------------------------------+

.. note::

   Tk 9.1 added the *locale* option, which selects the locale used to determine
   word and character boundaries within the text (``"C"`` by default).


Virtual events
^^^^^^^^^^^^^^

The combobox widgets generates a **<<ComboboxSelected>>** virtual event
when the user selects an element from the list of values.


ttk.Combobox
^^^^^^^^^^^^

.. class:: Combobox

   .. method:: current(newindex=None)

      If *newindex* is specified, sets the combobox value to the element
      position *newindex*. Otherwise, returns the index of the current value or
      -1 if the current value is not in the values list.


   .. method:: get()

      Returns the current value of the combobox.


   .. method:: set(value)

      Sets the value of the combobox to *value*.


Spinbox
-------

The :class:`ttk.Spinbox <Spinbox>` widget is a :class:`ttk.Entry <Entry>`
enhanced with increment and decrement arrows.
It can be used for numbers or lists of string values.
This widget is a subclass of :class:`Entry`.
Besides the methods inherited from :class:`Widget`: :meth:`~tkinter.Misc.cget`,
:meth:`~tkinter.Misc.configure`, :meth:`~Widget.identify`,
:meth:`~Widget.instate` and :meth:`~Widget.state`, and the following inherited
from :class:`Entry`: :meth:`~Entry.bbox`, :meth:`~tkinter.Entry.delete`,
:meth:`~tkinter.Entry.icursor`, :meth:`~tkinter.Entry.index`,
:meth:`~tkinter.Entry.insert`, :meth:`xview* <tkinter.XView.xview>`, it has
some other methods, described at :class:`ttk.Spinbox <Spinbox>`.

Options
^^^^^^^

This widget accepts the following specific options:

.. tabularcolumns:: |l|L|

+----------------------+------------------------------------------------------+
| Option               | Description                                          |
+======================+======================================================+
| from                 | Float value.  If set, this is the minimum value to   |
|                      | which the decrement button will decrement.  Must be  |
|                      | spelled as ``from_`` when used as an argument, since |
|                      | ``from`` is a Python keyword.                        |
+----------------------+------------------------------------------------------+
| to                   | Float value.  If set, this is the maximum value to   |
|                      | which the increment button will increment.           |
+----------------------+------------------------------------------------------+
| increment            | Float value.  Specifies the amount which the         |
|                      | increment/decrement buttons change the               |
|                      | value. Defaults to 1.0.                              |
+----------------------+------------------------------------------------------+
| values               | Sequence of string or float values.  If specified,   |
|                      | the increment/decrement buttons will cycle through   |
|                      | the items in this sequence rather than incrementing  |
|                      | or decrementing numbers.                             |
|                      |                                                      |
+----------------------+------------------------------------------------------+
| wrap                 | Boolean value.  If ``True``, increment and decrement |
|                      | buttons will cycle from the ``to`` value to the      |
|                      | ``from`` value or the ``from`` value to the ``to``   |
|                      | value, respectively.                                 |
+----------------------+------------------------------------------------------+
| format               | String value.  This specifies the format of numbers  |
|                      | set by the increment/decrement buttons.  It must be  |
|                      | in the form "%W.Pf", where W is the padded width of  |
|                      | the value, P is the precision, and '%' and 'f' are   |
|                      | literal.                                             |
+----------------------+------------------------------------------------------+
| command              | Python callable.  Will be called with no arguments   |
|                      | whenever either of the increment or decrement buttons|
|                      | are pressed.                                         |
|                      |                                                      |
+----------------------+------------------------------------------------------+


Virtual events
^^^^^^^^^^^^^^

The spinbox widget generates an **<<Increment>>** virtual event when the
user presses <Up>, and a **<<Decrement>>** virtual event when the user
presses <Down>.

ttk.Spinbox
^^^^^^^^^^^^

.. class:: Spinbox

   With a non-integer increment, see :ref:`numeric values and the locale
   <tkinter-numeric-locale>`.

   .. versionadded:: 3.8

   .. method:: get()

      Returns the current value of the spinbox.


   .. method:: set(value)

      Sets the value of the spinbox to *value*.



Notebook
--------

Ttk Notebook widget manages a collection of windows and displays a single
one at a time. Each child window is associated with a tab, which the user
may select to change the currently displayed window.


Options
^^^^^^^

This widget accepts the following specific options:

.. tabularcolumns:: |l|L|

+---------+----------------------------------------------------------------+
| Option  | Description                                                    |
+=========+================================================================+
| height  | If present and greater than zero, specifies the desired height |
|         | of the pane area (not including internal padding or tabs).     |
|         | Otherwise, the maximum height of all panes is used.            |
+---------+----------------------------------------------------------------+
| padding | Specifies the amount of extra space to add around the outside  |
|         | of the notebook. The padding is a list up to four length       |
|         | specifications left top right bottom. If fewer than four       |
|         | elements are specified, bottom defaults to top, right defaults |
|         | to left, and top defaults to left.                             |
+---------+----------------------------------------------------------------+
| width   | If present and greater than zero, specified the desired width  |
|         | of the pane area (not including internal padding). Otherwise,  |
|         | the maximum width of all panes is used.                        |
+---------+----------------------------------------------------------------+


Tab options
^^^^^^^^^^^

There are also specific options for tabs:

.. tabularcolumns:: |l|L|

+-----------+--------------------------------------------------------------+
| Option    | Description                                                  |
+===========+==============================================================+
| state     | Either "normal", "disabled" or "hidden". If "disabled", then |
|           | the tab is not selectable. If "hidden", then the tab is not  |
|           | shown.                                                       |
+-----------+--------------------------------------------------------------+
| sticky    | Specifies how the child window is positioned within the pane |
|           | area. Value is a string containing zero or more of the       |
|           | characters "n", "s", "e" or "w". Each letter refers to a     |
|           | side (north, south, east or west) that the child window will |
|           | stick to, as per the :meth:`grid <tkinter.Grid.grid>`        |
|           | geometry manager.                                            |
+-----------+--------------------------------------------------------------+
| padding   | Specifies the amount of extra space to add between the       |
|           | notebook and this pane. Syntax is the same as for the option |
|           | padding used by this widget.                                 |
+-----------+--------------------------------------------------------------+
| text      | Specifies a text to be displayed in the tab.                 |
+-----------+--------------------------------------------------------------+
| image     | Specifies an image to display in the tab. See the option     |
|           | image described in :class:`Widget`.                          |
+-----------+--------------------------------------------------------------+
| compound  | Specifies how to display the image relative to the text, in  |
|           | the case both options text and image are present. See        |
|           | `Label Options`_ for legal values.                           |
+-----------+--------------------------------------------------------------+
| underline | Specifies the index (0-based) of a character to underline in |
|           | the text string. The underlined character is used for        |
|           | mnemonic activation if :meth:`Notebook.enable_traversal` is  |
|           | called.                                                      |
+-----------+--------------------------------------------------------------+


Tab identifiers
^^^^^^^^^^^^^^^

The tab_id present in several methods of :class:`ttk.Notebook <Notebook>` may
take any of the following forms:

* An integer between zero and the number of tabs
* The name of a child window
* A positional specification of the form "@x,y", which identifies the tab
* The literal string "current", which identifies the currently selected tab
* The literal string "end", which returns the number of tabs (only valid for
  :meth:`Notebook.index`)


Virtual events
^^^^^^^^^^^^^^

This widget generates a **<<NotebookTabChanged>>** virtual event after a new
tab is selected.


ttk.Notebook
^^^^^^^^^^^^

.. class:: Notebook

   .. method:: add(child, **kw)

      Adds a new tab to the notebook.

      If window is currently managed by the notebook but hidden, it is
      restored to its previous position.

      See `Tab Options`_ for the list of available options.


   .. method:: forget(tab_id)

      Removes the tab specified by *tab_id*, unmaps and unmanages the
      associated window.

      This shadows the inherited geometry-manager :meth:`!forget`;
      use :meth:`~tkinter.Pack.pack_forget`, :meth:`~tkinter.Grid.grid_forget`
      or :meth:`~tkinter.Place.place_forget` to remove the widget itself from
      its manager.


   .. method:: hide(tab_id)

      Hides the tab specified by *tab_id*.

      The tab will not be displayed, but the associated window remains
      managed by the notebook and its configuration remembered. Hidden tabs
      may be restored with the :meth:`add` command.


   .. method:: identify(x, y)

      Returns the name of the tab element at position *x*, *y*, or the empty
      string if none.


   .. method:: index(tab_id)

      Returns the numeric index of the tab specified by *tab_id*, or the total
      number of tabs if *tab_id* is the string "end".


   .. method:: insert(pos, child, **kw)

      Inserts a pane at the specified position.

      *pos* is either the string "end", an integer index, or the name of a
      managed child. If *child* is already managed by the notebook, moves it to
      the specified position.

      See `Tab Options`_ for the list of available options.


   .. method:: select(tab_id=None)

      Selects the specified *tab_id*.

      The associated child window will be displayed, and the
      previously selected window (if different) is unmapped. If *tab_id* is
      omitted, returns the widget name of the currently selected pane.


   .. method:: tab(tab_id, option=None, **kw)

      Query or modify the options of the specific *tab_id*.

      If *kw* is not given, returns a dictionary of the tab option values. If
      *option* is specified, returns the value of that *option*. Otherwise,
      sets the options to the corresponding values.


   .. method:: tabs()

      Returns a tuple of windows managed by the notebook.


   .. method:: enable_traversal()

      Enable keyboard traversal for a toplevel window containing this notebook.

      This will extend the bindings for the toplevel window containing the
      notebook as follows:

      * :kbd:`Control-Tab`: selects the tab following the currently selected one.
      * :kbd:`Shift-Control-Tab`: selects the tab preceding the currently selected one.
      * :kbd:`Alt-K`: where *K* is the mnemonic (underlined) character of any tab, will
        select that tab.

      Multiple notebooks in a single toplevel may be enabled for traversal,
      including nested notebooks.
      However, notebook traversal only works properly if all panes are direct
      children of the notebook.


Progressbar
-----------

The :class:`ttk.Progressbar <Progressbar>` widget shows the status of a
long-running operation.
It can operate in two modes: 1) the determinate mode which shows the amount
completed relative to the total amount of work to be done and 2) the
indeterminate mode which provides an animated display to let the user know that
work is progressing.


Options
^^^^^^^

This widget accepts the following specific options:

.. tabularcolumns:: |l|L|

+----------+---------------------------------------------------------------+
| Option   | Description                                                   |
+==========+===============================================================+
| orient   | One of "horizontal" or "vertical". Specifies the orientation  |
|          | of the progress bar.                                          |
+----------+---------------------------------------------------------------+
| length   | Specifies the length of the long axis of the progress bar     |
|          | (width if horizontal, height if vertical).                    |
+----------+---------------------------------------------------------------+
| mode     | One of "determinate" or "indeterminate".                      |
+----------+---------------------------------------------------------------+
| maximum  | A number specifying the maximum value. Defaults to 100.       |
+----------+---------------------------------------------------------------+
| value    | The current value of the progress bar. In "determinate" mode, |
|          | this represents the amount of work completed. In              |
|          | "indeterminate" mode, it is interpreted as modulo *maximum*;  |
|          | that is, the progress bar completes one "cycle" when its value|
|          | increases by *maximum*.                                       |
+----------+---------------------------------------------------------------+
| variable | A name which is linked to the option value. If specified, the |
|          | value of the progress bar is automatically set to the value of|
|          | this name whenever the latter is modified.                    |
+----------+---------------------------------------------------------------+
| phase    | Read-only option. The widget periodically increments the value|
|          | of this option whenever its value is greater than 0 and, in   |
|          | determinate mode, less than maximum. This option may be used  |
|          | by the current theme to provide additional animation effects. |
+----------+---------------------------------------------------------------+


ttk.Progressbar
^^^^^^^^^^^^^^^

.. class:: Progressbar

   .. method:: start(interval=None)

      Begin autoincrement mode: schedules a recurring timer event that calls
      :meth:`Progressbar.step` every *interval* milliseconds. If omitted,
      *interval* defaults to 50 milliseconds.


   .. method:: step(amount=None)

      Increments the progress bar's value by *amount*.

      *amount* defaults to 1.0 if omitted.


   .. method:: stop()

      Stop autoincrement mode: cancels any recurring timer event initiated by
      :meth:`Progressbar.start` for this progress bar.


Separator
---------

The :class:`ttk.Separator <Separator>` widget displays a horizontal or vertical
separator bar.

It has no other methods besides the ones inherited from
:class:`ttk.Widget <Widget>`.


Options
^^^^^^^

This widget accepts the following specific option:

.. tabularcolumns:: |l|L|

+--------+----------------------------------------------------------------+
| Option | Description                                                    |
+========+================================================================+
| orient | One of "horizontal" or "vertical". Specifies the orientation of|
|        | the separator.                                                 |
+--------+----------------------------------------------------------------+


Sizegrip
--------

The :class:`ttk.Sizegrip <Sizegrip>` widget (also known as a grow box) allows
the user to resize the containing toplevel window by pressing and dragging the
grip.

This widget has neither specific options nor specific methods, besides the
ones inherited from :class:`ttk.Widget <Widget>`.


Platform-specific notes
^^^^^^^^^^^^^^^^^^^^^^^

* On macOS, toplevel windows automatically include a built-in size grip
  by default. Adding a :class:`Sizegrip` is harmless, since the built-in
  grip will just mask the widget.


Bugs
^^^^

* If the containing toplevel's position was specified relative to the right
  or bottom of the screen (for example, ....), the :class:`Sizegrip` widget
  will not resize the window.
* This widget supports only "southeast" resizing.


Treeview
--------

The :class:`ttk.Treeview <Treeview>` widget displays a hierarchical collection
of items.
Each item has a textual label, an optional image, and an optional list of data
values.
The data values are displayed in successive columns after the tree label.

The order in which data values are displayed may be controlled by setting
the widget option ``displaycolumns``. The tree widget can also display column
headings. Columns may be accessed by number or symbolic names listed in the
widget option columns. See `Column Identifiers`_.

Each item is identified by a unique name. The widget will generate item IDs
if they are not supplied by the caller. There is a distinguished root item,
named ``{}``. The root item itself is not displayed; its children appear at the
top level of the hierarchy.

Each item also has a list of tags, which can be used to associate event bindings
with individual items and control the appearance of the item.

The Treeview widget supports horizontal and vertical scrolling, according to
the options described in `Scrollable Widget Options`_ and the methods
:meth:`Treeview.xview` and :meth:`Treeview.yview`.


Options
^^^^^^^

This widget accepts the following specific options:

.. tabularcolumns:: |l|p{0.7\linewidth}|

+----------------+--------------------------------------------------------+
| Option         | Description                                            |
+================+========================================================+
| columns        | A list of column identifiers, specifying the number of |
|                | columns and their names.                               |
+----------------+--------------------------------------------------------+
| displaycolumns | A list of column identifiers (either symbolic or       |
|                | integer indices) specifying which data columns are     |
|                | displayed and the order in which they appear, or the   |
|                | string "#all".                                         |
+----------------+--------------------------------------------------------+
| height         | Specifies the number of rows which should be visible.  |
|                | Note: the requested width is determined from the sum   |
|                | of the column widths.                                  |
+----------------+--------------------------------------------------------+
| padding        | Specifies the internal padding for the widget. The     |
|                | padding is a list of up to four length specifications. |
+----------------+--------------------------------------------------------+
| selectmode     | Controls how the built-in class bindings manage the    |
|                | selection. One of "extended", "browse" or "none".      |
|                | If set to "extended" (the default), multiple items may |
|                | be selected. If "browse", only a single item will be   |
|                | selected at a time. If "none", the selection will not  |
|                | be changed.                                            |
|                |                                                        |
|                | Note that the application code and tag bindings can set|
|                | the selection however they wish, regardless of the     |
|                | value  of this option.                                 |
+----------------+--------------------------------------------------------+
| show           | A list containing zero or more of the following values,|
|                | specifying which elements of the tree to display.      |
|                |                                                        |
|                | * tree: display tree labels in column #0.              |
|                | * headings: display the heading row.                   |
|                |                                                        |
|                | The default is "tree headings", that is, show all      |
|                | elements.                                              |
|                |                                                        |
|                | **Note**: Column #0 always refers to the tree column,  |
|                | even if show="tree" is not specified.                  |
+----------------+--------------------------------------------------------+

.. note::

   Tk 9.0 added several :class:`Treeview` features.
   The *selectmode* option gained the values ``"single"`` and ``"multiple"``;
   the new widget options *selecttype* (``"item"`` or ``"cell"`` selection),
   *striped* (zebra-striped rows), and *titlecolumns* / *titleitems* (columns
   or rows frozen against scrolling) were introduced; the column *separator*
   option was added; and items gained a *hidden* option.
   Tk 9.1 added the *rowheight* and *headingheight* options.


Item options
^^^^^^^^^^^^

The following item options may be specified for items in the insert and item
widget commands.

.. tabularcolumns:: |l|L|

+--------+---------------------------------------------------------------+
| Option | Description                                                   |
+========+===============================================================+
| text   | The textual label to display for the item.                    |
+--------+---------------------------------------------------------------+
| image  | A Tk Image, displayed to the left of the label.               |
+--------+---------------------------------------------------------------+
| values | The list of values associated with the item.                  |
|        |                                                               |
|        | Each item should have the same number of values as the widget |
|        | option columns. If there are fewer values than columns, the   |
|        | remaining values are assumed empty. If there are more values  |
|        | than columns, the extra values are ignored.                   |
+--------+---------------------------------------------------------------+
| open   | ``True``/``False`` value indicating whether the item's        |
|        | children should be displayed or hidden.                       |
+--------+---------------------------------------------------------------+
| tags   | A list of tags associated with this item.                     |
+--------+---------------------------------------------------------------+


Tag options
^^^^^^^^^^^

The following options may be specified on tags:

.. tabularcolumns:: |l|L|

+------------+-----------------------------------------------------------+
| Option     | Description                                               |
+============+===========================================================+
| foreground | Specifies the text foreground color.                      |
+------------+-----------------------------------------------------------+
| background | Specifies the cell or item background color.              |
+------------+-----------------------------------------------------------+
| font       | Specifies the font to use when drawing text.              |
+------------+-----------------------------------------------------------+
| image      | Specifies the item image, in case the item's image option |
|            | is empty.                                                 |
+------------+-----------------------------------------------------------+


Column identifiers
^^^^^^^^^^^^^^^^^^

Column identifiers take any of the following forms:

* A symbolic name from the list of columns option.
* An integer n, specifying the nth data column.
* A string of the form #n, where n is an integer, specifying the nth display
  column.

Notes:

* Item's option values may be displayed in a different order than the order
  in which they are stored.
* Column #0 always refers to the tree column, even if show="tree" is not
  specified.

A data column number is an index into an item's option values list; a display
column number is the column number in the tree where the values are displayed.
Tree labels are displayed in column #0. If option displaycolumns is not set,
then data column n is displayed in column #n+1. Again, **column #0 always
refers to the tree column**.


Virtual events
^^^^^^^^^^^^^^

The Treeview widget generates the following virtual events.

.. tabularcolumns:: |l|L|

+--------------------+--------------------------------------------------+
| Event              | Description                                      |
+====================+==================================================+
| <<TreeviewSelect>> | Generated whenever the selection changes.        |
+--------------------+--------------------------------------------------+
| <<TreeviewOpen>>   | Generated just before settings the focus item to |
|                    | open=True.                                       |
+--------------------+--------------------------------------------------+
| <<TreeviewClose>>  | Generated just after setting the focus item to   |
|                    | open=False.                                      |
+--------------------+--------------------------------------------------+

The :meth:`Treeview.focus` and :meth:`Treeview.selection` methods can be used
to determine the affected item or items.


ttk.Treeview
^^^^^^^^^^^^

.. class:: Treeview

   .. method:: bbox(item, column=None)

      Returns the bounding box (relative to the treeview widget's window) of
      the specified *item* in the form (x, y, width, height).

      If *column* is specified, returns the bounding box of that cell. If the
      *item* is not visible (that is, if it is a descendant of a closed item
      or is scrolled offscreen), returns an empty string.

      This shadows the inherited :meth:`!Misc.bbox`;
      use :meth:`~tkinter.Misc.grid_bbox` for the grid bounding box.


   .. method:: get_children(item=None)

      Returns a tuple of children belonging to *item*.

      If *item* is not specified, returns root children.


   .. method:: set_children(item, *newchildren)

      Replaces *item*'s children with *newchildren*.

      Children present in *item* that are not present in *newchildren* are
      detached from the tree. No items in *newchildren* may be an ancestor of
      *item*. Note that not specifying *newchildren* results in detaching
      *item*'s children.


   .. method:: column(column, option=None, **kw)

      Query or modify the options for the specified *column*.

      If *kw* is not given, returns a dict of the column option values. If
      *option* is specified then the value for that *option* is returned.
      Otherwise, sets the options to the corresponding values.

      The valid options/values are:

      *id*
         Returns the column name. This is a read-only option.
      *anchor*: One of the standard Tk anchor values.
         Specifies how the text in this column should be aligned with respect
         to the cell.
      *minwidth*: width
         The minimum width of the column in pixels. The treeview widget will
         not make the column any smaller than specified by this option when
         the widget is resized or the user drags a column.
      *separator*: ``True``/``False``
         Specifies whether a column separator should be drawn to the right of
         the column.
      *stretch*: ``True``/``False``
         Specifies whether the column's width should be adjusted when
         the widget is resized.
      *width*: width
         The width of the column in pixels.

      To configure the tree column, call this with column = "#0"

   .. method:: delete(*items)

      Delete all specified *items* and all their descendants.

      The root item may not be deleted.


   .. method:: detach(*items)

      Unlinks all of the specified *items* from the tree.

      The items and all of their descendants are still present, and may be
      reinserted at another point in the tree, but will not be displayed.

      The root item may not be detached.


   .. method:: exists(item)

      Returns ``True`` if the specified *item* is present in the tree,
      ``False`` otherwise.


   .. method:: focus(item=None)

      If *item* is specified, sets the focus item to *item*. Otherwise, returns
      the current focus item, or '' if there is none.

      This shadows the inherited :meth:`!Misc.focus`;
      use :meth:`~tkinter.Misc.focus_set` to focus the widget itself.


   .. method:: heading(column, option=None, **kw)

      Query or modify the heading options for the specified *column*.

      If *kw* is not given, returns a dict of the heading option values. If
      *option* is specified then the value for that *option* is returned.
      Otherwise, sets the options to the corresponding values.

      The valid options/values are:

      *text*: text
         The text to display in the column heading.
      *image*: imageName
         Specifies an image to display to the right of the column heading.
      *anchor*: anchor
         Specifies how the heading text should be aligned. One of the standard
         Tk anchor values.
      *command*: callback
         A callback to be invoked when the heading label is pressed.

      To configure the tree column heading, call this with column = "#0".


   .. method:: identify(component, x, y)

      Returns a description of the specified *component* under the point given
      by *x* and *y*, or the empty string if no such *component* is present at
      that position.


   .. method:: identify_row(y)

      Returns the item ID of the item at position *y*.


   .. method:: identify_column(x)

      Returns the display column identifier of the cell at position *x*.

      The tree column has ID #0.


   .. method:: identify_region(x, y)

      Returns one of:

      +-----------+--------------------------------------+
      | region    | meaning                              |
      +===========+======================================+
      | heading   | Tree heading area.                   |
      +-----------+--------------------------------------+
      | separator | Space between two columns headings.  |
      +-----------+--------------------------------------+
      | tree      | The tree area.                       |
      +-----------+--------------------------------------+
      | cell      | A data cell.                         |
      +-----------+--------------------------------------+

      Availability: Tk 8.6.


   .. method:: identify_element(x, y)

      Returns the element at position *x*, *y*.

      Availability: Tk 8.6.


   .. method:: index(item)

      Returns the integer index of *item* within its parent's list of children.


   .. method:: insert(parent, index, iid=None, **kw)

      Creates a new item and returns the item identifier of the newly created
      item.

      *parent* is the item ID of the parent item, or the empty string to create
      a new top-level item. *index* is an integer, or the value "end",
      specifying where in the list of parent's children to insert the new item.
      If *index* is less than or equal to zero, the new node is inserted at
      the beginning; if *index* is greater than or equal to the current number
      of children, it is inserted at the end. If *iid* is specified, it is used
      as the item identifier; *iid* must not already exist in the tree.
      Otherwise, a new unique identifier is generated.

      See `Item Options`_ for the list of available options.


   .. method:: item(item, option=None, **kw)

      Query or modify the options for the specified *item*.

      If no options are given, a dict with options/values for the item is
      returned.
      If *option* is specified then the value for that option is returned.
      Otherwise, sets the options to the corresponding values as given by *kw*.


   .. method:: reattach(item, parent, index)
      :no-typesetting:

   .. method:: move(item, parent, index)

      Moves *item* to position *index* in *parent*'s list of children.

      It is illegal to move an item under one of its descendants. If *index* is
      less than or equal to zero, *item* is moved to the beginning; if greater
      than or equal to the number of children, it is moved to the end. If *item*
      was detached it is reattached.

      :meth:`reattach` is an alias of :meth:`!move`.


   .. method:: next(item)

      Returns the identifier of *item*'s next sibling,
      or '' if *item* is the last child of its parent.
      Equivalent to ``after_item(item, hidden=True, recurse=False)``.


   .. method:: parent(item)

      Returns the ID of the parent of *item*, or '' if *item* is at the top
      level of the hierarchy.


   .. method:: prev(item)

      Returns the identifier of *item*'s previous sibling,
      or '' if *item* is the first child of its parent.
      Equivalent to ``before_item(item, hidden=True, recurse=False)``.


   .. method:: after_item(item, *, hidden=False, recurse=True)

      Returns the identifier of the item after *item*
      (the first child, a next sibling, or a next sibling of an ancestor),
      or '' if there is none.
      By default only visible items are considered;
      if *hidden* is true, hidden items are included too.
      If *recurse* is false, only siblings of *item* are considered
      (see :meth:`next`).

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: before_item(item, *, hidden=False, recurse=True)

      Returns the identifier of the item before *item*
      (a previous sibling or the parent of *item*),
      or '' if there is none.
      By default only visible items are considered;
      if *hidden* is true, hidden items are included too.
      If *recurse* is false, only siblings of *item* are considered
      (see :meth:`prev`).

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: depth(item)

      Returns the number of levels between *item* and the root item.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: haschildren(item)

      Returns ``True`` if *item* has children, ``False`` otherwise.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: visible(item)

      Returns ``True`` if *item* is visible, ``False`` otherwise.
      An item is visible if it is not detached, not hidden,
      and all of its ancestors are open and not hidden.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: size(item, *, hidden=False, recurse=False)

      Returns the number of children of *item*.
      If *hidden* is true, hidden items are included.
      If *recurse* is true, all descendants of *item* are included.
      Use ``''`` for the root item.
      ``size(item, hidden=True)`` equals ``len(get_children(item))``
      (which always includes hidden items).

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: range(first, last, *, hidden=False, recurse=True)

      Returns a tuple of items from *first* through *last*, inclusive.
      If *hidden* is true, hidden items are included.
      If *recurse* is false, descendants and ancestors are excluded.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: identifier(item, index)

      Returns the identifier of the item at *index*
      within *item*'s list of children.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: current()

      Returns the current item id and column id as a 2-tuple,
      or an empty tuple if there is none.
      The current item is the item under the mouse pointer.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: expand(*items, recurse=False)

      Set all of the specified items to the open state.
      If *recurse* is true, also open all of their descendants;
      this requires Tk 9.1.
      Use ``''`` for the root item.
      ``expand(item)`` is equivalent to ``item(item, open=True)``.

      .. versionadded:: next


   .. method:: collapse(*items, recurse=False)

      Set all of the specified items to the closed state.
      If *recurse* is true, also close all of their descendants;
      this requires Tk 9.1.
      Use ``''`` for the root item.
      ``collapse(item)`` is equivalent to ``item(item, open=False)``.

      .. versionadded:: next


   .. method:: hide(*items, recurse=False)

      Hide all of the specified items and all of their child items.
      If *recurse* is true, also hide all of their descendants.
      Use ``''`` for the root item.
      ``hide(item)`` is equivalent to ``item(item, hidden=True)``.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: unhide(*items, recurse=False)

      Unhide all of the specified items.
      If *recurse* is true, also unhide all of their descendants.
      Use ``''`` for the root item.
      ``unhide(item)`` is equivalent to ``item(item, hidden=False)``.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: detached(item=None)

      Returns information about detached items
      (see :meth:`detach`).
      Without arguments, returns a tuple of all detached items,
      but not their descendants (see :meth:`detached_all`).
      With *item*, returns whether *item* is detached; since Tk 9.1, also
      returns ``True`` if an ancestor of *item* is detached.

      Requires Tk 9.0 or newer.

      .. versionadded:: next


   .. method:: detached_all()

      Returns a tuple of all detached items and all of their descendants
      (see :meth:`detach`).

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: cellfocus(cell=None)

      Get or set the focus cell.
      Without *cell*, returns the focus cell as an ``(item, column)`` 2-tuple,
      or an empty tuple if there is none.
      With *cell*, sets the focus cell; use ``''`` to clear it.
      A cell is specified as an ``(item, column)`` pair.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: sort(parent, *, column=None, command=None, dictionary=False, integer=False, real=False, nocase=False, decreasing=False, ignoreempty=False, recurse=False)

      Sort the children of *parent*.
      By default the children are sorted by the value of the first display
      column, as Unicode strings, in increasing order.
      *column* selects the column to sort on.
      *dictionary*, *integer* and *real* select the comparison type;
      *nocase* makes string comparison case-insensitive.
      *command* is a function of two values
      returning a negative, zero or positive number.
      *decreasing* reverses the order.
      *ignoreempty* skips empty values (with *integer* or *real*).
      *recurse* also sorts all descendants.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: search(parent, pattern, *, columns=None, start=None, stop=None, dictionary=False, integer=False, real=False, nocase=False, glob=False, regexp=False, backwards=False, hidden=False, recurse=False, wraparound=False)

      Search *parent*'s children for *pattern*
      and return the identifier of the first matching item,
      or ``''`` if there is no match.
      By default *pattern* is matched for exact equality
      against the value of each displayed column, as Unicode strings,
      searching forwards through the direct children of *parent*.
      *glob* or *regexp* select glob-style or regular expression matching;
      *dictionary*, *integer* and *real* select the comparison type;
      *nocase* makes it case-insensitive.
      *columns* limits the search to the given columns.
      *start* and *stop* bound the search;
      *backwards* reverses its direction;
      *wraparound* continues from the other end.
      *hidden* also searches hidden and closed items;
      *recurse* searches all descendants.

      See :meth:`search_all`, :meth:`search_cell` and :meth:`search_all_cells`.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: search_all(parent, pattern, **kwargs)

      Like :meth:`search`,
      but returns a tuple of the identifiers of all matching items.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: search_cell(parent, pattern, **kwargs)

      Like :meth:`search`,
      but returns the first matching cell as an ``(item, column)`` 2-tuple,
      or an empty tuple if there is no match.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: search_all_cells(parent, pattern, **kwargs)

      Like :meth:`search`,
      but returns a tuple of all matching cells,
      each an ``(item, column)`` 2-tuple.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: cellselection()

      Returns a tuple of the selected cells, each an ``(item, column)``
      2-tuple.
      The cell selection is independent from the item selection
      (see :meth:`selection`).

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: cellselection_set(*cells)

      The specified cells become the new cell selection.
      Each cell is an ``(item, column)`` pair.
      Call without arguments to clear the cell selection.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: cellselection_add(*cells)

      Add the specified cells to the cell selection.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: cellselection_remove(*cells)

      Remove the specified cells from the cell selection.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: cellselection_set_range(first, last, *, hidden=True, recurse=True)

      Set the cell selection to the rectangle of cells from *first* to *last*.
      *first* and *last* are the opposite corner cells,
      each an ``(item, column)`` pair, and must be in displayed columns.
      All other cells are unselected.
      If *hidden* is false, hidden cells are excluded;
      if *recurse* is false, cells in descendant items are excluded.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: cellselection_add_range(first, last, *, hidden=True, recurse=True)

      Like :meth:`cellselection_set_range`,
      but adds the rectangle of cells to the cell selection
      instead of replacing it.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: cellselection_remove_range(first, last, *, hidden=True, recurse=True)

      Like :meth:`cellselection_set_range`,
      but removes the rectangle of cells from the cell selection.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: cellselection_anchor(cell=None)

      Get or set the cell selection anchor.
      Without *cell*, returns the anchor as an ``(item, column)`` 2-tuple,
      or an empty tuple if it is unset.
      With *cell*, sets the anchor; use ``''`` to unset it.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: cellselection_includes(*cells)

      Returns whether all of the specified cells are selected.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: cellselection_present()

      Returns whether any cell is selected.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: tag_cell_add(tagname, *cells)

      Add the given tag to each of the specified cells.
      Each cell is an ``(item, column)`` pair.
      Cell tags are independent from item tags.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: tag_cell_remove(tagname, *cells)

      Remove the given tag from each of the specified cells.
      If no cell is specified, the tag is removed from all cells.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: tag_cell_has(tagname, cell=None)

      Test for a cell tag, or list the cells that have it.
      If *cell* is specified, returns whether that cell has the given tag.
      Otherwise returns a tuple of all cells (as ``(item, column)`` 2-tuples)
      that have the tag.

      Requires Tk 9.1 or newer.

      .. versionadded:: next


   .. method:: see(item)

      Ensure that *item* is visible.

      Sets all of *item*'s ancestors open option to ``True``, and scrolls the
      widget if necessary so that *item* is within the visible portion of
      the tree.


   .. method:: selection()

      Returns a tuple of selected items.

      .. versionchanged:: 3.8
         ``selection()`` no longer takes arguments.  For changing the selection
         state use the following selection methods.


   .. method:: selection_set(*items)

      *items* becomes the new selection.

      .. versionchanged:: 3.6
         *items* can be passed as separate arguments, not just as a single tuple.


   .. method:: selection_add(*items)

      Add *items* to the selection.

      .. versionchanged:: 3.6
         *items* can be passed as separate arguments, not just as a single tuple.


   .. method:: selection_remove(*items)

      Remove *items* from the selection.

      .. versionchanged:: 3.6
         *items* can be passed as separate arguments, not just as a single tuple.


   .. method:: selection_toggle(*items)

      Toggle the selection state of each item in *items*.

      .. versionchanged:: 3.6
         *items* can be passed as separate arguments, not just as a single tuple.


   .. method:: set(item, column=None, value=None)

      With one argument, returns a dictionary of column/value pairs for the
      specified *item*. With two arguments, returns the current value of the
      specified *column*. With three arguments, sets the value of given
      *column* in given *item* to the specified *value*.


   .. method:: tag_bind(tagname, sequence=None, callback=None)

      Bind a callback for the given event *sequence* to the tag *tagname*.
      When an event is delivered to an item, the callbacks for each of the
      item's tags option are called.


   .. method:: tag_configure(tagname, option=None, **kw)

      Query or modify the options for the specified *tagname*.

      If *kw* is not given, returns a dict of the option settings for
      *tagname*. If *option* is specified, returns the value for that *option*
      for the specified *tagname*. Otherwise, sets the options to the
      corresponding values for the given *tagname*.


   .. method:: tag_has(tagname, item=None)

      If *item* is specified, returns ``True`` if the specified *item* has the
      given *tagname* and ``False`` otherwise.
      Otherwise, returns a tuple of all items that have the specified tag.

      Availability: Tk 8.6


   .. method:: xview(*args)

      Query or modify horizontal position of the treeview.


   .. method:: yview(*args)

      Query or modify vertical position of the treeview.


.. _TtkStyling:

Ttk styling
-----------

Each widget in :mod:`!ttk` is assigned a style, which specifies the set of
elements making up the widget and how they are arranged, along with dynamic and
default settings for element options.
By default the style name is the same as the widget's class name, but it may be
overridden by the widget's style option.
If you don't know the class name of a widget, use the method
:meth:`Misc.winfo_class <tkinter.Misc.winfo_class>` (somewidget.winfo_class()).

.. seealso::

   `Introduction to the Tk theme engine <https://www.tcl-lang.org/man/tcl9.0/TkCmd/ttk_intro.html>`_
      The ``ttk::intro`` man page explains how the theme engine works.

   `The Tile Widget Set <https://tktable.sourceforge.net/tile/tile-tcl2004.pdf>`_
      Joe English's 2004 paper introducing the theme engine
      (then the separate *Tile* extension),
      with diagrams of how elements and layouts make up a widget's appearance.


.. class:: Style

   This class is used to manipulate the style database.


   .. method:: configure(style, query_opt=None, **kw)

      Query or set the default value of the specified option(s) in *style*.

      Each key in *kw* is an option and each value is a string identifying
      the value for that option.

      For example, to change every default button to be a flat button with
      some padding and a different background color::

         from tkinter import ttk
         import tkinter

         root = tkinter.Tk()

         ttk.Style().configure("TButton", padding=6, relief="flat",
            background="#ccc")

         btn = ttk.Button(text="Sample")
         btn.pack()

         root.mainloop()


   .. method:: map(style, query_opt=None, **kw)

      Query or sets dynamic values of the specified option(s) in *style*.

      Each key in *kw* is an option and each value should be a list or a
      tuple (usually) containing statespecs grouped in tuples, lists, or
      some other preference. A statespec is a compound of one
      or more states and then a value.

      An example may make it more understandable::

         import tkinter
         from tkinter import ttk

         root = tkinter.Tk()

         style = ttk.Style()
         style.map("C.TButton",
             foreground=[('pressed', 'red'), ('active', 'blue')],
             background=[('pressed', '!disabled', 'black'), ('active', 'white')]
             )

         colored_btn = ttk.Button(text="Test", style="C.TButton").pack()

         root.mainloop()


      Note that the order of the (states, value) sequences for an option does
      matter, if the order is changed to ``[('active', 'blue'), ('pressed',
      'red')]`` in the foreground option, for example, the result would be a
      blue foreground when the widget were in active or pressed states.

      When called to query the map (without specifying values to set), it
      returns a dictionary mapping each option to its list of statespecs.

      .. versionchanged:: 3.10
         The value returned when querying the map was corrected.


   .. method:: lookup(style, option, state=None, default=None)

      Returns the value specified for *option* in *style*.

      If *state* is specified, it is expected to be a sequence of one or more
      states. If the *default* argument is set, it is used as a fallback value
      in case no specification for option is found.

      To check what font a Button uses by default::

         from tkinter import ttk

         print(ttk.Style().lookup("TButton", "font"))


   .. method:: layout(style, layoutspec=None)

      Define the widget layout for given *style*. If *layoutspec* is omitted,
      return the layout specification for given style.

      *layoutspec*, if specified, is expected to be a list or some other
      sequence type (excluding strings), where each item should be a tuple and
      the first item is the layout name and the second item should have the
      format described in `Layouts`_.

      To understand the format, see the following example (it is not
      intended to do anything useful)::

         from tkinter import ttk
         import tkinter

         root = tkinter.Tk()

         style = ttk.Style()
         style.layout("TMenubutton", [
            ("Menubutton.background", None),
            ("Menubutton.button", {"children":
                [("Menubutton.focus", {"children":
                    [("Menubutton.padding", {"children":
                        [("Menubutton.label", {"side": "left", "expand": 1})]
                    })]
                })]
            }),
         ])

         mbtn = ttk.Menubutton(text='Text')
         mbtn.pack()
         root.mainloop()


   .. method:: element_create(elementname, etype, *args, **kw)

      Create a new element in the current theme, of the given *etype* which is
      expected to be either "image", "from" or "vsapi".
      The latter is only available in Tk 8.6 on Windows.

      If "image" is used, *args* should contain the default image name followed
      by statespec/value pairs (this is the imagespec), and *kw* may have the
      following options:

      border=padding
         padding is a list of up to four integers, specifying the left, top,
         right, and bottom borders, respectively.

      height=height
         Specifies a minimum height for the element. If less than zero, the
         base image's height is used as a default.

      padding=padding
         Specifies the element's interior padding. Defaults to border's value
         if not specified.

      sticky=spec
         Specifies how the image is placed within the final parcel. spec
         contains zero or more characters "n", "s", "w", or "e".

      width=width
         Specifies a minimum width for the element. If less than zero, the
         base image's width is used as a default.

      Example::

         img1 = tkinter.PhotoImage(master=root, file='button.png')
         img1 = tkinter.PhotoImage(master=root, file='button-pressed.png')
         img1 = tkinter.PhotoImage(master=root, file='button-active.png')
         style = ttk.Style(root)
         style.element_create('Button.button', 'image',
                              img1, ('pressed', img2), ('active', img3),
                              border=(2, 4), sticky='we')

      If "from" is used as the value of *etype*,
      :meth:`element_create` will clone an existing
      element. *args* is expected to contain a themename, from which
      the element will be cloned, and optionally an element to clone from.
      If this element to clone from is not specified, an empty element will
      be used. *kw* is discarded.

      Example::

         style = ttk.Style(root)
         style.element_create('plain.background', 'from', 'default')

      If "vsapi" is used as the value of *etype*, :meth:`element_create`
      will create a new element in the current theme whose visual appearance
      is drawn using the Microsoft Visual Styles API which is responsible
      for the themed styles on Windows XP and Vista.
      *args* is expected to contain the Visual Styles class and part as
      given in the Microsoft documentation followed by an optional sequence
      of tuples of ttk states and the corresponding Visual Styles API state
      value.
      *kw* may have the following options:

      padding=padding
         Specify the element's interior padding.
         *padding* is a list of up to four integers specifying the left,
         top, right and bottom padding quantities respectively.
         If fewer than four elements are specified, bottom defaults to top,
         right defaults to left, and top defaults to left.
         In other words, a list of three numbers specify the left, vertical,
         and right padding; a list of two numbers specify the horizontal
         and the vertical padding; a single number specifies the same
         padding all the way around the widget.
         This option may not be mixed with any other options.

      margins=padding
         Specifies the elements exterior padding.
         *padding* is a list of up to four integers specifying the left, top,
         right and bottom padding quantities respectively.
         This option may not be mixed with any other options.

      width=width
         Specifies the width for the element.
         If this option is set then the Visual Styles API will not be queried
         for the recommended size or the part.
         If this option is set then *height* should also be set.
         The *width* and *height* options cannot be mixed with the *padding*
         or *margins* options.

      height=height
         Specifies the height of the element.
         See the comments for *width*.

      Example::

         style = ttk.Style(root)
         style.element_create('pin', 'vsapi', 'EXPLORERBAR', 3, [
                              ('pressed', '!selected', 3),
                              ('active', '!selected', 2),
                              ('pressed', 'selected', 6),
                              ('active', 'selected', 5),
                              ('selected', 4),
                              ('', 1)])
         style.layout('Explorer.Pin',
                      [('Explorer.Pin.pin', {'sticky': 'news'})])
         pin = ttk.Checkbutton(style='Explorer.Pin')
         pin.pack(expand=True, fill='both')

      .. versionchanged:: 3.13
         Added support of the "vsapi" element factory.

   .. method:: element_names()

      Returns a tuple of elements defined in the current theme.


   .. method:: element_options(elementname)

      Returns a tuple of *elementname*'s options.


   .. method:: theme_create(themename, parent=None, settings=None)

      Create a new theme.

      It is an error if *themename* already exists. If *parent* is specified,
      the new theme will inherit styles, elements and layouts from the parent
      theme. If *settings* are present they are expected to have the same
      syntax used for :meth:`theme_settings`.


   .. method:: theme_settings(themename, settings)

      Temporarily sets the current theme to *themename*, apply specified
      *settings* and then restore the previous theme.

      Each key in *settings* is a style and each value may contain the keys
      'configure', 'map', 'layout' and 'element create' and they are expected
      to have the same format as specified by the methods
      :meth:`Style.configure`, :meth:`Style.map`, :meth:`Style.layout` and
      :meth:`Style.element_create` respectively.

      As an example, let's change the Combobox for the default theme a bit::

         from tkinter import ttk
         import tkinter

         root = tkinter.Tk()

         style = ttk.Style()
         style.theme_settings("default", {
            "TCombobox": {
                "configure": {"padding": 5},
                "map": {
                    "background": [("active", "green2"),
                                   ("!disabled", "green4")],
                    "fieldbackground": [("!disabled", "green3")],
                    "foreground": [("focus", "OliveDrab1"),
                                   ("!disabled", "OliveDrab2")]
                }
            }
         })

         combo = ttk.Combobox().pack()

         root.mainloop()


   .. method:: theme_names()

      Returns a tuple of all known themes.


   .. method:: theme_styles(themename=None)

      Returns a tuple of all styles in *themename*.
      If *themename* is not given, the current theme is used.

      .. versionadded:: next

      Availability: Tk 9.0.


   .. method:: theme_use(themename=None)

      If *themename* is not given, returns the theme in use.  Otherwise, sets
      the current theme to *themename*, refreshes all widgets and emits a
      <<ThemeChanged>> event.


Layouts
^^^^^^^

A layout can be just ``None``, if it takes no options, or a dict of
options specifying how to arrange the element. The layout mechanism
uses a simplified version of the pack geometry manager: given an
initial cavity, each element is allocated a parcel.

The valid options/values are:

*side*: whichside
   Specifies which side of the cavity to place the element; one of
   top, right, bottom or left. If omitted, the element occupies the
   entire cavity.

*sticky*: nswe
   Specifies where the element is placed inside its allocated parcel.

*unit*: 0 or 1
   If set to 1, causes the element and all of its descendants to be treated as
   a single element for the purposes of :meth:`Widget.identify` et al. It's
   used for things like scrollbar thumbs with grips.

*children*: [sublayout... ]
   Specifies a list of elements to place inside the element. Each
   element is a tuple (or other sequence type) where the first item is
   the layout name, and the other is a `Layout`_.

.. _Layout: `Layouts`_


Additional widgets
------------------

The following themed widgets complete the :mod:`tkinter.ttk` widget set.
Each is the themed counterpart of the like-named classic :mod:`tkinter` widget
and inherits the common methods of :class:`Widget`.

.. class:: Button(master=None, **kw)

   Ttk :class:`Button` widget, displays a textual label and/or image, and
   evaluates a command when pressed.
   It is the themed counterpart of :class:`tkinter.Button` and inherits the
   common widget methods from :class:`Widget`.

   .. method:: invoke()

      Invoke the command associated with the button and return its result.


.. class:: Checkbutton(master=None, **kw)

   Ttk :class:`Checkbutton` widget, used to control a boolean variable that is
   toggled on and off.
   It is the themed counterpart of :class:`tkinter.Checkbutton` and inherits
   the common widget methods from :class:`Widget`.

   .. method:: invoke()

      Toggle the button between its selected and deselected states, invoke the
      command associated with the button, and return its result.


.. class:: Entry(master=None, widget=None, **kw)

   Ttk :class:`Entry` widget, displays a one-line text string and allows the
   user to edit it.
   It is the themed counterpart of :class:`tkinter.Entry` and inherits the
   common widget methods from :class:`Widget` as well as the editing methods
   from :class:`tkinter.Entry`.

   .. method:: bbox(index)

      Return a tuple ``(x, y, width, height)`` giving the bounding box of the
      character at the given *index*.

      This shadows the inherited :meth:`!Misc.bbox`;
      use :meth:`~tkinter.Misc.grid_bbox` for the grid bounding box.

   .. method:: identify(x, y)

      Return the name of the element under the point given by *x* and *y*, or
      the empty string if no element is present at that location.

   .. method:: validate()

      Force validation of the entry and return ``True`` if validation
      succeeded, and ``False`` otherwise.


.. class:: Frame(master=None, **kw)

   Ttk :class:`Frame` widget, a container used to group and lay out other
   widgets.
   It is the themed counterpart of :class:`tkinter.Frame` and inherits the
   common widget methods from :class:`Widget`.


.. class:: Label(master=None, **kw)

   Ttk :class:`Label` widget, displays a textual label and/or image.
   It is the themed counterpart of :class:`tkinter.Label` and inherits the
   common widget methods from :class:`Widget`.


.. class:: Labelframe(master=None, **kw)

   Ttk :class:`Labelframe` widget, a container that draws a border and a title
   label around its contents.
   It is the themed counterpart of :class:`tkinter.LabelFrame` and inherits the
   common widget methods from :class:`Widget`.


.. class:: Menubutton(master=None, **kw)

   Ttk :class:`Menubutton` widget, displays a textual label and/or image, and
   pops up a menu when pressed.
   It is the themed counterpart of :class:`tkinter.Menubutton` and inherits the
   common widget methods from :class:`Widget`.


.. class:: OptionMenu(master, variable, default=None, *values, **kwargs)

   Ttk :class:`OptionMenu` widget, a :class:`Menubutton` that pops up a menu of
   mutually exclusive choices.
   *variable* is the variable that tracks the currently selected value,
   *default* is the value to set initially, and *values* are the entries to
   display in the menu.
   A *command* keyword argument may be given to specify a callable that is
   invoked with the selected value whenever the selection changes; the *style*
   keyword argument sets the style used by the underlying menubutton; the
   *direction* keyword argument sets where the menu is posted relative to the
   menubutton (one of ``'above'``, ``'below'`` (the default), ``'left'``,
   ``'right'`` or ``'flush'``); and the *name* keyword argument sets the Tk
   widget name.

   .. method:: set_menu(default=None, *values)

      Replace the entries of the menu with *values*.
      If *default* is given, also set it as the current value of the
      *variable*.

   .. method:: destroy()

      Destroy this widget and its associated menu.

   .. versionchanged:: 3.14
      Added support for the *name* keyword argument.



.. class:: Panedwindow(master=None, **kw)

   Ttk :class:`Panedwindow` widget, displays a number of subwindows stacked
   either vertically or horizontally.
   The user may adjust the relative sizes of the subwindows by dragging the
   sash between panes.
   It is the themed counterpart of :class:`tkinter.PanedWindow` and inherits
   the common widget methods from :class:`Widget`, as well as the :meth:`!add`
   and :meth:`!panes` methods from :class:`tkinter.PanedWindow`.

   .. method:: insert(pos, child, **kw)

      Insert a pane containing *child* at the position *pos*.
      *pos* is either the string ``'end'``, an integer index, or the name of a
      managed subwindow.
      If *child* is already managed by the paned window, move it to the
      specified position.
      Any keyword arguments set pane options.

   .. method:: forget(child)

      Remove *child*, which may be either an integer index or the name of a
      managed subwindow, from the panes.

      This shadows the inherited geometry-manager :meth:`!forget`;
      use :meth:`~tkinter.Pack.pack_forget`, :meth:`~tkinter.Grid.grid_forget`
      or :meth:`~tkinter.Place.place_forget` to remove the widget itself from
      its manager.

   .. method:: pane(pane, option=None, **kw)

      Query or modify the options of the specified *pane*, where *pane* is
      either an integer index or the name of a managed subwindow.
      If no arguments are given, return a dictionary of the pane option values.
      If *option* is specified, return the value of that option.
      Otherwise, set the options given as keyword arguments to their
      corresponding values.

   .. method:: sashpos(index, newpos=None)

      If *newpos* is specified, set the position of sash number *index* and
      return its new position.
      This may adjust the positions of adjacent sashes to ensure that positions
      are monotonically increasing; positions are also constrained to be
      between 0 and the total size of the widget.
      If *newpos* is omitted, return the current position of the sash.


.. class:: Radiobutton(master=None, **kw)

   Ttk :class:`Radiobutton` widget, used as part of a group to control a single
   shared variable by selecting one of several mutually exclusive values.
   It is the themed counterpart of :class:`tkinter.Radiobutton` and inherits
   the common widget methods from :class:`Widget`.

   .. method:: invoke()

      Set the option variable to the button's value, select the button, invoke
      the command associated with the button, and return its result.


.. class:: Scale(master=None, **kw)

   Ttk :class:`Scale` widget, displays a slider that lets the user select a
   numeric value from a range by moving the slider along a trough.
   It is the themed counterpart of :class:`tkinter.Scale` and inherits the
   common widget methods from :class:`Widget`.

   .. method:: configure(cnf=None, **kw)

      Modify or query the widget options, like
      :meth:`Widget.configure <tkinter.Misc.configure>`.
      In addition, this method clips the ``from`` and ``to`` values so that the
      current value stays within the range defined by them.

      .. versionchanged:: 3.9
         Now returns the configuration value, like
         :meth:`Widget.configure <tkinter.Misc.configure>`.


   .. method:: get(x=None, y=None)

      Return the current value of the scale.
      If *x* and *y* are given, return the value corresponding to the pixel
      coordinate *x*, *y* instead.


.. class:: Scrollbar(master=None, **kw)

   Ttk :class:`Scrollbar` widget, controls the viewport of an associated
   scrollable widget such as a :class:`Treeview`, :class:`Entry` or
   :class:`tkinter.Text`.
   It is the themed counterpart of :class:`tkinter.Scrollbar` and inherits the
   common widget methods from :class:`Widget`, as well as the :meth:`!set` and
   :meth:`!get` methods from :class:`tkinter.Scrollbar`.


.. class:: Separator(master=None, **kw)

   Ttk :class:`Separator` widget, displays a horizontal or vertical separator
   line.
   It has no direct counterpart in :mod:`tkinter` and inherits the common
   widget methods from :class:`Widget`.


.. class:: Sizegrip(master=None, **kw)

   Ttk :class:`Sizegrip` widget, displays a grip that allows the user to resize
   the containing toplevel window by pressing and dragging the grip, typically
   placed in the bottom-right corner.
   It has no direct counterpart in :mod:`tkinter` and inherits the common
   widget methods from :class:`Widget`.


.. class:: LabeledScale(master=None, variable=None, from_=0, to=10, **kw)

   A :class:`Frame` containing a :class:`Scale` and a :class:`Label` that shows
   the scale's current value.
   *variable* is the :class:`~tkinter.IntVar` tracked by the scale (one is
   created if it is not given), and *from_* and *to* define the range of the
   scale.

   .. method:: destroy()

      Destroy this widget and remove the trace callback registered on the
      associated variable.


.. class:: LabelFrame(master=None, **kw)

   Alias of :class:`Labelframe`, kept for naming compatibility with
   :class:`tkinter.LabelFrame`.


.. class:: PanedWindow(master=None, **kw)

   Alias of :class:`Panedwindow`, kept for naming compatibility with
   :class:`tkinter.PanedWindow`.
