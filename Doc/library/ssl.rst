:mod:`ssl` --- TLS/SSL wrapper for socket objects
=================================================

.. module:: ssl
   :synopsis: TLS/SSL wrapper for socket objects

.. moduleauthor:: Bill Janssen <bill.janssen@gmail.com>
.. sectionauthor::  Bill Janssen <bill.janssen@gmail.com>


.. index:: single: OpenSSL; (use in module ssl)

.. index:: TLS, SSL, Transport Layer Security, Secure Sockets Layer

.. versionadded:: 2.6

**Source code:** :source:`Lib/ssl.py`

--------------

This module provides access to Transport Layer Security (often known as "Secure
Sockets Layer") encryption and peer authentication facilities for network
sockets, both client-side and server-side.  This module uses the OpenSSL
library. It is available on all modern Unix systems, Windows, Mac OS X, and
probably additional platforms, as long as OpenSSL is installed on that platform.

.. note::

   Some behavior may be platform dependent, since calls are made to the
   operating system socket APIs.  The installed version of OpenSSL may also
   cause variations in behavior. For example, TLSv1.1 and TLSv1.2 come with
   openssl version 1.0.1.

.. warning::
   Don't use this module without reading the :ref:`ssl-security`.  Doing so
   may lead to a false sense of security, as the default settings of the
   ssl module are not necessarily appropriate for your application.


This section documents the objects and functions in the ``ssl`` module; for more
general information about TLS, SSL, and certificates, the reader is referred to
the documents in the "See Also" section at the bottom.

This module provides a class, :class:`ssl.SSLSocket`, which is derived from the
:class:`socket.socket` type, and provides a socket-like wrapper that also
encrypts and decrypts the data going over the socket with SSL.  It supports
additional methods such as :meth:`getpeercert`, which retrieves the
certificate of the other side of the connection, and :meth:`cipher`,which
retrieves the cipher being used for the secure connection.

For more sophisticated applications, the :class:`ssl.SSLContext` class
helps manage settings and certificates, which can then be inherited
by SSL sockets created through the :meth:`SSLContext.wrap_socket` method.


Functions, Constants, and Exceptions
------------------------------------

.. exception:: SSLError

   Raised to signal an error from the underlying SSL implementation (currently
   provided by the OpenSSL library).  This signifies some problem in the
   higher-level encryption and authentication layer that's superimposed on the
   underlying network connection.  This error is a subtype of
   :exc:`socket.error`, which in turn is a subtype of :exc:`IOError`.  The
   error code and message of :exc:`SSLError` instances are provided by the
   OpenSSL library.

   .. attribute:: library

      A string mnemonic designating the OpenSSL submodule in which the error
      occurred, such as ``SSL``, ``PEM`` or ``X509``.  The range of possible
      values depends on the OpenSSL version.

      .. versionadded:: 2.7.9

   .. attribute:: reason

      A string mnemonic designating the reason this error occurred, for
      example ``CERTIFICATE_VERIFY_FAILED``.  The range of possible
      values depends on the OpenSSL version.

      .. versionadded:: 2.7.9

.. exception:: SSLZeroReturnError

   A subclass of :exc:`SSLError` raised when trying to read or write and
   the SSL connection has been closed cleanly.  Note that this doesn't
   mean that the underlying transport (read TCP) has been closed.

   .. versionadded:: 2.7.9

.. exception:: SSLWantReadError

   A subclass of :exc:`SSLError` raised by a :ref:`non-blocking SSL socket
   <ssl-nonblocking>` when trying to read or write data, but more data needs
   to be received on the underlying TCP transport before the request can be
   fulfilled.

   .. versionadded:: 2.7.9

.. exception:: SSLWantWriteError

   A subclass of :exc:`SSLError` raised by a :ref:`non-blocking SSL socket
   <ssl-nonblocking>` when trying to read or write data, but more data needs
   to be sent on the underlying TCP transport before the request can be
   fulfilled.

   .. versionadded:: 2.7.9

.. exception:: SSLSyscallError

   A subclass of :exc:`SSLError` raised when a system error was encountered
   while trying to fulfill an operation on a SSL socket.  Unfortunately,
   there is no easy way to inspect the original errno number.

   .. versionadded:: 2.7.9

.. exception:: SSLEOFError

   A subclass of :exc:`SSLError` raised when the SSL connection has been
   terminated abruptly.  Generally, you shouldn't try to reuse the underlying
   transport when this error is encountered.

   .. versionadded:: 2.7.9

.. exception:: CertificateError

   Raised to signal an error with a certificate (such as mismatching
   hostname).  Certificate errors detected by OpenSSL, though, raise
   an :exc:`SSLError`.


Socket creation
^^^^^^^^^^^^^^^

The following function allows for standalone socket creation.  Starting from
Python 2.7.9, it can be more flexible to use :meth:`SSLContext.wrap_socket`
instead.

.. function:: wrap_socket(sock, keyfile=None, certfile=None, server_side=False, cert_reqs=CERT_NONE, ssl_version={see docs}, ca_certs=None, do_handshake_on_connect=True, suppress_ragged_eofs=True, ciphers=None)

   Takes an instance ``sock`` of :class:`socket.socket`, and returns an instance
   of :class:`ssl.SSLSocket`, a subtype of :class:`socket.socket`, which wraps
   the underlying socket in an SSL context.  ``sock`` must be a
   :data:`~socket.SOCK_STREAM` socket; other socket types are unsupported.

   For client-side sockets, the context construction is lazy; if the
   underlying socket isn't connected yet, the context construction will be
   performed after :meth:`connect` is called on the socket.  For
   server-side sockets, if the socket has no remote peer, it is assumed
   to be a listening socket, and the server-side SSL wrapping is
   automatically performed on client connections accepted via the
   :meth:`accept` method.  :func:`wrap_socket` may raise :exc:`SSLError`.

   The ``keyfile`` and ``certfile`` parameters specify optional files which
   contain a certificate to be used to identify the local side of the
   connection.  See the discussion of :ref:`ssl-certificates` for more
   information on how the certificate is stored in the ``certfile``.

   The parameter ``server_side`` is a boolean which identifies whether
   server-side or client-side behavior is desired from this socket.

   The parameter ``cert_reqs`` specifies whether a certificate is required from
   the other side of the connection, and whether it will be validated if
   provided.  It must be one of the three values :const:`CERT_NONE`
   (certificates ignored), :const:`CERT_OPTIONAL` (not required, but validated
   if provided), or :const:`CERT_REQUIRED` (required and validated).  If the
   value of this parameter is not :const:`CERT_NONE`, then the ``ca_certs``
   parameter must point to a file of CA certificates.

   The ``ca_certs`` file contains a set of concatenated "certification
   authority" certificates, which are used to validate certificates passed from
   the other end of the connection.  See the discussion of
   :ref:`ssl-certificates` for more information about how to arrange the
   certificates in this file.

   The parameter ``ssl_version`` specifies which version of the SSL protocol to
   use.  Typically, the server chooses a particular protocol version, and the
   client must adapt to the server's choice.  Most of the versions are not
   interoperable with the other versions.  If not specified, the default is
   :data:`PROTOCOL_SSLv23`; it provides the most compatibility with other
   versions.

   Here's a table showing which versions in a client (down the side) can connect
   to which versions in a server (along the top):

     .. table::

       ========================  =========  =========  ==========  =========  ===========  ===========
        *client* / **server**    **SSLv2**  **SSLv3**  **SSLv23**  **TLSv1**  **TLSv1.1**  **TLSv1.2**
       ------------------------  ---------  ---------  ----------  ---------  -----------  -----------
        *SSLv2*                    yes        no         yes         no         no         no
        *SSLv3*                    no         yes        yes         no         no         no
        *SSLv23*                   no         yes        yes         yes        yes        yes
        *TLSv1*                    no         no         yes         yes        no         no
        *TLSv1.1*                  no         no         yes         no         yes        no
        *TLSv1.2*                  no         no         yes         no         no         yes
       ========================  =========  =========  ==========  =========  ===========  ===========

   .. note::

      Which connections succeed will vary depending on the version of
      OpenSSL.  For example, before OpenSSL 1.0.0, an SSLv23 client
      would always attempt SSLv2 connections.

   The *ciphers* parameter sets the available ciphers for this SSL object.
   It should be a string in the `OpenSSL cipher list format
   <http://www.openssl.org/docs/apps/ciphers.html#CIPHER-LIST-FORMAT>`_.

   The parameter ``do_handshake_on_connect`` specifies whether to do the SSL
   handshake automatically after doing a :meth:`socket.connect`, or whether the
   application program will call it explicitly, by invoking the
   :meth:`SSLSocket.do_handshake` method.  Calling
   :meth:`SSLSocket.do_handshake` explicitly gives the program control over the
   blocking behavior of the socket I/O involved in the handshake.

   The parameter ``suppress_ragged_eofs`` specifies how the
   :meth:`SSLSocket.read` method should signal unexpected EOF from the other end
   of the connection.  If specified as :const:`True` (the default), it returns a
   normal EOF (an empty bytes object) in response to unexpected EOF errors
   raised from the underlying socket; if :const:`False`, it will raise the
   exceptions back to the caller.

   .. versionchanged:: 2.7
      New optional argument *ciphers*.


Context creation
^^^^^^^^^^^^^^^^

A convenience function helps create :class:`SSLContext` objects for common
purposes.

.. function:: create_default_context(purpose=Purpose.SERVER_AUTH, cafile=None, capath=None, cadata=None)

   Return a new :class:`SSLContext` object with default settings for
   the given *purpose*.  The settings are chosen by the :mod:`ssl` module,
   and usually represent a higher security level than when calling the
   :class:`SSLContext` constructor directly.

   *cafile*, *capath*, *cadata* represent optional CA certificates to
   trust for certificate verification, as in
   :meth:`SSLContext.load_verify_locations`.  If all three are
   :const:`None`, this function can choose to trust the system's default
   CA certificates instead.

   The settings are: :data:`PROTOCOL_SSLv23`, :data:`OP_NO_SSLv2`, and
   :data:`OP_NO_SSLv3` with high encryption cipher suites without RC4 and
   without unauthenticated cipher suites. Passing :data:`~Purpose.SERVER_AUTH`
   as *purpose* sets :data:`~SSLContext.verify_mode` to :data:`CERT_REQUIRED`
   and either loads CA certificates (when at least one of *cafile*, *capath* or
   *cadata* is given) or uses :meth:`SSLContext.load_default_certs` to load
   default CA certificates.

   .. note::
      The protocol, options, cipher and other settings may change to more
      restrictive values anytime without prior deprecation.  The values
      represent a fair balance between compatibility and security.

      If your application needs specific settings, you should create a
      :class:`SSLContext` and apply the settings yourself.

   .. note::
      If you find that when certain older clients or servers attempt to connect
      with a :class:`SSLContext` created by this function that they get an error
      stating "Protocol or cipher suite mismatch", it may be that they only
      support SSL3.0 which this function excludes using the
      :data:`OP_NO_SSLv3`. SSL3.0 is widely considered to be `completely broken
      <https://en.wikipedia.org/wiki/POODLE>`_. If you still wish to continue to
      use this function but still allow SSL 3.0 connections you can re-enable
      them using::

         ctx = ssl.create_default_context(Purpose.CLIENT_AUTH)
         ctx.options &= ~ssl.OP_NO_SSLv3

   .. versionadded:: 2.7.9

   .. versionchanged:: 2.7.10

     RC4 was dropped from the default cipher string.

.. function:: _https_verify_certificates(enable=True)

   Specifies whether or not server certificates are verified when creating
   client HTTPS connections without specifying a particular SSL context.

   Starting with Python 2.7.9, :mod:`httplib` and modules which use it, such as
   :mod:`urllib2` and :mod:`xmlrpclib`, default to verifying remote server
   certificates received when establishing client HTTPS connections. This
   default verification checks that the certificate is signed by a Certificate
   Authority in the system trust store and that the Common Name (or Subject
   Alternate Name) on the presented certificate matches the requested host.

   Setting *enable* to :const:`True` ensures this default behaviour is in
   effect.

   Setting *enable* to :const:`False` reverts the default HTTPS certificate
   handling to that of Python 2.7.8 and earlier, allowing connections to
   servers using self-signed certificates, servers using certificates signed
   by a Certicate Authority not present in the system trust store, and servers
   where the hostname does not match the presented server certificate.

   The leading underscore on this function denotes that it intentionally does
   not exist in any implementation of Python 3 and may not be present in all
   Python 2.7 implementations. The portable approach to bypassing certificate
   checks or the system trust store when necessary is for tools to enable that
   on a case-by-case basis by explicitly passing in a suitably configured SSL
   context, rather than reverting the default behaviour of the standard library
   client modules.

   .. versionadded:: 2.7.12

   .. seealso::

      * `CVE-2014-9365 <http://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2014-9365>`_
        -- HTTPS man-in-the-middle attack against Python clients using default settings
      * :pep:`476` -- Enabling certificate verification by default for HTTPS
      * :pep:`493` -- HTTPS verification migration tools for Python 2.7


Random generation
^^^^^^^^^^^^^^^^^

.. function:: RAND_status()

   Return ``True`` if the SSL pseudo-random number generator has been seeded
   with 'enough' randomness, and ``False`` otherwise.  You can use
   :func:`ssl.RAND_egd` and :func:`ssl.RAND_add` to increase the randomness of
   the pseudo-random number generator.

.. function:: RAND_egd(path)

   If you are running an entropy-gathering daemon (EGD) somewhere, and *path*
   is the pathname of a socket connection open to it, this will read 256 bytes
   of randomness from the socket, and add it to the SSL pseudo-random number
   generator to increase the security of generated secret keys.  This is
   typically only necessary on systems without better sources of randomness.

   See http://egd.sourceforge.net/ or http://prngd.sourceforge.net/ for sources
   of entropy-gathering daemons.

   Availability: not available with LibreSSL.

.. function:: RAND_add(bytes, entropy)

   Mix the given *bytes* into the SSL pseudo-random number generator.  The
   parameter *entropy* (a float) is a lower bound on the entropy contained in
   string (so you can always use :const:`0.0`).  See :rfc:`1750` for more
   information on sources of entropy.

Certificate handling
^^^^^^^^^^^^^^^^^^^^

.. function:: match_hostname(cert, hostname)

   Verify that *cert* (in decoded format as returned by
   :meth:`SSLSocket.getpeercert`) matches the given *hostname*.  The rules
   applied are those for checking the identity of HTTPS servers as outlined
   in :rfc:`2818` and :rfc:`6125`, except that IP addresses are not currently
   supported. In addition to HTTPS, this function should be suitable for
   checking the identity of servers in various SSL-based protocols such as
   FTPS, IMAPS, POPS and others.

   :exc:`CertificateError` is raised on failure. On success, the function
   returns nothing::

      >>> cert = {'subject': ((('commonName', 'example.com'),),)}
      >>> ssl.match_hostname(cert, "example.com")
      >>> ssl.match_hostname(cert, "example.org")
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
        File "/home/py3k/Lib/ssl.py", line 130, in match_hostname
      ssl.CertificateError: hostname 'example.org' doesn't match 'example.com'

   .. versionadded:: 2.7.9


.. function:: cert_time_to_seconds(cert_time)

   Return the time in seconds since the Epoch, given the ``cert_time``
   string representing the "notBefore" or "notAfter" date from a
   certificate in ``"%b %d %H:%M:%S %Y %Z"`` strptime format (C
   locale).

   Here's an example:

   .. doctest:: newcontext

      >>> import ssl
      >>> timestamp = ssl.cert_time_to_seconds("Jan  5 09:34:43 2018 GMT")
      >>> timestamp
      1515144883
      >>> from datetime import datetime
      >>> print(datetime.utcfromtimestamp(timestamp))
      2018-01-05 09:34:43

   "notBefore" or "notAfter" dates must use GMT (:rfc:`5280`).

   .. versionchanged:: 2.7.9
      Interpret the input time as a time in UTC as specified by 'GMT'
      timezone in the input string. Local timezone was used
      previously. Return an integer (no fractions of a second in the
      input format)

.. function:: get_server_certificate(addr, ssl_version=PROTOCOL_SSLv23, ca_certs=None)

   Given the address ``addr`` of an SSL-protected server, as a (*hostname*,
   *port-number*) pair, fetches the server's certificate, and returns it as a
   PEM-encoded string.  If ``ssl_version`` is specified, uses that version of
   the SSL protocol to attempt to connect to the server.  If ``ca_certs`` is
   specified, it should be a file containing a list of root certificates, the
   same format as used for the same parameter in :func:`wrap_socket`.  The call
   will attempt to validate the server certificate against that set of root
   certificates, and will fail if the validation attempt fails.

   .. versionchanged:: 2.7.9

      This function is now IPv6-compatible, and the default *ssl_version* is
      changed from :data:`PROTOCOL_SSLv3` to :data:`PROTOCOL_SSLv23` for
      maximum compatibility with modern servers.

.. function:: DER_cert_to_PEM_cert(DER_cert_bytes)

   Given a certificate as a DER-encoded blob of bytes, returns a PEM-encoded
   string version of the same certificate.

.. function:: PEM_cert_to_DER_cert(PEM_cert_string)

   Given a certificate as an ASCII PEM string, returns a DER-encoded sequence of
   bytes for that same certificate.

.. function:: get_default_verify_paths()

   Returns a named tuple with paths to OpenSSL's default cafile and capath.
   The paths are the same as used by
   :meth:`SSLContext.set_default_verify_paths`. The return value is a
   :term:`named tuple` ``DefaultVerifyPaths``:

   * :attr:`cafile` - resolved path to cafile or None if the file doesn't exist,
   * :attr:`capath` - resolved path to capath or None if the directory doesn't exist,
   * :attr:`openssl_cafile_env` - OpenSSL's environment key that points to a cafile,
   * :attr:`openssl_cafile` - hard coded path to a cafile,
   * :attr:`openssl_capath_env` - OpenSSL's environment key that points to a capath,
   * :attr:`openssl_capath` - hard coded path to a capath directory

   .. versionadded:: 2.7.9

.. function:: enum_certificates(store_name)

   Retrieve certificates from Windows' system cert store. *store_name* may be
   one of ``CA``, ``ROOT`` or ``MY``. Windows may provide additional cert
   stores, too.

   The function returns a list of (cert_bytes, encoding_type, trust) tuples.
   The encoding_type specifies the encoding of cert_bytes. It is either
   :const:`x509_asn` for X.509 ASN.1 data or :const:`pkcs_7_asn` for
   PKCS#7 ASN.1 data. Trust specifies the purpose of the certificate as a set
   of OIDS or exactly ``True`` if the certificate is trustworthy for all
   purposes.

   Example::

      >>> ssl.enum_certificates("CA")
      [(b'data...', 'x509_asn', {'1.3.6.1.5.5.7.3.1', '1.3.6.1.5.5.7.3.2'}),
       (b'data...', 'x509_asn', True)]

   Availability: Windows.

   .. versionadded:: 2.7.9

.. function:: enum_crls(store_name)

   Retrieve CRLs from Windows' system cert store. *store_name* may be
   one of ``CA``, ``ROOT`` or ``MY``. Windows may provide additional cert
   stores, too.

   The function returns a list of (cert_bytes, encoding_type, trust) tuples.
   The encoding_type specifies the encoding of cert_bytes. It is either
   :const:`x509_asn` for X.509 ASN.1 data or :const:`pkcs_7_asn` for
   PKCS#7 ASN.1 data.

   Availability: Windows.

   .. versionadded:: 2.7.9


Constants
^^^^^^^^^

.. data:: CERT_NONE

   Possible value for :attr:`SSLContext.verify_mode`, or the ``cert_reqs``
   parameter to :func:`wrap_socket`.  In this mode (the default), no
   certificates will be required from the other side of the socket connection.
   If a certificate is received from the other end, no attempt to validate it
   is made.

   See the discussion of :ref:`ssl-security` below.

.. data:: CERT_OPTIONAL

   Possible value for :attr:`SSLContext.verify_mode`, or the ``cert_reqs``
   parameter to :func:`wrap_socket`.  In this mode no certificates will be
   required from the other side of the socket connection; but if they
   are provided, validation will be attempted and an :class:`SSLError`
   will be raised on failure.

   Use of this setting requires a valid set of CA certificates to
   be passed, either to :meth:`SSLContext.load_verify_locations` or as a
   value of the ``ca_certs`` parameter to :func:`wrap_socket`.

.. data:: CERT_REQUIRED

   Possible value for :attr:`SSLContext.verify_mode`, or the ``cert_reqs``
   parameter to :func:`wrap_socket`.  In this mode, certificates are
   required from the other side of the socket connection; an :class:`SSLError`
   will be raised if no certificate is provided, or if its validation fails.

   Use of this setting requires a valid set of CA certificates to
   be passed, either to :meth:`SSLContext.load_verify_locations` or as a
   value of the ``ca_certs`` parameter to :func:`wrap_socket`.

.. data:: VERIFY_DEFAULT

   Possible value for :attr:`SSLContext.verify_flags`. In this mode, certificate
   revocation lists (CRLs) are not checked. By default OpenSSL does neither
   require nor verify CRLs.

   .. versionadded:: 2.7.9

.. data:: VERIFY_CRL_CHECK_LEAF

   Possible value for :attr:`SSLContext.verify_flags`. In this mode, only the
   peer cert is check but non of the intermediate CA certificates. The mode
   requires a valid CRL that is signed by the peer cert's issuer (its direct
   ancestor CA). If no proper has been loaded
   :attr:`SSLContext.load_verify_locations`, validation will fail.

   .. versionadded:: 2.7.9

