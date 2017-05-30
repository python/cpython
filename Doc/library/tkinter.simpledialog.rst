:mod:`tkinter.simpledialog` --- Tkinter simple dialogs
======================================================

.. module:: tkinter.simpledialog
   :platform: Tk
   :synopsis: Simple dialog windows

**Source code:** :source:`Lib/tkinter/simpledialog.py`

--------------

The :mod:`tkinter.simpledialog` module contains the standard classes and
methods for handling dialogs, including custom dialog windows.

.. class:: Dialog(self, parent, title = None)

   The base class for custom dialogs

    .. function:: body(self, master)

       Override to construct the dialog's interface and return widget that
       should have initial focus

    .. function:: buttonbox(self)

       Default behaviour adds OK and Cancel buttons. Override for custom button
       layouts.


.. Static factory methods

.. method:: askfloat(title, prompt, **kw)

   Creates dialog prompt that asks user for a float value

.. method:: askinteger(title, prompt, **kw)

   Creates dialog prompt that asks user for an integer value

.. method:: askstring(title, prompt, **kw)

   Creates dialog prompt that asks user for a string value