.. highlight:: c

.. _capi-slots:

Definition slots
================

To define :ref:`module objects <moduleobjects>` and
:ref:`classes <creating-heap-types>` using the C API, you may use
an array of *slots* -- essentally, key-value pairs that describe features
of the object to create.
This decouples the data from the structures used at runtime, allowing CPython
-- and other Python C API implementations -- to update the stuctures without
breaking backwards compatibility.

This section documents slots in general.
For object-specific behavior and slot values, see documentation for functions
that apply slots:

- :c:func:`PyType_FromSlots` for types;
- :c:func:`PyModule_FromSlotsAndSpec` and :ref:`extension-export-hook`
  for modules.

When slots are passed to a function that applies them, the function will
not modify the slot array, nor any data it points to (recursively).
After the function is done, the caller is allowed to modify or deallocate
the array and any data it points to (recursively), except data
explicitly marked with :c:macro:`PySlot_STATIC`.

Except when documented otherwise, multiple slots with the same ID
(:c:member:`~PySlot.sl_id`) may not occur in a single slots array.

.. versionadded:: next

   Slot arrays generalize an earlier way of defining objects:
   using :c:type:`PyType_Spec` with :c:type:`PyType_Slot` for types, and
   :c:type:`PyModuleDef` with :c:type:`PyModuleDef_Slot` for modules.
   The earlier API is :term:`soft deprecated`; there are no plans to remove it.

Entries of the slots array use the following structure:

.. c:type:: PySlot

   An entry in a slots array. Defined as:

   .. code-block:: c

    typedef struct {
        uint16_t sl_id;
        uint16_t sl_flags;
        uint32_t _reserved;  // must be 0
        union {
            void *sl_ptr;
            void (*sl_func)(void);
            Py_ssize_t sl_size;
            int64_t sl_int64;
            uint64_t sl_uint64;
        };
    } PySlot;

   .. c:member:: uint16_t sl_id

      A slot ID, chosen from:

      - ``Py_slot_*`` values documented in :ref:`pyslot-common-ids` below;
      - ``Py_mod_*`` values for modules, as documented in :ref:`c_module_slots`;
      - Values for types, as documented in :ref:`pyslot_type_slot_ids`.

      A :c:member:`!sl_id` of zero (:c:macro:`Py_slot_end`) marks the end of a
      slots array.

   .. c:member:: void *sl_ptr
                 void (*sl_func)(void)
                 Py_ssize_t sl_size
                 int64_t sl_int64
                 uint64_t sl_uint64

      The data for the slot.
      These members are part of an anonymous union;
      the member to use depends on which data type is required by the slot ID:
      data pointer, function pointer, size, signed or unsigned
      integer, respectively.

      Except when documented otherwise for a specific slot ID, pointers
      (that is :c:member:`!sl_ptr` and :c:member:`!sl_func`) may not be NULL.

   .. c:member:: uint16_t sl_flags

      Zero or more of the following flags, OR-ed together:

      .. c:namespace:: NULL

      .. c:macro:: PySlot_STATIC

         All data the slot points to is statically allocated and constant.
         Thus, the interpreter does not need to copy the information.

         This flag is implied for function pointers.

         The flag applies even to data the slot points to “indirectly”,
         except for slots nested via :c:macro:`Py_slot_subslots` which may
         have their own :c:macro:`!PySlot_STATIC` flags.
         For example, if applied to a :c:macro:`Py_tp_members` slot that
         points to an array of :c:type:`PyMemberDef` structures,
         then the entire array, as well as the name and doc strings
         in its elements, must be static and constant.

      .. c:macro:: PySlot_INTPTR

         The data is stored in ``sl_ptr``; CPython will cast it to
         the appropriate type.

         This flag can simplify porting from the older :c:type:`PyType_Slot`
         and :c:type:`PyModuleDef_Slot` structures.

      .. c:macro:: PySlot_OPTIONAL

         If the slot ID is unknown, the interpreter should ignore the
         slot, rather than fail.

         For example, if Python 3.16 adds a new feature with a new slot ID,attr
         the corresponding slot may be marked :c:macro:`!PySlot_OPTIONAL`
         so that Python 3.15 ignores it.

         Note that the "optionality" only applies to unknown slot IDs.
         This flag does not make Python skip invalid values of known slots.

   .. versionadded:: next


