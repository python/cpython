.. bpo: 28893
.. date: 9892
.. nonce: WTKnpj
.. release date: 2017-03-04
.. section: Core and Builtins

Set correct __cause__ for errors about invalid awaitables returned from
__aiter__ and __anext__.

..

.. bpo: 29683
.. date: 9891
.. nonce: G5iS-P
.. section: Core and Builtins

Fixes to memory allocation in _PyCode_SetExtra.  Patch by Brian Coleman.

..

.. bpo: 29684
.. date: 9890
.. nonce: wTgEoh
.. section: Core and Builtins

Fix minor regression of PyEval_CallObjectWithKeywords. It should raise
TypeError when kwargs is not a dict.  But it might cause segv when args=NULL
and kwargs is not a dict.

..

.. bpo: 28598
.. date: 9889
.. nonce: QxbzQn
.. section: Core and Builtins

Support __rmod__ for subclasses of str being called before str.__mod__.
Patch by Martijn Pieters.

..

.. bpo: 29607
.. date: 9888
.. nonce: 7NvBA1
.. section: Core and Builtins

Fix stack_effect computation for CALL_FUNCTION_EX. Patch by Matthieu
Dartiailh.

..

.. bpo: 29602
.. date: 9887
.. nonce: qyyskC
.. section: Core and Builtins

Fix incorrect handling of signed zeros in complex constructor for complex
subclasses and for inputs having a __complex__ method. Patch by Serhiy
Storchaka.

..

.. bpo: 29347
.. date: 9886
.. nonce: 1RPPGN
.. section: Core and Builtins

Fixed possibly dereferencing undefined pointers when creating weakref
objects.

..

.. bpo: 29438
.. date: 9885
.. nonce: IKxD6I
.. section: Core and Builtins

Fixed use-after-free problem in key sharing dict.

..

.. bpo: 29319
.. date: 9884
.. nonce: KLDUZf
.. section: Core and Builtins

Prevent RunMainFromImporter overwriting sys.path[0].

..

.. bpo: 29337
.. date: 9883
.. nonce: bjX8AE
.. section: Core and Builtins

Fixed possible BytesWarning when compare the code objects. Warnings could be
emitted at compile time.

..

.. bpo: 29327
.. date: 9882
.. nonce: XXQarW
.. section: Core and Builtins

Fixed a crash when pass the iterable keyword argument to sorted().

..

.. bpo: 29034
.. date: 9881
.. nonce: 7-uEDT
.. section: Core and Builtins

Fix memory leak and use-after-free in os module (path_converter).

..

.. bpo: 29159
.. date: 9880
.. nonce: gEn_kP
.. section: Core and Builtins

Fix regression in bytes(x) when x.__index__() raises Exception.

..

.. bpo: 28932
.. date: 9879
.. nonce: QnLx8A
.. section: Core and Builtins

Do not include <sys/random.h> if it does not exist.

..

.. bpo: 25677
.. date: 9878
.. nonce: RWhZrb
.. section: Core and Builtins

Correct the positioning of the syntax error caret for indented blocks.
Based on patch by Michael Layzell.

..

.. bpo: 29000
.. date: 9877
.. nonce: K6wQ-3
.. section: Core and Builtins

Fixed bytes formatting of octals with zero padding in alternate form.

..

.. bpo: 26919
.. date: 9876
.. nonce: Cm7MSa
.. section: Core and Builtins

On Android, operating system data is now always encoded/decoded to/from
UTF-8, instead of the locale encoding to avoid inconsistencies with
os.fsencode() and os.fsdecode() which are already using UTF-8.

..

.. bpo: 28991
.. date: 9875
.. nonce: lGA0FK
.. section: Core and Builtins

functools.lru_cache() was susceptible to an obscure reentrancy bug
triggerable by a monkey-patched len() function.

..

.. bpo: 28739
.. date: 9874
.. nonce: w1fvhk
.. section: Core and Builtins

