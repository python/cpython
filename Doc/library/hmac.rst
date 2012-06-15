:mod:`hmac` --- Keyed-Hashing for Message Authentication
========================================================

.. module:: hmac
   :synopsis: Keyed-Hashing for Message Authentication (HMAC) implementation
              for Python.
.. moduleauthor:: Gerhard Häring <ghaering@users.sourceforge.net>
.. sectionauthor:: Gerhard Häring <ghaering@users.sourceforge.net>

**Source code:** :source:`Lib/hmac.py`

--------------

This module implements the HMAC algorithm as described by :rfc:`2104`.


.. function:: new(key, msg=None, digestmod=None)

   Return a new hmac object.  *key* is a bytes object giving the secret key.  If
   *msg* is present, the method call ``update(msg)`` is made.  *digestmod* is
   the digest constructor or module for the HMAC object to use. It defaults to
   the :func:`hashlib.md5` constructor.


An HMAC object has the following methods:

.. method:: HMAC.update(msg)

   Update the hmac object with the bytes object *msg*.  Repeated calls are
   equivalent to a single call with the concatenation of all the arguments:
   ``m.update(a); m.update(b)`` is equivalent to ``m.update(a + b)``.


.. method:: HMAC.digest()

   Return the digest of the bytes passed to the :meth:`update` method so far.
   This bytes object will be the same length as the *digest_size* of the digest
   given to the constructor.  It may contain non-ASCII bytes, including NUL
   bytes.

   .. warning::

      When comparing the output of :meth:`digest` to an externally-supplied
      digest during a verification routine, it is recommended to use the
      :func:`compare_digest` function instead of the ``==`` operator
      to reduce the vulnerability to timing attacks.


.. method:: HMAC.hexdigest()

   Like :meth:`digest` except the digest is returned as a string twice the
   length containing only hexadecimal digits.  This may be used to exchange the
   value safely in email or other non-binary environments.

   .. warning::

      The output of :meth:`hexdigest` should not be compared directly to an
      externally-supplied digest during a verification routine. Instead, the
      externally supplied digest should be converted to a :class:`bytes`
      value and compared to the output of :meth:`digest` with
      :func:`compare_digest`.


.. method:: HMAC.copy()

   Return a copy ("clone") of the hmac object.  This can be used to efficiently
   compute the digests of strings that share a common initial substring.


This module also provides the following helper function:

.. function:: compare_digest(a, b)

   Returns the equivalent of ``a == b``, but avoids content based
   short circuiting behaviour to reduce the vulnerability to timing
   analysis. The inputs must be :class:`bytes` instances.

   Using a short circuiting comparison (that is, one that terminates as soon
   as it finds any difference between the values) to check digests for
   correctness can be problematic, as it introduces a potential
   vulnerability when an attacker can control both the message to be checked
   *and* the purported signature value. By keeping the plaintext consistent
   and supplying different signature values, an attacker may be able to use
   timing variations to search the signature space for the expected value in
   O(n) time rather than the desired O(2**n).

   .. note::

      While this function reduces the likelihood of leaking the contents of
      the expected digest via a timing attack, it still uses short circuiting
      behaviour based on the *length* of the inputs. It is assumed that the
      expected length of the digest is not a secret, as it is typically
      published as part of a file format, network protocol or API definition.

   .. versionadded:: 3.3

.. seealso::

   Module :mod:`hashlib`
      The Python module providing secure hash functions.
