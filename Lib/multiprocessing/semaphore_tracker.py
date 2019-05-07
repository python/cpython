#
# On Unix we run a server process which keeps track of unlinked
# semaphores. The server ignores SIGINT and SIGTERM and reads from a
# pipe.  Every other process of the program has a copy of the writable
# end of the pipe, so we get EOF when all other processes have exited.
# Then the server process unlinks any remaining semaphore names.
#
# This is important because the system only supports a limited number
# of named semaphores, and they will not be automatically removed till
# the next reboot.  Without this semaphore tracker process, "killall
# python" would probably leave unlinked semaphores.
#

import os
import signal
import sys
import threading
import warnings
import _multiprocessing

from . import spawn
from . import util

__all__ = ['ensure_running', 'register', 'unregister']

_HAVE_SIGMASK = hasattr(signal, 'pthread_sigmask')
_IGNORED_SIGNALS = (signal.SIGINT, signal.SIGTERM)


class SemaphoreTracker(object):

    def __init__(self):
        self._lock = threading.Lock()
        self._fd = None
        self._pid = None

    def getfd(self):
        self.ensure_running()
        return self._fd

    def ensure_running(self):
        '''Make sure that semaphore tracker process is running.

        This can be run from any process.  Usually a child process will use
        the semaphore created by its parent.'''
        with self._lock:
            if self._fd is not None:
                # semaphore tracker was launched before, is it still running?
                if self._check_alive():
                    # => still alive
                    return
                # => dead, launch it again
                os.close(self._fd)

                # Clean-up to avoid dangling processes.
                try:
                    # _pid can be None if this process is a child from another
                    # python process, which has started the semaphore_tracker.
                    if self._pid is not None:
                        os.waitpid(self._pid, 0)
                except ChildProcessError:
                    # The semaphore_tracker has already been terminated.
                    pass
                self._fd = None
                self._pid = None

                warnings.warn('semaphore_tracker: process died unexpectedly, '
                              'relaunching.  Some semaphores might leak.')

            fds_to_pass = []
            try:
                fds_to_pass.append(sys.stderr.fileno())
            except Exception:
                pass
            cmd = 'from multiprocessing.semaphore_tracker import main;main(%d)'
            r, w = os.pipe()
            try:
                fds_to_pass.append(r)
                # process will out live us, so no need to wait on pid
                exe = spawn.get_executable()
                args = [exe] + util._args_from_interpreter_flags()
                args += ['-c', cmd % r]
                # bpo-33613: Register a signal mask that will block the signals.
                # This signal mask will be inherited by the child that is going
                # to be spawned and will protect the child from a race condition
                # that can make the child die before it registers signal handlers
                # for SIGINT and SIGTERM. The mask is unregistered after spawning
                # the child.
                try:
                    if _HAVE_SIGMASK:
                        signal.pthread_sigmask(signal.SIG_BLOCK, _IGNORED_SIGNALS)
                    pid = util.spawnv_passfds(exe, args, fds_to_pass)
                finally:
                    if _HAVE_SIGMASK:
                        signal.pthread_sigmask(signal.SIG_UNBLOCK, _IGNORED_SIGNALS)
            except:
                os.close(w)
                raise
            else:
                self._fd = w
                self._pid = pid
            finally:
                os.close(r)

    def _check_alive(self):
        '''Check that the pipe has not been closed by sending a probe.'''
        try:
            # We cannot use send here as it calls ensure_running, creating
            # a cycle.
            os.write(self._fd, b'PROBE:0\n')
        except OSError:
            return False
        else:
            return True

    def register(self, name):
        '''Register name of semaphore with semaphore tracker.'''
        self._send('REGISTER', name)

    def unregister(self, name):
        '''Unregister name of semaphore with semaphore tracker.'''
        self._send('UNREGISTER', name)

    def _send(self, cmd, name):
        self.ensure_running()
        msg = '{0}:{1}\n'.format(cmd, name).encode('ascii')
        if len(name) > 512:
            # posix guarantees that writes to a pipe of less than PIPE_BUF
            # bytes are atomic, and that PIPE_BUF >= 512
            raise ValueError('name too long')
        nbytes = os.write(self._fd, msg)
        assert nbytes == len(msg), "nbytes {0:n} but len(msg) {1:n}".format(
            nbytes, len(msg))


_semaphore_tracker = SemaphoreTracker()
ensure_running = _semaphore_tracker.ensure_running
register = _semaphore_tracker.register
unregister = _semaphore_tracker.unregister
getfd = _semaphore_tracker.getfd

def main(fd):
    '''Run semaphore tracker.'''
    # protect the process from ^C and "killall python" etc
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    signal.signal(signal.SIGTERM, signal.SIG_IGN)
    if _HAVE_SIGMASK:
        signal.pthread_sigmask(signal.SIG_UNBLOCK, _IGNORED_SIGNALS)

    for f in (sys.stdin, sys.stdout):
        try:
            f.close()
        except Exception:
            pass

    cache = set()
    try:
        # keep track of registered/unregistered semaphores
        with open(fd, 'rb') as f:
            for line in f:
                try:
                    cmd, name = line.strip().split(b':')
                    if cmd == b'REGISTER':
                        cache.add(name)
                    elif cmd == b'UNREGISTER':
                        cache.remove(name)
                    elif cmd == b'PROBE':
                        pass
                    else:
                        raise RuntimeError('unrecognized command %r' % cmd)
                except Exception:
                    try:
                        sys.excepthook(*sys.exc_info())
                    except:
                        pass
    finally:
        # all processes have terminated; cleanup any remaining semaphores
        if cache:
            try:
                warnings.warn('semaphore_tracker: There appear to be %d '
                              'leaked semaphores to clean up at shutdown' %
                              len(cache))
            except Exception:
                pass
        for name in cache:
            # For some reason the process which created and registered this
            # semaphore has failed to unregister it. Presumably it has died.
            # We therefore unlink it.
            try:
                name = name.decode('ascii')
                try:
                    _multiprocessing.sem_unlink(name)
                except Exception as e:
                    warnings.warn('semaphore_tracker: %r: %s' % (name, e))
            finally:
                pass
