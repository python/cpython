
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

   .. versionchanged:: 3.9 Handles multiple entries for the same host.


.. exception:: NetrcParseError

   Exception raised by the :class:`~netrc.netrc` class when syntactical errors are
   encountered in source text.  Instances of this exception provide three
   interesting attributes:  :attr:`msg` is a textual explanation of the error,
   :attr:`filename` is the name of the source file, and :attr:`lineno` gives the
   line number on which the error was found.


.. _netrc-objects:

netrc Objects
-------------

A :class:`~netrc.netrc` instance has the following methods:


.. method:: netrc.authenticators(host, [login])

   Return a 3-tuple ``(login, account, password)`` of authenticators for *host*
   and *login*.
   If the netrc file did not contain an entry for the given host, return the tuple
   associated with the 'default' entry.  If neither matching host nor default entry
   is available, return ``None``.
   If optional parameter *login* is not provided, it returns the first
   authenticators for the matching host. For further information see
   :class:`~netrc.netrc._use_first`.


.. method:: netrc.__repr__()

   Dump the class data as a string in the format of a netrc file. (This discards
   comments.)

Instances of :class:`~netrc.netrc` have public instance variables:


.. attribute:: netrc.hosts

   Dictionary mapping host names to ``(login, account, password)`` tuples.  The
   'default' entry, if any, is represented as a pseudo-host by that name.
   This dictionary only contains a single entry per host. For further information
   see :class:`~netrc.netrc._use_first`.


.. attribute:: netrc.macros

   Dictionary mapping macro names to string lists.

Configuration of class :class:`~netrc.netrc` behavior:


.. attribute:: netrc._use_first

   Controls the order of machine entries for the same host. If *True*,
   :class:`~netrc.netrc.authenticators` will return the first entry for a host,
   when called without providing *login*. Also :class:`~netrc.netrc.hosts` will
   contain the first entry for this machine. This would be inline with the common
   netrc implementation of other Unix tools.
   For backward compatibility the default value is *False*, i.e. the last item
   is used when no *login* is provided or :class:`~netrc.netrc.hosts` is looked up.
   Libraries shall not change this value (hence it is not a constructor
   parameter). The intention is that the end-user can configure the target Python
   installation (e.g. via sitecustomize) for consistent behavior.

   .. versionchanged:: 3.9
      Added to control entry order for hosts with multiple login.

.. note::

   Passwords are limited to a subset of the ASCII character set.  All ASCII
   punctuation is allowed in passwords, however, note that whitespace and
   non-printable characters are not allowed in passwords.  This is a limitation
   of the way the .netrc file is parsed and may be removed in the future.
