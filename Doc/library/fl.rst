
:mod:`fl` --- FORMS library for graphical user interfaces
=========================================================

.. module:: fl
   :platform: IRIX
   :synopsis: FORMS library for applications with graphical user interfaces.
   :deprecated:


.. deprecated:: 2.6
    The :mod:`fl` module has been removed in Python 3.


.. index::
   single: FORMS Library
   single: Overmars, Mark

This module provides an interface to the FORMS Library by Mark Overmars.  The
source for the library can be retrieved by anonymous FTP from host
``ftp.cs.ruu.nl``, directory :file:`SGI/FORMS`.  It was last tested with version
2.0b.

Most functions are literal translations of their C equivalents, dropping the
initial ``fl_`` from their name.  Constants used by the library are defined in
module :mod:`FL` described below.

The creation of objects is a little different in Python than in C: instead of
the 'current form' maintained by the library to which new FORMS objects are
added, all functions that add a FORMS object to a form are methods of the Python
object representing the form. Consequently, there are no Python equivalents for
the C functions :c:func:`fl_addto_form` and :c:func:`fl_end_form`, and the
equivalent of :c:func:`fl_bgn_form` is called :func:`fl.make_form`.

Watch out for the somewhat confusing terminology: FORMS uses the word
:dfn:`object` for the buttons, sliders etc. that you can place in a form. In
Python, 'object' means any value.  The Python interface to FORMS introduces two
new Python object types: form objects (representing an entire form) and FORMS
objects (representing one button, slider etc.). Hopefully this isn't too
confusing.

There are no 'free objects' in the Python interface to FORMS, nor is there an
easy way to add object classes written in Python.  The FORMS interface to GL
event handling is available, though, so you can mix FORMS with pure GL windows.

**Please note:** importing :mod:`fl` implies a call to the GL function
:c:func:`foreground` and to the FORMS routine :c:func:`fl_init`.


.. _fl-functions:

Functions Defined in Module :mod:`fl`
-------------------------------------

Module :mod:`fl` defines the following functions.  For more information about
what they do, see the description of the equivalent C function in the FORMS
documentation:


.. function:: make_form(type, width, height)

   Create a form with given type, width and height.  This returns a :dfn:`form`
   object, whose methods are described below.


.. function:: do_forms()

   The standard FORMS main loop.  Returns a Python object representing the FORMS
   object needing interaction, or the special value :const:`FL.EVENT`.


.. function:: check_forms()

   Check for FORMS events.  Returns what :func:`do_forms` above returns, or
   ``None`` if there is no event that immediately needs interaction.


.. function:: set_event_call_back(function)

   Set the event callback function.


.. function:: set_graphics_mode(rgbmode, doublebuffering)

   Set the graphics modes.


.. function:: get_rgbmode()

   Return the current rgb mode.  This is the value of the C global variable
   :c:data:`fl_rgbmode`.


.. function:: show_message(str1, str2, str3)

   Show a dialog box with a three-line message and an OK button.


.. function:: show_question(str1, str2, str3)

   Show a dialog box with a three-line message and YES and NO buttons. It returns
   ``1`` if the user pressed YES, ``0`` if NO.


.. function:: show_choice(str1, str2, str3, but1[, but2[, but3]])

   Show a dialog box with a three-line message and up to three buttons. It returns
   the number of the button clicked by the user (``1``, ``2`` or ``3``).


.. function:: show_input(prompt, default)

   Show a dialog box with a one-line prompt message and text field in which the
   user can enter a string.  The second argument is the default input string.  It
   returns the string value as edited by the user.


.. function:: show_file_selector(message, directory, pattern, default)

   Show a dialog box in which the user can select a file.  It returns the absolute
   filename selected by the user, or ``None`` if the user presses Cancel.


.. function:: get_directory()
              get_pattern()
              get_filename()

   These functions return the directory, pattern and filename (the tail part only)
   selected by the user in the last :func:`show_file_selector` call.