f-string expressions are no longer accepted as docstrings and by
ast.literal_eval() even if they do not include expressions.

..

.. bpo: 28512
.. date: 9873
.. nonce: i-pv6d
.. section: Core and Builtins

Fixed setting the offset attribute of SyntaxError by
PyErr_SyntaxLocationEx() and PyErr_SyntaxLocationObject().

..

.. bpo: 28918
.. date: 9872
.. nonce: SFVuPz
.. section: Core and Builtins

Fix the cross compilation of xxlimited when Python has been built with
Py_DEBUG defined.

..

.. bpo: 28731
.. date: 9871
.. nonce: oNF59u
.. section: Core and Builtins

Optimize _PyDict_NewPresized() to create correct size dict. Improve speed of
dict literal with constant keys up to 30%.

..

.. bpo: 29169
.. date: 9870
.. nonce: 8ypApm
.. section: Library

Update zlib to 1.2.11.

..

.. bpo: 29623
.. date: 9869
.. nonce: D3-NP2
.. section: Library

Allow use of path-like object as a single argument in ConfigParser.read().
Patch by David Ellis.

..

.. bpo: 28963
.. date: 9868
.. nonce: tPl8dq
.. section: Library

Fix out of bound iteration in asyncio.Future.remove_done_callback
implemented in C.

..

.. bpo: 29704
.. date: 9867
.. nonce: r-kWqv
.. section: Library

asyncio.subprocess.SubprocessStreamProtocol no longer closes before all
pipes are closed.

..

.. bpo: 29271
.. date: 9866
.. nonce: y8Vj2v
.. section: Library

Fix Task.current_task and Task.all_tasks implemented in C to accept None
argument as their pure Python implementation.

..

.. bpo: 29703
.. date: 9865
.. nonce: ZdsPCR
.. section: Library

Fix asyncio to support instantiation of new event loops in child processes.

..

.. bpo: 29376
.. date: 9864
.. nonce: rrJhJy
.. section: Library

Fix assertion error in threading._DummyThread.is_alive().

..

.. bpo: 28624
.. date: 9863
.. nonce: 43TJib
.. section: Library

Add a test that checks that cwd parameter of Popen() accepts PathLike
objects.  Patch by Sayan Chowdhury.

..

.. bpo: 28518
.. date: 9862
.. nonce: o-Q2Nw
.. section: Library

Start a transaction implicitly before a DML statement. Patch by Aviv
Palivoda.

..

.. bpo: 29532
.. date: 9861
.. nonce: YCwVQn
.. section: Library

Altering a kwarg dictionary passed to functools.partial() no longer affects
a partial object after creation.

..

.. bpo: 29110
.. date: 9860
.. nonce: wmE-_T
.. section: Library

Fix file object leak in aifc.open() when file is given as a filesystem path
and is not in valid AIFF format. Patch by Anthony Zhang.

..

.. bpo: 28556
.. date: 9859
.. nonce: p6967e
.. section: Library

Various updates to typing module: typing.Counter, typing.ChainMap, improved
ABC caching, etc. Original PRs by Jelle Zijlstra, Ivan Levkivskyi, Manuel
Krebber, and Łukasz Langa.

..

.. bpo: 29100
.. date: 9858
.. nonce: LAAERS
.. section: Library

Fix datetime.fromtimestamp() regression introduced in Python 3.6.0: check
minimum and maximum years.

..

.. bpo: 29519
.. date: 9857
.. nonce: oGGgZ4
.. section: Library

Fix weakref spewing exceptions during interpreter shutdown when used with a
rare combination of multiprocessing and custom codecs.

..

.. bpo: 29416
.. date: 9856
.. nonce: KJGyI_
.. section: Library

Prevent infinite loop in pathlib.Path.mkdir

..

.. bpo: 29444
.. date: 9855
.. nonce: cEwgmk
.. section: Library

Fixed out-of-bounds buffer access in the group() method of the match object.
Based on patch by WGH.

