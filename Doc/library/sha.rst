
:mod:`sha` --- SHA-1 message digest algorithm
=============================================

.. module:: sha
   :synopsis: NIST's secure hash algorithm, SHA.
   :deprecated:
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


.. deprecated:: 2.5
   Use the :mod:`hashlib` module instead.

.. index::
   single: NIST
   single: Secure Hash Algorithm
   single: checksum; SHA

This module implements the interface to NIST's secure hash  algorithm, known as
SHA-1.  SHA-1 is an improved version of the original SHA hash algorithm.  It is
used in the same way as the :mod:`md5` module: use :func:`new` to create an sha
object, then feed this object with arbitrary strings using the :meth:`update`
method, and at any point you can ask it for the :dfn:`digest` of the
concatenation of the strings fed to it so far.  SHA-1 digests are 160 bits
instead of MD5's 128 bits.


.. function:: new([string])

   Return a new sha object.  If *string* is present, the method call
   ``update(string)`` is made.

The following values are provided as constants in the module and as attributes
of the sha objects returned by :func:`new`:


.. data:: blocksize

   Size of the blocks fed into the hash function; this is always ``1``.  This size
   is used to allow an arbitrary string to be hashed.


.. data:: digest_size

   The size of the resulting digest in bytes.  This is always ``20``.

An sha object has the same methods as md5 objects:


.. method:: sha.update(arg)

   Update the sha object with the string *arg*.  Repeated calls are equivalent to a
   single call with the concatenation of all the arguments: ``m.update(a);
   m.update(b)`` is equivalent to ``m.update(a+b)``.


.. method:: sha.digest()

   Return the digest of the strings passed to the :meth:`update` method so far.
   This is a 20-byte string which may contain non-ASCII characters, including null
   bytes.


.. method:: sha.hexdigest()

   Like :meth:`digest` except the digest is returned as a string of length 40,
   containing only hexadecimal digits.  This may  be used to exchange the value
   safely in email or other non-binary environments.


.. method:: sha.copy()

   Return a copy ("clone") of the sha object.  This can be used to efficiently
   compute the digests of strings that share a common initial substring.


.. seealso::

   `Secure Hash Standard <http://csrc.nist.gov/publications/fips/fips180-2/fips180-2withchangenotice.pdf>`_
      The Secure Hash Algorithm is defined by NIST document FIPS PUB 180-2: `Secure
      Hash Standard
      <http://csrc.nist.gov/publications/fips/fips180-2/fips180-2withchangenotice.pdf>`_,
      published in August 2002.

   `Cryptographic Toolkit (Secure Hashing) <http://csrc.nist.gov/encryption/tkhash.html>`_
      Links from NIST to various information on secure hashing.

