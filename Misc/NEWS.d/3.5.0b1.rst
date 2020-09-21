.. bpo: 24276
.. date: 9197
.. nonce: awsxJJ
.. release date: 2015-05-24
.. section: Core and Builtins

Fixed optimization of property descriptor getter.

..

.. bpo: 24268
.. date: 9196
.. nonce: nS7uea
.. section: Core and Builtins

PEP 489: Multi-phase extension module initialization. Patch by Petr
Viktorin.

..

.. bpo: 23955
.. date: 9195
.. nonce: hBHSaU
.. section: Core and Builtins

Add pyvenv.cfg option to suppress registry/environment lookup for generating
sys.path on Windows.

..

.. bpo: 24257
.. date: 9194
.. nonce: UBxshR
.. section: Core and Builtins

Fixed system error in the comparison of faked types.SimpleNamespace.

..

.. bpo: 22939
.. date: 9193
.. nonce: DWA9ls
.. section: Core and Builtins

Fixed integer overflow in iterator object.  Patch by Clement Rouault.

..

.. bpo: 23985
.. date: 9192
.. nonce: eezPxO
.. section: Core and Builtins

Fix a possible buffer overrun when deleting a slice from the front of a
bytearray and then appending some other bytes data.

..

.. bpo: 24102
.. date: 9191
.. nonce: 9T6h3m
.. section: Core and Builtins

Fixed exception type checking in standard error handlers.

..

.. bpo: 15027
.. date: 9190
.. nonce: wi9sCd
.. section: Core and Builtins

The UTF-32 encoder is now 3x to 7x faster.

..

.. bpo: 23290
.. date: 9189
.. nonce: 57aqLU
.. section: Core and Builtins

Optimize set_merge() for cases where the target is empty. (Contributed by
Serhiy Storchaka.)

..

.. bpo: 2292
.. date: 9188
.. nonce: h4sibO
.. section: Core and Builtins

PEP 448: Additional Unpacking Generalizations.

..

.. bpo: 24096
.. date: 9187
.. nonce: a_Rap7
.. section: Core and Builtins

Make warnings.warn_explicit more robust against mutation of the
warnings.filters list.

..

.. bpo: 23996
.. date: 9186
.. nonce: znqcT8
.. section: Core and Builtins

Avoid a crash when a delegated generator raises an unnormalized
StopIteration exception.  Patch by Stefan Behnel.

..

.. bpo: 23910
.. date: 9185
.. nonce: _gDzaj
.. section: Core and Builtins

Optimize property() getter calls.  Patch by Joe Jevnik.

..

.. bpo: 23911
.. date: 9184
.. nonce: 0FnTHk
.. section: Core and Builtins

Move path-based importlib bootstrap code to a separate frozen module.

..

.. bpo: 24192
.. date: 9183
.. nonce: 6ZxJ_R
.. section: Core and Builtins

Fix namespace package imports.

..

.. bpo: 24022
.. date: 9182
.. nonce: 1l8YBm
.. section: Core and Builtins

Fix tokenizer crash when processing undecodable source code.

..

.. bpo: 9951
.. date: 9181
.. nonce: wGztNC
.. section: Core and Builtins

Added a hex() method to bytes, bytearray, and memoryview.

..

.. bpo: 22906
.. date: 9180
.. nonce: WN_kQ6
.. section: Core and Builtins

PEP 479: Change StopIteration handling inside generators.

..

.. bpo: 24017
.. date: 9179
.. nonce: QJa1SC
.. section: Core and Builtins

PEP 492: Coroutines with async and await syntax.

..

.. bpo: 14373
.. date: 9178
.. nonce: 0sk6kE
.. section: Library

Added C implementation of functools.lru_cache().  Based on patches by Matt
Joiner and Alexey Kachayev.

..

.. bpo: 24230
.. date: 9177
.. nonce: b-kgme
.. section: Library

The tempfile module now accepts bytes for prefix, suffix and dir parameters
and returns bytes in such situations (matching the os module APIs).

..

.. bpo: 22189
.. date: 9176
.. nonce: 8epgat
.. section: Library

collections.UserString now supports __getnewargs__(), __rmod__(),
casefold(), format_map(), isprintable(), and maketrans(). Patch by Joe
Jevnik.

..

.. bpo: 24244
.. date: 9175
.. nonce: OKE_3R
.. section: Library

