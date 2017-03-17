Issue #21925: :func:`warnings.formatwarning` now catches exceptions when
calling :func:`linecache.getline` and
:func:`tracemalloc.get_object_traceback` to be able to log
:exc:`ResourceWarning` emitted late during the Python shutdown process.