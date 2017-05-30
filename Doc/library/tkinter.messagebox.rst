:mod:`tkinter.commondialog` --- Tkinter message prompts
=======================================================

.. module:: tkinter.messagebox
   :platform: Tk
   :synopsis: Tkinter base class for dialogs

**Source code:** :source:`Lib/tkinter/messagebox.py`

--------------

The :mod:`tkinter.messagebox` module provides a set of commonly used native
message box dialogs.

.. FIXME add graphic representation of message boxes for disambiguation

.. class:: Message(self, master=None, **options)

   Displays a default message box

.. function:: askokcancel(title=None, message=None, **options)

   Displays a confirmation prompt

.. function:: askquestion(title=None, message=None, **options)

   Displays a question prompt

.. function:: askretrycancel(title=None, message=None, **options)

   Displays a retry operation prompt

.. function:: askyesno(title=None, message=None, **options)

   Displays a consent prompt

.. function:: askyesnocancel(title=None, message=None, **options)

   Displays a confirmation prompt (returns None if cancelled)

.. function:: showinfo(title=None, message=None, **options)

   Displays an information-style messagebox

.. function:: showerror(title=None, message=None, **options)

   Displays an error message

.. function:: showwarning(title=None, message=None, **options)

   Displays a warning message
