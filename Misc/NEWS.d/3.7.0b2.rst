.. bpo: 28414
.. date: 2017-08-06-14-43-45
.. nonce: mzZ6vD
.. release date: 2018-02-27
.. section: Security

The ssl module now allows users to perform their own IDN en/decoding when
using SNI.

..

.. bpo: 32889
.. date: 2018-02-20-21-53-48
.. nonce: J6eWy5
.. section: Core and Builtins

Update Valgrind suppression list to account for the rename of
``Py_ADDRESS_IN_RANG`` to ``address_in_range``.

..

.. bpo: 31356
.. date: 2018-02-02-08-50-46
.. nonce: MNwUOQ
.. section: Core and Builtins

Remove the new API added in bpo-31356 (gc.ensure_disabled() context
manager).

..

.. bpo: 32305
.. date: 2018-02-01-10-56-41
.. nonce: dkU9Qa
.. section: Core and Builtins

For namespace packages, ensure that both ``__file__`` and
``__spec__.origin`` are set to None.

..

.. bpo: 32303
.. date: 2018-02-01-10-16-28
.. nonce: VsvhSl
.. section: Core and Builtins

Make sure ``__spec__.loader`` matches ``__loader__`` for namespace packages.

..

.. bpo: 32711
.. date: 2018-01-29-14-36-37
.. nonce: 8hQFJP
.. section: Core and Builtins

Fix the warning messages for Python/ast_unparse.c. Patch by Stéphane Wirtel

..

.. bpo: 32583
.. date: 2018-01-26-21-20-21
.. nonce: Fh3fau
.. section: Core and Builtins

Fix possible crashing in builtin Unicode decoders caused by write
out-of-bound errors when using customized decode error handlers.

..

.. bpo: 32960
.. date: 2018-02-26-20-04-40
.. nonce: 48r0Ml
.. section: Library

For dataclasses, disallow inheriting frozen from non-frozen classes, and
also disallow inheriting non-frozen from frozen classes. This restriction
will be relaxed at a future date.

..

.. bpo: 32713
.. date: 2018-02-26-13-16-36
.. nonce: 55yegW
.. section: Library

Fixed tarfile.itn handling of out-of-bounds float values. Patch by Joffrey
Fuhrer.

..

.. bpo: 32951
.. date: 2018-02-25-18-22-01
.. nonce: gHrCXq
.. section: Library

Direct instantiation of SSLSocket and SSLObject objects is now prohibited.
The constructors were never documented, tested, or designed as public
constructors. Users were suppose to use ssl.wrap_socket() or SSLContext.

..

.. bpo: 32929
.. date: 2018-02-25-13-47-48
.. nonce: X2gTDH
.. section: Library

Remove the tri-state parameter "hash", and add the boolean "unsafe_hash". If
unsafe_hash is True, add a __hash__ function, but if a __hash__ exists,
raise TypeError.  If unsafe_hash is False, add a __hash__ based on the
values of eq= and frozen=.  The unsafe_hash=False behavior is the same as
the old hash=None behavior.  unsafe_hash=False is the default, just as
hash=None used to be.

..

.. bpo: 32947
.. date: 2018-02-25-13-06-21
.. nonce: mqStVW
.. section: Library

Add OP_ENABLE_MIDDLEBOX_COMPAT and test workaround for TLSv1.3 for future
compatibility with OpenSSL 1.1.1.

..

.. bpo: 30622
.. date: 2018-02-24-21-40-42
.. nonce: dQjxSe
.. section: Library

The ssl module now detects missing NPN support in LibreSSL.

..

.. bpo: 32922
.. date: 2018-02-23-19-12-04
.. nonce: u-xe0B
.. section: Library

dbm.open() now encodes filename with the filesystem encoding rather than
default encoding.

..

.. bpo: 32859
.. date: 2018-02-19-17-46-31
.. nonce: kAT-Xp
.. section: Library

In ``os.dup2``, don't check every call whether the ``dup3`` syscall exists
or not.

..

.. bpo: 32556
.. date: 2018-02-19-14-27-51
.. nonce: CsRsgr
.. section: Library

