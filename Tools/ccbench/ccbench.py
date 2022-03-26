# This file should be kept compatible with both Python 2.6 and Python >= 3.0.

from __future__ import division
from __future__ import print_function

"""
ccbench, a Python concurrency benchmark.
"""

import time
import os
import sys
import itertools
import multiprocessing
import threading
import subprocess
import socket
from optparse import OptionParser, SUPPRESS_HELP
from statistics import fmean, pstdev
import platform

# Compatibility
try:
    xrange
except NameError:
    xrange = range

try:
    map = itertools.imap
except AttributeError:
    pass

# psutil
    try:
        import psutil
    except ImportError:
        psutil = None



PYTHON_VERSION = tuple(map(int,platform.python_version_tuple()))
# Add the first two parts of the version number as a tuple for any version of Python
# where the tests hang (because the same thread monopolises the GIL)
YIELD_VERSIONS = [
    # e.g. (2, 7)
]

CORES = multiprocessing.cpu_count()

THROUGHPUT_DURATION = 2.0

LATENCY_DURATION = 2.0
LATENCY_PING_INTERVAL = 0.1
LATENCY_PORT = 16000

BANDWIDTH_DURATION = 2.0
BANDWIDTH_ECHO_INTERVAL = 0.0
BANDWIDTH_PORT = 16001
BANDWIDTH_PACKET_SIZE = 1024

for mod in 'bz2', 'hashlib':
    try:
        globals()[mod] = __import__(mod)
    except ImportError:
        globals()[mod] = None


def cpu_times():
    if psutil is None:
        times = os.times()
        return times.user, times.system, None
    times = psutil.Process().cpu_times()
    iowait = times.iowait if len(times) > 4 else None
    return times.user, times.system, iowait

def cpu_delta(start, finish):
    user = finish[0] - start[0]
    system = finish[1] - start[1]
    total = user + system
    iowait = finish[2] - start[2] if start[2] is not None and finish[2] is not None else None
    return total, user, system, iowait

def calc_cpu_usage(start, finish, baseline=None):
    times = []
    delta = cpu_delta(start, finish)
    if baseline is None:
        baseline = (None,) * 4
    for type, cpu, base in zip(("Total", "User", "System", "IoWait"), delta, baseline):
        if cpu is not None:
            if base:
                times.append("%s: %7.4f (%+6.1f%%)" % (type, cpu, (cpu / base - 1.0) * 100))
            else:
                times.append("%s: %7.4f" % (type, cpu))
    return "CPU: %s" % ", ".join(times)

def task_pidigits():
    """Pi calculation (Python)"""
    _map = map
    _count = itertools.count
    _islice = itertools.islice

    def calc_ndigits(n):
        # From http://shootout.alioth.debian.org/
        def gen_x():
            return _map(lambda k: (k, 4*k + 2, 0, 2*k + 1), _count(1))

        def compose(a, b):
            aq, ar, as_, at = a
            bq, br, bs, bt = b
            return (aq * bq,
                    aq * br + ar * bt,
                    as_ * bq + at * bs,
                    as_ * br + at * bt)

        def extract(z, j):
            q, r, s, t = z
            return (q*j + r) // (s*j + t)

        def pi_digits():
            z = (1, 0, 0, 1)
            x = gen_x()
            while 1:
                y = extract(z, 3)
                while y != extract(z, 4):
                    z = compose(z, next(x))
                    y = extract(z, 3)
                z = compose((10, -10*y, 0, 1), z)
                yield y

        return list(_islice(pi_digits(), n))

    return calc_ndigits, (50, )

def task_regex():
    """regular expression (C)"""
    # XXX this task gives horrendous latency results.
    import re
    # Taken from the `inspect` module
    pat = re.compile(r'^(\s*def\s)|(.*(?<!\w)lambda(:|\s))|^(\s*@)', re.MULTILINE)
    with open(__file__, "r") as f:
        arg = f.read(2000)
    return pat.findall, (arg, )

