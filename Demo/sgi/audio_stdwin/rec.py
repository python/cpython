#! /ufs/guido/bin/sgi/python

import sys
import audio
import stdwin

import string
import getopt

from stdwinevents import *
from Buttons import *
from Sliders import *
#from Soundogram import Soundogram
from VUMeter import VUMeter
from WindowParent import WindowParent, MainLoop
from HVSplit import HSplit, VSplit

class TimeOutToggleButton(ToggleButton):
	def define(self, parent):
		self = ToggleButton.define(self, parent)
		self.parent.need_timer(self)
		self.timer_hook = 0
		return self
	def timer(self):
		if self.timer_hook:
			self.timer_hook(self)

K = 1024
BUFSIZE = 30*8*K
Rates = [0, 32*K, 16*K, 8*K]
Magics = ['', '0032', '0016', '0008']

class Struct: pass
G = Struct()

def main():
	#
	# Turn off scroll bars
	#
	stdwin.setdefscrollbars(0, 0)
	#
	# Set default state
	#
	G.gain = 60
	G.rate = 3
	G.nomuting = 0
	G.savefile = '@rec'
	#
	# Set default values
	#
	G.data = ''
	G.playing = 0
	G.recording = 0
	G.sogram = 0
	#
	# Parse options
	#
	optlist, args = getopt.getopt(sys.argv[1:], 'mdg:r:')
	#
	for optname, optarg in optlist:
		if 0: # (So all cases start with elif)
			pass
		elif optname == '-d':
			G.debug = 1
		elif optname == '-g':
			G.gain = string.atoi(optarg)
			if not (0 < G.gain < 256):
				raise optarg.error, '-g gain out of range'
		elif optname == '-m':
			G.nomuting = (not G.nomuting)
		elif optname == '-r':
			G.rate = string.atoi(optarg)
			if not (1 <= G.rate <= 3):
				raise optarg.error, '-r rate out of range'
	#
	if args:
		G.savefile = args[0]
	#
	# Initialize the sound package
	#
	audio.setoutgain(G.nomuting * G.gain)	# Silence the speaker
	audio.setrate(G.rate)
	#
	# Create the WindowParent and VSplit
	#
	G.window = WindowParent().create('Recorder', (0, 0))
	w = G.vsplit = VSplit().create(G.window)
	#
	# VU-meter
	#
	G.vubtn = VUMeter().define(w)
	#
	# Radiobuttons for rates
	#
	r1btn = RadioButton().definetext(w, '32 K/sec')
	r1btn.on_hook = rate_hook
	r1btn.rate = 1
	#
	r2btn = RadioButton().definetext(w, '16 K/sec')
	r2btn.on_hook = rate_hook
	r2btn.rate = 2
	#
	r3btn = RadioButton().definetext(w, '8 K/sec')
	r3btn.on_hook = rate_hook
	r3btn.rate = 3
	#
	radios = [r1btn, r2btn, r3btn]
	r1btn.group = r2btn.group = r3btn.group = radios
	for r in radios:
		if r.rate == G.rate: r.select(1)
	#
	# Other controls
	#
	G.recbtn = TimeOutToggleButton().definetext(w, 'Record')
	G.recbtn.on_hook = record_on_hook
	G.recbtn.timer_hook = record_timer_hook
	G.recbtn.off_hook = record_off_hook
	#
	G.mutebtn = CheckButton().definetext(w, 'Mute')
	G.mutebtn.select(not G.nomuting)
	G.mutebtn.hook = mute_hook
	#
	G.playbtn = TimeOutToggleButton().definetext(w, 'Playback')
	G.playbtn.on_hook = play_on_hook
	G.playbtn.timer_hook = play_timer_hook
	G.playbtn.off_hook = play_off_hook
	#
	G.gainbtn = ComplexSlider().define(w)
	G.gainbtn.settexts('  Volume: ', '  ')
	G.gainbtn.setminvalmax(0, G.gain, 255)
	G.gainbtn.sethook(gain_hook)
	#
	G.sizebtn = Label().definetext(w, `len(G.data)` + ' bytes')
	#
	#G.showbtn = PushButton().definetext(w, 'Sound-o-gram...')
	#G.showbtn.hook = show_hook
	#
	G.savebtn = PushButton().definetext(w, 'Save...')
	G.savebtn.hook = save_hook
	#
	G.quitbtn = PushButton().definetext(w, 'Quit')
	G.quitbtn.hook = quit_hook
	G.playbtn.enable(0)
	G.savebtn.enable(0)
	#G.showbtn.enable(0)
	start_vu()
	G.window.realize()
	#
	# Event loop
	#
	MainLoop()