nt._getfinalpathname, nt._getvolumepathname and nt._getdiskusage now
correctly convert from bytes.

..

.. bpo: 25988
.. date: 2018-02-18-13-04-59
.. nonce: ACidKC
.. section: Library

Emit a :exc:`DeprecationWarning` when using or importing an ABC directly
from :mod:`collections` rather than from :mod:`collections.abc`.

..

.. bpo: 21060
.. date: 2018-02-17-19-20-19
.. nonce: S1Z-x6
.. section: Library

Rewrite confusing message from setup.py upload from "No dist file created in
earlier command" to the more helpful "Must create and upload files in one
command".

..

.. bpo: 32852
.. date: 2018-02-15-12-04-29
.. nonce: HDqIxM
.. section: Library

Make sure sys.argv remains as a list when running trace.

..

.. bpo: 31333
.. date: 2018-02-15-08-18-52
.. nonce: 4fF-gM
.. section: Library

``_abc`` module is added.  It is a speedup module with C implementations for
various functions and methods in ``abc``.  Creating an ABC subclass and
calling ``isinstance`` or ``issubclass`` with an ABC subclass are up to 1.5x
faster. In addition, this makes Python start-up up to 10% faster.
Note that the new implementation hides internal registry and caches,
previously accessible via private attributes ``_abc_registry``,
``_abc_cache``, and ``_abc_negative_cache``.  There are three debugging
helper methods that can be used instead ``_dump_registry``,
``_abc_registry_clear``, and ``_abc_caches_clear``.

..

.. bpo: 32841
.. date: 2018-02-14-00-21-24
.. nonce: bvHDOc
.. section: Library

Fixed `asyncio.Condition` issue which silently ignored cancellation after
notifying and cancelling a conditional lock. Patch by Bar Harel.

..

.. bpo: 32819
.. date: 2018-02-11-15-54-41
.. nonce: ZTRX2Q
.. section: Library

ssl.match_hostname() has been simplified and no longer depends on re and
ipaddress module for wildcard and IP addresses. Error reporting for invalid
wildcards has been improved.

..

.. bpo: 32394
.. date: 2018-02-10-13-51-56
.. nonce: dFM9SI
.. section: Library

socket: Remove TCP_FASTOPEN,TCP_KEEPCNT,TCP_KEEPIDLE,TCP_KEEPINTVL flags on
older version Windows during run-time.

..

.. bpo: 31787
.. date: 2018-02-09-21-41-56
.. nonce: owSZ2t
.. section: Library

Fixed refleaks of ``__init__()`` methods in various modules. (Contributed by
Oren Milman)

..

.. bpo: 30157
.. date: 2018-02-09-14-44-43
.. nonce: lEiiAK
.. section: Library

Fixed guessing quote and delimiter in csv.Sniffer.sniff() when only the last
field is quoted.  Patch by Jake Davis.

..

.. bpo: 32792
.. date: 2018-02-08-00-47-07
.. nonce: NtyDb4
.. section: Library

collections.ChainMap() preserves the order of the underlying mappings.

..

.. bpo: 32775
.. date: 2018-02-07-19-12-10
.. nonce: -T77_c
.. section: Library

:func:`fnmatch.translate()` no longer produces patterns which contain set
operations. Sets starting with '[' or containing '--', '&&', '~~' or '||'
will be interpreted differently in regular expressions in future versions.
Currently they emit warnings. fnmatch.translate() now avoids producing
patterns containing such sets by accident.

..

.. bpo: 32622
.. date: 2018-02-06-17-58-15
.. nonce: AE0Jz7
.. section: Library

Implement native fast sendfile for Windows proactor event loop.

..

.. bpo: 32777
.. date: 2018-02-05-21-28-28
.. nonce: C-wIXF
.. section: Library

Fix a rare but potential pre-exec child process deadlock in subprocess on
POSIX systems when marking file descriptors inheritable on exec in the child
process.  This bug appears to have been introduced in 3.4.

..

.. bpo: 32647
.. date: 2018-02-05-13-31-42
.. nonce: ktmfR_
.. section: Library

