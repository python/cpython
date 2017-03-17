- Issue #27066: Fixed SystemError if a custom opener (for open()) returns a
  negative number without setting an exception.

- Issue #20041: Fixed TypeError when frame.f_trace is set to None.
  Patch by Xavier de Gaye.

- Issue #26168: Fixed possible refleaks in failing Py_BuildValue() with the "N"
  format unit.

- Issue #26991: Fix possible refleak when creating a function with annotations.

- Issue #27039: Fixed bytearray.remove() for values greater than 127.  Patch by
  Joe Jevnik.

- Issue #23640: int.from_bytes() no longer bypasses constructors for subclasses.

- Issue #26811: gc.get_objects() no longer contains a broken tuple with NULL
  pointer.

- Issue #20120: Use RawConfigParser for .pypirc parsing,
  removing support for interpolation unintentionally added
  with move to Python 3. Behavior no longer does any
  interpolation in .pypirc files, matching behavior in Python
  2.7 and Setuptools 19.0.

- Issue #26659: Make the builtin slice type support cycle collection.

- Issue #26718: super.__init__ no longer leaks memory if called multiple times.
  NOTE: A direct call of super.__init__ is not endorsed!

- Issue #25339: PYTHONIOENCODING now has priority over locale in setting the
  error handler for stdin and stdout.

- Issue #26494: Fixed crash on iterating exhausting iterators.
  Affected classes are generic sequence iterators, iterators of str, bytes,
  bytearray, list, tuple, set, frozenset, dict, OrderedDict, corresponding
  views and os.scandir() iterator.

- Issue #26581: If coding cookie is specified multiple times on a line in
  Python source code file, only the first one is taken to account.

- Issue #26464: Fix str.translate() when string is ASCII and first replacements
  removes character, but next replacement uses a non-ASCII character or a
  string longer than 1 character. Regression introduced in Python 3.5.0.

- Issue #22836: Ensure exception reports from PyErr_Display() and
  PyErr_WriteUnraisable() are sensible even when formatting them produces
  secondary errors.  This affects the reports produced by
  sys.__excepthook__() and when __del__() raises an exception.

- Issue #26302: Correct behavior to reject comma as a legal character for
  cookie names.

- Issue #4806: Avoid masking the original TypeError exception when using star
  (*) unpacking in function calls.  Based on patch by Hagen Fürstenau and
  Daniel Urban.

- Issue #27138: Fix the doc comment for FileFinder.find_spec().

- Issue #26154: Add a new private _PyThreadState_UncheckedGet() function to get
  the current Python thread state, but don't issue a fatal error if it is NULL.
  This new function must be used instead of accessing directly the
  _PyThreadState_Current variable.  The variable is no more exposed since
  Python 3.5.1 to hide the exact implementation of atomic C types, to avoid
  compiler issues.

- Issue #26194:  Deque.insert() gave odd results for bounded deques that had
  reached their maximum size.  Now an IndexError will be raised when attempting
  to insert into a full deque.

- Issue #25843: When compiling code, don't merge constants if they are equal
  but have a different types. For example, ``f1, f2 = lambda: 1, lambda: 1.0``
  is now correctly compiled to two different functions: ``f1()`` returns ``1``
  (``int``) and ``f2()`` returns ``1.0`` (``int``), even if ``1`` and ``1.0``
  are equal.

- Issue #22995: [UPDATE] Comment out the one of the pickleability tests in
  _PyObject_GetState() due to regressions observed in Cython-based projects.

- Issue #25961: Disallowed null characters in the type name.

- Issue #25973: Fix segfault when an invalid nonlocal statement binds a name
  starting with two underscores.

- Issue #22995: Instances of extension types with a state that aren't
  subclasses of list or dict and haven't implemented any pickle-related
  methods (__reduce__, __reduce_ex__, __getnewargs__, __getnewargs_ex__,
  or __getstate__), can no longer be pickled.  Including memoryview.

- Issue #20440: Massive replacing unsafe attribute setting code with special
  macro Py_SETREF.

- Issue #25766: Special method __bytes__() now works in str subclasses.

- Issue #25421: __sizeof__ methods of builtin types now use dynamic basic size.
  This allows sys.getsize() to work correctly with their subclasses with
  __slots__ defined.

- Issue #25709: Fixed problem with in-place string concatenation and utf-8
  cache.

- Issue #27147: Mention PEP 420 in the importlib docs.

- Issue #24097: Fixed crash in object.__reduce__() if slot name is freed inside
  __getattr__.

- Issue #24731: Fixed crash on converting objects with special methods
  __bytes__, __trunc__, and __float__ returning instances of subclasses of
  bytes, int, and float to subclasses of bytes, int, and float correspondingly.

- Issue #26478: Fix semantic bugs when using binary operators with dictionary
  views and tuples.

- Issue #26171: Fix possible integer overflow and heap corruption in
  zipimporter.get_data().

- Issue #25660: Fix TAB key behaviour in REPL with readline.

- Issue #25887: Raise a RuntimeError when a coroutine object is awaited
  more than once.

- Issue #27243: Update the __aiter__ protocol: instead of returning
  an awaitable that resolves to an asynchronous iterator, the asynchronous
  iterator should be returned directly.  Doing the former will trigger a
  PendingDeprecationWarning.