# XXX Disabled...
def show_hook(self):
	savetext = self.text
	self.settext('Be patient...')
	close_sogram()
	stdwin.setdefwinsize(400, 300)
	win = stdwin.open('Sound-o-gram')
	G.sogram = Soundogram().define(win, G.data)
	win.buttons = [G.sogram]
	self.settext(savetext)

def close_sogram():
	if G.sogram:
		# Break circular references
		G.sogram.win.buttons[:] = []
		del G.sogram.win
		G.sogram = 0

def mute_hook(self):
	G.nomuting = (not self.selected)
	audio.setoutgain(G.nomuting * G.gain)

def rate_hook(self):
	G.rate = self.rate
	audio.setrate(G.rate)

def record_on_hook(self):
	stop_vu()
	close_sogram()
	audio.setrate(G.rate)
	audio.setoutgain(G.nomuting * G.gain)
	audio.start_recording(BUFSIZE)
	G.recording = 1
	G.playbtn.enable(0)
	G.window.settimer(10 * BUFSIZE / Rates[G.rate])

def record_timer_hook(self):
	if G.recording:
		if audio.poll_recording():
			self.hilite(0)
			record_off_hook(self)
		else:
			self.parent.settimer(5)

def record_off_hook(self):
	if not G.recording:
		return
	G.data = audio.stop_recording()
	G.recording = 0
	G.sizebtn.settext(`len(G.data)` + ' bytes')
	audio.setoutgain(G.nomuting * G.gain)
	G.playbtn.enable((len(G.data) > 0))
	G.savebtn.enable((len(G.data) > 0))
	#G.showbtn.enable((len(G.data) > 0))
	G.window.settimer(0)
	start_vu()

def play_on_hook(self):
	stop_vu()
	audio.setrate(G.rate)
	audio.setoutgain(G.gain)
	audio.start_playing(G.data)
	G.playing = 1
	G.recbtn.enable(0)
	G.window.settimer(max(10 * len(G.data) / Rates[G.rate], 1))

def play_timer_hook(self):
	if G.playing:
		if audio.poll_playing():
			self.hilite(0)
			play_off_hook(self)
		else:
			self.parent.settimer(5)

def play_off_hook(self):
	if not G.playing:
		return
	x = audio.stop_playing()
	G.playing = 0
	audio.setoutgain(G.nomuting * G.gain)
	G.recbtn.enable(1)
	G.window.settimer(0)
	start_vu()

def gain_hook(self):
	G.gain = self.val
	if G.playing or G.nomuting: audio.setoutgain(G.gain)

def save_hook(self):
	if not G.data:
		stdwin.fleep()
	else:
		prompt = 'Store sampled data on file: '
		try:
			G.savefile = stdwin.askfile(prompt, G.savefile, 1)
		except KeyboardInterrupt:
			return
		try:
			fp = open(G.savefile, 'w')
			fp.write(Magics[G.rate] + G.data)
		except:
			stdwin.message('Cannot create ' + file)

def stop_vu():
	G.vubtn.stop()

def start_vu():
	G.vubtn.start()

def quit_hook(self):
	G.window.delayed_destroy()

try:
	main()
finally:
	audio.setoutgain(0)
