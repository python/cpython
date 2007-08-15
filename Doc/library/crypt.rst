
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
page for further details.  Possible uses include allowing Python scripts to
accept typed passwords from the user, or attempting to crack Unix passwords with
a dictionary.

.. index:: single: crypt(3)

Notice that the behavior of this module depends on the actual implementation  of
the :manpage:`crypt(3)` routine in the running system.  Therefore, any
extensions available on the current implementation will also  be available on
this module.


.. function:: crypt(word, salt)

   *word* will usually be a user's password as typed at a prompt or  in a graphical
   interface.  *salt* is usually a random two-character string which will be used
   to perturb the DES algorithm in one of 4096 ways.  The characters in *salt* must
   be in the set ``[./a-zA-Z0-9]``.  Returns the hashed password as a string, which
   will be composed of characters from the same alphabet as the salt (the first two
   characters represent the salt itself).

   .. index:: single: crypt(3)

   Since a few :manpage:`crypt(3)` extensions allow different values, with
   different sizes in the *salt*, it is recommended to use  the full crypted
   password as salt when checking for a password.

A simple example illustrating typical use::

   import crypt, getpass, pwd

   def raw_input(prompt):
       import sys
       sys.stdout.write(prompt)
       sys.stdout.flush()
       return sys.stdin.readline()

   def login():
       username = raw_input('Python login:')
       cryptedpasswd = pwd.getpwnam(username)[1]
       if cryptedpasswd:
           if cryptedpasswd == 'x' or cryptedpasswd == '*': 
               raise "Sorry, currently no support for shadow passwords"
           cleartext = getpass.getpass()
           return crypt.crypt(cleartext, cryptedpasswd) == cryptedpasswd
       else:
           return 1

