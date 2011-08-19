:mod:`sndhdr` --- Determine type of sound file
==============================================

.. module:: sndhdr
   :synopsis: Determine type of a sound file.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. Based on comments in the module source file.

.. index::
   single: A-LAW
   single: u-LAW

**Source code:** :source:`Lib/sndhdr.py`

--------------

The :mod:`sndhdr` provides utility functions which attempt to determine the type
of sound data which is in a file.  When these functions are able to determine
what type of sound data is stored in a file, they return a tuple ``(type,
sampling_rate, channels, frames, bits_per_sample)``.  The value for *type*
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
   :func:`whathdr`.  If it succeeds, returns a tuple as described above, otherwise
   ``None`` is returned.


.. function:: whathdr(filename)

   Determines the type of sound data stored in a file based on the file  header.
   The name of the file is given by *filename*.  This function returns a tuple as
   described above on success, or ``None``.