.. data:: VERIFY_CRL_CHECK_CHAIN

   Possible value for :attr:`SSLContext.verify_flags`. In this mode, CRLs of
   all certificates in the peer cert chain are checked.

   .. versionadded:: 2.7.9

.. data:: VERIFY_X509_STRICT

   Possible value for :attr:`SSLContext.verify_flags` to disable workarounds
   for broken X.509 certificates.

   .. versionadded:: 2.7.9

.. data:: VERIFY_X509_TRUSTED_FIRST

   Possible value for :attr:`SSLContext.verify_flags`. It instructs OpenSSL to
   prefer trusted certificates when building the trust chain to validate a
   certificate. This flag is enabled by default.

   .. versionadded:: 2.7.10

.. data:: PROTOCOL_SSLv23

   Selects the highest protocol version that both the client and server support.
   Despite the name, this option can select "TLS" protocols as well as "SSL".

.. data:: PROTOCOL_SSLv2

   Selects SSL version 2 as the channel encryption protocol.

   This protocol is not available if OpenSSL is compiled with the
   ``OPENSSL_NO_SSL2`` flag.

   .. warning::

      SSL version 2 is insecure.  Its use is highly discouraged.

.. data:: PROTOCOL_SSLv3

   Selects SSL version 3 as the channel encryption protocol.

   This protocol is not be available if OpenSSL is compiled with the
   ``OPENSSL_NO_SSLv3`` flag.

   .. warning::

      SSL version 3 is insecure.  Its use is highly discouraged.

