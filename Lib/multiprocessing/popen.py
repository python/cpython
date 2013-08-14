import sys
import threading

__all__ = ['Popen', 'get_spawning_popen', 'set_spawning_popen',
           'assert_spawning']

#
# Check that the current thread is spawning a child process
#

_tls = threading.local()

def get_spawning_popen():
    return getattr(_tls, 'spawning_popen', None)

def set_spawning_popen(popen):
    _tls.spawning_popen = popen

def assert_spawning(obj):
    if get_spawning_popen() is None:
        raise RuntimeError(
            '%s objects should only be shared between processes'
            ' through inheritance' % type(obj).__name__
            )

#
#
#

_Popen = None

def Popen(process_obj):
    if _Popen is None:
        set_start_method()
    return _Popen(process_obj)

def get_start_method():
    if _Popen is None:
        set_start_method()
    return _Popen.method

def set_start_method(meth=None, *, start_helpers=True):
    global _Popen
    try:
        modname = _method_to_module[meth]
        __import__(modname)
    except (KeyError, ImportError):
        raise ValueError('could not use start method %r' % meth)
    module = sys.modules[modname]
    if start_helpers:
        module.Popen.ensure_helpers_running()
    _Popen = module.Popen


if sys.platform == 'win32':

    _method_to_module = {
        None: 'multiprocessing.popen_spawn_win32',
        'spawn': 'multiprocessing.popen_spawn_win32',
        }

    def get_all_start_methods():
        return ['spawn']

else:
    _method_to_module = {
        None: 'multiprocessing.popen_fork',
        'fork': 'multiprocessing.popen_fork',
        'spawn': 'multiprocessing.popen_spawn_posix',
        'forkserver': 'multiprocessing.popen_forkserver',
        }

    def get_all_start_methods():
        from . import reduction
        if reduction.HAVE_SEND_HANDLE:
            return ['fork', 'spawn', 'forkserver']
        else:
            return ['fork', 'spawn']
