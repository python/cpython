import gl, GL
import VFile
import os


class InputViewer:

	def __init__(self, filename, title, *args):
		try:
			self.vin = VFile.VinFile(filename)
		except (EOFError, VFile.Error):
			raise IOError, 'bad video input file'
		self.vin.warmcache()
		if not title:
			title = os.path.split(filename)[1]
		self.filename = filename
		self.title = title
		self.qsize = len(self.vin.index)
		gl.foreground()
		gl.prefsize(self.vin.width, self.vin.height)
		self.wid = -1
		self.reset()

	def close(self):
		self.vin.close()
		if self.wid > 0:
			gl.winclose(self.wid)

	def rewind(self):
		self.vin.rewind()
		self.reset()

	def getinfo(self):
		return self.vin.getinfo()

	# Internal
	def reset(self):
		if self.wid > 0:
			gl.winset(self.wid)
			gl.clear()
			self.vin.initcolormap()
		self.qindex = 0

	def show(self):
		if self.wid < 0:
			gl.foreground()
			gl.prefsize(self.vin.width, self.vin.height)
			self.wid = gl.winopen(self.title)
			gl.clear()
			self.vin.initcolormap()
		gl.winset(self.wid)
		if self.qindex >= self.qsize:
			self.vin.clear()
			return
		dt, d, cd = self.vin.getrandomframe(self.qindex)
		self.vin.showframe(d, cd)

	def redraw(self, wid):
		if wid == self.wid >= 0:
			gl.winset(self.wid)
			gl.reshapeviewport()
			self.vin.clear()
			self.show()

	def get(self):
		if self.qindex >= self.qsize:
			return None
		if self.qindex > 0:
			prevt, ds, cs = \
				  self.vin.getrandomframeheader(self.qindex-1)
		else:
			prevt = 0
		t, data, cdata = self.vin.getrandomframe(self.qindex)
		self.qindex = self.qindex + 1
		return t-prevt, data, cdata

	def backup(self):
		if self.qindex == 0:
			return 0
		self.qindex = self.qindex - 1
		return 1

	def seek(self, i):
		if not 0 <= i <= self.qsize:
			return 0
		self.qindex = i
		return 1

	def tell(self):
		return self.qindex

	def qsizes(self):
		return self.qindex, self.qsize - self.qindex

	def qinfo(self):
		return 0, self.qindex, self.qsize


class OutputViewer:

	def __init__(self, filename, title, qsize):
		try:
			self.vout = VFile.VoutFile(filename)
		except (EOFError, VFile.Error):
			raise IOError, 'bad video output file'
		if not title:
			title = os.path.split(filename)[1]
		self.filename = filename
		self.title = title
		self.qsize = qsize
		gl.foreground()
		self.wid = -1
		self.reset()

	def close(self):
		while self.queue:
			self.flushq()
		self.vout.close()
		if self.wid > 0:
			gl.winclose(self.wid)

	def rewind(self):
		info = self.vout.getinfo()
		self.vout.close()
		self.vout = VFile.VoutFile(self.filename)
		self.vout.setinfo(info)
		self.reset()

	def getinfo(self):
		return self.vout.getinfo()

	def setinfo(self, info):
		if info == self.getinfo(): return # No change
		self.vout.setinfo(info)
		if self.wid > 0:
			gl.winclose(self.wid)
			self.wid = -1

	# Internal
	def reset(self):
		if self.wid > 0:
			gl.winset(self.wid)
			gl.clear()
			self.vout.initcolormap()
		self.queue = []
		self.spares = []
		self.written = 0
		self.lastt = 0

	# Internal
	def flushq(self):
		if self.written == 0:
			self.vout.writeheader()
		dt, d, cd = self.queue[0]
		self.lastt = self.lastt + dt
		self.vout.writeframe(self.lastt, d, cd)
		del self.queue[0]
		self.written = self.written + 1

	def show(self):
		if self.wid < 0:
			gl.foreground()
			gl.prefsize(self.vout.width, self.vout.height)
			self.wid = gl.winopen(self.title)
			gl.clear()
			self.vout.initcolormap()
		gl.winset(self.wid)
		if not self.queue:
			self.vout.clear()
			return
		dt, d, cd = self.queue[-1]
		self.vout.showframe(d, cd)

	def redraw(self, wid):
		if wid == self.wid >= 0:
			gl.winset(self.wid)
			gl.reshapeviewport()
			self.vout.clear()
			self.show()

	def backup(self):
		if len(self.queue) < 1: return 0
		self.spares.insert(0, self.queue[-1])
		del self.queue[-1]
		return 1

	def forward(self):
		if not self.spares: return 0
		self.queue.append(self.spares[0])
		del self.spares[0]
		return 1

	def put(self, item):
		self.queue.append(item)
		self.spares = []
		while len(self.queue) > self.qsize:
			self.flushq()

	def seek(self, i):
		i = i - self.written
		if not 0 <= i <= len(self.queue) + len(self.spares):
			return 0
		while i < len(self.queue):
			if not self.backup():
				return 0
		while i > len(self.queue):
			if not self.forward():
				return 0
		return 1

	def trunc(self):
		del self.spares[:]

	def tell(self):
		return self.written + len(self.queue)

	def qsizes(self):
		return len(self.queue), len(self.spares)

	def qinfo(self):
		first = self.written
		pos = first + len(self.queue)
		last = pos + len(self.spares)
		return first, pos, last


def test():
	import sys
	a = InputViewer(sys.argv[1], '')
	b = OutputViewer(sys.argv[2], '')
	b.setinfo(a.getinfo())
	
	while 1:
		a.show()
		data = a.get()
		if data is None:
			break
		b.put(data)
		b.show()

	while a.backup():
		data = a.get()
		b.put(data)
		b.show()
		if a.backup(): a.show()
	
	while 1:
		data = a.get()
		if data is None:
			break
		b.put(data)
		b.show()
		a.show()

	b.close()