.. data:: PROTOCOL_TLSv1

   Selects TLS version 1.0 as the channel encryption protocol.

.. data:: PROTOCOL_TLSv1_1

   Selects TLS version 1.1 as the channel encryption protocol.
   Available only with openssl version 1.0.1+.

   .. versionadded:: 2.7.9

.. data:: PROTOCOL_TLSv1_2

   Selects TLS version 1.2 as the channel encryption protocol. This is the
   most modern version, and probably the best choice for maximum protection,
   if both sides can speak it.  Available only with openssl version 1.0.1+.

   .. versionadded:: 2.7.9

.. data:: OP_ALL

   Enables workarounds for various bugs present in other SSL implementations.
   This option is set by default.  It does not necessarily set the same
   flags as OpenSSL's ``SSL_OP_ALL`` constant.

   .. versionadded:: 2.7.9

.. data:: OP_NO_SSLv2

   Prevents an SSLv2 connection.  This option is only applicable in
   conjunction with :const:`PROTOCOL_SSLv23`.  It prevents the peers from
   choosing SSLv2 as the protocol version.

   .. versionadded:: 2.7.9

.. data:: OP_NO_SSLv3

   Prevents an SSLv3 connection.  This option is only applicable in
   conjunction with :const:`PROTOCOL_SSLv23`.  It prevents the peers from
   choosing SSLv3 as the protocol version.

   .. versionadded:: 2.7.9

.. data:: OP_NO_TLSv1

   Prevents a TLSv1 connection.  This option is only applicable in
   conjunction with :const:`PROTOCOL_SSLv23`.  It prevents the peers from
   choosing TLSv1 as the protocol version.

   .. versionadded:: 2.7.9

.. data:: OP_NO_TLSv1_1

   Prevents a TLSv1.1 connection. This option is only applicable in conjunction
   with :const:`PROTOCOL_SSLv23`. It prevents the peers from choosing TLSv1.1 as
   the protocol version. Available only with openssl version 1.0.1+.

   .. versionadded:: 2.7.9

.. data:: OP_NO_TLSv1_2

   Prevents a TLSv1.2 connection. This option is only applicable in conjunction
   with :const:`PROTOCOL_SSLv23`. It prevents the peers from choosing TLSv1.2 as
   the protocol version. Available only with openssl version 1.0.1+.

   .. versionadded:: 2.7.9

.. data:: OP_CIPHER_SERVER_PREFERENCE

   Use the server's cipher ordering preference, rather than the client's.
   This option has no effect on client sockets and SSLv2 server sockets.

   .. versionadded:: 2.7.9

