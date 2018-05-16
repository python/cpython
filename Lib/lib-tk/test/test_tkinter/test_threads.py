from __future__ import print_function
import Tkinter as tkinter
import random
import string
import sys
import threading
import time

import unittest


class TestThreads(unittest.TestCase):
    def setUp(self):
        self.stderr = ""

    def test_threads(self):
        import subprocess
        p = subprocess.Popen([sys.executable, __file__, "run"],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)

        rt = threading.Thread(target=reader_thread, args=(p.stdout, self.stderr))
        rt.start()

        try:
            t = time.time()
            # Test code is designed to complete in a few seconds
            while time.time() - t < 10:
                if p.poll() is not None: break
                time.sleep(0.2)
            else:
                p.kill()
                setattr(self, 'killed', True)
        finally:
            p.stdout.close()
            rt.join()
        rc = p.returncode
        if hasattr(self, 'killed'):
            self.fail("Test code hang. Stderr: " + repr(self.stderr))
        self.assertTrue(rc == 0,
                        "Nonzero exit status: " + str(rc) + "; stderr:" + repr(self.stderr))
        self.assertTrue(len(self.stderr) == 0, "stderr: " + repr(self.stderr))


def reader_thread(stream, output):
    while True:
        s = stream.read()
        if not s: break
        output += s


running = True


class EventThread(threading.Thread):
    def __init__(self, target):
        threading.Thread.__init__(self)
        self.target = target

    def run(self):
        while running:
            time.sleep(0.02)
            c = random.choice(string.ascii_letters)
            self.target.event_generate(c)


class Main(object):
    def __init__(self):
        self.root = tkinter.Tk()
        self.root.bind('<Key>', dummy_handler)
        self.threads = []

        self.t_cleanup = threading.Thread(target=self.tf_stop)

    def go(self):
        self.root.after(0, self.add_threads)
        self.root.after(500, self.stop)
        self.root.protocol("WM_DELETE_WINDOW", self.stop)
        self.root.mainloop()
        self.t_cleanup.join()

    def stop(self):
        self.t_cleanup.start()

    def tf_stop(self):
        global running
        running = False
        for t in self.threads: t.join()
        self.root.after(0, self.root.destroy)

    def add_threads(self):
        for _ in range(20):
            t = EventThread(self.root)
            self.threads.append(t)
            t.start()


def dummy_handler(event):
    pass


tests_gui = (TestThreads,)

if __name__ == '__main__':
    import os

    if sys.argv[1:] == ['run']:
        if os.name == 'nt':
            import ctypes

            # the bug causes crashes
            ctypes.windll.kernel32.SetErrorMode(3)
        Main().go()
    else:
        from test.support import run_unittest

        run_unittest(*tests_gui)
