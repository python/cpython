Pending removal in Python 3.18
------------------------------

* :mod:`decimal`:

  * The non-standard and undocumented :class:`~decimal.Decimal` format
    specifier ``'N'``, which is only supported in the :mod:`!decimal` module's
    C implementation, has been deprecated since Python 3.13.
    (Contributed by Serhiy Storchaka in :gh:`89902`.)
