# A stab at a python debugger
import Res
import Qd
import Dlg
import Win
import FrameWork
import EasyDialogs
import sys
import TwitCore
from mac_widgets import MT_AnyList, MT_IconTextWidget

# Our dialogs
ID_MODULES=500
I_MODULES_TITLE=1
I_MODULES=2
I_VARS_TITLE=3
I_VARS=4
I_SOURCE_TITLE=5
I_SOURCE=6
I_RULER=7
I_EDIT=8

class ModuleBrowser(FrameWork.DialogWindow, TwitCore.ModuleBrowser):
	"""The module-browser dialog - mac-dependent part"""
	def open(self, module):
		FrameWork.DialogWindow.open(self, ID_MODULES)
		self.SetPort()
		Qd.TextFont(3)
		Qd.TextSize(9)

		tp, h, rect = self.wid.GetDialogItem(I_MODULES)
		self.modules = MT_AnyList(self.wid, rect, 1)
		tp, h, rect = self.wid.GetDialogItem(I_VARS)
		self.vars = MT_AnyList(self.wid, rect, 2)
		tp, h, rect = self.wid.GetDialogItem(I_SOURCE)
		self.source = MT_IconTextWidget(self.wid, rect)

		self.mi_open(module)
		
	def setsource(self, msg):
		tp, h, rect = self.wid.GetDialogItem(I_SOURCE_TITLE)
		if self.cur_source:
			Dlg.SetDialogItemText(h, self.cur_source)
		else:
			Dlg.SetDialogItemText(h, msg)
		self.source.setcontent(self.cur_source)

	def source_setbreaks(self, list):
		self.source.setbreaks(list)
		
	def source_setline(self, lineno, icon):
		self.source.setcurline(lineno, icon)
		
	def source_select(self, lineno):
		self.source.select(lineno)

	def setmodulenames(self):
		self.modules.setcontent(self.cont_modules)

	def module_select(self, number):
		self.modules.select(number)

	def setvars(self):
		self.vars.setcontent(self.cont_varnames, self.cont_varvalues)
				
	def do_itemhit(self, item, event):
		(what, message, when, where, modifiers) = event
		Qd.SetPort(self.wid)
		where = Qd.GlobalToLocal(where)
		
		if item == I_MODULES:
			new_module, double = self.modules.click(where, 0)
			self.click_module(new_module)
		elif item == I_VARS:
			new_var, double = self.vars.click(where, 0)
			if double:
				self.click_var(new_var)
		elif item == I_SOURCE:
			lineno, inborder = self.source.click(where, 0)
			if lineno <> None and lineno >= 0:
				self.click_source(lineno, inborder)
		elif item == I_EDIT:
			self.click_edit()
	
	def do_rawupdate(self, window, event):
		Qd.SetPort(self.wid)
		rgn = self.wid.GetWindowPort().visRgn
		tp, h, rect = self.wid.GetDialogItem(I_RULER)
		Qd.MoveTo(rect[0], rect[1])
		Qd.LineTo(rect[2], rect[1])
		self.modules.update(rgn)
		self.vars.update(rgn)
		self.source.update(rgn)
		
	def force_redraw(self):
		Qd.SetPort(self.wid)
		self.wid.InvalWindowRgn(self.wid.GetWindowPort().visRgn)
		
	def do_activate(self, activate, event):
		self.modules.activate(activate)
		self.vars.activate(activate)
		self.source.activate(activate)
		
	def close(self):
		self.parent.module_dialog = None
		self.source.close()
		del self.modules
		del self.vars
		del self.source
		self.do_postclose()

if __name__ == '__main__':
	main()
	