.. function:: qdevice(dev)
              unqdevice(dev)
              isqueued(dev)
              qtest()
              qread()
              qreset()
              qenter(dev, val)
              get_mouse()
              tie(button, valuator1, valuator2)

   These functions are the FORMS interfaces to the corresponding GL functions.  Use
   these if you want to handle some GL events yourself when using
   :func:`fl.do_events`.  When a GL event is detected that FORMS cannot handle,
   :func:`fl.do_forms` returns the special value :const:`FL.EVENT` and you should
   call :func:`fl.qread` to read the event from the queue.  Don't use the
   equivalent GL functions!

   .. \funcline{blkqread}{?}


.. function:: color()
              mapcolor()
              getmcolor()

   See the description in the FORMS documentation of :c:func:`fl_color`,
   :c:func:`fl_mapcolor` and :c:func:`fl_getmcolor`.


.. _form-objects:

Form Objects
------------

Form objects (returned by :func:`make_form` above) have the following methods.
Each method corresponds to a C function whose name is prefixed with ``fl_``; and
whose first argument is a form pointer; please refer to the official FORMS
documentation for descriptions.

All the :meth:`add_\*` methods return a Python object representing the FORMS
object.  Methods of FORMS objects are described below.  Most kinds of FORMS
object also have some methods specific to that kind; these methods are listed
here.


.. method:: form.show_form(placement, bordertype, name)

   Show the form.


.. method:: form.hide_form()

   Hide the form.


.. method:: form.redraw_form()

   Redraw the form.


.. method:: form.set_form_position(x, y)

   Set the form's position.


.. method:: form.freeze_form()

   Freeze the form.


.. method:: form.unfreeze_form()

   Unfreeze the form.


.. method:: form.activate_form()

   Activate the form.


.. method:: form.deactivate_form()

   Deactivate the form.


.. method:: form.bgn_group()

   Begin a new group of objects; return a group object.


.. method:: form.end_group()

   End the current group of objects.


.. method:: form.find_first()

   Find the first object in the form.


.. method:: form.find_last()

   Find the last object in the form.


.. method:: form.add_box(type, x, y, w, h, name)

   Add a box object to the form. No extra methods.


.. method:: form.add_text(type, x, y, w, h, name)

   Add a text object to the form. No extra methods.

.. \begin{methoddesc}[form]{add_bitmap}{type, x, y, w, h, name}
.. Add a bitmap object to the form.
.. \end{methoddesc}


.. method:: form.add_clock(type, x, y, w, h, name)

   Add a clock object to the form.  ---  Method: :meth:`get_clock`.


.. method:: form.add_button(type, x, y, w, h,  name)

   Add a button object to the form.  ---  Methods: :meth:`get_button`,
   :meth:`set_button`.


.. method:: form.add_lightbutton(type, x, y, w, h, name)

   Add a lightbutton object to the form.  ---  Methods: :meth:`get_button`,
   :meth:`set_button`.


.. method:: form.add_roundbutton(type, x, y, w, h, name)

   Add a roundbutton object to the form.  ---  Methods: :meth:`get_button`,
   :meth:`set_button`.


.. method:: form.add_slider(type, x, y, w, h, name)

   Add a slider object to the form.  ---  Methods: :meth:`set_slider_value`,
   :meth:`get_slider_value`, :meth:`set_slider_bounds`, :meth:`get_slider_bounds`,
   :meth:`set_slider_return`, :meth:`set_slider_size`,
   :meth:`set_slider_precision`, :meth:`set_slider_step`.


.. method:: form.add_valslider(type, x, y, w, h, name)

   Add a valslider object to the form.  ---  Methods: :meth:`set_slider_value`,
   :meth:`get_slider_value`, :meth:`set_slider_bounds`, :meth:`get_slider_bounds`,
   :meth:`set_slider_return`, :meth:`set_slider_size`,
   :meth:`set_slider_precision`, :meth:`set_slider_step`.


.. method:: form.add_dial(type, x, y, w, h, name)

   Add a dial object to the form.  ---  Methods: :meth:`set_dial_value`,
   :meth:`get_dial_value`, :meth:`set_dial_bounds`, :meth:`get_dial_bounds`.


