###############################################################################
# Server process to keep track of unlinked resources (like shared memory
# segments, semaphores etc.) and clean them.
#
# On Unix we run a server process which keeps track of unlinked
# resources. The server ignores SIGINT and SIGTERM and reads from a
# pipe.  Every other process of the program has a copy of the writable
# end of the pipe, so we get EOF when all other processes have exited.
# Then the server process unlinks any remaining resource names.
#
# This is important because there may be system limits for such resources: for
# instance, the system only supports a limited number of named semaphores, and
# shared-memory segments live in the RAM. If a python process leaks such a
# resource, this resource will not be removed till the next reboot.  Without
# this resource tracker process, "killall python" would probably leave unlinked
# resources.

import base64
import os
import signal
import sys
import threading
import time
import warnings
from collections import deque

import json

from . import spawn
from . import util

__all__ = ['ensure_running', 'register', 'unregister']

_HAVE_SIGMASK = hasattr(signal, 'pthread_sigmask')
_IGNORED_SIGNALS = (signal.SIGINT, signal.SIGTERM)

def cleanup_noop(name):
    raise RuntimeError('noop should never be registered or cleaned up')

_CLEANUP_FUNCS = {
    'noop': cleanup_noop,
    'dummy': lambda name: None,  # Dummy resource used in tests
}

if os.name == 'posix':
    import _multiprocessing
    import _posixshmem

    # Use sem_unlink() to clean up named semaphores.
    #
    # sem_unlink() may be missing if the Python build process detected the
    # absence of POSIX named semaphores. In that case, no named semaphores were
    # ever opened, so no cleanup would be necessary.
    if hasattr(_multiprocessing, 'sem_unlink'):
        _CLEANUP_FUNCS['semaphore'] = _multiprocessing.sem_unlink
    _CLEANUP_FUNCS['shared_memory'] = _posixshmem.shm_unlink


class ReentrantCallError(RuntimeError):
    pass


