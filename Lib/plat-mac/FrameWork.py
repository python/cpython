"A sort of application framework for the Mac"

DEBUG=0

import MacOS
import traceback

from Carbon.AE import *
from Carbon.AppleEvents import *
from Carbon.Ctl import *
from Carbon.Controls import *
from Carbon.Dlg import *
from Carbon.Dialogs import *
from Carbon.Evt import *
from Carbon.Events import *
from Carbon.Help import *
from Carbon.Menu import *
from Carbon.Menus import *
from Carbon.Qd import *
from Carbon.QuickDraw import *
#from Carbon.Res import *
#from Carbon.Resources import *
#from Carbon.Snd import *
#from Carbon.Sound import *
from Carbon.Win import *
from Carbon.Windows import *
import types

import EasyDialogs

try:
    MyFrontWindow = FrontNonFloatingWindow
except NameError:
    MyFrontWindow = FrontWindow

kHighLevelEvent = 23    # Don't know what header file this should come from
SCROLLBARWIDTH = 16         # Again, not a clue...

# Trick to forestall a set of SIOUX menus being added to our menubar
SIOUX_APPLEMENU_ID=32000


# Map event 'what' field to strings
eventname = {}
eventname[1] = 'mouseDown'
eventname[2] = 'mouseUp'
eventname[3] = 'keyDown'
eventname[4] = 'keyUp'
eventname[5] = 'autoKey'
eventname[6] = 'updateEvt'
eventname[7] = 'diskEvt'
eventname[8] = 'activateEvt'
eventname[15] = 'osEvt'
eventname[23] = 'kHighLevelEvent'

# Map part codes returned by WhichWindow() to strings
partname = {}
partname[0] = 'inDesk'
partname[1] = 'inMenuBar'
partname[2] = 'inSysWindow'
partname[3] = 'inContent'
partname[4] = 'inDrag'
partname[5] = 'inGrow'
partname[6] = 'inGoAway'
partname[7] = 'inZoomIn'
partname[8] = 'inZoomOut'

#
# The useable portion of the screen
#       ## but what happens with multiple screens? jvr
screenbounds = GetQDGlobalsScreenBits().bounds
screenbounds = screenbounds[0]+4, screenbounds[1]+4, \
    screenbounds[2]-4, screenbounds[3]-4

next_window_x = 16          # jvr
next_window_y = 44          # jvr

def windowbounds(width, height):
    "Return sensible window bounds"
    global next_window_x, next_window_y
    r, b = next_window_x+width, next_window_y+height
    if r > screenbounds[2]:
        next_window_x = 16
    if b > screenbounds[3]:
        next_window_y = 44
    l, t = next_window_x, next_window_y
    r, b = next_window_x+width, next_window_y+height
    next_window_x, next_window_y = next_window_x + 8, next_window_y + 20    # jvr
    return l, t, r, b

_watch = None
def setwatchcursor():
    global _watch

    if _watch == None:
        _watch = GetCursor(4).data
    SetCursor(_watch)

def setarrowcursor():
    SetCursor(GetQDGlobalsArrow())

