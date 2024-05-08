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

.. function:: getpass(prompt='Password: ', stream=None)

   Prompt the user for a password without echoing.  The user is prompted using
   the string *prompt*, which defaults to ``'Password: '``.  On Unix, the
   prompt is written to the file-like object *stream* using the replace error
   handler if needed.  *stream* defaults to the controlling terminal
   (:file:`/dev/tty`) or if that is unavailable to ``sys.stderr`` (this
   argument is ignored on Windows).

   If echo free input is unavailable getpass() falls back to printing
   a warning message to *stream* and reading from ``sys.stdin`` and
   issuing a :exc:`GetPassWarning`.

   .. note::
      If you call getpass from within IDLE, the input may be done in the
      terminal you launched IDLE from rather than the idle window itself.

.. exception:: GetPassWarning

   A :exc:`UserWarning` subclass issued when password input may be echoed.


.. function:: getuser()

   Return the "login name" of the user.

   This function checks the environment variables :envvar:`LOGNAME`,
   :envvar:`USER`, :envvar:`!LNAME` and :envvar:`USERNAME`, in order, and
   returns the value of the first one which is set to a non-empty string.  If
   none are set, the login name from the password database is returned on
   systems which support the :mod:`pwd` module, otherwise, an exception is
   raised.

   In general, this function should be preferred over :func:`os.getlogin()`.
