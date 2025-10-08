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
    __all__.append('InterpreterPoolExecutor')


def __dir__():
    return __all__ + ['__author__', '__doc__']


def __getattr__(name):
    global ProcessPoolExecutor, ThreadPoolExecutor, InterpreterPoolExecutor

    if name == 'ProcessPoolExecutor':
        from .process import ProcessPoolExecutor
        return ProcessPoolExecutor

    if name == 'ThreadPoolExecutor':
        from .thread import ThreadPoolExecutor
        return ThreadPoolExecutor

    if _interpreters and name == 'InterpreterPoolExecutor':
        from .interpreter import InterpreterPoolExecutor
        return InterpreterPoolExecutor

    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
