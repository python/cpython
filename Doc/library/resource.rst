:mod:`resource` --- Resource usage information
==============================================

.. module:: resource
   :platform: Unix
   :synopsis: An interface to provide resource usage information on the current process.

.. moduleauthor:: Jeremy Hylton <jeremy@alum.mit.edu>
.. sectionauthor:: Jeremy Hylton <jeremy@alum.mit.edu>

--------------

This module provides basic mechanisms for measuring and controlling system
resources utilized by a program.

Symbolic constants are used to specify particular system resources and to
request usage information about either the current process or its children.

An :exc:`OSError` is raised on syscall failure.


.. exception:: error

   A deprecated alias of :exc:`OSError`.

   .. versionchanged:: 3.3
      Following :pep:`3151`, this class was made an alias of :exc:`OSError`.


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


.. data:: RLIM_INFINITY

   Constant used to represent the limit for an unlimited resource.


.. function:: getrlimit(resource)

   Returns a tuple ``(soft, hard)`` with the current soft and hard limits of
   *resource*. Raises :exc:`ValueError` if an invalid resource is specified, or
   :exc:`error` if the underlying system call fails unexpectedly.


.. function:: setrlimit(resource, limits)

   Sets new limits of consumption of *resource*. The *limits* argument must be a
   tuple ``(soft, hard)`` of two integers describing the new limits. A value of
   :data:`~resource.RLIM_INFINITY` can be used to request a limit that is
   unlimited.

   Raises :exc:`ValueError` if an invalid resource is specified, if the new soft
   limit exceeds the hard limit, or if a process tries to raise its hard limit.
   Specifying a limit of :data:`~resource.RLIM_INFINITY` when the hard or
   system limit for that resource is not unlimited will result in a
   :exc:`ValueError`.  A process with the effective UID of super-user can
   request any valid limit value, including unlimited, but :exc:`ValueError`
   will still be raised if the requested limit exceeds the system imposed
   limit.

   ``setrlimit`` may also raise :exc:`error` if the underlying system call
   fails.

   VxWorks only supports setting :data:`RLIMIT_NOFILE`.

.. function:: prlimit(pid, resource[, limits])

   Combines :func:`setrlimit` and :func:`getrlimit` in one function and
   supports to get and set the resources limits of an arbitrary process. If
   *pid* is 0, then the call applies to the current process. *resource* and
   *limits* have the same meaning as in :func:`setrlimit`, except that
   *limits* is optional.

   When *limits* is not given the function returns the *resource* limit of the
   process *pid*. When *limits* is given the *resource* limit of the process is
   set and the former resource limit is returned.

   Raises :exc:`ProcessLookupError` when *pid* can't be found and
   :exc:`PermissionError` when the user doesn't have ``CAP_SYS_RESOURCE`` for
   the process.

   .. availability:: Linux 2.6.36 or later with glibc 2.13 or later.

   .. versionadded:: 3.4


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

   The maximum size of a file which the process may create.


.. data:: RLIMIT_DATA

   The maximum size (in bytes) of the process's heap.


.. data:: RLIMIT_STACK

   The maximum size (in bytes) of the call stack for the current process.  This only
   affects the stack of the main thread in a multi-threaded process.


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


.. data:: RLIMIT_MSGQUEUE

   The number of bytes that can be allocated for POSIX message queues.

   .. availability:: Linux 2.6.8 or later.

   .. versionadded:: 3.4


.. data:: RLIMIT_NICE

   The ceiling for the process's nice level (calculated as 20 - rlim_cur).

   .. availability:: Linux 2.6.12 or later.

   .. versionadded:: 3.4


.. data:: RLIMIT_RTPRIO

   The ceiling of the real-time priority.

   .. availability:: Linux 2.6.12 or later.

   .. versionadded:: 3.4


.. data:: RLIMIT_RTTIME

   The time limit (in microseconds) on CPU time that a process can spend
   under real-time scheduling without making a blocking syscall.

   .. availability:: Linux 2.6.25 or later.

   .. versionadded:: 3.4


.. data:: RLIMIT_SIGPENDING

   The number of signals which the process may queue.

   .. availability:: Linux 2.6.8 or later.

   .. versionadded:: 3.4

.. data:: RLIMIT_SBSIZE

   The maximum size (in bytes) of socket buffer usage for this user.
   This limits the amount of network memory, and hence the amount of mbufs,
   that this user may hold at any time.

   .. availability:: FreeBSD 9 or later.

   .. versionadded:: 3.4

