- [Security] Issue #26556: Update expat to 2.1.1, fixes CVE-2015-1283.

- [Security] Fix TLS stripping vulnerability in smtplib, CVE-2016-0772.
  Reported by Team Oststrom

- Issue #21386: Implement missing IPv4Address.is_global property.  It was
  documented since 07a5610bae9d.  Initial patch by Roger Luethi.

- Issue #20900: distutils register command now decodes HTTP responses
  correctly.  Initial patch by ingrid.

- A new version of typing.py provides several new classes and
  features: @overload outside stubs, Reversible, DefaultDict, Text,
  ContextManager, Type[], NewType(), TYPE_CHECKING, and numerous bug
  fixes (note that some of the new features are not yet implemented in
  mypy or other static analyzers).  Also classes for PEP 492
  (Awaitable, AsyncIterable, AsyncIterator) have been added (in fact
  they made it into 3.5.1 but were never mentioned).

- Issue #25738: Stop http.server.BaseHTTPRequestHandler.send_error() from
  sending a message body for 205 Reset Content.  Also, don't send Content
  header fields in responses that don't have a body.  Patch by Susumu
  Koshiba.

- Issue #21313: Fix the "platform" module to tolerate when sys.version
  contains truncated build information.

- [Security] Issue #26839: On Linux, :func:`os.urandom` now calls
  ``getrandom()`` with ``GRND_NONBLOCK`` to fall back on reading
  ``/dev/urandom`` if the urandom entropy pool is not initialized yet. Patch
  written by Colm Buckley.

- Issue #27164: In the zlib module, allow decompressing raw Deflate streams
  with a predefined zdict.  Based on patch by Xiang Zhang.

- Issue #24291: Fix wsgiref.simple_server.WSGIRequestHandler to completely
  write data to the client.  Previously it could do partial writes and
  truncate data.  Also, wsgiref.handler.ServerHandler can now handle stdout
  doing partial writes, but this is deprecated.

- Issue #26809: Add ``__all__`` to :mod:`string`.  Patch by Emanuel Barry.

- Issue #26373: subprocess.Popen.communicate now correctly ignores
  BrokenPipeError when the child process dies before .communicate()
  is called in more/all circumstances.

- Issue #21776: distutils.upload now correctly handles HTTPError.
  Initial patch by Claudiu Popa.

- Issue #27114: Fix SSLContext._load_windows_store_certs fails with
  PermissionError

- Issue #18383: Avoid creating duplicate filters when using filterwarnings
  and simplefilter.  Based on patch by Alex Shkop.

- Issue #27057: Fix os.set_inheritable() on Android, ioctl() is blocked by
  SELinux and fails with EACCESS. The function now falls back to fcntl().
  Patch written by Michał Bednarski.

- Issue #27014: Fix infinite recursion using typing.py.  Thanks to Kalle Tuure!

- Issue #14132: Fix urllib.request redirect handling when the target only has
  a query string.  Original fix by Ján Janech.

- Issue #17214: The "urllib.request" module now percent-encodes non-ASCII
  bytes found in redirect target URLs.  Some servers send Location header
  fields with non-ASCII bytes, but "http.client" requires the request target
  to be ASCII-encodable, otherwise a UnicodeEncodeError is raised.  Based on
  patch by Christian Heimes.

- Issue #26892: Honor debuglevel flag in urllib.request.HTTPHandler. Patch
  contributed by Chi Hsuan Yen.

- Issue #22274: In the subprocess module, allow stderr to be redirected to
  stdout even when stdout is not redirected.  Patch by Akira Li.

- Issue #26807: mock_open 'files' no longer error on readline at end of file.
  Patch from Yolanda Robla.

- Issue #25745: Fixed leaking a userptr in curses panel destructor.

- Issue #26977: Removed unnecessary, and ignored, call to sum of squares helper
  in statistics.pvariance.

- Issue #26881: The modulefinder module now supports extended opcode arguments.

- Issue #23815: Fixed crashes related to directly created instances of types in
  _tkinter and curses.panel modules.

- Issue #17765: weakref.ref() no longer silently ignores keyword arguments.
  Patch by Georg Brandl.

- Issue #26873: xmlrpc now raises ResponseError on unsupported type tags
  instead of silently return incorrect result.

