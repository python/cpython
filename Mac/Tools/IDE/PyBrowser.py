import W
import Wkeys
import struct
import string
import types
import re
from Carbon import Qd, Icn, Fm, QuickDraw
from Carbon.QuickDraw import hilitetransfermode


nullid = '\0\0'
closedid = struct.pack('h', 468)
openid = struct.pack('h', 469)
closedsolidid = struct.pack('h', 470)
opensolidid = struct.pack('h', 471)

arrows = (nullid, closedid, openid, closedsolidid, opensolidid)

has_ctlcharsRE = re.compile(r'[\000-\037\177-\377]')
def ctlcharsREsearch(str):
    if has_ctlcharsRE.search(str) is None:
        return -1
    return 1

def double_repr(key, value, truncvalue = 0,
                        type = type, StringType = types.StringType,
                        has_ctlchars = ctlcharsREsearch, _repr = repr, str = str):
    if type(key) == StringType and has_ctlchars(key) < 0:
        key = str(key)
    else:
        key = _repr(key)
    if key == '__builtins__':
        value = "<" + type(value).__name__ + " '__builtin__'>"
    elif key == '__return__':
        # bleh, when returning from a class codeblock we get infinite recursion in repr.
        # Use safe repr instead.
        import repr
        value = repr.repr(value)
    else:
        try:
            value = _repr(value)
            '' + value      # test to see if it is a string, in case a __repr__ method is buggy
        except:
            value = '\xa5\xa5\xa5 exception in repr()'
    if truncvalue:
        return key + '\t' + value[:255]
    return key + '\t' + value


def truncString(s, maxwid):
    if maxwid < 1:
        return 1, ""
    strlen = len(s)
    strwid = Qd.TextWidth(s, 0, strlen);
    if strwid <= maxwid:
        return 0, s

    Qd.TextFace(QuickDraw.condense)
    strwid = Qd.TextWidth(s, 0, strlen)
    ellipsis = Qd.StringWidth('\xc9')

    if strwid <= maxwid:
        Qd.TextFace(0)
        return 1, s
    if strwid < 1:
        Qd.TextFace(0)
        return 1, ""

    mid = int(strlen * maxwid / strwid)
    while 1:
        if mid <= 0:
            mid = 0
            break
        strwid = Qd.TextWidth(s, 0, mid) + ellipsis
        strwid2 = Qd.TextWidth(s, 0, mid + 1) + ellipsis
        if strwid <= maxwid and maxwid <= strwid2:
            if maxwid == strwid2:
                mid += 1
            break
        if strwid > maxwid:
            mid -= 1
            if mid <= 0:
                mid = 0
                break
        elif strwid2 < maxwid:
            mid += 1
    Qd.TextFace(0)
    return 1, s[:mid] + '\xc9'


def drawTextCell(text, cellRect, ascent, theList):
    l, t, r, b = cellRect
    cellwidth = r - l
    Qd.MoveTo(int(l + 2), int(t + ascent))
    condense, text = truncString(text, cellwidth - 3)
    if condense:
        Qd.TextFace(QuickDraw.condense)
    Qd.DrawText(text, 0, len(text))
    Qd.TextFace(0)


PICTWIDTH = 16


