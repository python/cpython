# Module 'Soundogram'

import audio
from Histogram import Histogram

class Soundogram() = Histogram():
	#
	def define(self, (win, chunk)):
		width, height = corner = win.getwinsize()
		bounds = (0, 0), corner
		self.chunk = chunk
		self.step = (len(chunk)-1)/(width/2+1) + 1
		ydata = _make_ydata(chunk, self.step)
		return Histogram.define(self, (win, bounds, ydata, (0, 128)))
	#
	def setchunk(self, chunk):
		self.chunk = chunk
		self.recompute()
	#
	def recompute(self):
		(left, top), (right, bottom) = self.bounds
		width = right - left
		self.step = (len(chunk)-1)/width + 1
		ydata = _make_ydata(chunk, self.step)
		self.setdata(ydata, (0, 128))
	#


def _make_ydata(chunk, step):
	ydata = []
	for i in range(0, len(chunk), step):
		piece = audio.chr2num(chunk[i:i+step])
		mi, ma = min(piece), max(piece)
		y = max(abs(mi), abs(ma))
		ydata.append(y)
	return ydata
