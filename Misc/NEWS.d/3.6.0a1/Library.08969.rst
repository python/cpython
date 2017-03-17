Issue #26637: The :mod:`importlib` module now emits an :exc:`ImportError`
rather than a :exc:`TypeError` if :func:`__import__` is tried during the
Python shutdown process but :data:`sys.path` is already cleared (set to
``None``).