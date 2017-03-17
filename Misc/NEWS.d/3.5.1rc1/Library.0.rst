- Issue #25626: Change three zlib functions to accept sizes that fit in
  Py_ssize_t, but internally cap those sizes to UINT_MAX.  This resolves a
  regression in 3.5 where GzipFile.read() failed to read chunks larger than 2
  or 4 GiB.  The change affects the zlib.Decompress.decompress() max_length
  parameter, the zlib.decompress() bufsize parameter, and the
  zlib.Decompress.flush() length parameter.

- Issue #25583: Avoid incorrect errors raised by os.makedirs(exist_ok=True)
  when the OS gives priority to errors such as EACCES over EEXIST.

- Issue #25593: Change semantics of EventLoop.stop() in asyncio.

- Issue #6973: When we know a subprocess.Popen process has died, do
  not allow the send_signal(), terminate(), or kill() methods to do
  anything as they could potentially signal a different process.

- Issue #25590: In the Readline completer, only call getattr() once per
  attribute.

- Issue #25498: Fix a crash when garbage-collecting ctypes objects created
  by wrapping a memoryview.  This was a regression made in 3.5a1.  Based
  on patch by Eryksun.

- Issue #25584: Added "escape" to the __all__ list in the glob module.

- Issue #25584: Fixed recursive glob() with patterns starting with '\*\*'.

- Issue #25446: Fix regression in smtplib's AUTH LOGIN support.

- Issue #18010: Fix the pydoc web server's module search function to handle
  exceptions from importing packages.

- Issue #25554: Got rid of circular references in regular expression parsing.

- Issue #25510: fileinput.FileInput.readline() now returns b'' instead of ''
  at the end if the FileInput was opened with binary mode.
  Patch by Ryosuke Ito.

- Issue #25503: Fixed inspect.getdoc() for inherited docstrings of properties.
  Original patch by John Mark Vandenberg.

- Issue #25515: Always use os.urandom as a source of randomness in uuid.uuid4.

- Issue #21827: Fixed textwrap.dedent() for the case when largest common
  whitespace is a substring of smallest leading whitespace.
  Based on patch by Robert Li.

- Issue #25447: The lru_cache() wrapper objects now can be copied and pickled
  (by returning the original object unchanged).

- Issue #25390: typing: Don't crash on Union[str, Pattern].

- Issue #25441: asyncio: Raise error from drain() when socket is closed.

- Issue #25410: Cleaned up and fixed minor bugs in C implementation of
  OrderedDict.

- Issue #25411: Improved Unicode support in SMTPHandler through better use of
  the email package. Thanks to user simon04 for the patch.

- Issue #25407: Remove mentions of the formatter module being removed in
  Python 3.6.

- Issue #25406: Fixed a bug in C implementation of OrderedDict.move_to_end()
  that caused segmentation fault or hang in iterating after moving several
  items to the start of ordered dict.

- Issue #25364: zipfile now works in threads disabled builds.

- Issue #25328: smtpd's SMTPChannel now correctly raises a ValueError if both
  decode_data and enable_SMTPUTF8 are set to true.

- Issue #25316: distutils raises OSError instead of DistutilsPlatformError
  when MSVC is not installed.

- Issue #25380: Fixed protocol for the STACK_GLOBAL opcode in
  pickletools.opcodes.

- Issue #23972: Updates asyncio datagram create method allowing reuseport
  and reuseaddr socket options to be set prior to binding the socket.
  Mirroring the existing asyncio create_server method the reuseaddr option
  for datagram sockets defaults to True if the O/S is 'posix' (except if the
  platform is Cygwin). Patch by Chris Laws.

- Issue #25304: Add asyncio.run_coroutine_threadsafe().  This lets you
  submit a coroutine to a loop from another thread, returning a
  concurrent.futures.Future.  By Vincent Michel.

- Issue #25232: Fix CGIRequestHandler to split the query from the URL at the
  first question mark (?) rather than the last. Patch from Xiang Zhang.

- Issue #24657: Prevent CGIRequestHandler from collapsing slashes in the
  query part of the URL as if it were a path. Patch from Xiang Zhang.

- Issue #24483: C implementation of functools.lru_cache() now calculates key's
  hash only once.

- Issue #22958: Constructor and update method of weakref.WeakValueDictionary
  now accept the self and the dict keyword arguments.

- Issue #22609: Constructor of collections.UserDict now accepts the self keyword
  argument.

- Issue #25111: Fixed comparison of traceback.FrameSummary.

- Issue #25262: Added support for BINBYTES8 opcode in Python implementation of
  unpickler.  Highest 32 bits of 64-bit size for BINUNICODE8 and BINBYTES8
  opcodes no longer silently ignored on 32-bit platforms in C implementation.

- Issue #25034: Fix string.Formatter problem with auto-numbering and
  nested format_specs. Patch by Anthon van der Neut.