class Application:

    "Application framework -- your application should be a derived class"

    def __init__(self, nomenubar=0):
        self._doing_asyncevents = 0
        self.quitting = 0
        self.needmenubarredraw = 0
        self._windows = {}
        self._helpmenu = None
        if nomenubar:
            self.menubar = None
        else:
            self.makemenubar()

    def __del__(self):
        if self._doing_asyncevents:
            self._doing_asyncevents = 0
            MacOS.SetEventHandler()

    def makemenubar(self):
        self.menubar = MenuBar(self)
        AppleMenu(self.menubar, self.getabouttext(), self.do_about)
        self.makeusermenus()

    def makeusermenus(self):
        self.filemenu = m = Menu(self.menubar, "File")
        self._quititem = MenuItem(m, "Quit", "Q", self._quit)

    def gethelpmenu(self):
        if self._helpmenu == None:
            self._helpmenu = HelpMenu(self.menubar)
        return self._helpmenu

    def _quit(self, *args):
        self.quitting = 1

    def cleanup(self):
        for w in self._windows.values():
            w.do_close()
        return self._windows == {}

    def appendwindow(self, wid, window):
        self._windows[wid] = window

    def removewindow(self, wid):
        del self._windows[wid]

    def getabouttext(self):
        return "About %s..." % self.__class__.__name__

    def do_about(self, id, item, window, event):
        EasyDialogs.Message("Hello, world!" + "\015(%s)" % self.__class__.__name__)

    # The main event loop is broken up in several simple steps.
    # This is done so you can override each individual part,
    # if you have a need to do extra processing independent of the
    # event type.
    # Normally, however, you'd just define handlers for individual
    # events.

    schedparams = (0, 0)    # By default disable Python's event handling
    default_wait = None         # By default we wait GetCaretTime in WaitNextEvent

    def mainloop(self, mask = everyEvent, wait = None):
        self.quitting = 0
        if hasattr(MacOS, 'SchedParams'):
            saveparams = MacOS.SchedParams(*self.schedparams)
        try:
            while not self.quitting:
                try:
                    self.do1event(mask, wait)
                except (Application, SystemExit):
                    # Note: the raising of "self" is old-fashioned idiom to
                    # exit the mainloop. Calling _quit() is better for new
                    # applications.
                    break
        finally:
            if hasattr(MacOS, 'SchedParams'):
                MacOS.SchedParams(*saveparams)

    def dopendingevents(self, mask = everyEvent):
        """dopendingevents - Handle all pending events"""
        while self.do1event(mask, wait=0):
            pass

    def do1event(self, mask = everyEvent, wait = None):
        ok, event = self.getevent(mask, wait)
        if IsDialogEvent(event):
            if self.do_dialogevent(event):
                return
        if ok:
            self.dispatch(event)
        else:
            self.idle(event)

    def idle(self, event):
        pass

    def getevent(self, mask = everyEvent, wait = None):
        if self.needmenubarredraw:
            DrawMenuBar()
            self.needmenubarredraw = 0
        if wait is None:
            wait = self.default_wait
            if wait is None:
                wait = GetCaretTime()
        ok, event = WaitNextEvent(mask, wait)
        return ok, event

    def dispatch(self, event):
        # The following appears to be double work (already done in do1event)
        # but we need it for asynchronous event handling
        if IsDialogEvent(event):
            if self.do_dialogevent(event):
                return
        (what, message, when, where, modifiers) = event
        if eventname.has_key(what):
            name = "do_" + eventname[what]
        else:
            name = "do_%d" % what
        try:
            handler = getattr(self, name)
        except AttributeError:
            handler = self.do_unknownevent
        handler(event)

    def asyncevents(self, onoff):
        """asyncevents - Set asynchronous event handling on or off"""
        if MacOS.runtimemodel == 'macho':
            raise 'Unsupported in MachoPython'
        old = self._doing_asyncevents
        if old:
            MacOS.SetEventHandler()
            MacOS.SchedParams(*self.schedparams)
        if onoff:
            MacOS.SetEventHandler(self.dispatch)
            doint, dummymask, benice, howoften, bgyield = \
                   self.schedparams
            MacOS.SchedParams(doint, everyEvent, benice,
                      howoften, bgyield)
        self._doing_asyncevents = onoff
        return old

    def do_dialogevent(self, event):
        gotone, dlg, item = DialogSelect(event)
        if gotone:
            window = dlg.GetDialogWindow()
            if self._windows.has_key(window):
                self._windows[window].do_itemhit(item, event)
            else:
                print 'Dialog event for unknown dialog'
            return 1
        return 0

    def do_mouseDown(self, event):
        (what, message, when, where, modifiers) = event
        partcode, wid = FindWindow(where)

        #
        # Find the correct name.
        #
        if partname.has_key(partcode):
            name = "do_" + partname[partcode]
        else:
            name = "do_%d" % partcode

        if wid == None:
            # No window, or a non-python window
            try:
                handler = getattr(self, name)
            except AttributeError:
                # Not menubar or something, so assume someone
                # else's window
                if hasattr(MacOS, 'HandleEvent'):
                    MacOS.HandleEvent(event)
                return
        elif self._windows.has_key(wid):
            # It is a window. Hand off to correct window.
            window = self._windows[wid]
            try:
                handler = getattr(window, name)
            except AttributeError:
                handler = self.do_unknownpartcode
        else:
            # It is a python-toolbox window, but not ours.
            handler = self.do_unknownwindow
        handler(partcode, wid, event)

    def do_inSysWindow(self, partcode, window, event):
        if hasattr(MacOS, 'HandleEvent'):
            MacOS.HandleEvent(event)

    def do_inDesk(self, partcode, window, event):
        if hasattr(MacOS, 'HandleEvent'):
            MacOS.HandleEvent(event)

    def do_inMenuBar(self, partcode, window, event):
        if not self.menubar:
            if hasattr(MacOS, 'HandleEvent'):
                MacOS.HandleEvent(event)
            return
        (what, message, when, where, modifiers) = event
        result = MenuSelect(where)
        id = (result>>16) & 0xffff      # Hi word
        if id >= 0x8000:
            id = -65536 + id
        item = result & 0xffff      # Lo word
        self.do_rawmenu(id, item, window, event)

    def do_rawmenu(self, id, item, window, event):
        try:
            self.do_menu(id, item, window, event)
        finally:
            HiliteMenu(0)

    def do_menu(self, id, item, window, event):
        if hasattr(MacOS, 'OutputSeen'):
            MacOS.OutputSeen()
        self.menubar.dispatch(id, item, window, event)


    def do_unknownpartcode(self, partcode, window, event):
        (what, message, when, where, modifiers) = event
        if DEBUG: print "Mouse down at global:", where
        if DEBUG: print "\tUnknown part code:", partcode
        if DEBUG: print "\tEvent:", self.printevent(event)
        if hasattr(MacOS, 'HandleEvent'):
            MacOS.HandleEvent(event)

    def do_unknownwindow(self, partcode, window, event):
        if DEBUG: print 'Unknown window:', window
        if hasattr(MacOS, 'HandleEvent'):
            MacOS.HandleEvent(event)

    def do_keyDown(self, event):
        self.do_key(event)

    def do_autoKey(self, event):
        if not event[-1] & cmdKey:
            self.do_key(event)

    def do_key(self, event):
        (what, message, when, where, modifiers) = event
        c = chr(message & charCodeMask)
        if self.menubar:
            result = MenuEvent(event)
            id = (result>>16) & 0xffff      # Hi word
            item = result & 0xffff      # Lo word
            if id:
                self.do_rawmenu(id, item, None, event)
                return
            # Otherwise we fall-through
        if modifiers & cmdKey:
            if c == '.':
                raise self
            else:
                if not self.menubar:
                    if hasattr(MacOS, 'HandleEvent'):
                        MacOS.HandleEvent(event)
                return
        else:
            # See whether the front window wants it
            w = MyFrontWindow()
            if w and self._windows.has_key(w):
                window = self._windows[w]
                try:
                    do_char = window.do_char
                except AttributeError:
                    do_char = self.do_char
                do_char(c, event)
            # else it wasn't for us, sigh...

    def do_char(self, c, event):
        if DEBUG: print "Character", repr(c)

    def do_updateEvt(self, event):
        (what, message, when, where, modifiers) = event
        wid = WhichWindow(message)
        if wid and self._windows.has_key(wid):
            window = self._windows[wid]
            window.do_rawupdate(wid, event)
        else:
            if hasattr(MacOS, 'HandleEvent'):
                MacOS.HandleEvent(event)

    def do_activateEvt(self, event):
        (what, message, when, where, modifiers) = event
        wid = WhichWindow(message)
        if wid and self._windows.has_key(wid):
            window = self._windows[wid]
            window.do_activate(modifiers & 1, event)
        else:
            if hasattr(MacOS, 'HandleEvent'):
                MacOS.HandleEvent(event)

    def do_osEvt(self, event):
        (what, message, when, where, modifiers) = event
        which = (message >> 24) & 0xff
        if which == 1:  # suspend/resume
            self.do_suspendresume(event)
        else:
            if DEBUG:
                print 'unknown osEvt:',
                self.printevent(event)

    def do_suspendresume(self, event):
        (what, message, when, where, modifiers) = event
        wid = MyFrontWindow()
        if wid and self._windows.has_key(wid):
            window = self._windows[wid]
            window.do_activate(message & 1, event)

    def do_kHighLevelEvent(self, event):
        (what, message, when, where, modifiers) = event
        if DEBUG:
            print "High Level Event:",
            self.printevent(event)
        try:
            AEProcessAppleEvent(event)
        except:
            pass
            #print "AEProcessAppleEvent error:"
            #traceback.print_exc()

    def do_unknownevent(self, event):
        if DEBUG:
            print "Unhandled event:",
            self.printevent(event)

    def printevent(self, event):
        (what, message, when, where, modifiers) = event
        nicewhat = repr(what)
        if eventname.has_key(what):
            nicewhat = eventname[what]
        print nicewhat,
        if what == kHighLevelEvent:
            h, v = where
            print repr(ostypecode(message)), hex(when), repr(ostypecode(h | (v<<16))),
        else:
            print hex(message), hex(when), where,
        print hex(modifiers)


