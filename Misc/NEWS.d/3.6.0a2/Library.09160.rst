[Security] Issue #26839: On Linux, :func:`os.urandom` now calls
``getrandom()`` with ``GRND_NONBLOCK`` to fall back on reading
``/dev/urandom`` if the urandom entropy pool is not initialized yet. Patch
written by Colm Buckley.