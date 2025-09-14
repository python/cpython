Pending removal in Python 3.17
------------------------------

* :mod:`typing`:

  - Before Python 3.14, old-style unions were implemented using the private class
    ``typing._UnionGenericAlias``. This class is no longer needed for the implementation,
    but it has been retained for backward compatibility, with removal scheduled for Python
    3.17. Users should use documented introspection helpers like :func:`typing.get_origin`
    and :func:`typing.get_args` instead of relying on private implementation details.

* CLI:

  - The :option:`-R` option. This option is does not have an effect
    in most cases as hash randomization is enabled by default since Python 3.3.
    See :gh:`137897` and :meth:`~object.__hash__` for more information.
