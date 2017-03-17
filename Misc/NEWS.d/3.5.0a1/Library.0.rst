- Issue #23399: pyvenv creates relative symlinks where possible.

- Issue #20289: cgi.FieldStorage() now supports the context management
  protocol.

- Issue #13128: Print response headers for CONNECT requests when debuglevel
  > 0. Patch by Demian Brecht.

- Issue #15381: Optimized io.BytesIO to make less allocations and copyings.

- Issue #22818: Splitting on a pattern that could match an empty string now
  raises a warning.  Patterns that can only match empty strings are now
  rejected.

- Issue #23099: Closing io.BytesIO with exported buffer is rejected now to
  prevent corrupting exported buffer.

- Issue #23326: Removed __ne__ implementations.  Since fixing default __ne__
  implementation in issue #21408 they are redundant.

- Issue #23363: Fix possible overflow in itertools.permutations.

- Issue #23364: Fix possible overflow in itertools.product.

- Issue #23366: Fixed possible integer overflow in itertools.combinations.

- Issue #23369: Fixed possible integer overflow in
  _json.encode_basestring_ascii.

- Issue #23353: Fix the exception handling of generators in
  PyEval_EvalFrameEx(). At entry, save or swap the exception state even if
  PyEval_EvalFrameEx() is called with throwflag=0. At exit, the exception state
  is now always restored or swapped, not only if why is WHY_YIELD or
  WHY_RETURN. Patch co-written with Antoine Pitrou.

- Issue #14099: Restored support of writing ZIP files to tellable but
  non-seekable streams.

- Issue #14099: Writing to ZipFile and reading multiple ZipExtFiles is
  threadsafe now.

- Issue #19361: JSON decoder now raises JSONDecodeError instead of ValueError.

- Issue #18518: timeit now rejects statements which can't be compiled outside
  a function or a loop (e.g. "return" or "break").

- Issue #23094: Fixed readline with frames in Python implementation of pickle.

- Issue #23268: Fixed bugs in the comparison of ipaddress classes.

- Issue #21408: Removed incorrect implementations of __ne__() which didn't
  returned NotImplemented if __eq__() returned NotImplemented.  The default
  __ne__() now works correctly.

- Issue #19996: :class:`email.feedparser.FeedParser` now handles (malformed)
  headers with no key rather than assuming the body has started.

- Issue #20188: Support Application-Layer Protocol Negotiation (ALPN) in the ssl
  module.

- Issue #23133: Pickling of ipaddress objects now produces more compact and
  portable representation.

- Issue #23248: Update ssl error codes from latest OpenSSL git master.

- Issue #23266: Much faster implementation of ipaddress.collapse_addresses()
  when there are many non-consecutive addresses.

- Issue #23098: 64-bit dev_t is now supported in the os module.

- Issue #21817: When an exception is raised in a task submitted to a
  ProcessPoolExecutor, the remote traceback is now displayed in the
  parent process.  Patch by Claudiu Popa.

- Issue #15955: Add an option to limit output size when decompressing LZMA
  data.  Patch by Nikolaus Rath and Martin Panter.

- Issue #23250: In the http.cookies module, capitalize "HttpOnly" and "Secure"
  as they are written in the standard.

- Issue #23063: In the disutils' check command, fix parsing of reST with code or
  code-block directives.

- Issue #23209, #23225: selectors.BaseSelector.get_key() now raises a
  RuntimeError if the selector is closed. And selectors.BaseSelector.close()
  now clears its internal reference to the selector mapping to break a
  reference cycle. Initial patch written by Martin Richard.

- Issue #17911: Provide a way to seed the linecache for a PEP-302 module
  without actually loading the code.

- Issue #17911: Provide a new object API for traceback, including the ability
  to not lookup lines at all until the traceback is actually rendered, without
  any trace of the original objects being kept alive.

- Issue #19777: Provide a home() classmethod on Path objects.  Contributed
  by Victor Salgado and Mayank Tripathi.

- Issue #23206: Make ``json.dumps(..., ensure_ascii=False)`` as fast as the
  default case of ``ensure_ascii=True``.  Patch by Naoki Inada.

- Issue #23185: Add math.inf and math.nan constants.

- Issue #23186: Add ssl.SSLObject.shared_ciphers() and
  ssl.SSLSocket.shared_ciphers() to fetch the client's list ciphers sent at
  handshake.

- Issue #23143: Remove compatibility with OpenSSLs older than 0.9.8.

- Issue #23132: Improve performance and introspection support of comparison
  methods created by functool.total_ordering.

- Issue #19776: Add an expanduser() method on Path objects.

- Issue #23112: Fix SimpleHTTPServer to correctly carry the query string and
  fragment when it redirects to add a trailing slash.

- Issue #21793: Added http.HTTPStatus enums (i.e. HTTPStatus.OK,
  HTTPStatus.NOT_FOUND).  Patch by Demian Brecht.

- Issue #23093: In the io, module allow more operations to work on detached
  streams.

- Issue #23111: In the ftplib, make ssl.PROTOCOL_SSLv23 the default protocol
  version.

- Issue #22585: On OpenBSD 5.6 and newer, os.urandom() now calls getentropy(),
  instead of reading /dev/urandom, to get pseudo-random bytes.

- Issue #19104: pprint now produces evaluable output for wrapped strings.

- Issue #23071: Added missing names to codecs.__all__.  Patch by Martin Panter.

- Issue #22783: Pickling now uses the NEWOBJ opcode instead of the NEWOBJ_EX
  opcode if possible.

- Issue #15513: Added a __sizeof__ implementation for pickle classes.

- Issue #19858: pickletools.optimize() now aware of the MEMOIZE opcode, can
  produce more compact result and no longer produces invalid output if input
  data contains MEMOIZE opcodes together with PUT or BINPUT opcodes.

- Issue #22095: Fixed HTTPConnection.set_tunnel with default port.  The port
  value in the host header was set to "None".  Patch by Demian Brecht.

- Issue #23016: A warning no longer produces an AttributeError when the program
  is run with pythonw.exe.