class BrowserWidget(W.CustomList):

    def __init__(self, possize, object = None, col = 100, closechildren = 0):
        W.List.__init__(self, possize, callback = self.listhit)
        self.object = (None,)
        self.indent = 16
        self.lastmaxindent = 0
        self.closechildren = closechildren
        self.children = []
        self.mincol = 64
        self.setcolumn(col)
        self.bind('return', self.openselection)
        self.bind('enter', self.openselection)
        if object is not None:
            self.set(object)

    def set(self, object):
        if self.object[0] is not object:
            self.object = object,
            self[:] = self.unpack(object, 0)
        elif self._parentwindow is not None and self._parentwindow.wid:
            self.update()

    def unpack(self, object, indent):
        return unpack_object(object, indent)

    def update(self):
        # for now...
        W.SetCursor('watch')
        self.setdrawingmode(0)
        sel = self.getselectedobjects()
        fold = self.getunfoldedobjects()
        topcell = self.gettopcell()
        self[:] = self.unpack(self.object[0], 0)
        self.unfoldobjects(fold)
        self.setselectedobjects(sel)
        self.settopcell(topcell)
        self.setdrawingmode(1)

    def setcolumn(self, col):
        self.col = col
        self.colstr = struct.pack('h', col)
        if self._list:
            sel = self.getselection()
            self.setitems(self.items)
            self.setselection(sel)

    def key(self, char, event):
        if char in (Wkeys.leftarrowkey, Wkeys.rightarrowkey):
            sel = self.getselection()
            sel.reverse()
            self.setdrawingmode(0)
            for index in sel:
                self.fold(index, char == Wkeys.rightarrowkey)
            self.setdrawingmode(1)
        else:
            W.List.key(self, char, event)

    def rollover(self, (x, y), onoff):
        if onoff:
            if self.incolumn((x, y)):
                W.SetCursor('hmover')
            else:
                W.SetCursor('arrow')

    def inarrow(self, (x, y)):
        cl, ct, cr, cb = self._list.LRect((0, 0))
        l, t, r, b = self._bounds
        if (x - cl) < 16:
            cellheight = cb - ct
            index = (y - ct) / cellheight
            if index < len(self.items):
                return 1, index
        return None, None

    def incolumn(self, (x, y)):
        l, t, r, b = self._list.LRect((0, 0))
        abscol = l + self.col
        return abs(abscol - x) < 3

    def trackcolumn(self, (x, y)):
        from Carbon import Qd, QuickDraw, Evt
        self.SetPort()
        l, t, r, b = self._bounds
        bounds = l, t, r, b = l + 1, t + 1, r - 16, b - 1
        abscol = l + self.col
        mincol = l + self.mincol
        maxcol = r - 10
        diff = abscol - x
        Qd.PenPat('\000\377\000\377\000\377\000\377')
        Qd.PenMode(QuickDraw.srcXor)
        rect = abscol - 1, t, abscol, b
        Qd.PaintRect(rect)
        lastpoint = (x, y)
        newcol = -1
        #W.SetCursor('fist')
        while Evt.Button():
            Evt.WaitNextEvent(0, 1, None)  # needed for OSX
            (x, y) = Evt.GetMouse()
            if (x, y) <> lastpoint:
                newcol = x + diff
                newcol = max(newcol, mincol)
                newcol = min(newcol, maxcol)
                Qd.PaintRect(rect)
                rect = newcol - 1, t, newcol, b
                Qd.PaintRect(rect)
                lastpoint = (x, y)
        Qd.PaintRect(rect)
        Qd.PenPat(Qd.GetQDGlobalsBlack())
        Qd.PenNormal()
        if newcol > 0 and newcol <> abscol:
            self.setcolumn(newcol - l)

    def click(self, point, modifiers):
        if point == (-1, -1):   # gross.
            W.List.click(self, point ,modifiers)
            return
        hit, index = self.inarrow(point)
        if hit:
            (key, value, arrow, indent) = self.items[index]
            self.fold(index, arrow == 1)
        elif self.incolumn(point):
            self.trackcolumn(point)
        else:
            W.List.click(self, point, modifiers)

    # for W.List.key
    def findmatch(self, tag):
        lower = string.lower
        items = self.items
        taglen = len(tag)
        match = '\377' * 100
        match_i = -1
        for i in range(len(items)):
            item = lower(str(items[i][0]))
            if tag <= item < match:
                match = item
                match_i = i
        if match_i >= 0:
            return match_i
        else:
            return len(items) - 1

    def close(self):
        if self.closechildren:
            for window in self.children:
                window.close()
        self.children = []
        W.List.close(self)

    def fold(self, index, onoff):
        (key, value, arrow, indent) = self.items[index]
        if arrow == 0 or (onoff and arrow == 2) or (not onoff and arrow == 1):
            return
        W.SetCursor('watch')
        topcell = self.gettopcell()
        if onoff:
            self[index] = (key, value, 4, indent)
            self.setdrawingmode(0)
            self[index+1:index+1] = self.unpack(value, indent + 1)
            self[index] = (key, value, 2, indent)
        else:
            self[index] = (key, value, 3, indent)
            self.setdrawingmode(0)
            count = 0
            for i in range(index + 1, len(self.items)):
                (dummy, dummy, dummy, subindent) = self.items[i]
                if subindent <= indent:
                    break
                count = count + 1
            self[index+1:index+1+count] = []
            self[index] = (key, value, 1, indent)
        maxindent = self.getmaxindent()
        if maxindent <> self.lastmaxindent:
            newabsindent = self.col + (maxindent - self.lastmaxindent) * self.indent
            if newabsindent >= self.mincol:
                self.setcolumn(newabsindent)
            self.lastmaxindent = maxindent
        self.settopcell(topcell)
        self.setdrawingmode(1)

    def unfoldobjects(self, objects):
        for obj in objects:
            try:
                index = self.items.index(obj)
            except ValueError:
                pass
            else:
                self.fold(index, 1)

    def getunfoldedobjects(self):
        curindent = 0
        objects = []
        for index in range(len(self.items)):
            (key, value, arrow, indent) = self.items[index]
            if indent > curindent:
                (k, v, a, i) = self.items[index - 1]
                objects.append((k, v, 1, i))
                curindent = indent
            elif indent < curindent:
                curindent = indent
        return objects

    def listhit(self, isdbl):
        if isdbl:
            self.openselection()

    def openselection(self):
        import os
        sel = self.getselection()
        for index in sel:
            (key, value, arrow, indent) = self[index]
            if arrow:
                self.children.append(Browser(value))
            elif type(value) == types.StringType and '\0' not in value:
                editor = self._parentwindow.parent.getscript(value)
                if editor:
                    editor.select()
                    return
                elif os.path.exists(value) and os.path.isfile(value):
                    if MacOS.GetCreatorAndType(value)[1] in ('TEXT', '\0\0\0\0'):
                        W.getapplication().openscript(value)

    def itemrepr(self, (key, value, arrow, indent), str = str, double_repr = double_repr,
                    arrows = arrows, pack = struct.pack):
        arrow = arrows[arrow]
        return arrow + pack('h', self.indent * indent) + self.colstr + \
                        double_repr(key, value, 1)

    def getmaxindent(self, max = max):
        maxindent = 0
        for item in self.items:
            maxindent = max(maxindent, item[3])
        return maxindent

    def domenu_copy(self, *args):
        sel = self.getselectedobjects()
        selitems = []
        for key, value, dummy, dummy in sel:
            selitems.append(double_repr(key, value))
        text = string.join(selitems, '\r')
        if text:
            from Carbon import Scrap
            if hasattr(Scrap, 'PutScrap'):
                Scrap.ZeroScrap()
                Scrap.PutScrap('TEXT', text)
            else:
                Scrap.ClearCurrentScrap()
                sc = Scrap.GetCurrentScrap()
                sc.PutScrapFlavor('TEXT', 0, text)

    def listDefDraw(self, selected, cellRect, theCell,
                    dataOffset, dataLen, theList):
        self.myDrawCell(0, selected, cellRect, theCell,
                dataOffset, dataLen, theList)

    def listDefHighlight(self, selected, cellRect, theCell,
                    dataOffset, dataLen, theList):
        self.myDrawCell(1, selected, cellRect, theCell,
                dataOffset, dataLen, theList)

    def myDrawCell(self, onlyHilite, selected, cellRect, theCell,
                    dataOffset, dataLen, theList):
        savedPort = Qd.GetPort()
        Qd.SetPort(theList.GetListPort())
        savedClip = Qd.NewRgn()
        Qd.GetClip(savedClip)
        Qd.ClipRect(cellRect)
        savedPenState = Qd.GetPenState()
        Qd.PenNormal()

        l, t, r, b = cellRect

        if not onlyHilite:
            Qd.EraseRect(cellRect)

            ascent, descent, leading, size, hm = Fm.FontMetrics()
            linefeed = ascent + descent + leading

            if dataLen >= 6:
                data = theList.LGetCell(dataLen, theCell)
                iconId, indent, tab = struct.unpack("hhh", data[:6])
                try:
                    key, value = data[6:].split("\t", 1)
                except ValueError:
                    # bogus data, at least don't crash.
                    indent = 0
                    tab = 0
                    iconId = 0
                    key = ""
                    value = data[6:]

                if iconId:
                    try:
                        theIcon = Icn.GetCIcon(iconId)
                    except Icn.Error:
                        pass
                    else:
                        rect = (0, 0, 16, 16)
                        rect = Qd.OffsetRect(rect, l, t)
                        rect = Qd.OffsetRect(rect, 0, (theList.cellSize[1] - (rect[3] - rect[1])) / 2)
                        Icn.PlotCIcon(rect, theIcon)

                if len(key) >= 0:
                    cl, ct, cr, cb = cellRect
                    vl, vt, vr, vb = self._viewbounds
                    cl = vl + PICTWIDTH + indent
                    cr = vl + tab
                    if cr > vr:
                        cr = vr
                    if cl < cr:
                        drawTextCell(key, (cl, ct, cr, cb), ascent, theList)
                    cl = vl + tab
                    cr = vr
                    if cl < cr:
                        drawTextCell(value, (cl, ct, cr, cb), ascent, theList)
            #elif dataLen != 0:
            #       drawTextCell("???", 3, cellRect, ascent, theList)
            else:
                return  # we have bogus data

            # draw nice dotted line
            l, t, r, b = cellRect
            l = self._viewbounds[0] + tab
            r = l + 1;
            if not (theList.cellSize[1] & 0x01) or (t & 0x01):
                myPat = "\xff\x00\xff\x00\xff\x00\xff\x00"
            else:
                myPat = "\x00\xff\x00\xff\x00\xff\x00\xff"
            Qd.PenPat(myPat)
            Qd.PenMode(QuickDraw.srcCopy)
            Qd.PaintRect((l, t, r, b))
            Qd.PenNormal()

        if selected or onlyHilite:
            l, t, r, b = cellRect
            l = self._viewbounds[0] + PICTWIDTH
            r = self._viewbounds[2]
            Qd.PenMode(hilitetransfermode)
            Qd.PaintRect((l, t, r, b))

        # restore graphics environment
        Qd.SetPort(savedPort)
        Qd.SetClip(savedClip)
        Qd.DisposeRgn(savedClip)
        Qd.SetPenState(savedPenState)



