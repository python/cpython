from Carbon import Evt, Events, Fm, Fonts
from Carbon import Qd, Res, Scrap
from Carbon import TE, TextEdit, Win
from Carbon import App
from Carbon.Appearance import kThemeStateActive, kThemeStateInactive
import waste
import WASTEconst
import Wbase
import Wkeys
import Wcontrols
import PyFontify
import string
from types import TupleType, StringType


class TextBox(Wbase.Widget):

    """A static text widget"""

    def __init__(self, possize, text="", align=TextEdit.teJustLeft,
                            fontsettings=None,
                            backgroundcolor=(0xffff, 0xffff, 0xffff)
                            ):
        if fontsettings is None:
            import W
            fontsettings = W.getdefaultfont()
        Wbase.Widget.__init__(self, possize)
        self.fontsettings = fontsettings
        self.text = text
        self.align = align
        self._backgroundcolor = backgroundcolor

    def draw(self, visRgn = None):
        if self._visible:
            (font, style, size, color) = self.fontsettings
            fontid = GetFNum(font)
            savestate = Qd.GetPenState()
            Qd.TextFont(fontid)
            Qd.TextFace(style)
            Qd.TextSize(size)
            Qd.RGBForeColor(color)
            Qd.RGBBackColor(self._backgroundcolor)
            TE.TETextBox(self.text, self._bounds, self.align)
            Qd.RGBBackColor((0xffff, 0xffff, 0xffff))
            Qd.SetPenState(savestate)

    def get(self):
        return self.text

    def set(self, text):
        self.text = text
        if self._parentwindow and self._parentwindow.wid:
            self.SetPort()
            self.draw()


class _ScrollWidget:

    # to be overridden
    def getscrollrects(self):
        """Return (destrect, viewrect)."""
        return None, None

    # internal method

    def updatescrollbars(self):
        (dl, dt, dr, db), (vl, vt, vr, vb) = self.getscrollrects()
        if self._parent._barx:
            viewwidth = vr - vl
            destwidth = dr - dl
            bar = self._parent._barx
            bar.setmax(destwidth - viewwidth)

            # MacOS 8.1 doesn't automatically disable
            # scrollbars whose max <= min
            bar.enable(destwidth > viewwidth)

            bar.setviewsize(viewwidth)
            bar.set(vl - dl)
        if self._parent._bary:
            viewheight = vb - vt
            destheight = db - dt
            bar = self._parent._bary
            bar.setmax(destheight - viewheight)

            # MacOS 8.1 doesn't automatically disable
            # scrollbars whose max <= min
            bar.enable(destheight > viewheight)

            bar.setviewsize(viewheight)
            bar.set(vt - dt)


UNDOLABELS = [  # Indexed by WEGetUndoInfo() value
        None, "", "typing", "Cut", "Paste", "Clear", "Drag", "Style",
        "Ruler", "backspace", "delete", "transform", "resize"]