- Issue #21775: shutil.copytree(): fix crash when copying to VFAT. An exception
  handler assumed that OSError objects always have a 'winerror' attribute.
  That is not the case, so the exception handler itself raised AttributeError
  when run on Linux (and, presumably, any other non-Windows OS).
  Patch by Greg Ward.

- Issue #1218234: Fix inspect.getsource() to load updated source of
  reloaded module. Initial patch by Berker Peksag.

- Issue #21740: Support wrapped callables in doctest. Patch by Claudiu Popa.

- Issue #23009: Make sure selectors.EpollSelecrtor.select() works when no
  FD is registered.

- Issue #22959: In the constructor of http.client.HTTPSConnection, prefer the
  context's check_hostname attribute over the *check_hostname* parameter.

- Issue #22696: Add function :func:`sys.is_finalizing` to know about
  interpreter shutdown.

- Issue #16043: Add a default limit for the amount of data xmlrpclib.gzip_decode
  will return. This resolves CVE-2013-1753.

- Issue #14099: ZipFile.open() no longer reopen the underlying file.  Objects
  returned by ZipFile.open() can now operate independently of the ZipFile even
  if the ZipFile was created by passing in a file-like object as the first
  argument to the constructor.

- Issue #22966: Fix __pycache__ pyc file name clobber when pyc_compile is
  asked to compile a source file containing multiple dots in the source file
  name.

- Issue #21971: Update turtledemo doc and add module to the index.

- Issue #21032: Fixed socket leak if HTTPConnection.getresponse() fails.
  Original patch by Martin Panter.

- Issue #22407: Deprecated the use of re.LOCALE flag with str patterns or
  re.ASCII. It was newer worked.

- Issue #22902: The "ip" command is now used on Linux to determine MAC address
  in uuid.getnode().  Pach by Bruno Cauet.

- Issue #22960: Add a context argument to xmlrpclib.ServerProxy constructor.

- Issue #22389: Add contextlib.redirect_stderr().

- Issue #21356: Make ssl.RAND_egd() optional to support LibreSSL. The
  availability of the function is checked during the compilation. Patch written
  by Bernard Spil.

- Issue #22915: SAX parser now supports files opened with file descriptor or
  bytes path.

- Issue #22609: Constructors and update methods of mapping classes in the
  collections module now accept the self keyword argument.

- Issue #22940: Add readline.append_history_file.

- Issue #19676: Added the "namereplace" error handler.

- Issue #22788: Add *context* parameter to logging.handlers.HTTPHandler.

- Issue #22921: Allow SSLContext to take the *hostname* parameter even if
  OpenSSL doesn't support SNI.

- Issue #22894: TestCase.subTest() would cause the test suite to be stopped
  when in failfast mode, even in the absence of failures.

- Issue #22796: HTTP cookie parsing is now stricter, in order to protect
  against potential injection attacks.

- Issue #22370: Windows detection in pathlib is now more robust.

- Issue #22841: Reject coroutines in asyncio add_signal_handler().
  Patch by Ludovic.Gasc.

- Issue #19494: Added urllib.request.HTTPBasicPriorAuthHandler. Patch by
  Matej Cepl.

- Issue #22578: Added attributes to the re.error class.

- Issue #22849: Fix possible double free in the io.TextIOWrapper constructor.

- Issue #12728: Different Unicode characters having the same uppercase but
  different lowercase are now matched in case-insensitive regular expressions.

- Issue #22821: Fixed fcntl() with integer argument on 64-bit big-endian
  platforms.

- Issue #21650: Add an `--sort-keys` option to json.tool CLI.

- Issue #22824: Updated reprlib output format for sets to use set literals.
  Patch contributed by Berker Peksag.

- Issue #22824: Updated reprlib output format for arrays to display empty
  arrays without an unnecessary empty list.  Suggested by Serhiy Storchaka.

- Issue #22406: Fixed the uu_codec codec incorrectly ported to 3.x.
  Based on patch by Martin Panter.

- Issue #17293: uuid.getnode() now determines MAC address on AIX using netstat.
  Based on patch by Aivars Kalvāns.

- Issue #22769: Fixed ttk.Treeview.tag_has() when called without arguments.

- Issue #22417: Verify certificates by default in httplib (PEP 476).

- Issue #22775: Fixed unpickling of http.cookies.SimpleCookie with protocol 2
  and above.  Patch by Tim Graham.

- Issue #22776: Brought excluded code into the scope of a try block in
  SysLogHandler.emit().

- Issue #22665: Add missing get_terminal_size and SameFileError to
  shutil.__all__.

- Issue #6623: Remove deprecated Netrc class in the ftplib module. Patch by
  Matt Chaput.

- Issue #17381: Fixed handling of case-insensitive ranges in regular
  expressions.

- Issue #22410: Module level functions in the re module now cache compiled
  locale-dependent regular expressions taking into account the locale.

- Issue #22759: Query methods on pathlib.Path() (exists(), is_dir(), etc.)
  now return False when the underlying stat call raises NotADirectoryError.

- Issue #8876: distutils now falls back to copying files when hard linking
  doesn't work.  This allows use with special filesystems such as VirtualBox
  shared folders.

- Issue #22217: Implemented reprs of classes in the zipfile module.

- Issue #22457: Honour load_tests in the start_dir of discovery.

- Issue #18216: gettext now raises an error when a .mo file has an
  unsupported major version number.  Patch by Aaron Hill.

- Issue #13918: Provide a locale.delocalize() function which can remove
  locale-specific number formatting from a string representing a number,
  without then converting it to a specific type.  Patch by Cédric Krier.

- Issue #22676: Make the pickling of global objects which don't have a
  __module__ attribute less slow.

- Issue #18853: Fixed ResourceWarning in shlex.__nain__.

- Issue #9351: Defaults set with set_defaults on an argparse subparser
  are no longer ignored when also set on the parent parser.

- Issue #7559: unittest test loading ImportErrors are reported as import errors
  with their import exception rather than as attribute errors after the import
  has already failed.

