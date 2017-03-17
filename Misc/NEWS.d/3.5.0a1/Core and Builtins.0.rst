- Issue #23285: PEP 475 - EINTR handling.

- Issue #22735: Fix many edge cases (including crashes) involving custom mro()
  implementations.

- Issue #22896: Avoid using PyObject_AsCharBuffer(), PyObject_AsReadBuffer()
  and PyObject_AsWriteBuffer().

- Issue #21295: Revert some changes (issue #16795) to AST line numbers and
  column offsets that constituted a regression.

- Issue #22986: Allow changing an object's __class__ between a dynamic type and
  static type in some cases.

- Issue #15859: PyUnicode_EncodeFSDefault(), PyUnicode_EncodeMBCS() and
  PyUnicode_EncodeCodePage() now raise an exception if the object is not a
  Unicode object. For PyUnicode_EncodeFSDefault(), it was already the case on
  platforms other than Windows. Patch written by Campbell Barton.

- Issue #21408: The default __ne__() now returns NotImplemented if __eq__()
  returned NotImplemented.  Original patch by Martin Panter.

- Issue #23321: Fixed a crash in str.decode() when error handler returned
  replacment string longer than mailformed input data.

- Issue #22286: The "backslashreplace" error handlers now works with
  decoding and translating.

- Issue #23253: Delay-load ShellExecute[AW] in os.startfile for reduced
  startup overhead on Windows.

- Issue #22038: pyatomic.h now uses stdatomic.h or GCC built-in functions for
  atomic memory access if available. Patch written by Vitor de Lima and Gustavo
  Temple.

- Issue #20284: %-interpolation (aka printf) formatting added for bytes and
  bytearray.

- Issue #23048: Fix jumping out of an infinite while loop in the pdb.

- Issue #20335: bytes constructor now raises TypeError when encoding or errors
  is specified with non-string argument.  Based on patch by Renaud Blanch.

- Issue #22834: If the current working directory ends up being set to a
  non-existent directory then import will no longer raise FileNotFoundError.

- Issue #22869: Move the interpreter startup & shutdown code to a new
  dedicated pylifecycle.c module

- Issue #22847: Improve method cache efficiency.

- Issue #22335: Fix crash when trying to enlarge a bytearray to 0x7fffffff
  bytes on a 32-bit platform.

- Issue #22653: Fix an assertion failure in debug mode when doing a reentrant
  dict insertion in debug mode.

- Issue #22643: Fix integer overflow in Unicode case operations (upper, lower,
  title, swapcase, casefold).

- Issue #17636: Circular imports involving relative imports are now
  supported.

- Issue #22604: Fix assertion error in debug mode when dividing a complex
  number by (nan+0j).

- Issue #21052: Do not raise ImportWarning when sys.path_hooks or sys.meta_path
  are set to None.

- Issue #16518: Use 'bytes-like object required' in error messages that
  previously used the far more cryptic "'x' does not support the buffer
  protocol.

- Issue #22470: Fixed integer overflow issues in "backslashreplace",
  "xmlcharrefreplace", and "surrogatepass" error handlers.

- Issue #22540: speed up `PyObject_IsInstance` and `PyObject_IsSubclass` in the
  common case that the second argument has metaclass `type`.

- Issue #18711: Add a new `PyErr_FormatV` function, similar to `PyErr_Format`
  but accepting a `va_list` argument.

- Issue #22520: Fix overflow checking when generating the repr of a unicode
  object.

- Issue #22519: Fix overflow checking in PyBytes_Repr.

- Issue #22518: Fix integer overflow issues in latin-1 encoding.

- Issue #16324: _charset parameter of MIMEText now also accepts
  email.charset.Charset instances. Initial patch by Claude Paroz.

- Issue #1764286: Fix inspect.getsource() to support decorated functions.
  Patch by Claudiu Popa.

- Issue #18554: os.__all__ includes posix functions.

- Issue #21391: Use os.path.abspath in the shutil module.

- Issue #11471: avoid generating a JUMP_FORWARD instruction at the end of
  an if-block if there is no else-clause.  Original patch by Eugene Toder.

- Issue #22215: Now ValueError is raised instead of TypeError when str or bytes
  argument contains not permitted null character or byte.

- Issue #22258: Fix the internal function set_inheritable() on Illumos.
  This platform exposes the function ``ioctl(FIOCLEX)``, but calling it fails
  with errno is ENOTTY: "Inappropriate ioctl for device". set_inheritable()
  now falls back to the slower ``fcntl()`` (``F_GETFD`` and then ``F_SETFD``).

- Issue #21389: Displaying the __qualname__ of the underlying function in the
  repr of a bound method.

