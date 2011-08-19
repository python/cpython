:mod:`smtpd` --- SMTP Server
============================

.. module:: smtpd
   :synopsis: A SMTP server implementation in Python.

.. moduleauthor:: Barry Warsaw <barry@zope.com>
.. sectionauthor:: Moshe Zadka <moshez@moshez.org>

**Source code:** :source:`Lib/smtpd.py`

--------------

This module offers several classes to implement SMTP servers.  One is a generic
do-nothing implementation, which can be overridden, while the other two offer
specific mail-sending strategies.


SMTPServer Objects
------------------


.. class:: SMTPServer(localaddr, remoteaddr)

   Create a new :class:`SMTPServer` object, which binds to local address
   *localaddr*.  It will treat *remoteaddr* as an upstream SMTP relayer.  It
   inherits from :class:`asyncore.dispatcher`, and so will insert itself into
   :mod:`asyncore`'s event loop on instantiation.


   .. method:: process_message(peer, mailfrom, rcpttos, data)

      Raise :exc:`NotImplementedError` exception. Override this in subclasses to
      do something useful with this message. Whatever was passed in the
      constructor as *remoteaddr* will be available as the :attr:`_remoteaddr`
      attribute. *peer* is the remote host's address, *mailfrom* is the envelope
      originator, *rcpttos* are the envelope recipients and *data* is a string
      containing the contents of the e-mail (which should be in :rfc:`2822`
      format).


DebuggingServer Objects
-----------------------


.. class:: DebuggingServer(localaddr, remoteaddr)

   Create a new debugging server.  Arguments are as per :class:`SMTPServer`.
   Messages will be discarded, and printed on stdout.


PureProxy Objects
-----------------


.. class:: PureProxy(localaddr, remoteaddr)

   Create a new pure proxy server. Arguments are as per :class:`SMTPServer`.
   Everything will be relayed to *remoteaddr*.  Note that running this has a good
   chance to make you into an open relay, so please be careful.


MailmanProxy Objects
--------------------


.. class:: MailmanProxy(localaddr, remoteaddr)

   Create a new pure proxy server. Arguments are as per :class:`SMTPServer`.
   Everything will be relayed to *remoteaddr*, unless local mailman configurations
   knows about an address, in which case it will be handled via mailman.  Note that
   running this has a good chance to make you into an open relay, so please be
   careful.

