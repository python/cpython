:mod:`!winsound` --- Sound-playing interface for Windows
========================================================

.. module:: winsound
   :platform: Windows
   :synopsis: Access to the sound-playing machinery for Windows.

.. moduleauthor:: Toby Dickenson <htrd90@zepler.org>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

**Source code:** :source:`PC/winsound.c`

--------------

The :mod:`!winsound` module provides access to the basic sound-playing machinery
provided by Windows platforms.  It includes functions and several constants.

.. availability:: Windows.


Functions
---------

.. function:: Beep(frequency, duration)

   Beep the PC's speaker. The *frequency* parameter specifies frequency, in hertz,
   of the sound, and must be in the range 37 through 32,767. The *duration*
   parameter specifies the number of milliseconds the sound should last.  If the
   system is not able to beep the speaker, :exc:`RuntimeError` is raised.


.. function:: PlaySound(sound, flags)

   Call the underlying :c:func:`!PlaySound` function from the Platform API.  The
   *sound* parameter may be a filename, a system sound alias, audio data as a
   :term:`bytes-like object`, or ``None``.  Its
   interpretation depends on the value of *flags*, which can be a bitwise ORed
   combination of the constants described below. If the *sound* parameter is
   ``None``, any currently playing waveform sound is stopped. If the system
   indicates an error, :exc:`RuntimeError` is raised.


.. function:: MessageBeep(type=MB_OK)

   Call the underlying :c:func:`!MessageBeep` function from the Platform API.  This
   plays a sound as specified in the registry.  The *type* argument specifies which
   sound to play; possible values are ``-1``, ``MB_ICONASTERISK``,
   ``MB_ICONEXCLAMATION``, ``MB_ICONHAND``, ``MB_ICONQUESTION``, and ``MB_OK``, all
   described below.  The value ``-1`` produces a "simple beep"; this is the final
   fallback if a sound cannot be played otherwise.  If the system indicates an
   error, :exc:`RuntimeError` is raised.


Constants
---------

.. data:: SND_FILENAME

   The *sound* parameter is the name of a WAV file. Do not use with
   :const:`SND_ALIAS`.


.. data:: SND_ALIAS

   The *sound* parameter is a sound association name from the registry.  If the
   registry contains no such name, play the system default sound unless
   :const:`SND_NODEFAULT` is also specified. If no default sound is registered,
   raise :exc:`RuntimeError`. Do not use with :const:`SND_FILENAME`.

   All Win32 systems support at least the following; most systems support many
   more:

   +--------------------------+----------------------------------------+
   | :func:`PlaySound` *name* | Corresponding Control Panel Sound name |
   +==========================+========================================+
   | ``'SystemAsterisk'``     | Asterisk                               |
   +--------------------------+----------------------------------------+
   | ``'SystemExclamation'``  | Exclamation                            |
   +--------------------------+----------------------------------------+
   | ``'SystemExit'``         | Exit Windows                           |
   +--------------------------+----------------------------------------+
   | ``'SystemHand'``         | Critical Stop                          |
   +--------------------------+----------------------------------------+
   | ``'SystemQuestion'``     | Question                               |
   +--------------------------+----------------------------------------+

   For example::

      import winsound
      # Play Windows exit sound.
      winsound.PlaySound("SystemExit", winsound.SND_ALIAS)

      # Probably play Windows default sound, if any is registered (because
      # "*" probably isn't the registered name of any sound).
      winsound.PlaySound("*", winsound.SND_ALIAS)


.. data:: SND_LOOP

   Play the sound repeatedly.  The :const:`SND_ASYNC` flag must also be used to
   avoid blocking.  Cannot be used with :const:`SND_MEMORY`.


.. data:: SND_MEMORY

   The *sound* parameter to :func:`PlaySound` is a memory image of a WAV file, as a
   :term:`bytes-like object`.

   .. note::

      This module does not support playing from a memory image asynchronously, so a
      combination of this flag and :const:`SND_ASYNC` will raise :exc:`RuntimeError`.


.. data:: SND_PURGE

   Stop playing all instances of the specified sound.

   .. note::

      This flag is not supported on modern Windows platforms.


.. data:: SND_ASYNC

   Return immediately, allowing sounds to play asynchronously.


.. data:: SND_NODEFAULT

   If the specified sound cannot be found, do not play the system default sound.


.. data:: SND_NOSTOP

   Do not interrupt sounds currently playing.


.. data:: SND_NOWAIT

   Return immediately if the sound driver is busy.

   .. note::

      This flag is not supported on modern Windows platforms.


.. data:: SND_APPLICATION

   The *sound* parameter is an application-specific alias in the registry.
   This flag can be combined with the :const:`SND_ALIAS` flag
   to specify an application-defined sound alias.


.. data:: SND_SENTRY

   Triggers a SoundSentry event when the sound is played.

   .. versionadded:: 3.14


.. data:: SND_SYNC

   The sound is played synchronously.  This is the default behavior.

   .. versionadded:: 3.14


.. data:: SND_SYSTEM

   Assign the sound to the audio session for system notification sounds.

   .. versionadded:: 3.14


.. data:: MB_ICONASTERISK

   Play the ``SystemDefault`` sound.


.. data:: MB_ICONEXCLAMATION

   Play the ``SystemExclamation`` sound.


.. data:: MB_ICONHAND

   Play the ``SystemHand`` sound.


.. data:: MB_ICONQUESTION

   Play the ``SystemQuestion`` sound.


.. data:: MB_OK

   Play the ``SystemDefault`` sound.


.. data:: MB_ICONERROR

   Play the ``SystemHand`` sound.

   .. versionadded:: 3.14


.. data:: MB_ICONINFORMATION

   Play the ``SystemDefault`` sound.

   .. versionadded:: 3.14


.. data:: MB_ICONSTOP

   Play the ``SystemHand`` sound.

   .. versionadded:: 3.14


.. data:: MB_ICONWARNING

   Play the ``SystemExclamation`` sound.

   .. versionadded:: 3.14
