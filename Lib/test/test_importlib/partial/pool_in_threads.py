import multiprocessing
import os
import threading
import traceback


def t():
    try:
        with multiprocessing.Pool(1):
            pass
    except Exception:
        traceback.print_exc()
        os._exit(1)


def main():
    threads = []
    for i in range(20):
        threads.append(threading.Thread(target=t))
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()


if __name__ == "__main__":
    main()
