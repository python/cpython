
:mod:`al` --- Audio functions on the SGI
========================================

.. module:: al
   :platform: IRIX
   :synopsis: Audio functions on the SGI.
   :deprecated:

.. deprecated:: 2.6
    The :mod:`al` module has been deprecated for removal in Python 3.0.


This module provides access to the audio facilities of the SGI Indy and Indigo
workstations.  See section 3A of the IRIX man pages for details.  You'll need to
read those man pages to understand what these functions do!  Some of the
functions are not available in IRIX releases before 4.0.5.  Again, see the
manual to check whether a specific function is available on your platform.

All functions and methods defined in this module are equivalent to the C
functions with ``AL`` prefixed to their name.

.. index:: module: AL

Symbolic constants from the C header file ``<audio.h>`` are defined in the
standard module :mod:`AL`, see below.

.. warning::

   The current version of the audio library may dump core when bad argument values
   are passed rather than returning an error status.  Unfortunately, since the
   precise circumstances under which this may happen are undocumented and hard to
   check, the Python interface can provide no protection against this kind of
   problems. (One example is specifying an excessive queue size --- there is no
   documented upper limit.)

The module defines the following functions:


.. function:: openport(name, direction[, config])

   The name and direction arguments are strings.  The optional *config* argument is
   a configuration object as returned by :func:`newconfig`.  The return value is an
   :dfn:`audio port object`; methods of audio port objects are described below.


.. function:: newconfig()

   The return value is a new :dfn:`audio configuration object`; methods of audio
   configuration objects are described below.


.. function:: queryparams(device)

   The device argument is an integer.  The return value is a list of integers
   containing the data returned by :c:func:`ALqueryparams`.


.. function:: getparams(device, list)

   The *device* argument is an integer.  The list argument is a list such as
   returned by :func:`queryparams`; it is modified in place (!).


.. function:: setparams(device, list)

   The *device* argument is an integer.  The *list* argument is a list such as
   returned by :func:`queryparams`.


.. _al-config-objects:

Configuration Objects
---------------------

Configuration objects returned by :func:`newconfig` have the following methods:


.. method:: audio configuration.getqueuesize()

   Return the queue size.


.. method:: audio configuration.setqueuesize(size)

   Set the queue size.


.. method:: audio configuration.getwidth()

   Get the sample width.


.. method:: audio configuration.setwidth(width)

   Set the sample width.


.. method:: audio configuration.getchannels()

   Get the channel count.


.. method:: audio configuration.setchannels(nchannels)

   Set the channel count.


.. method:: audio configuration.getsampfmt()

   Get the sample format.


.. method:: audio configuration.setsampfmt(sampfmt)

   Set the sample format.


.. method:: audio configuration.getfloatmax()

   Get the maximum value for floating sample formats.


.. method:: audio configuration.setfloatmax(floatmax)

   Set the maximum value for floating sample formats.


.. _al-port-objects:

Port Objects
------------

Port objects, as returned by :func:`openport`, have the following methods:


.. method:: audio port.closeport()

   Close the port.


.. method:: audio port.getfd()

   Return the file descriptor as an int.


.. method:: audio port.getfilled()

   Return the number of filled samples.


.. method:: audio port.getfillable()

   Return the number of fillable samples.


.. method:: audio port.readsamps(nsamples)

   Read a number of samples from the queue, blocking if necessary. Return the data
   as a string containing the raw data, (e.g., 2 bytes per sample in big-endian
   byte order (high byte, low byte) if you have set the sample width to 2 bytes).


.. method:: audio port.writesamps(samples)

   Write samples into the queue, blocking if necessary.  The samples are encoded as
   described for the :meth:`readsamps` return value.


.. method:: audio port.getfillpoint()

   Return the 'fill point'.


.. method:: audio port.setfillpoint(fillpoint)

   Set the 'fill point'.


.. method:: audio port.getconfig()

   Return a configuration object containing the current configuration of the port.


.. method:: audio port.setconfig(config)

   Set the configuration from the argument, a configuration object.


.. method:: audio port.getstatus(list)

   Get status information on last error.


:mod:`AL` --- Constants used with the :mod:`al` module
======================================================

.. module:: AL
   :platform: IRIX
   :synopsis: Constants used with the al module.
   :deprecated:

.. deprecated:: 2.6
   The :mod:`AL` module has been deprecated for removal in Python 3.0.


This module defines symbolic constants needed to use the built-in module
:mod:`al` (see above); they are equivalent to those defined in the C header file
``<audio.h>`` except that the name prefix ``AL_`` is omitted.  Read the module
source for a complete list of the defined names.  Suggested use::

   import al
   from AL import *

