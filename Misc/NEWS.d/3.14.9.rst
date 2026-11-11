.. date: 2026-10-20-13-15-00
.. gh-issue: 159894
.. nonce: 8Kp4mN
.. release date: 2026-10-21
.. section: Security

Bump the version of :pypi:`pip` bundled in :mod:`ensurepip` to version
26.2.2.

..

.. date: 2026-10-15-03-47-08
.. gh-issue: 159428
.. nonce: XHRKwY
.. section: Security

:mod:`mimetypes` no longer reads the files listed in
:data:`mimetypes.knownfiles` by default on Windows, and uses only the
registry.

..

.. date: 2026-09-10-13-38-11
.. gh-issue: 157265
.. nonce: -vYuMp
.. section: Security

In :mod:`tarfile`, when extracting a link falls back to extracting a member
of the archive, skip the member when the filter function returns None when
called with the extracted member's name replaced with the link's.

..

.. date: 2026-08-31-16-47-33
.. gh-issue: 157953
.. nonce: KxCFSN
.. section: Security

Update bundled `libexpat <https://libexpat.github.io/>`_ to version 2.9.0.

..

.. date: 2026-08-14-11-55-00
.. gh-issue: 157579
.. nonce: Kq3Vt2
.. section: Security

Fix a race condition in the cleanup of :class:`tempfile.TemporaryDirectory`.
When working around file system permission errors, files are now removed
relative to open directory file descriptors instead of resolving their path
again, so that replacing a directory of the tree with a symbolic link can no
longer make the cleanup delete files outside of the temporary directory.
This addresses :cve:`2026-12345`.

..

.. date: 2026-10-19-20-38-17
.. gh-issue: 159739
.. nonce: fc0ed2
.. section: Core and Builtins

Avoid a crash if allocation fails while creating a
:class:`string.templatelib.Template` iterator.

..

.. date: 2026-10-19-20-38-17
.. gh-issue: 159739
.. nonce: b62fc9
.. section: Core and Builtins

Propagate errors during generic class construction.

..

.. date: 2026-10-19-20-38-17
.. gh-issue: 159739
.. nonce: 78e530
.. section: Core and Builtins

Raise :exc:`OverflowError` when the absolute value of a dynamic printf-style
formatting width is too large.

..

.. date: 2026-10-19-20-38-17
.. gh-issue: 159739
.. nonce: 55fd08
.. section: Core and Builtins

Fix a reference leak in :func:`gc.get_objects` and :func:`gc.get_referrers`
when an internal allocation fails in the free-threaded build.

..

.. date: 2026-10-19-19-33-35
.. gh-issue: 159739
.. nonce: c25c27
.. section: Core and Builtins

Fix memory leaks during interpreter startup when a path configuration file
is empty or too large, or when an audit hook rejects a command.

..

.. date: 2026-10-19-19-33-35
.. gh-issue: 159739
.. nonce: a8775b
.. section: Core and Builtins

Avoid a crash when an interpreter configuration string cannot be encoded as
UTF-8.

..

.. date: 2026-10-19-19-33-35
.. gh-issue: 159739
.. nonce: 1fc3af
.. section: Core and Builtins

Preserve the original :exc:`SyntaxError` if allocating optional error
metadata fails.

..

.. date: 2026-10-19-19-33-35
.. gh-issue: 159739
.. nonce: 173d49
.. section: Core and Builtins

Avoid a crash if formatting an integer with the ``c`` presentation type runs
out of memory.

..

.. date: 2026-10-19-19-33-35
.. gh-issue: 159739
.. nonce: 09590b
.. section: Core and Builtins

Avoid a crash when a module filename cannot be encoded as UTF-8.

..

.. date: 2026-10-14-11-40-06
.. gh-issue: 159430
.. nonce: OWpyPm
.. section: Core and Builtins

Report the correct exception if looking up :meth:`~object.__bytes__` raises
an exception while performing %-formatting of :class:`bytes`.

..

.. date: 2026-10-13-16-38-00
.. gh-issue: 159320
.. nonce: _Yhg1s
.. section: Core and Builtins

Add dynamic detection of stack sizes on Android, allowing safer deep
recursion.

..

.. date: 2026-10-09-16-04-40
.. gh-issue: 159041
.. nonce: PUP8bU
.. section: Core and Builtins

