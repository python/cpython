"""browsepict - Display all "cicn" resources found"""

import FrameWork
import EasyDialogs
from Carbon import Res
from Carbon import Qd
from Carbon import Win
from Carbon import Controls
from Carbon import List
from Carbon import Icn
import macresource

#
# Resource definitions
ID_MAIN=512
MAIN_LIST=1
MAIN_SHOW=2

# Where is the picture window?
LEFT=200
TOP=64
MINWIDTH=32
MINHEIGHT=32
MAXWIDTH=320
MAXHEIGHT=320

def main():
    macresource.need('DLOG', ID_MAIN, "PICTbrowse.rsrc")
    CIconbrowse()

class CIconbrowse(FrameWork.Application):
    def __init__(self):
        # First init menus, etc.
        FrameWork.Application.__init__(self)
        # Next create our dialog
        self.main_dialog = MyDialog(self)
        # Now open the dialog
        contents = self.findcicnresources()
        self.main_dialog.open(ID_MAIN, contents)
        # Finally, go into the event loop
        self.mainloop()

    def makeusermenus(self):
        self.filemenu = m = FrameWork.Menu(self.menubar, "File")
        self.quititem = FrameWork.MenuItem(m, "Quit", "Q", self.quit)

    def quit(self, *args):
        self._quit()

    def showCIcon(self, resid):
        w = CIconwindow(self)
        w.open(resid)
        #EasyDialogs.Message('Show cicn %r' % (resid,))

    def findcicnresources(self):
        num = Res.CountResources('cicn')
        rv = []
        for i in range(1, num+1):
            Res.SetResLoad(0)
            try:
                r = Res.GetIndResource('cicn', i)
            finally:
                Res.SetResLoad(1)
            id, type, name = r.GetResInfo()
            rv.append((id, name))
        return rv

class CIconwindow(FrameWork.Window):
    def open(self, (resid, resname)):
        if not resname:
            resname = '#%r' % (resid,)
        self.resid = resid
        self.picture = Icn.GetCIcon(self.resid)
        l, t, r, b = 0, 0, 32, 32
        self.pictrect = (l, t, r, b)
        width = r-l
        height = b-t
        if width < MINWIDTH: width = MINWIDTH
        elif width > MAXWIDTH: width = MAXWIDTH
        if height < MINHEIGHT: height = MINHEIGHT
        elif height > MAXHEIGHT: height = MAXHEIGHT
        bounds = (LEFT, TOP, LEFT+width, TOP+height)

        self.wid = Win.NewWindow(bounds, resname, 1, 0, -1, 1, 0)
        self.do_postopen()

    def do_update(self, *args):
        currect = self.fitrect()
        Icn.PlotCIcon(currect, self.picture)

    def fitrect(self):
        """Return self.pictrect scaled to fit in window"""
        graf = self.wid.GetWindowPort()
        screenrect = graf.GetPortBounds()
        picwidth = self.pictrect[2] - self.pictrect[0]
        picheight = self.pictrect[3] - self.pictrect[1]
        if picwidth > screenrect[2] - screenrect[0]:
            factor = float(picwidth) / float(screenrect[2]-screenrect[0])
            picwidth = picwidth / factor
            picheight = picheight / factor
        if picheight > screenrect[3] - screenrect[1]:
            factor = float(picheight) / float(screenrect[3]-screenrect[1])
            picwidth = picwidth / factor
            picheight = picheight / factor
        return (screenrect[0], screenrect[1], screenrect[0]+int(picwidth),
                        screenrect[1]+int(picheight))

class MyDialog(FrameWork.DialogWindow):
    "Main dialog window for cicnbrowse"

    def open(self, id, contents):
        self.id = id
        FrameWork.DialogWindow.open(self, ID_MAIN)
        self.dlg.SetDialogDefaultItem(MAIN_SHOW)
        self.contents = contents
        self.ctl = self.dlg.GetDialogItemAsControl(MAIN_LIST)
        h = self.ctl.GetControlData_Handle(Controls.kControlListBoxPart,
                        Controls.kControlListBoxListHandleTag)
        self.list = List.as_List(h)
        self.setlist()

    def setlist(self):
        self.list.LDelRow(0, 0)
        self.list.LSetDrawingMode(0)
        if self.contents:
            self.list.LAddRow(len(self.contents), 0)
            for i in range(len(self.contents)):
                v = repr(self.contents[i][0])
                if self.contents[i][1]:
                    v = v + '"' + self.contents[i][1] + '"'
                self.list.LSetCell(v, (0, i))
        self.list.LSetDrawingMode(1)
        self.list.LUpdate(self.wid.GetWindowPort().visRgn)

    def getselection(self):
        items = []
        point = (0,0)
        while 1:
            ok, point = self.list.LGetSelect(1, point)
            if not ok:
                break
            items.append(point[1])
            point = point[0], point[1]+1
        values = []
        for i in items:
            values.append(self.contents[i])
        return values

    def do_show(self, *args):
        selection = self.getselection()
        for resid in selection:
            self.parent.showCIcon(resid)

    def do_close(self):
        self.close()

    def do_itemhit(self, item, event):
        if item == MAIN_SHOW:
            self.do_show()

main()
