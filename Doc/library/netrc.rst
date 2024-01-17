
:mod:`netrc` --- netrc file processing
======================================

.. module:: netrc
   :synopsis: Loading of .netrc files.

.. moduleauthor:: Eric S. Raymond <esr@snark.thyrsus.com>
.. sectionauthor:: Eric S. Raymond <esr@snark.thyrsus.com>

**Source code:** :source:`Lib/netrc.py`

--------------

The :class:`~netrc.netrc` class parses and encapsulates the netrc file format used by
the Unix :program:`ftp` program and other FTP clients.


.. class:: netrc([file])

   A :class:`~netrc.netrc` instance or subclass instance encapsulates data from  a netrc
   file.  The initialization argument, if present, specifies the file to parse.  If
   no argument is given, the file :file:`.netrc` in the user's home directory --
   as determined by :func:`os.path.expanduser` -- will be read.  Otherwise,
   a :exc:`FileNotFoundError` exception will be raised.
   Parse errors will raise :exc:`NetrcParseError` with diagnostic
   information including the file name, line number, and terminating token.
   If no argument is specified on a POSIX system, the presence of passwords in
   the :file:`.netrc` file will raise a :exc:`NetrcParseError` if the file
   ownership or permissions are insecure (owned by a user other than the user
   running the process, or accessible for read or write by any other user).
   This implements security behavior equivalent to that of ftp and other
   programs that use :file:`.netrc`.

   .. versionchanged:: 3.4 Added the POSIX permission check.

   .. versionchanged:: 3.7
      :func:`os.path.expanduser` is used to find the location of the
      :file:`.netrc` file when *file* is not passed as argument.

   .. versionchanged:: 3.10
      :class:`netrc` try UTF-8 encoding before using locale specific
      encoding.
      The entry in the netrc file no longer needs to contain all tokens.  The missing
      tokens' value default to an empty string.  All the tokens and their values now
      can contain arbitrary characters, like whitespace and non-ASCII characters.
      If the login name is anonymous, it won't trigger the security check.


.. exception:: NetrcParseError

   Exception raised by the :class:`~netrc.netrc` class when syntactical errors are
   encountered in source text.  Instances of this exception provide three
   interesting attributes:

   .. attribute:: msg

      Textual explanation of the error.

   .. attribute:: filename

      The name of the source file.

   .. attribute:: lineno

      The line number on which the error was found.


.. _netrc-objects:

netrc Objects
-------------

A :class:`~netrc.netrc` instance has the following methods:


.. method:: netrc.authenticators(host)

   Return a 3-tuple ``(login, account, password)`` of authenticators for *host*.
   If the netrc file did not contain an entry for the given host, return the tuple
   associated with the 'default' entry.  If neither matching host nor default entry
   is available, return ``None``.


.. method:: netrc.__repr__()

   Dump the class data as a string in the format of a netrc file. (This discards
   comments and may reorder the entries.)

Instances of :class:`~netrc.netrc` have public instance variables:


.. attribute:: netrc.hosts

   Dictionary mapping host names to ``(login, account, password)`` tuples.  The
   'default' entry, if any, is represented as a pseudo-host by that name.


.. attribute:: netrc.macros

   Dictionary mapping macro names to string lists.
