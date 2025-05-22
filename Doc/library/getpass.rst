:mod:`!getpass` --- Portable password input
===========================================

.. module:: getpass
   :synopsis: Portable reading of passwords and retrieval of the userid.

.. moduleauthor:: Piers Lauder <piers@cs.su.oz.au>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. Windows (& Mac?) support by Guido van Rossum.

**Source code:** :source:`Lib/getpass.py`

--------------

.. include:: ../includes/wasm-notavail.rst

The :mod:`getpass` module provides two functions:

.. function:: getpass(prompt='Password: ', stream=None, *, echo_char=None)

   Prompt the user for a password without echoing.  The user is prompted using
   the string *prompt*, which defaults to ``'Password: '``.  On Unix, the
   prompt is written to the file-like object *stream* using the replace error
   handler if needed.  *stream* defaults to the controlling terminal
   (:file:`/dev/tty`) or if that is unavailable to ``sys.stderr`` (this
   argument is ignored on Windows).

   The *echo_char* argument controls how user input is displayed while typing.
   If *echo_char* is ``None`` (default), input remains hidden. Otherwise,
   *echo_char* must be a printable ASCII string and each typed character
   is replaced by it. For example, ``echo_char='*'`` will display
   asterisks instead of the actual input.

   If echo free input is unavailable getpass() falls back to printing
   a warning message to *stream* and reading from ``sys.stdin`` and
   issuing a :exc:`GetPassWarning`.

   .. note::
      If you call getpass from within IDLE, the input may be done in the
      terminal you launched IDLE from rather than the idle window itself.

   .. versionchanged:: 3.14
      Added the *echo_char* parameter for keyboard feedback.

.. exception:: GetPassWarning

   A :exc:`UserWarning` subclass issued when password input may be echoed.


.. function:: getuser()

   Return the "login name" of the user.

   This function checks the environment variables :envvar:`LOGNAME`,
   :envvar:`USER`, :envvar:`!LNAME` and :envvar:`USERNAME`, in order, and
   returns the value of the first one which is set to a non-empty string.  If
   none are set, the login name from the password database is returned on
   systems which support the :mod:`pwd` module, otherwise, an :exc:`OSError`
   is raised.

   In general, this function should be preferred over :func:`os.getlogin`.

   .. versionchanged:: 3.13
      Previously, various exceptions beyond just :exc:`OSError` were raised.
