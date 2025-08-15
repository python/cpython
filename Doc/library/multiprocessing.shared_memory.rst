:mod:`!multiprocessing.shared_memory` --- Shared memory for direct access across processes
==========================================================================================

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
on a multicore or symmetric multiprocessor (SMP) machine.  To assist with
the life-cycle management of shared memory especially across distinct
processes, a :class:`~multiprocessing.managers.BaseManager` subclass,
:class:`~multiprocessing.managers.SharedMemoryManager`, is also provided in the
:mod:`multiprocessing.managers` module.

In this module, shared memory refers to "POSIX style" shared memory blocks
(though is not necessarily implemented explicitly as such) and does not refer
to "distributed shared memory".  This style of shared memory permits distinct
processes to potentially read and write to a common (or shared) region of
volatile memory.  Processes are conventionally limited to only have access to
their own process memory space but shared memory permits the sharing
of data between processes, avoiding the need to instead send messages between
processes containing that data.  Sharing data directly via memory can provide
significant performance benefits compared to sharing data via disk or socket
or other communications requiring the serialization/deserialization and
copying of data.


.. class:: SharedMemory(name=None, create=False, size=0, *, track=True)

   Create an instance of the :class:`!SharedMemory` class for either
   creating a new shared memory block or attaching to an existing shared
   memory block.  Each shared memory block is assigned a unique name.
   In this way, one process can create a shared memory block with a
   particular name and a different process can attach to that same shared
   memory block using that same name.

   As a resource for sharing data across processes, shared memory blocks
   may outlive the original process that created them.  When one process
   no longer needs access to a shared memory block that might still be
   needed by other processes, the :meth:`close` method should be called.
   When a shared memory block is no longer needed by any process, the
   :meth:`unlink` method should be called to ensure proper cleanup.

   :param name:
      The unique name for the requested shared memory, specified as a string.
      When creating a new shared memory block, if ``None`` (the default)
      is supplied for the name, a novel name will be generated.
   :type name: str | None

   :param bool create:
      Control whether a new shared memory block is created (``True``)
      or an existing shared memory block is attached (``False``).

   :param int size:
      The requested number of bytes when creating a new shared memory block.
      Because some platforms choose to allocate chunks of memory
      based upon that platform's memory page size, the exact size of the shared
      memory block may be larger or equal to the size requested.
      When attaching to an existing shared memory block,
      the *size* parameter is ignored.

   :param bool track:
      When ``True``, register the shared memory block with a resource
      tracker process on platforms where the OS does not do this automatically.
      The resource tracker ensures proper cleanup of the shared memory even
      if all other processes with access to the memory exit without doing so.
      Python processes created from a common ancestor using :mod:`multiprocessing`
      facilities share a single resource tracker process, and the lifetime of
      shared memory segments is handled automatically among these processes.
      Python processes created in any other way will receive their own
      resource tracker when accessing shared memory with *track* enabled.
      This will cause the shared memory to be deleted by the resource tracker
      of the first process that terminates.
      To avoid this issue, users of :mod:`subprocess` or standalone Python
      processes should set *track* to ``False`` when there is already another
      process in place that does the bookkeeping.
      *track* is ignored on Windows, which has its own tracking and
      automatically deletes shared memory when all handles to it have been closed.

   .. versionchanged:: 3.13
      Added the *track* parameter.

   .. method:: close()

      Close the file descriptor/handle to the shared memory from this
      instance.  :meth:`close` should be called once access to the shared
      memory block from this instance is no longer needed.  Depending
      on operating system, the underlying memory may or may not be freed
      even if all handles to it have been closed.  To ensure proper cleanup,
      use the :meth:`unlink` method.

   .. method:: unlink()

      Delete the underlying shared memory block.  This should be called only
      once per shared memory block regardless of the number of handles to it,
      even in other processes.
      :meth:`unlink` and :meth:`close` can be called in any order, but
      trying to access data inside a shared memory block after :meth:`unlink`
      may result in memory access errors, depending on platform.

      This method has no effect on Windows, where the only way to delete a
      shared memory block is to close all handles.

   .. attribute:: buf

      A memoryview of contents of the shared memory block.

   .. attribute:: name

      Read-only access to the unique name of the shared memory block.

   .. attribute:: size

      Read-only access to size in bytes of the shared memory block.


The following example demonstrates low-level use of :class:`SharedMemory`
instances::

   >>> from multiprocessing import shared_memory
   >>> shm_a = shared_memory.SharedMemory(create=True, size=10)
   >>> type(shm_a.buf)
   <class 'memoryview'>
   >>> buffer = shm_a.buf
   >>> len(buffer)
   10
   >>> buffer[:4] = bytearray([22, 33, 44, 55])  # Modify multiple at once
   >>> buffer[4] = 100                           # Modify single byte at a time
   >>> # Attach to an existing shared memory block
   >>> shm_b = shared_memory.SharedMemory(shm_a.name)
   >>> import array
   >>> array.array('b', shm_b.buf[:5])  # Copy the data into a new array.array
   array('b', [22, 33, 44, 55, 100])
   >>> shm_b.buf[:5] = b'howdy'  # Modify via shm_b using bytes
   >>> bytes(shm_a.buf[:5])      # Access via shm_a
   b'howdy'
   >>> shm_b.close()   # Close each SharedMemory instance
   >>> shm_a.close()
   >>> shm_a.unlink()  # Call unlink only once to release the shared memory



