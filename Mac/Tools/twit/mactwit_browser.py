"""A simple Mac-only browse utility to peek at the inner data structures of Python."""
# Minor modifications by Jack to facilitate incorporation in twit.

# june 1996
# Written by Just van Rossum <just@knoware.nl>, please send comments/improvements.
# Loosely based on Jack Jansens's PICTbrowse.py, but depends on his fabulous FrameWork.py
# XXX Some parts are *very* poorly solved. Will fix. Guido has to check if all the
# XXX "python-peeking" is done correctly. I kindof reverse-engineered it ;-)

# disclaimer: although I happen to be the brother of Python's father, programming is
# not what I've been trained to do. So don't be surprised if you find anything that's not 
# as nice as it could be...

# XXX to do:
# Arrow key support
# Copy & Paste? 
# MAIN_TEXT item should not contain (type); should be below or something. 
# MAIN_TEXT item should check if a string is binary or not: convert to '/000' style
# or convert newlines. 

version = "1.0"

import FrameWork
import EasyDialogs
import Dlg
import Res
import Qd
import List
import sys
from Types import *
from QuickDraw import *
import string
import time
import os

# The initial object to start browsing with. Can be anything, but 'sys' makes kindof sense.
start_object = sys

# Resource definitions
ID_MAIN = 503
NUM_LISTS = 4	# the number of lists used. could be changed, but the dlg item numbers should be consistent
MAIN_TITLE = 3	# this is only the first text item, the other three ID's should be 5, 7 and 9
MAIN_LIST = 4	# this is only the first list, the other three ID's should be 6, 8 and 10
MAIN_TEXT = 11
MAIN_LEFT = 1
MAIN_RIGHT = 2
MAIN_RESET = 12
MAIN_CLOSE = 13
MAIN_LINE = 14

def Initialize():
	# this bit ensures that this module will also work as an applet if the resources are
	# in the resource fork of the applet
	# stolen from Jack, so it should work(?!;-)
	try:
		# if this doesn't raise an error, we are an applet containing the necessary resources
		# so we don't have to bother opening the resource file
		dummy = Res.GetResource('DLOG', ID_MAIN)
	except Res.Error:
		savewd = os.getcwd()
		ourparentdir = os.path.split(openresfile.func_code.co_filename)[0]
		os.chdir(ourparentdir)		
		try:
			Res.FSpOpenResFile("mactwit_browse.rsrc", 1)
		except Res.Error, arg:
			EasyDialogs.Message("Cannot open mactwit_browse.rsrc: "+arg[1])
			sys.exit(1)
		os.chdir(savewd)

def main():
	Initialize()
	PythonBrowse()

# this is all there is to it to make an application. 
class PythonBrowse(FrameWork.Application):
	def __init__(self):
		FrameWork.Application.__init__(self)
		VarBrowser(self).open(start_object)
		self.mainloop()
	
	def do_about(self, id, item, window, event):
		EasyDialogs.Message(self.__class__.__name__ + " version " + version + "\rby Just van Rossum")
	
	def quit(self, *args):
		raise self

