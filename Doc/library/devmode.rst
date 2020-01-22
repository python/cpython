.. _devmode:

Python Development Mode
=======================

.. versionadded:: 3.7

The Python Development Mode introduces additional runtime checks which are too
expensive to be enabled by default. It should not be more verbose than the
default if the code is correct: new warnings are only emitted when an issue is
detected.

It can be enabled using :option:`-X dev <-X>` command line option or by setting
:envvar:`PYTHONDEVMODE` environment variable to ``1``.

Effects of the developer mode
=============================

Enabling the development mode has multiple effects:

* Add ``default`` :ref:`warning filter <describing-warning-filters>`, as
  :option:`-W default <-W>` command line option. The following warnings are
  shown, whereas they are ignored by the default warning filters:

  * :exc:`DeprecationWarning`;
  * :exc:`ImportWarning`;
  * :exc:`PendingDeprecationWarning`;
  * :exc:`ResourceWarning`.

  Use :option:`-W error <-W>` command line option or set
  :envvar:`PYTHONWARNINGS` environment variable to ``error`` to treat warnings
  as errors.

* Install debug hooks on memory allocators to check for:

  * buffer underflow;
  * buffer overflow
  * memory allocator API violation;
  * usage of the GIL.

  See the :c:func:`PyMem_SetupDebugHooks` C function. Similar to setting
  :envvar:`PYTHONMALLOC` environment variable to ``debug``.

  To enable the development without installing debug hooks on memory
  allocators, set :envvar:`PYTHONMALLOC` environment variable to ``default``.

* Call :func:`faulthandler.enable` at Python startup to enable the
  :mod:`faulthandler` module: install handlers for the :const:`SIGSEGV`,
  :const:`SIGFPE`, :const:`SIGABRT`, :const:`SIGBUS` and :const:`SIGILL`
  signals to dump the Python traceback. Similar to :option:`-X faulthandler
  <-X>` command line option or setting :envvar:`PYTHONFAULTHANDLER` environment
  variable to ``1``.

* Enable :ref:`asyncio debug mode <asyncio-debug-mode>`. Similar to setting
  :envvar:`PYTHONASYNCIODEBUG` environment variable to ``1``.
* Check *encoding* and *errors* arguments on string encoding and decoding
  operations. Examples: :func:`open`, :meth:`str.encode` and
  :meth:`bytes.decode`.
* :class:`io.IOBase` destructor logs ``close()`` exceptions.
* Set the :attr:`~sys.flags.dev_mode` attribute of :attr:`sys.flags` to
  ``True``.

The development mode does not enable the :mod:`tracemalloc` module by default,
because the overhead (performance and memory) would be too important. Enabling
the :mod:`tracemalloc` module provides additional information on the origin
of some errors. For example, :exc:`ResourceWarning` logs the traceback where
the resource was allocated, and a buffer overflow error logs the traceback
where the memory block was allocated.

The development mode does not prevent the :option:`-O` command line option to
remove :keyword:`assert` statements nor to set :const:`__debug__` to ``False``.

.. versionchanged:: 3.8
   Log ``close()`` exceptions in :class:`io.IOBase` destructor.


ResourceWarning Example
=======================

Example of a script counting the number of lines of the text file specified on
the command line::

    import sys

    def main():
        fp = open(sys.argv[1])
        nlines = len(fp.readlines())
        print(nlines)

    if __name__ == "__main__":
        main()

The script does not close the file explicitly, but Python doesn't complain by
default. Example with README.txt which is made 269 lines:

.. code-block:: shell-session

    $ python3 script.py README.txt
    269

Enabling the development mode displays a :exc:`ResourceWarning` warning:

.. code-block:: shell-session

    $ python3 -X dev script.py README.txt
    269
    script.py:9: ResourceWarning: unclosed file <_io.TextIOWrapper name='README.rst' mode='r' encoding='UTF-8'>
      main()
    ResourceWarning: Enable tracemalloc to get the object allocation traceback

Enabling also :mod:`tracemalloc` shows the line where the file was opened:

.. code-block:: shell-session

    $ python3 -X dev -X tracemalloc=5 script.py README.rst
    269
    script.py:9: ResourceWarning: unclosed file <_io.TextIOWrapper name='README.rst' mode='r' encoding='UTF-8'>
      main()
    Object allocated at (most recent call last):
      File "script.py", lineno 9
        main()
      File "script.py", lineno 4
        fp = open(sys.argv[1])

The fix is to close explicitly the file. Example using a context manager::

    def main():
        with open(sys.argv[1]) as fp:
            nlines = len(fp.readlines())
        print(nlines)

Not closing a resource explicitly can leave a resource open for way longer than
expected. It can cause severe issues at Python exit. It is bad in CPython, but
it is even worse in PyPy. Closing resources explicitly makes an application
more deterministic and more reliable.


Bad file descriptor error example
=================================

Script displaying the first line of itself::

    import os

    def main():
        fp = open(__file__)
        firstline = fp.readline()
        print(firstline.rstrip())
        os.close(fp.fileno())
        # fp object is destroyed implicitly

    main()

By default, Python does not emit any warning:

.. code-block:: shell-session

    $ python3 script.py
    import os

The development mode shows a :exc:`ResourceWarning` and logs a "Bad file
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
file descriptor`` error. A file descriptor must be closed only once: closing it
twice can lead to a crash in the worst case (:issue:`18748` in a crash
example).

The fix is to remove ``os.close(fp.fileno())`` line, or open the file with
``closefd=False``.
