.. highlight:: c

.. _thread-local-storage:

Thread-local storage support
============================

The Python interpreter provides low-level support for thread-local storage
(TLS) which wraps the underlying native TLS implementation to support the
Python-level thread-local storage API (:class:`threading.local`).  The
CPython C level APIs are similar to those offered by pthreads and Windows:
use a thread key and functions to associate a :c:expr:`void*` value per
thread.

A :term:`thread state` does *not* need to be :term:`attached <attached thread state>`
when calling these functions; they supply their own locking.

Note that :file:`Python.h` does not include the declaration of the TLS APIs,
you need to include :file:`pythread.h` to use thread-local storage.

.. note::
   None of these API functions handle memory management on behalf of the
   :c:expr:`void*` values.  You need to allocate and deallocate them yourself.
   If the :c:expr:`void*` values happen to be :c:expr:`PyObject*`, these
   functions don't do refcount operations on them either.

.. _thread-specific-storage-api:

Thread-specific storage API
---------------------------

The thread-specific storage (TSS) API was introduced to supersede the use of the existing TLS API within the
CPython interpreter.  This API uses a new type :c:type:`Py_tss_t` instead of
:c:expr:`int` to represent thread keys.

.. versionadded:: 3.7

.. seealso:: "A New C-API for Thread-Local Storage in CPython" (:pep:`539`)


.. c:type:: Py_tss_t

   This data structure represents the state of a thread key, the definition of
   which may depend on the underlying TLS implementation, and it has an
   internal field representing the key's initialization state.  There are no
   public members in this structure.

   When :ref:`Py_LIMITED_API <stable>` is not defined, static allocation of
   this type by :c:macro:`Py_tss_NEEDS_INIT` is allowed.


.. c:macro:: Py_tss_NEEDS_INIT

   This macro expands to the initializer for :c:type:`Py_tss_t` variables.
   Note that this macro won't be defined with :ref:`Py_LIMITED_API <stable>`.


Dynamic allocation
------------------

Dynamic allocation of the :c:type:`Py_tss_t`, required in extension modules
built with :ref:`Py_LIMITED_API <stable>`, where static allocation of this type
is not possible due to its implementation being opaque at build time.


.. c:function:: Py_tss_t* PyThread_tss_alloc()

   Return a value which is the same state as a value initialized with
   :c:macro:`Py_tss_NEEDS_INIT`, or ``NULL`` in the case of dynamic allocation
   failure.


.. c:function:: void PyThread_tss_free(Py_tss_t *key)

   Free the given *key* allocated by :c:func:`PyThread_tss_alloc`, after
   first calling :c:func:`PyThread_tss_delete` to ensure any associated
   thread locals have been unassigned. This is a no-op if the *key*
   argument is ``NULL``.

   .. note::
      A freed key becomes a dangling pointer. You should reset the key to
      ``NULL``.


Methods
-------

The parameter *key* of these functions must not be ``NULL``.  Moreover, the
behaviors of :c:func:`PyThread_tss_set` and :c:func:`PyThread_tss_get` are
undefined if the given :c:type:`Py_tss_t` has not been initialized by
:c:func:`PyThread_tss_create`.


.. c:function:: int PyThread_tss_is_created(Py_tss_t *key)

   Return a non-zero value if the given :c:type:`Py_tss_t` has been initialized
   by :c:func:`PyThread_tss_create`.


.. c:function:: int PyThread_tss_create(Py_tss_t *key)

   Return a zero value on successful initialization of a TSS key.  The behavior
   is undefined if the value pointed to by the *key* argument is not
   initialized by :c:macro:`Py_tss_NEEDS_INIT`.  This function can be called
   repeatedly on the same key -- calling it on an already initialized key is a
   no-op and immediately returns success.


.. c:function:: void PyThread_tss_delete(Py_tss_t *key)

   Destroy a TSS key to forget the values associated with the key across all
   threads, and change the key's initialization state to uninitialized.  A
   destroyed key is able to be initialized again by
   :c:func:`PyThread_tss_create`. This function can be called repeatedly on
   the same key -- calling it on an already destroyed key is a no-op.


.. c:function:: int PyThread_tss_set(Py_tss_t *key, void *value)

   Return a zero value to indicate successfully associating a :c:expr:`void*`
   value with a TSS key in the current thread.  Each thread has a distinct
   mapping of the key to a :c:expr:`void*` value.


.. c:function:: void* PyThread_tss_get(Py_tss_t *key)

   Return the :c:expr:`void*` value associated with a TSS key in the current
   thread.  This returns ``NULL`` if no value is associated with the key in the
   current thread.


.. _thread-local-storage-api:

Legacy APIs
-----------

.. deprecated:: 3.7
   This API is superseded by the
   :ref:`thread-specific storage (TSS) API <thread-specific-storage-api>`.

.. note::
   This version of the API does not support platforms where the native TLS key
   is defined in a way that cannot be safely cast to ``int``.  On such platforms,
   :c:func:`PyThread_create_key` will return immediately with a failure status,
   and the other TLS functions will all be no-ops on such platforms.

Due to the compatibility problem noted above, this version of the API should not
be used in new code.

.. c:function:: int PyThread_create_key()
.. c:function:: void PyThread_delete_key(int key)
.. c:function:: int PyThread_set_key_value(int key, void *value)
.. c:function:: void* PyThread_get_key_value(int key)
.. c:function:: void PyThread_delete_key_value(int key)
.. c:function:: void PyThread_ReInitTLS()
