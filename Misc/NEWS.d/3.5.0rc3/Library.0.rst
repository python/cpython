- Issue #24917: time_strftime() buffer over-read.

- Issue #24748: To resolve a compatibility problem found with py2exe and
  pywin32, imp.load_dynamic() once again ignores previously loaded modules
  to support Python modules replacing themselves with extension modules.
  Patch by Petr Viktorin.

- Issue #24635: Fixed a bug in typing.py where isinstance([], typing.Iterable)
  would return True once, then False on subsequent calls.

- Issue #24989: Fixed buffer overread in BytesIO.readline() if a position is
  set beyond size.  Based on patch by John Leitch.

- Issue #24913: Fix overrun error in deque.index().
  Found by John Leitch and Bryce Darling.