.. data:: OP_SINGLE_DH_USE

   Prevents re-use of the same DH key for distinct SSL sessions.  This
   improves forward secrecy but requires more computational resources.
   This option only applies to server sockets.

   .. versionadded:: 2.7.9

.. data:: OP_SINGLE_ECDH_USE

   Prevents re-use of the same ECDH key for distinct SSL sessions.  This
   improves forward secrecy but requires more computational resources.
   This option only applies to server sockets.

   .. versionadded:: 2.7.9

.. data:: OP_NO_COMPRESSION

   Disable compression on the SSL channel.  This is useful if the application
   protocol supports its own compression scheme.

   This option is only available with OpenSSL 1.0.0 and later.

   .. versionadded:: 2.7.9

.. data:: HAS_ALPN

   Whether the OpenSSL library has built-in support for the *Application-Layer
   Protocol Negotiation* TLS extension as described in :rfc:`7301`.

   .. versionadded:: 2.7.10

.. data:: HAS_ECDH

   Whether the OpenSSL library has built-in support for Elliptic Curve-based
   Diffie-Hellman key exchange.  This should be true unless the feature was
   explicitly disabled by the distributor.

   .. versionadded:: 2.7.9

.. data:: HAS_SNI

   Whether the OpenSSL library has built-in support for the *Server Name
   Indication* extension (as defined in :rfc:`4366`).

   .. versionadded:: 2.7.9

.. data:: HAS_NPN

   Whether the OpenSSL library has built-in support for *Next Protocol
   Negotiation* as described in the `NPN draft specification
   <https://tools.ietf.org/html/draft-agl-tls-nextprotoneg>`_. When true,
   you can use the :meth:`SSLContext.set_npn_protocols` method to advertise
   which protocols you want to support.

   .. versionadded:: 2.7.9

.. data:: CHANNEL_BINDING_TYPES

   List of supported TLS channel binding types.  Strings in this list
   can be used as arguments to :meth:`SSLSocket.get_channel_binding`.

   .. versionadded:: 2.7.9

.. data:: OPENSSL_VERSION

   The version string of the OpenSSL library loaded by the interpreter::

    >>> ssl.OPENSSL_VERSION
    'OpenSSL 0.9.8k 25 Mar 2009'

   .. versionadded:: 2.7

.. data:: OPENSSL_VERSION_INFO

   A tuple of five integers representing version information about the
   OpenSSL library::

    >>> ssl.OPENSSL_VERSION_INFO
    (0, 9, 8, 11, 15)

   .. versionadded:: 2.7

.. data:: OPENSSL_VERSION_NUMBER

   The raw version number of the OpenSSL library, as a single integer::

    >>> ssl.OPENSSL_VERSION_NUMBER
    9470143L
    >>> hex(ssl.OPENSSL_VERSION_NUMBER)
    '0x9080bfL'

   .. versionadded:: 2.7

.. data:: ALERT_DESCRIPTION_HANDSHAKE_FAILURE
          ALERT_DESCRIPTION_INTERNAL_ERROR
          ALERT_DESCRIPTION_*

   Alert Descriptions from :rfc:`5246` and others. The `IANA TLS Alert Registry
   <http://www.iana.org/assignments/tls-parameters/tls-parameters.xml#tls-parameters-6>`_
   contains this list and references to the RFCs where their meaning is defined.

   Used as the return value of the callback function in
   :meth:`SSLContext.set_servername_callback`.

   .. versionadded:: 2.7.9

.. data:: Purpose.SERVER_AUTH

   Option for :func:`create_default_context` and
   :meth:`SSLContext.load_default_certs`.  This value indicates that the
   context may be used to authenticate Web servers (therefore, it will
   be used to create client-side sockets).

   .. versionadded:: 2.7.9

.. data:: Purpose.CLIENT_AUTH

   Option for :func:`create_default_context` and
   :meth:`SSLContext.load_default_certs`.  This value indicates that the
   context may be used to authenticate Web clients (therefore, it will
   be used to create server-side sockets).

   .. versionadded:: 2.7.9


SSL Sockets
-----------

SSL sockets provide the following methods of :ref:`socket-objects`:

- :meth:`~socket.socket.accept()`
- :meth:`~socket.socket.bind()`
- :meth:`~socket.socket.close()`
- :meth:`~socket.socket.connect()`
- :meth:`~socket.socket.fileno()`
- :meth:`~socket.socket.getpeername()`, :meth:`~socket.socket.getsockname()`
- :meth:`~socket.socket.getsockopt()`, :meth:`~socket.socket.setsockopt()`
- :meth:`~socket.socket.gettimeout()`, :meth:`~socket.socket.settimeout()`,
  :meth:`~socket.socket.setblocking()`
- :meth:`~socket.socket.listen()`
- :meth:`~socket.socket.makefile()`
- :meth:`~socket.socket.recv()`, :meth:`~socket.socket.recv_into()`
  (but passing a non-zero ``flags`` argument is not allowed)
- :meth:`~socket.socket.send()`, :meth:`~socket.socket.sendall()` (with
  the same limitation)
- :meth:`~socket.socket.shutdown()`

However, since the SSL (and TLS) protocol has its own framing atop
of TCP, the SSL sockets abstraction can, in certain respects, diverge from
the specification of normal, OS-level sockets.  See especially the
:ref:`notes on non-blocking sockets <ssl-nonblocking>`.

SSL sockets also have the following additional methods and attributes:

.. method:: SSLSocket.do_handshake()

   Perform the SSL setup handshake.

   .. versionchanged:: 2.7.9

      The handshake method also performs :func:`match_hostname` when the
      :attr:`~SSLContext.check_hostname` attribute of the socket's
      :attr:`~SSLSocket.context` is true.

.. method:: SSLSocket.getpeercert(binary_form=False)

   If there is no certificate for the peer on the other end of the connection,
   return ``None``.  If the SSL handshake hasn't been done yet, raise
   :exc:`ValueError`.

   If the ``binary_form`` parameter is :const:`False`, and a certificate was
   received from the peer, this method returns a :class:`dict` instance.  If the
   certificate was not validated, the dict is empty.  If the certificate was
   validated, it returns a dict with several keys, amongst them ``subject``
   (the principal for which the certificate was issued) and ``issuer``
   (the principal issuing the certificate).  If a certificate contains an
   instance of the *Subject Alternative Name* extension (see :rfc:`3280`),
   there will also be a ``subjectAltName`` key in the dictionary.

   The ``subject`` and ``issuer`` fields are tuples containing the sequence
   of relative distinguished names (RDNs) given in the certificate's data
   structure for the respective fields, and each RDN is a sequence of
   name-value pairs.  Here is a real-world example::

      {'issuer': ((('countryName', 'IL'),),
                  (('organizationName', 'StartCom Ltd.'),),
                  (('organizationalUnitName',
                    'Secure Digital Certificate Signing'),),
                  (('commonName',
                    'StartCom Class 2 Primary Intermediate Server CA'),)),
       'notAfter': 'Nov 22 08:15:19 2013 GMT',
       'notBefore': 'Nov 21 03:09:52 2011 GMT',
       'serialNumber': '95F0',
       'subject': ((('description', '571208-SLe257oHY9fVQ07Z'),),
                   (('countryName', 'US'),),
                   (('stateOrProvinceName', 'California'),),
                   (('localityName', 'San Francisco'),),
                   (('organizationName', 'Electronic Frontier Foundation, Inc.'),),
                   (('commonName', '*.eff.org'),),
                   (('emailAddress', 'hostmaster@eff.org'),)),
       'subjectAltName': (('DNS', '*.eff.org'), ('DNS', 'eff.org')),
       'version': 3}

   .. note::

      To validate a certificate for a particular service, you can use the
      :func:`match_hostname` function.

   If the ``binary_form`` parameter is :const:`True`, and a certificate was
   provided, this method returns the DER-encoded form of the entire certificate
   as a sequence of bytes, or :const:`None` if the peer did not provide a
   certificate.  Whether the peer provides a certificate depends on the SSL
   socket's role:

   * for a client SSL socket, the server will always provide a certificate,
     regardless of whether validation was required;

   * for a server SSL socket, the client will only provide a certificate
     when requested by the server; therefore :meth:`getpeercert` will return
     :const:`None` if you used :const:`CERT_NONE` (rather than
     :const:`CERT_OPTIONAL` or :const:`CERT_REQUIRED`).

   .. versionchanged:: 2.7.9
      The returned dictionary includes additional items such as ``issuer`` and
      ``notBefore``. Additionall :exc:`ValueError` is raised when the handshake
      isn't done. The returned dictionary includes additional X509v3 extension
      items such as ``crlDistributionPoints``, ``caIssuers`` and ``OCSP`` URIs.

