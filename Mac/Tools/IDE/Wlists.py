import Wbase
import Wkeys
import string
from Carbon import Evt, Events, Fm, Lists, Qd, Scrap, Win
from Carbon.List import LNew, CreateCustomList
from Carbon.Lists import kListDefUserProcType, lInitMsg, lDrawMsg, lHiliteMsg, lCloseMsg
from Carbon.QuickDraw import hilitetransfermode
from Carbon import App
from Carbon.Appearance import kThemeStateActive, kThemeStateInactive, kThemeStatePressed


class List(Wbase.SelectableWidget):

    """Standard list widget."""

    LDEF_ID = 0

    def __init__(self, possize, items = None, callback = None, flags = 0, cols = 1, typingcasesens=0):
        if items is None:
            items = []
        self.items = items
        Wbase.SelectableWidget.__init__(self, possize)
        self._selected = 0
        self._enabled = 1
        self._list = None
        self._cols = cols
        self._callback = callback
        self._flags = flags
        self.typingcasesens = typingcasesens
        self.lasttyping = ""
        self.lasttime = Evt.TickCount()
        self.timelimit = 30
        self.setitems(items)
        self.drawingmode = 0

    def open(self):
        self.setdrawingmode(0)
        self.createlist()
        self.setdrawingmode(1)

    def createlist(self):
        self._calcbounds()
        self.SetPort()
        rect = self._bounds
        rect = rect[0]+1, rect[1]+1, rect[2]-16, rect[3]-1
        self._viewbounds = rect
        self._list = LNew(rect, (0, 0, self._cols, 0), (0, 0), self.LDEF_ID, self._parentwindow.wid,
                                0, 1, 0, 1)
        if self.drawingmode:
            self._list.LSetDrawingMode(0)
        self._list.selFlags = self._flags
        self.setitems(self.items)
        if hasattr(self, "_sel"):
            self.setselection(self._sel)
            del self._sel

    def adjust(self, oldbounds):
        self.SetPort()
        # Appearance frames are drawn outside the specified bounds,
        # so we always need to outset the invalidated area.
        self.GetWindow().InvalWindowRect(Qd.InsetRect(oldbounds, -3, -3))
        self.GetWindow().InvalWindowRect(Qd.InsetRect(self._bounds, -3, -3))

        if oldbounds[:2] == self._bounds[:2]:
            # set visRgn to empty, to prevent nasty drawing side effect of LSize()
            Qd.RectRgn(self._parentwindow.wid.GetWindowPort().visRgn, (0, 0, 0, 0))
            # list still has the same upper/left coordinates, use LSize
            l, t, r, b = self._bounds
            width = r - l - 17
            height = b - t - 2
            vl, vt, vr, vb = self._viewbounds
            self._viewbounds = vl, vt, vl + width, vt + height
            self._list.LSize(width, height)
            # now *why* doesn't the list manager recalc the cellrect???
            l, t, r, b = self._list.LRect((0,0))
            cellheight = b - t
            self._list.LCellSize((width/self._cols, cellheight))
            # reset visRgn
            self._parentwindow.wid.CalcVis()
        else:
            # oh well, since the list manager doesn't have a LMove call,
            # we have to make the list all over again...
            sel = self.getselection()
            topcell = self.gettopcell()
            self._list = None
            self.setdrawingmode(0)
            self.createlist()
            self.setselection(sel)
            self.settopcell(topcell)
            self.setdrawingmode(1)

    def close(self):
        self._list = None
        self._callback = None
        self.items = []
        Wbase.SelectableWidget.close(self)

    def set(self, items):
        self.setitems(items)

    def setitems(self, items):
        self.items = items
        the_list = self._list
        if not self._parent or not self._list:
            return
        self.setdrawingmode(0)
        topcell = self.gettopcell()
        the_list.LDelRow(0, 1)
        the_list.LAddRow(len(self.items), 0)
        self_itemrepr = self.itemrepr
        set_cell = the_list.LSetCell
        for i in range(len(items)):
            set_cell(self_itemrepr(items[i]), (0, i))
        self.settopcell(topcell)
        self.setdrawingmode(1)

    def click(self, point, modifiers):
        if not self._enabled:
            return
        isdoubleclick = self._list.LClick(point, modifiers)
        if self._callback:
            Wbase.CallbackCall(self._callback, 0, isdoubleclick)
        return 1

    def key(self, char, event):
        (what, message, when, where, modifiers) = event
        sel = self.getselection()
        newselection = []
        if char == Wkeys.uparrowkey:
            if len(sel) >= 1 and min(sel) > 0:
                newselection = [min(sel) - 1]
            else:
                newselection = [0]
        elif char == Wkeys.downarrowkey:
            if len(sel) >= 1 and max(sel) < (len(self.items) - 1):
                newselection = [max(sel) + 1]
            else:
                newselection = [len(self.items) - 1]
        else:
            modifiers = 0
            if (self.lasttime + self.timelimit) < Evt.TickCount():
                self.lasttyping = ""
            if self.typingcasesens:
                self.lasttyping = self.lasttyping + char
            else:
                self.lasttyping = self.lasttyping + string.lower(char)
            self.lasttime = Evt.TickCount()
            i = self.findmatch(self.lasttyping)
            newselection = [i]
        if modifiers & Events.shiftKey and not self._list.selFlags & Lists.lOnlyOne:
            newselection = newselection + sel
        self.setselection(newselection)
        self._list.LAutoScroll()
        self.click((-1, -1), 0)

    def findmatch(self, tag):
        lower = string.lower
        items = self.items
        typingcasesens = self.typingcasesens
        taglen = len(tag)
        match = '\377' * 100
        match_i = -1
        for i in range(len(items)):
            item = str(items[i])
            if not typingcasesens:
                item = lower(item)
            if tag <= item < match:
                match = item
                match_i = i
        if match_i >= 0:
            return match_i
        else:
            return len(items) - 1

    def domenu_copy(self, *args):
        sel = self.getselection()
        selitems = []
        for i in sel:
            selitems.append(str(self.items[i]))
        text = string.join(selitems, '\r')
        if text:
            if hasattr(Scrap, 'PutScrap'):
                Scrap.ZeroScrap()
                Scrap.PutScrap('TEXT', text)
            else:
                Scrap.ClearCurrentScrap()
                sc = Scrap.GetCurrentScrap()
                sc.PutScrapFlavor('TEXT', 0, text)

    def can_copy(self, *args):
        return len(self.getselection()) <> 0

    def domenu_selectall(self, *args):
        self.selectall()

    def can_selectall(self, *args):
        return not self._list.selFlags & Lists.lOnlyOne

    def selectall(self):
        if not self._list.selFlags & Lists.lOnlyOne:
            self.setselection(range(len(self.items)))
            self._list.LAutoScroll()
            self.click((-1, -1), 0)

    def getselection(self):
        if not self._parent or not self._list:
            if hasattr(self, "_sel"):
                return self._sel
            return []
        items = []
        point = (0,0)
        while 1:
            ok, point = self._list.LGetSelect(1, point)
            if not ok:
                break
            items.append(point[1])
            point = point[0], point[1]+1
        return items

    def setselection(self, selection):
        if not self._parent or not self._list:
            self._sel = selection
            return
        set_sel = self._list.LSetSelect
        for i in range(len(self.items)):
            if i in selection:
                set_sel(1, (0, i))
            else:
                set_sel(0, (0, i))
        self._list.LAutoScroll()

    def getselectedobjects(self):
        sel = self.getselection()
        objects = []
        for i in sel:
            objects.append(self.items[i])
        return objects

    def setselectedobjects(self, objects):
        sel = []
        for o in objects:
            try:
                sel.append(self.items.index(o))
            except:
                pass
        self.setselection(sel)

    def gettopcell(self):
        l, t, r, b = self._bounds
        t = t + 1
        cl, ct, cr, cb = self._list.LRect((0, 0))
        cellheight = cb - ct
        return (t - ct) / cellheight

    def settopcell(self, topcell):
        top = self.gettopcell()
        diff = topcell - top
        self._list.LScroll(0, diff)

    def draw(self, visRgn = None):
        if self._visible:
            if not visRgn:
                visRgn = self._parentwindow.wid.GetWindowPort().visRgn
            self._list.LUpdate(visRgn)
            state = [kThemeStateActive, kThemeStateInactive][not self._activated]
            App.DrawThemeListBoxFrame(Qd.InsetRect(self._bounds, 1, 1), state)
            if self._selected and self._activated:
                self.drawselframe(1)

    def select(self, onoff, isclick = 0):
        if Wbase.SelectableWidget.select(self, onoff):
            return
        self.SetPort()
        self.drawselframe(onoff)

    def activate(self, onoff):
        self._activated = onoff
        if self._visible:
            self._list.LActivate(onoff)
            #state = [kThemeStateActive, kThemeStateInactive][not onoff]
            #App.DrawThemeListBoxFrame(Qd.InsetRect(self._bounds, 1, 1), state)
            if self._selected:
                self.drawselframe(onoff)

    def get(self):
        return self.items

    def itemrepr(self, item):
        return str(item)[:255]

    def __getitem__(self, index):
        return self.items[index]

    def __setitem__(self, index, item):
        if self._parent and self._list:
            self._list.LSetCell(self.itemrepr(item), (0, index))
        self.items[index] = item

    def __delitem__(self, index):
        if self._parent and self._list:
            self._list.LDelRow(1, index)
        del self.items[index]

    def __getslice__(self, a, b):
        return self.items[a:b]

    def __delslice__(self, a, b):
        if b-a:
            if self._parent and self._list:
                self._list.LDelRow(b-a, a)
            del self.items[a:b]

    def __setslice__(self, a, b, items):
        if self._parent and self._list:
            l = len(items)
            the_list = self._list
            self.setdrawingmode(0)
            if b-a:
                if b > len(self.items):
                # fix for new 1.5 "feature" where b is sys.maxint instead of len(self)...
                # LDelRow doesn't like maxint.
                    b = len(self.items)
                the_list.LDelRow(b-a, a)
            the_list.LAddRow(l, a)
            self_itemrepr = self.itemrepr
            set_cell = the_list.LSetCell
            for i in range(len(items)):
                set_cell(self_itemrepr(items[i]), (0, i + a))
            self.items[a:b] = items
            self.setdrawingmode(1)
        else:
            self.items[a:b] = items

    def __len__(self):
        return len(self.items)

    def append(self, item):
        if self._parent and self._list:
            index = len(self.items)
            self._list.LAddRow(1, index)
            self._list.LSetCell(self.itemrepr(item), (0, index))
        self.items.append(item)

    def remove(self, item):
        index = self.items.index(item)
        self.__delitem__(index)

    def index(self, item):
        return self.items.index(item)

    def insert(self, index, item):
        if index < 0:
            index = 0
        if self._parent and self._list:
            self._list.LAddRow(1, index)
            self._list.LSetCell(self.itemrepr(item), (0, index))
        self.items.insert(index, item)

    def setdrawingmode(self, onoff):
        if onoff:
            self.drawingmode = self.drawingmode - 1
            if self.drawingmode == 0 and self._list is not None:
                self._list.LSetDrawingMode(1)
                if self._visible:
                    bounds = l, t, r, b = Qd.InsetRect(self._bounds, 1, 1)
                    cl, ct, cr, cb = self._list.LRect((0, len(self.items)-1))
                    if cb < b:
                        self.SetPort()
                        Qd.EraseRect((l, cb, cr, b))
                    self._list.LUpdate(self._parentwindow.wid.GetWindowPort().visRgn)
                    self.GetWindow().ValidWindowRect(bounds)
        else:
            if self.drawingmode == 0 and self._list is not None:
                self._list.LSetDrawingMode(0)
            self.drawingmode = self.drawingmode + 1


