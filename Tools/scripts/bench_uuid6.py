"""Generate UUIDv6 values for five seconds and check for duplicates.

Run with the CPython build being measured, for example::

    ./python Tools/scripts/bench_uuid6.py
    ./python Tools/scripts/bench_uuid6.py --threads 8

Each implementation runs in a fresh subprocess. Workers keep separate sets
during generation, then merge them to detect duplicates across threads too.
Reported rates include set insertion; merging happens outside the timed run.
The benchmark retains all unique UUIDs and can consume substantial memory.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import subprocess
import sys
import sysconfig
import threading
import time


def measure(implementation, seconds, threads, fixed_fields):
    if implementation == 'python':
        from test.support.import_helper import import_fresh_module
        uuid = import_fresh_module('uuid', blocked=['_uuid'])
        assert uuid.UUID is uuid._py_UUID
        assert uuid._c_uuid6 is None
    else:
        import uuid
        assert uuid._c_uuid6 is not None, 'C UUIDv6 implementation unavailable'
        assert uuid.uuid6 is uuid._c_uuid6

    uuid.getnode()  # Exclude initial hardware-address discovery.
    generate = uuid.uuid6
    if fixed_fields:
        def generate():
            return uuid.uuid6(node=0, clock_seq=0)

    start = deadline = 0.0

    def start_timer():
        nonlocal start, deadline
        start = time.perf_counter()
        deadline = start + seconds

    barrier = threading.Barrier(threads + 1, action=start_timer)

    def worker():
        seen = set()
        add = seen.add
        count = 0
        barrier.wait()
        while time.perf_counter() < deadline:
            # Amortize clock checks without exceeding the deadline by much.
            for _ in range(256):
                add(generate())
            count += 256
        return seen, count

    with ThreadPoolExecutor(threads) as pool:
        futures = [pool.submit(worker) for _ in range(threads)]
        barrier.wait()
        results = [future.result() for future in futures]
        elapsed = time.perf_counter() - start

    unique, generated = results.pop()
    duplicate_examples = []
    while results:
        seen, count = results.pop()
        if len(duplicate_examples) < 5:
            overlap = unique.intersection(seen)
            duplicate_examples.extend(
                str(value) for value in list(overlap)[:5 - len(duplicate_examples)])
        unique.update(seen)
        generated += count
    return {
        'implementation': implementation,
        'threads': threads,
        'gil_enabled': sys._is_gil_enabled(),
        'seconds': elapsed,
        'generated': generated,
        'unique': len(unique),
        'duplicates': generated - len(unique),
        'duplicate_examples': duplicate_examples,
        'per_second': generated / elapsed,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds', type=float, default=5.0)
    parser.add_argument('--threads', type=int, default=1)
    parser.add_argument('--implementation', choices=('python', 'c', 'both'),
                        default='both')
    parser.add_argument('--fixed-fields', action='store_true',
                        help='use node=0 and clock_seq=0 to isolate timestamp uniqueness')
    parser.add_argument('--worker', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if not 0 < args.seconds < float('inf') or args.threads < 1:
        parser.error('seconds and threads must be positive and finite')

    if args.worker:
        result = measure(args.implementation, args.seconds, args.threads,
                         args.fixed_fields)
        print(json.dumps(result))
        return bool(result['duplicates'])

    print(f'Python {sys.version.split()[0]}, '
          f'Py_DEBUG={sysconfig.get_config_var("Py_DEBUG")}, '
          f'GIL enabled={sys._is_gil_enabled()}, threads={args.threads}')
    print(f'CFLAGS: {sysconfig.get_config_var("PY_CFLAGS")}')
    print(f'{"Implementation":<16} {"Generated":>12} {"UUID/s":>12} '
          f'{"Seconds":>9} {"Duplicates":>12}')
    failed = False
    implementations = ('python', 'c') if args.implementation == 'both' else (
        args.implementation,)
    for implementation in implementations:
        command = [sys.executable, __file__, '--worker',
                   '--implementation', implementation,
                   '--seconds', str(args.seconds), '--threads', str(args.threads)]
        if args.fixed_fields:
            command.append('--fixed-fields')
        result = subprocess.run(command, check=False, text=True,
                                stdout=subprocess.PIPE)
        if result.returncode not in (0, 1) or not result.stdout.strip():
            return result.returncode or 1
        values = json.loads(result.stdout)
        print(f'{implementation:<16} {values["generated"]:>12,} '
              f'{values["per_second"]:>12,.0f} {values["seconds"]:>9.3f} '
              f'{values["duplicates"]:>12,}', flush=True)
        for value in values['duplicate_examples']:
            print(f'  Duplicate: {value}', flush=True)
        failed |= bool(result.returncode)
    return failed


if __name__ == '__main__':
    sys.exit(main())
