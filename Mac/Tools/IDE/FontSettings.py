"""usage:
newsettings = FontDialog(('Chicago', 0, 12, (0, 0, 0))) # font name or id, style flags, size, color (color is ignored)
if newsettings:
        fontsettings, tabsettings = newsettings
        font, style, size, color = fontsettings # 'font' is always the font name, not the id number
        # do something
"""

import W
import PyEdit
from Carbon import TextEdit
from Carbon import Qd
import string
import types
import sys
import MacOS
if hasattr(MacOS, "SysBeep"):
    SysBeep = MacOS.SysBeep
else:
    def SysBeep(*args):
        pass

_stylenames = ["Plain", "Bold", "Italic", "Underline", "Outline", "Shadow", "Condensed", "Extended"]


class _FontDialog:

    #def __del__(self):
    #       print "doei!"

    def __init__(self, fontsettings, tabsettings):
        leftmargin = 60
        leftmargin2 = leftmargin - 16
        self.w = W.ModalDialog((440, 180), 'Font settings')
        self.w.fonttitle = W.TextBox((10, 12, leftmargin2, 14), "Font:", TextEdit.teJustRight)
        self.w.pop = W.FontMenu((leftmargin, 10, 16, 16), self.setfont)
        self.w.fontname = W.TextBox((leftmargin + 20, 12, 150, 14))
        self.w.sizetitle = W.TextBox((10, 38, leftmargin2, 14), "Size:", TextEdit.teJustRight)
        self.w.sizeedit = W.EditText((leftmargin, 35, 40, 20), "", self.checksize)
        styletop = 64
        self.w.styletitle = W.TextBox((10, styletop + 2, leftmargin2, 14), "Style:", TextEdit.teJustRight)
        for i in range(len(_stylenames)):
            top = styletop + (i % 4) * 20
            left = leftmargin + 80 * (i > 3) - 2
            if i:
                self.w[i] = W.CheckBox((left, top, 76, 16), _stylenames[i], self.dostyle)
            else:
                self.w[i] = W.CheckBox((left, top, 70, 16), _stylenames[i], self.doplain)

        if tabsettings:
            self.lasttab, self.tabmode = tabsettings
            self.w.tabsizetitle = W.TextBox((10, -26, leftmargin2, 14), "Tabsize:", TextEdit.teJustRight)
            self.w.tabsizeedit = W.EditText((leftmargin, -29, 40, 20), "", self.checktab)
            self.w.tabsizeedit.set(repr(self.lasttab))
            radiobuttons = []
            self.w.tabsizechars = W.RadioButton((leftmargin + 48, -26, 55, 14), "Spaces",
                            radiobuttons, self.toggletabmode)
            self.w.tabsizepixels = W.RadioButton((leftmargin + 110, -26, 55, 14), "Pixels",
                            radiobuttons, self.toggletabmode)
            if self.tabmode:
                self.w.tabsizechars.set(1)
            else:
                self.w.tabsizepixels.set(1)
        else:
            self.tabmode = None

        self.w.cancelbutton = W.Button((-180, -26, 80, 16), "Cancel", self.cancel)
        self.w.donebutton = W.Button((-90, -26, 80, 16), "Done", self.done)

        sampletext = "Sample text."
        self.w.sample = W.EditText((230, 10, -10, 130), sampletext,
                        fontsettings = fontsettings, tabsettings = tabsettings)

        self.w.setdefaultbutton(self.w.donebutton)
        self.w.bind('cmd.', self.w.cancelbutton.push)
        self.w.bind('cmdw', self.w.donebutton.push)
        self.lastsize = fontsettings[2]
        self._rv = None
        self.set(fontsettings)
        self.w.open()

    def toggletabmode(self, onoff):
        if self.w.tabsizechars.get():
            tabmode = 1
        else:
            tabmode = 0
        if self.tabmode <> tabmode:
            port = self.w.wid.GetWindowPort()
            (font, style, size, color), (tabsize, dummy) = self.get()
            savesettings = W.GetPortFontSettings(port)
            W.SetPortFontSettings(port, (font, style, size))
            spacewidth = Qd.StringWidth(' ')
            W.SetPortFontSettings(port, savesettings)
            if tabmode:
                # convert pixels to spaces
                self.lasttab = int(round(float(tabsize) / spacewidth))
            else:
                # convert spaces to pixels
                self.lasttab = spacewidth * tabsize
            self.w.tabsizeedit.set(repr(self.lasttab))
            self.tabmode = tabmode
            self.doit()

    def set(self, fontsettings):
        font, style, size, color = fontsettings
        if type(font) <> types.StringType:
            from Carbon import Res
            res = Res.GetResource('FOND', font)
            font = res.GetResInfo()[2]
        self.w.fontname.set(font)
        self.w.sizeedit.set(str(size))
        if style:
            for i in range(1, len(_stylenames)):
                self.w[i].set(style & 0x01)
                style = style >> 1
        else:
            self.w[0].set(1)

    def get(self):
        font = self.w.fontname.get()
        style = 0
        if not self.w[0].get():
            flag = 0x01
            for i in range(1, len(_stylenames)):
                if self.w[i].get():
                    style = style | flag
                flag = flag << 1
        size = self.lastsize
        if self.tabmode is None:
            return (font, style, size, (0, 0, 0)), (32, 0)
        else:
            return (font, style, size, (0, 0, 0)), (self.lasttab, self.tabmode)

    def doit(self):
        if self.w[0].get():
            style = 0
        else:
            style = 0
            for i in range(1, len(_stylenames)):
                if self.w[i].get():
                    style = style | 2 ** (i - 1)
        #self.w.sample.set(repr(style))
        fontsettings, tabsettings = self.get()
        self.w.sample.setfontsettings(fontsettings)
        self.w.sample.settabsettings(tabsettings)

    def checktab(self):
        tabsize = self.w.tabsizeedit.get()
        if not tabsize:
            return
        try:
            tabsize = string.atoi(tabsize)
        except (ValueError, OverflowError):
            good = 0
            sys.exc_traceback = None
        else:
            good = 1 <= tabsize <= 500
        if good:
            if self.lasttab <> tabsize:
                self.lasttab = tabsize
                self.doit()
        else:
            SysBeep(0)
            self.w.tabsizeedit.set(repr(self.lasttab))
            self.w.tabsizeedit.selectall()

    def checksize(self):
        size = self.w.sizeedit.get()
        if not size:
            return
        try:
            size = string.atoi(size)
        except (ValueError, OverflowError):
            good = 0
            sys.exc_traceback = None
        else:
            good = 1 <= size <= 500
        if good:
            if self.lastsize <> size:
                self.lastsize = size
                self.doit()
        else:
            SysBeep(0)
            self.w.sizeedit.set(repr(self.lastsize))
            self.w.sizeedit.selectall()

    def doplain(self):
        for i in range(1, len(_stylenames)):
            self.w[i].set(0)
        self.w[0].set(1)
        self.doit()

    def dostyle(self):
        for i in range(1, len(_stylenames)):
            if self.w[i].get():
                self.w[0].set(0)
                break
        else:
            self.w[0].set(1)
        self.doit()

    def close(self):
        self.w.close()
        del self.w

    def cancel(self):
        self.close()

    def done(self):
        self._rv = self.get()
        self.close()

    def setfont(self, fontname):
        self.w.fontname.set(fontname)
        self.doit()


def FontDialog(fontsettings, tabsettings = (32, 0)):
    fd = _FontDialog(fontsettings, tabsettings)
    return fd._rv

def test():
    print FontDialog(('Zapata-Light', 0, 25, (0, 0, 0)))
