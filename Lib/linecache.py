"""Cache lines from Python source files.

This is intended to read lines from modules imported -- hence if a filename
is not found, it will look down the module search path for a file by
that name.
"""

__all__ = ["getline", "clearcache", "checkcache", "lazycache"]


# The cache. Maps filenames to either a thunk which will provide source code,
# or a tuple (size, mtime, lines, fullname) once loaded.
#
# By construction, the filenames being stored are truthy.
cache = {}


# The filenames for which we failed to get a result and the reason.
#
# The value being stored is a tuple (size, mtime_ns, fullname),
# possibly size=None and st_mtime_ns=None if they are unavailable.
#
# By convention, falsey filenames are not cached and treated as failures.
failures = {}


def clearcache():
    """Clear the cache entirely."""
    cache.clear()
    failures.clear()


def getline(filename, lineno, module_globals=None):
    """Get a line for a Python source file from the cache.
    Update the cache if it doesn't contain an entry for this file already.
    Previous failures are cached and must be invalidated via checkcache().
    """

    if filename in failures:
        # We explicitly validate the input even for failures
        # because for success, they would raise a TypeError.
        if not isinstance(lineno, int):
            raise TypeError(f"'lineno' must be an int, "
                            f"got {type(module_globals)}")
        _validate_module_globals_type(module_globals)
        return ''

    lines = getlines(filename, module_globals)
    if 1 <= lineno <= len(lines):
        return lines[lineno - 1]
    return ''


def getlines(filename, module_globals=None):
    """Get the lines for a Python source file from the cache.
    Update the cache if it doesn't contain an entry for this file already.
    Previous failures are cached and must be invalidated via checkcache().
    """

    _validate_module_globals_type(module_globals)

    if filename in cache:
        assert filename not in failures
        entry = cache[filename]
        if len(entry) != 1:
            return cache[filename][2]

    if filename in failures:
        return []

    try:
        return updatecache(filename, module_globals)
    except MemoryError:
        clearcache()
        return []


def checkcache(filename=None):
    """Discard cache entries that are out of date or now available for reading.
    (This is not checked upon each call!).
    """

    if filename is None:
        filenames = list(cache.keys())
        failed_filenames = list(failures.keys())
    elif filename in cache:
        filenames = [filename]
        failed_filenames = []
    elif filename in failures:
        filenames = []
        failed_filenames = [filename]
    else:
        return

    for filename in filenames:
        entry = cache[filename]
        if len(entry) == 1:
            # lazy cache entry, leave it lazy.
            continue
        size, mtime, lines, fullname = entry
        if mtime is None:
            continue   # no-op for files loaded via a __loader__
        try:
            # This import can fail if the interpreter is shutting down
            import os
        except ImportError:
            return
        try:
            stat = os.stat(fullname)
        except OSError:
            # a cached entry is now a failure
            assert filename not in failed_filenames
            failures[filename] = (None, None, fullname)
            cache.pop(filename, None)
            continue
        if size != stat.st_size or mtime != stat.st_mtime:
            cache.pop(filename, None)

    for filename in failed_filenames:
        size, mtime_ns, fullname = failures[filename]
        try:
            # This import can fail if the interpreter is shutting down
            import os
        except ImportError:
            return
        try:
            stat = os.stat(fullname)
        except OSError:
            if size is not None and mtime_ns is not None:
                # Previous failure was a decoding error,
                # this failure is due to os.stat() error.
                failures[filename] = (None, None, fullname)
            continue  # still unreadable

        if size is None or mtime_ns is None:
            # we may now be able to read the file
            failures.pop(filename, None)
        elif size != stat.st_size or mtime_ns != stat.st_mtime_ns:
            # the file might have been updated
            failures.pop(filename, None)