class EditText(Wbase.SelectableWidget, _ScrollWidget):

    """A text edit widget, mainly for simple entry fields."""

    def __init__(self, possize, text="",
                            callback=None, inset=(3, 3),
                            fontsettings=None,
                            tabsettings = (32, 0),
                            readonly = 0):
        if fontsettings is None:
            import W
            fontsettings = W.getdefaultfont()
        Wbase.SelectableWidget.__init__(self, possize)
        self.temptext = text
        self.ted = None
        self.selection = None
        self.oldselection = None
        self._callback = callback
        self.changed = 0
        self.selchanged = 0
        self._selected = 0
        self._enabled = 1
        self.wrap = 1
        self.readonly = readonly
        self.fontsettings = fontsettings
        self.tabsettings = tabsettings
        if type(inset) <> TupleType:
            self.inset = (inset, inset)
        else:
            self.inset = inset

    def open(self):
        if not hasattr(self._parent, "_barx"):
            self._parent._barx = None
        if not hasattr(self._parent, "_bary"):
            self._parent._bary = None
        self._calcbounds()
        self.SetPort()
        viewrect, destrect = self._calctextbounds()
        flags = self._getflags()
        self.ted = waste.WENew(destrect, viewrect, flags)
        self.ted.WEInstallTabHooks()
        self.ted.WESetAlignment(WASTEconst.weFlushLeft)
        self.setfontsettings(self.fontsettings)
        self.settabsettings(self.tabsettings)
        self.ted.WEUseText(Res.Resource(self.temptext))
        self.ted.WECalText()
        if self.selection:
            self.setselection(self.selection[0], self.selection[1])
            self.selection = None
        else:
            self.selview()
        self.temptext = None
        self.updatescrollbars()
        self.bind("pageup", self.scrollpageup)
        self.bind("pagedown", self.scrollpagedown)
        self.bind("top", self.scrolltop)
        self.bind("bottom", self.scrollbottom)
        self.selchanged = 0

    def close(self):
        self._parent._barx = None
        self._parent._bary = None
        self.ted = None
        self.temptext = None
        Wbase.SelectableWidget.close(self)

    def textchanged(self, all=0):
        self.changed = 1

    def selectionchanged(self):
        self.selchanged = 1
        self.oldselection = self.getselection()

    def gettabsettings(self):
        return self.tabsettings

    def settabsettings(self, (tabsize, tabmode)):
        self.tabsettings = (tabsize, tabmode)
        if hasattr(self.ted, "WESetTabSize"):
            port = self._parentwindow.wid.GetWindowPort()
            if tabmode:
                (font, style, size, color) = self.getfontsettings()
                savesettings = GetPortFontSettings(port)
                SetPortFontSettings(port, (font, style, size))
                tabsize = Qd.StringWidth(' ' * tabsize)
                SetPortFontSettings(port, savesettings)
            tabsize = max(tabsize, 1)
            self.ted.WESetTabSize(tabsize)
            self.SetPort()
            Qd.EraseRect(self.ted.WEGetViewRect())
            self.ted.WEUpdate(port.visRgn)

    def getfontsettings(self):
        from Carbon import Res
        (font, style, size, color) = self.ted.WEGetRunInfo(0)[4]
        font = Fm.GetFontName(font)
        return (font, style, size, color)

    def setfontsettings(self, (font, style, size, color)):
        self.SetPort()
        if type(font) <> StringType:
            font = Fm.GetFontName(font)
        self.fontsettings = (font, style, size, color)
        fontid = GetFNum(font)
        readonly = self.ted.WEFeatureFlag(WASTEconst.weFReadOnly, -1)
        if readonly:
            self.ted.WEFeatureFlag(WASTEconst.weFReadOnly, 0)
        try:
            self.ted.WEFeatureFlag(WASTEconst.weFInhibitRecal, 1)
            selstart, selend = self.ted.WEGetSelection()
            self.ted.WESetSelection(0, self.ted.WEGetTextLength())
            self.ted.WESetStyle(WASTEconst.weDoFace, (0, 0, 0, (0, 0, 0)))
            self.ted.WESetStyle(WASTEconst.weDoFace |
                                    WASTEconst.weDoColor |
                                    WASTEconst.weDoFont |
                                    WASTEconst.weDoSize,
                                    (fontid, style, size, color))
            self.ted.WEFeatureFlag(WASTEconst.weFInhibitRecal, 0)
            self.ted.WECalText()
            self.ted.WESetSelection(selstart, selend)
        finally:
            if readonly:
                self.ted.WEFeatureFlag(WASTEconst.weFReadOnly, 1)
        viewrect = self.ted.WEGetViewRect()
        Qd.EraseRect(viewrect)
        self.ted.WEUpdate(self._parentwindow.wid.GetWindowPort().visRgn)
        self.selectionchanged()
        self.updatescrollbars()

    def adjust(self, oldbounds):
        self.SetPort()
        # Note: if App.DrawThemeEditTextFrame is ever used, it will be necessary
        # to unconditionally outset the invalidated rectangles, since Appearance
        # frames are drawn outside the bounds.
        if self._selected and self._parentwindow._hasselframes:
            self.GetWindow().InvalWindowRect(Qd.InsetRect(oldbounds, -3, -3))
            self.GetWindow().InvalWindowRect(Qd.InsetRect(self._bounds, -3, -3))
        else:
            self.GetWindow().InvalWindowRect(oldbounds)
            self.GetWindow().InvalWindowRect(self._bounds)
        viewrect, destrect = self._calctextbounds()
        self.ted.WESetViewRect(viewrect)
        self.ted.WESetDestRect(destrect)
        if self.wrap:
            self.ted.WECalText()
        if self.ted.WEGetDestRect()[3] < viewrect[1]:
            self.selview()
        self.updatescrollbars()

    # interface -----------------------
    # selection stuff
    def selview(self):
        self.ted.WESelView()

    def selectall(self):
        self.ted.WESetSelection(0, self.ted.WEGetTextLength())
        self.selectionchanged()
        self.updatescrollbars()

    def selectline(self, lineno, charoffset = 0):
        newselstart, newselend = self.ted.WEGetLineRange(lineno)
        # Autoscroll makes the *end* of the selection visible, which,
        # in the case of a whole line, is the beginning of the *next* line.
        # So sometimes it leaves our line just above the view rect.
        # Let's fool Waste by initially selecting one char less:
        self.ted.WESetSelection(newselstart + charoffset, newselend-1)
        self.ted.WESetSelection(newselstart + charoffset, newselend)
        self.selectionchanged()
        self.updatescrollbars()

    def getselection(self):
        if self.ted:
            return self.ted.WEGetSelection()
        else:
            return self.selection

    def setselection(self, selstart, selend):
        self.selectionchanged()
        if self.ted:
            self.ted.WESetSelection(selstart, selend)
            self.ted.WESelView()
            self.updatescrollbars()
        else:
            self.selection = selstart, selend

    def offsettoline(self, offset):
        return self.ted.WEOffsetToLine(offset)

    def countlines(self):
        return self.ted.WECountLines()

    def getselectedtext(self):
        selstart, selend = self.ted.WEGetSelection()
        return self.ted.WEGetText().data[selstart:selend]

    def expandselection(self):
        oldselstart, oldselend = self.ted.WEGetSelection()
        selstart, selend = min(oldselstart, oldselend), max(oldselstart, oldselend)
        if selstart <> selend and chr(self.ted.WEGetChar(selend-1)) == '\r':
            selend = selend - 1
        newselstart, dummy = self.ted.WEFindLine(selstart, 1)
        dummy, newselend = self.ted.WEFindLine(selend, 1)
        if oldselstart <> newselstart or  oldselend <> newselend:
            self.ted.WESetSelection(newselstart, newselend)
            self.updatescrollbars()
        self.selectionchanged()

    def insert(self, text):
        self.ted.WEInsert(text, None, None)
        self.textchanged()
        self.selectionchanged()

    # text
    def set(self, text):
        if not self.ted:
            self.temptext = text
        else:
            self.ted.WEUseText(Res.Resource(text))
            self.ted.WECalText()
            self.SetPort()
            viewrect, destrect = self._calctextbounds()
            self.ted.WESetViewRect(viewrect)
            self.ted.WESetDestRect(destrect)
            rgn = Qd.NewRgn()
            Qd.RectRgn(rgn, viewrect)
            Qd.EraseRect(viewrect)
            self.draw(rgn)
            self.updatescrollbars()
            self.textchanged(1)

    def get(self):
        if not self._parent:
            return self.temptext
        else:
            return self.ted.WEGetText().data

    # events
    def key(self, char, event):
        (what, message, when, where, modifiers) = event
        if self._enabled and not modifiers & Events.cmdKey or char in Wkeys.arrowkeys:
            self.ted.WEKey(ord(char), modifiers)
            if char not in Wkeys.navigationkeys:
                self.textchanged()
            if char not in Wkeys.scrollkeys:
                self.selectionchanged()
            self.updatescrollbars()
            if self._callback:
                Wbase.CallbackCall(self._callback, 0, char, modifiers)

    def click(self, point, modifiers):
        if not self._enabled:
            return
        self.ted.WEClick(point, modifiers, Evt.TickCount())
        self.selectionchanged()
        self.updatescrollbars()
        return 1

    def idle(self):
        self.SetPort()
        self.ted.WEIdle()

    def rollover(self, point, onoff):
        if onoff:
            Wbase.SetCursor("iBeam")

    def activate(self, onoff):
        self._activated = onoff
        if self._visible:
            self.SetPort()

            # DISABLED!  There are too many places where it is assumed that
            # the frame of an EditText item is 1 pixel, inside the bounds.
            #state = [kThemeStateActive, kThemeStateInactive][not onoff]
            #App.DrawThemeEditTextFrame(Qd.InsetRect(self._bounds, 1, 1), state)

            if self._selected:
                if onoff:
                    self.ted.WEActivate()
                else:
                    self.ted.WEDeactivate()
                self.drawselframe(onoff)

    def select(self, onoff, isclick = 0):
        if Wbase.SelectableWidget.select(self, onoff):
            return
        self.SetPort()
        if onoff:
            self.ted.WEActivate()
            if self._parentwindow._tabbable and not isclick:
                self.selectall()
        else:
            self.ted.WEDeactivate()
        self.drawselframe(onoff)

    def draw(self, visRgn = None):
        if self._visible:
            if not visRgn:
                visRgn = self._parentwindow.wid.GetWindowPort().visRgn
            self.ted.WEUpdate(visRgn)

            # DISABLED!  There are too many places where it is assumed that
            # the frame of an EditText item is 1 pixel, inside the bounds.
            #state = [kThemeStateActive, kThemeStateInactive][not self._activated]
            #App.DrawThemeEditTextFrame(Qd.InsetRect(self._bounds, 1, 1), state)
            Qd.FrameRect(self._bounds)

            if self._selected and self._activated:
                self.drawselframe(1)

    # scrolling
    def scrollpageup(self):
        if self._parent._bary and self._parent._bary._enabled:
            self.vscroll("++")

    def scrollpagedown(self):
        if self._parent._bary and self._parent._bary._enabled:
            self.vscroll("--")

    def scrolltop(self):
        if self._parent._bary and self._parent._bary._enabled:
            self.vscroll(self._parent._bary.getmin())
        if self._parent._barx and self._parent._barx._enabled:
            self.hscroll(self._parent._barx.getmin())

    def scrollbottom(self):
        if self._parent._bary and self._parent._bary._enabled:
            self.vscroll(self._parent._bary.getmax())

    # menu handlers
    def domenu_copy(self, *args):
        selbegin, selend = self.ted.WEGetSelection()
        if selbegin == selend:
            return
        if hasattr(Scrap, 'ZeroScrap'):
            Scrap.ZeroScrap()
        else:
            Scrap.ClearCurrentScrap()
        self.ted.WECopy()
        self.updatescrollbars()

    def domenu_cut(self, *args):
        selbegin, selend = self.ted.WEGetSelection()
        if selbegin == selend:
            return
        if hasattr(Scrap, 'ZeroScrap'):
            Scrap.ZeroScrap()
        else:
            Scrap.ClearCurrentScrap()
        self.ted.WECut()
        self.updatescrollbars()
        self.selview()
        self.textchanged()
        self.selectionchanged()
        if self._callback:
            Wbase.CallbackCall(self._callback, 0, "", None)

    def domenu_paste(self, *args):
        if not self.ted.WECanPaste():
            return
        self.selview()
        self.ted.WEPaste()
        self.updatescrollbars()
        self.textchanged()
        self.selectionchanged()
        if self._callback:
            Wbase.CallbackCall(self._callback, 0, "", None)

    def domenu_clear(self, *args):
        self.ted.WEDelete()
        self.selview()
        self.updatescrollbars()
        self.textchanged()
        self.selectionchanged()
        if self._callback:
            Wbase.CallbackCall(self._callback, 0, "", None)

    def domenu_undo(self, *args):
        which, redo = self.ted.WEGetUndoInfo()
        if not which:
            return
        self.ted.WEUndo()
        self.updatescrollbars()
        self.textchanged()
        self.selectionchanged()
        if self._callback:
            Wbase.CallbackCall(self._callback, 0, "", None)

    def can_undo(self, menuitem):
        #doundo = self.ted.WEFeatureFlag(WASTEconst.weFUndo, -1)
        #print doundo
        #if not doundo:
        #       return 0
        which, redo = self.ted.WEGetUndoInfo()
        if which < len(UNDOLABELS):
            which = UNDOLABELS[which]
        else:
            which = ""
        if which == None:
            return None
        if redo:
            which = "Redo "+which
        else:
            which = "Undo "+which
        menuitem.settext(which)
        return 1

    def domenu_selectall(self, *args):
        self.selectall()

    # private
    def getscrollrects(self):
        return self.ted.WEGetDestRect(), self.ted.WEGetViewRect()

    def vscroll(self, value):
        lineheight = self.ted.WEGetHeight(0, 1)
        dr = self.ted.WEGetDestRect()
        vr = self.ted.WEGetViewRect()
        viewheight = vr[3] - vr[1]
        maxdelta = vr[1] - dr[1]
        mindelta = vr[3] - dr[3]
        if value == "+":
            delta = lineheight
        elif value == "-":
            delta = - lineheight
        elif value == "++":
            delta = viewheight - lineheight
        elif value == "--":
            delta = lineheight - viewheight
        else:   # in thumb
            delta = vr[1] - dr[1] - value
        delta = min(maxdelta, delta)
        delta = max(mindelta, delta)
        delta = int(delta)
        self.ted.WEScroll(0, delta)
        self.updatescrollbars()

    def hscroll(self, value):
        dr = self.ted.WEGetDestRect()
        vr = self.ted.WEGetViewRect()
        destwidth = dr[2] - dr[0]
        viewwidth = vr[2] - vr[0]
        viewoffset = maxdelta = vr[0] - dr[0]
        mindelta = vr[2] - dr[2]
        if value == "+":
            delta = 32
        elif value == "-":
            delta = - 32
        elif value == "++":
            delta = 0.5 * (vr[2] - vr[0])
        elif value == "--":
            delta = 0.5 * (vr[0] - vr[2])
        else:   # in thumb
            delta = vr[0] - dr[0] - value
            #cur = (32767 * viewoffset) / (destwidth - viewwidth)
            #delta = (cur-value)*(destwidth - viewwidth)/32767
            #if abs(delta - viewoffset) <=2:
            #       # compensate for irritating rounding error
            #       delta = viewoffset
        delta = min(maxdelta, delta)
        delta = max(mindelta, delta)
        delta = int(delta)
        self.ted.WEScroll(delta, 0)
        self.updatescrollbars()

    # some internals
    def _getflags(self):
        flags = WASTEconst.weDoAutoScroll | WASTEconst.weDoMonoStyled
        if self.readonly:
            flags = flags | WASTEconst.weDoReadOnly
        else:
            flags = flags | WASTEconst.weDoUndo
        return flags

    def _getviewrect(self):
        return Qd.InsetRect(self._bounds, self.inset[0], self.inset[1])

    def _calctextbounds(self):
        viewrect = l, t, r, b = self._getviewrect()
        if self.ted:
            dl, dt, dr, db = self.ted.WEGetDestRect()
            vl, vt, vr, vb = self.ted.WEGetViewRect()
            yshift = t - vt
            if (db - dt) < (b - t):
                destrect = viewrect
            else:
                destrect = l, dt + yshift, r, db + yshift
        else:
            destrect = viewrect
        return viewrect, destrect