.. method:: SSLSocket.cipher()

   Returns a three-value tuple containing the name of the cipher being used, the
   version of the SSL protocol that defines its use, and the number of secret
   bits being used.  If no connection has been established, returns ``None``.

.. method:: SSLSocket.compression()

   Return the compression algorithm being used as a string, or ``None``
   if the connection isn't compressed.

   If the higher-level protocol supports its own compression mechanism,
   you can use :data:`OP_NO_COMPRESSION` to disable SSL-level compression.

   .. versionadded:: 2.7.9

.. method:: SSLSocket.get_channel_binding(cb_type="tls-unique")

   Get channel binding data for current connection, as a bytes object.  Returns
   ``None`` if not connected or the handshake has not been completed.

   The *cb_type* parameter allow selection of the desired channel binding
   type. Valid channel binding types are listed in the
   :data:`CHANNEL_BINDING_TYPES` list.  Currently only the 'tls-unique' channel
   binding, defined by :rfc:`5929`, is supported.  :exc:`ValueError` will be
   raised if an unsupported channel binding type is requested.

   .. versionadded:: 2.7.9

.. method:: SSLSocket.selected_alpn_protocol()

   Return the protocol that was selected during the TLS handshake.  If
   :meth:`SSLContext.set_alpn_protocols` was not called, if the other party does
   not support ALPN, if this socket does not support any of the client's
   proposed protocols, or if the handshake has not happened yet, ``None`` is
   returned.

   .. versionadded:: 2.7.10

.. method:: SSLSocket.selected_npn_protocol()

   Return the higher-level protocol that was selected during the TLS/SSL
   handshake. If :meth:`SSLContext.set_npn_protocols` was not called, or
   if the other party does not support NPN, or if the handshake has not yet
   happened, this will return ``None``.

   .. versionadded:: 2.7.9

.. method:: SSLSocket.unwrap()

   Performs the SSL shutdown handshake, which removes the TLS layer from the
   underlying socket, and returns the underlying socket object.  This can be
   used to go from encrypted operation over a connection to unencrypted.  The
   returned socket should always be used for further communication with the
   other side of the connection, rather than the original socket.

.. method:: SSLSocket.version()

   Return the actual SSL protocol version negotiated by the connection
   as a string, or ``None`` is no secure connection is established.
   As of this writing, possible return values include ``"SSLv2"``,
   ``"SSLv3"``, ``"TLSv1"``, ``"TLSv1.1"`` and ``"TLSv1.2"``.
   Recent OpenSSL versions may define more return values.

   .. versionadded:: 2.7.9

.. attribute:: SSLSocket.context

   The :class:`SSLContext` object this SSL socket is tied to.  If the SSL
   socket was created using the top-level :func:`wrap_socket` function
   (rather than :meth:`SSLContext.wrap_socket`), this is a custom context
   object created for this SSL socket.

   .. versionadded:: 2.7.9


SSL Contexts
------------

.. versionadded:: 2.7.9

An SSL context holds various data longer-lived than single SSL connections,
such as SSL configuration options, certificate(s) and private key(s).
It also manages a cache of SSL sessions for server-side sockets, in order
to speed up repeated connections from the same clients.

.. class:: SSLContext(protocol)

   Create a new SSL context.  You must pass *protocol* which must be one
   of the ``PROTOCOL_*`` constants defined in this module.
   :data:`PROTOCOL_SSLv23` is currently recommended for maximum
   interoperability.

   .. seealso::
      :func:`create_default_context` lets the :mod:`ssl` module choose
      security settings for a given purpose.


:class:`SSLContext` objects have the following methods and attributes:

.. method:: SSLContext.cert_store_stats()

   Get statistics about quantities of loaded X.509 certificates, count of
   X.509 certificates flagged as CA certificates and certificate revocation
   lists as dictionary.

   Example for a context with one CA cert and one other cert::

      >>> context.cert_store_stats()
      {'crl': 0, 'x509_ca': 1, 'x509': 2}


.. method:: SSLContext.load_cert_chain(certfile, keyfile=None, password=None)

   Load a private key and the corresponding certificate.  The *certfile*
   string must be the path to a single file in PEM format containing the
   certificate as well as any number of CA certificates needed to establish
   the certificate's authenticity.  The *keyfile* string, if present, must
   point to a file containing the private key in.  Otherwise the private
   key will be taken from *certfile* as well.  See the discussion of
   :ref:`ssl-certificates` for more information on how the certificate
   is stored in the *certfile*.

   The *password* argument may be a function to call to get the password for
   decrypting the private key.  It will only be called if the private key is
   encrypted and a password is necessary.  It will be called with no arguments,
   and it should return a string, bytes, or bytearray.  If the return value is
   a string it will be encoded as UTF-8 before using it to decrypt the key.
   Alternatively a string, bytes, or bytearray value may be supplied directly
   as the *password* argument.  It will be ignored if the private key is not
   encrypted and no password is needed.

   If the *password* argument is not specified and a password is required,
   OpenSSL's built-in password prompting mechanism will be used to
   interactively prompt the user for a password.

   An :class:`SSLError` is raised if the private key doesn't
   match with the certificate.

.. method:: SSLContext.load_default_certs(purpose=Purpose.SERVER_AUTH)

   Load a set of default "certification authority" (CA) certificates from
   default locations. On Windows it loads CA certs from the ``CA`` and
   ``ROOT`` system stores. On other systems it calls
   :meth:`SSLContext.set_default_verify_paths`. In the future the method may
   load CA certificates from other locations, too.

   The *purpose* flag specifies what kind of CA certificates are loaded. The
   default settings :data:`Purpose.SERVER_AUTH` loads certificates, that are
   flagged and trusted for TLS web server authentication (client side
   sockets). :data:`Purpose.CLIENT_AUTH` loads CA certificates for client
   certificate verification on the server side.

.. method:: SSLContext.load_verify_locations(cafile=None, capath=None, cadata=None)

   Load a set of "certification authority" (CA) certificates used to validate
   other peers' certificates when :data:`verify_mode` is other than
   :data:`CERT_NONE`.  At least one of *cafile* or *capath* must be specified.

   This method can also load certification revocation lists (CRLs) in PEM or
   DER format. In order to make use of CRLs, :attr:`SSLContext.verify_flags`
   must be configured properly.

   The *cafile* string, if present, is the path to a file of concatenated
   CA certificates in PEM format. See the discussion of
   :ref:`ssl-certificates` for more information about how to arrange the
   certificates in this file.

   The *capath* string, if present, is
   the path to a directory containing several CA certificates in PEM format,
   following an `OpenSSL specific layout
   <http://www.openssl.org/docs/ssl/SSL_CTX_load_verify_locations.html>`_.

   The *cadata* object, if present, is either an ASCII string of one or more
   PEM-encoded certificates or a bytes-like object of DER-encoded
   certificates. Like with *capath* extra lines around PEM-encoded
   certificates are ignored but at least one certificate must be present.

.. method:: SSLContext.get_ca_certs(binary_form=False)

   Get a list of loaded "certification authority" (CA) certificates. If the
   ``binary_form`` parameter is :const:`False` each list
   entry is a dict like the output of :meth:`SSLSocket.getpeercert`. Otherwise
   the method returns a list of DER-encoded certificates. The returned list
   does not contain certificates from *capath* unless a certificate was
   requested and loaded by a SSL connection.

.. method:: SSLContext.set_default_verify_paths()

   Load a set of default "certification authority" (CA) certificates from
   a filesystem path defined when building the OpenSSL library.  Unfortunately,
   there's no easy way to know whether this method succeeds: no error is
   returned if no certificates are to be found.  When the OpenSSL library is
   provided as part of the operating system, though, it is likely to be
   configured properly.

.. method:: SSLContext.set_ciphers(ciphers)

   Set the available ciphers for sockets created with this context.
   It should be a string in the `OpenSSL cipher list format
   <http://www.openssl.org/docs/apps/ciphers.html#CIPHER-LIST-FORMAT>`_.
   If no cipher can be selected (because compile-time options or other
   configuration forbids use of all the specified ciphers), an
   :class:`SSLError` will be raised.

   .. note::
      when connected, the :meth:`SSLSocket.cipher` method of SSL sockets will
      give the currently selected cipher.

.. method:: SSLContext.set_alpn_protocols(protocols)

   Specify which protocols the socket should advertise during the SSL/TLS
   handshake. It should be a list of ASCII strings, like ``['http/1.1',
   'spdy/2']``, ordered by preference. The selection of a protocol will happen
   during the handshake, and will play out according to :rfc:`7301`. After a
   successful handshake, the :meth:`SSLSocket.selected_alpn_protocol` method will
   return the agreed-upon protocol.

   This method will raise :exc:`NotImplementedError` if :data:`HAS_ALPN` is
   False.

   .. versionadded:: 2.7.10