The ctypes module used to depend on indirect linking for dlopen. The shared
extension is now explicitly linked against libdl on platforms with dl.

..

.. bpo: 32741
.. date: 2018-02-01-17-54-08
.. nonce: KUvOPL
.. section: Library

Implement ``asyncio.TimerHandle.when()`` method.

..

.. bpo: 32691
.. date: 2018-02-01-15-53-35
.. nonce: VLWVTq
.. section: Library

Use mod_spec.parent when running modules with pdb

..

.. bpo: 32734
.. date: 2018-02-01-01-34-47
.. nonce: gCV9AD
.. section: Library

Fixed ``asyncio.Lock()`` safety issue which allowed acquiring and locking
the same lock multiple times, without it being free. Patch by Bar Harel.

..

.. bpo: 32727
.. date: 2018-01-30-17-46-18
.. nonce: aHVsRC
.. section: Library

Do not include name field in SMTP envelope from address. Patch by Stéphane
Wirtel

..

.. bpo: 31453
.. date: 2018-01-21-15-01-50
.. nonce: cZiZBe
.. section: Library

Add TLSVersion constants and SSLContext.maximum_version / minimum_version
attributes. The new API wraps OpenSSL 1.1
https://www.openssl.org/docs/man1.1.0/ssl/SSL_CTX_set_min_proto_version.html
feature.

..

.. bpo: 24334
.. date: 2018-01-20-23-17-25
.. nonce: GZuQLv
.. section: Library

Internal implementation details of ssl module were cleaned up. The SSLSocket
has one less layer of indirection. Owner and session information are now
handled by the SSLSocket and SSLObject constructor. Channel binding
implementation has been simplified.

..

.. bpo: 31848
.. date: 2018-01-18-23-34-17
.. nonce: M2cldy
.. section: Library

Fix the error handling in Aifc_read.initfp() when the SSND chunk is not
found. Patch by Zackery Spytz.

..

.. bpo: 32585
.. date: 2018-01-18-13-09-00
.. nonce: qpeijr
.. section: Library

Add Ttk spinbox widget to :mod:`tkinter.ttk`.  Patch by Alan D Moore.

..

.. bpo: 32221
.. date: 2017-12-06-10-10-10
.. nonce: ideco_
.. section: Library

Various functions returning tuple containing IPv6 addresses now omit
``%scope`` part since the same information is already encoded in *scopeid*
tuple item. Especially this speeds up :func:`socket.recvfrom` when it
receives multicast packet since useless resolving of network interface name
is omitted.

..

.. bpo: 30693
.. date: 2017-11-27-15-09-49
.. nonce: yC4mJ8
.. section: Library

The TarFile class now recurses directories in a reproducible way.

..

.. bpo: 30693
.. date: 2017-11-27-15-09-49
.. nonce: yC4mJ7
.. section: Library

The ZipFile class now recurses directories in a reproducible way.

..

.. bpo: 28124
.. date: 2018-02-25-16-33-35
.. nonce: _uzkgq
.. section: Documentation

The ssl module function ssl.wrap_socket() has been de-emphasized and
deprecated in favor of the more secure and efficient
SSLContext.wrap_socket() method.

..

.. bpo: 17232
.. date: 2018-02-23-12-48-03
.. nonce: tmuTKL
.. section: Documentation

Clarify docs for -O and -OO.  Patch by Terry Reedy.

..

.. bpo: 32436
.. date: 2018-02-14-11-10-41
.. nonce: TTJ2jb
.. section: Documentation

Add documentation for the contextvars module (PEP 567).

..

.. bpo: 32800
.. date: 2018-02-10-15-16-04
.. nonce: FyrqCk
.. section: Documentation

Update link to w3c doc for xml default namespaces.

..

.. bpo: 11015
.. date: 2018-02-10-12-48-38
.. nonce: -gUf34
.. section: Documentation

Update :mod:`test.support` documentation.

..

.. bpo: 8722
.. date: 2018-02-03-06-11-37
.. nonce: MPyVyj
.. section: Documentation

