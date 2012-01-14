
:mod:`sunaudiodev` --- Access to Sun audio hardware
===================================================

.. module:: sunaudiodev
   :platform: SunOS
   :synopsis: Access to Sun audio hardware.
   :deprecated:

.. deprecated:: 2.6
   The :mod:`sunaudiodev` module has been deprecated for removal in Python 3.0.



.. index:: single: u-LAW

This module allows you to access the Sun audio interface. The Sun audio hardware
is capable of recording and playing back audio data in u-LAW format with a
sample rate of 8K per second. A full description can be found in the
:manpage:`audio(7I)` manual page.

.. index:: module: SUNAUDIODEV

The module :mod:`SUNAUDIODEV`  defines constants which may be used with this
module.

This module defines the following variables and functions:


.. exception:: error

   This exception is raised on all errors. The argument is a string describing what
   went wrong.


.. function:: open(mode)

   This function opens the audio device and returns a Sun audio device object. This
   object can then be used to do I/O on. The *mode* parameter is one of ``'r'`` for
   record-only access, ``'w'`` for play-only access, ``'rw'`` for both and
   ``'control'`` for access to the control device. Since only one process is
   allowed to have the recorder or player open at the same time it is a good idea
   to open the device only for the activity needed. See :manpage:`audio(7I)` for
   details.

   As per the manpage, this module first looks in the environment variable
   ``AUDIODEV`` for the base audio device filename.  If not found, it falls back to
   :file:`/dev/audio`.  The control device is calculated by appending "ctl" to the
   base audio device.


.. _audio-device-objects:

Audio Device Objects
--------------------

The audio device objects are returned by :func:`.open` define the following
methods (except ``control`` objects which only provide :meth:`getinfo`,
:meth:`setinfo`, :meth:`fileno`, and :meth:`drain`):


.. method:: audio device.close()

   This method explicitly closes the device. It is useful in situations where
   deleting the object does not immediately close it since there are other
   references to it. A closed device should not be used again.


.. method:: audio device.fileno()

   Returns the file descriptor associated with the device.  This can be used to set
   up ``SIGPOLL`` notification, as described below.


.. method:: audio device.drain()

   This method waits until all pending output is processed and then returns.
   Calling this method is often not necessary: destroying the object will
   automatically close the audio device and this will do an implicit drain.


.. method:: audio device.flush()

   This method discards all pending output. It can be used avoid the slow response
   to a user's stop request (due to buffering of up to one second of sound).


.. method:: audio device.getinfo()

   This method retrieves status information like input and output volume, etc. and
   returns it in the form of an audio status object. This object has no methods but
   it contains a number of attributes describing the current device status. The
   names and meanings of the attributes are described in ``<sun/audioio.h>`` and in
   the :manpage:`audio(7I)` manual page.  Member names are slightly different from
   their C counterparts: a status object is only a single structure. Members of the
   :c:data:`play` substructure have ``o_`` prepended to their name and members of
   the :c:data:`record` structure have ``i_``. So, the C member
   :c:data:`play.sample_rate` is accessed as :attr:`o_sample_rate`,
   :c:data:`record.gain` as :attr:`i_gain` and :c:data:`monitor_gain` plainly as
   :attr:`monitor_gain`.


.. method:: audio device.ibufcount()

   This method returns the number of samples that are buffered on the recording
   side, i.e. the program will not block on a :func:`read` call of so many samples.


.. method:: audio device.obufcount()

   This method returns the number of samples buffered on the playback side.
   Unfortunately, this number cannot be used to determine a number of samples that
   can be written without blocking since the kernel output queue length seems to be
   variable.


.. method:: audio device.read(size)

   This method reads *size* samples from the audio input and returns them as a
   Python string. The function blocks until enough data is available.


.. method:: audio device.setinfo(status)

   This method sets the audio device status parameters. The *status* parameter is
   an device status object as returned by :func:`getinfo` and possibly modified by
   the program.


.. method:: audio device.write(samples)

   Write is passed a Python string containing audio samples to be played. If there
   is enough buffer space free it will immediately return, otherwise it will block.

The audio device supports asynchronous notification of various events, through
the SIGPOLL signal.  Here's an example of how you might enable this in Python::

   def handle_sigpoll(signum, frame):
       print 'I got a SIGPOLL update'

   import fcntl, signal, STROPTS

   signal.signal(signal.SIGPOLL, handle_sigpoll)
   fcntl.ioctl(audio_obj.fileno(), STROPTS.I_SETSIG, STROPTS.S_MSG)


:mod:`SUNAUDIODEV` --- Constants used with :mod:`sunaudiodev`
=============================================================

.. module:: SUNAUDIODEV
   :platform: SunOS
   :synopsis: Constants for use with sunaudiodev.
   :deprecated:

.. deprecated:: 2.6
   The :mod:`SUNAUDIODEV` module has been deprecated for removal in Python 3.0.



.. index:: module: sunaudiodev

This is a companion module to :mod:`sunaudiodev` which defines useful symbolic
constants like :const:`MIN_GAIN`, :const:`MAX_GAIN`, :const:`SPEAKER`, etc. The
names of the constants are the same names as used in the C include file
``<sun/audioio.h>``, with the leading string ``AUDIO_`` stripped.

