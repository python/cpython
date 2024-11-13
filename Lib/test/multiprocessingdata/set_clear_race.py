import multiprocessing
import sys


# Reproduction code copied and modified from https://github.com/python/cpython/issues/95826
class SimpleRepro:
    def __init__(self):
        self.heartbeat_event = multiprocessing.Event()
        self.shutdown_event = multiprocessing.Event()
        self.child_proc = multiprocessing.Process(target=self.child_process, daemon=True)
        self.child_proc.start()

    def child_process(self):
        while True:
            if self.shutdown_event.is_set():
                return
            self.heartbeat_event.set()
            self.heartbeat_event.clear()

    def test_heartbeat(self):
        exit_code = 0
        for i in range(2000):
            success = self.heartbeat_event.wait(100)
            if not success:
                exit_code = 1
                break
        self.shutdown_event.set()
        self.child_proc.join()
        sys.exit(exit_code)


if __name__ == '__main__':
    foo = SimpleRepro()
    foo.test_heartbeat()