Fix a use-after-free in :mod:`asyncio` task bookkeeping when a finalizer
creates a task during thread shutdown.

..

.. date: 2026-10-07-00-00-00
.. gh-issue: 158973
.. nonce: warnings-fork
.. section: Core and Builtins

Reset the warnings lock in the child after :func:`os.fork`, before clearing
other threads' states, to avoid deadlocks when their finalizers issue
warnings. Preserve the lock's recursion depth if the forking thread owns it.

..

.. date: 2026-10-06-14-12-49
.. gh-issue: 158918
.. nonce: TDVkZx
.. section: Core and Builtins

Reduce C stack usage of ``_PyModule_IsPossiblyShadowing``, which could
potentially cause stack overflows during module attribute access while
deeply recursing in agressively optimized builds.

..

.. date: 2026-10-06-05-12-21
.. gh-issue: 158803
.. nonce: iYTpRQ
.. section: Core and Builtins

Fix a crash in :meth:`bytes.join` and :meth:`bytearray.join` in the
:term:`free-threaded build` when another thread concurrently mutates the
list being joined. Patch by Christian Aurich Zanettini Martins.

..

.. date: 2026-10-04-11-59-33
.. gh-issue: 158592
.. nonce: _BpoeY
.. section: Core and Builtins

Skip the array reallocation when shrinking a small :class:`list`. Patch by
Donghee Na.

..

.. date: 2026-10-03-16-14-26
.. gh-issue: 158583
.. nonce: 7EUMvS
.. section: Core and Builtins

:meth:`bytes.fromhex` and :meth:`bytearray.fromhex`: Fix uninitialized
memory read. Patch by Victor Stinner.

..

.. date: 2026-10-02-16-19-35
.. gh-issue: 158592
.. nonce: V6W21J
.. section: Core and Builtins

``del list[index]`` now triggers shrinking the list's internal array.

..

.. date: 2026-09-29-14-35-00
.. gh-issue: 158121
.. nonce: Kj4nQ8
.. section: Core and Builtins

Fix missing :mod:`sys.monitoring` ``INSTRUCTION`` events when a ``LINE``
callback returns :data:`sys.monitoring.DISABLE`.

..

.. date: 2026-08-27-17-26-21
.. gh-issue: 106045
.. nonce: wzTH7f
.. section: Core and Builtins

Fix standard library discovery in virtual environments created from a Python
executable symlink located outside its base installation.

..

.. date: 2026-08-21-18-49-12
.. gh-issue: 156204
.. nonce: ZusA7e
.. section: Core and Builtins

Fix a crash in :c:func:`PyErr_GivenExceptionMatches` when evaluating deeply
nested exception tuples. The recursion is now bounded, and exceeding the
limit is reported as an unraisable exception.

..

.. date: 2026-07-14-20-19-48
.. gh-issue: 152075
.. nonce: V9Rwhp
.. section: Core and Builtins

Reduced lock contention during LOAD_GLOBAL bytecode specialization under
free threading.

..

.. date: 2026-05-22-17-10-29
.. gh-issue: 149816
.. nonce: u1-iqT
.. section: Core and Builtins

Fix a Use-After-Free race condition in ``subtype_getweakref()`` during
concurrent weakref creation and destruction in the free-threading build.

..

.. date: 2026-05-18-12-32-33
.. gh-issue: 149816
.. nonce: v18Ypf
.. section: Core and Builtins

Fix race conditions in ``invoke_gc_callback`` iterating ``gc.callbacks`` in
free-threading mode.

..

.. date: 2026-10-20-17-29-14
.. gh-issue: 138641
.. nonce: FN6qM0
.. section: Library

Ignore :func:`pdb.set_trace` when called directly at the :mod:`pdb` prompt.

..

.. date: 2026-10-19-23-25-29
.. gh-issue: 159811
.. nonce: cMB4DO
.. section: Library

Fix a crash in :mod:`hashlib` when the OpenSSL routine used by
:meth:`hash.update() <hashlib.hash.update>` or by hash object constructors
fails on data larger than 2048 bytes. Patch by Bénédikt Tran.

..

.. date: 2026-10-19-20-38-17
.. gh-issue: 159739
.. nonce: ff088c
.. section: Library

