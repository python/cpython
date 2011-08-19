:mod:`dummy_threading` --- Drop-in replacement for the :mod:`threading` module
==============================================================================

.. module:: dummy_threading
   :synopsis: Drop-in replacement for the threading module.

**Source code:** :source:`Lib/dummy_threading.py`

--------------

This module provides a duplicate interface to the :mod:`threading` module.  It
is meant to be imported when the :mod:`thread` module is not provided on a
platform.

Suggested usage is::

   try:
       import threading as _threading
   except ImportError:
       import dummy_threading as _threading

Be careful to not use this module where deadlock might occur from a thread
being created that blocks waiting for another thread to be created.  This  often
occurs with blocking I/O.

