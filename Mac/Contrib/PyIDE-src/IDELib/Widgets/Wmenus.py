import FrameWork
import Qd
import Wbase
from types import *
import WFrameWorkPatch

_arrowright = Qd.GetPicture(472)
_arrowdown = Qd.GetPicture(473)
	


class PopupWidget(Wbase.ClickableWidget):
	
	def __init__(self, possize, items = [], callback = None):
		Wbase.Widget.__init__(self, possize)
		self._items = items
		self._itemsdict = {}
		self._callback = callback
		self._enabled = 1
	
	def close(self):
		Wbase.Widget.close(self)
		self._items = None
		self._itemsdict = {}
	
	def draw(self, visRgn = None):
		if self._visible:
			Qd.FrameRect(self._bounds)
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
		reply = self.menu.menu.PopUpMenuSelect(t, l, 1)
		if reply:
			id = (reply & 0xffff0000) >> 16
			item = reply & 0xffff
			self._menu_callback(id, item)
		self._emptymenu()
	
	def set(self, items):
		self._items = items
	
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
				raise WidgetsError, "illegal itemlist for popup menu"
			
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
	
	def open(self):
		self._calcbounds()
		self.menu = WFrameWorkPatch.Menu(self._parentwindow.parent.menubar, 'Foo', -1)
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
		self.menu = WFrameWorkPatch.Menu(self._parentwindow.parent.menubar, 'Foo', -1)
		self._items = items
		self._additems(self._items, self.menu)
	
	def click(self, point, modifiers):
		if not self._enabled:
			return
		self.SetPort()
		l, t, r, b = self._bounds
		l, t = Qd.LocalToGlobal((l+1, t+1))
		Wbase.SetCursor("arrow")
		reply = self.menu.menu.PopUpMenuSelect(t, l, 1)
		if reply:
			id = (reply & 0xffff0000) >> 16
			item = reply & 0xffff
			self._menu_callback(id, item)


class FontMenu(PopupMenu):
	
	menu = None
	
	def __init__(self, possize, callback):
		PopupMenu.__init__(self, possize)
		makefontmenu()
		self._callback = callback
		self._enabled = 1
	
	def open(self):
		self._calcbounds()
	
	def close(self):
		pass
	
	def set(self):
		raise Wbase.WidgetsError, "can't change font menu widget"
	
	def _menu_callback(self, id, item):
		fontname = self.menu.menu.GetMenuItemText(item)
		if self._callback:
			Wbase.CallbackCall(self._callback, 0, fontname)

	def click(self, point, modifiers):
		if not self._enabled:
			return
		makefontmenu()
		return PopupMenu.click(self, point, modifiers)
	

def makefontmenu():
	if FontMenu.menu is not None:
		return
	import W
	FontMenu.menu = WFrameWorkPatch.Menu(W.getapplication().menubar, 'Foo', -1)
	W.SetCursor('watch')
	for i in range(FontMenu.menu.menu.CountMItems(), 0, -1):
		FontMenu.menu.menu.DeleteMenuItem(i)
	FontMenu.menu.menu.AppendResMenu('FOND')


def _getfontlist():
	import Res
	fontnames = []
	for i in range(1, Res.CountResources('FOND') + 1):
		r = Res.GetIndResource('FOND', i)
		fontnames.append(r.GetResInfo()[2])
	return fontnames
