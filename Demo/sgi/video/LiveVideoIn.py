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

	# Initialize an instance.  Arguments:
	# vw, vh: size of the video window data to be captured.
	# For some reason, vw MUST be a multiple of 4.
	# Note that the data has to be cropped unless vw and vh are
	# just right for the video board (vw:vh == 4:3 and vh even).

	def __init__(self, pktmax, vw, vh, type):
		if not have_video:
			raise RuntimeError, 'no video available'
		if vw % 4 != 0:
			raise ValueError, 'vw must be a multiple of 4'
		self.pktmax = pktmax
		realvw = vh*SV.PAL_XMAX/SV.PAL_YMAX
		if realvw < vw:
			realvw = vw
		self.realwidth, self.realheight = v.QuerySize(realvw, vh)
		if not type in ('rgb8', 'grey', 'mono', 'grey2', 'grey4'):
			raise 'Incorrent video data type', type
		self.type = type
		if type in ('grey', 'grey4', 'grey2', 'mono'):
			v.SetParam([SV.COLOR, SV.MONO, SV.INPUT_BYPASS, 1])
		else:
			v.SetParam([SV.COLOR, SV.DEFAULT_COLOR, \
				  SV.INPUT_BYPASS, 0])
		# Initialize capture
		(mode, self.realwidth, self.realheight, qsize, rate) = \
			v.InitContinuousCapture(SV.RGB8_FRAMES, \
				self.realwidth, self.realheight, 1, 2)
		self.width = vw
		self.height = vh
		self.x0 = (self.realwidth-self.width)/2
		self.x1 = self.x0 + self.width - 1
		self.y0 = (self.realheight-self.height)/2
		self.y1 = self.y0 + self.height - 1
		# Compute # full lines per packet
		self.lpp = pktmax / self.linewidth()
		self.pktsize = self.lpp*self.linewidth()
		self.data = None
		self.dataoffset = 0
		self.lpos = 0
		self.justright = (self.realwidth == self.width and \
			self.realheight == self.height)
##		if not self.justright:
##			print 'Want:', self.width, 'x', self.height,
##			print '; grab:', self.realwidth, 'x', self.realheight

	# Change the size of the video being displayed.

	def resizevideo(self, vw, vh):
		self.close()
		self.__init__(self.pktmax, vw, vh, self.type)

	# Remove an instance.
	# This turns off continuous capture.

	def close(self):
		v.EndContinuousCapture()

	# Get the length in bytes of a video line
	def linewidth(self):
		if self.type == 'mono':
			return (self.width+7)/8
		elif self.type == 'grey2':
			return (self.width+3)/4
		elif self.type == 'grey4':
			return (self.width+1)/2
		else:
			return self.width

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
			try:
				cd, id = v.GetCaptureData()
			except sv.error:
				return None
			data = cd.InterleaveFields(1)
			cd.UnlockCaptureData()
			if self.justright:
				self.data = data
			else:
				self.data = imageop.crop(data, 1, \
					  self.realwidth, \
					  self.realheight, \
					  self.x0, self.y0, \
					  self.x1, self.y1)
			self.lpos = 0
			self.dataoffset = 0
			if self.type == 'mono':
				self.data = imageop.dither2mono(self.data, \
					  self.width, self.height)
			elif self.type == 'grey2':
				self.data = imageop.dither2grey2(self.data, \
					  self.width, self.height)
			elif self.type == 'grey4':
				self.data = imageop.grey2grey4(self.data, \
					  self.width, self.height)
		data = self.data[self.dataoffset:self.dataoffset+self.pktsize]
		lpos = self.lpos
		self.dataoffset = self.dataoffset + self.pktsize
		self.lpos = self.lpos + self.lpp
		return lpos, data
