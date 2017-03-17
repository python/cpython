- Issue #25630: Fix a possible segfault during argument parsing in functions
  that accept filesystem paths.

- Issue #23564: Fixed a partially broken sanity check in the _posixsubprocess
  internals regarding how fds_to_pass were passed to the child.  The bug had
  no actual impact as subprocess.py already avoided it.

- Issue #25388: Fixed tokenizer crash when processing undecodable source code
  with a null byte.

- Issue #25462: The hash of the key now is calculated only once in most
  operations in C implementation of OrderedDict.

- Issue #22995: Default implementation of __reduce__ and __reduce_ex__ now
  rejects builtin types with not defined __new__.

- Issue #25555: Fix parser and AST: fill lineno and col_offset of "arg" node
  when compiling AST from Python objects.

- Issue #24802: Avoid buffer overreads when int(), float(), compile(), exec()
  and eval() are passed bytes-like objects.  These objects are not
  necessarily terminated by a null byte, but the functions assumed they were.

- Issue #24726: Fixed a crash and leaking NULL in repr() of OrderedDict that
  was mutated by direct calls of dict methods.

- Issue #25449: Iterating OrderedDict with keys with unstable hash now raises
  KeyError in C implementations as well as in Python implementation.

- Issue #25395: Fixed crash when highly nested OrderedDict structures were
  garbage collected.

- Issue #25274: sys.setrecursionlimit() now raises a RecursionError if the new
  recursion limit is too low depending at the current recursion depth. Modify
  also the "lower-water mark" formula to make it monotonic. This mark is used
  to decide when the overflowed flag of the thread state is reset.

- Issue #24402: Fix input() to prompt to the redirected stdout when
  sys.stdout.fileno() fails.

- Issue #24806: Prevent builtin types that are not allowed to be subclassed from
  being subclassed through multiple inheritance.

- Issue #24848: Fixed a number of bugs in UTF-7 decoding of misformed data.

- Issue #25280: Import trace messages emitted in verbose (-v) mode are no
  longer formatted twice.

- Issue #25003: On Solaris 11.3 or newer, os.urandom() now uses the
  getrandom() function instead of the getentropy() function. The getentropy()
  function is blocking to generate very good quality entropy, os.urandom()
  doesn't need such high-quality entropy.

- Issue #25182: The stdprinter (used as sys.stderr before the io module is
  imported at startup) now uses the backslashreplace error handler.

- Issue #25131: Make the line number and column offset of set/dict literals and
  comprehensions correspond to the opening brace.

- Issue #25150: Hide the private _Py_atomic_xxx symbols from the public
  Python.h header to fix a compilation error with OpenMP. PyThreadState_GET()
  becomes an alias to PyThreadState_Get() to avoid ABI incompatibilies.

