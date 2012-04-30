#
# Module for starting a process object using os.fork() or CreateProcess()
#
# multiprocessing/forking.py
#
# Copyright (c) 2006-2008, R Oudkerk
# Licensed to PSF under a Contributor Agreement.
#

import os
import sys
import signal

from multiprocessing import util, process

__all__ = ['Popen', 'assert_spawning', 'exit', 'duplicate', 'close', 'ForkingPickler']

#
# Check that the current thread is spawning a child process
#

def assert_spawning(self):
    if not Popen.thread_is_spawning():
        raise RuntimeError(
            '%s objects should only be shared between processes'
            ' through inheritance' % type(self).__name__
            )

#
# Try making some callable types picklable
#

from pickle import Pickler
from copyreg import dispatch_table

class ForkingPickler(Pickler):
    _extra_reducers = {}
    def __init__(self, *args):
        Pickler.__init__(self, *args)
        self.dispatch_table = dispatch_table.copy()
        self.dispatch_table.update(self._extra_reducers)
    @classmethod
    def register(cls, type, reduce):
        cls._extra_reducers[type] = reduce

def _reduce_method(m):
    if m.__self__ is None:
        return getattr, (m.__class__, m.__func__.__name__)
    else:
        return getattr, (m.__self__, m.__func__.__name__)
class _C:
    def f(self):
        pass
ForkingPickler.register(type(_C().f), _reduce_method)


def _reduce_method_descriptor(m):
    return getattr, (m.__objclass__, m.__name__)
ForkingPickler.register(type(list.append), _reduce_method_descriptor)
ForkingPickler.register(type(int.__add__), _reduce_method_descriptor)

try:
    from functools import partial
except ImportError:
    pass
else:
    def _reduce_partial(p):
        return _rebuild_partial, (p.func, p.args, p.keywords or {})
    def _rebuild_partial(func, args, keywords):
        return partial(func, *args, **keywords)
    ForkingPickler.register(partial, _reduce_partial)

#
# Unix
#

if sys.platform != 'win32':
    import select

    exit = os._exit
    duplicate = os.dup
    close = os.close
    _select = util._eintr_retry(select.select)

    #
    # We define a Popen class similar to the one from subprocess, but
    # whose constructor takes a process object as its argument.
    #

    class Popen(object):

        def __init__(self, process_obj):
            sys.stdout.flush()
            sys.stderr.flush()
            self.returncode = None

            r, w = os.pipe()
            self.sentinel = r

            self.pid = os.fork()
            if self.pid == 0:
                os.close(r)
                if 'random' in sys.modules:
                    import random
                    random.seed()
                code = process_obj._bootstrap()
                os._exit(code)

            # `w` will be closed when the child exits, at which point `r`
            # will become ready for reading (using e.g. select()).
            os.close(w)
            util.Finalize(self, os.close, (r,))

        def poll(self, flag=os.WNOHANG):
            if self.returncode is None:
                try:
                    pid, sts = os.waitpid(self.pid, flag)
                except os.error:
                    # Child process not yet created. See #1731717
                    # e.errno == errno.ECHILD == 10
                    return None
                if pid == self.pid:
                    if os.WIFSIGNALED(sts):
                        self.returncode = -os.WTERMSIG(sts)
                    else:
                        assert os.WIFEXITED(sts)
                        self.returncode = os.WEXITSTATUS(sts)
            return self.returncode

        def wait(self, timeout=None):
            if self.returncode is None:
                if timeout is not None:
                    r = _select([self.sentinel], [], [], timeout)[0]
                    if not r:
                        return None
                # This shouldn't block if select() returned successfully.
                return self.poll(os.WNOHANG if timeout == 0.0 else 0)
            return self.returncode

        def terminate(self):
            if self.returncode is None:
                try:
                    os.kill(self.pid, signal.SIGTERM)
                except OSError:
                    if self.wait(timeout=0.1) is None:
                        raise

        @staticmethod
        def thread_is_spawning():
            return False

