import atexit
import os
import signal
import threading

from . import util

__all__ = ['Popen']

#
# Start child process using fork
#

class Popen(object):
    method = 'fork'

    def __init__(self, process_obj):
        util._flush_std_streams()
        self.returncode = None
        self.finalizer = None
        self._exit_condition = threading.Condition()
        self._exit_blockers = 0
        self._launch(process_obj)

    def duplicate_for_child(self, fd):
        return fd

    def poll(self, flag=os.WNOHANG):
        with self._exit_condition:
            if self.returncode is not None:
                return self.returncode
            elif flag & os.WNOHANG == os.WNOHANG:
                return self._nonblocking_poll(flag)
            else:
                self._exit_blockers += 1

        # We have released the lock, so may be racing with blocking &
        # non-blocking calls at this point...
        pid = None
        try:
            pid, sts = os.waitpid(self.pid, flag)
        except OSError:
            # Child process doesn't exist because it hasn't started yet (see
            # bpo-1731717) or has already been awaited on a racing thread (see
            # gh-130895)
            pass

        with self._exit_condition:
            self._exit_blockers -= 1
            if pid == self.pid:
                self._set_returncode(sts)
            elif self._exit_blockers == 0:
                self._exit_condition.notify_all()

            # Wait until we get a definitive result, or we know there are no
            # racing calls that might be about to set it
            while self.returncode is None and self._exit_blockers > 0:
                self._exit_condition.wait()

            return self.returncode

    def _nonblocking_poll(self, flag):
        assert self._exit_condition._is_owned()
        assert self.returncode is None
        assert flag & os.WNOHANG == os.WNOHANG
        try:
            pid, sts = os.waitpid(self.pid, flag)
            if pid == self.pid:
                self._set_returncode(sts)
        except OSError:
            # See comments in the poll(...) except clause above
            pass

        # We may be racing with a blocking wait call, in which case (if we lose
        # the race) it is arbitrary whether this returns None or the exit code
        # (if there is one): calling code must always be prepared to handle a
        # situation where this method returns None but the process has ended.
        return self.returncode

    def _set_returncode(self, sts):
        assert self._exit_condition._is_owned()
        assert self.returncode is None
        self.returncode = os.waitstatus_to_exitcode(sts)
        self._exit_condition.notify_all()

    def wait(self, timeout=None):
        if self.returncode is None:
            if timeout is not None:
                from multiprocessing.connection import wait
                if not wait([self.sentinel], timeout):
                    return None
            # This shouldn't block if wait() returned successfully.
            return self.poll(os.WNOHANG if timeout == 0.0 else 0)
        return self.returncode

    def _send_signal(self, sig):
        if self.returncode is None:
            try:
                os.kill(self.pid, sig)
            except ProcessLookupError:
                pass
            except OSError:
                if self.wait(timeout=0.1) is None:
                    raise

    def interrupt(self):
        self._send_signal(signal.SIGINT)

    def terminate(self):
        self._send_signal(signal.SIGTERM)

    def kill(self):
        self._send_signal(signal.SIGKILL)

    def _launch(self, process_obj):
        code = 1
        parent_r, child_w = os.pipe()
        child_r, parent_w = os.pipe()
        self.pid = os.fork()
        if self.pid == 0:
            try:
                atexit._clear()
                atexit.register(util._exit_function)
                os.close(parent_r)
                os.close(parent_w)
                code = process_obj._bootstrap(parent_sentinel=child_r)
            finally:
                atexit._run_exitfuncs()
                os._exit(code)
        else:
            os.close(child_w)
            os.close(child_r)
            self.finalizer = util.Finalize(self, util.close_fds,
                                           (parent_r, parent_w,))
            self.sentinel = parent_r

    def close(self):
        if self.finalizer is not None:
            self.finalizer()
