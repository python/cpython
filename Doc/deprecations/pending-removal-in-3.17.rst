Pending removal in Python 3.17
------------------------------

* :mod:`collections.abc`:

  - :class:`collections.abc.ByteString` is scheduled for removal in Python 3.17.

    Use ``isinstance(obj, collections.abc.Buffer)`` to test if ``obj``
    implements the :ref:`buffer protocol <bufferobjects>` at runtime. For use
    in type annotations, either use :class:`~collections.abc.Buffer` or a union
    that explicitly specifies the types your code supports (e.g.,
    ``bytes | bytearray | memoryview``).

    :class:`!ByteString` was originally intended to be an abstract class that
    would serve as a supertype of both :class:`bytes` and :class:`bytearray`.
    However, since the ABC never had any methods, knowing that an object was an
    instance of :class:`!ByteString` never actually told you anything useful
    about the object. Other common buffer types such as :class:`memoryview`
    were also never understood as subtypes of :class:`!ByteString` (either at
    runtime or by static type checkers).

    See :pep:`PEP 688 <688#current-options>` for more details.
    (Contributed by Shantanu Jain in :gh:`91896`.)


* :mod:`encodings`:

  - Passing non-ascii *encoding* names to :func:`encodings.normalize_encoding`
    is deprecated and scheduled for removal in Python 3.17.
    (Contributed by Stan Ulbrych in :gh:`136702`)

* :mod:`typing`:

  - Before Python 3.14, old-style unions were implemented using the private class
    ``typing._UnionGenericAlias``. This class is no longer needed for the implementation,
    but it has been retained for backward compatibility, with removal scheduled for Python
    3.17. Users should use documented introspection helpers like :func:`typing.get_origin`
    and :func:`typing.get_args` instead of relying on private implementation details.
  - :class:`typing.ByteString`, deprecated since Python 3.9, is scheduled for removal in
    Python 3.17.

    Use ``isinstance(obj, collections.abc.Buffer)`` to test if ``obj``
    implements the :ref:`buffer protocol <bufferobjects>` at runtime. For use
    in type annotations, either use :class:`~collections.abc.Buffer` or a union
    that explicitly specifies the types your code supports (e.g.,
    ``bytes | bytearray | memoryview``).

    :class:`!ByteString` was originally intended to be an abstract class that
    would serve as a supertype of both :class:`bytes` and :class:`bytearray`.
    However, since the ABC never had any methods, knowing that an object was an
    instance of :class:`!ByteString` never actually told you anything useful
    about the object. Other common buffer types such as :class:`memoryview`
    were also never understood as subtypes of :class:`!ByteString` (either at
    runtime or by static type checkers).

    See :pep:`PEP 688 <688#current-options>` for more details.
    (Contributed by Shantanu Jain in :gh:`91896`.)