- Issue #26711: Fixed the comparison of plistlib.Data with other types.

- Issue #24114: Fix an uninitialized variable in `ctypes.util`.
  
  The bug only occurs on SunOS when the ctypes implementation searches
  for the `crle` program.  Patch by Xiang Zhang.  Tested on SunOS by
  Kees Bos.

- Issue #26864: In urllib.request, change the proxy bypass host checking
  against no_proxy to be case-insensitive, and to not match unrelated host
  names that happen to have a bypassed hostname as a suffix.  Patch by Xiang
  Zhang.

- Issue #26634: recursive_repr() now sets __qualname__ of wrapper.  Patch by
  Xiang Zhang.

- Issue #26804: urllib.request will prefer lower_case proxy environment
  variables over UPPER_CASE or Mixed_Case ones. Patch contributed by Hans-Peter
  Jansen.

- Issue #26837: assertSequenceEqual() now correctly outputs non-stringified
  differing items (like bytes in the -b mode).  This affects assertListEqual()
  and assertTupleEqual().

- Issue #26041: Remove "will be removed in Python 3.7" from deprecation
  messages of platform.dist() and platform.linux_distribution().
  Patch by Kumaripaba Miyurusara Athukorala.

- Issue #26822: itemgetter, attrgetter and methodcaller objects no longer
  silently ignore keyword arguments.

- Issue #26733: Disassembling a class now disassembles class and static methods.
  Patch by Xiang Zhang.

- Issue #26801: Fix error handling in :func:`shutil.get_terminal_size`, catch
  :exc:`AttributeError` instead of :exc:`NameError`. Patch written by Emanuel
  Barry.

- Issue #24838: tarfile's ustar and gnu formats now correctly calculate name
  and link field limits for multibyte character encodings like utf-8.

- [Security] Issue #26657: Fix directory traversal vulnerability with
  http.server on Windows.  This fixes a regression that was introduced in
  3.3.4rc1 and 3.4.0rc1.  Based on patch by Philipp Hagemeister.

- Issue #26717: Stop encoding Latin-1-ized WSGI paths with UTF-8.  Patch by
  Anthony Sottile.

- Issue #26735: Fix :func:`os.urandom` on Solaris 11.3 and newer when reading
  more than 1,024 bytes: call ``getrandom()`` multiple times with a limit of
  1024 bytes per call.

- Issue #16329: Add .webm to mimetypes.types_map.  Patch by Giampaolo Rodola'.

- Issue #13952: Add .csv to mimetypes.types_map.  Patch by Geoff Wilson.

- Issue #26709: Fixed Y2038 problem in loading binary PLists.

- Issue #23735: Handle terminal resizing with Readline 6.3+ by installing our
  own SIGWINCH handler.  Patch by Eric Price.

- Issue #26586: In http.server, respond with "413 Request header fields too
  large" if there are too many header fields to parse, rather than killing
  the connection and raising an unhandled exception.  Patch by Xiang Zhang.

- Issue #22854: Change BufferedReader.writable() and
  BufferedWriter.readable() to always return False.

- Issue #25195: Fix a regression in mock.MagicMock. _Call is a subclass of
  tuple (changeset 3603bae63c13 only works for classes) so we need to
  implement __ne__ ourselves.  Patch by Andrew Plummer.

- Issue #26644: Raise ValueError rather than SystemError when a negative
  length is passed to SSLSocket.recv() or read().

- Issue #23804: Fix SSL recv(0) and read(0) methods to return zero bytes
  instead of up to 1024.

- Issue #26616: Fixed a bug in datetime.astimezone() method.

- Issue #21925: :func:`warnings.formatwarning` now catches exceptions on
  ``linecache.getline(...)`` to be able to log :exc:`ResourceWarning` emitted
  late during the Python shutdown process.

- Issue #24266: Ctrl+C during Readline history search now cancels the search
  mode when compiled with Readline 7.

- Issue #26560: Avoid potential ValueError in BaseHandler.start_response.
  Initial patch by Peter Inglesby.

- [Security] Issue #26313: ssl.py _load_windows_store_certs fails if windows
  cert store is empty. Patch by Baji.