- Issue #19746: Make it possible to examine the errors from unittest
  discovery without executing the test suite. The new `errors` attribute
  on TestLoader exposes these non-fatal errors encountered during discovery.

- Issue #21991: Make email.headerregistry's header 'params' attributes
  be read-only (MappingProxyType).  Previously the dictionary was modifiable
  but a new one was created on each access of the attribute.

- Issue #22638: SSLv3 is now disabled throughout the standard library.
  It can still be enabled by instantiating a SSLContext manually.

- Issue #22641: In asyncio, the default SSL context for client connections
  is now created using ssl.create_default_context(), for stronger security.

- Issue #17401: Include closefd in io.FileIO repr.

- Issue #21338: Add silent mode for compileall. quiet parameters of
  compile_{dir, file, path} functions now have a multilevel value. Also,
  -q option of the CLI now have a multilevel value. Patch by Thomas Kluyver.

- Issue #20152: Convert the array and cmath modules to Argument Clinic.

- Issue #18643: Add socket.socketpair() on Windows.

- Issue #22435: Fix a file descriptor leak when socketserver bind fails.

- Issue #13096: Fixed segfault in CTypes POINTER handling of large
  values.

- Issue #11694: Raise ConversionError in xdrlib as documented.  Patch
  by Filip Gruszczyński and Claudiu Popa.

- Issue #19380: Optimized parsing of regular expressions.

- Issue #1519638: Now unmatched groups are replaced with empty strings in re.sub()
  and re.subn().

- Issue #18615: sndhdr.what/whathdr now return a namedtuple.

- Issue #22462: Fix pyexpat's creation of a dummy frame to make it
  appear in exception tracebacks.

- Issue #21965: Add support for in-memory SSL to the ssl module.  Patch
  by Geert Jansen.

- Issue #21173: Fix len() on a WeakKeyDictionary when .clear() was called
  with an iterator alive.

- Issue #11866: Eliminated race condition in the computation of names
  for new threads.

- Issue #21905: Avoid RuntimeError in pickle.whichmodule() when sys.modules
  is mutated while iterating.  Patch by Olivier Grisel.

- Issue #11271: concurrent.futures.Executor.map() now takes a *chunksize*
  argument to allow batching of tasks in child processes and improve
  performance of ProcessPoolExecutor.  Patch by Dan O'Reilly.

- Issue #21883: os.path.join() and os.path.relpath() now raise a TypeError with
  more helpful error message for unsupported or mismatched types of arguments.

- Issue #22219: The zipfile module CLI now adds entries for directories
  (including empty directories) in ZIP file.

- Issue #22449: In the ssl.SSLContext.load_default_certs, consult the
  environmental variables SSL_CERT_DIR and SSL_CERT_FILE on Windows.

- Issue #22508: The email.__version__ variable has been removed; the email
  code is no longer shipped separately from the stdlib, and __version__
  hasn't been updated in several releases.

- Issue #20076: Added non derived UTF-8 aliases to locale aliases table.

- Issue #20079: Added locales supported in glibc 2.18 to locale alias table.

- Issue #20218: Added convenience methods read_text/write_text and read_bytes/
  write_bytes to pathlib.Path objects.

- Issue #22396: On 32-bit AIX platform, don't expose os.posix_fadvise() nor
  os.posix_fallocate() because their prototypes in system headers are wrong.

- Issue #22517: When an io.BufferedRWPair object is deallocated, clear its
  weakrefs.

- Issue #22437: Number of capturing groups in regular expression is no longer
  limited by 100.

- Issue #17442: InteractiveInterpreter now displays the full chained traceback
  in its showtraceback method, to match the built in interactive interpreter.

- Issue #23392: Added tests for marshal C API that works with FILE*.

- Issue #10510: distutils register and upload methods now use HTML standards
  compliant CRLF line endings.

- Issue #9850: Fixed macpath.join() for empty first component.  Patch by
  Oleg Oshmyan.

- Issue #5309: distutils' build and build_ext commands now accept a ``-j``
  option to enable parallel building of extension modules.

- Issue #22448: Improve canceled timer handles cleanup to prevent
  unbound memory usage. Patch by Joshua Moore-Oliva.

- Issue #22427: TemporaryDirectory no longer attempts to clean up twice when
  used in the with statement in generator.

- Issue #22362: Forbidden ambiguous octal escapes out of range 0-0o377 in
  regular expressions.

- Issue #20912: Now directories added to ZIP file have correct Unix and MS-DOS
  directory attributes.

- Issue #21866: ZipFile.close() no longer writes ZIP64 central directory
  records if allowZip64 is false.

- Issue #22278: Fix urljoin problem with relative urls, a regression observed
  after changes to issue22118 were submitted.

- Issue #22415: Fixed debugging output of the GROUPREF_EXISTS opcode in the re
  module.  Removed trailing spaces in debugging output.

- Issue #22423: Unhandled exception in thread no longer causes unhandled
  AttributeError when sys.stderr is None.

- Issue #21332: Ensure that ``bufsize=1`` in subprocess.Popen() selects
  line buffering, rather than block buffering.  Patch by Akira Li.

- Issue #21091: Fix API bug: email.message.EmailMessage.is_attachment is now
  a method.

- Issue #21079: Fix email.message.EmailMessage.is_attachment to return the
  correct result when the header has parameters as well as a value.

- Issue #22247: Add NNTPError to nntplib.__all__.

- Issue #22366: urllib.request.urlopen will accept a context object
  (SSLContext) as an argument which will then be used for HTTPS connection.
  Patch by Alex Gaynor.

- Issue #4180: The warnings registries are now reset when the filters
  are modified.

- Issue #22419: Limit the length of incoming HTTP request in wsgiref server to
  65536 bytes and send a 414 error code for higher lengths. Patch contributed
  by Devin Cook.

- Lax cookie parsing in http.cookies could be a security issue when combined
  with non-standard cookie handling in some Web browsers.  Reported by
  Sergey Bobrov.

