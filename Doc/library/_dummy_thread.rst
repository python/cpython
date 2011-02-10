:mod:`_dummy_thread` --- Drop-in replacement for the :mod:`_thread` module
==========================================================================

.. module:: _dummy_thread
   :synopsis: Drop-in replacement for the _thread module.

**Source code:** :source:`Lib/_dummy_thread.py`

--------------

This module provides a duplicate interface to the :mod:`_thread` module.  It is
meant to be imported when the :mod:`_thread` module is not provided on a
platform.

Suggested usage is::

   try:
       import _thread
   except ImportError:
       import dummy_thread as _thread

Be careful to not use this module where deadlock might occur from a thread being
created that blocks waiting for another thread to be created.  This often occurs
with blocking I/O.