class CustomList(List):

    """Base class for writing custom list definitions."""

    _cellHeight = 0

    def createlist(self):
        self._calcbounds()
        self.SetPort()
        rect = self._bounds
        rect = rect[0]+1, rect[1]+1, rect[2]-16, rect[3]-1
        self._viewbounds = rect
        self._list = CreateCustomList(
                        rect,
                        (0, 0, 1, 0),
                        (0, self._cellHeight),
                        (kListDefUserProcType, self.listDefinitionFunc),
                        self._parentwindow.wid,
                        0, 1, 0, 1)
        if self.drawingmode:
            self._list.LSetDrawingMode(0)
        self._list.selFlags = self._flags
        self.setitems(self.items)
        if hasattr(self, "_sel"):
            self.setselection(self._sel)
            del self._sel

    def listDefinitionFunc(self, message, selected, cellRect, theCell,
                    dataOffset, dataLen, theList):
        """The LDEF message dispatcher routine, no need to override."""
        if message == lInitMsg:
            self.listDefInit(theList)
        elif message == lDrawMsg:
            self.listDefDraw(selected, cellRect, theCell,
                            dataOffset, dataLen, theList)
        elif message == lHiliteMsg:
            self.listDefHighlight(selected, cellRect, theCell,
                            dataOffset, dataLen, theList)
        elif message == lCloseMsg:
            self.listDefClose(theList)

    def listDefInit(self, theList):
        pass
    def listDefClose(self, theList):
        pass
    def listDefDraw(self, selected, cellRect, theCell,
                    dataOffset, dataLen, theList):
        pass
    def listDefHighlight(self, selected, cellRect, theCell,
                    dataOffset, dataLen, theList):
        pass