Prevents termination when an invalid format string is encountered on Windows
in strftime.

..

.. bpo: 23973
.. date: 9174
.. nonce: EK6awi
.. section: Library

PEP 484: Add the typing module.

..

.. bpo: 23086
.. date: 9173
.. nonce: Aix6Nv
.. section: Library

The collections.abc.Sequence() abstract base class added *start* and *stop*
parameters to the index() mixin. Patch by Devin Jeanpierre.

..

.. bpo: 20035
.. date: 9172
.. nonce: UNZzw6
.. section: Library

Replaced the ``tkinter._fix`` module used for setting up the Tcl/Tk
environment on Windows with a private function in the ``_tkinter`` module
that makes no permanent changes to the environment.

..

.. bpo: 24257
.. date: 9171
.. nonce: L_efq0
.. section: Library

Fixed segmentation fault in sqlite3.Row constructor with faked cursor type.

..

.. bpo: 15836
.. date: 9170
.. nonce: gU3Rmx
.. section: Library

assertRaises(), assertRaisesRegex(), assertWarns() and assertWarnsRegex()
assertments now check the type of the first argument to prevent possible
user error.  Based on patch by Daniel Wagner-Hall.

..

.. bpo: 9858
.. date: 9169
.. nonce: uke9pa
.. section: Library

Add missing method stubs to _io.RawIOBase.  Patch by Laura Rupprecht.

..

.. bpo: 22955
.. date: 9168
.. nonce: Jw_B9_
.. section: Library

attrgetter, itemgetter and methodcaller objects in the operator module now
support pickling.  Added readable and evaluable repr for these objects.
Based on patch by Josh Rosenberg.

..

.. bpo: 22107
.. date: 9167
.. nonce: 2F8k4W
.. section: Library

tempfile.gettempdir() and tempfile.mkdtemp() now try again when a directory
with the chosen name already exists on Windows as well as on Unix.
tempfile.mkstemp() now fails early if parent directory is not valid (not
exists or is a file) on Windows.

..

.. bpo: 23780
.. date: 9166
.. nonce: jFPVcN
.. section: Library

Improved error message in os.path.join() with single argument.

..

.. bpo: 6598
.. date: 9165
.. nonce: JdZNDt
.. section: Library

Increased time precision and random number range in email.utils.make_msgid()
to strengthen the uniqueness of the message ID.

..

.. bpo: 24091
.. date: 9164
.. nonce: Jw0-wj
.. section: Library

Fixed various crashes in corner cases in C implementation of ElementTree.

..

.. bpo: 21931
.. date: 9163
.. nonce: t6lGxY
.. section: Library

msilib.FCICreate() now raises TypeError in the case of a bad argument
instead of a ValueError with a bogus FCI error number. Patch by Jeffrey
Armstrong.

..

.. bpo: 13866
.. date: 9162
.. nonce: n5NAj0
.. section: Library

*quote_via* argument added to urllib.parse.urlencode.

..

.. bpo: 20098
.. date: 9161
.. nonce: Y4otaf
.. section: Library

New mangle_from policy option for email, default True for compat32, but
False for all other policies.

..

.. bpo: 24211
.. date: 9160
.. nonce: j3Afpc
.. section: Library

The email library now supports RFC 6532: it can generate headers using utf-8
instead of encoded words.

..

.. bpo: 16314
.. date: 9159
.. nonce: Xc4d1O
.. section: Library

Added support for the LZMA compression in distutils.

..

.. bpo: 21804
.. date: 9158
.. nonce: lEhTlc
.. section: Library

poplib now supports RFC 6856 (UTF8).

..

.. bpo: 18682
.. date: 9157
.. nonce: 6Pnfte
.. section: Library

Optimized pprint functions for builtin scalar types.

..

.. bpo: 22027
.. date: 9156
.. nonce: _aeUQS
.. section: Library

smtplib now supports RFC 6531 (SMTPUTF8).

..

.. bpo: 23488
.. date: 9155
.. nonce: 7gs3Cm
.. section: Library

Random generator objects now consume 2x less memory on 64-bit.

..

.. bpo: 1322
.. date: 9154
.. nonce: 495nFL
.. section: Library

platform.dist() and platform.linux_distribution() functions are now
deprecated.  Initial patch by Vajrasky Kok.