class MyList:
	def __init__(self, wid, rect, itemnum):
		# wid is the window (dialog) where our list is going to be in
		# rect is it's item rectangle (as in dialog item)
		# itemnum is the itemnumber in the dialog
		self.rect = rect
		rect2 = rect[0]+1, rect[1]+1, rect[2]-16, rect[3]-1		# Scroll bar space, that's 15 + 1, Jack!
		self.list = List.LNew(rect2, (0, 0, 1, 0), (0,0), 0, wid,
					0, 1, 0, 1)
		self.wid = wid
		self.active = 0
		self.itemnum = itemnum
	
	def setcontent(self, content, title = ""):
		# first, gather some stuff
		keylist = []
		valuelist = []
		thetype = type(content)
		if thetype == DictType:
			keylist = content.keys()
			keylist.sort()
			for key in keylist:
				valuelist.append(content[key])
		elif thetype == ListType:
			keylist = valuelist = content
		elif thetype == TupleType:
			
			keylist = valuelist = []
			for i in content:
				keylist.append(i)
		else:
			# XXX help me! is all this correct? is there more I should consider???
			# XXX is this a sensible way to do it in the first place????
			# XXX I'm not familiar enough with Python's guts to be sure. GUIDOOOOO!!!
			if hasattr(content, "__dict__"):
				keylist = keylist + content.__dict__.keys()
			if hasattr(content, "__methods__"):
				keylist = keylist + content.__methods__
			if hasattr(content, "__members__"):
				keylist = keylist + content.__members__
			if hasattr(content, "__class__"):
				keylist.append("__class__")
			if hasattr(content, "__bases__"):
				keylist.append("__bases__")
			if hasattr(content, "__name__"):
				title = content.__name__
				if "__name__" not in keylist:
					keylist.append("__name__")
			keylist.sort()
			for key in keylist:
				valuelist.append(getattr(content, key))
		if content <> None:
			title = title + "\r" + cleantype(content)
		# now make that list!
		tp, h, rect = self.wid.GetDialogItem(self.itemnum - 1)
		Dlg.SetDialogItemText(h, title[:255])
		self.list.LDelRow(0, 1)
		self.list.LSetDrawingMode(0)
		self.list.LAddRow(len(keylist), 0)
		for i in range(len(keylist)):
			self.list.LSetCell(str(keylist[i]), (0, i))
		self.list.LSetDrawingMode(1)
		self.list.LUpdate(self.wid.GetWindowPort().visRgn)
		self.content = content
		self.keylist = keylist
		self.valuelist = valuelist
		self.title = title
	
	# draw a frame around the list, List Manager doesn't do that
	def drawframe(self):
		Qd.SetPort(self.wid)
		Qd.FrameRect(self.rect)
		rect2 = Qd.InsetRect(self.rect, -3, -3)
		save = Qd.GetPenState()
		Qd.PenSize(2, 2)
		if self.active:
			Qd.PenPat(Qd.qd.black)
		else:
			Qd.PenPat(Qd.qd.white)
		# draw (or erase) an extra frame to indicate this is the acive list (or not)
		Qd.FrameRect(rect2)
		Qd.SetPenState(save)
		
		