class MenuBar:
    """Represent a set of menus in a menu bar.

    Interface:

    - (constructor)
    - (destructor)
    - addmenu
    - addpopup (normally used internally)
    - dispatch (called from Application)
    """

    nextid = 1      # Necessarily a class variable

    def getnextid(self):
        id = MenuBar.nextid
        MenuBar.nextid = id+1
        return id

    def __init__(self, parent=None):
        self.parent = parent
        ClearMenuBar()
        self.bar = GetMenuBar()
        self.menus = {}

    # XXX necessary?
    def close(self):
        self.parent = None
        self.bar = None
        self.menus = None

    def addmenu(self, title, after = 0, id=None):
        if id == None:
            id = self.getnextid()
        if DEBUG: print 'Newmenu', title, id # XXXX
        m = NewMenu(id, title)
        m.InsertMenu(after)
        if after >= 0:
            if self.parent:
                self.parent.needmenubarredraw = 1
            else:
                DrawMenuBar()
        return id, m

    def delmenu(self, id):
        if DEBUG: print 'Delmenu', id # XXXX
        DeleteMenu(id)

    def addpopup(self, title = ''):
        return self.addmenu(title, -1)

# Useless:
#       def install(self):
#           if not self.bar: return
#           SetMenuBar(self.bar)
#           if self.parent:
#               self.parent.needmenubarredraw = 1
#           else:
#               DrawMenuBar()

    def fixmenudimstate(self):
        for m in self.menus.keys():
            menu = self.menus[m]
            if menu.__class__ == FrameWork.AppleMenu:
                continue
            for i in range(len(menu.items)):
                label, shortcut, callback, kind = menu.items[i]
                if type(callback) == types.StringType:
                    wid = MyFrontWindow()
                    if wid and self.parent._windows.has_key(wid):
                        window = self.parent._windows[wid]
                        if hasattr(window, "domenu_" + callback):
                            menu.menu.EnableMenuItem(i + 1)
                        elif hasattr(self.parent, "domenu_" + callback):
                            menu.menu.EnableMenuItem(i + 1)
                        else:
                            menu.menu.DisableMenuItem(i + 1)
                    elif hasattr(self.parent, "domenu_" + callback):
                        menu.menu.EnableMenuItem(i + 1)
                    else:
                        menu.menu.DisableMenuItem(i + 1)
                elif callback:
                    pass

    def dispatch(self, id, item, window, event):
        if self.menus.has_key(id):
            self.menus[id].dispatch(id, item, window, event)
        else:
            if DEBUG: print "MenuBar.dispatch(%d, %d, %s, %s)" % \
                (id, item, window, event)