Fix copying :class:`collections.deque` subclasses with a large maximum
length.

..

.. date: 2026-10-19-20-38-17
.. gh-issue: 159739
.. nonce: bc65da
.. section: Library

Allow cyclic garbage collection to reclaim group records returned by
:mod:`grp`.

..

.. date: 2026-10-19-20-38-17
.. gh-issue: 159739
.. nonce: a81270
.. section: Library

Fix :class:`io.BytesIO` reads into overlapping views of the same buffer.

..

.. date: 2026-10-19-20-38-17
.. gh-issue: 159739
.. nonce: 5a6e94
.. section: Library

Avoid a crash when memory allocation fails while creating a
:class:`collections.OrderedDict` items iterator.

..

.. date: 2026-10-19-20-38-17
.. gh-issue: 159739
.. nonce: 243aa8
.. section: Library

Avoid reading past the end of an encoding name when the ``surrogatepass``
error handler receives a truncated UTF-16 or UTF-32 name.

..

.. date: 2026-10-19-20-38-17
.. gh-issue: 159739
.. nonce: 08caa7
.. section: Library

Fix incorrect :mod:`ctypes` scalar values after :func:`ctypes.resize`
enlarges the object.

..

.. date: 2026-10-19-19-33-35
.. gh-issue: 159739
.. nonce: b5fbca
.. section: Library

Fix a memory leak when Unicode normalization cannot grow its temporary
buffer.

..

.. date: 2026-10-19-19-33-35
.. gh-issue: 159739
.. nonce: a1c3c7
.. section: Library

Fix resource leaks in :mod:`ssl` when certificate conversion or key log
setup fails.

..

.. date: 2026-10-19-00-00-00
.. gh-issue: 149015
.. nonce: bdb
.. section: Library

Fix :meth:`bdb.Bdb.clear_all_file_breaks` leaving breakpoints behind when
multiple breakpoints are set on the same line.

..

.. date: 2026-10-17-15-22-16
.. gh-issue: 159620
.. nonce: Ynsu5L
.. section: Library

Fix ``python -m asyncio ps`` and ``pstree`` stacks stopping at asynchronous
generators. Patch by Timofei Ivankov.

..

.. date: 2026-10-16-10-25-22
.. gh-issue: 159458
.. nonce: q02EnY
.. section: Library

Fix a ThreadSanitizer data race on the static cache in
``normalize_century()`` of the datetime module under the free-threaded
build.

..

.. date: 2026-10-14-11-41-00
.. gh-issue: 159430
.. nonce: ZEW-mx
.. section: Library

Report the correct exception if looking up ``__name__`` raises an exception
when calling :func:`repr` on a :class:`weakref.ref`.

..

.. date: 2026-10-13-12-00-00
.. gh-issue: 159304
.. nonce: mFbYq2
.. section: Library

Fix :exc:`AttributeError` in :meth:`email.message.EmailMessage.get_body`
when a nested ``multipart`` part is malformed and its payload is a string.

..

.. date: 2026-10-12-21-20-00
.. gh-issue: 159225
.. nonce: T8jlor
.. section: Library

Prevent ``input`` calls in PyREPL from modifying readline history.

..

.. date: 2026-10-12-00-00-00
.. gh-issue: 159044
.. nonce: Tr8kXv
.. section: Library

Restore the previous behaviour of
:meth:`http.client.HTTPConnection.set_tunnel` where header values that are
not strings (such as :const:`None`, :class:`int` or other objects) are
converted with :class:`str` instead of raising :exc:`AttributeError` when
the request is sent.

..

.. date: 2026-10-11-08-41-00
.. gh-issue: 159184
.. nonce: Nn4tOr
.. section: Library

Fix :func:`marshal.dumps` raising :exc:`MemoryError` for a set or frozenset
containing several complex NaNs, which also made it impossible to write a
:file:`.pyc` file for source code containing such a constant.

..

.. date: 2026-10-11-08-40-00
.. gh-issue: 159185
.. nonce: Sq7cZd
.. section: Library

Fix a crash in :func:`marshal.dumps` and :func:`marshal.dump` when a set is
modified while its elements are serialized, for example by the
:meth:`~object.__buffer__` method of one of its elements.
:exc:`RuntimeError` is now raised.

..

