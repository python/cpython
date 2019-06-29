.. bpo: 31558
.. date: 2017-10-16-14-27-25
.. nonce: K-uRRm
.. release date: 2017-10-16
.. section: Core and Builtins

``gc.freeze()`` is a new API that allows for moving all objects currently
tracked by the garbage collector to a permanent generation, effectively
removing them from future collection events. This can be used to protect
those objects from having their PyGC_Head mutated. In effect, this enables
great copy-on-write stability at fork().

..

.. bpo: 31642
.. date: 2017-10-08-10-00-55
.. nonce: 1IKqgs
.. section: Core and Builtins

Restored blocking "from package import module" by setting
sys.modules["package.module"] to None.

..

.. bpo: 31708
.. date: 2017-10-06-02-10-48
.. nonce: 66CCVU
.. section: Core and Builtins

Allow use of asynchronous generator expressions in synchronous functions.

..

.. bpo: 31709
.. date: 2017-10-06-00-27-04
.. nonce: _PmU51
.. section: Core and Builtins

Drop support of asynchronous __aiter__.

..

.. bpo: 30404
.. date: 2017-10-03-23-46-39
.. nonce: _9Yi5u
.. section: Core and Builtins

The -u option now makes the stdout and stderr streams unbuffered rather than
line-buffered.

..

.. bpo: 31619
.. date: 2017-09-29-20-32-24
.. nonce: 6gQ1kv
.. section: Core and Builtins

Fixed a ValueError when convert a string with large number of underscores to
integer with binary base.

..

.. bpo: 31602
.. date: 2017-09-27-09-30-03
.. nonce: MtgLCn
.. section: Core and Builtins

Fix an assertion failure in `zipimporter.get_source()` in case of a bad
`zlib.decompress()`. Patch by Oren Milman.

..

.. bpo: 31592
.. date: 2017-09-26-16-05-04
.. nonce: IFBZj9
.. section: Core and Builtins

Fixed an assertion failure in Python parser in case of a bad
`unicodedata.normalize()`. Patch by Oren Milman.

..

.. bpo: 31588
.. date: 2017-09-26-13-03-16
.. nonce: wT9Iy7
.. section: Core and Builtins

Raise a `TypeError` with a helpful error message when class creation fails
due to a metaclass with a bad ``__prepare__()`` method. Patch by Oren
Milman.

..

.. bpo: 31574
.. date: 2017-09-25-12-35-48
.. nonce: 5yX5r5
.. section: Core and Builtins

Importlib was instrumented with two dtrace probes to profile import timing.

..

.. bpo: 31566
.. date: 2017-09-24-09-57-04
.. nonce: OxwINs
.. section: Core and Builtins

Fix an assertion failure in `_warnings.warn()` in case of a bad ``__name__``
global. Patch by Oren Milman.

..

.. bpo: 31506
.. date: 2017-09-19-10-29-36
.. nonce: pRVTRB
.. section: Core and Builtins

Improved the error message logic for object.__new__ and object.__init__.

..

.. bpo: 31505
.. date: 2017-09-18-12-07-39
.. nonce: VomaFa
.. section: Core and Builtins

Fix an assertion failure in `json`, in case `_json.make_encoder()` received
a bad `encoder()` argument. Patch by Oren Milman.

..

.. bpo: 31492
.. date: 2017-09-16-22-49-16
.. nonce: RtyteL
.. section: Core and Builtins

Fix assertion failures in case of failing to import from a module with a bad
``__name__`` attribute, and in case of failing to access an attribute of
such a module. Patch by Oren Milman.

..

.. bpo: 31478
.. date: 2017-09-15-09-13-07
.. nonce: o06iKD
.. section: Core and Builtins

Fix an assertion failure in `_random.Random.seed()` in case the argument has
a bad ``__abs__()`` method. Patch by Oren Milman.

..

.. bpo: 31336
.. date: 2017-09-13-12-04-23
.. nonce: gi2ahY
.. section: Core and Builtins

Speed up class creation by 10-20% by reducing the overhead in the necessary
special method lookups.  Patch by Stefan Behnel.

..

.. bpo: 31415
.. date: 2017-09-11-14-28-56
.. nonce: GBdz7o
.. section: Core and Builtins

