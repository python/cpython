:mod:`hmac` --- Keyed-Hashing for Message Authentication
========================================================

.. module:: hmac
   :synopsis: Keyed-Hashing for Message Authentication (HMAC) implementation for Python.
.. moduleauthor:: Gerhard Häring <ghaering@users.sourceforge.net>
.. sectionauthor:: Gerhard Häring <ghaering@users.sourceforge.net>


.. versionadded:: 2.2

**Source code:** :source:`Lib/hmac.py`

--------------

This module implements the HMAC algorithm as described by :rfc:`2104`.


.. function:: new(key[, msg[, digestmod]])

   Return a new hmac object.  If *msg* is present, the method call ``update(msg)``
   is made. *digestmod* is the digest constructor or module for the HMAC object to
   use. It defaults to  the :data:`hashlib.md5` constructor.


An HMAC object has the following methods:

.. method:: HMAC.update(msg)

   Update the hmac object with the string *msg*.  Repeated calls are equivalent to
   a single call with the concatenation of all the arguments: ``m.update(a);
   m.update(b)`` is equivalent to ``m.update(a + b)``.


.. method:: HMAC.digest()

   Return the digest of the strings passed to the :meth:`update` method so far.
   This string will be the same length as the *digest_size* of the digest given to
   the constructor.  It may contain non-ASCII characters, including NUL bytes.


.. method:: HMAC.hexdigest()

   Like :meth:`digest` except the digest is returned as a string twice the length
   containing only hexadecimal digits.  This may be used to exchange the value
   safely in email or other non-binary environments.


.. method:: HMAC.copy()

   Return a copy ("clone") of the hmac object.  This can be used to efficiently
   compute the digests of strings that share a common initial substring.


.. seealso::

   Module :mod:`hashlib`
      The Python module providing secure hash functions.

