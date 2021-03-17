.. bpo: 36216
.. date: 2019-03-06-09-38-40
.. nonce: 6q1m4a
.. release date: 2019-03-25
.. section: Security

Changes urlsplit() to raise ValueError when the URL contains characters that
decompose under IDNA encoding (NFKC-normalization) into characters that
affect how the URL is parsed.

..

.. bpo: 35121
.. date: 2018-10-31-15-39-17
.. nonce: EgHv9k
.. section: Security

Don't send cookies of domain A without Domain attribute to domain B when
domain A is a suffix match of domain B while using a cookiejar with
:class:`http.cookiejar.DefaultCookiePolicy` policy. Patch by Karthikeyan
Singaravelan.

..

.. bpo: 36421
.. date: 2019-03-24-21-33-22
.. nonce: gJ2Pv9
.. section: Core and Builtins

Fix a possible double decref in _ctypes.c's ``PyCArrayType_new()``.

..

.. bpo: 36412
.. date: 2019-03-23-19-51-09
.. nonce: C7acGn
.. section: Core and Builtins

Fix a possible crash when creating a new dictionary.

..

.. bpo: 36398
.. date: 2019-03-21-22-19-38
.. nonce: B_jXGe
.. section: Core and Builtins

Fix a possible crash in ``structseq_repr()``.

..

.. bpo: 36256
.. date: 2019-03-21-00-24-18
.. nonce: OZHa0t
.. section: Core and Builtins

Fix bug in parsermodule when parsing a state in a DFA that has two or more
arcs with labels of the same type. Patch by Pablo Galindo.

..

.. bpo: 36365
.. date: 2019-03-19-15-58-23
.. nonce: jHaErz
.. section: Core and Builtins

repr(structseq) is no longer limited to 512 bytes.

..

.. bpo: 36374
.. date: 2019-03-19-15-46-42
.. nonce: EWKMZE
.. section: Core and Builtins

Fix a possible null pointer dereference in ``merge_consts_recursive()``.
Patch by Zackery Spytz.

..

.. bpo: 36236
.. date: 2019-03-19-03-08-26
.. nonce: 5qN9qK
.. section: Core and Builtins

At Python initialization, the current directory is no longer prepended to
:data:`sys.path` if it has been removed.

..

.. bpo: 36352
.. date: 2019-03-19-02-36-40
.. nonce: qj2trz
.. section: Core and Builtins

Python initialization now fails with an error, rather than silently
truncating paths, if a path is too long.

..

.. bpo: 36301
.. date: 2019-03-19-00-54-31
.. nonce: xvOCJb
.. section: Core and Builtins

Python initialization now fails if decoding ``pybuilddir.txt`` configuration
file fails at startup.

..

.. bpo: 36333
.. date: 2019-03-18-10-56-53
.. nonce: 4dqemZ
.. section: Core and Builtins

Fix leak in _PyRuntimeState_Fini. Contributed by Stéphane Wirtel.

..

.. bpo: 36332
.. date: 2019-03-18-09-27-54
.. nonce: yEC-Vz
.. section: Core and Builtins

The builtin :func:`compile` can now handle AST objects that contain
assignment expressions. Patch by Pablo Galindo.

..

.. bpo: 36282
.. date: 2019-03-13-22-47-28
.. nonce: zs7RKP
.. section: Core and Builtins

Improved error message for too much positional arguments in some builtin
functions.

..

.. bpo: 30040
.. date: 2019-03-11-22-30-56
.. nonce: W9z8X7
.. section: Core and Builtins

New empty dict uses fewer memory for now.  It used more memory than empty
dict created by ``dict.clear()``.  And empty dict creation and deletion is
about 2x faster.  Patch by Inada Naoki.

..

.. bpo: 36262
.. date: 2019-03-11-15-37-33
.. nonce: v3N6Fz
.. section: Core and Builtins

Fix an unlikely memory leak on conversion from string to float in the
function ``_Py_dg_strtod()`` used by ``float(str)``, ``complex(str)``,
:func:`pickle.load`, :func:`marshal.load`, etc.

..

.. bpo: 36252
.. date: 2019-03-09-15-47-05
.. nonce: sCQFKq
.. section: Core and Builtins

Update Unicode databases to version 12.0.0.

..

.. bpo: 36218
.. date: 2019-03-07-13-05-43
.. nonce: dZemNt
.. section: Core and Builtins

