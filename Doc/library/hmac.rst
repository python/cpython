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
      :func:`hmac.secure_compare` function instead of the ``==`` operator
      to avoid potential timing attacks.


.. method:: HMAC.hexdigest()

   Like :meth:`digest` except the digest is returned as a string twice the
   length containing only hexadecimal digits.  This may be used to exchange the
   value safely in email or other non-binary environments.

   .. warning::

      When comparing the output of :meth:`hexdigest` to an externally-supplied
      digest during a verification routine, it is recommended to use the
      :func:`hmac.secure_compare` function instead of the ``==`` operator
      to avoid potential timing attacks.


.. method:: HMAC.copy()

   Return a copy ("clone") of the hmac object.  This can be used to efficiently
   compute the digests of strings that share a common initial substring.


This module also provides the following helper function:

.. function:: secure_compare(a, b)

   Returns the equivalent of ``a == b``, but using a time-independent
   comparison method. Comparing the full lengths of the inputs *a* and *b*,
   instead of short-circuiting the comparison upon the first unequal byte,
   prevents leaking information about the inputs being compared and mitigates
   potential timing attacks. The inputs must be either :class:`str` or
   :class:`bytes` instances.

   .. note::

      While the :func:`hmac.secure_compare` function prevents leaking the
      contents of the inputs via a timing attack, it does leak the length
      of the inputs. However, this generally is not a security risk.

   .. versionadded:: 3.3

.. seealso::

   Module :mod:`hashlib`
      The Python module providing secure hash functions.
