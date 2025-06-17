import multiprocessing
import sys

modname = 'preloaded_module'
if __name__ == '__main__':
    assert modname not in sys.modules
    multiprocessing.set_start_method('forkserver')
    multiprocessing.set_forkserver_preload([modname])
    for _ in range(2):
        p = multiprocessing.Process()
        p.start()
        p.join()
else:
    assert modname in sys.modules
