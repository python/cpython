"""imgbrowse - Display pictures using img"""

import FrameWork
import EasyDialogs
from Carbon import Res
from Carbon import Qd
from Carbon import QuickDraw
from Carbon import Win
#ifrom Carbon mport List
import sys
import struct
import img
import imgformat
import struct
import mac_image


# Where is the picture window?
LEFT=200
TOP=64
MINWIDTH=64
MINHEIGHT=64
MAXWIDTH=320
MAXHEIGHT=320


def main():
    print 'hello world'
    imgbrowse()

class imgbrowse(FrameWork.Application):
    def __init__(self):
        # First init menus, etc.
        FrameWork.Application.__init__(self)
        self.lastwin = None
        # Finally, go into the event loop
        self.mainloop()

    def makeusermenus(self):
        self.filemenu = m = FrameWork.Menu(self.menubar, "File")
        self.openitem = FrameWork.MenuItem(m, "Open...", "O", self.opendoc)
        self.infoitem = FrameWork.MenuItem(m, "Info", "I", self.info)
        self.quititem = FrameWork.MenuItem(m, "Quit", "Q", self.quit)

    def quit(self, *args):
        self._quit()

    def opendoc(self, *args):
        pathname = EasyDialogs.AskFileForOpen() # Any file type
        if not pathname:
            return
        bar = EasyDialogs.ProgressBar('Reading and converting...')
        try:
            rdr = img.reader(imgformat.macrgb16, pathname)
        except img.error, arg:
            EasyDialogs.Message(repr(arg))
            return
        w, h = rdr.width, rdr.height
        bar.set(10)
        data = rdr.read()
        del bar
        pixmap = mac_image.mkpixmap(w, h, imgformat.macrgb16, data)
        self.showimg(w, h, pixmap, data)

    def showimg(self, w, h, pixmap, data):
        win = imgwindow(self)
        win.open(w, h, pixmap, data)
        self.lastwin = win

    def info(self, *args):
        if self.lastwin:
            self.lastwin.info()

class imgwindow(FrameWork.Window):
    def open(self, width, height, pixmap, data):
        self.pixmap = pixmap
        self.data = data
        self.pictrect = (0, 0, width, height)
        bounds = (LEFT, TOP, LEFT+width, TOP+height)

        self.wid = Win.NewCWindow(bounds, "Picture", 1, 0, -1, 1, 0)
        self.do_postopen()

    def do_update(self, *args):
        pass
        currect = self.fitrect()
        print 'PICT:', self.pictrect
        print 'WIND:', currect
        print 'ARGS:', (self.pixmap, self.wid.GetWindowPort().GetPortBitMapForCopyBits(), self.pictrect,
                        currect, QuickDraw.srcCopy, None)
        self.info()
        Qd.CopyBits(self.pixmap, self.wid.GetWindowPort().GetPortBitMapForCopyBits(), self.pictrect,
                        currect, QuickDraw.srcCopy, None)

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

    def info(self):
        graf = self.wid.GetWindowPort()
        bits = graf.GetPortBitMapForCopyBits()
        mac_image.dumppixmap(bits.pixmap_data)

main()
