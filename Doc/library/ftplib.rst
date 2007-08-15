
:mod:`ftplib` --- FTP protocol client
=====================================

.. module:: ftplib
   :synopsis: FTP protocol client (requires sockets).


.. index::
   pair: FTP; protocol
   single: FTP; ftplib (standard module)

This module defines the class :class:`FTP` and a few related items. The
:class:`FTP` class implements the client side of the FTP protocol.  You can use
this to write Python programs that perform a variety of automated FTP jobs, such
as mirroring other ftp servers.  It is also used by the module :mod:`urllib` to
handle URLs that use FTP.  For more information on FTP (File Transfer Protocol),
see Internet :rfc:`959`.

Here's a sample session using the :mod:`ftplib` module::

   >>> from ftplib import FTP
   >>> ftp = FTP('ftp.cwi.nl')   # connect to host, default port
   >>> ftp.login()               # user anonymous, passwd anonymous@
   >>> ftp.retrlines('LIST')     # list directory contents
   total 24418
   drwxrwsr-x   5 ftp-usr  pdmaint     1536 Mar 20 09:48 .
   dr-xr-srwt 105 ftp-usr  pdmaint     1536 Mar 21 14:32 ..
   -rw-r--r--   1 ftp-usr  pdmaint     5305 Mar 20 09:48 INDEX
    .
    .
    .
   >>> ftp.retrbinary('RETR README', open('README', 'wb').write)
   '226 Transfer complete.'
   >>> ftp.quit()

The module defines the following items:


.. class:: FTP([host[, user[, passwd[, acct[, timeout]]]]])

   Return a new instance of the :class:`FTP` class.  When *host* is given, the
   method call ``connect(host)`` is made.  When *user* is given, additionally the
   method call ``login(user, passwd, acct)`` is made (where *passwd* and *acct*
   default to the empty string when not given). The optional *timeout* parameter
   specifies a timeout in seconds for the connection attempt (if is not specified,
   or passed as None, the global default timeout setting will be used).

   .. versionchanged:: 2.6
      *timeout* was added.


.. data:: all_errors

   The set of all exceptions (as a tuple) that methods of :class:`FTP` instances
   may raise as a result of problems with the FTP connection (as opposed to
   programming errors made by the caller).  This set includes the four exceptions
   listed below as well as :exc:`socket.error` and :exc:`IOError`.


.. exception:: error_reply

   Exception raised when an unexpected reply is received from the server.


.. exception:: error_temp

   Exception raised when an error code in the range 400--499 is received.


.. exception:: error_perm

   Exception raised when an error code in the range 500--599 is received.


.. exception:: error_proto

   Exception raised when a reply is received from the server that does not begin
   with a digit in the range 1--5.


.. seealso::

   Module :mod:`netrc`
      Parser for the :file:`.netrc` file format.  The file :file:`.netrc` is typically
      used by FTP clients to load user authentication information before prompting the
      user.

   .. index:: single: ftpmirror.py

   The file :file:`Tools/scripts/ftpmirror.py` in the Python source distribution is
   a script that can mirror FTP sites, or portions thereof, using the :mod:`ftplib`
   module. It can be used as an extended example that applies this module.


.. _ftp-objects:

FTP Objects
-----------

Several methods are available in two flavors: one for handling text files and
another for binary files.  These are named for the command which is used
followed by ``lines`` for the text version or ``binary`` for the binary version.

:class:`FTP` instances have the following methods:


.. method:: FTP.set_debuglevel(level)

   Set the instance's debugging level.  This controls the amount of debugging
   output printed.  The default, ``0``, produces no debugging output.  A value of
   ``1`` produces a moderate amount of debugging output, generally a single line
   per request.  A value of ``2`` or higher produces the maximum amount of
   debugging output, logging each line sent and received on the control connection.


.. method:: FTP.connect(host[, port[, timeout]])

   Connect to the given host and port.  The default port number is ``21``, as
   specified by the FTP protocol specification.  It is rarely needed to specify a
   different port number.  This function should be called only once for each
   instance; it should not be called at all if a host was given when the instance
   was created.  All other methods can only be used after a connection has been
   made.

   The optional *timeout* parameter specifies a timeout in seconds for the
   connection attempt. If is not specified, or passed as None, the  object timeout
   is used (the timeout that you passed when instantiating the class); if the
   object timeout is also None, the global default timeout  setting will be used.

   .. versionchanged:: 2.6
      *timeout* was added.


.. method:: FTP.getwelcome()

   Return the welcome message sent by the server in reply to the initial
   connection.  (This message sometimes contains disclaimers or help information
   that may be relevant to the user.)


.. method:: FTP.login([user[, passwd[, acct]]])

   Log in as the given *user*.  The *passwd* and *acct* parameters are optional and
   default to the empty string.  If no *user* is specified, it defaults to
   ``'anonymous'``.  If *user* is ``'anonymous'``, the default *passwd* is
   ``'anonymous@'``.  This function should be called only once for each instance,
   after a connection has been established; it should not be called at all if a
   host and user were given when the instance was created.  Most FTP commands are
   only allowed after the client has logged in.


.. method:: FTP.abort()

   Abort a file transfer that is in progress.  Using this does not always work, but
   it's worth a try.


.. method:: FTP.sendcmd(command)

   Send a simple command string to the server and return the response string.


.. method:: FTP.voidcmd(command)

   Send a simple command string to the server and handle the response. Return
   nothing if a response code in the range 200--299 is received. Raise an exception
   otherwise.


