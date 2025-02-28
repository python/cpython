import sys
import glob
import importlib
import os
import time
import argparse


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-n',
        '--iterations',
        type=int,
        default=1,
        help='Number of iterations to run.',
    )
    parser.add_argument('tasks', nargs='*', help='Name of tasks to run.')
    args = parser.parse_args()
    cmdline_tasks = set(args.tasks)
    tasks = []
    package_dir = os.path.dirname(__file__)
    for fn in glob.glob(os.path.join(package_dir, 'bm_*.py')):
        tasks.append(fn)
    total_time = 0
    print('Running PGO tasks...')
    for fn in sorted(tasks):
        name, ext = os.path.splitext(os.path.basename(fn))
        if cmdline_tasks and name not in cmdline_tasks:
            continue
        module = importlib.import_module(f'test.pgo_task.{name}')
        if not hasattr(module, 'run_pgo'):
            print('task module missing run_pgo()', fn)
            continue
        t0 = time.perf_counter()
        print(f'{name:>40}', end='', flush=True)
        for _ in range(args.iterations):
            module.run_pgo()
        tm = time.perf_counter() - t0
        total_time += tm
        print(f' {tm:.3f}s')
    print(f'Total time for tasks {total_time:.3f} seconds')


if __name__ == '__main__':
    main()