class VarBrowser(FrameWork.DialogWindow):
	def open(self, start_object, title = ""):
		FrameWork.DialogWindow.open(self, ID_MAIN)
		if title <> "":
			windowtitle = self.wid.GetWTitle()
			self.wid.SetWTitle(windowtitle + " >> " + title)
		else:
			if hasattr(start_object, "__name__"):
				windowtitle = self.wid.GetWTitle()
				self.wid.SetWTitle(windowtitle + " >> " + str(getattr(start_object, "__name__")) )
				
		self.SetPort()
		Qd.TextFont(3)
		Qd.TextSize(9)
		self.lists = []
		self.listitems = []
		for i in range(NUM_LISTS):
			self.listitems.append(MAIN_LIST + 2 * i)	# dlg item numbers... have to be consistent
		for i in self.listitems:
			tp, h, rect = self.wid.GetDialogItem(i)
			list = MyList(self.wid, rect, i)
			self.lists.append(list)
		self.leftover = []
		self.rightover = []
		self.setup(start_object, title)
		
	def close(self):
		self.lists = []
		self.listitems = []
		self.do_postclose()
	
	def setup(self, start_object, title = ""):
		# here we set the starting point for our expedition
		self.start = start_object
		self.lists[0].setcontent(start_object, title)
		for list in self.lists[1:]:
			list.setcontent(None)
		
	def do_listhit(self, event, item):
		(what, message, when, where, modifiers) = event
		Qd.SetPort(self.wid)
		where = Qd.GlobalToLocal(where)
		for list in self.lists:
			list.active = 0
		list = self.lists[self.listitems.index(item)]
		list.active = 1
		for l in self.lists:
			l.drawframe()
		
		point = (0,0)
		ok, point = list.list.LGetSelect(1, point)
		if ok:
			oldsel = point[1]
		else:
			oldsel = -1
		# This should be: list.list.LClick(where, modifiers)
		# Since the selFlags field of the list is not accessible from Python I have to do it like this.
		# The effect is that you can't select more items by using shift or command.
		list.list.LClick(where, 0)
		
		index = self.listitems.index(item) + 1
		point = (0,0)
		ok, point = list.list.LGetSelect(1, point)
		if oldsel == point[1]:
			return	# selection didn't change, do nothing.
		if not ok:
			for i in range(index, len(self.listitems)):
				self.lists[i].setcontent(None)
			self.rightover = []
			return
			
		if point[1] >= len(list.keylist):
			return		# XXX is this still necessary? is ok really true?
		key = str(list.keylist[point[1]])
		value = list.valuelist[point[1]]
		
		self.settextitem("")
		thetype = type(value)
		if thetype == ListType or 				\
				thetype == TupleType or 		\
				thetype == DictType or 			\
				hasattr(value, "__dict__") or 		\
				hasattr(value, "__methods__") or	\
				hasattr(value, "__members__"):	# XXX or, or... again: did I miss something?
			if index >= len(self.listitems):
				# we've reached the right side of our dialog. move everything to the left
				# (by pushing the rightbutton...)
				self.do_rightbutton(1)
				index = index - 1
			newlist = self.lists[index]
			newlist.setcontent(value, key)
		else:
			index = index - 1
			self.settextitem( str(value) + "\r" + cleantype(value))
		for i in range(index + 1, len(self.listitems)):
			self.lists[i].setcontent(None)
		self.rightover = []
	
	# helper to set the big text item at the bottom of the dialog.
	def settextitem(self, text):
		tp, h, rect = self.wid.GetDialogItem(MAIN_TEXT)
		Dlg.SetDialogItemText(h, text[:255])
	
	def do_rawupdate(self, window, event):
		Qd.SetPort(self.wid)
		iType, iHandle, iRect = window.GetDialogItem(MAIN_LINE)
		Qd.FrameRect(iRect)
		for list in self.lists:
			Qd.FrameRect(list.rect)
			if list.active:
				# see MyList.drawframe
				rect2 = Qd.InsetRect(list.rect, -3, -3)
				save = Qd.GetPenState()
				Qd.PenSize(2, 2)
				Qd.FrameRect(rect2)
				Qd.SetPenState(save)
		for list in self.lists:
			list.list.LUpdate(self.wid.GetWindowPort().visRgn)
		
	def do_activate(self, activate, event):
		for list in self.lists:
			list.list.LActivate(activate)
		
	# scroll everything one 'unit' to the left
	# XXX I don't like the way this works. Too many 'manual' assignments
	def do_rightbutton(self, force = 0):
		if not force and self.rightover == []:
			return
		self.scroll(-1)
		point = (0, 0)
		ok, point = self.lists[0].list.LGetSelect(1, point)
		self.leftover.append((point, self.lists[0].content, self.lists[0].title, self.lists[0].active))
		for i in range(len(self.lists)-1):
			point = (0, 0)
			ok, point = self.lists[i+1].list.LGetSelect(1, point)
			self.lists[i].setcontent(self.lists[i+1].content, self.lists[i+1].title)
			self.lists[i].list.LSetSelect(ok, point)
			self.lists[i].list.LAutoScroll()
			self.lists[i].active = self.lists[i+1].active
			self.lists[i].drawframe()
		if len(self.rightover) > 0:
			point, content, title, active = self.rightover[-1]
			self.lists[-1].setcontent(content, title)
			self.lists[-1].list.LSetSelect(1, point)
			self.lists[-1].list.LAutoScroll()
			self.lists[-1].active = active
			self.lists[-1].drawframe()
			del self.rightover[-1]
		else:
			self.lists[-1].setcontent(None)
			self.lists[-1].active = 0
		for list in self.lists:
			list.drawframe()
	
	# scroll everything one 'unit' to the right
	def do_leftbutton(self):
		if self.leftover == []:
			return
		self.scroll(1)
		if self.lists[-1].content <> None:
			point = (0, 0)
			ok, point = self.lists[-1].list.LGetSelect(1, point)
			self.rightover.append((point, self.lists[-1].content, self.lists[-1].title, self.lists[-1].active ))
		for i in range(len(self.lists)-1, 0, -1):
			point = (0, 0)
			ok, point = self.lists[i-1].list.LGetSelect(1, point)
			self.lists[i].setcontent(self.lists[i-1].content, self.lists[i-1].title)
			self.lists[i].list.LSetSelect(ok, point)
			self.lists[i].list.LAutoScroll()
			self.lists[i].active = self.lists[i-1].active
			self.lists[i].drawframe()
		if len(self.leftover) > 0:
			point, content, title, active = self.leftover[-1]
			self.lists[0].setcontent(content, title)
			self.lists[0].list.LSetSelect(1, point)
			self.lists[0].list.LAutoScroll()
			self.lists[0].active = active
			self.lists[0].drawframe()
			del self.leftover[-1]
		else:
			self.lists[0].setcontent(None)
			self.lists[0].active = 0
	
	# create some visual feedback when 'scrolling' the lists to the left or to the right
	def scroll(self, leftright):	# leftright should be 1 or -1
		# first, build a region containing all list rectangles
		myregion = Qd.NewRgn()
		mylastregion = Qd.NewRgn()
		for list in self.lists:
			AddRect2Rgn(list.rect, myregion)
			AddRect2Rgn(list.rect, mylastregion)
		# set the pen, but save it's state first
		self.SetPort()
		save = Qd.GetPenState()
		Qd.PenPat(Qd.qd.gray)
		Qd.PenMode(srcXor)
		# how far do we have to scroll?
		distance = self.lists[1].rect[0] - self.lists[0].rect[0]
		step = 30
		lasttime = time.clock()	# for delay
		# do it
		for i in range(0, distance, step):
			if i <> 0:
				Qd.FrameRgn(mylastregion)	# erase last region
				Qd.OffsetRgn(mylastregion, step * leftright, 0)
			# draw gray region
			Qd.FrameRgn(myregion)
			Qd.OffsetRgn(myregion, step * leftright, 0)
			while time.clock() - lasttime < 0.05:
				pass	# delay
			lasttime = time.clock()
		# clean up after your dog
		Qd.FrameRgn(mylastregion)
		Qd.SetPenState(save)
	
	def reset(self):
		for list in self.lists:
			point = (0,0)
			ok, point = list.list.LGetSelect(1, point)
			if ok:
				sel = list.keylist[point[1]]
			list.setcontent(list.content, list.title)
			if ok:
				list.list.LSetSelect(1, (0, list.keylist.index(sel)))
				list.list.LAutoScroll()
	
	def do_itemhit(self, item, event):
		if item in self.listitems:
			self.do_listhit(event, item)
		elif item == MAIN_LEFT:
			self.do_leftbutton()
		elif item == MAIN_RIGHT:
			self.do_rightbutton()
		elif item == MAIN_CLOSE:
			self.close()
		elif item == MAIN_RESET:
			self.reset()

# helper function that returns a short string containing the type of an arbitrary object
# eg: cleantype("wat is dit nu weer?") -> '(string)'
def cleantype(obj):
	# type() typically returns something like: <type 'string'>
	items = string.split(str(type(obj)), "'")
	if len(items) == 3:
		return '(' + items[1] + ')'
	else:
		# just in case, I don't know.
		return str(type(obj))
	
# helper for VarBrowser.scroll
def AddRect2Rgn(theRect, theRgn):
	rRgn = Qd.NewRgn()
	Qd.RectRgn(rRgn, theRect)
	Qd.UnionRgn(rRgn, theRgn, theRgn)


if __name__ == "__main__":
	main()
