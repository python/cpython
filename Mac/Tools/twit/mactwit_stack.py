# A stab at a python debugger
import Res
import Qd
import Dlg
import Win
import FrameWork
import EasyDialogs
import sys
from mac_widgets import MT_AnyList, MT_IndexList, MT_IconTextWidget

# Our dialogs
ID_STACK=513
I_STACK_TITLE=1
I_STACK=2
I_VARS_TITLE=3
I_VARS=4
I_SOURCE_TITLE=5
I_SOURCE=6
I_EXC_TITLE=7
I_EXC=8
I_EXCVALUE_TITLE=9
I_EXCVALUE=10
I_QUIT=11
I_CONTINUE=12
I_STEP=13
I_BROWSE=14
I_RULER1=15
I_RULER2=16
I_STATE_TITLE=17
I_STATE=18
I_STEP_IN=19
I_STEP_OUT=20
I_RUN=21
I_SHOW_COMPLEX=22
I_SHOW_SYSTEM=23
I_EDIT=24

class StackBrowser(FrameWork.DialogWindow):
	"""The stack-browser dialog - mac-dependent part"""
	def open(self):
		FrameWork.DialogWindow.open(self, ID_STACK)
		self.SetPort()
		Qd.TextFont(3)
		Qd.TextSize(9)
		self.mi_open()
		
	def create_items(self):
		"""Create the lists we need"""
		tp, h, rect = self.wid.GetDialogItem(I_STACK)
		self.stack = MT_IndexList(self.wid, rect, 2)
		tp, h, rect = self.wid.GetDialogItem(I_VARS)
		self.vars = MT_AnyList(self.wid, rect, 2)
		tp, h, rect = self.wid.GetDialogItem(I_SOURCE)
		self.source = MT_IconTextWidget(self.wid, rect)
		
	def setsource(self, msg):
		tp, h, rect = self.wid.GetDialogItem(I_SOURCE_TITLE)
		if self.cur_source:
			Dlg.SetDialogItemText(h, self.cur_source)
		else:
			Dlg.SetDialogItemText(h, msg)
		self.source.setcontent(self.cur_source)
		
	def setexception(self, name, value):
		print 'SETEXC', (name, value)
		if name == None:
			self.wid.HideDialogItem(I_EXC)
			self.wid.HideDialogItem(I_EXC_TITLE)
			value = None
		else:
			self.wid.ShowDialogItem(I_EXC)
			self.wid.ShowDialogItem(I_EXC_TITLE)
			tp, h, rect = self.wid.GetDialogItem(I_EXC)
			Dlg.SetDialogItemText(h, name)
		if value == None:
			self.wid.HideDialogItem(I_EXCVALUE)
			self.wid.HideDialogItem(I_EXCVALUE_TITLE)
		else:
			self.wid.ShowDialogItem(I_EXCVALUE)
			self.wid.ShowDialogItem(I_EXCVALUE_TITLE)
			tp, h, rect = self.wid.GetDialogItem(I_EXCVALUE)
			Dlg.SetDialogItemText(h, value)
		
	def setprogramstate(self, msg):
		tp, h, rect = self.wid.GetDialogItem(I_STATE)
		Dlg.SetDialogItemText(h, msg)
		
	def setsession_none(self):
		"""Nothing running, no stack trace"""
		self.wid.HideDialogItem(I_CONTINUE)
		self.wid.HideDialogItem(I_STEP)
		self.wid.HideDialogItem(I_STEP_IN)
		self.wid.HideDialogItem(I_STEP_OUT)
		self.wid.ShowDialogItem(I_RUN)
		
	def setsession_run(self):
		"""Running"""
		self.wid.HideDialogItem(I_RUN)
		self.wid.ShowDialogItem(I_CONTINUE)
		self.wid.ShowDialogItem(I_STEP)
		self.wid.ShowDialogItem(I_STEP_IN)
		self.wid.ShowDialogItem(I_STEP_OUT)
		
	def setsession_pm(self):
		"""Post-mortem debugging"""
		self.wid.HideDialogItem(I_CONTINUE)
		self.wid.HideDialogItem(I_STEP)
		self.wid.HideDialogItem(I_STEP_IN)
		self.wid.HideDialogItem(I_STEP_OUT)
		self.wid.HideDialogItem(I_RUN)
		
	def setsession_none(self):
		"""Nothing running, no stack trace"""
		self.wid.HideDialogItem(I_CONTINUE)
		self.wid.HideDialogItem(I_STEP)
		self.wid.HideDialogItem(I_STEP_IN)
		self.wid.HideDialogItem(I_STEP_OUT)
		self.wid.ShowDialogItem(I_RUN)
		
		
	def do_itemhit(self, item, event):
		(what, message, when, where, modifiers) = event
		Qd.SetPort(self.wid)
		where = Qd.GlobalToLocal(where)
		
		print 'STACKHIT', item
		
		if item == I_STACK:
			new_stackitem, double = self.stack.click(where, 0)
			self.click_stack(new_stackitem)
		elif item == I_VARS:
			new_var, double = self.vars.click(where, 0)
			if double:
				self.click_var(new_var)
		elif item == I_SOURCE:
			lineno, inborder = self.source.click(where, 0)
			if lineno <> None and lineno >= 0:
				self.click_source(lineno, inborder)
		elif item == I_QUIT:
			self.click_quit()
		elif item == I_CONTINUE:
			self.click_continue()
		elif item == I_STEP:
			self.click_step()
		elif item == I_STEP_IN:
			self.click_step_in()
		elif item == I_STEP_OUT:
			self.click_step_out()
		elif item == I_BROWSE:
			self.click_browse()
		elif item == I_RUN:
			self.click_run()
		elif item == I_SHOW_COMPLEX:
			self.show_complex = not self.show_complex
			self.setup_frame()
		elif item == I_SHOW_SYSTEM:
			self.show_system = not self.show_system
			self.setup_frame()
		elif item == I_EDIT:
			self.click_edit()
			
	def set_var_buttons(self):
		tp, h, rect = self.wid.GetDialogItem(I_SHOW_COMPLEX)
		h.as_Control().SetControlValue(self.show_complex)
		tp, h, rect = self.wid.GetDialogItem(I_SHOW_SYSTEM)
		h.as_Control().SetControlValue(self.show_system)
	
	def do_rawupdate(self, window, event):
		Qd.SetPort(self.wid)
		rgn = self.wid.GetWindowPort().visRgn
		tp, h, rect = self.wid.GetDialogItem(I_RULER1)
		Qd.MoveTo(rect[0], rect[1])
		Qd.LineTo(rect[2], rect[1])
		tp, h, rect = self.wid.GetDialogItem(I_RULER2)
		Qd.MoveTo(rect[0], rect[1])
		Qd.LineTo(rect[2], rect[1])
		self.stack.update(rgn)
		self.vars.update(rgn)
		self.source.update(rgn)
		
	def do_activate(self, activate, event):
		self.stack.activate(activate)
		self.vars.activate(activate)
		self.source.activate(activate)
				
	def close(self):
		self.source.close()
		del self.stack
		del self.vars
		del self.source
		self.do_postclose()
