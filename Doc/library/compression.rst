The :mod:`!compression` package
=================================

.. versionadded:: 3.14

.. note::

    Several modules in :mod:`!compression` re-export modules that currently
    exist at the repository top-level. These re-exported modules are the new
    canonical import name for the respective top-level module. The existing
    modules are not currently deprecated and will not be removed prior to Python
    3.19, but users are encouraged to migrate to the new import names when
    feasible.

* :mod:`!compression.bz2` -- Re-exports :mod:`bz2`

* :mod:`!compression.gzip` -- Re-exports :mod:`gzip`

* :mod:`!compression.lzma` -- Re-exports :mod:`lzma`

* :mod:`!compression.zlib` -- Re-exports :mod:`zlib`

* :mod:`compression.zstd` -- Wrapper for the Zstandard compression library


