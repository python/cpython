- Issue #15812: inspect.getframeinfo() now correctly shows the first line of
  a context.  Patch by Sam Breese.

- Issue #29094: Offsets in a ZIP file created with extern file object and modes
  "w" and "x" now are relative to the start of the file.

- Issue #13051: Fixed recursion errors in large or resized
  curses.textpad.Textbox.  Based on patch by Tycho Andersen.

- Issue #29119: Fix weakrefs in the pure python version of
  collections.OrderedDict move_to_end() method.
  Contributed by Andra Bogildea.

- Issue #9770: curses.ascii predicates now work correctly with negative
  integers.

- Issue #28427: old keys should not remove new values from
  WeakValueDictionary when collecting from another thread.

- Issue 28923: Remove editor artifacts from Tix.py.

- Issue #28871: Fixed a crash when deallocate deep ElementTree.

- Issue #19542: Fix bugs in WeakValueDictionary.setdefault() and
  WeakValueDictionary.pop() when a GC collection happens in another
  thread.

- Issue #20191: Fixed a crash in resource.prlimit() when pass a sequence that
  doesn't own its elements as limits.

- Issue #28779: multiprocessing.set_forkserver_preload() would crash the
  forkserver process if a preloaded module instantiated some
  multiprocessing objects such as locks.

- Issue #28847: dbm.dumb now supports reading read-only files and no longer
  writes the index file when it is not changed.

- Issue #25659: In ctypes, prevent a crash calling the from_buffer() and
  from_buffer_copy() methods on abstract classes like Array.

- Issue #28732: Fix crash in os.spawnv() with no elements in args

- Issue #28485: Always raise ValueError for negative
  compileall.compile_dir(workers=...) parameter, even when multithreading is
  unavailable.

- Issue #28387: Fixed possible crash in _io.TextIOWrapper deallocator when
  the garbage collector is invoked in other thread.  Based on patch by
  Sebastian Cufre.

- Issue #27517: LZMA compressor and decompressor no longer raise exceptions if
  given empty data twice.  Patch by Benjamin Fogle.

- Issue #28549: Fixed segfault in curses's addch() with ncurses6.

- Issue #28449: tarfile.open() with mode "r" or "r:" now tries to open a tar
  file with compression before trying to open it without compression.  Otherwise
  it had 50% chance failed with ignore_zeros=True.

- Issue #23262: The webbrowser module now supports Firefox 36+ and derived
  browsers.  Based on patch by Oleg Broytman.

- Issue #27939: Fixed bugs in tkinter.ttk.LabeledScale and tkinter.Scale caused
  by representing the scale as float value internally in Tk.  tkinter.IntVar
  now works if float value is set to underlying Tk variable.

- Issue #28255: calendar.TextCalendar().prmonth() no longer prints a space
  at the start of new line after printing a month's calendar.  Patch by
  Xiang Zhang.

- Issue #20491: The textwrap.TextWrapper class now honors non-breaking spaces.
  Based on patch by Kaarle Ritvanen.

- Issue #28353: os.fwalk() no longer fails on broken links.

- Issue #25464: Fixed HList.header_exists() in tkinter.tix module by addin
  a workaround to Tix library bug.

- Issue #28488: shutil.make_archive() no longer add entry "./" to ZIP archive.

- Issue #24452: Make webbrowser support Chrome on Mac OS X.

- Issue #20766: Fix references leaked by pdb in the handling of SIGINT
  handlers.

- Issue #26293: Fixed writing ZIP files that starts not from the start of the
  file.  Offsets in ZIP file now are relative to the start of the archive in
  conforming to the specification.

- Issue #28321: Fixed writing non-BMP characters with binary format in plistlib.

- Issue #28322: Fixed possible crashes when unpickle itertools objects from
  incorrect pickle data.  Based on patch by John Leitch.

- Fix possible integer overflows and crashes in the mmap module with unusual
  usage patterns.