.. data:: RLIMIT_SWAP

   The maximum size (in bytes) of the swap space that may be reserved or
   used by all of this user id's processes.
   This limit is enforced only if bit 1 of the vm.overcommit sysctl is set.
   Please see :manpage:`tuning(7)` for a complete description of this sysctl.

   .. availability:: FreeBSD 9 or later.

   .. versionadded:: 3.4

.. data:: RLIMIT_NPTS

   The maximum number of pseudo-terminals created by this user id.

   .. availability:: FreeBSD 9 or later.

   .. versionadded:: 3.4

Resource Usage
--------------

These functions are used to retrieve resource usage information:


.. function:: getrusage(who)

   This function returns an object that describes the resources consumed by either
   the current process or its children, as specified by the *who* parameter.  The
   *who* parameter should be specified using one of the :const:`RUSAGE_\*`
   constants described below.

   A simple example::

      from resource import *
      import time

      # a non CPU-bound task
      time.sleep(3)
      print(getrusage(RUSAGE_SELF))

      # a CPU-bound task
      for i in range(10 ** 8):
         _ = 1 + 1
      print(getrusage(RUSAGE_SELF))

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

   +--------+---------------------+---------------------------------------+
   | Index  | Field               | Resource                              |
   +========+=====================+=======================================+
   | ``0``  | :attr:`ru_utime`    | time in user mode (float seconds)     |
   +--------+---------------------+---------------------------------------+
   | ``1``  | :attr:`ru_stime`    | time in system mode (float seconds)   |
   +--------+---------------------+---------------------------------------+
   | ``2``  | :attr:`ru_maxrss`   | maximum resident set size             |
   +--------+---------------------+---------------------------------------+
   | ``3``  | :attr:`ru_ixrss`    | shared memory size                    |
   +--------+---------------------+---------------------------------------+
   | ``4``  | :attr:`ru_idrss`    | unshared memory size                  |
   +--------+---------------------+---------------------------------------+
   | ``5``  | :attr:`ru_isrss`    | unshared stack size                   |
   +--------+---------------------+---------------------------------------+
   | ``6``  | :attr:`ru_minflt`   | page faults not requiring I/O         |
   +--------+---------------------+---------------------------------------+
   | ``7``  | :attr:`ru_majflt`   | page faults requiring I/O             |
   +--------+---------------------+---------------------------------------+
   | ``8``  | :attr:`ru_nswap`    | number of swap outs                   |
   +--------+---------------------+---------------------------------------+
   | ``9``  | :attr:`ru_inblock`  | block input operations                |
   +--------+---------------------+---------------------------------------+
   | ``10`` | :attr:`ru_oublock`  | block output operations               |
   +--------+---------------------+---------------------------------------+
   | ``11`` | :attr:`ru_msgsnd`   | messages sent                         |
   +--------+---------------------+---------------------------------------+
   | ``12`` | :attr:`ru_msgrcv`   | messages received                     |
   +--------+---------------------+---------------------------------------+
   | ``13`` | :attr:`ru_nsignals` | signals received                      |
   +--------+---------------------+---------------------------------------+
   | ``14`` | :attr:`ru_nvcsw`    | voluntary context switches            |
   +--------+---------------------+---------------------------------------+
   | ``15`` | :attr:`ru_nivcsw`   | involuntary context switches          |
   +--------+---------------------+---------------------------------------+

   This function will raise a :exc:`ValueError` if an invalid *who* parameter is
   specified. It may also raise :exc:`error` exception in unusual circumstances.


.. function:: getpagesize()

   Returns the number of bytes in a system page. (This need not be the same as the
   hardware page size.)

The following :const:`RUSAGE_\*` symbols are passed to the :func:`getrusage`
function to specify which processes information should be provided for.


.. data:: RUSAGE_SELF

   Pass to :func:`getrusage` to request resources consumed by the calling
   process, which is the sum of resources used by all threads in the process.


.. data:: RUSAGE_CHILDREN

   Pass to :func:`getrusage` to request resources consumed by child processes
   of the calling process which have been terminated and waited for.


.. data:: RUSAGE_BOTH

   Pass to :func:`getrusage` to request resources consumed by both the current
   process and child processes.  May not be available on all systems.


.. data:: RUSAGE_THREAD

   Pass to :func:`getrusage` to request resources consumed by the current
   thread.  May not be available on all systems.

   .. versionadded:: 3.2
