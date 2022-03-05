:mod:`_dummy_thread` --- Drop-in replacement for the :mod:`_thread` module
==========================================================================

.. module:: _dummy_thread
   :synopsis: Drop-in replacement for the _thread module.

**Source code:** :source:`Lib/_dummy_thread.py`

.. deprecated:: 3.7
   Python now always has threading enabled.  Please use :mod:`_thread`
   (or, better, :mod:`threading`) instead.

--------------

This module provides a duplicate interface to the :mod:`_thread` module.
It was meant to be imported when the :mod:`_thread` module was not provided
on a platform.

Be careful to not use this module where deadlock might occur from a thread being
created that blocks waiting for another thread to be created.  This often occurs
with blocking I/O.