Add ``-X importtime`` option to show how long each import takes. It can be
used to optimize application's startup time.  Support the
:envvar:`PYTHONPROFILEIMPORTTIME` as an equivalent way to enable this.

..

.. bpo: 31410
.. date: 2017-09-10-20-58-51
.. nonce: wD_RbH
.. section: Core and Builtins

Optimized calling wrapper and classmethod descriptors.

..

.. bpo: 31353
.. date: 2017-09-05-14-19-02
.. nonce: oGZUeJ
.. section: Core and Builtins

:pep:`553` - Add a new built-in called ``breakpoint()`` which calls
``sys.breakpointhook()``.  By default this imports ``pdb`` and calls
``pdb.set_trace()``, but users may override ``sys.breakpointhook()`` to call
whatever debugger they want.  The original value of the hook is saved in
``sys.__breakpointhook__``.

..

.. bpo: 17852
.. date: 2017-09-04-12-46-25
.. nonce: OxAtCg
.. section: Core and Builtins

Maintain a list of open buffered files, flush them before exiting the
interpreter.  Based on a patch from Armin Rigo.

..

.. bpo: 31315
.. date: 2017-09-01-00-40-58
.. nonce: ZX20bl
.. section: Core and Builtins

Fix an assertion failure in imp.create_dynamic(), when spec.name is not a
string. Patch by Oren Milman.

..

.. bpo: 31311
.. date: 2017-08-31-17-52-56
.. nonce: bNE2l-
.. section: Core and Builtins

Fix a crash in the ``__setstate__()`` method of `ctypes._CData`, in case of
a bad ``__dict__``. Patch by Oren Milman.

..

.. bpo: 31293
.. date: 2017-08-28-17-51-42
.. nonce: eMYZXj
.. section: Core and Builtins

Fix crashes in true division and multiplication of a timedelta object by a
float with a bad as_integer_ratio() method. Patch by Oren Milman.

..

.. bpo: 31285
.. date: 2017-08-27-21-18-30
.. nonce: 7lzaKV
.. section: Core and Builtins

Fix an assertion failure in `warnings.warn_explicit`, when the return value
of the received loader's get_source() has a bad splitlines() method. Patch
by Oren Milman.

..

.. bpo: 30406
.. date: 2017-07-20-22-03-44
.. nonce: _kr47t
.. section: Core and Builtins

Make ``async`` and ``await`` proper keywords, as specified in :pep:`492`.

..

.. bpo: 30058
.. date: 2017-10-12-19-00-53
.. nonce: cENtry
.. section: Library

Fixed buffer overflow in select.kqueue.control().

..

.. bpo: 31672
.. date: 2017-10-12-02-47-16
.. nonce: DaOkVd
.. section: Library

``idpattern`` in ``string.Template`` matched some non-ASCII characters. Now
it uses ``-i`` regular expression local flag to avoid non-ASCII characters.

..

.. bpo: 31701
.. date: 2017-10-09-17-42-30
.. nonce: NRrVel
.. section: Library

On Windows, faulthandler.enable() now ignores MSC and COM exceptions.

..

.. bpo: 31728
.. date: 2017-10-08-23-28-30
.. nonce: XrVMME
.. section: Library

Prevent crashes in `_elementtree` due to unsafe cleanup of `Element.text`
and `Element.tail`. Patch by Oren Milman.

..

.. bpo: 31671
.. date: 2017-10-04-21-28-44
.. nonce: E-zfc9
.. section: Library

Now ``re.compile()`` converts passed RegexFlag to normal int object before
compiling. bm_regex_compile benchmark shows 14% performance improvements.

..

.. bpo: 30397
.. date: 2017-10-03-22-45-50
.. nonce: e4F7Kr
.. section: Library

The types of compiled regular objects and match objects are now exposed as
`re.Pattern` and `re.Match`.  This adds information in pydoc output for the
re module.

..

.. bpo: 31675
.. date: 2017-10-03-15-06-24
.. nonce: Nh7jJ3
.. section: Library

Fixed memory leaks in Tkinter's methods splitlist() and split() when pass a
string larger than 2 GiB.

..

.. bpo: 31673
.. date: 2017-10-03-14-37-46
.. nonce: RFCrka
.. section: Library

Fixed typo in the name of Tkinter's method adderrorinfo().

..

.. bpo: 31648
.. date: 2017-09-30-10-45-12
.. nonce: Cai7ji
.. section: Library

