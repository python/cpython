# Live video input class.
# Note that importing this module attempts to initialize video.


# Check if video is available.
# There are three reasons for failure here:
# (1) this version of Python may not have the sv or imageop modules;
# (2) this machine may not have a video board;
# (3) initializing the video board may fail for another reason.
# The global variable have_video is set to true iff we reall do have video.

try:
	import sv
	import SV
	import imageop
	try:
		v = sv.OpenVideo()
		have_video = 1
	except sv.error:
		have_video = 0
except ImportError:
	have_video = 0


# The live video input class.
# Only instantiate this if have_video is true!

class LiveVideoIn:

	# Initialize an instance.
	# Parameters:
	# - vw, vh specify the size of the video window.
	# This initializes continuous capture.

	def init(self, pktmax, vw, vh):
		if not have_video:
			raise RuntimeError, 'no video available'
		realvw = vh*SV.PAL_XMAX/SV.PAL_YMAX
		if realvw < vw:
			print 'Funny, image too narrow...'
		self.realwidth, self.realheight = v.QuerySize(realvw, vh)
		##print 'Recording video in size', \
		##	self.realwidth, self.realheight
		self.width = vw
		self.height = vh
		self.x0 = (self.realwidth-self.width)/2
		self.x1 = self.x0 + self.width - 1
		self.y0 = (self.realheight-self.height)/2
		self.y1 = self.y0 + self.height - 1
		# Compute # full lines per packet
		self.lpp = pktmax / self.width
		self.pktsize = self.lpp*self.width
		##print 'lpp =', self.lpp, '; pktsize =', self.pktsize
		# Initialize capture
		v.SetSize(self.realwidth, self.realheight)
		dummy = v.InitContinuousCapture(SV.RGB8_FRAMES, \
			  self.realwidth, self.realheight, 2, 5)
		self.data = None
		self.lpos = 0
		return self

	# Remove an instance.
	# This turns off continuous capture.

	def close(self):
		v.EndContinuousCapture()

	# Get the next video packet.
	# This returns (lpos, data) where:
	# - lpos is the line position
	# - data is a piece of data
	# The dimensions of data are:
	# - pixel depth = 1 byte
	# - scan line width = self.width (the vw argument to init())
	# - number of scan lines = self.lpp (PKTMAX / vw)

	def getnextpacket(self):
		if not self.data:
			try:
				cd, id = v.GetCaptureData()
			except sv.error:
				return None
			data = cd.InterleaveFields(1)
			cd.UnlockCaptureData()
			self.data = imageop.crop(data, 1, \
				  self.realwidth, \
				  self.realheight, \
				  self.x0, self.y0, \
				  self.x1, self.y1)
			self.lpos = 0
		data = self.data[:self.pktsize]
		self.data = self.data[self.pktsize:]
		lpos = self.lpos
		self.lpos = self.lpos + self.lpp
		return lpos, data