class ResourceTracker(object):

    def __init__(self):
        self._lock = threading.RLock()
        self._fd = None
        self._pid = None
        self._exitcode = None
        self._reentrant_messages = deque()

        # True to use colon-separated lines, rather than JSON lines,
        # for internal communication. (Mainly for testing).
        # Filenames not supported by the simple format will always be sent
        # using JSON.
        # The reader should understand all formats.
        self._use_simple_format = False

        # Set to True by _stop_locked() if the waitpid polling loop ran to
        # its timeout without reaping the tracker.  Exposed for tests.
        self._waitpid_timed_out = False

    def _reentrant_call_error(self):
        # gh-109629: this happens if an explicit call to the ResourceTracker
        # gets interrupted by a garbage collection, invoking a finalizer (*)
        # that itself calls back into ResourceTracker.
        #   (*) for example the SemLock finalizer
        raise ReentrantCallError(
            "Reentrant call into the multiprocessing resource tracker")

    def __del__(self):
        # making sure child processess are cleaned before ResourceTracker
        # gets destructed.
        # see https://github.com/python/cpython/issues/88887
        # gh-146313: use a timeout to avoid deadlocking if a forked child
        # still holds the pipe's write end open.
        self._stop(use_blocking_lock=False, wait_timeout=1.0)

    def _after_fork_in_child(self):
        # gh-146313: Called in the child right after os.fork().
        #
        # The tracker process is a child of the *parent*, not of us, so we
        # could never waitpid() it anyway.  Clearing _pid means our __del__
        # becomes a no-op (the early return for _pid is None).
        #
        # Whether we keep the inherited _fd depends on who forked us:
        #
        #   - multiprocessing.Process with the 'fork' start method sets
        #     _fork_intent.preserve_fd before forking.  The child keeps the
        #     fd and reuses the parent's tracker (gh-80849).  This is safe
        #     because multiprocessing's atexit handler joins all children
        #     before the parent's __del__ runs, so by then the fd copies
        #     are gone and the parent can reap the tracker promptly.
        #
        #   - A raw os.fork() leaves the flag unset.  We close the fd in the child after forking so
        #     the parent's __del__ can reap the tracker without waiting
        #     for the child to exit.  If we later need a tracker, ensure_running()
        #     will launch a fresh one.
        self._lock._at_fork_reinit()
        self._reentrant_messages.clear()
        self._pid = None
        self._exitcode = None
        if (self._fd is not None and
            not getattr(_fork_intent, 'preserve_fd', False)):
            fd = self._fd
            self._fd = None
            try:
                os.close(fd)
            except OSError:
                pass

    def _stop(self, use_blocking_lock=True, wait_timeout=None):
        if use_blocking_lock:
            with self._lock:
                self._stop_locked(wait_timeout=wait_timeout)
        else:
            acquired = self._lock.acquire(blocking=False)
            try:
                self._stop_locked(wait_timeout=wait_timeout)
            finally:
                if acquired:
                    self._lock.release()

    def _stop_locked(
        self,
        close=os.close,
        waitpid=os.waitpid,
        waitstatus_to_exitcode=os.waitstatus_to_exitcode,
        monotonic=time.monotonic,
        sleep=time.sleep,
        WNOHANG=getattr(os, 'WNOHANG', None),
        wait_timeout=None,
    ):
        # This shouldn't happen (it might when called by a finalizer)
        # so we check for it anyway.
        if self._lock._recursion_count() > 1:
            raise self._reentrant_call_error()
        if self._fd is None:
            # not running
            return
        if self._pid is None:
            return

        # closing the "alive" file descriptor stops main()
        close(self._fd)
        self._fd = None

        try:
            if wait_timeout is None:
                _, status = waitpid(self._pid, 0)
            else:
                # gh-146313: A forked child may still hold the pipe's write
                # end open, preventing the tracker from seeing EOF and
                # exiting.  Poll with WNOHANG to avoid blocking forever.
                deadline = monotonic() + wait_timeout
                delay = 0.001
                while True:
                    result_pid, status = waitpid(self._pid, WNOHANG)
                    if result_pid != 0:
                        break
                    remaining = deadline - monotonic()
                    if remaining <= 0:
                        # The tracker is still running; it will be
                        # reparented to PID 1 (or the nearest subreaper)
                        # when we exit, and reaped there once all pipe
                        # holders release their fd.
                        self._pid = None
                        self._exitcode = None
                        self._waitpid_timed_out = True
                        return
                    delay = min(delay * 2, remaining, 0.1)
                    sleep(delay)
        except ChildProcessError:
            self._pid = None
            self._exitcode = None
            return

        self._pid = None

        try:
            self._exitcode = waitstatus_to_exitcode(status)
        except ValueError:
            # os.waitstatus_to_exitcode may raise an exception for invalid values
            self._exitcode = None

    def getfd(self):
        self.ensure_running()
        return self._fd

    def ensure_running(self):
        '''Make sure that resource tracker process is running.

        This can be run from any process.  Usually a child process will use
        the resource created by its parent.'''
        return self._ensure_running_and_write()

    def _teardown_dead_process(self):
        os.close(self._fd)

        # Clean-up to avoid dangling processes.
        try:
            # _pid can be None if this process is a child from another
            # python process, which has started the resource_tracker.
            if self._pid is not None:
                os.waitpid(self._pid, 0)
        except ChildProcessError:
            # The resource_tracker has already been terminated.
            pass
        self._fd = None
        self._pid = None
        self._exitcode = None

        warnings.warn('resource_tracker: process died unexpectedly, '
                      'relaunching.  Some resources might leak.')

    def _launch(self):
        fds_to_pass = []
        try:
            fds_to_pass.append(sys.stderr.fileno())
        except Exception:
            pass
        r, w = os.pipe()
        try:
            fds_to_pass.append(r)
            # process will out live us, so no need to wait on pid
            exe = spawn.get_executable()
            args = [
                exe,
                *util._args_from_interpreter_flags(),
                '-c',
                f'from multiprocessing.resource_tracker import main;main({r})',
            ]
            # bpo-33613: Register a signal mask that will block the signals.
            # This signal mask will be inherited by the child that is going
            # to be spawned and will protect the child from a race condition
            # that can make the child die before it registers signal handlers
            # for SIGINT and SIGTERM. The mask is unregistered after spawning
            # the child.
            prev_sigmask = None
            try:
                if _HAVE_SIGMASK:
                    prev_sigmask = signal.pthread_sigmask(signal.SIG_BLOCK, _IGNORED_SIGNALS)
                pid = util.spawnv_passfds(exe, args, fds_to_pass)
            finally:
                if prev_sigmask is not None:
                    signal.pthread_sigmask(signal.SIG_SETMASK, prev_sigmask)
        except:
            os.close(w)
            raise
        else:
            self._fd = w
            self._pid = pid
        finally:
            os.close(r)

    def _make_probe_message(self):
        """Return a probe message."""
        if self._use_simple_format:
            return b'PROBE:0:noop\n'
        return (
            json.dumps(
                {"cmd": "PROBE", "rtype": "noop"},
                ensure_ascii=True,
                separators=(",", ":"),
            )
            + "\n"
        ).encode("ascii")

    def _ensure_running_and_write(self, msg=None):
        with self._lock:
            if self._lock._recursion_count() > 1:
                # The code below is certainly not reentrant-safe, so bail out
                if msg is None:
                    raise self._reentrant_call_error()
                return self._reentrant_messages.append(msg)

            if self._fd is not None:
                # resource tracker was launched before, is it still running?
                if msg is None:
                    to_send = self._make_probe_message()
                else:
                    to_send = msg
                try:
                    self._write(to_send)
                except OSError:
                    self._teardown_dead_process()
                    self._launch()

                msg = None  # message was sent in probe
            else:
                self._launch()

        while True:
            try:
                reentrant_msg = self._reentrant_messages.popleft()
            except IndexError:
                break
            self._write(reentrant_msg)
        if msg is not None:
            self._write(msg)

    def _check_alive(self):
        '''Check that the pipe has not been closed by sending a probe.'''
        try:
            # We cannot use send here as it calls ensure_running, creating
            # a cycle.
            os.write(self._fd, self._make_probe_message())
        except OSError:
            return False
        else:
            return True

    def register(self, name, rtype):
        '''Register name of resource with resource tracker.'''
        self._send('REGISTER', name, rtype)

    def unregister(self, name, rtype):
        '''Unregister name of resource with resource tracker.'''
        self._send('UNREGISTER', name, rtype)

    def _write(self, msg):
        nbytes = os.write(self._fd, msg)
        assert nbytes == len(msg), f"{nbytes=} != {len(msg)=}"

    def _send(self, cmd, name, rtype):
        if self._use_simple_format and '\n' not in name:
            msg = f"{cmd}:{name}:{rtype}\n".encode("ascii")
            if len(msg) > 512:
                # posix guarantees that writes to a pipe of less than PIPE_BUF
                # bytes are atomic, and that PIPE_BUF >= 512
                raise ValueError('msg too long')
            self._ensure_running_and_write(msg)
            return

        # POSIX guarantees that writes to a pipe of less than PIPE_BUF (512 on Linux)
        # bytes are atomic. Therefore, we want the message to be shorter than 512 bytes.
        # POSIX shm_open() and sem_open() require the name, including its leading slash,
        # to be at most NAME_MAX bytes (255 on Linux)
        # With json.dump(..., ensure_ascii=True) every non-ASCII byte becomes a 6-char
        # escape like \uDC80.
        # As we want the overall message to be kept atomic and therefore smaller than 512,
        # we encode encode the raw name bytes with URL-safe Base64 - so a 255 long name
        # will not exceed 340 bytes.
        b = name.encode('utf-8', 'surrogateescape')
        if len(b) > 255:
            raise ValueError('shared memory name too long (max 255 bytes)')
        b64 = base64.urlsafe_b64encode(b).decode('ascii')

        payload = {"cmd": cmd, "rtype": rtype, "base64_name": b64}
        msg = (json.dumps(payload, ensure_ascii=True, separators=(",", ":")) + "\n").encode("ascii")

        # The entire JSON message is guaranteed < PIPE_BUF (512 bytes) by construction.
        assert len(msg) <= 512, f"internal error: message too long ({len(msg)} bytes)"
        assert msg.startswith(b'{')

        self._ensure_running_and_write(msg)