# XXX Need a way to get menus as resources and bind them to callbacks

class Menu:
    "One menu."

    def __init__(self, bar, title, after=0, id=None):
        self.bar = bar
        self.id, self.menu = self.bar.addmenu(title, after, id)
        bar.menus[self.id] = self
        self.items = []
        self._parent = None

    def delete(self):
        self.bar.delmenu(self.id)
        del self.bar.menus[self.id]
        self.menu.DisposeMenu()
        del self.bar
        del self.items
        del self.menu
        del self.id
        del self._parent

    def additem(self, label, shortcut=None, callback=None, kind=None):
        self.menu.AppendMenu('x')           # add a dummy string
        self.items.append((label, shortcut, callback, kind))
        item = len(self.items)
        if isinstance(label, unicode):
            self.menu.SetMenuItemTextWithCFString(item, label)
        else:
            self.menu.SetMenuItemText(item, label)
        if shortcut and type(shortcut) == type(()):
            modifiers, char = shortcut[:2]
            self.menu.SetItemCmd(item, ord(char))
            self.menu.SetMenuItemModifiers(item, modifiers)
            if len(shortcut) > 2:
                self.menu.SetMenuItemKeyGlyph(item, shortcut[2])
        elif shortcut:
            self.menu.SetItemCmd(item, ord(shortcut))
        return item

    def delitem(self, item):
        if item != len(self.items):
            raise 'Can only delete last item of a menu'
        self.menu.DeleteMenuItem(item)
        del self.items[item-1]

    def addcheck(self, label, shortcut=None, callback=None):
        return self.additem(label, shortcut, callback, 'check')

    def addradio(self, label, shortcut=None, callback=None):
        return self.additem(label, shortcut, callback, 'radio')

    def addseparator(self):
        self.menu.AppendMenu('(-')
        self.items.append(('', None, None, 'separator'))

    def addsubmenu(self, label, title=''):
        sub = Menu(self.bar, title, -1)
        item = self.additem(label, '\x1B', None, 'submenu')
        self.menu.SetItemMark(item, sub.id)
        sub._parent = self
        sub._parent_item = item
        return sub

    def dispatch(self, id, item, window, event):
        title, shortcut, callback, mtype = self.items[item-1]
        if callback:
            if not self.bar.parent or type(callback) <> types.StringType:
                menuhandler = callback
            else:
                # callback is string
                wid = MyFrontWindow()
                if wid and self.bar.parent._windows.has_key(wid):
                    window = self.bar.parent._windows[wid]
                    if hasattr(window, "domenu_" + callback):
                        menuhandler = getattr(window, "domenu_" + callback)
                    elif hasattr(self.bar.parent, "domenu_" + callback):
                        menuhandler = getattr(self.bar.parent, "domenu_" + callback)
                    else:
                        # nothing we can do. we shouldn't have come this far
                        # since the menu item should have been disabled...
                        return
                elif hasattr(self.bar.parent, "domenu_" + callback):
                    menuhandler = getattr(self.bar.parent, "domenu_" + callback)
                else:
                    # nothing we can do. we shouldn't have come this far
                    # since the menu item should have been disabled...
                    return
            menuhandler(id, item, window, event)

    def enable(self, onoff):
        if onoff:
            self.menu.EnableMenuItem(0)
            if self._parent:
                self._parent.menu.EnableMenuItem(self._parent_item)
        else:
            self.menu.DisableMenuItem(0)
            if self._parent:
                self._parent.menu.DisableMenuItem(self._parent_item)
        if self.bar and self.bar.parent:
            self.bar.parent.needmenubarredraw = 1

