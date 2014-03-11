:mod:`hashlib` --- Secure hashes and message digests
====================================================

.. module:: hashlib
   :synopsis: Secure hash and message digest algorithms.
.. moduleauthor:: Gregory P. Smith <greg@krypto.org>
.. sectionauthor:: Gregory P. Smith <greg@krypto.org>


.. index::
   single: message digest, MD5
   single: secure hash algorithm, SHA1, SHA224, SHA256, SHA384, SHA512

**Source code:** :source:`Lib/hashlib.py`

--------------

This module implements a common interface to many different secure hash and
message digest algorithms.  Included are the FIPS secure hash algorithms SHA1,
SHA224, SHA256, SHA384, and SHA512 (defined in FIPS 180-2) as well as RSA's MD5
algorithm (defined in Internet :rfc:`1321`).  The terms "secure hash" and
"message digest" are interchangeable.  Older algorithms were called message
digests.  The modern term is secure hash.

.. note::

   If you want the adler32 or crc32 hash functions, they are available in
   the :mod:`zlib` module.

.. warning::

   Some algorithms have known hash collision weaknesses, refer to the "See
   also" section at the end.


.. _hash-algorithms:

Hash algorithms
---------------

There is one constructor method named for each type of :dfn:`hash`.  All return
a hash object with the same simple interface. For example: use :func:`sha1` to
create a SHA1 hash object. You can now feed this object with :term:`bytes-like
object`\ s (normally :class:`bytes`) using the :meth:`update` method.
At any point you can ask it for the :dfn:`digest` of the
concatenation of the data fed to it so far using the :meth:`digest` or
:meth:`hexdigest` methods.

.. note::

   For better multithreading performance, the Python :term:`GIL` is released for
   data larger than 2047 bytes at object creation or on update.

.. note::

   Feeding string objects into :meth:`update` is not supported, as hashes work
   on bytes, not on characters.

.. index:: single: OpenSSL; (use in module hashlib)

Constructors for hash algorithms that are always present in this module are
:func:`md5`, :func:`sha1`, :func:`sha224`, :func:`sha256`, :func:`sha384`,
and :func:`sha512`. Additional algorithms may also be available depending upon
the OpenSSL library that Python uses on your platform.

For example, to obtain the digest of the byte string ``b'Nobody inspects the
spammish repetition'``::

   >>> import hashlib
   >>> m = hashlib.md5()
   >>> m.update(b"Nobody inspects")
   >>> m.update(b" the spammish repetition")
   >>> m.digest()
   b'\xbbd\x9c\x83\xdd\x1e\xa5\xc9\xd9\xde\xc9\xa1\x8d\xf0\xff\xe9'
   >>> m.digest_size
   16
   >>> m.block_size
   64

More condensed:

   >>> hashlib.sha224(b"Nobody inspects the spammish repetition").hexdigest()
   'a4337bc45a8fc544c03f52dc550cd6e1e87021bc896588bd79e901e2'

.. function:: new(name[, data])

   Is a generic constructor that takes the string name of the desired
   algorithm as its first parameter.  It also exists to allow access to the
   above listed hashes as well as any other algorithms that your OpenSSL
   library may offer.  The named constructors are much faster than :func:`new`
   and should be preferred.

Using :func:`new` with an algorithm provided by OpenSSL:

   >>> h = hashlib.new('ripemd160')
   >>> h.update(b"Nobody inspects the spammish repetition")
   >>> h.hexdigest()
   'cc4a5ce1b3df48aec5d22d1f16b894a0b894eccc'

Hashlib provides the following constant attributes:

.. data:: algorithms_guaranteed

   A set containing the names of the hash algorithms guaranteed to be supported
   by this module on all platforms.

   .. versionadded:: 3.2

.. data:: algorithms_available

   A set containing the names of the hash algorithms that are available in the
   running Python interpreter.  These names will be recognized when passed to
   :func:`new`.  :attr:`algorithms_guaranteed` will always be a subset.  The
   same algorithm may appear multiple times in this set under different names
   (thanks to OpenSSL).

   .. versionadded:: 3.2

