import _winapi
import winreg
import msvcrt
import subprocess
import uuid
from test import support

# Max size of asynchronous reads
BUFSIZE = 8192
# Exponential damping factor (see below)
LOAD_FACTOR_1 = 0.9200444146293232478931553241

# Seconds per measurement
SAMPLING_INTERVAL = 5
# windows registry subkey of HKEY_LOCAL_MACHINE where the counter names of typeperf are registered
COUNTER_REGISTRY_KEY = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib\\CurrentLanguage"


class WindowsLoadTracker():
    """
    This class asynchronously interacts with the `typeperf` command to read
    the system load on Windows. Multiprocessing and threads can't be used
    here because they interfere with the test suite's cases for those
    modules.
    """

    def __init__(self):
        self.load = 0.0
        self.typePerfProcess = None
        self.counter_name = ''
        self.start()

    def start(self):
        # Create a named pipe which allows for asynchronous IO in Windows
        pipe_name = r'\\.\pipe\typeperf_output_' + str(uuid.uuid4())

        open_mode = _winapi.PIPE_ACCESS_INBOUND
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
        self.counter_name = self._get_counter_name()
        command = ['typeperf', self.counter_name, '-si', str(SAMPLING_INTERVAL)]
        self.typePerfProcess = subprocess.Popen(' '.join(command), stdout=command_stdout, cwd=support.SAVEDCWD)

    def _get_counter_name(self):
        # accessing the registry to get the counter localization name
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, COUNTER_REGISTRY_KEY) as perfkey:
            counter_list = winreg.QueryValueEx(perfkey, 'Counter')[0]
            # converting the alternating counter list to dict
            i = iter(counter_list)
            counter_dict = dict(zip(i, i))
            # system counter is at '2' and processor queue length is '44'
            counter_name = r'"\{}\{}"'.format(counter_dict['2'], counter_dict['44'])
        return counter_name

    def close(self):
        if self.typePerfProcess is None:
            return
        self.typePerfProcess.kill()
        self.typePerfProcess.wait()
        self.typePerfProcess = None

    def __del__(self):
        self.close()

    def read_output(self):
        overlapped, _ = _winapi.ReadFile(self.pipe, BUFSIZE, True)
        bytes_read, res = overlapped.GetOverlappedResult(False)
        if res != 0:
            return
        response = overlapped.getbuffer()
        # output is windows 'oem' encoded
        return response.decode(encoding='oem', errors='ignore')

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
            if line.strip() == '' or '\\\\' in line or len(toks) != 2:
                continue

            load = float(toks[1].replace('"', ''))
            # We use an exponentially weighted moving average, imitating the
            # load calculation on Unix systems.
            # https://en.wikipedia.org/wiki/Load_(computing)#Unix-style_load_calculation
            new_load = self.load * LOAD_FACTOR_1 + load * (1.0 - LOAD_FACTOR_1)
            self.load = new_load

        return self.load
