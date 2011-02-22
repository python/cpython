:mod:`crypt` --- Function to check Unix passwords
=================================================

.. module:: crypt
   :platform: Unix
   :synopsis: The crypt() function used to check Unix passwords.
.. moduleauthor:: Steven D. Majewski <sdm7g@virginia.edu>
.. sectionauthor:: Steven D. Majewski <sdm7g@virginia.edu>
.. sectionauthor:: Peter Funk <pf@artcom-gmbh.de>


.. index::
   single: crypt(3)
   pair: cipher; DES

This module implements an interface to the :manpage:`crypt(3)` routine, which is
a one-way hash function based upon a modified DES algorithm; see the Unix man
page for further details.  Possible uses include storing hashed passwords
so you can check passwords without storing the actual password, or attempting
to crack Unix passwords with a dictionary.

.. index:: single: crypt(3)

Notice that the behavior of this module depends on the actual implementation  of
the :manpage:`crypt(3)` routine in the running system.  Therefore, any
extensions available on the current implementation will also  be available on
this module.

Hashing Methods
---------------

The :mod:`crypt` module defines the list of hashing methods (not all methods
are available on all platforms):

.. data:: METHOD_SHA512

   A Modular Crypt Format method with 16 character salt and 86 character
   hash.  This is the strongest method.

.. versionadded:: 3.3

.. data:: METHOD_SHA256

   Another Modular Crypt Format method with 16 character salt and 43
   character hash.

.. versionadded:: 3.3

.. data:: METHOD_MD5

   Another Modular Crypt Format method with 8 character salt and 22
   character hash.

.. versionadded:: 3.3

.. data:: METHOD_CRYPT

   The traditional method with a 2 character salt and 13 characters of
   hash.  This is the weakest method.

.. versionadded:: 3.3


Module Attributes
-----------------


.. attribute:: methods

   A list of available password hashing algorithms, as
   ``crypt.METHOD_*`` objects.  This list is sorted from strongest to
   weakest, and is guaranteed to have at least ``crypt.METHOD_CRYPT``.

.. versionadded:: 3.3


Module Functions
----------------

The :mod:`crypt` module defines the following functions:

.. function:: crypt(word, salt=None)

   *word* will usually be a user's password as typed at a prompt or  in a graphical
   interface.  The optional *salt* is either a string as returned from
   :func:`mksalt`, one of the ``crypt.METHOD_*`` values (though not all
   may be available on all platforms), or a full encrypted password
   including salt, as returned by this function.  If *salt* is not
   provided, the strongest method will be used (as returned by
   :func:`methods`.

   Checking a password is usually done by passing the plain-text password
   as *word* and the full results of a previous :func:`crypt` call,
   which should be the same as the results of this call.

   *salt* (either a random 2 or 16 character string, possibly prefixed with
   ``$digit$`` to indicate the method) which will be used to perturb the
   encryption algorithm.  The characters in *salt* must be in the set
   ``[./a-zA-Z0-9]``, with the exception of Modular Crypt Format which
   prefixes a ``$digit$``.

   Returns the hashed password as a string, which will be composed of
   characters from the same alphabet as the salt.

   .. index:: single: crypt(3)

   Since a few :manpage:`crypt(3)` extensions allow different values, with
   different sizes in the *salt*, it is recommended to use  the full crypted
   password as salt when checking for a password.

.. versionchanged:: 3.3
   Before version 3.3, *salt*  must be specified as a string and cannot
   accept ``crypt.METHOD_*`` values (which don't exist anyway).


.. function:: mksalt(method=None)

   Return a randomly generated salt of the specified method.  If no
   *method* is given, the strongest method available as returned by
   :func:`methods` is used.

   The return value is a string either of 2 characters in length for
   ``crypt.METHOD_CRYPT``, or 19 characters starting with ``$digit$`` and
   16 random characters from the set ``[./a-zA-Z0-9]``, suitable for
   passing as the *salt* argument to :func:`crypt`.

.. versionadded:: 3.3

Examples
--------

A simple example illustrating typical use::

   import crypt, getpass, pwd

   def login():
       username = input('Python login:')
       cryptedpasswd = pwd.getpwnam(username)[1]
       if cryptedpasswd:
           if cryptedpasswd == 'x' or cryptedpasswd == '*':
               raise "Sorry, currently no support for shadow passwords"
           cleartext = getpass.getpass()
           return crypt.crypt(cleartext, cryptedpasswd) == cryptedpasswd
       else:
           return 1

To generate a hash of a password using the strongest available method and
check it against the original::

   import crypt

   hashed = crypt.crypt(plaintext)
   if hashed != crypt.crypt(plaintext, hashed):
      raise "Hashed version doesn't validate against original"
