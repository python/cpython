import FrameWork
import EasyDialogs
import Res
import Dlg
import sys
import interslip
#
# Definitions for our resources
ID_MAIN=512
ID_ABOUT=513

ITEM_CONNECT=1
ITEM_DISCONNECT=2
ITEM_STATUS=3
ITEM_MESSAGE=4

status2text = ["<idle>", "<wait-modem>", "<dialling>", "<logging in>", 
	"<connected>", "<disconnecting>"]

def main():
	try:
		interslip.open()
	except interslip.error, arg:
		EasyDialogs.Message("Cannot open interslip: "+arg[1])
		sys.exit(1)	
	try:
		dummy = Res.GetResource('DLOG', ID_MAIN)
	except Res.Error:
		try:
			Res.OpenResFile("InterslipControl-2.rsrc")
		except Res.error:
			EasyDialogs.Message("Cannot open InterslipControl-2.rsrc: "+arg[1])
			sys.exit(1)	
	InterslipControl()
	
class InterslipControl(FrameWork.Application):
	"Application class for InterslipControl"
	
	def __init__(self):
		# First init menus, etc.
		FrameWork.Application.__init__(self)
		# Next create our dialog
		self.main_dialog = MyDialog(self)
		# Now open the dialog
		self.main_dialog.open(ID_MAIN)
		# Finally, go into the event loop
		self.mainloop()
	
	def makeusermenus(self):
		self.filemenu = m = FrameWork.Menu(self.menubar, "File")
		self.quititem = FrameWork.MenuItem(m, "Quit", "Q", self.quit)
	
	def quit(self, *args):
		self._quit()
		
	def do_about(self, *args):
		f = Dlg.GetNewDialog(ID_ABOUT, -1)
		while 1:
			n = Dlg.ModalDialog(None)
			if n == 1:
				return
				
	def idle(self, event):
		"Idle routine - update status"
		self.main_dialog.updatestatus()
		
class MyDialog(FrameWork.DialogWindow):
	"Main dialog window for InterslipControl"
	def __init__(self, parent):
		FrameWork.DialogWindow.__init__(self, parent)
		self.last_status = None
		self.last_msgnum = None
	
	def do_itemhit(self, item, event):
		if item == ITEM_DISCONNECT:
			self.disconnect()
		elif item == ITEM_CONNECT:
			self.connect()

	def connect(self):
		try:
			interslip.connect()
		except interslip.error, arg:
			EasyDialogs.Message("Cannot connect: "+arg[1])

	def disconnect(self):
		try:
			interslip.disconnect()
		except interslip.error, arg:
			EasyDialogs.Message("Cannot disconnect: "+arg[1])
			
	def updatestatus(self):
		try:
			status, msgnum, msg = interslip.status()
		except interslip.error, arg:
			EasyDialogs.Message("Cannot get status: "+arg[1])
			sys.exit(1)
		if status == self.last_status and msgnum == self.last_msgnum:
			return
		self.last_status = status
		self.last_msgnum = msgnum
		if msgnum == 0:
			msg = ''
		
		try:
			txt = status2text[status]
		except IndexError:
			txt = "<unknown state %d>"%status

		tp, h, rect = self.wid.GetDialogItem(ITEM_STATUS)
		Dlg.SetDialogItemText(h, txt)
		
		tp, h, rect = self.wid.GetDialogItem(ITEM_MESSAGE)
		Dlg.SetDialogItemText(h, msg)

main()
