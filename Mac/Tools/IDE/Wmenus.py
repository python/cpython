import FrameWork
import Wbase, Wcontrols
from Carbon import Ctl, Controls, Qd, Res
from types import *
import Wapplication

#_arrowright = Qd.GetPicture(472)
#_arrowdown = Qd.GetPicture(473)

_arrowright = Res.Resource(
        '\x00I\x00\x00\x00\x00\x00\n\x00\n\x11\x01\x01\x00\n\x00\x00\x00'
        '\x00\x00\n\x00\n\x90\x00\x02\x00\x00\x00\x00\x00\n\x00\n\x00\x00'
        '\x00\x00\x00\n\x00\n\x00\x00\x00\x00\x00\n\x00\n\x00\x00\x10\x00'
        '\x18\x00\x1c\x00\x1e\x00\x1f\x00\x1f\x00\x1e\x00\x1c\x00\x18\x00'
        '\x10\x00\xff')


class PopupControl(Wcontrols.ControlWidget):

    def __init__(self, possize, items=None, callback=None):
        if items is None:
            items = []
        procID = Controls.popupMenuProc|Controls.popupFixedWidth|Controls.useWFont
        Wcontrols.ControlWidget.__init__(self, possize, "", procID, callback, 0, 0, 0)
        self._items = items[:]

    def set(self, value):
        self._control.SetControlValue(value+1)

    def get(self):
        return self._control.GetControlValue() - 1

    def open(self):
        self.menu = menu = FrameWork.Menu(self._parentwindow.parent.menubar, 'Foo', -1)

        for i in range(len(self._items)):
            item = self._items[i]
            if type(item) == StringType:
                menuitemtext = object = item
            elif type(item) == TupleType and len(item) == 2:
                menuitemtext, object = item
                self._items[i] = object
            else:
                raise Wbase.WidgetsError, "illegal itemlist for popup menu"
            menuitem = FrameWork.MenuItem(menu, menuitemtext, None, None)

        self._calcbounds()
        self._control = Ctl.NewControl(self._parentwindow.wid,
                                        self._bounds,
                                        self._title,
                                        1,
                                        self._value,
                                        self.menu.id,
                                        self._max,
                                        self._procID,
                                        0)
        self.SetPort()
        self.enable(self._enabled)

    def close(self):
        self.menu.delete()
        return Wcontrols.ControlWidget.close(self)

    def click(self, point, modifiers):
        if not self._enabled:
            return
        part = self._control.TrackControl(point, -1)
        if part:
            if self._callback:
                Wbase.CallbackCall(self._callback, 0, self._items[self.get()])


class PopupWidget(Wbase.ClickableWidget):

    """Simple title-less popup widget. Should be 16x16 pixels.
    Builds the menu items on the fly, good for dynamic popup menus."""

    def __init__(self, possize, items=None, callback=None):
        Wbase.ClickableWidget.__init__(self, possize)
        if items is None:
            items = []
        self._items = items
        self._itemsdict = {}
        self._callback = callback
        self._enabled = 1

    def close(self):
        Wbase.ClickableWidget.close(self)
        self._items = None
        self._itemsdict = {}

    def draw(self, visRgn = None):
        if self._visible:
            Qd.FrameRect(self._bounds)
            Qd.EraseRect(Qd.InsetRect(self._bounds, 1, 1))
            l, t, r, b = self._bounds
            l = l + 2
            t = t + 3
            pictframe = (l, t, l + 10, t + 10)
            Qd.DrawPicture(_arrowright, pictframe)

    def click(self, point, modifiers):
        if not self._enabled:
            return
        self.menu = FrameWork.Menu(self._parentwindow.parent.menubar, 'Foo', -1)
        self._additems(self._items, self.menu)
        self.SetPort()
        l, t, r, b = self._bounds
        l, t = Qd.LocalToGlobal((l+1, t+1))
        Wbase.SetCursor("arrow")
        self.menu.menu.EnableMenuItem(0)
        reply = self.menu.menu.PopUpMenuSelect(t, l, 1)
        if reply:
            id = reply >> 16
            item = reply & 0xffff
            self._menu_callback(id, item)
        self._emptymenu()

    def set(self, items):
        self._items = items

    def get(self):
        return self._items

    def _additems(self, items, menu):
        from FrameWork import SubMenu, MenuItem
        menu_id = menu.id
        for item in items:
            if item == "-":
                menu.addseparator()
                continue
            elif type(item) == ListType:
                submenu = SubMenu(menu, item[0])
                self._additems(item[1:], submenu)
                continue
            elif type(item) == StringType:
                menuitemtext = object = item
            elif type(item) == TupleType and len(item) == 2:
                menuitemtext, object = item
            else:
                raise Wbase.WidgetsError, "illegal itemlist for popup menu"

            if menuitemtext[:1] == '\0':
                check = ord(menuitemtext[1])
                menuitemtext = menuitemtext[2:]
            else:
                check = 0
            menuitem = MenuItem(menu, menuitemtext, None, None)
            if check:
                menuitem.check(1)
            self._itemsdict[(menu_id, menuitem.item)] = object

    def _emptymenu(self):
        menus = self._parentwindow.parent.menubar.menus
        for id, item in self._itemsdict.keys():
            if menus.has_key(id):
                self.menu = menus[id]
                self.menu.delete()
        self._itemsdict = {}

    def _menu_callback(self, id, item):
        thing = self._itemsdict[(id, item)]
        if callable(thing):
            thing()
        elif self._callback:
            Wbase.CallbackCall(self._callback, 0, thing)


