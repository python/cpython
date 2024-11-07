Pending Removal in Python 3.16
------------------------------

* :mod:`array`:
  :class:`array.array` ``'u'`` type (:c:type:`wchar_t`):
  use the ``'w'`` type instead (``Py_UCS4``).

* :mod:`builtins`:
  ``~bool``, bitwise inversion on bool.

* :mod:`socket`:
  :class:`socket.SocketType` use :class:`socket.socket` type instead.
  (Contributed by James Roy in :gh:`88427`.)

* :mod:`symtable`:
  Deprecate :meth:`symtable.Class.get_methods` due to the lack of interest.
  (Contributed by Bénédikt Tran in :gh:`119698`.)

* :mod:`shutil`: Deprecate :class:`!shutil.ExecError`, which hasn't
  been raised by any :mod:`!shutil` function since Python 3.4. It's
  now an alias for :exc:`RuntimeError`.