Fix a segfault occurring when sorting a list of heterogeneous values. Patch
contributed by Rémi Lapeyre and Elliot Gorokhovsky.

..

.. bpo: 36188
.. date: 2019-03-04-18-05-31
.. nonce: EuUZNz
.. section: Core and Builtins

Cleaned up left-over vestiges of Python 2 unbound method handling in method
objects and documentation. Patch by Martijn Pieters

..

.. bpo: 36124
.. date: 2019-03-01-13-48-01
.. nonce: Blzxq1
.. section: Core and Builtins

Add a new interpreter-specific dict and expose it in the C-API via
PyInterpreterState_GetDict().  This parallels PyThreadState_GetDict().
However, extension modules should continue using PyModule_GetState() for
their own internal per-interpreter state.

..

.. bpo: 35975
.. date: 2019-02-27-16-49-08
.. nonce: IescLY
.. section: Core and Builtins

Add a ``feature_version`` flag to ``ast.parse()`` (documented) and
``compile()`` (hidden) that allows tweaking the parser to support older
versions of the grammar. In particular, if ``feature_version`` is 5 or 6,
the hacks for the ``async`` and ``await`` keyword from PEP 492 are
reinstated. (For 7 or higher, these are unconditionally treated as keywords,
but they are still special tokens rather than ``NAME`` tokens that the
parser driver recognizes.)

..

.. bpo: 31904
.. date: 2019-02-26-17-34-49
.. nonce: R4KSj6
.. section: Core and Builtins

Use UTF-8 as the system encoding on VxWorks.

..

.. bpo: 36048
.. date: 2019-02-20-08-51-04
.. nonce: I3LJt9
.. section: Core and Builtins

The :meth:`~object.__index__` special method will be used instead of
:meth:`~object.__int__` for implicit conversion of Python numbers to C
integers.  Using the ``__int__()`` method in implicit conversions has been
deprecated.

..

.. bpo: 35808
.. date: 2019-02-11-00-50-03
.. nonce: M12CMH
.. section: Core and Builtins

Retire pgen and use a modified version of pgen2 to generate the parser.
Patch by Pablo Galindo.

..

.. bpo: 36401
.. date: 2019-03-23-10-25-07
.. nonce: hYpVBS
.. section: Library

The class documentation created by pydoc now has a separate section for
readonly properties.

..

.. bpo: 36320
.. date: 2019-03-18-01-08-14
.. nonce: -06b9_
.. section: Library

The typing.NamedTuple() class has deprecated the _field_types attribute in
favor of the __annotations__ attribute which carried the same information.
Also, both attributes were converted from OrderedDict to a regular dict.

..

.. bpo: 34745
.. date: 2019-03-17-16-43-29
.. nonce: nOfm7_
.. section: Library

Fix :mod:`asyncio` ssl memory issues caused by circular references

..

.. bpo: 36324
.. date: 2019-03-17-01-17-45
.. nonce: dvNrRe
.. section: Library

Add method to statistics.NormalDist for computing the inverse cumulative
normal distribution.

..

.. bpo: 36321
.. date: 2019-03-16-13-40-59
.. nonce: s6crQx
.. section: Library

collections.namedtuple() misspelled the name of an attribute.  To be
consistent with typing.NamedTuple, the attribute name should have been
"_field_defaults" instead of "_fields_defaults".  For backwards
compatibility, both spellings are now created.  The misspelled version may
be removed in the future.

..

.. bpo: 36297
.. date: 2019-03-15-21-41-22
.. nonce: Gz9ZfU
.. section: Library

"unicode_internal" codec is removed.  It was deprecated since Python 3.3.
Patch by Inada Naoki.

..

.. bpo: 36298
.. date: 2019-03-15-13-54-07
.. nonce: amEVK2
.. section: Library

Raise ModuleNotFoundError in pyclbr when a module can't be found. Thanks to
'mental' for the bug report.

..

.. bpo: 36268
.. date: 2019-03-14-16-25-17
.. nonce: MDXLw6
.. section: Library

Switch the default format used for writing tars with mod:`tarfile` to the
modern POSIX.1-2001 pax standard, from the vendor-specific GNU. Contributed
by C.A.M. Gerlach.

..

.. bpo: 36285
.. date: 2019-03-14-01-09-59
.. nonce: G-usj8
.. section: Library