def task_sort():
    """list sorting (C)"""
    def list_sort(l):
        l = l[::-1]
        l.sort()

    return list_sort, (list(range(1000)), )

throughput_tasks = [task_pidigits, task_regex]

# For whatever reasons, zlib gives irregular results,
# so we only run zlib if bz2 and hashlib are not available.
# (NOTE: hashlib releases the GIL from 2.7 and 3.1 onwards)
if bz2 is not None:
    def task_compress_bz2():
        """bz2 compression (C)"""
        with open(__file__, "rb") as f:
            arg = f.read(3000) * 2

        def compress(s):
            bz2.compress(s)
        return compress, (arg, )
    throughput_tasks.append(task_compress_bz2)

if hashlib is not None:
    with open(__file__, "rb") as f:
        arg = f.read(5000) * 30

    def task_hashing_sha1():
        """SHA1 hashing (C)"""
        def compute(s):
            hashlib.sha1(s).digest()
        return compute, (arg, )
    throughput_tasks.append(task_hashing_sha1)

    def task_hashing_sha512():
        """SHA512 hashing (C) - GIL is released"""
        def compute(s):
            hashlib.sha512(s).digest()
        return compute, (arg, )
    throughput_tasks.append(task_hashing_sha512)

if bz2 is None and hashlib is None:
    def task_compress_zlib():
        """zlib compression (C)"""
        import zlib
        with open(__file__, "rb") as f:
            arg = f.read(5000) * 3

        def compress(s):
            zlib.decompress(zlib.compress(s, 5))
        return compress, (arg, )

    throughput_tasks.append(task_compress_zlib)

latency_tasks = throughput_tasks
bandwidth_tasks = [task_pidigits]
bandwidth_tasks = throughput_tasks


class TimedLoop:
    def __init__(self, func, args):
        self.func = func
        self.args = args

    def __call__(self, start_time, min_duration, end_event, do_yield=False):
        step = 20
        niters = 0
        duration = 0.0
        _time = time.time
        _sleep = time.sleep
        _func = self.func
        _args = self.args
        t1 = _time()
        while True:
            for i in range(step):
                _func(*_args)
            t2 = _time()
            # If another thread terminated, the current measurement is invalid
            # => return the previous one.
            if end_event:
                return float(niters)/float(duration) if duration > 0 else 0.0
            niters += step
            duration = t2 - start_time
            if duration >= min_duration:
                end_event.append(None)
                return float(niters)/float(duration) if duration > 0 else 0.0
            if t2 - t1 < 0.01:
                # Minimize interference of measurement on overall runtime
                step = step * 3 // 2
            elif do_yield and PYTHON_VERSION[:2] in YIELD_VERSIONS:
                # OS scheduling of Python threads is sometimes so bad that we
                # have to force thread switching ourselves, otherwise we get
                # completely useless results.
                _sleep(0.0001)
            t1 = t2


def run_throughput_test(func, args, nthreads):
    assert nthreads >= 1

    # Warm up
    func(*args)

    results = []
    loop = TimedLoop(func, args)
    end_event = []

    if nthreads == 1:
        # Pure single-threaded performance, without any switching or
        # synchronization overhead.
        start_time = time.time()

        results.append(loop(start_time, THROUGHPUT_DURATION, end_event))
        return results

    started = False
    ready_cond = threading.Condition()
    start_cond = threading.Condition()
    ready = []

    def run():
        with ready_cond:
            ready.append(None)
            ready_cond.notify()
        with start_cond:
            while not started:
                start_cond.wait()
        results.append(loop(start_time, THROUGHPUT_DURATION,
                            end_event, do_yield=True))

    threads = []
    for i in range(nthreads):
        threads.append(threading.Thread(target=run))
    for t in threads:
        t.daemon = True
        t.start()
    # We don't want measurements to include thread startup overhead,
    # so we arrange for timing to start after all threads are ready.
    with ready_cond:
        while len(ready) < nthreads:
            ready_cond.wait()
    with start_cond:
        start_time = time.time()
        started = True
        start_cond.notify(nthreads)
    for t in threads:
        t.join()

    return results

