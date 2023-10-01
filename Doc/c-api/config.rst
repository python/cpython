.. highlight:: c

.. _config-c-api:

********************
Python Configuration
********************

.. versionadded:: 3.13

API part of the limited C API version 3.13 to configure the Python
initialization and get the Python runtime configuration.

See also :ref:`Python Initialization Configuration <init-config>`.


Initialize Python
=================

Configuration to customize Python initialization. Configuration option names
are names of a :c:type:`PyConfig` members.

.. c:struct:: PyInitConfig

   Opaque structure to configure the Python initialization.


.. c:function:: PyInitConfig* PyInitConfig_Python_New(void)

   Create a new initialization configuration using :ref:`Python Configuration
   <init-python-config>` default values.

   It must be freed by :c:func:`PyInitConfig_Free`.

   Return ``NULL`` on memory allocation failure.


.. c:function:: PyInitConfig* PyInitConfig_Isolated_New(void)

   Similar to :c:func:`PyInitConfig_Python_New`, but use :ref:`Isolated
   Configuration <init-isolated-conf>` default values.


.. c:function:: void PyInitConfig_Free(PyInitConfig *config)

   Free memory of an initialization configuration.


.. c:function:: int PyInitConfig_SetInt(PyInitConfig *config, const char *name, int64_t value)

   Set an integer configuration option.

   * Return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.


.. c:function:: int PyInitConfig_SetStr(PyInitConfig *config, const char *name, const char *value)

   Set a string configuration option from a null-terminated bytes string.

   The bytes string is decoded by :c:func:`Py_DecodeLocale`. If Python is not
   yet preinitialized, this function preinitializes it to ensure that encodings
   are properly configured.

   * Return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.


.. c:function:: int PyInitConfig_SetWStr(PyInitConfig *config, const char *name, const wchar_t *value)

   Set a string configuration option from a null-terminated wide string.

   If Python is not yet preinitialized, this function preinitializes it.

   * Return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.


.. c:function:: int PyInitConfig_SetStrList(PyInitConfig *config, const char *name, size_t length, char * const *items)

   Set a string list configuration option from an array of null-terminated
   bytes strings.

   The bytes string is decoded by :c:func:`Py_DecodeLocale`. If Python is not
   yet preinitialized, this function preinitializes it to ensure that encodings
   are properly configured.

   * Return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.


.. c:function:: int PyInitConfig_SetWStrList(PyInitConfig *config, const char *name, size_t length, wchar_t * const *items)

   Set a string list configuration option from a null-terminated wide strings.

   If Python is not yet preinitialized, this function preinitializes it.

   * Return ``0`` on success.
   * Set an error in *config* and return ``-1`` on error.


.. c:function:: int Py_InitializeFromInitConfig(PyInitConfig *config)

   Initialize Python from the initialization configuration.

   * Return ``0`` on success.
   * Return ``-1`` if an error was set or if an exit code was set.


Error handling
==============

.. c:function:: int PyInitConfig_Exception(PyInitConfig* config)

   Check if an exception is set in *config*:

   * Return non-zero if an error was set or if an exit code was set.
   * Return zero otherwise.


.. c:function:: int PyInitConfig_GetError(PyInitConfig* config, const char **err_msg)

   Get the *config* error message.

   * Set *\*err_msg* and return ``1`` if an error is set.
   * Set *\*err_msg* to ``NULL`` and return ``0`` otherwise.


.. c:function:: int PyInitConfig_GetExitCode(PyInitConfig* config, int *exitcode)

   Get the *config* exit code.

   * Set *\*exitcode* and return ``1`` if an exit code is set.
   * Return ``0`` otherwise.


.. c:function:: void Py_ExitWithInitConfig(PyInitConfig *config)

   Exit Python and free memory of a initialization configuration.

   If an error message is set, display the error message.

   If an exit code is set, use it to exit the process.

   The function does not return.


Example
-------

Code::

    void init_python(void)
    {
        PyInitConfig *config = PyInitConfig_Python_New();
        if (config == NULL) {
            printf("Init allocation error\n");
            return;
        }

        if (PyInitConfig_SetInt(config, "dev_mode", 1) < 0) {
            goto error;
        }

        // Set a list of wide strings (argv)
        wchar_t *argv[] = {L"my_program"", L"-c", L"pass"};
        if (PyInitConfig_SetWStrList(config, "argv",
                                     Py_ARRAY_LENGTH(argv), argv) < 0) {
            goto error;
        }

        // Set a wide string (program_name)
        if (PyInitConfig_SetWStr(config, "program_name", L"my_program") < 0) {
            goto error;
        }

        // Set a list of bytes strings (xoptions)
        char* xoptions[] = {"faulthandler"};
        if (PyInitConfig_SetStrList(config, "xoptions",
                                    Py_ARRAY_LENGTH(xoptions), xoptions) < 0) {
            goto error;
        }

        if (Py_InitializeFromInitConfig(config) < 0) {
            Py_ExitWithInitConfig(config);
        }
        PyInitConfig_Free(config);
    }
