.. highlight:: c

Curses C API
------------

:mod:`curses` exposes a small C interface for extension modules.
Consumers must include the header file :file:`py_curses.h` (which is not
included by default by :file:`Python.h`) and :c:func:`import_curses` must
be invoked, usually as part of the module initialisation function, to populate
:c:var:`PyCurses_API`.

.. c:macro:: import_curses()

   Import the curses C API. The macro does not need a semi-colon to be called.

   On success, populate the :c:var:`PyCurses_API` pointer.

   On failure, set :c:var:`PyCurses_API` to NULL and set an exception.
   The caller must check if an error occurred via :c:func:`PyErr_Occurred`:

   .. code-block::

      import_curses();  // semi-colon is optional but recommended
      if (PyErr_Occurred()) { /* cleanup */ }


.. c:var:: void **PyCurses_API

   Dynamically allocated object containing the curses C API.
   This variable is only available once :c:macro:`import_curses` succeeded.

   ``PyCurses_API[0]`` corresponds to :c:data:`PyCursesWindow_Type`.

   ``PyCurses_API[1]``, ``PyCurses_API[2]``, and ``PyCurses_API[3]``
   are pointers to predicate functions of type ``int (*)(void)``.

   When called, these predicates return whether :func:`curses.setupterm`,
   :func:`curses.initscr`, and :func:`curses.start_color` have been called
   respectively.

   See also the convenience macros :c:macro:`PyCursesSetupTermCalled`,
   :c:macro:`PyCursesInitialised`, and :c:macro:`PyCursesInitialisedColor`.


.. c:data:: PyTypeObject PyCursesWindow_Type

   The :ref:`heap type <heap-types>` corresponding to :class:`curses.window`.


.. c:macro:: PyCursesWindow_Check(op)

   Return *1* if *op* is an :class:`curses.window` instance, *0* otherwise.

   The macro expansion is equivalent to:

   .. code-block::

      Py_IS_TYPE((op), &PyCursesWindow_Type)


The following macros are convenience macros expanding into C statements.
In particular, they can only be used as ``macro;`` or ``macro``, but not
``macro()`` or ``macro();``.

.. c:macro:: PyCursesSetupTermCalled

   Macro checking if :func:`curses.setupterm` has been called.

   The macro expansion is roughly equivalent to:

   .. code-block::

      {
          typedef int (*predicate_t)(void);
          predicate_t was_setupterm_called = (predicate_t)PyCurses_API[1];
          if (!was_setupterm_called()) {
              return NULL;
          }
      }


.. c:macro:: PyCursesInitialised

   Macro checking if :func:`curses.initscr` has been called.

   The macro expansion is roughly equivalent to:

   .. code-block::

      {
          typedef int (*predicate_t)(void);
          predicate_t was_initscr_called = (predicate_t)PyCurses_API[2];
          if (!was_initscr_called()) {
              return NULL;
          }
      }


.. c:macro:: PyCursesInitialisedColor

   Macro checking if :func:`curses.start_color` has been called.

   The macro expansion is roughly equivalent to:

   .. code-block::

      {
          typedef int (*predicate_t)(void);
          predicate_t was_start_color_called = (predicate_t)PyCurses_API[3];
          if (!was_start_color_called()) {
              return NULL;
          }
      }


Internal data
-------------

The following objects are exposed by the C API but should be considered
internal-only.


.. c:macro:: PyCurses_API_pointers

   The number of accessible fields in :c:var:`PyCurses_API`.

   Internal usage only.


.. c:macro:: PyCurses_CAPSULE_NAME

   Name of the curses capsule to pass to :c:func:`PyCapsule_Import`.

   Internal usage only. Use :c:macro:`import_curses` instead.

