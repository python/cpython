# Copyright 2009 Brian Quinlan. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Execute computations asynchronously using threads or processes."""

# Original author: Brian Quinlan (brian@sweetapp.com)

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

lazy import warnings
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


def __dir__():
    return __all__ + ['__doc__']


def __getattr__(name):
    if name == "__author__":
        warnings._deprecated("__author__", remove=(3, 20))
        return 'Brian Quinlan (brian@sweetapp.com)'  # Do not change
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
