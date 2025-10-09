Pending removal in Python 3.19
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* The following private functions are deprecated
  and planned for removal in Python 3.19:

  * :c:func:`!_PyType_Lookup`: use :c:func:`PyType_Lookup`.
  * :c:func:`!_PyType_LookupRef`: use :c:func:`PyType_Lookup`.

  The `pythoncapi-compat project
  <https://github.com/python/pythoncapi-compat/>`__ can be used to get
  these new public functions on Python 3.14 and older.
  (Contributed by Victor Stinner in :gh:`128863`.)