def run_throughput_tests(max_threads):
    for task in throughput_tasks:
        print(task.__doc__)
        print()
        func, args = task()
        nthreads = 1
        baseline_speed = baseline_cpu = None
        while nthreads <= max_threads:
            start_cpu = cpu_times()
            results = run_throughput_test(func, args, nthreads)
            finish_cpu = cpu_times()
            # Taking the max duration rather than average gives pessimistic
            # results rather than optimistic.
            speed = round(sum(results))

            print("threads=%2d: %4d" % (nthreads, speed), end="")
            if baseline_speed is None:
                print(" iterations/sec        - Baseline: ", end="")
                baseline_speed = speed
            else:
                ratio = (speed / baseline_speed - 1.0) * 100
                stdev = pstdev(results)
                print(" (%+6.1f%%, std dev: %3d its/sec) - " % (ratio, stdev), end="")
            print(calc_cpu_usage(start_cpu, finish_cpu, baseline_cpu))
            if baseline_cpu is None:
                baseline_cpu = cpu_delta(start_cpu, finish_cpu)
            nthreads += 1
        print()


LAT_END = "END"

def _sendto(sock, s, addr):
    sock.sendto(s.encode('ascii'), addr)

def _recv(sock, n):
    return sock.recv(n).decode('ascii')

def latency_client(
    addr="127.0.0.1",
    duration=LATENCY_DURATION,
    interval=LATENCY_PING_INTERVAL,
    port = LATENCY_PORT,
):
    nb_pings = int(duration / interval)
    if isinstance(addr, str):
        addr = (addr, port)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        _time = time.time
        _sleep = time.sleep
        def _ping():
            _sendto(sock, "%r\n" % _time(), addr)
        # The first ping signals the parent process that we are ready.
        _ping()
        # We give the parent a bit of time to notice.
        _sleep(1.0)
        for i in range(nb_pings):
            _sleep(interval)
            _ping()
        _sendto(sock, LAT_END + "\n", addr)
    finally:
        sock.close()

def run_latency_client(*args):
    cmd_line = [sys.executable, '-E', os.path.abspath(__file__)]
    cmd_line.extend(['--latclient', str(args)])
    print(" ".join(cmd_line))
    return subprocess.Popen(cmd_line) #, stdin=subprocess.PIPE,
                            #stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

def run_latency_test(func, args, nthreads, interval):
    # Create a listening socket to receive the pings. We use UDP which should
    # be painlessly cross-platform.
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", 0))
    addr = sock.getsockname()

    duration = LATENCY_DURATION
    nb_pings = int(duration / interval)

    results = []
    threads = []
    thread_results = []
    end_event = []
    start_cond = threading.Condition()
    started = False
    if nthreads > 0:
        # Warm up
        func(*args)

        results = []
        loop = TimedLoop(func, args)
        ready = []
        ready_cond = threading.Condition()

        def run():
            with ready_cond:
                ready.append(None)
                ready_cond.notify()
            with start_cond:
                while not started:
                    start_cond.wait()
            thread_results.append(loop(start_time, duration * 1.5, end_event))

        for i in range(nthreads):
            threads.append(threading.Thread(target=run))
        for t in threads:
            t.daemon = True
            t.start()
        # Wait for threads to be ready
        with ready_cond:
            while len(ready) < nthreads:
                ready_cond.wait()

    # Run the client and wait for the first ping(s) to arrive before
    # unblocking the background threads.
    chunks = []
    process = run_latency_client(sock.getsockname(), options.client_delay)
    s = _recv(sock, 4096)
    _time = time.time

    with start_cond:
        start_time = _time()
        started = True
        start_cond.notify(nthreads)

    while LAT_END not in s:
        s = _recv(sock, 4096)
        t = _time()
        chunks.append((t, s))

    # Tell the background threads to stop.
    end_event.append(None)
    for t in threads:
        t.join()
    process.wait()
    sock.close()

    for recv_time, chunk in chunks:
        # NOTE: it is assumed that a line sent by a client wasn't received
        # in two chunks because the lines are very small.
        for line in chunk.splitlines():
            line = line.strip()
            if line and line != LAT_END:
                send_time = eval(line)
                assert isinstance(send_time, float)
                results.append((send_time, recv_time))

    return results, thread_results

