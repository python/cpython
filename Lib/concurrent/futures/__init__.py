# Copyright 2009 Brian Quinlan. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Execute computations asynchronously using threads or processes."""

__author__ = 'Brian Quinlan (brian@sweetapp.com)'

from concurrent.futures._base import (FIRST_COMPLETED,
                                      FIRST_EXCEPTION,
                                      ALL_COMPLETED,
                                      CancelledError,
                                      TimeoutError,
                                      InvalidStateError,
                                      BrokenExecutor,
                                      Future,
                                      Executor,
                                      wait,
                                      as_completed)

lazy from .process import ProcessPoolExecutor
lazy from .thread import ThreadPoolExecutor

__all__ = [
    'FIRST_COMPLETED',
    'FIRST_EXCEPTION',
    'ALL_COMPLETED',
    'CancelledError',
    'TimeoutError',
    'InvalidStateError',
    'BrokenExecutor',
    'Future',
    'Executor',
    'wait',
    'as_completed',
    'ProcessPoolExecutor',
    'ThreadPoolExecutor',
]


try:
    import _interpreters
except ImportError:
    _interpreters = None

if _interpreters:
    lazy from .interpreter import InterpreterPoolExecutor  # noqa: F401
    __all__.append('InterpreterPoolExecutor')


# Set __module__ to the public location rather than the private _base module.
Future.__module__ = 'concurrent.futures'
Executor.__module__ = 'concurrent.futures'
CancelledError.__module__ = 'concurrent.futures'
InvalidStateError.__module__ = 'concurrent.futures'
BrokenExecutor.__module__ = 'concurrent.futures'


def __dir__():
    return __all__ + ['__author__', '__doc__']