.. method:: SSLContext.set_npn_protocols(protocols)

   Specify which protocols the socket should advertise during the SSL/TLS
   handshake. It should be a list of strings, like ``['http/1.1', 'spdy/2']``,
   ordered by preference. The selection of a protocol will happen during the
   handshake, and will play out according to the `NPN draft specification
   <https://tools.ietf.org/html/draft-agl-tls-nextprotoneg>`_. After a
   successful handshake, the :meth:`SSLSocket.selected_npn_protocol` method will
   return the agreed-upon protocol.

   This method will raise :exc:`NotImplementedError` if :data:`HAS_NPN` is
   False.

.. method:: SSLContext.set_servername_callback(server_name_callback)

   Register a callback function that will be called after the TLS Client Hello
   handshake message has been received by the SSL/TLS server when the TLS client
   specifies a server name indication. The server name indication mechanism
   is specified in :rfc:`6066` section 3 - Server Name Indication.

   Only one callback can be set per ``SSLContext``.  If *server_name_callback*
   is ``None`` then the callback is disabled. Calling this function a
   subsequent time will disable the previously registered callback.

   The callback function, *server_name_callback*, will be called with three
   arguments; the first being the :class:`ssl.SSLSocket`, the second is a string
   that represents the server name that the client is intending to communicate
   (or :const:`None` if the TLS Client Hello does not contain a server name)
   and the third argument is the original :class:`SSLContext`. The server name
   argument is the IDNA decoded server name.

   A typical use of this callback is to change the :class:`ssl.SSLSocket`'s
   :attr:`SSLSocket.context` attribute to a new object of type
   :class:`SSLContext` representing a certificate chain that matches the server
   name.

   Due to the early negotiation phase of the TLS connection, only limited
   methods and attributes are usable like
   :meth:`SSLSocket.selected_alpn_protocol` and :attr:`SSLSocket.context`.
   :meth:`SSLSocket.getpeercert`, :meth:`SSLSocket.getpeercert`,
   :meth:`SSLSocket.cipher` and :meth:`SSLSocket.compress` methods require that
   the TLS connection has progressed beyond the TLS Client Hello and therefore
   will not contain return meaningful values nor can they be called safely.

   The *server_name_callback* function must return ``None`` to allow the
   TLS negotiation to continue.  If a TLS failure is required, a constant
   :const:`ALERT_DESCRIPTION_* <ALERT_DESCRIPTION_INTERNAL_ERROR>` can be
   returned.  Other return values will result in a TLS fatal error with
   :const:`ALERT_DESCRIPTION_INTERNAL_ERROR`.

   If there is an IDNA decoding error on the server name, the TLS connection
   will terminate with an :const:`ALERT_DESCRIPTION_INTERNAL_ERROR` fatal TLS
   alert message to the client.

   If an exception is raised from the *server_name_callback* function the TLS
   connection will terminate with a fatal TLS alert message
   :const:`ALERT_DESCRIPTION_HANDSHAKE_FAILURE`.

   This method will raise :exc:`NotImplementedError` if the OpenSSL library
   had OPENSSL_NO_TLSEXT defined when it was built.

.. method:: SSLContext.load_dh_params(dhfile)

   Load the key generation parameters for Diffie-Helman (DH) key exchange.
   Using DH key exchange improves forward secrecy at the expense of
   computational resources (both on the server and on the client).
   The *dhfile* parameter should be the path to a file containing DH
   parameters in PEM format.

   This setting doesn't apply to client sockets.  You can also use the
   :data:`OP_SINGLE_DH_USE` option to further improve security.

.. method:: SSLContext.set_ecdh_curve(curve_name)

   Set the curve name for Elliptic Curve-based Diffie-Hellman (ECDH) key
   exchange.  ECDH is significantly faster than regular DH while arguably
   as secure.  The *curve_name* parameter should be a string describing
   a well-known elliptic curve, for example ``prime256v1`` for a widely
   supported curve.

   This setting doesn't apply to client sockets.  You can also use the
   :data:`OP_SINGLE_ECDH_USE` option to further improve security.

   This method is not available if :data:`HAS_ECDH` is False.

   .. seealso::
      `SSL/TLS & Perfect Forward Secrecy <http://vincent.bernat.im/en/blog/2011-ssl-perfect-forward-secrecy.html>`_
         Vincent Bernat.

.. method:: SSLContext.wrap_socket(sock, server_side=False, \
      do_handshake_on_connect=True, suppress_ragged_eofs=True, \
      server_hostname=None)

   Wrap an existing Python socket *sock* and return an :class:`SSLSocket`
   object.  *sock* must be a :data:`~socket.SOCK_STREAM` socket; other socket
   types are unsupported.

   The returned SSL socket is tied to the context, its settings and
   certificates.  The parameters *server_side*, *do_handshake_on_connect*
   and *suppress_ragged_eofs* have the same meaning as in the top-level
   :func:`wrap_socket` function.

   On client connections, the optional parameter *server_hostname* specifies
   the hostname of the service which we are connecting to.  This allows a
   single server to host multiple SSL-based services with distinct certificates,
   quite similarly to HTTP virtual hosts. Specifying *server_hostname* will
   raise a :exc:`ValueError` if *server_side* is true.

   .. versionchanged:: 2.7.9
      Always allow a server_hostname to be passed, even if OpenSSL does not
      have SNI.

.. method:: SSLContext.session_stats()

   Get statistics about the SSL sessions created or managed by this context.
   A dictionary is returned which maps the names of each `piece of information
   <http://www.openssl.org/docs/ssl/SSL_CTX_sess_number.html>`_ to their
   numeric values.  For example, here is the total number of hits and misses
   in the session cache since the context was created::

      >>> stats = context.session_stats()
      >>> stats['hits'], stats['misses']
      (0, 0)

.. method:: SSLContext.get_ca_certs(binary_form=False)

   Returns a list of dicts with information of loaded CA certs. If the
   optional argument is true, returns a DER-encoded copy of the CA
   certificate.

   .. note::
      Certificates in a capath directory aren't loaded unless they have
      been used at least once.

.. attribute:: SSLContext.check_hostname

   Wether to match the peer cert's hostname with :func:`match_hostname` in
   :meth:`SSLSocket.do_handshake`. The context's
   :attr:`~SSLContext.verify_mode` must be set to :data:`CERT_OPTIONAL` or
   :data:`CERT_REQUIRED`, and you must pass *server_hostname* to
   :meth:`~SSLContext.wrap_socket` in order to match the hostname.

   Example::

      import socket, ssl

      context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
      context.verify_mode = ssl.CERT_REQUIRED
      context.check_hostname = True
      context.load_default_certs()

      s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      ssl_sock = context.wrap_socket(s, server_hostname='www.verisign.com')
      ssl_sock.connect(('www.verisign.com', 443))

   .. note::

     This features requires OpenSSL 0.9.8f or newer.

.. attribute:: SSLContext.options

   An integer representing the set of SSL options enabled on this context.
   The default value is :data:`OP_ALL`, but you can specify other options
   such as :data:`OP_NO_SSLv2` by ORing them together.

   .. note::
      With versions of OpenSSL older than 0.9.8m, it is only possible
      to set options, not to clear them.  Attempting to clear an option
      (by resetting the corresponding bits) will raise a ``ValueError``.

.. attribute:: SSLContext.protocol

   The protocol version chosen when constructing the context.  This attribute
   is read-only.

.. attribute:: SSLContext.verify_flags

   The flags for certificate verification operations. You can set flags like
   :data:`VERIFY_CRL_CHECK_LEAF` by ORing them together. By default OpenSSL
   does neither require nor verify certificate revocation lists (CRLs).
   Available only with openssl version 0.9.8+.

.. attribute:: SSLContext.verify_mode

   Whether to try to verify other peers' certificates and how to behave
   if verification fails.  This attribute must be one of
   :data:`CERT_NONE`, :data:`CERT_OPTIONAL` or :data:`CERT_REQUIRED`.


.. index:: single: certificates

.. index:: single: X509 certificate

.. _ssl-certificates:

Certificates
------------

Certificates in general are part of a public-key / private-key system.  In this
system, each *principal*, (which may be a machine, or a person, or an
organization) is assigned a unique two-part encryption key.  One part of the key
is public, and is called the *public key*; the other part is kept secret, and is
called the *private key*.  The two parts are related, in that if you encrypt a
message with one of the parts, you can decrypt it with the other part, and
**only** with the other part.

