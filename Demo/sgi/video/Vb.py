#! /ufs/guido/bin/sgi/python

# Video bag-of-tricks

# XXX To do:
# - audio
# - single frame recording
# - improve user interface
# - help button?
# - command line options to set initial settings
# - save settings in a file
# - ...?

import sys
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

ARROW = 0
WATCH = 1
watchcursor.defwatch(WATCH)

def main():
##	fl.set_graphics_mode(0, 1)
	vb = VideoBagOfTricks().init()
	while 1:
		dummy = fl.do_forms()
		[dummy]

StopCapture = 'StopCapture'

Labels = ['rgb8', 'grey8', 'grey4', 'grey2', \
	  'grey2 dith', 'mono dith', 'mono thresh']
Formats = ['rgb8', 'grey', 'grey4', 'grey2', \
	   'grey2', 'mono', 'mono']

class VideoBagOfTricks:

	def init(self):
		formdef = flp.parse_form('VbForm', 'form')
		flp.create_full_form(self, formdef)
		self.g_stop.hide_object()
		self.g_burst.hide_object()
		self.setdefaults()
		self.openvideo()
		self.makewindow()
		self.bindvideo()
		self.capturing = 0
		self.showform()
		fl.set_event_call_back(self.do_event)
		return self

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
		self.form.show_form(FL.PLACE_FREE, FL.TRUE, \
				    'Video Bag Of Tricks')

	def setdefaults(self):
		self.mono_thresh = 128
		self.format = 'rgb8'
		self.c_format.clear_choice()
		for label in Labels:
			self.c_format.addto_choice(label)
		self.get_format()
		self.b_drop.set_button(1)
		self.b_burst.set_button(0)
		self.in_rate.set_input('2')
		self.in_maxmem.set_input('1.0')
		self.in_nframes.set_input('0')
		self.in_file.set_input('film.video')

	def openvideo(self):
		try:
			self.video = sv.OpenVideo()
		except sv.error, msg:
			print 'Error opening video:', msg
			self.video = None
			#sys.exit(1)
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

	def makewindow(self):
		x, y = self.maxx, self.maxy
		gl.foreground()
		gl.maxsize(x, y)
		gl.keepaspect(x, y)
		gl.stepunit(8, 6)
		width = 256 # XXX
		if width:
			height = width*3/4
			x1 = 150
			x2 = x1 + width-1
			y2 = 768-150
			y1 = y2-height+1
			gl.prefposition(x1, x2, y1, y2)
		self.window = gl.winopen('Vb: initializing')
		self.settitle()
		if width:
			gl.maxsize(x, y)
			gl.keepaspect(x, y)
			gl.stepunit(8, 6)
			gl.winconstraints()
		gl.qdevice(DEVICE.LEFTMOUSE)
		gl.qdevice(DEVICE.WINQUIT)
		gl.qdevice(DEVICE.WINSHUT)

	def settitle(self):
		gl.winset(self.window)
		x, y = gl.getsize()
		title = 'Vb:' + self.in_file.get_input() + ' (%dx%d)' % (x, y)
		gl.wintitle(title)

	def bindvideo(self):
		if not self.video: return
		x, y = gl.getsize()
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

	def do_event(self, dev, val):
		#print 'Event:', dev, val
		if dev in (DEVICE.WINSHUT, DEVICE.WINQUIT):
			self.cb_quit()
		if dev == DEVICE.REDRAW and val == self.window:
			self.rebindvideo()
			self.settitle()

	def cb_format(self, *args):
		self.get_format()
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

	def cb_rate(self, *args):
		pass

	def cb_drop(self, *args):
		self.rebindvideo()

	def cb_burst(self, *args):
		if self.b_burst.get_button():
			self.in_rate.set_input('1')
			self.b_drop.set_button(1)
##			self.g_stop.hide_object()
			self.g_burst.show_object()
		else:
			self.in_rate.set_input('2')
			self.b_drop.set_button(0)