The following example demonstrates a practical use of the :class:`SharedMemory`
class with `NumPy arrays <https://numpy.org/>`_, accessing the
same :class:`!numpy.ndarray` from two distinct Python shells:

.. doctest::
   :options: +SKIP

   >>> # In the first Python interactive shell
   >>> import numpy as np
   >>> a = np.array([1, 1, 2, 3, 5, 8])  # Start with an existing NumPy array
   >>> from multiprocessing import shared_memory
   >>> shm = shared_memory.SharedMemory(create=True, size=a.nbytes)
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
   >>> import numpy as np
   >>> from multiprocessing import shared_memory
   >>> # Attach to the existing shared memory block
   >>> existing_shm = shared_memory.SharedMemory(name='psm_21467_46075')
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


.. class:: SharedMemoryManager([address[, authkey]])
   :module: multiprocessing.managers

   A subclass of :class:`multiprocessing.managers.BaseManager` which can be
   used for the management of shared memory blocks across processes.

   A call to :meth:`~multiprocessing.managers.BaseManager.start` on a
   :class:`!SharedMemoryManager` instance causes a new process to be started.
   This new process's sole purpose is to manage the life cycle
   of all shared memory blocks created through it.  To trigger the release
   of all shared memory blocks managed by that process, call
   :meth:`~multiprocessing.managers.BaseManager.shutdown` on the instance.
   This triggers a :meth:`~multiprocessing.shared_memory.SharedMemory.unlink` call
   on all of the :class:`SharedMemory` objects managed by that process and then
   stops the process itself.  By creating :class:`!SharedMemory` instances
   through a :class:`!SharedMemoryManager`, we avoid the need to manually track
   and trigger the freeing of shared memory resources.

   This class provides methods for creating and returning :class:`SharedMemory`
   instances and for creating a list-like object (:class:`ShareableList`)
   backed by shared memory.

   Refer to :class:`~multiprocessing.managers.BaseManager` for a description
   of the inherited *address* and *authkey* optional input arguments and how
   they may be used to connect to an existing :class:`!SharedMemoryManager` service
   from other processes.

   .. method:: SharedMemory(size)

      Create and return a new :class:`SharedMemory` object with the
      specified *size* in bytes.

   .. method:: ShareableList(sequence)

      Create and return a new :class:`ShareableList` object, initialized
      by the values from the input *sequence*.


The following example demonstrates the basic mechanisms of a
:class:`~multiprocessing.managers.SharedMemoryManager`:

.. doctest::
   :options: +SKIP

   >>> from multiprocessing.managers import SharedMemoryManager
   >>> smm = SharedMemoryManager()
   >>> smm.start()  # Start the process that manages the shared memory blocks
   >>> sl = smm.ShareableList(range(4))
   >>> sl
   ShareableList([0, 1, 2, 3], name='psm_6572_7512')
   >>> raw_shm = smm.SharedMemory(size=128)
   >>> another_sl = smm.ShareableList('alpha')
   >>> another_sl
   ShareableList(['a', 'l', 'p', 'h', 'a'], name='psm_6572_12221')
   >>> smm.shutdown()  # Calls unlink() on sl, raw_shm, and another_sl

The following example depicts a potentially more convenient pattern for using
:class:`~multiprocessing.managers.SharedMemoryManager` objects via the
:keyword:`with` statement to ensure that all shared memory blocks are released
after they are no longer needed:

.. doctest::
   :options: +SKIP

   >>> with SharedMemoryManager() as smm:
   ...     sl = smm.ShareableList(range(2000))
   ...     # Divide the work among two processes, storing partial results in sl
   ...     p1 = Process(target=do_work, args=(sl, 0, 1000))
   ...     p2 = Process(target=do_work, args=(sl, 1000, 2000))
   ...     p1.start()
   ...     p2.start()  # A multiprocessing.Pool might be more efficient
   ...     p1.join()
   ...     p2.join()   # Wait for all work to complete in both processes
   ...     total_result = sum(sl)  # Consolidate the partial results now in sl

When using a :class:`~multiprocessing.managers.SharedMemoryManager`
in a :keyword:`with` statement, the shared memory blocks created using that
manager are all released when the :keyword:`!with` statement's code block
finishes execution.


