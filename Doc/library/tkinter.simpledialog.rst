:mod:`tkinter.simpledialog` --- Tkinter simple dialogs
======================================================

.. module:: tkinter.simpledialog
   :platform: Tk
   :synopsis: Simple dialog windows

**Source code:** :source:`Lib/tkinter/simpledialog.py`

--------------

The :mod:`tkinter.simpledialog` module contains the standard classes and
functions for handling dialogs, including custom dialog windows.

.. class:: Dialog(parent, title=None)

   The base class for custom dialogs.

    .. method:: body(master)

       Override to construct the dialog's interface and return widget that
       should have initial focus.

    .. method:: buttonbox()

       Default behaviour adds OK and Cancel buttons. Override for custom button
       layouts.


.. Static factory function


.. function:: askfloat(title, prompt, **kw)
              askinteger(title, prompt, **kw)
              askstring(title, prompt, **kw)

   The above three functions provide dialogs that prompt the user to enter a value
   of the desired type.