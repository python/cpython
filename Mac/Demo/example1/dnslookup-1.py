"""Sample program performing domain name lookups and showing off EasyDialogs,
Res and Dlg in the process"""

import EasyDialogs
from Carbon import Res
from Carbon import Dlg
import sys
import socket
import string
import macresource
#
# Definitions for our resources
ID_MAIN=512

ITEM_LOOKUP_ENTRY=1
ITEM_RESULT=2
ITEM_LOOKUP_BUTTON=3
ITEM_QUIT_BUTTON=4

def main():
    """Main routine: open resource file, call dialog handler"""
    macresource.need("DLOG", ID_MAIN, "dnslookup-1.rsrc")
    do_dialog()

def do_dialog():
    """Post dialog and handle user interaction until quit"""
    my_dlg = Dlg.GetNewDialog(ID_MAIN, -1)
    while 1:
        n = Dlg.ModalDialog(None)
        if n == ITEM_LOOKUP_BUTTON:
            tp, h, rect = my_dlg.GetDialogItem(ITEM_LOOKUP_ENTRY)
            txt = Dlg.GetDialogItemText(h)

            tp, h, rect = my_dlg.GetDialogItem(ITEM_RESULT)
            Dlg.SetDialogItemText(h, dnslookup(txt))
        elif n == ITEM_QUIT_BUTTON:
            break

def dnslookup(str):
    """ Perform DNS lookup on str.  If first character of digit is numeric,
        assume that str contains an IP address.  Otherwise, assume that str
        contains a hostname."""
    if str == '': str = ' '
    if str[0] in string.digits:
        try:
            value = socket.gethostbyaddr(str)[0]
        except:
            value = 'Lookup failed'
    else:
        try:
            value = socket.gethostbyname(str)
        except:
            value = 'Lookup failed'
    return value

main()