.. class:: ShareableList(sequence=None, *, name=None)

   Provide a mutable list-like object where all values stored within are
   stored in a shared memory block.
   This constrains storable values to the following built-in data types:

   * :class:`int` (signed 64-bit)
   * :class:`float`
   * :class:`bool`
   * :class:`str` (less than 10M bytes each when encoded as UTF-8)
   * :class:`bytes` (less than 10M bytes each)
   * ``None``

   It also notably differs from the built-in :class:`list` type
   in that these lists can not change their overall length
   (i.e. no :meth:`!append`, :meth:`!insert`, etc.) and do not
   support the dynamic creation of new :class:`!ShareableList` instances
   via slicing.

   *sequence* is used in populating a new :class:`!ShareableList` full of values.
   Set to ``None`` to instead attach to an already existing
   :class:`!ShareableList` by its unique shared memory name.

   *name* is the unique name for the requested shared memory, as described
   in the definition for :class:`SharedMemory`.  When attaching to an
   existing :class:`!ShareableList`, specify its shared memory block's unique
   name while leaving *sequence* set to ``None``.

   .. note::

      A known issue exists for :class:`bytes` and :class:`str` values.
      If they end with ``\x00`` nul bytes or characters, those may be
      *silently stripped* when fetching them by index from the
      :class:`!ShareableList`. This ``.rstrip(b'\x00')`` behavior is
      considered a bug and may go away in the future. See :gh:`106939`.

   For applications where rstripping of trailing nulls is a problem,
   work around it by always unconditionally appending an extra non-0
   byte to the end of such values when storing and unconditionally
   removing it when fetching:

   .. doctest::

       >>> from multiprocessing import shared_memory
       >>> nul_bug_demo = shared_memory.ShareableList(['?\x00', b'\x03\x02\x01\x00\x00\x00'])
       >>> nul_bug_demo[0]
       '?'
       >>> nul_bug_demo[1]
       b'\x03\x02\x01'
       >>> nul_bug_demo.shm.unlink()
       >>> padded = shared_memory.ShareableList(['?\x00\x07', b'\x03\x02\x01\x00\x00\x00\x07'])
       >>> padded[0][:-1]
       '?\x00'
       >>> padded[1][:-1]
       b'\x03\x02\x01\x00\x00\x00'
       >>> padded.shm.unlink()

   .. method:: count(value)

      Return the number of occurrences of *value*.

   .. method:: index(value)

      Return first index position of *value*.
      Raise :exc:`ValueError` if *value* is not present.

   .. attribute:: format

      Read-only attribute containing the :mod:`struct` packing format used by
      all currently stored values.

   .. attribute:: shm

      The :class:`SharedMemory` instance where the values are stored.


The following example demonstrates basic use of a :class:`ShareableList`
instance:

   >>> from multiprocessing import shared_memory
   >>> a = shared_memory.ShareableList(['howdy', b'HoWdY', -273.154, 100, None, True, 42])
   >>> [ type(entry) for entry in a ]
   [<class 'str'>, <class 'bytes'>, <class 'float'>, <class 'int'>, <class 'NoneType'>, <class 'bool'>, <class 'int'>]
   >>> a[2]
   -273.154
   >>> a[2] = -78.5
   >>> a[2]
   -78.5
   >>> a[2] = 'dry ice'  # Changing data types is supported as well
   >>> a[2]
   'dry ice'
   >>> a[2] = 'larger than previously allocated storage space'
   Traceback (most recent call last):
     ...
   ValueError: exceeds available storage for existing str
   >>> a[2]
   'dry ice'
   >>> len(a)
   7
   >>> a.index(42)
   6
   >>> a.count(b'howdy')
   0
   >>> a.count(b'HoWdY')
   1
   >>> a.shm.close()
   >>> a.shm.unlink()
   >>> del a  # Use of a ShareableList after call to unlink() is unsupported

The following example depicts how one, two, or many processes may access the
same :class:`ShareableList` by supplying the name of the shared memory block
behind it:

   >>> b = shared_memory.ShareableList(range(5))         # In a first process
   >>> c = shared_memory.ShareableList(name=b.shm.name)  # In a second process
   >>> c
   ShareableList([0, 1, 2, 3, 4], name='...')
   >>> c[-1] = -999
   >>> b[-1]
   -999
   >>> b.shm.close()
   >>> c.shm.close()
   >>> c.shm.unlink()

The following examples demonstrates that :class:`ShareableList`
(and underlying :class:`SharedMemory`) objects
can be pickled and unpickled if needed.
Note, that it will still be the same shared object.
This happens, because the deserialized object has
the same unique name and is just attached to an existing
object with the same name (if the object is still alive):

   >>> import pickle
   >>> from multiprocessing import shared_memory
   >>> sl = shared_memory.ShareableList(range(10))
   >>> list(sl)
   [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

   >>> deserialized_sl = pickle.loads(pickle.dumps(sl))
   >>> list(deserialized_sl)
   [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

   >>> sl[0] = -1
   >>> deserialized_sl[1] = -2
   >>> list(sl)
   [-1, -2, 2, 3, 4, 5, 6, 7, 8, 9]
   >>> list(deserialized_sl)
   [-1, -2, 2, 3, 4, 5, 6, 7, 8, 9]

   >>> sl.shm.close()
   >>> sl.shm.unlink()