class PopupMenu(Menu):
    def __init__(self, bar):
        Menu.__init__(self, bar, '(popup)', -1)

    def popup(self, x, y, event, default=1, window=None):
        # NOTE that x and y are global coordinates, and they should probably
        # be topleft of the button the user clicked (not mouse-coordinates),
        # so the popup nicely overlaps.
        reply = self.menu.PopUpMenuSelect(x, y, default)
        if not reply:
            return
        id = (reply >> 16) & 0xffff
        item = reply & 0xffff
        if not window:
            wid = MyFrontWindow()
            try:
                window = self.bar.parent._windows[wid]
            except:
                pass # If we can't find the window we pass None
        self.dispatch(id, item, window, event)

class MenuItem:
    def __init__(self, menu, title, shortcut=None, callback=None, kind=None):
        self.item = menu.additem(title, shortcut, callback)
        self.menu = menu

    def delete(self):
        self.menu.delitem(self.item)
        del self.menu
        del self.item

    def check(self, onoff):
        self.menu.menu.CheckMenuItem(self.item, onoff)

    def enable(self, onoff):
        if onoff:
            self.menu.menu.EnableMenuItem(self.item)
        else:
            self.menu.menu.DisableMenuItem(self.item)

    def settext(self, text):
        self.menu.menu.SetMenuItemText(self.item, text)

    def setstyle(self, style):
        self.menu.menu.SetItemStyle(self.item, style)

    def seticon(self, icon):
        self.menu.menu.SetItemIcon(self.item, icon)

    def setcmd(self, cmd):
        self.menu.menu.SetItemCmd(self.item, cmd)

    def setmark(self, cmd):
        self.menu.menu.SetItemMark(self.item, cmd)


class RadioItem(MenuItem):
    def __init__(self, menu, title, shortcut=None, callback=None):
        MenuItem.__init__(self, menu, title, shortcut, callback, 'radio')

class CheckItem(MenuItem):
    def __init__(self, menu, title, shortcut=None, callback=None):
        MenuItem.__init__(self, menu, title, shortcut, callback, 'check')

def Separator(menu):
    menu.addseparator()

def SubMenu(menu, label, title=''):
    return menu.addsubmenu(label, title)


class AppleMenu(Menu):

    def __init__(self, bar, abouttext="About me...", aboutcallback=None):
        Menu.__init__(self, bar, "\024", id=SIOUX_APPLEMENU_ID)
        if MacOS.runtimemodel == 'ppc':
            self.additem(abouttext, None, aboutcallback)
            self.addseparator()
            self.menu.AppendResMenu('DRVR')
        else:
            # Additem()'s tricks do not work for "apple" menu under Carbon
            self.menu.InsertMenuItem(abouttext, 0)
            self.items.append((abouttext, None, aboutcallback, None))

    def dispatch(self, id, item, window, event):
        if item == 1:
            Menu.dispatch(self, id, item, window, event)
        elif MacOS.runtimemodel == 'ppc':
            name = self.menu.GetMenuItemText(item)
            OpenDeskAcc(name)

