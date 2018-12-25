.. bpo: 27473
.. date: 9385
.. nonce: _nOtTA
.. release date: 2016-07-11
.. section: Core and Builtins

Fixed possible integer overflow in bytes and bytearray concatenations.
Patch by Xiang Zhang.

..

.. bpo: 23034
.. date: 9384
.. nonce: GWaUqn
.. section: Core and Builtins

The output of a special Python build with defined COUNT_ALLOCS,
SHOW_ALLOC_COUNT or SHOW_TRACK_COUNT macros is now off by  default.  It can
be re-enabled using the "-X showalloccount" option.  It now outputs to
stderr instead of stdout.

..

.. bpo: 27443
.. date: 9383
.. nonce: 87ZwZ1
.. section: Core and Builtins

__length_hint__() of bytearray iterators no longer return a negative integer
for a resized bytearray.

..

.. bpo: 27007
.. date: 9382
.. nonce: Gg8Um4
.. section: Core and Builtins

The fromhex() class methods of bytes and bytearray subclasses now return an
instance of corresponding subclass.

..

.. bpo: 26844
.. date: 9381
.. nonce: I0wdnY
.. section: Library

Fix error message for imp.find_module() to refer to 'path' instead of
'name'. Patch by Lev Maximov.

..

.. bpo: 23804
.. date: 9380
.. nonce: ipFvxc
.. section: Library

Fix SSL zero-length recv() calls to not block and not raise an error about
unclean EOF.

..

.. bpo: 27466
.. date: 9379
.. nonce: C_3a8E
.. section: Library

Change time format returned by http.cookie.time2netscape, confirming the
netscape cookie format and making it consistent with documentation.

..

.. bpo: 21708
.. date: 9378
.. nonce: RpPYiv
.. section: Library

Deprecated dbm.dumb behavior that differs from common dbm behavior: creating
a database in 'r' and 'w' modes and modifying a database in 'r' mode.

..

.. bpo: 26721
.. date: 9377
.. nonce: L37Y7r
.. section: Library

Change the socketserver.StreamRequestHandler.wfile attribute to implement
BufferedIOBase. In particular, the write() method no longer does partial
writes.

..

.. bpo: 22115
.. date: 9376
.. nonce: vG5UQW
.. section: Library

Added methods trace_add, trace_remove and trace_info in the tkinter.Variable
class.  They replace old methods trace_variable, trace, trace_vdelete and
trace_vinfo that use obsolete Tcl commands and might not work in future
versions of Tcl.  Fixed old tracing methods: trace_vdelete() with wrong mode
no longer break tracing, trace_vinfo() now always returns a list of pairs of
strings, tracing in the "u" mode now works.

..

.. bpo: 26243
.. date: 9375
.. nonce: dBtlhI
.. section: Library

Only the level argument to zlib.compress() is keyword argument now.  The
first argument is positional-only.

..

.. bpo: 27038
.. date: 9374
.. nonce: yGMV4h
.. section: Library

Expose the DirEntry type as os.DirEntry. Code patch by Jelle Zijlstra.

..

.. bpo: 27186
.. date: 9373
.. nonce: OtorpF
.. section: Library

Update os.fspath()/PyOS_FSPath() to check the return value of __fspath__()
to be either str or bytes.

..

.. bpo: 18726
.. date: 9372
.. nonce: eIXHIl
.. section: Library

All optional parameters of the dump(), dumps(), load() and loads() functions
and JSONEncoder and JSONDecoder class constructors in the json module are
now keyword-only.

..

.. bpo: 27319
.. date: 9371
.. nonce: vDl2zm
.. section: Library

Methods selection_set(), selection_add(), selection_remove() and
selection_toggle() of ttk.TreeView now allow passing multiple items as
multiple arguments instead of passing them as a tuple.  Deprecated
undocumented ability of calling the selection() method with arguments.

..

.. bpo: 27079
.. date: 9370
.. nonce: c7d0Ym
.. section: Library

Fixed curses.ascii functions isblank(), iscntrl() and ispunct().