def updatecache(filename, module_globals=None):
    """Update a cache entry and return its list of lines.

    If something's wrong, possibly print a message, discard the cache entry,
    add the file name to the known failures, and return an empty list.
    """

    # These imports are not at top level because linecache is in the critical
    # path of the interpreter startup and importing os and sys take a lot of time
    # and slows down the startup sequence.
    import os
    import sys
    import tokenize

    _validate_module_globals_type(module_globals)

    if filename in cache:
        if len(cache[filename]) != 1:
            cache.pop(filename, None)
    if not filename or (filename.startswith('<') and filename.endswith('>')):
        return []

    fullname = filename
    try:
        stat = os.stat(fullname)
    except OSError:
        basename = filename

        # Realise a lazy loader based lookup if there is one
        # otherwise try to lookup right now.
        if lazycache(filename, module_globals):
            try:
                data = cache[filename][0]()
            except (ImportError, OSError):
                pass
            else:
                if data is None:
                    # No luck, the PEP302 loader cannot find the source
                    # for this module.
                    return []

                failures.pop(filename, None)
                cache[filename] = (
                    len(data),
                    None,
                    [line + '\n' for line in data.splitlines()],
                    fullname
                )
                return cache[filename][2]

        # Try looking through the module search path, which is only useful
        # when handling a relative filename.
        if os.path.isabs(filename):
            # os.stat() failed, so we won't read it
            failures[filename] = (None, None, fullname)
            return []

        for dirname in sys.path:
            try:
                fullname = os.path.join(dirname, basename)
            except (TypeError, AttributeError):
                # Not sufficiently string-like to do anything useful with.
                continue
            try:
                stat = os.stat(fullname)
                break
            except OSError:
                pass
        else:
            failures[filename] = (None, None, fullname)
            return []
    else:
        if filename in failures:
            size, mtime_ns, _ = failures[filename]
            if size is None or mtime_ns is None:
                # we may now be able to read the file
                failures.pop(filename, None)
            if size != stat.st_size or mtime_ns != stat.st_mtime_ns:
                # the file might have been updated
                failures.pop(filename, None)
            del size, mtime_ns  # to avoid using them

    if filename in failures:
        return []

    try:
        with tokenize.open(fullname) as fp:
            lines = fp.readlines()
    except OSError:
        # The file might have been deleted and thus, we need to
        # be sure that the next time checkcache() or updatecache()
        # is called, we do not trust the old os.stat() values.
        failures[filename] = (None, None, fullname)
        return []
    except (UnicodeDecodeError, SyntaxError):
        # The file content is incorrect but at least we could
        # read it. The next time checkcache() or updatecache()
        # is called, we can forget reading the file if nothing
        # was modified.
        failures[filename] = (stat.st_size, stat.st_mtime_ns, fullname)
        return []
    if not lines:
        lines = ['\n']
    elif not lines[-1].endswith('\n'):
        lines[-1] += '\n'
    size, mtime = stat.st_size, stat.st_mtime
    cache[filename] = size, mtime, lines, fullname
    failures.pop(filename, None)
    return lines


def lazycache(filename, module_globals):
    """Seed the cache for filename with module_globals.

    The module loader will be asked for the source only when getlines is
    called, not immediately.

    If there is an entry in the cache already, it is not altered.

    :return: True if a lazy load is registered in the cache,
        otherwise False. To register such a load a module loader with a
        get_source method must be found, the filename must be a cacheable
        filename, and the filename must not be already cached.
    """
    if filename in cache:
        if len(cache[filename]) == 1:
            return True
        else:
            return False
    if not filename or (filename.startswith('<') and filename.endswith('>')):
        return False
    # Try for a __loader__, if available
    if module_globals and '__name__' in module_globals:
        spec = module_globals.get('__spec__')
        name = getattr(spec, 'name', None) or module_globals['__name__']
        loader = getattr(spec, 'loader', None)
        if loader is None:
            loader = module_globals.get('__loader__')
        get_source = getattr(loader, 'get_source', None)

        if name and get_source:
            def get_lines(name=name, *args, **kwargs):
                return get_source(name, *args, **kwargs)
            cache[filename] = (get_lines,)
            # It might happen that a file is marked as a failure
            # before lazycache() is being called but should not
            # be a failure after (but before calling getlines()).
            failures.pop(filename, None)
            return True
    return False


def _register_code(code, string, name):
    cache[code] = (
            len(string),
            None,
            [line + '\n' for line in string.splitlines()],
            name)


def _validate_module_globals_type(module_globals):
    if module_globals is not None:
        # Validation is done here and not in linecache
        # since the latter may not necessarily use it.
        from collections.abc import Mapping

        if not isinstance(module_globals, Mapping):
            raise TypeError(f"'module_globals' must be a Mapping, "
                            f"got {type(module_globals)}")