.. date: 2026-10-10-22-29-13
.. gh-issue: 159049
.. nonce: eJUiKL
.. section: Library

Fix reference cycles in :meth:`asyncio.loop.create_connection` and
:meth:`asyncio.loop.start_tls` after a failed TLS handshake. Patch by
Timofei Ivankov.

..

.. date: 2026-10-08-16-18-43
.. gh-issue: 87720
.. nonce: Elqdl9
.. section: Library

Fixed error in :mod:`email` new policies where header folding failed to
detect the need to re-quote text obtained from encoded words in certain
cases.

..

.. date: 2026-10-07-20-32-20
.. gh-issue: 158929
.. nonce: 6dwqfQ
.. section: Library

Fix crash when calling :func:`syslog.syslog` while a subinterpreter in
another thread calls :func:`syslog.openlog` or :func:`syslog.closelog`.

..

.. date: 2026-10-06-20-03-01
.. gh-issue: 158893
.. nonce: B6A53q
.. section: Library

Make :func:`os.strerror` thread-safe: use the reentrant ``strerror_r()``
function if available. Patch by Victor Stinner.

..

.. date: 2026-10-05-18-00-01
.. gh-issue: 158539
.. nonce: k8Lv2a
.. section: Library

Fix macOS runtime section lookup for remote debugging when virtual memory
region sizes differ from Mach-O segment file sizes.

..

.. date: 2026-10-01-16-45-00
.. gh-issue: 156321
.. nonce: Kq3xVt
.. section: Library

Fix :func:`asyncio.shield` reporting an exception from the inner future
through the loop exception handler as soon as it completes after the shield
was cancelled, even when the exception is retrieved afterwards. The
exception is now reported only if it is never retrieved, as with any other
future.

..

.. date: 2026-10-01-13-59-02
.. gh-issue: 158563
.. nonce: kR7wQe
.. section: Library

Fix :func:`turtle.undo` after :func:`turtle.teleport` and after an exception
in :func:`turtle.circle`, :func:`turtle.dot` or :func:`turtle.write`. Fix
:func:`turtle.stamp`, :func:`turtle.clearstamp`, :func:`turtle.clear` and
:func:`turtle.reset` when the undo buffer is disabled.

..

.. date: 2026-09-23-19-59-03
.. gh-issue: 158004
.. nonce: p68swW
.. section: Library

Fix :func:`asyncio.as_completed` to continue yielding every awaitable after
a pending iteration step is cancelled.

..

.. date: 2026-09-12-18-03-15
.. gh-issue: 157364
.. nonce: Qd7mKs
.. section: Library

Fix a use-after-free in :class:`io.TextIOWrapper` when a call to the
underlying buffer reentrantly detaches it. Patched by Shamil Abdulaev.

..

.. date: 2026-09-12-03-14-57
.. gh-issue: 157242
.. nonce: ycbtjC
.. section: Library

:class:`io.BytesIO` is no longer closed on ``write()`` and ``truncate()``
failure (:exc:`MemoryError`). Patch by Victor Stinner.

..

.. date: 2026-09-07-23-40-00
.. gh-issue: 157146
.. nonce: lczip1
.. section: Library

:mod:`linecache` can now read the source of a module that was imported from
a zip archive on :data:`sys.path` when given only the file name, as
:mod:`pdb`, :mod:`warnings` and :mod:`doctest` do.

..

.. date: 2026-09-04-09-18-05
.. gh-issue: 156920
.. nonce: lONsKT
.. section: Library

Fix :mod:`asyncio` on Windows: closing a :class:`~asyncio.DatagramTransport`
under :class:`~asyncio.ProactorEventLoop` while datagrams were still queued,
or while an in-flight write failed right as ``close()`` was draining the
buffer, could strand the queued data and never call ``connection_lost()``,
hanging the close indefinitely.

..

.. date: 2026-08-31-00-00-00
.. gh-issue: 156698
.. nonce: gyDxUe
.. section: Library

Fix :class:`asyncio.ProactorEventLoop` UDP transports so that a write error
no longer strands a paused protocol: the write loop is now rescheduled when
data remains buffered, and the protocol is resumed when the buffer has
drained.

..

.. date: 2026-08-29-14-28-46
.. gh-issue: 156568
.. nonce: mkxLY8
.. section: Library