..

.. bpo: 27294
.. date: 9369
.. nonce: 0WSp9y
.. section: Library

Numerical state in the repr for Tkinter event objects is now represented as
a combination of known flags.

..

.. bpo: 27177
.. date: 9368
.. nonce: U6jRnd
.. section: Library

Match objects in the re module now support index-like objects as group
indices.  Based on patches by Jeroen Demeyer and Xiang Zhang.

..

.. bpo: 26754
.. date: 9367
.. nonce: J3n0QW
.. section: Library

Some functions (compile() etc) accepted a filename argument encoded as an
iterable of integers. Now only strings and byte-like objects are accepted.

..

.. bpo: 26536
.. date: 9366
.. nonce: DgLWm-
.. section: Library

socket.ioctl now supports SIO_LOOPBACK_FAST_PATH. Patch by Daniel Stokes.

..

.. bpo: 27048
.. date: 9365
.. nonce: EVe-Bk
.. section: Library

Prevents distutils failing on Windows when environment variables contain
non-ASCII characters

..

.. bpo: 27330
.. date: 9364
.. nonce: GJaFCV
.. section: Library

Fixed possible leaks in the ctypes module.

..

.. bpo: 27238
.. date: 9363
.. nonce: Q6v6Qv
.. section: Library

Got rid of bare excepts in the turtle module.  Original patch by Jelle
Zijlstra.

..

.. bpo: 27122
.. date: 9362
.. nonce: 06t7zN
.. section: Library

When an exception is raised within the context being managed by a
contextlib.ExitStack() and one of the exit stack generators catches and
raises it in a chain, do not re-raise the original exception when exiting,
let the new chained one through.  This avoids the :pep:`479` bug described in
issue25782.

..

.. bpo: 27278
.. date: 9361
.. nonce: y_HkGw
.. original section: Library
.. section: Security

Fix os.urandom() implementation using getrandom() on Linux.  Truncate size
to INT_MAX and loop until we collected enough random bytes, instead of
casting a directly Py_ssize_t to int.

..

.. bpo: 16864
.. date: 9360
.. nonce: W7tJDa
.. section: Library

sqlite3.Cursor.lastrowid now supports REPLACE statement. Initial patch by
Alex LordThorsen.

..

.. bpo: 26386
.. date: 9359
.. nonce: 9L3Ut4
.. section: Library

Fixed ttk.TreeView selection operations with item id's containing spaces.

..

.. bpo: 8637
.. date: 9358
.. nonce: lHiUSA
.. section: Library

Honor a pager set by the env var MANPAGER (in preference to one set by the
env var PAGER).

..

.. bpo: 22636
.. date: 9357
.. nonce: 3fQW_g
.. original section: Library
.. section: Security

Avoid shell injection problems with ctypes.util.find_library().

..

.. bpo: 16182
.. date: 9356
.. nonce: RgFXyr
.. section: Library

Fix various functions in the "readline" module to use the locale encoding,
and fix get_begidx() and get_endidx() to return code point indexes.

..

.. bpo: 27392
.. date: 9355
.. nonce: obfni7
.. section: Library

Add loop.connect_accepted_socket(). Patch by Jim Fulton.

..

.. bpo: 27477
.. date: 9354
.. nonce: iEuL-9
.. section: IDLE

IDLE search dialogs now use ttk widgets.

..

.. bpo: 27173
.. date: 9353
.. nonce: M-fYaV
.. section: IDLE

Add 'IDLE Modern Unix' to the built-in key sets. Make the default key set
depend on the platform. Add tests for the changes to the config module.

..

.. bpo: 27452
.. date: 9352
.. nonce: dLxZ8W
.. section: IDLE

make command line "idle-test> python test_help.py" work. __file__ is
relative when python is started in the file's directory.

..

.. bpo: 27452
.. date: 9351
.. nonce: RtWnyR
.. section: IDLE

add line counter and crc to IDLE configHandler test dump.

..

.. bpo: 27380
.. date: 9350
.. nonce: Q39r9U
.. section: IDLE