- Issue #22206: Using pthread, PyThread_create_key() now sets errno to ENOMEM
  and returns -1 (error) on integer overflow.

- Issue #20184: Argument Clinic based signature introspection added for
  30 of the builtin functions.

- Issue #22116: C functions and methods (of the 'builtin_function_or_method'
  type) can now be weakref'ed.  Patch by Wei Wu.

- Issue #22077: Improve index error messages for bytearrays, bytes, lists,
  and tuples by adding 'or slices'. Added ', not <typename>' for bytearrays.
  Original patch by Claudiu Popa.

- Issue #20179: Apply Argument Clinic to bytes and bytearray.
  Patch by Tal Einat.

- Issue #22082: Clear interned strings in slotdefs.

- Upgrade Unicode database to Unicode 7.0.0.

- Issue #21897: Fix a crash with the f_locals attribute with closure
  variables when frame.clear() has been called.

- Issue #21205: Add a new ``__qualname__`` attribute to generator, the
  qualified name, and use it in the representation of a generator
  (``repr(gen)``). The default name of the generator (``__name__`` attribute)
  is now get from the function instead of the code. Use ``gen.gi_code.co_name``
  to get the name of the code.

- Issue #21669: With the aid of heuristics in SyntaxError.__init__, the
  parser now attempts to generate more meaningful (or at least more search
  engine friendly) error messages when "exec" and "print" are used as
  statements.

- Issue #21642: In the conditional if-else expression, allow an integer written
  with no space between itself and the ``else`` keyword (e.g. ``True if 42else
  False``) to be valid syntax.

- Issue #21523: Fix over-pessimistic computation of the stack effect of
  some opcodes in the compiler.  This also fixes a quadratic compilation
  time issue noticeable when compiling code with a large number of "and"
  and "or" operators.

- Issue #21418: Fix a crash in the builtin function super() when called without
  argument and without current frame (ex: embedded Python).

- Issue #21425: Fix flushing of standard streams in the interactive
  interpreter.

- Issue #21435: In rare cases, when running finalizers on objects in cyclic
  trash a bad pointer dereference could occur due to a subtle flaw in
  internal iteration logic.

- Issue #21377: PyBytes_Concat() now tries to concatenate in-place when the
  first argument has a reference count of 1.  Patch by Nikolaus Rath.

- Issue #20355: -W command line options now have higher priority than the
  PYTHONWARNINGS environment variable.  Patch by Arfrever.

- Issue #21274: Define PATH_MAX for GNU/Hurd in Python/pythonrun.c.

- Issue #20904: Support setting FPU precision on m68k.

- Issue #21209: Fix sending tuples to custom generator objects with the yield
  from syntax.

- Issue #21193: pow(a, b, c) now raises ValueError rather than TypeError when b
  is negative.  Patch by Josh Rosenberg.

- PEP 465 and Issue #21176: Add the '@' operator for matrix multiplication.

- Issue #21134: Fix segfault when str is called on an uninitialized
  UnicodeEncodeError, UnicodeDecodeError, or UnicodeTranslateError object.

- Issue #19537: Fix PyUnicode_DATA() alignment under m68k.  Patch by
  Andreas Schwab.

- Issue #20929: Add a type cast to avoid shifting a negative number.

- Issue #20731: Properly position in source code files even if they
  are opened in text mode. Patch by Serhiy Storchaka.

- Issue #20637: Key-sharing now also works for instance dictionaries of
  subclasses.  Patch by Peter Ingebretson.

- Issue #8297: Attributes missing from modules now include the module name
  in the error text.  Original patch by ysj.ray.

- Issue #19995: %c, %o, %x, and %X now raise TypeError on non-integer input.

- Issue #19655: The ASDL parser - used by the build process to generate code for
  managing the Python AST in C - was rewritten. The new parser is self contained
  and does not require to carry long the spark.py parser-generator library;
  spark.py was removed from the source base.

- Issue #12546: Allow ``\x00`` to be used as a fill character when using str, int,
  float, and complex __format__ methods.

- Issue #20480: Add ipaddress.reverse_pointer. Patch by Leon Weber.

- Issue #13598: Modify string.Formatter to support auto-numbering of
  replacement fields. It now matches the behavior of str.format() in
  this regard. Patches by Phil Elson and Ramchandra Apte.

- Issue #8931: Make alternate formatting ('#') for type 'c' raise an
  exception. In versions prior to 3.5, '#' with 'c' had no effect. Now
  specifying it is an error.  Patch by Torsten Landschoff.

- Issue #23165: Perform overflow checks before allocating memory in the
  _Py_char2wchar function.

