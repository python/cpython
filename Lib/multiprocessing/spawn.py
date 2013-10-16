#
# Code used to start processes when using the spawn or forkserver
# start methods.
#
# multiprocessing/spawn.py
#
# Copyright (c) 2006-2008, R Oudkerk
# Licensed to PSF under a Contributor Agreement.
#

import os
import pickle
import sys

from . import get_start_method, set_start_method
from . import process
from . import util

__all__ = ['_main', 'freeze_support', 'set_executable', 'get_executable',
           'get_preparation_data', 'get_command_line', 'import_main_path']

#
# _python_exe is the assumed path to the python executable.
# People embedding Python want to modify it.
#

if sys.platform != 'win32':
    WINEXE = False
    WINSERVICE = False
else:
    WINEXE = (sys.platform == 'win32' and getattr(sys, 'frozen', False))
    WINSERVICE = sys.executable.lower().endswith("pythonservice.exe")

if WINSERVICE:
    _python_exe = os.path.join(sys.exec_prefix, 'python.exe')
else:
    _python_exe = sys.executable

def set_executable(exe):
    global _python_exe
    _python_exe = exe

def get_executable():
    return _python_exe

#
#
#

def is_forking(argv):
    '''
    Return whether commandline indicates we are forking
    '''
    if len(argv) >= 2 and argv[1] == '--multiprocessing-fork':
        return True
    else:
        return False


def freeze_support():
    '''
    Run code for process object if this in not the main process
    '''
    if is_forking(sys.argv):
        main()
        sys.exit()


def get_command_line(**kwds):
    '''
    Returns prefix of command line used for spawning a child process
    '''
    if getattr(sys, 'frozen', False):
        return [sys.executable, '--multiprocessing-fork']
    else:
        prog = 'from multiprocessing.spawn import spawn_main; spawn_main(%s)'
        prog %= ', '.join('%s=%r' % item for item in kwds.items())
        opts = util._args_from_interpreter_flags()
        return [_python_exe] + opts + ['-c', prog, '--multiprocessing-fork']


def spawn_main(pipe_handle, parent_pid=None, tracker_fd=None):
    '''
    Run code specifed by data received over pipe
    '''
    assert is_forking(sys.argv)
    if sys.platform == 'win32':
        import msvcrt
        from .reduction import steal_handle
        new_handle = steal_handle(parent_pid, pipe_handle)
        fd = msvcrt.open_osfhandle(new_handle, os.O_RDONLY)
    else:
        from . import semaphore_tracker
        semaphore_tracker._semaphore_tracker._fd = tracker_fd
        fd = pipe_handle
    exitcode = _main(fd)
    sys.exit(exitcode)


def _main(fd):
    with os.fdopen(fd, 'rb', closefd=True) as from_parent:
        process.current_process()._inheriting = True
        try:
            preparation_data = pickle.load(from_parent)
            prepare(preparation_data)
            self = pickle.load(from_parent)
        finally:
            del process.current_process()._inheriting
    return self._bootstrap()


def _check_not_importing_main():
    if getattr(process.current_process(), '_inheriting', False):
        raise RuntimeError('''
        An attempt has been made to start a new process before the
        current process has finished its bootstrapping phase.

        This probably means that you are not using fork to start your
        child processes and you have forgotten to use the proper idiom
        in the main module:

            if __name__ == '__main__':
                freeze_support()
                ...

        The "freeze_support()" line can be omitted if the program
        is not going to be frozen to produce an executable.''')


def get_preparation_data(name):
    '''
    Return info about parent needed by child to unpickle process object
    '''
    _check_not_importing_main()
    d = dict(
        log_to_stderr=util._log_to_stderr,
        authkey=process.current_process().authkey,
        )

    if util._logger is not None:
        d['log_level'] = util._logger.getEffectiveLevel()

    sys_path=sys.path.copy()
    try:
        i = sys_path.index('')
    except ValueError:
        pass
    else:
        sys_path[i] = process.ORIGINAL_DIR

    d.update(
        name=name,
        sys_path=sys_path,
        sys_argv=sys.argv,
        orig_dir=process.ORIGINAL_DIR,
        dir=os.getcwd(),
        start_method=get_start_method(),
        )

    if sys.platform != 'win32' or (not WINEXE and not WINSERVICE):
        main_path = getattr(sys.modules['__main__'], '__file__', None)
        if not main_path and sys.argv[0] not in ('', '-c'):
            main_path = sys.argv[0]
        if main_path is not None:
            if (not os.path.isabs(main_path) and
                        process.ORIGINAL_DIR is not None):
                main_path = os.path.join(process.ORIGINAL_DIR, main_path)
            d['main_path'] = os.path.normpath(main_path)

    return d

#
# Prepare current process
#

old_main_modules = []

def prepare(data):
    '''
    Try to get current process ready to unpickle process object
    '''
    if 'name' in data:
        process.current_process().name = data['name']

    if 'authkey' in data:
        process.current_process().authkey = data['authkey']

    if 'log_to_stderr' in data and data['log_to_stderr']:
        util.log_to_stderr()

    if 'log_level' in data:
        util.get_logger().setLevel(data['log_level'])

    if 'sys_path' in data:
        sys.path = data['sys_path']

    if 'sys_argv' in data:
        sys.argv = data['sys_argv']

    if 'dir' in data:
        os.chdir(data['dir'])

    if 'orig_dir' in data:
        process.ORIGINAL_DIR = data['orig_dir']

    if 'start_method' in data:
        set_start_method(data['start_method'])

    if 'main_path' in data:
        import_main_path(data['main_path'])


def import_main_path(main_path):
    '''
    Set sys.modules['__main__'] to module at main_path
    '''
    # XXX (ncoghlan): The following code makes several bogus
    # assumptions regarding the relationship between __file__
    # and a module's real name. See PEP 302 and issue #10845
    if getattr(sys.modules['__main__'], '__file__', None) == main_path:
        return

    main_name = os.path.splitext(os.path.basename(main_path))[0]
    if main_name == '__init__':
        main_name = os.path.basename(os.path.dirname(main_path))

    if main_name == '__main__':
        main_module = sys.modules['__main__']
        main_module.__file__ = main_path
    elif main_name != 'ipython':
        # Main modules not actually called __main__.py may
        # contain additional code that should still be executed
        import importlib
        import types

        if main_path is None:
            dirs = None
        elif os.path.basename(main_path).startswith('__init__.py'):
            dirs = [os.path.dirname(os.path.dirname(main_path))]
        else:
            dirs = [os.path.dirname(main_path)]

        assert main_name not in sys.modules, main_name
        sys.modules.pop('__mp_main__', None)
        # We should not try to load __main__
        # since that would execute 'if __name__ == "__main__"'
        # clauses, potentially causing a psuedo fork bomb.
        loader = importlib.find_loader(main_name, path=dirs)
        main_module = types.ModuleType(main_name)
        try:
            loader.init_module_attrs(main_module)
        except AttributeError:  # init_module_attrs is optional
            pass
        main_module.__name__ = '__mp_main__'
        code = loader.get_code(main_name)
        exec(code, main_module.__dict__)

        old_main_modules.append(sys.modules['__main__'])
        sys.modules['__main__'] = sys.modules['__mp_main__'] = main_module
