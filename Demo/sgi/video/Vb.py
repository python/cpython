#! /usr/bin/env python

# Video bag of tricks: record video(+audio) in various formats and modes

# XXX To do:
# - audio
# - improve user interface
# - help button?
# - command line options to set initial settings
# - save settings in a file
# - ...?

import sys
import time
import getopt
import string
import os
sts = os.system('makemap')		# Must be before "import fl" to work
import sgi
import gl
import GL
import DEVICE
import fl
import FL
import flp
import watchcursor
import sv
import SV
import VFile
import VGrabber
import imageop
sys.path.append('/ufs/jack/src/av/vcr')
import VCR
try:
	import cl
except ImportError:
	cl = None

ARROW = 0
WATCH = 1
watchcursor.defwatch(WATCH)

def main():
##	fl.set_graphics_mode(0, 1)
	vb = VideoBagOfTricks()
	while 1:
		dummy = fl.do_forms()
		[dummy]

StopCapture = 'StopCapture'

VideoFormatLabels = ['Video off', 'rgb8', 'grey8', 'grey4', 'grey2', \
	  'grey2 dith', 'mono dith', 'mono thresh', 'rgb24', 'rgb24-jpeg', \
	  'compress']
VideoFormats = ['', 'rgb8', 'grey', 'grey4', 'grey2', \
	  'grey2', 'mono', 'mono', 'rgb', 'jpeg', 'compress']

VideoModeLabels = ['Continuous', 'Burst', 'Single frame', 'VCR sync']
[VM_CONT, VM_BURST, VM_SINGLE, VM_VCR] = range(1, 5)

AudioFormatLabels = ['Audio off', \
	  '16 bit mono', '16 bit stereo', '8 bit mono', '8 bit stereo']
[A_OFF, A_16_MONO, A_16_STEREO, A_8_MONO, A_8_STEREO] = range(1, 6)

VcrSpeedLabels = ['normal', '1/3', '1/5', '1/10', '1/30', 'single-step']
VcrSpeeds = [None, 5, 4, 3, 2, 1, 0]

RgbSizeLabels = ['full', 'quarter', 'sixteenth']

# init file stuff:
if os.environ.has_key('HOME'):
	HOME=os.environ['HOME']
else:
	HOME='.'
VB_INIT_FILE=HOME + '/.Vb_init'

VB_INIT_KEYS=['vfile', 'vmode', 'mono_thresh', 'vformat', 'comp_scheme', \
	  'rgb24_size', 'afile', 'aformat']

