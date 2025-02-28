import sys
import glob
import importlib
import os
import time


def main():
    cmdline_tasks = set(sys.argv[1:])
    tasks = []
    package_dir = os.path.dirname(__file__)
    for fn in glob.glob(os.path.join(package_dir, 'bm_*.py')):
        tasks.append(fn)
    total_time = 0
    for fn in sorted(tasks):
        name, ext = os.path.splitext(os.path.basename(fn))
        if cmdline_tasks and name not in cmdline_tasks:
            continue
        module = importlib.import_module(f'test.pgo_task.{name}')
        if not hasattr(module, 'run_pgo'):
            print('missing run_pgo', fn)
            continue
        t0 = time.perf_counter()
        print('Running task', name, end='...')
        module.run_pgo()
        tm = time.perf_counter() - t0
        total_time += tm
        print(f' {tm:.3f} seconds.')
    print(f'Total time for tasks {total_time:.3f} seconds')


if __name__ == '__main__':
    main()
