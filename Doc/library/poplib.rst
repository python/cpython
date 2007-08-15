
:mod:`poplib` --- POP3 protocol client
======================================

.. module:: poplib
   :synopsis: POP3 protocol client (requires sockets).


.. index:: pair: POP3; protocol

.. % By Andrew T. Csillag
.. % Even though I put it into LaTeX, I cannot really claim that I wrote
.. % it since I just stole most of it from the poplib.py source code and
.. % the imaplib ``chapter''.
.. % Revised by ESR, January 2000

This module defines a class, :class:`POP3`, which encapsulates a connection to a
POP3 server and implements the protocol as defined in :rfc:`1725`.  The
:class:`POP3` class supports both the minimal and optional command sets.
Additionally, this module provides a class :class:`POP3_SSL`, which provides
support for connecting to POP3 servers that use SSL as an underlying protocol
layer.

Note that POP3, though widely supported, is obsolescent.  The implementation
quality of POP3 servers varies widely, and too many are quite poor. If your
mailserver supports IMAP, you would be better off using the
:class:`imaplib.IMAP4` class, as IMAP servers tend to be better implemented.

A single class is provided by the :mod:`poplib` module:


.. class:: POP3(host[, port[, timeout]])

   This class implements the actual POP3 protocol.  The connection is created when
   the instance is initialized. If *port* is omitted, the standard POP3 port (110)
   is used. The optional *timeout* parameter specifies a timeout in seconds for the
   connection attempt (if not specified, or passed as None, the global default
   timeout setting will be used).

   .. versionchanged:: 2.6
      *timeout* was added.


.. class:: POP3_SSL(host[, port[, keyfile[, certfile]]])

   This is a subclass of :class:`POP3` that connects to the server over an SSL
   encrypted socket.  If *port* is not specified, 995, the standard POP3-over-SSL
   port is used.  *keyfile* and *certfile* are also optional - they can contain a
   PEM formatted private key and certificate chain file for the SSL connection.

   .. versionadded:: 2.4

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

All POP3 commands are represented by methods of the same name, in lower-case;
most return the response text sent by the server.

An :class:`POP3` instance has the following methods:


.. method:: POP3.set_debuglevel(level)

   Set the instance's debugging level.  This controls the amount of debugging
   output printed.  The default, ``0``, produces no debugging output.  A value of
   ``1`` produces a moderate amount of debugging output, generally a single line
   per request.  A value of ``2`` or higher produces the maximum amount of
   debugging output, logging each line sent and received on the control connection.


.. method:: POP3.getwelcome()

   Returns the greeting string sent by the POP3 server.


.. method:: POP3.user(username)

   Send user command, response should indicate that a password is required.


.. method:: POP3.pass_(password)

   Send password, response includes message count and mailbox size. Note: the
   mailbox on the server is locked until :meth:`quit` is called.


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


.. method:: POP3.uidl([which])

   Return message digest (unique id) list. If *which* is specified, result contains
   the unique id for that message in the form ``'response mesgnum uid``, otherwise
   result is list ``(response, ['mesgnum uid', ...], octets)``.

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
           print j

At the end of the module, there is a test section that contains a more extensive
example of usage.

