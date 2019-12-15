.. bpo: 32551
.. date: 2018-01-16-16-05-37
.. nonce: U0z4W-
.. release date: 2018-01-23
.. section: Security

The ``sys.path[0]`` initialization change for bpo-29139 caused a regression
by revealing an inconsistency in how sys.path is initialized when executing
``__main__`` from a zipfile, directory, or other import location. This is
considered a potential security issue, as it may lead to privileged
processes unexpectedly loading code from user controlled directories in
situations where that was not previously the case.
The interpreter now consistently avoids ever adding the import location's
parent directory to ``sys.path``, and ensures no other ``sys.path`` entries
are inadvertently modified when inserting the import location named on the
command line. (Originally reported as bpo-29723 against Python 3.6rc1, but
it was missed at the time that the then upcoming Python 3.5.4 release would
also be affected)

..

.. bpo: 30657
.. date: 2017-12-01-18-51-03
.. nonce: Fd8kId
.. section: Security

Fixed possible integer overflow in PyBytes_DecodeEscape, CVE-2017-1000158.
Original patch by Jay Bosamiya; rebased to Python 3 by Miro Hrončok.

..

.. bpo: 30947
.. date: 2017-09-05-20-34-44
.. nonce: iNMmm4
.. section: Security

Upgrade libexpat embedded copy from version 2.2.1 to 2.2.3 to get security
fixes.

..

.. bpo: 31095
.. date: 2017-08-01-18-48-30
.. nonce: bXWZDb
.. section: Core and Builtins

Fix potential crash during GC caused by ``tp_dealloc`` which doesn't call
``PyObject_GC_UnTrack()``.

..

.. bpo: 32072
.. date: 2017-11-18-21-13-52
.. nonce: nwDV8L
.. section: Library

Fixed issues with binary plists:
Fixed saving bytearrays.
Identical objects will be saved only once.
Equal references will be load as identical objects.
Added support for saving and loading recursive data structures.

..

.. bpo: 31170
.. date: 2017-09-05-20-35-21
.. nonce: QGmJ1t
.. section: Library

expat: Update libexpat from 2.2.3 to 2.2.4. Fix copying of partial
characters for UTF-8 input (libexpat bug 115):
https://github.com/libexpat/libexpat/issues/115