- Issue #20537: logging methods now accept an exception instance as well as a
  Boolean value or exception tuple. Thanks to Yury Selivanov for the patch.

- Issue #22384: An exception in Tkinter callback no longer crashes the program
  when it is run with pythonw.exe.

- Issue #22168: Prevent turtle AttributeError with non-default Canvas on OS X.

- Issue #21147: sqlite3 now raises an exception if the request contains a null
  character instead of truncating it.  Based on patch by Victor Stinner.

- Issue #13968: The glob module now supports recursive search in
  subdirectories using the "**" pattern.

- Issue #21951: Fixed a crash in Tkinter on AIX when called Tcl command with
  empty string or tuple argument.

- Issue #21951: Tkinter now most likely raises MemoryError instead of crash
  if the memory allocation fails.

- Issue #22338: Fix a crash in the json module on memory allocation failure.

- Issue #12410: imaplib.IMAP4 now supports the context management protocol.
  Original patch by Tarek Ziadé.

- Issue #21270: We now override tuple methods in mock.call objects so that
  they can be used as normal call attributes.

- Issue #16662: load_tests() is now unconditionally run when it is present in
  a package's __init__.py.  TestLoader.loadTestsFromModule() still accepts
  use_load_tests, but it is deprecated and ignored.  A new keyword-only
  attribute `pattern` is added and documented.  Patch given by Robert Collins,
  tweaked by Barry Warsaw.

- Issue #22226: First letter no longer is stripped from the "status" key in
  the result of Treeview.heading().

- Issue #19524: Fixed resource leak in the HTTP connection when an invalid
  response is received.  Patch by Martin Panter.

- Issue #20421: Add a .version() method to SSL sockets exposing the actual
  protocol version in use.

- Issue #19546: configparser exceptions no longer expose implementation details.
  Chained KeyErrors are removed, which leads to cleaner tracebacks.  Patch by
  Claudiu Popa.

- Issue #22051: turtledemo no longer reloads examples to re-run them.
  Initialization of variables and gui setup should be done in main(),
  which is called each time a demo is run, but not on import.

- Issue #21933: Turtledemo users can change the code font size with a menu
  selection or control(command) '-' or '+' or control-mousewheel.
  Original patch by Lita Cho.

- Issue #21597: The separator between the turtledemo text pane and the drawing
  canvas can now be grabbed and dragged with a mouse.  The code text pane can
  be widened to easily view or copy the full width of the text.  The canvas
  can be widened on small screens.  Original patches by Jan Kanis and Lita Cho.

- Issue #18132: Turtledemo buttons no longer disappear when the window is
  shrunk.  Original patches by Jan Kanis and Lita Cho.

- Issue #22043: time.monotonic() is now always available.
  ``threading.Lock.acquire()``, ``threading.RLock.acquire()`` and socket
  operations now use a monotonic clock, instead of the system clock, when a
  timeout is used.

- Issue #21527: Add a default number of workers to ThreadPoolExecutor equal
  to 5 times the number of CPUs.  Patch by Claudiu Popa.

- Issue #22216: smtplib now resets its state more completely after a quit.  The
  most obvious consequence of the previous behavior was a STARTTLS failure
  during a connect/starttls/quit/connect/starttls sequence.

- Issue #22098: ctypes' BigEndianStructure and LittleEndianStructure now
  define an empty __slots__ so that subclasses don't always get an instance
  dict.  Patch by Claudiu Popa.

- Issue #22185: Fix an occasional RuntimeError in threading.Condition.wait()
  caused by mutation of the waiters queue without holding the lock.  Patch
  by Doug Zongker.

- Issue #22287: On UNIX, _PyTime_gettimeofday() now uses
  clock_gettime(CLOCK_REALTIME) if available. As a side effect, Python now
  depends on the librt library on Solaris and on Linux (only with glibc older
  than 2.17).

- Issue #22182: Use e.args to unpack exceptions correctly in
  distutils.file_util.move_file. Patch by Claudiu Popa.

- The webbrowser module now uses subprocess's start_new_session=True rather
  than a potentially risky preexec_fn=os.setsid call.

- Issue #22042: signal.set_wakeup_fd(fd) now raises an exception if the file
  descriptor is in blocking mode.

- Issue #16808: inspect.stack() now returns a named tuple instead of a tuple.
  Patch by Daniel Shahaf.

- Issue #22236: Fixed Tkinter images copying operations in NoDefaultRoot mode.

- Issue #2527: Add a *globals* argument to timeit functions, in order to
  override the globals namespace in which the timed code is executed.
  Patch by Ben Roberts.

- Issue #22118: Switch urllib.parse to use RFC 3986 semantics for the
  resolution of relative URLs, rather than RFCs 1808 and 2396.
  Patch by Demian Brecht.

- Issue #21549: Added the "members" parameter to TarFile.list().

- Issue #19628: Allow compileall recursion depth to be specified with a -r
  option.

- Issue #15696: Add a __sizeof__ implementation for mmap objects on Windows.

- Issue #22068: Avoided reference loops with Variables and Fonts in Tkinter.

- Issue #22165: SimpleHTTPRequestHandler now supports undecodable file names.

- Issue #15381: Optimized line reading in io.BytesIO.

- Issue #8797: Raise HTTPError on failed Basic Authentication immediately.
  Initial patch by Sam Bull.

- Issue #20729: Restored the use of lazy iterkeys()/itervalues()/iteritems()
  in the mailbox module.

- Issue #21448: Changed FeedParser feed() to avoid O(N**2) behavior when
  parsing long line.  Original patch by Raymond Hettinger.

- Issue #22184: The functools LRU Cache decorator factory now gives an earlier
  and clearer error message when the user forgets the required parameters.

- Issue #17923: glob() patterns ending with a slash no longer match non-dirs on
  AIX.  Based on patch by Delhallt.

- Issue #21725: Added support for RFC 6531 (SMTPUTF8) in smtpd.

- Issue #22176: Update the ctypes module's libffi to v3.1.  This release
  adds support for the Linux AArch64 and POWERPC ELF ABIv2 little endian
  architectures.

