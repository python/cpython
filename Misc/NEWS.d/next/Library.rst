bdb: Fix :meth:`bdb.Bdb.clear_all_file_breaks` to not skip breakpoints when multiple
breakpoints share the same file and line. Patch by Aman Sachan.

.. bpo-149015.
