# Define menu operations for GL stdwin

import gl
from glstdwin import key2code

class MenuObject:
	#
	def _init(self, win, title):
		self._win = win
		self._title = title
		self._items = []
		return self
	#
	def close(self):
		self._win.remove(self)
		del self._win
	#
	def additem(self, *args):
		if len(args) == 2:
			text, shortcut = args
		elif len(args) == 1:
			text, shortcut = args[0], None
		else:
			raise TypeError, 'arg count'
		self._items.append([text, shortcut, 1, 0])
	#
	def setitem(self, i, text):
		self._items[i][0] = text
	#
	def enable(self, i, flag):
		self._items[i][2] = flag
	#
	def check(self, i, flag):
		self._items[i][3] = flag
	#
	def _makepup(self, firstitem):
		pup = gl.newpup()
		if self._title:
			gl.addtopup(pup, self._title + '%t', 0)
		for item in self._items:
			text = item[0]
			if not item[2]: # Disabled
				text = ' ( ' + text + ' )%x-1'
			else:
				if item[3]: # Check mark
					text = '-> ' + text
				else:
					text = '    ' + text
				if key2code.has_key(item[1]):
					text = text + '  [Alt-' + item[1] + ']'
				text = text + '%x' + `firstitem`
			gl.addtopup(pup, text, 0)
			firstitem = firstitem + 1
		return pup
	#
	def _checkshortcut(self, char):
		for i in range(len(self._items)):
			item = self._items[i]
			if item[2] and item[1] == char:
				return i
		return -1
	#