- Issue #26569: Fix :func:`pyclbr.readmodule` and :func:`pyclbr.readmodule_ex`
  to support importing packages.

- Issue #26499: Account for remaining Content-Length in
  HTTPResponse.readline() and read1().  Based on patch by Silent Ghost.
  Also document that HTTPResponse now supports these methods.

- Issue #25320: Handle sockets in directories unittest discovery is scanning.
  Patch from Victor van den Elzen.

- Issue #16181: cookiejar.http2time() now returns None if year is higher than
  datetime.MAXYEAR.

- Issue #26513: Fixes platform module detection of Windows Server

- Issue #23718: Fixed parsing time in week 0 before Jan 1.  Original patch by
  Tamás Bence Gedai.

- Issue #20589: Invoking Path.owner() and Path.group() on Windows now raise
  NotImplementedError instead of ImportError.

- Issue #26177: Fixed the keys() method for Canvas and Scrollbar widgets.

- Issue #15068: Got rid of excessive buffering in the fileinput module.
  The bufsize parameter is no longer used.

- Issue #2202: Fix UnboundLocalError in
  AbstractDigestAuthHandler.get_algorithm_impls.  Initial patch by Mathieu
  Dupuy.

- Issue #25718: Fixed pickling and copying the accumulate() iterator with
  total is None.

- Issue #26475: Fixed debugging output for regular expressions with the (?x)
  flag.

- Issue #26457: Fixed the subnets() methods in IP network classes for the case
  when resulting prefix length is equal to maximal prefix length.
  Based on patch by Xiang Zhang.

- Issue #26385: Remove the file if the internal open() call in
  NamedTemporaryFile() fails.  Patch by Silent Ghost.

- Issue #26402: Fix XML-RPC client to retry when the server shuts down a
  persistent connection.  This was a regression related to the new
  http.client.RemoteDisconnected exception in 3.5.0a4.

- Issue #25913: Leading ``<~`` is optional now in base64.a85decode() with
  adobe=True.  Patch by Swati Jaiswal.

- Issue #26186: Remove an invalid type check in importlib.util.LazyLoader.

- Issue #26367: importlib.__import__() raises SystemError like
  builtins.__import__() when ``level`` is specified but without an accompanying
  package specified.

- Issue #26309: In the "socketserver" module, shut down the request (closing
  the connected socket) when verify_request() returns false.  Patch by Aviv
  Palivoda.

- [Security] Issue #25939: On Windows open the cert store readonly in
  ssl.enum_certificates.

- Issue #25995: os.walk() no longer uses FDs proportional to the tree depth.

- Issue #26117: The os.scandir() iterator now closes file descriptor not only
  when the iteration is finished, but when it was failed with error.

- Issue #25911: Restored support of bytes paths in os.walk() on Windows.

- Issue #26045: Add UTF-8 suggestion to error message when posting a
  non-Latin-1 string with http.client.

- Issue #12923: Reset FancyURLopener's redirect counter even if there is an
  exception.  Based on patches by Brian Brazil and Daniel Rocco.

- Issue #25945: Fixed a crash when unpickle the functools.partial object with
  wrong state.  Fixed a leak in failed functools.partial constructor.
  "args" and "keywords" attributes of functools.partial have now always types
  tuple and dict correspondingly.

- Issue #26202: copy.deepcopy() now correctly copies range() objects with
  non-atomic attributes.

- Issue #23076: Path.glob() now raises a ValueError if it's called with an
  invalid pattern.  Patch by Thomas Nyberg.

- Issue #19883: Fixed possible integer overflows in zipimport.

- Issue #26227: On Windows, getnameinfo(), gethostbyaddr() and
  gethostbyname_ex() functions of the socket module now decode the hostname
  from the ANSI code page rather than UTF-8.

- Issue #26147: xmlrpc now works with strings not encodable with used
  non-UTF-8 encoding.

- Issue #25935: Garbage collector now breaks reference loops with OrderedDict.

- Issue #16620: Fixed AttributeError in msilib.Directory.glob().

- Issue #26013: Added compatibility with broken protocol 2 pickles created
  in old Python 3 versions (3.4.3 and lower).

- Issue #25850: Use cross-compilation by default for 64-bit Windows.

- Issue #17633: Improve zipimport's support for namespace packages.