A certificate contains information about two principals.  It contains the name
of a *subject*, and the subject's public key.  It also contains a statement by a
second principal, the *issuer*, that the subject is who he claims to be, and
that this is indeed the subject's public key.  The issuer's statement is signed
with the issuer's private key, which only the issuer knows.  However, anyone can
verify the issuer's statement by finding the issuer's public key, decrypting the
statement with it, and comparing it to the other information in the certificate.
The certificate also contains information about the time period over which it is
valid.  This is expressed as two fields, called "notBefore" and "notAfter".

In the Python use of certificates, a client or server can use a certificate to
prove who they are.  The other side of a network connection can also be required
to produce a certificate, and that certificate can be validated to the
satisfaction of the client or server that requires such validation.  The
connection attempt can be set to raise an exception if the validation fails.
Validation is done automatically, by the underlying OpenSSL framework; the
application need not concern itself with its mechanics.  But the application
does usually need to provide sets of certificates to allow this process to take
place.

Python uses files to contain certificates.  They should be formatted as "PEM"
(see :rfc:`1422`), which is a base-64 encoded form wrapped with a header line
and a footer line::

      -----BEGIN CERTIFICATE-----
      ... (certificate in base64 PEM encoding) ...
      -----END CERTIFICATE-----

Certificate chains
^^^^^^^^^^^^^^^^^^

