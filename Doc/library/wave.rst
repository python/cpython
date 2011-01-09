:mod:`wave` --- Read and write WAV files
========================================

.. module:: wave
   :synopsis: Provide an interface to the WAV sound format.
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>
.. Documentations stolen from comments in file.

The :mod:`wave` module provides a convenient interface to the WAV sound format.
It does not support compression/decompression, but it does support mono/stereo.

The :mod:`wave` module defines the following function and exception:


.. function:: open(file, mode=None)

   If *file* is a string, open the file by that name, otherwise treat it as a
   seekable file-like object.  *mode* can be any of

   ``'r'``, ``'rb'``
      Read only mode.

   ``'w'``, ``'wb'``
      Write only mode.

   Note that it does not allow read/write WAV files.

   A *mode* of ``'r'`` or ``'rb'`` returns a :class:`Wave_read` object, while a
   *mode* of ``'w'`` or ``'wb'`` returns a :class:`Wave_write` object.  If
   *mode* is omitted and a file-like object is passed as *file*, ``file.mode``
   is used as the default value for *mode* (the ``'b'`` flag is still added if
   necessary).

   If you pass in a file-like object, the wave object will not close it when its
   :meth:`close` method is called; it is the caller's responsibility to close
   the file object.


.. function:: openfp(file, mode)

   A synonym for :func:`.open`, maintained for backwards compatibility.


.. exception:: Error

   An error raised when something is impossible because it violates the WAV
   specification or hits an implementation deficiency.


.. _wave-read-objects:

Wave_read Objects
-----------------

Wave_read objects, as returned by :func:`.open`, have the following methods:


.. method:: Wave_read.close()

   Close the stream if it was opened by :mod:`wave`, and make the instance
   unusable.  This is called automatically on object collection.


.. method:: Wave_read.getnchannels()

   Returns number of audio channels (``1`` for mono, ``2`` for stereo).


.. method:: Wave_read.getsampwidth()

   Returns sample width in bytes.


.. method:: Wave_read.getframerate()

   Returns sampling frequency.


.. method:: Wave_read.getnframes()

   Returns number of audio frames.


.. method:: Wave_read.getcomptype()

   Returns compression type (``'NONE'`` is the only supported type).


.. method:: Wave_read.getcompname()

   Human-readable version of :meth:`getcomptype`. Usually ``'not compressed'``
   parallels ``'NONE'``.


.. method:: Wave_read.getparams()

   Returns a tuple ``(nchannels, sampwidth, framerate, nframes, comptype,
   compname)``, equivalent to output of the :meth:`get\*` methods.


.. method:: Wave_read.readframes(n)

   Reads and returns at most *n* frames of audio, as a string of bytes.


.. method:: Wave_read.rewind()

   Rewind the file pointer to the beginning of the audio stream.

The following two methods are defined for compatibility with the :mod:`aifc`
module, and don't do anything interesting.


.. method:: Wave_read.getmarkers()

   Returns ``None``.


.. method:: Wave_read.getmark(id)

   Raise an error.

The following two methods define a term "position" which is compatible between
them, and is otherwise implementation dependent.


.. method:: Wave_read.setpos(pos)

   Set the file pointer to the specified position.


.. method:: Wave_read.tell()

   Return current file pointer position.


.. _wave-write-objects:

Wave_write Objects
------------------

Wave_write objects, as returned by :func:`.open`, have the following methods:


.. method:: Wave_write.close()

   Make sure *nframes* is correct, and close the file if it was opened by
   :mod:`wave`.  This method is called upon object collection.


.. method:: Wave_write.setnchannels(n)

   Set the number of channels.


.. method:: Wave_write.setsampwidth(n)

   Set the sample width to *n* bytes.


.. method:: Wave_write.setframerate(n)

   Set the frame rate to *n*.


.. method:: Wave_write.setnframes(n)

   Set the number of frames to *n*. This will be changed later if more frames are
   written.


.. method:: Wave_write.setcomptype(type, name)

   Set the compression type and description. At the moment, only compression type
   ``NONE`` is supported, meaning no compression.


.. method:: Wave_write.setparams(tuple)

   The *tuple* should be ``(nchannels, sampwidth, framerate, nframes, comptype,
   compname)``, with values valid for the :meth:`set\*` methods.  Sets all
   parameters.


.. method:: Wave_write.tell()

   Return current position in the file, with the same disclaimer for the
   :meth:`Wave_read.tell` and :meth:`Wave_read.setpos` methods.


.. method:: Wave_write.writeframesraw(data)

   Write audio frames, without correcting *nframes*.


.. method:: Wave_write.writeframes(data)

   Write audio frames and make sure *nframes* is correct.


Note that it is invalid to set any parameters after calling :meth:`writeframes`
or :meth:`writeframesraw`, and any attempt to do so will raise
:exc:`wave.Error`.

