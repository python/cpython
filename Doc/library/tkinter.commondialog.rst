:mod:`tkinter.commondialog` --- Standard dialog windows
=======================================================

.. module:: tkinter.commondialog
   :platform: Tk
   :synopsis: Tkinter base class for dialogs

**Source code:** :source:`Lib/tkinter/commondialog.py`

--------------

The :mod:`tkinter.commondialog` module provides the :class:`Dialog` class that
is the base class for dialogs defined in other supporting modules.

.. class:: Dialog(master=None, **options)

   .. method:: show(color=None, **options)

      Render the Dialog window.


.. seealso::

   Modules :mod:`tkinter.filedialog`, :mod:`tkinter.messagebox`, :mod:`tkinter.simpledialog`
      Individual specialized dialogs