- Issue #24705: Fix sysconfig._parse_makefile not expanding ${} vars
  appearing before $() vars.

- Issue #22138: Fix mock.patch behavior when patching descriptors. Restore
  original values after patching. Patch contributed by Sean McCully.

- Issue #25672: In the ssl module, enable the SSL_MODE_RELEASE_BUFFERS mode
  option if it is safe to do so.

- Issue #26012: Don't traverse into symlinks for ** pattern in
  pathlib.Path.[r]glob().

- Issue #24120: Ignore PermissionError when traversing a tree with
  pathlib.Path.[r]glob().  Patch by Ulrich Petri.

- Issue #25447: fileinput now uses sys.stdin as-is if it does not have a
  buffer attribute (restores backward compatibility).

- Issue #25447: Copying the lru_cache() wrapper object now always works,
  independedly from the type of the wrapped object (by returning the original
  object unchanged).

- Issue #24103: Fixed possible use after free in ElementTree.XMLPullParser.

- Issue #25860: os.fwalk() no longer skips remaining directories when error
  occurs.  Original patch by Samson Lee.

- Issue #25914: Fixed and simplified OrderedDict.__sizeof__.

- Issue #25902: Fixed various refcount issues in ElementTree iteration.

- Issue #25717: Restore the previous behaviour of tolerating most fstat()
  errors when opening files.  This was a regression in 3.5a1, and stopped
  anonymous temporary files from working in special cases.

- Issue #24903: Fix regression in number of arguments compileall accepts when
  '-d' is specified.  The check on the number of arguments has been dropped
  completely as it never worked correctly anyway.

- Issue #25764: In the subprocess module, preserve any exception caused by
  fork() failure when preexec_fn is used.

- Issue #6478: _strptime's regexp cache now is reset after changing timezone
  with time.tzset().

- Issue #14285: When executing a package with the "python -m package" option,
  and package initialization fails, a proper traceback is now reported.  The
  "runpy" module now lets exceptions from package initialization pass back to
  the caller, rather than raising ImportError.

- Issue #19771: Also in runpy and the "-m" option, omit the irrelevant
  message ". . . is a package and cannot be directly executed" if the package
  could not even be initialized (e.g. due to a bad ``*.pyc`` file).

- Issue #25177: Fixed problem with the mean of very small and very large
  numbers. As a side effect, statistics.mean and statistics.variance should
  be significantly faster.

- Issue #25718: Fixed copying object with state with boolean value is false.

- Issue #10131: Fixed deep copying of minidom documents.  Based on patch
  by Marian Ganisin.

- Issue #25725: Fixed a reference leak in pickle.loads() when unpickling
  invalid data including tuple instructions.

- Issue #25663: In the Readline completer, avoid listing duplicate global
  names, and search the global namespace before searching builtins.

- Issue #25688: Fixed file leak in ElementTree.iterparse() raising an error.

- Issue #23914: Fixed SystemError raised by unpickler on broken pickle data.

- Issue #25691: Fixed crash on deleting ElementTree.Element attributes.

- Issue #25624: ZipFile now always writes a ZIP_STORED header for directory
  entries.  Patch by Dingyuan Wang.

- Skip getaddrinfo if host is already resolved.
  Patch by A. Jesse Jiryu Davis.

- Issue #26050: Add asyncio.StreamReader.readuntil() method.
  Patch by Марк Коренберг.

- Issue #25924: Avoid unnecessary serialization of getaddrinfo(3) calls on
  OS X versions 10.5 or higher.  Original patch by A. Jesse Jiryu Davis.

- Issue #26406: Avoid unnecessary serialization of getaddrinfo(3) calls on
  current versions of OpenBSD and NetBSD.  Patch by A. Jesse Jiryu Davis.

- Issue #26848: Fix asyncio/subprocess.communicate() to handle empty input.
  Patch by Jack O'Connor.

- Issue #27040: Add loop.get_exception_handler method

- Issue #27041: asyncio: Add loop.create_future method

- Issue #27223: asyncio: Fix _read_ready and _write_ready to respect
  _conn_lost.
  Patch by Łukasz Langa.

- Issue #22970: asyncio: Fix inconsistency cancelling Condition.wait.
  Patch by David Coles.

