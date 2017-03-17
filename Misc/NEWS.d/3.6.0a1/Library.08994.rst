Issue #26801: Fix error handling in :func:`shutil.get_terminal_size`, catch
:exc:`AttributeError` instead of :exc:`NameError`. Patch written by Emanuel
Barry.