class Browser:

    def __init__(self, object = None, title = None, closechildren = 0):
        if hasattr(object, '__name__'):
            name = object.__name__
        else:
            name = ''
        if title is None:
            title = 'Object browser'
            if name:
                title = title + ': ' + name
        self.w = w = W.Window((300, 400), title, minsize = (100, 100))
        w.info = W.TextBox((18, 8, -70, 15))
        w.updatebutton = W.BevelButton((-64, 4, 50, 16), 'Update', self.update)
        w.browser = BrowserWidget((-1, 24, 1, -14), None)
        w.bind('cmdu', w.updatebutton.push)
        w.open()
        self.set(object, name)

    def close(self):
        if self.w.wid:
            self.w.close()

    def set(self, object, name = ''):
        W.SetCursor('watch')
        tp = type(object).__name__
        try:
            length = len(object)
        except:
            length = -1
        if not name and hasattr(object, '__name__'):
            name = object.__name__
        if name:
            info = name + ': ' + tp
        else:
            info = tp
        if length >= 0:
            if length == 1:
                info = info + ' (%d element)' % length
            else:
                info = info + ' (%d elements)' % length
        self.w.info.set(info)
        self.w.browser.set(object)

    def update(self):
        self.w.browser.update()


SIMPLE_TYPES = (
        type(None),
        int,
        long,
        float,
        complex,
        str,
        unicode,
)

