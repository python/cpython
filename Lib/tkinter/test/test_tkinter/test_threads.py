#!/usr/bin/python3

import sys
import time
import threading
import random
import tkinter

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

class Track(threading.Thread):
    """
    Calculates coordinates for a ballistic track
    with random angle and velocity
    and fires the callback to draw each consecutive segment.
    """
    def __init__(self, track_number, draw_callback):
        """
        :param track_number: ordinal
        :param draw_callback: fn(track_number, x, y)
            that draws the extention of the specified track
            to the specified point. The callback must keep track
            of any previous coordinates itself.
        """
        threading.Thread.__init__(self)
        #self.setDaemon(True)
        self.track_number = track_number
        self.draw_callback = draw_callback

    def run(self):
        #starting point for the track
        y = 0.0001 #height
        x = 999.0 #range
        #initial velocities
        yVel = 400. + random.random() * 200.
        xVel = -200. - random.random() * 150.
        # Stop drawing when the track hits the ground
        while y > 0 and running:
            #How long to sleep, in seconds, between track updates
            time.sleep(0.01) #realism: >1   Fun: 0.01

            yVel -=  9.8 #gravity, more or less
            xVel *= .9998 #air resistance

            y += yVel
            x += xVel

            if running:
                self.draw_callback(self.track_number, x, y)


class App:
    """
    The main app logic
    """
    def __init__(self, window):
        self.track_no = 0 #last index of track added
        self.track_coordinates = {}  #coords of track tips
        self.threads = []

        self.window = window
        self.frame = tkinter.Frame(window)
        self.frame.pack()
        self.graph = tkinter.Canvas(self.frame)
        self.graph.pack()

        self.t_cleanup = threading.Thread(target=self.tf_cleanup)
        self.window.after(0, self.add_tracks, 5)
        self.window.after(1000, self.t_cleanup.start)

    def add_tracks(self,depth):
        if depth>0: self.window.after(5, self.add_tracks, depth-1)
        tracks_to_add = 40
        start_no = self.track_no
        self.track_no += tracks_to_add
        for self.track_no in range(start_no, self.track_no):
            #self.window.after(t, self.add_tracks)
            #self.track_no += 1 #Make a new track number
            #if (self.track_no > 40):
            t = Track(self.track_no, self.draw_track_segment)
            self.threads.append(t)
            t.start()

    def tf_cleanup(self):
        global running
        running = False
        for t in self.threads: t.join()
        self.window.after(0,self.window.destroy)

    def draw_track_segment(self, track_no, x, y):
        # x & y are virtual coordinates for the purpose of simulation.
        #To convert them to screen coordinates,
        # we scale them down so the graphs fit into the window, and move the origin.
        # Y is also inverted because in canvas, the UL corner is the origin.
        newsx, newsy = (250.+x/100., 150.-y/100.)

        try:
            (oldsx, oldsy) = self.track_coordinates[track_no]
        except KeyError:
            pass
        else:
            self.graph.create_line(oldsx, oldsy,
                                   newsx, newsy)
        self.track_coordinates[track_no] = (newsx, newsy)

    def go(self):
        self.window.mainloop()

tests_gui = (TestThreads,)

if __name__=='__main__':
    import sys,os
    if sys.argv[1:]==['run']:
        if os.name == 'nt':
            import ctypes
            #the bug causes crashes
            ctypes.windll.kernel32.SetErrorMode(3)
        App(tkinter.Tk()).go()
    else:
        from test.support import run_unittest
        run_unittest(*tests_gui)
