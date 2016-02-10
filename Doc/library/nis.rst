
:mod:`nis` --- Interface to Sun's NIS (Yellow Pages)
====================================================

.. module:: nis
   :platform: Unix
   :synopsis: Interface to Sun's NIS (Yellow Pages) library.
.. moduleauthor:: Fred Gansevles <Fred.Gansevles@cs.utwente.nl>
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>


The :mod:`nis` module gives a thin wrapper around the NIS library, useful for
central administration of several hosts.

Because NIS exists only on Unix systems, this module is only available for Unix.

The :mod:`nis` module defines the following functions:


.. function:: match(key, mapname[, domain=default_domain])

   Return the match for *key* in map *mapname*, or raise an error
   (:exc:`nis.error`) if there is none. Both should be strings, *key* is 8-bit
   clean. Return value is an arbitrary array of bytes (may contain ``NULL`` and
   other joys).

   Note that *mapname* is first checked if it is an alias to another name.

   .. versionchanged:: 2.5
      The *domain* argument allows overriding the NIS domain used for the lookup. If
      unspecified, lookup is in the default NIS domain.


.. function:: cat(mapname[, domain=default_domain])

   Return a dictionary mapping *key* to *value* such that ``match(key,
   mapname)==value``. Note that both keys and values of the dictionary are
   arbitrary arrays of bytes.

   Note that *mapname* is first checked if it is an alias to another name.

   .. versionchanged:: 2.5
      The *domain* argument allows overriding the NIS domain used for the lookup. If
      unspecified, lookup is in the default NIS domain.


.. function:: maps([domain=default_domain])

   Return a list of all valid maps.

   .. versionchanged:: 2.5
      The *domain* argument allows overriding the NIS domain used for the lookup. If
      unspecified, lookup is in the default NIS domain.


.. function:: get_default_domain()

   Return the system default NIS domain.

   .. versionadded:: 2.5

The :mod:`nis` module defines the following exception:


.. exception:: error

   An error raised when a NIS function returns an error code.