class TextEditor(EditText):

    """A text edit widget."""

    def __init__(self, possize, text="", callback=None, wrap=1, inset=(4, 4),
                            fontsettings=None,
                            tabsettings=(32, 0),
                            readonly=0):
        EditText.__init__(self, possize, text, callback, inset, fontsettings, tabsettings, readonly)
        self.wrap = wrap

    def _getflags(self):
        flags = WASTEconst.weDoAutoScroll | WASTEconst.weDoMonoStyled | \
                        WASTEconst.weDoOutlineHilite
        if self.readonly:
            flags = flags | WASTEconst.weDoReadOnly
        else:
            flags = flags | WASTEconst.weDoUndo
        return flags

    def _getviewrect(self):
        l, t, r, b = self._bounds
        return (l + 5, t + 2, r, b - 2)

    def _calctextbounds(self):
        if self.wrap:
            return EditText._calctextbounds(self)
        else:
            viewrect = l, t, r, b = self._getviewrect()
            if self.ted:
                dl, dt, dr, db = self.ted.WEGetDestRect()
                vl, vt, vr, vb = self.ted.WEGetViewRect()
                xshift = l - vl
                yshift = t - vt
                if (db - dt) < (b - t):
                    yshift = t - dt
                destrect = (dl + xshift, dt + yshift, dr + xshift, db + yshift)
            else:
                destrect = (l, t, r + 5000, b)
            return viewrect, destrect

    def draw(self, visRgn = None):
        if self._visible:
            if not visRgn:
                visRgn = self._parentwindow.wid.GetWindowPort().visRgn
            self.ted.WEUpdate(visRgn)
            if self._selected and self._activated:
                self.drawselframe(1)

    def activate(self, onoff):
        self._activated = onoff
        if self._visible:
            self.SetPort()
            # doesn't draw frame, as EditText.activate does
            if self._selected:
                if onoff:
                    self.ted.WEActivate()
                else:
                    self.ted.WEDeactivate()
                self.drawselframe(onoff)