class HelpMenu(Menu):
    def __init__(self, bar):
        # Note we don't call Menu.__init__, we do the necessary things by hand
        self.bar = bar
        self.menu, index = HMGetHelpMenu()
        self.id = self.menu.GetMenuID()
        bar.menus[self.id] = self
        # The next line caters for the entries the system already handles for us
        self.items = [None]*(index-1)
        self._parent = None


class Window:
    """A single window belonging to an application"""

    def __init__(self, parent):
        self.wid = None
        self.parent = parent

    def open(self, bounds=(40, 40, 400, 400), resid=None):
        if resid <> None:
            self.wid = GetNewWindow(resid, -1)
        else:
            self.wid = NewWindow(bounds, self.__class__.__name__, 1,
                8, -1, 1, 0)    # changed to proc id 8 to include zoom box. jvr
        self.do_postopen()

    def do_postopen(self):
        """Tell our parent we exist"""
        self.parent.appendwindow(self.wid, self)

    def close(self):
        self.do_postclose()

    def do_postclose(self):
        self.parent.removewindow(self.wid)
        self.parent = None
        self.wid = None

    def SetPort(self):
        # Convinience method
        SetPort(self.wid)

    def GetWindow(self):
        return self.wid

    def do_inDrag(self, partcode, window, event):
        where = event[3]
        window.DragWindow(where, self.draglimit)

    draglimit = screenbounds

    def do_inGoAway(self, partcode, window, event):
        where = event[3]
        if window.TrackGoAway(where):
            self.close()

    def do_inZoom(self, partcode, window, event):
        (what, message, when, where, modifiers) = event
        if window.TrackBox(where, partcode):
            window.ZoomWindow(partcode, 1)
            rect = window.GetWindowUserState()                  # so that zoom really works... jvr
            self.do_postresize(rect[2] - rect[0], rect[3] - rect[1], window)    # jvr

    def do_inZoomIn(self, partcode, window, event):
        SetPort(window) # !!!
        self.do_inZoom(partcode, window, event)

    def do_inZoomOut(self, partcode, window, event):
        SetPort(window) # !!!
        self.do_inZoom(partcode, window, event)

    def do_inGrow(self, partcode, window, event):
        (what, message, when, where, modifiers) = event
        result = window.GrowWindow(where, self.growlimit)
        if result:
            height = (result>>16) & 0xffff  # Hi word
            width = result & 0xffff     # Lo word
            self.do_resize(width, height, window)

    growlimit = (50, 50, screenbounds[2] - screenbounds[0], screenbounds[3] - screenbounds[1])      # jvr

    def do_resize(self, width, height, window):
        l, t, r, b = self.wid.GetWindowPort().GetPortBounds()           # jvr, forGrowIcon
        self.SetPort()                          # jvr
        self.wid.InvalWindowRect((r - SCROLLBARWIDTH + 1, b - SCROLLBARWIDTH + 1, r, b))    # jvr
        window.SizeWindow(width, height, 1)         # changed updateFlag to true jvr
        self.do_postresize(width, height, window)

    def do_postresize(self, width, height, window):
        SetPort(window)
        self.wid.InvalWindowRect(window.GetWindowPort().GetPortBounds())

    def do_inContent(self, partcode, window, event):
        #
        # If we're not frontmost, select ourselves and wait for
        # the activate event.
        #
        if MyFrontWindow() <> window:
            window.SelectWindow()
            return
        # We are. Handle the event.
        (what, message, when, where, modifiers) = event
        SetPort(window)
        local = GlobalToLocal(where)
        self.do_contentclick(local, modifiers, event)

    def do_contentclick(self, local, modifiers, event):
        if DEBUG:
            print 'Click in contents at %s, modifiers %s'%(local, modifiers)

    def do_rawupdate(self, window, event):
        if DEBUG: print "raw update for", window
        SetPort(window)
        window.BeginUpdate()
        self.do_update(window, event)
        window.EndUpdate()

    def do_update(self, window, event):
        if DEBUG:
            import time
            for i in range(8):
                time.sleep(0.1)
                InvertRgn(window.GetWindowPort().visRgn)
            FillRgn(window.GetWindowPort().visRgn, GetQDGlobalsGray())
        else:
            EraseRgn(window.GetWindowPort().visRgn)

    def do_activate(self, activate, event):
        if DEBUG: print 'Activate %d for %s'%(activate, self.wid)