- Issue #5411: Added support for the "xztar" format in the shutil module.

- Issue #21121: Don't force 3rd party C extensions to be built with
  -Werror=declaration-after-statement.

- Issue #21975: Fixed crash when using uninitialized sqlite3.Row (in particular
  when unpickling pickled sqlite3.Row).  sqlite3.Row is now initialized in the
  __new__() method.

- Issue #20170: Convert posixmodule to use Argument Clinic.

- Issue #21539: Add an *exists_ok* argument to `Pathlib.mkdir()` to mimic
  `mkdir -p` and `os.makedirs()` functionality.  When true, ignore
  FileExistsErrors.  Patch by Berker Peksag.

- Issue #22127: Bypass IDNA for pure-ASCII host names in the socket module
  (in particular for numeric IPs).

- Issue #21047: set the default value for the *convert_charrefs* argument
  of HTMLParser to True.  Patch by Berker Peksag.

- Add an __all__ to html.entities.

- Issue #15114: the strict mode and argument of HTMLParser, HTMLParser.error,
  and the HTMLParserError exception have been removed.

- Issue #22085: Dropped support of Tk 8.3 in Tkinter.

- Issue #21580: Now Tkinter correctly handles bytes arguments passed to Tk.
  In particular this allows initializing images from binary data.

- Issue #22003: When initialized from a bytes object, io.BytesIO() now
  defers making a copy until it is mutated, improving performance and
  memory use on some use cases.  Patch by David Wilson.

- Issue #22018: On Windows, signal.set_wakeup_fd() now also supports sockets.
  A side effect is that Python depends to the WinSock library.

- Issue #22054: Add os.get_blocking() and os.set_blocking() functions to get
  and set the blocking mode of a file descriptor (False if the O_NONBLOCK flag
  is set, True otherwise). These functions are not available on Windows.

- Issue #17172: Make turtledemo start as active on OS X even when run with
  subprocess.  Patch by Lita Cho.

- Issue #21704: Fix build error for _multiprocessing when semaphores
  are not available.  Patch by Arfrever Frehtes Taifersar Arahesis.

- Issue #20173: Convert sha1, sha256, sha512 and md5 to ArgumentClinic.
  Patch by Vajrasky Kok.

- Fix repr(_socket.socket) on Windows 64-bit: don't fail with OverflowError
  on closed socket. repr(socket.socket) already works fine.

- Issue #22033: Reprs of most Python implemened classes now contain actual
  class name instead of hardcoded one.

- Issue #21947: The dis module can now disassemble generator-iterator
  objects based on their gi_code attribute. Patch by Clement Rouault.

- Issue #16133: The asynchat.async_chat.handle_read() method now ignores
  BlockingIOError exceptions.

- Issue #22044: Fixed premature DECREF in call_tzinfo_method.
  Patch by Tom Flanagan.