- Issue #1703178: Fix the ability to pass the --link-objects option to the
  distutils build_ext command.

- Issue #28253: Fixed calendar functions for extreme months: 0001-01
  and 9999-12.
  
  Methods itermonthdays() and itermonthdays2() are reimplemented so
  that they don't call itermonthdates() which can cause datetime.date
  under/overflow.

- Issue #28275: Fixed possible use after free in the decompress()
  methods of the LZMADecompressor and BZ2Decompressor classes.
  Original patch by John Leitch.

- Issue #27897: Fixed possible crash in sqlite3.Connection.create_collation()
  if pass invalid string-like object as a name.  Patch by Xiang Zhang.

- Issue #18893: Fix invalid exception handling in Lib/ctypes/macholib/dyld.py.
  Patch by Madison May.

- Issue #27611: Fixed support of default root window in the tkinter.tix module.

- Issue #27348: In the traceback module, restore the formatting of exception
  messages like "Exception: None".  This fixes a regression introduced in
  3.5a2.

- Issue #25651: Allow falsy values to be used for msg parameter of subTest().

- Issue #27932: Prevent memory leak in win32_ver().

- Fix UnboundLocalError in socket._sendfile_use_sendfile.

- Issue #28075: Check for ERROR_ACCESS_DENIED in Windows implementation of
  os.stat().  Patch by Eryk Sun.

- Issue #25270: Prevent codecs.escape_encode() from raising SystemError when
  an empty bytestring is passed.

- Issue #28181: Get antigravity over HTTPS. Patch by Kaartic Sivaraam.

- Issue #25895: Enable WebSocket URL schemes in urllib.parse.urljoin.
  Patch by Gergely Imreh and Markus Holtermann.

- Issue #27599: Fixed buffer overrun in binascii.b2a_qp() and binascii.a2b_qp().

- Issue #19003:m email.generator now replaces only \r and/or \n line
  endings, per the RFC, instead of all unicode line endings.

- Issue #28019: itertools.count() no longer rounds non-integer step in range
  between 1.0 and 2.0 to 1.

- Issue #25969: Update the lib2to3 grammar to handle the unpacking
  generalizations added in 3.5.

- Issue #14977: mailcap now respects the order of the lines in the mailcap
  files ("first match"), as required by RFC 1542.  Patch by Michael Lazar.

- Issue #24594: Validates persist parameter when opening MSI database

- Issue #17582: xml.etree.ElementTree nows preserves whitespaces in attributes
  (Patch by Duane Griffin.  Reviewed and approved by Stefan Behnel.)

- Issue #28047: Fixed calculation of line length used for the base64 CTE
  in the new email policies.

- Issue #27445: Don't pass str(_charset) to MIMEText.set_payload().
  Patch by Claude Paroz.

- Issue #22450: urllib now includes an "Accept: */*" header among the
  default headers.  This makes the results of REST API requests more
  consistent and predictable especially when proxy servers are involved.

- lib2to3.pgen3.driver.load_grammar() now creates a stable cache file
  between runs given the same Grammar.txt input regardless of the hash
  randomization setting.

- Issue #27570: Avoid zero-length memcpy() etc calls with null source
  pointers in the "ctypes" and "array" modules.

- Issue #22233: Break email header lines *only* on the RFC specified CR and LF
  characters, not on arbitrary unicode line breaks.  This also fixes a bug in
  HTTP header parsing.

- Issue 27988: Fix email iter_attachments incorrect mutation of payload list.

- Issue #27691: Fix ssl module's parsing of GEN_RID subject alternative name
  fields in X.509 certs.

- Issue #27850: Remove 3DES from ssl module's default cipher list to counter
  measure sweet32 attack (CVE-2016-2183).

- Issue #27766: Add ChaCha20 Poly1305 to ssl module's default ciper list.
  (Required OpenSSL 1.1.0 or LibreSSL).

