- Issue #29073: bytearray formatting no longer truncates on first null byte.

- Issue #28932: Do not include <sys/random.h> if it does not exist.

- Issue #28147: Fix a memory leak in split-table dictionaries: setattr()
  must not convert combined table into split table.

- Issue #25677: Correct the positioning of the syntax error caret for
  indented blocks.  Based on patch by Michael Layzell.

- Issue #29000: Fixed bytes formatting of octals with zero padding in alternate
  form.

- Issue #28512: Fixed setting the offset attribute of SyntaxError by
  PyErr_SyntaxLocationEx() and PyErr_SyntaxLocationObject().

- Issue #28991:  functools.lru_cache() was susceptible to an obscure reentrancy
  bug caused by a monkey-patched len() function.

- Issue #28648: Fixed crash in Py_DecodeLocale() in debug build on Mac OS X
  when decode astral characters.  Patch by Xiang Zhang.

- Issue #19398: Extra slash no longer added to sys.path components in case of
  empty compile-time PYTHONPATH components.

- Issue #28426: Fixed potential crash in PyUnicode_AsDecodedObject() in debug
  build.

- Issue #23782: Fixed possible memory leak in _PyTraceback_Add() and exception
  loss in PyTraceBack_Here().

- Issue #28379: Added sanity checks and tests for PyUnicode_CopyCharacters().
  Patch by Xiang Zhang.

- Issue #28376: The type of long range iterator is now registered as Iterator.
  Patch by Oren Milman.

- Issue #28376: The constructor of range_iterator now checks that step is not 0.
  Patch by Oren Milman.

- Issue #26906: Resolving special methods of uninitialized type now causes
  implicit initialization of the type instead of a fail.

- Issue #18287: PyType_Ready() now checks that tp_name is not NULL.
  Original patch by Niklas Koep.

- Issue #24098: Fixed possible crash when AST is changed in process of
  compiling it.

- Issue #28350: String constants with null character no longer interned.

- Issue #26617: Fix crash when GC runs during weakref callbacks.

- Issue #27942: String constants now interned recursively in tuples and frozensets.

- Issue #21578: Fixed misleading error message when ImportError called with
  invalid keyword args.

- Issue #28203: Fix incorrect type in error message from
  ``complex(1.0, {2:3})``. Patch by Soumya Sharma.

- Issue #27955: Fallback on reading /dev/urandom device when the getrandom()
  syscall fails with EPERM, for example when blocked by SECCOMP.

- Issue #28131: Fix a regression in zipimport's compile_source().  zipimport
  should use the same optimization level as the interpreter.

- Issue #25221: Fix corrupted result from PyLong_FromLong(0) when
  Python is compiled with NSMALLPOSINTS = 0.

- Issue #25758: Prevents zipimport from unnecessarily encoding a filename
  (patch by Eryk Sun)

- Issue #28189: dictitems_contains no longer swallows compare errors.
  (Patch by Xiang Zhang)

- Issue #27812: Properly clear out a generator's frame's backreference to the
  generator to prevent crashes in frame.clear().

- Issue #27811: Fix a crash when a coroutine that has not been awaited is
  finalized with warnings-as-errors enabled.

- Issue #27587: Fix another issue found by PVS-Studio: Null pointer check
  after use of 'def' in _PyState_AddModule().
  Initial patch by Christian Heimes.

- Issue #26020: set literal evaluation order did not match documented behaviour.

- Issue #27782: Multi-phase extension module import now correctly allows the
  ``m_methods`` field to be used to add module level functions to instances
  of non-module types returned from ``Py_create_mod``. Patch by Xiang Zhang.

- Issue #27936: The round() function accepted a second None argument
  for some types but not for others.  Fixed the inconsistency by
  accepting None for all numeric types.

- Issue #27487: Warn if a submodule argument to "python -m" or
  runpy.run_module() is found in sys.modules after parent packages are
  imported, but before the submodule is executed.

- Issue #27558: Fix a SystemError in the implementation of "raise" statement.
  In a brand new thread, raise a RuntimeError since there is no active
  exception to reraise. Patch written by Xiang Zhang.

- Issue #27419: Standard __import__() no longer look up "__import__" in globals
  or builtins for importing submodules or "from import".  Fixed handling an
  error of non-string package name.

- Issue #27083: Respect the PYTHONCASEOK environment variable under Windows.

- Issue #27514: Make having too many statically nested blocks a SyntaxError
  instead of SystemError.

- Issue #27473: Fixed possible integer overflow in bytes and bytearray
  concatenations.  Patch by Xiang Zhang.

- Issue #27507: Add integer overflow check in bytearray.extend().  Patch by
  Xiang Zhang.

- Issue #27581: Don't rely on wrapping for overflow check in
  PySequence_Tuple().  Patch by Xiang Zhang.

- Issue #27443: __length_hint__() of bytearray iterators no longer return a
  negative integer for a resized bytearray.

- Issue #27942: Fix memory leak in codeobject.c

