.. highlight:: c

.. _config-c-api:

********************
Python Configuration
********************

.. versionadded:: 3.14

C API to configure the Python initialization (:pep:`741`).

See also :ref:`Python Initialization Configuration <init-config>`.


Initialize Python
=================

Create Config
-------------

.. c:struct:: PyInitConfig

   Opaque structure to configure the Python initialization.


.. c:function:: PyInitConfig* PyInitConfig_Create(void)

   Create a new initialization configuration using :ref:`Isolated Configuration
   <init-isolated-conf>` default values.

   It must be freed by :c:func:`PyInitConfig_Free`.

   Return ``NULL`` on memory allocation failure.


.. c:function:: void PyInitConfig_Free(PyInitConfig *config)

   Free memory of the initialization configuration *config*.


Get Options
-----------

The configuration option *name* parameter must be a non-NULL
null-terminated UTF-8 encoded string.

.. c:function:: int PyInitConfig_HasOption(PyInitConfig *config, const char *name)

   Test if the configuration has an option called *name*.

   Return ``1`` if the option exists, or return ``0`` otherwise.


.. c:function:: int PyInitConfig_GetInt(PyInitConfig *config, const char *name, int64_t *value)

   Get an integer configuration option.

   * Set *\*value*, and return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.


.. c:function:: int PyInitConfig_GetStr(PyInitConfig *config, const char *name, char **value)

   Get a string configuration option as a null-terminated UTF-8
   encoded string.

   * Set *\*value*, and return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.

   On success, the string must be released with ``free(value)``.


.. c:function:: int PyInitConfig_GetStrList(PyInitConfig *config, const char *name, size_t *length, char ***items)

   Get a string list configuration option as an array of
   null-terminated UTF-8 encoded strings.

   * Set *\*length* and *\*value*, and return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.

   On success, the string list must be released with
   ``PyInitConfig_FreeStrList(length, items)``.


.. c:function:: void PyInitConfig_FreeStrList(size_t length, char **items)

   Free memory of a string list created by
   ``PyInitConfig_GetStrList()``.


Set Options
-----------

The configuration option *name* parameter must be a non-NULL null-terminated
UTF-8 encoded string.

Some configuration options have side effects on other options. This logic is
only implemented when ``Py_InitializeFromInitConfig()`` is called, not by the
"Set" functions below. For example, setting ``dev_mode`` to ``1`` does not set
``faulthandler`` to ``1``.

.. c:function:: int PyInitConfig_SetInt(PyInitConfig *config, const char *name, int64_t value)

   Set an integer configuration option.

   * Return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.


.. c:function:: int PyInitConfig_SetStr(PyInitConfig *config, const char *name, const char *value)

   Set a string configuration option from a null-terminated UTF-8
   encoded string. The string is copied.

   * Return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.


.. c:function:: int PyInitConfig_SetStrList(PyInitConfig *config, const char *name, size_t length, char * const *items)

   Set a string list configuration option from an array of
   null-terminated UTF-8 encoded strings. The string list is copied.

   * Return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.


Initialize Python
-----------------

.. c:function:: int Py_InitializeFromInitConfig(PyInitConfig *config)

   Initialize Python from the initialization configuration.

   * Return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.
   * Set an exit code in *config* and return ``-1`` if Python wants to
     exit.

   See ``PyInitConfig_GetExitcode()`` for the exit code case.


Error Handling
--------------

.. c:function:: int PyInitConfig_GetError(PyInitConfig* config, const char **err_msg)

   Get the *config* error message.

   * Set *\*err_msg* and return ``1`` if an error is set.
   * Set *\*err_msg* to ``NULL`` and return ``0`` otherwise.

   An error message is an UTF-8 encoded string.

   If *config* has an exit code, format the exit code as an error
   message.

   The error message remains valid until another ``PyInitConfig``
   function is called with *config*. The caller doesn't have to free the
   error message.


.. c:function:: int PyInitConfig_GetExitCode(PyInitConfig* config, int *exitcode)

   Get the *config* exit code.

   * Set *\*exitcode* and return ``1`` if Python wants to exit.
   * Return ``0`` if *config* has no exit code set.

   Only the ``Py_InitializeFromInitConfig()`` function can set an exit
   code if the ``parse_argv`` option is non-zero.

   An exit code can be set when parsing the command line failed (exit
   code ``2``) or when a command line option asks to display the command
   line help (exit code ``0``).


Example
=======

Example initializing Python, set configuration options of various types,
return ``-1`` on error:

.. code-block:: c

    int init_python(void)
    {
        PyInitConfig *config = PyInitConfig_Create();
        if (config == NULL) {
            printf("PYTHON INIT ERROR: memory allocation failed\n");
            return -1;
        }

        // Set an integer (dev mode)
        if (PyInitConfig_SetInt(config, "dev_mode", 1) < 0) {
            goto error;
        }

        // Set a list of UTF-8 strings (argv)
        char *argv[] = {"my_program", "-c", "pass"};
        if (PyInitConfig_SetStrList(config, "argv",
                                     Py_ARRAY_LENGTH(argv), argv) < 0) {
            goto error;
        }

        // Set a UTF-8 string (program name)
        if (PyInitConfig_SetStr(config, "program_name", L"my_program") < 0) {
            goto error;
        }

        // Initialize Python with the configuration
        if (Py_InitializeFromInitConfig(config) < 0) {
            goto error;
        }
        PyInitConfig_Free(config);
        return 0;

        // Display the error message
        const char *err_msg;
    error:
        (void)PyInitConfig_GetError(config, &err_msg);
        printf("PYTHON INIT ERROR: %s\n", err_msg);
        PyInitConfig_Free(config);

        return -1;
    }
