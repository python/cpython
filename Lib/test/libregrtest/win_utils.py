import _winapi
import msvcrt
import os
import subprocess
import uuid
import winreg
from test import support
from test.libregrtest.utils import print_warning


# Max size of asynchronous reads
BUFSIZE = 8192
# Exponential damping factor (see below)
LOAD_FACTOR_1 = 0.9200444146293232478931553241

# Seconds per measurement
SAMPLING_INTERVAL = 5
# Windows registry subkey of HKEY_LOCAL_MACHINE where the counter names
# of typeperf are registered
COUNTER_REGISTRY_KEY = (r"SOFTWARE\Microsoft\Windows NT\CurrentVersion"
                        r"\Perflib\CurrentLanguage")


class WindowsLoadTracker():
    """
    This class asynchronously interacts with the `typeperf` command to read
    the system load on Windows. Multiprocessing and threads can't be used
    here because they interfere with the test suite's cases for those
    modules.
    """

    def __init__(self):
        self.load = 0.0
        self.counter_name = ''
        self._buffer = b''
        self.popen = None
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
        counter_name = self._get_counter_name()
        command = ['typeperf', counter_name, '-si', str(SAMPLING_INTERVAL)]
        self.popen = subprocess.Popen(' '.join(command), stdout=command_stdout, cwd=support.SAVEDCWD)

        # Close our copy of the write end of the pipe
        os.close(command_stdout)

    def _get_counter_name(self):
        # accessing the registry to get the counter localization name
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, COUNTER_REGISTRY_KEY) as perfkey:
            counters = winreg.QueryValueEx(perfkey, 'Counter')[0]

        # Convert [key1, value1, key2, value2, ...] list
        # to {key1: value1, key2: value2, ...} dict
        counters = iter(counters)
        counters_dict = dict(zip(counters, counters))

        # System counter has key '2' and Processor Queue Length has key '44'
        system = counters_dict['2']
        process_queue_length = counters_dict['44']
        return f'"\\{system}\\{process_queue_length}"'

    def close(self):
        if self.popen is None:
            return
        self.popen.kill()
        self.popen.wait()
        self.popen = None

    def __del__(self):
        self.close()

    def read_output(self):
        overlapped, _ = _winapi.ReadFile(self.pipe, BUFSIZE, True)
        bytes_read, res = overlapped.GetOverlappedResult(False)
        if res != 0:
            return

        # self._buffer stores an incomplete line
        output = self._buffer + overlapped.getbuffer()
        output, _, self._buffer = output.rpartition(b'\n')
        return output.decode('oem', 'replace')

    def getloadavg(self):
        typeperf_output = self.read_output()
        # Nothing to update, just return the current load
        if not typeperf_output:
            return self.load

        # Process the backlog of load values
        for line in typeperf_output.splitlines():
            # Ignore the initial header:
            # "(PDH-CSV 4.0)","\\\\WIN\\System\\Processor Queue Length"
            if '\\\\' in line:
                continue

            # Ignore blank lines
            if not line.strip():
                continue

            # typeperf outputs in a CSV format like this:
            # "07/19/2018 01:32:26.605","3.000000"
            # (date, process queue length)
            try:
                tokens = line.split(',')
                if len(tokens) != 2:
                    raise ValueError

                value = tokens[1].replace('"', '')
                load = float(value)
            except ValueError:
                print_warning("Failed to parse typeperf output: %a" % line)
                continue

            # We use an exponentially weighted moving average, imitating the
            # load calculation on Unix systems.
            # https://en.wikipedia.org/wiki/Load_(computing)#Unix-style_load_calculation
            new_load = self.load * LOAD_FACTOR_1 + load * (1.0 - LOAD_FACTOR_1)
            self.load = new_load

        return self.load
