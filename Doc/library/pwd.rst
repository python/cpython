:mod:`!pwd` --- The password database
=====================================

.. module:: pwd
   :platform: Unix
   :synopsis: The password database (getpwnam() and friends).

--------------

This module provides access to the Unix user account and password database.  It
is available on all Unix versions.

.. availability:: Unix, not WASI, not iOS.

Password database entries are reported as a tuple-like object, whose attributes
correspond to the members of the ``passwd`` structure (Attribute field below,
see ``<pwd.h>``):

+-------+---------------+-----------------------------+
| Index | Attribute     | Meaning                     |
+=======+===============+=============================+
| 0     | ``pw_name``   | Login name                  |
+-------+---------------+-----------------------------+
| 1     | ``pw_passwd`` | Optional encrypted password |
+-------+---------------+-----------------------------+
| 2     | ``pw_uid``    | Numerical user ID           |
+-------+---------------+-----------------------------+
| 3     | ``pw_gid``    | Numerical group ID          |
+-------+---------------+-----------------------------+
| 4     | ``pw_gecos``  | User name or comment field  |
+-------+---------------+-----------------------------+
| 5     | ``pw_dir``    | User home directory         |
+-------+---------------+-----------------------------+
| 6     | ``pw_shell``  | User command interpreter    |
+-------+---------------+-----------------------------+

The uid and gid items are integers, all others are strings. :exc:`KeyError` is
raised if the entry asked for cannot be found.

.. note::

   In traditional Unix the field ``pw_passwd`` usually contains a password
   encrypted with a DES derived algorithm.  However most
   modern unices  use a so-called *shadow password* system.  On those unices the
   *pw_passwd* field only contains an asterisk (``'*'``) or the  letter ``'x'``
   where the encrypted password is stored in a file :file:`/etc/shadow` which is
   not world readable.  Whether the *pw_passwd* field contains anything useful is
   system-dependent.

It defines the following items:


.. function:: getpwuid(uid)

   Return the password database entry for the given numeric user ID.


.. function:: getpwnam(name)

   Return the password database entry for the given user name.


.. function:: getpwall()

   Return a list of all available password database entries, in arbitrary order.


.. seealso::

   Module :mod:`grp`
      An interface to the group database, similar to this.