.. method:: form.add_positioner(type, x, y, w, h, name)

   Add a positioner object to the form.  ---  Methods:
   :meth:`set_positioner_xvalue`, :meth:`set_positioner_yvalue`,
   :meth:`set_positioner_xbounds`, :meth:`set_positioner_ybounds`,
   :meth:`get_positioner_xvalue`, :meth:`get_positioner_yvalue`,
   :meth:`get_positioner_xbounds`, :meth:`get_positioner_ybounds`.


.. method:: form.add_counter(type, x, y, w, h, name)

   Add a counter object to the form.  ---  Methods: :meth:`set_counter_value`,
   :meth:`get_counter_value`, :meth:`set_counter_bounds`, :meth:`set_counter_step`,
   :meth:`set_counter_precision`, :meth:`set_counter_return`.


.. method:: form.add_input(type, x, y, w, h, name)

   Add an input object to the form.  ---  Methods: :meth:`set_input`,
   :meth:`get_input`, :meth:`set_input_color`, :meth:`set_input_return`.


.. method:: form.add_menu(type, x, y, w, h, name)

   Add a menu object to the form.  ---  Methods: :meth:`set_menu`,
   :meth:`get_menu`, :meth:`addto_menu`.


.. method:: form.add_choice(type, x, y, w, h, name)

   Add a choice object to the form.  ---  Methods: :meth:`set_choice`,
   :meth:`get_choice`, :meth:`clear_choice`, :meth:`addto_choice`,
   :meth:`replace_choice`, :meth:`delete_choice`, :meth:`get_choice_text`,
   :meth:`set_choice_fontsize`, :meth:`set_choice_fontstyle`.


.. method:: form.add_browser(type, x, y, w, h, name)

   Add a browser object to the form.  ---  Methods: :meth:`set_browser_topline`,
   :meth:`clear_browser`, :meth:`add_browser_line`, :meth:`addto_browser`,
   :meth:`insert_browser_line`, :meth:`delete_browser_line`,
   :meth:`replace_browser_line`, :meth:`get_browser_line`, :meth:`load_browser`,
   :meth:`get_browser_maxline`, :meth:`select_browser_line`,
   :meth:`deselect_browser_line`, :meth:`deselect_browser`,
   :meth:`isselected_browser_line`, :meth:`get_browser`,
   :meth:`set_browser_fontsize`, :meth:`set_browser_fontstyle`,
   :meth:`set_browser_specialkey`.


.. method:: form.add_timer(type, x, y, w, h, name)

   Add a timer object to the form.  ---  Methods: :meth:`set_timer`,
   :meth:`get_timer`.

Form objects have the following data attributes; see the FORMS documentation:

+---------------------+-----------------+--------------------------------+
| Name                | C Type          | Meaning                        |
+=====================+=================+================================+
| :attr:`window`      | int (read-only) | GL window id                   |
+---------------------+-----------------+--------------------------------+
| :attr:`w`           | float           | form width                     |
+---------------------+-----------------+--------------------------------+
| :attr:`h`           | float           | form height                    |
+---------------------+-----------------+--------------------------------+
| :attr:`x`           | float           | form x origin                  |
+---------------------+-----------------+--------------------------------+
| :attr:`y`           | float           | form y origin                  |
+---------------------+-----------------+--------------------------------+
| :attr:`deactivated` | int             | nonzero if form is deactivated |
+---------------------+-----------------+--------------------------------+
| :attr:`visible`     | int             | nonzero if form is visible     |
+---------------------+-----------------+--------------------------------+
| :attr:`frozen`      | int             | nonzero if form is frozen      |
+---------------------+-----------------+--------------------------------+
| :attr:`doublebuf`   | int             | nonzero if double buffering on |
+---------------------+-----------------+--------------------------------+


.. _forms-objects:

FORMS Objects
-------------

Besides methods specific to particular kinds of FORMS objects, all FORMS objects
also have the following methods:


.. method:: FORMS object.set_call_back(function, argument)

   Set the object's callback function and argument.  When the object needs
   interaction, the callback function will be called with two arguments: the
   object, and the callback argument.  (FORMS objects without a callback function
   are returned by :func:`fl.do_forms` or :func:`fl.check_forms` when they need
   interaction.)  Call this method without arguments to remove the callback
   function.


.. method:: FORMS object.delete_object()

   Delete the object.


