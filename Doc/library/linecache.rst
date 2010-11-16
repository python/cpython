:mod:`linecache` --- Random access to text lines
================================================

.. module:: linecache
   :synopsis: This module provides random access to individual lines from text files.
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>


The :mod:`linecache` module allows one to get any line from any file, while
attempting to optimize internally, using a cache, the common case where many
lines are read from a single file.  This is used by the :mod:`traceback` module
to retrieve source lines for inclusion in  the formatted traceback.

.. seealso::

   Latest version of the :source:`linecache module Python source code
   <Lib/linecache.py>`

The :mod:`linecache` module defines the following functions:


.. function:: getline(filename, lineno, module_globals=None)

   Get line *lineno* from file named *filename*. This function will never raise an
   exception --- it will return ``''`` on errors (the terminating newline character
   will be included for lines that are found).

   .. index:: triple: module; search; path

   If a file named *filename* is not found, the function will look for it in the
   module search path, ``sys.path``, after first checking for a :pep:`302`
   ``__loader__`` in *module_globals*, in case the module was imported from a
   zipfile or other non-filesystem import source.


.. function:: clearcache()

   Clear the cache.  Use this function if you no longer need lines from files
   previously read using :func:`getline`.


.. function:: checkcache(filename=None)

   Check the cache for validity.  Use this function if files in the cache  may have
   changed on disk, and you require the updated version.  If *filename* is omitted,
   it will check all the entries in the cache.


Example::

   >>> import linecache
   >>> linecache.getline('/etc/passwd', 4)
   'sys:x:3:3:sys:/dev:/bin/sh\n'

