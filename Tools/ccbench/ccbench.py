# -*- coding: utf-8 -*-
# This file should be kept compatible with both Python 2.6 and Python >= 3.0.

from __future__ import division
from __future__ import print_function

"""
ccbench, a Python concurrency benchmark.
"""

import time
import os
import sys
import functools
import itertools
import threading
import subprocess
import socket
from optparse import OptionParser, SUPPRESS_HELP
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


THROUGHPUT_DURATION = 2.0

LATENCY_PING_INTERVAL = 0.1
LATENCY_DURATION = 2.0

BANDWIDTH_PACKET_SIZE = 1024
BANDWIDTH_DURATION = 2.0


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

    def findall(s):
        t = time.time()
        try:
            return pat.findall(s)
        finally:
            print(time.time() - t)
    return pat.findall, (arg, )

def task_sort():
    """list sorting (C)"""
    def list_sort(l):
        l = l[::-1]
        l.sort()

    return list_sort, (list(range(1000)), )

def task_compress_zlib():
    """zlib compression (C)"""
    import zlib
    with open(__file__, "rb") as f:
        arg = f.read(5000) * 3

    def compress(s):
        zlib.decompress(zlib.compress(s, 5))
    return compress, (arg, )

def task_compress_bz2():
    """bz2 compression (C)"""
    import bz2
    with open(__file__, "rb") as f:
        arg = f.read(3000) * 2

    def compress(s):
        bz2.compress(s)
    return compress, (arg, )

def task_hashing():
    """SHA1 hashing (C)"""
    import hashlib
    with open(__file__, "rb") as f:
        arg = f.read(5000) * 30

    def compute(s):
        hashlib.sha1(s).digest()
    return compute, (arg, )


throughput_tasks = [task_pidigits, task_regex]
for mod in 'bz2', 'hashlib':
    try:
        globals()[mod] = __import__(mod)
    except ImportError:
        globals()[mod] = None

# For whatever reasons, zlib gives irregular results, so we prefer bz2 or
# hashlib if available.
# (NOTE: hashlib releases the GIL from 2.7 and 3.1 onwards)
if bz2 is not None:
    throughput_tasks.append(task_compress_bz2)
elif hashlib is not None:
    throughput_tasks.append(task_hashing)
else:
    throughput_tasks.append(task_compress_zlib)

latency_tasks = throughput_tasks
bandwidth_tasks = [task_pidigits]


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
        t1 = start_time
        while True:
            for i in range(step):
                _func(*_args)
            t2 = _time()
            # If another thread terminated, the current measurement is invalid
            # => return the previous one.
            if end_event:
                return niters, duration
            niters += step
            duration = t2 - start_time
            if duration >= min_duration:
                end_event.append(None)
                return niters, duration
            if t2 - t1 < 0.01:
                # Minimize interference of measurement on overall runtime
                step = step * 3 // 2
            elif do_yield:
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
        results.append(loop(start_time, THROUGHPUT_DURATION,
                            end_event, do_yield=False))
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
        t.setDaemon(True)
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
        baseline_speed = None
        while nthreads <= max_threads:
            results = run_throughput_test(func, args, nthreads)
            # Taking the max duration rather than average gives pessimistic
            # results rather than optimistic.
            speed = sum(r[0] for r in results) / max(r[1] for r in results)
            print("threads=%d: %d" % (nthreads, speed), end="")
            if baseline_speed is None:
                print(" iterations/s.")
                baseline_speed = speed
            else:
                print(" ( %d %%)" % (speed / baseline_speed * 100))
            nthreads += 1
        print()


LAT_END = "END"

def _sendto(sock, s, addr):
    sock.sendto(s.encode('ascii'), addr)

def _recv(sock, n):
    return sock.recv(n).decode('ascii')

def latency_client(addr, nb_pings, interval):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
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

def run_latency_client(**kwargs):
    cmd_line = [sys.executable, '-E', os.path.abspath(__file__)]
    cmd_line.extend(['--latclient', repr(kwargs)])
    return subprocess.Popen(cmd_line) #, stdin=subprocess.PIPE,
                            #stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

def run_latency_test(func, args, nthreads):
    # Create a listening socket to receive the pings. We use UDP which should
    # be painlessly cross-platform.
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", 0))
    addr = sock.getsockname()

    interval = LATENCY_PING_INTERVAL
    duration = LATENCY_DURATION
    nb_pings = int(duration / interval)

    results = []
    threads = []
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
            loop(start_time, duration * 1.5, end_event, do_yield=False)

        for i in range(nthreads):
            threads.append(threading.Thread(target=run))
        for t in threads:
            t.setDaemon(True)
            t.start()
        # Wait for threads to be ready
        with ready_cond:
            while len(ready) < nthreads:
                ready_cond.wait()

    # Run the client and wait for the first ping(s) to arrive before
    # unblocking the background threads.
    chunks = []
    process = run_latency_client(addr=sock.getsockname(),
                                 nb_pings=nb_pings, interval=interval)
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

    for recv_time, chunk in chunks:
        # NOTE: it is assumed that a line sent by a client wasn't received
        # in two chunks because the lines are very small.
        for line in chunk.splitlines():
            line = line.strip()
            if line and line != LAT_END:
                send_time = eval(line)
                assert isinstance(send_time, float)
                results.append((send_time, recv_time))

    return results

