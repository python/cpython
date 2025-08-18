:mod:`!msvcrt` --- Useful routines from the MS VC++ runtime
===========================================================

.. module:: msvcrt
   :platform: Windows
   :synopsis: Miscellaneous useful routines from the MS VC++ runtime.

.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

--------------

These functions provide access to some useful capabilities on Windows platforms.
Some higher-level modules use these functions to build the Windows
implementations of their services. For example, the :mod:`getpass` module uses
this in the implementation of the :func:`getpass` function.

Further documentation on these functions can be found in the Platform API
documentation.

The module implements both the normal and wide char variants of the console I/O
api. The normal API deals only with ASCII characters and is of limited use
for internationalized applications. The wide char API should be used where
ever possible.

.. versionchanged:: 3.3
   Operations in this module now raise :exc:`OSError` where :exc:`IOError`
   was raised.


.. _msvcrt-files:

File Operations
---------------


.. function:: locking(fd, mode, nbytes)

   Lock part of a file based on file descriptor *fd* from the C runtime. Raises
   :exc:`OSError` on failure. The locked region of the file extends from the
   current file position for *nbytes* bytes, and may continue beyond the end of the
   file. *mode* must be one of the :const:`!LK_\*` constants listed below. Multiple
   regions in a file may be locked at the same time, but may not overlap. Adjacent
   regions are not merged; they must be unlocked individually.

   .. audit-event:: msvcrt.locking fd,mode,nbytes msvcrt.locking


.. data:: LK_LOCK
          LK_RLCK

   Locks the specified bytes. If the bytes cannot be locked, the program
   immediately tries again after 1 second. If, after 10 attempts, the bytes cannot
   be locked, :exc:`OSError` is raised.


.. data:: LK_NBLCK
          LK_NBRLCK

   Locks the specified bytes. If the bytes cannot be locked, :exc:`OSError` is
   raised.


.. data:: LK_UNLCK

   Unlocks the specified bytes, which must have been previously locked.


.. function:: setmode(fd, flags)

   Set the line-end translation mode for the file descriptor *fd*. To set it to
   text mode, *flags* should be :const:`os.O_TEXT`; for binary, it should be
   :const:`os.O_BINARY`.


.. function:: open_osfhandle(handle, flags)

   Create a C runtime file descriptor from the file handle *handle*. The *flags*
   parameter should be a bitwise OR of :const:`os.O_APPEND`,
   :const:`os.O_RDONLY`, :const:`os.O_TEXT` and :const:`os.O_NOINHERIT`.
   The returned file descriptor may be used as a parameter
   to :func:`os.fdopen` to create a file object.

   The file descriptor is inheritable by default. Pass :const:`os.O_NOINHERIT`
   flag to make it non inheritable.

   .. audit-event:: msvcrt.open_osfhandle handle,flags msvcrt.open_osfhandle


.. function:: get_osfhandle(fd)

   Return the file handle for the file descriptor *fd*. Raises :exc:`OSError` if
   *fd* is not recognized.

   .. audit-event:: msvcrt.get_osfhandle fd msvcrt.get_osfhandle


.. _msvcrt-console:

Console I/O
-----------


.. function:: kbhit()

   Returns a nonzero value if a keypress is waiting to be read. Otherwise,
   return 0.


.. function:: getch()

   Read a keypress and return the resulting character as a byte string.
   Nothing is echoed to the console. This call will block if a keypress
   is not already available, but will not wait for :kbd:`Enter` to be
   pressed. If the pressed key was a special function key, this will
   return ``'\000'`` or ``'\xe0'``; the next call will return the keycode.
   The :kbd:`Control-C` keypress cannot be read with this function.


.. function:: getwch()

   Wide char variant of :func:`getch`, returning a Unicode value.


.. function:: getche()

   Similar to :func:`getch`, but the keypress will be echoed if it represents a
   printable character.


.. function:: getwche()

   Wide char variant of :func:`getche`, returning a Unicode value.


.. function:: putch(char)

   Print the byte string *char* to the console without buffering.


.. function:: putwch(unicode_char)

   Wide char variant of :func:`putch`, accepting a Unicode value.


.. function:: ungetch(char)

   Cause the byte string *char* to be "pushed back" into the console buffer;
   it will be the next character read by :func:`getch` or :func:`getche`.


.. function:: ungetwch(unicode_char)

   Wide char variant of :func:`ungetch`, accepting a Unicode value.


.. _msvcrt-other:

Other Functions
---------------


.. function:: heapmin()

   Force the :c:func:`malloc` heap to clean itself up and return unused blocks to
   the operating system. On failure, this raises :exc:`OSError`.


.. function:: set_error_mode(mode)

   Changes the location where the C runtime writes an error message for an error
   that might end the program. *mode* must be one of the :const:`!OUT_\*`
   constants listed below  or :const:`REPORT_ERRMODE`. Returns the old setting
   or -1 if an error occurs. Only available in
   :ref:`debug build of Python <debug-build>`.


.. data:: OUT_TO_DEFAULT

   Error sink is determined by the app's type. Only available in
   :ref:`debug build of Python <debug-build>`.


.. data:: OUT_TO_STDERR

   Error sink is a standard error. Only available in
   :ref:`debug build of Python <debug-build>`.


.. data:: OUT_TO_MSGBOX

   Error sink is a message box. Only available in
   :ref:`debug build of Python <debug-build>`.


.. data:: REPORT_ERRMODE

   Report the current error mode value. Only available in
   :ref:`debug build of Python <debug-build>`.


.. function:: CrtSetReportMode(type, mode)

   Specifies the destination or destinations for a specific report type
   generated by :c:func:`!_CrtDbgReport` in the MS VC++ runtime. *type* must be
   one of the :const:`!CRT_\*` constants listed below. *mode* must be one of the
   :const:`!CRTDBG_\*` constants listed below. Only available in
   :ref:`debug build of Python <debug-build>`.


.. function:: CrtSetReportFile(type, file)

   After you use :func:`CrtSetReportMode` to specify :const:`CRTDBG_MODE_FILE`,
   you can specify the file handle to receive the message text. *type* must be
   one of the :const:`!CRT_\*` constants listed below. *file* should be the file
   handle your want specified. Only available in
   :ref:`debug build of Python <debug-build>`.


.. data:: CRT_WARN

   Warnings, messages, and information that doesn't need immediate attention.


.. data:: CRT_ERROR

   Errors, unrecoverable problems, and issues that require immediate attention.


.. data:: CRT_ASSERT

   Assertion failures.


.. data:: CRTDBG_MODE_DEBUG

   Writes the message to the debugger's output window.


.. data:: CRTDBG_MODE_FILE

   Writes the message to a user-supplied file handle. :func:`CrtSetReportFile`
   should be called to define the specific file or stream to use as
   the destination.


.. data:: CRTDBG_MODE_WNDW

   Creates a message box to display the message along with the ``Abort``,
   ``Retry``, and ``Ignore`` buttons.


.. data:: CRTDBG_REPORT_MODE

   Returns current *mode* for the specified *type*.


.. data:: CRT_ASSEMBLY_VERSION

   The CRT Assembly version, from the :file:`crtassem.h` header file.


.. data:: VC_ASSEMBLY_PUBLICKEYTOKEN

   The VC Assembly public key token, from the :file:`crtassem.h` header file.


.. data:: LIBRARIES_ASSEMBLY_NAME_PREFIX

   The Libraries Assembly name prefix, from the :file:`crtassem.h` header file.
