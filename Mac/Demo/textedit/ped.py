# A minimal text editor.
#
# To be done:
# - Update viewrect after resize
# - Handle horizontal scrollbar correctly
# - Functionality: find, etc.

from Carbon.Menu import DrawMenuBar
from FrameWork import *
from Carbon import Win
from Carbon import Qd
from Carbon import TE
from Carbon import Scrap
import os
import macfs

class TEWindow(ScrolledWindow):
    def open(self, path, name, data):
        self.path = path
        self.name = name
        r = windowbounds(400, 400)
        w = Win.NewWindow(r, name, 1, 0, -1, 1, 0)
        self.wid = w
        x0, y0, x1, y1 = self.wid.GetWindowPort().GetPortBounds()
        x0 = x0 + 4
        y0 = y0 + 4
        x1 = x1 - 20
        y1 = y1 - 20
        vr = dr = x0, y0, x1, y1
        ##vr = 4, 0, r[2]-r[0]-15, r[3]-r[1]-15
        ##dr = (0, 0, vr[2], 0)
        Qd.SetPort(w)
        Qd.TextFont(4)
        Qd.TextSize(9)
        self.ted = TE.TENew(dr, vr)
        self.ted.TEAutoView(1)
        self.ted.TESetText(data)
        w.DrawGrowIcon()
        self.scrollbars()
        self.changed = 0
        self.do_postopen()
        self.do_activate(1, None)

    def do_idle(self):
        self.ted.TEIdle()

    def getscrollbarvalues(self):
        dr = self.ted.destRect
        vr = self.ted.viewRect
        height = self.ted.nLines * self.ted.lineHeight
        vx = self.scalebarvalue(dr[0], dr[2]-dr[0], vr[0], vr[2])
        vy = self.scalebarvalue(dr[1], dr[1]+height, vr[1], vr[3])
        print dr, vr, height, vx, vy
        return None, vy

    def scrollbar_callback(self, which, what, value):
        if which == 'y':
            if what == 'set':
                height = self.ted.nLines * self.ted.lineHeight
                cur = self.getscrollbarvalues()[1]
                delta = (cur-value)*height/32767
            if what == '-':
                delta = self.ted.lineHeight
            elif what == '--':
                delta = (self.ted.viewRect[3]-self.ted.lineHeight)
                if delta <= 0:
                    delta = self.ted.lineHeight
            elif what == '+':
                delta = -self.ted.lineHeight
            elif what == '++':
                delta = -(self.ted.viewRect[3]-self.ted.lineHeight)
                if delta >= 0:
                    delta = -self.ted.lineHeight
            self.ted.TEPinScroll(0, delta)
            print 'SCROLL Y', delta
        else:
            pass # No horizontal scrolling

    def do_activate(self, onoff, evt):
        print "ACTIVATE", onoff
        ScrolledWindow.do_activate(self, onoff, evt)
        if onoff:
            self.ted.TEActivate()
            self.parent.active = self
            self.parent.updatemenubar()
        else:
            self.ted.TEDeactivate()

    def do_update(self, wid, event):
        Qd.EraseRect(wid.GetWindowPort().GetPortBounds())
        self.ted.TEUpdate(wid.GetWindowPort().GetPortBounds())
        self.updatescrollbars()

    def do_contentclick(self, local, modifiers, evt):
        shifted = (modifiers & 0x200)
        self.ted.TEClick(local, shifted)
        self.updatescrollbars()
        self.parent.updatemenubar()

    def do_char(self, ch, event):
        self.ted.TESelView()
        self.ted.TEKey(ord(ch))
        self.changed = 1
        self.updatescrollbars()
        self.parent.updatemenubar()

    def close(self):
        if self.changed:
            save = EasyDialogs.AskYesNoCancel('Save window "%s" before closing?'%self.name, 1)
            if save > 0:
                self.menu_save()
            elif save < 0:
                return
        if self.parent.active == self:
            self.parent.active = None
        self.parent.updatemenubar()
        del self.ted
        self.do_postclose()

    def menu_save(self):
        if not self.path:
            self.menu_save_as()
            return # Will call us recursively
        print 'Saving to ', self.path
        dhandle = self.ted.TEGetText()
        data = dhandle.data
        fp = open(self.path, 'wb')  # NOTE: wb, because data has CR for end-of-line
        fp.write(data)
        if data[-1] <> '\r': fp.write('\r')
        fp.close()
        self.changed = 0

    def menu_save_as(self):
        path = EasyDialogs.AskFileForSave(message='Save as:')
        if not path: return
        self.path = path
        self.name = os.path.split(self.path)[-1]
        self.wid.SetWTitle(self.name)
        self.menu_save()

    def menu_cut(self):
        self.ted.TESelView()
        self.ted.TECut()
        if hasattr(Scrap, 'ZeroScrap'):
            Scrap.ZeroScrap()
        else:
            Scrap.ClearCurrentScrap()
        TE.TEToScrap()
        self.updatescrollbars()
        self.parent.updatemenubar()
        self.changed = 1

    def menu_copy(self):
        self.ted.TECopy()
        if hasattr(Scrap, 'ZeroScrap'):
            Scrap.ZeroScrap()
        else:
            Scrap.ClearCurrentScrap()
        TE.TEToScrap()
        self.updatescrollbars()
        self.parent.updatemenubar()

    def menu_paste(self):
        TE.TEFromScrap()
        self.ted.TESelView()
        self.ted.TEPaste()
        self.updatescrollbars()
        self.parent.updatemenubar()
        self.changed = 1

    def menu_clear(self):
        self.ted.TESelView()
        self.ted.TEDelete()
        self.updatescrollbars()
        self.parent.updatemenubar()
        self.changed = 1

    def have_selection(self):
        return (self.ted.selStart < self.ted.selEnd)