- Issue #19884: readline: Disable the meta modifier key if stdout is not
  a terminal to not write the ANSI sequence ``"\033[1034h"`` into stdout. This
  sequence is used on some terminal (ex: TERM=xterm-256color") to enable
  support of 8 bit characters.

- Issue #4350: Removed a number of out-of-dated and non-working for a long time
  Tkinter methods.

- Issue #6167: Scrollbar.activate() now returns the name of active element if
  the argument is not specified.  Scrollbar.set() now always accepts only 2
  arguments.

- Issue #15275: Clean up and speed up the ntpath module.

- Issue #21888: plistlib's load() and loads() now work if the fmt parameter is
  specified.

- Issue #22032: __qualname__ instead of __name__ is now always used to format
  fully qualified class names of Python implemented classes.

- Issue #22031: Reprs now always use hexadecimal format with the "0x" prefix
  when contain an id in form " at 0x...".

- Issue #22018: signal.set_wakeup_fd() now raises an OSError instead of a
  ValueError on ``fstat()`` failure.

- Issue #21044: tarfile.open() now handles fileobj with an integer 'name'
  attribute.  Based on patch by Antoine Pietri.

- Issue #21966: Respect -q command-line option when code module is ran.

- Issue #19076: Don't pass the redundant 'file' argument to self.error().

- Issue #16382: Improve exception message of warnings.warn() for bad
  category. Initial patch by Phil Elson.

- Issue #21932: os.read() now uses a :c:func:`Py_ssize_t` type instead of
  :c:type:`int` for the size to support reading more than 2 GB at once. On
  Windows, the size is truncted to INT_MAX. As any call to os.read(), the OS
  may read less bytes than the number of requested bytes.

- Issue #21942: Fixed source file viewing in pydoc's server mode on Windows.

- Issue #11259: asynchat.async_chat().set_terminator() now raises a ValueError
  if the number of received bytes is negative.

- Issue #12523: asynchat.async_chat.push() now raises a TypeError if it doesn't
  get a bytes string

- Issue #21707: Add missing kwonlyargcount argument to
  ModuleFinder.replace_paths_in_code().

- Issue #20639: calling Path.with_suffix('') allows removing the suffix
  again.  Patch by July Tikhonov.

- Issue #21714: Disallow the construction of invalid paths using
  Path.with_name().  Original patch by Antony Lee.

- Issue #15014: Added 'auth' method to smtplib to make implementing auth
  mechanisms simpler, and used it internally in the login method.

- Issue #21151: Fixed a segfault in the winreg module when ``None`` is passed
  as a ``REG_BINARY`` value to SetValueEx.  Patch by John Ehresman.

- Issue #21090: io.FileIO.readall() does not ignore I/O errors anymore. Before,
  it ignored I/O errors if at least the first C call read() succeed.

- Issue #5800: headers parameter of wsgiref.headers.Headers is now optional.
  Initial patch by Pablo Torres Navarrete and SilentGhost.

- Issue #21781: ssl.RAND_add() now supports strings longer than 2 GB.

- Issue #21679: Prevent extraneous fstat() calls during open().  Patch by
  Bohuslav Kabrda.

- Issue #21863: cProfile now displays the module name of C extension functions,
  in addition to their own name.

- Issue #11453: asyncore: emit a ResourceWarning when an unclosed file_wrapper
  object is destroyed. The destructor now closes the file if needed. The
  close() method can now be called twice: the second call does nothing.

- Issue #21858: Better handling of Python exceptions in the sqlite3 module.

- Issue #21476: Make sure the email.parser.BytesParser TextIOWrapper is
  discarded after parsing, so the input file isn't unexpectedly closed.

- Issue #20295: imghdr now recognizes OpenEXR format images.

- Issue #21729: Used the "with" statement in the dbm.dumb module to ensure
  files closing.  Patch by Claudiu Popa.

- Issue #21491: socketserver: Fix a race condition in child processes reaping.

- Issue #21719: Added the ``st_file_attributes`` field to os.stat_result on
  Windows.

- Issue #21832: Require named tuple inputs to be exact strings.

- Issue #21722: The distutils "upload" command now exits with a non-zero
  return code when uploading fails.  Patch by Martin Dengler.

- Issue #21723: asyncio.Queue: support any type of number (ex: float) for the
  maximum size. Patch written by Vajrasky Kok.

- Issue #21711: support for "site-python" directories has now been removed
  from the site module (it was deprecated in 3.4).

- Issue #17552: new socket.sendfile() method allowing a file to be sent over a
  socket by using high-performance os.sendfile() on UNIX.
  Patch by Giampaolo Rodola'.

- Issue #18039: dbm.dump.open() now always creates a new database when the
  flag has the value 'n'.  Patch by Claudiu Popa.

- Issue #21326: Add a new is_closed() method to asyncio.BaseEventLoop.
  run_forever() and run_until_complete() methods of asyncio.BaseEventLoop now
  raise an exception if the event loop was closed.

- Issue #21766: Prevent a security hole in CGIHTTPServer by URL unquoting paths
  before checking for a CGI script at that path.

- Issue #21310: Fixed possible resource leak in failed open().

- Issue #21256: Printout of keyword args should be in deterministic order in
  a mock function call. This will help to write better doctests.

- Issue #21677: Fixed chaining nonnormalized exceptions in io close() methods.

- Issue #11709: Fix the pydoc.help function to not fail when sys.stdin is not a
  valid file.

- Issue #21515: tempfile.TemporaryFile now uses os.O_TMPFILE flag is available.

- Issue #13223: Fix pydoc.writedoc so that the HTML documentation for methods
  that use 'self' in the example code is generated correctly.

- Issue #21463: In urllib.request, fix pruning of the FTP cache.

- Issue #21618: The subprocess module could fail to close open fds that were
  inherited by the calling process and already higher than POSIX resource
  limits would otherwise allow.  On systems with a functioning /proc/self/fd
  or /dev/fd interface the max is now ignored and all fds are closed.

- Issue #20383: Introduce importlib.util.module_from_spec() as the preferred way
  to create a new module.

- Issue #21552: Fixed possible integer overflow of too long string lengths in
  the tkinter module on 64-bit platforms.

- Issue #14315: The zipfile module now ignores extra fields in the central
  directory that are too short to be parsed instead of letting a struct.unpack
  error bubble up as this "bad data" appears in many real world zip files in
  the wild and is ignored by other zip tools.

- Issue #13742: Added "key" and "reverse" parameters to heapq.merge().
  (First draft of patch contributed by Simon Sapin.)

- Issue #21402: tkinter.ttk now works when default root window is not set.

- Issue #3015: _tkinter.create() now creates tkapp object with wantobject=1 by
  default.

- Issue #10203: sqlite3.Row now truly supports sequence protocol.  In particular
  it supports reverse() and negative indices.  Original patch by Claudiu Popa.

- Issue #18807: If copying (no symlinks) specified for a venv, then the python
  interpreter aliases (python, python3) are now created by copying rather than
  symlinking.

- Issue #20197: Added support for the WebP image type in the imghdr module.
  Patch by Fabrice Aneche and Claudiu Popa.

- Issue #21513: Speedup some properties of IP addresses (IPv4Address,
  IPv6Address) such as .is_private or .is_multicast.

- Issue #21137: Improve the repr for threading.Lock() and its variants
  by showing the "locked" or "unlocked" status.  Patch by Berker Peksag.

- Issue #21538: The plistlib module now supports loading of binary plist files
  when reference or offset size is not a power of two.

- Issue #21455: Add a default backlog to socket.listen().

- Issue #21525: Most Tkinter methods which accepted tuples now accept lists too.

- Issue #22166: With the assistance of a new internal _codecs._forget_codec
  helping function, test_codecs now clears the encoding caches to avoid the
  appearance of a reference leak

- Issue #22236: Tkinter tests now don't reuse default root window.  New root
  window is created for every test class.

- Issue #10744: Fix PEP 3118 format strings on ctypes objects with a nontrivial
  shape.

- Issue #20826: Optimize ipaddress.collapse_addresses().

- Issue #21487: Optimize ipaddress.summarize_address_range() and
  ipaddress.{IPv4Network,IPv6Network}.subnets().

- Issue #21486: Optimize parsing of netmasks in ipaddress.IPv4Network and
  ipaddress.IPv6Network.

- Issue #13916: Disallowed the surrogatepass error handler for non UTF-\*
  encodings.

- Issue #20998: Fixed re.fullmatch() of repeated single character pattern
  with ignore case.  Original patch by Matthew Barnett.

- Issue #21075: fileinput.FileInput now reads bytes from standard stream if
  binary mode is specified.  Patch by Sam Kimbrel.

- Issue #19775: Add a samefile() method to pathlib Path objects.  Initial
  patch by Vajrasky Kok.

- Issue #21226: Set up modules properly in PyImport_ExecCodeModuleObject
  (and friends).

- Issue #21398: Fix a unicode error in the pydoc pager when the documentation
  contains characters not encodable to the stdout encoding.

- Issue #16531: ipaddress.IPv4Network and ipaddress.IPv6Network now accept
  an (address, netmask) tuple argument, so as to easily construct network
  objects from existing addresses.

