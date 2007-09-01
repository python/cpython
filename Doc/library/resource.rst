
:mod:`resource` --- Resource usage information
==============================================

.. module:: resource
   :platform: Unix
   :synopsis: An interface to provide resource usage information on the current process.
.. moduleauthor:: Jeremy Hylton <jeremy@alum.mit.edu>
.. sectionauthor:: Jeremy Hylton <jeremy@alum.mit.edu>


This module provides basic mechanisms for measuring and controlling system
resources utilized by a program.

Symbolic constants are used to specify particular system resources and to
request usage information about either the current process or its children.

A single exception is defined for errors:


.. exception:: error

   The functions described below may raise this error if the underlying system call
   failures unexpectedly.


Resource Limits
---------------

Resources usage can be limited using the :func:`setrlimit` function described
below. Each resource is controlled by a pair of limits: a soft limit and a hard
limit. The soft limit is the current limit, and may be lowered or raised by a
process over time. The soft limit can never exceed the hard limit. The hard
limit can be lowered to any value greater than the soft limit, but not raised.
(Only processes with the effective UID of the super-user can raise a hard
limit.)

The specific resources that can be limited are system dependent. They are
described in the :manpage:`getrlimit(2)` man page.  The resources listed below
are supported when the underlying operating system supports them; resources
which cannot be checked or controlled by the operating system are not defined in
this module for those platforms.


.. function:: getrlimit(resource)

   Returns a tuple ``(soft, hard)`` with the current soft and hard limits of
   *resource*. Raises :exc:`ValueError` if an invalid resource is specified, or
   :exc:`error` if the underlying system call fails unexpectedly.


.. function:: setrlimit(resource, limits)

   Sets new limits of consumption of *resource*. The *limits* argument must be a
   tuple ``(soft, hard)`` of two integers describing the new limits. A value of
   ``-1`` can be used to specify the maximum possible upper limit.

   Raises :exc:`ValueError` if an invalid resource is specified, if the new soft
   limit exceeds the hard limit, or if a process tries to raise its hard limit
   (unless the process has an effective UID of super-user).  Can also raise
   :exc:`error` if the underlying system call fails.

These symbols define resources whose consumption can be controlled using the
:func:`setrlimit` and :func:`getrlimit` functions described below. The values of
these symbols are exactly the constants used by C programs.

The Unix man page for :manpage:`getrlimit(2)` lists the available resources.
Note that not all systems use the same symbol or same value to denote the same
resource.  This module does not attempt to mask platform differences --- symbols
not defined for a platform will not be available from this module on that
platform.


.. data:: RLIMIT_CORE

   The maximum size (in bytes) of a core file that the current process can create.
   This may result in the creation of a partial core file if a larger core would be
   required to contain the entire process image.


.. data:: RLIMIT_CPU

   The maximum amount of processor time (in seconds) that a process can use. If
   this limit is exceeded, a :const:`SIGXCPU` signal is sent to the process. (See
   the :mod:`signal` module documentation for information about how to catch this
   signal and do something useful, e.g. flush open files to disk.)


.. data:: RLIMIT_FSIZE

   The maximum size of a file which the process may create.  This only affects the
   stack of the main thread in a multi-threaded process.


.. data:: RLIMIT_DATA

   The maximum size (in bytes) of the process's heap.


.. data:: RLIMIT_STACK

   The maximum size (in bytes) of the call stack for the current process.


.. data:: RLIMIT_RSS

   The maximum resident set size that should be made available to the process.


.. data:: RLIMIT_NPROC

   The maximum number of processes the current process may create.


.. data:: RLIMIT_NOFILE

   The maximum number of open file descriptors for the current process.


.. data:: RLIMIT_OFILE

   The BSD name for :const:`RLIMIT_NOFILE`.


.. data:: RLIMIT_MEMLOCK

   The maximum address space which may be locked in memory.


.. data:: RLIMIT_VMEM

   The largest area of mapped memory which the process may occupy.


.. data:: RLIMIT_AS

   The maximum area (in bytes) of address space which may be taken by the process.


