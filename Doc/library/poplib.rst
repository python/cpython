:mod:`poplib` --- POP3 protocol client
======================================

.. module:: poplib
   :synopsis: POP3 protocol client (requires sockets).

.. sectionauthor:: Andrew T. Csillag
.. revised by ESR, January 2000

**Source code:** :source:`Lib/poplib.py`

.. index:: pair: POP3; protocol

--------------

This module defines a class, :class:`POP3`, which encapsulates a connection to a
POP3 server and implements the protocol as defined in :rfc:`1939`. The
:class:`POP3` class supports both the minimal and optional command sets from
:rfc:`1939`. The :class:`POP3` class also supports the ``STLS`` command introduced
in :rfc:`2595` to enable encrypted communication on an already established connection.

Additionally, this module provides a class :class:`POP3_SSL`, which provides
support for connecting to POP3 servers that use SSL as an underlying protocol
layer.

Note that POP3, though widely supported, is obsolescent.  The implementation
quality of POP3 servers varies widely, and too many are quite poor. If your
mailserver supports IMAP, you would be better off using the
:class:`imaplib.IMAP4` class, as IMAP servers tend to be better implemented.

.. include:: ../includes/wasm-notavail.rst

The :mod:`poplib` module provides two classes:


.. class:: POP3(host, port=POP3_PORT[, timeout])

   This class implements the actual POP3 protocol.  The connection is created when
   the instance is initialized. If *port* is omitted, the standard POP3 port (110)
   is used. The optional *timeout* parameter specifies a timeout in seconds for the
   connection attempt (if not specified, the global default timeout setting will
   be used).

   .. audit-event:: poplib.connect self,host,port poplib.POP3

   .. audit-event:: poplib.putline self,line poplib.POP3

      All commands will raise an :ref:`auditing event <auditing>`
      ``poplib.putline`` with arguments ``self`` and ``line``,
      where ``line`` is the bytes about to be sent to the remote host.

   .. versionchanged:: 3.9
      If the *timeout* parameter is set to be zero, it will raise a
      :class:`ValueError` to prevent the creation of a non-blocking socket.

.. class:: POP3_SSL(host, port=POP3_SSL_PORT, *, timeout=None, context=None)

   This is a subclass of :class:`POP3` that connects to the server over an SSL
   encrypted socket.  If *port* is not specified, 995, the standard POP3-over-SSL
   port is used.  *timeout* works as in the :class:`POP3` constructor.
   *context* is an optional :class:`ssl.SSLContext` object which allows
   bundling SSL configuration options, certificates and private keys into a
   single (potentially long-lived) structure.  Please read :ref:`ssl-security`
   for best practices.

   .. audit-event:: poplib.connect self,host,port poplib.POP3_SSL

   .. audit-event:: poplib.putline self,line poplib.POP3_SSL

      All commands will raise an :ref:`auditing event <auditing>`
      ``poplib.putline`` with arguments ``self`` and ``line``,
      where ``line`` is the bytes about to be sent to the remote host.

   .. versionchanged:: 3.2
      *context* parameter added.

   .. versionchanged:: 3.4
      The class now supports hostname check with
      :attr:`ssl.SSLContext.check_hostname` and *Server Name Indication* (see
      :const:`ssl.HAS_SNI`).

   .. versionchanged:: 3.9
      If the *timeout* parameter is set to be zero, it will raise a
      :class:`ValueError` to prevent the creation of a non-blocking socket.

   .. versionchanged:: 3.12
      The deprecated *keyfile* and *certfile* parameters have been removed.

One exception is defined as an attribute of the :mod:`poplib` module:


.. exception:: error_proto

   Exception raised on any errors from this module (errors from :mod:`socket`
   module are not caught). The reason for the exception is passed to the
   constructor as a string.


.. seealso::

   Module :mod:`imaplib`
      The standard Python IMAP module.

   `Frequently Asked Questions About Fetchmail <http://www.catb.org/~esr/fetchmail/fetchmail-FAQ.html>`_
      The FAQ for the :program:`fetchmail` POP/IMAP client collects information on
      POP3 server variations and RFC noncompliance that may be useful if you need to
      write an application based on the POP protocol.


.. _pop3-objects:

POP3 Objects
------------

