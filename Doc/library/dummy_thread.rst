:mod:`dummy_thread` --- Drop-in replacement for the :mod:`thread` module
========================================================================

.. module:: dummy_thread
   :synopsis: Drop-in replacement for the thread module.

.. note::
   The :mod:`dummy_thread` module has been renamed to :mod:`_dummy_thread` in
   Python 3.0.  The :term:`2to3` tool will automatically adapt imports when
   converting your sources to 3.0; however, you should consider using the
   high-lever :mod:`dummy_threading` module instead.


This module provides a duplicate interface to the :mod:`thread` module.  It is
meant to be imported when the :mod:`thread` module is not provided on a
platform.

Suggested usage is::

   try:
       import thread as _thread
   except ImportError:
       import dummy_thread as _thread

Be careful to not use this module where deadlock might occur from a thread
being created that blocks waiting for another thread to be created.  This  often
occurs with blocking I/O.

