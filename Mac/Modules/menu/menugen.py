# Generated from 'Moes:CW5 GOLD \304:Metrowerks C/C++ \304:Headers \304:Universal Headers 2.0a3 \304:Menus.h'

f = Function(short, 'GetMBarHeight',
)
functions.append(f)

f = Function(void, 'InitMenus',
)
functions.append(f)

f = Function(MenuRef, 'NewMenu',
    (short, 'menuID', InMode),
    (ConstStr255Param, 'menuTitle', InMode),
)
functions.append(f)

f = Function(MenuRef, 'GetMenu',
    (short, 'resourceID', InMode),
)
functions.append(f)

f = Method(void, 'DisposeMenu',
    (MenuRef, 'theMenu', InMode),
)
methods.append(f)

f = Method(void, 'AppendMenu',
    (MenuRef, 'menu', InMode),
    (ConstStr255Param, 'data', InMode),
)
methods.append(f)

f = Method(void, 'AppendResMenu',
    (MenuRef, 'theMenu', InMode),
    (ResType, 'theType', InMode),
)
methods.append(f)

f = Method(void, 'InsertResMenu',
    (MenuRef, 'theMenu', InMode),
    (ResType, 'theType', InMode),
    (short, 'afterItem', InMode),
)
methods.append(f)

f = Method(void, 'InsertMenu',
    (MenuRef, 'theMenu', InMode),
    (short, 'beforeID', InMode),
)
methods.append(f)

f = Function(void, 'DrawMenuBar',
)
functions.append(f)

f = Function(void, 'InvalMenuBar',
)
functions.append(f)

f = Function(void, 'DeleteMenu',
    (short, 'menuID', InMode),
)
functions.append(f)

f = Function(void, 'ClearMenuBar',
)
functions.append(f)

f = Function(Handle, 'GetNewMBar',
    (short, 'menuBarID', InMode),
)
functions.append(f)

f = Function(Handle, 'GetMenuBar',
)
functions.append(f)

f = Function(void, 'SetMenuBar',
    (Handle, 'menuList', InMode),
)
functions.append(f)

f = Method(void, 'InsertMenuItem',
    (MenuRef, 'theMenu', InMode),
    (ConstStr255Param, 'itemString', InMode),
    (short, 'afterItem', InMode),
)
methods.append(f)

f = Method(void, 'DeleteMenuItem',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
)
methods.append(f)

f = Function(long, 'MenuKey',
    (short, 'ch', InMode),
)
functions.append(f)

f = Function(void, 'HiliteMenu',
    (short, 'menuID', InMode),
)
functions.append(f)

f = Method(void, 'SetMenuItemText',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
    (ConstStr255Param, 'itemString', InMode),
)
methods.append(f)

f = Method(void, 'GetMenuItemText',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
    (Str255, 'itemString', OutMode),
)
methods.append(f)

f = Method(void, 'DisableItem',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
)
methods.append(f)

f = Method(void, 'EnableItem',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
)
methods.append(f)

f = Method(void, 'CheckItem',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
    (Boolean, 'checked', InMode),
)
methods.append(f)

f = Method(void, 'SetItemMark',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
    (short, 'markChar', InMode),
)
methods.append(f)

f = Method(void, 'GetItemMark',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
    (short, 'markChar', OutMode),
)
methods.append(f)

f = Method(void, 'SetItemIcon',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
    (short, 'iconIndex', InMode),
)
methods.append(f)

f = Method(void, 'GetItemIcon',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
    (short, 'iconIndex', OutMode),
)
methods.append(f)

f = Method(void, 'SetItemStyle',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
    (short, 'chStyle', InMode),
)
methods.append(f)

f = Method(void, 'GetItemStyle',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
    (unsigned_char, 'chStyle', OutMode),
)
methods.append(f)

f = Method(void, 'CalcMenuSize',
    (MenuRef, 'theMenu', InMode),
)
methods.append(f)

f = Method(short, 'CountMItems',
    (MenuRef, 'theMenu', InMode),
)
methods.append(f)

f = Function(MenuRef, 'GetMenuHandle',
    (short, 'menuID', InMode),
)
functions.append(f)

f = Function(void, 'FlashMenuBar',
    (short, 'menuID', InMode),
)
functions.append(f)

f = Function(void, 'SetMenuFlash',
    (short, 'count', InMode),
)
functions.append(f)

f = Function(long, 'MenuSelect',
    (Point, 'startPt', InMode),
)
functions.append(f)

f = Function(void, 'InitProcMenu',
    (short, 'resID', InMode),
)
functions.append(f)

f = Method(void, 'GetItemCmd',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
    (short, 'cmdChar', OutMode),
)
methods.append(f)

f = Method(void, 'SetItemCmd',
    (MenuRef, 'theMenu', InMode),
    (short, 'item', InMode),
    (short, 'cmdChar', InMode),
)
methods.append(f)

f = Method(long, 'PopUpMenuSelect',
    (MenuRef, 'menu', InMode),
    (short, 'top', InMode),
    (short, 'left', InMode),
    (short, 'popUpItem', InMode),
)
methods.append(f)

f = Function(long, 'MenuChoice',
)
functions.append(f)

f = Function(void, 'DeleteMCEntries',
    (short, 'menuID', InMode),
    (short, 'menuItem', InMode),
)
functions.append(f)

f = Method(void, 'InsertFontResMenu',
    (MenuRef, 'theMenu', InMode),
    (short, 'afterItem', InMode),
    (short, 'scriptFilter', InMode),
)
methods.append(f)

f = Method(void, 'InsertIntlResMenu',
    (MenuRef, 'theMenu', InMode),
    (ResType, 'theType', InMode),
    (short, 'afterItem', InMode),
    (short, 'scriptFilter', InMode),
)
methods.append(f)

f = Function(Boolean, 'SystemEdit',
    (short, 'editCmd', InMode),
)
functions.append(f)

f = Function(void, 'SystemMenu',
    (long, 'menuResult', InMode),
)
functions.append(f)

