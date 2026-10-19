import multiprocessing
import sys

print(__name__, end='', file=sys.stderr)
print(__name__, end='', file=sys.stdout)
if __name__ == '__main__':
    multiprocessing.set_start_method('forkserver')
    for _ in range(2):
        p = multiprocessing.Process()
        p.start()
        p.join()