..

.. bpo: 29335
.. date: 9854
.. nonce: _KC7IK
.. section: Library

Fix subprocess.Popen.wait() when the child process has exited to a stopped
instead of terminated state (ex: when under ptrace).

..

.. bpo: 29290
.. date: 9853
.. nonce: XBqptF
.. section: Library

Fix a regression in argparse that help messages would wrap at non-breaking
spaces.

..

.. bpo: 28735
.. date: 9852
.. nonce: admHLO
.. section: Library

Fixed the comparison of mock.MagickMock with mock.ANY.

..

.. bpo: 29316
.. date: 9851
.. nonce: OeOQw5
.. section: Library

Restore the provisional status of typing module, add corresponding note to
documentation. Patch by Ivan L.

..

.. bpo: 29219
.. date: 9850
.. nonce: kxui7t
.. section: Library

Fixed infinite recursion in the repr of uninitialized ctypes.CDLL instances.

..

.. bpo: 29011
.. date: 9849
.. nonce: MI5f2R
.. section: Library

Fix an important omission by adding Deque to the typing module.

..

.. bpo: 28969
.. date: 9848
.. nonce: j3HJYO
.. section: Library

Fixed race condition in C implementation of functools.lru_cache. KeyError
could be raised when cached function with full cache was simultaneously
called from different threads with the same uncached arguments.

..

.. bpo: 29142
.. date: 9847
.. nonce: xo6kAv
.. section: Library

In urllib.request, suffixes in no_proxy environment variable with leading
dots could match related hostnames again (e.g. .b.c matches a.b.c). Patch by
Milan Oberkirch.

..

.. bpo: 28961
.. date: 9846
.. nonce: Rt93vg
.. section: Library

Fix unittest.mock._Call helper: don't ignore the name parameter anymore.
Patch written by Jiajun Huang.

..

.. bpo: 29203
.. date: 9845
.. nonce: kN5S6v
.. section: Library

functools.lru_cache() now respects :pep:`468` and preserves the order of
keyword arguments.  f(a=1, b=2) is now cached separately from f(b=2, a=1)
since both calls could potentially give different results.

..

.. bpo: 15812
.. date: 9844
.. nonce: R1U-Ec
.. section: Library

inspect.getframeinfo() now correctly shows the first line of a context.
Patch by Sam Breese.

..

.. bpo: 29094
.. date: 9843
.. nonce: 460ZQo
.. section: Library

Offsets in a ZIP file created with extern file object and modes "w" and "x"
now are relative to the start of the file.

..

.. bpo: 29085
.. date: 9842
.. nonce: bm3gkx
.. section: Library

Allow random.Random.seed() to use high quality OS randomness rather than the
pid and time.

..

.. bpo: 29061
.. date: 9841
.. nonce: YKq0Ba
.. section: Library

Fixed bug in secrets.randbelow() which would hang when given a negative
input.  Patch by Brendan Donegan.

..

.. bpo: 29079
.. date: 9840
.. nonce: g4YLix
.. section: Library

Prevent infinite loop in pathlib.resolve() on Windows

..

.. bpo: 13051
.. date: 9839
.. nonce: YzC1Te
.. section: Library

Fixed recursion errors in large or resized curses.textpad.Textbox.  Based on
patch by Tycho Andersen.

..

.. bpo: 29119
.. date: 9838
.. nonce: Ov69fr
.. section: Library

Fix weakrefs in the pure python version of collections.OrderedDict
move_to_end() method. Contributed by Andra Bogildea.

..

.. bpo: 9770
.. date: 9837
.. nonce: WJJnwP
.. section: Library

curses.ascii predicates now work correctly with negative integers.

..

.. bpo: 28427
.. date: 9836
.. nonce: vUd-va
.. section: Library

old keys should not remove new values from WeakValueDictionary when
collecting from another thread.

..

.. bpo: 28923
.. date: 9835
.. nonce: naVULD
.. section: Library

