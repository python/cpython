
:mod:`getpass` --- Portable password input
==========================================

.. module:: getpass
   :synopsis: Portable reading of passwords and retrieval of the userid.
.. moduleauthor:: Piers Lauder <piers@cs.su.oz.au>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


.. % Windows (& Mac?) support by Guido van Rossum.

The :mod:`getpass` module provides two functions:


.. function:: getpass([prompt[, stream]])

   Prompt the user for a password without echoing.  The user is prompted using the
   string *prompt*, which defaults to ``'Password: '``. On Unix, the prompt is
   written to the file-like object *stream*, which defaults to ``sys.stdout`` (this
   argument is ignored on Windows).

   Availability: Macintosh, Unix, Windows.


.. function:: getuser()

   Return the "login name" of the user. Availability: Unix, Windows.

   This function checks the environment variables :envvar:`LOGNAME`,
   :envvar:`USER`, :envvar:`LNAME` and :envvar:`USERNAME`, in order, and returns
   the value of the first one which is set to a non-empty string.  If none are set,
   the login name from the password database is returned on systems which support
   the :mod:`pwd` module, otherwise, an exception is raised.

