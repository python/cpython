- Issue #23840: tokenize.open() now closes the temporary binary file on error
  to fix a resource warning.

- Issue #16914: new debuglevel 2 in smtplib adds timestamps to debug output.

- Issue #7159: urllib.request now supports sending auth credentials
  automatically after the first 401.  This enhancement is a superset of the
  enhancement from issue #19494 and supersedes that change.

- Issue #23703: Fix a regression in urljoin() introduced in 901e4e52b20a.
  Patch by Demian Brecht.

- Issue #4254: Adds _curses.update_lines_cols().  Patch by Arnon Yaari

- Issue #19933: Provide default argument for ndigits in round. Patch by
  Vajrasky Kok.

- Issue #23193: Add a numeric_owner parameter to
  tarfile.TarFile.extract and tarfile.TarFile.extractall. Patch by
  Michael Vogt and Eric Smith.

- Issue #23342: Add a subprocess.run() function than returns a CalledProcess
  instance for a more consistent API than the existing call* functions.

- Issue #21217: inspect.getsourcelines() now tries to compute the start and end
  lines from the code object, fixing an issue when a lambda function is used as
  decorator argument. Patch by Thomas Ballinger and Allison Kaptur.

- Issue #24521: Fix possible integer overflows in the pickle module.

- Issue #22931: Allow '[' and ']' in cookie values.

- The keywords attribute of functools.partial is now always a dictionary.

- Issue #23811: Add missing newline to the PyCompileError error message.
  Patch by Alex Shkop.

- Issue #21116: Avoid blowing memory when allocating a multiprocessing shared
  array that's larger than 50% of the available RAM.  Patch by Médéric Boquien.

- Issue #22982: Improve BOM handling when seeking to multiple positions of
  a writable text file.

- Issue #23464: Removed deprecated asyncio JoinableQueue.

- Issue #23529: Limit the size of decompressed data when reading from
  GzipFile, BZ2File or LZMAFile.  This defeats denial of service attacks
  using compressed bombs (i.e. compressed payloads which decompress to a huge
  size).  Patch by Martin Panter and Nikolaus Rath.

- Issue #21859: Added Python implementation of io.FileIO.

- Issue #23865: close() methods in multiple modules now are idempotent and more
  robust at shutdown. If they need to release multiple resources, all are
  released even if errors occur.

- Issue #23400: Raise same exception on both Python 2 and 3 if sem_open is not
  available.  Patch by Davin Potts.

- Issue #10838: The subprocess now module includes SubprocessError and
  TimeoutError in its list of exported names for the users wild enough
  to use ``from subprocess import *``.

- Issue #23411: Added DefragResult, ParseResult, SplitResult, DefragResultBytes,
  ParseResultBytes, and SplitResultBytes to urllib.parse.__all__.
  Patch by Martin Panter.

- Issue #23881: urllib.request.ftpwrapper constructor now closes the socket if
  the FTP connection failed to fix a ResourceWarning.

- Issue #23853: :meth:`socket.socket.sendall` does no more reset the socket
  timeout each time data is sent successfully. The socket timeout is now the
  maximum total duration to send all data.

- Issue #22721: An order of multiline pprint output of set or dict containing
  orderable and non-orderable elements no longer depends on iteration order of
  set or dict.

- Issue #15133: _tkinter.tkapp.getboolean() now supports Tcl_Obj and always
  returns bool.  tkinter.BooleanVar now validates input values (accepted bool,
  int, str, and Tcl_Obj).  tkinter.BooleanVar.get() now always returns bool.

- Issue #10590: xml.sax.parseString() now supports string argument.

- Issue #23338: Fixed formatting ctypes error messages on Cygwin.
  Patch by Makoto Kato.

- Issue #15582: inspect.getdoc() now follows inheritance chains.

- Issue #2175: SAX parsers now support a character stream of InputSource object.

- Issue #16840: Tkinter now supports 64-bit integers added in Tcl 8.4 and
  arbitrary precision integers added in Tcl 8.5.

- Issue #23834: Fix socket.sendto(), use the C Py_ssize_t type to store the
  result of sendto() instead of the C int type.

- Issue #23618: :meth:`socket.socket.connect` now waits until the connection
  completes instead of raising :exc:`InterruptedError` if the connection is
  interrupted by signals, signal handlers don't raise an exception and the
  socket is blocking or has a timeout. :meth:`socket.socket.connect` still
  raise :exc:`InterruptedError` for non-blocking sockets.

- Issue #21526: Tkinter now supports new boolean type in Tcl 8.5.

- Issue #23836: Fix the faulthandler module to handle reentrant calls to
  its signal handlers.

- Issue #23838: linecache now clears the cache and returns an empty result on
  MemoryError.

- Issue #10395: Added os.path.commonpath(). Implemented in posixpath and ntpath.
  Based on patch by Rafik Draoui.

- Issue #23611: Serializing more "lookupable" objects (such as unbound methods
  or nested classes) now are supported with pickle protocols < 4.

- Issue #13583: sqlite3.Row now supports slice indexing.

- Issue #18473: Fixed 2to3 and 3to2 compatible pickle mappings.  Fixed
  ambigious reverse mappings.  Added many new mappings.  Import mapping is no
  longer applied to modules already mapped with full name mapping.

- Issue #23485: select.select() is now retried automatically with the
  recomputed timeout when interrupted by a signal, except if the signal handler
  raises an exception. This change is part of the PEP 475.

- Issue #23752: When built from an existing file descriptor, io.FileIO() now
  only calls fstat() once. Before fstat() was called twice, which was not
  necessary.

- Issue #23704: collections.deque() objects now support __add__, __mul__, and
  __imul__().

- Issue #23171: csv.Writer.writerow() now supports arbitrary iterables.

- Issue #23745: The new email header parser now handles duplicate MIME
  parameter names without error, similar to how get_param behaves.

- Issue #22117: Fix os.utime(), it now rounds the timestamp towards minus
  infinity (-inf) instead of rounding towards zero.

- Issue #23310: Fix MagicMock's initializer to work with __methods__, just
  like configure_mock().  Patch by Kasia Jachim.

