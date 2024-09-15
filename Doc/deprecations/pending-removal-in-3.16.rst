Pending Removal in Python 3.16
------------------------------

* :mod:`array`:
  :class:`array.array` ``'u'`` type (:c:type:`wchar_t`):
  use the ``'w'`` type instead (``Py_UCS4``).

* :mod:`builtins`:
  ``~bool``, bitwise inversion on bool.

* :mod:`symtable`:
  Deprecate :meth:`symtable.Class.get_methods` due to the lack of interest.
  (Contributed by Bénédikt Tran in :gh:`119698`.)