Convenience macros
------------------

.. c:macro:: PySlot_DATA(name, value)
             PySlot_FUNC(name, value)
             PySlot_SIZE(name, value)
             PySlot_INT64(name, value)
             PySlot_UINT64(name, value)
             PySlot_STATIC_DATA(name, value)

   Convenience macros to define :c:type:`!PySlot` structures with
   :c:member:`~PySlot.sl_id` and a particular union member set.

   :c:macro:`!PySlot_STATIC_DATA` sets the :c:macro:`PySlot_STATIC` flag;
   others set no flags.

   Note that these macros use *designated initializers*, a C language feature
   that C++ added in the 2020 version of the standard.
   If your code needs to be compatible with C++11 or older,
   use :c:macro:`PySlot_PTR` instead.

   Defined as::

      #define PySlot_DATA(NAME, VALUE) \
         {.sl_id=NAME, .sl_ptr=(void*)(VALUE)}

      #define PySlot_FUNC(NAME, VALUE) \
         {.sl_id=NAME, .sl_func=(VALUE)}

      #define PySlot_SIZE(NAME, VALUE) \
         {.sl_id=NAME, .sl_size=(VALUE)}

      #define PySlot_INT64(NAME, VALUE) \
         {.sl_id=NAME, .sl_int64=(VALUE)}

      #define PySlot_UINT64(NAME, VALUE) \
         {.sl_id=NAME, .sl_uint64=(VALUE)}

      #define PySlot_STATIC_DATA(NAME, VALUE) \
         {.sl_id=NAME, .sl_flags=PySlot_STATIC, .sl_ptr=(VALUE)}

   .. versionadded:: next

.. c:macro:: PySlot_END

   Convenience macros to mark the end of a :c:type:`!PySlot` array.

   Defined as::

      #define PySlot_END {0}

   .. versionadded:: next

.. c:macro:: PySlot_PTR(name, value)
             PySlot_PTR_STATIC(name, value)

   Convenience macros for use in C++11-compatible code.
   This version of C++ does not allow setting arbitrary union members in
   literals; instead, these macros set the :c:macro:`PySlot_INTPTR` flag and cast
   the value to ``(void*)``.

   Defined as::

      #define PySlot_PTR(NAME, VALUE) \
         {NAME, PySlot_INTPTR, {0}, {(void*)(VALUE)}}

      #define PySlot_PTR_STATIC(NAME, VALUE) \
         {NAME, PySlot_INTPTR|Py_SLOT_STATIC, {0}, {(void*)(VALUE)}}

   .. versionadded:: next

.. _pyslot-common-ids:

Common slot IDs
---------------

The following slot IDs may be used in both type and module definitions.

.. c:macro:: Py_slot_end

   Marks the end of a slots array.
   Defined as zero.

   .. versionadded:: next

.. c:macro:: Py_slot_subslots

   Nested slots array.

   The value (:c:member:`~PySlot.sl_ptr`) should point to an array of
   :c:type:`PySlot` structures.
   The slots in the array (up to but not including the zero-ID
   terminator) will be treated as if they were inserted if the current
   slot array, at the point :c:macro:`!Py_slot_subslots` appears.

   Slot nesting depth is limited to 5 levels.
   This restriction may be lifted in the future.

   .. versionadded:: next

.. c:macro:: Py_slot_invalid

   Reserved; will always be treated as an unknown slot ID.
   Defined as ``UINT16_MAX`` (``0xFFFF``).

   When used with the :c:macro:`PySlot_OPTIONAL` flag, defines a slot with
   no effect.
   Without the flag, processing a slot with this ID will fail.

   .. versionadded:: next
