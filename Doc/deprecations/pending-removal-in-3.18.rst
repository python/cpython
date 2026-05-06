Pending removal in Python 3.18
------------------------------

* No longer accept a boolean value when a file descriptor is expected.
  (Contributed by Serhiy Storchaka in :gh:`82626`.)

* :mod:`decimal`:

  * The non-standard and undocumented :class:`~decimal.Decimal` format
    specifier ``'N'``, which is only supported in the :mod:`!decimal` module's
    C implementation, has been deprecated since Python 3.13.
    (Contributed by Serhiy Storchaka in :gh:`89902`.)
