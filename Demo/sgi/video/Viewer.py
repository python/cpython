import gl, GL
import VFile
import os


class InputViewer:

	def init(self, filename, title, qsize):
		try:
			self.vin = VFile.VinFile().init(filename)
		except (EOFError, VFile.Error):
			raise IOError, 'bad video input file'
		if not title:
			title = os.path.split(filename)[1]
		self.filename = filename
		self.title = title
		self.qsize = qsize
		gl.foreground()
		gl.prefsize(self.vin.width, self.vin.height)
		self.wid = -1
		self.reset()
		return self

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
		self.queue = []
		self.qindex = 0
		self.lost = 0
		self.lastt = 0
		self.eofread = 0

	# Internal
	def fillq(self):
		if self.qindex < len(self.queue) or self.eofread: return
		try:
			t, d, cd = self.vin.getnextframe()
		except EOFError:
			self.eofread = 1
			return
		dt = t - self.lastt
		self.lastt = t
		self.queue.append(dt, d, cd)
		while len(self.queue) > self.qsize:
			del self.queue[0]
			self.qindex = self.qindex - 1
			self.lost = self.lost + 1

	def show(self):
		if self.wid < 0:
			gl.foreground()
			gl.prefsize(self.vin.width, self.vin.height)
			self.wid = gl.winopen(self.title)
			gl.clear()
			self.vin.initcolormap()
		self.fillq()
		gl.winset(self.wid)
		if self.qindex >= len(self.queue):
			self.vin.clear()
			return
		dt, d, cd = self.queue[self.qindex]
		self.vin.showframe(d, cd)

	def redraw(self, wid):
		if wid == self.wid >= 0:
			gl.winset(self.wid)
			gl.reshapeviewport()
			self.vin.clear()
			self.show()

	def get(self):
		if self.qindex >= len(self.queue):
			self.fillq()
			if self.eofread:
				return None
		item = self.queue[self.qindex]
		self.qindex = self.qindex + 1
		return item

	def backup(self):
		if self.qindex == 0:
			return 0
		self.qindex = self.qindex - 1
		return 1

	def tell(self):
		return self.lost + self.qindex

	def qsizes(self):
		return self.qindex, len(self.queue) - self.qindex


class OutputViewer:

	def init(self, filename, title, qsize):
		try:
			self.vout = VFile.VoutFile().init(filename)
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
		return self

	def close(self):
		while self.queue:
			self.flushq()
		self.vout.close()
		if self.wid > 0:
			gl.winclose(self.wid)

	def rewind(self):
		info = self.vout.getinfo()
		self.vout.close()
		self.vout = VFile.VoutFile().init(self.filename)
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

	def tell(self):
		return self.written + len(self.queue)

	def qsizes(self):
		return len(self.queue), len(self.spares)


def test():
	import sys
	a = InputViewer().init(sys.argv[1], '')
	b = OutputViewer().init(sys.argv[2], '')
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