- Issue #25233: Rewrite the guts of asyncio.Queue and
  asyncio.Semaphore to be more understandable and correct.

- Issue #25203: Failed readline.set_completer_delims() no longer left the
  module in inconsistent state.

- Issue #23600: Default implementation of tzinfo.fromutc() was returning
  wrong results in some cases.

- Issue #23329: Allow the ssl module to be built with older versions of
  LibreSSL.

- Prevent overflow in _Unpickler_Read.

- Issue #25047: The XML encoding declaration written by Element Tree now
  respects the letter case given by the user. This restores the ability to
  write encoding names in uppercase like "UTF-8", which worked in Python 2.

- Issue #25135: Make deque_clear() safer by emptying the deque before clearing.
  This helps avoid possible reentrancy issues.

- Issue #19143: platform module now reads Windows version from kernel32.dll to
  avoid compatibility shims.

- Issue #25092: Fix datetime.strftime() failure when errno was already set to
  EINVAL.

- Issue #23517: Fix rounding in fromtimestamp() and utcfromtimestamp() methods
  of datetime.datetime: microseconds are now rounded to nearest with ties
  going to nearest even integer (ROUND_HALF_EVEN), instead of being rounding
  towards minus infinity (ROUND_FLOOR). It's important that these methods use
  the same rounding mode than datetime.timedelta to keep the property:
  (datetime(1970,1,1) + timedelta(seconds=t)) == datetime.utcfromtimestamp(t).
  It also the rounding mode used by round(float) for example.

- Issue #25155: Fix datetime.datetime.now() and datetime.datetime.utcnow() on
  Windows to support date after year 2038. It was a regression introduced in
  Python 3.5.0.

- Issue #25108: Omitted internal frames in traceback functions print_stack(),
  format_stack(), and extract_stack() called without arguments.

- Issue #25118: Fix a regression of Python 3.5.0 in os.waitpid() on Windows.

- Issue #24684: socket.socket.getaddrinfo() now calls
  PyUnicode_AsEncodedString() instead of calling the encode() method of the
  host, to handle correctly custom string with an encode() method which doesn't
  return a byte string. The encoder of the IDNA codec is now called directly
  instead of calling the encode() method of the string.

- Issue #25060: Correctly compute stack usage of the BUILD_MAP opcode.

- Issue #24857: Comparing call_args to a long sequence now correctly returns a
  boolean result instead of raising an exception.  Patch by A Kaptur.

- Issue #23144: Make sure that HTMLParser.feed() returns all the data, even
  when convert_charrefs is True.

- Issue #24982: shutil.make_archive() with the "zip" format now adds entries
  for directories (including empty directories) in ZIP file.

- Issue #25019: Fixed a crash caused by setting non-string key of expat parser.
  Based on patch by John Leitch.

- Issue #16180: Exit pdb if file has syntax error, instead of trapping user
  in an infinite loop.  Patch by Xavier de Gaye.

- Issue #24891: Fix a race condition at Python startup if the file descriptor
  of stdin (0), stdout (1) or stderr (2) is closed while Python is creating
  sys.stdin, sys.stdout and sys.stderr objects. These attributes are now set
  to None if the creation of the object failed, instead of raising an OSError
  exception. Initial patch written by Marco Paolini.

- Issue #24992: Fix error handling and a race condition (related to garbage
  collection) in collections.OrderedDict constructor.

- Issue #24881: Fixed setting binary mode in Python implementation of FileIO
  on Windows and Cygwin.  Patch from Akira Li.

- Issue #25578: Fix (another) memory leak in SSLSocket.getpeercer().

- Issue #25530: Disable the vulnerable SSLv3 protocol by default when creating
  ssl.SSLContext.

- Issue #25569: Fix memory leak in SSLSocket.getpeercert().

- Issue #25471: Sockets returned from accept() shouldn't appear to be
  nonblocking.

- Issue #25319: When threading.Event is reinitialized, the underlying condition
  should use a regular lock rather than a recursive lock.

- Issue #21112: Fix regression in unittest.expectedFailure on subclasses.
  Patch from Berker Peksag.

- Issue #24764: cgi.FieldStorage.read_multi() now ignores the Content-Length
  header in part headers. Patch written by Peter Landry and reviewed by Pierre
  Quentel.

- Issue #24913: Fix overrun error in deque.index().
  Found by John Leitch and Bryce Darling.

- Issue #24774: Fix docstring in http.server.test. Patch from Chiu-Hsiang Hsu.

- Issue #21159: Improve message in configparser.InterpolationMissingOptionError.
  Patch from Łukasz Langa.

- Issue #20362: Honour TestCase.longMessage correctly in assertRegex.
  Patch from Ilia Kurenkov.

- Issue #23572: Fixed functools.singledispatch on classes with falsy
  metaclasses.  Patch by Ethan Furman.

- asyncio: ensure_future() now accepts awaitable objects.

