:mod:`sndhdr` --- Determine type of sound file
==============================================

.. module:: sndhdr
   :synopsis: Determine type of a sound file.

.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. Based on comments in the module source file.

**Source code:** :source:`Lib/sndhdr.py`

.. index::
   single: A-LAW
   single: u-LAW

--------------

The :mod:`sndhdr` provides utility functions which attempt to determine the type
of sound data which is in a file.  When these functions are able to determine
what type of sound data is stored in a file, they return a
:func:`~collections.namedtuple`, containing five attributes: (``filetype``,
``framerate``, ``nchannels``, ``nframes``, ``sampwidth``). The value for *type*
indicates the data type and will be one of the strings ``'aifc'``, ``'aiff'``,
``'au'``, ``'hcom'``, ``'sndr'``, ``'sndt'``, ``'voc'``, ``'wav'``, ``'8svx'``,
``'sb'``, ``'ub'``, or ``'ul'``.  The *sampling_rate* will be either the actual
value or ``0`` if unknown or difficult to decode.  Similarly, *channels* will be
either the number of channels or ``0`` if it cannot be determined or if the
value is difficult to decode.  The value for *frames* will be either the number
of frames or ``-1``.  The last item in the tuple, *bits_per_sample*, will either
be the sample size in bits or ``'A'`` for A-LAW or ``'U'`` for u-LAW.


.. function:: what(filename)

   Determines the type of sound data stored in the file *filename* using
   :func:`whathdr`.  If it succeeds, returns a namedtuple as described above, otherwise
   ``None`` is returned.

   .. versionchanged:: 3.5
      Result changed from a tuple to a namedtuple.


.. function:: whathdr(filename)

   Determines the type of sound data stored in a file based on the file  header.
   The name of the file is given by *filename*.  This function returns a namedtuple as
   described above on success, or ``None``.

   .. versionchanged:: 3.5
      Result changed from a tuple to a namedtuple.

