:mod:`packaging.version` --- Version number classes
===================================================

.. module:: packaging.version
   :synopsis: Classes that represent project version numbers.


This module contains classes and functions useful to deal with version numbers.
It's an implementation of version specifiers `as defined in PEP 345
<http://www.python.org/dev/peps/pep-0345/#version-specifiers>`_.


Version numbers
---------------

.. class:: NormalizedVersion(self, s, error_on_huge_major_num=True)

   A specific version of a distribution, as described in PEP 345.  *s* is a
   string object containing the version number (for example ``'1.2b1'``),
   *error_on_huge_major_num* a boolean specifying whether to consider an
   apparent use of a year or full date as the major version number an error.

   The rationale for the second argument is that there were projects using years
   or full dates as version numbers, which could cause problems with some
   packaging systems sorting.

   Instances of this class can be compared and sorted::

      >>> NormalizedVersion('1.2b1') < NormalizedVersion('1.2')
      True

   :class:`NormalizedVersion` is used internally by :class:`VersionPredicate` to
   do its work.


.. class:: IrrationalVersionError

   Exception raised when an invalid string is given to
   :class:`NormalizedVersion`.

      >>> NormalizedVersion("irrational_version_number")
      ...
      IrrationalVersionError: irrational_version_number


.. function:: suggest_normalized_version(s)

   Before standardization in PEP 386, various schemes were in use.  Packaging
   provides a function to try to convert any string to a valid, normalized
   version::

      >>> suggest_normalized_version('2.1-rc1')
      2.1c1


   If :func:`suggest_normalized_version` can't make sense of the given string,
   it will return ``None``::

      >>> print(suggest_normalized_version('not a version'))
      None


Version predicates
------------------

.. class:: VersionPredicate(predicate)

   This class deals with the parsing of field values like
   ``ProjectName (>=version)``.

   .. method:: match(version)

      Test if a version number matches the predicate:

         >>> version = VersionPredicate("ProjectName (<1.2, >1.0)")
         >>> version.match("1.2.1")
         False
         >>> version.match("1.1.1")
         True


Validation helpers
------------------

If you want to use :term:`LBYL`-style checks instead of instantiating the
classes and catching :class:`IrrationalVersionError` and :class:`ValueError`,
you can use these functions:

.. function:: is_valid_version(predicate)

   Check whether the given string is a valid version number.  Example of valid
   strings: ``'1.2'``,  ``'4.2.0.dev4'``, ``'2.5.4.post2'``.


.. function:: is_valid_versions(predicate)

   Check whether the given string is a valid value for specifying multiple
   versions, such as in the Requires-Python field.  Example: ``'2.7, >=3.2'``.


.. function:: is_valid_predicate(predicate)

   Check whether the given string is a valid version predicate.  Examples:
   ``'some.project == 4.5, <= 4.7'``, ``'speciallib (> 1.0, != 1.4.2, < 2.0)'``.
