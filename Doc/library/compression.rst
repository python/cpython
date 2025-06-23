The :mod:`!compression` package
===============================

.. versionadded:: 3.14

The :mod:`!compression` package contains the canonical compression modules
containing interfaces to several different compression algorithms. Some of
these modules have historically been available as separate modules; those will
continue to be available under their original names for compatibility reasons,
and will not be removed without a deprecation cycle. The use of modules in
:mod:`!compression` is encouraged where practical.

* :mod:`!compression.bz2` -- Re-exports :mod:`bz2`
* :mod:`!compression.gzip` -- Re-exports :mod:`gzip`
* :mod:`!compression.lzma` -- Re-exports :mod:`lzma`
* :mod:`!compression.zlib` -- Re-exports :mod:`zlib`
* :mod:`compression.zstd` -- Wrapper for the Zstandard compression library