:func:`zipapp.create_archive` now correctly sets the executable bit on the
target archive when copying a zipapp where the target is a path-like object.

..

.. date: 2026-08-15-13-54-22
.. gh-issue: 155843
.. nonce: PD5Af3
.. section: Library

:mod:`hmac`: ensure that HMAC objects are properly initialized to prevent
rare crashes on allocation failures. Patch by Bénédikt Tran.

..

.. date: 2026-08-15-13-12-09
.. gh-issue: 155835
.. nonce: DsbSmy
.. section: Library

:mod:`hashlib`: Fix data races when accessing
:attr:`~hashlib.hash.digest_size` and :attr:`~hashlib.hash.block_size` on
SHA-3 objects. Patch by Bénédikt Tran.

..

.. date: 2026-08-15-12-08-33
.. gh-issue: 155833
.. nonce: Pi2Yjp
.. section: Library

Ensure that MD5 and SHA* objects are properly GC-untracked before freeing
their internal HACL* state. Patch by Bénédikt Tran.

..

.. date: 2026-08-15-11-49-30
.. gh-issue: 155830
.. nonce: cY5xgK
.. section: Library

:mod:`!_blake2`: Ensure that BLAKE-2 objects are correctly initialized
before being tracked by the garbage collector. Patch by Bénédikt Tran.

..

.. date: 2026-08-15-09-47-23
.. gh-issue: 155717
.. nonce: jWFLR2
.. section: Library