Improvements to path predicates in ElementTree:
Allow whitespace around predicate parts, i.e. "[a = 'text']" instead of requiring the less readable "[a='text']".
Add support for text comparison of the current node, like "[.='text']".
Patch by Stefan Behnel.

..

.. bpo: 30806
.. date: 2017-09-29
.. nonce: lP5GrH
.. section: Library

Fix the string representation of a netrc object.

..

.. bpo: 31638
.. date: 2017-09-29-07-14-28
.. nonce: jElfhl
.. section: Library

Add optional argument ``compressed`` to ``zipapp.create_archive``, and add
option ``--compress`` to the command line interface of ``zipapp``.

..

.. bpo: 25351
.. date: 2017-09-28-23-10-51
.. nonce: 2JmFpF
.. section: Library

Avoid venv activate failures with undefined variables

..

.. bpo: 20519
.. date: 2017-09-28-13-17-33
.. nonce: FteeQQ
.. section: Library

Avoid ctypes use (if possible) and improve import time for uuid.

..

.. bpo: 28293
.. date: 2017-09-26-17-51-17
.. nonce: UC5pm4
.. section: Library

The regular expression cache is no longer completely dumped when it is full.

..

.. bpo: 31596
.. date: 2017-09-26-11-38-52
.. nonce: 50Eyel
.. section: Library

Added pthread_getcpuclockid() to the time module

..

.. bpo: 27494
.. date: 2017-09-26-01-43-17
.. nonce: 37QnaT
.. section: Library

Make 2to3 accept a trailing comma in generator expressions. For example,
``set(x for x in [],)`` is now allowed.

..

.. bpo: 30347
.. date: 2017-09-25-14-04-30
.. nonce: B4--_D
.. section: Library

Stop crashes when concurrently iterate over itertools.groupby() iterators.

..

.. bpo: 30346
.. date: 2017-09-24-13-08-46
.. nonce: Csse77
.. section: Library

An iterator produced by itertools.groupby() iterator now becomes exhausted
after advancing the groupby iterator.

..

.. bpo: 31556
.. date: 2017-09-22-23-48-49
.. nonce: 9J0u5H
.. section: Library

Cancel asyncio.wait_for future faster if timeout <= 0

..

.. bpo: 31540
.. date: 2017-09-22-16-02-00
.. nonce: ybDHT5
.. section: Library

Allow passing a context object in
:class:`concurrent.futures.ProcessPoolExecutor` constructor. Also, free job
resources in :class:`concurrent.futures.ProcessPoolExecutor` earlier to
improve memory usage when a worker waits for new jobs.

..

.. bpo: 31516
.. date: 2017-09-20-18-43-01
.. nonce: 23Yuq3
.. section: Library

``threading.current_thread()`` should not return a dummy thread at shutdown.

..

.. bpo: 31525
.. date: 2017-09-19-18-48-21
.. nonce: O2TIL2
.. section: Library

In the sqlite module, require the sqlite3_prepare_v2 API. Thus, the sqlite
module now requires sqlite version at least 3.3.9.

..

.. bpo: 26510
.. date: 2017-09-19-13-29-29
.. nonce: oncW6V
.. section: Library

argparse subparsers are now required by default.  This matches behaviour in
Python 2. For optional subparsers, use the new parameter
``add_subparsers(required=False)``. Patch by Anthony Sottile.
(As of 3.7.0rc1, the default was changed to not required as had been the case
since Python 3.3.)

..

.. bpo: 27541
.. date: 2017-09-17-19-59-04
.. nonce: cIMFJW
.. section: Library

Reprs of subclasses of some collection and iterator classes (`bytearray`,
`array.array`, `collections.deque`, `collections.defaultdict`,
`itertools.count`, `itertools.repeat`) now contain actual type name insteads
of hardcoded name of the base class.

..

.. bpo: 31351
.. date: 2017-09-17-15-24-25
.. nonce: yQdKv-
.. section: Library

python -m ensurepip now exits with non-zero exit code if pip bootstrapping
has failed.

..

.. bpo: 31389
.. date: 2017-09-07-15-31-47
.. nonce: jNFYqB
.. section: Library

``pdb.set_trace()`` now takes an optional keyword-only argument ``header``.
If given, this is printed to the console just before debugging begins.