Document :meth:`__getattr__` behavior when property :meth:`get` method
raises :exc:`AttributeError`.

..

.. bpo: 32614
.. date: 2018-02-02-07-41-57
.. nonce: LSqzGw
.. section: Documentation

Modify RE examples in documentation to use raw strings to prevent
:exc:`DeprecationWarning` and add text to REGEX HOWTO to highlight the
deprecation.

..

.. bpo: 31972
.. date: 2018-01-25-14-23-12
.. nonce: w1m_8r
.. section: Documentation

Improve docstrings for `pathlib.PurePath` subclasses.

..

.. bpo: 31809
.. date: 2017-10-18-18-07-45
.. nonce: KlQrkE
.. section: Tests

Add tests to verify connection with secp ECDH curves.

..

.. bpo: 32898
.. date: 2018-02-21-12-46-00
.. nonce: M15bZh
.. section: Build

Fix the python debug build when using COUNT_ALLOCS.

..

.. bpo: 32901
.. date: 2018-02-23-00-47-13
.. nonce: mGKz5_
.. section: Windows

Update Tcl and Tk versions to 8.6.8

..

.. bpo: 31966
.. date: 2018-02-19-13-54-42
.. nonce: _Q3HPb
.. section: Windows

Fixed WindowsConsoleIO.write() for writing empty data.

..

.. bpo: 32409
.. date: 2018-02-19-10-00-57
.. nonce: nocuDg
.. section: Windows

Ensures activate.bat can handle Unicode contents.

..

.. bpo: 32457
.. date: 2018-02-19-08-54-06
.. nonce: vVP0Iz
.. section: Windows

Improves handling of denormalized executable path when launching Python.

..

.. bpo: 32370
.. date: 2018-02-10-15-38-19
.. nonce: kcKuct
.. section: Windows

Use the correct encoding for ipconfig output in the uuid module. Patch by
Segev Finer.

..

.. bpo: 29248
.. date: 2018-02-07-17-50-48
.. nonce: Xzwj-6
.. section: Windows

Fix :func:`os.readlink` on Windows, which was mistakenly treating the
``PrintNameOffset`` field of the reparse data buffer as a number of
characters instead of bytes. Patch by Craig Holmquist and SSE4.

..

.. bpo: 32901
.. date: 2018-02-27-17-33-15
.. nonce: hQu0w3
.. section: macOS

Update macOS 10.9+ installer to Tcl/Tk 8.6.8.

..

.. bpo: 32916
.. date: 2018-02-23-07-32-36
.. nonce: 4MsQ5F
.. section: IDLE

Change ``str`` to ``code`` in pyparse.

..

.. bpo: 32905
.. date: 2018-02-22-00-09-27
.. nonce: VlXj0x
.. section: IDLE

Remove unused code in pyparse module.

..

.. bpo: 32874
.. date: 2018-02-19-10-56-41
.. nonce: 6pZ9Gv
.. section: IDLE

Add tests for pyparse.

..

.. bpo: 32837
.. date: 2018-02-12-17-22-48
.. nonce: -33QPl
.. section: IDLE

Using the system and place-dependent default encoding for open() is a bad
idea for IDLE's system and location-independent files.

..

.. bpo: 32826
.. date: 2018-02-12-11-05-22
.. nonce: IxNZrk
.. section: IDLE

Add "encoding=utf-8" to open() in IDLE's test_help_about. GUI test
test_file_buttons() only looks at initial ascii-only lines, but failed on
systems where open() defaults to 'ascii' because readline() internally reads
and decodes far enough ahead to encounter a non-ascii character in
CREDITS.txt.

..

.. bpo: 32765
.. date: 2018-02-04-17-52-54
.. nonce: qm0eCu
.. section: IDLE

Update configdialog General tab docstring to add new widgets to the widget
list.

..

.. bpo: 32222
.. date: 2017-12-07-20-51-20
.. nonce: hPBcGT
.. section: Tools/Demos

Fix pygettext not extracting docstrings for functions with type annotated
arguments. Patch by Toby Harradine.
