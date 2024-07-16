.. highlight:: c

.. _dictobjects:

Dictionary Objects
------------------

.. index:: pair: object; dictionary


.. c:type:: PyDictObject

   This subtype of :c:type:`PyObject` represents a Python dictionary object.


.. c:var:: PyTypeObject PyDict_Type

   This instance of :c:type:`PyTypeObject` represents the Python dictionary
   type.  This is the same object as :class:`dict` in the Python layer.


.. c:function:: int PyDict_Check(PyObject *p)

   Return true if *p* is a dict object or an instance of a subtype of the dict
   type.  This function always succeeds.


.. c:function:: int PyDict_CheckExact(PyObject *p)

   Return true if *p* is a dict object, but not an instance of a subtype of
   the dict type.  This function always succeeds.


.. c:function:: PyObject* PyDict_New()

   Return a new empty dictionary, or ``NULL`` on failure.


.. c:function:: PyObject* PyDictProxy_New(PyObject *mapping)

   Return a :class:`types.MappingProxyType` object for a mapping which
   enforces read-only behavior.  This is normally used to create a view to
   prevent modification of the dictionary for non-dynamic class types.


.. c:function:: void PyDict_Clear(PyObject *p)

   Empty an existing dictionary of all key-value pairs.


.. c:function:: int PyDict_Contains(PyObject *p, PyObject *key)

   Determine if dictionary *p* contains *key*.  If an item in *p* is matches
   *key*, return ``1``, otherwise return ``0``.  On error, return ``-1``.
   This is equivalent to the Python expression ``key in p``.


.. c:function:: int PyDict_ContainsString(PyObject *p, const char *key)

   This is the same as :c:func:`PyDict_Contains`, but *key* is specified as a
   :c:expr:`const char*` UTF-8 encoded bytes string, rather than a
   :c:expr:`PyObject*`.

   .. versionadded:: 3.13


.. c:function:: PyObject* PyDict_Copy(PyObject *p)

   Return a new dictionary that contains the same key-value pairs as *p*.


.. c:function:: int PyDict_SetItem(PyObject *p, PyObject *key, PyObject *val)

   Insert *val* into the dictionary *p* with a key of *key*.  *key* must be
   :term:`hashable`; if it isn't, :exc:`TypeError` will be raised. Return
   ``0`` on success or ``-1`` on failure.  This function *does not* steal a
   reference to *val*.


.. c:function:: int PyDict_SetItemString(PyObject *p, const char *key, PyObject *val)

   This is the same as :c:func:`PyDict_SetItem`, but *key* is
   specified as a :c:expr:`const char*` UTF-8 encoded bytes string,
   rather than a :c:expr:`PyObject*`.


.. c:function:: int PyDict_DelItem(PyObject *p, PyObject *key)

   Remove the entry in dictionary *p* with key *key*. *key* must be :term:`hashable`;
   if it isn't, :exc:`TypeError` is raised.
   If *key* is not in the dictionary, :exc:`KeyError` is raised.
   Return ``0`` on success or ``-1`` on failure.


.. c:function:: int PyDict_DelItemString(PyObject *p, const char *key)

   This is the same as :c:func:`PyDict_DelItem`, but *key* is
   specified as a :c:expr:`const char*` UTF-8 encoded bytes string,
   rather than a :c:expr:`PyObject*`.


.. c:function:: int PyDict_GetItemRef(PyObject *p, PyObject *key, PyObject **result)

   Return a new :term:`strong reference` to the object from dictionary *p*
   which has a key *key*:

   * If the key is present, set *\*result* to a new :term:`strong reference`
     to the value and return ``1``.
   * If the key is missing, set *\*result* to ``NULL`` and return ``0``.
   * On error, raise an exception and return ``-1``.

   .. versionadded:: 3.13

   See also the :c:func:`PyObject_GetItem` function.


