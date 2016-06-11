:mod:`posix` --- The most common POSIX system calls
===================================================

.. module:: posix
   :platform: Unix
   :synopsis: The most common POSIX system calls (normally used via module os).

--------------

This module provides access to operating system functionality that is
standardized by the C Standard and the POSIX standard (a thinly disguised Unix
interface).

.. index:: module: os

**Do not import this module directly.**  Instead, import the module :mod:`os`,
which provides a *portable* version of this interface.  On Unix, the :mod:`os`
module provides a superset of the :mod:`posix` interface.  On non-Unix operating
systems the :mod:`posix` module is not available, but a subset is always
available through the :mod:`os` interface.  Once :mod:`os` is imported, there is
*no* performance penalty in using it instead of :mod:`posix`.  In addition,
:mod:`os` provides some additional functionality, such as automatically calling
:func:`~os.putenv` when an entry in ``os.environ`` is changed.

Errors are reported as exceptions; the usual exceptions are given for type
errors, while errors reported by the system calls raise :exc:`OSError`.


.. _posix-large-files:

Large File Support
------------------

.. index::
   single: large files
   single: file; large files

.. sectionauthor:: Steve Clift <clift@mail.anacapa.net>

Several operating systems (including AIX, HP-UX, Irix and Solaris) provide
support for files that are larger than 2 GiB from a C programming model where
:c:type:`int` and :c:type:`long` are 32-bit values. This is typically accomplished
by defining the relevant size and offset types as 64-bit values. Such files are
sometimes referred to as :dfn:`large files`.

Large file support is enabled in Python when the size of an :c:type:`off_t` is
larger than a :c:type:`long` and the :c:type:`long long` type is available and is
at least as large as an :c:type:`off_t`.
It may be necessary to configure and compile Python with certain compiler flags
to enable this mode. For example, it is enabled by default with recent versions
of Irix, but with Solaris 2.6 and 2.7 you need to do something like::

   CFLAGS="`getconf LFS_CFLAGS`" OPT="-g -O2 $CFLAGS" \
           ./configure

On large-file-capable Linux systems, this might work::

   CFLAGS='-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64' OPT="-g -O2 $CFLAGS" \
           ./configure


.. _posix-contents:

Notable Module Contents
-----------------------

In addition to many functions described in the :mod:`os` module documentation,
:mod:`posix` defines the following data item:

.. data:: environ

   A dictionary representing the string environment at the time the interpreter
   was started. Keys and values are bytes on Unix and str on Windows. For
   example, ``environ[b'HOME']`` (``environ['HOME']`` on Windows) is the
   pathname of your home directory, equivalent to ``getenv("HOME")`` in C.

   Modifying this dictionary does not affect the string environment passed on by
   :func:`~os.execv`, :func:`~os.popen` or :func:`~os.system`; if you need to
   change the environment, pass ``environ`` to :func:`~os.execve` or add
   variable assignments and export statements to the command string for
   :func:`~os.system` or :func:`~os.popen`.

   .. versionchanged:: 3.2
      On Unix, keys and values are bytes.

   .. note::

      The :mod:`os` module provides an alternate implementation of ``environ``
      which updates the environment on modification. Note also that updating
      :data:`os.environ` will render this dictionary obsolete. Use of the
      :mod:`os` module version of this is recommended over direct access to the
      :mod:`posix` module.