.. method:: FORMS object.show_object()

   Show the object.


.. method:: FORMS object.hide_object()

   Hide the object.


.. method:: FORMS object.redraw_object()

   Redraw the object.


.. method:: FORMS object.freeze_object()

   Freeze the object.


.. method:: FORMS object.unfreeze_object()

   Unfreeze the object.

FORMS objects have these data attributes; see the FORMS documentation:

.. \begin{methoddesc}[FORMS object]{handle_object}{} XXX
.. \end{methoddesc}
.. \begin{methoddesc}[FORMS object]{handle_object_direct}{} XXX
.. \end{methoddesc}

+--------------------+-----------------+------------------+
| Name               | C Type          | Meaning          |
+====================+=================+==================+
| :attr:`objclass`   | int (read-only) | object class     |
+--------------------+-----------------+------------------+
| :attr:`type`       | int (read-only) | object type      |
+--------------------+-----------------+------------------+
| :attr:`boxtype`    | int             | box type         |
+--------------------+-----------------+------------------+
| :attr:`x`          | float           | x origin         |
+--------------------+-----------------+------------------+
| :attr:`y`          | float           | y origin         |
+--------------------+-----------------+------------------+
| :attr:`w`          | float           | width            |
+--------------------+-----------------+------------------+
| :attr:`h`          | float           | height           |
+--------------------+-----------------+------------------+
| :attr:`col1`       | int             | primary color    |
+--------------------+-----------------+------------------+
| :attr:`col2`       | int             | secondary color  |
+--------------------+-----------------+------------------+
| :attr:`align`      | int             | alignment        |
+--------------------+-----------------+------------------+
| :attr:`lcol`       | int             | label color      |
+--------------------+-----------------+------------------+
| :attr:`lsize`      | float           | label font size  |
+--------------------+-----------------+------------------+
| :attr:`label`      | string          | label string     |
+--------------------+-----------------+------------------+
| :attr:`lstyle`     | int             | label style      |
+--------------------+-----------------+------------------+
| :attr:`pushed`     | int (read-only) | (see FORMS docs) |
+--------------------+-----------------+------------------+
| :attr:`focus`      | int (read-only) | (see FORMS docs) |
+--------------------+-----------------+------------------+
| :attr:`belowmouse` | int (read-only) | (see FORMS docs) |
+--------------------+-----------------+------------------+
| :attr:`frozen`     | int (read-only) | (see FORMS docs) |
+--------------------+-----------------+------------------+
| :attr:`active`     | int (read-only) | (see FORMS docs) |
+--------------------+-----------------+------------------+
| :attr:`input`      | int (read-only) | (see FORMS docs) |
+--------------------+-----------------+------------------+
| :attr:`visible`    | int (read-only) | (see FORMS docs) |
+--------------------+-----------------+------------------+
| :attr:`radio`      | int (read-only) | (see FORMS docs) |
+--------------------+-----------------+------------------+
| :attr:`automatic`  | int (read-only) | (see FORMS docs) |
+--------------------+-----------------+------------------+


:mod:`FL` --- Constants used with the :mod:`fl` module
======================================================

.. module:: FL
   :platform: IRIX
   :synopsis: Constants used with the fl module.
   :deprecated:


.. deprecated:: 2.6
    The :mod:`FL` module has been removed in Python 3.


This module defines symbolic constants needed to use the built-in module
:mod:`fl` (see above); they are equivalent to those defined in the C header file
``<forms.h>`` except that the name prefix ``FL_`` is omitted.  Read the module
source for a complete list of the defined names.  Suggested use::

   import fl
   from FL import *


:mod:`flp` --- Functions for loading stored FORMS designs
=========================================================

.. module:: flp
   :platform: IRIX
   :synopsis: Functions for loading stored FORMS designs.
   :deprecated:


.. deprecated:: 2.6
    The :mod:`flp` module has been removed in Python 3.


This module defines functions that can read form definitions created by the
'form designer' (:program:`fdesign`) program that comes with the FORMS library
(see module :mod:`fl` above).

For now, see the file :file:`flp.doc` in the Python library source directory for
a description.

XXX A complete description should be inserted here!

