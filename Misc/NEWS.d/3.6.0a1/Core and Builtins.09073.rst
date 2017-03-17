Issue #25843: When compiling code, don't merge constants if they are equal
but have a different types. For example, ``f1, f2 = lambda: 1, lambda: 1.0``
is now correctly compiled to two different functions: ``f1()`` returns ``1``
(``int``) and ``f2()`` returns ``1.0`` (``float``), even if ``1`` and ``1.0``
are equal.