Fix integer overflows in the array module. Patch by Stephan Hohe.

..

.. bpo: 31904
.. date: 2019-03-13-14-55-02
.. nonce: 834kfY
.. section: Library

Add _signal module support for VxWorks.

..

.. bpo: 36272
.. date: 2019-03-13-14-14-36
.. nonce: f3l2IG
.. section: Library

:mod:`logging` does not silently ignore RecursionError anymore. Patch
contributed by Rémi Lapeyre.

..

.. bpo: 36280
.. date: 2019-03-12-21-02-55
.. nonce: mOd3iH
.. section: Library

Add a kind field to ast.Constant. It is 'u' if the literal has a 'u' prefix
(i.e. a Python 2 style unicode literal), else None.

..

.. bpo: 35931
.. date: 2019-03-11-22-06-36
.. nonce: Qp_Tbe
.. section: Library

The :mod:`pdb` ``debug`` command now gracefully handles all exceptions.

..

.. bpo: 36251
.. date: 2019-03-09-18-01-24
.. nonce: zOp9l0
.. section: Library

Fix format strings used for stderrprinter and re.Match reprs. Patch by
Stephan Hohe.

..

.. bpo: 36235
.. date: 2019-03-08-13-32-21
.. nonce: _M72wU
.. section: Library

Fix ``CFLAGS`` in ``customize_compiler()`` of ``distutils.sysconfig``: when
the ``CFLAGS`` environment variable is defined, don't override ``CFLAGS``
variable with the ``OPT`` variable anymore. Initial patch written by David
Malcolm.

..

.. bpo: 35807
.. date: 2019-03-06-13-21-33
.. nonce: W7mmu3
.. section: Library

Update ensurepip to install pip 19.0.3 and setuptools 40.8.0.

..

.. bpo: 36139
.. date: 2019-03-06-13-07-29
.. nonce: 6kedum
.. section: Library

Release GIL when closing :class:`~mmap.mmap` objects.

..

.. bpo: 36179
.. date: 2019-03-04-10-42-46
.. nonce: jEyuI-
.. section: Library

Fix two unlikely reference leaks in _hashopenssl. The leaks only occur in
out-of-memory cases.

..

.. bpo: 36169
.. date: 2019-03-03-11-37-09
.. nonce: 8nWJy7
.. section: Library

Add overlap() method to statistics.NormalDist.  Computes the overlapping
coefficient for two normal distributions.

..

.. bpo: 36103
.. date: 2019-03-01-16-10-01
.. nonce: n6VgXL
.. section: Library

Default buffer size used by ``shutil.copyfileobj()`` is changed from 16 KiB
to 64 KiB on non-Windows platform to reduce system call overhead.
Contributed by Inada Naoki.

..

.. bpo: 36130
.. date: 2019-02-26-22-41-38
.. nonce: _BnZOo
.. section: Library

Fix ``pdb`` with ``skip=...`` when stepping into a frame without a
``__name__`` global.  Patch by Anthony Sottile.

..

.. bpo: 35652
.. date: 2019-02-26-11-34-44
.. nonce: 6KRJu_
.. section: Library

shutil.copytree(copy_function=...) erroneously pass DirEntry instead of a
path string.

..

.. bpo: 35178
.. date: 2019-02-25-23-04-00
.. nonce: NA_rXa
.. section: Library

Ensure custom :func:`warnings.formatwarning` function can receive `line` as
positional argument. Based on patch by Tashrif Billah.

..

.. bpo: 36106
.. date: 2019-02-25-13-21-43
.. nonce: VuhEiQ
.. section: Library

Resolve potential name clash with libm's sinpi(). Patch by Dmitrii
Pasechnik.

..

.. bpo: 36091
.. date: 2019-02-23-06-49-06
.. nonce: 26o4Lc
.. section: Library

Clean up reference to async generator in Lib/types. Patch by Henry Chen.

..

.. bpo: 36043
.. date: 2019-02-19-19-53-46
.. nonce: l867v0
.. section: Library

:class:`FileCookieJar` supports :term:`path-like object`. Contributed by
Stéphane Wirtel

..

.. bpo: 35899
.. date: 2019-02-16-07-11-02
.. nonce: cjfn5a
.. section: Library

Enum has been fixed to correctly handle empty strings and strings with
non-Latin characters (ie. 'α', 'א') without crashing. Original patch
contributed by Maxwell. Assisted by Stéphane Wirtel.

