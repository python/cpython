:mod:`ssl` --- SSL wrapper for socket objects
=============================================

.. module:: ssl
   :synopsis: SSL wrapper for socket objects

.. moduleauthor:: Bill Janssen <bill.janssen@gmail.com>
.. sectionauthor::  Bill Janssen <bill.janssen@gmail.com>


.. index:: single: OpenSSL; (use in module ssl)

.. index:: TLS, SSL, Transport Layer Security, Secure Sockets Layer

This module provides access to Transport Layer Security (often known as "Secure
Sockets Layer") encryption and peer authentication facilities for network
sockets, both client-side and server-side.  This module uses the OpenSSL
library. It is available on all modern Unix systems, Windows, Mac OS X, and
probably additional platforms, as long as OpenSSL is installed on that platform.

.. note::

   Some behavior may be platform dependent, since calls are made to the
   operating system socket APIs.  The installed version of OpenSSL may also
   cause variations in behavior.

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

   Raised to signal an error from the underlying SSL implementation
   (currently provided by the OpenSSL library).  This signifies some
   problem in the higher-level encryption and authentication layer that's
   superimposed on the underlying network connection.  This error
   is a subtype of :exc:`socket.error`, which in turn is a subtype of
   :exc:`IOError`.  The error code and message of :exc:`SSLError` instances
   are provided by the OpenSSL library.

.. exception:: CertificateError

   Raised to signal an error with a certificate (such as mismatching
   hostname).  Certificate errors detected by OpenSSL, though, raise
   an :exc:`SSLError`.


Socket creation
^^^^^^^^^^^^^^^

The following function allows for standalone socket creation.  Starting from
Python 3.2, it can be more flexible to use :meth:`SSLContext.wrap_socket`
instead.

.. function:: wrap_socket(sock, keyfile=None, certfile=None, server_side=False, cert_reqs=CERT_NONE, ssl_version={see docs}, ca_certs=None, do_handshake_on_connect=True, suppress_ragged_eofs=True, ciphers=None)

   Takes an instance ``sock`` of :class:`socket.socket`, and returns an instance
   of :class:`ssl.SSLSocket`, a subtype of :class:`socket.socket`, which wraps
   the underlying socket in an SSL context.  For client-side sockets, the
   context construction is lazy; if the underlying socket isn't connected yet,
   the context construction will be performed after :meth:`connect` is called on
   the socket.  For server-side sockets, if the socket has no remote peer, it is
   assumed to be a listening socket, and the server-side SSL wrapping is
   automatically performed on client connections accepted via the :meth:`accept`
   method.  :func:`wrap_socket` may raise :exc:`SSLError`.

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
   interoperable with the other versions.  If not specified, for client-side
   operation, the default SSL version is SSLv3; for server-side operation,
   SSLv23.  These version selections provide the most compatibility with other
   versions.

   Here's a table showing which versions in a client (down the side) can connect
   to which versions in a server (along the top):

     .. table::

       ========================  =========  =========  ==========  =========
        *client* / **server**    **SSLv2**  **SSLv3**  **SSLv23**  **TLSv1**
       ------------------------  ---------  ---------  ----------  ---------
        *SSLv2*                    yes        no         yes         no
        *SSLv3*                    yes        yes        yes         no
        *SSLv23*                   yes        no         yes         no
        *TLSv1*                    no         no         yes         yes
       ========================  =========  =========  ==========  =========

   .. note::

      Which connections succeed will vary depending on the version of
      OpenSSL.  For instance, in some older versions of OpenSSL (such
      as 0.9.7l on OS X 10.4), an SSLv2 client could not connect to an
      SSLv23 server.  Another example: beginning with OpenSSL 1.0.0,
      an SSLv23 client will not actually attempt SSLv2 connections
      unless you explicitly enable SSLv2 ciphers; for example, you
      might specify ``"ALL"`` or ``"SSLv2"`` as the *ciphers* parameter
      to enable them.

   The *ciphers* parameter sets the available ciphers for this SSL object.
   It should be a string in the `OpenSSL cipher list format
   <http://www.openssl.org/docs/apps/ciphers.html#CIPHER_LIST_FORMAT>`_.

   The parameter ``do_handshake_on_connect`` specifies whether to do the SSL
   handshake automatically after doing a :meth:`socket.connect`, or whether the
   application program will call it explicitly, by invoking the
   :meth:`SSLSocket.do_handshake` method.  Calling
   :meth:`SSLSocket.do_handshake` explicitly gives the program control over the
   blocking behavior of the socket I/O involved in the handshake.

   The parameter ``suppress_ragged_eofs`` specifies how the
   :meth:`SSLSocket.recv` method should signal unexpected EOF from the other end
   of the connection.  If specified as :const:`True` (the default), it returns a
   normal EOF (an empty bytes object) in response to unexpected EOF errors
   raised from the underlying socket; if :const:`False`, it will raise the
   exceptions back to the caller.

   .. versionchanged:: 3.2
      New optional argument *ciphers*.

Random generation
^^^^^^^^^^^^^^^^^

.. function:: RAND_status()

   Returns True if the SSL pseudo-random number generator has been seeded with
   'enough' randomness, and False otherwise.  You can use :func:`ssl.RAND_egd`
   and :func:`ssl.RAND_add` to increase the randomness of the pseudo-random
   number generator.

.. function:: RAND_egd(path)

   If you are running an entropy-gathering daemon (EGD) somewhere, and ``path``
   is the pathname of a socket connection open to it, this will read 256 bytes
   of randomness from the socket, and add it to the SSL pseudo-random number
   generator to increase the security of generated secret keys.  This is
   typically only necessary on systems without better sources of randomness.

   See http://egd.sourceforge.net/ or http://prngd.sourceforge.net/ for sources
   of entropy-gathering daemons.

.. function:: RAND_add(bytes, entropy)

   Mixes the given ``bytes`` into the SSL pseudo-random number generator.  The
   parameter ``entropy`` (a float) is a lower bound on the entropy contained in
   string (so you can always use :const:`0.0`).  See :rfc:`1750` for more
   information on sources of entropy.

Certificate handling
^^^^^^^^^^^^^^^^^^^^

.. function:: match_hostname(cert, hostname)

   Verify that *cert* (in decoded format as returned by
   :meth:`SSLSocket.getpeercert`) matches the given *hostname*.  The rules
   applied are those for checking the identity of HTTPS servers as outlined
   in :rfc:`2818`, except that IP addresses are not currently supported.
   In addition to HTTPS, this function should be suitable for checking the
   identity of servers in various SSL-based protocols such as FTPS, IMAPS,
   POPS and others.

   :exc:`CertificateError` is raised on failure. On success, the function
   returns nothing::

      >>> cert = {'subject': ((('commonName', 'example.com'),),)}
      >>> ssl.match_hostname(cert, "example.com")
      >>> ssl.match_hostname(cert, "example.org")
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
        File "/home/py3k/Lib/ssl.py", line 130, in match_hostname
      ssl.CertificateError: hostname 'example.org' doesn't match 'example.com'

   .. versionadded:: 3.2

.. function:: cert_time_to_seconds(timestring)

   Returns a floating-point value containing a normal seconds-after-the-epoch
   time value, given the time-string representing the "notBefore" or "notAfter"
   date from a certificate.

   Here's an example::

     >>> import ssl
     >>> ssl.cert_time_to_seconds("May  9 00:00:00 2007 GMT")
     1178694000.0
     >>> import time
     >>> time.ctime(ssl.cert_time_to_seconds("May  9 00:00:00 2007 GMT"))
     'Wed May  9 00:00:00 2007'

.. function:: get_server_certificate(addr, ssl_version=PROTOCOL_SSLv3, ca_certs=None)

   Given the address ``addr`` of an SSL-protected server, as a (*hostname*,
   *port-number*) pair, fetches the server's certificate, and returns it as a
   PEM-encoded string.  If ``ssl_version`` is specified, uses that version of
   the SSL protocol to attempt to connect to the server.  If ``ca_certs`` is
   specified, it should be a file containing a list of root certificates, the
   same format as used for the same parameter in :func:`wrap_socket`.  The call
   will attempt to validate the server certificate against that set of root
   certificates, and will fail if the validation attempt fails.

.. function:: DER_cert_to_PEM_cert(DER_cert_bytes)

   Given a certificate as a DER-encoded blob of bytes, returns a PEM-encoded
   string version of the same certificate.

.. function:: PEM_cert_to_DER_cert(PEM_cert_string)

   Given a certificate as an ASCII PEM string, returns a DER-encoded sequence of
   bytes for that same certificate.

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

.. data:: PROTOCOL_SSLv2

   Selects SSL version 2 as the channel encryption protocol.

   .. warning::

      SSL version 2 is insecure.  Its use is highly discouraged.

.. data:: PROTOCOL_SSLv23

   Selects SSL version 2 or 3 as the channel encryption protocol.  This is a
   setting to use with servers for maximum compatibility with the other end of
   an SSL connection, but it may cause the specific ciphers chosen for the
   encryption to be of fairly low quality.

.. data:: PROTOCOL_SSLv3

   Selects SSL version 3 as the channel encryption protocol.  For clients, this
   is the maximally compatible SSL variant.

.. data:: PROTOCOL_TLSv1

   Selects TLS version 1 as the channel encryption protocol.  This is the most
   modern version, and probably the best choice for maximum protection, if both
   sides can speak it.

.. data:: OP_ALL

   Enables workarounds for various bugs present in other SSL implementations.
   This option is set by default.

   .. versionadded:: 3.2

.. data:: OP_NO_SSLv2

   Prevents an SSLv2 connection.  This option is only applicable in
   conjunction with :const:`PROTOCOL_SSLv23`.  It prevents the peers from
   choosing SSLv2 as the protocol version.

   .. versionadded:: 3.2

.. data:: OP_NO_SSLv3

   Prevents an SSLv3 connection.  This option is only applicable in
   conjunction with :const:`PROTOCOL_SSLv23`.  It prevents the peers from
   choosing SSLv3 as the protocol version.

   .. versionadded:: 3.2

.. data:: OP_NO_TLSv1

   Prevents a TLSv1 connection.  This option is only applicable in
   conjunction with :const:`PROTOCOL_SSLv23`.  It prevents the peers from
   choosing TLSv1 as the protocol version.

   .. versionadded:: 3.2

.. data:: HAS_SNI

   Whether the OpenSSL library has built-in support for the *Server Name
   Indication* extension to the SSLv3 and TLSv1 protocols (as defined in
   :rfc:`4366`).  When true, you can use the *server_hostname* argument to
   :meth:`SSLContext.wrap_socket`.

   .. versionadded:: 3.2

.. data:: OPENSSL_VERSION

   The version string of the OpenSSL library loaded by the interpreter::

    >>> ssl.OPENSSL_VERSION
    'OpenSSL 0.9.8k 25 Mar 2009'

   .. versionadded:: 3.2

.. data:: OPENSSL_VERSION_INFO

   A tuple of five integers representing version information about the
   OpenSSL library::

    >>> ssl.OPENSSL_VERSION_INFO
    (0, 9, 8, 11, 15)

   .. versionadded:: 3.2

.. data:: OPENSSL_VERSION_NUMBER

   The raw version number of the OpenSSL library, as a single integer::

    >>> ssl.OPENSSL_VERSION_NUMBER
    9470143
    >>> hex(ssl.OPENSSL_VERSION_NUMBER)
    '0x9080bf'

   .. versionadded:: 3.2


SSL Sockets
-----------

SSL sockets provide the following methods of :ref:`socket-objects`:

- :meth:`~socket.socket.accept()`
- :meth:`~socket.socket.bind()`
- :meth:`~socket.socket.close()`
- :meth:`~socket.socket.connect()`
- :meth:`~socket.socket.detach()`
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

They also have the following additional methods and attributes:

.. method:: SSLSocket.do_handshake()

   Performs the SSL setup handshake.  If the socket is non-blocking, this method
   may raise :exc:`SSLError` with the value of the exception instance's
   ``args[0]`` being either :const:`SSL_ERROR_WANT_READ` or
   :const:`SSL_ERROR_WANT_WRITE`, and should be called again until it stops
   raising those exceptions.  Here's an example of how to do that::

        while True:
            try:
                sock.do_handshake()
                break
            except ssl.SSLError as err:
                if err.args[0] == ssl.SSL_ERROR_WANT_READ:
                    select.select([sock], [], [])
                elif err.args[0] == ssl.SSL_ERROR_WANT_WRITE:
                    select.select([], [sock], [])
                else:
                    raise

.. method:: SSLSocket.getpeercert(binary_form=False)

   If there is no certificate for the peer on the other end of the connection,
   returns ``None``.

   If the parameter ``binary_form`` is :const:`False`, and a certificate was
   received from the peer, this method returns a :class:`dict` instance.  If the
   certificate was not validated, the dict is empty.  If the certificate was
   validated, it returns a dict with the keys ``subject`` (the principal for
   which the certificate was issued), and ``notAfter`` (the time after which the
   certificate should not be trusted).  The certificate was already validated,
   so the ``notBefore`` and ``issuer`` fields are not returned.  If a
   certificate contains an instance of the *Subject Alternative Name* extension
   (see :rfc:`3280`), there will also be a ``subjectAltName`` key in the
   dictionary.

   The "subject" field is a tuple containing the sequence of relative
   distinguished names (RDNs) given in the certificate's data structure for the
   principal, and each RDN is a sequence of name-value pairs::

      {'notAfter': 'Feb 16 16:54:50 2013 GMT',
       'subject': ((('countryName', 'US'),),
                   (('stateOrProvinceName', 'Delaware'),),
                   (('localityName', 'Wilmington'),),
                   (('organizationName', 'Python Software Foundation'),),
                   (('organizationalUnitName', 'SSL'),),
                   (('commonName', 'somemachine.python.org'),))}

   If the ``binary_form`` parameter is :const:`True`, and a certificate was
   provided, this method returns the DER-encoded form of the entire certificate
   as a sequence of bytes, or :const:`None` if the peer did not provide a
   certificate.  This return value is independent of validation; if validation
   was required (:const:`CERT_OPTIONAL` or :const:`CERT_REQUIRED`), it will have
   been validated, but if :const:`CERT_NONE` was used to establish the
   connection, the certificate, if present, will not have been validated.

.. method:: SSLSocket.cipher()

   Returns a three-value tuple containing the name of the cipher being used, the
   version of the SSL protocol that defines its use, and the number of secret
   bits being used.  If no connection has been established, returns ``None``.


.. method:: SSLSocket.unwrap()

   Performs the SSL shutdown handshake, which removes the TLS layer from the
   underlying socket, and returns the underlying socket object.  This can be
   used to go from encrypted operation over a connection to unencrypted.  The
   returned socket should always be used for further communication with the
   other side of the connection, rather than the original socket.


.. attribute:: SSLSocket.context

   The :class:`SSLContext` object this SSL socket is tied to.  If the SSL
   socket was created using the top-level :func:`wrap_socket` function
   (rather than :meth:`SSLContext.wrap_socket`), this is a custom context
   object created for this SSL socket.

   .. versionadded:: 3.2


SSL Contexts
------------

.. versionadded:: 3.2

An SSL context holds various data longer-lived than single SSL connections,
such as SSL configuration options, certificate(s) and private key(s).
It also manages a cache of SSL sessions for server-side sockets, in order
to speed up repeated connections from the same clients.

.. class:: SSLContext(protocol)

   Create a new SSL context.  You must pass *protocol* which must be one
   of the ``PROTOCOL_*`` constants defined in this module.
   :data:`PROTOCOL_SSLv23` is recommended for maximum interoperability.


:class:`SSLContext` objects have the following methods and attributes:

.. method:: SSLContext.load_cert_chain(certfile, keyfile=None)

   Load a private key and the corresponding certificate.  The *certfile*
   string must be the path to a single file in PEM format containing the
   certificate as well as any number of CA certificates needed to establish
   the certificate's authenticity.  The *keyfile* string, if present, must
   point to a file containing the private key in.  Otherwise the private
   key will be taken from *certfile* as well.  See the discussion of
   :ref:`ssl-certificates` for more information on how the certificate
   is stored in the *certfile*.

   An :class:`SSLError` is raised if the private key doesn't
   match with the certificate.

.. method:: SSLContext.load_verify_locations(cafile=None, capath=None)

   Load a set of "certification authority" (CA) certificates used to validate
   other peers' certificates when :data:`verify_mode` is other than
   :data:`CERT_NONE`.  At least one of *cafile* or *capath* must be specified.

   The *cafile* string, if present, is the path to a file of concatenated
   CA certificates in PEM format. See the discussion of
   :ref:`ssl-certificates` for more information about how to arrange the
   certificates in this file.

   The *capath* string, if present, is
   the path to a directory containing several CA certificates in PEM format,
   following an `OpenSSL specific layout
   <http://www.openssl.org/docs/ssl/SSL_CTX_load_verify_locations.html>`_.

.. method:: SSLContext.set_ciphers(ciphers)

   Set the available ciphers for sockets created with this context.
   It should be a string in the `OpenSSL cipher list format
   <http://www.openssl.org/docs/apps/ciphers.html#CIPHER_LIST_FORMAT>`_.
   If no cipher can be selected (because compile-time options or other
   configuration forbids use of all the specified ciphers), an
   :class:`SSLError` will be raised.

   .. note::
      when connected, the :meth:`SSLSocket.cipher` method of SSL sockets will
      give the currently selected cipher.

.. method:: SSLContext.wrap_socket(sock, server_side=False, \
      do_handshake_on_connect=True, suppress_ragged_eofs=True, \
      server_hostname=None)

   Wrap an existing Python socket *sock* and return an :class:`SSLSocket`
   object.  The SSL socket is tied to the context, its settings and
   certificates.  The parameters *server_side*, *do_handshake_on_connect*
   and *suppress_ragged_eofs* have the same meaning as in the top-level
   :func:`wrap_socket` function.

   On client connections, the optional parameter *server_hostname* specifies
   the hostname of the service which we are connecting to.  This allows a
   single server to host multiple SSL-based services with distinct certificates,
   quite similarly to HTTP virtual hosts.  Specifying *server_hostname*
   will raise a :exc:`ValueError` if the OpenSSL library doesn't have support
   for it (that is, if :data:`HAS_SNI` is :const:`False`).  Specifying
   *server_hostname* will also raise a :exc:`ValueError` if *server_side*
   is true.

.. method:: SSLContext.session_stats()

   Get statistics about the SSL sessions created or managed by this context.
   A dictionary is returned which maps the names of each `piece of information
   <http://www.openssl.org/docs/ssl/SSL_CTX_sess_number.html>`_ to their
   numeric values.  For example, here is the total number of hits and misses
   in the session cache since the context was created::

      >>> stats = context.session_stats()
      >>> stats['hits'], stats['misses']
      (0, 0)

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
chain it finds in the file which matches.  Some "standard" root certificates are
available from various certification authorities: `CACert.org
<http://www.cacert.org/index.php?id=3>`_, `Thawte
<http://www.thawte.com/roots/>`_, `Verisign
<http://www.verisign.com/support/roots.html>`_, `Positive SSL
<http://www.PositiveSSL.com/ssl-certificate-support/cert_installation/UTN-USERFirst-Hardware.crt>`_
(used by python.org), `Equifax and GeoTrust
<http://www.geotrust.com/resources/root_certificates/index.asp>`_.

In general, if you are using SSL3 or TLS1, you don't need to put the full chain
in your "CA certs" file; you only need the root certificates, and the remote
peer is supposed to furnish the other certificates necessary to chain from its
certificate to a root certificate.  See :rfc:`4158` for more discussion of the
way in which certification chains can be built.

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
      [ do something that requires SSL support ]

Client-side operation
^^^^^^^^^^^^^^^^^^^^^

This example connects to an SSL server and prints the server's certificate::

   import socket, ssl, pprint

   s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
   # require a certificate from the server
   ssl_sock = ssl.wrap_socket(s,
                              ca_certs="/etc/ca_certs_file",
                              cert_reqs=ssl.CERT_REQUIRED)
   ssl_sock.connect(('www.verisign.com', 443))

   pprint.pprint(ssl_sock.getpeercert())
   # note that closing the SSLSocket will also close the underlying socket
   ssl_sock.close()

As of October 6, 2010, the certificate printed by this program looks like
this::

   {'notAfter': 'May 25 23:59:59 2012 GMT',
    'subject': ((('1.3.6.1.4.1.311.60.2.1.3', 'US'),),
                (('1.3.6.1.4.1.311.60.2.1.2', 'Delaware'),),
                (('businessCategory', 'V1.0, Clause 5.(b)'),),
                (('serialNumber', '2497886'),),
                (('countryName', 'US'),),
                (('postalCode', '94043'),),
                (('stateOrProvinceName', 'California'),),
                (('localityName', 'Mountain View'),),
                (('streetAddress', '487 East Middlefield Road'),),
                (('organizationName', 'VeriSign, Inc.'),),
                (('organizationalUnitName', ' Production Security Services'),),
                (('commonName', 'www.verisign.com'),))}

This other example first creates an SSL context, instructs it to verify
certificates sent by peers, and feeds it a set of recognized certificate
authorities (CA)::

   >>> context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
   >>> context.verify_mode = ssl.CERT_REQUIRED
   >>> context.load_verify_locations("/etc/ssl/certs/ca-bundle.crt")

(it is assumed your operating system places a bundle of all CA certificates
in ``/etc/ssl/certs/ca-bundle.crt``; if not, you'll get an error and have
to adjust the location)

When you use the context to connect to a server, :const:`CERT_REQUIRED`
validates the server certificate: it ensures that the server certificate
was signed with one of the CA certificates, and checks the signature for
correctness::

   >>> conn = context.wrap_socket(socket.socket(socket.AF_INET))
   >>> conn.connect(("linuxfr.org", 443))

You should then fetch the certificate and check its fields for conformity::

   >>> cert = conn.getpeercert()
   >>> ssl.match_hostname(cert, "linuxfr.org")

Visual inspection shows that the certificate does identify the desired service
(that is, the HTTPS host ``linuxfr.org``)::

   >>> pprint.pprint(cert)
   {'notAfter': 'Jun 26 21:41:46 2011 GMT',
    'subject': ((('commonName', 'linuxfr.org'),),),
    'subjectAltName': (('DNS', 'linuxfr.org'), ('othername', '<unsupported>'))}

Now that you are assured of its authenticity, you can proceed to talk with
the server::

   >>> conn.sendall(b"HEAD / HTTP/1.0\r\nHost: linuxfr.org\r\n\r\n")
   >>> pprint.pprint(conn.recv(1024).split(b"\r\n"))
   [b'HTTP/1.1 302 Found',
    b'Date: Sun, 16 May 2010 13:43:28 GMT',
    b'Server: Apache/2.2',
    b'Location: https://linuxfr.org/pub/',
    b'Vary: Accept-Encoding',
    b'Connection: close',
    b'Content-Type: text/html; charset=iso-8859-1',
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

   context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
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
         connstream.close()

Then you'll read data from the ``connstream`` and do something with it till you
are finished with the client (or the client is finished with you)::

   def deal_with_client(connstream):
      data = connstream.recv(1024)
      # empty data means the client is finished with us
      while data:
         if not do_something(connstream, data):
            # we'll assume do_something returns False
            # when we're finished with client
            break
         data = connstream.recv(1024)
      # finished with client

And go back to listening for new client connections (of course, a real server
would probably handle each client connection in a separate thread, or put
the sockets in non-blocking mode and use an event loop).


.. _ssl-security:

Security considerations
-----------------------

Verifying certificates
^^^^^^^^^^^^^^^^^^^^^^

:const:`CERT_NONE` is the default.  Since it does not authenticate the other
peer, it can be insecure, especially in client mode where most of time you
would like to ensure the authenticity of the server you're talking to.
Therefore, when in client mode, it is highly recommended to use
:const:`CERT_REQUIRED`.  However, it is in itself not sufficient; you also
have to check that the server certificate, which can be obtained by calling
:meth:`SSLSocket.getpeercert`, matches the desired service.  For many
protocols and applications, the service can be identified by the hostname;
in this case, the :func:`match_hostname` function can be used.

In server mode, if you want to authenticate your clients using the SSL layer
(rather than using a higher-level authentication mechanism), you'll also have
to specify :const:`CERT_REQUIRED` and similarly check the client certificate.

   .. note::

      In client mode, :const:`CERT_OPTIONAL` and :const:`CERT_REQUIRED` are
      equivalent unless anonymous ciphers are enabled (they are disabled
      by default).

Protocol versions
^^^^^^^^^^^^^^^^^

SSL version 2 is considered insecure and is therefore dangerous to use.  If
you want maximum compatibility between clients and servers, it is recommended
to use :const:`PROTOCOL_SSLv23` as the protocol version and then disable
SSLv2 explicitly using the :data:`SSLContext.options` attribute::

   context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
   context.options |= ssl.OP_NO_SSLv2

The SSL context created above will allow SSLv3 and TLSv1 connections, but
not SSLv2.


.. seealso::

   Class :class:`socket.socket`
            Documentation of underlying :mod:`socket` class

   `Introducing SSL and Certificates using OpenSSL <http://old.pseudonym.org/ssl/wwwj-index.html>`_
       Frederick J. Hirsch

   `RFC 1422: Privacy Enhancement for Internet Electronic Mail: Part II: Certificate-Based Key Management <http://www.ietf.org/rfc/rfc1422>`_
       Steve Kent

   `RFC 1750: Randomness Recommendations for Security <http://www.ietf.org/rfc/rfc1750>`_
       D. Eastlake et. al.

   `RFC 3280: Internet X.509 Public Key Infrastructure Certificate and CRL Profile <http://www.ietf.org/rfc/rfc3280>`_
       Housley et. al.

   `RFC 4366: Transport Layer Security (TLS) Extensions <http://www.ietf.org/rfc/rfc4366>`_
       Blake-Wilson et. al.
