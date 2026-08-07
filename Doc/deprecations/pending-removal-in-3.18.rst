Pending removal in Python 3.18
------------------------------

* No longer accept a boolean value when a file descriptor is expected.
  (Contributed by Serhiy Storchaka in :gh:`82626`.)

* :mod:`decimal`:

  * The non-standard and undocumented :class:`~decimal.Decimal` format
    specifier ``'N'``, which is only supported in the :mod:`!decimal` module's
    C implementation, has been deprecated since Python 3.13.
    (Contributed by Serhiy Storchaka in :gh:`89902`.)

* Deprecations defined by :pep:`829`:

  * ``import`` lines in :file:`{name}.pth` files are silently ignored.

  (Contributed by Barry Warsaw in :gh:`148641`.)

* :mod:`http.client`:

  * :meth:`!http.client.HTTPMessage.getallmatchingheaders` has been deprecated
    since Python 3.16.  It has returned an empty list for every input since
    Python 3.0; use :meth:`email.message.Message.get_all` instead.
    (Contributed by Julian Soreavis in :gh:`153648`.)
