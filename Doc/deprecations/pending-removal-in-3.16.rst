Pending Removal in Python 3.16
------------------------------

* The import system:

  * Setting :attr:`~module.__loader__` on a module while
    failing to set :attr:`__spec__.loader <importlib.machinery.ModuleSpec.loader>`
    is deprecated. In Python 3.16, :attr:`!__loader__` will cease to be set or
    taken into consideration by the import system or the standard library.

* :mod:`array`:
  :class:`array.array` ``'u'`` type (:c:type:`wchar_t`):
  use the ``'w'`` type instead (``Py_UCS4``).

* :mod:`builtins`:
  ``~bool``, bitwise inversion on bool.

* :mod:`symtable`:
  Deprecate :meth:`symtable.Class.get_methods` due to the lack of interest.
  (Contributed by Bénédikt Tran in :gh:`119698`.)
