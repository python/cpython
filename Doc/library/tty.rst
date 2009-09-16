:mod:`tty` --- Terminal control functions
=========================================

.. module:: tty
   :platform: Unix
   :synopsis: Utility functions that perform common terminal control operations.
.. moduleauthor:: Steen Lumholt
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>


The :mod:`tty` module defines functions for putting the tty into cbreak and raw
modes.

Because it requires the :mod:`termios` module, it will work only on Unix.

The :mod:`tty` module defines the following functions:


.. function:: setraw(fd, when=termios.TCSAFLUSH)

   Change the mode of the file descriptor *fd* to raw. If *when* is omitted, it
   defaults to :const:`termios.TCSAFLUSH`, and is passed to
   :func:`termios.tcsetattr`.


.. function:: setcbreak(fd, when=termios.TCSAFLUSH)

   Change the mode of file descriptor *fd* to cbreak. If *when* is omitted, it
   defaults to :const:`termios.TCSAFLUSH`, and is passed to
   :func:`termios.tcsetattr`.


.. seealso::

   Module :mod:`termios`
      Low-level terminal control interface.