- Issue #21156: importlib.abc.InspectLoader.source_to_code() is now a
  staticmethod.

- Issue #21424: Simplified and optimized heaqp.nlargest() and nmsmallest()
  to make fewer tuple comparisons.

- Issue #21396: Fix TextIOWrapper(..., write_through=True) to not force a
  flush() on the underlying binary stream.  Patch by akira.

- Issue #18314: Unlink now removes junctions on Windows. Patch by Kim Gräsman

- Issue #21088: Bugfix for curses.window.addch() regression in 3.4.0.
  In porting to Argument Clinic, the first two arguments were reversed.

- Issue #21407: _decimal: The module now supports function signatures.

- Issue #10650: Remove the non-standard 'watchexp' parameter from the
  Decimal.quantize() method in the Python version.  It had never been
  present in the C version.

- Issue #21469: Reduced the risk of false positives in robotparser by
  checking to make sure that robots.txt has been read or does not exist
  prior to returning True in can_fetch().

- Issue #19414: Have the OrderedDict mark deleted links as unusable.
  This gives an early failure if the link is deleted during iteration.

- Issue #21421: Add __slots__ to the MappingViews ABC.
  Patch by Josh Rosenberg.

- Issue #21101: Eliminate double hashing in the C speed-up code for
  collections.Counter().

- Issue #21321: itertools.islice() now releases the reference to the source
  iterator when the slice is exhausted.  Patch by Anton Afanasyev.

- Issue #21057: TextIOWrapper now allows the underlying binary stream's
  read() or read1() method to return an arbitrary bytes-like object
  (such as a memoryview).  Patch by Nikolaus Rath.

- Issue #20951: SSLSocket.send() now raises either SSLWantReadError or
  SSLWantWriteError on a non-blocking socket if the operation would block.
  Previously, it would return 0.  Patch by Nikolaus Rath.

- Issue #13248: removed previously deprecated asyncore.dispatcher __getattr__
  cheap inheritance hack.

- Issue #9815: assertRaises now tries to clear references to local variables
  in the exception's traceback.

- Issue #19940: ssl.cert_time_to_seconds() now interprets the given time
  string in the UTC timezone (as specified in RFC 5280), not the local
  timezone.

- Issue #13204: Calling sys.flags.__new__ would crash the interpreter,
  now it raises a TypeError.

- Issue #19385: Make operations on a closed dbm.dumb database always raise the
  same exception.

- Issue #21207: Detect when the os.urandom cached fd has been closed or
  replaced, and open it anew.

- Issue #21291: subprocess's Popen.wait() is now thread safe so that
  multiple threads may be calling wait() or poll() on a Popen instance
  at the same time without losing the Popen.returncode value.

- Issue #21127: Path objects can now be instantiated from str subclass
  instances (such as ``numpy.str_``).

- Issue #15002: urllib.response object to use _TemporaryFileWrapper (and
  _TemporaryFileCloser) facility. Provides a better way to handle file
  descriptor close. Patch contributed by Christian Theune.

- Issue #12220: mindom now raises a custom ValueError indicating it doesn't
  support spaces in URIs instead of letting a 'split' ValueError bubble up.

- Issue #21068: The ssl.PROTOCOL* constants are now enum members.

- Issue #21276: posixmodule: Don't define USE_XATTRS on KFreeBSD and the Hurd.

- Issue #21262: New method assert_not_called for Mock.
  It raises AssertionError if the mock has been called.

- Issue #21238: New keyword argument `unsafe` to Mock. It raises
  `AttributeError` incase of an attribute startswith assert or assret.

- Issue #20896: ssl.get_server_certificate() now uses PROTOCOL_SSLv23, not
  PROTOCOL_SSLv3, for maximum compatibility.

- Issue #21239: patch.stopall() didn't work deterministically when the same
  name was patched more than once.

- Issue #21203: Updated fileConfig and dictConfig to remove inconsistencies.
  Thanks to Jure Koren for the patch.

- Issue #21222: Passing name keyword argument to mock.create_autospec now
  works.

- Issue #21197: Add lib64 -> lib symlink in venvs on 64-bit non-OS X POSIX.

- Issue #17498: Some SMTP servers disconnect after certain errors, violating
  strict RFC conformance.  Instead of losing the error code when we issue the
  subsequent RSET, smtplib now returns the error code and defers raising the
  SMTPServerDisconnected error until the next command is issued.

- Issue #17826: setting an iterable side_effect on a mock function created by
  create_autospec now works. Patch by Kushal Das.

- Issue #7776: Fix ``Host:`` header and reconnection when using
  http.client.HTTPConnection.set_tunnel(). Patch by Nikolaus Rath.

- Issue #20968: unittest.mock.MagicMock now supports division.
  Patch by Johannes Baiter.

- Issue #21529 (CVE-2014-4616): Fix arbitrary memory access in
  JSONDecoder.raw_decode with a negative second parameter. Bug reported by Guido
  Vranken.

- Issue #21169: getpass now handles non-ascii characters that the
  input stream encoding cannot encode by re-encoding using the
  replace error handler.

- Issue #21171: Fixed undocumented filter API of the rot13 codec.
  Patch by Berker Peksag.

- Issue #20539: Improved math.factorial error message for large positive inputs
  and changed exception type (OverflowError -> ValueError) for large negative
  inputs.

- Issue #21172: isinstance check relaxed from dict to collections.Mapping.

- Issue #21155: asyncio.EventLoop.create_unix_server() now raises a ValueError
  if path and sock are specified at the same time.

- Issue #21136: Avoid unnecessary normalization of Fractions resulting from
  power and other operations.  Patch by Raymond Hettinger.

- Issue #17621: Introduce importlib.util.LazyLoader.

- Issue #21076: signal module constants were turned into enums.
  Patch by Giampaolo Rodola'.

- Issue #20636: Improved the repr of Tkinter widgets.

- Issue #19505: The items, keys, and values views of OrderedDict now support
  reverse iteration using reversed().

