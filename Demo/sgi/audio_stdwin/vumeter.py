#! /usr/local/python

import audio
import stdwin

from VUMeter import VUMeter
from WindowParent import WindowParent
import MainLoop

NBUFS=20
BUFSIZE = NBUFS*48
SCALE=128

class MyVUMeter(VUMeter):
	def init_reactivity(self):
		self.parent.need_mouse(self)
	def mouse_down(self, detail):
		if self.enabled:
			self.stop()
		else:
			self.start()
	def mouse_move(self, detail): pass
	def mouse_up(self, detail): pass

def main():
	audio.setrate(3)
	audio.setoutgain(0)
	w = WindowParent().create('VU Meter', (200, 100))
	v = MyVUMeter().define(w)
	v.start()
	w.realize()
	while 1:
		w.dispatch(stdwin.getevent())

main()
