The :mod:`!compression` package
===============================

.. versionadded:: 3.14

.. attention::

   The :mod:`!compression` package is the new location for the data compression
   modules in the standard library, listed below. The existing modules are not
   deprecated and will not be removed before Python 3.19. The new ``compression.*``
   import names are encouraged for use where practical.

* :mod:`!compression.bz2` -- Re-exports :mod:`bz2`
* :mod:`!compression.gzip` -- Re-exports :mod:`gzip`
* :mod:`!compression.lzma` -- Re-exports :mod:`lzma`
* :mod:`!compression.zlib` -- Re-exports :mod:`zlib`
* :mod:`compression.zstd` -- Wrapper for the Zstandard compression library

