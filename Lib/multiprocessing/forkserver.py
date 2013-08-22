import errno
import os
import select
import signal
import socket
import struct
import sys
import threading

from . import connection
from . import process
from . import reduction
from . import spawn
from . import util

__all__ = ['ensure_running', 'get_inherited_fds', 'connect_to_new_process',
           'set_forkserver_preload']

#
#
#

MAXFDS_TO_SEND = 256
UNSIGNED_STRUCT = struct.Struct('Q')     # large enough for pid_t

_forkserver_address = None
_forkserver_alive_fd = None
_inherited_fds = None
_lock = threading.Lock()
_preload_modules = ['__main__']

#
# Public function
#

def set_forkserver_preload(modules_names):
    '''Set list of module names to try to load in forkserver process.'''
    global _preload_modules
    _preload_modules = modules_names


def get_inherited_fds():
    '''Return list of fds inherited from parent process.

    This returns None if the current process was not started by fork server.
    '''
    return _inherited_fds


def connect_to_new_process(fds):
    '''Request forkserver to create a child process.

    Returns a pair of fds (status_r, data_w).  The calling process can read
    the child process's pid and (eventually) its returncode from status_r.
    The calling process should write to data_w the pickled preparation and
    process data.
    '''
    if len(fds) + 3 >= MAXFDS_TO_SEND:
        raise ValueError('too many fds')
    with socket.socket(socket.AF_UNIX) as client:
        client.connect(_forkserver_address)
        parent_r, child_w = util.pipe()
        child_r, parent_w = util.pipe()
        allfds = [child_r, child_w, _forkserver_alive_fd]
        allfds += fds
        try:
            reduction.sendfds(client, allfds)
            return parent_r, parent_w
        except:
            os.close(parent_r)
            os.close(parent_w)
            raise
        finally:
            os.close(child_r)
            os.close(child_w)


def ensure_running():
    '''Make sure that a fork server is running.

    This can be called from any process.  Note that usually a child
    process will just reuse the forkserver started by its parent, so
    ensure_running() will do nothing.
    '''
    global _forkserver_address, _forkserver_alive_fd
    with _lock:
        if _forkserver_alive_fd is not None:
            return

        assert all(type(mod) is str for mod in _preload_modules)
        config = process.current_process()._config
        semaphore_tracker_fd = config['semaphore_tracker_fd']
        cmd = ('from multiprocessing.forkserver import main; ' +
               'main(%d, %d, %r, **%r)')

        if _preload_modules:
            desired_keys = {'main_path', 'sys_path'}
            data = spawn.get_preparation_data('ignore')
            data = dict((x,y) for (x,y) in data.items() if x in desired_keys)
        else:
            data = {}

        with socket.socket(socket.AF_UNIX) as listener:
            address = connection.arbitrary_address('AF_UNIX')
            listener.bind(address)
            os.chmod(address, 0o600)
            listener.listen(100)

            # all client processes own the write end of the "alive" pipe;
            # when they all terminate the read end becomes ready.
            alive_r, alive_w = util.pipe()
            try:
                fds_to_pass = [listener.fileno(), alive_r, semaphore_tracker_fd]
                cmd %= (listener.fileno(), alive_r, _preload_modules, data)
                exe = spawn.get_executable()
                args = [exe] + util._args_from_interpreter_flags() + ['-c', cmd]
                pid = util.spawnv_passfds(exe, args, fds_to_pass)
            except:
                os.close(alive_w)
                raise
            finally:
                os.close(alive_r)
            _forkserver_address = address
            _forkserver_alive_fd = alive_w


def main(listener_fd, alive_r, preload, main_path=None, sys_path=None):
    '''Run forkserver.'''
    if preload:
        if '__main__' in preload and main_path is not None:
            process.current_process()._inheriting = True
            try:
                spawn.import_main_path(main_path)
            finally:
                del process.current_process()._inheriting
        for modname in preload:
            try:
                __import__(modname)
            except ImportError:
                pass

    # close sys.stdin
    if sys.stdin is not None:
        try:
            sys.stdin.close()
            sys.stdin = open(os.devnull)
        except (OSError, ValueError):
            pass

    # ignoring SIGCHLD means no need to reap zombie processes
    handler = signal.signal(signal.SIGCHLD, signal.SIG_IGN)
    with socket.socket(socket.AF_UNIX, fileno=listener_fd) as listener:
        global _forkserver_address
        _forkserver_address = listener.getsockname()
        readers = [listener, alive_r]

        while True:
            try:
                rfds, wfds, xfds = select.select(readers, [], [])

                if alive_r in rfds:
                    # EOF because no more client processes left
                    assert os.read(alive_r, 1) == b''
                    raise SystemExit

                assert listener in rfds
                with listener.accept()[0] as s:
                    code = 1
                    if os.fork() == 0:
                        try:
                            _serve_one(s, listener, alive_r, handler)
                        except Exception:
                            sys.excepthook(*sys.exc_info())
                            sys.stderr.flush()
                        finally:
                            os._exit(code)

            except InterruptedError:
                pass
            except OSError as e:
                if e.errno != errno.ECONNABORTED:
                    raise

#
# Code to bootstrap new process
#

def _serve_one(s, listener, alive_r, handler):
    global _inherited_fds, _forkserver_alive_fd

    # close unnecessary stuff and reset SIGCHLD handler
    listener.close()
    os.close(alive_r)
    signal.signal(signal.SIGCHLD, handler)

    # receive fds from parent process
    fds = reduction.recvfds(s, MAXFDS_TO_SEND + 1)
    s.close()
    assert len(fds) <= MAXFDS_TO_SEND
    child_r, child_w, _forkserver_alive_fd, *_inherited_fds = fds

    # send pid to client processes
    write_unsigned(child_w, os.getpid())

    # reseed random number generator
    if 'random' in sys.modules:
        import random
        random.seed()

    # run process object received over pipe
    code = spawn._main(child_r)

    # write the exit code to the pipe
    write_unsigned(child_w, code)

#
# Read and write unsigned numbers
#

def read_unsigned(fd):
    data = b''
    length = UNSIGNED_STRUCT.size
    while len(data) < length:
        while True:
            try:
                s = os.read(fd, length - len(data))
            except InterruptedError:
                pass
            else:
                break
        if not s:
            raise EOFError('unexpected EOF')
        data += s
    return UNSIGNED_STRUCT.unpack(data)[0]

def write_unsigned(fd, n):
    msg = UNSIGNED_STRUCT.pack(n)
    while msg:
        while True:
            try:
                nbytes = os.write(fd, msg)
            except InterruptedError:
                pass
            else:
                break
        if nbytes == 0:
            raise RuntimeError('should not get here')
        msg = msg[nbytes:]
