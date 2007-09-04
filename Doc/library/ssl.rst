
:mod:`ssl` --- SSL wrapper for socket objects
====================================================================

.. module:: ssl
   :synopsis: SSL wrapper for socket objects

.. moduleauthor:: Bill Janssen <bill.janssen@gmail.com>
.. sectionauthor::  Bill Janssen <bill.janssen@gmail.com>


This module provides access to Transport Layer Security (often known
as "Secure Sockets Layer") encryption and peer authentication
facilities for network sockets, both client-side and server-side.
This module uses the OpenSSL library. It is available on all modern
Unix systems, Windows, Mac OS X, and probably additional
platforms, as long as OpenSSL is installed on that platform.

.. note::

   Some behavior may be platform dependent, since calls are made to the operating
   system socket APIs.

This section documents the objects and functions in the ``ssl`` module;
for more general information about TLS, SSL, and certificates, the
reader is referred to the documents in the :ref:`ssl-references` section.

This module defines a class, :class:`ssl.sslsocket`, which is
derived from the :class:`socket.socket` type, and supports additional
:meth:`read` and :meth:`write` methods, along with a method, :meth:`getpeercert`,
to retrieve the certificate of the other side of the connection.

This module defines the following functions, exceptions, and constants:

.. function:: cert_time_to_seconds(timestring)

   Returns a floating-point value containing a normal seconds-after-the-epoch time
   value, given the time-string representing the "notBefore" or "notAfter" date
   from a certificate.

   Here's an example::

     >>> import ssl
     >>> ssl.cert_time_to_seconds("May  9 00:00:00 2007 GMT")
     1178694000.0
     >>> import time
     >>> time.ctime(ssl.cert_time_to_seconds("May  9 00:00:00 2007 GMT"))
     'Wed May  9 00:00:00 2007'
     >>> 

.. exception:: sslerror

   Raised to signal an error from the underlying SSL implementation.  This 
   signifies some problem in the higher-level
   encryption and authentication layer that's superimposed on the underlying
   network connection.

.. data:: CERT_NONE

   Value to pass to the ``cert_reqs`` parameter to :func:`sslobject`
   when no certificates will be required or validated from the other
   side of the socket connection.

.. data:: CERT_OPTIONAL

   Value to pass to the ``cert_reqs`` parameter to :func:`sslobject`
   when no certificates will be required from the other side of the
   socket connection, but if they are provided, will be validated.
   Note that use of this setting requires a valid certificate
   validation file also be passed as a value of the ``ca_certs``
   parameter.

.. data:: CERT_REQUIRED

   Value to pass to the ``cert_reqs`` parameter to :func:`sslobject`
   when certificates will be required from the other side of the
   socket connection.  Note that use of this setting requires a valid certificate
   validation file also be passed as a value of the ``ca_certs``
   parameter.

.. data:: PROTOCOL_SSLv2

   Selects SSL version 2 as the channel encryption protocol.

.. data:: PROTOCOL_SSLv23

   Selects SSL version 2 or 3 as the channel encryption protocol.  This is a setting to use for maximum compatibility
   with the other end of an SSL connection, but it may cause the specific ciphers chosen for the encryption to be
   of fairly low quality.

.. data:: PROTOCOL_SSLv3

   Selects SSL version 3 as the channel encryption protocol.

.. data:: PROTOCOL_TLSv1

   Selects SSL version 2 as the channel encryption protocol.  This is
   the most modern version, and probably the best choice for maximum
   protection, if both sides can speak it.


.. _ssl-certificates:

Certificates
------------

Certificates in general are part of a public-key / private-key system.  In this system, each *principal*,
(which may be a machine, or a person, or an organization) is assigned a unique two-part encryption key.
One part of the key is public, and is called the *public key*; the other part is kept secret, and is called
the *private key*.  The two parts are related, in that if you encrypt a message with one of the parts, you can
decrypt it with the other part, and **only** with the other part.

A certificate contains information about two principals.  It contains
the name of a *subject*, and the subject's public key.  It also
contains a statement by a second principal, the *issuer*, that the
subject is who he claims to be, and that this is indeed the subject's
public key.  The issuer's statement is signed with the issuer's
private key, which only the issuer knows.  However, anyone can verify
the issuer's statement by finding the issuer's public key, decrypting
the statement with it, and comparing it to the other information in
the certificate.  The certificate also contains information about the
time period over which it is valid.  This is expressed as two fields,
called "notBefore" and "notAfter".

In the Python use of certificates, a client or server
can use a certificate to prove who they are.  The other
side of a network connection can also be required to produce a certificate,
and that certificate can be validated to the satisfaction
of the client or server that requires such validation.
The connection can be set to fail automatically if such
validation is not achieved.

Python uses files to contain certificates.  They should be formatted
as "PEM" (see :rfc:`1422`), which is a base-64 encoded form wrapped
with a header line and a footer line::

      -----BEGIN CERTIFICATE-----
      ... (certificate in base64 PEM encoding) ...
      -----END CERTIFICATE-----

