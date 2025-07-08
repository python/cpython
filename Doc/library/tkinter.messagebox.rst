:mod:`!tkinter.messagebox` --- Tkinter message prompts
======================================================

.. module:: tkinter.messagebox
   :platform: Tk
   :synopsis: Various types of alert dialogs

**Source code:** :source:`Lib/tkinter/messagebox.py`

--------------

The :mod:`tkinter.messagebox` module provides a template base class as well as
a variety of convenience methods for commonly used configurations. The message
boxes are modal and will return a subset of (``True``, ``False``, ``None``,
:data:`OK`, :data:`CANCEL`, :data:`YES`, :data:`NO`) based on
the user's selection. Common message box styles and layouts include but are not
limited to:

.. figure:: tk_msg.png

.. class:: Message(master=None, **options)

   Create a message window with an application-specified message, an icon
   and a set of buttons.
   Each of the buttons in the message window is identified by a unique symbolic name (see the *type* options).

   The following options are supported:

      *command*
         Specifies the function to invoke when the user closes the dialog.
         The name of the button clicked by the user to close the dialog is
         passed as argument.
         This is only available on macOS.

      *default*
         Gives the :ref:`symbolic name <messagebox-buttons>` of the default button
         for this message window (:data:`OK`, :data:`CANCEL`, and so on).
         If this option is not specified, the first button in the dialog will
         be made the default.

      *detail*
         Specifies an auxiliary message to the main message given by the
         *message* option.
         The message detail will be presented beneath the main message and,
         where supported by the OS, in a less emphasized font than the main
         message.

      *icon*
         Specifies an :ref:`icon <messagebox-icons>` to display.
         If this option is not specified, then the :data:`INFO` icon will be
         displayed.

      *message*
         Specifies the message to display in this message box.
         The default value is an empty string.

      *parent*
         Makes the specified window the logical parent of the message box.
         The message box is displayed on top of its parent window.

      *title*
         Specifies a string to display as the title of the message box.
         This option is ignored on macOS, where platform guidelines forbid
         the use of a title on this kind of dialog.

      *type*
         Arranges for a :ref:`predefined set of buttons <messagebox-types>`
         to be displayed.

   .. method:: show(**options)

      Display a message window and wait for the user to select one of the buttons. Then return the symbolic name of the selected button.
      Keyword arguments can override options specified in the constructor.


**Information message box**

.. function:: showinfo(title=None, message=None, **options)

   Creates and displays an information message box with the specified title
   and message.

**Warning message boxes**

.. function:: showwarning(title=None, message=None, **options)

   Creates and displays a warning message box with the specified title
   and message.

.. function:: showerror(title=None, message=None, **options)

   Creates and displays an error message box with the specified title
   and message.

**Question message boxes**

.. function:: askquestion(title=None, message=None, *, type=YESNO, **options)

   Ask a question. By default shows buttons :data:`YES` and :data:`NO`.
   Returns the symbolic name of the selected button.

.. function:: askokcancel(title=None, message=None, **options)

   Ask if operation should proceed. Shows buttons :data:`OK` and :data:`CANCEL`.
   Returns ``True`` if the answer is ok and ``False`` otherwise.

.. function:: askretrycancel(title=None, message=None, **options)

   Ask if operation should be retried. Shows buttons :data:`RETRY` and :data:`CANCEL`.
   Return ``True`` if the answer is yes and ``False`` otherwise.

.. function:: askyesno(title=None, message=None, **options)

   Ask a question. Shows buttons :data:`YES` and :data:`NO`.
   Returns ``True`` if the answer is yes and ``False`` otherwise.

.. function:: askyesnocancel(title=None, message=None, **options)

   Ask a question. Shows buttons :data:`YES`, :data:`NO` and :data:`CANCEL`.
   Return ``True`` if the answer is yes, ``None`` if cancelled, and ``False``
   otherwise.


.. _messagebox-buttons:

Symbolic names of buttons:

.. data:: ABORT
   :value: 'abort'
.. data:: RETRY
   :value: 'retry'
.. data:: IGNORE
   :value: 'ignore'
.. data:: OK
   :value: 'ok'
.. data:: CANCEL
   :value: 'cancel'
.. data:: YES
   :value: 'yes'
.. data:: NO
   :value: 'no'

.. _messagebox-types:

Predefined sets of buttons:

.. data:: ABORTRETRYIGNORE
   :value: 'abortretryignore'

   Displays three buttons whose symbolic names are :data:`ABORT`,
   :data:`RETRY` and :data:`IGNORE`.

.. data:: OK
   :value: 'ok'
   :noindex:

   Displays one button whose symbolic name is :data:`OK`.

.. data:: OKCANCEL
   :value: 'okcancel'

   Displays two buttons whose symbolic names are :data:`OK` and
   :data:`CANCEL`.

.. data:: RETRYCANCEL
   :value: 'retrycancel'

   Displays two buttons whose symbolic names are :data:`RETRY` and
   :data:`CANCEL`.

.. data:: YESNO
   :value: 'yesno'

   Displays two buttons whose symbolic names are :data:`YES` and
   :data:`NO`.

.. data:: YESNOCANCEL
   :value: 'yesnocancel'

   Displays three buttons whose symbolic names are :data:`YES`,
   :data:`NO` and :data:`CANCEL`.

.. _messagebox-icons:

Icon images:

.. data:: ERROR
   :value: 'error'
.. data:: INFO
   :value: 'info'
.. data:: QUESTION
   :value: 'question'
.. data:: WARNING
   :value: 'warning'