def run_latency_tests(max_threads, interval):
    for task in latency_tasks:
        print("Background CPU task:", task.__doc__)
        print()
        func, args = task()
        nthreads = 0
        baseline_cpu = None
        while nthreads <= max_threads:
            start_cpu = cpu_times()
            results, thread_results = run_latency_test(func, args, nthreads, interval)
            throughput = sum(thread_results)
            finish_cpu = cpu_times()
            # We print out milliseconds
            lats = [1000 * (t2 - t1) for (t1, t2) in results]
            #print(list(map(int, lats)))
            avg = fmean(lats)
            stdev = pstdev(lats)
            print("CPU threads=%2d: %3d ms (std dev: %3d ms)" % (nthreads, avg, stdev), end="")
            if nthreads > 0:
                print(" - Throughput: %4d its/sec - %s"
                    % ( throughput, calc_cpu_usage(start_cpu, finish_cpu, baseline_cpu)),
                    end=""
                )
                if baseline_cpu is None:
                    baseline_cpu = cpu_delta(start_cpu, finish_cpu)
            print()
            nthreads += 1
        print()


BW_END = "END"

def bandwidth_client(addr, packet_size, duration, interval, port=BANDWIDTH_PORT):
    if isinstance(addr, str):
        addr = (addr, port)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", 0))
    local_addr = sock.getsockname()
    _time = time.time
    _sleep = time.sleep
    def _send_chunk(msg):
        _sendto(sock, ("%r#%s\n" % (local_addr, msg)).rjust(packet_size), addr)
    # We give the parent some time to be ready.
    _sleep(1.0)
    try:
        start_time = _time()
        end_time = start_time + duration * 2.0
        i = 0
        while _time() < end_time:
            _send_chunk(str(i))
            s = _recv(sock, packet_size)
            assert len(s) == packet_size
            _sleep(interval)
            i += 1
        _send_chunk(BW_END)
    finally:
        sock.close()

def run_bandwidth_client(*args):
    cmd_line = [sys.executable, '-E', os.path.abspath(__file__)]
    cmd_line.extend(['--bwclient', str(args)])
    # print(" ".join(cmd_line))
    return subprocess.Popen(cmd_line) #, stdin=subprocess.PIPE,
                            #stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