# gh-146313: Per-thread flag set by .popen_fork.Popen._launch() just before
# os.fork(), telling _after_fork_in_child() to keep the inherited pipe fd so
# the child can reuse this tracker (gh-80849).  Unset for raw os.fork() calls,
# where the child instead closes the fd so the parent's __del__ can reap the
# tracker.  Using threading.local() keeps multiple threads calling
# popen_fork.Popen._launch() at once from clobbering eachothers intent.
_fork_intent = threading.local()

_resource_tracker = ResourceTracker()
ensure_running = _resource_tracker.ensure_running
register = _resource_tracker.register
unregister = _resource_tracker.unregister
getfd = _resource_tracker.getfd

# gh-146313: See _after_fork_in_child docstring.
if hasattr(os, 'register_at_fork'):
    os.register_at_fork(after_in_child=_resource_tracker._after_fork_in_child)


def _decode_message(line):
    if line.startswith(b'{'):
        try:
            obj = json.loads(line.decode('ascii'))
        except Exception as e:
            raise ValueError("malformed resource_tracker message: %r" % (line,)) from e

        cmd = obj["cmd"]
        rtype = obj["rtype"]
        b64  = obj.get("base64_name", "")

        if not isinstance(cmd, str) or not isinstance(rtype, str) or not isinstance(b64, str):
            raise ValueError("malformed resource_tracker fields: %r" % (obj,))

        try:
            name = base64.urlsafe_b64decode(b64).decode('utf-8', 'surrogateescape')
        except ValueError as e:
            raise ValueError("malformed resource_tracker base64_name: %r" % (b64,)) from e
    else:
        cmd, rest = line.strip().decode('ascii').split(':', maxsplit=1)
        name, rtype = rest.rsplit(':', maxsplit=1)
    return cmd, rtype, name