def run_latency_tests(max_threads):
    for task in latency_tasks:
        print("Background CPU task:", task.__doc__)
        print()
        func, args = task()
        nthreads = 0
        while nthreads <= max_threads:
            results = run_latency_test(func, args, nthreads)
            n = len(results)
            # We print out milliseconds
            lats = [1000 * (t2 - t1) for (t1, t2) in results]
            #print(list(map(int, lats)))
            avg = sum(lats) / n
            dev = (sum((x - avg) ** 2 for x in lats) / n) ** 0.5
            print("CPU threads=%d: %d ms. (std dev: %d ms.)" % (nthreads, avg, dev), end="")
            print()
            #print("    [... from %d samples]" % n)
            nthreads += 1
        print()


BW_END = "END"

def bandwidth_client(addr, packet_size, duration):
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
            i += 1
        _send_chunk(BW_END)
    finally:
        sock.close()

def run_bandwidth_client(**kwargs):
    cmd_line = [sys.executable, '-E', os.path.abspath(__file__)]
    cmd_line.extend(['--bwclient', repr(kwargs)])
    return subprocess.Popen(cmd_line) #, stdin=subprocess.PIPE,
                            #stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

def run_bandwidth_test(func, args, nthreads):
    # Create a listening socket to receive the packets. We use UDP which should
    # be painlessly cross-platform.
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", 0))
    addr = sock.getsockname()

    duration = BANDWIDTH_DURATION
    packet_size = BANDWIDTH_PACKET_SIZE

    results = []
    threads = []
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
            loop(start_time, duration * 1.5, end_event, do_yield=False)

        for i in range(nthreads):
            threads.append(threading.Thread(target=run))
        for t in threads:
            t.setDaemon(True)
            t.start()
        # Wait for threads to be ready
        with ready_cond:
            while len(ready) < nthreads:
                ready_cond.wait()

    # Run the client and wait for the first packet to arrive before
    # unblocking the background threads.
    process = run_bandwidth_client(addr=addr,
                                   packet_size=packet_size,
                                   duration=duration)
    _time = time.time
    # This will also wait for the parent to be ready
    s = _recv(sock, packet_size)
    remote_addr = eval(s.partition('#')[0])

    with start_cond:
        start_time = _time()
        started = True
        start_cond.notify(nthreads)

    n = 0
    first_time = None
    while not end_event and BW_END not in s:
        _sendto(sock, s, remote_addr)
        s = _recv(sock, packet_size)
        if first_time is None:
            first_time = _time()
        n += 1
    end_time = _time()

    end_event.append(None)
    for t in threads:
        t.join()
    process.kill()

    return (n - 1) / (end_time - first_time)

def run_bandwidth_tests(max_threads):
    for task in bandwidth_tasks:
        print("Background CPU task:", task.__doc__)
        print()
        func, args = task()
        nthreads = 0
        baseline_speed = None
        while nthreads <= max_threads:
            results = run_bandwidth_test(func, args, nthreads)
            speed = results
            #speed = len(results) * 1.0 / results[-1][0]
            print("CPU threads=%d: %.1f" % (nthreads, speed), end="")
            if baseline_speed is None:
                print(" packets/s.")
                baseline_speed = speed
            else:
                print(" ( %d %%)" % (speed / baseline_speed * 100))
            nthreads += 1
        print()


def main():
    usage = "usage: %prog [-h|--help] [options]"
    parser = OptionParser(usage=usage)
    parser.add_option("-t", "--throughput",
                      action="store_true", dest="throughput", default=False,
                      help="run throughput tests")
    parser.add_option("-l", "--latency",
                      action="store_true", dest="latency", default=False,
                      help="run latency tests")
    parser.add_option("-b", "--bandwidth",
                      action="store_true", dest="bandwidth", default=False,
                      help="run I/O bandwidth tests")
    parser.add_option("-i", "--interval",
                      action="store", type="int", dest="check_interval", default=None,
                      help="sys.setcheckinterval() value")
    parser.add_option("-I", "--switch-interval",
                      action="store", type="float", dest="switch_interval", default=None,
                      help="sys.setswitchinterval() value")
    parser.add_option("-n", "--num-threads",
                      action="store", type="int", dest="nthreads", default=4,
                      help="max number of threads in tests")

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
        kwargs = eval(options.latclient)
        latency_client(**kwargs)
        return

    if options.bwclient:
        kwargs = eval(options.bwclient)
        bandwidth_client(**kwargs)
        return

    if not options.throughput and not options.latency and not options.bandwidth:
        options.throughput = options.latency = options.bandwidth = True
    if options.check_interval:
        sys.setcheckinterval(options.check_interval)
    if options.switch_interval:
        sys.setswitchinterval(options.switch_interval)

    print("== %s %s (%s) ==" % (
        platform.python_implementation(),
        platform.python_version(),
        platform.python_build()[0],
    ))
    # Processor identification often has repeated spaces
    cpu = ' '.join(platform.processor().split())
    print("== %s %s on '%s' ==" % (
        platform.machine(),
        platform.system(),
        cpu,
    ))
    print()

    if options.throughput:
        print("--- Throughput ---")
        print()
        run_throughput_tests(options.nthreads)

    if options.latency:
        print("--- Latency ---")
        print()
        run_latency_tests(options.nthreads)

    if options.bandwidth:
        print("--- I/O bandwidth ---")
        print()
        run_bandwidth_tests(options.nthreads)

if __name__ == "__main__":
    main()