import re
commentPat = re.compile("[ \t]*(#)")
indentPat = re.compile("[ \t]*")
kStringColor = (0, 0x7fff, 0)
kCommentColor = (0, 0, 0xb000)


class PyEditor(TextEditor):

    """A specialized Python source edit widget"""

    def __init__(self, possize, text="", callback=None, inset=(4, 4),
                            fontsettings=None,
                            tabsettings=(32, 0),
                            readonly=0,
                            debugger=None,
                            file=''):
        TextEditor.__init__(self, possize, text, callback, 0, inset, fontsettings, tabsettings, readonly)
        self.bind("cmd[", self.domenu_shiftleft)
        self.bind("cmd]", self.domenu_shiftright)
        self.bind("cmdshift[", self.domenu_uncomment)
        self.bind("cmdshift]", self.domenu_comment)
        self.bind("cmdshiftd", self.alldirty)
        self.file = file        # only for debugger reference
        self._debugger = debugger
        if debugger:
            debugger.register_editor(self, self.file)
        self._dirty = (0, None)
        self.do_fontify = 0

    #def open(self):
    #       TextEditor.open(self)
    #       if self.do_fontify:
    #               self.fontify()
    #       self._dirty = (None, None)

    def _getflags(self):
        flags = (WASTEconst.weDoDrawOffscreen | WASTEconst.weDoUseTempMem |
                        WASTEconst.weDoAutoScroll | WASTEconst.weDoOutlineHilite)
        if self.readonly:
            flags = flags | WASTEconst.weDoReadOnly
        else:
            flags = flags | WASTEconst.weDoUndo
        return flags

    def textchanged(self, all=0):
        self.changed = 1
        if all:
            self._dirty = (0, None)
            return
        oldsel = self.oldselection
        sel = self.getselection()
        if not sel:
            # XXX what to do?
            return
        selstart, selend = sel
        selstart, selend = min(selstart, selend), max(selstart, selend)
        if oldsel:
            oldselstart, oldselend = min(oldsel), max(oldsel)
            selstart, selend = min(selstart, oldselstart), max(selend, oldselend)
        startline = self.offsettoline(selstart)
        endline = self.offsettoline(selend)
        selstart, _ = self.ted.WEGetLineRange(startline)
        _, selend = self.ted.WEGetLineRange(endline)
        if selstart > 0:
            selstart = selstart - 1
        self._dirty = (selstart, selend)

    def idle(self):
        self.SetPort()
        self.ted.WEIdle()
        if not self.do_fontify:
            return
        start, end = self._dirty
        if start is None:
            return
        textLength = self.ted.WEGetTextLength()
        if end is None:
            end = textLength
        if start >= end:
            self._dirty = (None, None)
        else:
            self.fontify(start, end)
            self._dirty = (None, None)

    def alldirty(self, *args):
        self._dirty = (0, None)

    def fontify(self, start=0, end=None):
        #W.SetCursor('watch')
        if self.readonly:
            self.ted.WEFeatureFlag(WASTEconst.weFReadOnly, 0)
        self.ted.WEFeatureFlag(WASTEconst.weFOutlineHilite, 0)
        self.ted.WEDeactivate()
        self.ted.WEFeatureFlag(WASTEconst.weFAutoScroll, 0)
        self.ted.WEFeatureFlag(WASTEconst.weFUndo, 0)
        pytext = self.get().replace("\r", "\n")
        if end is None:
            end = len(pytext)
        else:
            end = min(end, len(pytext))
        selstart, selend = self.ted.WEGetSelection()
        self.ted.WESetSelection(start, end)
        self.ted.WESetStyle(WASTEconst.weDoFace | WASTEconst.weDoColor,
                        (0, 0, 12, (0, 0, 0)))

        tags = PyFontify.fontify(pytext, start, end)
        styles = {
                'string': (WASTEconst.weDoColor, (0, 0, 0, kStringColor)),
                'keyword': (WASTEconst.weDoFace, (0, 1, 0, (0, 0, 0))),
                'comment': (WASTEconst.weDoFace | WASTEconst.weDoColor, (0, 0, 0, kCommentColor)),
                'identifier': (WASTEconst.weDoColor, (0, 0, 0, (0xbfff, 0, 0)))
        }
        setselection = self.ted.WESetSelection
        setstyle = self.ted.WESetStyle
        for tag, start, end, sublist in tags:
            setselection(start, end)
            mode, style = styles[tag]
            setstyle(mode, style)
        self.ted.WESetSelection(selstart, selend)
        self.SetPort()
        self.ted.WEFeatureFlag(WASTEconst.weFAutoScroll, 1)
        self.ted.WEFeatureFlag(WASTEconst.weFUndo, 1)
        self.ted.WEActivate()
        self.ted.WEFeatureFlag(WASTEconst.weFOutlineHilite, 1)
        if self.readonly:
            self.ted.WEFeatureFlag(WASTEconst.weFReadOnly, 1)

    def domenu_shiftleft(self):
        self.expandselection()
        selstart, selend = self.ted.WEGetSelection()
        selstart, selend = min(selstart, selend), max(selstart, selend)
        snippet = self.getselectedtext()
        lines = string.split(snippet, '\r')
        for i in range(len(lines)):
            if lines[i][:1] == '\t':
                lines[i] = lines[i][1:]
        snippet = string.join(lines, '\r')
        self.insert(snippet)
        self.ted.WESetSelection(selstart, selstart + len(snippet))

    def domenu_shiftright(self):
        self.expandselection()
        selstart, selend = self.ted.WEGetSelection()
        selstart, selend = min(selstart, selend), max(selstart, selend)
        snippet = self.getselectedtext()
        lines = string.split(snippet, '\r')
        for i in range(len(lines) - (not lines[-1])):
            lines[i] = '\t' + lines[i]
        snippet = string.join(lines, '\r')
        self.insert(snippet)
        self.ted.WESetSelection(selstart, selstart + len(snippet))

    def domenu_uncomment(self):
        self.expandselection()
        selstart, selend = self.ted.WEGetSelection()
        selstart, selend = min(selstart, selend), max(selstart, selend)
        snippet = self.getselectedtext()
        lines = string.split(snippet, '\r')
        for i in range(len(lines)):
            m = commentPat.match(lines[i])
            if m:
                pos = m.start(1)
                lines[i] = lines[i][:pos] + lines[i][pos+1:]
        snippet = string.join(lines, '\r')
        self.insert(snippet)
        self.ted.WESetSelection(selstart, selstart + len(snippet))

    def domenu_comment(self):
        self.expandselection()
        selstart, selend = self.ted.WEGetSelection()
        selstart, selend = min(selstart, selend), max(selstart, selend)
        snippet = self.getselectedtext()
        lines = string.split(snippet, '\r')
        indent = 3000 # arbitrary large number...
        for line in lines:
            if string.strip(line):
                m = indentPat.match(line)
                if m:
                    indent = min(indent, m.regs[0][1])
                else:
                    indent = 0
                    break
        for i in range(len(lines) - (not lines[-1])):
            lines[i] = lines[i][:indent] + "#" + lines[i][indent:]
        snippet = string.join(lines, '\r')
        self.insert(snippet)
        self.ted.WESetSelection(selstart, selstart + len(snippet))

    def setfile(self, file):
        self.file = file

    def set(self, text, file = ''):
        oldfile = self.file
        self.file = file
        if self._debugger:
            self._debugger.unregister_editor(self, oldfile)
            self._debugger.register_editor(self, file)
        TextEditor.set(self, text)

    def close(self):
        if self._debugger:
            self._debugger.unregister_editor(self, self.file)
            self._debugger = None
        TextEditor.close(self)

    def click(self, point, modifiers):
        if not self._enabled:
            return
        if self._debugger and self.pt_in_breaks(point):
            self.breakhit(point, modifiers)
        elif self._debugger:
            bl, bt, br, bb = self._getbreakrect()
            Qd.EraseRect((bl, bt, br-1, bb))
            TextEditor.click(self, point, modifiers)
            self.drawbreakpoints()
        else:
            TextEditor.click(self, point, modifiers)
            if self.ted.WEGetClickCount() >= 3:
                # select block with our indent
                lines = string.split(self.get(), '\r')
                selstart, selend = self.ted.WEGetSelection()
                lineno = self.ted.WEOffsetToLine(selstart)
                tabs = 0
                line = lines[lineno]
                while line[tabs:] and line[tabs] == '\t':
                    tabs = tabs + 1
                tabstag = '\t' * tabs
                fromline = 0
                toline = len(lines)
                if tabs:
                    for i in range(lineno - 1, -1, -1):
                        line = lines[i]
                        if line[:tabs] <> tabstag:
                            fromline = i + 1
                            break
                    for i in range(lineno + 1, toline):
                        line = lines[i]
                        if line[:tabs] <> tabstag:
                            toline = i - 1
                            break
                selstart, dummy = self.ted.WEGetLineRange(fromline)
                dummy, selend = self.ted.WEGetLineRange(toline)
                self.ted.WESetSelection(selstart, selend)

    def breakhit(self, point, modifiers):
        if not self.file:
            return
        destrect = self.ted.WEGetDestRect()
        offset, edge = self.ted.WEGetOffset(point)
        lineno = self.ted.WEOffsetToLine(offset) + 1
        if point[1] <= destrect[3]:
            self._debugger.clear_breaks_above(self.file, self.countlines())
            self._debugger.toggle_break(self.file, lineno)
        else:
            self._debugger.clear_breaks_above(self.file, lineno)

    def key(self, char, event):
        (what, message, when, where, modifiers) = event
        if modifiers & Events.cmdKey and not char in Wkeys.arrowkeys:
            return
        if char == '\r':
            selstart, selend = self.ted.WEGetSelection()
            selstart, selend = min(selstart, selend), max(selstart, selend)
            lastchar = chr(self.ted.WEGetChar(selstart-1))
            if lastchar <> '\r' and selstart:
                pos, dummy = self.ted.WEFindLine(selstart, 0)
                lineres = Res.Resource('')
                self.ted.WECopyRange(pos, selstart, lineres, None, None)
                line = lineres.data + '\n'
                tabcount = self.extratabs(line)
                self.ted.WEKey(ord('\r'), 0)
                for i in range(tabcount):
                    self.ted.WEKey(ord('\t'), 0)
            else:
                self.ted.WEKey(ord('\r'), 0)
        elif char in ')]}':
            self.ted.WEKey(ord(char), modifiers)
            self.balanceparens(char)
        else:
            self.ted.WEKey(ord(char), modifiers)
        if char not in Wkeys.navigationkeys:
            self.textchanged()
        self.selectionchanged()
        self.updatescrollbars()

    def balanceparens(self, char):
        if char == ')':
            target = '('
        elif char == ']':
            target = '['
        elif char == '}':
            target = '{'
        recursionlevel = 1
        selstart, selend = self.ted.WEGetSelection()
        count = min(selstart, selend) - 2
        mincount = max(0, count - 2048)
        lastquote = None
        while count > mincount:
            testchar = chr(self.ted.WEGetChar(count))
            if testchar in "\"'" and chr(self.ted.WEGetChar(count - 1)) <> '\\':
                if lastquote == testchar:
                    recursionlevel = recursionlevel - 1
                    lastquote = None
                elif not lastquote:
                    recursionlevel = recursionlevel + 1
                    lastquote = testchar
            elif not lastquote and testchar == char:
                recursionlevel = recursionlevel + 1
            elif not lastquote and testchar == target:
                recursionlevel = recursionlevel - 1
                if recursionlevel == 0:
                    import time
                    autoscroll = self.ted.WEFeatureFlag(WASTEconst.weFAutoScroll, -1)
                    if autoscroll:
                        self.ted.WEFeatureFlag(WASTEconst.weFAutoScroll, 0)
                    self.ted.WESetSelection(count, count + 1)
                    self._parentwindow.wid.GetWindowPort().QDFlushPortBuffer(None)  # needed under OSX
                    time.sleep(0.2)
                    self.ted.WESetSelection(selstart, selend)
                    if autoscroll:
                        self.ted.WEFeatureFlag(WASTEconst.weFAutoScroll, 1)
                    break
            count = count - 1

    def extratabs(self, line):
        tabcount = 0
        for c in line:
            if c <> '\t':
                break
            tabcount = tabcount + 1
        last = 0
        cleanline = ''
        tags = PyFontify.fontify(line)
        # strip comments and strings
        for tag, start, end, sublist in tags:
            if tag in ('string', 'comment'):
                cleanline = cleanline + line[last:start]
                last = end
        cleanline = cleanline + line[last:]
        cleanline = string.strip(cleanline)
        if cleanline and cleanline[-1] == ':':
            tabcount = tabcount + 1
        else:
            # extra indent after unbalanced (, [ or {
            for open, close in (('(', ')'), ('[', ']'), ('{', '}')):
                count = string.count(cleanline, open)
                if count and count > string.count(cleanline, close):
                    tabcount = tabcount + 2
                    break
        return tabcount

    def rollover(self, point, onoff):
        if onoff:
            if self._debugger and self.pt_in_breaks(point):
                Wbase.SetCursor("arrow")
            else:
                Wbase.SetCursor("iBeam")

    def draw(self, visRgn = None):
        TextEditor.draw(self, visRgn)
        if self._debugger:
            self.drawbreakpoints()

    def showbreakpoints(self, onoff):
        if (not not self._debugger) <> onoff:
            if onoff:
                if not __debug__:
                    import W
                    raise W.AlertError, "Can't debug in \"Optimize bytecode\" mode.\r(see \"Default startup options\" in EditPythonPreferences)"
                import PyDebugger
                self._debugger = PyDebugger.getdebugger()
                self._debugger.register_editor(self, self.file)
            elif self._debugger:
                self._debugger.unregister_editor(self, self.file)
                self._debugger = None
            self.adjust(self._bounds)

    def togglebreakpoints(self):
        self.showbreakpoints(not self._debugger)

    def clearbreakpoints(self):
        if self.file:
            self._debugger.clear_all_file_breaks(self.file)

    def editbreakpoints(self):
        if self._debugger:
            self._debugger.edit_breaks()
            self._debugger.breaksviewer.selectfile(self.file)

    def drawbreakpoints(self, eraseall = 0):
        breakrect = bl, bt, br, bb = self._getbreakrect()
        br = br - 1
        self.SetPort()
        Qd.PenPat(Qd.GetQDGlobalsGray())
        Qd.PaintRect((br, bt, br + 1, bb))
        Qd.PenNormal()
        self._parentwindow.tempcliprect(breakrect)
        Qd.RGBForeColor((0xffff, 0, 0))
        try:
            lasttop = bt
            self_ted = self.ted
            Qd_PaintOval = Qd.PaintOval
            Qd_EraseRect = Qd.EraseRect
            for lineno in self._debugger.get_file_breaks(self.file):
                start, end = self_ted.WEGetLineRange(lineno - 1)
                if lineno <> self_ted.WEOffsetToLine(start) + 1:
                    # breakpoints beyond our text: erase rest, and back out
                    Qd_EraseRect((bl, lasttop, br, bb))
                    break
                (x, y), h = self_ted.WEGetPoint(start, 0)
                bottom = y + h
                #print y, (lasttop, bottom)
                if bottom > lasttop:
                    Qd_EraseRect((bl, lasttop, br, y + h * eraseall))
                    lasttop = bottom
                redbullet = bl + 2, y + 3, bl + 8, y + 9
                Qd_PaintOval(redbullet)
            else:
                Qd_EraseRect((bl, lasttop, br, bb))
            Qd.RGBForeColor((0, 0, 0))
        finally:
            self._parentwindow.restoreclip()

    def updatescrollbars(self):
        if self._debugger:
            self.drawbreakpoints(1)
        TextEditor.updatescrollbars(self)

    def pt_in_breaks(self, point):
        return Qd.PtInRect(point, self._getbreakrect())

    def _getbreakrect(self):
        if self._debugger:
            l, t, r, b = self._bounds
            return (l+1, t+1, l + 12, b-1)
        else:
            return (0, 0, 0, 0)

    def _getviewrect(self):
        l, t, r, b = self._bounds
        if self._debugger:
            return (l + 17, t + 2, r, b - 2)
        else:
            return (l + 5, t + 2, r, b - 2)

    def _calctextbounds(self):
        viewrect = l, t, r, b = self._getviewrect()
        if self.ted:
            dl, dt, dr, db = self.ted.WEGetDestRect()
            vl, vt, vr, vb = self.ted.WEGetViewRect()
            xshift = l - vl
            yshift = t - vt
            if (db - dt) < (b - t):
                yshift = t - dt
            destrect = (dl + xshift, dt + yshift, dr + xshift, db + yshift)
        else:
            destrect = (l, t, r + 5000, b)
        return viewrect, destrect


def GetFNum(fontname):
    """Same as Fm.GetFNum(), but maps a missing font to Monaco instead of the system font."""
    if fontname <> Fm.GetFontName(0):
        fontid = Fm.GetFNum(fontname)
        if fontid == 0:
            fontid = Fonts.monaco
    else:
        fontid = 0
    return fontid

# b/w compat. Anyone using this?
GetFName = Fm.GetFontName

def GetPortFontSettings(port):
    return Fm.GetFontName(port.GetPortTextFont()), port.GetPortTextFace(), port.GetPortTextSize()

def SetPortFontSettings(port, (font, face, size)):
    saveport = Qd.GetPort()
    Qd.SetPort(port)
    Qd.TextFont(GetFNum(font))
    Qd.TextFace(face)
    Qd.TextSize(size)
    Qd.SetPort(saveport)
