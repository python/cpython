# DirList -- Directory Listing widget

# XXX Displays messy paths when following '..'

import os
import stdwin, rect
from stdwinevents import *
from Buttons import PushButton
from WindowParent import WindowParent
from HVSplit import HSplit, VSplit

class DirList(VSplit):
	#
	def create(self, (parent, dirname)):
		self = VSplit.create(self, parent)
		names = os.listdir(dirname)
		for name in names:
			if os.path.isdir(os.path.join(dirname, name)):
				fullname = os.path.join(dirname, name)
				btn = SubdirButton().definetext(self, fullname)
			elif name[-3:] == '.py':
				btn = ModuleButton().definetext(self, name)
			else:
				btn = FileButton().definetext(self, name)
		return self
	#

class DirListWindow(WindowParent):
	#
	def create(self, dirname):
		self = WindowParent.create(self, (dirname, (0, 0)))
		child = DirList().create(self, dirname)
		self.realize()
		return self
	#

class SubdirButton(PushButton):
	#
	def drawpict(self, d):
		PushButton.drawpict(self, d)
		d.box(rect.inset(self.bounds, (3, 1)))
	#
	def up_trigger(self):
		window = DirListWindow().create(self.text)
	#

class FileButton(PushButton):
	#
	def up_trigger(self):
		stdwin.fleep()
	#

class ModuleButton(FileButton):
	#
	def drawpict(self, d):
		PushButton.drawpict(self, d)
		d.box(rect.inset(self.bounds, (1, 3)))
	#