The Python files which contain certificates can contain a sequence of
certificates, sometimes called a *certificate chain*.  This chain should start
with the specific certificate for the principal who "is" the client or server,
and then the certificate for the issuer of that certificate, and then the
certificate for the issuer of *that* certificate, and so on up the chain till
you get to a certificate which is *self-signed*, that is, a certificate which
has the same subject and issuer, sometimes called a *root certificate*.  The
certificates should just be concatenated together in the certificate file.  For
example, suppose we had a three certificate chain, from our server certificate
to the certificate of the certification authority that signed our server
certificate, to the root certificate of the agency which issued the
certification authority's certificate::

      -----BEGIN CERTIFICATE-----
      ... (certificate for your server)...
      -----END CERTIFICATE-----
      -----BEGIN CERTIFICATE-----
      ... (the certificate for the CA)...
      -----END CERTIFICATE-----
      -----BEGIN CERTIFICATE-----
      ... (the root certificate for the CA's issuer)...
      -----END CERTIFICATE-----

CA certificates
^^^^^^^^^^^^^^^

If you are going to require validation of the other side of the connection's
certificate, you need to provide a "CA certs" file, filled with the certificate
chains for each issuer you are willing to trust.  Again, this file just contains
these chains concatenated together.  For validation, Python will use the first
chain it finds in the file which matches.  The platform's certificates file can
be used by calling :meth:`SSLContext.load_default_certs`, this is done
automatically with :func:`.create_default_context`.

Combined key and certificate
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Often the private key is stored in the same file as the certificate; in this
case, only the ``certfile`` parameter to :meth:`SSLContext.load_cert_chain`
and :func:`wrap_socket` needs to be passed.  If the private key is stored
with the certificate, it should come before the first certificate in
the certificate chain::

   -----BEGIN RSA PRIVATE KEY-----
   ... (private key in base64 encoding) ...
   -----END RSA PRIVATE KEY-----
   -----BEGIN CERTIFICATE-----
   ... (certificate in base64 PEM encoding) ...
   -----END CERTIFICATE-----

Self-signed certificates
^^^^^^^^^^^^^^^^^^^^^^^^

If you are going to create a server that provides SSL-encrypted connection
services, you will need to acquire a certificate for that service.  There are
many ways of acquiring appropriate certificates, such as buying one from a
certification authority.  Another common practice is to generate a self-signed
certificate.  The simplest way to do this is with the OpenSSL package, using
something like the following::

  % openssl req -new -x509 -days 365 -nodes -out cert.pem -keyout cert.pem
  Generating a 1024 bit RSA private key
  .......++++++
  .............................++++++
  writing new private key to 'cert.pem'
  -----
  You are about to be asked to enter information that will be incorporated
  into your certificate request.
  What you are about to enter is what is called a Distinguished Name or a DN.
  There are quite a few fields but you can leave some blank
  For some fields there will be a default value,
  If you enter '.', the field will be left blank.
  -----
  Country Name (2 letter code) [AU]:US
  State or Province Name (full name) [Some-State]:MyState
  Locality Name (eg, city) []:Some City
  Organization Name (eg, company) [Internet Widgits Pty Ltd]:My Organization, Inc.
  Organizational Unit Name (eg, section) []:My Group
  Common Name (eg, YOUR name) []:myserver.mygroup.myorganization.com
  Email Address []:ops@myserver.mygroup.myorganization.com
  %

The disadvantage of a self-signed certificate is that it is its own root
certificate, and no one else will have it in their cache of known (and trusted)
root certificates.


Examples
--------

Testing for SSL support
^^^^^^^^^^^^^^^^^^^^^^^

To test for the presence of SSL support in a Python installation, user code
should use the following idiom::

   try:
       import ssl
   except ImportError:
       pass
   else:
       ... # do something that requires SSL support

Client-side operation
^^^^^^^^^^^^^^^^^^^^^

This example creates a SSL context with the recommended security settings
for client sockets, including automatic certificate verification::

   >>> context = ssl.create_default_context()

If you prefer to tune security settings yourself, you might create
a context from scratch (but beware that you might not get the settings
right)::

   >>> context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
   >>> context.verify_mode = ssl.CERT_REQUIRED
   >>> context.check_hostname = True
   >>> context.load_verify_locations("/etc/ssl/certs/ca-bundle.crt")

(this snippet assumes your operating system places a bundle of all CA
certificates in ``/etc/ssl/certs/ca-bundle.crt``; if not, you'll get an
error and have to adjust the location)

When you use the context to connect to a server, :const:`CERT_REQUIRED`
validates the server certificate: it ensures that the server certificate
was signed with one of the CA certificates, and checks the signature for
correctness::

   >>> conn = context.wrap_socket(socket.socket(socket.AF_INET),
   ...                            server_hostname="www.python.org")
   >>> conn.connect(("www.python.org", 443))

You may then fetch the certificate::

   >>> cert = conn.getpeercert()

Visual inspection shows that the certificate does identify the desired service
(that is, the HTTPS host ``www.python.org``)::

   >>> pprint.pprint(cert)
   {'OCSP': ('http://ocsp.digicert.com',),
    'caIssuers': ('http://cacerts.digicert.com/DigiCertSHA2ExtendedValidationServerCA.crt',),
    'crlDistributionPoints': ('http://crl3.digicert.com/sha2-ev-server-g1.crl',
                              'http://crl4.digicert.com/sha2-ev-server-g1.crl'),
    'issuer': ((('countryName', 'US'),),
               (('organizationName', 'DigiCert Inc'),),
               (('organizationalUnitName', 'www.digicert.com'),),
               (('commonName', 'DigiCert SHA2 Extended Validation Server CA'),)),
    'notAfter': 'Sep  9 12:00:00 2016 GMT',
    'notBefore': 'Sep  5 00:00:00 2014 GMT',
    'serialNumber': '01BB6F00122B177F36CAB49CEA8B6B26',
    'subject': ((('businessCategory', 'Private Organization'),),
                (('1.3.6.1.4.1.311.60.2.1.3', 'US'),),
                (('1.3.6.1.4.1.311.60.2.1.2', 'Delaware'),),
                (('serialNumber', '3359300'),),
                (('streetAddress', '16 Allen Rd'),),
                (('postalCode', '03894-4801'),),
                (('countryName', 'US'),),
                (('stateOrProvinceName', 'NH'),),
                (('localityName', 'Wolfeboro,'),),
                (('organizationName', 'Python Software Foundation'),),
                (('commonName', 'www.python.org'),)),
    'subjectAltName': (('DNS', 'www.python.org'),
                       ('DNS', 'python.org'),
                       ('DNS', 'pypi.python.org'),
                       ('DNS', 'docs.python.org'),
                       ('DNS', 'testpypi.python.org'),
                       ('DNS', 'bugs.python.org'),
                       ('DNS', 'wiki.python.org'),
                       ('DNS', 'hg.python.org'),
                       ('DNS', 'mail.python.org'),
                       ('DNS', 'packaging.python.org'),
                       ('DNS', 'pythonhosted.org'),
                       ('DNS', 'www.pythonhosted.org'),
                       ('DNS', 'test.pythonhosted.org'),
                       ('DNS', 'us.pycon.org'),
                       ('DNS', 'id.python.org')),
    'version': 3}

Now the SSL channel is established and the certificate verified, you can
proceed to talk with the server::

   >>> conn.sendall(b"HEAD / HTTP/1.0\r\nHost: linuxfr.org\r\n\r\n")
   >>> pprint.pprint(conn.recv(1024).split(b"\r\n"))
   [b'HTTP/1.1 200 OK',
    b'Date: Sat, 18 Oct 2014 18:27:20 GMT',
    b'Server: nginx',
    b'Content-Type: text/html; charset=utf-8',
    b'X-Frame-Options: SAMEORIGIN',
    b'Content-Length: 45679',
    b'Accept-Ranges: bytes',
    b'Via: 1.1 varnish',
    b'Age: 2188',
    b'X-Served-By: cache-lcy1134-LCY',
    b'X-Cache: HIT',
    b'X-Cache-Hits: 11',
    b'Vary: Cookie',
    b'Strict-Transport-Security: max-age=63072000; includeSubDomains',
    b'Connection: close',
    b'',
    b'']

See the discussion of :ref:`ssl-security` below.


Server-side operation
^^^^^^^^^^^^^^^^^^^^^

For server operation, typically you'll need to have a server certificate, and
private key, each in a file.  You'll first create a context holding the key
and the certificate, so that clients can check your authenticity.  Then
you'll open a socket, bind it to a port, call :meth:`listen` on it, and start
waiting for clients to connect::

   import socket, ssl

   context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
   context.load_cert_chain(certfile="mycertfile", keyfile="mykeyfile")

   bindsocket = socket.socket()
   bindsocket.bind(('myaddr.mydomain.com', 10023))
   bindsocket.listen(5)

When a client connects, you'll call :meth:`accept` on the socket to get the
new socket from the other end, and use the context's :meth:`SSLContext.wrap_socket`
method to create a server-side SSL socket for the connection::

   while True:
       newsocket, fromaddr = bindsocket.accept()
       connstream = context.wrap_socket(newsocket, server_side=True)
       try:
           deal_with_client(connstream)
       finally:
           connstream.shutdown(socket.SHUT_RDWR)
           connstream.close()

Then you'll read data from the ``connstream`` and do something with it till you
are finished with the client (or the client is finished with you)::

   def deal_with_client(connstream):
       data = connstream.read()
       # null data means the client is finished with us
       while data:
           if not do_something(connstream, data):
               # we'll assume do_something returns False
               # when we're finished with client
               break
           data = connstream.read()
       # finished with client

And go back to listening for new client connections (of course, a real server
would probably handle each client connection in a separate thread, or put
the sockets in non-blocking mode and use an event loop).


.. _ssl-nonblocking:

Notes on non-blocking sockets
-----------------------------

When working with non-blocking sockets, there are several things you need
to be aware of:

- Calling :func:`~select.select` tells you that the OS-level socket can be
  read from (or written to), but it does not imply that there is sufficient
  data at the upper SSL layer.  For example, only part of an SSL frame might
  have arrived.  Therefore, you must be ready to handle :meth:`SSLSocket.recv`
  and :meth:`SSLSocket.send` failures, and retry after another call to
  :func:`~select.select`.

- Conversely, since the SSL layer has its own framing, a SSL socket may
  still have data available for reading without :func:`~select.select`
  being aware of it.  Therefore, you should first call
  :meth:`SSLSocket.recv` to drain any potentially available data, and then
  only block on a :func:`~select.select` call if still necessary.

  (of course, similar provisions apply when using other primitives such as
  :func:`~select.poll`, or those in the :mod:`selectors` module)

- The SSL handshake itself will be non-blocking: the
  :meth:`SSLSocket.do_handshake` method has to be retried until it returns
  successfully.  Here is a synopsis using :func:`~select.select` to wait for
  the socket's readiness::

    while True:
        try:
            sock.do_handshake()
            break
        except ssl.SSLWantReadError:
            select.select([sock], [], [])
        except ssl.SSLWantWriteError:
            select.select([], [sock], [])


.. _ssl-security:

Security considerations
-----------------------

Best defaults
^^^^^^^^^^^^^

For **client use**, if you don't have any special requirements for your
security policy, it is highly recommended that you use the
:func:`create_default_context` function to create your SSL context.
It will load the system's trusted CA certificates, enable certificate
validation and hostname checking, and try to choose reasonably secure
protocol and cipher settings.

If a client certificate is needed for the connection, it can be added with
:meth:`SSLContext.load_cert_chain`.

By contrast, if you create the SSL context by calling the :class:`SSLContext`
constructor yourself, it will not have certificate validation nor hostname
checking enabled by default.  If you do so, please read the paragraphs below
to achieve a good security level.

Manual settings
^^^^^^^^^^^^^^^

Verifying certificates
''''''''''''''''''''''

When calling the :class:`SSLContext` constructor directly,
:const:`CERT_NONE` is the default.  Since it does not authenticate the other
peer, it can be insecure, especially in client mode where most of time you
would like to ensure the authenticity of the server you're talking to.
Therefore, when in client mode, it is highly recommended to use
:const:`CERT_REQUIRED`.  However, it is in itself not sufficient; you also
have to check that the server certificate, which can be obtained by calling
:meth:`SSLSocket.getpeercert`, matches the desired service.  For many
protocols and applications, the service can be identified by the hostname;
in this case, the :func:`match_hostname` function can be used.  This common
check is automatically performed when :attr:`SSLContext.check_hostname` is
enabled.

In server mode, if you want to authenticate your clients using the SSL layer
(rather than using a higher-level authentication mechanism), you'll also have
to specify :const:`CERT_REQUIRED` and similarly check the client certificate.

   .. note::

      In client mode, :const:`CERT_OPTIONAL` and :const:`CERT_REQUIRED` are
      equivalent unless anonymous ciphers are enabled (they are disabled
      by default).

Protocol versions
'''''''''''''''''

SSL versions 2 and 3 are considered insecure and are therefore dangerous to
use.  If you want maximum compatibility between clients and servers, it is
recommended to use :const:`PROTOCOL_SSLv23` as the protocol version and then
disable SSLv2 and SSLv3 explicitly using the :data:`SSLContext.options`
attribute::

   context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
   context.options |= ssl.OP_NO_SSLv2
   context.options |= ssl.OP_NO_SSLv3

The SSL context created above will only allow TLSv1 and later (if
supported by your system) connections.

Cipher selection
''''''''''''''''

If you have advanced security requirements, fine-tuning of the ciphers
enabled when negotiating a SSL session is possible through the
:meth:`SSLContext.set_ciphers` method.  Starting from Python 2.7.9, the
ssl module disables certain weak ciphers by default, but you may want
to further restrict the cipher choice. Be sure to read OpenSSL's documentation
about the `cipher list format <http://www.openssl.org/docs/apps/ciphers.html#CIPHER-LIST-FORMAT>`_.
If you want to check which ciphers are enabled by a given cipher list, use the
``openssl ciphers`` command on your system.

Multi-processing
^^^^^^^^^^^^^^^^

If using this module as part of a multi-processed application (using,
for example the :mod:`multiprocessing` or :mod:`concurrent.futures` modules),
be aware that OpenSSL's internal random number generator does not properly
handle forked processes.  Applications must change the PRNG state of the
parent process if they use any SSL feature with :func:`os.fork`.  Any
successful call of :func:`~ssl.RAND_add`, :func:`~ssl.RAND_bytes` or
:func:`~ssl.RAND_pseudo_bytes` is sufficient.


.. seealso::

   Class :class:`socket.socket`
       Documentation of underlying :mod:`socket` class

   `SSL/TLS Strong Encryption: An Introduction <https://httpd.apache.org/docs/trunk/en/ssl/ssl_intro.html>`_
       Intro from the Apache webserver documentation

   `RFC 1422: Privacy Enhancement for Internet Electronic Mail: Part II: Certificate-Based Key Management <https://www.ietf.org/rfc/rfc1422>`_
       Steve Kent

   `RFC 1750: Randomness Recommendations for Security <https://www.ietf.org/rfc/rfc1750>`_
       D. Eastlake et. al.

   `RFC 3280: Internet X.509 Public Key Infrastructure Certificate and CRL Profile <https://www.ietf.org/rfc/rfc3280>`_
       Housley et. al.

   `RFC 4366: Transport Layer Security (TLS) Extensions <https://www.ietf.org/rfc/rfc4366>`_
       Blake-Wilson et. al.

   `RFC 5246: The Transport Layer Security (TLS) Protocol Version 1.2 <https://tools.ietf.org/html/rfc5246>`_
       T. Dierks et. al.

   `RFC 6066: Transport Layer Security (TLS) Extensions <https://tools.ietf.org/html/rfc6066>`_
       D. Eastlake

   `IANA TLS: Transport Layer Security (TLS) Parameters <http://www.iana.org/assignments/tls-parameters/tls-parameters.xml>`_
       IANA
