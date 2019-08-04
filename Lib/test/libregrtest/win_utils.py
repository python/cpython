import _winapi
import msvcrt
import os
import subprocess
import uuid
from test import support


# Max size of asynchronous reads
BUFSIZE = 8192
# Exponential damping factor (see below)
LOAD_FACTOR_1 = 0.9200444146293232478931553241
# Seconds per measurement
SAMPLING_INTERVAL = 5
COUNTER_NAME = r'\System\Processor Queue Length'


class WindowsLoadTracker():
    """
    This class asynchronously interacts with the `typeperf` command to read
    the system load on Windows. Multiprocessing and threads can't be used
    here because they interfere with the test suite's cases for those
    modules.
    """

    def __init__(self):
        self.load = 0.0
        self.start()

    def start(self):
        # Create a named pipe which allows for asynchronous IO in Windows
        pipe_name =  r'\\.\pipe\typeperf_output_' + str(uuid.uuid4())

        open_mode =  _winapi.PIPE_ACCESS_INBOUND
        open_mode |= _winapi.FILE_FLAG_FIRST_PIPE_INSTANCE
        open_mode |= _winapi.FILE_FLAG_OVERLAPPED

        # This is the read end of the pipe, where we will be grabbing output
        self.pipe = _winapi.CreateNamedPipe(
            pipe_name, open_mode, _winapi.PIPE_WAIT,
            1, BUFSIZE, BUFSIZE, _winapi.NMPWAIT_WAIT_FOREVER, _winapi.NULL
        )
        # The write end of the pipe which is passed to the created process
        pipe_write_end = _winapi.CreateFile(
            pipe_name, _winapi.GENERIC_WRITE, 0, _winapi.NULL,
            _winapi.OPEN_EXISTING, 0, _winapi.NULL
        )
        # Open up the handle as a python file object so we can pass it to
        # subprocess
        command_stdout = msvcrt.open_osfhandle(pipe_write_end, 0)

        # Connect to the read end of the pipe in overlap/async mode
        overlap = _winapi.ConnectNamedPipe(self.pipe, overlapped=True)
        overlap.GetOverlappedResult(True)

        # Spawn off the load monitor
        command = ['typeperf', COUNTER_NAME, '-si', str(SAMPLING_INTERVAL)]
        self.p = subprocess.Popen(command, stdout=command_stdout, cwd=support.SAVEDCWD)

        # Close our copy of the write end of the pipe
        os.close(command_stdout)

    def close(self):
        if self.p is None:
            return
        self.p.kill()
        self.p.wait()
        self.p = None

    def __del__(self):
        self.close()

    def read_output(self):
        import _winapi

        overlapped, _ = _winapi.ReadFile(self.pipe, BUFSIZE, True)
        bytes_read, res = overlapped.GetOverlappedResult(False)
        if res != 0:
            return

        return overlapped.getbuffer().decode()

    def getloadavg(self):
        typeperf_output = self.read_output()
        # Nothing to update, just return the current load
        if not typeperf_output:
            return self.load

        # Process the backlog of load values
        for line in typeperf_output.splitlines():
            # typeperf outputs in a CSV format like this:
            # "07/19/2018 01:32:26.605","3.000000"
            toks = line.split(',')
            # Ignore blank lines and the initial header
            if line.strip() == '' or (COUNTER_NAME in line) or len(toks) != 2:
                continue

            load = float(toks[1].replace('"', ''))
            # We use an exponentially weighted moving average, imitating the
            # load calculation on Unix systems.
            # https://en.wikipedia.org/wiki/Load_(computing)#Unix-style_load_calculation
            new_load = self.load * LOAD_FACTOR_1 + load * (1.0 - LOAD_FACTOR_1)
            self.load = new_load

        return self.load
