.. bpo: 28532
.. date: 9778
.. nonce: KEYJny
.. release date: 2016-11-21
.. section: Core and Builtins

Show sys.version when -V option is supplied twice.

..

.. bpo: 27100
.. date: 9777
.. nonce: poVjXq
.. section: Core and Builtins

The with-statement now checks for __enter__ before it checks for __exit__.
This gives less confusing error messages when both methods are missing.
Patch by Jonathan Ellington.

..

.. bpo: 28746
.. date: 9776
.. nonce: r5MXdB
.. section: Core and Builtins

Fix the set_inheritable() file descriptor method on platforms that do not
have the ioctl FIOCLEX and FIONCLEX commands.

..

.. bpo: 26920
.. date: 9775
.. nonce: 1URwGb
.. section: Core and Builtins

Fix not getting the locale's charset upon initializing the interpreter, on
platforms that do not have langinfo.

..

.. bpo: 28648
.. date: 9774
.. nonce: z7B52W
.. section: Core and Builtins

Fixed crash in Py_DecodeLocale() in debug build on Mac OS X when decode
astral characters.  Patch by Xiang Zhang.

..

.. bpo: 19398
.. date: 9773
.. nonce: RYbEGH
.. section: Core and Builtins

Extra slash no longer added to sys.path components in case of empty
compile-time PYTHONPATH components.

..

.. bpo: 28665
.. date: 9772
.. nonce: v4nx86
.. section: Core and Builtins

Improve speed of the STORE_DEREF opcode by 40%.

..

.. bpo: 28583
.. date: 9771
.. nonce: F-QAx1
.. section: Core and Builtins

PyDict_SetDefault didn't combine split table when needed. Patch by Xiang
Zhang.

..

.. bpo: 27243
.. date: 9770
.. nonce: 61E6K5
.. section: Core and Builtins

Change PendingDeprecationWarning -> DeprecationWarning. As it was agreed in
the issue, __aiter__ returning an awaitable should result in
PendingDeprecationWarning in 3.5 and in DeprecationWarning in 3.6.

..

.. bpo: 26182
.. date: 9769
.. nonce: a8JXK2
.. section: Core and Builtins

Fix a refleak in code that raises DeprecationWarning.

..

.. bpo: 28721
.. date: 9768
.. nonce: BO9BUF
.. section: Core and Builtins

Fix asynchronous generators aclose() and athrow() to handle
StopAsyncIteration propagation properly.

..

.. bpo: 28752
.. date: 9767
.. nonce: Q-4oRE
.. section: Library

Restored the __reduce__() methods of datetime objects.

..

.. bpo: 28727
.. date: 9766
.. nonce: ubZP_b
.. section: Library

Regular expression patterns, _sre.SRE_Pattern objects created by
re.compile(), become comparable (only x==y and x!=y operators). This change
should fix the issue #18383: don't duplicate warning filters when the
warnings module is reloaded (thing usually only done in unit tests).

..

.. bpo: 20572
.. date: 9765
.. nonce: lGXaH9
.. section: Library

The subprocess.Popen.wait method's undocumented endtime parameter now raises
a DeprecationWarning.

..

.. bpo: 25659
.. date: 9764
.. nonce: lE2IlT
.. section: Library

In ctypes, prevent a crash calling the from_buffer() and from_buffer_copy()
methods on abstract classes like Array.

..

.. bpo: 19717
.. date: 9763
.. nonce: HXCAIz
.. section: Library

Makes Path.resolve() succeed on paths that do not exist. Patch by Vajrasky
Kok

..

.. bpo: 28563
.. date: 9762
.. nonce: iweEiw
.. section: Library

Fixed possible DoS and arbitrary code execution when handle plural form
selections in the gettext module.  The expression parser now supports exact
syntax supported by GNU gettext.

..

.. bpo: 28387
.. date: 9761
.. nonce: 1clJu7
.. section: Library

Fixed possible crash in _io.TextIOWrapper deallocator when the garbage
collector is invoked in other thread.  Based on patch by Sebastian Cufre.

..

.. bpo: 28600
.. date: 9760
.. nonce: wMVrjN
.. section: Library

Optimize loop.call_soon.

..

.. bpo: 28613
.. date: 9759
.. nonce: sqUPrv
.. section: Library

Fix get_event_loop() return the current loop if called from
coroutines/callbacks.

..

.. bpo: 28634
.. date: 9758
.. nonce: YlRydz
.. section: Library

Fix asyncio.isfuture() to support unittest.Mock.

..

.. bpo: 26081
.. date: 9757
.. nonce: 2Y8-a9
.. section: Library

Fix refleak in _asyncio.Future.__iter__().throw.

..

.. bpo: 28639
.. date: 9756
.. nonce: WUPo1o
.. section: Library

Fix inspect.isawaitable to always return bool Patch by Justin Mayfield.

..

.. bpo: 28652
.. date: 9755
.. nonce: f5M8FG
.. section: Library

Make loop methods reject socket kinds they do not support.

..

.. bpo: 28653
.. date: 9754
.. nonce: S5bA9i
.. section: Library

Fix a refleak in functools.lru_cache.

..

.. bpo: 28703
.. date: 9753
.. nonce: CRLTJc
.. section: Library

Fix asyncio.iscoroutinefunction to handle Mock objects.

..

.. bpo: 28704
.. date: 9752
.. nonce: EFWBII
.. section: Library

Fix create_unix_server to support Path-like objects (PEP 519).

..

.. bpo: 28720
.. date: 9751
.. nonce: Fsz-Lf
.. section: Library

Add collections.abc.AsyncGenerator.

..

.. bpo: 28513
.. date: 9750
.. nonce: L3joAz
.. section: Documentation

Documented command-line interface of zipfile.

..

.. bpo: 28666
.. date: 9749
.. nonce: RtTk-4
.. section: Tests

Now test.support.rmtree is able to remove unwritable or unreadable
directories.

..

.. bpo: 23839
.. date: 9748
.. nonce: zsT_L9
.. section: Tests

Various caches now are cleared before running every test file.

..

.. bpo: 10656
.. date: 9747
.. nonce: pR8FFU
.. section: Build

Fix out-of-tree building on AIX.  Patch by Tristan Carel and Michael
Haubenwallner.

..

.. bpo: 26359
.. date: 9746
.. nonce: CLz6qy
.. section: Build

Rename --with-optimiations to --enable-optimizations.

..

.. bpo: 28676
.. date: 9745
.. nonce: Wxf6Ds
.. section: Build

Prevent missing 'getentropy' declaration warning on macOS. Patch by Gareth
Rees.