The following values are provided as constant attributes of the hash objects
returned by the constructors:


.. data:: hash.digest_size

   The size of the resulting hash in bytes.

.. data:: hash.block_size

   The internal block size of the hash algorithm in bytes.

A hash object has the following attributes:

.. attribute:: hash.name

   The canonical name of this hash, always lowercase and always suitable as a
   parameter to :func:`new` to create another hash of this type.

   .. versionchanged:: 3.4
      The name attribute has been present in CPython since its inception, but
      until Python 3.4 was not formally specified, so may not exist on some
      platforms.

A hash object has the following methods:


.. method:: hash.update(arg)

   Update the hash object with the object *arg*, which must be interpretable as
   a buffer of bytes.  Repeated calls are equivalent to a single call with the
   concatenation of all the arguments: ``m.update(a); m.update(b)`` is
   equivalent to ``m.update(a+b)``.

   .. versionchanged:: 3.1
      The Python GIL is released to allow other threads to run while hash
      updates on data larger than 2047 bytes is taking place when using hash
      algorithms supplied by OpenSSL.


.. method:: hash.digest()

   Return the digest of the data passed to the :meth:`update` method so far.
   This is a bytes object of size :attr:`digest_size` which may contain bytes in
   the whole range from 0 to 255.


.. method:: hash.hexdigest()

   Like :meth:`digest` except the digest is returned as a string object of
   double length, containing only hexadecimal digits.  This may be used to
   exchange the value safely in email or other non-binary environments.


.. method:: hash.copy()

   Return a copy ("clone") of the hash object.  This can be used to efficiently
   compute the digests of data sharing a common initial substring.


Key Derivation Function
-----------------------

Key derivation and key stretching algorithms are designed for secure password
hashing. Naive algorithms such as ``sha1(password)`` are not resistant
against brute-force attacks. A good password hashing function must be tunable,
slow and include a salt.


.. function:: pbkdf2_hmac(name, password, salt, rounds, dklen=None)

   The function provides PKCS#5 password-based key derivation function 2. It
   uses HMAC as pseudorandom function.

   The string *name* is the desired name of the hash digest algorithm for
   HMAC, e.g. 'sha1' or 'sha256'. *password* and *salt* are interpreted as
   buffers of bytes. Applications and libraries should limit *password* to
   a sensible value (e.g. 1024). *salt* should be about 16 or more bytes from
   a proper source, e.g. :func:`os.urandom`.

   The number of *rounds* should be chosen based on the hash algorithm and
   computing power. As of 2013 a value of at least 100,000 rounds of SHA-256
   have been suggested.

   *dklen* is the length of the derived key. If *dklen* is ``None`` then the
   digest size of the hash algorithm *name* is used, e.g. 64 for SHA-512.

   >>> import hashlib, binascii
   >>> dk = hashlib.pbkdf2_hmac('sha256', b'password', b'salt', 100000)
   >>> binascii.hexlify(dk)
   b'0394a2ede332c9a13eb82e9b24631604c31df978b4e2f0fbd2c549944f9d79a5'

   .. versionadded:: 3.4

   .. note:: A fast implementation of *pbkdf2_hmac* is available with OpenSSL.
      The Python implementation uses an inline version of :mod:`hmac`. It is
      about three times slower and doesn't release the GIL.


.. seealso::

   Module :mod:`hmac`
      A module to generate message authentication codes using hashes.

   Module :mod:`base64`
      Another way to encode binary hashes for non-binary environments.

   http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf
      The FIPS 180-2 publication on Secure Hash Algorithms.

   http://en.wikipedia.org/wiki/Cryptographic_hash_function#Cryptographic_hash_algorithms
      Wikipedia article with information on which algorithms have known issues and
      what that means regarding their use.

   http://www.ietf.org/rfc/rfc2898.txt
      PKCS #5: Password-Based Cryptography Specification Version 2.0
