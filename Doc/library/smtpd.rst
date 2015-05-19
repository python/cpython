:mod:`smtpd` --- SMTP Server
============================

.. module:: smtpd
   :synopsis: A SMTP server implementation in Python.

.. moduleauthor:: Barry Warsaw <barry@python.org>
.. sectionauthor:: Moshe Zadka <moshez@moshez.org>

**Source code:** :source:`Lib/smtpd.py`

--------------

This module offers several classes to implement SMTP (email) servers.

Several server implementations are present; one is a generic
do-nothing implementation, which can be overridden, while the other two offer
specific mail-sending strategies.

Additionally the SMTPChannel may be extended to implement very specific
interaction behaviour with SMTP clients.

The code supports :RFC:`5321`, plus the :rfc:`1870` SIZE and :rfc:`6531`
SMTPUTF8 extensions.


SMTPServer Objects
------------------


.. class:: SMTPServer(localaddr, remoteaddr, data_size_limit=33554432,\
                      map=None, enable_SMTPUTF8=False, decode_data=True)

   Create a new :class:`SMTPServer` object, which binds to local address
   *localaddr*.  It will treat *remoteaddr* as an upstream SMTP relayer.  It
   inherits from :class:`asyncore.dispatcher`, and so will insert itself into
   :mod:`asyncore`'s event loop on instantiation.

   *data_size_limit* specifies the maximum number of bytes that will be
   accepted in a ``DATA`` command.  A value of ``None`` or ``0`` means no
   limit.

   *map* is the socket map to use for connections (an initially empty
   dictionary is a suitable value).  If not specified the :mod:`asyncore`
   global socket map is used.

   *enable_SMTPUTF8* determins whether the ``SMTPUTF8`` extension (as defined
   in :RFC:`6531`) should be enabled.  The default is ``False``.  If set to
   ``True``, *decode_data* must be ``False`` (otherwise an error is raised).
   When ``True``, ``SMTPUTF8`` is accepted as a parameter to the ``MAIL``
   command and when present is passed to :meth:`process_message` in the
   ``kwargs['mail_options']`` list.

   *decode_data* specifies whether the data portion of the SMTP transaction
   should be decoded using UTF-8.  The default is ``True`` for backward
   compatibility reasons, but will change to ``False`` in Python 3.6; specify
   the keyword value explicitly to avoid the :exc:`DeprecationWarning`.  When
   *decode_data* is set to ``False`` the server advertises the ``8BITMIME``
   extension (:rfc:`6152`), accepts the ``BODY=8BITMIME`` parameter to
   the ``MAIL`` command, and when present passes it to :meth:`process_message`
   in the ``kwargs['mail_options']`` list.

   .. method:: process_message(peer, mailfrom, rcpttos, data, **kwargs)

      Raise a :exc:`NotImplementedError` exception. Override this in subclasses to
      do something useful with this message. Whatever was passed in the
      constructor as *remoteaddr* will be available as the :attr:`_remoteaddr`
      attribute. *peer* is the remote host's address, *mailfrom* is the envelope
      originator, *rcpttos* are the envelope recipients and *data* is a string
      containing the contents of the e-mail (which should be in :rfc:`5321`
      format).

      If the *decode_data* constructor keyword is set to ``True``, the *data*
      argument will be a unicode string.  If it is set to ``False``, it
      will be a bytes object.

      *kwargs* is a dictionary containing additional information. It is empty
      unless at least one of ``decode_data=False`` or ``enable_SMTPUTF8=True``
      was given as an init parameter, in which case it contains the following
      keys:

          *mail_options*:
             a list of all received parameters to the ``MAIL``
             command (the elements are uppercase strings; example:
             ``['BODY=8BITMIME', 'SMTPUTF8']``).

          *rcpt_options*:
             same as *mail_options* but for the ``RCPT`` command.
             Currently no ``RCPT TO`` options are supported, so for now
             this will always be an empty list.

      Implementations of ``process_message`` should use the ``**kwargs``
      signature to accept arbitrary keyword arguments, since future feature
      enhancements may add keys to the kwargs dictionary.

      Return ``None`` to request a normal ``250 Ok`` response; otherwise
      return the desired response string in :RFC:`5321` format.

   .. attribute:: channel_class

      Override this in subclasses to use a custom :class:`SMTPChannel` for
      managing SMTP clients.

   .. versionadded:: 3.4
      The *map* constructor argument.

   .. versionchanged:: 3.5
      *localaddr* and *remoteaddr* may now contain IPv6 addresses.

   .. versionadded:: 3.5
      the *decode_data* and *enable_SMTPUTF8* constructor arguments, and the
      *kwargs* argument to :meth:`process_message` when one or more of these is
      specified.


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

