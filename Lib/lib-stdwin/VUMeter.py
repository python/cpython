# Module 'VUMeter'

import audio
from StripChart import StripChart

K = 1024
Rates = [0, 32*K, 16*K, 8*K]

class VUMeter() = StripChart():
	#
	# Override define() and timer() methods
	#
	def define(self, parent):
		self = StripChart.define(self, (parent, 128))
		self.parent.need_timer(self)
		self.sampling = 0
		self.rate = 3
		self.enable(0)
		return self
	#
	def timer(self):
		if self.sampling:
			chunk = audio.wait_recording()
			self.sampling = 0
			nums = audio.chr2num(chunk)
			ampl = max(abs(min(nums)), abs(max(nums)))
			self.append(ampl)
		if self.enabled and not self.sampling:
			audio.setrate(self.rate)
			size = Rates[self.rate]/10
			size = size/48*48
			audio.start_recording(size)
			self.sampling = 1
		if self.sampling:
			self.parent.settimer(1)
	#
	# New methods: start() and stop()
	#
	def stop(self):
		if self.sampling:
			chunk = audio.stop_recording()
			self.sampling = 0
		self.enable(0)
	#
	def start(self):
		self.enable(1)
		self.timer()
