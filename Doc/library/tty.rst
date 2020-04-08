:mod:`tty` --- Terminal control functions
=========================================

.. module:: tty
   :platform: Unix
   :synopsis: Utility functions that perform common terminal control operations.

.. moduleauthor:: Steen Lumholt
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>

**Source code:** :source:`Lib/tty.py`

--------------

The :mod:`tty` module defines functions for putting the tty into cbreak and raw
modes.

Because it requires the :mod:`termios` module, it will work only on Unix.

The :mod:`tty` module defines the following functions:


.. function:: setraw(fd, when=termios.TCSAFLUSH, block=True)

   Change the mode of the file descriptor *fd* to raw. If *when* is omitted, it
   defaults to :const:`termios.TCSAFLUSH`, and is passed to
   :func:`termios.tcsetattr`.

   *block* specifies whether reading from the file with the descriptor *fd* will be blocking.
   If *block* is ``True`` (which is the default), reading from the file will expect a minimum of 1 character and no timeout;
   if *block* is ``False``, reading from the file has no minimum expectation and times out after 1 millisecond,
   which comes in handy when you want to have a mainloop or something that keeps running when there is no input.


.. function:: setcbreak(fd, when=termios.TCSAFLUSH)

   Change the mode of file descriptor *fd* to cbreak. If *when* is omitted, it
   defaults to :const:`termios.TCSAFLUSH`, and is passed to
   :func:`termios.tcsetattr`.


.. function:: save(fd=None, key=None)

   Save the mode of file descriptor *fd*, either with or without a key (which is typically a string but can be anything that is not ``None``).

   *fd* can be omitted in the case that you have set a default file descriptor using :func:`setdefaultfd`.
   If you have not set a default file descriptor, calling :func:`save` without *fd* will result in a :exc:`ValueError`.
   If you have set a default file descriptor, calling :func:`save` with another file descriptor *fd* will use the one that is provided as argument.

   If *key* is omitted, the save will be accessible only when using :func:`restore` with *key* omitted as well.
   No matter if you provide or omit *key*, :func:`save` always creates one step forward in history.

   Historical modes are stored internally in the module during the runtime.


.. function:: restore(fd=None, key=None, when=termios.TCSAFLUSH)

   Save the mode of file descriptor *fd*, either with or without a key (which is typically a string but can be anything that is not ``None``).

   *fd* can be omitted in the case that you have set a default file descriptor using :func:`setdefaultfd`.
   If you have not set a default file descriptor, calling :func:`restore` without *fd* will result in a :exc:`ValueError`.
   If you have set a default file descriptor, calling :func:`restore` with another file descriptor *fd* will use the one that is provided as argument.

   If *key* is omitted, the mode one step back in the history will be restored;
   sequential calls of :func:`restore` without *key* will go back in the mode history one step at a time.
   If *key* is provided, the mode saved with the specified key will be restored (if a mode has been saved with that key using :func:`save`),
   and this modification creates one step forward in the history.

   If *when* is omitted, it defaults to :const:`termios.TCSAFLUSH`, and is passed to :func:`termios.tcsetattr`.

   Historical modes are stored internally in the module during the runtime.


.. function:: setdefaultfd(fd)

   Set the default file descriptor *fd* for :func:`save` and :func:`restore`.
   This is especially useful when you just want to build something that runs on your terminal and all whose mode you care about is :attr:`sys.stdin`.

   The default file descriptor does not apply to :func:`setraw` or :func:`setcbreak`, in order to maintain compatibility.


.. seealso::

   Module :mod:`termios`
      Low-level terminal control interface.
