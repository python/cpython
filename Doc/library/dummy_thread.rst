
:mod:`dummy_thread` --- Drop-in replacement for the :mod:`thread` module
========================================================================

.. module:: dummy_thread
   :synopsis: Drop-in replacement for the thread module.


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

