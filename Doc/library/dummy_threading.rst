:mod:`dummy_threading` --- Drop-in replacement for the :mod:`threading` module
==============================================================================

.. module:: dummy_threading
   :synopsis: Drop-in replacement for the threading module.

**Source code:** :source:`Lib/dummy_threading.py`

.. deprecated:: 3.7
   Python now always has threading enabled.  Please use :mod:`threading` instead.

--------------

This module provides a duplicate interface to the :mod:`threading` module.
It was meant to be imported when the :mod:`_thread` module was not provided
on a platform.

Be careful to not use this module where deadlock might occur from a thread being
created that blocks waiting for another thread to be created.  This often occurs
with blocking I/O.
