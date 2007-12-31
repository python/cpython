
:mod:`select` --- Waiting for I/O completion
============================================

.. module:: select
   :synopsis: Wait for I/O completion on multiple streams.


This module provides access to the :cfunc:`select` and :cfunc:`poll` functions
available in most operating systems.  Note that on Windows, it only works for
sockets; on other operating systems, it also works for other file types (in
particular, on Unix, it works on pipes).  It cannot be used on regular files to
determine whether a file has grown since it was last read.

The module defines the following:


.. exception:: error

   The exception raised when an error occurs.  The accompanying value is a pair
   containing the numeric error code from :cdata:`errno` and the corresponding
   string, as would be printed by the C function :cfunc:`perror`.


.. function:: poll()

   (Not supported by all operating systems.)  Returns a polling object, which
   supports registering and unregistering file descriptors, and then polling them
   for I/O events; see section :ref:`poll-objects` below for the methods supported
   by polling objects.


.. function:: select(iwtd, owtd, ewtd[, timeout])

   This is a straightforward interface to the Unix :cfunc:`select` system call.
   The first three arguments are sequences of 'waitable objects': either
   integers representing file descriptors or objects with a parameterless method
   named :meth:`fileno` returning such an integer.  The three sequences of
   waitable objects are for input, output and 'exceptional conditions',
   respectively.  Empty sequences are allowed, but acceptance of three empty
   sequences is platform-dependent. (It is known to work on Unix but not on
   Windows.)  The optional *timeout* argument specifies a time-out as a floating
   point number in seconds.  When the *timeout* argument is omitted the function
   blocks until at least one file descriptor is ready.  A time-out value of zero
   specifies a poll and never blocks.

   The return value is a triple of lists of objects that are ready: subsets of the
   first three arguments.  When the time-out is reached without a file descriptor
   becoming ready, three empty lists are returned.

   .. index::
      single: socket() (in module socket)
      single: popen() (in module os)

   Among the acceptable object types in the sequences are Python file objects (e.g.
   ``sys.stdin``, or objects returned by :func:`open` or :func:`os.popen`), socket
   objects returned by :func:`socket.socket`.  You may also define a :dfn:`wrapper`
   class yourself, as long as it has an appropriate :meth:`fileno` method (that
   really returns a file descriptor, not just a random integer).

   .. note::

      .. index:: single: WinSock

      File objects on Windows are not acceptable, but sockets are.  On Windows, the
      underlying :cfunc:`select` function is provided by the WinSock library, and does
      not handle file descriptors that don't originate from WinSock.


.. _poll-objects:

Polling Objects
---------------

The :cfunc:`poll` system call, supported on most Unix systems, provides better
scalability for network servers that service many, many clients at the same
time. :cfunc:`poll` scales better because the system call only requires listing
the file descriptors of interest, while :cfunc:`select` builds a bitmap, turns
on bits for the fds of interest, and then afterward the whole bitmap has to be
linearly scanned again. :cfunc:`select` is O(highest file descriptor), while
:cfunc:`poll` is O(number of file descriptors).


.. method:: poll.register(fd[, eventmask])

   Register a file descriptor with the polling object.  Future calls to the
   :meth:`poll` method will then check whether the file descriptor has any pending
   I/O events.  *fd* can be either an integer, or an object with a :meth:`fileno`
   method that returns an integer.  File objects implement :meth:`fileno`, so they
   can also be used as the argument.

   *eventmask* is an optional bitmask describing the type of events you want to
   check for, and can be a combination of the constants :const:`POLLIN`,
   :const:`POLLPRI`, and :const:`POLLOUT`, described in the table below.  If not
   specified, the default value used will check for all 3 types of events.

   +-------------------+------------------------------------------+
   | Constant          | Meaning                                  |
   +===================+==========================================+
   | :const:`POLLIN`   | There is data to read                    |
   +-------------------+------------------------------------------+
   | :const:`POLLPRI`  | There is urgent data to read             |
   +-------------------+------------------------------------------+
   | :const:`POLLOUT`  | Ready for output: writing will not block |
   +-------------------+------------------------------------------+
   | :const:`POLLERR`  | Error condition of some sort             |
   +-------------------+------------------------------------------+
   | :const:`POLLHUP`  | Hung up                                  |
   +-------------------+------------------------------------------+
   | :const:`POLLNVAL` | Invalid request: descriptor not open     |
   +-------------------+------------------------------------------+

   Registering a file descriptor that's already registered is not an error, and has
   the same effect as registering the descriptor exactly once.


.. method:: poll.unregister(fd)

   Remove a file descriptor being tracked by a polling object.  Just like the
   :meth:`register` method, *fd* can be an integer or an object with a
   :meth:`fileno` method that returns an integer.

   Attempting to remove a file descriptor that was never registered causes a
   :exc:`KeyError` exception to be raised.


.. method:: poll.poll([timeout])

   Polls the set of registered file descriptors, and returns a possibly-empty list
   containing ``(fd, event)`` 2-tuples for the descriptors that have events or
   errors to report. *fd* is the file descriptor, and *event* is a bitmask with
   bits set for the reported events for that descriptor --- :const:`POLLIN` for
   waiting input, :const:`POLLOUT` to indicate that the descriptor can be written
   to, and so forth. An empty list indicates that the call timed out and no file
   descriptors had any events to report. If *timeout* is given, it specifies the
   length of time in milliseconds which the system will wait for events before
   returning. If *timeout* is omitted, negative, or :const:`None`, the call will
   block until there is an event for this poll object.

