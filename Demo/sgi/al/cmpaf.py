# Compare different audio compression schemes.
#
# This copies mono audio data from the input port to the output port,
# and puts up a window with 4 toggle buttons:
#
# uLAW       : convert the data to uLAW and back
# ADPCM      : convert the data to ADPCM and back
# Difference : make only the difference between the converted and the
#              original data audible
# Exit       : quit from the program

import fl
import FL
import flp
import al
import AL
import audioop
import sys

class Cmpaf:
	def __init__(self):
		parsetree = flp.parse_form('cmpaf_form','form')
		flp.create_full_form(self, parsetree)
		c = al.newconfig()
		c.setchannels(AL.MONO)
		c.setqueuesize(1800)
		self.iport = al.openport('cmpaf','r', c)
		self.oport = al.openport('cmpaf','w', c)
		self.do_adpcm = self.do_ulaw = self.do_diff = 0
		self.acstate = None
		self.form.show_form(FL.PLACE_SIZE, 1, 'compare audio formats')

	def run(self):
		while 1:
			olddata = data = self.iport.readsamps(600)
			if self.do_ulaw:
				data = audioop.lin2ulaw(data, 2)
				data = audioop.ulaw2lin(data, 2)
			if self.do_adpcm:
				data, nacstate = audioop.lin2adpcm(data, 2, \
					  self.acstate)
				data, dummy = audioop.adpcm2lin(data, 2, \
					  self.acstate)
				self.acstate = nacstate
			if self.do_diff:
				olddata = audioop.mul(olddata, 2, -1)
				data = audioop.add(olddata, data, 2)
			self.oport.writesamps(data)
			fl.check_forms()

	def cb_exit(self, *args):
		sys.exit(0)

	def cb_adpcm(self, obj, val):
		self.do_adpcm = obj.get_button()

	def cb_ulaw(self, obj, val):
		self.do_ulaw = obj.get_button()

	def cb_diff(self, obj, val):
		self.do_diff = obj.get_button()

cmpaf = Cmpaf()
cmpaf.run()
