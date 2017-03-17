- Issue #14260: The groupindex attribute of regular expression pattern object
  now is non-modifiable mapping.

- Issue #23792: Ignore KeyboardInterrupt when the pydoc pager is active.
  This mimics the behavior of the standard unix pagers, and prevents
  pipepager from shutting down while the pager itself is still running.

- Issue #23775: pprint() of OrderedDict now outputs the same representation
  as repr().

- Issue #23765: Removed IsBadStringPtr calls in ctypes

- Issue #22364: Improved some re error messages using regex for hints.

- Issue #23742: ntpath.expandvars() no longer loses unbalanced single quotes.

- Issue #21717: The zipfile.ZipFile.open function now supports 'x' (exclusive
  creation) mode.

- Issue #21802: The reader in BufferedRWPair now is closed even when closing
  writer failed in BufferedRWPair.close().

- Issue #23622: Unknown escapes in regular expressions that consist of ``'\'``
  and ASCII letter now raise a deprecation warning and will be forbidden in
  Python 3.6.

- Issue #23671: string.Template now allows specifying the "self" parameter as
  a keyword argument.  string.Formatter now allows specifying the "self" and
  the "format_string" parameters as keyword arguments.

- Issue #23502: The pprint module now supports mapping proxies.

- Issue #17530: pprint now wraps long bytes objects and bytearrays.

- Issue #22687: Fixed some corner cases in breaking words in tetxtwrap.
  Got rid of quadratic complexity in breaking long words.

- Issue #4727: The copy module now uses pickle protocol 4 (PEP 3154) and
  supports copying of instances of classes whose __new__ method takes
  keyword-only arguments.

- Issue #23491: Added a zipapp module to support creating executable zip
  file archives of Python code. Registered ".pyz" and ".pyzw" extensions
  on Windows for these archives (PEP 441).

- Issue #23657: Avoid explicit checks for str in zipapp, adding support
  for pathlib.Path objects as arguments.

- Issue #23688: Added support of arbitrary bytes-like objects and avoided
  unnecessary copying of memoryview in gzip.GzipFile.write().
  Original patch by Wolfgang Maier.

- Issue #23252: Added support for writing ZIP files to unseekable streams.

- Issue #23647: Increase impalib's MAXLINE to accommodate modern mailbox sizes.

- Issue #23539: If body is None, http.client.HTTPConnection.request now sets
  Content-Length to 0 for PUT, POST, and PATCH headers to avoid 411 errors from
  some web servers.

- Issue #22351: The nntplib.NNTP constructor no longer leaves the connection
  and socket open until the garbage collector cleans them up.  Patch by
  Martin Panter.

- Issue #23704: collections.deque() objects now support methods for index(),
  insert(), and copy().  This allows deques to be registered as a
  MutableSequence and it improves their substitutability for lists.

- Issue #23715: :func:`signal.sigwaitinfo` and :func:`signal.sigtimedwait` are
  now retried when interrupted by a signal not in the *sigset* parameter, if
  the signal handler does not raise an exception. signal.sigtimedwait()
  recomputes the timeout with a monotonic clock when it is retried.

- Issue #23001: Few functions in modules mmap, ossaudiodev, socket, ssl, and
  codecs, that accepted only read-only bytes-like object now accept writable
  bytes-like object too.

- Issue #23646: If time.sleep() is interrupted by a signal, the sleep is now
  retried with the recomputed delay, except if the signal handler raises an
  exception (PEP 475).

- Issue #23136: _strptime now uniformly handles all days in week 0, including
  Dec 30 of previous year.  Based on patch by Jim Carroll.

- Issue #23700: Iterator of NamedTemporaryFile now keeps a reference to
  NamedTemporaryFile instance.  Patch by Bohuslav Kabrda.

- Issue #22903: The fake test case created by unittest.loader when it fails
  importing a test module is now picklable.

- Issue #22181: On Linux, os.urandom() now uses the new getrandom() syscall if
  available, syscall introduced in the Linux kernel 3.17. It is more reliable
  and more secure, because it avoids the need of a file descriptor and waits
  until the kernel has enough entropy.

- Issue #2211: Updated the implementation of the http.cookies.Morsel class.
  Setting attributes key, value and coded_value directly now is deprecated.
  update() and setdefault() now transform and check keys.  Comparing for
  equality now takes into account attributes key, value and coded_value.
  copy() now returns a Morsel, not a dict.  repr() now contains all attributes.
  Optimized checking keys and quoting values.  Added new tests.
  Original patch by Demian Brecht.

- Issue #18983: Allow selection of output units in timeit.
  Patch by Julian Gindi.

- Issue #23631: Fix traceback.format_list when a traceback has been mutated.

- Issue #23568: Add rdivmod support to MagicMock() objects.
  Patch by Håkan Lövdahl.

- Issue #2052: Add charset parameter to HtmlDiff.make_file().

- Issue #23668: Support os.truncate and os.ftruncate on Windows.

- Issue #23138: Fixed parsing cookies with absent keys or values in cookiejar.
  Patch by Demian Brecht.

- Issue #23051: multiprocessing.Pool methods imap() and imap_unordered() now
  handle exceptions raised by an iterator.  Patch by Alon Diamant and Davin
  Potts.

- Issue #23581: Add matmul support to MagicMock. Patch by Håkan Lövdahl.

- Issue #23566: enable(), register(), dump_traceback() and
  dump_traceback_later() functions of faulthandler now accept file
  descriptors. Patch by Wei Wu.

- Issue #22928: Disabled HTTP header injections in http.client.
  Original patch by Demian Brecht.

- Issue #23615: Modules bz2, tarfile and tokenize now can be reloaded with
  imp.reload().  Patch by Thomas Kluyver.

- Issue #23605: os.walk() now calls os.scandir() instead of os.listdir().
  The usage of os.scandir() reduces the number of calls to os.stat().
  Initial patch written by Ben Hoyt.

