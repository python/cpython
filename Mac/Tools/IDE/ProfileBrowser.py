import W
from Carbon import Evt
import EasyDialogs

import sys
import StringIO
import string
import pstats, fpformat

# increase precision
def f8(x):
    return string.rjust(fpformat.fix(x, 4), 8)
pstats.f8 = f8

# hacking around a hack
if sys.version[:3] > '1.4':
    timer = Evt.TickCount
else:
    def timer(TickCount = Evt.TickCount):
        return TickCount() / 60.0

class ProfileBrowser:

    def __init__(self, stats = None):
        self.sortkeys = ('calls',)
        self.setupwidgets()
        self.setstats(stats)

    def setupwidgets(self):
        self.w = W.Window((580, 400), "Profile Statistics", minsize = (200, 100), tabbable = 0)
        self.w.divline = W.HorizontalLine((0, 20, 0, 0))
        self.w.titlebar = W.TextBox((4, 4, 40, 12), 'Sort by:')
        self.buttons = []
        x = 54
        width1 = 50
        width2 = 75
        for name in ["calls", "time", "cumulative", "stdname", "file", "line", "name"]:
            if len(name) > 6:
                width = width2
            else:
                width = width1
            self.w["button_" + name] = W.RadioButton((x, 4, width, 12), name, self.buttons, self.setsort)
            x += width + 10
        self.w.button_calls.set(1)
        self.w.text = W.TextEditor((0, 21, -15, -15), inset = (6, 5),
                        readonly = 1, wrap = 0, fontsettings = ('Monaco', 0, 9, (0, 0, 0)))
        self.w._bary = W.Scrollbar((-15, 20, 16, -14), self.w.text.vscroll, max = 32767)
        self.w._barx = W.Scrollbar((-1, -15, -14, 16), self.w.text.hscroll, max = 32767)
        self.w.open()

    def setstats(self, stats):
        self.stats = stats
        self.stats.strip_dirs()
        self.displaystats()

    def setsort(self):
        # Grmpf. The callback doesn't give us the button:-(
        for b in self.buttons:
            if b.get():
                if b._title == self.sortkeys[0]:
                    return
                self.sortkeys = (b._title,) + self.sortkeys[:3]
                break
        self.displaystats()

    def displaystats(self):
        W.SetCursor('watch')
        apply(self.stats.sort_stats, self.sortkeys)
        saveout = sys.stdout
        try:
            s = sys.stdout = StringIO.StringIO()
            self.stats.print_stats()
        finally:
            sys.stdout = saveout
        text = string.join(string.split(s.getvalue(), '\n'), '\r')
        self.w.text.set(text)


def main():
    import pstats
    args = sys.argv[1:]
    for i in args:
        stats = pstats.Stats(i)
        browser = ProfileBrowser(stats)
    else:
        filename = EasyDialogs.AskFileForOpen(message='Profiler data')
        if not filename: sys.exit(0)
        stats = pstats.Stats(filename)
        browser = ProfileBrowser(stats)

if __name__ == '__main__':
    main()