..

.. bpo: 21269
.. date: 2019-02-10-16-49-16
.. nonce: Fqi7VH
.. section: Library

Add ``args`` and ``kwargs`` properties to mock call objects. Contributed by
Kumar Akshay.

..

.. bpo: 30670
.. date: 2019-02-06-12-07-46
.. nonce: yffB3F
.. section: Library

`pprint.pp` has been added to pretty-print objects with dictionary keys
being sorted with their insertion order by default. Parameter *sort_dicts*
has been added to `pprint.pprint`, `pprint.pformat` and
`pprint.PrettyPrinter`. Contributed by Rémi Lapeyre.

..

.. bpo: 35843
.. date: 2019-01-28-10-19-40
.. nonce: 7rXGQE
.. section: Library

Implement ``__getitem__`` for ``_NamespacePath``.  Patch by Anthony Sottile.

..

.. bpo: 35802
.. date: 2019-01-21-13-56-55
.. nonce: 6633PE
.. section: Library

Clean up code which checked presence of ``os.stat`` / ``os.lstat`` /
``os.chmod`` which are always present.  Patch by Anthony Sottile.

..

.. bpo: 35715
.. date: 2019-01-11-08-47-58
.. nonce: Wi3gl0
.. section: Library

Librates the return value of a ProcessPoolExecutor _process_worker after
it's no longer needed to free memory

..

.. bpo: 35493
.. date: 2019-01-09-23-43-08
.. nonce: kEcRGE
.. section: Library

Use :func:`multiprocessing.connection.wait` instead of polling each 0.2
seconds for worker updates in :class:`multiprocessing.Pool`. Patch by Pablo
Galindo.

..

.. bpo: 35661
.. date: 2019-01-05-16-16-20
.. nonce: H_UOXc
.. section: Library

Store the venv prompt in pyvenv.cfg.

..

.. bpo: 35121
.. date: 2018-12-30-14-35-19
.. nonce: oWmiGU
.. section: Library

Don't set cookie for a request when the request path is a prefix match of
the cookie's path attribute but doesn't end with "/". Patch by Karthikeyan
Singaravelan.

..

.. bpo: 21478
.. date: 2018-12-21-09-54-30
.. nonce: 5gsXtc
.. section: Library

Calls to a child function created with :func:`unittest.mock.create_autospec`
should propagate to the parent. Patch by Karthikeyan Singaravelan.

..

.. bpo: 35198
.. date: 2018-11-09-12-45-28
.. nonce: EJ8keW
.. section: Library

Fix C++ extension compilation on AIX

..

.. bpo: 36329
.. date: 2019-03-17-20-01-41
.. nonce: L5dJPD
.. section: Documentation

Declare the path of the Python binary for the usage of
``Tools/scripts/serve.py`` when executing ``make -C Doc/ serve``.
Contributed by Stéphane Wirtel

..

.. bpo: 36138
.. date: 2019-03-02-00-40-57
.. nonce: yfjNzG
.. section: Documentation

Improve documentation about converting datetime.timedelta to scalars.

..

.. bpo: 21314
.. date: 2018-11-21-23-01-37
.. nonce: PG33VT
.. section: Documentation

A new entry was added to the Core Language Section of the Programming FAQ,
which explaines the usage of slash(/) in the signature of a function. Patch
by Lysandros Nikolaou

..

.. bpo: 36234
.. date: 2019-03-08-12-53-37
.. nonce: NRVK6W
.. section: Tests

test_posix.PosixUidGidTests: add tests for invalid uid/gid type (str).
Initial patch written by David Malcolm.

..

.. bpo: 29571
.. date: 2019-02-28-18-33-29
.. nonce: r6b9fr
.. section: Tests

Fix ``test_re.test_locale_flag()``:  use ``locale.getpreferredencoding()``
rather than ``locale.getlocale()`` to get the locale encoding. With some
locales, ``locale.getlocale()`` returns the wrong encoding.

..

.. bpo: 36123
.. date: 2019-02-26-12-51-35
.. nonce: QRhhRS
.. section: Tests

Fix race condition in test_socket.

..

.. bpo: 36356
.. date: 2019-03-18-23-49-15
.. nonce: WNrwYI
.. section: Build

Fix leaks that led to build failure when configured with address sanitizer.

