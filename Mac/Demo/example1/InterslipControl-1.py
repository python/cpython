"""Sample program handling InterSLIP control and showing off EasyDialogs,
Res and Dlg in the process"""

import EasyDialogs
import Res
import Dlg
import sys
import interslip
#
# Definitions for our resources
ID_MAIN=512

ITEM_CONNECT=1
ITEM_DISCONNECT=2
ITEM_UPDATE=3
ITEM_QUIT=4
ITEM_STATUS=5
ITEM_MESSAGE=6

status2text = ["<idle>", "<wait-modem>", "<dialling>", "<logging in>", 
	"<connected>", "<disconnecting>"]
	
			
def main():
	"""Main routine: open resourcefile, open interslip, call dialog handler"""
	try:
		Res.OpenResFile("InterslipControl-1.rsrc")
	except Res.Error, arg:
		EasyDialogs.Message("Cannot open resource file InterslipControl-1.rsrc: "+
			arg[1])
		sys.exit(1)
	try:
		interslip.open()
	except interslip.error, arg:
		EasyDialogs.Message("Cannot open interslip: "+arg[1])
		sys.exit(1)
	do_dialog()

def do_dialog():
	"""Post dialog and handle user interaction until quit"""
	my_dlg = Dlg.GetNewDialog(ID_MAIN, -1)
	while 1:
		n = Dlg.ModalDialog(None)
		if n == ITEM_CONNECT:
			do_connect()
		elif n == ITEM_DISCONNECT:
			do_disconnect()
		elif n == ITEM_UPDATE:
			status, msg = do_status()

			# Convert status number to a text string
			try:
				txt = status2text[status]
			except IndexError:
				txt = "<unknown state %d>"%status

			# Set the status text field
			tp, h, rect = my_dlg.GetDialogItem(ITEM_STATUS)
			Dlg.SetDialogItemText(h, txt)
			
			# Set the message text field
			tp, h, rect = my_dlg.GetDialogItem(ITEM_MESSAGE)
			Dlg.SetDialogItemText(h, msg)
		elif n == ITEM_QUIT:
			break

def do_connect():
	"""Connect, posting error message in case of failure"""
	try:
		interslip.connect()
	except interslip.error, arg:
		EasyDialogs.Message("Cannot connect: "+arg[1])

def do_disconnect():
	"""Disconnect, posting error message in case of failure"""
	try:
		interslip.disconnect()
	except interslip.error, arg:
		EasyDialogs.Message("Cannot disconnect: "+arg[1])
		
def do_status():
	"""Get status as (state_index, message),
	posting error message in case of failure"""
	try:
		status, msgnum, msg = interslip.status()
	except interslip.error, arg:
		EasyDialogs.Message("Cannot get status: "+arg[1])
		return 0, ''
	return status, msg
		

main()
