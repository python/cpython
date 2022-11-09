.. bpo: 28128
.. date: 9744
.. nonce: Lc2sFu
.. release date: 2016-10-31
.. section: Core and Builtins

Deprecation warning for invalid str and byte escape sequences now prints
better information about where the error occurs. Patch by Serhiy Storchaka
and Eric Smith.

..

.. bpo: 28509
.. date: 9743
.. nonce: _Fa4Uq
.. section: Core and Builtins

dict.update() no longer allocate unnecessary large memory.

..

.. bpo: 28426
.. date: 9742
.. nonce: E_quyK
.. section: Core and Builtins

Fixed potential crash in PyUnicode_AsDecodedObject() in debug build.

..

.. bpo: 28517
.. date: 9741
.. nonce: ExPkm9
.. section: Core and Builtins

Fixed of-by-one error in the peephole optimizer that caused keeping
unreachable code.

..

.. bpo: 28214
.. date: 9740
.. nonce: 6ECJox
.. section: Core and Builtins

Improved exception reporting for problematic __set_name__ attributes.

..

.. bpo: 23782
.. date: 9739
.. nonce: lonDzj
.. section: Core and Builtins

Fixed possible memory leak in _PyTraceback_Add() and exception loss in
PyTraceBack_Here().

..

.. bpo: 28471
.. date: 9738
.. nonce: Vd5pv7
.. section: Core and Builtins

Fix "Python memory allocator called without holding the GIL" crash in
socket.setblocking.

..

.. bpo: 27517
.. date: 9737
.. nonce: 1CYM8A
.. section: Library

LZMA compressor and decompressor no longer raise exceptions if given empty
data twice.  Patch by Benjamin Fogle.

..

.. bpo: 28549
.. date: 9736
.. nonce: ShnM2y
.. section: Library

Fixed segfault in curses's addch() with ncurses6.

..

.. bpo: 28449
.. date: 9735
.. nonce: 5JK6ES
.. section: Library

tarfile.open() with mode "r" or "r:" now tries to open a tar file with
compression before trying to open it without compression.  Otherwise it had
50% chance failed with ignore_zeros=True.

..

.. bpo: 23262
.. date: 9734
.. nonce: 6EVB7N
.. section: Library

The webbrowser module now supports Firefox 36+ and derived browsers.  Based
on patch by Oleg Broytman.

..

.. bpo: 27939
.. date: 9733
.. nonce: mTfADV
.. section: Library

Fixed bugs in tkinter.ttk.LabeledScale and tkinter.Scale caused by
representing the scale as float value internally in Tk.  tkinter.IntVar now
works if float value is set to underlying Tk variable.

..

.. bpo: 18844
.. date: 9732
.. nonce: oif1-H
.. section: Library

The various ways of specifying weights for random.choices() now produce the
same result sequences.

..

.. bpo: 28255
.. date: 9731
.. nonce: _ZH4wm
.. section: Library

calendar.TextCalendar().prmonth() no longer prints a space at the start of
new line after printing a month's calendar.  Patch by Xiang Zhang.

..

.. bpo: 20491
.. date: 9730
.. nonce: ObgnQ2
.. section: Library

The textwrap.TextWrapper class now honors non-breaking spaces. Based on
patch by Kaarle Ritvanen.

..

.. bpo: 28353
.. date: 9729
.. nonce: sKGbLL
.. section: Library

os.fwalk() no longer fails on broken links.

..

.. bpo: 28430
.. date: 9728
.. nonce: 4MiEYT
.. section: Library

Fix iterator of C implemented asyncio.Future doesn't accept non-None value
is passed to it.send(val).

..

.. bpo: 27025
.. date: 9727
.. nonce: foAViS
.. section: Library

Generated names for Tkinter widgets now start by the "!" prefix for
readability.

..

.. bpo: 25464
.. date: 9726
.. nonce: HDUTCu
.. section: Library

Fixed HList.header_exists() in tkinter.tix module by addin a workaround to
Tix library bug.

..

.. bpo: 28488
.. date: 9725
.. nonce: TgO112
.. section: Library

shutil.make_archive() no longer adds entry "./" to ZIP archive.

..

.. bpo: 25953
.. date: 9724
.. nonce: EKKJAQ
.. section: Library

re.sub() now raises an error for invalid numerical group reference in
replacement template even if the pattern is not found in the string.  Error
message for invalid group reference now includes the group index and the
position of the reference. Based on patch by SilentGhost.

..

.. bpo: 18219
.. date: 9723
.. nonce: 1ANQN1
.. section: Library

Optimize csv.DictWriter for large number of columns. Patch by Mariatta
Wijaya.

..

.. bpo: 28448
.. date: 9722
.. nonce: 5bduWe
.. section: Library

Fix C implemented asyncio.Future didn't work on Windows.

..

.. bpo: 28480
.. date: 9721
.. nonce: 9lHw6m
.. section: Library

Fix error building socket module when multithreading is disabled.

..

.. bpo: 24452
.. date: 9720
.. nonce: m9Kyg3
.. section: Library

Make webbrowser support Chrome on Mac OS X.

..

.. bpo: 20766
.. date: 9719
.. nonce: 4kvCzx
.. section: Library

Fix references leaked by pdb in the handling of SIGINT handlers.

..

.. bpo: 28492
.. date: 9718
.. nonce: pFRLQE
.. section: Library

Fix how StopIteration exception is raised in _asyncio.Future.

..

.. bpo: 28500
.. date: 9717
.. nonce: NINKzZ
.. section: Library

Fix asyncio to handle async gens GC from another thread.

..

.. bpo: 26923
.. date: 9716
.. nonce: 8dh3AV
.. section: Library

Fix asyncio.Gather to refuse being cancelled once all children are done.
Patch by Johannes Ebke.

..

.. bpo: 26796
.. date: 9715
.. nonce: TZyAfJ
.. section: Library

Don't configure the number of workers for default threadpool executor.
Initial patch by Hans Lawrenz.

..

.. bpo: 28544
.. date: 9714
.. nonce: KD1oFP
.. section: Library

Implement asyncio.Task in C.

..

.. bpo: 28522
.. date: 9713
.. nonce: XHMQa7
.. section: Windows

Fixes mishandled buffer reallocation in getpathp.c

..

.. bpo: 28444
.. date: 9712
.. nonce: zkc9nT
.. section: Build

Fix missing extensions modules when cross compiling.

..

.. bpo: 28208
.. date: 9711
.. nonce: DtoP1i
.. section: Build

Update Windows build and OS X installers to use SQLite 3.14.2.

..

.. bpo: 28248
.. date: 9710
.. nonce: KY_-en
.. section: Build

Update Windows build and OS X installers to use OpenSSL 1.0.2j.

..

.. bpo: 26944
.. date: 9709
.. nonce: ChZ_BO
.. section: Tests

Fix test_posix for Android where 'id -G' is entirely wrong or missing the
effective gid.

..

.. bpo: 28409
.. date: 9708
.. nonce: Q2IlxJ
.. section: Tests

regrtest: fix the parser of command line arguments.
