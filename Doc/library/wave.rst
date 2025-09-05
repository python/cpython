:mod:`!wave` --- Read and write WAV files
=========================================

.. module:: wave
   :synopsis: Provide an interface to the WAV sound format.

.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>
.. Documentations stolen from comments in file.

**Source code:** :source:`Lib/wave.py`

--------------

The :mod:`wave` module provides a convenient interface to the Waveform Audio
"WAVE" (or "WAV") file format. Only uncompressed PCM encoded wave files are
supported.

.. versionchanged:: 3.12

   Support for ``WAVE_FORMAT_EXTENSIBLE`` headers was added, provided that the
   extended format is ``KSDATAFORMAT_SUBTYPE_PCM``.

The :mod:`wave` module defines the following function and exception:


.. function:: open(file, mode=None)

   If *file* is a string, open the file by that name, otherwise treat it as a
   file-like object.  *mode* can be:

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

.. exception:: Error

   An error raised when something is impossible because it violates the WAV
   specification or hits an implementation deficiency.


.. _wave-read-objects:

Wave_read Objects
-----------------

.. class:: Wave_read

   Read a WAV file.

   Wave_read objects, as returned by :func:`.open`, have the following methods:


   .. method:: close()

      Close the stream if it was opened by :mod:`wave`, and make the instance
      unusable.  This is called automatically on object collection.


   .. method:: getnchannels()

      Returns number of audio channels (``1`` for mono, ``2`` for stereo).


   .. method:: getsampwidth()

      Returns sample width in bytes.


   .. method:: getframerate()

      Returns sampling frequency.


   .. method:: getnframes()

      Returns number of audio frames.


   .. method:: getcomptype()

      Returns compression type (``'NONE'`` is the only supported type).


   .. method:: getcompname()

      Human-readable version of :meth:`getcomptype`. Usually ``'not compressed'``
      parallels ``'NONE'``.


   .. method:: getparams()

      Returns a :func:`~collections.namedtuple` ``(nchannels, sampwidth,
      framerate, nframes, comptype, compname)``, equivalent to output of the
      ``get*()`` methods.


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
      :mod:`wave`.  This method is called upon object collection.  It will raise
      an exception if the output stream is not seekable and *nframes* does not
      match the number of frames actually written.


   .. method:: setnchannels(n)

      Set the number of channels.


   .. method:: setsampwidth(n)

      Set the sample width to *n* bytes.


   .. method:: setframerate(n)

      Set the frame rate to *n*.

      .. versionchanged:: 3.2
         A non-integral input to this method is rounded to the nearest
         integer.


   .. method:: setnframes(n)

      Set the number of frames to *n*.  This will be changed later if the number
      of frames actually written is different (this update attempt will
      raise an error if the output stream is not seekable).


   .. method:: setcomptype(type, name)

      Set the compression type and description. At the moment, only compression type
      ``NONE`` is supported, meaning no compression.


   .. method:: setparams(tuple)

      The *tuple* should be ``(nchannels, sampwidth, framerate, nframes, comptype,
      compname)``, with values valid for the ``set*()`` methods.  Sets all
      parameters.


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
