# Live video input from display class.

import gl
import GL

# The live video input class.
# Only instantiate this if have_video is true!

class DisplayVideoIn:

	# Initialize an instance.  Arguments:
	# vw, vh: size of the video window data to be captured.
	# position defaults to 0, 0 but can be set later
	def __init__(self, pktmax, vw, vh, type):
		self.pktmax = pktmax
		self.realwidth, self.realheight = vw, vh
		if type <> 'rgb':
			raise 'Incorrent video data type', type
		self.type = type
		self.width = vw
		self.height = vh
		#
		# Open dummy window
		#
		gl.foreground()
		gl.noport()
		self.wid = gl.winopen('DisplayVideoIn')
		
		self.x0 = 0
		self.x1 = self.x0 + self.width - 1
		self.y0 = 0
		self.y1 = self.y0 + self.height - 1
		# Compute # full lines per packet
		self.lpp = pktmax / self.linewidth()
		if self.lpp <= 0:
			raise 'No lines in packet', self.linewidth()
		self.pktsize = self.lpp*self.linewidth()
		self.data = None
		self.old_data = None
		self.dataoffset = 0
		self.lpos = 0
		self.hints = 0

	# Change the size of the video being displayed.

	def resizevideo(self, vw, vh):
		self.width = vw
		self.height = vh
		self.x1 = self.x0 + self.width - 1
		self.y1 = self.y0 + self.height - 1

	def positionvideo(self, x, y):
		self.x0 = x
		self.y0 = y
		self.x1 = self.x0 + self.width - 1
		self.y1 = self.y0 + self.height - 1

	# Remove an instance.
	# This turns off continuous capture.

	def close(self):
		gl.winclose(self.wid)

	# Get the length in bytes of a video line
	def linewidth(self):
		return self.width*4

	# Get the next video packet.
	# This returns (lpos, data) where:
	# - lpos is the line position
	# - data is a piece of data
	# The dimensions of data are:
	# - pixel depth = 1 byte
	# - scan line width = self.width (the vw argument to __init__())
	# - number of scan lines = self.lpp (PKTMAX / vw)

	def getnextpacket(self):
		if not self.data or self.dataoffset >= len(self.data):
			self.old_data = self.data
			self.data = gl.readdisplay(self.x0, self.y0, \
				  self.x1, self.y1, self.hints)
			self.dataoffset = 0
			self.lpos = 0
		data = self.data[self.dataoffset:self.dataoffset+self.pktsize]
		while self.old_data and \
			  self.dataoffset+self.pktsize < len(self.data):
			odata = self.old_data[self.dataoffset: \
				  self.dataoffset+self.pktsize]
			if odata <> data:
				break
			print 'skip', self.lpos
			self.lpos = self.lpos + self.lpp
			self.dataoffset = self.dataoffset + self.pktsize
			data = self.data[self.dataoffset:\
				  self.dataoffset+self.pktsize]
		lpos = self.lpos
		self.dataoffset = self.dataoffset + self.pktsize
		self.lpos = self.lpos + self.lpp
		return lpos, data
