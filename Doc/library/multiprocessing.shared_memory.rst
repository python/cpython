:mod:`multiprocessing.shared_memory` ---  Provides shared memory for direct access across processes
===================================================================================================

.. module:: multiprocessing.shared_memory
   :synopsis: Provides shared memory for direct access across processes.

**Source code:** :source:`Lib/multiprocessing/shared_memory.py`

.. versionadded:: 3.8

.. index::
   single: Shared Memory
   single: POSIX Shared Memory
   single: Named Shared Memory

--------------

This module provides a class, :class:`SharedMemory`, for the allocation
and management of shared memory to be accessed by one or more processes
on a multicore or SMP machine.  To assist with the life-cycle management
of shared memory across distinct processes, a
:func:`multiprocessing.Manager` class, :class:`SharedMemoryManager`, is
also provided.

In this module, shared memory refers to "System V style" shared memory blocks
(though is not necessarily implemented explicitly as such).  This style
of memory, when created, permits distinct processes to potentially read and
write to a common (or shared) region of volatile memory.  Because processes
conventionally only have access to their own process memory space, this
construct of shared memory permits the sharing of data between processes,
avoiding the need to instead send messages between processes containing
that data.  Sharing data directly via memory can provide significant
performance benefits compared to sharing data via disk or socket or other
communications requiring the serialization/de-serialization and copying of
data.


.. class:: SharedMemory(name, flags=None, mode=384, size=0, read_only=False)

   This class creates and returns an instance of either a
   :class:`PosixSharedMemory` or :class:`NamedSharedMemory` class depending
   upon their availability on the local system.

   *name* is the unique name for the requested shared memory, specified as
   a string.  If ``None`` is supplied for the name, a new shared memory
   block with a novel name will be created without needing to also specify
   ``flags``.

   *flags* is set to ``None`` when attempting to attach to an existing shared
   memory block by its unique name but if no existing block has that name, an
   exception will be raised.  To request the creation of a new shared
   memory block, set to ``O_CREX``.  To request the optional creation of a
   new shared memory block or attach to an existing one by the same name,
   set to ``O_CREAT``.

   *mode* controls user/group/all-based read/write permissions on the
   shared memory block.  Its specification is not enforceable on all platforms.

   *size* specifies the number of bytes requested for a shared memory block.
   Because some platforms choose to allocate chunks of memory based upon
   that platform's memory page size, the exact size of the shared memory
   block may be larger or equal to the size requested.  When attaching to an
   existing shared memory block, set to ``0`` (which is the default).

   *read_only* controls whether a shared memory block is to be available
   for only reading or for both reading and writing.  Its specification is
   not enforceable on all platforms.

   .. method:: close()

      Closes access to the shared memory from this instance.  In order to
      ensure proper cleanup of resources, all instances should call
      ``close()`` once the instance is no longer needed.  Note that calling
      ``close()`` does not cause the shared memory block itself to be
      destroyed.

   .. method:: unlink()

      Requests that the underlying shared memory block be destroyed.  In
      order to ensure proper cleanup of resources, ``unlink()`` should be
      called once (and only once) across all processes which have need
      for the shared memory block.  After requesting its destruction, a
      shared memory block may or may not be immediately destroyed and
      this behavior may differ across platforms.  Attempts to access data
      inside the shared memory block after ``unlink()`` has been called may
      result in memory access errors.  Note: the last process relinquishing
      its hold on a shared memory block may call ``unlink()`` and
      ``close()`` in either order.

   .. attribute:: buf

      A memoryview of contents of the shared memory block.

   .. attribute:: name

      Read-only access to the unique name of the shared memory block.

   .. attribute:: mode

      Read-only access to access permissions mode of the shared memory block.

   .. attribute:: size

      Read-only access to size in bytes of the shared memory block.


The following example demonstrates low-level use of :class:`SharedMemory`
instances::

   >>> from multiprocessing import shared_memory
   >>> shm_a = shared_memory.SharedMemory(None, size=10)
   >>> type(shm_b.buf)
   <class 'memoryview'>
   >>> buffer = shm_a.buf
   >>> len(buffer)
   10
   >>> buffer[:4] = bytearray([22, 33, 44, 55])  # Modify multiple at once
   >>> buffer[4] = 100  # Modify a single byte at a time
   >>> # Attach to an existing shared memory block
   >>> shm_b = shared_memory.SharedMemory(shm_a.name)
   >>> import array
   >>> array.array('b', shm_b.buf[:5])  # Copy the data into a new array.array
   array('b', [22, 33, 44, 55, 100])
   >>> shm_b.buf[:5] = b'howdy'  # Modify via shm_b using bytes
   >>> bytes(shm_a.buf[:5])  # Access via shm_a
   b'howdy'
   >>> shm_b.close()  # Close each SharedMemory instance
   >>> shm_a.close()
   >>> shm_a.unlink()  # Call unlink only once to release the shared memory



The following example demonstrates a practical use of the :class:`SharedMemory`
class with ``numpy`` arrays, accessing the same ``numpy.ndarray`` from
two distinct Python shells::

   >>> # In the first Python interactive shell
   >>> import numpy as np
   >>> a = np.array([1, 1, 2, 3, 5, 8])  # Start with an existing NumPy array
   >>> from multiprocessing import shared_memory
   >>> shm = shared_memory.SharedMemory(None, size=a.nbytes)
   >>> # Now create a NumPy array backed by shared memory
   >>> b = np.ndarray(a.shape, dtype=a.dtype, buffer=shm.buf)
   >>> b[:] = a[:]  # Copy the original data into shared memory
   >>> b
   array([1, 1, 2, 3, 5, 8])
   >>> type(b)
   <class 'numpy.ndarray'>
   >>> type(a)
   <class 'numpy.ndarray'>
   >>> shm.name  # We did not specify a name so one was chosen for us
   'psm_21467_46075'

   >>> # In either the same shell or a new Python shell on the same machine
   >>> # Attach to the existing shared memory block
   >>> existing_shm = shared_memory.SharedMemory('psm_21467_46075')
   >>> # Note that a.shape is (6,) and a.dtype is np.int64 in this example
   >>> c = np.ndarray((6,), dtype=np.int64, buffer=existing_shm.buf)
   >>> c
   array([1, 1, 2, 3, 5, 8])
   >>> c[-1] = 888
   >>> c
   array([  1,   1,   2,   3,   5, 888])

   >>> # Back in the first Python interactive shell, b reflects this change
   >>> b
   array([  1,   1,   2,   3,   5, 888])

   >>> # Clean up from within the second Python shell
   >>> del c  # Unnecessary; merely emphasizing the array is no longer used
   >>> existing_shm.close()

   >>> # Clean up from within the first Python shell
   >>> del b  # Unnecessary; merely emphasizing the array is no longer used
   >>> shm.close()
   >>> shm.unlink()  # Free and release the shared memory block at the very end


.. class:: SharedMemoryManager

   A subclass of :class:`multiprocessing.managers.SyncManager` which can be
   used for the management of shared memory blocks across processes.

   It provides methods for creating and returning a :class:`SharedMemory`
   instance and for creating a list-like object (:class:`ShareableList`)
   backed by shared memory.