def get_ivars(obj):
    """Return a list the names of all (potential) instance variables."""
    # __mro__ recipe from Guido
    slots = {}
    # old-style C objects
    if hasattr(obj, "__members__"):
        for name in obj.__members__:
            slots[name] = None
    if hasattr(obj, "__methods__"):
        for name in obj.__methods__:
            slots[name] = None
    # generic type
    if hasattr(obj, "__dict__"):
        slots.update(obj.__dict__)
    cls = type(obj)
    if hasattr(cls, "__mro__"):
        # new-style class, use descriptors
        for base in cls.__mro__:
            for name, value in base.__dict__.items():
                # XXX using callable() is a heuristic which isn't 100%
                # foolproof.
                if hasattr(value, "__get__") and not callable(value):
                    slots[name] = None
    if "__dict__" in slots:
        del slots["__dict__"]
    slots = slots.keys()
    slots.sort()
    return slots

def unpack_object(object, indent = 0):
    tp = type(object)
    if isinstance(object, SIMPLE_TYPES) and object is not None:
        raise TypeError, "can't browse simple type: %s" % tp.__name__
    elif isinstance(object, dict):
        return unpack_dict(object, indent)
    elif isinstance(object, (tuple, list)):
        return unpack_sequence(object, indent)
    elif isinstance(object, types.ModuleType):
        return unpack_dict(object.__dict__, indent)
    else:
        return unpack_other(object, indent)

