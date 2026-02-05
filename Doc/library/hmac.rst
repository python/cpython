:mod:`!hmac` --- Keyed-Hashing for Message Authentication
=========================================================

.. module:: hmac
   :synopsis: Keyed-Hashing for Message Authentication (HMAC) implementation

.. moduleauthor:: Gerhard Häring <ghaering@users.sourceforge.net>
.. sectionauthor:: Gerhard Häring <ghaering@users.sourceforge.net>

**Source code:** :source:`Lib/hmac.py`

--------------

This module implements the HMAC algorithm as described by :rfc:`2104`.
The interface allows to use any hash function with a *fixed* digest size.
In particular, extendable output functions such as SHAKE-128 or SHAKE-256
cannot be used with HMAC.


.. function:: new(key, msg=None, digestmod)

   Return a new hmac object.  *key* is a bytes or bytearray object giving the
   secret key.  If *msg* is present, the method call ``update(msg)`` is made.
   *digestmod* is the digest name, digest constructor or module for the HMAC
   object to use.  It may be any name suitable to :func:`hashlib.new`.
   Despite its argument position, it is required.

   .. versionchanged:: 3.4
      Parameter *key* can be a bytes or bytearray object.
      Parameter *msg* can be of any type supported by :mod:`hashlib`.
      Parameter *digestmod* can be the name of a hash algorithm.

   .. versionchanged:: 3.8
      The *digestmod* argument is now required.  Pass it as a keyword
      argument to avoid awkwardness when you do not have an initial *msg*.


.. function:: digest(key, msg, digest)

   Return digest of *msg* for given secret *key* and *digest*. The
   function is equivalent to ``HMAC(key, msg, digest).digest()``, but
   uses an optimized C or inline implementation, which is faster for messages
   that fit into memory. The parameters *key*, *msg*, and *digest* have
   the same meaning as in :func:`~hmac.new`.

   CPython implementation detail, the optimized C implementation is only used
   when *digest* is a string and name of a digest algorithm, which is
   supported by OpenSSL.

   .. versionadded:: 3.7


.. class:: HMAC

   An HMAC object has the following methods:

.. method:: HMAC.update(msg)

   Update the hmac object with *msg*.  Repeated calls are equivalent to a
   single call with the concatenation of all the arguments:
   ``m.update(a); m.update(b)`` is equivalent to ``m.update(a + b)``.

   .. versionchanged:: 3.4
      Parameter *msg* can be of any type supported by :mod:`hashlib`.


.. method:: HMAC.digest()

   Return the digest of the bytes passed to the :meth:`update` method so far.
   This bytes object will be the same length as the *digest_size* of the digest
   given to the constructor.  It may contain non-ASCII bytes, including NUL
   bytes.

   .. warning::

      When comparing the output of :meth:`digest` to an externally supplied
      digest during a verification routine, it is recommended to use the
      :func:`compare_digest` function instead of the ``==`` operator
      to reduce the vulnerability to timing attacks.


.. method:: HMAC.hexdigest()

   Like :meth:`digest` except the digest is returned as a string twice the
   length containing only hexadecimal digits.  This may be used to exchange the
   value safely in email or other non-binary environments.

   .. warning::

      When comparing the output of :meth:`hexdigest` to an externally supplied
      digest during a verification routine, it is recommended to use the
      :func:`compare_digest` function instead of the ``==`` operator
      to reduce the vulnerability to timing attacks.


.. method:: HMAC.copy()

   Return a copy ("clone") of the hmac object.  This can be used to efficiently
   compute the digests of strings that share a common initial substring.


A hash object has the following attributes:

.. attribute:: HMAC.digest_size

   The size of the resulting HMAC digest in bytes.

.. attribute:: HMAC.block_size

   The internal block size of the hash algorithm in bytes.

   .. versionadded:: 3.4

.. attribute:: HMAC.name

   The canonical name of this HMAC, always lowercase, e.g. ``hmac-md5``.

   .. versionadded:: 3.4


.. versionchanged:: 3.10
   Removed the undocumented attributes ``HMAC.digest_cons``, ``HMAC.inner``,
   and ``HMAC.outer``.

This module also provides the following helper function:

.. function:: compare_digest(a, b)

   Return ``a == b``.  This function uses an approach designed to prevent
   timing analysis by avoiding content-based short circuiting behaviour,
   making it appropriate for cryptography.  *a* and *b* must both be of the
   same type: either :class:`str` (ASCII only, as e.g. returned by
   :meth:`HMAC.hexdigest`), or a :term:`bytes-like object`.

   .. note::

      If *a* and *b* are of different lengths, or if an error occurs,
      a timing attack could theoretically reveal information about the
      types and lengths of *a* and *b*—but not their values.

   .. versionadded:: 3.3

   .. versionchanged:: 3.10

      The function uses OpenSSL's ``CRYPTO_memcmp()`` internally when
      available.


.. seealso::

   Module :mod:`hashlib`
      The Python module providing secure hash functions.