- Issue #21149: Improved thread-safety in logging cleanup during interpreter
  shutdown. Thanks to Devin Jeanpierre for the patch.

- Issue #21058: Fix a leak of file descriptor in
  :func:`tempfile.NamedTemporaryFile`, close the file descriptor if
  :func:`io.open` fails

- Issue #21200: Return None from pkgutil.get_loader() when __spec__ is missing.

- Issue #21013: Enhance ssl.create_default_context() when used for server side
  sockets to provide better security by default.

- Issue #20145: `assertRaisesRegex` and `assertWarnsRegex` now raise a
  TypeError if the second argument is not a string or compiled regex.

- Issue #20633: Replace relative import by absolute import.

- Issue #20980: Stop wrapping exception when using ThreadPool.

- Issue #21082: In os.makedirs, do not set the process-wide umask. Note this
  changes behavior of makedirs when exist_ok=True.

- Issue #20990: Fix issues found by pyflakes for multiprocessing.

- Issue #21015: SSL contexts will now automatically select an elliptic
  curve for ECDH key exchange on OpenSSL 1.0.2 and later, and otherwise
  default to "prime256v1".

- Issue #21000: Improve the command-line interface of json.tool.

- Issue #20995: Enhance default ciphers used by the ssl module to enable
  better security and prioritize perfect forward secrecy.

- Issue #20884: Don't assume that __file__ is defined on importlib.__init__.

- Issue #21499: Ignore __builtins__ in several test_importlib.test_api tests.

- Issue #20627: xmlrpc.client.ServerProxy is now a context manager.

- Issue #19165: The formatter module now raises DeprecationWarning instead of
  PendingDeprecationWarning.

- Issue #13936: Remove the ability of datetime.time instances to be considered
  false in boolean contexts.

- Issue #18931: selectors module now supports /dev/poll on Solaris.
  Patch by Giampaolo Rodola'.

- Issue #19977: When the ``LC_TYPE`` locale is the POSIX locale (``C`` locale),
  :py:data:`sys.stdin` and :py:data:`sys.stdout` are now using the
  ``surrogateescape`` error handler, instead of the ``strict`` error handler.

- Issue #20574: Implement incremental decoder for cp65001 code (Windows code
  page 65001, Microsoft UTF-8).

- Issue #20879: Delay the initialization of encoding and decoding tables for
  base32, ascii85 and base85 codecs in the base64 module, and delay the
  initialization of the unquote_to_bytes() table of the urllib.parse module, to
  not waste memory if these modules are not used.

- Issue #19157: Include the broadcast address in the usuable hosts for IPv6
  in ipaddress.

- Issue #11599: When an external command (e.g. compiler) fails, distutils now
  prints out the whole command line (instead of just the command name) if the
  environment variable DISTUTILS_DEBUG is set.

- Issue #4931: distutils should not produce unhelpful "error: None" messages
  anymore.  distutils.util.grok_environment_error is kept but doc-deprecated.

- Issue #20875: Prevent possible gzip "'read' is not defined" NameError.
  Patch by Claudiu Popa.

- Issue #11558: ``email.message.Message.attach`` now returns a more
  useful error message if ``attach`` is called on a message for which
  ``is_multipart`` is False.

- Issue #20283: RE pattern methods now accept the string keyword parameters
  as documented.  The pattern and source keyword parameters are left as
  deprecated aliases.

- Issue #20778: Fix modulefinder to work with bytecode-only modules.

- Issue #20791: copy.copy() now doesn't make a copy when the input is
  a bytes object.  Initial patch by Peter Otten.

- Issue #19748: On AIX, time.mktime() now raises an OverflowError for year
  outsize range [1902; 2037].

- Issue #19573: inspect.signature: Use enum for parameter kind constants.

- Issue #20726: inspect.signature: Make Signature and Parameter picklable.

- Issue #17373: Add inspect.Signature.from_callable method.

- Issue #20378: Improve repr of inspect.Signature and inspect.Parameter.

- Issue #20816: Fix inspect.getcallargs() to raise correct TypeError for
  missing keyword-only arguments. Patch by Jeremiah Lowin.

- Issue #20817: Fix inspect.getcallargs() to fail correctly if more
  than 3 arguments are missing. Patch by Jeremiah Lowin.

- Issue #6676: Ensure a meaningful exception is raised when attempting
  to parse more than one XML document per pyexpat xmlparser instance.
  (Original patches by Hirokazu Yamamoto and Amaury Forgeot d'Arc, with
  suggested wording by David Gutteridge)

- Issue #21117: Fix inspect.signature to better support functools.partial.
  Due to the specifics of functools.partial implementation,
  positional-or-keyword arguments passed as keyword arguments become
  keyword-only.

- Issue #20334: inspect.Signature and inspect.Parameter are now hashable.
  Thanks to Antony Lee for bug reports and suggestions.

- Issue #15916: doctest.DocTestSuite returns an empty unittest.TestSuite instead
  of raising ValueError if it finds no tests

- Issue #21209: Fix asyncio.tasks.CoroWrapper to workaround a bug
  in yield-from implementation in CPythons prior to 3.4.1.

- asyncio: Add gi_{frame,running,code} properties to CoroWrapper
  (upstream issue #163).

- Issue #21311: Avoid exception in _osx_support with non-standard compiler
  configurations.  Patch by John Szakmeister.

- Issue #11571: Ensure that the turtle window becomes the topmost window
  when launched on OS X.

- Issue #21801: Validate that __signature__ is None or an instance of Signature.

- Issue #21923: Prevent AttributeError in distutils.sysconfig.customize_compiler
  due to possible uninitialized _config_vars.

- Issue #21323: Fix http.server to again handle scripts in CGI subdirectories,
  broken by the fix for security issue #19435.  Patch by Zach Byrne.

- Issue #22733: Fix ffi_prep_args not zero-extending argument values correctly
  on 64-bit Windows.

- Issue #23302: Default to TCP_NODELAY=1 upon establishing an HTTPConnection.
  Removed use of hard-coded MSS as it's an optimization that's no longer needed
  with Nagle disabled.

