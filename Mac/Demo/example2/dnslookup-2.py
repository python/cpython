import FrameWork
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
ID_ABOUT=513

ITEM_LOOKUP_ENTRY=1
ITEM_RESULT=2
ITEM_LOOKUP_BUTTON=3

def main():
    macresource.need("DLOG", ID_MAIN, "dnslookup-2.rsrc")
    DNSLookup()

class DNSLookup(FrameWork.Application):
    "Application class for DNS Lookup"

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

class MyDialog(FrameWork.DialogWindow):
    "Main dialog window for DNSLookup"
    def __init__(self, parent):
        FrameWork.DialogWindow.__init__(self, parent)
        self.parent = parent

    def do_itemhit(self, item, event):
        if item == ITEM_LOOKUP_BUTTON:
            self.dolookup()

    def dolookup(self):
        """Get text entered in the lookup entry area.  Place result of the
           call to dnslookup in the result entry area."""
        tp, h, rect = self.dlg.GetDialogItem(ITEM_LOOKUP_ENTRY)
        txt = Dlg.GetDialogItemText(h)

        tp, h, rect = self.dlg.GetDialogItem(ITEM_RESULT)
        Dlg.SetDialogItemText(h, self.dnslookup(txt))

    def dnslookup(self, str):
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
