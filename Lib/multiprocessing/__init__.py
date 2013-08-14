#
# Package analogous to 'threading.py' but using processes
#
# multiprocessing/__init__.py
#
# This package is intended to duplicate the functionality (and much of
# the API) of threading.py but uses processes instead of threads.  A
# subpackage 'multiprocessing.dummy' has the same API but is a simple
# wrapper for 'threading'.
#
# Copyright (c) 2006-2008, R Oudkerk
# Licensed to PSF under a Contributor Agreement.
#

__version__ = '0.70a1'

__all__ = [
    'Process', 'current_process', 'active_children', 'freeze_support',
    'Manager', 'Pipe', 'cpu_count', 'log_to_stderr', 'get_logger',
    'allow_connection_pickling', 'BufferTooShort', 'TimeoutError',
    'Lock', 'RLock', 'Semaphore', 'BoundedSemaphore', 'Condition',
    'Event', 'Barrier', 'Queue', 'SimpleQueue', 'JoinableQueue', 'Pool',
    'Value', 'Array', 'RawValue', 'RawArray', 'SUBDEBUG', 'SUBWARNING',
    'set_executable', 'set_start_method', 'get_start_method',
    'get_all_start_methods', 'set_forkserver_preload'
    ]

#
# Imports
#

import os
import sys

from .process import Process, current_process, active_children

#
# XXX These should not really be documented or public.
#

SUBDEBUG = 5
SUBWARNING = 25

#
# Alias for main module -- will be reset by bootstrapping child processes
#

if '__main__' in sys.modules:
    sys.modules['__mp_main__'] = sys.modules['__main__']

#
# Exceptions
#

class ProcessError(Exception):
    pass

class BufferTooShort(ProcessError):
    pass

class TimeoutError(ProcessError):
    pass

class AuthenticationError(ProcessError):
    pass

#
# Definitions not depending on native semaphores
#

def Manager():
    '''
    Returns a manager associated with a running server process

    The managers methods such as `Lock()`, `Condition()` and `Queue()`
    can be used to create shared objects.
    '''
    from .managers import SyncManager
    m = SyncManager()
    m.start()
    return m

def Pipe(duplex=True):
    '''
    Returns two connection object connected by a pipe
    '''
    from .connection import Pipe
    return Pipe(duplex)

def cpu_count():
    '''
    Returns the number of CPUs in the system
    '''
    num = os.cpu_count()
    if num is None:
        raise NotImplementedError('cannot determine number of cpus')
    else:
        return num

def freeze_support():
    '''
    Check whether this is a fake forked process in a frozen executable.
    If so then run code specified by commandline and exit.
    '''
    if sys.platform == 'win32' and getattr(sys, 'frozen', False):
        from .spawn import freeze_support
        freeze_support()

def get_logger():
    '''
    Return package logger -- if it does not already exist then it is created
    '''
    from .util import get_logger
    return get_logger()

def log_to_stderr(level=None):
    '''
    Turn on logging and add a handler which prints to stderr
    '''
    from .util import log_to_stderr
    return log_to_stderr(level)

def allow_connection_pickling():
    '''
    Install support for sending connections and sockets between processes
    '''
    # This is undocumented.  In previous versions of multiprocessing
    # its only effect was to make socket objects inheritable on Windows.
    from . import connection

#
# Definitions depending on native semaphores
#

def Lock():
    '''
    Returns a non-recursive lock object
    '''
    from .synchronize import Lock
    return Lock()

def RLock():
    '''
    Returns a recursive lock object
    '''
    from .synchronize import RLock
    return RLock()

def Condition(lock=None):
    '''
    Returns a condition object
    '''
    from .synchronize import Condition
    return Condition(lock)

def Semaphore(value=1):
    '''
    Returns a semaphore object
    '''
    from .synchronize import Semaphore
    return Semaphore(value)

def BoundedSemaphore(value=1):
    '''
    Returns a bounded semaphore object
    '''
    from .synchronize import BoundedSemaphore
    return BoundedSemaphore(value)

def Event():
    '''
    Returns an event object
    '''
    from .synchronize import Event
    return Event()

def Barrier(parties, action=None, timeout=None):
    '''
    Returns a barrier object
    '''
    from .synchronize import Barrier
    return Barrier(parties, action, timeout)

def Queue(maxsize=0):
    '''
    Returns a queue object
    '''
    from .queues import Queue
    return Queue(maxsize)

def JoinableQueue(maxsize=0):
    '''
    Returns a queue object
    '''
    from .queues import JoinableQueue
    return JoinableQueue(maxsize)

def SimpleQueue():
    '''
    Returns a queue object
    '''
    from .queues import SimpleQueue
    return SimpleQueue()

def Pool(processes=None, initializer=None, initargs=(), maxtasksperchild=None):
    '''
    Returns a process pool object
    '''
    from .pool import Pool
    return Pool(processes, initializer, initargs, maxtasksperchild)

def RawValue(typecode_or_type, *args):
    '''
    Returns a shared object
    '''
    from .sharedctypes import RawValue
    return RawValue(typecode_or_type, *args)

def RawArray(typecode_or_type, size_or_initializer):
    '''
    Returns a shared array
    '''
    from .sharedctypes import RawArray
    return RawArray(typecode_or_type, size_or_initializer)

def Value(typecode_or_type, *args, lock=True):
    '''
    Returns a synchronized shared object
    '''
    from .sharedctypes import Value
    return Value(typecode_or_type, *args, lock=lock)

def Array(typecode_or_type, size_or_initializer, *, lock=True):
    '''
    Returns a synchronized shared array
    '''
    from .sharedctypes import Array
    return Array(typecode_or_type, size_or_initializer, lock=lock)

#
#
#

def set_executable(executable):
    '''
    Sets the path to a python.exe or pythonw.exe binary used to run
    child processes instead of sys.executable when using the 'spawn'
    start method.  Useful for people embedding Python.
    '''
    from .spawn import set_executable
    set_executable(executable)

def set_start_method(method):
    '''
    Set method for starting processes: 'fork', 'spawn' or 'forkserver'.
    '''
    from .popen import set_start_method
    set_start_method(method)

def get_start_method():
    '''
    Get method for starting processes: 'fork', 'spawn' or 'forkserver'.
    '''
    from .popen import get_start_method
    return get_start_method()

def get_all_start_methods():
    '''
    Get list of availables start methods, default first.
    '''
    from .popen import get_all_start_methods
    return get_all_start_methods()

def set_forkserver_preload(module_names):
    '''
    Set list of module names to try to load in the forkserver process
    when it is started.  Properly chosen this can significantly reduce
    the cost of starting a new process using the forkserver method.
    The default list is ['__main__'].
    '''
    try:
        from .forkserver import set_forkserver_preload
    except ImportError:
        pass
    else:
        set_forkserver_preload(module_names)