class ControlsWindow(Window):

    def do_rawupdate(self, window, event):
        if DEBUG: print "raw update for", window
        SetPort(window)
        window.BeginUpdate()
        self.do_update(window, event)
        #DrawControls(window)                   # jvr
        UpdateControls(window, window.GetWindowPort().visRgn)   # jvr
        window.DrawGrowIcon()
        window.EndUpdate()

    def do_controlhit(self, window, control, pcode, event):
        if DEBUG: print "control hit in", window, "on", control, "; pcode =", pcode

    def do_inContent(self, partcode, window, event):
        if MyFrontWindow() <> window:
            window.SelectWindow()
            return
        (what, message, when, where, modifiers) = event
        SetPort(window)  # XXXX Needed?
        local = GlobalToLocal(where)
        pcode, control = FindControl(local, window)
        if pcode and control:
            self.do_rawcontrolhit(window, control, pcode, local, event)
        else:
            if DEBUG: print "FindControl(%s, %s) -> (%s, %s)" % \
                (local, window, pcode, control)
            self.do_contentclick(local, modifiers, event)

    def do_rawcontrolhit(self, window, control, pcode, local, event):
        pcode = control.TrackControl(local)
        if pcode:
            self.do_controlhit(window, control, pcode, event)

class ScrolledWindow(ControlsWindow):
    def __init__(self, parent):
        self.barx = self.bary = None
        self.barx_enabled = self.bary_enabled = 1
        self.activated = 1
        ControlsWindow.__init__(self, parent)

    def scrollbars(self, wantx=1, wanty=1):
        SetPort(self.wid)
        self.barx = self.bary = None
        self.barx_enabled = self.bary_enabled = 1
        x0, y0, x1, y1 = self.wid.GetWindowPort().GetPortBounds()
        vx, vy = self.getscrollbarvalues()
        if vx == None: self.barx_enabled, vx = 0, 0
        if vy == None: self.bary_enabled, vy = 0, 0
        if wantx:
            rect = x0-1, y1-(SCROLLBARWIDTH-1), x1-(SCROLLBARWIDTH-2), y1+1
            self.barx = NewControl(self.wid, rect, "", 1, vx, 0, 32767, 16, 0)
            if not self.barx_enabled: self.barx.HiliteControl(255)
##              self.wid.InvalWindowRect(rect)
        if wanty:
            rect = x1-(SCROLLBARWIDTH-1), y0-1, x1+1, y1-(SCROLLBARWIDTH-2)
            self.bary = NewControl(self.wid, rect, "", 1, vy, 0, 32767, 16, 0)
            if not self.bary_enabled: self.bary.HiliteControl(255)