The Python files which contain certificates can contain a sequence
of certificates, sometimes called a *certificate chain*.  This chain
should start with the specific certificate for the principal who "is"
the client or server, and then the certificate for the issuer of that
certificate, and then the certificate for the issuer of *that* certificate,
and so on up the chain till you get to a certificate which is *self-signed*,
that is, a certificate which has the same subject and issuer, 
sometimes called a *root certificate*.  The certificates should just
be concatenated together in the certificate file.  For example, suppose
we had a three certificate chain, from our server certificate to the
certificate of the certification authority that signed our server certificate,
to the root certificate of the agency which issued the certification authority's
certificate::

      -----BEGIN CERTIFICATE-----
      ... (certificate for your server)...
      -----END CERTIFICATE-----
      -----BEGIN CERTIFICATE-----
      ... (the certificate for the CA)...
      -----END CERTIFICATE-----
      -----BEGIN CERTIFICATE-----
      ... (the root certificate for the CA's issuer)...
      -----END CERTIFICATE-----

If you are going to require validation of the other side of the connection's
certificate, you need to provide a "CA certs" file, filled with the certificate
chains for each issuer you are willing to trust.  Again, this file just
contains these chains concatenated together.  For validation, Python will
use the first chain it finds in the file which matches.
Some "standard" root certificates are available at
http://www.thawte.com/roots/  (for Thawte roots) and
http://www.verisign.com/support/roots.html  (for Verisign roots).


sslsocket Objects
-----------------

.. class:: sslsocket(sock [, keyfile=None, certfile=None, server_side=False, cert_reqs=CERT_NONE, ssl_version=PROTOCOL_SSLv23, ca_certs=None])

   Takes an instance ``sock`` of :class:`socket.socket`, and returns an instance of a subtype
   of :class:`socket.socket` which wraps the underlying socket in an SSL context.
   For client-side sockets, the context construction is lazy; if the underlying socket isn't
   connected yet, the context construction will be performed after :meth:`connect` is called
   on the socket.

   The ``keyfile`` and ``certfile`` parameters specify optional files which contain a certificate
   to be used to identify the local side of the connection.  See the above discussion of :ref:`ssl-certificates`
   for more information on how the certificate is stored in the ``certfile``.

   Often the private key is stored
   in the same file as the certificate; in this case, only the ``certfile`` parameter need be
   passed.  If the private key is stored in a separate file, both parameters must be used.
   If the private key is stored in the ``certfile``, it should come before the first certificate
   in the certificate chain::

      -----BEGIN RSA PRIVATE KEY-----
      ... (private key in base64 encoding) ...
      -----END RSA PRIVATE KEY-----
      -----BEGIN CERTIFICATE-----
      ... (certificate in base64 PEM encoding) ...
      -----END CERTIFICATE-----

   The parameter ``server_side`` is a boolean which identifies whether server-side or client-side
   behavior is desired from this socket.

   The parameter ``cert_reqs`` specifies whether a certificate is
   required from the other side of the connection, and whether it will
   be validated if provided.  It must be one of the three values
   :const:`CERT_NONE` (certificates ignored), :const:`CERT_OPTIONAL` (not required,
   but validated if provided), or :const:`CERT_REQUIRED` (required and
   validated).  If the value of this parameter is not :const:`CERT_NONE`, then
   the ``ca_certs`` parameter must point to a file of CA certificates.

   The parameter ``ssl_version`` specifies which version of the SSL protocol to use.  Typically,
   the server specifies this, and a client connecting to it must use the same protocol.  An
   SSL server using :const:`PROTOCOL_SSLv23` can understand a client connecting via SSL2, SSL3, or TLS1,
   but a client using :const:`PROTOCOL_SSLv23` can only connect to an SSL2 server.

   The ``ca_certs`` file contains a set of concatenated "certification authority" certificates,
   which are used to validate certificates passed from the other end of the connection.
   See the above discussion of :ref:`ssl-certificates` for more information about how to arrange
   the certificates in this file.

.. method:: sslsocket.read([nbytes])

   Reads up to ``nbytes`` bytes from the SSL-encrypted channel and returns them.

.. method:: sslsocket.write(data)

   Writes the ``data`` to the other side of the connection, using the SSL channel to encrypt.  Returns the number
   of bytes written.

.. method:: sslsocket.getpeercert()

   If there is no certificate for the peer on the other end of the connection, returns ``None``.
   If a certificate was received from the peer, but not validated, returns an empty ``dict`` instance.
   If a certificate was received and validated, returns a ``dict`` instance with the fields
   ``subject`` (the principal for which the certificate was issued), ``issuer`` (the signer of
   the certificate), ``notBefore`` (the time before which the certificate should not be trusted),
   and ``notAfter`` (the time after which the certificate should not be trusted) filled in.

   The "subject" and "issuer" fields are themselves dictionaries containing the fields given
   in the certificate's data structure for each principal::

      {'issuer': {'commonName': u'somemachine.python.org',
                  'countryName': u'US',
                  'localityName': u'Wilmington',
                  'organizationName': u'Python Software Foundation',
                  'organizationalUnitName': u'SSL',
                  'stateOrProvinceName': u'Delaware'},
       'subject': {'commonName': u'somemachine.python.org',
                   'countryName': u'US',
                   'localityName': u'Wilmington',
                   'organizationName': u'Python Software Foundation',
                   'organizationalUnitName': u'SSL',
                   'stateOrProvinceName': u'Delaware'},
       'notAfter': 'Sep  4 21:54:26 2007 GMT',
       'notBefore': 'Aug 25 21:54:26 2007 GMT',
       'version': 2}

   This certificate is said to be *self-signed*, because the subject
   and issuer are the same entity.  The *version* field refers to the X509 version
   that's used for the certificate.

.. method:: sslsocket.ssl_shutdown()

   Closes the SSL context (if any) over the socket, but leaves the socket connection
   open for further use, if both sides are willing.  This is different from :meth:`socket.socket.shutdown`,
   which will close the connection, but leave the local socket available for further use.


Examples
--------

Testing for SSL support
^^^^^^^^^^^^^^^^^^^^^^^

To test for the presence of SSL support in a Python installation, user code should use the following idiom::

   try:
      import ssl
   except ImportError:
      pass
   else:
      [ do something that requires SSL support ]

Client-side operation
^^^^^^^^^^^^^^^^^^^^^

This example connects to an SSL server, prints the server's address and certificate,
sends some bytes, and reads part of the response::

   import socket, ssl, pprint

   s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
   ssl_sock = ssl.sslsocket(s, ca_certs="/etc/ca_certs_file", cert_reqs=ssl.CERT_REQUIRED)

   ssl_sock.connect(('www.verisign.com', 443))

   print(repr(ssl_sock.getpeername()))
   pprint.pprint(ssl_sock.getpeercert())

   # Set a simple HTTP request -- use httplib in actual code.
   ssl_sock.write("""GET / HTTP/1.0\r
   Host: www.verisign.com\r\n\r\n""")

   # Read a chunk of data.  Will not necessarily
   # read all the data returned by the server.
   data = ssl_sock.read()

   # note that closing the sslsocket will also close the underlying socket
   ssl_sock.close()

As of August 25, 2007, the certificate printed by this program
looked like this::

   {'issuer': {'commonName': u'VeriSign Class 3 Extended Validation SSL SGC CA',
               'countryName': u'US',
               'organizationName': u'VeriSign, Inc.',
               'organizationalUnitName': u'Terms of use at https://www.verisign.com/rpa (c)06'},
    'subject': {'1.3.6.1.4.1.311.60.2.1.2': u'Delaware',
                '1.3.6.1.4.1.311.60.2.1.3': u'US',
                'commonName': u'www.verisign.com',
                'countryName': u'US',
                'localityName': u'Mountain View',
                'organizationName': u'VeriSign, Inc.',
                'organizationalUnitName': u'Terms of use at www.verisign.com/rpa (c)06',
                'postalCode': u'94043',
                'serialNumber': u'2497886',
                'stateOrProvinceName': u'California',
                'streetAddress': u'487 East Middlefield Road'},
    'notAfter': 'May  8 23:59:59 2009 GMT',
    'notBefore': 'May  9 00:00:00 2007 GMT',
    'version': 2}

Server-side operation
^^^^^^^^^^^^^^^^^^^^^

For server operation, typically you'd need to have a server certificate, and private key, each in a file.
You'd open a socket, bind it to a port, call :meth:`listen` on it, then start waiting for clients
to connect::

   import socket, ssl

   bindsocket = socket.socket()
   bindsocket.bind(('myaddr.mydomain.com', 10023))
   bindsocket.listen(5)

When one did, you'd call :meth:`accept` on the socket to get the new socket from the other
end, and use :func:`sslsocket` to create a server-side SSL context for it::

   while True:
      newsocket, fromaddr = bindsocket.accept()
      connstream = ssl.sslsocket(newsocket, server_side=True, certfile="mycertfile",
                                 keyfile="mykeyfile", ssl_protocol=ssl.PROTOCOL_TLSv1)
      deal_with_client(connstream)

Then you'd read data from the ``connstream`` and do something with it till you are finished with the client (or the client is finished with you)::

   def deal_with_client(connstream):

      data = connstream.read()
      # null data means the client is finished with us
      while data:
         if not do_something(connstream, data):
            # we'll assume do_something returns False when we're finished with client
            break
         data = connstream.read()
      # finished with client
      connstream.close()

And go back to listening for new client connections.

           
.. _ssl-references:

References
----------

Class :class:`socket.socket`
      Documentation of underlying :mod:`socket` class

`Introducing SSL and Certificates using OpenSSL <http://old.pseudonym.org/ssl/wwwj-index.html>`_, by Frederick J. Hirsch

`Privacy Enhancement for Internet Electronic Mail: Part II: Certificate-Based Key Management`, :rfc:`1422`, by Steve Kent