class TwoLineList(CustomList):

    _cellHeight = 28

    def listDefDraw(self, selected, cellRect, theCell,
                    dataOffset, dataLen, theList):
        savedPort = Qd.GetPort()
        Qd.SetPort(theList.GetListPort())
        savedClip = Qd.NewRgn()
        Qd.GetClip(savedClip)
        Qd.ClipRect(cellRect)
        savedPenState = Qd.GetPenState()
        Qd.PenNormal()
        Qd.EraseRect(cellRect)

        #draw the cell if it contains data
        ascent, descent, leading, size, hm = Fm.FontMetrics()
        linefeed = ascent + descent + leading

        if dataLen:
            left, top, right, bottom = cellRect
            data = theList.LGetCell(dataLen, theCell)
            lines = data.split("\r")
            line1 = lines[0]
            if len(lines) > 1:
                line2 = lines[1]
            else:
                line2 = ""
            Qd.MoveTo(int(left + 4), int(top + ascent))
            Qd.DrawText(line1, 0, len(line1))
            if line2:
                Qd.MoveTo(int(left + 4), int(top + ascent + linefeed))
                Qd.DrawText(line2, 0, len(line2))
            Qd.PenPat("\x11\x11\x11\x11\x11\x11\x11\x11")
            bottom = top + theList.cellSize[1]
            Qd.MoveTo(left, bottom - 1)
            Qd.LineTo(right, bottom - 1)
        if selected:
            self.listDefHighlight(selected, cellRect, theCell,
                            dataOffset, dataLen, theList)
        #restore graphics environment
        Qd.SetPort(savedPort)
        Qd.SetClip(savedClip)
        Qd.DisposeRgn(savedClip)
        Qd.SetPenState(savedPenState)

    def listDefHighlight(self, selected, cellRect, theCell,
                    dataOffset, dataLen, theList):
        savedPort = Qd.GetPort()
        Qd.SetPort(theList.GetListPort())
        savedClip = Qd.NewRgn()
        Qd.GetClip(savedClip)
        Qd.ClipRect(cellRect)
        savedPenState = Qd.GetPenState()
        Qd.PenNormal()
        Qd.PenMode(hilitetransfermode)
        Qd.PaintRect(cellRect)

        #restore graphics environment
        Qd.SetPort(savedPort)
        Qd.SetClip(savedClip)
        Qd.DisposeRgn(savedClip)
        Qd.SetPenState(savedPenState)