def unpack_sequence(seq, indent = 0):
    return [(i, v, not isinstance(v, SIMPLE_TYPES), indent)
             for i, v in enumerate(seq)]

def unpack_dict(dict, indent = 0):
    items = dict.items()
    return pack_items(items, indent)

def unpack_instance(inst, indent = 0):
    if hasattr(inst, '__pybrowse_unpack__'):
        return unpack_object(inst.__pybrowse_unpack__(), indent)
    else:
        items = [('__class__', inst.__class__)] + inst.__dict__.items()
        return pack_items(items, indent)

def unpack_class(clss, indent = 0):
    items = [('__bases__', clss.__bases__), ('__name__', clss.__name__)] + clss.__dict__.items()
    return pack_items(items, indent)

def unpack_other(object, indent = 0):
    attrs = get_ivars(object)
    items = []
    for attr in attrs:
        try:
            value = getattr(object, attr)
        except:
            pass
        else:
            items.append((attr, value))
    return pack_items(items, indent)

def pack_items(items, indent = 0):
    items = [(k, v, not isinstance(v, SIMPLE_TYPES), indent)
             for k, v in items]
    return tuple_caselesssort(items)

def caselesssort(alist):
    """Return a sorted copy of a list. If there are only strings in the list,
    it will not consider case"""

    try:
        # turn ['FOO',  'aaBc', 'ABcD'] into [('foo', 'FOO'), ('aabc', 'aaBc'), ('abcd', 'ABcD')], if possible
        tupledlist = map(lambda item, lower = string.lower: (lower(item), item), alist)
    except TypeError:
        # at least one element in alist is not a string, proceed the normal way...
        alist = alist[:]
        alist.sort()
        return alist
    else:
        tupledlist.sort()
        # turn [('aabc', 'aaBc'), ('abcd', 'ABcD'), ('foo', 'FOO')] into ['aaBc', 'ABcD', 'FOO']
        return map(lambda x: x[1], tupledlist)

def tuple_caselesssort(items):
    try:
        tupledlist = map(lambda tuple, lower = string.lower: (lower(tuple[0]), tuple), items)
    except (AttributeError, TypeError):
        items = items[:]
        items.sort()
        return items
    else:
        tupledlist.sort()
        return map(lambda (low, tuple): tuple, tupledlist)
