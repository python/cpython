.. bpo: 24305
.. date: 9302
.. nonce: QeF4A8
.. release date: 2015-09-07
.. section: Core and Builtins

Prevent import subsystem stack frames from being counted by the
warnings.warn(stacklevel=) parameter.

..

.. bpo: 24912
.. date: 9301
.. nonce: ubSi5J
.. section: Core and Builtins

Prevent __class__ assignment to immutable built-in objects.

..

.. bpo: 24975
.. date: 9300
.. nonce: 2gLdfN
.. section: Core and Builtins

Fix AST compilation for :pep:`448` syntax.

..

.. bpo: 24917
.. date: 9299
.. nonce: xaQocz
.. section: Library

time_strftime() buffer over-read.

..

.. bpo: 24748
.. date: 9298
.. nonce: 83NuO8
.. section: Library

To resolve a compatibility problem found with py2exe and pywin32,
imp.load_dynamic() once again ignores previously loaded modules to support
Python modules replacing themselves with extension modules. Patch by Petr
Viktorin.

..

.. bpo: 24635
.. date: 9297
.. nonce: EiJPPf
.. section: Library

Fixed a bug in typing.py where isinstance([], typing.Iterable) would return
True once, then False on subsequent calls.

..

.. bpo: 24989
.. date: 9296
.. nonce: 9BJLiy
.. section: Library

Fixed buffer overread in BytesIO.readline() if a position is set beyond
size.  Based on patch by John Leitch.

..

.. bpo: 24913
.. date: 9295
.. nonce: p2ZAJ4
.. section: Library

Fix overrun error in deque.index(). Found by John Leitch and Bryce Darling.