##              self.wid.InvalWindowRect(rect)

    def do_postclose(self):
        self.barx = self.bary = None
        ControlsWindow.do_postclose(self)

    def do_activate(self, onoff, event):
        self.activated = onoff
        if onoff:
            if self.barx and self.barx_enabled:
                self.barx.ShowControl() # jvr
            if self.bary and self.bary_enabled:
                self.bary.ShowControl() # jvr
        else:
            if self.barx:
                self.barx.HideControl() # jvr; An inactive window should have *hidden*
                            # scrollbars, not just dimmed (no matter what
                            # BBEdit does... look at the Finder)
            if self.bary:
                self.bary.HideControl() # jvr
        self.wid.DrawGrowIcon()         # jvr

    def do_postresize(self, width, height, window):
        l, t, r, b = self.wid.GetWindowPort().GetPortBounds()
        self.SetPort()
        if self.barx:
            self.barx.HideControl()     # jvr
            self.barx.MoveControl(l-1, b-(SCROLLBARWIDTH-1))
            self.barx.SizeControl((r-l)-(SCROLLBARWIDTH-3), SCROLLBARWIDTH) # jvr
        if self.bary:
            self.bary.HideControl()     # jvr
            self.bary.MoveControl(r-(SCROLLBARWIDTH-1), t-1)
            self.bary.SizeControl(SCROLLBARWIDTH, (b-t)-(SCROLLBARWIDTH-3)) # jvr
        if self.barx:
            self.barx.ShowControl()     # jvr
            self.wid.ValidWindowRect((l, b - SCROLLBARWIDTH + 1, r - SCROLLBARWIDTH + 2, b))    # jvr
        if self.bary:
            self.bary.ShowControl()     # jvr
            self.wid.ValidWindowRect((r - SCROLLBARWIDTH + 1, t, r, b - SCROLLBARWIDTH + 2))    # jvr
        self.wid.InvalWindowRect((r - SCROLLBARWIDTH + 1, b - SCROLLBARWIDTH + 1, r, b))    # jvr, growicon


    def do_rawcontrolhit(self, window, control, pcode, local, event):
        if control == self.barx:
            which = 'x'
        elif control == self.bary:
            which = 'y'
        else:
            return 0
        if pcode in (inUpButton, inDownButton, inPageUp, inPageDown):
            # We do the work for the buttons and grey area in the tracker
            dummy = control.TrackControl(local, self.do_controltrack)
        else:
            # but the thumb is handled here
            pcode = control.TrackControl(local)
            if pcode == inThumb:
                value = control.GetControlValue()
                print 'setbars', which, value #DBG
                self.scrollbar_callback(which, 'set', value)
                self.updatescrollbars()
            else:
                print 'funny part', pcode #DBG
        return 1

    def do_controltrack(self, control, pcode):
        if control == self.barx:
            which = 'x'
        elif control == self.bary:
            which = 'y'
        else:
            return

        if pcode == inUpButton:
            what = '-'
        elif pcode == inDownButton:
            what = '+'
        elif pcode == inPageUp:
            what = '--'
        elif pcode == inPageDown:
            what = '++'
        else:
            return
        self.scrollbar_callback(which, what, None)
        self.updatescrollbars()

    def updatescrollbars(self):
        SetPort(self.wid)
        vx, vy = self.getscrollbarvalues()
        if self.barx:
            if vx == None:
                self.barx.HiliteControl(255)
                self.barx_enabled = 0
            else:
                if not self.barx_enabled:
                    self.barx_enabled = 1
                    if self.activated:
                        self.barx.HiliteControl(0)
                self.barx.SetControlValue(vx)
        if self.bary:
            if vy == None:
                self.bary.HiliteControl(255)
                self.bary_enabled = 0
            else:
                if not self.bary_enabled:
                    self.bary_enabled = 1
                    if self.activated:
                        self.bary.HiliteControl(0)
                self.bary.SetControlValue(vy)

    # Auxiliary function: convert standard text/image/etc coordinate
    # to something palatable as getscrollbarvalues() return
    def scalebarvalue(self, absmin, absmax, curmin, curmax):
        if curmin <= absmin and curmax >= absmax:
            return None
        if curmin <= absmin:
            return 0
        if curmax >= absmax:
            return 32767
        perc = float(curmin-absmin)/float(absmax-absmin)
        return int(perc*32767)

    # To be overridden:

    def getscrollbarvalues(self):
        return 0, 0

    def scrollbar_callback(self, which, what, value):
        print 'scroll', which, what, value

class DialogWindow(Window):
    """A modeless dialog window"""

    def open(self, resid):
        self.dlg = GetNewDialog(resid, -1)
        self.wid = self.dlg.GetDialogWindow()
        self.do_postopen()

    def close(self):
        self.do_postclose()

    def do_postclose(self):
        self.dlg = None
        Window.do_postclose(self)

    def do_itemhit(self, item, event):
        print 'Dialog %s, item %d hit'%(self.dlg, item)

    def do_rawupdate(self, window, event):
        pass

def ostypecode(x):
    "Convert a long int to the 4-character code it really is"
    s = ''
    for i in range(4):
        x, c = divmod(x, 256)
        s = chr(c) + s
    return s


class TestApp(Application):

    "This class is used by the test() function"

    def makeusermenus(self):
        self.filemenu = m = Menu(self.menubar, "File")
        self.saveitem = MenuItem(m, "Save", "S", self.save)
        Separator(m)
        self.optionsmenu = mm = SubMenu(m, "Options")
        self.opt1 = CheckItem(mm, "Arguments", "A")
        self.opt2 = CheckItem(mm, "Being hit on the head lessons", (kMenuOptionModifier, "A"))
        self.opt3 = CheckItem(mm, "Complaints", (kMenuOptionModifier|kMenuNoCommandModifier, "A"))
        Separator(m)
        self.itemeh = MenuItem(m, "Enable Help", None, self.enablehelp)
        self.itemdbg = MenuItem(m, "Debug", None, self.debug)
        Separator(m)
        self.quititem = MenuItem(m, "Quit", "Q", self.quit)

    def save(self, *args):
        print "Save"

    def quit(self, *args):
        raise self

    def enablehelp(self, *args):
        hm = self.gethelpmenu()
        self.nohelpitem = MenuItem(hm, "There isn't any", None, self.nohelp)

    def nohelp(self, *args):
        print "I told you there isn't any!"

    def debug(self, *args):
        import pdb
        pdb.set_trace()


def test():
    "Test program"
    app = TestApp()
    app.mainloop()


if __name__ == '__main__':
    test()