def run_bandwidth_test(func, args, nthreads, interval):
    # Create a listening socket to receive the packets. We use UDP which should
    # be painlessly cross-platform.
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind(("127.0.0.1", 0))
        addr = sock.getsockname()

        duration = BANDWIDTH_DURATION
        packet_size = BANDWIDTH_PACKET_SIZE

        results = []
        threads = []
        thread_results = []
        end_event = []
        start_cond = threading.Condition()
        started = False
        if nthreads > 0:
            # Warm up
            func(*args)

            loop = TimedLoop(func, args)
            ready = []
            ready_cond = threading.Condition()

            def run():
                with ready_cond:
                    ready.append(None)
                    ready_cond.notify()
                with start_cond:
                    while not started:
                        start_cond.wait()
                thread_results.append(loop(start_time, duration * 1.5, end_event))

            for i in range(nthreads):
                threads.append(threading.Thread(target=run))
            for t in threads:
                t.daemon = True
                t.start()
            # Wait for threads to be ready
            with ready_cond:
                while len(ready) < nthreads:
                    ready_cond.wait()

        # Run the client and wait for the first packet to arrive before
        # unblocking the background threads.
        process = run_bandwidth_client(addr, packet_size, duration, interval)
        _time = time.time
        # This will also wait for the parent to be ready
        s = _recv(sock, packet_size)
        remote_addr = eval(s.partition('#')[0])

        with start_cond:
            start_time = _time()
            started = True
            start_cond.notify(nthreads)

        n = -1
        first_time = None
        while not end_event:
            _sendto(sock, s, remote_addr)
            s = _recv(sock, packet_size)
            if first_time is None:
                first_time = _time()
            if BW_END in s:
                break
            n += 1
        end_time = _time()

    end_event.append(None)
    for t in threads:
        t.join()
    process.kill()

    speed = n / (end_time - first_time) if end_time != first_time else 0.0

    return speed, sum(thread_results)

def run_bandwidth_tests(max_threads, interval):
    for task in bandwidth_tasks:
        print("Background CPU task:", task.__doc__)
        print()
        func, args = task()
        nthreads = 0
        baseline_speed = baseline_cpu = None
        while nthreads <= max_threads:
            start_cpu = cpu_times()
            speed, throughput = run_bandwidth_test(func, args, nthreads, interval)
            finish_cpu = cpu_times()
            #speed = len(results) * 1.0 / results[-1][0]
            print("CPU threads=%2d: %6.1f" % (nthreads, speed), end="")
            if nthreads == 0:
                print(" packets/sec")
                baseline_speed = speed
            else:
                delta = (speed / baseline_speed - 1) * 100
                print(" pkt/sec (%+6.1f%%) - Throughput: %4d its/sec - %s"
                    % (delta, throughput, calc_cpu_usage(start_cpu, finish_cpu, baseline_cpu))
                )
                if baseline_cpu is None and nthreads > 0:
                    baseline_cpu = cpu_delta(start_cpu, finish_cpu)
            nthreads += 1
        print()

def print_env(*args, **kwargs):
    print("== %s %s (%s) %d-bit ==" % (
        platform.python_implementation(),
        platform.python_version(),
        platform.python_build()[0],
        (len(hex(sys.maxsize)) - 2) * 4,
    ))
    # Processor identification often has repeated spaces
    cpu = ' '.join(platform.processor().split())
    print("== %s %s %s on '%s' with %d cores ==" % (
        platform.machine(),
        platform.system(),
        platform.architecture()[0],
        cpu,
        CORES,
    ))
    name, lock, version = sys.thread_info
    print("== Threads: %s, Lock: %s, Version: %s ==" % (name, lock, version))
    check_interval = None if PYTHON_VERSION >= (3, 9) else sys.getcheckinterval()
    switch_interval = None if PYTHON_VERSION < (3, 2) else sys.getswitchinterval()
    print("== Check interval: %s, Switch interval: %s ==" % (check_interval, switch_interval))

    if psutil:
        analyse_psutil(psutil, *args, **kwargs)
        analyse_psutil(psutil, *args, **kwargs)
    else:
        print("== Unable to document priority and affinity - run 'pip install psutil ==")

    print()