..

.. bpo: 31537
.. date: 2017-10-08-23-02-14
.. nonce: SiFNM8
.. section: Documentation

Fix incorrect usage of ``get_history_length`` in readline documentation
example code. Patch by Brad Smith.

..

.. bpo: 30085
.. date: 2017-09-14-18-44-50
.. nonce: 0J9w-u
.. section: Documentation

The operator functions without double underscores are preferred for clarity.
The one with underscores are only kept for back-compatibility.

..

.. bpo: 31696
.. date: 2017-10-04-23-40-32
.. nonce: Y3_aBV
.. section: Build

Improve compiler version information in :data:`sys.version` when Python is
built with Clang.

..

.. bpo: 31625
.. date: 2017-09-28-23-21-20
.. nonce: Bb2NXr
.. section: Build

Stop using ranlib on static libraries. Instead, we assume ar supports the
's' flag.

..

.. bpo: 31624
.. date: 2017-09-28-20-54-52
.. nonce: 11w91_
.. section: Build

Remove support for BSD/OS.

..

.. bpo: 22140
.. date: 2017-09-26-22-39-58
.. nonce: ZRf7Wn
.. section: Build

Prevent double substitution of prefix in python-config.sh.

..

.. bpo: 31569
.. date: 2017-09-25-00-25-23
.. nonce: TS49pM
.. section: Build

Correct PCBuild/ case to PCbuild/ in build scripts and documentation.

..

.. bpo: 31536
.. date: 2017-09-20-21-32-21
.. nonce: KUDjno
.. section: Build

Avoid wholesale rebuild after `make regen-all` if nothing changed.

..

.. bpo: 31460
.. date: 2017-09-30-19-03-26
.. nonce: HpveI6
.. section: IDLE

Simplify the API of IDLE's Module Browser.
Passing a widget instead of an flist with a root widget opens the option of
creating a browser frame that is only part of a window.  Passing a full file
name instead of pieces assumed to come from a .py file opens the possibility
of browsing python files that do not end in .py.

..

.. bpo: 31649
.. date: 2017-09-30-13-59-18
.. nonce: LxN4Vb
.. section: IDLE

IDLE - Make _htest, _utest parameters keyword only.

..

.. bpo: 31559
.. date: 2017-09-23-12-52-24
.. nonce: ydckYX
.. section: IDLE

Remove test order dependence in idle_test.test_browser.

..

.. bpo: 31459
.. date: 2017-09-22-20-26-23
.. nonce: L0pnH9
.. section: IDLE

Rename IDLE's module browser from Class Browser to Module Browser. The
original module-level class and method browser became a module browser, with
the addition of module-level functions, years ago. Nested classes and
functions were added yesterday.  For back-compatibility, the virtual event
<<open-class-browser>>, which appears on the Keys tab of the Settings
dialog, is not changed. Patch by Cheryl Sabella.

..

.. bpo: 31500
.. date: 2017-09-18-10-43-03
.. nonce: Y_YDxA
.. section: IDLE

Default fonts now are scaled on HiDPI displays.

..

.. bpo: 1612262
.. date: 2017-08-14-15-13-50
.. nonce: -x_Oyq
.. section: IDLE

IDLE module browser now shows nested classes and functions. Original patches
for code and tests by Guilherme Polo and Cheryl Sabella, respectively.

..

.. bpo: 28280
.. date: 2017-09-30-19-41-44
.. nonce: K_EjpO
.. section: C API

Make `PyMapping_Keys()`, `PyMapping_Values()` and `PyMapping_Items()` always
return a `list` (rather than a `list` or a `tuple`). Patch by Oren Milman.

..

.. bpo: 31532
.. date: 2017-09-20-21-59-52
.. nonce: s9Cw9_
.. section: C API

Fix memory corruption due to allocator mix in getpath.c between Py_GetPath()
and Py_SetPath()

..

.. bpo: 25658
.. date: 2017-06-24-14-30-44
.. nonce: vm8vGE
.. section: C API

Implement :pep:`539` for Thread Specific Storage (TSS) API: it is a new Thread
Local Storage (TLS) API to CPython which would supersede use of the existing
TLS API within the CPython interpreter, while deprecating the existing API.
PEP written by Erik M. Bray, patch by Masayuki Yamamoto.
