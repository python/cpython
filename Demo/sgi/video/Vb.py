#! /ufs/guido/bin/sgi/python

# Video bag-of-tricks

import sys
import getopt
import string
import os
sts = os.system('makemap')		# Must be before "import fl"
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
	vb = VideoBagOfTricks().init()
	while 1:
		dummy = fl.do_forms()
		[dummy]

StopCapture = 'StopCapture'

formats = ['rgb8', 'grey8', 'grey4', 'grey2', \
	   'grey2_dith', 'mono_dith', 'mono_thresh']
formatmap = {'rgb24': 'rgb', 'grey8': 'grey'}

class VideoBagOfTricks:

	def init(self):
		formdef = flp.parse_form('VbForm', 'form')
		flp.create_full_form(self, formdef)
		self.g_stop.hide_object()
		self.setdefaults()
		self.openvideo()
		self.makewindow()
		self.bindvideo()
		self.capturing = 0
		self.form.show_form(FL.PLACE_SIZE, FL.TRUE, \
				    'Video Bag Of Tricks')
		fl.set_event_call_back(self.do_event)
		return self

	def setwatch(self):
		gl.winset(self.form.window)
		gl.setcursor(WATCH, 0, 0)

	def setarrow(self):
		gl.winset(self.form.window)
		gl.setcursor(ARROW, 0, 0)

	def setdefaults(self):
		self.format = 'rgb8'
		self.c_format.clear_choice()
		for label in formats:
			self.c_format.addto_choice(label)
		self.c_format.set_choice(1 + formats.index(self.format))
		self.mono_thresh = 128
		self.mono_use_thresh = 0
		self.b_drop.set_button(1)
		self.b_burst.set_button(0)
		self.in_rate.set_input('2')
		self.in_maxmem.set_input('1.0')
		self.in_nframes.set_input('0')
		self.in_file.set_input('film.video')

	def openvideo(self):
		self.video = sv.OpenVideo()
		param = [SV.BROADCAST, 0]
		self.video.GetParam(param)
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
		gl.wintitle(self.maketitle())

	def bindvideo(self):
		x, y = gl.getsize()
		self.video.SetSize(x, y)
		drop = self.b_drop.get_button()
		if drop:
			param = [SV.FIELDDROP, 1, SV.GENLOCK, SV.GENLOCK_OFF]
		else:
			param = [SV.FIELDDROP, 0, SV.GENLOCK, SV.GENLOCK_ON]
		if self.getformat()[:3] == 'rgb':
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
		i = self.c_format.get_choice()
		label = format = formats[i-1]
		if '_' in format:
			i = string.find(format, '_')
			format = format[:i]
		if formatmap.has_key(format):
			format = formatmap[format]
		self.format = format
		#
		if label == 'mono_thresh':
			self.mono_use_thresh = 1
			s = `self.mono_thresh`
			s = fl.show_input('Please enter mono threshold', s)
			if s:
				try:
					self.mono_thresh = string.atoi(s)
				except string.atoi_error:
					fl.show_message( \
						  'Bad input, using ' + \
						  `self.mono_thresh`)
		else:
			self.mono_use_thresh = 0
		#
		self.rebindvideo()

	def getformat(self):
		return self.format

	def cb_rate(self, *args):
		pass

	def cb_drop(self, *args):
		self.rebindvideo()

	def cb_burst(self, *args):
		pass

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

	def cb_play(self, *args):
		filename = self.in_file.get_input()
		sts = os.system('Vplay -q ' + filename + ' &')

	def cb_stop(self, *args):
		if self.capturing:
			raise StopCapture
		gl.ringbell()

	def cb_capture(self, *args):
		self.setwatch()
		self.g_main.hide_object()
		self.cb_file() # Make sure filename is OK
		filename = self.in_file.get_input()
		format = self.getformat()
		vout = VFile.VoutFile().init(filename)
		vout.setformat(format)
		gl.winset(self.window)
		x, y = gl.getsize()
		vout.setsize(x, y)
		vout.writeheader()
		convertor = None
		if format[:4] == 'grey':
			s = format[4:]
			if s:
				greybits = string.atoi(s)
			else:
				greybits = 8
			# XXX Should get this from somewhere else?
			if greybits == 2:
				convertor = imageop.grey2grey2
			elif greybits == 4:
				convertor = imageop.grey2grey4
			elif greybits == -2:
				convertor = imageop.dither2grey2
		mono = (format == 'mono')
		vformat = SV.RGB8_FRAMES
		qsize = 0
		rate = eval(self.in_rate.get_input())
		info = (vformat, x, y, qsize, rate)
		ids = []
		tpf = 50
		self.video.InitContinuousCapture(info)
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
			if convertor:
				data = convertor(data, len(data), 1)
			elif mono:
				if self.mono_use_thresh:
					data = imageop.grey2mono(data, \
						  len(data), 1,\
						  self.mono_thresh)
				else:
					data = imageop.dither2mono(data, \
						  len(data), 1)
			try:
				vout.writeframe(t, data, None)
			except IOError, msg:
				if msg == (0, 'Error 0'):
					msg = 'disk full??'
				fl.show_message('IOError: ' + str(msg))
				break
		self.setwatch()
		self.g_stop.hide_object()
		self.capturing = 0
		vout.close()
		self.video.EndContinuousCapture()
		self.g_main.show_object()
		self.setarrow()

	def cb_quit(self, *args):
		raise SystemExit, 0

	def maketitle(self):
		x, y = gl.getsize()
		return 'Vb:' + self.in_file.get_input() + ' (%dx%d)' % (x, y)

try:
	main()
except KeyboardInterrupt:
	print '[Interrupt]'
	sys.exit(1)
