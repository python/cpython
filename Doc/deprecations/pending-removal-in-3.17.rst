Pending removal in Python 3.17
------------------------------

* :mod:`collections.abc`:

  - :class:`collections.abc.ByteString` is scheduled for removal in Python 3.17.

    ``ByteString`` was originally intended to be an abstract type that would serve as a
    supertype of both :class:`bytes` and :class:`bytearray`, but its semantics were never
    clearly specified, and it was never understood properly by type checkers. See
    :pep:`PEP 688 <688#current-options>` for more details.

    Prefer :class:`~collections.abc.Sequence` or :class:`~collections.abc.Buffer`. For
    use in type annotations, prefer a union, like ``bytes | bytearray``, or
    :class:`collections.abc.Buffer`. (Contributed by Shantanu Jain in :gh:`91896`.)

* :mod:`typing`:

  - Before Python 3.14, old-style unions were implemented using the private class
    ``typing._UnionGenericAlias``. This class is no longer needed for the implementation,
    but it has been retained for backward compatibility, with removal scheduled for Python
    3.17. Users should use documented introspection helpers like :func:`typing.get_origin`
    and :func:`typing.get_args` instead of relying on private implementation details.
  - :class:`typing.ByteString`, deprecated since Python 3.9, is scheduled for removal in
    Python 3.17.

   ``ByteString`` was originally intended to be an abstract type that would serve as a
    supertype of both :class:`bytes` and :class:`bytearray`, but its semantics were never
    clearly specified, and it was never understood properly by type checkers. See
    :pep:`PEP 688 <688#current-options>` for more details.

    Prefer :class:`~collections.abc.Sequence` or :class:`~collections.abc.Buffer`. For
    use in type annotations, prefer a union, like ``bytes | bytearray``, or
    :class:`collections.abc.Buffer`.
    (Contributed by Shantanu Jain in :gh:`91896`.)
