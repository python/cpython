import os
import msvcrt
import signal
import sys
import _winapi

from .context import reduction, get_spawning_popen, set_spawning_popen
from . import spawn
from . import util

__all__ = ['Popen']

#
#
#

TERMINATE = 0x10000
WINEXE = (sys.platform == 'win32' and getattr(sys, 'frozen', False))
WINSERVICE = sys.executable.lower().endswith("pythonservice.exe")

#
# We define a Popen class similar to the one from subprocess, but
# whose constructor takes a process object as its argument.
#

class Popen(object):
    '''
    Start a subprocess to run the code of a process object
    '''
    method = 'spawn'

    def __init__(self, process_obj):
        prep_data = spawn.get_preparation_data(process_obj._name)

        # read end of pipe will be "stolen" by the child process
        # -- see spawn_main() in spawn.py.
        rhandle, whandle = _winapi.CreatePipe(None, 0)
        wfd = msvcrt.open_osfhandle(whandle, 0)
        cmd = spawn.get_command_line(parent_pid=os.getpid(),
                                     pipe_handle=rhandle)
        cmd = ' '.join('"%s"' % x for x in cmd)

        with open(wfd, 'wb', closefd=True) as to_child:
            # start process
            try:
                hp, ht, pid, tid = _winapi.CreateProcess(
                    spawn.get_executable(), cmd,
                    None, None, False, 0, None, None, None)
                _winapi.CloseHandle(ht)
            except:
                _winapi.CloseHandle(rhandle)
                raise

            # set attributes of self
            self.pid = pid
            self.returncode = None
            self._handle = hp
            self.sentinel = int(hp)
            self.finalizer = util.Finalize(self, _winapi.CloseHandle, (self.sentinel,))

            # send information to child
            set_spawning_popen(self)
            try:
                reduction.dump(prep_data, to_child)
                reduction.dump(process_obj, to_child)
            finally:
                set_spawning_popen(None)

    def duplicate_for_child(self, handle):
        assert self is get_spawning_popen()
        return reduction.duplicate(handle, self.sentinel)

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
            except OSError:
                if self.wait(timeout=1.0) is None:
                    raise

    kill = terminate

    def close(self):
        self.finalizer()
