import _winapi
import math
import msvcrt
import os
import subprocess
import uuid
import winreg
from test.support import os_helper
from test.libregrtest.utils import print_warning


# Max size of asynchronous reads
BUFSIZE = 8192
# Seconds per measurement
SAMPLING_INTERVAL = 1
# Exponential damping factor to compute exponentially weighted moving average
# on 1 minute (60 seconds)
LOAD_FACTOR_1 = 1 / math.exp(SAMPLING_INTERVAL / 60)
# Initialize the load using the arithmetic mean of the first NVALUE values
# of the Processor Queue Length
NVALUE = 5
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
        self._values = []
        self._load = None
        self._buffer = ''
        self._popen = None
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
        self._popen = subprocess.Popen(' '.join(command),
                                       stdout=command_stdout,
                                       cwd=os_helper.SAVEDCWD)

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

    def close(self, kill=True):
        if self._popen is None:
            return

        self._load = None

        if kill:
            self._popen.kill()
        self._popen.wait()
        self._popen = None

    def __del__(self):
        self.close()

    def _parse_line(self, line):
        # typeperf outputs in a CSV format like this:
        # "07/19/2018 01:32:26.605","3.000000"
        # (date, process queue length)
        tokens = line.split(',')
        if len(tokens) != 2:
            raise ValueError

        value = tokens[1]
        if not value.startswith('"') or not value.endswith('"'):
            raise ValueError
        value = value[1:-1]
        return float(value)

    def _read_lines(self):
        overlapped, _ = _winapi.ReadFile(self.pipe, BUFSIZE, True)
        bytes_read, res = overlapped.GetOverlappedResult(False)
        if res != 0:
            return ()

        output = overlapped.getbuffer()
        output = output.decode('oem', 'replace')
        output = self._buffer + output
        lines = output.splitlines(True)

        # bpo-36670: typeperf only writes a newline *before* writing a value,
        # not after. Sometimes, the written line in incomplete (ex: only
        # timestamp, without the process queue length). Only pass the last line
        # to the parser if it's a valid value, otherwise store it in
        # self._buffer.
        try:
            self._parse_line(lines[-1])
        except ValueError:
            self._buffer = lines.pop(-1)
        else:
            self._buffer = ''

        return lines

    def getloadavg(self):
        if self._popen is None:
            return None

        returncode = self._popen.poll()
        if returncode is not None:
            self.close(kill=False)
            return None

        try:
            lines = self._read_lines()
        except BrokenPipeError:
            self.close()
            return None

        for line in lines:
            line = line.rstrip()

            # Ignore the initial header:
            # "(PDH-CSV 4.0)","\\\\WIN\\System\\Processor Queue Length"
            if 'PDH-CSV' in line:
                continue

            # Ignore blank lines
            if not line:
                continue

            try:
                processor_queue_length = self._parse_line(line)
            except ValueError:
                print_warning("Failed to parse typeperf output: %a" % line)
                continue

            # We use an exponentially weighted moving average, imitating the
            # load calculation on Unix systems.
            # https://en.wikipedia.org/wiki/Load_(computing)#Unix-style_load_calculation
            # https://en.wikipedia.org/wiki/Moving_average#Exponential_moving_average
            if self._load is not None:
                self._load = (self._load * LOAD_FACTOR_1
                              + processor_queue_length  * (1.0 - LOAD_FACTOR_1))
            elif len(self._values) < NVALUE:
                self._values.append(processor_queue_length)
            else:
                self._load = sum(self._values) / len(self._values)

        return self._load