Remove editor artifacts from Tix.py.

..

.. bpo: 29055
.. date: 9834
.. nonce: -r_9jc
.. section: Library

Neaten-up empty population error on random.choice() by suppressing the
upstream exception.

..

.. bpo: 28871
.. date: 9833
.. nonce: cPMXCJ
.. section: Library

Fixed a crash when deallocate deep ElementTree.

..

.. bpo: 19542
.. date: 9832
.. nonce: 5tCkaK
.. section: Library

Fix bugs in WeakValueDictionary.setdefault() and WeakValueDictionary.pop()
when a GC collection happens in another thread.

..

.. bpo: 20191
.. date: 9831
.. nonce: Q7uZCS
.. section: Library

Fixed a crash in resource.prlimit() when passing a sequence that doesn't own
its elements as limits.

..

.. bpo: 28779
.. date: 9830
.. nonce: t-mjED
.. section: Library

multiprocessing.set_forkserver_preload() would crash the forkserver process
if a preloaded module instantiated some multiprocessing objects such as
locks.

..

.. bpo: 28847
.. date: 9829
.. nonce: J7d3nG
.. section: Library

dbm.dumb now supports reading read-only files and no longer writes the index
file when it is not changed.

..

.. bpo: 26937
.. date: 9828
.. nonce: c9kgiA
.. section: Library

The chown() method of the tarfile.TarFile class does not fail now when the
grp module cannot be imported, as for example on Android platforms.

..

.. bpo: 29071
.. date: 9827
.. nonce: FCOpJn
.. section: IDLE

IDLE colors f-string prefixes (but not invalid ur prefixes).

..

.. bpo: 28572
.. date: 9826
.. nonce: 1_duKY
.. section: IDLE

Add 10% to coverage of IDLE's test_configdialog. Update and augment
description of the configuration system.

..

.. bpo: 29579
.. date: 9825
.. nonce: Ih-G2Q
.. section: Windows

Removes readme.txt from the installer

..

.. bpo: 29326
.. date: 9824
.. nonce: 4qDQzs
.. section: Windows

Ignores blank lines in ._pth files (Patch by Alexey Izbyshev)

..

.. bpo: 28164
.. date: 9823
.. nonce: h4CFX8
.. section: Windows

Correctly handle special console filenames (patch by Eryk Sun)

..

.. bpo: 29409
.. date: 9822
.. nonce: bhvrJ2
.. section: Windows

Implement :pep:`529` for io.FileIO (Patch by Eryk Sun)

..

.. bpo: 29392
.. date: 9821
.. nonce: OtqS5t
.. section: Windows

Prevent crash when passing invalid arguments into msvcrt module.

..

.. bpo: 25778
.. date: 9820
.. nonce: 8uKJ82
.. section: Windows

winreg does not truncate string correctly (Patch by Eryk Sun)

..

.. bpo: 28896
.. date: 9819
.. nonce: VMi9w0
.. section: Windows

Deprecate WindowsRegistryFinder and disable it by default.

..

.. bpo: 27867
.. date: 9818
.. nonce: UC5ohc
.. section: C API

Function PySlice_GetIndicesEx() is replaced with a macro if Py_LIMITED_API
is not set or set to the value between 0x03050400 and 0x03060000 (not
including) or 0x03060100 or higher.

..

.. bpo: 29083
.. date: 9817
.. nonce: tGTjr_
.. section: C API

Fixed the declaration of some public API functions. PyArg_VaParse() and
PyArg_VaParseTupleAndKeywords() were not available in limited API.
PyArg_ValidateKeywordArguments(), PyArg_UnpackTuple() and Py_BuildValue()
were not available in limited API of version < 3.3 when PY_SSIZE_T_CLEAN is
defined.

..

.. bpo: 29058
.. date: 9816
.. nonce: 0wNVP8
.. section: C API

