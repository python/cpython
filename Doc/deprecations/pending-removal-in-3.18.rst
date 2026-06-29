Pending removal in Python 3.18
------------------------------

* No longer accept a boolean value when a file descriptor is expected.
  (Contributed by Serhiy Storchaka in :gh:`82626`.)

* :mod:`decimal`:

  * The non-standard and undocumented :class:`~decimal.Decimal` format
    specifier ``'N'``, which is only supported in the :mod:`!decimal` module's
    C implementation, has been deprecated since Python 3.13.
    (Contributed by Serhiy Storchaka in :gh:`89902`.)

* :mod:`tkinter`:

  * :func:`tkinter.filedialog.askopenfiles` has been deprecated since Python
    3.16.  Iterate over the names returned by
    :func:`~tkinter.filedialog.askopenfilenames` and open them one by one
    instead.
    (Contributed by Serhiy Storchaka in :gh:`152638`.)

* Deprecations defined by :pep:`829`:

  * ``import`` lines in :file:`{name}.pth` files are silently ignored.

  (Contributed by Barry Warsaw in :gh:`148641`.)