..

.. bpo: 22486
.. date: 9153
.. nonce: Yxov5m
.. section: Library

Added the math.gcd() function.  The fractions.gcd() function now is
deprecated.  Based on patch by Mark Dickinson.

..

.. bpo: 24064
.. date: 9152
.. nonce: zXC7OL
.. section: Library

Property() docstrings are now writeable. (Patch by Berker Peksag.)

..

.. bpo: 22681
.. date: 9151
.. nonce: 2rIoA2
.. section: Library

Added support for the koi8_t encoding.

..

.. bpo: 22682
.. date: 9150
.. nonce: cP4i3L
.. section: Library

Added support for the kz1048 encoding.

..

.. bpo: 23796
.. date: 9149
.. nonce: JJmUnc
.. section: Library

peek and read1 methods of BufferedReader now raise ValueError if they called
on a closed object. Patch by John Hergenroeder.

..

.. bpo: 21795
.. date: 9148
.. nonce: BDLMS4
.. section: Library

smtpd now supports the 8BITMIME extension whenever the new *decode_data*
constructor argument is set to False.

..

.. bpo: 24155
.. date: 9147
.. nonce: FZx5c2
.. section: Library

optimize heapq.heapify() for better cache performance when heapifying large
lists.

..

.. bpo: 21800
.. date: 9146
.. nonce: evGSKc
.. section: Library

imaplib now supports RFC 5161 (enable), RFC 6855 (utf8/internationalized
email) and automatically encodes non-ASCII usernames and passwords to UTF8.

..

.. bpo: 20274
.. date: 9145
.. nonce: uVHogg
.. section: Library

When calling a _sqlite.Connection, it now complains if passed any keyword
arguments.  Previously it silently ignored them.

..

.. bpo: 20274
.. date: 9144
.. nonce: hBst4M
.. section: Library

Remove ignored and erroneous "kwargs" parameters from three METH_VARARGS
methods on _sqlite.Connection.

..

.. bpo: 24134
.. date: 9143
.. nonce: Ajw0S-
.. section: Library

assertRaises(), assertRaisesRegex(), assertWarns() and assertWarnsRegex()
checks now emits a deprecation warning when callable is None or keyword
arguments except msg is passed in the context manager mode.

..

.. bpo: 24018
.. date: 9142
.. nonce: hk7Rcn
.. section: Library

Add a collections.abc.Generator abstract base class. Contributed by Stefan
Behnel.

..

.. bpo: 23880
.. date: 9141
.. nonce: QtKupC
.. section: Library

Tkinter's getint() and getdouble() now support Tcl_Obj. Tkinter's
getdouble() now supports any numbers (in particular int).

..

.. bpo: 22619
.. date: 9140
.. nonce: 1gJEqV
.. section: Library

Added negative limit support in the traceback module. Based on patch by
Dmitry Kazakov.

..

.. bpo: 24094
.. date: 9139
.. nonce: 7T-u7k
.. section: Library

Fix possible crash in json.encode with poorly behaved dict subclasses.

..

.. bpo: 9246
.. date: 9138
.. nonce: oM-Ikk
.. section: Library

On POSIX, os.getcwd() now supports paths longer than 1025 bytes. Patch
written by William Orr.

..

.. bpo: 17445
.. date: 9137
.. nonce: Z-QYh5
.. section: Library

add difflib.diff_bytes() to support comparison of byte strings (fixes a
regression from Python 2).

..

.. bpo: 23917
.. date: 9136
.. nonce: uMVPV7
.. section: Library

Fall back to sequential compilation when ProcessPoolExecutor doesn't exist.
Patch by Claudiu Popa.

..

.. bpo: 23008
.. date: 9135
.. nonce: OZFCd-
.. section: Library

Fixed resolving attributes with boolean value is False in pydoc.

..

.. bpo: 0
.. date: 9134
.. nonce: 6tJNf2
.. section: Library

Fix asyncio issue 235: LifoQueue and PriorityQueue's put didn't increment
unfinished tasks (this bug was introduced when JoinableQueue was merged with
Queue).

..

.. bpo: 23908
.. date: 9133
.. nonce: ATdNG-
.. section: Library

os functions now reject paths with embedded null character on Windows
instead of silently truncating them.

..

.. bpo: 23728
.. date: 9132
.. nonce: YBmQmV
.. section: Library