class ResultsWindow:

    """Simple results window. The user cannot make this window go away completely:
    closing it will just hide it. It will remain in the windows list. The owner of this window
    should call the done() method to indicate it's done with it.
    """

    def __init__(self, possize=None, title="Results", callback=None):
        import W
        if possize is None:
            possize = (500, 200)
        self.w = W.Window(possize, title, minsize=(200, 100))
        self.w.results = W.TwoLineList((-1, -1, 1, -14), callback=None)
        self.w.bind("<close>", self.hide)
        self.w.open()
        self._done = 0

    def done(self):
        self._done = 1
        if not self.w.isvisible():
            self.w.close()

    def hide(self):
        if not self._done:
            self.w.show(0)
            return -1

    def append(self, msg):
        if not self.w.isvisible():
            self.w.show(1)
            self.w.select()
        msg = string.replace(msg, '\n', '\r')
        self.w.results.append(msg)
        self.w.results.setselection([len(self.w.results)-1])

    def __getattr__(self, attr):
        return getattr(self.w.results, attr)


class MultiList(List):

    """XXX Experimantal!!!"""

    def setitems(self, items):
        self.items = items
        if not self._parent or not self._list:
            return
        self._list.LDelRow(0, 1)
        self.setdrawingmode(0)
        self._list.LAddRow(len(self.items), 0)
        self_itemrepr = self.itemrepr
        set_cell = self._list.LSetCell
        for i in range(len(items)):
            row = items[i]
            for j in range(len(row)):
                item = row[j]
                set_cell(self_itemrepr(item), (j, i))
        self.setdrawingmode(1)

    def getselection(self):
        if not self._parent or not self._list:
            if hasattr(self, "_sel"):
                return self._sel
            return []
        items = []
        point = (0,0)
        while 1:
            ok, point = self._list.LGetSelect(1, point)
            if not ok:
                break
            items.append(point[1])
            point = point[0], point[1]+1
        return items

    def setselection(self, selection):
        if not self._parent or not self._list:
            self._sel = selection
            return
        set_sel = self._list.LSetSelect
        for i in range(len(self.items)):
            for j in range(len(self.items[i])):
                if i in selection:
                    set_sel(1, (j, i))
                else:
                    set_sel(0, (j, i))
        #self._list.LAutoScroll()