def main(fd):
    '''Run resource tracker.'''
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

    cache = {rtype: set() for rtype in _CLEANUP_FUNCS.keys()}
    exit_code = 0

    try:
        # keep track of registered/unregistered resources
        with open(fd, 'rb') as f:
            for line in f:
                try:
                    cmd, rtype, name = _decode_message(line)
                    cleanup_func = _CLEANUP_FUNCS.get(rtype, None)
                    if cleanup_func is None:
                        raise ValueError(
                            f'Cannot register {name} for automatic cleanup: '
                            f'unknown resource type {rtype}')

                    if cmd == 'REGISTER':
                        cache[rtype].add(name)
                    elif cmd == 'UNREGISTER':
                        cache[rtype].remove(name)
                    elif cmd == 'PROBE':
                        pass
                    else:
                        raise RuntimeError('unrecognized command %r' % cmd)
                except Exception:
                    exit_code = 3
                    try:
                        sys.excepthook(*sys.exc_info())
                    except:
                        pass
    finally:
        # all processes have terminated; cleanup any remaining resources
        for rtype, rtype_cache in cache.items():
            if rtype_cache:
                try:
                    exit_code = 1
                    if rtype == 'dummy':
                        # The test 'dummy' resource is expected to leak.
                        # We skip the warning (and *only* the warning) for it.
                        pass
                    else:
                        warnings.warn(
                            f'resource_tracker: There appear to be '
                            f'{len(rtype_cache)} leaked {rtype} objects to '
                            f'clean up at shutdown: {rtype_cache}'
                        )
                except Exception:
                    pass
            for name in rtype_cache:
                # For some reason the process which created and registered this
                # resource has failed to unregister it. Presumably it has
                # died.  We therefore unlink it.
                try:
                    try:
                        _CLEANUP_FUNCS[rtype](name)
                    except Exception as e:
                        exit_code = 2
                        warnings.warn('resource_tracker: %r: %s' % (name, e))
                finally:
                    pass

        sys.exit(exit_code)