All stable API extensions added after Python 3.2 are now available only when
Py_LIMITED_API is set to the PY_VERSION_HEX value of the minimum Python
version supporting this API.

..

.. bpo: 28929
.. date: 9815
.. nonce: Md7kb0
.. section: Documentation

Link the documentation to its source file on GitHub.

..

.. bpo: 25008
.. date: 9814
.. nonce: CeIzyU
.. section: Documentation

Document smtpd.py as effectively deprecated and add a pointer to aiosmtpd, a
third-party asyncio-based replacement.

..

.. bpo: 26355
.. date: 9813
.. nonce: SDq_8Y
.. section: Documentation

Add canonical header link on each page to corresponding major version of the
documentation. Patch by Matthias Bussonnier.

..

.. bpo: 29349
.. date: 9812
.. nonce: PjSo-t
.. section: Documentation

Fix Python 2 syntax in code for building the documentation.

..

.. bpo: 28087
.. date: 9811
.. nonce: m8dc4R
.. section: Tests

Skip test_asyncore and test_eintr poll failures on macOS. Skip some tests of
select.poll when running on macOS due to unresolved issues with the
underlying system poll function on some macOS versions.

..

.. bpo: 29571
.. date: 9810
.. nonce: r6Dixr
.. section: Tests

to match the behaviour of the ``re.LOCALE`` flag, test_re.test_locale_flag
now uses ``locale.getpreferredencoding(False)`` to determine the candidate
encoding for the test regex (allowing it to correctly skip the test when the
default locale encoding is a multi-byte encoding)

..

.. bpo: 28950
.. date: 9809
.. nonce: 1W8Glo
.. section: Tests

Disallow -j0 to be combined with -T/-l in regrtest command line arguments.

..

.. bpo: 28683
.. date: 9808
.. nonce: Fp-Hdq
.. section: Tests

Fix the tests that bind() a unix socket and raise PermissionError on Android
for a non-root user.

..

.. bpo: 26939
.. date: 9807
.. nonce: 7j_W5R
.. section: Tests

Add the support.setswitchinterval() function to fix test_functools hanging
on the Android armv7 qemu emulator.

..

.. bpo: 27593
.. date: 9806
.. nonce: v87xEr
.. section: Build

sys.version and the platform module python_build(), python_branch(), and
python_revision() functions now use git information rather than hg when
building from a repo.

..

.. bpo: 29572
.. date: 9805
.. nonce: iZ1XKK
.. section: Build

Update Windows build and OS X installers to use OpenSSL 1.0.2k.

..

.. bpo: 26851
.. date: 9804
.. nonce: R5243g
.. section: Build

Set Android compilation and link flags.

..

.. bpo: 28768
.. date: 9803
.. nonce: b9_a6E
.. section: Build

Fix implicit declaration of function _setmode. Patch by Masayuki Yamamoto

..

.. bpo: 29080
.. date: 9802
.. nonce: b3qLQT
.. section: Build

Removes hard dependency on hg.exe from PCBuild/build.bat

..

.. bpo: 23903
.. date: 9801
.. nonce: JXJ889
.. section: Build

Added missed names to PC/python3.def.

..

.. bpo: 28762
.. date: 9800
.. nonce: Ru0YN_
.. section: Build

lockf() is available on Android API level 24, but the F_LOCK macro is not
defined in android-ndk-r13.

..

.. bpo: 28538
.. date: 9799
.. nonce: FqtN7v
.. section: Build

Fix the compilation error that occurs because if_nameindex() is available on
Android API level 24, but the if_nameindex structure is not defined.

..

.. bpo: 20211
.. date: 9798
.. nonce: gpNptI
.. section: Build

Do not add the directory for installing C header files and the directory for
installing object code libraries to the cross compilation search paths.
Original patch by Thomas Petazzoni.

..

.. bpo: 28849
.. date: 9797
.. nonce: AzRRF5
.. section: Build

Do not define sys.implementation._multiarch on Android.