IDLE: add query.py with base Query dialog and ttk widgets. Module had
subclasses SectionName, ModuleName, and HelpSource, which are used to get
information from users by configdialog and file =>Load Module. Each subclass
has itw own validity checks.  Using ModuleName allows users to edit bad
module names instead of starting over. Add tests and delete the two files
combined into the new one.

..

.. bpo: 27372
.. date: 9349
.. nonce: k3Wj2V
.. section: IDLE

Test_idle no longer changes the locale.

..

.. bpo: 27365
.. date: 9348
.. nonce: y7ys_A
.. section: IDLE

Allow non-ascii chars in IDLE NEWS.txt, for contributor names.

..

.. bpo: 27245
.. date: 9347
.. nonce: u9aKO1
.. section: IDLE

IDLE: Cleanly delete custom themes and key bindings. Previously, when IDLE
was started from a console or by import, a cascade of warnings was emitted.
Patch by Serhiy Storchaka.

..

.. bpo: 24137
.. date: 9346
.. nonce: v8o-IT
.. section: IDLE

Run IDLE, test_idle, and htest with tkinter default root disabled.  Fix code
and tests that fail with this restriction.  Fix htests to not create a
second and redundant root and mainloop.

..

.. bpo: 27310
.. date: 9345
.. nonce: KiURpC
.. section: IDLE

Fix IDLE.app failure to launch on OS X due to vestigial import.

..

.. bpo: 26754
.. date: 9344
.. nonce: Qm_N79
.. section: C API

PyUnicode_FSDecoder() accepted a filename argument encoded as an iterable of
integers. Now only strings and byte-like objects are accepted.

..

.. bpo: 28066
.. date: 9343
.. nonce: _3xImV
.. section: Build

Fix the logic that searches build directories for generated include files
when building outside the source tree.

..

.. bpo: 27442
.. date: 9342
.. nonce: S2M0cz
.. section: Build

Expose the Android API level that python was built against, in
sysconfig.get_config_vars() as 'ANDROID_API_LEVEL'.

..

.. bpo: 27434
.. date: 9341
.. nonce: 4nRZmn
.. section: Build

The interpreter that runs the cross-build, found in PATH, must now be of the
same feature version (e.g. 3.6) as the source being built.

..

.. bpo: 26930
.. date: 9340
.. nonce: 9JUeSD
.. section: Build

Update Windows builds to use OpenSSL 1.0.2h.

..

.. bpo: 23968
.. date: 9339
.. nonce: 7AuSK9
.. section: Build

Rename the platform directory from plat-$(MACHDEP) to
plat-$(PLATFORM_TRIPLET). Rename the config directory (LIBPL) from
config-$(LDVERSION) to config-$(LDVERSION)-$(PLATFORM_TRIPLET). Install the
platform specific _sysconfigdata module into the platform directory and
rename it to include the ABIFLAGS.

..

.. bpo: 0
.. date: 9338
.. nonce: U46i2u
.. section: Build

Don't use largefile support for GNU/Hurd.

..

.. bpo: 27332
.. date: 9337
.. nonce: OuRZp9
.. section: Tools/Demos

Fixed the type of the first argument of module-level functions generated by
Argument Clinic.  Patch by Petr Viktorin.

..

.. bpo: 27418
.. date: 9336
.. nonce: W2m_8I
.. section: Tools/Demos

Fixed Tools/importbench/importbench.py.

..

.. bpo: 19489
.. date: 9335
.. nonce: jvzuO7
.. section: Documentation

Moved the search box from the sidebar to the header and footer of each page.
Patch by Ammar Askar.

..

.. bpo: 27285
.. date: 9334
.. nonce: wZur0b
.. section: Documentation

Update documentation to reflect the deprecation of ``pyvenv`` and normalize
on the term "virtual environment". Patch by Steve Piercy.

..

.. bpo: 27027
.. date: 9333
.. nonce: 5oRSGL
.. section: Tests

Added test.support.is_android that is True when this is an Android build.
