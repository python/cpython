.. _devmode:

Python Development Mode
=======================

.. versionadded:: 3.7

The Python Development Mode introduces additional runtime checks that are too
expensive to be enabled by default. It should not be more verbose than the
default if the code is correct; new warnings are only emitted when an issue is
detected.

It can be enabled using the :option:`-X dev <-X>` command line option or by
setting the :envvar:`PYTHONDEVMODE` environment variable to ``1``.

Effects of the Python Development Mode
======================================

Enabling the Python Development Mode is similar to the following command, but
with additional effects described below::

    PYTHONMALLOC=debug PYTHONASYNCIODEBUG=1 python3 -W default -X faulthandler

Effects of the Python Development Mode:

* Add ``default`` :ref:`warning filter <describing-warning-filters>`. The
  following warnings are shown:

  * :exc:`DeprecationWarning`
  * :exc:`ImportWarning`
  * :exc:`PendingDeprecationWarning`
  * :exc:`ResourceWarning`

  Normally, the above warnings are filtered by the default :ref:`warning
  filters <describing-warning-filters>`.

  It behaves as if the :option:`-W default <-W>` command line option is used.

  Use the :option:`-W error <-W>` command line option or set the
  :envvar:`PYTHONWARNINGS` environment variable to ``error`` to treat warnings
  as errors.

* Install debug hooks on memory allocators to check for:

  * Buffer underflow
  * Buffer overflow
  * Memory allocator API violation
  * Unsafe usage of the GIL

  See the :c:func:`PyMem_SetupDebugHooks` C function.

  It behaves as if the :envvar:`PYTHONMALLOC` environment variable is set to
  ``debug``.

  To enable the Python Development Mode without installing debug hooks on
  memory allocators, set the :envvar:`PYTHONMALLOC` environment variable to
  ``default``.

* Call :func:`faulthandler.enable` at Python startup to install handlers for
  the :const:`SIGSEGV`, :const:`SIGFPE`, :const:`SIGABRT`, :const:`SIGBUS` and
  :const:`SIGILL` signals to dump the Python traceback on a crash.

  It behaves as if the :option:`-X faulthandler <-X>` command line option is
  used or if the :envvar:`PYTHONFAULTHANDLER` environment variable is set to
  ``1``.

* Enable :ref:`asyncio debug mode <asyncio-debug-mode>`. For example,
  :mod:`asyncio` checks for coroutines that were not awaited and logs them.

  It behaves as if the :envvar:`PYTHONASYNCIODEBUG` environment variable is set
  to ``1``.

* Check the *encoding* and *errors* arguments for string encoding and decoding
  operations. Examples: :func:`open`, :meth:`str.encode` and
  :meth:`bytes.decode`.

  By default, for best performance, the *errors* argument is only checked at
  the first encoding/decoding error and the *encoding* argument is sometimes
  ignored for empty strings.

* The :class:`io.IOBase` destructor logs ``close()`` exceptions.
* Set the :attr:`~sys.flags.dev_mode` attribute of :attr:`sys.flags` to
  ``True``.

The Python Development Mode does not enable the :mod:`tracemalloc` module by
default, because the overhead cost (to performance and memory) would be too
large. Enabling the :mod:`tracemalloc` module provides additional information
on the origin of some errors. For example, :exc:`ResourceWarning` logs the
traceback where the resource was allocated, and a buffer overflow error logs
the traceback where the memory block was allocated.

The Python Development Mode does not prevent the :option:`-O` command line
option from removing :keyword:`assert` statements nor from setting
:const:`__debug__` to ``False``.

The Python Development Mode can only be enabled at the Python startup. Its
value can be read from :data:`sys.flags.dev_mode <sys.flags>`.

.. versionchanged:: 3.8
   The :class:`io.IOBase` destructor now logs ``close()`` exceptions.

.. versionchanged:: 3.9
   The *encoding* and *errors* arguments are now checked for string encoding
   and decoding operations.


ResourceWarning Example
=======================

Example of a script counting the number of lines of the text file specified in
the command line::

    import sys

    def main():
        fp = open(sys.argv[1])
        nlines = len(fp.readlines())
        print(nlines)
        # The file is closed implicitly

    if __name__ == "__main__":
        main()

The script does not close the file explicitly. By default, Python does not emit
any warning. Example using README.txt, which has 269 lines:

.. code-block:: shell-session

    $ python3 script.py README.txt
    269

Enabling the Python Development Mode displays a :exc:`ResourceWarning` warning:

.. code-block:: shell-session

    $ python3 -X dev script.py README.txt
    269
    script.py:10: ResourceWarning: unclosed file <_io.TextIOWrapper name='README.rst' mode='r' encoding='UTF-8'>
      main()
    ResourceWarning: Enable tracemalloc to get the object allocation traceback

In addition, enabling :mod:`tracemalloc` shows the line where the file was
opened:

.. code-block:: shell-session

    $ python3 -X dev -X tracemalloc=5 script.py README.rst
    269
    script.py:10: ResourceWarning: unclosed file <_io.TextIOWrapper name='README.rst' mode='r' encoding='UTF-8'>
      main()
    Object allocated at (most recent call last):
      File "script.py", lineno 10
        main()
      File "script.py", lineno 4
        fp = open(sys.argv[1])

The fix is to close explicitly the file. Example using a context manager::

    def main():
        # Close the file explicitly when exiting the with block
        with open(sys.argv[1]) as fp:
            nlines = len(fp.readlines())
        print(nlines)

Not closing a resource explicitly can leave a resource open for way longer than
expected; it can cause severe issues upon exiting Python. It is bad in
CPython, but it is even worse in PyPy. Closing resources explicitly makes an
application more deterministic and more reliable.


Bad file descriptor error example
=================================

Script displaying the first line of itself::

    import os

    def main():
        fp = open(__file__)
        firstline = fp.readline()
        print(firstline.rstrip())
        os.close(fp.fileno())
        # The file is closed implicitly

    main()

By default, Python does not emit any warning:

.. code-block:: shell-session

    $ python3 script.py
    import os

The Python Development Mode shows a :exc:`ResourceWarning` and logs a "Bad file
descriptor" error when finalizing the file object:

.. code-block:: shell-session

    $ python3 script.py
    import os
    script.py:10: ResourceWarning: unclosed file <_io.TextIOWrapper name='script.py' mode='r' encoding='UTF-8'>
      main()
    ResourceWarning: Enable tracemalloc to get the object allocation traceback
    Exception ignored in: <_io.TextIOWrapper name='script.py' mode='r' encoding='UTF-8'>
    Traceback (most recent call last):
      File "script.py", line 10, in <module>
        main()
    OSError: [Errno 9] Bad file descriptor

``os.close(fp.fileno())`` closes the file descriptor. When the file object
finalizer tries to close the file descriptor again, it fails with the ``Bad
file descriptor`` error. A file descriptor must be closed only once. In the
worst case scenario, closing it twice can lead to a crash (see :issue:`18748`
for an example).

The fix is to remove the ``os.close(fp.fileno())`` line, or open the file with
``closefd=False``.