All POP3 commands are represented by methods of the same name, in lowercase;
most return the response text sent by the server.

A :class:`POP3` instance has the following methods:


.. method:: POP3.set_debuglevel(level)

   Set the instance's debugging level.  This controls the amount of debugging
   output printed.  The default, ``0``, produces no debugging output.  A value of
   ``1`` produces a moderate amount of debugging output, generally a single line
   per request.  A value of ``2`` or higher produces the maximum amount of
   debugging output, logging each line sent and received on the control connection.


.. method:: POP3.getwelcome()

   Returns the greeting string sent by the POP3 server.


.. method:: POP3.capa()

   Query the server's capabilities as specified in :rfc:`2449`.
   Returns a dictionary in the form ``{'name': ['param'...]}``.

   .. versionadded:: 3.4


.. method:: POP3.user(username)

   Send user command, response should indicate that a password is required.


.. method:: POP3.pass_(password)

   Send password, response includes message count and mailbox size. Note: the
   mailbox on the server is locked until :meth:`~POP3.quit` is called.


.. method:: POP3.apop(user, secret)

   Use the more secure APOP authentication to log into the POP3 server.


.. method:: POP3.rpop(user)

   Use RPOP authentication (similar to UNIX r-commands) to log into POP3 server.


.. method:: POP3.stat()

   Get mailbox status.  The result is a tuple of 2 integers: ``(message count,
   mailbox size)``.


.. method:: POP3.list([which])

   Request message list, result is in the form ``(response, ['mesg_num octets',
   ...], octets)``. If *which* is set, it is the message to list.


.. method:: POP3.retr(which)

   Retrieve whole message number *which*, and set its seen flag. Result is in form
   ``(response, ['line', ...], octets)``.


.. method:: POP3.dele(which)

   Flag message number *which* for deletion.  On most servers deletions are not
   actually performed until QUIT (the major exception is Eudora QPOP, which
   deliberately violates the RFCs by doing pending deletes on any disconnect).


.. method:: POP3.rset()

   Remove any deletion marks for the mailbox.


.. method:: POP3.noop()

   Do nothing.  Might be used as a keep-alive.


.. method:: POP3.quit()

   Signoff:  commit changes, unlock mailbox, drop connection.


.. method:: POP3.top(which, howmuch)

   Retrieves the message header plus *howmuch* lines of the message after the
   header of message number *which*. Result is in form ``(response, ['line', ...],
   octets)``.

   The POP3 TOP command this method uses, unlike the RETR command, doesn't set the
   message's seen flag; unfortunately, TOP is poorly specified in the RFCs and is
   frequently broken in off-brand servers. Test this method by hand against the
   POP3 servers you will use before trusting it.


.. method:: POP3.uidl(which=None)

   Return message digest (unique id) list. If *which* is specified, result contains
   the unique id for that message in the form ``'response mesgnum uid``, otherwise
   result is list ``(response, ['mesgnum uid', ...], octets)``.


.. method:: POP3.utf8()

   Try to switch to UTF-8 mode. Returns the server response if successful,
   raises :class:`error_proto` if not. Specified in :RFC:`6856`.

   .. versionadded:: 3.5


.. method:: POP3.stls(context=None)

   Start a TLS session on the active connection as specified in :rfc:`2595`.
   This is only allowed before user authentication

   *context* parameter is a :class:`ssl.SSLContext` object which allows
   bundling SSL configuration options, certificates and private keys into
   a single (potentially long-lived) structure.  Please read :ref:`ssl-security`
   for best practices.

   This method supports hostname checking via
   :attr:`ssl.SSLContext.check_hostname` and *Server Name Indication* (see
   :const:`ssl.HAS_SNI`).

   .. versionadded:: 3.4


Instances of :class:`POP3_SSL` have no additional methods. The interface of this
subclass is identical to its parent.


.. _pop3-example:

POP3 Example
------------

Here is a minimal example (without error checking) that opens a mailbox and
retrieves and prints all messages::

   import getpass, poplib

   M = poplib.POP3('localhost')
   M.user(getpass.getuser())
   M.pass_(getpass.getpass())
   numMessages = len(M.list()[1])
   for i in range(numMessages):
       for j in M.retr(i+1)[1]:
           print(j)

At the end of the module, there is a test section that contains a more extensive
example of usage.
