:mod:`tkinter.commondialog` --- Tkinter standard dialog
=======================================================

.. module:: tkinter.commondialog
   :platform: Tk
   :synopsis: Tkinter base class for dialogs

**Source code:** :source:`Lib/tkinter/commondialog.py`

--------------

The :mod:`tkinter.commondialog` module provides the :class:`Dialog` class that
is the base class for dialogs defined in other supporting modules.

.. class:: Dialog(self, master=None, **options)

    .. function:: show(color = None, **options)

       The `show` function renders the Dialog window


.. seealso::

   Modules :mod:`tkinter.filedialog`, :mod:`tkinter.messagebox`, :mod:`tkinter.simpledialog`
      Individual specialized dialogs