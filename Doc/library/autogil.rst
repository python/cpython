
:mod:`autoGIL` --- Global Interpreter Lock handling in event loops
==================================================================

.. module:: autoGIL
   :platform: Mac
   :synopsis: Global Interpreter Lock handling in event loops.
.. moduleauthor:: Just van Rossum <just@letterror.com>


The :mod:`autoGIL` module provides a function :func:`installAutoGIL` that
automatically locks and unlocks Python's :term:`Global Interpreter Lock` when
running an event loop.


.. exception:: AutoGILError

   Raised if the observer callback cannot be installed, for example because the
   current thread does not have a run loop.


.. function:: installAutoGIL()

   Install an observer callback in the event loop (CFRunLoop) for the current
   thread, that will lock and unlock the Global Interpreter Lock (GIL) at
   appropriate times, allowing other Python threads to run while the event loop is
   idle.

   Availability: OSX 10.1 or later.