#
# Windows
#

else:
    import _thread
    import msvcrt
    import _winapi

    from pickle import load, HIGHEST_PROTOCOL

    def dump(obj, file, protocol=None):
        ForkingPickler(file, protocol).dump(obj)

    #
    #
    #

    TERMINATE = 0x10000
    WINEXE = (sys.platform == 'win32' and getattr(sys, 'frozen', False))
    WINSERVICE = sys.executable.lower().endswith("pythonservice.exe")

    exit = _winapi.ExitProcess
    close = _winapi.CloseHandle

    #
    # _python_exe is the assumed path to the python executable.
    # People embedding Python want to modify it.
    #

    if WINSERVICE:
        _python_exe = os.path.join(sys.exec_prefix, 'python.exe')
    else:
        _python_exe = sys.executable

    def set_executable(exe):
        global _python_exe
        _python_exe = exe

    #
    #
    #

    def duplicate(handle, target_process=None, inheritable=False):
        if target_process is None:
            target_process = _winapi.GetCurrentProcess()
        return _winapi.DuplicateHandle(
            _winapi.GetCurrentProcess(), handle, target_process,
            0, inheritable, _winapi.DUPLICATE_SAME_ACCESS
            )

    #
    # We define a Popen class similar to the one from subprocess, but
    # whose constructor takes a process object as its argument.
    #

    class Popen(object):
        '''
        Start a subprocess to run the code of a process object
        '''
        _tls = _thread._local()

        def __init__(self, process_obj):
            # create pipe for communication with child
            rfd, wfd = os.pipe()

            # get handle for read end of the pipe and make it inheritable
            rhandle = duplicate(msvcrt.get_osfhandle(rfd), inheritable=True)
            os.close(rfd)

            # start process
            cmd = get_command_line() + [rhandle]
            cmd = ' '.join('"%s"' % x for x in cmd)
            hp, ht, pid, tid = _winapi.CreateProcess(
                _python_exe, cmd, None, None, 1, 0, None, None, None
                )
            _winapi.CloseHandle(ht)
            close(rhandle)

            # set attributes of self
            self.pid = pid
            self.returncode = None
            self._handle = hp
            self.sentinel = int(hp)

            # send information to child
            prep_data = get_preparation_data(process_obj._name)
            to_child = os.fdopen(wfd, 'wb')
            Popen._tls.process_handle = int(hp)
            try:
                dump(prep_data, to_child, HIGHEST_PROTOCOL)
                dump(process_obj, to_child, HIGHEST_PROTOCOL)
            finally:
                del Popen._tls.process_handle
                to_child.close()

        @staticmethod
        def thread_is_spawning():
            return getattr(Popen._tls, 'process_handle', None) is not None

        @staticmethod
        def duplicate_for_child(handle):
            return duplicate(handle, Popen._tls.process_handle)

        def wait(self, timeout=None):
            if self.returncode is None:
                if timeout is None:
                    msecs = _winapi.INFINITE
                else:
                    msecs = max(0, int(timeout * 1000 + 0.5))

                res = _winapi.WaitForSingleObject(int(self._handle), msecs)
                if res == _winapi.WAIT_OBJECT_0:
                    code = _winapi.GetExitCodeProcess(self._handle)
                    if code == TERMINATE:
                        code = -signal.SIGTERM
                    self.returncode = code

            return self.returncode

        def poll(self):
            return self.wait(timeout=0)

        def terminate(self):
            if self.returncode is None:
                try:
                    _winapi.TerminateProcess(int(self._handle), TERMINATE)
                except WindowsError:
                    if self.wait(timeout=0.1) is None:
                        raise

    #
    #
    #

    def is_forking(argv):
        '''
        Return whether commandline indicates we are forking
        '''
        if len(argv) >= 2 and argv[1] == '--multiprocessing-fork':
            assert len(argv) == 3
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


    def get_command_line():
        '''
        Returns prefix of command line used for spawning a child process
        '''
        if process.current_process()._identity==() and is_forking(sys.argv):
            raise RuntimeError('''
            Attempt to start a new process before the current process
            has finished its bootstrapping phase.

            This probably means that you are on Windows and you have
            forgotten to use the proper idiom in the main module:

                if __name__ == '__main__':
                    freeze_support()
                    ...

            The "freeze_support()" line can be omitted if the program
            is not going to be frozen to produce a Windows executable.''')

        if getattr(sys, 'frozen', False):
            return [sys.executable, '--multiprocessing-fork']
        else:
            prog = 'from multiprocessing.forking import main; main()'
            return [_python_exe, '-c', prog, '--multiprocessing-fork']


    def main():
        '''
        Run code specifed by data received over pipe
        '''
        assert is_forking(sys.argv)

        handle = int(sys.argv[-1])
        fd = msvcrt.open_osfhandle(handle, os.O_RDONLY)
        from_parent = os.fdopen(fd, 'rb')

        process.current_process()._inheriting = True
        preparation_data = load(from_parent)
        prepare(preparation_data)
        self = load(from_parent)
        process.current_process()._inheriting = False

        from_parent.close()

        exitcode = self._bootstrap()
        exit(exitcode)


    def get_preparation_data(name):
        '''
        Return info about parent needed by child to unpickle process object
        '''
        from .util import _logger, _log_to_stderr

        d = dict(
            name=name,
            sys_path=sys.path,
            sys_argv=sys.argv,
            log_to_stderr=_log_to_stderr,
            orig_dir=process.ORIGINAL_DIR,
            authkey=process.current_process().authkey,
            )

        if _logger is not None:
            d['log_level'] = _logger.getEffectiveLevel()

        if not WINEXE and not WINSERVICE:
            main_path = getattr(sys.modules['__main__'], '__file__', None)
            if not main_path and sys.argv[0] not in ('', '-c'):
                main_path = sys.argv[0]
            if main_path is not None:
                if not os.path.isabs(main_path) and \
                                          process.ORIGINAL_DIR is not None:
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
    old_main_modules.append(sys.modules['__main__'])

    if 'name' in data:
        process.current_process().name = data['name']

    if 'authkey' in data:
        process.current_process()._authkey = data['authkey']

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

    if 'main_path' in data:
        # XXX (ncoghlan): The following code makes several bogus
        # assumptions regarding the relationship between __file__
        # and a module's real name. See PEP 302 and issue #10845
        main_path = data['main_path']
        main_name = os.path.splitext(os.path.basename(main_path))[0]
        if main_name == '__init__':
            main_name = os.path.basename(os.path.dirname(main_path))

        if main_name == '__main__':
            main_module = sys.modules['__main__']
            main_module.__file__ = main_path
        elif main_name != 'ipython':
            # Main modules not actually called __main__.py may
            # contain additional code that should still be executed
            import imp

            if main_path is None:
                dirs = None
            elif os.path.basename(main_path).startswith('__init__.py'):
                dirs = [os.path.dirname(os.path.dirname(main_path))]
            else:
                dirs = [os.path.dirname(main_path)]

            assert main_name not in sys.modules, main_name
            file, path_name, etc = imp.find_module(main_name, dirs)
            try:
                # We would like to do "imp.load_module('__main__', ...)"
                # here.  However, that would cause 'if __name__ ==
                # "__main__"' clauses to be executed.
                main_module = imp.load_module(
                    '__parents_main__', file, path_name, etc
                    )
            finally:
                if file:
                    file.close()

            sys.modules['__main__'] = main_module
            main_module.__name__ = '__main__'

            # Try to make the potentially picklable objects in
            # sys.modules['__main__'] realize they are in the main
            # module -- somewhat ugly.
            for obj in list(main_module.__dict__.values()):
                try:
                    if obj.__module__ == '__parents_main__':
                        obj.__module__ = '__main__'
                except Exception:
                    pass