:mod:`multiprocessing`'s default start method is now :ref:`"spawn"
<multiprocessing-start-methods>` instead of ``"forkserver"`` when there is
no writable temporary directory, such as on a read-only file system.
Previously starting a process failed with an :exc:`OSError` there. Patch by
Bénédikt Tran.

..

.. date: 2026-08-14-09-28-21
.. gh-issue: 150449
.. nonce: Nf3Qz1
.. section: Library

Fix :class:`sqlite3.Blob` slices with a negative step. Reading or writing
such a slice raised :exc:`SystemError`. It now raises :exc:`ValueError`.

..

.. date: 2026-08-09-14-06-27
.. gh-issue: 155426
.. nonce: zZVPMd
.. section: Library

Make sure that the pure Python fallback of :func:`hmac.digest` rejects keys
that are neither :class:`bytes` or :class:`bytearray`. Previously, an
:exc:`AttributeError` was raised instead of :exc:`TypeError`. Patch by
Bénédikt Tran.

..

.. date: 2026-07-23-18-53-48
.. gh-issue: 154523
.. nonce: XUSRBO
.. section: Library

Fixed data-race when calling :meth:`io.TextIOBase.detach` in
:term:`free-threaded build`.

..

.. date: 2026-07-22-19-21-20
.. gh-issue: 82535
.. nonce: METFqd
.. section: Library

:class:`logging.handlers.SysLogHandler` no longer fails if the address
cannot be resolved when the handler is created. The address is resolved
again when a record is emitted.

..

.. date: 2026-07-19-22-08-05
.. gh-issue: 154194
.. nonce: 1bqRdx
.. section: Library

Fix the sampling profiler dropping entire samples when a non-fatal read
fails; frames now keep any readable metadata, and long funcnames and
filenames are truncated instead. Patch by Maurycy Pawłowski-Wieroński.

..

.. date: 2026-07-09-14-56-24
.. gh-issue: 152754
.. nonce: CyyC5j
.. section: Library

Fix a crash when the same :func:`os.scandir` iterator is used concurrently
from multiple threads.  It no longer releases the directory handle while
another thread is reading from it.  Sharing an iterator between threads
remains subject to race conditions: which entries each thread receives is
unspecified.

..

.. date: 2026-07-08-22-18-04
.. gh-issue: 153364
.. nonce: JBFHEg
.. section: Library

Make frame, coroutine and task-waiter walks iterative and bounded, avoiding
potential hangs and stack overflows. Fix asyncio task inspection when
awaited-by sets contain removed entries. Patch by Maurycy
Pawłowski-Wieroński.

..

.. date: 2026-06-30-15-00-00
.. gh-issue: 152692
.. nonce: tr3pr0
.. section: Library

Fix :func:`annotationlib.type_repr` returning a string such as
``None.list.append`` for bound built-in methods, whose ``__module__`` is
``None``.  It now returns the qualified name, as it already did for other
builtins.

..

.. date: 2026-06-28-14-37-01
.. gh-issue: 152490
.. nonce: A8qLbF
.. section: Library

Fix :exc:`TypeError` when formatting a :exc:`SyntaxError` raised with
:data:`NotImplemented` as its argument, which caused unexpected REPL exit
and IDLE Shell restart.

..

.. date: 2026-06-27-08-31-30
.. gh-issue: 152373
.. nonce: 3cd0aa
.. section: Library

Fix the :mod:`pdb` ``until`` command not stepping over a list comprehension
when a breakpoint is set on it. Patch by tonghuaroot.

..

.. date: 2026-06-20-20-24-03
.. gh-issue: 133998
.. nonce: nkqhMz
.. section: Library

Fix :func:`tarfile.open` in streaming write mode (``w|gz``) raising
:exc:`struct.error` when given an *mtime* outside the range of the gzip
header's 32-bit timestamp field.  Out-of-range values are now stored as
``0``, matching :mod:`gzip`.

..

.. date: 2026-06-09-19-43-24
.. gh-issue: 151179
.. nonce: hl_6Z0
.. section: Library

Fix a pidfd leak in ``_PidfdChildWatcher`` on Linux: the watcher no longer
leaks the process file descriptor when ``waitpid()`` fails with an error
other than :exc:`ChildProcessError`.

..

.. date: 2026-06-04-18-22-56
.. gh-issue: 143008
.. nonce: z5tw-J
.. section: Library

Fix race conditions when re-initializing a :class:`io.TextIOWrapper` object.

..

.. date: 2026-03-28-14-34-18
.. gh-issue: 143376
.. nonce: FvWTwi
.. section: Library

:mod:`concurrent.interpreters`: fix a crash when internal cross-interpreter
channel ends have incorrect channel IDs. Patch by Bénédikt Tran.

..

.. date: 2026-03-23-00-04-13
.. gh-issue: 146065
.. nonce: FIdn8D
.. section: Library

Fix a crash in asyncio.Future iterator when send() is called after throw()
or close().

..

.. date: 2025-12-21-17-56-37
.. gh-issue: 143008
.. nonce: aakErJ
.. section: Library

Fix crash in :class:`io.TextIOWrapper` when reentrant
:meth:`io.TextIOBase.detach` is called reentrantly from the underlying
buffer.

..

.. date: 2025-05-19-20-29-35
.. gh-issue: 133998
.. nonce: KmElUw
.. section: Library

Fix :exc:`struct.error` exception when creating a file with
:class:`gzip.GzipFile` or compressing data with :func:`gzip.compress` if the
system time is outside the range 00:00:00 UTC, January 1, 1970 through
06:28:15 UTC, February 7, 2106, or explicitly passed *mtime* argument is
outside the range ``0`` to ``2**32-1``.

..

.. date: 2025-02-04-09-01-06
.. gh-issue: 129605
.. nonce: wKGNXQ
.. section: Library

Improve exception handling when computing suggestions in traceback.py. This
fixes a bug that could cause PyREPL to exit unexpectedly.

..

.. date: 2026-09-24-01-20-36
.. gh-issue: 142083
.. nonce: WOlkvr
.. section: Documentation

Document the ``'w'`` type code in the :class:`array.array` docstring, so
that it lists the same type codes as :data:`array.typecodes` and the
:mod:`array` module documentation.

..

.. date: 2026-10-13-13-34-00
.. gh-issue: 159293
.. nonce: Lr8q2N
.. section: Tests

Restore the recursion limit after the free-threading recursion-limit race
test to avoid failures in subsequent tests run in the same process.

..

.. date: 2026-10-12-11-26-01
.. gh-issue: 159211
.. nonce: FWFj-p
.. section: Tests

The iOS testbed no longer gathers simulator diagnostics when a test run
fails.

..

.. date: 2026-09-16-17-06-54
.. gh-issue: 157628
.. nonce: KAwhlY
.. section: Tests

In regrtest, only consider that a test leaks if all test runs leak at least
one file descriptor. For example, ignore "[0, 1, -1] file descriptors,
sum=0" deltas, instead of reporting a leak. Patch by Victor Stinner.

..

.. date: 2026-08-18-10-16-44
.. gh-issue: 75876
.. nonce: PEZplM
.. section: Tests

A test decorated with :func:`~test.support.bigmemtest` now runs in a
subprocess if it really allocates the memory it asks for (that is, if the
``-M`` option is used), so that the memory it uses and the address space it
fragments are released when it ends.  A dummy run stays in the process.  The
separate memory watchdog process is no longer used.

..

.. date: 2026-10-18-12-00-00
.. gh-issue: 159759
.. nonce: LibStA
.. section: Build

Fix the ``libpython.static`` path in :pep:`739` ``build-details.json``,
which pointed to a file that is not installed when building with
:option:`--enable-shared`, and no longer list a static library for framework
builds, which do not install one.

..

.. date: 2026-10-15-11-20-48
.. gh-issue: 159440
.. nonce: uIz6ms
.. section: Build

The version string reported by ``sys.version`` no longer carries a spurious
``-dirty`` suffix when CPython is built out-of-tree from a clean git
checkout: the build stamp now checks the source tree instead of the build
directory.

..

.. date: 2026-10-03-23-23-12
.. gh-issue: 158683
.. nonce: eMlp60
.. section: Build

Fix the :file:`Modules/Setup` entry for the :mod:`!_zstd` module by adding
missing source files.

..

.. date: 2026-10-20-21-37-34
.. gh-issue: 159880
.. nonce: eKBiLr
.. section: Windows

Fix a key handle leak in :func:`winreg.CreateKey`,
:func:`winreg.CreateKeyEx` and :func:`winreg.OpenKey` when the
``winreg.OpenKey/result`` audit hook fails.

..

.. date: 2026-09-21-20-54-37
.. gh-issue: 157906
.. nonce: RlyLkY
.. section: Windows

Fix an incorrect architecture macro check that prevented CPU prefetching in
the free-threaded garbage collector on 32-bit Windows builds.

..

.. date: 2026-09-19-13-18-48
.. gh-issue: 157789
.. nonce: o6-rmo
.. section: Windows

Use compiler option ``-Og`` in case of clang-cl debug builds on Windows to
reduce stack usage. Patch by Chris Eibl.

..

.. date: 2026-10-19-19-33-35
.. gh-issue: 159739
.. nonce: aaf575
.. section: macOS

Propagate errors that occur while converting macOS system proxy settings.

..

.. date: 2026-10-16-15-12-36
.. gh-issue: 159517
.. nonce: uzRtAd
.. section: IDLE

Fix the completion list and the call tip grabbing the focus on macOS with
Tcl/Tk 9.1.

..

.. date: 2026-10-19-21-05-12
.. gh-issue: 159739
.. nonce: Rk3fTq
.. section: C API

Fix a crash in :c:func:`_PyTuple_Resize` when resizing a non-empty tuple to
a negative or too large size. It now raises :exc:`SystemError` or
:exc:`MemoryError` respectively.

..

.. date: 2026-10-19-20-38-17
.. gh-issue: 159739
.. nonce: eb9cdc
.. section: C API

:c:func:`PyInitConfig_SetStr` now releases a previously configured value and
preserves it if conversion of the replacement fails.

..

.. date: 2026-10-19-19-33-35
.. gh-issue: 159739
.. nonce: dee9a0
.. section: C API

Fix a reference leak when :c:func:`_PyTuple_Resize` fails.

..

.. date: 2026-10-05-01-09-32
.. gh-issue: 158810
.. nonce: F1p-PK
.. section: C API

Fix :c:macro:`Py_ARRAY_LENGTH()` macro on C++: do not use
``__builtin_types_compatible_p()`` on C++. Patch by Victor Stinner.

..

.. date: 2026-09-30-03-19-34
.. gh-issue: 158451
.. nonce: pUb2BJ
.. section: C API

Fix :c:func:`!_PyUnicodeWriter_WriteLatin1String` when the writer buffer
kind is UCS-2 or UCS-4. Patch by Victor Stinner.

..

.. date: 2026-09-16-21-24-36
.. gh-issue: 142217
.. nonce: 44sqt4
.. section: C API

Add tests on the undocumented and deprecated ``_Py_Identifier`` C API. Patch
by Victor Stinner.