def analyse_psutil(psutil, new_affinity=None):
    system = platform.system()
    process = psutil.Process()
    priority = process.nice()
    io_priority = process.ionice()
    affinity = process.cpu_affinity()
    if system == "Windows":
        priorities = {
            psutil.IDLE_PRIORITY_CLASS: "Idle",
            psutil.BELOW_NORMAL_PRIORITY_CLASS: "Below normal",
            psutil.NORMAL_PRIORITY_CLASS: "Normal",
            psutil.ABOVE_NORMAL_PRIORITY_CLASS: "Above normal",
            psutil.HIGH_PRIORITY_CLASS: "High",
            psutil.REALTIME_PRIORITY_CLASS: "Real-time",
        }
        io_priorities = {
            psutil.IOPRIO_VERYLOW: "Very low",
            psutil.IOPRIO_LOW: "Low",
            psutil.IOPRIO_NORMAL: "Normal",
            psutil.IOPRIO_HIGH: "High",
        }
        priority = priorities[priority] if priority in priorities else repr(priority)
        io_priority = io_priorities[io_priority] if io_priority in io_priorities else repr(io_priority)
    else:
        io_priorities = {
            psutil.IOPRIO_CLASS_NONE: "None",
            psutil.IOPRIO_CLASS_IDLE: "Idle",
            psutil.IOPRIO_CLASS_BE: "Best efforts",
            psutil.IOPRIO_CLASS_RT: "Real-time",
        }
        priority = str(priority)
        io_value = iopriority[1]
        io_priority = io_priorities[io_priority[0]] if io_priority[0] in io_priorities else repr(io_priority)
        if isinstance(io_value, int):
            io_priority += ":%d" % io_value

    print("== Cores: %d, Hyperthreads: %d, Priority: %s, I/O priority: %s, Affinity: %s ==" % (
        psutil.cpu_count(False),
        psutil.cpu_count(),
        priority,
        io_priority,
        str(affinity),
    ))

    set_env(priority, io_priority, new_affinity)

def set_env(priority="", io_priority="", new_affinity=None):
    if not psutil:
        return
    explicit_affinity = new_affinity is not None
    system = platform.system()
    process = psutil.Process()
    affinity = process.cpu_affinity()
    if new_affinity is None:
        new_affinity = list(range(CORES))
    elif isinstance(new_affinity, int) and len(affinity) != int:
        n = min(new_affinity, CORES)
        # Just in case this is a hyperthreading processor (AFAIK max 2 hyperthreads per real core)
        # we will use alternate threads as much as we can.
        # This should make no real difference if the processor does not have hyperthreading
        # NOTE: We are assuming that all cores are of equivalent power - if this is not the case
        # user will have to set affinity explicitly using options as AFAIK there is no way
        # to determine the details of individual cores in Python.
        real_cores = int(CORES / 2)
        if n == real_cores:
            new_affinity = list(range(CORES))
        elif n <= real_cores:
            new_affinity = list(range(0, n * 2, 2))
        else:
            new_affinity = list(range(2 * n - CORES))
            new_affinity.extend(list(range(2 * n - CORES, CORES, 2)))

    # Attempt to adjust priorities to highest possible for benchmark to minimise impact of everything else
    if system == "Windows":
        if priority not in ("Real-time", "High"):
            process.nice(psutil.HIGH_PRIORITY_CLASS)
            if priority:
                print("!! Process priority set to HIGH !!")
        if io_priority != "High":
            try:
                process.ionice(psutil.IOPRIO_HIGH)
                if io_priority:
                    print("!! Process I/O priority set to HIGH !!")
            except psutil.AccessDenied:
                pass
    else:
        if priority != "-20":
            process.nice(-20)
            if priority:
                print("!! Process nice set to -20 !!")
        if not io_priority.startswith("Real-time"):
            try:
                process.ionice(psutil.IOPRIO_CLASS_RT, 0)
                if io_priority:
                    print("!! Process I/O priority set to REALTIME:0 !!")
            except psutil.AccessDenied:
                pass
    if affinity != new_affinity:
        process.cpu_affinity(new_affinity)
        if explicit_affinity:
            print("!! Process affinity set to %s !!" % str(new_affinity))