..

.. bpo: 36146
.. date: 2019-03-01-17-49-22
.. nonce: VeoyG7
.. section: Build

Add ``TEST_EXTENSIONS`` constant to ``setup.py`` to allow to not build test
extensions like ``_testcapi``.

..

.. bpo: 36146
.. date: 2019-02-28-18-09-01
.. nonce: IwPJVT
.. section: Build

Fix setup.py on macOS: only add ``/usr/include/ffi`` to include directories
of _ctypes, not for all extensions.

..

.. bpo: 31904
.. date: 2019-02-21-14-48-31
.. nonce: J82jY2
.. section: Build

Enable build system to cross-build for VxWorks RTOS.

..

.. bpo: 36312
.. date: 2019-03-16-16-51-17
.. nonce: Niwm-T
.. section: Windows

Fixed decoders for the following code pages: 50220, 50221, 50222, 50225,
50227, 50229, 57002 through 57011, 65000 and 42.

..

.. bpo: 36264
.. date: 2019-03-11-09-33-47
.. nonce: rTzWce
.. section: Windows

Don't honor POSIX ``HOME`` in ``os.path.expanduser`` on windows.  Patch by
Anthony Sottile.

..

.. bpo: 24643
.. date: 2019-02-24-07-52-39
.. nonce: PofyiS
.. section: Windows

Fix name collisions due to ``#define timezone _timezone`` in PC/pyconfig.h.

..

.. bpo: 36405
.. date: 2019-03-23-01-45-56
.. nonce: m7Wv1F
.. section: IDLE

Use dict unpacking in idlelib.

..

.. bpo: 36396
.. date: 2019-03-21-22-43-21
.. nonce: xSTX-I
.. section: IDLE

Remove fgBg param of idlelib.config.GetHighlight(). This param was only used
twice and changed the return type.

..

.. bpo: 36176
.. date: 2019-03-10-00-07-46
.. nonce: jk_vv6
.. section: IDLE

Fix IDLE autocomplete & calltip popup colors. Prevent conflicts with Linux
dark themes (and slightly darken calltip background).

..

.. bpo: 23205
.. date: 2019-03-06-14-47-57
.. nonce: Vv0gfH
.. section: IDLE

For the grep module, add tests for findfiles, refactor findfiles to be a
module-level function, and refactor findfiles to use os.walk.

..

.. bpo: 23216
.. date: 2019-03-02-19-39-53
.. nonce: ZA7H8H
.. section: IDLE

Add docstrings to IDLE search modules.

..

.. bpo: 36152
.. date: 2019-02-28-18-52-40
.. nonce: 9pkHIU
.. section: IDLE

Remove colorizer.ColorDelegator.close_when_done and the corresponding
argument of .close().  In IDLE, both have always been None or False since
2007.

..

.. bpo: 32129
.. date: 2019-02-25-11-40-14
.. nonce: 4qVCzD
.. section: IDLE

Avoid blurry IDLE application icon on macOS with Tk 8.6. Patch by Kevin
Walzer.

..

.. bpo: 36096
.. date: 2019-02-23-17-53-53
.. nonce: mN5Ly3
.. section: IDLE

Refactor class variables to instance variables in colorizer.

..

.. bpo: 30348
.. date: 2018-06-27-21-18-41
.. nonce: WbaRJW
.. section: IDLE

Increase test coverage of idlelib.autocomplete by 30%. Patch by Louie
Lu

..

.. bpo: 35132
.. date: 2019-03-04-02-09-09
.. nonce: 1R_pnL
.. section: Tools/Demos

Fix py-list and py-bt commands of python-gdb.py on gdb7.

..

.. bpo: 32217
.. date: 2017-12-19-20-42-36
.. nonce: axXcjA
.. section: Tools/Demos

Fix freeze script on Windows.

..

.. bpo: 36381
.. date: 2019-03-20-22-02-40
.. nonce: xlzDJ2
.. section: C API

Raise ``DeprecationWarning`` when '#' formats are used for building or
parsing values without ``PY_SSIZE_T_CLEAN``.

..

.. bpo: 36142
.. date: 2019-03-01-03-23-48
.. nonce: 7F6wJd
.. section: C API

The whole coreconfig.h header is now excluded from Py_LIMITED_API. Move
functions definitions into a new internal pycore_coreconfig.h header.