- Issue #26470: Port ssl and hashlib module to OpenSSL 1.1.0.

- Remove support for passing a file descriptor to os.access. It never worked but
  previously didn't raise.

- Issue #12885: Fix error when distutils encounters symlink.

- Issue #27881: Fixed possible bugs when setting sqlite3.Connection.isolation_level.
  Based on patch by Xiang Zhang.

- Issue #27861: Fixed a crash in sqlite3.Connection.cursor() when a factory
  creates not a cursor.  Patch by Xiang Zhang.

- Issue #19884: Avoid spurious output on OS X with Gnu Readline.

- Issue #27706: Restore deterministic behavior of random.Random().seed()
  for string seeds using seeding version 1.  Allows sequences of calls
  to random() to exactly match those obtained in Python 2.
  Patch by Nofar Schnider.

- Issue #10513: Fix a regression in Connection.commit().  Statements should
  not be reset after a commit.

- A new version of typing.py from https://github.com/python/typing:
  - Collection (only for 3.6) (Issue #27598)
  - Add FrozenSet to __all__ (upstream #261)
  - fix crash in _get_type_vars() (upstream #259)
  - Remove the dict constraint in ForwardRef._eval_type (upstream #252)

- Issue #27539: Fix unnormalised ``Fraction.__pow__`` result in the case
  of negative exponent and negative base.

- Issue #21718: cursor.description is now available for queries using CTEs.

- Issue #2466: posixpath.ismount now correctly recognizes mount points which
  the user does not have permission to access.

- Issue #27773: Correct some memory management errors server_hostname in
  _ssl.wrap_socket().

- Issue #26750: unittest.mock.create_autospec() now works properly for
  subclasses of property() and other data descriptors.

- In the curses module, raise an error if window.getstr() or window.instr() is
  passed a negative value.

- Issue #27783: Fix possible usage of uninitialized memory in
  operator.methodcaller.

- Issue #27774: Fix possible Py_DECREF on unowned object in _sre.

- Issue #27760: Fix possible integer overflow in binascii.b2a_qp.

- Issue #27758: Fix possible integer overflow in the _csv module for large
  record lengths.

- Issue #27568: Prevent HTTPoxy attack (CVE-2016-1000110). Ignore the
  HTTP_PROXY variable when REQUEST_METHOD environment is set, which indicates
  that the script is in CGI mode.

- Issue #27656: Do not assume sched.h defines any SCHED_* constants.

- Issue #27130: In the "zlib" module, fix handling of large buffers
  (typically 4 GiB) when compressing and decompressing.  Previously, inputs
  were limited to 4 GiB, and compression and decompression operations did not
  properly handle results of 4 GiB.

- Issue #27533: Release GIL in nt._isdir

- Issue #17711: Fixed unpickling by the persistent ID with protocol 0.
  Original patch by Alexandre Vassalotti.

- Issue #27522: Avoid an unintentional reference cycle in email.feedparser.

- Issue #26844: Fix error message for imp.find_module() to refer to 'path'
  instead of 'name'. Patch by Lev Maximov.

- Issue #23804: Fix SSL zero-length recv() calls to not block and not raise
  an error about unclean EOF.

- Issue #27466: Change time format returned by http.cookie.time2netscape,
  confirming the netscape cookie format and making it consistent with
  documentation.

- Issue #26664: Fix activate.fish by removing mis-use of ``$``.

- Issue #22115: Fixed tracing Tkinter variables: trace_vdelete() with wrong
  mode no longer break tracing, trace_vinfo() now always returns a list of
  pairs of strings, tracing in the "u" mode now works.

- Fix a scoping issue in importlib.util.LazyLoader which triggered an
  UnboundLocalError when lazy-loading a module that was already put into
  sys.modules.

- Issue #27079: Fixed curses.ascii functions isblank(), iscntrl() and ispunct().

- Issue #26754: Some functions (compile() etc) accepted a filename argument
  encoded as an iterable of integers. Now only strings and byte-like objects
  are accepted.

- Issue #27048: Prevents distutils failing on Windows when environment
  variables contain non-ASCII characters

- Issue #27330: Fixed possible leaks in the ctypes module.

- Issue #27238: Got rid of bare excepts in the turtle module.  Original patch
  by Jelle Zijlstra.

- Issue #27122: When an exception is raised within the context being managed
  by a contextlib.ExitStack() and one of the exit stack generators
  catches and raises it in a chain, do not re-raise the original exception
  when exiting, let the new chained one through.  This avoids the PEP 479
  bug described in issue25782.

- [Security] Issue #27278: Fix os.urandom() implementation using getrandom() on
  Linux.  Truncate size to INT_MAX and loop until we collected enough random
  bytes, instead of casting a directly Py_ssize_t to int.

- Issue #26386: Fixed ttk.TreeView selection operations with item id's
  containing spaces.

- [Security] Issue #22636: Avoid shell injection problems with
  ctypes.util.find_library().

- Issue #16182: Fix various functions in the "readline" module to use the
  locale encoding, and fix get_begidx() and get_endidx() to return code point
  indexes.

- Issue #27392: Add loop.connect_accepted_socket().
  Patch by Jim Fulton.

- Issue #27930: Improved behaviour of logging.handlers.QueueListener.
  Thanks to Paulo Andrade and Petr Viktorin for the analysis and patch.

- Issue #21201: Improves readability of multiprocessing error message.  Thanks
  to Wojciech Walczak for patch.

- Issue #27456: asyncio: Set TCP_NODELAY by default.

- Issue #27906: Fix socket accept exhaustion during high TCP traffic.
  Patch by Kevin Conway.

- Issue #28174: Handle when SO_REUSEPORT isn't properly supported.
  Patch by Seth Michael Larson.

- Issue #26654: Inspect functools.partial in asyncio.Handle.__repr__.
  Patch by iceboy.

- Issue #26909: Fix slow pipes IO in asyncio.
  Patch by INADA Naoki.

- Issue #28176: Fix callbacks race in asyncio.SelectorLoop.sock_connect.

- Issue #27759: Fix selectors incorrectly retain invalid file descriptors.
  Patch by Mark Williams.

- Issue #28368: Refuse monitoring processes if the child watcher has
  no loop attached.
  Patch by Vincent Michel.

- Issue #28369: Raise RuntimeError when transport's FD is used with
  add_reader, add_writer, etc.

- Issue #28370: Speedup asyncio.StreamReader.readexactly.
  Patch by Коренберг Марк.

- Issue #28371: Deprecate passing asyncio.Handles to run_in_executor.

- Issue #28372: Fix asyncio to support formatting of non-python coroutines.

- Issue #28399: Remove UNIX socket from FS before binding.
  Patch by Коренберг Марк.

- Issue #27972: Prohibit Tasks to await on themselves.

- Issue #26923: Fix asyncio.Gather to refuse being cancelled once all
  children are done.
  Patch by Johannes Ebke.

- Issue #26796: Don't configure the number of workers for default
  threadpool executor.
  Initial patch by Hans Lawrenz.

- Issue #28600: Optimize loop.call_soon().

- Issue #28613: Fix get_event_loop() return the current loop if
  called from coroutines/callbacks.

- Issue #28639: Fix inspect.isawaitable to always return bool
  Patch by Justin Mayfield.

- Issue #28652: Make loop methods reject socket kinds they do not support.

- Issue #28653: Fix a refleak in functools.lru_cache.

- Issue #28703: Fix asyncio.iscoroutinefunction to handle Mock objects.

- Issue #24142: Reading a corrupt config file left the parser in an
  invalid state.  Original patch by Florian Höch.

- Issue #28990: Fix SSL hanging if connection is closed before handshake
  completed.
  (Patch by HoHo-Ho)

