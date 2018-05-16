from __future__ import print_function
import tkinter
import random
import string
import sys
import threading
import time

import unittest
class TestThreads(unittest.TestCase):
    def test_threads(self):
        import subprocess
        p = subprocess.Popen([sys.executable, __file__, "run"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        try:
            #Test code is designed to complete in a few seconds
            stdout, stderr = p.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            p.kill()
            stdout, stderr = p.communicate()
            self.fail("Test code hang. Stderr: " + repr(stderr))
        rc = p.returncode
        self.assertTrue(rc == 0,
                        "Nonzero exit status: " + str(rc) + "; stderr:" + repr(stderr))
        self.assertTrue(len(stderr) == 0, "stderr: " + repr(stderr))



running = True

class EventThread(threading.Thread):
    def __init__(self,target):
        super(EventThread,self).__init__()
        self.target = target
    def run(self):
        while running:
            time.sleep(0.02)
            c = random.choice(string.ascii_letters)
            self.target.event_generate(c)

class Main(object):
    def __init__(self):
        self.root = tkinter.Tk()
        self.root.bind('<Key>',dummy_handler)
        self.threads=[]

        self.t_cleanup = threading.Thread(target=self.tf_stop)

    def go(self):
        self.root.after(0,self.add_threads)
        self.root.after(500,self.stop)
        self.root.protocol("WM_DELETE_WINDOW",self.stop)
        self.root.mainloop()
        self.t_cleanup.join()
    def stop(self):
        self.t_cleanup.start()
    def tf_stop(self):
        global running
        running = False
        for t in self.threads: t.join()
        self.root.after(0,self.root.destroy)
    def add_threads(self):
        for _ in range(20):
            t = EventThread(self.root)
            self.threads.append(t)
            t.start()

def dummy_handler(event):
    pass


tests_gui = (TestThreads,)

if __name__=='__main__':
    import sys,os
    if sys.argv[1:]==['run']:
        if os.name == 'nt':
            import ctypes
            #the bug causes crashes
            ctypes.windll.kernel32.SetErrorMode(3)
        Main().go()
    else:
        from test.support import run_unittest
        run_unittest(*tests_gui)
