:mod:`!wave` --- Read and write WAV files
=========================================

.. module:: wave
   :synopsis: Provide an interface to the WAV sound format.

**Source code:** :source:`Lib/wave.py`

--------------

The :mod:`!wave` module provides a convenient interface to the Waveform Audio
"WAVE" (or "WAV") file format.

The module supports uncompressed PCM and IEEE floating-point WAV formats.

.. versionchanged:: 3.12

   Support for ``WAVE_FORMAT_EXTENSIBLE`` headers was added, provided that the
   extended format is ``KSDATAFORMAT_SUBTYPE_PCM``.

.. versionchanged:: next

   Support for reading and writing ``WAVE_FORMAT_IEEE_FLOAT`` files was added.

The :mod:`!wave` module defines the following function and exception:


.. function:: open(file, mode=None)

   If *file* is a string, a :term:`path-like object` or a
   :term:`bytes-like object` open the file by that name, otherwise treat it as
   a file-like object.  *mode* can be:

   ``'rb'``
      Read only mode.

   ``'wb'``
      Write only mode.

   Note that it does not allow read/write WAV files.

   A *mode* of ``'rb'`` returns a :class:`Wave_read` object, while a *mode* of
   ``'wb'`` returns a :class:`Wave_write` object.  If *mode* is omitted and a
   file-like object is passed as *file*, ``file.mode`` is used as the default
   value for *mode*.

   If you pass in a file-like object, the wave object will not close it when its
   ``close()`` method is called; it is the caller's responsibility to close
   the file object.

   The :func:`.open` function may be used in a :keyword:`with` statement.  When
   the :keyword:`!with` block completes, the :meth:`Wave_read.close` or
   :meth:`Wave_write.close` method is called.

   .. versionchanged:: 3.4
      Added support for unseekable files.

   .. versionchanged:: 3.15
      Added support for :term:`path-like objects <path-like object>`
      and :term:`bytes-like objects <bytes-like object>`.

.. exception:: Error

   An error raised when something is impossible because it violates the WAV
   specification or hits an implementation deficiency.


.. data:: WAVE_FORMAT_PCM

   Format code for uncompressed PCM audio.


.. data:: WAVE_FORMAT_IEEE_FLOAT

   Format code for IEEE floating-point audio.


.. data:: WAVE_FORMAT_EXTENSIBLE

   Format code for WAVE extensible headers.


.. _wave-read-objects:

Wave_read Objects
-----------------

.. class:: Wave_read

   Read a WAV file.

   Wave_read objects, as returned by :func:`.open`, have the following methods:


   .. method:: close()

      Close the stream if it was opened by :mod:`!wave`, and make the instance
      unusable.  This is called automatically on object collection.


   .. method:: getnchannels()

      Returns number of audio channels (``1`` for mono, ``2`` for stereo).


   .. method:: getsampwidth()

      Returns sample width in bytes.


   .. method:: getframerate()

      Returns sampling frequency.


   .. method:: getnframes()

      Returns number of audio frames.


   .. method:: getformat()

      Returns the frame format code.

      This is one of :data:`WAVE_FORMAT_PCM`,
      :data:`WAVE_FORMAT_IEEE_FLOAT`, or :data:`WAVE_FORMAT_EXTENSIBLE`.


   .. method:: getcomptype()

      Returns compression type (``'NONE'`` is the only supported type).


   .. method:: getcompname()

      Human-readable version of :meth:`getcomptype`. Usually ``'not compressed'``
      parallels ``'NONE'``.


   .. method:: getparams()

      Returns a :func:`~collections.namedtuple` ``(nchannels, sampwidth,
      framerate, nframes, comptype, compname)``, equivalent to output
      of the ``get*()`` methods.


   .. method:: readframes(n)

      Reads and returns at most *n* frames of audio, as a :class:`bytes` object.


   .. method:: rewind()

      Rewind the file pointer to the beginning of the audio stream.


   The following two methods define a term "position" which is compatible between
   them, and is otherwise implementation dependent.


   .. method:: setpos(pos)

      Set the file pointer to the specified position.


   .. method:: tell()

      Return current file pointer position.


.. _wave-write-objects:

Wave_write Objects
------------------