def main():
    usage = "usage: %prog [-h|--help] [options]"
    parser = OptionParser(usage=usage)
    parser.add_option("-t", "--throughput",
        action="store_true", dest="throughput", default=False,
        help="run throughput tests"
    )
    parser.add_option("-l", "--latency",
        action="store_true", dest="latency", default=False,
        help="run latency tests"
    )
    parser.add_option("-b", "--bandwidth",
        action="store_true", dest="bandwidth", default=False,
        help="run I/O bandwidth tests"
    )
    parser.add_option("-i", "--interval",
        action="store", type="int", dest="check_interval", default=None,
        help="sys.setcheckinterval() value "
            "(Python 3.8 and older)"
    )
    parser.add_option("-I", "--switch-interval",
        action="store", type="float", dest="switch_interval", default=None,
        help="sys.setswitchinterval() value "
            "(Python 3.2 and newer - default 0.005s)"
    )
    parser.add_option("-a", "--affinity",
        action="store", type="str", dest="affinity", default=None,
        help="process affinity - number of cores or a python list of cores that "
            "this process should run on (default all processor cores "
            "i.e. on this computer %d cores)." % CORES
    )
    parser.add_option("-n", "--num-threads",
        action="store", type="int", dest="nthreads", default=None,
        help="max number of threads in tests (default 2*affinity "
        "i.e. %d threads on this computer)." % (CORES * 2)
    )
    parser.add_option("-p", "--ping-interval",
        action="store", type="float", dest="ping_interval", default=LATENCY_PING_INTERVAL,
        help="the delay in seconds between receiving a latency/ping response "
            "and sending the next ping request (default %default sec). "
            "A delay of (say) between 0.01s and 0.1s is likely representative of either/both "
            "real I/O and GUI event loop activities such as moving the mouse."
    )
    parser.add_option("-e", "--echo-interval",
        action="store", type="float", dest="echo_interval", default=BANDWIDTH_ECHO_INTERVAL,
        help="the delay in seconds between receiving a bandwidth/echo response "
            "and sending the next echo request (default %default sec). "
            "If the echo-interval is >0.0, there is very little difference between "
            "the ping and echo tests. "
            "On the other hand, a 0.0 delay does not mimic any known real-life workload."
    )

    # Hidden option to run the pinging and bandwidth clients
    parser.add_option("", "--latclient",
                      action="store", dest="latclient", default=None,
                      help=SUPPRESS_HELP)
    parser.add_option("", "--bwclient",
                      action="store", dest="bwclient", default=None,
                      help=SUPPRESS_HELP)

    options, args = parser.parse_args()
    if args:
        parser.error("unexpected arguments")

    if options.latclient:
        set_env()
        args = tuple(eval(options.latclient))
        latency_client(*args)
        return

    if options.bwclient:
        set_env()
        args = tuple(eval(options.bwclient))
        bandwidth_client(*args)
        return

    if options.affinity:
        options.affinity = eval(options.affinity)

    if options.nthreads is None:
        if options.affinity is None:
            options.nthreads = 2 * CORES
        elif isinstance(options.affinity, int):
            options.nthreads = 2 * options.affinity
        elif isinstance(options.affinity, list):
            options.nthreads = 2 * len(options.affinity)
        else:
            options.nthreads = 2 * CORES

    if options.check_interval:
        print("!! Setting check interval to %d !!" % options.check_interval)
        sys.setcheckinterval(options.check_interval)
    if options.switch_interval:
        print("!! Setting switch interval to %.3fs !!" % options.switch_interval)
        sys.setswitchinterval(options.switch_interval)

    print_env(new_affinity=options.affinity)

    if not options.throughput and not options.latency and not options.bandwidth:
        options.throughput = options.latency = options.bandwidth = True
    if options.throughput:
        print("--- Throughput ---")
        print()
        run_throughput_tests(options.nthreads)
        print()

    if options.latency:
        print("--- Latency ---")
        print()
        run_latency_tests(options.nthreads, options.ping_interval)

    if options.bandwidth:
        print("--- I/O bandwidth ---")
        print()
        run_bandwidth_tests(options.nthreads, options.echo_interval)

if __name__ == "__main__":
    main()
