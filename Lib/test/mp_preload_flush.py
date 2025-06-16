import multiprocessing
import sys

if __name__ == '__main__':
    assert 'a' not in sys.modules
    multiprocessing.set_start_method('forkserver')
    multiprocessing.set_forkserver_preload(['a'])
    for _ in range(2):
        p = multiprocessing.Process()
        p.start()
        p.join()
else:
    assert 'a' in sys.modules