.. c:function:: PyObject* PyDict_GetItem(PyObject *p, PyObject *key)

   Return a :term:`borrowed reference` to the object from dictionary *p* which
   has a key *key*.  Return ``NULL`` if the key *key* is missing *without*
   setting an exception.

   .. note::

      Exceptions that occur while this calls :meth:`~object.__hash__` and
      :meth:`~object.__eq__` methods are silently ignored.
      Prefer the :c:func:`PyDict_GetItemWithError` function instead.

   .. versionchanged:: 3.10
      Calling this API without :term:`GIL` held had been allowed for historical
      reason. It is no longer allowed.


.. c:function:: PyObject* PyDict_GetItemWithError(PyObject *p, PyObject *key)

   Variant of :c:func:`PyDict_GetItem` that does not suppress
   exceptions. Return ``NULL`` **with** an exception set if an exception
   occurred.  Return ``NULL`` **without** an exception set if the key
   wasn't present.


.. c:function:: PyObject* PyDict_GetItemString(PyObject *p, const char *key)

   This is the same as :c:func:`PyDict_GetItem`, but *key* is specified as a
   :c:expr:`const char*` UTF-8 encoded bytes string, rather than a
   :c:expr:`PyObject*`.

   .. note::

      Exceptions that occur while this calls :meth:`~object.__hash__` and
      :meth:`~object.__eq__` methods or while creating the temporary :class:`str`
      object are silently ignored.
      Prefer using the :c:func:`PyDict_GetItemWithError` function with your own
      :c:func:`PyUnicode_FromString` *key* instead.


.. c:function:: int PyDict_GetItemStringRef(PyObject *p, const char *key, PyObject **result)

   Similar to :c:func:`PyDict_GetItemRef`, but *key* is specified as a
   :c:expr:`const char*` UTF-8 encoded bytes string, rather than a
   :c:expr:`PyObject*`.

   .. versionadded:: 3.13


.. c:function:: PyObject* PyDict_SetDefault(PyObject *p, PyObject *key, PyObject *defaultobj)

   This is the same as the Python-level :meth:`dict.setdefault`.  If present, it
   returns the value corresponding to *key* from the dictionary *p*.  If the key
   is not in the dict, it is inserted with value *defaultobj* and *defaultobj*
   is returned.  This function evaluates the hash function of *key* only once,
   instead of evaluating it independently for the lookup and the insertion.

   .. versionadded:: 3.4


