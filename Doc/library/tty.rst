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

.. availability:: Unix.

Because it requires the :mod:`termios` module, it will work only on Unix.

The :mod:`tty` module defines the following functions:


.. function:: cfmakeraw(mode)

   Convert the tty attribute list *mode*, which is a list like the one returned
   by :func:`termios.tcgetattr`, to that of a tty in raw mode.

   .. versionadded:: 3.12


.. function:: cfmakecbreak(mode)

   Convert the tty attribute list *mode*, which is a list like the one returned
   by :func:`termios.tcgetattr`, to that of a tty in cbreak mode.

   .. versionadded:: 3.12


.. function:: setraw(fd, when=termios.TCSAFLUSH)

   Change the mode of the file descriptor *fd* to raw. If *when* is omitted, it
   defaults to :const:`termios.TCSAFLUSH`, and is passed to
   :func:`termios.tcsetattr`. The return value of :func:`termios.tcgetattr`
   is saved before setting *fd* to raw mode; this value is returned.

   .. versionchanged:: 3.12
      The return value is now the original tty attributes, instead of None.


.. function:: setcbreak(fd, when=termios.TCSAFLUSH)

   Change the mode of file descriptor *fd* to cbreak. If *when* is omitted, it
   defaults to :const:`termios.TCSAFLUSH`, and is passed to
   :func:`termios.tcsetattr`. The return value of :func:`termios.tcgetattr`
   is saved before setting *fd* to cbreak mode; this value is returned.

   .. versionchanged:: 3.12
      The return value is now the original tty attributes, instead of None.


.. seealso::

   Module :mod:`termios`
      Low-level terminal control interface.