.. class:: Wave_write

   Write a WAV file.

   Wave_write objects, as returned by :func:`.open`.

   For seekable output streams, the ``wave`` header will automatically be updated
   to reflect the number of frames actually written.  For unseekable streams, the
   *nframes* value must be accurate when the first frame data is written.  An
   accurate *nframes* value can be achieved either by calling
   :meth:`setnframes` or :meth:`setparams` with the number
   of frames that will be written before :meth:`close` is called and
   then using :meth:`writeframesraw` to write the frame data, or by
   calling :meth:`writeframes` with all of the frame data to be
   written.  In the latter case :meth:`writeframes` will calculate
   the number of frames in the data and set *nframes* accordingly before writing
   the frame data.

   .. versionchanged:: 3.4
      Added support for unseekable files.

   Wave_write objects have the following methods:

   .. method:: close()

      Make sure *nframes* is correct, and close the file if it was opened by
      :mod:`!wave`.  This method is called upon object collection.  It will raise
      an exception if the output stream is not seekable and *nframes* does not
      match the number of frames actually written.


   .. method:: setnchannels(n)

      Set the number of channels.


   .. method:: getnchannels()

      Return the number of channels.


   .. method:: setsampwidth(n)

      Set the sample width to *n* bytes.

      For :data:`WAVE_FORMAT_IEEE_FLOAT`, only 4-byte (32-bit) and
      8-byte (64-bit) sample widths are supported.


   .. method:: getsampwidth()

      Return the sample width in bytes.


   .. method:: setframerate(n)

      Set the frame rate to *n*.

      .. versionchanged:: 3.2
         A non-integral input to this method is rounded to the nearest
         integer.


   .. method:: getframerate()

      Return the frame rate.


   .. method:: setnframes(n)

      Set the number of frames to *n*.  This will be changed later if the number
      of frames actually written is different (this update attempt will
      raise an error if the output stream is not seekable).


   .. method:: getnframes()

      Return the number of audio frames written so far.


   .. method:: setcomptype(type, name)

      Set the compression type and description. At the moment, only compression type
      ``NONE`` is supported, meaning no compression.


   .. method:: getcomptype()

      Return the compression type (``'NONE'``).


   .. method:: getcompname()

      Return the human-readable compression type name.


   .. method:: setformat(format)

      Set the frame format code.

      Supported values are :data:`WAVE_FORMAT_PCM` and
      :data:`WAVE_FORMAT_IEEE_FLOAT`.

      When setting :data:`WAVE_FORMAT_IEEE_FLOAT`, the sample width must be
      4 or 8 bytes.


   .. method:: getformat()

      Return the current frame format code.


   .. method:: setparams(tuple)

      The *tuple* should be
      ``(nchannels, sampwidth, framerate, nframes, comptype, compname, format)``,
      with values valid for the ``set*()`` methods. Sets all parameters.

      For backwards compatibility, a 6-item tuple without *format* is also
      accepted and defaults to :data:`WAVE_FORMAT_PCM`.

      For ``format=WAVE_FORMAT_IEEE_FLOAT``, *sampwidth* must be 4 or 8.


   .. method:: getparams()

      Return a :func:`~collections.namedtuple`
      ``(nchannels, sampwidth, framerate, nframes, comptype, compname)``
      containing the current output parameters.


   .. method:: tell()

      Return current position in the file, with the same disclaimer for the
      :meth:`Wave_read.tell` and :meth:`Wave_read.setpos` methods.


   .. method:: writeframesraw(data)

      Write audio frames, without correcting *nframes*.

      .. versionchanged:: 3.4
         Any :term:`bytes-like object` is now accepted.


   .. method:: writeframes(data)

      Write audio frames and make sure *nframes* is correct.  It will raise an
      error if the output stream is not seekable and the total number of frames
      that have been written after *data* has been written does not match the
      previously set value for *nframes*.

      .. versionchanged:: 3.4
         Any :term:`bytes-like object` is now accepted.

      Note that it is invalid to set any parameters after calling :meth:`writeframes`
      or :meth:`writeframesraw`, and any attempt to do so will raise
      :exc:`wave.Error`.

      For :data:`WAVE_FORMAT_IEEE_FLOAT` output, a ``fact`` chunk is written as
      required by the WAVE specification for non-PCM formats.
