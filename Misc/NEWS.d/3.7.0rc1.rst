.. bpo: 33803
.. date: 2018-06-07-20-18-38
.. nonce: n-Nq6_
.. release date: 2018-06-12
.. section: Core and Builtins

Fix a crash in hamt.c caused by enabling GC tracking for an object that
hadn't all of its fields set to NULL.

..

.. bpo: 33706
.. date: 2018-05-31-14-50-04
.. nonce: ztlH04
.. section: Core and Builtins

Fix a crash in Python initialization when parsing the command line options.
Thanks Christoph Gohlke for the bug report and the fix!

..

.. bpo: 30654
.. date: 2018-05-28-12-28-53
.. nonce: 9fDJye
.. section: Core and Builtins

Fixed reset of the SIGINT handler to SIG_DFL on interpreter shutdown even
when there was a custom handler set previously. Patch by Philipp Kerling.

..

.. bpo: 31849
.. date: 2018-05-14-11-00-00
.. nonce: EmHaH4
.. section: Core and Builtins

Fix signed/unsigned comparison warning in pyhash.c.

..

.. bpo: 30167
.. date: 2018-06-10-19-29-17
.. nonce: G5EgC5
.. section: Library

Prevent site.main() exception if PYTHONSTARTUP is set. Patch by Steve Weber.

..

.. bpo: 33812
.. date: 2018-06-10-13-26-02
.. nonce: frGAOr
.. section: Library

Datetime instance d with non-None tzinfo, but with d.tzinfo.utcoffset(d)
returning None is now treated as naive by the astimezone() method.

..

.. bpo: 30805
.. date: 2018-06-08-17-34-16
.. nonce: 3qCWa0
.. section: Library

Avoid race condition with debug logging

..

.. bpo: 33694
.. date: 2018-06-07-23-51-00
.. nonce: F1zIR1
.. section: Library

asyncio: Fix a race condition causing data loss on
pause_reading()/resume_reading() when using the ProactorEventLoop.

..

.. bpo: 32493
.. date: 2018-06-07-18-55-35
.. nonce: 1Bte62
.. section: Library

Correct test for ``uuid_enc_be`` availability in ``configure.ac``. Patch by
Michael Felt.

..

.. bpo: 33792
.. date: 2018-06-07-12-38-12
.. nonce: 3aKG7u
.. section: Library

Add asyncio.WindowsSelectorEventLoopPolicy and
asyncio.WindowsProactorEventLoopPolicy.

..

.. bpo: 33778
.. date: 2018-06-05-20-22-30
.. nonce: _tSAS6
.. section: Library

Update ``unicodedata``'s database to Unicode version 11.0.0.

..

.. bpo: 33770
.. date: 2018-06-05-11-29-26
.. nonce: oBhxxw
.. section: Library

improve base64 exception message for encoded inputs of invalid length

..

.. bpo: 33769
.. date: 2018-06-04-13-46-39
.. nonce: D_pxYz
.. section: Library

asyncio/start_tls: Fix error message; cancel callbacks in case of an
unhandled error; mark SSLTransport as closed if it is aborted.

..

.. bpo: 33767
.. date: 2018-06-03-22-41-59
.. nonce: 2e82g3
.. section: Library

The concatenation (``+``) and repetition (``*``) sequence operations now
raise :exc:`TypeError` instead of :exc:`SystemError` when performed on
:class:`mmap.mmap` objects.  Patch by Zackery Spytz.

..

.. bpo: 33734
.. date: 2018-06-01-10-55-48
.. nonce: x1W9x0
.. section: Library

asyncio/ssl: Fix AttributeError, increase default handshake timeout

..

.. bpo: 11874
.. date: 2018-05-23-00-26-27
.. nonce: glK5iP
.. section: Library

Use a better regex when breaking usage into wrappable parts. Avoids bogus
assertion errors from custom metavar strings.

..

.. bpo: 33582
.. date: 2018-05-19-15-58-14
.. nonce: qBZPmF
.. section: Library

Emit a deprecation warning for inspect.formatargspec

..

.. bpo: 33409
.. date: 2018-06-08-23-46-01
.. nonce: r4z9MM
.. section: Documentation

Clarified the relationship between :pep:`538`'s PYTHONCOERCECLOCALE and PEP
540's PYTHONUTF8 mode.

..

.. bpo: 33736
.. date: 2018-06-01-12-27-40
.. nonce: JVegIu
.. section: Documentation

Improve the documentation of :func:`asyncio.open_connection`,
:func:`asyncio.start_server` and their UNIX socket counterparts.

..

.. bpo: 31432
.. date: 2017-09-13-07-14-59
.. nonce: yAY4Z3
.. section: Documentation

Clarify meaning of CERT_NONE, CERT_OPTIONAL, and CERT_REQUIRED flags for
ssl.SSLContext.verify_mode.

..

.. bpo: 5755
.. date: 2018-06-04-21-34-34
.. nonce: 65GmCj
.. section: Build

Move ``-Wstrict-prototypes`` option to ``CFLAGS_NODIST`` from ``OPT``. This
option emitted annoying warnings when building extension modules written in
C++.

..

.. bpo: 33720
.. date: 2018-06-04-09-20-53
.. nonce: VKDXHK
.. section: Windows

Reduces maximum marshal recursion depth on release builds.

..

.. bpo: 33656
.. date: 2018-06-10-17-59-36
.. nonce: 60ZqJS
.. section: IDLE

On Windows, add API call saying that tk scales for DPI. On Windows 8.1+ or
10, with DPI compatibility properties of the Python binary unchanged, and a
monitor resolution greater than 96 DPI, this should make text and lines
sharper.  It should otherwise have no effect.

..

.. bpo: 33768
.. date: 2018-06-04-19-23-11
.. nonce: I_2qpV
.. section: IDLE

Clicking on a context line moves that line to the top of the editor window.

..

.. bpo: 33763
.. date: 2018-06-03-20-12-57
.. nonce: URiFlE
.. section: IDLE

IDLE: Use read-only text widget for code context instead of label widget.

..

.. bpo: 33664
.. date: 2018-06-03-09-13-28
.. nonce: PZzQyL
.. section: IDLE

Scroll IDLE editor text by lines. Previously, the mouse wheel and scrollbar
slider moved text by a fixed number of pixels, resulting in partial lines at
the top of the editor box.  The change also applies to the shell and grep
output windows, but not to read-only text views.

..

.. bpo: 33679
.. date: 2018-05-29-07-14-37
.. nonce: MgX_Ui
.. section: IDLE

Enable theme-specific color configuration for Code Context. Use the
Highlights tab to see the setting for built-in themes or add settings to
custom themes.

..

.. bpo: 33642
.. date: 2018-05-24-20-42-44
.. nonce: J0VQbS
.. section: IDLE

Display up to maxlines non-blank lines for Code Context. If there is no
current context, show a single blank line.