class VideoBagOfTricks:

	# Init/close stuff

	def __init__(self):
		self.window = None
		formdef = flp.parse_form('VbForm', 'form')
		flp.create_full_form(self, formdef)
		self.setdefaults()
		if self.vmode <> VM_CONT:
			self.g_cont.hide_object()
		if self.vmode <> VM_BURST:
			self.g_burst.hide_object()
		if self.vmode <> VM_SINGLE:
			self.g_single.hide_object()
		if self.vmode <> VM_VCR:
			self.g_vcr.hide_object()
		if self.vformat <> 'compress':
			self.g_compress.hide_object()

		self.openvideo()
		self.makewindow()
		self.bindvideo()
		if self.use_24:
			self.optfullsizewindow()
		self.showform()
		fl.set_event_call_back(self.do_event)

	def close(self):
		self.close_video()
		self.close_audio()
		self.savedefaults()
		raise SystemExit, 0

	def showform(self):
		# Get position of video window
		gl.winset(self.window)
		x, y = gl.getorigin()
		width, height = gl.getsize()
		# Calculate position of form window
		x1 = x + width + 10
		x2 = x1 + int(self.form.w) - 1
		y2 = y + height - 1
		y1 = y2 - int(self.form.h) + 1
		# Position and show form window
		gl.prefposition(x1, x2, y1, y2)
		self.form.show_form(FL.PLACE_FREE, FL.TRUE, 'Vb Control')

	def getdefaultdefaults(self):
		# Video defaults
		self.vfile = 'film.video'
		self.vmode = VM_CONT
		self.mono_thresh = 128
		self.vformat = 'rgb8'
		self.comp_scheme = 'Uncompressed'
		self.rgb24_size = 1
		# Missing: drop, rate, maxmem, nframes, rate, vcrspeed
		# Audio defaults:
		self.afile = 'film.aiff'
		self.aformat = A_OFF

	def getdefaults(self):
		self.getdefaultdefaults()
		# XXXX Read defaults file and override.
		try:
			fp = open(VB_INIT_FILE, 'r')
		except IOError:
			print 'Vb: no init file'
			self.initcont = {}
			return
		data = fp.read(1000000)
		try:
			self.initcont = eval(data)
		except:
			print 'Vb: Ill-formatted init file'
			self.initcont = {}
		for k in self.initcont.keys():
			if hasattr(self, k):
				setattr(self, k, self.initcont[k])

	def savedefaults(self):
		newdb = {}
		for k in VB_INIT_KEYS:
			newdb[k] = getattr(self, k)
		if newdb <> self.initcont:
			try:
				fp = open(VB_INIT_FILE, 'w')
			except IOError:
				print 'Vb: Cannot create', VB_INIT_FILE
				return
			fp.write(`newdb`)
			fp.close()
			
	def setdefaults(self):
		self.getdefaults()
		self.vcr = None
		self.vout = None
		self.capturing = 0
		self.c_vformat.clear_choice()
		for label in VideoFormatLabels:
			self.c_vformat.addto_choice(label)
		self.c_vformat.set_choice(1 + VideoFormats.index(self.vformat))
		self.c_vmode.clear_choice()
		for label in VideoModeLabels:
			self.c_vmode.addto_choice(label)
		self.c_vmode.set_choice(self.vmode)
		self.get_vformat()
		self.b_drop.set_button(1)
		self.in_rate.set_input('2')
		self.in_maxmem.set_input('1.0')
		self.in_nframes.set_input('0')
		self.in_nframes_vcr.set_input('1')
		self.in_rate_vcr.set_input('1')
		self.c_vcrspeed.clear_choice()
		for label in VcrSpeedLabels:
			self.c_vcrspeed.addto_choice(label)
		self.c_vcrspeed.set_choice(4)
		self.c_rgb24_size.clear_choice()
		for label in RgbSizeLabels:
			self.c_rgb24_size.addto_choice(label)
		self.c_rgb24_size.set_choice(self.rgb24_size)
		if cl:
			algs = cl.QueryAlgorithms(cl.VIDEO)
			self.all_comp_schemes = []
			for i in range(0, len(algs), 2):
				if algs[i+1] in (cl.COMPRESSOR, cl.CODEC):
					self.all_comp_schemes.append(algs[i])
			self.c_cformat.clear_choice()
			for label in self.all_comp_schemes:
				self.c_cformat.addto_choice(label)
			i = self.all_comp_schemes.index(self.comp_scheme)
			self.c_cformat.set_choice(i+1)
		# Audio defaults
		self.aout = None
		self.aport = None
		self.c_aformat.clear_choice()
		for label in AudioFormatLabels:
			self.c_aformat.addto_choice(label)
		self.c_aformat.set_choice(self.aformat)
		self.get_aformat()

	def openvideo(self):
		try:
			self.video = sv.OpenVideo()
		except sv.error, msg:
			print 'Error opening video:', msg
			self.video = None
		param = [SV.BROADCAST, SV.PAL]
		if self.video: self.video.GetParam(param)
		if param[1] == SV.PAL:
			x = SV.PAL_XMAX
			y = SV.PAL_YMAX
		elif param[1] == SV.NTSC:
			x = SV.NTSC_XMAX
			y = SV.NTSC_YMAX
		else:
			print 'Unknown video standard:', param[1]
			sys.exit(1)
		self.maxx, self.maxy = x, y
		self.curx = 256
		self.cury = 256*3/4

	def makewindow(self):
		x, y = self.maxx, self.maxy
		gl.foreground()
		gl.maxsize(x, y)
		gl.keepaspect(x, y)
		gl.stepunit(8, 6)
		width = self.curx
		height = self.cury
		if width and height:
			# Place the window at (150, 150) from top left
			# (the video board likes this location...)
			x1 = 150
			x2 = x1+width-1
			SCRHEIGHT = 768
			y2 = SCRHEIGHT-1-150
			y1 = y2-height+1
			gl.prefposition(x1, x2, y1, y2)
		self.window = gl.winopen('Vb video')
		self.settitle()
		if width:
			gl.maxsize(x, y)
			gl.keepaspect(x, y)
			gl.stepunit(8, 6)
			gl.winconstraints()
		gl.qdevice(DEVICE.LEFTMOUSE)
		gl.qdevice(DEVICE.WINQUIT)
		gl.qdevice(DEVICE.WINSHUT)

	def optfullsizewindow(self):
		if not self.window:
			return
		gl.winset(self.window)
		if self.use_24:
			x, y = self.maxx, self.maxy
		else:
			x, y = self.curx, self.cury
		left, bottom = gl.getorigin()
		width, height = gl.getsize()
		bottom = bottom+height-y
		gl.prefposition(left, left+x-1, bottom, bottom+y-1)
		gl.winconstraints()
		if not self.use_24:
			gl.keepaspect(x, y)
			gl.stepunit(8, 6)
			gl.maxsize(self.maxx, self.maxy)
			gl.winconstraints()
		self.bindvideo()

	def bindvideo(self):
		if not self.video: return
		x, y = gl.getsize()
		if not self.use_24:
			self.curx, self.cury = x, y
		self.video.SetSize(x, y)
		drop = self.b_drop.get_button()
		if drop:
			param = [SV.FIELDDROP, 1, SV.GENLOCK, SV.GENLOCK_OFF]
		else:
			param = [SV.FIELDDROP, 0, SV.GENLOCK, SV.GENLOCK_ON]
		if self.rgb:
			param = param+[SV.COLOR, SV.DEFAULT_COLOR, \
				       SV.DITHER, 1, \
				       SV.INPUT_BYPASS, 0]
		else:
			param = param+[SV.COLOR, SV.MONO, SV.DITHER, 0, \
				       SV.INPUT_BYPASS, 1]
		self.video.BindGLWindow(self.window, SV.IN_REPLACE)
		self.video.SetParam(param)

	def rebindvideo(self):
		gl.winset(self.window)
		self.bindvideo()

	def reset(self):
		self.close_video()
		self.close_audio()
		if self.vcr:
			try:
				ok = self.vcr.still()
			except VCR.error:
				pass
			self.vcr = None
		self.b_capture.set_button(0)

	# Event handler (catches resize of video window)

	def do_event(self, dev, val):
		#print 'Event:', dev, val
		if dev in (DEVICE.WINSHUT, DEVICE.WINQUIT):
			self.close()
		if dev == DEVICE.REDRAW and val == self.window:
			self.rebindvideo()
			self.settitle()

	# Video controls: format, mode, file

	def cb_vformat(self, *args):
		self.reset()
		self.get_vformat()
		if self.mono_use_thresh:
			s = `self.mono_thresh`
			s = fl.show_input('Please enter mono threshold', s)
			if s:
				try:
					self.mono_thresh = string.atoi(s)
				except string.atoi_error:
					fl.show_message('Bad input, using', \
						  `self.mono_thresh`, '')
		self.rebindvideo()

	def cb_cformat(self, *args):
		i = self.c_cformat.get_choice()
		self.comp_scheme = self.all_comp_schemes[i-1]

		
	def cb_vmode(self, *args):
		if self.vcr:
			self.vcr = None
		self.vmode = self.c_vmode.get_choice()
		self.form.freeze_form()
		self.g_cont.hide_object()
		self.g_burst.hide_object()
		self.g_single.hide_object()
		self.g_vcr.hide_object()
		if self.vmode == VM_CONT:
			self.g_cont.show_object()
		elif self.vmode == VM_BURST:
			self.g_burst.show_object()
		elif self.vmode == VM_SINGLE:
			self.g_single.show_object()
		elif self.vmode == VM_VCR:
			self.g_vcr.show_object()
		self.form.unfreeze_form()

	def cb_vfile(self, *args):
		filename = self.vfile
		hd, tl = os.path.split(filename)
		filename = fl.file_selector('Video save file:', hd, '', tl)
		if filename:
			self.reset()
			hd, tl = os.path.split(filename)
			if hd == os.getcwd():
				filename = tl
			self.vfile = filename

	# Video mode specific video controls

	def cb_rate(self, *args):
		pass

	def cb_drop(self, *args):
		self.rebindvideo()

	def cb_maxmem(self, *args):
		pass

	def cb_nframes(self, *args):
		pass

	def cb_fps(self, *args):
		pass

	def cb_nframes_vcr(self, *args):
		pass

	def cb_rate_vcr(self, *args):
		pass

	def cb_vcrspeed(self, *args):
		pass

	def cb_rgb24_size(self, *args):
		i = self.c_rgb24_size.get_choice()
		if i:
			self.rgb24_size = i

	# Audio controls: format, file

	def cb_aformat(self, *args):
		self.get_aformat()

	def cb_afile(self, *args):
		filename = self.afile
		hd, tl = os.path.split(filename)
		filename = fl.file_selector('Audio save file:', hd, '', tl)
		if filename:
			self.reset()
			hd, tl = os.path.split(filename)
			if hd == os.getcwd():
				filename = tl
			self.afile = filename

	# General controls: capture, reset, play, quit

	def cb_capture(self, *args):
		if self.capturing:
			raise StopCapture
		if not self.b_capture.get_button():
			return
		if not self.video or not self.vformat:
			gl.ringbell()
			return
		if self.vmode == VM_CONT:
			self.cont_capture()
		elif self.vmode == VM_BURST:
			self.burst_capture()
		elif self.vmode == VM_SINGLE:
			self.single_capture(None, None)
		elif self.vmode == VM_VCR:
			self.vcr_capture()

	def cb_reset(self, *args):
		self.reset()

	def cb_play(self, *args):
		self.reset()
		sts = os.system('Vplay -q ' + self.vfile + ' &')

	def cb_quit(self, *args):
		self.close()

	# Capture routines

	def burst_capture(self):
		self.setwatch()
		gl.winset(self.window)
		x, y = gl.getsize()
		if self.use_24:
			fl.show_message('Sorry, no 24 bit continuous capture yet', '', '')
			return
		vformat = SV.RGB8_FRAMES
		nframes = self.getint(self.in_nframes, 0)
		if nframes == 0:
			maxmem = self.getint(self.in_maxmem, 1.0)
			memsize = int(maxmem * 1024 * 1024)
			nframes = self.calcnframes(memsize)
		info = (vformat, x, y, nframes, 1)
		try:
			info2, data, bitvec = self.video.CaptureBurst(info)
		except sv.error, msg:
			self.b_capture.set_button(0)
			self.setarrow()
			fl.show_message('Capture error:', str(msg), '')
			return
		if info <> info2: print info, '<>', info2
		self.save_burst(info2, data, bitvec)
		self.setarrow()

	def calcnframes(self, memsize):
		gl.winset(self.window)
		x, y = gl.getsize()
		pixels = x*y
		pixels = pixels/2	# XXX always assume fields
		if self.mono or self.grey:
			n = memsize/pixels
		else:
			n = memsize/(4*pixels)
		return max(1, n)

	def save_burst(self, info, data, bitvec):
		(vformat, x, y, nframes, rate) = info
		self.open_if_closed()
		fieldsize = x*y/2
		nskipped = 0
		realframeno = 0
		tpf = 1000 / 50.0     # XXX
		for frameno in range(0, nframes*2):
			if frameno <> 0 and \
				  bitvec[frameno] == bitvec[frameno-1]:
				nskipped = nskipped + 1
				continue
			#
			# Save field.
			# XXX Works only for fields and top-to-bottom
			#
			start = frameno*fieldsize
			field = data[start:start+fieldsize]
			realframeno = realframeno + 1
			fn = int(realframeno*tpf)
			if not self.write_frame(fn, field):
				break

	def cont_capture(self):
		saved_label = self.b_capture.label
		self.b_capture.label = 'Stop\n' + saved_label
		self.open_if_closed()
		self.init_cont()
		fps = 59.64		# Fields per second
		# XXX (fps of Indigo monitor, not of PAL or NTSC!)
		tpf = 1000.0 / fps	# Time per field in msec
		self.capturing = 1
		self.start_audio()
		while 1:
			try:
				void = fl.check_forms()
			except StopCapture:
				break
			try:
				cd, id = self.video.GetCaptureData()
			except sv.error:
				sgi.nap(1)
				continue
			id = id + 2*self.rate
			data = cd.InterleaveFields(1)
			cd.UnlockCaptureData()
			t = id*tpf
			if not self.write_frame(t, data):
				break
		self.stop_audio()
		self.capturing = 0
		self.end_cont()
		if self.aout:
			# If recording audio, can't capture multiple sequences
			self.reset()
		self.b_capture.label = saved_label

	def single_capture(self, stepfunc, timecode):
		self.open_if_closed()
		self.init_cont()
		while 1:
			try:
				cd, id = self.video.GetCaptureData()
				break
			except sv.error:
				pass
			sgi.nap(1)
			if stepfunc:		# This might step the video
				d=stepfunc()	# to the next frame
		if not self.use_24:
			data = cd.InterleaveFields(1)
		else:
			x, y = self.vout.getsize()
			if self.use_compress:
				if self.rgb24_size == 1:
					data = cd.YUVtoYUV422DC(0)
				elif self.rgb24_size == 2:
					data = cd.YUVtoYUV422DC_quarter(1)
					x = x/2
					y = y/2
				elif self.rgb24_size == 3:
					data = cd.YUVtoYUV422DC_sixteenth(1)
					x = x/4
					y = y/4
			else:
				data = cd.YUVtoRGB(1)
				if self.maxx*self.maxy*4 <> len(data):
					print 'maxx,maxy,exp,got=', self.maxx,
					print self.maxy,self.maxx*self.maxy*4,
					print len(data)
					fl.showmessage('Wrong sized data')
					return 0
				if self.rgb24_size <> 1:
					data = imageop.scale(data, 4, \
						  self.maxx, self.maxy, x, y)
			if self.use_jpeg:
				import jpeg
				data = jpeg.compress(data, x, y, 4)
			if self.use_compress:
				data = self.compressor.Compress(1, data)
		cd.UnlockCaptureData()
		self.end_cont()
		if timecode == None:
			timecode = (self.nframes+1) * (1000/25)
		return self.write_frame(timecode, data)

	def vcr_capture(self):
		if not self.vcr:
			try:
				print 'Connecting to VCR ...'
				self.vcr = VCR.VCR()
				print 'Waiting for VCR to come online ...'
				self.vcr.initvcr()
				print 'Preparing VCR ...'
				if not (self.vcr.fmmode('dnr') and \
					  self.vcr.dmcontrol('digital slow')):
					self.vcr_error('digital slow failed')
					return
				print 'VCR OK.'
			except VCR.error, msg:
				self.vcr = None
				self.vcr_error(msg)
				return
		if not self.vcr.still():
			self.vcr_error('still failed')
			return
		self.open_if_closed()
		rate = self.getint(self.in_rate_vcr, 1)
		rate = max(rate, 1)
		vcrspeed = self.c_vcrspeed.get_choice()
		vcrspeed = VcrSpeeds[vcrspeed]
		if vcrspeed == 0:
			stepfunc = self.vcr.step
		else:
			stepfunc = None
		self.speed_factor = rate
		addr = start_addr = self.vcr.sense()
		if not self.single_capture(None, 0):
			return
		print 'captured %02d:%02d:%02d:%02d' % self.vcr.addr2tc(addr)
		count = self.getint(self.in_nframes_vcr, 1) - 1
		if count <= 0:
			while rate > 0:
				if not self.vcr.step():
					self.vcr_error('step failed')
				here = self.vcr.sense()
				if here > addr:
					rate = rate - (here - addr)
				addr = here
			return
		if not self.vcr.fwdshuttle(vcrspeed):
			self.vcr_error('fwd shuttle failed')
			return
		cycle = 0
		while count > 0:
			try:
				here = self.vcr.sense()
			except VCR.error, msg:
				self.vcr_error(msg)
				break
			if here <> addr:
				if here <> addr+1:
					print 'Missed', here-addr-1,
					print 'frame' + 's'*(here-addr-1 <> 1)
				cycle = (cycle+1) % rate
				if cycle == 0:
					tc = (here-start_addr)*40
					if not self.single_capture(stepfunc, \
						  tc):
						break
					print 'captured %02d:%02d:%02d:%02d' \
						  % self.vcr.addr2tc(here)
					count = count -1
				addr = here
		if self.vcr and not self.vcr.still():
			self.vcr_error('still failed')

	def vcr_error(self, msg):
		self.reset()
		fl.show_message('VCR error:', str(msg), '')

	# Init/end continuous capture mode

	def init_cont(self):
		qsize = 1
		if self.vmode == VM_CONT:
			self.rate = self.getint(self.in_rate, 2)
		else:
			self.rate = 2
		x, y = self.vout.getsize()
		if self.use_24:
			info = (SV.YUV411_FRAMES, x, y, qsize, self.rate)
		else:
			info = (SV.RGB8_FRAMES, x, y, qsize, self.rate)
		info2 = self.video.InitContinuousCapture(info)
		if info2 <> info:
			# XXX This is really only debug info
			print 'Info mismatch: requested', info, 'got', info2

	def end_cont(self):
		self.video.EndContinuousCapture()

	# Misc stuff

	def settitle(self):
		gl.winset(self.window)
		x, y = gl.getsize()
		title = 'Vb ' + self.vfile + ' (%dx%d)' % (x, y)
		gl.wintitle(title)

	def get_vformat(self):
		i = self.c_vformat.get_choice()
		label = VideoFormatLabels[i-1]
		format = VideoFormats[i-1]
		if format == 'compress' and cl == None:
			fl.show_message('Sorry, no compression library support')
			format = ''
			label = 'Video off'
		self.vformat = format
		if self.vformat == '':
			self.form.freeze_form()
			self.g_video.hide_object()
			self.g_cont.hide_object()
			self.g_burst.hide_object()
			self.g_single.hide_object()
			self.form.unfreeze_form()
		else:
			self.g_video.show_object()
			if self.vmode == VM_CONT:
				self.g_cont.show_object()
			elif self.vmode == VM_BURST:
				self.g_burst.show_object()
			elif self.vmode == VM_SINGLE:
				self.g_single.show_object()
		#
		self.rgb = (format[:3] == 'rgb' or format == 'compress')
		self.mono = (format == 'mono')
		self.grey = (format[:4] == 'grey')
		self.use_24 = (format in ('rgb', 'jpeg', 'compress'))
		if self.use_24:
			self.g_rgb24.show_object()
		else:
			self.g_rgb24.hide_object()
		self.use_jpeg = (format == 'jpeg')
		self.mono_use_thresh = (label == 'mono thresh')
		self.use_compress = (format == 'compress')
		if self.use_compress:
			self.g_compress.show_object()
		else:
			self.g_compress.hide_object()
		s = format[4:]
		if self.grey and s:
			self.greybits = string.atoi(s)
		else:
			self.greybits = 8
		if label == 'grey2 dith':
			self.greybits = -2
		#
		convertor = None
		if self.grey:
			if self.greybits == 2:
				convertor = imageop.grey2grey2
			elif self.greybits == 4:
				convertor = imageop.grey2grey4
			elif self.greybits == -2:
				convertor = imageop.dither2grey2
		self.convertor = convertor
		self.optfullsizewindow()

	def get_aformat(self):
		self.reset()
		self.aformat = self.c_aformat.get_choice()
		if self.aformat == A_OFF:
			self.g_audio.hide_object()
		else:
			self.g_audio.show_object()

	def init_compressor(self, w, h):
		self.compressor = None
		scheme = cl.QuerySchemeFromName(cl.VIDEO, self.comp_scheme)
		self.compressor = cl.OpenCompressor(scheme)
		parambuf = [cl.IMAGE_WIDTH, w, \
			  cl.IMAGE_HEIGHT, h, \
			  cl.ORIGINAL_FORMAT, cl.YUV422DC]
		self.compressor.SetParams(parambuf)
		return self.compressor.Compress(0, '')

	def open_if_closed(self):
		if not self.vout:
			self.open_video()
		if not self.aout:
			self.open_audio()

	# File I/O handling

	def open_video(self):
		self.close_video()
		gl.winset(self.window)
		x, y = gl.getsize()
		if self.use_24:
			if self.rgb24_size == 2:
				x, y = x/2, y/2
			elif self.rgb24_size == 3:
				x, y = x/4, y/4
		vout = VFile.VoutFile(self.vfile)
		vout.setformat(self.vformat)
		if self.vformat == 'compress':
			cheader = self.init_compressor(x, y)
			vout.setcompressheader(cheader)
		vout.setsize(x, y)
		if self.vmode == VM_BURST:
			vout.setpf((1, -2))
		vout.writeheader()
		self.vout = vout
		self.nframes = 0
		self.speed_factor = 1
		self.t_nframes.label = `self.nframes`

	def write_frame(self, t, data):
		t = t * self.speed_factor
		if not self.vout:
			gl.ringbell()
			return 0
		if self.convertor:
			data = self.convertor(data, len(data), 1)
		elif self.mono:
			if self.mono_use_thresh:
				data = imageop.grey2mono(data, \
					  len(data), 1,\
					  self.mono_thresh)
			else:
				data = imageop.dither2mono(data, \
					  len(data), 1)
		try:
			self.vout.writeframe(int(t), data, None)
		except IOError, msg:
			self.reset()
			if msg == (0, 'Error 0'):
				msg = 'disk full??'
			fl.show_message('IOError', str(msg), '')
			return 0
		self.nframes = self.nframes + 1
		self.t_nframes.label = `self.nframes`
		return 1

	def close_video(self):
		if not self.vout:
			return
		self.nframes = 0
		self.t_nframes.label = ''
		try:
			self.vout.close()
		except IOError, msg:
			if msg == (0, 'Error 0'):
				msg = 'disk full??'
			fl.show_message('IOError', str(msg), '')
		self.vout = None
		self.compressor = None

	# Watch cursor handling

	def setwatch(self):
		gl.winset(self.form.window)
		gl.setcursor(WATCH, 0, 0)
		gl.winset(self.window)
		gl.setcursor(WATCH, 0, 0)

	def setarrow(self):
		gl.winset(self.form.window)
		gl.setcursor(ARROW, 0, 0)
		gl.winset(self.window)
		gl.setcursor(ARROW, 0, 0)

	# Numeric field handling

	def getint(self, field, default):
		try:
			value = string.atoi(field.get_input())
		except string.atoi_error:
			value = default
		field.set_input(`value`)
		return value

	def getfloat(self, field, default):
		try:
			value = float(eval(field.get_input()))
		except:
			value = float(default)
		field.set_input(`value`)
		return value

	# Audio stuff

	def open_audio(self):
		if self.aformat == A_OFF:
			return
		import aifc
		import al
		import AL
		import thread
		self.close_audio()
		params = [AL.INPUT_RATE, 0]
		al.getparams(AL.DEFAULT_DEVICE, params)
		rate = params[1]
		self.aout = aifc.open(self.afile, 'w')
		if self.aformat in (A_16_STEREO, A_8_STEREO):
			nch = AL.STEREO
		else:
			nch = AL.MONO
		if self.aformat in (A_16_STEREO, A_16_MONO):
			width = AL.SAMPLE_16
		else:
			width = AL.SAMPLE_8
		self.aout.setnchannels(nch)
		self.aout.setsampwidth(width)
		self.aout.setframerate(rate)
		c = al.newconfig()
		c.setqueuesize(8000)
		c.setchannels(nch)
		c.setwidth(width)
		self.aport = al.openport('Vb audio record', 'r', c)
		self.audio_stop = 0
		self.audio_ok = 0
		self.audio_busy = 1
		thread.start_new_thread(self.record_audio, ())

	def start_audio(self):
		if self.aformat == A_OFF:
			return
		self.audio_ok = 1

	def record_audio(self, *args):
		# This function runs in a separate thread
		# Currently no semaphores are used
		while not self.audio_stop:
			data = self.aport.readsamps(4000)
			if self.audio_ok:
				self.aout.writeframes(data)
			data = None
		self.audio_busy = 0

	def stop_audio(self):
		self.audio_ok = 0

	def close_audio(self):
		if self.aout:
			self.audio_ok = 0
			self.audio_stop = 1
			while self.audio_busy:
				time.sleep(0.1)
			self.aout.close()
			self.aout = None
		if self.aport:
			self.aport.closeport()
			self.aport = None


try:
	main()
except KeyboardInterrupt:
	print '[Interrupt]'
	sys.exit(1)
