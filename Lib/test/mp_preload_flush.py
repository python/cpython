import multiprocessing
if __name__ == '__main__':
    multiprocessing.set_forkserver_preload(['a'])
    for _ in range(2):
        p = multiprocessing.Process()
        p.start()
        p.join()