.. c:function:: int PyDict_SetDefaultRef(PyObject *p, PyObject *key, PyObject *default_value, PyObject **result)

   Inserts *default_value* into the dictionary *p* with a key of *key* if the
   key is not already present in the dictionary. If *result* is not ``NULL``,
   then *\*result* is set to a :term:`strong reference` to either
   *default_value*, if the key was not present, or the existing value, if *key*
   was already present in the dictionary.
   Returns ``1`` if the key was present and *default_value* was not inserted,
   or ``0`` if the key was not present and *default_value* was inserted.
   On failure, returns ``-1``, sets an exception, and sets ``*result``
   to ``NULL``.

   For clarity: if you have a strong reference to *default_value* before
   calling this function, then after it returns, you hold a strong reference
   to both *default_value* and *\*result* (if it's not ``NULL``).
   These may refer to the same object: in that case you hold two separate
   references to it.

   .. versionadded:: 3.13


.. c:function:: int PyDict_Pop(PyObject *p, PyObject *key, PyObject **result)

   Remove *key* from dictionary *p* and optionally return the removed value.
   Do not raise :exc:`KeyError` if the key missing.

   - If the key is present, set *\*result* to a new reference to the removed
     value if *result* is not ``NULL``, and return ``1``.
   - If the key is missing, set *\*result* to ``NULL`` if *result* is not
     ``NULL``, and return ``0``.
   - On error, raise an exception and return ``-1``.

   Similar to :meth:`dict.pop`, but without the default value and
   not raising :exc:`KeyError` if the key missing.

   .. versionadded:: 3.13


.. c:function:: int PyDict_PopString(PyObject *p, const char *key, PyObject **result)

   Similar to :c:func:`PyDict_Pop`, but *key* is specified as a
   :c:expr:`const char*` UTF-8 encoded bytes string, rather than a
   :c:expr:`PyObject*`.

   .. versionadded:: 3.13


.. c:function:: PyObject* PyDict_Items(PyObject *p)

   Return a :c:type:`PyListObject` containing all the items from the dictionary.


.. c:function:: PyObject* PyDict_Keys(PyObject *p)

   Return a :c:type:`PyListObject` containing all the keys from the dictionary.


.. c:function:: PyObject* PyDict_Values(PyObject *p)

   Return a :c:type:`PyListObject` containing all the values from the dictionary
   *p*.


.. c:function:: Py_ssize_t PyDict_Size(PyObject *p)

   .. index:: pair: built-in function; len

   Return the number of items in the dictionary.  This is equivalent to
   ``len(p)`` on a dictionary.


.. c:function:: int PyDict_Next(PyObject *p, Py_ssize_t *ppos, PyObject **pkey, PyObject **pvalue)

   Iterate over all key-value pairs in the dictionary *p*.  The
   :c:type:`Py_ssize_t` referred to by *ppos* must be initialized to ``0``
   prior to the first call to this function to start the iteration; the
   function returns true for each pair in the dictionary, and false once all
   pairs have been reported.  The parameters *pkey* and *pvalue* should either
   point to :c:expr:`PyObject*` variables that will be filled in with each key
   and value, respectively, or may be ``NULL``.  Any references returned through
   them are borrowed.  *ppos* should not be altered during iteration. Its
   value represents offsets within the internal dictionary structure, and
   since the structure is sparse, the offsets are not consecutive.

   For example::

      PyObject *key, *value;
      Py_ssize_t pos = 0;

      while (PyDict_Next(self->dict, &pos, &key, &value)) {
          /* do something interesting with the values... */
          ...
      }

   The dictionary *p* should not be mutated during iteration.  It is safe to
   modify the values of the keys as you iterate over the dictionary, but only
   so long as the set of keys does not change.  For example::

      PyObject *key, *value;
      Py_ssize_t pos = 0;

      while (PyDict_Next(self->dict, &pos, &key, &value)) {
          long i = PyLong_AsLong(value);
          if (i == -1 && PyErr_Occurred()) {
              return -1;
          }
          PyObject *o = PyLong_FromLong(i + 1);
          if (o == NULL)
              return -1;
          if (PyDict_SetItem(self->dict, key, o) < 0) {
              Py_DECREF(o);
              return -1;
          }
          Py_DECREF(o);
      }

   The function is not thread-safe in the :term:`free-threaded <free threading>`
   build without external synchronization.  You can use
   :c:macro:`Py_BEGIN_CRITICAL_SECTION` to lock the dictionary while iterating
   over it::

      Py_BEGIN_CRITICAL_SECTION(self->dict);
      while (PyDict_Next(self->dict, &pos, &key, &value)) {
          ...
      }
      Py_END_CRITICAL_SECTION();


.. c:function:: int PyDict_Merge(PyObject *a, PyObject *b, int override)

   Iterate over mapping object *b* adding key-value pairs to dictionary *a*.
   *b* may be a dictionary, or any object supporting :c:func:`PyMapping_Keys`
   and :c:func:`PyObject_GetItem`. If *override* is true, existing pairs in *a*
   will be replaced if a matching key is found in *b*, otherwise pairs will
   only be added if there is not a matching key in *a*. Return ``0`` on
   success or ``-1`` if an exception was raised.


.. c:function:: int PyDict_Update(PyObject *a, PyObject *b)

   This is the same as ``PyDict_Merge(a, b, 1)`` in C, and is similar to
   ``a.update(b)`` in Python except that :c:func:`PyDict_Update` doesn't fall
   back to the iterating over a sequence of key value pairs if the second
   argument has no "keys" attribute.  Return ``0`` on success or ``-1`` if an
   exception was raised.


.. c:function:: int PyDict_MergeFromSeq2(PyObject *a, PyObject *seq2, int override)

   Update or merge into dictionary *a*, from the key-value pairs in *seq2*.
   *seq2* must be an iterable object producing iterable objects of length 2,
   viewed as key-value pairs.  In case of duplicate keys, the last wins if
   *override* is true, else the first wins. Return ``0`` on success or ``-1``
   if an exception was raised. Equivalent Python (except for the return
   value)::

      def PyDict_MergeFromSeq2(a, seq2, override):
          for key, value in seq2:
              if override or key not in a:
                  a[key] = value

.. c:function:: int PyDict_AddWatcher(PyDict_WatchCallback callback)

   Register *callback* as a dictionary watcher. Return a non-negative integer
   id which must be passed to future calls to :c:func:`PyDict_Watch`. In case
   of error (e.g. no more watcher IDs available), return ``-1`` and set an
   exception.

   .. versionadded:: 3.12

.. c:function:: int PyDict_ClearWatcher(int watcher_id)

   Clear watcher identified by *watcher_id* previously returned from
   :c:func:`PyDict_AddWatcher`. Return ``0`` on success, ``-1`` on error (e.g.
   if the given *watcher_id* was never registered.)

   .. versionadded:: 3.12

.. c:function:: int PyDict_Watch(int watcher_id, PyObject *dict)

   Mark dictionary *dict* as watched. The callback granted *watcher_id* by
   :c:func:`PyDict_AddWatcher` will be called when *dict* is modified or
   deallocated. Return ``0`` on success or ``-1`` on error.

   .. versionadded:: 3.12

.. c:function:: int PyDict_Unwatch(int watcher_id, PyObject *dict)

   Mark dictionary *dict* as no longer watched. The callback granted
   *watcher_id* by :c:func:`PyDict_AddWatcher` will no longer be called when
   *dict* is modified or deallocated. The dict must previously have been
   watched by this watcher. Return ``0`` on success or ``-1`` on error.

   .. versionadded:: 3.12

.. c:type:: PyDict_WatchEvent

   Enumeration of possible dictionary watcher events: ``PyDict_EVENT_ADDED``,
   ``PyDict_EVENT_MODIFIED``, ``PyDict_EVENT_DELETED``, ``PyDict_EVENT_CLONED``,
   ``PyDict_EVENT_CLEARED``, or ``PyDict_EVENT_DEALLOCATED``.

   .. versionadded:: 3.12

.. c:type:: int (*PyDict_WatchCallback)(PyDict_WatchEvent event, PyObject *dict, PyObject *key, PyObject *new_value)

   Type of a dict watcher callback function.

   If *event* is ``PyDict_EVENT_CLEARED`` or ``PyDict_EVENT_DEALLOCATED``, both
   *key* and *new_value* will be ``NULL``. If *event* is ``PyDict_EVENT_ADDED``
   or ``PyDict_EVENT_MODIFIED``, *new_value* will be the new value for *key*.
   If *event* is ``PyDict_EVENT_DELETED``, *key* is being deleted from the
   dictionary and *new_value* will be ``NULL``.

   ``PyDict_EVENT_CLONED`` occurs when *dict* was previously empty and another
   dict is merged into it. To maintain efficiency of this operation, per-key
   ``PyDict_EVENT_ADDED`` events are not issued in this case; instead a
   single ``PyDict_EVENT_CLONED`` is issued, and *key* will be the source
   dictionary.

   The callback may inspect but must not modify *dict*; doing so could have
   unpredictable effects, including infinite recursion. Do not trigger Python
   code execution in the callback, as it could modify the dict as a side effect.

   If *event* is ``PyDict_EVENT_DEALLOCATED``, taking a new reference in the
   callback to the about-to-be-destroyed dictionary will resurrect it and
   prevent it from being freed at this time. When the resurrected object is
   destroyed later, any watcher callbacks active at that time will be called
   again.

   Callbacks occur before the notified modification to *dict* takes place, so
   the prior state of *dict* can be inspected.

   If the callback sets an exception, it must return ``-1``; this exception will
   be printed as an unraisable exception using :c:func:`PyErr_WriteUnraisable`.
   Otherwise it should return ``0``.

   There may already be a pending exception set on entry to the callback. In
   this case, the callback should return ``0`` with the same exception still
   set. This means the callback may not call any other API that can set an
   exception unless it saves and clears the exception state first, and restores
   it before returning.

   .. versionadded:: 3.12
