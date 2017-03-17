- Issue #14373: Added C implementation of functools.lru_cache().  Based on
  patches by Matt Joiner and Alexey Kachayev.

- Issue #24230: The tempfile module now accepts bytes for prefix, suffix and dir
  parameters and returns bytes in such situations (matching the os module APIs).

- Issue #22189: collections.UserString now supports __getnewargs__(),
  __rmod__(), casefold(), format_map(), isprintable(), and maketrans().
  Patch by Joe Jevnik.

- Issue #24244: Prevents termination when an invalid format string is
  encountered on Windows in strftime.

- Issue #23973: PEP 484: Add the typing module.

- Issue #23086: The collections.abc.Sequence() abstract base class added
  *start* and *stop* parameters to the index() mixin.
  Patch by Devin Jeanpierre.

- Issue #20035: Replaced the ``tkinter._fix`` module used for setting up the
  Tcl/Tk environment on Windows with a private function in the ``_tkinter``
  module that makes no permanent changes to the environment.

- Issue #24257: Fixed segmentation fault in sqlite3.Row constructor with faked
  cursor type.

- Issue #15836: assertRaises(), assertRaisesRegex(), assertWarns() and
  assertWarnsRegex() assertments now check the type of the first argument
  to prevent possible user error.  Based on patch by Daniel Wagner-Hall.

- Issue #9858: Add missing method stubs to _io.RawIOBase.  Patch by Laura
  Rupprecht.

- Issue #22955: attrgetter, itemgetter and methodcaller objects in the operator
  module now support pickling.  Added readable and evaluable repr for these
  objects.  Based on patch by Josh Rosenberg.

- Issue #22107: tempfile.gettempdir() and tempfile.mkdtemp() now try again
  when a directory with the chosen name already exists on Windows as well as
  on Unix.  tempfile.mkstemp() now fails early if parent directory is not
  valid (not exists or is a file) on Windows.

- Issue #23780: Improved error message in os.path.join() with single argument.

- Issue #6598: Increased time precision and random number range in
  email.utils.make_msgid() to strengthen the uniqueness of the message ID.

- Issue #24091: Fixed various crashes in corner cases in C implementation of
  ElementTree.

- Issue #21931: msilib.FCICreate() now raises TypeError in the case of a bad
  argument instead of a ValueError with a bogus FCI error number.
  Patch by Jeffrey Armstrong.

- Issue #13866: *quote_via* argument added to urllib.parse.urlencode.

- Issue #20098: New mangle_from policy option for email, default True
  for compat32, but False for all other policies.

- Issue #24211: The email library now supports RFC 6532: it can generate
  headers using utf-8 instead of encoded words.

- Issue #16314: Added support for the LZMA compression in distutils.

- Issue #21804: poplib now supports RFC 6856 (UTF8).

- Issue #18682: Optimized pprint functions for builtin scalar types.

- Issue #22027: smtplib now supports RFC 6531 (SMTPUTF8).

- Issue #23488: Random generator objects now consume 2x less memory on 64-bit.

- Issue #1322: platform.dist() and platform.linux_distribution() functions are
  now deprecated.  Initial patch by Vajrasky Kok.

- Issue #22486: Added the math.gcd() function.  The fractions.gcd() function
  now is deprecated.  Based on patch by Mark Dickinson.

- Issue #24064: Property() docstrings are now writeable.
  (Patch by Berker Peksag.)

- Issue #22681: Added support for the koi8_t encoding.

- Issue #22682: Added support for the kz1048 encoding.

- Issue #23796: peek and read1 methods of BufferedReader now raise ValueError
  if they called on a closed object. Patch by John Hergenroeder.

- Issue #21795: smtpd now supports the 8BITMIME extension whenever
  the new *decode_data* constructor argument is set to False.

- Issue #24155: optimize heapq.heapify() for better cache performance
  when heapifying large lists.

- Issue #21800: imaplib now supports RFC 5161 (enable), RFC 6855
  (utf8/internationalized email) and automatically encodes non-ASCII
  usernames and passwords to UTF8.

- Issue #20274: When calling a _sqlite.Connection, it now complains if passed
  any keyword arguments.  Previously it silently ignored them.

- Issue #20274: Remove ignored and erroneous "kwargs" parameters from three
  METH_VARARGS methods on _sqlite.Connection.

- Issue #24134: assertRaises(), assertRaisesRegex(), assertWarns() and
  assertWarnsRegex() checks now emits a deprecation warning when callable is
  None or keyword arguments except msg is passed in the context manager mode.

- Issue #24018: Add a collections.abc.Generator abstract base class.
  Contributed by Stefan Behnel.

- Issue #23880: Tkinter's getint() and getdouble() now support Tcl_Obj.
  Tkinter's getdouble() now supports any numbers (in particular int).

- Issue #22619: Added negative limit support in the traceback module.
  Based on patch by Dmitry Kazakov.

- Issue #24094: Fix possible crash in json.encode with poorly behaved dict
  subclasses.

- Issue #9246: On POSIX, os.getcwd() now supports paths longer than 1025 bytes.
  Patch written by William Orr.

- Issue #17445: add difflib.diff_bytes() to support comparison of
  byte strings (fixes a regression from Python 2).

- Issue #23917: Fall back to sequential compilation when ProcessPoolExecutor
  doesn't exist.  Patch by Claudiu Popa.

- Issue #23008: Fixed resolving attributes with boolean value is False in pydoc.

- Fix asyncio issue 235: LifoQueue and PriorityQueue's put didn't
  increment unfinished tasks (this bug was introduced when
  JoinableQueue was merged with Queue).

- Issue #23908: os functions now reject paths with embedded null character
  on Windows instead of silently truncating them.

- Issue #23728: binascii.crc_hqx() could return an integer outside of the range
  0-0xffff for empty data.

- Issue #23887: urllib.error.HTTPError now has a proper repr() representation.
  Patch by Berker Peksag.

- asyncio: New event loop APIs: set_task_factory() and get_task_factory().

- asyncio: async() function is deprecated in favour of ensure_future().

- Issue #24178: asyncio.Lock, Condition, Semaphore, and BoundedSemaphore
  support new 'async with' syntax.  Contributed by Yury Selivanov.

- Issue #24179: Support 'async for' for asyncio.StreamReader.
  Contributed by Yury Selivanov.

- Issue #24184: Add AsyncIterator and AsyncIterable ABCs to
  collections.abc.  Contributed by Yury Selivanov.

- Issue #22547: Implement informative __repr__ for inspect.BoundArguments.
  Contributed by Yury Selivanov.

- Issue #24190: Implement inspect.BoundArgument.apply_defaults() method.
  Contributed by Yury Selivanov.

- Issue #20691: Add 'follow_wrapped' argument to
  inspect.Signature.from_callable() and inspect.signature().
  Contributed by Yury Selivanov.

- Issue #24248: Deprecate inspect.Signature.from_function() and
  inspect.Signature.from_builtin().

- Issue #23898: Fix inspect.classify_class_attrs() to support attributes
  with overloaded __eq__ and __bool__.  Patch by Mike Bayer.

- Issue #24298: Fix inspect.signature() to correctly unwrap wrappers
  around bound methods.

