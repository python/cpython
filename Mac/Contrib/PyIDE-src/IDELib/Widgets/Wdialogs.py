import W
from types import *
import string

"""
import Wdialogs
testDict1 = {1:1, 2:2, 3:3}
testDict2 = {3:3,4:4, 'testDict1':testDict1, 6:6, 7:7}
testDict3 = {3:3,4:4, 'testDict2':testDict2, 'testDict1':testDict1, 6:6, 7:7}
Wdialogs.EditDictionary(testDict3)

import Wdialogs
a = Wdialogs.Ask('xxx', 'default text', ['font', 'typografie', 'lettertonwerpen', 'huisstijl'])
"""

def Message(text, button = "OK"):
	w = W.ModalDialog((300, 100))
	w.button = W.Button((-90, -30, 80, 16), button, w.close)
	w.message = W.TextBox((10, 10, -10, -40), text)
	w.setdefaultbutton(w.button)
	w.open()


def Ask(question, defaulttext = "", selections = []):
	d = _Ask(question, defaulttext, selections)
	return d.rv


class _Ask:
	#	selections is a list of possible for selections
	
	def __init__(self, question, defaulttext, selections):
		self.selections = []
		for s in selections:
			self.selections.append(string.lower(s))
		self.selections.sort()
		self.w = W.ModalDialog((300, 120))
		self.w.button1 = W.Button((-90, -30, 80, 16), "OK", self.button1hit)
		self.w.button2 = W.Button((-180, -30, 80, 16), "Cancel", self.button2hit)
		self.w.question = W.TextBox((10, 10, -10, 30), question)
		self.w.input = W.EditText((10, 40, -10, 20), defaulttext, self.processInput)
		self.rv = None
		self.w.setdefaultbutton(self.w.button1)
		
		self.w.bind("cmd.", self.w.button2.push)
		self.w.open()
	
	def processInput(self, key, modifiers):	# Process user input to match a selection
		pos = self.w.input.getselection()
		input = string.lower(self.w.input.get()[0:pos[1]])
		if len(input):
			for t in self.selections:
				if input == t[0:pos[0]]:
					self.w.input.set(t)
					self.w.input.setselection(pos[0], pos[1])
					return
		self.w.input.set(input)
		self.w.input.setselection(pos[1], pos[1])
		
	def button1hit(self):
		self.rv = self.w.input.get()
		self.w.close()
		
	def button2hit(self):
		self.w.close()

class _AskYesNo:
	
	def __init__(self, question, cancelFlag= 0):
		if cancelFlag:
			size = 190, 80
		else:	size = 150, 80
		self.w = W.ModalDialog(size)
		self.w.yes = W.Button((10, -36, 50, 24), 'Yes', self.yes)
		if cancelFlag:
			self.w.cancel = W.Button((70, -36, -70, 24), "Cancel", self.cancel)	
		self.w.no = W.Button((-60, -36, -10, 24), 'No', self.no)
		self.w.question = W.TextBox((10, 10, -10, 30), question)
		self.rv = None
		self.w.setdefaultbutton(self.w.yes)
		if cancelFlag:
			self.w.bind("cmd.", self.w.cancel)
		else:	self.w.bind("cmd.", self.w.no)
		self.w.open()
	
	def yes(self):
		self.rv = 1
		self.w.close()
		
	def no(self):
		self.rv = 0
		self.w.close()

	def cancel(self):
		self.rv = -1
		self.w.close()

def AskYesNo(question):
	d = _AskYesNo(question, 0)
	return d.rv
	
def AskYesCancelNo(question):
	d = _AskYesNo(question, 1)
	return d.rv
	
class CallBackButton(W.Button):
	def click(self, point, modifiers):
		if not self._enabled:
			return
		part = self._control.TrackControl(point)
		if part:
			if self._callback:
				self._callback(self.dict)
	
	def push(self):
		if not self._enabled:
			return
		import time
		self._control.HiliteControl(1)
		time.sleep(0.1)
		self._control.HiliteControl(0)
		if self._callback:
			self._callback(self.dict)
		
class EditDictionary:			# Auto layout editor of dictionary
	def __init__(self, dictionary, title = 'Dictionary Editor'):
		self.leading = 20
		self.d = dictionary
		keys = self.d.keys()
		windowSize = 400, len(keys) * self.leading + 100
		self.w = w = W.ModalDialog(windowSize)
		y = 2 * self.leading
		theFont = fontsettings = ('Geneva', 0, 10, (0,0,0))
		keys.sort()
		for key in keys:
			if type(key) == StringType:
				label = key
			else:	label = `key`
			if type(self.d[key]) == StringType:
				value = self.d[key]
			else:
				value = `self.d[key]`		# Just show the value
			
			if type(self.d[key]) == DictType:	# Make a button
				button = w[label] = CallBackButton((110, y, 50, 18), label, self.pushDict)
				button.dict = self.d[key]
			else:
				w['k_' + label] = W.TextBox((10, y, 200, 18), label, fontsettings = theFont)
				w[label] = W.EditText((110, y, -10, 18), value, fontsettings = theFont)
			y = y + self.leading
		
		w._name = W.TextBox((10, 4, 100, 10), title)
		w._ok = W.Button((-160, -36, 60, 24), "OK", self.ok)
		w._cancel = W.Button((-80, -36, 60, 24), "Cancel", self.cancel)
		w.setdefaultbutton(self.w._ok)

		self.rv = None	# Return value
		w.open()
	
	def pushDict(self, dict):
		EditDictionary(dict)
		
	def popDict(self):
		self.w.close()
		
	def ok(self):
		self.rv = 1
		for key in self.d.keys():
			if type(key) == StringType:
				label = key
			else:	label = `key`
			if type(self.d[key]) == StringType or self.d[key] == None:
				self.d[key] = self.w[label].get()
			else:	
				try:
					self.d[key] = eval(self.w[label].get())
				except:
					pass
		self.popDict()
		
	def cancel(self):
		self.rv = 0
		self.popDict()