##			self.g_stop.show_object()
			self.g_burst.hide_object()

	def cb_maxmem(self, *args):
		pass

	def cb_nframes(self, *args):
		pass

	def cb_file(self, *args):
		filename = self.in_file.get_input()
		if filename == '':
			filename = 'film.video'
			self.in_file.set_input(filename)
		self.settitle()

	def cb_open(self, *args):
		filename = self.in_file.get_input()
		hd, tl = os.path.split(filename)
		filename = fl.file_selector('Select file:', hd, '', tl)
		if filename:
			hd, tl = os.path.split(filename)
			if hd == os.getcwd():
				filename = tl
			self.in_file.set_input(filename)
			self.cb_file()

	def cb_capture(self, *args):
		if not self.video:
			gl.ringbell()
			return
		if self.b_burst.get_button():
			self.burst_capture()
		else:
			self.cont_capture()

	def cb_stop(self, *args):
		if self.capturing:
			raise StopCapture
		gl.ringbell()

	def cb_play(self, *args):
		filename = self.in_file.get_input()
		sts = os.system('Vplay -q ' + filename + ' &')

	def cb_quit(self, *args):
		raise SystemExit, 0

	def burst_capture(self):
		self.setwatch()
		gl.winset(self.window)
		x, y = gl.getsize()
		vformat = SV.RGB8_FRAMES
		try:
			nframes = string.atoi(self.in_nframes.get_input())
		except string.atoi_error:
			nframes = 0
		if nframes == 0:
			try:
				maxmem = \
				  float(eval(self.in_maxmem.get_input()))
			except:
				maxmem = 1.0
			memsize = int(maxmem * 1024 * 1024)
			nframes = calcnframes(x, y, \
				  self.mono or self.grey, memsize)
			print 'nframes =', nframes
		rate = string.atoi(self.in_rate.get_input())
		info = (vformat, x, y, nframes, rate)
		try:
			info2, data, bitvec = self.video.CaptureBurst(info)
		except sv.error, msg:
			fl.show_message('Capture error:', str(msg), '')
			self.setarrow()
			return
		if info <> info2: print info, '<>', info2
		self.save_burst(info2, data, bitvec)
		self.setarrow()

	def save_burst(self, info, data, bitvec):
		(vformat, x, y, nframes, rate) = info
		self.open_file()
		fieldsize = x*y/2
		nskipped = 0
		realframeno = 0
		tpf = 1000 / 50.0     #XXXX
		# Trying to find the pattern in frame skipping
		okstretch = 0
		skipstretch = 0
		for frameno in range(0, nframes*2):
			if frameno <> 0 and \
				  bitvec[frameno] == bitvec[frameno-1]:
				nskipped = nskipped + 1
				if okstretch:
					#print okstretch, 'ok',
					okstretch = 0
				skipstretch = skipstretch + 1
				continue
			if skipstretch:
				#print skipstretch, 'skipped'
				skipstretch = 0
			okstretch = okstretch + 1
			#
			# Save field.
			# XXXX Works only for fields and top-to-bottom
			#
			start = frameno*fieldsize
			field = data[start:start+fieldsize]
			realframeno = realframeno + 1
			fn = int(realframeno*tpf)
			if not self.write_frame(fn, field):
				break
		#print okstretch, 'ok',
		#print skipstretch, 'skipped'
		#print 'Skipped', nskipped, 'duplicate frames'
		self.close_file()

	def cont_capture(self):
		self.setwatch()
		self.g_main.hide_object()
		self.open_file()
		vformat = SV.RGB8_FRAMES
		qsize = 1 # XXX Should be an option?
		try:
			rate = string.atoi(self.in_rate.get_input())
		except string.atoi_error:
			rate = 2
		x, y = self.vout.getsize()
		info = (vformat, x, y, qsize, rate)
		ids = []
		fps = 59.64 # Fields per second
		# XXX (fps of Indigo monitor, not of PAL or NTSC!)
		tpf = 1000.0 / fps # Time per field in msec
		info2 = self.video.InitContinuousCapture(info)
		if info2 <> info:
			print 'Info mismatch: requested', info, 'got', info2
		self.capturing = 1
		self.g_stop.show_object()
		self.setarrow()
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
			ids.append(id)
			id = id + 2*rate
			data = cd.InterleaveFields(1)
			cd.UnlockCaptureData()
			t = id*tpf
			if not self.write_frame(t, data):
				break
		self.setwatch()
		self.g_stop.hide_object()
		self.capturing = 0
		self.video.EndContinuousCapture()
		self.close_file()
		self.g_main.show_object()
		self.setarrow()

	def get_format(self):
		i = self.c_format.get_choice()
		label = Labels[i-1]
		format = Formats[i-1]
		self.format = format
		#
		self.rgb = (format[:3] == 'rgb')
		self.mono = (format == 'mono')
		self.grey = (format[:4] == 'grey')
		self.mono_use_thresh = (label == 'mono thresh')
		s = format[4:]
		if s:
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

	def open_file(self):
		gl.winset(self.window)
		x, y = gl.getsize()
		self.cb_file() # Make sure filename is OK
		filename = self.in_file.get_input()
		vout = VFile.VoutFile().init(filename)
		vout.setformat(self.format)
		vout.setsize(x, y)
		if self.b_burst.get_button():
			vout.setpf((1, -2))
		vout.writeheader()
		self.vout = vout

	def write_frame(self, t, data):
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
			self.vout.writeframe(t, data, None)
		except IOError, msg:
			if msg == (0, 'Error 0'):
				msg = 'disk full??'
			fl.show_message('IOError', str(msg), '')
			return 0
		return 1

	def close_file(self):
		try:
			self.vout.close()
		except IOError, msg:
			if msg == (0, 'Error 0'):
				msg = 'disk full??'
			fl.show_message('IOError', str(msg), '')
		del self.vout

	def setwatch(self):
		gl.winset(self.form.window)
		gl.setcursor(WATCH, 0, 0)

	def setarrow(self):
		gl.winset(self.form.window)
		gl.setcursor(ARROW, 0, 0)

def calcnframes(x, y, grey, memsize):
	pixels = x*y
	pixels = pixels/2		# XXX always assume fields
	if grey: n = memsize/pixels
	else: n = memsize/(4*pixels)
	return max(1, n)

try:
	main()
except KeyboardInterrupt:
	print '[Interrupt]'
	sys.exit(1)