Resource Usage
--------------

These functions are used to retrieve resource usage information:


.. function:: getrusage(who)

   This function returns an object that describes the resources consumed by either
   the current process or its children, as specified by the *who* parameter.  The
   *who* parameter should be specified using one of the :const:`RUSAGE_\*`
   constants described below.

   The fields of the return value each describe how a particular system resource
   has been used, e.g. amount of time spent running is user mode or number of times
   the process was swapped out of main memory. Some values are dependent on the
   clock tick internal, e.g. the amount of memory the process is using.

   For backward compatibility, the return value is also accessible as a tuple of 16
   elements.

   The fields :attr:`ru_utime` and :attr:`ru_stime` of the return value are
   floating point values representing the amount of time spent executing in user
   mode and the amount of time spent executing in system mode, respectively. The
   remaining values are integers. Consult the :manpage:`getrusage(2)` man page for
   detailed information about these values. A brief summary is presented here:

   +--------+---------------------+-------------------------------+
   | Index  | Field               | Resource                      |
   +========+=====================+===============================+
   | ``0``  | :attr:`ru_utime`    | time in user mode (float)     |
   +--------+---------------------+-------------------------------+
   | ``1``  | :attr:`ru_stime`    | time in system mode (float)   |
   +--------+---------------------+-------------------------------+
   | ``2``  | :attr:`ru_maxrss`   | maximum resident set size     |
   +--------+---------------------+-------------------------------+
   | ``3``  | :attr:`ru_ixrss`    | shared memory size            |
   +--------+---------------------+-------------------------------+
   | ``4``  | :attr:`ru_idrss`    | unshared memory size          |
   +--------+---------------------+-------------------------------+
   | ``5``  | :attr:`ru_isrss`    | unshared stack size           |
   +--------+---------------------+-------------------------------+
   | ``6``  | :attr:`ru_minflt`   | page faults not requiring I/O |
   +--------+---------------------+-------------------------------+
   | ``7``  | :attr:`ru_majflt`   | page faults requiring I/O     |
   +--------+---------------------+-------------------------------+
   | ``8``  | :attr:`ru_nswap`    | number of swap outs           |
   +--------+---------------------+-------------------------------+
   | ``9``  | :attr:`ru_inblock`  | block input operations        |
   +--------+---------------------+-------------------------------+
   | ``10`` | :attr:`ru_oublock`  | block output operations       |
   +--------+---------------------+-------------------------------+
   | ``11`` | :attr:`ru_msgsnd`   | messages sent                 |
   +--------+---------------------+-------------------------------+
   | ``12`` | :attr:`ru_msgrcv`   | messages received             |
   +--------+---------------------+-------------------------------+
   | ``13`` | :attr:`ru_nsignals` | signals received              |
   +--------+---------------------+-------------------------------+
   | ``14`` | :attr:`ru_nvcsw`    | voluntary context switches    |
   +--------+---------------------+-------------------------------+
   | ``15`` | :attr:`ru_nivcsw`   | involuntary context switches  |
   +--------+---------------------+-------------------------------+

   This function will raise a :exc:`ValueError` if an invalid *who* parameter is
   specified. It may also raise :exc:`error` exception in unusual circumstances.


.. function:: getpagesize()

   Returns the number of bytes in a system page. (This need not be the same as the
   hardware page size.) This function is useful for determining the number of bytes
   of memory a process is using. The third element of the tuple returned by
   :func:`getrusage` describes memory usage in pages; multiplying by page size
   produces number of bytes.

The following :const:`RUSAGE_\*` symbols are passed to the :func:`getrusage`
function to specify which processes information should be provided for.


.. data:: RUSAGE_SELF

   :const:`RUSAGE_SELF` should be used to request information pertaining only to
   the process itself.


.. data:: RUSAGE_CHILDREN

   Pass to :func:`getrusage` to request resource information for child processes of
   the calling process.


.. data:: RUSAGE_BOTH

   Pass to :func:`getrusage` to request resources consumed by both the current
   process and child processes.  May not be available on all systems.

