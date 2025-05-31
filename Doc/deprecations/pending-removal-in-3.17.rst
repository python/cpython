Pending removal in Python 3.17
------------------------------

* :mod:`hashlib`:

  - Supporting the undocumented ``string`` keyword parameter in :func:`~hashlib.new`
    and hash function constructors such as :func:`~hashlib.md5` is deprecated.
    Use the ``data`` keyword parameter instead, or simply pass the data to hash
    as a positional argument.

    Before Python 3.13, the ``string`` keyword parameter was not correctly
    supported depending on the backend implementation of hash functions, and
    users should prefer passing the data to hash as a positional argument.

* :mod:`typing`:

  - Before Python 3.14, old-style unions were implemented using the private class
    ``typing._UnionGenericAlias``. This class is no longer needed for the implementation,
    but it has been retained for backward compatibility, with removal scheduled for Python
    3.17. Users should use documented introspection helpers like :func:`typing.get_origin`
    and :func:`typing.get_args` instead of relying on private implementation details.
