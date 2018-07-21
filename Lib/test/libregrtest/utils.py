import os
import math
import textwrap
import subprocess
import sys
from test import support


def format_duration(seconds):
    ms = math.ceil(seconds * 1e3)
    seconds, ms = divmod(ms, 1000)
    minutes, seconds = divmod(seconds, 60)
    hours, minutes = divmod(minutes, 60)

    parts = []
    if hours:
        parts.append('%s hour' % hours)
    if minutes:
        parts.append('%s min' % minutes)
    if seconds:
        parts.append('%s sec' % seconds)
    if ms:
        parts.append('%s ms' % ms)
    if not parts:
        return '0 ms'

    parts = parts[:2]
    return ' '.join(parts)


def removepy(names):
    if not names:
        return
    for idx, name in enumerate(names):
        basename, ext = os.path.splitext(name)
        if ext == '.py':
            names[idx] = basename


def count(n, word):
    if n == 1:
        return "%d %s" % (n, word)
    else:
        return "%d %ss" % (n, word)


def printlist(x, width=70, indent=4, file=None):
    """Print the elements of iterable x to stdout.

    Optional arg width (default 70) is the maximum line length.
    Optional arg indent (default 4) is the number of blanks with which to
    begin each line.
    """

    blanks = ' ' * indent
    # Print the sorted list: 'x' may be a '--random' list or a set()
    print(textwrap.fill(' '.join(str(elt) for elt in sorted(x)), width,
                        initial_indent=blanks, subsequent_indent=blanks),
          file=file)

BUFSIZE = 8192
LOAD_FACTOR_1 = 0.9200444146293232478931553241
SAMPLING_INTERVAL = 5
COUNTER_NAME = r'\System\Processor Queue Length'

"""
This class asynchronously interacts with the `typeperf` command to read
the system load on Windows. Mulitprocessing and threads can't be used
here because they interfere with the test suite's cases for those
modules.
"""
class WindowsLoadTracker():
    def __init__(self):
        self.load = 0.0
        self.start()

    def start(self):
        import _winapi
        import msvcrt
        import uuid

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
            _winapi.OPEN_EXISTING, 0x80000000, _winapi.NULL
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

    def __del__(self):
        self.p.kill()
        self.p.wait()