class PopupMenu(PopupWidget):

    """Simple title-less popup widget. Should be 16x16 pixels.
    Prebuilds the menu items, good for static (non changing) popup menus."""

    def open(self):
        self._calcbounds()
        self.menu = Wapplication.Menu(self._parentwindow.parent.menubar, 'Foo', -1)
        self._additems(self._items, self.menu)

    def close(self):
        self._emptymenu()
        Wbase.Widget.close(self)
        self._items = None
        self._itemsdict = {}
        self.menu = None

    def set(self, items):
        if self._itemsdict:
            self._emptymenu()
        self.menu = Wapplication.Menu(self._parentwindow.parent.menubar, 'Foo', -1)
        self._items = items
        self._additems(self._items, self.menu)

    def click(self, point, modifiers):
        if not self._enabled:
            return
        self.SetPort()
        l, t, r, b = self._bounds
        l, t = Qd.LocalToGlobal((l+1, t+1))
        Wbase.SetCursor("arrow")
        self.menu.menu.EnableMenuItem(0)
        reply = self.menu.menu.PopUpMenuSelect(t, l, 1)
        if reply:
            id = reply >> 16
            item = reply & 0xffff
            self._menu_callback(id, item)


class FontMenu(PopupMenu):

    """A font popup menu."""

    menu = None

    def __init__(self, possize, callback):
        PopupMenu.__init__(self, possize)
        _makefontmenu()
        self._callback = callback
        self._enabled = 1

    def open(self):
        self._calcbounds()

    def close(self):
        del self._callback

    def set(self):
        raise Wbase.WidgetsError, "can't change font menu widget"

    def _menu_callback(self, id, item):
        fontname = self.menu.menu.GetMenuItemText(item)
        if self._callback:
            Wbase.CallbackCall(self._callback, 0, fontname)

    def click(self, point, modifiers):
        if not self._enabled:
            return
        _makefontmenu()
        return PopupMenu.click(self, point, modifiers)


def _makefontmenu():
    """helper for font menu"""
    if FontMenu.menu is not None:
        return
    import W
    FontMenu.menu = Wapplication.Menu(W.getapplication().menubar, 'Foo', -1)
    W.SetCursor('watch')
    for i in range(FontMenu.menu.menu.CountMenuItems(), 0, -1):
        FontMenu.menu.menu.DeleteMenuItem(i)
    FontMenu.menu.menu.AppendResMenu('FOND')


def _getfontlist():
    from Carbon import Res
    fontnames = []
    for i in range(1, Res.CountResources('FOND') + 1):
        r = Res.GetIndResource('FOND', i)
        fontnames.append(r.GetResInfo()[2])
    return fontnames