SMTPChannel Objects
-------------------

.. class:: SMTPChannel(server, conn, addr, data_size_limit=33554432,\
                       map=None, enable_SMTPUTF8=False, decode_data=True)

   Create a new :class:`SMTPChannel` object which manages the communication
   between the server and a single SMTP client.

   *conn* and *addr* are as per the instance variables described below.

   *data_size_limit* specifies the maximum number of bytes that will be
   accepted in a ``DATA`` command.  A value of ``None`` or ``0`` means no
   limit.

   *enable_SMTPUTF8* determins whether the ``SMTPUTF8`` extension (as defined
   in :RFC:`6531`) should be enabled.  The default is ``False``.  A
   :exc:`ValueError` is raised if both *enable_SMTPUTF8* and *decode_data* are
   set to ``True`` at the same time.

   A dictionary can be specified in *map* to avoid using a global socket map.

   *decode_data* specifies whether the data portion of the SMTP transaction
   should be decoded using UTF-8.  The default is ``True`` for backward
   compatibility reasons, but will change to ``False`` in Python 3.6.  Specify
   the keyword value explicitly to avoid the :exc:`DeprecationWarning`.

   To use a custom SMTPChannel implementation you need to override the
   :attr:`SMTPServer.channel_class` of your :class:`SMTPServer`.

   .. versionchanged:: 3.5
      the *decode_data* and *enable_SMTPUTF8* arguments were added.

   The :class:`SMTPChannel` has the following instance variables:

   .. attribute:: smtp_server

      Holds the :class:`SMTPServer` that spawned this channel.

   .. attribute:: conn

      Holds the socket object connecting to the client.

   .. attribute:: addr

      Holds the address of the client, the second value returned by
      :func:`socket.accept <socket.socket.accept>`

   .. attribute:: received_lines

      Holds a list of the line strings (decoded using UTF-8) received from
      the client. The lines have their ``"\r\n"`` line ending translated to
      ``"\n"``.

   .. attribute:: smtp_state

      Holds the current state of the channel. This will be either
      :attr:`COMMAND` initially and then :attr:`DATA` after the client sends
      a "DATA" line.

   .. attribute:: seen_greeting

      Holds a string containing the greeting sent by the client in its "HELO".

   .. attribute:: mailfrom

      Holds a string containing the address identified in the "MAIL FROM:" line
      from the client.

   .. attribute:: rcpttos

      Holds a list of strings containing the addresses identified in the
      "RCPT TO:" lines from the client.

   .. attribute:: received_data

      Holds a string containing all of the data sent by the client during the
      DATA state, up to but not including the terminating ``"\r\n.\r\n"``.

   .. attribute:: fqdn

      Holds the fully-qualified domain name of the server as returned by
      :func:`socket.getfqdn`.

   .. attribute:: peer

      Holds the name of the client peer as returned by ``conn.getpeername()``
      where ``conn`` is :attr:`conn`.

   The :class:`SMTPChannel` operates by invoking methods named ``smtp_<command>``
   upon reception of a command line from the client. Built into the base
   :class:`SMTPChannel` class are methods for handling the following commands
   (and responding to them appropriately):

   ======== ===================================================================
   Command  Action taken
   ======== ===================================================================
   HELO     Accepts the greeting from the client and stores it in
            :attr:`seen_greeting`.  Sets server to base command mode.
   EHLO     Accepts the greeting from the client and stores it in
            :attr:`seen_greeting`.  Sets server to extended command mode.
   NOOP     Takes no action.
   QUIT     Closes the connection cleanly.
   MAIL     Accepts the "MAIL FROM:" syntax and stores the supplied address as
            :attr:`mailfrom`.  In extended command mode, accepts the
            :rfc:`1870` SIZE attribute and responds appropriately based on the
            value of *data_size_limit*.
   RCPT     Accepts the "RCPT TO:" syntax and stores the supplied addresses in
            the :attr:`rcpttos` list.
   RSET     Resets the :attr:`mailfrom`, :attr:`rcpttos`, and
            :attr:`received_data`, but not the greeting.
   DATA     Sets the internal state to :attr:`DATA` and stores remaining lines
            from the client in :attr:`received_data` until the terminator
            ``"\r\n.\r\n"`` is received.
   HELP     Returns minimal information on command syntax
   VRFY     Returns code 252 (the server doesn't know if the address is valid)
   EXPN     Reports that the command is not implemented.
   ======== ===================================================================