binascii.crc_hqx() could return an integer outside of the range 0-0xffff for
empty data.

..

.. bpo: 23887
.. date: 9131
.. nonce: _XpjPN
.. section: Library

urllib.error.HTTPError now has a proper repr() representation. Patch by
Berker Peksag.

..

.. bpo: 0
.. date: 9130
.. nonce: MjNdSC
.. section: Library

asyncio: New event loop APIs: set_task_factory() and get_task_factory().

..

.. bpo: 0
.. date: 9129
.. nonce: rVcHXp
.. section: Library

asyncio: async() function is deprecated in favour of ensure_future().

..

.. bpo: 24178
.. date: 9128
.. nonce: -enO4y
.. section: Library

asyncio.Lock, Condition, Semaphore, and BoundedSemaphore support new 'async
with' syntax.  Contributed by Yury Selivanov.

..

.. bpo: 24179
.. date: 9127
.. nonce: wDy_WZ
.. section: Library

Support 'async for' for asyncio.StreamReader. Contributed by Yury Selivanov.

..

.. bpo: 24184
.. date: 9126
.. nonce: El74TU
.. section: Library

Add AsyncIterator and AsyncIterable ABCs to collections.abc.  Contributed by
Yury Selivanov.

..

.. bpo: 22547
.. date: 9125
.. nonce: _ikCaj
.. section: Library

Implement informative __repr__ for inspect.BoundArguments. Contributed by
Yury Selivanov.

..

.. bpo: 24190
.. date: 9124
.. nonce: 1a3vWW
.. section: Library

Implement inspect.BoundArgument.apply_defaults() method. Contributed by Yury
Selivanov.

..

.. bpo: 20691
.. date: 9123
.. nonce: -raLyf
.. section: Library

Add 'follow_wrapped' argument to inspect.Signature.from_callable() and
inspect.signature(). Contributed by Yury Selivanov.

..

.. bpo: 24248
.. date: 9122
.. nonce: IxWooo
.. section: Library

Deprecate inspect.Signature.from_function() and
inspect.Signature.from_builtin().

..

.. bpo: 23898
.. date: 9121
.. nonce: OSiZie
.. section: Library

Fix inspect.classify_class_attrs() to support attributes with overloaded
__eq__ and __bool__.  Patch by Mike Bayer.

..

.. bpo: 24298
.. date: 9120
.. nonce: u_TaxI
.. section: Library

Fix inspect.signature() to correctly unwrap wrappers around bound methods.

..

.. bpo: 23184
.. date: 9119
.. nonce: G_Cp9v
.. section: IDLE

remove unused names and imports in idlelib. Initial patch by Al Sweigart.

..

.. bpo: 21520
.. date: 9118
.. nonce: FKtvmQ
.. section: Tests

test_zipfile no longer fails if the word 'bad' appears anywhere in the name
of the current directory.

..

.. bpo: 9517
.. date: 9117
.. nonce: W0Ag2V
.. section: Tests

Move script_helper into the support package. Patch by Christie Wilson.

..

.. bpo: 22155
.. date: 9116
.. nonce: 9EbOit
.. section: Documentation

Add File Handlers subsection with createfilehandler to tkinter doc.  Remove
obsolete example from FAQ.  Patch by Martin Panter.

..

.. bpo: 24029
.. date: 9115
.. nonce: M2Bnks
.. section: Documentation

Document the name binding behavior for submodule imports.

..

.. bpo: 24077
.. date: 9114
.. nonce: 2Og2j-
.. section: Documentation

Fix typo in man page for -I command option: -s, not -S

..

.. bpo: 24000
.. date: 9113
.. nonce: MJyXRr
.. section: Tools/Demos

Improved Argument Clinic's mapping of converters to legacy "format units".
Updated the documentation to match.

..

.. bpo: 24001
.. date: 9112
.. nonce: m74vst
.. section: Tools/Demos

Argument Clinic converters now use accept={type} instead of types={'type'}
to specify the types the converter accepts.

..

.. bpo: 23330
.. date: 9111
.. nonce: LTlKDp
.. section: Tools/Demos

h2py now supports arbitrary filenames in #include.

..

.. bpo: 24031
.. date: 9110
.. nonce: duGo88
.. section: Tools/Demos

make patchcheck now supports git checkouts, too.