.. method:: FTP.retrbinary(command, callback[, maxblocksize[, rest]])

   Retrieve a file in binary transfer mode.  *command* should be an appropriate
   ``RETR`` command: ``'RETR filename'``. The *callback* function is called for
   each block of data received, with a single string argument giving the data
   block. The optional *maxblocksize* argument specifies the maximum chunk size to
   read on the low-level socket object created to do the actual transfer (which
   will also be the largest size of the data blocks passed to *callback*).  A
   reasonable default is chosen. *rest* means the same thing as in the
   :meth:`transfercmd` method.


.. method:: FTP.retrlines(command[, callback])

   Retrieve a file or directory listing in ASCII transfer mode. *command* should be
   an appropriate ``RETR`` command (see :meth:`retrbinary`) or a ``LIST`` command
   (usually just the string ``'LIST'``).  The *callback* function is called for
   each line, with the trailing CRLF stripped.  The default *callback* prints the
   line to ``sys.stdout``.


.. method:: FTP.set_pasv(boolean)

   Enable "passive" mode if *boolean* is true, other disable passive mode.  (In
   Python 2.0 and before, passive mode was off by default; in Python 2.1 and later,
   it is on by default.)


.. method:: FTP.storbinary(command, file[, blocksize])

   Store a file in binary transfer mode.  *command* should be an appropriate
   ``STOR`` command: ``"STOR filename"``. *file* is an open file object which is
   read until EOF using its :meth:`read` method in blocks of size *blocksize* to
   provide the data to be stored.  The *blocksize* argument defaults to 8192.

   .. versionchanged:: 2.1
      default for *blocksize* added.


.. method:: FTP.storlines(command, file)

   Store a file in ASCII transfer mode.  *command* should be an appropriate
   ``STOR`` command (see :meth:`storbinary`).  Lines are read until EOF from the
   open file object *file* using its :meth:`readline` method to provide the data to
   be stored.


.. method:: FTP.transfercmd(cmd[, rest])

   Initiate a transfer over the data connection.  If the transfer is active, send a
   ``EPRT`` or  ``PORT`` command and the transfer command specified by *cmd*, and
   accept the connection.  If the server is passive, send a ``EPSV`` or ``PASV``
   command, connect to it, and start the transfer command.  Either way, return the
   socket for the connection.

   If optional *rest* is given, a ``REST`` command is sent to the server, passing
   *rest* as an argument.  *rest* is usually a byte offset into the requested file,
   telling the server to restart sending the file's bytes at the requested offset,
   skipping over the initial bytes.  Note however that RFC 959 requires only that
   *rest* be a string containing characters in the printable range from ASCII code
   33 to ASCII code 126.  The :meth:`transfercmd` method, therefore, converts
   *rest* to a string, but no check is performed on the string's contents.  If the
   server does not recognize the ``REST`` command, an :exc:`error_reply` exception
   will be raised.  If this happens, simply call :meth:`transfercmd` without a
   *rest* argument.


.. method:: FTP.ntransfercmd(cmd[, rest])

   Like :meth:`transfercmd`, but returns a tuple of the data connection and the
   expected size of the data.  If the expected size could not be computed, ``None``
   will be returned as the expected size.  *cmd* and *rest* means the same thing as
   in :meth:`transfercmd`.


.. method:: FTP.nlst(argument[, ...])

   Return a list of files as returned by the ``NLST`` command.  The optional
   *argument* is a directory to list (default is the current server directory).
   Multiple arguments can be used to pass non-standard options to the ``NLST``
   command.


.. method:: FTP.dir(argument[, ...])

   Produce a directory listing as returned by the ``LIST`` command, printing it to
   standard output.  The optional *argument* is a directory to list (default is the
   current server directory).  Multiple arguments can be used to pass non-standard
   options to the ``LIST`` command.  If the last argument is a function, it is used
   as a *callback* function as for :meth:`retrlines`; the default prints to
   ``sys.stdout``.  This method returns ``None``.


.. method:: FTP.rename(fromname, toname)

   Rename file *fromname* on the server to *toname*.


.. method:: FTP.delete(filename)

   Remove the file named *filename* from the server.  If successful, returns the
   text of the response, otherwise raises :exc:`error_perm` on permission errors or
   :exc:`error_reply` on other errors.


.. method:: FTP.cwd(pathname)

   Set the current directory on the server.


.. method:: FTP.mkd(pathname)

   Create a new directory on the server.


.. method:: FTP.pwd()

   Return the pathname of the current directory on the server.


.. method:: FTP.rmd(dirname)

   Remove the directory named *dirname* on the server.


.. method:: FTP.size(filename)

   Request the size of the file named *filename* on the server.  On success, the
   size of the file is returned as an integer, otherwise ``None`` is returned.
   Note that the ``SIZE`` command is not  standardized, but is supported by many
   common server implementations.


.. method:: FTP.quit()

   Send a ``QUIT`` command to the server and close the connection. This is the
   "polite" way to close a connection, but it may raise an exception of the server
   reponds with an error to the ``QUIT`` command.  This implies a call to the
   :meth:`close` method which renders the :class:`FTP` instance useless for
   subsequent calls (see below).


.. method:: FTP.close()

   Close the connection unilaterally.  This should not be applied to an already
   closed connection such as after a successful call to :meth:`quit`.  After this
   call the :class:`FTP` instance should not be used any more (after a call to
   :meth:`close` or :meth:`quit` you cannot reopen the connection by issuing
   another :meth:`login` method).