class Ped(Application):
    def __init__(self):
        Application.__init__(self)
        self.num = 0
        self.active = None
        self.updatemenubar()

    def makeusermenus(self):
        self.filemenu = m = Menu(self.menubar, "File")
        self.newitem = MenuItem(m, "New window", "N", self.open)
        self.openitem = MenuItem(m, "Open...", "O", self.openfile)
        self.closeitem = MenuItem(m, "Close", "W", self.closewin)
        m.addseparator()
        self.saveitem = MenuItem(m, "Save", "S", self.save)
        self.saveasitem = MenuItem(m, "Save as...", "", self.saveas)
        m.addseparator()
        self.quititem = MenuItem(m, "Quit", "Q", self.quit)

        self.editmenu = m = Menu(self.menubar, "Edit")
        self.undoitem = MenuItem(m, "Undo", "Z", self.undo)
        self.cutitem = MenuItem(m, "Cut", "X", self.cut)
        self.copyitem = MenuItem(m, "Copy", "C", self.copy)
        self.pasteitem = MenuItem(m, "Paste", "V", self.paste)
        self.clearitem = MenuItem(m, "Clear", "", self.clear)

        # Not yet implemented:
        self.undoitem.enable(0)

        # Groups of items enabled together:
        self.windowgroup = [self.closeitem, self.saveitem, self.saveasitem, self.editmenu]
        self.focusgroup = [self.cutitem, self.copyitem, self.clearitem]
        self.windowgroup_on = -1
        self.focusgroup_on = -1
        self.pastegroup_on = -1

    def updatemenubar(self):
        changed = 0
        on = (self.active <> None)
        if on <> self.windowgroup_on:
            for m in self.windowgroup:
                m.enable(on)
            self.windowgroup_on = on
            changed = 1
        if on:
            # only if we have an edit menu
            on = self.active.have_selection()
            if on <> self.focusgroup_on:
                for m in self.focusgroup:
                    m.enable(on)
                self.focusgroup_on = on
                changed = 1
            if hasattr(Scrap, 'InfoScrap'):
                on = (Scrap.InfoScrap()[0] <> 0)
            else:
                flavors = Scrap.GetCurrentScrap().GetScrapFlavorInfoList()
                for tp, info in flavors:
                    if tp == 'TEXT':
                        on = 1
                        break
                else:
                    on = 0
            if on <> self.pastegroup_on:
                self.pasteitem.enable(on)
                self.pastegroup_on = on
                changed = 1
        if changed:
            DrawMenuBar()

    #
    # Apple menu
    #

    def do_about(self, id, item, window, event):
        EasyDialogs.Message("A simple single-font text editor")

    #
    # File menu
    #

    def open(self, *args):
        self._open(0)

    def openfile(self, *args):
        self._open(1)

    def _open(self, askfile):
        if askfile:
            path = EasyDialogs.AskFileForOpen(typeList=('TEXT',))
            if not path:
                return
            name = os.path.split(path)[-1]
            try:
                fp = open(path, 'rb') # NOTE binary, we need cr as end-of-line
                data = fp.read()
                fp.close()
            except IOError, arg:
                EasyDialogs.Message("IOERROR: %r" % (arg,))
                return
        else:
            path = None
            name = "Untitled %d"%self.num
            data = ''
        w = TEWindow(self)
        w.open(path, name, data)
        self.num = self.num + 1

    def closewin(self, *args):
        if self.active:
            self.active.close()
        else:
            EasyDialogs.Message("No active window?")

    def save(self, *args):
        if self.active:
            self.active.menu_save()
        else:
            EasyDialogs.Message("No active window?")

    def saveas(self, *args):
        if self.active:
            self.active.menu_save_as()
        else:
            EasyDialogs.Message("No active window?")


    def quit(self, *args):
        for w in self._windows.values():
            w.close()
        if self._windows:
            return
        self._quit()

    #
    # Edit menu
    #

    def undo(self, *args):
        pass

    def cut(self, *args):
        if self.active:
            self.active.menu_cut()
        else:
            EasyDialogs.Message("No active window?")

    def copy(self, *args):
        if self.active:
            self.active.menu_copy()
        else:
            EasyDialogs.Message("No active window?")

    def paste(self, *args):
        if self.active:
            self.active.menu_paste()
        else:
            EasyDialogs.Message("No active window?")

    def clear(self, *args):
        if self.active:
            self.active.menu_clear()
        else:
            EasyDialogs.Message("No active window?")

    #
    # Other stuff
    #

    def idle(self, *args):
        if self.active:
            self.active.do_idle()
        else:
            Qd.SetCursor(Qd.GetQDGlobalsArrow())

def main():
    App = Ped()
    App.mainloop()

if __name__ == '__main__':
    main()
