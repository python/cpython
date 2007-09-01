
:mod:`spwd` --- The shadow password database
============================================

.. module:: spwd
   :platform: Unix
   :synopsis: The shadow password database (getspnam() and friends).


This module provides access to the Unix shadow password database. It is
available on various Unix versions.

You must have enough privileges to access the shadow password database (this
usually means you have to be root).

Shadow password database entries are reported as a tuple-like object, whose
attributes correspond to the members of the ``spwd`` structure (Attribute field
below, see ``<shadow.h>``):

+-------+---------------+---------------------------------+
| Index | Attribute     | Meaning                         |
+=======+===============+=================================+
| 0     | ``sp_nam``    | Login name                      |
+-------+---------------+---------------------------------+
| 1     | ``sp_pwd``    | Encrypted password              |
+-------+---------------+---------------------------------+
| 2     | ``sp_lstchg`` | Date of last change             |
+-------+---------------+---------------------------------+
| 3     | ``sp_min``    | Minimal number of days between  |
|       |               | changes                         |
+-------+---------------+---------------------------------+
| 4     | ``sp_max``    | Maximum number of days between  |
|       |               | changes                         |
+-------+---------------+---------------------------------+
| 5     | ``sp_warn``   | Number of days before password  |
|       |               | expires to warn user about it   |
+-------+---------------+---------------------------------+
| 6     | ``sp_inact``  | Number of days after password   |
|       |               | expires until account is        |
|       |               | blocked                         |
+-------+---------------+---------------------------------+
| 7     | ``sp_expire`` | Number of days since 1970-01-01 |
|       |               | until account is disabled       |
+-------+---------------+---------------------------------+
| 8     | ``sp_flag``   | Reserved                        |
+-------+---------------+---------------------------------+

The sp_nam and sp_pwd items are strings, all others are integers.
:exc:`KeyError` is raised if the entry asked for cannot be found.

It defines the following items:


.. function:: getspnam(name)

   Return the shadow password database entry for the given user name.


.. function:: getspall()

   Return a list of all available shadow password database entries, in arbitrary
   order.


.. seealso::

   Module :mod:`grp`
      An interface to the group database, similar to this.

   Module :mod:`pwd`
      An interface to the normal password database, similar to this.

